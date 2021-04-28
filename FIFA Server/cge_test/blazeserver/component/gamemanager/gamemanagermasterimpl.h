/*! ************************************************************************************************/
/*!
    \file gamemanagermasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_MASTERIMPL_H
#define BLAZE_GAMEMANAGER_MASTERIMPL_H

#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/usersessions/usersessionsubscriber.h" 

#include "framework/controller/controller.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/system/timerset.h"
#include "framework/system/fiberjobqueue.h"
#include "framework/storage/storagetable.h"
#include "framework/redis/rediskeymap.h"
#include "framework/protocol/outboundhttpresult.h" // for OutboundHttpResult in ExternalTournamentEventResult

#include "gamemanager/rpc/gamemanagermaster_stub.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/gamemanagermetrics_server.h"
#include "gamemanager/gamestatehelper.h"
#include "gamemanager/gamemanagermetrics.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/scenario.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/pinutil/pinutil.h"
#include "gamemanager/externalsessions/externalsessionutilmanager.h"

#include "EASTL/map.h"
#include "EASTL/set.h"
#include "EASTL/vector_set.h"
#include "EASTL/vector_map.h"
#include "EASTL/functional.h"
#include "EATDF/tdfblobhash.h" // for tdfblobhash, tdfblob_equal_to in ExternalPlayerBlobToGameIdsMap

namespace eastl
{
    template <> 
    struct hash<EA::TDF::TdfBlob>
    {
        size_t operator()(const EA::TDF::TdfBlob& x) const
        { return EA::TDF::tdfblobhash<const EA::TDF::TdfBlob*>()(&x); }
    };

    template <>
    struct equal_to<EA::TDF::TdfBlob>
    {
        bool operator()(const EA::TDF::TdfBlob& a, const EA::TDF::TdfBlob& b) const
        { return EA::TDF::tdfblob_equal_to()(&a, &b); }
    };
}

namespace Blaze
{

    class UserSession;

namespace Util
{
    class UtilMasterImpl;
}

namespace GameManager
{
    class GameNetworkBehavior;
    class GameSessionMaster;
    struct PlayerRemovalContext;
    class HostInjectionJob;
    class PlayerInfoMaster;

    typedef RedisKeyMap<UUID, SliverId> PersistedIdOwnerMap;

    class TelemetryReportStats;

namespace Matchmaker
{
    class GroupUedExpressionList;
}


class GamePermissionSystemInternal
{
public:
    GamePermissionSystemInternal() : mEnableAutomaticAdminSelection(false) {}
    bool mEnableAutomaticAdminSelection;

    typedef eastl::set<GameAction::Actions> GameActionSet;
    GameActionSet mHost;
    GameActionSet mAdmin;
    GameActionSet mParticipant;
    GameActionSet mSpectator;
};


class GameManagerMasterImpl : 
    public GameManagerMasterStub, 
    private UserSetProvider,
    private UserSessionSubscriber,
    private Controller::DrainStatusCheckHandler
{
public:
    GameManagerMasterImpl();
    ~GameManagerMasterImpl() override;

    uint16_t getDbSchemaVersion() const override { return 5; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

    void addUserSessionGameInfo(const UserSessionId userSessionId, const GameSessionMaster& gameSession, bool isNowActive = false, UserSessionInfo* userInfo =nullptr);
    void removeUserSessionGameInfo(const UserSessionId userSessionId, const GameSessionMaster& gameSession, bool wasActive =false, UserSessionInfo* userInfo =nullptr);

    // player removals can be destructive (they can trigger game destruction or host migration)
    //   this enum lets us know when a call to removePlayer resulted in a destructive action.
    enum RemovePlayerGameSideEffect
    {
        REMOVE_PLAYER_NO_GAME_SIDE_EFFECT, // NOTE: returned for a 'regular' player removal (removing the player isn't a side effect).
        REMOVE_PLAYER_MIGRATION_TRIGGERED,
        REMOVE_PLAYER_GAME_DESTROYED
    };

    const GameSessionMaster* getReadOnlyGameSession(GameId id) const;
    GameSessionMasterPtr getWritableGameSession(GameId id, bool autoCommit = true);
    void eraseGameData(GameId gameId);

    GameStateHelper* getGameStateHelper() { return &mGameStateHelper; }

    // the following helpers are public because they're either used by the GameSessionMaster, the Matchmaker or the Dynamic DS load balancer
    CreateGameMasterError::Error createGame(const CreateGameMasterRequest& request, CreateGameMasterResponse& response, const UserSessionSetupReasonMap &setupReasons, Blaze::GameManager::CreateGameMasterErrorInfo *errorInfo = nullptr);

    BlazeRpcError resetDedicatedServer(const CreateGameMasterRequest& createGameRequest, const UserSessionSetupReasonMap &setupReasons, GameSessionMasterPtr& gInfo, JoinGameMasterResponse& joinGameResponse);

    RemovePlayerGameSideEffect removePlayer(const RemovePlayerMasterRequest* request, BlazeId kickerBlazeId,
        RemovePlayerMasterError::Error &removePlayerError, LeaveGroupExternalSessionParameters *externalSessionParams = nullptr);

    DestroyGameMasterError::Error destroyGame(GameSessionMaster* gameSessionMaster, GameDestructionReason destructionReason, PlayerId removedPlayerId = 0);

    BlazeRpcError joinPlayersToDedicatedServer( GameSessionMaster& availableGame, const CreateGameMasterRequest& masterRequest, const UserSessionSetupReasonMap &setupReasons, JoinGameMasterResponse& joinResponse);  

    GameNetworkBehavior* getGameNetworkBehavior(GameNetworkTopology id);
    GameReportingId getNextGameReportingId();
    bool getEvaluateGameProtocolVersionString() { return getConfig().getGameSession().getEvaluateGameProtocolVersionString(); }
    uint16_t getDnfListMax() { return getConfig().getGameSession().getDnfListMax(); }

    // UserSetProvider interface functions
    BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) override;
    BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) override;
    BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) override;
    BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;
    BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

    RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) override
    {
        // For Games and GameGroups we find the correct sliver from based on the id used:
        RoutingOptions routeTo;
        routeTo.setSliverIdentityFromKey(GameManagerMaster::COMPONENT_ID, bobjId.id);
        return routeTo;
    }


    // UserSessionProvider interface impls
    void onUserSessionExistence(const UserSession& userSession) override;
    void onUserSessionExtinction(const UserSession& userSession) override;

    uint16_t getQueueCapacityMax() const { return getConfig().getGameSession().getQueueCapacityMax(); }
    uint16_t getBanListMax() const { return getConfig().getGameSession().getBanListMax(); }
    int64_t getJoinTimeout() const { return getConfig().getGameSession().getJoinTimeout().getMicroSeconds(); }
    int64_t getHostMigrationTimeout() const { return getConfig().getGameSession().getHostMigrationTimeout().getMicroSeconds(); }
    int64_t getHostPendingKickTimeout() const { return getConfig().getGameSession().getHostPendingKickTimeout().getMicroSeconds(); }
    int64_t getCCSRequestIssueDelay() const { return getConfig().getGameSession().getCcsRequestIssueDelay().getMicroSeconds(); }
    int64_t getCCSBidirectionalConnLostUpdateTimeout() const { return getConfig().getGameSession().getCcsBidirectionalConnLostUpdateTimeout().getMicroSeconds(); }

    uint16_t getCCSLeaseTimeMinutes() const 
    { 
        uint16_t leaseTime = static_cast<uint16_t>(getConfig().getGameSession().getCcsLeaseTime().getSec() / 60);
        if (leaseTime < 1)
            leaseTime = 1;
        return leaseTime;
    }

    bool getEvaluateGameProtocolVersionString() const { return getConfig().getGameSession().getEvaluateGameProtocolVersionString(); }
    bool getAllowMismatchedPingSiteForVirtualizedGames() const { return getConfig().getGameSession().getAllowMismatchedPingSiteForVirtualizedGames(); }
    bool getTrustedDedicatedServerReports() const { return getConfig().getGameSession().getTrustedDedicatedServerReports(); }
    TimeValue getPendingKickTimeout() const { return getConfig().getGameSession().getPendingKickTimeout(); }
    GameSessionQueueMethod getGameSessionQueueMethod() const { return getConfig().getGameSession().getGameSessionQueueMethod(); }
    bool getClearBannedListWithReset() const { return getConfig().getGameSession().getClearBannedListWithReset(); }
    TimeValue getPlayerReservationTimeout() const { return getConfig().getGameSession().getPlayerReservationTimeout(); }
    TimeValue getDisconnectReservationTimeout() const { return getConfig().getGameSession().getDisconnectReservationTimeout(); }
    bool getEnableSpectatorDisconnectReservations() const { return getConfig().getGameSession().getEnableSpectatorDisconnectReservations(); }
    bool getEnableQueuedDisconnectReservations() const { return getConfig().getGameSession().getEnableQueuedDisconnectReservations(); }
    bool getTrustHostForTwoPlayerPeerTimeout() const { return getConfig().getGameSession().getTrustHostForTwoPlayerPeerTimeout(); }
    TimeValue getLockGamesForPreferredJoinsTimeout() const { return getConfig().getGameSession().getLockGamesForPreferredJoinsTimeout(); }
    TimeValue getMmQosValidationTimeout() const { return getConfig().getMatchmakerSettings().getQosValidationTimeout(); }
    bool getEnableNetworkAddressProtection() const { return getConfig().getGameSession().getEnableNetworkAddressProtection(); }

    const GamePermissionSystemInternal& getGamePermissionSystem(const char8_t* permissionSystemName) const;

    //! \brief return whether blaze will lock games for preferred joins
    bool getLockGamesForPreferredJoins() const { return (getLockGamesForPreferredJoinsTimeout() != 0); }

    // logging for dedicated servers
    void logDedicatedServerEvent(const char8_t* eventName, const GameSessionMaster &dedicatedServerGame, const char8_t* additionalOutput = "") const;
    void logUserInfoForDedicatedServerEvent(const char8_t* eventName, BlazeId blazeId, const PingSiteLatencyByAliasMap &userLatencyMap, const GameSessionMaster &dedicatedServerGame, const char8_t* additionalOutput = "") const;

    void incrementDemanglerHealthCheckCounter(const GameDataMaster::ClientConnectionDetails& connDetails, DemanglerConnectionHealthcheck check, GameNetworkTopology topology, const char8_t* pingSite);
    void incrementConnectionResultMetrics(const GameDataMaster::ClientConnectionDetails& connDetails, const GameSessionMaster& game, const char8_t* pingSite, PlayerRemovedReason updateReason);
    void incrementExternalReplacedMetric() { mMetrics.mExternalPlayerChangedToLoggedInPlayer.increment(); }
    void incrementExternalSessionFailuresNonImpacting(ClientPlatformType platform);

    GameSessionMasterPtr chooseAvailableDedicatedServer(GameId gameIdToReset);
    bool getDedicatedServerForHostInjection(ChooseHostForInjectionRequest& request, ChooseHostForInjectionResponse &response, GameIdsByFitScoreMap& gamesToSend);

    void addHostInjectionJob(const GameSessionMaster &virtualizedGameSession, const JoinGameMasterRequest& joinGameRequest);
    void removeHostInjectionJob(const GameId &gameId);

    void removePersistedGameIdMapping(GameSessionMaster& gameSession, bool block = false);

    size_t getNumberOfActiveGameSessions(const Metrics::TagPairList& baseQuery) const;

    void buildTeamIdRoleSizeMapFromJoinRequest(const JoinGameRequest &request, TeamIdRoleSizeMap &roleRequirements, uint16_t joiningPlayerCount) const;
    void buildTeamIndexRoleSizeMapFromJoinRequest(const JoinGameRequest &request, TeamIndexRoleSizeMap &roleRequirements, uint16_t joiningPlayerCount) const;

    typedef eastl::vector_set<GameId> GameIdSet;
    // GM Census Data methods
    void populateGameAttributeCensusData(GameManagerCensusData::GameAttributeCensusDataList& attrCensusData) const;
    void getInfoFromCensusCache(const GameIdSet& gameIdSet, GameAttributeCensusData& gameAttributeCensusData) const;
    void addGameToCensusCache(GameSessionMaster& gameSession);
    void removeGameFromCensusCache(GameSessionMaster& gameSession);
    void removeGameFromConnectionMetrics(const GameId gameId);

    void insertIntoExternalPlayerToGameIdsMap(const UserInfoData& playerInfo, GameId gameId);
    bool eraseFromExternalPlayerToGameIdsMap(const UserInfoData& playerInfo, GameId gameId);

    void handleUpdateExternalSessionProperties(GameId gameId, ExternalSessionUpdateInfoPtr origValues, ExternalSessionUpdateEventContextPtr context);
    void handleLeaveExternalSession(LeaveGroupExternalSessionParametersPtr params);
    const ExternalSessionUtilManager& getExternalSessionUtilManager() const { return mExternalSessionServices; }

    FiberJobQueue& getExternalSessionJobQueue() { return mExternalSessionJobQueue; }
    FiberJobQueue& getGameReportingRpcJobQueue() { return mGameReportingRpcJobQueue; }

    //DrainStatusCheckHandler interface
    bool isDrainComplete() override;

    bool checkPermission(const GameSessionMaster& gameSession, GameAction::Actions action, BlazeId blazeId = INVALID_BLAZE_ID) const;

    void sendQosDataToMatchmaker(GameSessionMasterPtr gameSession, GameSessionConnectionCompletePtr mmQosData);

    void updateConnectionQosMetrics(const GameDataMaster::ClientConnectionDetails& connDetails, GameId gameId, const TimeValue& termTime, PlayerRemovedReason reason, PINSubmission* pinSubmission);

    const Matchmaker::GroupUedExpressionList* getGroupUedExpressionList(const char8_t* ruleName) const; 

    bool isUserAudited(BlazeId blazeId) const {return (mAuditedUsers.find(blazeId) != mAuditedUsers.end()); }
    void addConnectionMetricsToDb(const AuditedUserDataPtr data);
    GameManagerMasterMetrics& getMetrics() { return mMetrics; }

    void updatePktReceivedMetrics(GameSessionMasterPtr gameSessionMaster, const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request);


    typedef TimerSet<GameId> GameTimerSet;
    typedef TimerSet<PlayerInfoMaster::GamePlayerIdPair> PlayerTimerSet;
    typedef TimerSet<PlayerInfoMaster::GameConnectionGroupIdPair> ConnectionGroupRedisTimerSet;

    GameTimerSet& getMigrationTimerSet() { return mMigrationTimerSet; }
    GameTimerSet& getLockForJoinsTimerSet() { return mLockForJoinsTimerSet; }
    GameTimerSet& getLockedAsBusyTimerSet() { return mLockedAsBusyTimerSet; }
    GameTimerSet& getCCSRequestIssueTimerSet() { return mCCSRequestIssueTimerSet;}
    GameTimerSet& getCCSExtendLeaseTimerSet() { return mCCSExtendLeaseTimerSet;}

    PlayerTimerSet& getPlayerJoinGameTimerSet() { return mPlayerJoinGameTimerSet; }
    PlayerTimerSet& getPlayerPendingKickTimerSet() { return mPlayerPendingKickTimerSet; }
    PlayerTimerSet& getPlayerReservationTimerSet() { return mPlayerReservationTimerSet; }
    PlayerTimerSet& getPlayerQosValidationTimerSet() { return mPlayerQosValidationTimerSet; }
    StorageTable& getGameStorageTable() { return mGameTable; }
    Fiber::FiberGroupId getQosAuditFiberGroupId() const { return mAuditUserDataGroupId; }

    uint32_t getNextCCSRequestId();
    bool isCCSConfigValid() const { return mIsValidCCSConfig;}
    // for sending MP match info events for round start/end/player leave
    void buildAndSendMatchInfoEvent(const GameSessionMaster& gameSession, const char8_t* statusString) const;
    void buildAndSendMatchLeftEvent(const GameSessionMaster& gameSession, const PlayerInfoMaster &leavingPlayer, const PlayerRemovalContext &removalContext) const;
    void buildAndSendMatchJoinEvent(const GameSessionMaster& gameSession, const UserSessionSetupReasonMap &setupReasons, const RelatedPlayerSetsMap &relatedPlayerLists) const;
    void buildMatchJoinEvent(const GameSessionMaster& gameSession, const UserSessionSetupReasonMap &setupReasons, const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission, const char8_t* trackingTag) const;
    void buildAndSendMatchJoinFailedEvent(const GameSessionMasterPtr gameSession, const JoinGameByGroupMasterRequest& request, const RelatedPlayerSetsMap &relatedPlayerLists, BlazeRpcError errorCode) const;
    void buildAndSendMatchJoinFailedEvent(const GameSessionMasterPtr gameSession, const JoinGameMasterRequest& request, const RelatedPlayerSetsMap &relatedPlayerLists, BlazeRpcError errorCode) const;

    void sendExternalTournamentGameEvent(const ExternalHttpGameEventDataPtr data, eastl::string gameEventAddress, eastl::string gameEventUri, eastl::string gameEventContentType);
    static BlazeRpcError doSendTournamentGameEvent(const ExternalHttpGameEventDataPtr data, eastl::string gameEventAddress, eastl::string gameEventUri, eastl::string gameEventContentType, uint8_t retryLimit);
    static void populateTournamentGameEventPlayer(ExternalHttpGameEventData::ExternalHttpGamePlayerEventDataList& playerDataList, SlotType slotType, const char8_t* encryptedBlazeId, BlazeId blazeId, PlayerState playerState);
    static const HttpProtocolUtil::HttpMethod getTournamentGameEventHttpMethod() { return HttpProtocolUtil::HTTP_PUT; }

    static bool isUUIDFormat(const char8_t* persistedGameId);

private:
    typedef eastl::pair<UserSessionId, BlazeId> UserSessionIdBlazeIdPair;
    typedef eastl::vector_set<UserSessionId> UserSessionIdSet;

    void buildRelatedPlayersMap(const GameSessionMaster* gameSession, const UserJoinInfoList &joiningUsers, RelatedPlayerSetsMap &joiningPlayersLists );

    void addConnectionGroupJoinFailureMap(ConnectionGroupId connectionGroupId, GameId gameId);
    void cleanConnectionGroupJoinFailureMap();

    void startConnectionGroupJoinFailureTimer();
    void cancelConnectionGroupJoinFailureTimer();
    void resetConnectionGroupJoinFailureTimer();


    void cleanUpGroupUedExpressionLists();
    bool setUpGroupUedExpressionLists();

    void checkCapacityCriteriaChanges(const SetPlayerCapacityRequest &request, GameSessionMaster& gameSessionMaster, bool& newCapacity, bool& criteriaChanged);

    void fillUserSessionSetupReasonMap(UserSessionSetupReasonMap& setupReasons, const UserJoinInfoList& usersInfo, 
                                      GameSetupReason* creatorContext, GameSetupReason* pgContext, GameSetupReason* optContext) const;

    void removeUserFromAllGames(UserSessionId userSessionId, BlazeId blazeId);
    BlazeRpcError validateAdminListUpdateRequest(const UpdateAdminListRequest &request, BlazeId sourceId, GameSessionMasterPtr &gameSessionMaster); 

    // we track available dedicated servers in a multimap for efficiency
    //      we can easily get the range of servers in a data center, and we can also easily choose
    //      randomly from all the available servers if no data center is desired
    typedef eastl::multimap<const char8_t*, GameSessionMaster*, CaseInsensitiveStringLessThan> DedicatedServerByPingSiteMap;
    void logAvailableDedicatedServerCounts(BlazeId joiningBlazeId, const char8_t* gameProtocolVersionString,
        DedicatedServerByPingSiteMap &availableDedicatedServerMap) const;

    bool onValidatePreconfig(GameManagerServerPreconfig& config, const GameManagerServerPreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onPreconfigure() override;

    bool onConfigure() override;
    bool onReconfigure() override;

    bool onValidateConfig(GameManagerServerConfig& config, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onPrepareForReconfigure(const GameManagerServerConfig& newConfig) override;
    bool onResolve() override;
    bool onActivate() override;
    void onShutdown() override;
    void scheduleValidateUserSession(UserSessionId userSessionId, BlazeId blazeId, UserSessionIdSet& validatedSessionIdSet);
    void validateUserExistence(UserSessionIdBlazeIdPair pair);
    void getStatusInfo(ComponentStatus& status) const override;
    bool getHoldIncomingRpcsUntilReadyForService() const override { return false; } // RPC hold disabled to address: GOS-29063, expect enabling in the future.

    void clearGamePermissionSystem();
    void configureGamePermissionSystem();
    bool validateGameActionList(const GameActionList& actionList, const StringBuilder& listLocation, ConfigureValidationErrors& validationErrors) const;
    bool validateGamePermissions(const GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors) const;

    void groupRequestFromCreateRequest(const CreateGameMasterRequest& createRequest, GameId gameId, JoinGameByGroupMasterRequest& groupRequest) const;

    void handleResyncExternalSessionMembers(GameId gameId);

    BlazeRpcError validateOrCreate(const CreateGameRequest& createGameRequest, SliverId curSliverId, PersistedGameId& persistedId, PersistedGameIdSecret& persistedIdSecret, GameId& existingGameId);
    void cleanupPersistedIdFromRedis(PersistedGameId persistedId, GameSessionMasterPtr/*intentionally unused*/);

    BlazeRpcError processTelemetryReport(ConnectionGroupId localConnGroupId, GameId gameId, const TelemetryReport& report, PingSiteAlias pingSite = "");

    JoinGameMasterError::Error processJoinGameMaster(const JoinGameMasterRequest& request, JoinGameMasterResponse& response, Blaze::EntryCriteriaError &entryCriteriaError, const Message *message) override;
    JoinGameByGroupMasterError::Error processJoinGameByGroupMaster(const JoinGameByGroupMasterRequest& request, JoinGameMasterResponse& response, Blaze::EntryCriteriaError& entryCriteriaError, const Message *message) override;
    ResetDedicatedServerMasterError::Error processResetDedicatedServerMaster(const CreateGameMasterRequest& request, JoinGameMasterResponse& response, const Message *message) override;
    AdvanceGameStateMasterError::Error processAdvanceGameStateMaster(const AdvanceGameStateRequest &request, const Message *message) override;
    CreateGameMasterError::Error processCreateGameMaster(const CreateGameMasterRequest& request, CreateGameMasterResponse& response, Blaze::GameManager::CreateGameMasterErrorInfo &error, const Message *message) override;
    DestroyGameMasterError::Error processDestroyGameMaster(const DestroyGameRequest &request,DestroyGameResponse& response, const Message *message) override;
    DestroyAllPseudoGamesMasterError::Error processDestroyAllPseudoGamesMaster(const Message *message) override;
    SetPlayerCapacityMasterError::Error processSetPlayerCapacityMaster(const SetPlayerCapacityRequest &request, Blaze::GameManager::SwapPlayersErrorInfo &error, const Message *message) override;
    SetPlayerAttributesMasterError::Error processSetPlayerAttributesMaster(const SetPlayerAttributesRequest &request, const Message *message) override;
    MeshEndpointsConnectedMasterError::Error processMeshEndpointsConnectedMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message) override;
    MeshEndpointsDisconnectedMasterError::Error processMeshEndpointsDisconnectedMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message) override;
    MeshEndpointsConnectionLostMasterError::Error processMeshEndpointsConnectionLostMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message) override;
    SetGameSettingsMasterError::Error processSetGameSettingsMaster(const SetGameSettingsRequest &request, const Message *message) override;
    SetPlayerCustomDataMasterError::Error processSetPlayerCustomDataMaster(const Blaze::GameManager::SetPlayerCustomDataRequest &request, const Message *message) override;
    FinalizeGameCreationMasterError::Error processFinalizeGameCreationMaster(const Blaze::GameManager::UpdateGameSessionRequest &request, const Message *message) override;
    ReplayGameMasterError::Error processReplayGameMaster(const Blaze::GameManager::ReplayGameRequest &request, const Message *message) override;
    ReturnDedicatedServerToPoolMasterError::Error processReturnDedicatedServerToPoolMaster(const Blaze::GameManager::ReturnDedicatedServerToPoolRequest &request, const Message *message) override;
    LeaveGameByGroupMasterError::Error processLeaveGameByGroupMaster(const Blaze::GameManager::LeaveGameByGroupMasterRequest &request, Blaze::GameManager::LeaveGameByGroupMasterResponse &response, const Message *message) override;
    MigrateGameMasterError::Error processMigrateGameMaster(const MigrateHostRequest &request, const Message *message) override;
    RemovePlayerMasterError::Error processRemovePlayerMaster(const Blaze::GameManager::RemovePlayerMasterRequest &request, RemovePlayerMasterResponse& response, const Message *message) override;
    UpdateGameSessionMasterError::Error processUpdateGameSessionMaster(const Blaze::GameManager::UpdateGameSessionRequest &request, const Message *message) override;
    UpdateGameHostMigrationStatusMasterError::Error processUpdateGameHostMigrationStatusMaster(const Blaze::GameManager::UpdateGameHostMigrationStatusRequest &request, const Message *message) override;
    SetGameAttributesMasterError::Error processSetGameAttributesMaster(const Blaze::GameManager::SetGameAttributesRequest &request, const Message *message) override;
    SetDedicatedServerAttributesMasterError::Error processSetDedicatedServerAttributesMaster(const Blaze::GameManager::SetDedicatedServerAttributesRequest &request, const Message *message) override;
    GetCensusDataError::Error processGetCensusData(GameManagerCensusData &response, const Message *message) override;
    AddAdminPlayerMasterError::Error processAddAdminPlayerMaster(const Blaze::GameManager::UpdateAdminListRequest &request, const Message* message) override;
    RemoveAdminPlayerMasterError::Error processRemoveAdminPlayerMaster(const Blaze::GameManager::UpdateAdminListRequest &request, const Message* message) override;
    BanPlayerMasterError::Error processBanPlayerMaster(const Blaze::GameManager::BanPlayerMasterRequest &request, Blaze::GameManager::BanPlayerMasterResponse &response, const Message* message) override;
    SwapPlayersMasterError::Error processSwapPlayersMaster(const Blaze::GameManager::SwapPlayersRequest &request, Blaze::GameManager::SwapPlayersErrorInfo &error, const Message* message) override;
    ChangeGameTeamIdMasterError::Error processChangeGameTeamIdMaster(const Blaze::GameManager::ChangeTeamIdRequest &request, const Message* message) override;
    MigrateAdminPlayerMasterError::Error processMigrateAdminPlayerMaster(const Blaze::GameManager::UpdateAdminListRequest &request, const Message* message) override;
public:
    CaptureGameManagerEnvironmentMasterError::Error processCaptureGameManagerEnvironmentMaster(Blaze::GameManager::GameCaptureResponse &response, const Message* message);
    IsGameCaptureDoneMasterError::Error processIsGameCaptureDoneMaster(const Blaze::GameManager::GameCaptureResponse &request, Blaze::GameManager::IsGameCaptureDoneResponse &response, const Message* message);
    GetRedisDumpLocationsMasterError::Error processGetRedisDumpLocationsMaster(Blaze::GameManager::RedisDumpLocationsResponse &response, const Message* message);
private:
    GetGameInfoSnapshotError::Error processGetGameInfoSnapshot(const Blaze::GameManager::GetGameInfoSnapshotRequest &request, Blaze::GameManager::GameInfo &response, const Message* message) override;
    AddQueuedPlayerToGameMasterError::Error processAddQueuedPlayerToGameMaster(const Blaze::GameManager::AddQueuedPlayerToGameRequest &request, const Message* message) override;
    DemoteReservedPlayerToQueueMasterError::Error processDemoteReservedPlayerToQueueMaster(const Blaze::GameManager::DemoteReservedPlayerToQueueRequest &request, const Message* message) override;
    RemovePlayerFromBannedListMasterError::Error processRemovePlayerFromBannedListMaster(const Blaze::GameManager::RemovePlayerFromBannedListMasterRequest &request, const Message* message) override;
    ClearBannedListMasterError::Error processClearBannedListMaster(const Blaze::GameManager::BannedListRequest &request, const Message* message) override;
    GetBannedListMasterError::Error processGetBannedListMaster( const Blaze::GameManager::BannedListRequest &request, Blaze::GameManager::BannedListResponse &response, const Message *message) override;
    UpdateGameNameMasterError::Error processUpdateGameNameMaster(const Blaze::GameManager::UpdateGameNameRequest &request, const Message *message) override;
    EjectHostMasterError::Error processEjectHostMaster(const Blaze::GameManager::EjectHostRequest &request, const Message *message) override;
    SetPresenceModeMasterError::Error processSetPresenceModeMaster(const Blaze::GameManager::SetPresenceModeRequest &request,const Blaze::Message *message) override;
    ChooseHostForInjectionError::Error processChooseHostForInjection(const Blaze::GameManager::ChooseHostForInjectionRequest &request, Blaze::GameManager::ChooseHostForInjectionResponse &response, const Blaze::Message *message) override;
    SetGameModRegisterMasterError::Error processSetGameModRegisterMaster(const Blaze::GameManager::SetGameModRegisterRequest &request, const Blaze::Message *message) override;
    SetGameEntryCriteriaMasterError::Error processSetGameEntryCriteriaMaster(const Blaze::GameManager::SetGameEntryCriteriaRequest &request, const Blaze::Message *message) override;
    PreferredJoinOptOutMasterError::Error processPreferredJoinOptOutMaster(const Blaze::GameManager::PreferredJoinOptOutMasterRequest &request, const Blaze::Message *message) override;
    ResyncExternalSessionMembersError::Error processResyncExternalSessionMembers(const Blaze::GameManager::ResyncExternalSessionMembersRequest &request, const Blaze::Message *message) override;
    SetGameResponsiveMasterError::Error processSetGameResponsiveMaster(const Blaze::GameManager::SetGameResponsiveRequest &request, const Message* message) override;
    HasGameMappedByPersistedIdMasterError::Error processHasGameMappedByPersistedIdMaster(const Blaze::GameManager::HasGameMappedByPersistedIdMasterRequest &request, Blaze::GameManager::HasGameMappedByPersistedIdMasterResponse &response, const Message* message) override;
    ReservePersistedIdMasterError::Error processReservePersistedIdMaster(const Blaze::GameManager::ReservePersistedIdMasterRequest &request, Blaze::GameManager::CreateGameMasterErrorInfo &errorResp, const Message* message) override;
    ProcessTelemetryReportError::Error processProcessTelemetryReport(const Blaze::GameManager::ProcessTelemetryReportRequest &request, const Message* message) override;
    AddUsersToConnectionMetricAuditMasterError::Error processAddUsersToConnectionMetricAuditMaster(const Blaze::GameManager::UserAuditInfoMasterRequest &request, const Message* message) override;
    RemoveUsersFromConnectionMetricAuditMasterError::Error processRemoveUsersFromConnectionMetricAuditMaster(const Blaze::GameManager::UserAuditInfoMasterRequest &request, const Message* message) override;
    FetchAuditedUsersMasterError::Error processFetchAuditedUsersMaster(Blaze::GameManager::FetchAuditedUsersResponse& response, const Message* message) override;
    RequestPlatformHostMasterError::Error processRequestPlatformHostMaster(const MigrateHostRequest &request, const Message *message) override;
    GetPSUByGameNetworkTopologyError::Error processGetPSUByGameNetworkTopology(const Blaze::GetMetricsByGeoIPDataRequest &request, GetPSUResponse& response, const Message* message) override;
    GetConnectionMetricsError::Error processGetConnectionMetrics(Blaze::GameManager::GetConnectionMetricsResponse &response, const Message* message) override;
    GetMatchmakingGameMetricsError::Error processGetMatchmakingGameMetrics(Blaze::GameManager::GetMatchmakingGameMetricsResponse& response, const Message* message) override;
    GetGameModeMetricsError::Error processGetGameModeMetrics(Blaze::GameManager::GetGameModeMetrics& response, const Message* message) override;
    CcsConnectivityResultsAvailableError::Error processCcsConnectivityResultsAvailable(const Blaze::GameManager::CCSAllocateRequestMaster &request, const Message* message) override;
    CcsLeaseExtensionResultsAvailableError::Error processCcsLeaseExtensionResultsAvailable(const Blaze::GameManager::CCSLeaseExtensionRequestMaster &request, const Message* message) override;
    GetPktReceivedMetricsError::Error processGetPktReceivedMetrics(const Blaze::GameManager::GetPktReceivedMetricsRequest &request, Blaze::GameManager::GetPktReceivedMetricsResponse &response, const Message* message) override;
    GetExternalDataSourceApiListByGameIdError::Error processGetExternalDataSourceApiListByGameId(const GetExternalDataSourceApiListRequest& request, GetExternalDataSourceApiListResponse &response, const Message* message) override;


    BlazeRpcError updateConnectionMetricAuditList(const Blaze::GameManager::UserAuditInfoMasterRequest &request, bool doAdd);
    void cleanupExpiredAudits();
    uint64_t getRedisMostRecentSaveTime();
    uint64_t getRedisOldestSaveTime();
    void populateCensusData(GameManagerCensusData& curCensus);

public:
    typedef eastl::list<eastl::pair<PlayerId, UserSessionId>> PlayerUserSessionIdList;
    void sendPINConnStatsEvent(const GameSessionMaster* gameSessionMaster, ConnectionGroupId sourceConnGroupId, GameManagerMasterMetrics::ConnStatsPINEventList& pinStatsEventList);
    void addPinEventToSubmission(const GameSessionMaster* gameSessionMaster, PlayerUserSessionIdList *playerSessionList, bool connectionSuccessful,
        RiverPoster::ConnectionEventPtr connectionEvent, GameManagerMasterMetrics::ConnStatsPINEventList& connectionStatsEventList, PINSubmissionPtr pinSubmission);

public:
    PackerFoundGameError::Error processPackerFoundGame(const Blaze::GameManager::PackerFoundGameRequest &request, Blaze::GameManager::PackerFoundGameResponse &response, const Message* message) override;

    // RPCs originating from matchmaker
    MatchmakingFoundGameError::Error processMatchmakingFoundGame(const Blaze::GameManager::MatchmakingFoundGameRequest &request, Blaze::GameManager::MatchmakingFoundGameResponse &response, const Message* message) override;
private:
    MatchmakingCreateGameError::Error processMatchmakingCreateGame(const Blaze::GameManager::MatchmakingCreateGameRequest &request, Blaze::GameManager::CreateGameMasterResponse &response, const Message* message) override;
    MatchmakingCreateGameWithPrivilegedIdError::Error processMatchmakingCreateGameWithPrivilegedId(const Blaze::GameManager::MatchmakingCreateGameRequest &request, Blaze::GameManager::CreateGameMasterResponse &response, const Message* message) override;
    GetGameSessionCountError::Error processGetGameSessionCount(Blaze::GameManager::GetGameSessionCountResponse &response, const Blaze::Message *message) override;
    GetExternalSessionInfoMasterError::Error processGetExternalSessionInfoMaster(const Blaze::GameManager::GetExternalSessionInfoMasterRequest &request, Blaze::GameManager::GetExternalSessionInfoMasterResponse &response, const Blaze::Message *message) override;
    TrackExtSessionMembershipMasterError::Error processTrackExtSessionMembershipMaster(const Blaze::GameManager::TrackExtSessionMembershipRequest &request, Blaze::GameManager::GetExternalSessionInfoMasterResponse &response, Blaze::GameManager::GetExternalSessionInfoMasterResponse &errorResponse, const Blaze::Message *message) override;
    UntrackExtSessionMembershipMasterError::Error processUntrackExtSessionMembershipMaster(const UntrackExtSessionMembershipRequest &request, UntrackExtSessionMembershipResponse &response, const Blaze::Message *message) override;
    UpdateExternalSessionStatusMasterError::Error processUpdateExternalSessionStatusMaster(const Blaze::GameManager::UpdateExternalSessionStatusRequest &request, const Blaze::Message *message) override;

    GameSessionMaster* getGameSessionMaster(GameId gameId) const;

    class ExternalTournamentEventResult : public OutboundHttpResult
    {
    public:
        ExternalTournamentEventResult() : mError(ERR_OK) { clearError(); }
        ~ExternalTournamentEventResult() override {}
        void setHttpError(BlazeRpcError err = ERR_SYSTEM) override { mError = err; }
        void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) override {}
        bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) override { return false; }
        void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override {}
        bool hasError() { return ((mError != ERR_OK) && (getHttpStatusCode() < 200 || getHttpStatusCode() >= 300)); }
        BlazeRpcError getError() const { return mError; }
        void clearError() { mError = ERR_OK; setHttpStatusCode(0); }
    private:
        BlazeRpcError mError;
    };
    static bool isTournamentEventRetryError(BlazeRpcError error, const ExternalTournamentEventResult& result);

    uint32_t getQuantizedPacketLoss(uint32_t packetLossPct) const;
    uint32_t getQuantizedLatency(uint32_t latencyMs) const;

    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    void onCommitStorageRecord(StorageRecordId recordId);

    BlazeRpcError populateRequestCrossplaySettings(CreateGameMasterRequest& masterRequest);
private:
    TimerId mAuditCleanupTimer;

    typedef eastl::map<GameNetworkTopology,GameNetworkBehavior*> GameNetworkBehaviorMap;
    GameNetworkBehaviorMap mGameNetworkBehaviorMap;
    
    // hash for GAME_PROTOCOL_VERSIONG_MATCH_ANY
    uint64_t mMatchAnyHash;

    GameStateHelper mGameStateHelper; // validates GameState's state machine transitions; validates that an RPC is allowed in a certain gameState.
    GameManagerMasterMetrics mMetrics; // healthcheck metrics for the GameManagerMaster
    PktReceivedMetricsByGameIdMap mPktReceivedMetricsByGameIdMap;

    typedef eastl::map<GameId, HostInjectionJob*> HostInjectionJobMap;
    HostInjectionJobMap mHostInjectionJobMap;
    // Helper List for tracking the pseudo games. (All destroyed at once.)
    GameIdList mPseudoGameIdList;
    void destroyAllPseudoGamesFiber(GameIdList* gameIdList);

    // GM Census Data tracks the # of games and the total # of players that have certain attrib values
    //   (defined in the GM Census cfg)
    typedef eastl::hash_map<eastl::string, CensusEntry> CensusEntryByAttrValueMap;
    typedef eastl::hash_map<eastl::string, CensusEntryByAttrValueMap> CensusByAttrNameMap;
    CensusByAttrNameMap mCensusByAttrNameMap; // NOTE: Gets rebuilt from Redis on server startup

    // the group UED expressions from the MM config are needed to ensure the same logic is used on the GM master as the search slave when selecting a team to place players on
    typedef eastl::map<RuleName, Matchmaker::GroupUedExpressionList*> GroupUedExpressionListsMap;
    GroupUedExpressionListsMap mGroupUedExpressionLists;

    GameManager::ExternalSessionUtilManager mExternalSessionServices;

    typedef eastl::hash_map<BlazeId, TimeValue> ConnectionMetricAuditMap;
    ConnectionMetricAuditMap mAuditedUsers;
    Fiber::FiberGroupId mAuditUserDataGroupId;

    void updateConnectionMetricAuditDb(ConnectionMetricAuditMap& inputData, bool doAdd);

    struct ConnectionGroupJoinFailureInfo
    {
        MeshDisconnectReceived mMeshDisconnectReceived;
        TimeValue mExpiryTime;
        GameId mGameId;
    };

    typedef eastl::map<ConnectionGroupId, ConnectionGroupJoinFailureInfo> ConnectionGroupJoinFailureMap;
    ConnectionGroupJoinFailureMap mConnectionGroupJoinFailureMap;
    TimerId mConnectionGroupJoinFailureTimer; //Note: This timer is intentionally not persisted in Redis, its only used for updating total metrics which are volatile across server restarts

    MatchmakingScenarioManager mScenarioManager;

    GamePermissionSystemInternal mGlobalPermissionSystem;
    eastl::map<eastl::string, GamePermissionSystemInternal*> mGamePermissionSystemMap;

    // NOTE: Secondary index collections below are *not* stored in Redis becuase they can be effectively rebuilt from existing information on server startup
    typedef eastl::hash_map<UserSessionId, GameIdSet> GameIdSetByUserSessionMap;
    typedef eastl::hash_map<OriginPersonaId, GameIdSet> GameIdSetByOriginPersonaIdMap;
    typedef eastl::hash_map<ExternalPsnAccountId, GameIdSet> GameIdSetByPsnAccountIdMap;
    typedef eastl::hash_map<ExternalXblAccountId, GameIdSet> GameIdSetByXblAccountIdMap;
    typedef eastl::hash_map<ExternalSwitchId, GameIdSet, EA::TDF::TdfStringHash> GameIdSetBySwitchIdMap;
    typedef eastl::hash_map<ExternalSteamAccountId, GameIdSet> GameIdSetBySteamAccountIdMap;
    typedef eastl::hash_map<ExternalStadiaAccountId, GameIdSet> GameIdSetByStadiaAccountIdMap;
    typedef eastl::hash_map<PersistedGameId, GameId, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > GameIdByPersistedIdMap;
    typedef eastl::hash_map<GameId, GameSessionMasterPtr> GameSessionMasterByIdMap;

    StorageTable mGameTable; // storage table backing and replicating the primary game map
    GameSessionMasterByIdMap mGameSessionMasterByIdMap; // primary game map, backed by StorageTable
    GameIdSetByUserSessionMap mGameIdSetByUserSessionMap; // secondary index
    GameIdSetByUserSessionMap mActiveGameIdSetByUserSessionMapsGames;  // secondary index
    GameIdSetByUserSessionMap mActiveGameIdSetByUserSessionMapsGameGroups; // secondary index
    GameIdByPersistedIdMap mGameIdByPersistedIdMap; // secondary index
    GameIdSetByOriginPersonaIdMap mGameIdSetByOriginPersonaIdMap; // secondary index
    GameIdSetByPsnAccountIdMap mGameIdSetByPsnAccountIdMap; // secondary index
    GameIdSetByXblAccountIdMap mGameIdSetByXblAccountIdMap; // secondary index
    GameIdSetBySwitchIdMap mGameIdSetBySwitchIdMap; // secondary index
    GameIdSetBySteamAccountIdMap mGameIdSetBySteamAccountIdMap; // secondary index
    GameIdSetByStadiaAccountIdMap mGameIdSetByStadiaAccountIdMap; // secondary index

    GameTimerSet mMigrationTimerSet;
    GameTimerSet mLockForJoinsTimerSet;
    GameTimerSet mLockedAsBusyTimerSet;
    GameTimerSet mCCSRequestIssueTimerSet;
    GameTimerSet mCCSExtendLeaseTimerSet;

    PlayerTimerSet mPlayerJoinGameTimerSet;
    PlayerTimerSet mPlayerPendingKickTimerSet;
    PlayerTimerSet mPlayerReservationTimerSet;
    PlayerTimerSet mPlayerQosValidationTimerSet;

    FiberJobQueue mValidateExistenceJobQueue; // Queue that validates usersessions ref'ed by games after a new game is imported
    FiberJobQueue mExternalSessionJobQueue; // Queue that completes external session operations
    FiberJobQueue mGameReportingRpcJobQueue; // Queue that completes RPCs sent to a GameReportingSlave

    PersistedIdOwnerMap mPersistedIdOwnerMap; // stores SliverId that owns a given PersistedId
    typedef eastl::hash_map<eastl::string, FiberMutex*> FiberMutexByPersistedIdMap;
    FiberMutexByPersistedIdMap mFiberMutexByPersistedIdMap;

    uint32_t mNextCCSRequestId;
    bool mIsValidCCSConfig;
    TimeValue mCensusCacheExpiry;
    GameManagerCensusData mCensusCache;
    static bool isSecretMatchUUID(const char8_t* uuid, const EA::TDF::TdfBlob& blob);
};

extern EA_THREAD_LOCAL GameManagerMasterImpl* gGameManagerMaster;

extern const char8_t PUBLIC_GAME_FIELD[];
extern const char8_t PRIVATE_GAME_FIELD[];
extern const char8_t PUBLIC_PLAYER_FIELD[];
extern const char8_t PRIVATE_PLAYER_FIELD[];
extern const char8_t PRIVATE_PLAYER_FIELD_FMT[];
extern const char8_t PUBLIC_PLAYER_FIELD_FMT[];

} // namespace GameManager
} // namespace Blaze



#endif  // BLAZE_GAMEMANAGER_MASTERIMPL_H

