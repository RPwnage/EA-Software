/*************************************************************************************************/
/*!
    \file   arsonslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
#define BLAZE_ARSONCOMPONENT_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "arson/rpc/arsonslave_stub.h"
#include "arson/rpc/arsonmaster.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/controller.h"
#include "EASTL/list.h"
#include "customcomponent/arson/tdf/arson_server.h"
#include "framework/database/preparedstatement.h"
#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "component/gamemanager/externalsessions/externalsessionutilmanager.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"
#include "arson/tournamentorganizer/arsonexternaltournamentutilxboxone.h" // for compilation

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
class ExternalSessionBlazeSpecifiedCustomDataPs5;
namespace XBLServices { class HandlesGetActivityResult; class GetMultiplayerSessionsResponse; }
namespace PSNServices { class GetNpSessionResponse; } // for blazePs4NpSessionResponseToArsonResponse()
namespace PSNServices { namespace PlayerSessions { class GetPlayerSessionResponseItem; class PlayerSessionMember; } } // for blazePs5PlayerSessionResponseToArsonResponse()
namespace PSNServices { namespace Matches { class GetMatchDetailResponse; class MatchPlayer; class MatchPlayerResult; class MatchPlayerStats; class MatchTeam; class MatchTeamResult; class MatchTeamStats; } } // for blazePs5MatchResponseToArsonResponse()
namespace GameManager { class ExternalSessionUtilPs4; class ExternalSessionUtilPs5; class ExternalSessionUtilXboxOne; struct ExternalSessionChangeableBinaryDataHead; class GameManagerServerConfig;  }

namespace Arson
{

struct ExtendedData
{
    uint16_t key;
    uint16_t value;

    ExtendedData() { }
};

class SecondArsonListener : public ArsonSlaveStub, public ArsonMasterListener
{
public:
    ~SecondArsonListener() override {};
    void onVoidNotification(UserSession*) override;
    void onSlaveExceptionNotification(UserSession*) override;
    void onResponseMasterNotification(const ArsonResponse& data, UserSession *) override;
    void onVoidMasterNotification(UserSession*) override;
    MapGetError::Error processMapGet(const ExceptionMapValueRequest &request, ExceptionMapEntry &response, const Message *message) override;
    ShutdownError::Error processShutdown(const Blaze::Arson::ShutdownRequest &request, const Message* message) override
    {
        return ShutdownError::ERR_SYSTEM;
    }
    PullRemoteXmlFilesError::Error processPullRemoteXmlFiles(PullRemoteXmlFilesResponse & response, const Message* message) override
    {
        return PullRemoteXmlFilesError::ERR_SYSTEM;
    }
    virtual GetStatusError::Error getStatus(const Blaze::Arson::ShutdownRequest &request, const Message* message)
    {
        return GetStatusError::ERR_SYSTEM;
    }
};

class ArsonSlaveImpl : public ArsonSlaveStub,
    private ArsonMasterListener,
    private ArsonSlaveStub::ExceptionEntriesMap::MapFactorySubscriber,
    private ArsonSlaveStub::ExceptionEntriesMap::Subscriber,
    private ExtendedDataProvider, private BlazeControllerMasterListener,
    private LocalUserSessionSubscriber
{
public:
    //Redis& getRedis() { return mRedis; }
    ArsonSlaveImpl();
    ~ArsonSlaveImpl() override;

    //Borrow from component\achievements\achievementsslaveimpl.cpp
    void initializeConnManager(const char8_t* serverAddress, InetAddress** inetAddress, 
        OutboundHttpConnectionManager** connManager, int32_t poolSize, const char8_t* serverDesc) const;
    // Following two sendhttpRequest wrap RestProtocolUtil::sendHttpRequest() to send http request through rest protocol
    // Encode request payload use Encoder::JSON
    BlazeRpcError sendHttpRequest(ComponentId componentId, CommandId commandId, HttpStatusCode* statusCode, const EA::TDF::Tdf* requestTdf, EA::TDF::Tdf* responseTdf = nullptr, EA::TDF::Tdf* errorTdf = nullptr);
    BlazeRpcError sendHttpRequest(ComponentId componentId, CommandId commandId, const EA::TDF::Tdf* requestTdf, EA::TDF::Tdf* responseTdf = nullptr, EA::TDF::Tdf* errorTdf = nullptr);
    // Encode request payload use encoderType
    BlazeRpcError sendHttpRequest(ComponentId componentId, CommandId commandId, HttpStatusCode* statusCode, const EA::TDF::Tdf* requestTdf, const char8_t* contentType, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf = nullptr);

    void onSetComponentState(const Blaze::SetComponentStateRequest& data,UserSession *associatedUserSession) override {}
    void onNotifyPrepareForReconfigure(const Blaze::PrepareForReconfigureNotification& data,UserSession *associatedUserSession) override {}
    void onNotifyPrepareForReconfigureComplete(const Blaze::PrepareForReconfigureCompleteNotification& data,UserSession *associatedUserSession) override;
    void onNotifyReconfigure(const Blaze::ReconfigureNotification& data,UserSession *associatedUserSession) override;
    
    const ArsonConfig& getArsonConfig() const { return getConfig(); }

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

    Blaze::BlazeRpcError getXblExternalSessionParams(Blaze::ExternalUserAuthInfo& authInfo, Blaze::GameManager::GetExternalSessionInfoMasterResponse& externalSessionInfo, Blaze::XblContractVersion& contractVersion,
        Blaze::EntityId gameMmSessId, const Blaze::UserIdentification& ident, EA::TDF::ObjectType entityType);
    Blaze::BlazeRpcError getPsnExternalSessionParams(Blaze::ExternalUserAuthInfo& authInfo, Blaze::GameManager::GetExternalSessionInfoMasterResponse& externalSessionInfo,
        Blaze::GameManager::GameId gameId, const Blaze::UserIdentification& ident, ExternalSessionActivityType activityType);

    // external session util methods
    BlazeRpcError getNpSessionChangeableData(const Blaze::UserIdentification& caller, const Blaze::ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, Blaze::GameManager::ExternalSessionChangeableBinaryDataHead& resultHead, Blaze::ExternalSessionCustomData* resultTitle, ExternalSessionErrorInfo* errorInfo) const;
    BlazeRpcError getNpSession(const Blaze::UserIdentification& caller, const Blaze::ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, const char8_t* language, Blaze::PSNServices::GetNpSessionResponse& sessionResult, ExternalSessionErrorInfo* errorInfo) const;
    DEFINE_ASYNC(retrieveCurrentPrimarySessionPs4(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, Blaze::ExternalSessionIdentification& currentPrimarySession, ExternalSessionErrorInfo& errorInfo));
    DEFINE_ASYNC(retrieveExternalSessionSessionData(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, const Blaze::ExternalSessionIdentification& sessionIdentification, Blaze::ExternalSessionCustomData& arsonResponse, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveExternalSessionChangeableSessionData(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, const Blaze::ExternalSessionIdentification& sessionIdentification, Blaze::ExternalSessionCustomData& arsonResponse, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveExternalSessionPs4(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, const Blaze::ExternalSessionIdentification& sessionIdentification, const char8_t* language, Blaze::Arson::ArsonNpSession &arsonResponse, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveExternalPs5PlayerSession(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, const Blaze::ExternalSessionIdentification& sessionIdentification, const char8_t* language, Blaze::Arson::ArsonPsnPlayerSession& arsonResponse, Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& decodedCustomData1, Blaze::ExternalSessionCustomData& decodedCustomData2, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveCurrentPrimarySessionPs5(const Blaze::UserIdentification& ident, const Blaze::ExternalUserAuthInfo& authInfo, Blaze::ExternalSessionIdentification& currentPrimarySessionIdent, ArsonPsnPlayerSession* currentPrimarySession, Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& decodedCustomData1, Blaze::ExternalSessionCustomData& decodedCustomData2, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveExternalPs5Match(const Blaze::UserIdentification& ident, const Blaze::ExternalSessionIdentification& sessionIdentification, const char8_t* language, Blaze::Arson::ArsonPsnMatch& arsonResponse, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(retrieveCurrentPs5Matches(const Blaze::UserIdentification& ident, Blaze::Arson::GetPs5MatchResponseList& currentMatches, ExternalSessionErrorInfo& errorInfo, bool ignoreJoinFlag) const);
    DEFINE_ASYNC(clearCurrentPs5Matches(const Blaze::UserIdentification& ident, Blaze::Arson::GetPs5MatchResponseList& currentMatches, ExternalSessionErrorInfo& errorInfo) const);
    DEFINE_ASYNC(callPSNsessionManager(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo) const);
    DEFINE_ASYNC(retrieveExternalSessionXbox(const Blaze::XblSessionTemplateName& sessionTemplateName, const Blaze::XblSessionName& sessionName, const Blaze::XblContractVersion& contractVersion, Arson::GetXblMultiplayerSessionResponse &arsonResponse, const UserIdentification* onBehalfUser));
    DEFINE_ASYNC(retrieveExternalSessionXbox(const Blaze::XblSessionTemplateName& sessionTemplateName, const Blaze::XblSessionName& sessionName, const Blaze::XblContractVersion& contractVersion, Arson::GetXblMultiplayerSessionResponse &arsonResponse, const UserIdentificationList& onBehalfUsers));
    DEFINE_ASYNC(getExternalSessionXbox(const Blaze::XblContractVersion& contractVersion, const char8_t* sessionTemplateName, const char8_t* sessionName, XBLServices::MultiplayerSessionResponse& result, const UserIdentification* onBehalfUser));
    DEFINE_ASYNC(retrieveCurrentPrimarySessionXbox(const Blaze::UserIdentification& ident, XBLServices::HandlesGetActivityResult& currentPrimarySession) const);
    DEFINE_ASYNC(getGameDataFromSearchSlaves(Blaze::GameManager::GameId gameId, Blaze::GameManager::ReplicatedGameData& gameData));

    HttpConnectionManagerPtr getConnectionManagerPtr() const { return mHttpConnectionManagerPtr; }
    HttpConnectionManagerPtr getXblMpsdConnectionManagerPtr() const { return mXblMpsdConnectionManagerPtr; }
    HttpConnectionManagerPtr getXblSocialConnectionManagerPtr() const { return mXblSocialConnectionManagerPtr; }
    HttpConnectionManagerPtr getXblHandlesConnectionManagerPtr() const { return mXblHandlesConnectionManagerPtr; }
    const Blaze::GameManager::ExternalSessionUtilManager& getExternalSessionServices() const { return mGmExternalSessionServices; }
    Blaze::GameManager::ExternalSessionUtilXboxOne* getExternalSessionServiceXbox(ClientPlatformType platform) const;
    Blaze::GameManager::ExternalSessionUtilPs4* getExternalSessionServicePs4() const;
    Blaze::GameManager::ExternalSessionUtilPs5* getExternalSessionServicePs5(ClientPlatformType platform) const;
    Blaze::GameManager::ExternalSessionUtil* getExternalSessionService(ClientPlatformType platform) const;

    const Blaze::ExternalSessionActivityTypeInfo* getExternalSessionActivityTypeInfoPs4() const;
    const Blaze::ExternalSessionActivityTypeInfo* getExternalSessionActivityTypeInfoPs5PlayerSession(ClientPlatformType platform) const;
    const Blaze::ExternalSessionActivityTypeInfo* getExternalSessionActivityTypeInfoPs5Match(ClientPlatformType platform) const;
    
    static Blaze::ExternalId getExternalIdFromPlatformInfo(const Blaze::PlatformInfo& platformInfo);

    static const size_t MAX_EXTERNAL_SESSION_SERVICE_UNAVAILABLE_RETRIES = 2; //Blaze's ExternalSessionUtilXone::MAX_SERVICE_UNAVAILABLE_RETRIES
    static const uint64_t EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS = 2; // Default for Blaze's MultiplayerSessionErrorResponse::getRetryAfter()
    static bool isArsonMpsdRetryError(BlazeRpcError mpsdCallErr);
    static BlazeRpcError waitSeconds(uint64_t seconds, const char8_t* context);

    void updateSessionSubscription(UserSessionMaster *mySession, UserSessionId session, bool isSubscribed);
    void updateUserSubscription(UserSessionMaster *mySession, BlazeId blazeId, bool isSubscribed);
    void updateUserPresenceInSession(UserSessionMaster *mySession, BlazeId blazeId, bool isSubscribed);

    static bool hasHigherRateLimitRemaining(const Blaze::RateLimitInfo& a, const Blaze::RateLimitInfo& b);

    BlazeRpcError leaveAllMpsesForUser(const UserIdentification& ident);
    BlazeRpcError leaveAllNpsessionsForUser(const UserIdentification& ident) { return ERR_OK; }//possible future enhancement: implement future cleanup enhancement
    BlazeRpcError leaveAllPsnMatchesForUser(const UserIdentification& ident) { return ERR_OK; }//possible future enhancement: implement future cleanup enhancement: awaiting PSN Activity/UDS APIs in 1.00 before this cleanup ability is available.
    BlazeRpcError leaveAllPsnPlayerSessionsForUser(const UserIdentification& ident) { return ERR_OK; }//possible future enhancement: implement future cleanup enhancement
    Blaze::BlazeRpcError getExternalSessionsForUser(XBLServices::GetMultiplayerSessionsResponse& result, const Blaze::UserIdentification& user) const;

    // helper
    enum IsInPsnMatchEnum { IS_IN_PSNMATCH_YES_WITH_JOINFLAG_TRUE, IS_IN_PSNMATCH_YES_WITH_JOINFLAG_FALSE, IS_IN_PSNMATCH_NO };
    static IsInPsnMatchEnum isPlayerIdInPsnMatchTeamOtherThanTeam(const char8_t* playerId, const PSNServices::Matches::GetMatchDetailResponse& restReponse, const char8_t* otherThanPsnMatchTeamId);
    static IsInPsnMatchEnum isBlazeUserInPsnMatch(const Blaze::UserIdentification& blazeMember, const Blaze::Arson::ArsonPsnMatch& psnMatch);
    bool isTestingPs4Xgen() const;
    static eastl::string& toExtGameInfoLogStr(eastl::string& buf, const Blaze::GameManager::GetExternalSessionInfoMasterResponse& gameInfo);
    static eastl::string& toExtGameInfoLogStr(eastl::string& buf, const Blaze::ExternalSessionUpdateInfo& gameInfo);

    eastl::string& toCurrPlatformsHavingExtSessConfigLogStr(eastl::string& buf, const Blaze::GameManager::GameManagerServerConfig* gmServerConfig = nullptr) const;

    /*! \brief convert the error code from game manager to arson test error */
    static BlazeRpcError convertGameManagerErrorToArsonError(BlazeRpcError err);
    static BlazeRpcError convertExternalServiceErrorToArsonError(BlazeRpcError err);
private:

    //Component interface
    bool onConfigure() override;
    bool onResolve() override;
    bool onReconfigure() override
    {
        char8_t buf[4096];
        ArsonConfig current_config = getConfig();
        SftpConfig &sftpConfig = current_config.getSftpConfig();
        sftpConfig.setFtpPassword("<CENSORED>");
        current_config.print(buf, sizeof(buf));
        // actually log 'em
        INFO("[ArsonSlaveImpl:reconfigure] \n%s", buf);
        return true;
    }

    bool onConfigureReplication() override;
    bool onValidateConfig(ArsonConfig& config, const ArsonConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool replicationSyncComplete(uint16_t);
    void onShutdown() override;

    //extendedDataProvider interface
    BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;
    virtual bool lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const;
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override { return ERR_OK; }

    typedef eastl::hash_map<const char8_t*, ExtendedData*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > ExtendedDataMap;
    ExtendedDataMap mExtendedData;

    // Dynamic map callback
    void onCreateMap(ExceptionEntriesMap& replMap, const ExceptionReplicationReason *reason) override;
    void onDestroyMap(ExceptionEntriesMap& replMap, const ExceptionReplicationReason *reason) override;

    //Subscriber callback
    void onInsert(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason) override;
    void onUpdate(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason) override;
    void onErase(const ExceptionEntriesMap& replMap, ObjectId objectId, ReplicatedMapItem<ExceptionMapEntry>& tdfObject, const ExceptionReplicationReason *reason) override;

    MapGetError::Error processMapGet(const ExceptionMapValueRequest &request, ExceptionMapEntry &response, const Message *message) override;

    void onVoidNotification(UserSession *associatedUserSession) override;
    void onSlaveExceptionNotification(UserSession *associatedUserSession) override;
    
    void onResponseMasterNotification(const ArsonResponse& data, UserSession*) override;
    void onVoidMasterNotification(UserSession*) override;
    
    ShutdownError::Error processShutdown(const Blaze::Arson::ShutdownRequest &request, const Message* message) override;

    void espnLiveFeedFiber();
    OutboundHttpConnectionManager mEspnLiveFeedConnManager;
    OutboundHttpConnection* mEspnLiveFeedConn;

    PullRemoteXmlFilesError::Error processPullRemoteXmlFiles(PullRemoteXmlFilesResponse & response, const Message* message) override;

    //UserSessionSubscriber interface
    void onLocalUserSessionLogin(const UserSessionMaster& userSession) override { mUsers.push_back(&userSession); }
    void onLocalUserSessionLogout(const UserSessionMaster& userSession) override { mUsers.remove(&userSession); }

    void getStatusInfo(Blaze::ComponentStatus& status) const override;

    typedef eastl::list<const UserSessionMaster *> SessionList;
    SessionList mUsers;

    SecondArsonListener *mSecondArsonListener;

    PreparedStatementId mStatementId[QUERY_MAXSIZE];
    //Redis mRedis;

    InetAddress* mBytevaultAddr;

    HttpConnectionManagerPtr mHttpConnectionManagerPtr;

    bool isMpsdRetryError(BlazeRpcError mpsdCallErr);
    static void blazeXblMultiplayerSessionResponseToArsonResponse(const Blaze::XBLServices::MultiplayerSessionResponse& mpsdResult, Arson::GetXblMultiplayerSessionResponse &arsonResponse);
    static void blazePs4NpSessionResponseToArsonResponse(const Blaze::PSNServices::GetNpSessionResponse& restReponse, const char8_t* npSessionId, Arson::ArsonNpSession &arsonResponse);
    static void blazePs5PlayerSessionResponseToArsonResponse(const Blaze::PSNServices::PlayerSessions::GetPlayerSessionResponseItem& psnReponse, const char8_t* psnPlayerSessionId, Blaze::Arson::ArsonPsnPlayerSession &arsonResponse);
    static void blazePs5PlayerSessionMemberToArsonResponse(const Blaze::PSNServices::PlayerSessions::PlayerSessionMember& psnReponse, Blaze::Arson::ArsonPsnPlayerSessionMember &arsonResponse);
    static void blazePs5MatchResponseToArsonResponse(const Blaze::PSNServices::Matches::GetMatchDetailResponse& restReponse, Blaze::Arson::ArsonPsnMatch &arsonResponse);
    static void blazePs5MatchPlayerToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayer& psnReponse, Blaze::Arson::ArsonPsnMatchPlayer &arsonResponse);
    static void blazePs5MatchTeamToArsonResponse(const Blaze::PSNServices::Matches::MatchTeam& psnReponse, Blaze::Arson::ArsonPsnMatchTeam &arsonResponse);
    static void blazePs5MatchPlayerResultToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayerResult& psnReponse, Blaze::Arson::ArsonPsnMatchPlayerResult &arsonResponse);
    static void blazePs5MatchTeamResultToArsonResponse(const Blaze::PSNServices::Matches::MatchTeamResult& psnReponse, Blaze::Arson::ArsonPsnMatchTeamResult &arsonResponse);
    static void blazePs5MatchPlayerStatsToArsonResponse(const Blaze::PSNServices::Matches::MatchPlayerStats& psnReponse, Blaze::Arson::ArsonPsnMatchPlayerStats &arsonResponse);
    static void blazePs5MatchTeamStatsToArsonResponse(const Blaze::PSNServices::Matches::MatchTeamStats& psnReponse, Blaze::Arson::ArsonPsnMatchTeamStats &arsonResponse);
    
    static void blazeRspToArsonResponse(const XBLServices::TournamentRef& xblRsp, ArsonTournamentRef& arsonResponse);
    static void blazeRspToArsonResponse(const XBLServices::MultiplayerSessionRef& xblRsp, ArsonMultiplayerSessionRef& arsonResponse);
    BlazeRpcError validatePs4NpSessionResponse(const PSNServices::GetNpSessionResponse& restReponse, const char8_t* npSessionId) const;
    bool validatePs5PlayerSessionResponse(ExternalSessionErrorInfo& errorInfo, const PSNServices::PlayerSessions::GetPlayerSessionResponseItem& psnReponse, const char8_t* psnPlayersSessionId) const;
    bool validatePs5MatchResponse(ExternalSessionErrorInfo& errorInfo, const PSNServices::Matches::GetMatchDetailResponse& restReponse, const char8_t* psnMatchId) const;

    // external session service util, used to test/fetch auth tokens for the external session service.
    GameManager::ExternalSessionUtilManager mGmExternalSessionServices;
    Arson::ArsonExternalTournamentUtil *mExternalTournamentService;
    InetAddress* mPsnSessionManagerAddr;
    HttpConnectionManagerPtr mPsnSessMgrConnectionManagerPtr;
    InetAddress* mXblMpsdAddr;
    HttpConnectionManagerPtr mXblMpsdConnectionManagerPtr;
    InetAddress* mXblSocialAddr;
    HttpConnectionManagerPtr mXblSocialConnectionManagerPtr;
    InetAddress* mXblHandlesAddr;
    HttpConnectionManagerPtr mXblHandlesConnectionManagerPtr;

    bool testDecoderWithUnionUpdates(const eastl::string& testName, TdfEncoder* encoder, TdfDecoder* decoder, bool testDecodeOnlyChanges, bool actuallyMakeChanges) const;
    bool validateOldVsNewReplicatedExternalSessionData(const Blaze::GameManager::ReplicatedGameData& replicatedGameData);

    const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ArsonSlaveImpl]").c_str() : mLogPrefix.c_str()); }
    mutable eastl::string mLogPrefix;
    mutable eastl::string mUserIdentLogBuf;
};

} // Arson
} // Blaze

#endif // BLAZE_ARSONCOMPONENT_SLAVEIMPL_H

