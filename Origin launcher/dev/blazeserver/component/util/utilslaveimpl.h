/*************************************************************************************************/
/*!
    \file   utilimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_UTILIMPL_H
#define BLAZE_UTILIMPL_H

/*** Include files *******************************************************************************/
// global includes
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/string.h"

// framework includes
#include "framework/database/preparedstatement.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/metrics/metrics.h"
// util includes
#include "util/rpc/utilslave_stub.h"
#include "util/rpc/utilslave/usersettingsloadallforuserid_stub.h"
#include "util/tdf/utiltypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

namespace Util
{

struct AccessibilityMetrics
{
    Metrics::TaggedCounter<ClientPlatformType> SttEventCount;
    Metrics::TaggedCounter<ClientPlatformType> SttEmptyResultCount;
    Metrics::TaggedCounter<ClientPlatformType> SttErrorCount;
    Metrics::TaggedCounter<ClientPlatformType> SttDelay;
    Metrics::TaggedCounter<ClientPlatformType> SttDurationMsSent;
    Metrics::TaggedCounter<ClientPlatformType> SttCharCountRecv;
    Metrics::TaggedCounter<ClientPlatformType> TtsEventCount;
    Metrics::TaggedCounter<ClientPlatformType> TtsEmptyResultCount;
    Metrics::TaggedCounter<ClientPlatformType> TtsErrorCount;
    Metrics::TaggedCounter<ClientPlatformType> TtsDelay;
    Metrics::TaggedCounter<ClientPlatformType> TtsCharCountSent;
    Metrics::TaggedCounter<ClientPlatformType> TtsDurationMsRecv;

    AccessibilityMetrics(Metrics::MetricsCollection& metricsCollection):
        SttEventCount(metricsCollection, "sttEventCount", Metrics::Tag::platform),
        SttEmptyResultCount(metricsCollection, "sttEmptyResultCount", Metrics::Tag::platform),
        SttErrorCount(metricsCollection, "sttErrorCount", Metrics::Tag::platform),
        SttDelay(metricsCollection, "sttDelay", Metrics::Tag::platform),
        SttDurationMsSent(metricsCollection, "sttDurationMsSent", Metrics::Tag::platform),
        SttCharCountRecv(metricsCollection, "sttCharCountRecv", Metrics::Tag::platform),
        TtsEventCount(metricsCollection, "ttsEventCount", Metrics::Tag::platform),
        TtsEmptyResultCount(metricsCollection, "ttsEmptyResultCount", Metrics::Tag::platform),
        TtsErrorCount(metricsCollection, "ttsErrorCount", Metrics::Tag::platform),
        TtsDelay(metricsCollection, "ttsDelay", Metrics::Tag::platform),
        TtsCharCountSent(metricsCollection, "ttsCharCountSent", Metrics::Tag::platform),
        TtsDurationMsRecv(metricsCollection, "ttsDurationMsRecv", Metrics::Tag::platform)
    {
        SttEventCount.defineTagGroups({ { Metrics::Tag::platform } });
        SttEmptyResultCount.defineTagGroups({ { Metrics::Tag::platform } });
        SttErrorCount.defineTagGroups({ { Metrics::Tag::platform } });
        SttDelay.defineTagGroups({ { Metrics::Tag::platform } });
        SttDurationMsSent.defineTagGroups({ { Metrics::Tag::platform } });
        SttCharCountRecv.defineTagGroups({ { Metrics::Tag::platform } });
        TtsEventCount.defineTagGroups({ { Metrics::Tag::platform } });
        TtsEmptyResultCount.defineTagGroups({ { Metrics::Tag::platform } });
        TtsErrorCount.defineTagGroups({ { Metrics::Tag::platform } });
        TtsDelay.defineTagGroups({ { Metrics::Tag::platform } });
        TtsCharCountSent.defineTagGroups({ { Metrics::Tag::platform } });
        TtsDurationMsRecv.defineTagGroups({ { Metrics::Tag::platform } });
    }
};

class UtilSlaveImpl : public UtilSlaveStub
{
    NON_COPYABLE(UtilSlaveImpl);

public:
    static const uint32_t MAX_KEYS = 8;
    static const uint32_t MAX_TELEM_DOMAIN_LEN = 64;

    typedef enum QueryId
    {
        USER_SETTINGS_LOAD,
        USER_SETTINGS_SAVE,
        QUERY_MAXSIZE
    } QueryId;

public:
    UtilSlaveImpl();
    ~UtilSlaveImpl() override;

    uint32_t getMaxKeys() const { return getConfig().getUserSmallStorage().getMaxkeys();}
    PreparedStatementId getQueryId(uint32_t dbId, QueryId id) const;

    // Component state change functions.
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onPrepareForReconfigure(const UtilConfig& config) override;
    bool onResolve() override;
    void onShutdown() override;

    bool onValidateConfig(UtilConfig& config, const UtilConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    //public helper
    bool readUserSettingsForUserIdFromDb(BlazeId blazeId, Blaze::Util::UserSettingsLoadAllResponse* response);

    BlazeRpcError getUserOptionsFromDb(Blaze::Util::UserOptions &response);
    BlazeRpcError setUserOptionsFromDb(const Blaze::Util::UserOptions &request);
    BlazeRpcError postAuthExc(PostAuthRequest& request, PostAuthResponse &response, const Command* callingCommand);
    uint16_t getDbSchemaVersion() const override { return 3; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    bool requiresDbForPlatform(ClientPlatformType platform) const override { return true; }

    void getStatusInfo(ComponentStatus& status) const override;

private:
    typedef eastl::hash_map<eastl::string, FetchConfigResponse*> ConfigsBySection;
    typedef struct QueryIds
    {
        PreparedStatementId ids[QUERY_MAXSIZE];
        PreparedStatementId& operator[](QueryId queryId)
        {
            return ids[queryId];
        }
        PreparedStatementId operator[](QueryId queryId) const
        {
            if (queryId >= QUERY_MAXSIZE)
                return INVALID_PREPARED_STATEMENT_ID;
            return ids[queryId];
        }
    } QueryIds;
    typedef eastl::hash_map<uint32_t, QueryIds> QueryIdsByDbIdMap;

    QueryIdsByDbIdMap mStatementIds{ BlazeStlAllocator("UtilSlaveImpl::mStatementIds") };

    // TODO_MC: Can this be gotten rid of in favor of the globalized UserSessionManager auditing features?
    BlazeIdSet mAuditBlazeIds;
    BlazeIdSet mTempAuditBlazeIds;

    // Ticker
    uint32_t mLastTickerServIndex;

    Metrics::TaggedCounter<Metrics::BlazeSdkVersion, Metrics::ProductName> mClientConnectCount;
    Metrics::TaggedCounter<Blaze::ClientType> mMaxPsuExceededCount;
    AccessibilityMetrics mAccessibilityMetrics;

private:
    // Helper functions.
    bool configComponent(bool reconfig = false);
    bool configUserSmallStorage(bool reconfig = false);
    bool checkTelemetryConfig(UtilConfig& config, ConfigureValidationErrors& validationErrors) const;
    bool checkTelemetryServers(const TelemetryElement& pTelElement, ConfigureValidationErrors& validationErrors) const;
    bool checkUserInGroup(const char* userIdType, const char* groupDefinition) const;
    bool checkUserInGroup(const char* userIdType, const char* groupDefinition, uint32_t uStep) const;
    bool checkTickerConfig(UtilConfig& config, ConfigureValidationErrors& validationErrors) const;
    bool configTickerConfig();
    bool configTelemetryConfig(bool isReconfigure);
    bool parseAuditPersonasConfig(const TelemetryData& config, BlazeIdSet& auditBlazeIds);
    uint32_t generateMachineId(const PreAuthRequest &request, const Message* message);

    size_t telemetrySecretCreate(
            const char *pSecret, 
            uint32_t uAddr, 
            const char *pPers, 
            const char *pUid, 
            const char *pDP, 
            uint32_t 
            uTime, 
            int32_t uLocality, 
            char *pOut, 
            int32_t iLen);
    bool encodeTelemetrySessionID(char8_t *pOut, int32_t iLen, uint64_t uSessID);

    //version check
    void getValidVersionList(const char8_t* versionStr, uint32_t* version, uint32_t length);
    bool isOlderBlazeServerVersion(const char8_t* blazeSdkVersion, const char8_t* serverVersion);



    
    // RPC Calls.
    FetchClientConfigError::Error processFetchClientConfig(
            const FetchClientConfigRequest &request, FetchConfigResponse &response, const Message *message) override;
    PingError::Error processPing(PingResponse& response, const Message *message) override;
    SetClientDataError::Error processSetClientData(const ClientData &request, const Message *message) override;
    LocalizeStringsError::Error processLocalizeStrings(
        const LocalizeStringsRequest &request, LocalizeStringsResponse &response, const Message *message) override;
    GetTelemetryServerError::Error processGetTelemetryServer(
            const GetTelemetryServerRequest &request,
            GetTelemetryServerResponse &response, 
            const Message* message) override;
    GetTelemetryServerError::Error processGetTelemetryServerImpl(const GetTelemetryServerRequest &request,
            GetTelemetryServerResponse &response, 
            Blaze::PeerInfo& peerInfo);
    GetTickerServerError::Error processGetTickerServer(
            GetTickerServerResponse &response, 
            const Message* message) override;
    FilterForProfanityError::Error processFilterForProfanity(const Blaze::Util::FilterUserTextResponse &request, Blaze::Util::FilterUserTextResponse &response, const Message* message) override;
    FetchQosConfigError::Error processFetchQosConfig(Blaze::QosConfigInfo& response, const Message* message) override;
    PreAuthError::Error processPreAuth(const PreAuthRequest &request, PreAuthResponse &response, const Message* message) override;
    SetClientMetricsError::Error processSetClientMetrics(const Blaze::ClientMetrics &request, const Message* message) override;
    SetClientUserMetricsError::Error processSetClientUserMetrics(const Blaze::ClientUserMetrics &request, const Message* message) override;
    SetConnectionStateError::Error processSetConnectionState(const Blaze::Util::SetConnectionStateRequest &request, const Message* message) override;
    SuspendUserPingError::Error processSuspendUserPing(const Blaze::Util::SuspendUserPingRequest &request, const Message* message) override;
    SetClientStateError::Error processSetClientState(const Blaze::ClientState &request, const Message* message) override;
};

} // Util
} // Blaze

#endif // BLAZE_UTILIMPL_H

