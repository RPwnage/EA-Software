/*! ************************************************************************************************/
/*!
    \file gamesessionmaster.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_SESSION_MASTER_H
#define BLAZE_GAMEMANAGER_GAME_SESSION_MASTER_H

#include "EASTL/intrusive_ptr.h"
#include "framework/slivers/ownedsliver.h"
#include "gamemanager/gamesession.h"
#include "gamemanager/endpointsconnectionmesh.h"
#include "gamemanager/playerrostermaster.h"
#include "gamemanager/gamesessionmetrics.h"

namespace Blaze
{

namespace GameManager
{
    class GameBrowserGameData;
    class GameNetworkBehavior;
    class GameSetupReason;
    class ExternalSessionUtilManager;
    class PlayerRosterMaster;
    class GamePermissionSystemInternal;

    typedef UserSessionGameSetupReasonMap UserSessionSetupReasonMap;

    struct CCSRequestPairEqual
    {
        CCSRequestPairEqual(CCSRequestPair requestPair) : mRequestPair(requestPair) {}
        bool operator()(const CCSRequestPair* info) const
        {
            if (info != nullptr)
            {
                return ((mRequestPair.getConsoleFirstConnGrpId() == info->getConsoleFirstConnGrpId()) && (mRequestPair.getConsoleSecondConnGrpId() == info->getConsoleSecondConnGrpId()))
                    || ((mRequestPair.getConsoleFirstConnGrpId() == info->getConsoleSecondConnGrpId()) && (mRequestPair.getConsoleSecondConnGrpId() == info->getConsoleFirstConnGrpId()));
            }
            return false;
        }
        CCSRequestPair mRequestPair;
    };

    class GameSessionMaster : public GameSession
    {
        NON_COPYABLE(GameSessionMaster);
    public:
        // construct an uninitialized GameSessionMaster instance.  You must initialize it before use.
        GameSessionMaster(GameId gameId, OwnedSliverRef& ownedSliverRef);
        GameSessionMaster(ReplicatedGameDataServer& gameSession, GameDataMaster& gameDataMaster, GameNetworkBehavior& gameNetwork, OwnedSliverRef& ownedSliverRef); // Redis read-back ctor

        // initialize given a createGame request.  Adds game to the replicated game map & initializes the player roster.
        CreateGameMasterError::Error initialize(const CreateGameMasterRequest& createGameRequest,
            const UserSessionSetupReasonMap &setupReasons, const PersistedGameId& persistedId, const PersistedGameIdSecret& persistedIdSecret, JoinExternalSessionParameters &params);

        void setResponsive(bool responsive);
        BlazeRpcError setGameState(const GameState state, bool canTransitionFromConnectionVerification = false);
        SetGameSettingsMasterError::Error setGameSettings(const GameSettings* setting);
        BlazeRpcError setGameAttributes(const Collections::AttributeMap *updatedGameAttributes, uint32_t &updatedAttributeCount);
        BlazeRpcError setDedicatedServerAttributes(const Collections::AttributeMap *updatedDSAttributes, uint32_t &updatedAttributeCount);
        SetPlayerCapacityMasterError::Error processSetPlayerCapacityMaster(const SetPlayerCapacityRequest &request, TeamDetailsList& newTeamRosters, SwapPlayersErrorInfo &errorInfo);
        SetPlayerAttributesMasterError::Error processSetPlayerAttributesMaster(const SetPlayerAttributesRequest &request, BlazeId callingPlayerId);
        SetPlayerCustomDataMasterError::Error processSetPlayerCustomDataMaster(const SetPlayerCustomDataRequest &request, BlazeId callingPlayerId);
        SwapPlayersMasterError::Error processSwapPlayersMaster(const SwapPlayersRequest &requestConst, BlazeId callingPlayerId, SwapPlayersErrorInfo &errorInfo);
        ChangeGameTeamIdMasterError::Error processChangeGameTeamIdMaster(const ChangeTeamIdRequest &request, BlazeId callingPlayerId);
        ChangeGameTeamIdMasterError::Error changeGameTeamIdMaster(TeamIndex teamIndex, TeamId newTeamId);

        const char8_t* getGameMode() const override;
        const char8_t* getPINGameModeType() const override;
        const char8_t* getPINGameType() const override;
        const char8_t* getPINGameMap() const override;

        ReplayGameMasterError::Error processReplayGameMaster(const Blaze::GameManager::ReplayGameRequest &request, BlazeId callingPlayerId);
        UpdateGameSessionMasterError::Error updateGameSession(const Blaze::GameManager::UpdateGameSessionRequest &request, BlazeId callingPlayerId, bool forceNotification = false);
        UpdateGameHostMigrationStatusMasterError::Error updateGameHostMigrationStatus(const Blaze::GameManager::UpdateGameHostMigrationStatusRequest &request, BlazeId callingPlayerId);

        FinalizeGameCreationMasterError::Error finalizeGameCreation(const Blaze::GameManager::UpdateGameSessionRequest &request, BlazeId callingPlayerId);

        void destroyGame(const GameDestructionReason& destructionReason);
        ReturnDedicatedServerToPoolMasterError::Error returnDedicatedServerToPool();

        JoinGameMasterError::Error joinGame(const JoinGameMasterRequest& request, JoinGameMasterResponse &joinGameResponse, Blaze::EntryCriteriaError &error, const GameSetupReason &playerJoinContext);
        MeshEndpointsConnectedMasterError::Error meshEndpointsConnected(const UpdateMeshConnectionMasterRequest& request);
        MeshEndpointsDisconnectedMasterError::Error meshEndpointsDisconnected(const UpdateMeshConnectionMasterRequest& request);
        MeshEndpointsConnectionLostMasterError::Error meshEndpointsConnectionLost(const UpdateMeshConnectionMasterRequest& request);
        CcsConnectivityResultsAvailableError::Error ccsConnectivityResultsAvailable(const CCSAllocateRequestMaster& request);
        CcsLeaseExtensionResultsAvailableError::Error ccsLeaseExtensionResultsAvailable(const CCSLeaseExtensionRequestMaster& request);

        RemovePlayerMasterError::Error processRemovePlayer(const RemovePlayerMasterRequest* request, BlazeId kickerBlazeId, LeaveGroupExternalSessionParameters *externalSessionParams = nullptr);

        BanPlayerMasterError::Error banUserFromGame(BlazeId bannedBlazeId, AccountId bannedAccountId, PlayerRemovedTitleContext titleContext, Blaze::GameManager::BanPlayerMasterResponse &resp);

        RemovePlayerFromBannedListMasterError::Error removePlayerFromBannedList(AccountId removedAccountId);

        void clearBannedList();

        void getBannedList(AccountIdList &accountIds);

        void setGameModRegister(GameModRegister gameModRegister) { getGameData().setGameModRegister(gameModRegister); }
        GameModRegister getGameModRegister() const { return getGameData().getGameModRegister(); }

        BlazeRpcError setGameEntryCriteria(const SetGameEntryCriteriaRequest& request);

        JoinGameByGroupMasterError::Error joinGameByGroup(const JoinGameByGroupMasterRequest& request, JoinGameMasterResponse& response,  Blaze::EntryCriteriaError& error,
            const UserSessionSetupReasonMap &setupReasons);
        JoinGameByGroupMasterError::Error joinGameByGroupMembersOnly(const JoinGameByGroupMasterRequest& request, JoinGameMasterResponse& joinGameMasterResponse, Blaze::EntryCriteriaError& error,
            const UserSessionSetupReasonMap &setupReasons);
        JoinGameByGroupMasterError::Error validateGroupJoinSetTeams(JoinGameByGroupMasterRequest& request, BlazeId leaderBlazeId) const;

        // Note: not technically an RPC yet, but it will be soon
        BlazeRpcError resetDedicatedServer(const CreateGameMasterRequest& masterRequest, const UserSessionSetupReasonMap &setupReasons, const PersistedGameId& persistedId, const PersistedGameIdSecret& persistedIdSecret);

        void setGameReportingId(GameReportingId gameReportingId);
        void clearPersistedGameIdAndSecret();
        void setAutoCommitOnRelease() { mAutoCommit = true; }

        ReplicatedGameData& getReplicatedGameSession() { return getGameData(); }
        const ReplicatedGameData& getReplicatedGameSession() const { return getGameData(); }

        bool isHostMigrationDone() const;

        bool isTeamPresentInGame(const TeamId& teamId) const;

        BlazeRpcError changePlayerState(PlayerInfoMaster* playerInfo, const PlayerState state);

        EndPointsConnectionMesh* getPlayerConnectionMesh() { return &mPlayerConnectionMesh; }
        const EndPointsConnectionMesh* getPlayerConnectionMesh() const { return &mPlayerConnectionMesh; }
        
        void setNpSessionInfo(BlazeId blazeId, const NpSessionId &npSessionid);

        bool startHostMigration(PlayerInfoMaster *nextHost, HostMigrationType hostMigrationType, PlayerRemovedReason playerRemovedReason, bool& gameDestroyed);
        void hostMigrationCompleted();

        void insertPlayer(PlayerInfoMaster& playerInfo) { mPlayerRoster.insertPlayer(playerInfo); }
        const PlayerRosterMaster *getPlayerRoster() const override { return &mPlayerRoster; }
        const char8_t *getGameStatusUrl() const { return mGameDataMaster->getGameStatusUrl(); }
        const char8_t* getGameEventAddress() const { return mGameDataMaster->getGameEventAddress(); }
        const char8_t* getGameStartEventUri() const { return mGameDataMaster->getGameStartEventUri(); }
        const char8_t* getGameEndEventUri() const { return mGameDataMaster->getGameEndEventUri(); }
        const char8_t* getGameEventContentType() const { return mGameDataMaster->getGameEventContentType(); }
        const char8_t* getTournamentId() const { return mGameDataMaster->getTournamentIdentification().getTournamentId(); }
        const char8_t* getTournamentOrganizer() const { return mGameDataMaster->getTournamentIdentification().getTournamentOrganizer(); }
        void buildAndSendTournamentGameEvent(const char8_t* eventUri, BlazeRpcError eventErrorCode) const;

        const GamePermissionSystemInternal& getGamePermissionSystem() const;
        bool hasPermission(PlayerId playerId, GameAction::Actions action) const;

        bool isAdminPlayer(PlayerId playerId) const;

        void addAdminPlayer(PlayerId playerId, PlayerId sourcePlayerId);
        void removeAdminPlayer(PlayerId playerId, PlayerId sourcePlayerId, ConnectionGroupId connectionGroupToSkip); 
        void migrateAdminPlayer(PlayerId targetPlayerId, PlayerId sourcePlayerId);
        void updateAdminPlayerList(PlayerId playerId, UpdateAdminListOperation operation, PlayerId updaterPlayerId);

        void gameStarted();
        void gameFinished(PlayerId removedPlayerId = 0);
        bool hasGameStarted() const { return mGameDataMaster->getGameStarted(); }

        bool usesTrustedGameReporting() const;

        void fillGameInfo(GameInfo& gameInfo) const;

        BlazeRpcError addPlayerFromQueue(PlayerInfoMaster& queuedPlayer, TeamIndex overrideTeamIndex = UNSPECIFIED_TEAM_INDEX, const char8_t *overrideRoleName = nullptr);
        BlazeRpcError demotePlayerToQueue(PlayerInfoMaster& queuedPlayer, bool checkForFullQueue = true);
        
        void refreshDynamicPlatformList();

        void updateGameName(const GameName &name);

        void setPresenceMode(PresenceMode presenceMode);

        bool isGameCreationFinalized() const { return mGameDataMaster->getIsGameCreationFinalized(); }

        BlazeRpcError injectHost(const UserSessionInfo& hostInfo, const GameId& hostGameId, const NetworkAddressList& hostNetworkAddressList, const char8_t* joiningProtocolVersionString);
        void ejectHost(UserSessionId hostSessionId, BlazeId hostBlazeId, bool replaceHost = false);
        void cleanUpFailedRemoteInjection();
        void assignConnectionToFirstOpenSlot(PlayerInfoMaster& player);

        // helper to pump queue if game size increases after set player capacity.
        // this is called separately because we want the game size replication to happen before the queue is updated
        void processQueueAfterSetPlayerCapacity();

        PlayerInfoMaster* replaceExternalPlayer(UserSessionId newSessionId, const UserSession& newUserData);

        void refreshPINIsCrossplayGameFlag();
        void addToCrossplayPlatformSet(ClientPlatformType platform, eastl::hash_set<ClientPlatformType>& platformSet);

        class SessionIdIterator
        {
        public:
            SessionIdIterator(const GameSessionMaster& master, bool onDedicatedSession, PlayerRosterMaster::PlayerInfoList::const_iterator itr) :
              mMaster(&master), mOnDedicatedSession(onDedicatedSession), mCurrentItr(itr)
              {
                  if (mOnDedicatedSession && !mMaster->hasDedicatedServerHost() && !mMaster->hasExternalOwner())
                  {
                      mOnDedicatedSession = false;
                  }
              }

              UserSessionId operator*() const 
              {
                  if (mOnDedicatedSession)
                  {
                      return mMaster->hasDedicatedServerHost() ? mMaster->getDedicatedServerHostSessionId() : mMaster->getExternalOwnerSessionId();
                  }
                  return (*mCurrentItr)->getPlayerSessionId();
              }
              bool operator==(const SessionIdIterator& other) const { return mMaster == other.mMaster && mOnDedicatedSession == other.mOnDedicatedSession && mCurrentItr == other.mCurrentItr; }
              bool operator!=(const SessionIdIterator& other) const { return !(*this == other); }

              SessionIdIterator& operator++()
              {
                  if (mOnDedicatedSession)
                      mOnDedicatedSession = false;
                  else
                      ++mCurrentItr;

                  return *this;
              }

        private:
            const GameSessionMaster* mMaster;
            bool mOnDedicatedSession;
            PlayerRosterMaster::PlayerInfoList::const_iterator mCurrentItr;
        };

        class SessionIdIteratorPair
        {
        public:
            SessionIdIteratorPair(const GameSessionMaster& master) : mMaster(&master)
                { const PlayerRoster* roster = master.getPlayerRoster(); roster->getPlayers(mPlayerList, PlayerRoster::ALL_PLAYERS); }
            SessionIdIterator begin(bool includeDedicatedServer = true) const { return SessionIdIterator(*mMaster, includeDedicatedServer, mPlayerList.begin()); }
            SessionIdIterator end() const { return SessionIdIterator(*mMaster, false, mPlayerList.end()); }
        private:
            PlayerRosterMaster::PlayerInfoList mPlayerList;
            const GameSessionMaster* mMaster;
        };

        SessionIdIteratorPair getSessionIdIteratorPair() const { return SessionIdIteratorPair(*this); }

        bool isLockedForPreferredJoins() const override;
        bool lockForPreferredJoins();
        void clearLockForPreferredJoins(bool sendUpdate = true);
        void clearLockedAsBusy();
        void insertToPendingPreferredJoinSet(PlayerId playerId);
        void eraseFromPendingPreferredJoinSet(PlayerId playerId);

        bool isGameAndQueueFull() const { return ((getTotalPlayerCapacity() <= mPlayerRoster.getRosterPlayerCount()) && isQueueFull()); }
        bool isQueueFull() const { return mPlayerRoster.getQueueCount() >= getQueueCapacity(); }
        static void clearLockForPreferredJoinsAndSendUpdate(GameId gameId); // called by RedisTimer from GameManagerMasterImpl
        static void clearLockedAsBusy(GameId gameId);
        static void hostMigrationTimeout(GameId gameId); // called by RedisTimer from GameManagerMasterImpl
        static void ccsRequestIssueTimerFired(GameId gameId);
        static void ccsExtendLeaseTimerFired(GameId gameId);

        void incrementQueuePumpsPending() { mGameDataMaster->setQueuePumpsPending(mGameDataMaster->getQueuePumpsPending()+1); }
        void decrementQueuePumpsPending() { uint16_t pumps = mGameDataMaster->getQueuePumpsPending(); if (pumps > 0) mGameDataMaster->setQueuePumpsPending(pumps-1); }
        bool areQueuePumpsPending() { return mGameDataMaster->getQueuePumpsPending() != 0; }
        // sets mIsAnyQueueMemberPromotable to true if there are queued players that can be promoted
        bool updatePromotabilityOfQueuedPlayers();
        bool isAnyQueueMemberPromotable();
        bool canQueuedPlayerSwapWithReservedInGamePlayer(const PlayerInfoMaster *qPlayer, PlayerId* playerToDemote = nullptr);

        // returns true if we allow a new player joining the game to try to bypass the queue (no currently queued player is eligible for the game's open slots
        bool isQueueBypassPossible(SlotType slotType) const { return (mPlayerRoster.isQueueEmptyForSlotType(slotType) || !mGameDataMaster->getIsAnyQueueMemberPromotable()); }

        bool getIsPendingResyncExternalSession() const { return mGameDataMaster->getIsPendingResyncExternalSession(); }
        void setIsPendingResyncExternalSession(bool val) { mGameDataMaster->setIsPendingResyncExternalSession(val); }
        uint32_t getTotalExternalSessionsDesynced() const { return mGameDataMaster->getTotalExternalSessionsDesynced();}
        void incrementTotalExternalSessionsDesynced() { mGameDataMaster->setTotalExternalSessionsDesynced(mGameDataMaster->getTotalExternalSessionsDesynced()+1); }

        bool isPingingHostConnection() const { return mGameDataMaster->getIsPingingHostConnection(); }
        void setIsPingingHostConnection(bool val) { mGameDataMaster->setIsPingingHostConnection(val); }

        DataSourceNameList& getDataSourceNameList() const { return mGameDataMaster->getDataSourceNameList(); }
        void setDataSourceNameList(const DataSourceNameList& dataSourceNameList) { dataSourceNameList.copyInto(mGameDataMaster->getDataSourceNameList()); }

        // matchmaking QoS validation
        void addMatchmakingQosData(const PlayerIdList &playerIds, MatchmakingSessionId matchmakingSessionId);
        void addMatchmakingQosData(const UserJoinInfoList &userJoinInfo, MatchmakingSessionId matchmakingSessionId);
        // for removal of MM qos data if a join or create fails, and MM shouldn't be notified
        void removeMatchmakingQosData(const PlayerIdList &playerIds);
        void removeMatchmakingQosData(const UserJoinInfoList &userJoinInfo);
        static bool isPerformQosValidationListEmpty(const UserJoinInfoList &userJoinInfo);

        bool updateMatchmakingQosData(PlayerId playerId, const PlayerNetConnectionStatus* status, ConnectionGroupId connectionGroupId);
        bool isConnectionGroupInMatchmakingQosDataMap(ConnectionGroupId connectionGroupId) const;
        bool isPlayerInMatchmakingQosDataMap(PlayerId playerId) const { return mGameDataMaster->getMatchmakingSessionMap().find(playerId) != mGameDataMaster->getMatchmakingSessionMap().end(); }
        void updateQosTestedPlayers(const PlayerIdList &connectedPlayers, const PlayerIdList &removedPlayers);
        
        bool isGameGroup() const { return getGameData().getGameType() == GAME_TYPE_GROUP; }
        GameDataMaster* getGameDataMaster() { return mGameDataMaster; }
        const GameDataMaster* getGameDataMaster() const { return mGameDataMaster; }
        bool shouldEndpointsConnect(ConnectionGroupId source, ConnectionGroupId target) const;
        bool isConnectionRequired(ConnectionGroupId source, ConnectionGroupId target) const;
        ClientPlatformType getClientPlatformForConnection(ConnectionGroupId connGrpId) const;


        const ExternalSessionUtilManager& getExternalSessionUtilManager() const;

        // GM_TODO: This same functionality is not available on the slave (may not be possible?)
        bool isGameInProgress() const
        {
            return (getGameState() == IN_GAME) || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == IN_GAME))
                || ((getGameState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == IN_GAME))
                || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == IN_GAME));
        }

        bool isGamePreMatch() const
        {
            return (getGameState() == PRE_GAME) || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == PRE_GAME))
                || ((getGameState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == PRE_GAME))
                || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == PRE_GAME))
                || (getGameState() == NEW_STATE) || (getGameState() == INITIALIZING) || (getGameState() == INACTIVE_VIRTUAL) || (getGameState() == CONNECTION_VERIFICATION);
        }

        bool isGamePostMatch() const
        {
            return (getGameState() == POST_GAME) || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == POST_GAME))
                || ((getGameState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == POST_GAME))
                || ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == UNRESPONSIVE) && (mGameDataMaster->getPreUnresponsiveState() == POST_GAME));
        }

        EA::TDF::TimeValue getMatchStartTime() const { return mGameDataMaster->getGameStartTime(); }
        EA::TDF::TimeValue getMatchEndTime() const { return mMatchEndTime; }
        EA::TDF::TimeValue getGameCreationTime() const { return getGameData().getCreateTime(); }

        void addToCensusEntry(CensusEntry& entry);
        void removeFromCensusEntries();
        ConnectionGroupId getPlayerConnectionGroupId(PlayerId playerId) const;

        const ExternalSessionIdentification& getExternalSessionIdentification() const { return getGameData().getExternalSessionIdentification(); }
        bool doesLeverageCCS() const;
        void addPairToPendingCCSRequests(const CCSRequestPair& requestPair);

        const char8_t* getCcsPingSite() const
        {
            return mCcsPingSite.c_str();
        }

        const char8_t* getBestPingSiteForConnection(ConnectionGroupId connGrpId) const;
        ReplicatedGameDataServer* getGameDataServer() { return mGameData; } // used only by GameManagerMasterImpl::upsertGameData

        const GameSessionMetricsHandler& getMetricsHandler() const { return mMetricsHandler;}
        GameSessionMetricsHandler& getMetricsHandler() { return mMetricsHandler;}

        void getExternalSessionInfoMaster(const UserIdentification &caller, GetExternalSessionInfoMasterResponse &response) const;

        // get list of players tracked as being in the game's external session
        const ExternalMemberInfoList& getTrackedExtSessionMembers(const ExternalSessionActivityTypeInfo& platformActivityType) const;
        TrackExtSessionMembershipMasterError::Error trackExtSessionMembership(const TrackExtSessionMembershipRequest &request, GetExternalSessionInfoMasterResponse &response, GetExternalSessionInfoMasterResponse &errorResponse);
        void untrackExtSessionMembership(BlazeId blazeId, ClientPlatformType platform, const ExternalSessionActivityType* type);
        void untrackAllExtSessionMemberships();

        UpdateExternalSessionStatusMasterError::Error updateExternalSessionStatus(const Blaze::GameManager::UpdateExternalSessionStatusRequest &request);

        const char8_t* getTeamName(GameManager::TeamIndex teamIndex) const;

    private:
        friend class PlayerRosterMaster;

        void notifyExistingMembersOfJoiningPlayer(const PlayerInfo &joiningPlayer, PlayerRoster::RosterType rosterType, const GameSetupReason& reason);
        void initNotifyGameSetup(NotifyGameSetup &notifyJoinGame, const GameSetupReason &reason, bool performQosValidation, UserSessionId notifySessionId = INVALID_USER_SESSION_ID, ConnectionGroupId notifyConnectionGroupId = INVALID_CONNECTION_GROUP_ID, ClientPlatformType notifyPlatform = INVALID);
        void fillOutClientSpecificHostedConnectivityInfo(HostedConnectivityInfo& replicatedInfo, ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId);
        void notifyHostedConnectivityAvailable(ConnectionGroupId console1ConnGrpId, ConnectionGroupId console2ConnGrpId, const HostedConnectivityInfo& info);
        void processSetReplicatedGameState(const GameState state);

        BlazeRpcError initGameDataCommon(const CreateGameMasterRequest& request, bool creatingNewGame = true);
        BlazeRpcError initializeGameHost(const CreateGameMasterRequest& request, const GameSetupReason &playerJoinContext);
        BlazeRpcError initializeVirtualGameHost(const CreateGameRequest& request);
        BlazeRpcError initializeExternalOwner(const CreateGameRequest& request);
        void initializeDedicatedGameHost(const UserSessionInfo& hostInfo);
        void setHostInfo(const UserSessionInfo* hostInfo);
        void virtualizeDedicatedGame();
        bool isQueueEnabled() const { return getQueueCapacity() > 0; }

        // Helpers to put players in game.
        BlazeRpcError addActivePlayer(PlayerInfoMaster* player, const GameSetupReason &playerJoinContext, bool previousTeamIndexUnspecified = false, bool wasQueued = false);
        BlazeRpcError addQueuedPlayer(PlayerInfoMaster& newPlayer, const GameSetupReason &playerJoinContext);
        BlazeRpcError addReservedPlayer(PlayerInfoMaster* newPlayer, const GameSetupReason& playerJoinContext, bool wasQueued = false);

        void processPlayerQueue();
        void processPlayerServerManagedQueue();

        void subscribePlayerRoster(UserSessionId sessionId, SubscriptionAction action);

    public:
        void processPlayerAdminManagedQueue();
        void sendGameUpdateEvent();
        ~GameSessionMaster() override;
        void sendPlayerJoinCompleted(const PlayerInfoMaster& player);

    private:
        PlayerInfoMaster* selectNewHost(HostMigrationType migrationType, PlayerRemovedReason playerRemovedReason);
        PlayerInfoMaster* selectNewHost(const PlayerRoster::PlayerInfoList& potentialHosts, ConnectionGroupId fromGroupId = INVALID_CONNECTION_GROUP_ID);

        void finishHostMigration();

        BlazeRpcError postProccessSessionOrHostUpdate();
        bool checkDisconnectReservationCondition(const PlayerRemovalContext& removeContext) const;
        RemovePlayerMasterError::Error reserveForDisconnectedPlayer(PlayerInfoMaster* player, PlayerRemovedReason reason);
        RemovePlayerMasterError::Error removePlayer(PlayerInfoMaster* player, const PlayerRemovalContext &removalContext, LeaveGroupExternalSessionParameters *externalSessionParams = nullptr, bool isRemovingAllPlayers = false);
        void removeAllPlayers(const PlayerRemovedReason& preason);
        void banAccount(AccountId banId);
        bool isAccountBanned(AccountId bannedAccount) const;
        void updateOldestBannedAccount();

        BlazeRpcError findOpenRole(const PlayerInfoMaster& player, const PlayerJoinData* groupJoinData, const RoleNameList& desiredRoles, RoleName &foundRoleName) const;
        bool validateOpenRole(const PlayerInfoMaster& player, const RoleCriteriaMap::const_iterator& roleCriteriaItr, const PlayerJoinData* groupJoinData, RoleName &foundRoleName) const;
        uint16_t countJoinersRequiringRole(TeamIndex teamIndex, const RoleName &roleName, const PlayerJoinData* groupJoinData) const;

        void generateRebuiltTeams(const TeamDetailsList &teamMembership, TeamDetailsList& newTeamRosters, uint16_t teamCapacity);
        void finalizeRebuiltTeams(const TeamDetailsList &teamMembership, TeamDetailsList& newTeamRosters, uint16_t teamCapacity);

        JoinGameMasterError::Error validatePlayerJoin(const JoinGameMasterRequest &joinGameRequest) const;
        JoinGameMasterError::Error validateNewPlayerJoin(const JoinGameMasterRequest &joinGameRequest) const;
        JoinGameMasterError::Error validateClaimReservationJoin(const PlayerInfoMaster* joiningPlayer, BlazeId joiningBlazeId) const;
        bool validateJoinReputation(const UserSessionInfo& joiningUserInfo, GameEntryType entryType, JoinMethod joinMethod) const;
        JoinGameMasterError::Error validatePlayerJoinTeamGame(const JoinGameMasterRequest &joinGameRequest) const;
        JoinGameMasterError::Error validateGameEntryCriteria(const JoinGameMasterRequest& joinGameRequest, EntryCriteriaError &entryCriteriaError, const char8_t* joiningRole = nullptr, SlotType joiningSlotType = INVALID_SLOT_TYPE) const;

        bool isSupportedJoinMethod(const JoinMethod joinMethod, const JoinGameMasterRequest & req) const;
        JoinGameMasterError::Error validateExistingPlayerJoinGame(const GameEntryType& gameEntryType, const PlayerInfoMaster &existingPlayer) const;
        JoinGameMasterError::Error joinGameAsNewPlayer(const JoinGameMasterRequest &joinGameRequest, JoinGameMasterResponse &joinGameResponse,
                                        const GameSetupReason &playerJoinContext, PlayerInfoMasterPtr& joiningPlayer);

        // returns true if the given join error would permit queueing
        bool isQueueableJoinError( JoinGameMasterError::Error joinCapacityError ) const
        {
            return (joinCapacityError == JoinGameMasterError::GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL)
                || (joinCapacityError == JoinGameMasterError::GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL)
                || (joinCapacityError == JoinGameMasterError::GAMEMANAGER_ERR_TEAM_FULL)
                || (joinCapacityError == JoinGameMasterError::GAMEMANAGER_ERR_ROLE_FULL)
                || (joinCapacityError == JoinGameMasterError::GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED);
        }

        JoinGameMasterError::Error validateJoinGameCapacities(const RoleName& requiredRole, const SlotType requestedSlotType, TeamIndex requestedTeamIndex, const BlazeId joiningPlayerId, 
                                                                             SlotType& suggestedSlotType, TeamIndex& outUnspecifiedTeamIndex, bool targetPlayerInQueue = false) const;
        JoinGameMasterError::Error validateJoinTeamGameTeamCapacities(const TeamIndexRoleSizeMap &joiningRoles, const BlazeId joiningPlayerId, uint16_t joiningPlayerCount, TeamIndex& outUnspecifiedTeamIndex, bool targetPlayerInQueue = false) const;

        JoinGameMasterError::Error joinGameClaimReservation(const JoinGameMasterRequest &joinGameRequest, const GameSetupReason &playerJoinContext, PlayerInfoMaster &joiningPlayer);

        JoinGameMasterError::Error validateJoinGameSlotType(const SlotType requestedSlotType, SlotType& suggestedSlotType) const;
        JoinGameMasterError::Error validateJoinAction(const JoinGameMasterRequest& joinGameRequest) const;
        JoinGameMasterError::Error validatePlayerJoinUserSessionExistence(const JoinGameMasterRequest& joinGameMasterRequest) const;

        void assignPlayerToFirstOpenSlot(PlayerInfoMaster& player);
        void markSlotOpen(SlotId slotId);
        void markSlotOccupied(SlotId slotId);

        void markConnectionSlotOpen(SlotId slotId, ConnectionGroupId connGroupId = 0);
        void markConnectionSlotOccupied(SlotId slotId, ConnectionGroupId connGroupId = 0);

        void updateNetworkQosData(const Util::NetworkQosData& hostQosData);
        void updateGameNatType();
        Util::NatType getMostRestrictivePlayerNatType() const;

        bool ignoreEntryCriteria(JoinMethod joinMethod, SlotType slotType) const;
        bool ignoreBanList(JoinMethod joinMethod) const;
        bool ignoreEntryCheckAdminInvited(const JoinGameRequest &joinGameRequest) const;

        BlazeRpcError validateSlotCapacities(const SlotCapacitiesVector &newSlotCapacities) const;
        BlazeRpcError validateTeams(const TeamIdVector &newTeamIds, const uint16_t &totalPlayerCapacity) const;
        BlazeRpcError validateRoleInformation(const RoleInformation &newRoleInfo, const uint16_t teamCapacity) const;


        void addGroupIdToAllowedSingleGroupJoinGroupIds(const UserGroupId& groupId);

        bool isSlotOccupied(SlotId slot) const;

        void submitJoinGameEvent(const JoinGameMasterRequest &joinRequest, const JoinGameResponse &joinResponse, const PlayerInfoMaster &joiningPlayer);
        void submitEventPlayerLeftGame(const PlayerInfoMaster& player, const GameSessionMaster& gameSession, const PlayerRemovalContext& removeContext);
        void submitEventDequeuePlayer(const PlayerInfoMaster& player);

        void sendGameStateUpdate();
        void sendGameSettingsUpdate();

        JoinGameByGroupMasterError::Error joinGameByGroupMembersOnlyInternal(const UserJoinInfoList& sessionIdList, bool isOptionalReservedList, const JoinGameByGroupMasterRequest& masterRequest, 
            JoinGameMasterResponse &joinGameMasterResponse, Blaze::EntryCriteriaError& error, const UserSessionSetupReasonMap &setupReasons, uint16_t &outNumPlayersAttemptingJoin);
        void updateMemberJoinRequestFromMemberInfo(JoinGameRequest& memberJoinRequest, const UserJoinInfo& memberJoinInfo, const GameSetupReason& memberReason, const JoinGameRequest& groupRequest, bool isOptionalList) const;
        GameEntryType getUserGameEntryTypeFromData(UserSessionId userSessionId, const PlayerJoinData& joinData, const GameSetupReason& memberReason) const;

        bool shouldPumpServerManagedQueue() const;

        void initExternalSessionData(const ExternalSessionMasterRequestData& newData, const GameCreationData* createData = nullptr, const TournamentSessionData* tournamentInfo = nullptr, const ExternalSessionActivityTypeInfo* forType = nullptr);
        void clearExternalSessionData();
        // this does not clear out all information about external sessions, just ids for a particular platform
        void clearExternalSessionIds(ClientPlatformType platform, ExternalSessionActivityType type = EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);

        void cleanUpMatchmakingQosDataMap();

        bool isBusyGameState(GameState gameState) const;
        bool areAllMembersGoodReputation(BlazeId leavingBlazeId = INVALID_BLAZE_ID) const;
        void updateAllowAnyReputationOnJoin(const UserSessionInfo& joiningUserInfo);
        void updateAllowAnyReputationOnRemove(const UserSessionInfo& leavingUserInfo);

        EA::TDF::ObjectId getBlazeObjectId() const { return EA::TDF::ObjectId((isGameGroup()? ENTITY_TYPE_GAME_GROUP : ENTITY_TYPE_GAME), getGameId()); }

        bool changePresenceState();
        //  returns whether the game session controls its own presence state on the clients machines
        //  basically all game members must use this game's presence for this to return true
        //  if any member does not use the game's presence status, this will return false.
        //  conforms with a limitation of Xbox 360 presence management
        //  if other platforms end up supporting this feature, this method should return true or false
        //  according to that platform's requirements.
        bool doesGameSessionControlPresence() const;

        void updateExternalSessionProperties(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context = mDefaultExternalSessionUpdateContext);
        void updateFinalExternalSessionProperties();

        const ExternalSessionUpdateInfo& getCurrentExternalSessionUpdateInfo() const;

        ExternalSessionIdentification& getExternalSessionIdentification() { return getGameData().getExternalSessionIdentification(); }
        void untrackExtSessionMembershipInternal(ExternalMemberInfoList& trackedSessionMembers, BlazeId blazeId, ClientPlatformType plat, ExternalSessionActivityType type);
        void setNpSessionId(const char8_t* value);

        // Helpers to allow players of a group to follow a specific player in case of group join. This is for the case where the target player is established
        // as the first player in the group that can get into the game without being queued.  
        PlayerInfoMaster* getTargetPlayer(const UserGroupId& groupId, bool& groupFound) const;
        void addTargetPlayerIdForGroup(const UserGroupId& groupId, PlayerId playerId);
        void addQueuedPlayerCountForGroup(const UserGroupId& groupId);
        void eraseUserGroupIdFromLists(const UserGroupId& groupId, bool onQueuedPlayerRemoval);

        void cleanupCCS(bool immediate = true);
        void scheduleCCSRequestTimer();
        void scheduleCCSExtendLeaseTimer(uint16_t leaseTimeMinutes);
        MeshEndpointsConnectionLostMasterError::Error updatePlayersConnLostStatus(GameId gameId, const CCSRequestPair* requestPair);

        // This is a check to see if we can protect participants' ip addresses from non-hosting participants.
        bool enableProtectedIP() const
        { 
            return mGameDataMaster->getEnableNetworkAddressProtection();
        }

        // This is a check to see if we can protect some participants' ip addresses from non-hosting participants.
        bool enablePartialProtectedIP() const
        {
            return enableProtectedIP() && (getCCSMode() == CCS_MODE_HOSTEDFALLBACK) && isCrossplayEnabled();
        }

        // builds the SessionIdSetIdentityFuncs for the set of players that will and will not receive an unmaksed IP address
        // sessionIdSetIdentityFuncProtectedIP contains the session ids that are supposed to get an unmasked IP
        // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that are going to get masked IP
        void chooseProtectedIPRecipients(const PlayerInfo &subjectPlayer, SessionIdSetIdentityFunc& sessionIdSetIdentityFuncProtectedIP, SessionIdSetIdentityFunc& sessionIdSetIdentityFuncUnprotectedIP);

        static void sendGameStartedToGameReportingSlave(StartedGameInfoPtr startedGameInfo);
        static void sendGameFinishedToGameReportingSlave(GameId gameId, GameInfoPtr finishedGameInfo);

        void updateAutomaticAdminSelection(const PlayerInfoMaster& joiningActivePlayer);

    private:
        // SlotId tracking for game clients (each game client occupies a connection endpoint slot)
        static const uint8_t ON_GAME_CREATE_HOST_SLOT_ID = 0; //!< the default SlotId used for game hosts when creating games
        static const uint16_t SLOT_ID_MAX_COUNT = 256; //!< the max possible number of clients who can connect to a game (the num of connection endpoints - aka SlotIds)
        typedef eastl::bitset<SLOT_ID_MAX_COUNT, GameDataMaster::EndpointSlotList::value_type> EndpointSlotBitset; //!< bitset used for getting/setting values in GameDataMaster::EndpointSlotList
        static const uint32_t SLOT_ID_MAX_ARRAY_SIZE = sizeof(EndpointSlotBitset)/sizeof(GameDataMaster::EndpointSlotList::value_type); //!< number of uint64_t array elements needed to store SLOT_ID_MAX_COUNT bits
        static EndpointSlotBitset& asEndpointSlotBitset(GameDataMaster::EndpointSlotList& slotList);
        static const EndpointSlotBitset& asEndpointSlotBitset(const GameDataMaster::EndpointSlotList& slotList);

        bool mAutoCommit;
        uint32_t mRefCount;
        OwnedSliverRef mOwnedSliverRef;
        Sliver::AccessRef mOwnedSliverAccessRef;
        GameDataMasterPtr mGameDataMaster;
        GameNetworkBehavior *mGameNetwork;
        PlayerRosterMaster mPlayerRoster;
        EndPointsConnectionMesh mPlayerConnectionMesh;
        GameSessionMetricsHandler mMetricsHandler;
        CensusEntrySet mCensusEntrySet; // Non-persistent cached state, intentionally not written to redis.
        EA::TDF::TimeValue mMatchEndTime; // Non-persistent cached state, intentionally not written to redis.
        EA::TDF::TdfHashValue mLastGameUpdateEventHash; // Non-persistent cached state, intentionally not written to redis.
        eastl::string mCcsPingSite;
        ExternalMemberInfoList mSentinelEmptyExternalMembersList;
        static ExternalSessionUpdateEventContext mDefaultExternalSessionUpdateContext;//default not final update

        friend void intrusive_ptr_add_ref(GameSessionMaster* ptr);
        friend void intrusive_ptr_release(GameSessionMaster* ptr);
        
    };


    typedef eastl::intrusive_ptr<GameSessionMaster> GameSessionMasterPtr;

    void intrusive_ptr_add_ref(GameSessionMaster* ptr);
    void intrusive_ptr_release(GameSessionMaster* ptr);

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAME_SESSION_MASTER_H

