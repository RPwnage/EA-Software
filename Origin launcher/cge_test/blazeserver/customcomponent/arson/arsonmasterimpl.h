/*************************************************************************************************/
/*!
    \file   arsonmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ARSONCOMPONENT_MASTERIMPL_H
#define BLAZE_ARSONCOMPONENT_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "arson/rpc/arsonmaster_stub.h"
#include "arson/tdf/arson.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "framework/database/preparedstatement.h" // For ArsonMasterImpl::mStatementId

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameManager
{
class CreateGameMasterResponse;
}

namespace Arson
{

class ArsonMasterImpl : public ArsonMasterStub
{
public:
    ArsonMasterImpl();
    ~ArsonMasterImpl() override;

private:
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onValidateConfig(ArsonConfig& config, const ArsonConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    typedef enum QueryId
    {
        TEST_DATA_SAVE,
        TEST_DATA_LOAD_SINGLE_ROW,
        TEST_DATA_LOAD_MULTI_ROW,
        QUERY_MAXSIZE
    } QueryId;

    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    PreparedStatementId getQueryId(QueryId id) const { return (id < QUERY_MAXSIZE) ? mStatementId[id] : INVALID_PREPARED_STATEMENT_ID; }
    uint16_t getDbSchemaVersion() const override { return 1; }

    GenerateMasterExceptionError::Error processGenerateMasterException(const Message *message) override;
    GenerateSlaveNotificationExceptionError::Error processGenerateSlaveNotificationException(const Message *message) override;
    SecondSlaveMasterNotificationError::Error processSecondSlaveMasterNotification(const Message *message) override;
    CreateMapMasterError::Error processCreateMapMaster(const ExceptionMapRequest &request, const Message *message) override;
    DestroyMapMasterError::Error processDestroyMapMaster(const ExceptionMapRequest &request, const Message *message) override;
    MapInsertMasterError::Error processMapInsertMaster(const ExceptionMapUpdateRequest &request, const Message *message) override;
    MapUpdateMasterError::Error processMapUpdateMaster(const ExceptionMapUpdateRequest &request, const Message *message) override;
    MapEraseMasterError::Error processMapEraseMaster(const ExceptionMapValueRequest &request, const Message *message) override;
    
    PokeMasterError::Error processPokeMaster(const ArsonRequest &request, ArsonResponse &response, ArsonError &error, const Message *message) override;
    PokeMasterVoidError::Error processPokeMasterVoid(const Message *message) override;
    CreateGameMasterError::Error processCreateGameMaster(const ArsonCreateGameRequest &request, Blaze::GameManager::CreateGameResponse &response, const Message *message) override;
    SendTestNotificationToClientMasterError::Error processSendTestNotificationToClientMaster(const Message* message) override;
    Query64BitIntMasterError::Error processQuery64BitIntMaster(const Message* message) override;

    void doCreateGameMaster(Blaze::Arson::ArsonCreateGameRequestPtr request, Blaze::GameManager::CreateGameMasterResponsePtr response);
    void sendNotificationToSlaves();

private:
    TimerId mPokeSleepTimerId;
    ArsonRequest mRequest;
    ArsonResponse mResponse;

    PreparedStatementId mStatementId[QUERY_MAXSIZE];

private:
    BlazeRpcError insert_id_query();
    BlazeRpcError fetch_id_query();
};

} // Arson
} // Blaze

#endif  // BLAZE_ARSONCOMPONENT_MASTERIMPL_H
