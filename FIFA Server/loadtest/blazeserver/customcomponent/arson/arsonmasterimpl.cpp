/*************************************************************************************************/
/*!
    \file   arsonmasterimpl.cpp


    \attention
        (c) Electronic Arts Inc. 2007
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ArsonMasterImpl

    Arson Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "arsonmasterimpl.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/replication/replicatedmap.h"
#include "arson/tdf/arson.h"
#include "arson/tdf/arson_server.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/uuid.h"
#include "framework/database/dbscheduler.h"
#include "gamemanager/rpc/gamemanagermaster.h"
#include "gamemanager/tdf/gamemanager_server.h" // for CreateGameMasterRequest in ArsonMasterImpl::doCreateGameMaster

namespace Blaze
{
namespace Arson
{

// static
ArsonMaster* ArsonMaster::createImpl()
{
    return BLAZE_NEW_NAMED("ArsonMasterImpl") ArsonMasterImpl();
}

/*** Public Methods ******************************************************************************/
ArsonMasterImpl::ArsonMasterImpl() :
    mPokeSleepTimerId(INVALID_TIMER_ID)
{
}

ArsonMasterImpl::~ArsonMasterImpl()
{  
    if (mPokeSleepTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mPokeSleepTimerId);
}

bool ArsonMasterImpl::onValidateConfig(ArsonConfig& config, const ArsonConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    return true;
}

bool ArsonMasterImpl::onConfigure()
{
    static const char8_t TEST_DATA_SAVE_QUERY[] =
        "INSERT INTO `arson_test_data` (`id`, `data1`, `data2`, `data3`)"
        " VALUES (?,?,?,?)"
        " ON DUPLICATE KEY UPDATE `data1`=?, `data2`=?, `data3`=?";

    static const char8_t TEST_DATA_LOAD_SINGLE_ROW_QUERY[] =
        "SELECT `id`, `data1`, `data2`, `data3`"
        " FROM `arson_test_data`"
        " WHERE `id` = ?";

    static const char8_t TEST_DATA_LOAD_MULTI_ROW_QUERY[] =
        "SELECT `id`, `data1`, `data2`, `data3`"
        " FROM `arson_test_data`"
        " WHERE `id` >= ? AND `id` <= ?";


    // Configure prepared statements.
    uint32_t dbId = getDbId();
    mStatementId[TEST_DATA_SAVE] = gDbScheduler->registerPreparedStatement(dbId, "test_data_save", TEST_DATA_SAVE_QUERY);
    mStatementId[TEST_DATA_LOAD_SINGLE_ROW] = gDbScheduler->registerPreparedStatement(dbId, "test_data_load_single_row", TEST_DATA_LOAD_SINGLE_ROW_QUERY);
    mStatementId[TEST_DATA_LOAD_MULTI_ROW] = gDbScheduler->registerPreparedStatement(dbId, "test_data_load_multi_row", TEST_DATA_LOAD_MULTI_ROW_QUERY);

    return true;
}

bool ArsonMasterImpl::onReconfigure()
{
    return true;
}

GenerateMasterExceptionError::Error ArsonMasterImpl::processGenerateMasterException(const Message *message)
{
    int *p = nullptr;
    *p = 1;

    return GenerateMasterExceptionError::ERR_OK;
}

GenerateSlaveNotificationExceptionError::Error ArsonMasterImpl::processGenerateSlaveNotificationException(const Message *message)
{
    sendSlaveExceptionNotificationToSlaves();

    return GenerateSlaveNotificationExceptionError::ERR_OK;
}

SecondSlaveMasterNotificationError::Error ArsonMasterImpl::processSecondSlaveMasterNotification(const Message *message)
{
    sendVoidNotificationToSlaves();

    return SecondSlaveMasterNotificationError::ERR_OK;
}

CreateMapMasterError::Error ArsonMasterImpl::processCreateMapMaster(const ExceptionMapRequest &request, const Message *message)
{
    CollectionId collectionId(getId(), request.getCollectionId());

    if (mMapCollection.getMap(collectionId) != nullptr)
    {
        return CreateMapMasterError::ARSON_ERR_COLLECTION_ID_IN_USE;
    }

    ExceptionReplicationReason reason;
    reason.setReason(ExceptionReplicationReason::MAP_CREATED);
    createExceptionEntriesMap(collectionId, &reason);
    return CreateMapMasterError::ERR_OK;
}

DestroyMapMasterError::Error ArsonMasterImpl::processDestroyMapMaster(const ExceptionMapRequest &request, const Message *message)
{
    CollectionId collectionId(getId(), request.getCollectionId());
    ExceptionEntriesMap* map = getExceptionEntriesMap(collectionId);
    if (map == nullptr)
    {
        return DestroyMapMasterError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    ExceptionReplicationReason reason;
    reason.setReason(ExceptionReplicationReason::MAP_DESTROYED);
    destroyExceptionEntriesMap(map, &reason);
    return DestroyMapMasterError::ERR_OK;
}


MapEraseMasterError::Error ArsonMasterImpl::processMapEraseMaster(const ExceptionMapValueRequest &request, const Message *message)
{
    CollectionId collectionId(getId(), request.getCollectionId());
    ExceptionEntriesMap* map = getExceptionEntriesMap(collectionId);
    if (map == nullptr)
    {
        return MapEraseMasterError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    if (!map->exists(request.getObjectId()))
    {
        return MapEraseMasterError::ARSON_ERR_OBJECT_ID_NOT_FOUND;
    }

    ExceptionReplicationReason reason;
    reason.setReason(ExceptionReplicationReason::OBJECT_DESTROYED);
    map->erase(request.getObjectId(), &reason);
    return MapEraseMasterError::ERR_OK;
}

MapInsertMasterError::Error ArsonMasterImpl::processMapInsertMaster(const ExceptionMapUpdateRequest &request, const Message *message)
{
    CollectionId collectionId(getId(), request.getCollectionId());
    ExceptionEntriesMap* map = getExceptionEntriesMap(collectionId);
    if (map == nullptr)
    {
        return MapInsertMasterError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    if (map->exists(request.getObjectId()))
    {
        return MapInsertMasterError::ARSON_ERR_OBJECT_ID_IN_USE;
    }

    ExceptionReplicationReason reason;
    reason.setReason(ExceptionReplicationReason::OBJECT_CREATED);

    map->insert(request.getObjectId(), *request.getEntry().clone(), &reason);
    return MapInsertMasterError::ERR_OK;
}


MapUpdateMasterError::Error ArsonMasterImpl::processMapUpdateMaster(const ExceptionMapUpdateRequest &request, const Message *message)
{
    CollectionId collectionId(getId(), request.getCollectionId());
    ExceptionEntriesMap* map = getExceptionEntriesMap(collectionId);
    if (map == nullptr)
    {
        return MapUpdateMasterError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    ExceptionMapEntry *entry = map->findItem(request.getObjectId());
    if (entry != nullptr)
    {
        return MapUpdateMasterError::ARSON_ERR_OBJECT_ID_NOT_FOUND;
    }

    entry->setInt(request.getEntry().getInt());
    entry->setString(request.getEntry().getString());

    ExceptionReplicationReason reason;
    reason.setReason(ExceptionReplicationReason::OBJECT_UPDATED);
    map->update(request.getObjectId(), *entry, &reason);

    return MapUpdateMasterError::ERR_OK;
}
PokeMasterError::Error ArsonMasterImpl::processPokeMaster(const ArsonRequest &request, 
                                                 ArsonResponse &response, ArsonError &error, const Message *message)
{
    TRACE_LOG("[ArsonMasterImpl].processPokeMaster() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // Arson check for failure.
    if (request.getNum() >= 0)
    {
        TRACE_LOG("[ArsonMasterImpl].processPokeMaster() - Respond Success.");

        request.copyInto(mRequest);
        response.copyInto(mResponse);
        
        // If a sleep was requested
        if (request.getMasterSleepMs() > 0)
        {
            TRACE_LOG("[ArsonMasterImpl].processPokeMaster() - Going to wait for " << request.getMasterSleepMs() << " milli-seconds.");

            // Set up a job to wait for the timer
            if (mPokeSleepTimerId != INVALID_TIMER_ID)
            {
                gSelector->cancelTimer(mPokeSleepTimerId);
                mPokeSleepTimerId = INVALID_TIMER_ID;
            }

            TimeValue pokeSleepPeriod = TimeValue::getTimeOfDay() + request.getMasterSleepMs() * 1000; // Sleep period in micro-seconds
            mPokeSleepTimerId = gSelector->scheduleFiberTimerCall(pokeSleepPeriod, this, &ArsonMasterImpl::sendNotificationToSlaves, "ArsonMasterImpl::processPokeMaster");

            if (mPokeSleepTimerId == INVALID_TIMER_ID)
            {
                WARN_LOG("[ArsonMasterImpl].processPokeMaster(): Failed to scheduleFiberTimerCall at line:" << __LINE__);
            }
        }
        else
        {
            sendNotificationToSlaves();
        }

        response.setMessage("Master got poked.");
        return PokeMasterError::ERR_OK;
    }
    else
    {
        WARN_LOG("[ArsonMasterImpl].processPokeMaster() - Respond Failure");        
        error.setMessage("Master didn't get poked.");
        return PokeMasterError::ARSON_ERR_BAD_NUM;
    } // if
}

PokeMasterVoidError::Error ArsonMasterImpl::processPokeMasterVoid(const Message *message)
{
    // Arson check for failure.
    TRACE_LOG("[PokeMasterVoidCommand].start() - Respond Success.");
    
    //Lets spawn a notification - delay it so the response gets back first
    sendVoidMasterNotificationToSlaves();

    return PokeMasterVoidError::ERR_OK;
}

CreateGameMasterError::Error ArsonMasterImpl::processCreateGameMaster(const ArsonCreateGameRequest &request, 
                                                                      Blaze::GameManager::CreateGameResponse &response, const Message *message)
{
    Blaze::GameManager::CreateGameMasterResponse masterResponse;
    masterResponse.setCreateResponse(response);
    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &ArsonMasterImpl::doCreateGameMaster, Blaze::Arson::ArsonCreateGameRequestPtr(request.clone()), Blaze::GameManager::CreateGameMasterResponsePtr(masterResponse.clone()), "processCreateGameMaster::doCreateGameMaster", params);

    return CreateGameMasterError::ERR_OK;
}

void ArsonMasterImpl::doCreateGameMaster(Blaze::Arson::ArsonCreateGameRequestPtr request, Blaze::GameManager::CreateGameMasterResponsePtr response)
{
    GameManager::GameManagerMaster* gameManagerComponent = nullptr;
    gameManagerComponent = static_cast<GameManager::GameManagerMaster*>(gController->getComponent(GameManager::GameManagerMaster::COMPONENT_ID, false, true));

    if (gameManagerComponent == nullptr)
    {
        ERR_LOG("[doCreateGameMaster] GameManagerComponent not found '" << ErrorHelp::getErrorName(ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND) << "'.");
        return;
    }

    if(request->getNullCurrentUserSession())
    {
        // Set the gCurrentUserSession to nullptr
        UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
    }

    Blaze::GameManager::CreateGameRequest& createGameRequest = request->getCreateGameRequest();
    const char8_t* persistedGameId = createGameRequest.getPersistedGameId();
    
    // If persisted game id is enabled, but it's not set
    if( createGameRequest.getGameCreationData().getGameSettings().getEnablePersistedGameId() && ( persistedGameId == nullptr || strlen(persistedGameId) == 0 ) )
    {
        // Generate the persisted game id and secret
        UUID uuid;
        UUIDUtil::generateUUID(uuid);
        if (uuid.c_str()[0] == '\0')
        {
            TRACE_LOG("[doCreateGameMaster] unable to generate persisted game id");
        }

        createGameRequest.setPersistedGameId(uuid.c_str());
        UUIDUtil::generateSecretText(uuid, createGameRequest.getPersistedGameIdSecret());
    }

    {
        Blaze::GameManager::CreateGameMasterRequest masterReq;
        Blaze::GameManager::CreateGameMasterErrorInfo masterErr;
        UserSession::SuperUserPermissionAutoPtr permission(true);
        createGameRequest.copyInto(masterReq.getCreateRequest());
        RpcCallOptions opts;
        opts.routeTo.setSliverNamespace(GameManager::GameManagerMaster::COMPONENT_ID);
        BlazeRpcError err = gameManagerComponent->createGameMaster(masterReq, *response, masterErr, opts);
        TRACE_LOG("[doCreateGameMaster] returned error code '" << ErrorHelp::getErrorName(err) << "'.");
    }
}

SendTestNotificationToClientMasterError::Error ArsonMasterImpl::processSendTestNotificationToClientMaster(const Message* message)
{
    TestNotification notification;
    notification.setText("Test Send Notification from Master To Client With Passthrough");
    sendNotifyTestNotificationToUserSessionById(gCurrentUserSession->getSessionId(), &notification);
    return SendTestNotificationToClientMasterError::ERR_OK;
}

void ArsonMasterImpl::sendNotificationToSlaves()
{
    TRACE_LOG("[ArsonMasterImpl].sendNotificationToSlaves()");

    size_t blobSize = mRequest.getMasterReturnedBlobSize();
    if (blobSize > 0)
    {
        uint8_t* mem = BLAZE_NEW_ARRAY(uint8_t, blobSize);
        for (size_t counter = 0; counter < blobSize; ++counter)
        {
            mem[counter] = (uint8_t) Random::getRandomNumber(255);
        }                
        mResponse.getBlob().setData(mem, blobSize);

        delete [] mem;
    }

    // Let's spawn a notification - the notification will be the response
    sendResponseMasterNotificationToSlaves(&mResponse);
}

Query64BitIntMasterError::Error ArsonMasterImpl::processQuery64BitIntMaster(const Message* message)
{
    // Insert a 64bit integer into arson_test_data
    BlazeRpcError insertErr = insert_id_query();
    // Query the integer using getInt() instead of getInt64()
    BlazeRpcError fetchErr = fetch_id_query();

    if (ERR_OK == insertErr && ERR_OK == fetchErr)
        return Query64BitIntMasterError::ERR_OK;
    else
        return Query64BitIntMasterError::ERR_SYSTEM;
}

BlazeRpcError ArsonMasterImpl::insert_id_query()
{
    BlazeRpcError rc = Blaze::ERR_SYSTEM;

    // Query the DB
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            query->append("INSERT INTO arson_test_data VALUES ('123','datafield 1','datafield 2','datafield 3')");
            BlazeRpcError dbError = conn->executeQuery(query);

            if (dbError == DB_ERR_OK)
                rc = Blaze::ERR_OK;
            else
            {
                // query failed
                ERR_LOG("[ArsonMasterImpl].insert_id_query: Query failed");
            }
        }
        else
        {
            // failed to create query
            ERR_LOG("[ArsonMasterImpl].insert_id_query: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[ArsonMasterImpl].insert_id_query: Failed to obtain connection");
    }

    return rc;
}

BlazeRpcError ArsonMasterImpl::fetch_id_query()
{
    BlazeRpcError rc = Blaze::ERR_SYSTEM;

    // Query the DB
    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            query->append("SELECT * FROM arson_test_data");
            DbResultPtr dbResult;
            BlazeRpcError dbError = conn->executeQuery(query, dbResult);

            if (dbError == DB_ERR_OK)
            {
                rc = Blaze::ERR_OK;

                DbResult::const_iterator itr = dbResult->begin();
                DbResult::const_iterator end = dbResult->end();
                for (; itr != end; ++itr)
                {
                    const DbRow* row = *itr;
                    row->getInt("id");
                }
            }
            else
            {
                // query failed
                ERR_LOG("[Query64BitIntCommand].insert_id_query: Query failed");
            }
        }
        else
        {
            // failed to create query
            ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to obtain connection");
    }

    return rc;
}

} // Arson
} // Blaze
