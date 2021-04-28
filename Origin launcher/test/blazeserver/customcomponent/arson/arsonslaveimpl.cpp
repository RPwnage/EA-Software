/*************************************************************************************************/
/*!
    \file   arsonslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ArsonSlaveImpl

    Arson Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include <sstream>
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/tdf/controllertypes_server.h"
#include "arson/tdf/arson.h"
#include "framework/replication/replicator.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/database/dbscheduler.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/redis/redis.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"
#include "framework/connection/outboundhttpservice.h" // for retrieveExternalSession
#include "proxycomponent/xblserviceconfigs/rpc/xblserviceconfigsslave.h"// For XBLServiceConfigsSlave in retrieveExternalSession
#include "xblserviceconfigs/tdf/xblserviceconfigs.h"
#include "proxycomponent/xblclientsessiondirectory/rpc/xblclientsessiondirectoryslave.h"// For xblclientsessiondirectory in retrieveExternalSession
#include "xblclientsessiondirectory/tdf/xblclientsessiondirectory.h"
#include "arson/tournamentorganizer/proxycomponent/tdf/arsonxblsessiondirectory_arena.h"
#include "arson/tournamentorganizer/proxycomponent/rpc/arsonxblsessiondirectoryarena_defines.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/shared/externalsessionbinarydatashared.h" // for ExternalSessionBinaryDataHeader in retrieveExternalSessionCustomData
#include "gamemanager/shared/externalsessionjsondatashared.h"// for psnPlayerSessionCustomDataToBlazeData() in validatePs5PlayerSessionResponse()
#include "gamemanager/externalsessions/externaluserscommoninfo.h" // for externalUserIdentificationToString in getNpSession()
#include "gamemanager/tdf/externalsessiontypes_server.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h" // for MatchmakerConfig in getPsnExternalSessionParams
#include "gamemanager/gamemanagerslaveimpl.h"
#include "curlutil.h"
#include "EAIO/EAFileBase.h"
#include "EAIO/EAFileUtil.h"
#include "EAIO/PathString.h"
#include "EAStdC/EAString.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h" //for ExternalSessionUtilPs4
#include "component/gamemanager/externalsessions/externalsessionutilps5.h" //for ExternalSessionUtilPs5
#include "component/gamemanager/externalsessions/externalsessionutilps5playersessions.h" //for ExternalSessionUtilPs5playerSession
#include "psnsessioninvitation/rpc/psnsessioninvitationslave.h" // for PSNSessionInvitationSlave::CMD_INFO_GETNPSESSION in getNpSession()
#include "psnsessioninvitation/tdf/psnsessioninvitation.h" //for GetNpSessionResponse in retrieveExternalSession
#include "psnsessionmanager/tdf/psnsessionmanager.h"
#include "psnmatches/tdf/psnmatches.h"
#include "psnmatches/rpc/psnmatches_defines.h"
#include "psnsessionmanager/rpc/psnsessionmanager_defines.h"
#include "psnbaseurl/rpc/psnbaseurl_defines.h"
#include "framework/rpc/oauth_defines.h"


namespace Blaze
{
namespace Arson
{

/*** Public Methods ******************************************************************************/

void SecondArsonListener::onVoidNotification(UserSession*)
{
    TRACE_LOG("[SecondArsonListener].onVoidNotification");

    // TODO_MC: Arson is an example of a custom component that can/does run on an AUX_SLAVE.  These components should not
    //          be allowed to access gUserSessionMaster.  In fact, gUserSessionMaster should really go away.  Some consideration
    //          on how to allow coreSlave components to have access to the full UserSession object, while not allowing
    //          custom components access, needs to be had.
    if (gUserSessionMaster != nullptr)
    {
        const UserSessionMasterPtrByIdMap& map = gUserSessionMaster->getOwnedUserSessionsMap();
        for (UserSessionMasterPtrByIdMap::const_iterator it = map.begin(), end = map.end(); it != end; ++it)
        {
            sendNotifySecondSlaveToUserSession(it->second.get());
        }
    }
}

void SecondArsonListener::onSlaveExceptionNotification(UserSession*)
{
    TRACE_LOG("[SecondArsonListener].onSlaveExceptionNotification");
}

void SecondArsonListener::onResponseMasterNotification(const ArsonResponse& data, UserSession *)
{
    TRACE_LOG("[SecondArsonListener].onResponseMasterNotification");
}

void SecondArsonListener::onVoidMasterNotification(UserSession*)
{
    TRACE_LOG("[SecondArsonListener].onVoidMasterNotification");
}

MapGetError::Error SecondArsonListener::processMapGet(const ExceptionMapValueRequest &request, ExceptionMapEntry &response, const Message *message)
{
    ExceptionEntriesMap* map = (ExceptionEntriesMap *) gReplicator->getRemoteMap(
        CollectionId(RpcMakeMasterId(getId()), request.getCollectionId()));

    if (map == nullptr)
    {
        return MapGetError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    MapGetError::Error result = map->cloneItem(request.getObjectId(), response) ? MapGetError::ERR_OK : MapGetError::ARSON_ERR_OBJECT_ID_NOT_FOUND;
    return result;
}

// static
ArsonSlave* ArsonSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ArsonSlaveImpl") ArsonSlaveImpl();
}

ArsonSlaveImpl::ArsonSlaveImpl()
  : mExtendedData(),
    mEspnLiveFeedConnManager("espnLiveFeed"),
    mEspnLiveFeedConn(nullptr),
    mSecondArsonListener(new SecondArsonListener()),
    mBytevaultAddr(nullptr),
    mExternalTournamentService(nullptr),
    mPsnSessionManagerAddr(nullptr),
    mXblMpsdAddr(nullptr),
    mXblSocialAddr(nullptr),
    mXblHandlesAddr(nullptr)
{
    gUserSessionManager->registerExtendedDataProvider(COMPONENT_ID, *this);
}

ArsonSlaveImpl::~ArsonSlaveImpl()
{
    delete mSecondArsonListener;
    for(ExtendedDataMap::iterator i = mExtendedData.begin(), e = mExtendedData.end(); i != e; ++i)
    {
        delete i->second;
    }
    if (mBytevaultAddr != nullptr)
    {
        delete mBytevaultAddr;
        mBytevaultAddr = nullptr;
    }
    if(mHttpConnectionManagerPtr != nullptr)
    {
        mHttpConnectionManagerPtr.reset();
    }
    if (mPsnSessionManagerAddr != nullptr)
    {
        delete mPsnSessionManagerAddr;
    }
    if (mXblMpsdAddr != nullptr)
    {
        delete mXblMpsdAddr;
    }
    if (mXblSocialAddr != nullptr)
    {
        delete mXblSocialAddr;
    }
    if (mXblHandlesAddr != nullptr)
    {
        delete mXblHandlesAddr;
    }
    if (mPsnSessMgrConnectionManagerPtr != nullptr)
    {
        mPsnSessMgrConnectionManagerPtr.reset();
    }
    if(mXblMpsdConnectionManagerPtr != nullptr)
    {
        mXblMpsdConnectionManagerPtr.reset();
    }
    if(mXblSocialConnectionManagerPtr != nullptr)
    {
        mXblSocialConnectionManagerPtr.reset();
    }
    if(mXblHandlesConnectionManagerPtr != nullptr)
    {
        mXblHandlesConnectionManagerPtr.reset();
    }
    if (mExternalTournamentService != nullptr)
    {
        delete mExternalTournamentService;
    }
}

bool ArsonSlaveImpl::onValidateConfig(ArsonConfig& config, const ArsonConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (config.getIsInvalidConfig())
    {
        eastl::string msg;
        msg.sprintf("Invalid configuration option set", this);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    return validationErrors.getErrorMessages().empty();
}

bool ArsonSlaveImpl::onConfigure()
{
    const ArsonConfig &config = getConfig();
    ArsonConfig current_config = getConfig();
    SftpConfig &sftpConfig = current_config.getSftpConfig();
    sftpConfig.setFtpPassword("<CENSORED>");
    // actually log 'em
    INFO_LOG("[ArsonSlaveImpl:configure] \n" << current_config);

    const char8_t* bytevaultServerAddress = config.getBytevaultServer();
    if((bytevaultServerAddress[0] != '\0') && (mHttpConnectionManagerPtr == nullptr))
    {
        int32_t poolSize = 10;
        OutboundHttpConnectionManager* mOutboundHttpConnectionManager = nullptr;
        initializeConnManager(bytevaultServerAddress, &mBytevaultAddr, &mOutboundHttpConnectionManager, poolSize, "bytevault service");
        mHttpConnectionManagerPtr = HttpConnectionManagerPtr(mOutboundHttpConnectionManager);
    }

    // PS5 PlayerSessions
    const char8_t* psnSessionManagerAddress = "https://s2s.sp-int.playstation.net/api/sessionManager";
    if ((psnSessionManagerAddress[0] != '\0') && (mPsnSessMgrConnectionManagerPtr == nullptr))
    {
        int32_t poolSize = 10;
        OutboundHttpConnectionManager* psnSessionManagerConnectionManager = nullptr;
        initializeConnManager(psnSessionManagerAddress, &mPsnSessionManagerAddr, &psnSessionManagerConnectionManager, poolSize, "xbl users service");
        mPsnSessMgrConnectionManagerPtr = HttpConnectionManagerPtr(psnSessionManagerConnectionManager);
    }

    const char8_t* xblMpsdAddress = config.getXblMpsdHost();
    if((xblMpsdAddress[0] != '\0') && (mXblMpsdConnectionManagerPtr == nullptr))
    {
        int32_t poolSize = 10;
        OutboundHttpConnectionManager* xblMpsdConnectionManager = nullptr;
        initializeConnManager(xblMpsdAddress, &mXblMpsdAddr, &xblMpsdConnectionManager, poolSize, "xbl mpsd service");
        mXblMpsdConnectionManagerPtr = HttpConnectionManagerPtr(xblMpsdConnectionManager);
    }

    // social users for xbox
    const char8_t* xblSocialAddress = "https://social.xboxlive.com";
    if((xblSocialAddress[0] != '\0') && (mXblSocialConnectionManagerPtr == nullptr))
    {
        int32_t poolSize = 10;
        OutboundHttpConnectionManager* xblUsersConnectionManager = nullptr;
        initializeConnManager(xblSocialAddress, &mXblSocialAddr, &xblUsersConnectionManager, poolSize, "xbl users service");
        mXblSocialConnectionManagerPtr = HttpConnectionManagerPtr(xblUsersConnectionManager);
    }

    // handles for xbox
    const char8_t* xblHandlesAddress = "https://sessiondirectory.xboxlive.com";
    if((xblHandlesAddress[0] != '\0') && (mXblHandlesConnectionManagerPtr == nullptr))
    {
        int32_t poolSize = 10;
        OutboundHttpConnectionManager* xblHandlesConnectionManager = nullptr;
        initializeConnManager(xblHandlesAddress, &mXblHandlesAddr, &xblHandlesConnectionManager, poolSize, "xbl handles service");
        mXblHandlesConnectionManagerPtr = HttpConnectionManagerPtr(xblHandlesConnectionManager);
    }

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

    // GOS-12290 - Config preprocessor silently truncates 64 bit value #defines
    // this assert will fail in Blaze 3.19.04 because the config preprocessor
    // will truncate 1000000000000 by silently converting it to an int!
    const SomeLargeValueConfigSection & someLargeValueConfigSection = config.getSomeLargeValueConfigSection();
    if (someLargeValueConfigSection.getValue() != 1000000000000LL) 
    {
        FAIL_LOG("[ArsonSlaveImpl].onConfigure: large value define someLargeValueConfigSection.value " << someLargeValueConfigSection.getValue() << " != 1000000000000");
        return false; 
    }

    //load some extended data into the session
    const ArsonDataList &extendedDataList = config.getExtendedData();

    Blaze::UserExtendedDataIdMap uedIdMap;
    ArsonDataList::const_iterator itor = extendedDataList.begin();
    ArsonDataList::const_iterator itorEnd = extendedDataList.end();

    for (;itor != itorEnd; ++itor)
    {
        ExtendedData* exData = BLAZE_NEW ExtendedData();

        const char8_t* name = (*itor)->getName();
        exData->key = (*itor)->getKey();
        exData->value = (*itor)->getValue();
        mExtendedData.insert(eastl::make_pair(name, exData));
        UserSessionManager::addUserExtendedDataId(uedIdMap, name, ArsonSlave::COMPONENT_ID, exData->key);

    }

    //RedisConfig redisConnConfig;
    //redisConnConfig.setHostAndPort(config.getRedisHostAndPort());
    //redisConnConfig.setMaxConnCount(config.getRedisConnPoolSize());
    //mRedis.configure(redisConnConfig);
    //mRedis.activate();

    // Update the User Session Extended Ids so other components can refer to the stats
    // stats puts in the User Extended Data by name instead of by key.
    BlazeRpcError rc = gUserSessionManager->updateUserSessionExtendedIds(uedIdMap);

    InetAddress inetAddress("realtime.caster.espn.go.com:80");
    mEspnLiveFeedConnManager.initialize(inetAddress, 1);

    // Get the Game Manager config from gController instead of GameManagerSlaveImpl, otherwise it will crash when Arson is on a different Aux Slave
    Blaze::GameManager::GameManagerServerConfig gameManagerServerConfig;
    rc = gController->getConfigTdf("gamemanager", false, gameManagerServerConfig);
    if (rc != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].onConfigure: Failure fetching config for game manager, error=" << ErrorHelp::getErrorName(rc));
    }
    else
    {
        bool success = GameManager::ExternalSessionUtil::createExternalSessionServices(mGmExternalSessionServices, gameManagerServerConfig.getGameSession().getExternalSessions(),
            gameManagerServerConfig.getGameSession().getPlayerReservationTimeout(), UINT16_MAX, &gameManagerServerConfig.getGameSession().getTeamNameByTeamIdMap(), nullptr);
        if (!success)
        {
            return false;
        }
    }

    Blaze::Arson::ArsonTournamentOrganizerConfig toConfig;
    rc = gController->getConfigTdf("arsontournamentorganizer", false, toConfig);
    if (rc != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].onConfigure: Failure fetching config for arsontournamentorganizer, error=" << ErrorHelp::getErrorName(rc));
    }
    else
    {
        mExternalTournamentService = Arson::ArsonExternalTournamentUtil::createExternalTournamentService(toConfig);
        if (mExternalTournamentService == nullptr)
        {
            return false;
        }
    }

    return true;
}

bool ArsonSlaveImpl::testDecoderWithUnionUpdates(const eastl::string& testName, TdfEncoder* encoder, TdfDecoder* decoder, bool testDecodeOnlyChanges, bool actuallyMakeChanges) const
{    
    ERR_LOG(testName.c_str() << ": QA to rework this test since XboxAddr is gone.");
    return false;
}


bool ArsonSlaveImpl::onResolve()
{
    getMaster()->addNotificationListener(*this);
    getMaster()->addNotificationListener(*mSecondArsonListener);
    gController->getMaster()->addNotificationListener(*this);
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);

    return true;
}

bool ArsonSlaveImpl::onConfigureReplication()
{
    gReplicator->subscribeToDynamicMapFactory<ArsonSlaveStub::ExceptionEntriesMap>(ArsonSlaveStub::EXCEPTIONENTRIES_MAP_ID_RANGE, *this);

    return true;
}

void ArsonSlaveImpl::onShutdown()
{
    gUserSessionManager->deregisterExtendedDataProvider(COMPONENT_ID);
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);
}

// Replication notifications
void ArsonSlaveImpl::onCreateMap(ExceptionEntriesMap& replMap, const ExceptionReplicationReason *reason)
{
    TRACE_LOG("[ArsonSlaveImpl].onCreateMap: " << replMap.getCollectionId().getComponentId() 
              << "/" << replMap.getCollectionId().getCollectionNum() << " : " << &replMap);

#ifdef EA_PLATFORM_WINDOWS
    Sleep(rand() % 10 + 1);
#else
    sleep(rand() % 10 + 1);
#endif

    int *p = nullptr;
    *p = 1;

    replMap.addSubscriber(*this);
}

void ArsonSlaveImpl::onDestroyMap(ExceptionEntriesMap& replMap, const ExceptionReplicationReason *reason)
{
    TRACE_LOG("[ArsonSlaveImpl].onDestroyMap: " << replMap.getCollectionId().getComponentId() 
             <<"/" <<  replMap.getCollectionId().getCollectionNum() <<" : " << &replMap);
}

void ArsonSlaveImpl::onInsert(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason)
{
    TRACE_LOG("[ArsonSlaveImpl].onInsert: " << replMap.getCollectionId().getComponentId() 
              << "/" << replMap.getCollectionId().getCollectionNum() << " objid=" << objectId);
}

void ArsonSlaveImpl::onUpdate(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason)
{
    TRACE_LOG("[ArsonSlaveImpl].onUpdate: " << replMap.getCollectionId().getComponentId() 
              << "/" << replMap.getCollectionId().getCollectionNum() <<" objid=" << objectId);
}

void ArsonSlaveImpl::onErase(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason)
{
    TRACE_LOG("[ArsonSlaveImpl].onErase: " << replMap.getCollectionId().getComponentId() 
              << "/" << replMap.getCollectionId().getCollectionNum() << " objid=" << objectId );
}

MapGetError::Error ArsonSlaveImpl::processMapGet(const ExceptionMapValueRequest &request, ExceptionMapEntry &response, const Message *message)
{
    ExceptionEntriesMap* map = (ExceptionEntriesMap *) gReplicator->getRemoteMap(
        CollectionId(RpcMakeMasterId(getId()), request.getCollectionId()));

    if (map == nullptr)
    {
        return MapGetError::ARSON_ERR_COLLECTION_ID_NOT_FOUND;
    }

    MapGetError::Error result = map->cloneItem(request.getObjectId(), response) ? MapGetError::ERR_OK : MapGetError::ARSON_ERR_OBJECT_ID_NOT_FOUND;
    return result;
}

// Master async notifications
void ArsonSlaveImpl::onVoidNotification(UserSession *associatedUserSession)
{
    TRACE_LOG("[ArsonSlaveImpl].onVoidNotification");
}

void ArsonSlaveImpl::onSlaveExceptionNotification(UserSession *associatedUserSession)
{
    // TODO: This no longer makes sense, associatedUserSession would have always been nullptr, and always would have early'd out.
    //if ((associatedUserSession ==  nullptr) || (!associatedUserSession->isLocal()))
    //{
    //    // to avoid immediately overrunning exceptions.cfg's 'delayPerFiberException', only one slave handles the exception
    //    return;
    //}
    //TRACE_LOG("[ArsonSlaveImpl].onSlaveExceptionNotification");
    //int *p = nullptr;
    //*p = 1;

    //return;
}

BlazeRpcError ArsonSlaveImpl::onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData)
{
    Blaze::EntityType dummyArsonEntityType = 1;
    Blaze::EntityId dummyArsonEntityId = 54321;

    UserExtendedDataMap& userMap = extendedData.getDataMap();
    ExtendedDataMap::iterator itr = mExtendedData.begin();
    ExtendedDataMap::iterator end = mExtendedData.end();
    for (; itr != end; ++itr)
    {
        ExtendedData* exData = itr->second;
        uint32_t key = UED_KEY_FROM_IDS(ArsonSlave::COMPONENT_ID,exData->key);
        (userMap)[key] = exData->value;
    }

    EA::TDF::ObjectId objId = EA::TDF::ObjectId(EA::TDF::ObjectType(ArsonSlave::COMPONENT_ID, dummyArsonEntityType), dummyArsonEntityId);
    extendedData.getBlazeObjectIdList().push_back(objId);

    return ERR_OK;
}

// NOTE: this function doesn't work on the master as there is no data provider.
// TODO: modify Session::getDataValue so that a lookup on the master does the same as lookupExtended Data on the slave.
bool ArsonSlaveImpl::lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const
{
//     ExtendedDataMap::iterator findItr = mExtendedData.find(key);
//     if (findItr != mExtendedData.end())
//     {
//          session->getDataValue(getId(), findItr->second->key, value);
//          return true;
//     }
//     return false;

    //stats will be looked up by id
    int16_t mapKey = (int16_t)EA::StdC::AtoI32(key);
    return session.getDataValue(COMPONENT_ID, mapKey, value);
}

void ArsonSlaveImpl::onNotifyPrepareForReconfigureComplete(const Blaze::PrepareForReconfigureCompleteNotification& data,UserSession *associatedUserSession)
{
    if (gUserSessionMaster != nullptr)
    {
        const UserSessionMasterPtrByIdMap& map = gUserSessionMaster->getOwnedUserSessionsMap();
        for (UserSessionMasterPtrByIdMap::const_iterator it = map.begin(), end = map.end(); it != end; ++it)
        {
            sendNotifyValidationCompletedToUserSession(it->second.get());
        }

        INFO_LOG("[ArsonSlaveImpl].onNotifyPrepareForReconfigureComplete(): Sent NotifyPrepareForReconfigureCompleted notification to everybody on this slave")
    }

}

void ArsonSlaveImpl::onNotifyReconfigure(const Blaze::ReconfigureNotification& data,UserSession *associatedUserSession)
{
    if (data.getFinal() && (gUserSessionMaster != nullptr))
    {
        const UserSessionMasterPtrByIdMap& map = gUserSessionMaster->getOwnedUserSessionsMap();
        for (UserSessionMasterPtrByIdMap::const_iterator it = map.begin(), end = map.end(); it != end; ++it)
        {
            sendNotifyReconfigureCompletedToUserSession(it->second.get());
        }

        INFO_LOG("[ArsonSlaveImpl].onControllerStateChange(): Sent NotifyReconfigureCompleted notification to everybody on this slave")
    }
}

void ArsonSlaveImpl::getStatusInfo(Blaze::ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();
    map["foo"] = "bar";
}

struct UserPassthrough
{
    UserSessionId operator()(const UserSessionMaster *u) { return u->getUserSessionId(); }
};

void ArsonSlaveImpl::onResponseMasterNotification(const ArsonResponse& data, UserSession *)
{
    TRACE_LOG("[ArsonSlaveImpl].onResponse");

    sendResponseNotificationToUserSessionsById(mUsers.begin(), mUsers.end(), UserPassthrough(), &data);
}

void ArsonSlaveImpl::onVoidMasterNotification(UserSession*)
{
    TRACE_LOG("[ArsonSlaveImpl].onVoid");
}

void ArsonSlaveImpl::updateSessionSubscription(UserSessionMaster *mySession, UserSessionId sessionId, bool isSubscribed)
{
    if (isSubscribed)
    {
        mySession->addUser(gUserSessionManager->getBlazeId(sessionId));
        mySession->addNotifier(sessionId);
    }
    else
    {
        mySession->removeNotifier(sessionId);
        mySession->removeUser(gUserSessionManager->getBlazeId(sessionId));
    }
}

void ArsonSlaveImpl::updateUserSubscription(UserSessionMaster *mySession, BlazeId blazeId, bool isSubscribed)
{
    if (isSubscribed)
        mySession->addUser(blazeId);
    else
        mySession->removeUser(blazeId);
}

void ArsonSlaveImpl::updateUserPresenceInSession(UserSessionMaster *mySession, BlazeId blazeId, bool isSubscribed)
{
    if (isSubscribed)
        mySession->addUser(blazeId);
    else
        mySession->removeUser(blazeId);
}

ShutdownError::Error ArsonSlaveImpl::processShutdown(const Blaze::Arson::ShutdownRequest &request, const Message* message)
{
    // translate the Arson REQ to Blaze REQ
    Blaze::ShutdownRequest blazeREQ;
    blazeREQ.setReason(static_cast<Blaze::ShutdownReason>(request.getReason()));
    blazeREQ.setTarget(static_cast<Blaze::ShutdownTarget>(request.getTarget()));
    for (Arson::ShutdownRequest::InstanceIdList::const_iterator i = request.getInstanceIds().begin(), e = request.getInstanceIds().end(); i != e; ++i)
    {
        blazeREQ.getInstanceIds().push_back((InstanceId)*i);
    }
    blazeREQ.setRestart(request.getRestart());

    // make the call
    return ShutdownError::commandErrorFromBlazeError(gController->shutdown(blazeREQ));
}

void ArsonSlaveImpl::initializeConnManager(const char8_t* serverAddress, InetAddress** inetAddress, 
    OutboundHttpConnectionManager** connManager, int32_t poolSize, const char8_t* serverDesc) const
{
    const char8_t* serviceHostname = nullptr;
    bool serviceSecure = false;
    HttpProtocolUtil::getHostnameFromConfig(serverAddress, serviceHostname, serviceSecure);

    if (*inetAddress != nullptr)
        delete *inetAddress;
    *inetAddress = BLAZE_NEW InetAddress(serviceHostname);

    if (*connManager != nullptr) 
        delete *connManager;
    *connManager = BLAZE_NEW OutboundHttpConnectionManager(serverAddress);
    (*connManager)->initialize(**inetAddress, poolSize, serviceSecure);

    TRACE_LOG("[ArsonSlaveImpl].initializeConnManager: Setting " << serverDesc << " server connection to: " << serverAddress << "");
    TRACE_LOG("[ArsonSlaveImpl].initializeConnManager: Setting " << serverDesc << " server connection pool size to: " << poolSize << "");
    TRACE_LOG("[ArsonSlaveImpl].initializeConnManager: Setting " << serverDesc << " server connection SSL enabled: " << serviceSecure << "");

    return;
}


BlazeRpcError ArsonSlaveImpl::sendHttpRequest(ComponentId componentId, CommandId commandId, HttpStatusCode* statusCode, const EA::TDF::Tdf* requestTdf, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf)
{
    if(mHttpConnectionManagerPtr == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].sendHttpRequest: HttpConnectionManagerPtr is nullptr.");
        return ERR_SYSTEM;
    }

    HttpHeaderMap headerMap;

    RestProtocolUtil::sendHttpRequest(mHttpConnectionManagerPtr, componentId, commandId, requestTdf, JSON_CONTENTTYPE, true, responseTdf, errorTdf, &headerMap, "/1.0", statusCode);

    // Extract Blaze error
    const char8_t* blazeError = HttpProtocolUtil::getHeaderValue(headerMap, RestProtocolUtil::HEADER_BLAZE_ERRORCODE);

    if(blazeError == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].sendHttpRequest: BlazeRpcError extracted from headerMap is nullptr. Cannot Get Blaze Error Code.");
        return ERR_SYSTEM;
    }

    return EA::StdC::AtoU32(blazeError);
}

BlazeRpcError ArsonSlaveImpl::sendHttpRequest(ComponentId componentId, CommandId commandId, const EA::TDF::Tdf* requestTdf, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf)
{
    HttpStatusCode statusCode = 0;
    return sendHttpRequest(componentId, commandId, &statusCode, requestTdf, responseTdf, errorTdf);
}

BlazeRpcError ArsonSlaveImpl::sendHttpRequest(ComponentId componentId, CommandId commandId, HttpStatusCode* statusCode, const EA::TDF::Tdf* requestTdf, const char8_t* contentType, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf)
{
    if(mHttpConnectionManagerPtr == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].sendHttpRequest: HttpConnectionManagerPtr is nullptr.");
        return ERR_SYSTEM;
    }

    HttpHeaderMap headerMap;

    const RestResourceInfo* restInfo = BlazeRpcComponentDb::getRestResourceInfo(componentId, commandId);

    // encode payload
    RawBuffer payload(2048);
    // if content type is null, use default Encoder::INVALID encoder type for upsertRecord non-json payload
    RestProtocolUtil::encodePayload(restInfo, HttpXmlProtocol::getEncoderTypeFromMIMEType(contentType), requestTdf, payload);
    RestProtocolUtil::sendHttpRequest(mHttpConnectionManagerPtr, restInfo, requestTdf, contentType, responseTdf, errorTdf, &payload, &headerMap, "/1.0", statusCode);

    // Extract Blaze error
    const char8_t* blazeError = HttpProtocolUtil::getHeaderValue(headerMap, RestProtocolUtil::HEADER_BLAZE_ERRORCODE);

    if(blazeError == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].sendHttpRequest: BlazeRpcError extracted from headerMap is nullptr. Cannot Get Blaze Error Code.");
        return ERR_SYSTEM;
    }

    return EA::StdC::AtoU32(blazeError);
}

class EspnLiveFeedResponseHandler : public OutboundHttpResponseHandler
{
public:
    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override
    {
        TRACE_LOG("[EspnLiveFeedResponseHandler].setHttpError(): Request resulted in BlazeRpcError (" << ErrorHelp::getErrorName(err) << ")");
    }
protected:
    void processHeaders() override
    {
        TRACE_LOG("[EspnLiveFeedResponseHandler].processHeaders(): HTTP status code is (" << getHttpStatusCode() << ")");

        HttpHeaderMap::const_iterator it = getHeaderMap().begin();
        HttpHeaderMap::const_iterator end = getHeaderMap().end();
        for ( ; it != end; ++it)
        {
            TRACE_LOG("[EspnLiveFeedResponseHandler].processHeaders(): HTTP Header [" << it->first.c_str() << ": " << it->second.c_str() << "]");
        }
    }

    uint32_t processSendData(char8_t* data, uint32_t size) override
    {
        return 0;
    }

    uint32_t processRecvData(const char8_t* data, uint32_t size, bool isResultInCache = false) override
    {
        // data isn't nullptr terminated, so just get it into a string so we can safely print it.
        eastl::string dataStr(data, size);
        TRACE_LOG("[EspnLiveFeedResponseHandler].processData(): Received " << size << " bytes of data >> " << dataStr.c_str() << " <<");
        return size;
    }
    BlazeRpcError requestComplete(CURLcode res) override
    {
        TRACE_LOG("[EspnLiveFeedResponseHandler].requestComplete(): Request finished with CURLcode (" << res << ")");
        return ERR_OK;
    }
};

void ArsonSlaveImpl::espnLiveFeedFiber()
{
    TRACE_LOG("[ArsonSlaveImpl].espnLiveFeedFiber(): Starting ESPN Live Feed request fiber");

    if (mEspnLiveFeedConn != nullptr)
    {
        EspnLiveFeedResponseHandler responseHandler;
        BlazeRpcError err = mEspnLiveFeedConn->sendStreamRequest(HttpProtocolUtil::HTTP_GET, "/bottomline/EA", nullptr, 0, &responseHandler, nullptr, 0);
        if (err != ERR_OK)
            TRACE_LOG("[ArsonSlaveImpl].espnLiveFeedFiber(): mEspnLiveFeedConn->sendStreamRequest() failed with (" << ErrorHelp::getErrorName(err) << ")");
    }

    TRACE_LOG("[ArsonSlaveImpl].espnLiveFeedFiber(): Stoping ESPN Live Feed request fiber");
}

// For XONE or XBSX:
// Return the contract version and external token for external session URL
Blaze::BlazeRpcError ArsonSlaveImpl::getXblExternalSessionParams(EA::TDF::ObjectType entityType,
    Blaze::EntityId gameMmSessId, const UserIdentification& ident,
    Blaze::ExternalUserAuthInfo& authInfo, Blaze::XblContractVersion& contractVersion)
{
    eastl::string extToken;
    const char8_t* currentUserServiceName = gCurrentUserSession ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
    if (getExternalSessionServices().getExternalSessionUtilMap().empty())
    {
        ERR_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: ARSON's local ExternalSessionService doesn't exist, probably because it wasn't loaded from the config file correctly. (Preconfig must run on the component)");
        return ERR_SYSTEM;
    }

    // set parameters for external session
    if ((entityType == Blaze::GameManager::ENTITY_TYPE_GAME) || (entityType == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP))
    {
        // Get the Game Manager config from gController instead of GameManagerSlaveImpl, otherwise it will crash when Arson is on a different Aux Slave
        Blaze::GameManager::GameManagerServerConfig gameManagerServerConfig;
        BlazeRpcError rc = gController->getConfigTdf("gamemanager", false, gameManagerServerConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Failure fetching config for game manager, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }
        else
        {
            contractVersion = (getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform()) ? getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getConfig().getContractVersion() : "");
        }
        
        Blaze::GameManager::ExternalSessionUtil* extService = getExternalSessionServices().getUtil(ident.getPlatformInfo().getClientPlatform());
        if (extService == nullptr)
        {
            WARN_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Missing information for the ident provided platform: " << ident.getPlatformInfo().getClientPlatform());
            return ERR_SYSTEM;
        }
        

        Blaze::BlazeRpcError error = extService->getAuthToken(ident, currentUserServiceName, extToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Could not retrieve external session auth token. Cannot attempt to retrieve external session; error " << ErrorHelp::getErrorName(error));
            return error;
        }
    }
    else
    {
        // must be matchmaking session
        Blaze::Matchmaker::MatchmakerConfig mmConfig;
        BlazeRpcError rc = gController->getConfigTdf("matchmaker", false, mmConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Failure fetching config for matchmaker, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }
        else
        {
            contractVersion = (getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform()) ? getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getConfig().getContractVersion() : "");
        }

        Blaze::GameManager::ExternalSessionUtil* extService = getExternalSessionServices().getUtil(ident.getPlatformInfo().getClientPlatform());
        if (extService == nullptr)
        {
            WARN_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Missing information for the ident provided platform: " << ident.getPlatformInfo().getClientPlatform());
            return ERR_SYSTEM;
        }

        Blaze::BlazeRpcError error = extService->getAuthToken(ident, currentUserServiceName, extToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[ArsonSlaveImpl].getXblExternalSessionParams: Could not retrieve external session auth token. Cannot attempt to retrieve external session; error " << ErrorHelp::getErrorName(error));
            return error;
        }
        authInfo.setCachedExternalSessionToken(extToken.c_str());
        authInfo.setServiceName(currentUserServiceName);
    }
    return ERR_OK;
}

// Return the external session identification info and external token. This method does NOT call PSN, but fetches the
// info from Blaze. Note: on PS4 only games can have external sessions (this method isn't used for MM external sessions).
Blaze::BlazeRpcError ArsonSlaveImpl::getPsnExternalSessionParams(Blaze::ExternalUserAuthInfo& authInfo, Blaze::GameManager::GetExternalSessionInfoMasterResponse& externalSessionInfo,
    Blaze::GameManager::GameId gameId, const Blaze::UserIdentification& ident, ExternalSessionActivityType activityType)
{
    Blaze::ExternalSessionActivityTypeInfo extSessTypeInfo;
    extSessTypeInfo.setPlatform(ident.getPlatformInfo().getClientPlatform());
    extSessTypeInfo.setType(activityType);

    Blaze::GameManager::ExternalSessionUtil* extSessUtil = getExternalSessionService(extSessTypeInfo.getPlatform());
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    const char8_t* currentUserServiceName = gCurrentUserSession ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
    eastl::string extToken;
    Blaze::BlazeRpcError error = extSessUtil->getAuthToken(ident, currentUserServiceName, extToken);
    if (error != Blaze::ERR_OK)
    {
        WARN_LOG("[ArsonSlaveImpl].getPsnExternalSessionParams: (Arson) Could not retrieve external session auth token. Cannot attempt to retrieve external session; error " << ErrorHelp::getErrorName(error));
        return error;
    }
    if (extToken.empty())
    {
        ERR_LOG("[ArsonSlaveImpl].getPsnExternalSessionParams: (Arson) Could not retrieve external session auth token, token was empty (was authentication info correct or external sessions enabled?). error " << ErrorHelp::getErrorName(error));
        return error;
    }
    authInfo.setCachedExternalSessionToken(extToken.c_str());
    authInfo.setServiceName(currentUserServiceName);

    // get the session identification stored on the game master
    if (gameId != Blaze::GameManager::INVALID_GAME_ID)
    {
        Blaze::GameManager::GameManagerMaster* gameManagerComponent = static_cast<Blaze::GameManager::GameManagerMaster*>(gController->getComponent(Blaze::GameManager::GameManagerMaster::COMPONENT_ID, false, true, &error));
        if (gameManagerComponent == nullptr)
        {
            return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
        }
        eastl::string logBuf;
        TRACE_LOG("[ArsonSlaveImpl].getPsnExternalSessionParams: (Arson) Retrieving 1P session data from game manager master. Caller(" << GameManager::externalUserIdentificationToString(ident, logBuf) << ").");

        Blaze::GameManager::GetExternalSessionInfoMasterRequest req;
        req.setGameId(gameId);

        error = gameManagerComponent->getExternalSessionInfoMaster(req, externalSessionInfo);//fill externalSessionInfo

        // The below unit like checks maybe candidates for removal, but slotted here since closer to the server and easy to debug:
        // Boiler plate check: Blaze game should say it has an NP session, if and only if it also says theres externalSession members for it (See DevNet ticket 58807).
        if ((error == ERR_OK) &&
            ((extSessTypeInfo.getPlatform() == ps4 && (activityType == EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY) && !static_cast<Blaze::GameManager::ExternalSessionUtilPs4*>(extSessUtil)->isMultipleMembershipSupported()) ||
            (extSessTypeInfo.getPlatform() == ps5 && (activityType == EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY))))
        {
            auto* trackedMemberList = GameManager::getListFromMap(externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), extSessTypeInfo);
            size_t numExtSessMembers = (trackedMemberList ? trackedMemberList->size() : 0);
            if ((numExtSessMembers != 0) && !GameManager::isExtSessIdentSet(externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification(), extSessTypeInfo))
            {
                ERR_LOG("[ArsonSlaveImpl].getPsnExternalSessionParams: (Arson) Boiler plate check failed: Unexpected internal Blaze state: Blaze game is tracking " << numExtSessMembers << " members being in its external session, but it has no 1st party session id, for activityType(" << ExternalSessionActivityTypeToString(activityType) << "). Caller(" << GameManager::externalUserIdentificationToString(ident, logBuf) << ").");
                return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
            }
        }
    }

    return error;
}

// wraps and does the http REST call to get the user's primary external session.
// \param[out] errorInfo Stores PSN call rate limit info, plus additional error info on error. Note rate limit info may be filled, even if there's no error.
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveCurrentPrimarySessionPs4(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo,
    Blaze::ExternalSessionIdentification& currentPrimarySession, ExternalSessionErrorInfo& errorInfo)
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].retrieveCurrentPrimarySessionPs4: call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }

    // Crossgen PS4 uses PS5 PlayerSessions instead:
    if (isTestingPs4Xgen())
    {
        Blaze::Arson::ArsonPsnPlayerSession arsonPses;
        Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5 custDat1;
        Blaze::ExternalSessionCustomData custData2;
        BlazeRpcError err = retrieveCurrentPrimarySessionPs5(ident, authInfo, currentPrimarySession, &arsonPses, custDat1, custData2, errorInfo);
        return convertExternalServiceErrorToArsonError(err);
    }
    //else non crossgen NpSessions

    // validates tested platform, etc correct:
    Blaze::GameManager::ExternalSessionUtilPs4* extSessUtil = getExternalSessionServicePs4();
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    Blaze::PSNServices::NpSessionIdContainer result;
    BlazeRpcError err = extSessUtil->getPrimaryNpsId(ident, authInfo, result, &errorInfo);
    currentPrimarySession.getPs4().setNpSessionId(result.getSessionId());
    return convertExternalServiceErrorToArsonError(err);
}

// wraps and does the http REST call to get the external session data, for PS4 platform.
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalSessionSessionData(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo,
    const Blaze::ExternalSessionIdentification& sessionIdentification, Blaze::ExternalSessionCustomData& arsonResponse, ExternalSessionErrorInfo& errorInfo) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].retrieveExternalSessionChangeableSessionData: (Arson) call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    Blaze::GameManager::ExternalSessionUtilPs4* extSessUtilPs4 = getExternalSessionServicePs4();
    if (extSessUtilPs4 == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    
    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSessionSessionData: (Arson) Retrieving NP session(" << sessionIdentification.getPs4().getNpSessionId() << ")'s session-data from PSN. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
    Blaze::GameManager::ExternalSessionBinaryDataHead resultHead;
    Blaze::ExternalSessionCustomData resultTitle;
    BlazeRpcError sesErr = extSessUtilPs4->getNpSessionData(ident, authInfo, sessionIdentification.getPs4().getNpSessionId(), resultHead, &resultTitle, &errorInfo);
    if (sesErr == ERR_OK)
    {
        //repack into raw blob, to allow client to test re-decoding
        arsonResponse.resize(sizeof(resultHead) + resultTitle.getCount());
        memcpy(arsonResponse.getData(), &resultHead, sizeof(resultHead));
        memcpy(arsonResponse.getData() + sizeof(resultHead), resultTitle.getData(), resultTitle.getCount());
        arsonResponse.setCount(arsonResponse.getSize());
    }
    return convertExternalServiceErrorToArsonError(sesErr);
}

// wraps and does the http REST call to get the external session data, for PS4 platform.
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalSessionChangeableSessionData(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo,
    const Blaze::ExternalSessionIdentification& sessionIdentification, Blaze::ExternalSessionCustomData& arsonResponse, ExternalSessionErrorInfo& errorInfo) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].retrieveExternalSessionChangeableSessionData: (Arson) call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    eastl::string logBuf;
    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSessionChangeableSessionData: (Arson) Retrieving NP session(" << sessionIdentification.getPs4().getNpSessionId() << ")'s changeable-session-data from PSN. Caller(" << GameManager::externalUserIdentificationToString(ident, logBuf) << ").");
    Blaze::GameManager::ExternalSessionChangeableBinaryDataHead resultHead;//side: currently title-specified changeable data is n/a:
    BlazeRpcError sesErr = getNpSessionChangeableData(ident, authInfo, sessionIdentification.getPs4().getNpSessionId(), resultHead, nullptr, &errorInfo);
    if (sesErr == ERR_OK)
    {
        //repack into raw blob, to allow client to test re-decoding
        arsonResponse.resize(sizeof(resultHead));
        memcpy(arsonResponse.getData(), &resultHead, sizeof(resultHead));
        arsonResponse.setCount(arsonResponse.getSize());
    }
    return sesErr;
}

/*! ************************************************************************************************/
/*! \brief retrieves the NP session's custom changeable data
***************************************************************************************************/
BlazeRpcError ArsonSlaveImpl::getNpSessionChangeableData(const UserIdentification& caller,
    const ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, Blaze::GameManager::ExternalSessionChangeableBinaryDataHead& resultHead,
    ExternalSessionCustomData* resultTitle, ExternalSessionErrorInfo* errorInfo) const
{
    ClientPlatformType platform = caller.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].getNpSessionChangeableData: (Arson) call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    Blaze::GameManager::ExternalSessionUtilPs4* extSessUtil = getExternalSessionServicePs4();
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (npSessionId == nullptr) || (npSessionId[0] == '\0'))
    {
        if (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID)
        {
            ERR_LOG("[ArsonSlaveImpl].getNpSessionChangeableData: (Arson) invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or input NP session(" << ((npSessionId != nullptr) ? npSessionId : "<nullptr>") << "). No op. Caller(" << Blaze::GameManager::externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        }
        return ERR_SYSTEM;
    }

    Blaze::PSNServices::GetNpSessionChangeableDataRequest req;
    RawBuffer rsp(sizeof(Blaze::GameManager::ExternalSessionChangeableBinaryDataHead));
    req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
    req.setServiceLabel(extSessUtil->getServiceLabel());
    req.setSessionId(npSessionId);

    const CommandInfo& cmd = Blaze::PSNServices::PSNSessionInvitationSlave::CMD_INFO_GETNPSESSIONCHANGEABLEDATA;

    BlazeRpcError err = extSessUtil->callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, &rsp, errorInfo);
    if (err != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].getNpSessionChangeableData: (Arson) failed to get NP session(" << req.getSessionId() << ")'s data, " << ErrorHelp::getErrorName(err) << ".");
        return convertExternalServiceErrorToArsonError(err);
    }

    if (!externalSessionRawBinaryDataToBlazeData(rsp.data(), rsp.datasize(), resultHead, resultTitle))
    {
        ERR_LOG("[ArsonSlaveImpl].getNpSessionChangeableData: (Arson) failed to extract Blaze header from NP session(" << req.getSessionId() << ")'s data. Size of header structure(" << sizeof(resultHead) << "), size of raw session data(" << rsp.datasize() << "). Caller(" << Blaze::GameManager::externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << ErrorHelp::getErrorName(err) << ".");
    }
    TRACE_LOG("[ArsonSlaveImpl].getNpSessionChangeableData: (Arson) retrieved NP session(" << req.getSessionId() << ")'s changeable-data(strGameMode(" << eastl::string().sprintf("%.*s", (int)sizeof(resultHead.strGameMode), resultHead.strGameMode).c_str() << ")). Caller(" << Blaze::GameManager::externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
    return err;
}

// wraps and does the http REST call to get the external session, for PS4 platform.
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalSessionPs4(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo,
    const Blaze::ExternalSessionIdentification& sessionIdentification, const char8_t* language, Blaze::Arson::ArsonNpSession& arsonResponse, ExternalSessionErrorInfo& errorInfo) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].retrieveExternalSessionPs4: (Arson) call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    if (isTestingPs4Xgen())
    {
        ERR_LOG("[ArsonSlaveImpl].retrieveExternalSessionPs4: internal test issue: attempt at calling PS4 non-xgen fn, when xgen is enabled for PS4.");
        return ERR_NOT_SUPPORTED;
    }
    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSessionPs4: (Arson) Retrieving NP session(" << sessionIdentification.getPs4().getNpSessionId() << "), for language(" << (language != nullptr ? language : "<N/A>") << ") from PSN. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
    Blaze::PSNServices::GetNpSessionResponse sessionResult;
    BlazeRpcError sesErr = convertExternalServiceErrorToArsonError(
        getNpSession(ident, authInfo, sessionIdentification.getPs4().getNpSessionId(), language, sessionResult, &errorInfo));
    if (sesErr == ERR_OK)
    {
        if (!sessionResult.getSessions().empty() && sessionResult.getSessions().front()->getMembers().empty())
        {
            // There's been a rare Sony issue where the returned session's members could be empty (See DevNet ticket 63834). Sony recommends retrying the request
            // Blaze *already retried the requests* inside getNpSession(). Treat as error after the retry, though its PSN side
            ERR_LOG("[ArsonSlaveImpl].retrieveExternalSessionPs4 retry fetch of NP session(" << sessionIdentification.getPs4().getNpSessionId() << ") from PSN succeeded but session STILL had empty members list. Likely Sony/external service side error.");
            return ARSON_ERR_EXTERNAL_SESSION_SERVICE_INTERNAL_ERROR;
        }
        blazePs4NpSessionResponseToArsonResponse(sessionResult, sessionIdentification.getPs4().getNpSessionId(), arsonResponse);
        TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSessionPs4 (Arson): PSN REST call got user(" << ident.getName() << ")'s NP session(" << arsonResponse.getSessionId() << "), language(" << language <<
            "), sessionCreator(" << arsonResponse.getSessionCreator() << "), lockFlag(" << (arsonResponse.getSessionLockFlag()? "true" : "false") << "), maxUser(" << arsonResponse.getSessionMaxUser() <<
            "), name(" << arsonResponse.getSessionName() << "), privacy(" << arsonResponse.getSessionPrivacy() << "), status(" << arsonResponse.getSessionStatus() << "), memberCount(" << arsonResponse.getMembers().size() << "). caller(" << GameManager::toExtUserLogStr(ident) << ").");
        sesErr = validatePs4NpSessionResponse(sessionResult, sessionIdentification.getPs4().getNpSessionId());
    }
    return sesErr;
}

/*! ************************************************************************************************/
/*! \brief retrieves NP session info, including sessionPrivacy, sessionMaxUser, sessionName, sessionStatus and members list.
    \param[out] errorInfo Stores PSN call error and rate limit info on error. nullptr omits
***************************************************************************************************/
BlazeRpcError ArsonSlaveImpl::getNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
    const char8_t* npSessionId, const char8_t* language,
    Blaze::PSNServices::GetNpSessionResponse& result, ExternalSessionErrorInfo* errorInfo) const
{
    ClientPlatformType platform = caller.getPlatformInfo().getClientPlatform();
    if (platform != ps4)
    {
        WARN_LOG("[ArsonSlaveImpl].getNpSession: (Arson) call to ps4 fn using non ps4 platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    if (isTestingPs4Xgen())
    {
        ERR_LOG("[ArsonSlaveImpl].getNpSession: internal test issue: attempt at calling PS4 non-xgen fn, when xgen is enabled for PS4.");
        return ERR_NOT_SUPPORTED;
    }
    Blaze::GameManager::ExternalSessionUtilPs4* extSessUtil = getExternalSessionServicePs4();
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }

    if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (npSessionId == nullptr) || (npSessionId[0] == '\0'))
    {
        if (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID)
        {
            WARN_LOG("[ArsonSlaveImpl].getNpSession: (Arson) invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or ExternalId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or input NP session(" << ((npSessionId != nullptr) ? npSessionId : "<nullptr>") << "). No op. Caller(" << Blaze::GameManager::externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        }
        return ERR_SYSTEM;
    }
    Blaze::PSNServices::GetNpSessionRequest req;
    req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
    if (language != nullptr && language[0] != '\0')
        req.getHeader().setAcceptLanguage(language);
    req.setServiceLabel(extSessUtil->getServiceLabel());
    req.setSessionId(npSessionId);
    req.setLanguage(language);

    const CommandInfo& cmd = Blaze::PSNServices::PSNSessionInvitationSlave::CMD_INFO_GETNPSESSION;

    BlazeRpcError err = extSessUtil->callPSN(caller, cmd, authInfo, req.getHeader(), req, &result, nullptr, errorInfo);
    if (err != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].getNpSession: (Arson) failed to get user(" << caller.getBlazeId() << ")'s NP session(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ".");
        return convertExternalServiceErrorToArsonError(err);
    }
            
    // There's been a rare Sony issue where the returned session members is empty (See DevNet ticket 63834). Sony recommends retrying the request
    if ((err == ERR_OK) && !result.getSessions().empty() && result.getSessions().front()->getMembers().empty())
    {
        err = extSessUtil->callPSN(caller, cmd, authInfo, req.getHeader(), req, &result, nullptr, errorInfo);
        if (EA_UNLIKELY((err == ERR_OK) && !result.getSessions().empty() && result.getSessions().front()->getMembers().empty()))
        {
            WARN_LOG("[ArsonSlaveImpl].getNpSession: (Arson) fetch of NP session(" << req.getSessionId() << ") succeeded but session's members list was unexpectedly empty. Possible Sony side error.");
        }
    }
    if (result.getSessions().empty())
    {
        err = Blaze::PSNSESSIONINVITATION_RESOURCE_NOT_FOUND;//for back compat w/older pre-multiServiceLabel Sony API (tests expected this)
    }

    return convertExternalServiceErrorToArsonError(err);
}


//////////////// PS5 PlayerSession ////////////////

/*! ************************************************************************************************/
/*! \brief wraps and does the http REST call to get the 1st party PlayerSession, for PS5 platform.
***************************************************************************************************/
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalPs5PlayerSession(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& origAuthInfo,
    const Blaze::ExternalSessionIdentification& sessionIdentification, const char8_t* language, Blaze::Arson::ArsonPsnPlayerSession& arsonResponse,
    Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& decodedCustomData1, ExternalSessionCustomData& decodedCustomData2,
    ExternalSessionErrorInfo& errorInfo) const
{
    auto authInfo = origAuthInfo;//orig is const, just to ensure its token owner etc not updated
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    auto* extSessUtil = getExternalSessionServicePs5(platform);
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    if ((platform != ps5) && (!isTestingPs4Xgen() || (platform != ps4)))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].retrieveExternalPs5PlayerSession: (Arson) call to fn using wrong platform(" << Blaze::ClientPlatformTypeToString(platform) << ")").c_str());
        WARN_LOG(errorInfo.getMessage());
        return ARSON_ERR_INVALID_PARAMETER;
    }
    const char8_t* sessionId = sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId();

    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalPs5PlayerSession: (Arson) Retrieving session(" << sessionId << "), for language(" << 
        (language != nullptr ? language : "<N/A>") << ") from PSN. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
    Blaze::PSNServices::PlayerSessions::GetPlayerSessionResponseItem sessionResult;
    BlazeRpcError sesErr = convertExternalServiceErrorToArsonError(
        extSessUtil->mPlayerSessions.getPlayerSession(sessionResult, &errorInfo, authInfo, sessionId, ident));
    if (sesErr == ERR_OK)
    {
        blazePs5PlayerSessionResponseToArsonResponse(sessionResult, sessionId, arsonResponse);
        TRACE_LOG("[ArsonSlaveImpl].retrieveExternalPs5PlayerSession (Arson): PSN REST call got user(" << ident.getName() << ")'s session(" << arsonResponse.getSessionId() << "), language(" << language <<
            "), leader(" << arsonResponse.getLeader().getAccountId() << "), joinDisabled(" << (arsonResponse.getJoinDisabled() ? "true" : "false") << 
            "), maxPlayers(" << arsonResponse.getMaxPlayers() << "), maxSpectators(" << arsonResponse.getMaxSpectators() <<
            "), name(" << arsonResponse.getSessionName() << "), joinableUserType(" << arsonResponse.getJoinableUserType() << ", invitableUserType(" << arsonResponse.getInvitableUserType() <<
            "), playerCount(" << arsonResponse.getPlayers().size() << "), spectatorCount(" << arsonResponse.getSpectators().size() << "). caller(" << GameManager::toExtUserLogStr(ident) << ").");

        // For convenience, decode custom data's
        eastl::string psnCustomData1 = sessionResult.getCustomData1();
        if (!Blaze::GameManager::psnPlayerSessionCustomDataToBlazeData(decodedCustomData1, psnCustomData1))
        {
            errorInfo.setMessage((StringBuilder() << "retrieveExternalPs5PlayerSession (Arson): validation of customData1: failed to decode session's customData1 to expected Blaze structure. See logs").c_str());
            ERR_LOG(errorInfo.getMessage());
            sesErr = ARSON_ERR_EXTERNAL_SESSION_FAILED;
        }
        eastl::string buf;
        eastl::string psnCustomData2 = sessionResult.getCustomData2();
        if (!Blaze::GameManager::fromBase64(buf, psnCustomData2))
        {
            errorInfo.setMessage((StringBuilder() << "retrieveExternalPs5PlayerSession (Arson): validation of customData2: failed to decode session's customData2. See logs").c_str());
            ERR_LOG(errorInfo.getMessage());
            sesErr = ARSON_ERR_EXTERNAL_SESSION_FAILED;
        }
        decodedCustomData2.setData((uint8_t*)buf.data(), buf.length());
        decodedCustomData2.setCount(buf.length());

        if (!validatePs5PlayerSessionResponse(errorInfo, sessionResult, sessionId))
        {
            ERR_LOG(errorInfo.getMessage());
            sesErr = ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
        }
    }

    return sesErr;
}

/*! ************************************************************************************************/
/*! \brief wraps and does the http REST call to get the user's primary external session on Ps5, i.e. fetches user's current PlayerSessionId via PSN GET PlayerSessionIds method.
    \param[in] currentPrimarySession If non null, fill the complete PlayerSession's info in here.
***************************************************************************************************/
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveCurrentPrimarySessionPs5(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& origAuthInfo,
    Blaze::ExternalSessionIdentification& currentPrimarySessionIdent, ArsonPsnPlayerSession* currentPrimarySession,
    Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& decodedCustomData1, ExternalSessionCustomData& decodedCustomData2,
    ExternalSessionErrorInfo& errorInfo) const
{
    auto authInfo = origAuthInfo;//orig is const, just to ensure its token owner etc not updated
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if ((platform != ps5) && (!isTestingPs4Xgen() || (platform != ps4)))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].retrieveExternalPs5PlayerSession: (Arson) call to fn using wrong platform(" << Blaze::ClientPlatformTypeToString(platform) << ")").c_str());
        WARN_LOG(errorInfo.getMessage());
        return ARSON_ERR_INVALID_PARAMETER;
    }
    auto* extSessUtil = getExternalSessionServicePs5(platform);
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }

    Blaze::PSNServices::PlayerSessions::GetPlayerSessionIdResponseItem result;
    BlazeRpcError err = extSessUtil->mPlayerSessions.getPrimaryPlayerSessionId(result, &errorInfo, authInfo, ident);
    if (err != ERR_OK)
    {
        return convertExternalServiceErrorToArsonError(err);
    }
    if (result.getSessionId()[0] == '\0')
    {
        TRACE_LOG("[ArsonSlaveImpl].retrieveCurrentPrimarySessionPs5: (Arson) found NO current primary session for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
        return ERR_OK;
    }
    currentPrimarySessionIdent.getPs5().getPlayerSession().setPlayerSessionId(result.getSessionId());

    if (currentPrimarySession != nullptr)
    {
        // also fill entire PlayerSession info
        err = retrieveExternalPs5PlayerSession(ident, authInfo, currentPrimarySessionIdent, nullptr, *currentPrimarySession, decodedCustomData1, decodedCustomData2, errorInfo);
    }
    return err;
}


// transfer to client-exposed tdf, from the server side only rest tdf.
// (side: GetPlayerSessionResponse tdf cannot be reused for client, as its blaze server side only).
void ArsonSlaveImpl::blazePs5PlayerSessionResponseToArsonResponse(const Blaze::PSNServices::PlayerSessions::GetPlayerSessionResponseItem& restReponse,
    const char8_t* psnPlayerSessionId, Blaze::Arson::ArsonPsnPlayerSession &arsonResponse)
{
    arsonResponse.setSessionId(psnPlayerSessionId);
    arsonResponse.setCreationTimestamp(EA::StdC::AtoU64(restReponse.getCreatedTimestamp()));
    blazePs5PlayerSessionMemberToArsonResponse(restReponse.getLeader(), arsonResponse.getLeader());
    arsonResponse.setJoinDisabled(restReponse.getJoinDisabled());
    arsonResponse.setMaxPlayers(restReponse.getMaxPlayers());
    arsonResponse.setMaxSpectators(restReponse.getMaxSpectators());
    arsonResponse.setSessionName(restReponse.getSessionName());
    arsonResponse.setJoinableUserType(restReponse.getJoinableUserType());
    arsonResponse.setInvitableUserType(restReponse.getInvitableUserType());
    arsonResponse.setCustomData1(restReponse.getCustomData1());
    arsonResponse.setCustomData2(restReponse.getCustomData2());
    for (auto itr : restReponse.getMember().getPlayers())
    {
        auto nextMember = arsonResponse.getPlayers().pull_back();
        blazePs5PlayerSessionMemberToArsonResponse(*itr, *nextMember);
    }
    for (auto itr : restReponse.getMember().getSpectators())
    {
        auto nextMember = arsonResponse.getSpectators().pull_back();
        blazePs5PlayerSessionMemberToArsonResponse(*itr, *nextMember);
    }
}
void ArsonSlaveImpl::blazePs5PlayerSessionMemberToArsonResponse(const Blaze::PSNServices::PlayerSessions::PlayerSessionMember& psnReponse,
    Blaze::Arson::ArsonPsnPlayerSessionMember &arsonResponse)
{
    arsonResponse.setOnlineId(psnReponse.getOnlineId());
    arsonResponse.setAccountId(psnReponse.getAccountId());
    arsonResponse.setCustomData1(psnReponse.getCustomData1());
    arsonResponse.setJoinTimestamp(psnReponse.getJoinTimestamp());
    arsonResponse.setPlatform(psnReponse.getPlatform());
}

// Boiler plate checks for 1st party PlayerSession's validity. Kept here on server side so easier to find and update, when developing Blaze S2S code (in case these expected values may need to change)
// On failure returns false and sets errorInfo.message.
bool ArsonSlaveImpl::validatePs5PlayerSessionResponse(ExternalSessionErrorInfo& errorInfo, const PSNServices::PlayerSessions::GetPlayerSessionResponseItem& restReponse, const char8_t* psnPlayersSessionId) const
{
    if (psnPlayersSessionId == nullptr || psnPlayersSessionId[0] == '\0')
    {
        return true;// none
    }
    // validate swapSupported, leaderPrivileges, supportedPlatforms
    const bool expectedSwapSupported = false;//only this is supported by Blaze
    if (restReponse.getSwapSupported() != expectedSwapSupported)
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has invalid swapSupported(" << restReponse.getSwapSupported() << "), expected(" << expectedSwapSupported << ").").c_str());
        ERR_LOG(errorInfo.getMessage());
        return false;
    }
    if (!restReponse.getLeaderPrivileges().empty())
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has invalid leaderPrivileges(" << restReponse.getLeaderPrivileges() << "), expected empty.").c_str());
        ERR_LOG(errorInfo.getMessage());
        return false;
    }
    if (!restReponse.getJoinableSpecifiedUsers().empty())
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has invalid joinableSpecifiedUsers(" << restReponse.getJoinableSpecifiedUsers() << "), expected empty.").c_str());
        ERR_LOG(errorInfo.getMessage());
        return false;
    }
    if (restReponse.getSupportedPlatforms().empty() || (restReponse.getSupportedPlatforms().size() > 2))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has invalid number of supportedPlatforms(" << restReponse.getSupportedPlatforms() << ").").c_str());
        ERR_LOG(errorInfo.getMessage());
        return false;
    }
    for (auto it : restReponse.getSupportedPlatforms())
    {
        if ((blaze_strcmp(it.c_str(), GameManager::ExternalSessionUtilPs5PlayerSessions::toSupportedPlatform(ps5)) != 0) && (blaze_strcmp(it.c_str(), GameManager::ExternalSessionUtilPs5PlayerSessions::toSupportedPlatform(ps4)) != 0))
        {
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has an invalid platform in supportedPlatforms(" << restReponse.getSupportedPlatforms() << ").").c_str());
            ERR_LOG(errorInfo.getMessage());
            return false;
        }
    }
    if (restReponse.getCustomData1()[0] == '\0')
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN has empty customData1. Expected to contain encoded Blaze data (non empty).").c_str());
        ERR_LOG(errorInfo.getMessage());
        return false;
    }
    else
    {
        // validate customData1 decodes and contains the GameId
        const eastl::string psnCustomData1 = restReponse.getCustomData1();
        Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5 blazeData;
        if (!Blaze::GameManager::psnPlayerSessionCustomDataToBlazeData(blazeData, psnCustomData1))
        {
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN failed to decode customData1(" << psnCustomData1 << "). See logs.").c_str());
            ERR_LOG(errorInfo.getMessage());
            return false;
        }
        if (blazeData.getGameId() == Blaze::GameManager::INVALID_GAME_ID)
        {
            eastl::string jsonBuf;
            Blaze::GameManager::fromBase64(jsonBuf, psnCustomData1);
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5PlayerSessionResponse: (Arson) 1st party PlayerSession(" << psnPlayersSessionId << ") returned from PSN customData1(" << psnCustomData1 << " -> json: " << jsonBuf << ") missing valid decodable GameId. See logs.").c_str());
            ERR_LOG(errorInfo.getMessage());
            return false;
        }
    }
    
    return true;
}

//////////////// PS5 Match ////////////////

/*! ************************************************************************************************/
/*! \brief wraps and does the http REST call to get the 1st party PSN Match, for PS5 platform.
***************************************************************************************************/
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalPs5Match(const Blaze::UserIdentification& ident,
    const ExternalSessionIdentification& sessionIdentification, const char8_t* language, Arson::ArsonPsnMatch& arsonResponse,
    ExternalSessionErrorInfo& errorInfo) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if ((platform != ps5) && (!isTestingPs4Xgen() || (platform != ps4)))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].retrieveExternalPs5Match: (Arson) call to fn using wrong platform(" << Blaze::ClientPlatformTypeToString(platform) << ")").c_str());
        WARN_LOG(errorInfo.getMessage());
        return ARSON_ERR_INVALID_PARAMETER;
    }
    auto* extSessUtil = getExternalSessionServicePs5(platform);
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    const char8_t* sessionId = sessionIdentification.getPs5().getMatch().getMatchId();
    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalPs5Match: (Arson) Retrieving session(" << sessionId << "), for language(" <<
        (language != nullptr ? language : "<N/A>") << ") from PSN. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
    PSNServices::Matches::GetMatchDetailResponse sessionResult;
    BlazeRpcError sesErr = convertExternalServiceErrorToArsonError(
        extSessUtil->mMatches.getMatch(sessionResult, errorInfo, sessionId, &ident));
    if (sesErr == ERR_OK)
    {
        blazePs5MatchResponseToArsonResponse(sessionResult, arsonResponse);
        TRACE_LOG("[ArsonSlaveImpl].retrieveExternalPs5Match: (Arson) PSN REST call got user(" << ident.getName() << ")'s session(" << sessionId << "). caller(" << GameManager::toExtUserLogStr(ident) << ").");

        if (!validatePs5MatchResponse(errorInfo, sessionResult, sessionId))
        {
            ERR_LOG("[ArsonSlaveImpl].retrieveExternalPs5Match: validate Match failed: " << errorInfo.getMessage());
            sesErr = ARSON_ERR_EXTERNAL_SESSION_FAILED;
        }
    }

    return sesErr;
}

/*! ************************************************************************************************/
/*! \param[in] ignoreJoinFlag If true, include in return results Matches where user has a player object,
        even if its PSN Match player joinFlag is false. If false, Matches having the player but with joinFlag false are omitted.
***************************************************************************************************/
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveCurrentPs5Matches(const Blaze::UserIdentification& ident, Blaze::Arson::GetPs5MatchResponseList& currentMatches,
    ExternalSessionErrorInfo& errorInfo, bool ignoreJoinFlag) const
{
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if ((platform != ps5) && (!isTestingPs4Xgen() || (platform != ps4)))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].retrieveCurrentPs5Matches: (Arson) call to fn using wrong platform(" << Blaze::ClientPlatformTypeToString(platform) << ")").c_str());
        WARN_LOG(errorInfo.getMessage())
        return ARSON_ERR_INVALID_PARAMETER;
    }
    auto* extSessUtil = getExternalSessionServicePs5(platform);
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    BlazeRpcError err = ERR_OK;
    auto* gmComp = static_cast<Blaze::GameManager::GameManagerSlaveImpl*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, true, true, &err));
    if (gmComp == nullptr)
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].retrieveCurrentPs5Matches: (Arson) internal unexpected error " << Blaze::ErrorHelp::getErrorName(err) << ", GameManager component n/a.").c_str());
        WARN_LOG(errorInfo.getMessage());
        return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
    }
    TRACE_LOG("[ArsonSlaveImpl].retrieveCurrentPs5Matches: (Arson) fetching Games and checking for Matches, for user(" << GameManager::toExtUserLogStr(ident) <<").");
    // PS5 does not have a 1st party API for fetching all Matches of user currently. Need to get from Blaze all users games to check
    //Find out from the search slaves where the player has a game
    Blaze::Search::GetGamesRequest ggReq;
    Blaze::Search::GetGamesResponse ggResp;
    ggReq.getBlazeIds().push_back(ident.getBlazeId());
    ggReq.setGameProtocolVersionString(Blaze::GameManager::GAME_PROTOCOL_VERSION_MATCH_ANY);
    ggReq.setErrorIfGameProtocolVersionStringsMismatch(false);
    ggReq.setFetchOnlyOne(false);
    ggReq.setResponseType(Blaze::Search::GET_GAMES_RESPONSE_TYPE_FULL);
    ggReq.getGameTypeList().push_back();
    err = gmComp->fetchGamesFromSearchSlaves(ggReq, ggResp);
    if (err == ERR_OK)
    {
        for (auto itr : ggResp.getFullGameData())
        {
            auto& sessIdent = itr->getGame().getExternalSessionIdentification();
            const auto gameId = itr->getGame().getGameId();
            Blaze::Arson::ArsonPsnMatch rspMatch;
            auto matErr = retrieveExternalPs5Match(ident, sessIdent, nullptr, rspMatch, errorInfo);
            if (matErr != ERR_OK)
            {
                ERR_LOG("[ArsonSlaveImpl].retrieveCurrentPs5Matches: (Arson) error(" << Blaze::ErrorHelp::getErrorName(err) << " " << errorInfo.getMessage() << ") fetching Match(" << sessIdent.getPs5().getMatch().getMatchId() << "), whose id was found on Game(" << gameId << ").");
                err = matErr;
                continue;
            }
            // For tests, we may return the Match even if its 'joined' flag is false, tests can check joined flag
            auto isInMatch = isBlazeUserInPsnMatch(ident, rspMatch);
            if ((IS_IN_PSNMATCH_NO == isInMatch) || ((isInMatch == IS_IN_PSNMATCH_YES_WITH_JOINFLAG_FALSE) && !ignoreJoinFlag))
            {
                continue;
            }
            // add result
            auto currMatchReturnItem = currentMatches.pull_back();
            currMatchReturnItem->setGameId(gameId);
            sessIdent.copyInto(currMatchReturnItem->getSessionIdentification());
            rspMatch.copyInto(currMatchReturnItem->getPsnMatch());
        }
    }
    return err;
}

// \param[out] currentMatches Existing Matches the player was found with joinFlag true in, which needed to be cleared.
Blaze::BlazeRpcError ArsonSlaveImpl::clearCurrentPs5Matches(const Blaze::UserIdentification& ident, Blaze::Arson::GetPs5MatchResponseList& currentMatches,
    ExternalSessionErrorInfo& errorInfo) const
{
    BlazeRpcError err = retrieveCurrentPs5Matches(ident, currentMatches, errorInfo, false);
    if (err != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].clearCurrentPs5Matches: (Arson) failed to get user's current Matches, " << Blaze::ErrorHelp::getErrorName(err) << ".");
        return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
    }
    auto* extSessUtil = getExternalSessionServicePs5(ident.getPlatformInfo().getClientPlatform());
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;
    }
    TRACE_LOG("[ArsonSlaveImpl].clearCurrentPs5Matches: (Arson) attempting to clear() user from () potential Matches its in.");
    for (auto itr : currentMatches)
    {
        UpdatePrimaryExternalSessionForUserParameters params;
        ident.copyInto(params.getUserIdentification());
        itr->getSessionIdentification().copyInto(params.getChangedGame().getSessionIdentification());
        params.getChangedGame().setGameId(itr->getGameId());
        params.setRemoveReason(Blaze::GameManager::BLAZESERVER_CONN_LOST);
        auto sesErr = extSessUtil->mMatches.leaveMatch(errorInfo, params);
        if (sesErr != ERR_OK)
            err = convertExternalServiceErrorToArsonError(sesErr);
    }
    return err;
}

// transfer to client-exposed tdf, from the server side only rest tdf.
// (side: GetMatchDetailResponse tdf cannot be reused for client, as its blaze server side only).
void ArsonSlaveImpl::blazePs5MatchResponseToArsonResponse(const Blaze::PSNServices::Matches::GetMatchDetailResponse& restReponse,
    Blaze::Arson::ArsonPsnMatch &arsonResponse)
{
    arsonResponse.setActivityId(restReponse.getActivityId());
    arsonResponse.setCompetitionType(restReponse.getCompetitionType());
    arsonResponse.setExpirationTime(restReponse.getExpirationTime());
    arsonResponse.setGroupingType(restReponse.getGroupingType());
    arsonResponse.setMatchEndTimestamp(restReponse.getMatchEndTimestamp());
    arsonResponse.setMatchStartTimestamp(restReponse.getMatchStartTimestamp());
    arsonResponse.setResultType(restReponse.getResultType());
    arsonResponse.setStatus(restReponse.getStatus());
    arsonResponse.getMatchResults().setCooperativeResult(restReponse.getMatchResults().getCooperativeResult());
    arsonResponse.getMatchResults().setVersion(restReponse.getMatchResults().getVersion());
    for (auto itr : restReponse.getInGameRoster().getPlayers())
    {
        auto next = arsonResponse.getInGameRoster().getPlayers().pull_back();
        blazePs5MatchPlayerToArsonResponse(*itr, *next);
    }
    for (auto itr : restReponse.getInGameRoster().getTeams())
    {
        auto next = arsonResponse.getInGameRoster().getTeams().pull_back();
        blazePs5MatchTeamToArsonResponse(*itr, *next);
    }
    for (auto itr : restReponse.getMatchResults().getCompetitiveResult().getPlayerResults())
    {
        auto next = arsonResponse.getMatchResults().getCompetitiveResult().getPlayerResults().pull_back();
        blazePs5MatchPlayerResultToArsonResponse(*itr, *next);
    }
    for (auto itr : restReponse.getMatchResults().getCompetitiveResult().getTeamResults())
    {
        auto next = arsonResponse.getMatchResults().getCompetitiveResult().getTeamResults().pull_back();
        blazePs5MatchTeamResultToArsonResponse(*itr, *next);
    }
    for (auto itr : restReponse.getMatchStatistics().getPlayerStatistics())
    {
        auto next = arsonResponse.getMatchStatistics().getPlayerStatistics().pull_back();
        blazePs5MatchPlayerStatsToArsonResponse(*itr, *next);
    }
    for (auto itr : restReponse.getMatchStatistics().getTeamStatistics())
    {
        auto next = arsonResponse.getMatchStatistics().getTeamStatistics().pull_back();
        blazePs5MatchTeamStatsToArsonResponse(*itr, *next);
    }
}
void ArsonSlaveImpl::blazePs5MatchPlayerToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayer& psnReponse,
    Blaze::Arson::ArsonPsnMatchPlayer &arsonResponse)
{
    arsonResponse.setOnlineId(psnReponse.getOnlineId());
    arsonResponse.setAccountId(psnReponse.getAccountId());
    arsonResponse.setJoinFlag(psnReponse.getJoinFlag());
    arsonResponse.setPlayerId(psnReponse.getPlayerId());
    arsonResponse.setPlayerName(psnReponse.getPlayerName());
    arsonResponse.setPlayerType(psnReponse.getPlayerType());
    arsonResponse.setTeamId(psnReponse.getTeamId());
}
void ArsonSlaveImpl::blazePs5MatchTeamToArsonResponse(const Blaze::PSNServices::Matches::MatchTeam& psnReponse,
    Blaze::Arson::ArsonPsnMatchTeam &arsonResponse)
{
    arsonResponse.setTeamId(psnReponse.getTeamId());
    arsonResponse.setTeamName(psnReponse.getTeamName());
    for (auto itr : psnReponse.getMembers())
    {
        auto next = arsonResponse.getMembers().pull_back();
        next->setJoinFlag(itr->getJoinFlag());
        next->setPlayerId(itr->getPlayerId());
    }
}
void ArsonSlaveImpl::blazePs5MatchPlayerResultToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayerResult& psnReponse,
    Blaze::Arson::ArsonPsnMatchPlayerResult &arsonResponse)
{
    arsonResponse.setPlayerId(psnReponse.getPlayerId());
    arsonResponse.setRank(psnReponse.getRank());
    arsonResponse.setScore(psnReponse.getScore());
}
void ArsonSlaveImpl::blazePs5MatchTeamResultToArsonResponse(const Blaze::PSNServices::Matches::MatchTeamResult& psnReponse,
    Blaze::Arson::ArsonPsnMatchTeamResult &arsonResponse)
{
    arsonResponse.setTeamId(psnReponse.getTeamId());
    arsonResponse.setRank(psnReponse.getRank());
    arsonResponse.setScore(psnReponse.getScore());
    for (auto itr : psnReponse.getTeamMemberResults())
    {
        auto next = arsonResponse.getTeamMemberResults().pull_back();
        next->setScore(itr->getScore());
        next->setPlayerId(itr->getPlayerId());
    }
}
void ArsonSlaveImpl::blazePs5MatchPlayerStatsToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayerStats& psnReponse,
    Blaze::Arson::ArsonPsnMatchPlayerStats &arsonResponse)
{
    arsonResponse.setPlayerId(psnReponse.getPlayerId());
    for (auto itr : psnReponse.getStats())
    {
        auto next = arsonResponse.getStats().pull_back();
        next->setStatsKey(itr->getStatsKey());
        next->setStatsValue(itr->getStatsValue());
    }
}
void ArsonSlaveImpl::blazePs5MatchTeamStatsToArsonResponse(const Blaze::PSNServices::Matches::MatchTeamStats& psnReponse,
    Blaze::Arson::ArsonPsnMatchTeamStats &arsonResponse)
{
    arsonResponse.setTeamId(psnReponse.getTeamId());
    for (auto itr : psnReponse.getStats())
    {
        auto next = arsonResponse.getStats().pull_back();
        next->setStatsKey(itr->getStatsKey());
        next->setStatsValue(itr->getStatsValue());
    }
    for (auto itr : psnReponse.getTeamMemberStatistics())
    {
        auto next = arsonResponse.getTeamMemberStatistics().pull_back();
        blazePs5MatchPlayerStatsToArsonResponse(*itr, *next);
    }
}

// Boiler plate checks for 1st party Match's validity. Kept here on server side so easier to find and update, when developing Blaze S2S code (in case these expected values may need to change)
// On failure returns false and sets errorInfo.message.
bool ArsonSlaveImpl::validatePs5MatchResponse(ExternalSessionErrorInfo& errorInfo, const PSNServices::Matches::GetMatchDetailResponse& restReponse, const char8_t* psnMatchId) const
{
    if (psnMatchId == nullptr || psnMatchId[0] == '\0')
    {
        return true;// none
    }
    // validate 
    if (restReponse.getActivityId()[0] == '\0')
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has invalid getActivityId(" << restReponse.getActivityId() << "), NOT expected empty. Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    auto competitionType = PSNServices::Matches::INVALID_COMPETITION_TYPE;
    if (!Blaze::PSNServices::Matches::ParsePsnCompetitionTypeEnum(restReponse.getCompetitionType(), competitionType))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has poss invalid competitionType(" << restReponse.getCompetitionType() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    auto groupingType = PSNServices::Matches::INVALID_GROUPING_TYPE;
    if (!Blaze::PSNServices::Matches::ParsePsnGroupingTypeEnum(restReponse.getGroupingType(), groupingType))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has poss invalid groupingType(" << restReponse.getGroupingType() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    auto resultType = PSNServices::Matches::INVALID_RESULT_TYPE;
    if (!Blaze::PSNServices::Matches::ParsePsnResultTypeEnum(restReponse.getResultType(), resultType))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has poss invalid resultType(" << restReponse.getResultType() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    auto status = PSNServices::Matches::INVALID_MATCH_STATUS_TYPE;
    if (!Blaze::PSNServices::Matches::ParsePsnMatchStatusEnum(restReponse.getStatus(), status))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has poss invalid status(" << restReponse.getStatus() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    if (restReponse.getInGameRoster().getPlayers().empty())
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has empty getInGameRoster().getPlayers(). Expected to contain non empty. Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    for (auto itr : restReponse.getInGameRoster().getPlayers())
    {
        if ((itr->getAccountId()[0] == '\0') || (itr->getOnlineId()[0] == '\0') || (itr->getPlayerId()[0] == '\0') || (itr->getPlayerName()[0] == '\0') || (itr->getPlayerType()[0] == '\0'))
        {
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has player with empty one of: accountId(" << itr->getAccountId() << "), onlineId(" << itr->getOnlineId() << "), playerId(" << itr->getPlayerId() << "), playerName(" << itr->getPlayerName() << "), playerType(" << itr->getPlayerType() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
            return false;
        }
        auto playerType = PSNServices::Matches::INVALID_PLAYER_TYPE;
        if (!Blaze::PSNServices::Matches::ParsePsnPlayerTypeEnum(itr->getPlayerType(), playerType))
        {
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has poss invalid playerType(" << itr->getPlayerType() << ") for player(" << itr->getAccountId() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
            return false;
        }
    }
    if (restReponse.getInGameRoster().getTeams().empty() && (groupingType == PSNServices::Matches::TEAM_MATCH))
    {
        errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has empty getInGameRoster().getTeams(). Expected to contain non empty, since its groupingType(" << restReponse.getGroupingType() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
        return false;
    }
    for (auto itr : restReponse.getInGameRoster().getTeams())
    {
        if (itr->getTeamId()[0] == '\0')
        {
            errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN has player with empty one of: getTeamId(" << itr->getTeamId() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue. For ref, team's size(" << itr->getMembers().size() << ")").c_str());
            return false;
        }
        // validate all members of the PSN Match
        for (auto teamMemIt : itr->getMembers())
        {
            if (teamMemIt->getPlayerId()[0] == '\0')
            {
                errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN, team(" << itr->getTeamId()  << ") member had empty one of: playerId(" << teamMemIt->getPlayerId() << "). Poss decoding issue, or, incorrect Blaze TDF, or, Sony issue.").c_str());
                return false;
            }
            if (IS_IN_PSNMATCH_YES_WITH_JOINFLAG_TRUE == isPlayerIdInPsnMatchTeamOtherThanTeam(teamMemIt->getPlayerId(), restReponse, itr->getTeamId()))
            {
                // in such case, the loop itr team must have its joinFlag false
                if (teamMemIt->getJoinFlag())
                {
                    errorInfo.setMessage((StringBuilder() << "[ArsonSlaveImpl].validatePs5MatchResponse: (Arson) 1st party Match(" << psnMatchId << ") returned from PSN, team(" << itr->getTeamId() << ") member playerId(" << teamMemIt->getPlayerId() << ") was ALSO in another team ALSO with joinflag true. Should only at most be in ONE team at a time!").c_str());
                    return false;
                }
            }
        }
    }
    return true;
}
/*! ************************************************************************************************/
/*! \brief helper to check whether the Blaze member was in the PSN Match Team other than the specified one. (Used to boiler-plate validate no dupes, see caller)
***************************************************************************************************/
ArsonSlaveImpl::IsInPsnMatchEnum ArsonSlaveImpl::isPlayerIdInPsnMatchTeamOtherThanTeam(const char8_t* playerId, const PSNServices::Matches::GetMatchDetailResponse& restReponse, const char8_t* otherThanPsnMatchTeamId)
{
    if (otherThanPsnMatchTeamId == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].isPlayerIdInPsnMatchTeamOtherThanTeam: (Arson) Test issue: null input playerId(" << (playerId ? playerId : "<null>") << ") or otherThanPsnMatchTeamId(" << (otherThanPsnMatchTeamId ? otherThanPsnMatchTeamId : "<null>") << "), cannot check.");
        return IS_IN_PSNMATCH_NO;
    }
    for (auto itr : restReponse.getInGameRoster().getTeams())
    {
        if (blaze_stricmp(itr->getTeamId(), otherThanPsnMatchTeamId) == 0)
        {
            continue;
        }
        for (auto teamMemIt : itr->getMembers())
        {
            if (blaze_stricmp(teamMemIt->getPlayerId(), playerId) == 0)
            {
                auto result = (teamMemIt->getJoinFlag() ? IS_IN_PSNMATCH_YES_WITH_JOINFLAG_TRUE : IS_IN_PSNMATCH_YES_WITH_JOINFLAG_FALSE);
                TRACE_LOG("[ArsonSlaveImpl].isPlayerIdInPsnMatchTeamOtherThanTeam: (Arson) playerId(" << (playerId ? playerId : "<null>") << ") found in a team(" << itr->getTeamId() << "), with joinFlag(" << teamMemIt->getJoinFlag() << "), as well as other in team(" << (otherThanPsnMatchTeamId ? otherThanPsnMatchTeamId : "<null>") << "). (This maybe ok as long as at most one of the teams has player's joinFlag true)");
                return result;
            }
        }
    }
    return IS_IN_PSNMATCH_NO;
}


/*! ************************************************************************************************/
/*! \brief helper to check whether the Blaze member was in the PSN Match fetched from PSN
***************************************************************************************************/
ArsonSlaveImpl::IsInPsnMatchEnum ArsonSlaveImpl::isBlazeUserInPsnMatch(const Blaze::UserIdentification& blazeMember, const Blaze::Arson::ArsonPsnMatch& psnMatch)
{
    for (auto itr : psnMatch.getInGameRoster().getPlayers())
    {
        ExternalPsnAccountId id = EA::StdC::AtoU64(itr->getAccountId());
        if (id == blazeMember.getPlatformInfo().getExternalIds().getPsnAccountId())
        {
            return (itr->getJoinFlag() ? IS_IN_PSNMATCH_YES_WITH_JOINFLAG_TRUE : IS_IN_PSNMATCH_YES_WITH_JOINFLAG_FALSE);
        }
    }
    return IS_IN_PSNMATCH_NO;
}





/////////////// Xbox One

// wraps and does the http REST call to get the external session, for Xbox MPSD platform.
// (note: there is no GET external session available with stock blazeserver, so we have a custom method here)
Blaze::BlazeRpcError ArsonSlaveImpl::retrieveExternalSessionXbox(const Blaze::XblScid& scid,
    const Blaze::XblSessionTemplateName& sessionTemplateName, const Blaze::XblSessionName& sessionName,
    const Blaze::XblContractVersion& contractVersion,
    Arson::GetXblMultiplayerSessionResponse &arsonResponse)
{
    // Side: the get request is patterned after externalsessionutilxboxone.cpp's calls.
    TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSessionXbox: retrieving external session. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName << ".");
    ArsonXBLServices::ArenaMultiplayerSessionResponse result;
    BlazeRpcError error = getExternalSessionXbox(scid, contractVersion, sessionTemplateName, sessionName, result);
    if (error != ERR_OK)
    {
        TRACE_LOG("[ArsonSlaveImpl].retrieveExternalSession: Failed to retrieve external session. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName << ", error " << ErrorHelp::getErrorName(error));
        arsonResponse.setBlazeErrorCode(ARSON_ERR_EXTERNAL_SESSION_FAILED);
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;
    }

    // pack the response for sending back to the client. Note we have to do this because 
    // GetRestExternalSessionResponse tdf cannot be reused for client, as it contains blaze server side only tdfs as members
    blazeXblMultiplayerSessionResponseToArsonResponse(result, arsonResponse);

    return ERR_OK;
}

/*! \brief  helper to attempt to retrieve MPS. returns XBLSERVICECONFIGS_RESOURCE_NOT_FOUND if it does not exist */
Blaze::BlazeRpcError ArsonSlaveImpl::getExternalSessionXbox(const Blaze::XblScid& scid, const Blaze::XblContractVersion& contractVersion,
    const char8_t* sessionTemplateName, const char8_t* sessionName, ArsonXBLServices::ArenaMultiplayerSessionResponse& result)
{
    Blaze::Arson::ArsonExternalTournamentUtilXboxOne* extSessUtil = static_cast<Arson::ArsonExternalTournamentUtilXboxOne*>(mExternalTournamentService);
    BlazeRpcError err = extSessUtil->getMps(sessionTemplateName, sessionName, result);
    if (err == XBLCLIENTSESSIONDIRECTORY_NO_CONTENT || err == ARSON_XBLSESSIONDIRECTORYARENA_NO_CONTENT)
    {
        TRACE_LOG("[ArsonSlaveImpl].getExternalSessionXbox: session did not exist. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName);
    }
    else if (err != ERR_OK)
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionXbox: Failed to get session. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName<< ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
    }
    return err;
}

Blaze::BlazeRpcError ArsonSlaveImpl::retrieveCurrentPrimarySessionXbox(const Blaze::UserIdentification& ident, XBLServices::HandlesGetActivityResult& currentPrimarySession) const
{
    BlazeRpcError err = ERR_OK;
    ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
    if ((platform != ClientPlatformType::xone) && (platform != ClientPlatformType::xbsx))
    {
        TRACE_LOG("[ArsonSlaveImpl].retrieveCurrentPrimarySessionXbox: internal test error, attempted illegal call of xbox only method. Current platform is " << Blaze::ClientPlatformTypeToString(platform) << ".");
        return ERR_SYSTEM;
    }

    err = getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getPrimaryExternalSession(ident.getPlatformInfo().getExternalIds().getXblAccountId(), currentPrimarySession);
    return err;
}

bool ArsonSlaveImpl::isMpsdRetryError(BlazeRpcError mpsdCallErr)
{ 
    return ((mpsdCallErr == ERR_TIMEOUT) ||
        (mpsdCallErr == XBLSERVICECONFIGS_SERVICE_UNAVAILABLE) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_SERVICE_UNAVAILABLE) ||
        (mpsdCallErr == XBLSERVICECONFIGS_SERVICE_INTERNAL_ERROR) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_SERVICE_INTERNAL_ERROR) ||
        (mpsdCallErr == XBLSERVICECONFIGS_BAD_GATEWAY) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_BAD_GATEWAY));
}

// For ARSON custom rpc's to MPSD (not via Blaze). Should parallel the MPSD errors above.
bool ArsonSlaveImpl::isArsonMpsdRetryError(BlazeRpcError mpsdCallErr)
{ 
    return ((mpsdCallErr == ERR_TIMEOUT) ||
        (mpsdCallErr == ARSON_ERR_EXTERNAL_SESSION_SERVICE_UNAVAILABLE) ||
        (mpsdCallErr == ARSON_ERR_EXTERNAL_SESSION_SERVICE_INTERNAL_ERROR) ||
        (mpsdCallErr == ARSON_ERR_EXTERNAL_SESSION_BAD_GATEWAY));
}

BlazeRpcError ArsonSlaveImpl::waitSeconds(uint64_t seconds, const char8_t* context)
{
    TimeValue t;
    t.setSeconds(seconds);
    BlazeRpcError serr = Fiber::sleep(t, context);
    if (ERR_OK != serr)
    {
        ERR_LOG(((context != nullptr)? context : "[ArsonSlaveImpl].waitSeconds") << ": failed to sleep fiber, error(" << ErrorHelp::getErrorName(serr) << ").");
    }
    return serr;
}

BlazeRpcError ArsonSlaveImpl::callPSNsessionManager(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo, PSNServices::PsnWebApiHeader& reqHeader,
    const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo) const
{
    BlazeRpcError err = ERR_OK;
    ClientPlatformType platform = caller.getPlatformInfo().getClientPlatform();
    if ((platform != ps5) && (!isTestingPs4Xgen() || (platform != ps4)))
    {
        WARN_LOG("[ArsonSlaveImpl].callPSNsessionManager: (Arson) call to fn using wrong platform(" << Blaze::ClientPlatformTypeToString(platform) << ")");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    auto* extSessUtil = getExternalSessionServicePs5(platform);
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }
    err = extSessUtil->mPlayerSessions.callPSN(caller, cmdInfo, authInfo, reqHeader, req, rsp, errorInfo);
    TRACE_LOG("[ArsonSlaveImpl].callPSNsessionManager: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " result(" << ErrorHelp::getErrorName(err) << "). Caller(" << GameManager::toExtUserLogStr(caller) << ").");
    return err;
}


// transfer to client-exposed tdf, from the server side only rest tdf.
// (side: GetRestExternalSessionResponse tdf cannot be reused for client, as it contains blaze server side only tdfs as members).
void ArsonSlaveImpl::blazeXblMultiplayerSessionResponseToArsonResponse(const ArsonXBLServices::ArenaMultiplayerSessionResponse& restReponse,
    Arson::GetXblMultiplayerSessionResponse &arsonResponse)
{
    uint64_t externalSessionId = 0;
    blaze_str2int(restReponse.getConstants().getCustom().getExternalSessionId(), &externalSessionId);
    arsonResponse.getMultiplayerSessionResponse().getConstants().getCustom().setExternalSessionId(externalSessionId);
    arsonResponse.getMultiplayerSessionResponse().getConstants().getCustom().setExternalSessionType(restReponse.getConstants().getCustom().getExternalSessionType());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getCustom().setExternalSessionCustomDataString(restReponse.getConstants().getCustom().getExternalSessionCustomDataString());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setInviteProtocol(restReponse.getConstants().getSystem().getInviteProtocol());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setVisibility(restReponse.getConstants().getSystem().getVisibility());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setReservedRemovalTimeout(restReponse.getConstants().getSystem().getReservedRemovalTimeout());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setReadyRemovalTimeout(restReponse.getConstants().getSystem().getReadyRemovalTimeout());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setInactiveRemovalTimeout(restReponse.getConstants().getSystem().getInactiveRemovalTimeout());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().setSessionEmptyTimeout(restReponse.getConstants().getSystem().getSessionEmptyTimeout());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().getCapabilities().setGameplay(restReponse.getConstants().getSystem().getCapabilities().getGameplay());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().getCapabilities().setArbitration(restReponse.getConstants().getSystem().getCapabilities().getArbitration());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().getArbitration().setArbitrationTimeout(restReponse.getConstants().getSystem().getArbitration().getArbitrationTimeout());
    arsonResponse.getMultiplayerSessionResponse().getConstants().getSystem().getArbitration().setForfeitTimeout(restReponse.getConstants().getSystem().getArbitration().getForfeitTimeout());
    arsonResponse.getMultiplayerSessionResponse().getProperties().getSystem().setClosed(restReponse.getProperties().getSystem().getClosed());
    arsonResponse.getMultiplayerSessionResponse().getProperties().getSystem().setJoinRestriction(restReponse.getProperties().getSystem().getJoinRestriction());
    arsonResponse.getMultiplayerSessionResponse().setCorrelationId(restReponse.getCorrelationId());
    arsonResponse.getMultiplayerSessionResponse().getArbitration().setStatus(restReponse.getArbitration().getStatus());
    
    // MPS servers section

    // MPS servers section, for Arena Match MPSes:
    blazeRspToArsonResponse(restReponse.getServers().getTournaments().getConstants().getSystem().getTournamentRef(), arsonResponse.getMultiplayerSessionResponse().getServers().getTournaments().getConstants().getSystem().getTournamentRef());
    arsonResponse.getMultiplayerSessionResponse().getServers().getArbitration().getConstants().getSystem().setStartTime(restReponse.getServers().getArbitration().getConstants().getSystem().getStartTime());
    arsonResponse.getMultiplayerSessionResponse().getServers().getArbitration().getProperties().getSystem().setResultConfidenceLevel(restReponse.getServers().getArbitration().getProperties().getSystem().getResultConfidenceLevel());
    arsonResponse.getMultiplayerSessionResponse().getServers().getArbitration().getProperties().getSystem().setResultSource(restReponse.getServers().getArbitration().getProperties().getSystem().getResultSource());
    arsonResponse.getMultiplayerSessionResponse().getServers().getArbitration().getProperties().getSystem().setResultState(restReponse.getServers().getArbitration().getProperties().getSystem().getResultState());
    restReponse.getServers().getArbitration().getProperties().getSystem().getResults().copyInto(arsonResponse.getMultiplayerSessionResponse().getServers().getArbitration().getProperties().getSystem().getResults());

    // MPS members section
    ArsonXBLServices::ArsonArenaMembers::const_iterator itr = restReponse.getMembers().begin();
    ArsonXBLServices::ArsonArenaMembers::const_iterator end = restReponse.getMembers().end();
    for (; itr != end; ++itr)
    {
        //ARSON_TODO somehow seemed to get external id 0 sometimes. Poss external session bug or as designed, or, test/cleanup issue on our end?
        ExternalId xuid = INVALID_EXTERNAL_ID;
        sscanf(itr->second->getConstants().getSystem().getXuid(), "%" SCNu64, &xuid);
        if (xuid == 0)
            continue;

        Arson::ArsonMember* nextMember = arsonResponse.getMultiplayerSessionResponse().getMembers().allocate_element();
        nextMember->setArbitrationStatus(itr->second->getArbitrationStatus());
        nextMember->getConstants().getSystem().setXuid(xuid);
        nextMember->getConstants().getSystem().setInitialize(itr->second->getConstants().getSystem().getInitialize());
        nextMember->getConstants().getSystem().setTeam(itr->second->getConstants().getSystem().getTeam());
        nextMember->getProperties().getSystem().setActive(itr->second->getProperties().getSystem().getActive());
        // MPS member's properties.system.arbitration.results section
        itr->second->getProperties().getSystem().getArbitration().getResults().copyInto(nextMember->getProperties().getSystem().getArbitration().getResults());
        //add populated member to the map
        arsonResponse.getMultiplayerSessionResponse().getMembers()[itr->first] = nextMember;
    }
}

// transfer to client-exposed tdf, from the server side only rest tdf.
// (side: GetNpSessionResponse tdf cannot be reused for client, as its blaze server side only).
void ArsonSlaveImpl::blazePs4NpSessionResponseToArsonResponse(const PSNServices::GetNpSessionResponse& restReponseList, const char8_t* npSessionId,
    Arson::ArsonNpSession &arsonResponse)
{
    if (restReponseList.getSessions().empty())
    {
        ERR_LOG("[ArsonSlaveImpl].blazePs4NpSessionResponseToArsonResponse: empty GetNpSessionResponse, cannot populate info on npSession(" + eastl::string(npSessionId) + ")");
        return;
    }
    const PSNServices::GetNpSessionResponseItem& restReponse = *restReponseList.getSessions().front();

    arsonResponse.setSessionId(npSessionId);//side: not from PSN rsp, but from Blaze
    arsonResponse.setSessionCreateTimestamp(restReponse.getSessionCreateTimestamp());
    arsonResponse.setSessionCreator(restReponse.getSessionCreator().getOnlineId());
    arsonResponse.setSessionLockFlag(restReponse.getSessionLockFlag());
    arsonResponse.setSessionMaxUser(restReponse.getSessionMaxUser());
    arsonResponse.setSessionName(restReponse.getSessionName());
    arsonResponse.setSessionPrivacy(restReponse.getSessionPrivacy());
    arsonResponse.setSessionStatus(restReponse.getSessionStatus());
    arsonResponse.setSessionType(restReponse.getSessionType());
    for (auto itr : restReponse.getMembers())
    {
        Arson::ArsonNpSessionMember* nextMember = arsonResponse.getMembers().pull_back();
        nextMember->setOnlineId(itr->getOnlineId());
        nextMember->setAccountId(itr->getAccountId());
    }
}

void ArsonSlaveImpl::blazeRspToArsonResponse(const XBLServices::TournamentRef& xblRsp, ArsonTournamentRef& arsonResponse)
{
    arsonResponse.getTournamentIdentification().setTournamentId(xblRsp.getTournamentId());
    arsonResponse.getTournamentIdentification().setTournamentOrganizer(xblRsp.getOrganizer());
}

void ArsonSlaveImpl::blazeRspToArsonResponse(const XBLServices::MultiplayerSessionRef& xblRsp, ArsonMultiplayerSessionRef& arsonResponse)
{
    arsonResponse.setScid(xblRsp.getScid());
    arsonResponse.setName(xblRsp.getName());
    arsonResponse.setTemplateName(xblRsp.getTemplateName());
}

// Boiler plate checks for NP session's validity
BlazeRpcError ArsonSlaveImpl::validatePs4NpSessionResponse(const PSNServices::GetNpSessionResponse& restReponseList, const char8_t* npSessionId) const
{
    if (npSessionId == nullptr || npSessionId[0] == '\0')
    {
        return ERR_OK;// none
    }
    if (restReponseList.getSessions().empty())
    {
        ERR_LOG("[ArsonSlaveImpl].validatePs4NpSessionResponse: empty GetNpSessionResponse, cannot populate info on npSession(" + eastl::string(npSessionId) + ")");
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;
    }
    if (isTestingPs4Xgen())
    {
        ERR_LOG("[ArsonSlaveImpl].validatePs4NpSessionResponse: internal test issue: attempt at calling PS4 non-xgen fn, when xgen is enabled for PS4.");
        return ERR_NOT_SUPPORTED;
    }


    const PSNServices::GetNpSessionResponseItem& restReponse = *restReponseList.getSessions().front();

    const char8_t* expectedSessionType = "owner-migration";//only this is supported by Blaze
    if (blaze_strcmp(restReponse.getSessionType(), expectedSessionType) != 0)
    {
        ERR_LOG("[ArsonSlaveImpl].validatePs4NpSessionResponse NP session(" << npSessionId << ") returned from PSN has invalid session-type(" << restReponse.getSessionType() << "), expected(" << expectedSessionType << ").");
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;
    }
    // There's been very rare Sony issues where an 'empty members' NP session could be returned. (See DevNet ticket 63834)
    if (restReponse.getMembers().empty())
    {
        ERR_LOG("[ArsonSlaveImpl].validatePs4NpSessionResponse a valid NP session(" << npSessionId << ") was returned from PSN but its members list is empty. This indicates a possible error on the PSN side.");
        return ARSON_ERR_EXTERNAL_SESSION_SERVICE_INTERNAL_ERROR;
    }
    if (restReponse.getSessionMaxUser() < (uint16_t)restReponse.getMembers().size())
    {
        ERR_LOG("[ArsonSlaveImpl].validatePs4NpSessionResponse NP session(" << npSessionId << ") returned from PSN has a session max users(" << restReponse.getSessionMaxUser() << ") that is less than the total number of users in its member list(" << restReponse.getMembers().empty() << ").");
        return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
    }
    return ERR_OK;
}

// Return whether the first rate limit info has a higher rate limit remaining (closer/sooner) than the second.
// If rate limit info n/a, treated as highest possible rate limit remaining.
bool ArsonSlaveImpl::hasHigherRateLimitRemaining(const Blaze::RateLimitInfo& a, const Blaze::RateLimitInfo& b)
{
    bool aHasInfo = (a.getRateLimitTimePeriod()[0] != '\0');
    if (!aHasInfo)
        return true;
    return (a.getRateLimitRemaining() > b.getRateLimitRemaining());
}

// Test Libcurl SSL Support using an SFtp Server to try downloading xml files
PullRemoteXmlFilesError::Error ArsonSlaveImpl::processPullRemoteXmlFiles(PullRemoteXmlFilesResponse & response, const Message* message)
{
    const Blaze::Arson::SftpConfig& sftpConfig = getConfig().getSftpConfig();
    Blaze::Arson::CurlUtil curlUtil(sftpConfig);

    char8_t pDirectory[EA::IO::kMaxPathLength];
    int dirPathLength = EA::IO::Directory::GetCurrentWorkingDirectory(pDirectory);
    if (dirPathLength > EA::IO::kMaxPathLength)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].open: Cannot save files to directory with length " << dirPathLength <<
            " greater than " << EA::IO::kMaxPathLength << ".");
        return PullRemoteXmlFilesError::ERR_SYSTEM; 
    }
    
    if (!curlUtil.PullXmlFiles(pDirectory, sftpConfig.getDeletePulledFiles(), response.getFtpResponseList()))
    {
        return PullRemoteXmlFilesError::ERR_SYSTEM;
    }

    return PullRemoteXmlFilesError::ERR_OK;
} 

BlazeRpcError ArsonSlaveImpl::getGameDataFromSearchSlaves(Blaze::GameManager::GameId gameId, Blaze::GameManager::ReplicatedGameData& gameData)
{
    BlazeRpcError error;
    Blaze::GameManager::GameManagerSlaveImpl* gmComp = static_cast<Blaze::GameManager::GameManagerSlaveImpl*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, true, true, &error));
    if (gmComp == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].execute: internal unexpected error " << Blaze::ErrorHelp::getErrorName(error) << ", GameManager component n/a. Unable to complete boiler plate validation checks.");
        return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
    }
    
    Blaze::Search::GetGamesRequest ggReq;
    Blaze::Search::GetGamesResponse ggResp;
    ggReq.getGameIds().push_back(gameId);
    ggReq.setFetchOnlyOne(true);
    ggReq.setResponseType(Blaze::Search::GET_GAMES_RESPONSE_TYPE_FULL);

    BlazeRpcError lookupUserGameError = gmComp->fetchGamesFromSearchSlaves(ggReq, ggResp);
    if (lookupUserGameError != ERR_OK)
        return lookupUserGameError;
    if (ggResp.getFullGameData().empty())
    {
        ERR_LOG("[ArsonSlaveImpl].getGameDataFromSearchSlaves could not find the game " << gameId << ". Unable to retrieve its external session data.");
        return GAMEMANAGER_ERR_INVALID_GAME_ID;
    }

    ggResp.getFullGameData().front()->getGame().copyInto(gameData);

    if (!validateOldVsNewReplicatedExternalSessionData(gameData))
        return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;

    return ERR_OK;
}

/*! ************************************************************************************************/
/*! \brief boiler plate checks to ensure deprecated external sessions data members equal the new ExternalSessionIdentification members.
***************************************************************************************************/
bool ArsonSlaveImpl::validateOldVsNewReplicatedExternalSessionData(const Blaze::GameManager::ReplicatedGameData& replicatedGameData)
{
    bool isPass = true;
    if (blaze_strcmp(replicatedGameData.getNpSessionId(), replicatedGameData.getExternalSessionIdentification().getPs4().getNpSessionId()) != 0)
    {
        ERR_LOG("[ArsonSlaveImpl].validateOldVsNewReplicatedExternalSessionData: A game's ReplicatedGameData's deprecated mNpSessionId does not match the its new ExternalSessionIdentification.mPs4.mNpSessionId! Check logs for details.");
        isPass = false;
    }
    if (blaze_strcmp(replicatedGameData.getExternalSessionTemplateName(), replicatedGameData.getExternalSessionIdentification().getXone().getTemplateName()) != 0)
    {
        ERR_LOG("[ArsonSlaveImpl].validateOldVsNewReplicatedExternalSessionData: A game's ReplicatedGameData's deprecated mExternalSessionTemplateName does not match the its new ExternalSessionIdentification.mXone.mTemplateName! Check logs for details.");
        isPass = false;
    }
    if (blaze_strcmp(replicatedGameData.getExternalSessionName(), replicatedGameData.getExternalSessionIdentification().getXone().getSessionName()) != 0)
    {
        ERR_LOG("[ArsonSlaveImpl].validateOldVsNewReplicatedExternalSessionData: A game's ReplicatedGameData's deprecated mExternalSessionName does not match the its new ExternalSessionIdentification.mXone.mSessionName! Check logs for details.");
        isPass = false;
    }
    if (blaze_strcmp(replicatedGameData.getExternalSessionCorrelationId(), replicatedGameData.getExternalSessionIdentification().getXone().getCorrelationId()) != 0)
    {
        ERR_LOG("[ArsonSlaveImpl].validateOldVsNewReplicatedExternalSessionData: A game's ReplicatedGameData's deprecated mExternalSessionCorrelationId does not match the its new ExternalSessionIdentification.mXone.mCorrelationId! Check logs for details.");
        isPass = false;
    }
    return isPass;
}




Blaze::ExternalId ArsonSlaveImpl::getExternalIdFromPlatformInfo(const Blaze::PlatformInfo& platformInfo)//TODO: this extra step may no longer be required with crossplay changes in CL 1478670
{
    // with arson requests, this could be a mock platform set by the script.
    Blaze::PlatformInfo platInfoTemp;
    platformInfo.copyInto(platInfoTemp);
    Blaze::ClientPlatformType plat = platInfoTemp.getClientPlatform();
    if (plat == INVALID)
        platInfoTemp.setClientPlatform(gController->getDefaultClientPlatform());

    ExternalId extId = INVALID_EXTERNAL_ID;
    convertFromPlatformInfo(platInfoTemp, &extId, nullptr, nullptr);
    return extId;
}


/*! ************************************************************************************************/
/*! \brief return the local x1 external session util.
***************************************************************************************************/
Blaze::GameManager::ExternalSessionUtilXboxOne* ArsonSlaveImpl::getExternalSessionServiceXbox(ClientPlatformType platform) const
{
    return (GameManager::ExternalSessionUtilXboxOne*)getExternalSessionService(platform);
}

/*! ************************************************************************************************/
/*! \brief return the local ps4 external session util.
    Ensures the util's owning GameManagerSlave is set. NOTE: ARSON should call this fn to use util's commands that need the owning GameManagerSlave set (see Blaze's ExternalSessionUtilPs4 for details).
    \note: for fetching the crossgen *ps5* externalSessionUtil, use getExternalSessionServicePs5(ps4)
***************************************************************************************************/
Blaze::GameManager::ExternalSessionUtilPs4* ArsonSlaveImpl::getExternalSessionServicePs4() const 
{
    if (isTestingPs4Xgen())
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionServicePs4: internal test issue: attempt at calling PS4 non-xgen fn, when xgen is enabled for PS4.");
        return nullptr;
    }
    return (GameManager::ExternalSessionUtilPs4*)getExternalSessionService(ps4);
}
const Blaze::ExternalSessionActivityTypeInfo* ArsonSlaveImpl::getExternalSessionActivityTypeInfoPs4() const
{
    if (isTestingPs4Xgen())
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionActivityTypeInfoPs4: internal test issue: attempt at calling PS4 non-xgen fn, when xgen is enabled for PS4.");
        return nullptr;
    }
    auto extSessUtil = getExternalSessionServicePs4();
    return (extSessUtil ? &extSessUtil->mTrackHelper.getExternalSessionActivityTypeInfo() : nullptr);
}
/*! ************************************************************************************************/
/*! \brief return the local ps5 external session util.
    Ensures the util's owning GameManagerSlave is set. NOTE: ARSON should call this fn to use util's commands that need the owning GameManagerSlave set (see Blaze's ExternalSessionUtilPs5 for details).
***************************************************************************************************/
Blaze::GameManager::ExternalSessionUtilPs5* ArsonSlaveImpl::getExternalSessionServicePs5(ClientPlatformType platform) const
{
    return (GameManager::ExternalSessionUtilPs5*)getExternalSessionService(platform);
}
/*! ************************************************************************************************/
/*! \brief return the local external session util.
    For appropriate platforms, ensures the util's owning GameManagerSlave is set. NOTE: ARSON should call this fn to use util's commands that need the owning GameManagerSlave set (see Blaze's ExternalSessionUtilPs5 for details).
***************************************************************************************************/
Blaze::GameManager::ExternalSessionUtil* ArsonSlaveImpl::getExternalSessionService(ClientPlatformType platform) const
{
    auto* extSessUtil = (Blaze::GameManager::ExternalSessionUtil*)mGmExternalSessionServices.getUtil(platform);
    if (extSessUtil == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionService: internal test error, local ExternalSessionUtil for platform(" << ClientPlatformTypeToString(platform) << ") is nullptr.");
        return nullptr;
    }
    BlazeRpcError error;
    Blaze::GameManager::GameManagerSlaveImpl* gmComp = static_cast<Blaze::GameManager::GameManagerSlaveImpl*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, true, true, &error));
    if (gmComp == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionService: unexpected error " << Blaze::ErrorHelp::getErrorName(error) << ". Unable to assign an owning GameManager slave to our local ExternalSessionUtil, for platform(" << ClientPlatformTypeToString(platform) << ").");
    }
    // For ARSON's NP session helper calls which need this, make the local GM slave the owner of our ExternalSessionUtil
    bool isPs4xgen = ((platform == ps4) && extSessUtil->getConfig().getCrossgen());
    if ((platform == ps4) && !isPs4xgen)
    {
        ((GameManager::ExternalSessionUtilPs4*)extSessUtil)->mTrackHelper.mGameManagerSlave = gmComp;
    }
    if ((platform == ps5) || isPs4xgen)
    {
        ((GameManager::ExternalSessionUtilPs5*)extSessUtil)->mMatches.mTrackHelper.mGameManagerSlave = gmComp;
        ((GameManager::ExternalSessionUtilPs5*)extSessUtil)->mPlayerSessions.mTrackHelper.mGameManagerSlave = gmComp;
    }
    return extSessUtil;
}

const Blaze::ExternalSessionActivityTypeInfo* ArsonSlaveImpl::getExternalSessionActivityTypeInfoPs5PlayerSession(ClientPlatformType platform) const
{
    auto extSessUtil = getExternalSessionServicePs5(platform);
    return (extSessUtil ? &extSessUtil->mPlayerSessions.mTrackHelper.getExternalSessionActivityTypeInfo() : nullptr);
}

const Blaze::ExternalSessionActivityTypeInfo* ArsonSlaveImpl::getExternalSessionActivityTypeInfoPs5Match(ClientPlatformType platform) const
{
    auto extSessUtil = getExternalSessionServicePs5(platform);
    return (extSessUtil ? &extSessUtil->mMatches.mTrackHelper.getExternalSessionActivityTypeInfo() : nullptr);
}

BlazeRpcError ArsonSlaveImpl::leaveAllMpsesForUser(const UserIdentification& ident)
{
    eastl::string identLogBuf;
    if (!Blaze::GameManager::hasFirstPartyId(ident))
    {
        ERR_LOG("[ArsonSlaveImpl].leaveAllMpsesForUser: (Arson) internal test error, no valid user identification for the user(" << GameManager::externalUserIdentificationToString(ident, identLogBuf) << ") specified.");
        return ARSON_ERR_INVALID_PARAMETER;
    }
    Blaze::GameManager::ExternalSessionUtilXboxOne* extSessUtil = getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform());
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }

    XBLServices::GetMultiplayerSessionsResponse getRsp;
    BlazeRpcError err = getExternalSessionsForUser(getRsp, ident);
    if (err == XBLCLIENTSESSIONDIRECTORY_RESOURCE_NOT_FOUND || err == XBLCLIENTSESSIONDIRECTORY_NO_CONTENT)
    {
        return ERR_OK;
    }
    else if (err != ERR_OK)
    {
        return convertExternalServiceErrorToArsonError(err);//logged
    }

    Blaze::LeaveGroupExternalSessionParameters cleanupParams;
    ident.copyInto(cleanupParams.getExternalUserLeaveInfos().pull_back()->getUserIdentification());

    XBLServices::MultiplayerSessionResponse nextMps;
    for (XBLServices::GetMultiplayerSessionsResponseResults::const_iterator itr = getRsp.getResults().begin(), end = getRsp.getResults().end();
        itr != end; ++itr)
    {
        cleanupParams.getSessionIdentification().getXone().setSessionName((*itr)->getSessionRef().getName());
        cleanupParams.getSessionIdentification().getXone().setTemplateName((*itr)->getSessionRef().getTemplateName());
        BlazeRpcError sessErr = extSessUtil->leave(cleanupParams);
        if (sessErr == XBLCLIENTSESSIONDIRECTORY_RESOURCE_NOT_FOUND || sessErr == XBLCLIENTSESSIONDIRECTORY_NO_CONTENT)
        {
            sessErr = ERR_OK;
        }
        if (sessErr != ERR_OK)
        {
            ERR_LOG("[ArsonSlaveImpl].leaveAllMpsesForUser: couldn't leave user(" << GameManager::externalUserIdentificationToString(ident, identLogBuf) << ") from ext session(" << GameManager::toLogStr(cleanupParams.getSessionIdentification()) << "), error(" << ErrorHelp::getErrorName(sessErr) << ").");
            err = sessErr;
        }
    }
    return convertExternalServiceErrorToArsonError(err);
}

/*! \brief retrieves the user's available MPS's. returns XBLCLIENTSESSIONDIRECTORY_RESOURCE_NOT_FOUND if none exist */
Blaze::BlazeRpcError ArsonSlaveImpl::getExternalSessionsForUser(XBLServices::GetMultiplayerSessionsResponse& result,
    const Blaze::UserIdentification& user) const
{
    XBLServices::XBLClientSessionDirectorySlave * xblClientSessDirSlave = (XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
    if (xblClientSessDirSlave == nullptr)
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionsForUser: Failed to instantiate XBLClientSessionDirectorySlave object. " <<
            "Can't get sessions for user(" << user.getBlazeId() << ":" << user.getPlatformInfo().getExternalIds().getXblAccountId() << ").");
        return ERR_SYSTEM;
    }
    if (user.getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID)
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionsForUser: Invalid xuid for user(" << user.getBlazeId() << ":" <<
            user.getPlatformInfo().getExternalIds().getXblAccountId() << "). Can't get sessions.");
        return ERR_SYSTEM;
    }

    Blaze::GameManager::ExternalSessionUtilXboxOne* extSessUtil = getExternalSessionServiceXbox(user.getPlatformInfo().getClientPlatform());
    if (extSessUtil == nullptr)
    {
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
    }

    XBLServices::GetMultiplayerSessionsForUserRequest getSessionsReq;
    getSessionsReq.setScid(extSessUtil->mConfig.getScid());
    getSessionsReq.setXuid(eastl::string().sprintf("%" PRIu64, user.getPlatformInfo().getExternalIds().getXblAccountId()).c_str());
    getSessionsReq.getHeader().setContractVersion(extSessUtil->mConfig.getContractVersion());

    Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;
    BlazeRpcError err = xblClientSessDirSlave->getMultiplayerSessionsForUser(getSessionsReq, result, errorTdf);
    for (size_t i = 0; (extSessUtil->isRetryError(err) && (i <= extSessUtil->mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
    {
        if (ERR_OK != extSessUtil->waitSeconds(errorTdf.getRetryAfter(), "[ArsonSlaveImpl].getExternalSessionsForUser", i + 1))
            return ERR_SYSTEM;
        err = xblClientSessDirSlave->getMultiplayerSessionsForUser(getSessionsReq, result, errorTdf);
    }

    if ((err != ERR_OK) && (err != XBLCLIENTSESSIONDIRECTORY_RESOURCE_NOT_FOUND))
    {
        ERR_LOG("[ArsonSlaveImpl].getExternalSessionsForUser: Failed to get sessions for user(" << user.getBlazeId() <<
            ":" << user.getPlatformInfo().getExternalIds().getXblAccountId() << "). Error(" << ErrorHelp::getErrorName(err) << ").");
    }
    else
    {
        TRACE_LOG("[ArsonSlaveImpl].getExternalSessionsForUser: got (" << result.getResults().size() << ") sessions (" <<
            (result.getResults().empty() ? "empty results list" : result.getResults().front()->getSessionRef().getName()) <<
            "..) for user(" << user.getBlazeId() << ":" << user.getPlatformInfo().getExternalIds().getXblAccountId() << ").");
    }
    return convertExternalServiceErrorToArsonError(err);
}


bool ArsonSlaveImpl::isTestingPs4Xgen() const
{
    auto* extSessUtil = getExternalSessionService(ps4);
    return ((extSessUtil != nullptr) && extSessUtil->getConfig().getCrossgen());
}


eastl::string& ArsonSlaveImpl::toExtGameInfoLogStr(eastl::string& buf, const Blaze::GameManager::GetExternalSessionInfoMasterResponse& gameInfo)
{
    return toExtGameInfoLogStr(buf, gameInfo.getExternalSessionCreationInfo().getUpdateInfo());
}
eastl::string& ArsonSlaveImpl::toExtGameInfoLogStr(eastl::string& buf, const Blaze::ExternalSessionUpdateInfo& gameInfo)
{
    return buf.sprintf("%" PRIu64 ":%s:typ(%s)", gameInfo.getGameId(), gameInfo.getGameName(), Blaze::GameManager::GameTypeToString(gameInfo.getGameType()));
}

/*! \brief convert the error code from Blaze GameManager to arson test error */
BlazeRpcError ArsonSlaveImpl::convertGameManagerErrorToArsonError(BlazeRpcError err)
{
    switch (err)
    {
    case Blaze::ERR_OK:
    case Blaze::ERR_COULDNT_CONNECT:
        return err;

    case Blaze::GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
        return ARSON_ERR_EXTERNAL_SERVICE_BUSY;

    case Blaze::GAMEMANAGER_ERR_INVALID_GAME_ID:
        return ARSON_ERR_INVALID_GAME_ID;

        //TODO: fill in more as appropriate here
    default:
        return err;
    }
}

/*! \brief convert the direct ExternalSessionUtil/1st-party RPC error code to arson test error.  For ARSON internal calls to 1st party. Avoid passing other GAMEMANAGER_ERRs. */
BlazeRpcError ArsonSlaveImpl::convertExternalServiceErrorToArsonError(BlazeRpcError err)
{
    switch (err)
    {

    /////////
    // This section mirroring stock Blaze GameManager::convertExternalServiceErrorToGameManagerError():

    case Blaze::ERR_OK:
    case Blaze::ERR_COULDNT_CONNECT:
        return err;

    case Blaze::GAMEMANAGER_ERR_EXTERNAL_SESSION_IMAGE_INVALID:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_VISIBLE:
    case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_PRIVATE:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_FOLLOWED:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::PSNBASEURL_TOO_MANY_REQUESTS:
    case Blaze::PSNSESSIONINVITATION_TOO_MANY_REQUESTS:
    case Blaze::PSNSESSIONMANAGER_TOO_MANY_REQUESTS:
    case Blaze::PSNMATCHES_TOO_MANY_REQUESTS:
    case Blaze::GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
        return ARSON_ERR_EXTERNAL_SERVICE_BUSY;

    case Blaze::GAMEMANAGER_ERR_EXTERNAL_SESSION_CUSTOM_DATA_TOO_LARGE:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;

    case Blaze::OAUTH_ERR_INVALID_SANDBOX_ID:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;


    //////////
    // Arson specific:
        
    case Blaze::XBLSERVICECONFIGS_RESOURCE_NOT_FOUND:
    case Blaze::XBLCLIENTSESSIONDIRECTORY_RESOURCE_NOT_FOUND:
    case Blaze::PSNBASEURL_RESOURCE_NOT_FOUND:
    case Blaze::PSNSESSIONINVITATION_RESOURCE_NOT_FOUND:
    case Blaze::PSNMATCHES_RESOURCE_NOT_FOUND:
    case Blaze::PSNSESSIONMANAGER_RESOURCE_NOT_FOUND:
        return ARSON_ERR_EXTERNAL_SESSION_NOT_FOUND;

    case Blaze::XBLSERVICECONFIGS_SERVICE_UNAVAILABLE:
    case Blaze::XBLCLIENTSESSIONDIRECTORY_SERVICE_UNAVAILABLE:
    case Blaze::PSNBASEURL_SERVICE_UNAVAILABLE:
    case Blaze::PSNSESSIONINVITATION_SERVICE_UNAVAILABLE:
    case Blaze::PSNMATCHES_SERVICE_UNAVAILABLE:
    case Blaze::PSNSESSIONMANAGER_SERVICE_UNAVAILABLE:
        return ARSON_ERR_EXTERNAL_SESSION_SERVICE_UNAVAILABLE;

    case Blaze::XBLSERVICECONFIGS_SERVICE_INTERNAL_ERROR:
    case Blaze::XBLCLIENTSESSIONDIRECTORY_SERVICE_INTERNAL_ERROR:
    case Blaze::PSNBASEURL_SERVICE_INTERNAL_ERROR:
    case Blaze::PSNSESSIONINVITATION_SERVICE_INTERNAL_ERROR:
    case Blaze::PSNMATCHES_SERVICE_INTERNAL_ERROR:
    case Blaze::PSNSESSIONMANAGER_SERVICE_INTERNAL_ERROR:
        return ARSON_ERR_EXTERNAL_SESSION_SERVICE_INTERNAL_ERROR;

    case Blaze::XBLSERVICECONFIGS_BAD_REQUEST:
    case Blaze::XBLCLIENTSESSIONDIRECTORY_BAD_REQUEST:
    case Blaze::PSNBASEURL_BAD_REQUEST:
    case Blaze::PSNSESSIONINVITATION_BAD_REQUEST:
    case Blaze::PSNMATCHES_BAD_REQUEST:
    case Blaze::PSNSESSIONMANAGER_BAD_REQUEST:
        return ARSON_ERR_EXTERNAL_SESSION_BAD_REQUEST;

    case Blaze::XBLSERVICECONFIGS_ACCESS_FORBIDDEN:
    case Blaze::XBLCLIENTSESSIONDIRECTORY_ACCESS_FORBIDDEN:
    case Blaze::PSNBASEURL_ACCESS_FORBIDDEN:
    case Blaze::PSNSESSIONINVITATION_ACCESS_FORBIDDEN:
    case Blaze::PSNMATCHES_ACCESS_FORBIDDEN:
    case Blaze::PSNSESSIONMANAGER_ACCESS_FORBIDDEN:
        return ARSON_ERR_EXTERNAL_SESSION_ACCESS_FORBIDDEN;

    case Blaze::XBLSERVICECONFIGS_CONFLICTING_REQUEST:
    case Blaze::PSNSESSIONINVITATION_CONFLICTING_REQUEST:
    case Blaze::PSNMATCHES_CONFLICTING_REQUEST:
    case Blaze::PSNSESSIONMANAGER_CONFLICTING_REQUEST:
        return ARSON_ERR_EXTERNAL_SESSION_CONFLICTING_REQUEST;

    case ARSON_ERR_EXTERNAL_SESSION_FAILED:
        return ARSON_ERR_EXTERNAL_SESSION_FAILED;
    case ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC:
        return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;

    // Try -> GM -> Arson conversion
    // (No direct party proxy error to arson error mapping?)
    default:
    {
        auto gmErr = GameManager::convertExternalServiceErrorToGameManagerError(err);
        if (gmErr != ERR_SYSTEM)
        {
            err = gmErr;
        }
        auto arsonGmErr = convertGameManagerErrorToArsonError(err);
        if (arsonGmErr != ERR_SYSTEM)
        {
            err = arsonGmErr;
        }
        return err;
    }
    }
}

} // Arson
} // Blaze
