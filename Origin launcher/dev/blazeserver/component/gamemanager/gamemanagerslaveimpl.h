/*! ************************************************************************************************/
/*!
    \file gamemanagerslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_SLAVE_IMPL_H
#define BLAZE_GAMEMANAGER_SLAVE_IMPL_H

#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionsubscriber.h" // for UserSessionSubscriber
#include "framework/tdf/userextendeddatatypes_server.h"
#include "framework/tdf/userextendeddatatypes_server.h"
#include "framework/tdf/userdefines.h"
#include "framework/userset/userset.h"
#include "framework/component/censusdata.h" // for CensusDataManager & CensusDataProvider
#include "framework/connection/inetfilter.h"
#include "framework/tdf/attributes.h"
#include "framework/redis/rediskeymap.h"
#include "framework/redis/redisresponse.h"
#include "framework/system/fiberjobqueue.h"

#include "gamemanager/rpc/gamemanagerslave_stub.h"
#include "gamemanager/rpc/gamemanagermaster.h" // for GameManager master notification listeners
#include "gamemanager/rpc/gamepackermaster.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/tdf/externalsessiontypes_server.h" //for  CheckExternalSessionRestrictionsParameters in addUserListToExternalSessionCheckRestrictionsParams
#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/externalsessions/externalsessionutilmanager.h"
#include "gamemanager/playerinfo.h"
#include "util/tdf/utiltypes.h"
#include "gamemanager/scenario.h"

namespace Blaze
{
    namespace Search 
    {
        class GetGamesRequest;
        class GetGamesResponse;
    }

    class UserSession;

    namespace GameManager
    {
    class ExternalSessionUtil;
    class ExternalUserProfileUtil;
    typedef eastl::map<InstanceId, MatchmakingStatus> InstanceIdToStatusMap;

    struct MatchmakingShardingInformation
    {
        MatchmakingShardingInformation()
            : mActiveMatchmakerCount(1), mLastRecoupTime(0)
        {}

        // trackers for mm sharding
        uint32_t mActiveMatchmakerCount;
        InstanceIdToStatusMap mStatusMap;
        TimeValue mLastRecoupTime;
    };


    class GameManagerSlaveImpl : public GameManagerSlaveStub,
        private GameManagerMasterListener,
        private UserSessionSubscriber,
        private LocalUserSessionSubscriber,
        private CensusDataProvider,
        private UserSetProvider,
        private Blaze::GamePacker::GamePackerMasterListener,
        private Blaze::Matchmaker::MatchmakerSlaveListener,
        private Blaze::Controller::RemoteServerInstanceListener,
        private Blaze::SliverSyncListener,
        private Blaze::Controller::DrainStatusCheckHandler
    {
    public:
        GameManagerSlaveImpl();
        ~GameManagerSlaveImpl() override;

        GameBrowser &getGameBrowser() { return mGameBrowser; }
        MatchmakingScenarioManager& getScenarioManager() { return mScenarioManager; }
        ExternalDataManager& getExternalDataManager() { return mExternalDataManager; }

        BlazeRpcError getNextUniqueGameBrowserSessionId(uint64_t& sessionId) const; //< Gets the next unique session id for GB.

        /* \brief return server metrics */
        void getStatusInfo(ComponentStatus& status) const override;

        uint16_t getDbSchemaVersion() const override { return 5; }

        BlazeRpcError updateDedicatedServerOverride(PlayerId playerId, GameId gameId);

        ///////////////////////////////////////////////////////////////////////////
        // UserSetProvider interface functions
        //
        /* \brief Find all UserSessions matching EA::TDF::ObjectId (fiber) */
        BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) override;

        /* \brief Find all UserIds matching EA::TDF::ObjectId (fiber) */
        BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) override;

        /* \brief Find all UserIds matching EA::TDF::ObjectId (fiber) */
        BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) override;

        /* \brief Count all UserSessions matching EA::TDF::ObjectId (fiber) */
        BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

        /* \brief Count all Users matching EA::TDF::ObjectId (fiber) */
        BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

        RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) override
        {
            // For Games and GameGroups we find the correct sliver from based on the id used:
            RoutingOptions routeTo;
            routeTo.setSliverIdentityFromKey(GameManagerMaster::COMPONENT_ID, bobjId.id);
            return routeTo;
        }

        const GameBrowser& getGameBrowser() const { return mGameBrowser; }
        const MatchmakingScenarioManager& getScenarioManager() const { return mScenarioManager; }

        MatchmakingShardingInformation &getMatchmakingShardingInformation() { return mMatchmakingShardingInformation; }

        /*! ************************************************************************************************/
        /*! \brief updates load information for given matchmaker slave
        ***************************************************************************************************/
        void updateMatchmakingSlaveStatus(const Blaze::GameManager::MatchmakingStatus& data);

        void addMMSessionForUser(UserSessionId userSessionId, Rete::ProductionId sessionId);
        void removeMMSessionForUser(UserSessionId userSessionId, Rete::ProductionId sessionId);

        void incMMSessionReqForUser(UserSessionId userSessionId);
        void decMMSessionReqForUser(UserSessionId userSessionId);

        const PlayerQosValidationDataPtr getMatchmakingQosCriteriaForUser(UserSessionId userSessionId, GameNetworkTopology networkTopology);
        void upsertMatchmakingQosCriteriaForUser(UserSessionId userSessionId, const ConnectionValidationResults& validationData);
        void removeMatchmakingQosCriteriaForUser(UserSessionId userSessionId);

        BlazeRpcError DEFINE_ASYNC_RET(fetchGamesFromSearchSlaves(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse& response) const);
        BlazeRpcError DEFINE_ASYNC_RET(fetchExternalSessionDataFromSearchSlaves(GameId gameId, ExternalSessionIdentification& externalSessionIdentification) const);

        BlazeRpcError DEFINE_ASYNC_RET(createAndJoinExternalSession(const JoinExternalSessionParameters& params, bool isResetDedicatedServer, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionIdentification& sessionIdentification));
        BlazeRpcError DEFINE_ASYNC_RET(joinExternalSession(const JoinExternalSessionParameters& params, bool joinLeader, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionIdentification& sessionIdentification));

        /*! ************************************************************************************************/
        /*! \brief leaves the specified id's from the game's external session.  Intentionally non async, as this is fire and forget.

            \param[in] params - the LeaveGroupExternalSessionParameters param. It will be owned and deleted within this method.
            \param[out] void
         ***************************************************************************************************/
        void leaveExternalSession(LeaveGroupExternalSessionParametersPtr params);
        void clearPrimaryExternalSessionForUser(ClearPrimaryExternalSessionParameters& clearParams);
        BlazeRpcError DEFINE_ASYNC_RET(checkExternalSessionJoinRestrictions(ExternalSessionIdentification& externalSessionIdentification, const JoinGameRequest& joinRequest,
            const UserJoinInfoList& userInfos));

        void populateExternalPlayersInfo(PlayerJoinData& playerJoinData, const UserIdentificationList& groupUserIds, UserJoinInfoList& usersInfo, const char8_t* groupAuthToken) const;

        void incrementTotalRequestsUpdatedForReputation() { mTotalClientRequestsUpdatedDueToPoorReputation.increment(); }
        void incExternalSessionFailureImpacting(ClientPlatformType platform) { mExternalSessionFailuresImpacting.increment(1, platform); }
        void incExternalSessionFailureNonImpacting(ClientPlatformType platform) { mExternalSessionFailuresNonImpacting.increment(1, platform); }

        const ExternalSessionUtilManager& getExternalSessionUtilManager() const { return mExternalSessionServices; }

        FiberJobQueue& getExternalSessionJobQueue() { return mExternalSessionJobQueue; }
        FiberJobQueue& getCcsJobQueue() { return mCcsJobQueue; }

        const InetFilterPtr& getExternalHostIpWhitelist() const { return mExternalHostIpWhitelist; }
        const InetFilterPtr& getInternalHostIpWhitelist() const { return mInternalHostIpWhitelist; }

        BlazeRpcError fetchAuditedUsers(const Blaze::GameManager::FetchAuditedUsersRequest& request, Blaze::GameManager::FetchAuditedUsersResponse& response);
        BlazeRpcError fetchAuditedUserData(const Blaze::GameManager::FetchAuditedUserDataRequest& request, Blaze::GameManager::FetchAuditedUserDataResponse& response);
        BlazeRpcError deleteUserAuditMetricData(BlazeId blazeId);
        BlazeRpcError getFullGameDataForBlazeObjectId(const EA::TDF::ObjectId& bobjId, Search::GetGamesResponse& resp);

        const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

        void startCreatePseudoGamesThread(const PseudoGameVariantCountMap& pseudoVariantMap);
        void cancelCreatePseudoGamesThread();

        // call ccs REST API from a fiber spawned by the slave rpc. No return value as there is no real handler.
        void callCCSAllocate(CCSAllocateRequestPtr ccsRequest);
        void callCCSFree(CCSFreeRequestPtr ccsRequest);
        void callCCSExtendLease(CCSLeaseExtensionRequestPtr ccsRequest);

        void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;
        void onSliverSyncComplete(SliverNamespace sliverNamespace, InstanceId instanceId) override;

        BlazeRpcError createGameTemplate(Command* cmd, CreateGameTemplateRequest& templateRequest, JoinGameResponse& response, EncryptedBlazeIdList* failedEncryptedIds = nullptr, bool isCreateTournamentGame = false);
        BlazeRpcError internalGetCreateGameTemplateAttributesConfig(GetTemplatesAttributesResponse& response) const;
        BlazeRpcError internalGetTemplatesAndAttributes(GetTemplatesAndAttributesResponse& response) const;
        BlazeRpcError decryptBlazeIds(PerPlayerJoinDataList& playerDataList, EncryptedBlazeIdList* failedIds);

        const UserExtendedDataIdMap& getGameManagerUserExtendedDataIdMap() const { return mGameManagerUserExtendedDataIdMap; }

        BlazeRpcError setUserScenarioVariant(const Blaze::GameManager::SetUserScenarioVariantRequest& request);
        BlazeRpcError clearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& request);

        BlazeRpcError getCensusData(CensusDataByComponent& censusData) override;
        BlazeRpcError getCensusDataInternal(GameManagerCensusData& gmCensusData, bool getAllData = true);

        RedisError testAndUpdateScenarioConcurrencyLimit(const UserGroupId& userGroupId, MatchmakingScenarioId scenarioId, int32_t limit, const TimeValue& expiryTtl, RedisResponsePtr& resp) const;
        RedisError callRemoveMMId(const UserGroupId& userGroupId, MatchmakingScenarioId scenarioId, RedisResponsePtr& resp);
        BlazeRpcError populateRequestCrossplaySettings(CreateGameMasterRequest& masterRequest);
        BlazeRpcError resetDedicatedServerInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, JoinGameResponse& response);
        BlazeRpcError createGameInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, Blaze::GameManager::CreateGameResponse &response,
            CreateGameMasterErrorInfo& errInfo, bool isPseudoGame = false);
        BlazeRpcError createOrJoinGameInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, Blaze::GameManager::JoinGameResponse &response, bool isPseudoGame = false);


        /*! ************************************************************************************************/
        /*! \brief checks & updates the reputations of the provided UserSessionIdList to see if any listed member has a poor reputation.


        \param[in] userSessionIdList - list of UserSessionIds to check.
        \return true if any user provided has a poor reputation
        ***************************************************************************************************/
        static bool updateAndCheckUserReputations(const UserSessionIdList& userSessionIdList, const UserIdentificationList& externalUserList);
        static bool updateAndCheckUserReputations(UserJoinInfoList& usersInfo);
        static bool updateAndCheckUserReputations(UserSessionInfo& userInfo);

        static uint16_t setUpDefaultTeams(TeamIdVector &teamIds, PlayerJoinData& playerJoinData, uint16_t participantCapacity);

        void obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const override;
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::ReplicatedGameData& tdf);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::NotifyGameSetup& tdf);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::NotifyPlayerJoining& tdf);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::GameBrowserMatchDataList& list);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::GameBrowserGameData& tdf);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::ListGameData& tdf);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::ReplicatedGamePlayerList& list);
        static void obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::NotifyMatchmakingPseudoSuccess& tdf);
        
        

    private:

        //Component functions
        bool onActivate() override;
        bool onResolve() override;
        bool onValidatePreconfig(GameManagerServerPreconfig& config, const GameManagerServerPreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
        bool onPreconfigure() override;
        bool onValidateConfig(GameManagerServerConfig& config, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
        bool onConfigure() override;
        bool onPrepareForReconfigure(const GameManagerServerConfig& newConfig) override;
        bool onReconfigure() override;
        void onShutdown() override;

        //DrainStatusCheckHandler interface
        bool isDrainComplete() override;

        // matchmaker component notification handlers
        void onNotifyMatchmakingFailed(const NotifyMatchmakingFailed& data, UserSession*) override;
        void onNotifyMatchmakingFinished(const NotifyMatchmakingFinished& data, UserSession*) override;
        void onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated& data, UserSession*) override;
        void onNotifyMatchmakingAsyncStatus(const NotifyMatchmakingAsyncStatus& data, UserSession*) override;
        void onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess& data, UserSession*) override;
        void onNotifyRemoteMatchmakingStarted(const NotifyRemoteMatchmakingStarted& data, UserSession*) override;
        void onNotifyRemoteMatchmakingEnded(const NotifyRemoteMatchmakingEnded& data, UserSession*) override;
        void onNotifyMatchmakingStatusUpdate(const MatchmakingStatus& data, UserSession*) override;
        void onNotifyMatchmakingDedicatedServerOverride(const NotifyMatchmakingDedicatedServerOverride& data, UserSession*) override {}
        void onNotifyServerMatchmakingReservedExternalPlayers(const NotifyServerMatchmakingReservedExternalPlayers& data, UserSession*) override;
        //{{
        // These notifications are never sent to the GM slaves, therefore these handlers should never be called.
        void onNotifyWorkerPackerSessionAvailable(const Blaze::GameManager::NotifyWorkerPackerSessionAvailable& data, ::Blaze::UserSession *associatedUserSession) override {}
        void onNotifyWorkerPackerSessionRelinquish(const Blaze::GameManager::NotifyWorkerPackerSessionRelinquish& data, ::Blaze::UserSession *associatedUserSession) override {}
        void onNotifyWorkerPackerSessionTerminated(const Blaze::GameManager::NotifyWorkerPackerSessionTerminated& data, ::Blaze::UserSession *associatedUserSession)  override {}
        //}}
        // scenario user variants notification handlers
        void onClearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& data, UserSession *associatedUserSession) override;
        void onUserScenarioVariantUpdate(const Blaze::GameManager::SetUserScenarioVariantRequest& data, UserSession *associatedUserSession) override;

        // when a GM master instance is dying, the slave needs to dispatch game destruction notifications, and clean up the object id list for the local users
        void dispatchGameDestructionNotificationsForDyingMaster(InstanceId instanceId);

        // UserSessionSubscriber impls
        void onUserSessionExtinction(const UserSession& userSession) override;

        // LocalUserSessionSubscriber impls
        void onLocalUserSessionLogout(const UserSessionMaster& userSession) override;
        void onLocalUserSessionResponsive(const UserSessionMaster& userSession) override;
        void onLocalUserSessionUnresponsive(const UserSessionMaster& userSession) override;
        void onLocalUserSessionImported(const UserSessionMaster& userSession) override;
        void onLocalUserSessionExported(const UserSessionMaster& userSession) override;

        void removePlayersFromGame(GameId gameId, bool joinLeader, const JoinExternalSessionParameters& params, const ExternalSessionApiError& apiErr);
        bool validateExternalSessionParams(const JoinExternalSessionParameters& params) const;
        void externalUserJoinInfosToUserSessionIdList(const ExternalUserJoinInfoList& externalUserInfos, UserSessionIdList& userSessionIdList) const;
        BlazeId allocateTemporaryBlazeIdForExternalUser(const UserIdentification& externalUserIdentity) const;
        void clearPrimaryExternalSessionForUserPtr(ClearPrimaryExternalSessionParametersPtr params) { clearPrimaryExternalSessionForUser(*params); }

        void setUserResponsive(const UserSessionMaster& user, bool responsive) const;


        struct ProcessedPseudoGameConfig
        {
            // CreateGameRequest mCreateRequest;
            PseudoGameVariantConfig mConfig;
        };
        eastl::map<PseudoGameVariantName, ProcessedPseudoGameConfig> mProcessedPseudoGameConfigMap; 

        void buildInternalPseudoGameConfigs();
        void buildInternalPseudoGameConfig(const PseudoGameVariantConfig* variantConfig, ProcessedPseudoGameConfig& processedConfig, const PseudoGameConfigMap& pgConfigMap);
        void buildInternalPseudoGameConfigValues(const VariableVariantValueMap& inputMap, VariableVariantValueMap& outputMap);

        void createPseudoGamesThread(const PseudoGameVariantCountMap& pseudoVariantMap);

        bool buildPseudoGameRequest(CreateGameRequest& outputReq, const PseudoGameVariantConfig& config);
        bool buildPseudoGameRequestValues(EA::TDF::TdfGenericReference criteriaRef, const VariableVariantValueMap& valueMap);
        bool validatePseudoGameConfig() const;
        bool validatePseudoGameConfigValues(EA::TDF::TdfGenericReference criteriaRef,  const VariableVariantValueMap& valueMap) const;
        
        void generatePseudoGamesThread(void);

        bool validateCreateGameTemplateConfig(GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors) const;
        
        eastl::string& makeClientInfoLogStr(eastl::string& str) const;

        BlazeRpcError whitelistCheckAddr(NetworkAddress & address, const char8_t* gameName, BlazeId blazeId);

    private:
        // Matchmaking slave owns the RuleDefinition collection
        // The definition collection must be destructed after the MatchmakingSlave and GameBrowser.
        GameBrowser mGameBrowser;

        Metrics::Counter mTotalClientRequestsUpdatedDueToPoorReputation;
        Metrics::PolledGauge mActiveMatchmakingSlaves;

        Metrics::TaggedCounter<ClientPlatformType> mExternalSessionFailuresImpacting;
        Metrics::TaggedCounter<ClientPlatformType> mExternalSessionFailuresNonImpacting;

        MatchmakingShardingInformation mMatchmakingShardingInformation;

        UserExtendedDataIdMap mGameManagerUserExtendedDataIdMap;

        // NOTE: This request count map is needed because pending requests are not tracked by mUserSessionMMSessionSlaveMap
        // meanwhile they need to count against the per/user matchmaking session limit.
        typedef eastl::hash_map<UserSessionId, uint32_t> UserSessionMMSessionReqCountMap;
        UserSessionMMSessionReqCountMap mUserSessionMMSessionReqCountMap;

        GameManager::ExternalSessionUtilManager mExternalSessionServices;

        Fiber::FiberHandle mPseudoGameCreateFiber;

        InetFilterPtr mExternalHostIpWhitelistStaging;
        InetFilterPtr mExternalHostIpWhitelist;

        InetFilterPtr mInternalHostIpWhitelistStaging;
        InetFilterPtr mInternalHostIpWhitelist;

        MatchmakingScenarioManager mScenarioManager;
        ExternalDataManager mExternalDataManager;

        FiberJobQueue mCcsJobQueue;
        FiberJobQueue mExternalSessionJobQueue;
        FiberJobQueue mCleanupGameBrowserForInstanceJobQueue;

        static RedisScript msAddScenarioIdIfUnderLimit;
        static RedisScript msRemoveMMScenariodId;

        RedisKeyMap<PlayerId, GameId> mDedicatedServerOverrideMap;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_SLAVE_IMPL_H
