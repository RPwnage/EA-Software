/*! ************************************************************************************************/
/*!
    \file gamemanagermasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/storage/storagemanager.h"
#include "framework/controller/controller.h"
#include "framework/redis/redismanager.h"
#include "framework/util/random.h"
#include "framework/util/uuid.h"
#include "framework/util/quantize.h"
#include "framework/usersessions/qosconfig.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmaster.h"

#include "gamemanager/censusinfo.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/gamenetworkbehavior.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/hostinjectionjob.h"
#include "gamemanager/gamereportingidhelper.h"
#include "gamemanager/commoninfo.h"
#include "EASTL/algorithm.h"

#include "framework/util/shared/blazestring.h"
#include "framework/tdf/attributes.h"
#include "framework/database/dbscheduler.h"

#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanagermetrics_server.h"
#include "gamemanager/externalsessions/externalsessionutil.h" // for mExternalSessionService
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/inputsanitizer.h"

// event management
#include "gamemanager/tdf/gamemanagerevents_server.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/rpc/gamemanagermetricsmaster.h"

#include "framework/tdf/userpineventtypes_server.h"

#ifdef TARGET_gamereporting
#include "gamereporting/rpc/gamereportingslave.h"
#include "gamereporting/tdf/gamereporting_server.h"
#endif

#include "gamemanager/matchmaker/groupuedexpressionlist.h"
#include "framework/connection/selector.h"
#include "framework/connection/connectionmanager.h" // for ConnectionManager in sendExternalTournamentGameEvent

namespace Blaze
{
namespace GameManager
{
    const char8_t PUBLIC_GAME_FIELD[] = "pub.game";
    const char8_t PRIVATE_GAME_FIELD[] = "pri.game";
    const char8_t PUBLIC_PLAYER_FIELD[] = "pub.player";
    const char8_t PRIVATE_PLAYER_FIELD[] = "pri.player";
    const char8_t PUBLIC_PLAYER_FIELD_FMT[] = "pub.player.%" PRId64;
    const char8_t PRIVATE_PLAYER_FIELD_FMT[] = "pri.player.%" PRId64;
    const char8_t* VALID_GAME_FIELD_PREFIXES[] = { PUBLIC_GAME_FIELD, PRIVATE_GAME_FIELD, PUBLIC_PLAYER_FIELD, PRIVATE_PLAYER_FIELD, nullptr };

    EA_THREAD_LOCAL GameManagerMasterImpl* gGameManagerMaster = nullptr;

    // static factory method
    GameManagerMaster* GameManagerMaster::createImpl()
    {
        return BLAZE_NEW_NAMED("GameManagerMasterImpl") GameManagerMasterImpl();
    }

    GameManagerMasterImpl::GameManagerMasterImpl()
        : GameManagerMasterStub(), 
          mAuditCleanupTimer(INVALID_TIMER_ID),
          mMetrics(getMetricsCollection()),
          mCensusByAttrNameMap(BlazeStlAllocator("mCensusByAttrNameMap", COMPONENT_MEMORY_GROUP)),
          mAuditedUsers(BlazeStlAllocator("mAuditedUsers", COMPONENT_MEMORY_GROUP)),
          mAuditUserDataGroupId(Fiber::INVALID_FIBER_GROUP_ID),
          mConnectionGroupJoinFailureTimer(INVALID_TIMER_ID),
          mScenarioManager(nullptr),
          mGameTable(GameManagerMaster::COMPONENT_ID, GameManagerMaster::COMPONENT_MEMORY_GROUP, GameManagerMaster::LOGGING_CATEGORY),
          mGameSessionMasterByIdMap(BlazeStlAllocator("mGameSessionMasterByIdMap", COMPONENT_MEMORY_GROUP)),
          mGameIdSetByUserSessionMap(BlazeStlAllocator("mGameIdSetByUserSessionMap", COMPONENT_MEMORY_GROUP)),
          mActiveGameIdSetByUserSessionMapsGames(BlazeStlAllocator("mActiveGameIdSetByUserSessionMapsGames", COMPONENT_MEMORY_GROUP)),
          mActiveGameIdSetByUserSessionMapsGameGroups(BlazeStlAllocator("mActiveGameIdSetByUserSessionMapsGameGroups", COMPONENT_MEMORY_GROUP)),
          mGameIdByPersistedIdMap(BlazeStlAllocator("mGameIdByPersistedIdMap", COMPONENT_MEMORY_GROUP)),
          mGameIdSetByOriginPersonaIdMap(BlazeStlAllocator("mGameIdSetByOriginPersonaIdMap", COMPONENT_MEMORY_GROUP)),
          mGameIdSetByPsnAccountIdMap(BlazeStlAllocator("mGameIdSetByPsnAccountIdMap", COMPONENT_MEMORY_GROUP)),
          mGameIdSetByXblAccountIdMap(BlazeStlAllocator("mGameIdSetByXblAccountIdMap", COMPONENT_MEMORY_GROUP)),
          mGameIdSetBySwitchIdMap(BlazeStlAllocator("mGameIdSetBySwitchIdMap", COMPONENT_MEMORY_GROUP)),
          mMigrationTimerSet(GameTimerSet::TimerCb(&GameSessionMaster::hostMigrationTimeout), "GameSessionMaster::hostMigrationTimeout"),
          mLockForJoinsTimerSet(GameTimerSet::TimerCb(&GameSessionMaster::clearLockForPreferredJoinsAndSendUpdate), "GameSessionMaster::clearLockForPreferredJoinsAndSendUpdate"),
          mLockedAsBusyTimerSet(GameTimerSet::TimerCb(&GameSessionMaster::clearLockedAsBusy), "GameSessionMaster::clearLockedAsBusy"),
          mCCSRequestIssueTimerSet(GameTimerSet::TimerCb(&GameSessionMaster::ccsRequestIssueTimerFired), "GameSessionMaster::ccsRequestIssueTimerFired"),
          mCCSExtendLeaseTimerSet(GameTimerSet::TimerCb(&GameSessionMaster::ccsExtendLeaseTimerFired), "GameSessionMaster::ccsExtendLeaseTimerFired"),
          mPlayerJoinGameTimerSet(PlayerTimerSet::TimerCb(&PlayerInfoMaster::onJoinGameTimeout), "PlayerInfoMaster::onJoinGameTimeout"),
          mPlayerPendingKickTimerSet(PlayerTimerSet::TimerCb(&PlayerInfoMaster::onPendingKick), "PlayerInfoMaster::onPendingKick"),
          mPlayerReservationTimerSet(PlayerTimerSet::TimerCb(&PlayerInfoMaster::onReservationTimeout), "PlayerInfoMaster::onReservationTimeout"),
          mPlayerQosValidationTimerSet(PlayerTimerSet::TimerCb(&PlayerInfoMaster::onQosValidationTimeout), "PlayerInfoMaster::onQosValidationTimeout"),
          mValidateExistenceJobQueue("GameManagerMasterImpl::mValidateExistenceJobQueue"),
          mExternalSessionJobQueue("GameManagerMasterImpl::mExternalSessionJobQueue"),
          mGameReportingRpcJobQueue("GameManagerMasterImpl::mGameReportingRpcJobQueue"),
          mPersistedIdOwnerMap(RedisId(COMPONENT_INFO.name, "mPersistentIdOwnerMap")),
          mNextCCSRequestId(0),
          mIsValidCCSConfig(false),
          mCensusCacheExpiry(0)
    {
        
        EA_ASSERT(gGameManagerMaster == nullptr);
        gGameManagerMaster = this;
        // Initialize supported game network topologies
        mGameNetworkBehaviorMap[CLIENT_SERVER_DEDICATED] = BLAZE_NEW NetworkBehaviorDedicatedServer;
        mGameNetworkBehaviorMap[PEER_TO_PEER_FULL_MESH] = BLAZE_NEW NetworkBehaviorFullMesh;
        mGameNetworkBehaviorMap[CLIENT_SERVER_PEER_HOSTED] = BLAZE_NEW NetworkBehaviorPeerHosted;
        mGameNetworkBehaviorMap[NETWORK_DISABLED] = BLAZE_NEW NetworkBehaviorDisabled;
        eastl::hash<const char8_t*> stringHash;
        mMatchAnyHash = stringHash(GAME_PROTOCOL_VERSION_MATCH_ANY);

        mValidateExistenceJobQueue.setMaximumWorkerFibers(FiberJobQueue::SMALL_WORKER_LIMIT);
        mExternalSessionJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
        mGameReportingRpcJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
    }

    GameManagerMasterImpl::~GameManagerMasterImpl()
    {
        EA_ASSERT_MSG(mGameSessionMasterByIdMap.empty(), "All games must have been exported before destroying the master!");

        // Note: components are shutdown in a specific (safe) order
        //   order of destruction is not guaranteed, so use onShutdown if you need to unregister/unsubscribe from another component

        // free the gameNetworkMap instances
        GameNetworkBehaviorMap::iterator networkMapIter = mGameNetworkBehaviorMap.begin();
        GameNetworkBehaviorMap::iterator networkMapEnd = mGameNetworkBehaviorMap.end();
        for ( ; networkMapIter!=networkMapEnd; ++networkMapIter)
        {
            delete networkMapIter->second;
        }

        // free any remaining remote host injection jobs
        HostInjectionJobMap::iterator hostInjectionJobMapIter = mHostInjectionJobMap.begin();
        HostInjectionJobMap::iterator hostInjectionJobMapEnd = mHostInjectionJobMap.end();
        for(; hostInjectionJobMapIter != hostInjectionJobMapEnd; ++hostInjectionJobMapIter)
        {
            delete hostInjectionJobMapIter->second;
        }

        mGameIdByPersistedIdMap.clear();

        cleanUpGroupUedExpressionLists();
        clearGamePermissionSystem();

        gGameManagerMaster = nullptr;
    }

    bool GameManagerMasterImpl::onActivate()
    {
        // Intentionally using GameManagerSlave here, since the provider lookup is based on component id of the BlazeObjectId, which doesn't separate slave/master.
        gUserSetManager->registerProvider(GameManagerSlave::COMPONENT_ID, *this);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief all components are up, so it's safe to lookup/cache/subscribe to them.
    ***************************************************************************************************/
    bool GameManagerMasterImpl::onResolve()
    {
        BlazeRpcError rc;
        // Need to ensure user manager is active and replicating. Otherwise when gamemanager initializes replication
        // after resolving, the slave will spew errors due to no user sessions.
        rc = gUserSessionManager->waitForState(ACTIVE, "GameManagerMasterImpl::onResolve");
        if (rc != ERR_OK)
        {
            FATAL_LOG("[GameManagerMaster] Unable to resolve, user session manager not active.");
            return false;
        }
        gUserSessionManager->addSubscriber(*this);
        gController->registerDrainStatusCheckHandler(GameManager::GameManagerMaster::COMPONENT_INFO.name, *this);

        mAuditUserDataGroupId = Fiber::allocateFiberGroupId();

        mGameTable.registerFieldPrefixes(VALID_GAME_FIELD_PREFIXES);
        mGameTable.registerRecordHandlers(
            ImportStorageRecordCb(this, &GameManagerMasterImpl::onImportStorageRecord),
            ExportStorageRecordCb(this, &GameManagerMasterImpl::onExportStorageRecord));
        mGameTable.registerRecordCommitHandler(CommitStorageRecordCb(this, &GameManagerMasterImpl::onCommitStorageRecord));

        if (gStorageManager == nullptr)
        {
            FATAL_LOG("[GameManagerMaster] Storage manager is missing.  This shouldn't happen (was it in the components.cfg?).");
            return false;
        }
        gStorageManager->registerStorageTable(mGameTable);

        return true;
    }

    CaptureGameManagerEnvironmentMasterError::Error GameManagerMasterImpl::processCaptureGameManagerEnvironmentMaster(Blaze::GameManager::GameCaptureResponse &response, const Message* message)
    {
        // Check that the permission needed for the capture is met.
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CAPTURE_GAME_MANAGER_ENVIRONMENT, false))
        {
            ERR_LOG("[GameManagerMaster].processCaptureGameManagerEnvironmentMaster: Attempted to run Redis BGSAVE without proper permission.");
            return CaptureGameManagerEnvironmentMasterError::ERR_AUTHORIZATION_REQUIRED;
        }

        response.setLastSaveTime(getRedisMostRecentSaveTime());

        RedisCommand cmd;
        cmd.add("BGSAVE");
        RedisResponsePtrList dummyList;
        RedisError rc = gRedisManager->execAll(cmd, dummyList);
        if (rc == REDIS_ERR_OK)
        {
            return CaptureGameManagerEnvironmentMasterError::ERR_OK;
        }
        else
        {
            // Not sure what errors would happen here.
            ERR_LOG("[GameManagerMaster].processCaptureGameManagerEnvironmentMaster: Failed to run BGSAVE command for Redis.");
        }

        return CaptureGameManagerEnvironmentMasterError::ERR_OK;
    }
    
    IsGameCaptureDoneMasterError::Error GameManagerMasterImpl::processIsGameCaptureDoneMaster(const Blaze::GameManager::GameCaptureResponse &request, 
                                                                                              Blaze::GameManager::IsGameCaptureDoneResponse &response, const Message* message)
    {
        response.setIsRedisDone(request.getLastSaveTime() < getRedisOldestSaveTime());
        return IsGameCaptureDoneMasterError::ERR_OK;
    }

    uint64_t GameManagerMasterImpl::getRedisMostRecentSaveTime()
    {
        uint64_t mostRecentSaveTime = 0;

        RedisCommand cmd;
        cmd.add("LASTSAVE");

        RedisResponsePtrList lastSaveTimes;
        RedisError rc = gRedisManager->execAll(cmd, lastSaveTimes);
        if (rc == REDIS_ERR_OK)
        {
            RedisResponsePtrList::iterator curIter = lastSaveTimes.begin();
            RedisResponsePtrList::iterator end = lastSaveTimes.end();
            for (; curIter != end; ++curIter) 
            {
                if ((*curIter)->isInteger())
                {
                    if ((uint64_t)(*curIter)->getInteger() > mostRecentSaveTime)
                        mostRecentSaveTime = (uint64_t)(*curIter)->getInteger();
                }
            }
        }

        return mostRecentSaveTime;
    }

    uint64_t GameManagerMasterImpl::getRedisOldestSaveTime()
    {
        uint64_t mostRecentSaveTime = UINT64_MAX;

        RedisCommand cmd;
        cmd.add("LASTSAVE");

        RedisResponsePtrList lastSaveTimes;
        RedisError rc = gRedisManager->execAll(cmd, lastSaveTimes);
        if (rc == REDIS_ERR_OK)
        {
            RedisResponsePtrList::iterator curIter = lastSaveTimes.begin();
            RedisResponsePtrList::iterator end = lastSaveTimes.end();
            for (; curIter != end; ++curIter) 
            {
                if ((*curIter)->isInteger())
                {
                    if ((uint64_t)(*curIter)->getInteger() < mostRecentSaveTime)
                        mostRecentSaveTime = (uint64_t)(*curIter)->getInteger();
                }
            }
        }

        return mostRecentSaveTime;
    }

    void GameManagerMasterImpl::addPinEventToSubmission(const GameSessionMaster* gameSessionMaster, GameManagerMasterImpl::PlayerUserSessionIdList *playerSessionList, bool connectionSuccessful,
                                                        RiverPoster::ConnectionEventPtr connectionEvent, eastl::list<RiverPoster::ConnectionStatsEventPtr>& connectionStatsEventList, PINSubmissionPtr pinSubmission)
    {
        if (pinSubmission != nullptr)
        {
            if (connectionEvent != nullptr)
                connectionSuccessful ?  connectionEvent->setStatus(RiverPoster::CONNECTION_STATUS_SUCCESS) : connectionEvent->setStatus(RiverPoster::CONNECTION_STATUS_FAILED);

            // The connection event is complete at this point, add it into the PIN submission for every endpoint on the source conn group
            for (auto& playerSessionPair : *playerSessionList)
            {
                PlayerId playerId = playerSessionPair.first;
                UserSessionId sessionId = playerSessionPair.second;

                if (sessionId != INVALID_USER_SESSION_ID)
                {
                    RiverPoster::PINEventList *newPinEventList = pinSubmission->getEventsMap()[sessionId];
                    if (newPinEventList == nullptr)
                    {
                        // put in the list
                        newPinEventList = pinSubmission->getEventsMap().allocate_element();
                        pinSubmission->getEventsMap()[sessionId] = newPinEventList;
                    }

                    // update the player id in the header (in case we are doing a logout)
                    eastl::string playerIdStr;
                    playerIdStr.sprintf("%" PRId64, playerId);

                    // set up the connection event
                    if (connectionEvent != nullptr)
                    {
                        connectionEvent->getHeaderCore().setPlayerId(playerIdStr.c_str());
                        connectionEvent->getHeaderCore().setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);

                        EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> variableTdfBase = newPinEventList->pull_back();
                        variableTdfBase->set(*connectionEvent->clone());
                    }

                    // set up the connection_stats events
                    for (auto connStatsEvent : connectionStatsEventList)
                    {
                        connStatsEvent->getHeaderCore().setPlayerId(playerIdStr.c_str());
                        connStatsEvent->getHeaderCore().setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);

                        EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> variableTdfBase = newPinEventList->pull_back();
                        variableTdfBase->set(*connStatsEvent->clone());
                    }
                }
            }
        }

        // Clear the stats lists after submitting:
        for (auto connStatsEvent : connectionStatsEventList)
        {
            connStatsEvent->getSamplesFloat().clear();
            connStatsEvent->getSamplesInt().clear();
        }
    }

    GetRedisDumpLocationsMasterError::Error GameManagerMasterImpl::processGetRedisDumpLocationsMaster(Blaze::GameManager::RedisDumpLocationsResponse &response, const Message* message)
    {
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CAPTURE_GAME_MANAGER_ENVIRONMENT, false))
        {
            ERR_LOG("[GameManagerMaster].processGetRedisDumpLocationsMaster: Attempted to get Redis dump locations without proper permission.");
            return GetRedisDumpLocationsMasterError::ERR_AUTHORIZATION_REQUIRED;
        }

        RedisError redisErr = gRedisManager->getRedisCluster()->getDumpOutputLocations(response.getDumpLocations());
        if (redisErr != REDIS_ERR_OK)
        {
            ERR_LOG("[GameManagerMaster].processCaptureGameManagerEnvironmentMaster: Failed to get Redis dump locations.");
            return GetRedisDumpLocationsMasterError::ERR_SYSTEM;
        }

        return GetRedisDumpLocationsMasterError::ERR_OK;
    }

    void GameManagerMasterImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
    {
        // Explicitly suppress blocking here, we don't want to accidentally let timers execute while the game is beign unpacked...
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

        // If we're loading pseudo games, then we just set the pseudo flag on the game, which will disable the player activity checks (and disable normal matchmaking).
        const bool loadAsPseudoGames = getConfig().getLoadRedisAsPseudoGames();
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr;
        snapFieldItr = getStorageFieldByPrefix(snapshot.fieldMap, PUBLIC_GAME_FIELD);
        if (snapFieldItr == snapshot.fieldMap.end())
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Field prefix=" << PUBLIC_GAME_FIELD << " matches no fields in game(" << snapshot.recordId << ").");
            return;
        }
        ReplicatedGameDataServer& gameSession = static_cast<ReplicatedGameDataServer&>(*snapFieldItr->second);

        GameNetworkBehavior* gameNetwork = getGameNetworkBehavior(gameSession.getReplicatedGameData().getNetworkTopology());
        if (gameNetwork == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Failed to init game(" << gameSession.getReplicatedGameData().getGameId() 
                << ") with an unknown network topology(" << gameSession.getReplicatedGameData() << ").");
            return;
            }

        snapFieldItr = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_GAME_FIELD);
        if (snapFieldItr == snapshot.fieldMap.end())
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Field prefix=" << PRIVATE_GAME_FIELD << " matches no fields in game(" << snapshot.recordId << ").");
            return;
        }
        GameDataMaster& gameDataMaster = static_cast<GameDataMaster&>(*snapFieldItr->second);

        GameSessionMasterByIdMap::insert_return_type gameRet = mGameSessionMasterByIdMap.insert(snapshot.recordId);
        if (!gameRet.second)
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Unexpected collision inserting game(" << snapshot.recordId << ").");
            return;
        }
        OwnedSliverRef sliverRef = &ownedSliver; // pin the owned sliver, and pass it to the game
        gameRet.first->second = BLAZE_NEW GameSessionMaster(gameSession, gameDataMaster, *gameNetwork, sliverRef);
        if (loadAsPseudoGames)
        {
            gameSession.getReplicatedGameData().setIsPseudoGame(true);
        }

        GameSessionMaster& gameSessionMaster = *gameRet.first->second;
        int32_t errorCount = gameSessionMaster.createCriteriaExpressions();
        if (errorCount > 0)
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Failed to parse game entry criteria for game(" << gameSessionMaster.getGameId() << ").");
        }

        BlazeRpcError err = gameSessionMaster.updateRoleEntryCriteriaEvaluators();
        if (err != ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Failed to parse role-specific game entry criteria for game(" << gameSessionMaster.getGameId() << ").");
        }
        addGameToCensusCache(gameSessionMaster);

        UserSessionIdSet validatedUserSessionIdSet;
        validatedUserSessionIdSet.reserve(64); // usually no more than 64 players/game

        if (gameSessionMaster.hasDedicatedServerHost())
        {
            const HostInfo& dedicatedServerHostInfo = gameSessionMaster.getDedicatedServerHostInfo();
            if (dedicatedServerHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
            {
                if (dedicatedServerHostInfo.getPlayerId() != INVALID_BLAZE_ID)
                    scheduleValidateUserSession(dedicatedServerHostInfo.getUserSessionId(), dedicatedServerHostInfo.getPlayerId(), validatedUserSessionIdSet);
                // rebuild secondary indexes and in-game gauge metrics
                addUserSessionGameInfo(dedicatedServerHostInfo.getUserSessionId(), gameSessionMaster);
            }
        }

        const HostInfo& topologyHostInfo = gameSessionMaster.getTopologyHostInfo();
        if (topologyHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
        {
            if (topologyHostInfo.getPlayerId() != INVALID_BLAZE_ID)
                scheduleValidateUserSession(topologyHostInfo.getUserSessionId(), topologyHostInfo.getPlayerId(), validatedUserSessionIdSet);
            // rebuild secondary indexes and in-game gauge metrics
            addUserSessionGameInfo(topologyHostInfo.getUserSessionId(), gameSessionMaster);
        }
        const HostInfo& platformHostInfo = gameSessionMaster.getPlatformHostInfo();
        if (platformHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
        {
            if (platformHostInfo.getPlayerId() != INVALID_BLAZE_ID)
                scheduleValidateUserSession(platformHostInfo.getUserSessionId(), platformHostInfo.getPlayerId(), validatedUserSessionIdSet);
            // rebuild secondary indexes and in-game gauge metrics
            addUserSessionGameInfo(platformHostInfo.getUserSessionId(), gameSessionMaster);
        }
        const UserSessionInfo& creatorHostInfo = gameDataMaster.getCreatorHostInfo();
        if (creatorHostInfo.getSessionId() != INVALID_USER_SESSION_ID)
        {
            if (creatorHostInfo.getUserInfo().getId() != INVALID_BLAZE_ID)
                scheduleValidateUserSession(creatorHostInfo.getSessionId(), creatorHostInfo.getUserInfo().getId(), validatedUserSessionIdSet);
            // rebuild secondary indexes and in-game gauge metrics
            addUserSessionGameInfo(creatorHostInfo.getSessionId(), gameSessionMaster);
        }
        if (gameSessionMaster.getExternalOwnerSessionId() != INVALID_USER_SESSION_ID)
        {
            if (gameSessionMaster.getExternalOwnerBlazeId() != INVALID_BLAZE_ID)
                scheduleValidateUserSession(gameSessionMaster.getExternalOwnerSessionId(), gameSessionMaster.getExternalOwnerBlazeId(), validatedUserSessionIdSet);
            // rebuild secondary indexes and in-game gauge metrics
            addUserSessionGameInfo(gameSessionMaster.getExternalOwnerSessionId(), gameSessionMaster);
        }
        if (gameSessionMaster.hasPersistedGameId())
        {
            GameIdByPersistedIdMap::insert_return_type ret = mGameIdByPersistedIdMap.insert(eastl::make_pair(gameSessionMaster.getPersistedGameId(), gameSessionMaster.getGameId()));
            if (!ret.second)
            {
                ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Internal error: Failed to add persisted game id mapping for game(" << gameSessionMaster.getGameId() << "), as a game(" << ret.first->second << ") already exists with the same persisted id(" << gameSessionMaster.getPersistedGameId() << "). Blaze may not be able to enforce only one game has the persisted game id.");
            }
        }
        gameSessionMaster.getMetricsHandler().rebuildGameMetrics();
                                
        // rebuild timers (if not a pseudo game)
        if (!loadAsPseudoGames)
        {
            // rebuild timers
            if (gameDataMaster.getMigrationTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Queued migration timeout: " << gameDataMaster.getMigrationTimeout().getMicroSeconds() << " for game: " << gameSessionMaster.getGameId());
                mMigrationTimerSet.scheduleTimer(gameSessionMaster.getGameId(), gameDataMaster.getMigrationTimeout());
            }
            if (gameDataMaster.getLockForPreferredJoinsTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Queued lock-for-joins timeout: " << gameDataMaster.getLockForPreferredJoinsTimeout().getMicroSeconds() << " for game: " << gameSessionMaster.getGameId());
                mLockForJoinsTimerSet.scheduleTimer(gameSessionMaster.getGameId(), gameDataMaster.getLockForPreferredJoinsTimeout());
            }
            if (gameDataMaster.getLockedAsBusyTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Queued locked as busy timeout: " << gameDataMaster.getLockedAsBusyTimeout().getMicroSeconds() << " for game: " << gameSessionMaster.getGameId());
                mLockedAsBusyTimerSet.scheduleTimer(gameSessionMaster.getGameId(), gameDataMaster.getLockedAsBusyTimeout());
            }
            if (gameDataMaster.getCCSRequestIssueTime() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Queued ccs request timer: " << gameDataMaster.getCCSRequestIssueTime().getMicroSeconds() << " for game: " << gameSessionMaster.getGameId());
                mCCSRequestIssueTimerSet.scheduleTimer(gameSessionMaster.getGameId(), gameDataMaster.getCCSRequestIssueTime());
            }
            if (gameDataMaster.getCCSExtendLeaseTime() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Queued ccs extend lease timer: " << gameDataMaster.getCCSExtendLeaseTime().getMicroSeconds() << " for game: " << gameSessionMaster.getGameId());
                mCCSExtendLeaseTimerSet.scheduleTimer(gameSessionMaster.getGameId(), gameDataMaster.getCCSExtendLeaseTime());
            }
        }
        TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Added game: " << gameSessionMaster.getGameId());

        // game is good

        snapFieldItr = Blaze::getStorageFieldByPrefix(snapshot.fieldMap, PUBLIC_PLAYER_FIELD);
        if (snapFieldItr == snapshot.fieldMap.end())
        {
            // NOTE: Dedicated Server games may exist without players
            TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Field prefix=" << PUBLIC_PLAYER_FIELD << " matches no fields in game(" << snapshot.recordId << ").");
            return;
        }

        size_t pubPlayerFieldPrefixLen = strlen(PUBLIC_PLAYER_FIELD);
        do
        {
            // build corresponding private field key
            StorageFieldName privatePlayerFieldName(PRIVATE_PLAYER_FIELD);
            privatePlayerFieldName.append(snapFieldItr->first.c_str() + pubPlayerFieldPrefixLen);
            StorageRecordSnapshot::FieldEntryMap::const_iterator privPlayerFieldItr = snapshot.fieldMap.find(privatePlayerFieldName);
            if (privPlayerFieldItr != snapshot.fieldMap.end())
            {
                ReplicatedGamePlayerServer& playerData = static_cast<ReplicatedGamePlayerServer&>(*snapFieldItr->second);
                PlayerDataMaster& playerDataMaster = static_cast<PlayerDataMaster&>(*privPlayerFieldItr->second);
                PlayerInfoMasterPtr playerInfoPtr = BLAZE_NEW PlayerInfoMaster(playerDataMaster, playerData);
                PlayerInfoMaster& playerInfo = *playerInfoPtr;

                // NOTE: This is where we currently grab our game to add a player to it
                GameSessionMaster* sessionMaster = getGameSessionMaster(playerInfo.getGameId());
                if (sessionMaster != nullptr)
                {
                    // save player session id to a marked list so that we can validate it later
                    scheduleValidateUserSession(playerInfo.getPlayerSessionId(), playerInfo.getPlayerId(), validatedUserSessionIdSet);
                    sessionMaster->getMetricsHandler().onPlayerAdded(playerInfo, false, false);
                    sessionMaster->insertPlayer(playerInfo); // attach player to the game
                    // rebuild secondary indexes and in-game gauge metrics
                    addUserSessionGameInfo(playerInfo.getPlayerSessionId(), *sessionMaster, playerInfo.isActive(), &playerInfo.getUserInfo());
                    insertIntoExternalPlayerToGameIdsMap(playerInfo.getUserInfo().getUserInfo(), playerInfo.getGameId());
                    // rebuild timers (if not a pseudo game)
                    if (!loadAsPseudoGames)
                    {
                        // rebuild timers
                        if (playerDataMaster.getJoinGameTimeout() != 0)
                        {
                            TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Scheduled join game timeout: " << playerDataMaster.getJoinGameTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                            mPlayerJoinGameTimerSet.scheduleTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()), playerDataMaster.getJoinGameTimeout());
                        }
                        if (playerDataMaster.getPendingKickTimeout() != 0)
                        {
                            TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Scheduled pending kick timeout: " << playerDataMaster.getPendingKickTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                            mPlayerPendingKickTimerSet.scheduleTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()), playerDataMaster.getPendingKickTimeout());
                        }
                        if (playerDataMaster.getReservationTimeout() != 0)
                        {
                            TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Scheduled reservation timeout: " << playerDataMaster.getReservationTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                            mPlayerReservationTimerSet.scheduleTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()), playerDataMaster.getReservationTimeout());
                        }
                        if (playerDataMaster.getQosValidationTimeout() != 0)
                        {
                            TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Scheduled qos validation timeout: " << playerDataMaster.getQosValidationTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                            mPlayerQosValidationTimerSet.scheduleTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()), playerDataMaster.getQosValidationTimeout());
                        }
                    }
                    TRACE_LOG("[GameManagerMasterImpl].onImportStorageRecord: Added player: " << playerInfo.getPlayerId() << " to game: " << playerInfo.getGameId());
                }
                else
                {
                    ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Failed to find game: " << playerInfo.getGameId() << " for player: " << playerInfo.getPlayerId());
                }
            }
            else
            {
                ERR_LOG("[GameManagerMasterImpl].onImportStorageRecord: Missing private player data in game(" << snapshot.recordId << "), storage corrupt.");
            }
            ++snapFieldItr;
            if (snapFieldItr == snapshot.fieldMap.end())
                break;
        }
        while (blaze_strncmp(snapFieldItr->first.c_str(), PUBLIC_PLAYER_FIELD, pubPlayerFieldPrefixLen) == 0);
    }

    void GameManagerMasterImpl::onExportStorageRecord(StorageRecordId recordId)
    {
        // This method is a matched counterpart to onImportStorageRecord() and is called whenever 
        // another master is taking over our game, or this master is draining.
        // This method shall perform exactly the same operations that are used to *silently* release 
        // all locally cached state associated with a given game, as would occur in the GameManagerMasterImpl::dtor().
        // Example of state to be reverted: Secondary indexes, Gauge metric counters, etc.
        // NOTE: No events are to be signalled during this removal, since it merely corresponds to the game
        // migrating from one live Blaze instance to another. 
        const GameId gameId = recordId;

        GameSessionMasterByIdMap::iterator gameItr = mGameSessionMasterByIdMap.find(gameId);
        if (gameItr == mGameSessionMasterByIdMap.end())
        {
            WARN_LOG("[GameManagerMasterImpl].onExportStorageRecord: Failed to export game(" << gameId << "), not found.");
            return;
        }

        // NOTE: Intentionally *not* using GameSessionMasterPtr here because it would attempt to aquire sliver access while sliver is locked for migration, which we want to avoid.
        // It is always safe to delete the GameSessionMaster object after export, because export is only triggered when there are no more outstanding sliver accesses on this sliver.
        GameSessionMaster* gameSessionMaster = gameItr->second.detach();
        mGameSessionMasterByIdMap.erase(gameItr);

        {
            // cancel pending timers
            GameDataMaster& gameDataMaster = *gameSessionMaster->getGameDataMaster();
            if (gameDataMaster.getMigrationTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled migration timeout: " << gameDataMaster.getMigrationTimeout().getMicroSeconds() << " for game: " << gameId);
                mMigrationTimerSet.cancelTimer(gameId);
            }
            if (gameDataMaster.getLockForPreferredJoinsTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled lock-for-joins timeout: " << gameDataMaster.getLockForPreferredJoinsTimeout().getMicroSeconds() << " for game: " << gameId);
                mLockForJoinsTimerSet.cancelTimer(gameId);
            }
            if (gameDataMaster.getLockedAsBusyTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled locked as busy timeout: " << gameDataMaster.getLockedAsBusyTimeout().getMicroSeconds() << " for game: " << gameId);
                mLockedAsBusyTimerSet.cancelTimer(gameId);
            }
            if (gameDataMaster.getCCSRequestIssueTime() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled ccs request timer: " << gameDataMaster.getCCSRequestIssueTime().getMicroSeconds() << " for game: " << gameId);
                mCCSRequestIssueTimerSet.cancelTimer(gameId);
            }
            if (gameDataMaster.getCCSExtendLeaseTime() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled ccs extend lease timer: " << gameDataMaster.getCCSExtendLeaseTime().getMicroSeconds() << " for game: " << gameId);
                mCCSExtendLeaseTimerSet.cancelTimer(gameId);
            }
        }

        removeGameFromCensusCache(*gameSessionMaster);


        for (auto& pinConnEvents : mMetrics.mConnectionStatsEventListByConnGroup[gameId])
        {
            GameManagerMasterMetrics::ConnectionGroupIdPair connPair = pinConnEvents.first;
            sendPINConnStatsEvent(gameSessionMaster, connPair.first, pinConnEvents.second);
        }

        removeGameFromConnectionMetrics(gameSessionMaster->getGameId());   // Not really required, since the exported Game implies the server is being shutdown, but good anyways.

        gameSessionMaster->clearCriteriaExpressions();

        // proceed cleaning up all game/player related data
        if (gameSessionMaster->hasDedicatedServerHost())
        {
            const HostInfo& dedicatedServerHostInfo = gameSessionMaster->getDedicatedServerHostInfo();
            if (dedicatedServerHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
            {
                // rebuild secondary indexes and in-game gauge metrics
                removeUserSessionGameInfo(dedicatedServerHostInfo.getUserSessionId(), *gameSessionMaster);
            }
        }
        const HostInfo& topologyHostInfo = gameSessionMaster->getTopologyHostInfo();
        if (topologyHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
        {
            // rebuild secondary indexes and in-game gauge metrics
            removeUserSessionGameInfo(topologyHostInfo.getUserSessionId(), *gameSessionMaster);
        }
        const HostInfo& platformHostInfo = gameSessionMaster->getPlatformHostInfo();
        if (platformHostInfo.getUserSessionId() != INVALID_USER_SESSION_ID)
        {
            // rebuild secondary indexes and in-game gauge metrics
            removeUserSessionGameInfo(platformHostInfo.getUserSessionId(), *gameSessionMaster);
        }
        const GameDataMaster& gameDataMaster = *gameSessionMaster->getGameDataMaster();
        const UserSessionInfo& creatorHostInfo = gameDataMaster.getCreatorHostInfo();
        if (creatorHostInfo.getSessionId() != INVALID_USER_SESSION_ID)
        {
            // rebuild secondary indexes and in-game gauge metrics
            removeUserSessionGameInfo(creatorHostInfo.getSessionId(), *gameSessionMaster);
        }
        if (gameSessionMaster->getExternalOwnerSessionId() != INVALID_USER_SESSION_ID)
        {
            // rebuild secondary indexes and in-game gauge metrics
            removeUserSessionGameInfo(gameSessionMaster->getExternalOwnerSessionId(), *gameSessionMaster);
        }
        if (gameSessionMaster->hasPersistedGameId())
        {
            GameIdByPersistedIdMap::iterator it = mGameIdByPersistedIdMap.find(gameSessionMaster->getPersistedGameId());
            if (it != mGameIdByPersistedIdMap.end() && it->second == gameSessionMaster->getGameId())
            {
                // Clean up the local cache.
                // we do NOT need to clean up mRedisGamePersistedIdBySliverIdMap in redis since the id of the corresponding sliver which owns this game remains unchanged.
                mGameIdByPersistedIdMap.erase(gameSessionMaster->getPersistedGameId());
            }
        }

        gameSessionMaster->getMetricsHandler().teardownGameMetrics();

        PlayerRoster::PlayerInfoList playerList = gameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS);
        for (PlayerRoster::PlayerInfoList::iterator playerItr = playerList.begin(), playerEnd = playerList.end(); playerItr != playerEnd; ++playerItr)
        {
            PlayerInfoMaster& playerInfo = *static_cast<PlayerInfoMaster*>(*playerItr);
            gameSessionMaster->getMetricsHandler().teardownPlayerMetrics(playerInfo);

            // rebuild secondary indexes and in-game gauge metrics
            removeUserSessionGameInfo(playerInfo.getPlayerSessionId(), *gameSessionMaster, playerInfo.isActive(), &playerInfo.getUserInfo());
            eraseFromExternalPlayerToGameIdsMap(playerInfo.getUserInfo().getUserInfo(), playerInfo.getGameId());
            
            const PlayerDataMaster& playerDataMaster = *playerInfo.getPlayerDataMaster();
            // cancel timers
            if (playerDataMaster.getJoinGameTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled join game timeout: " << playerDataMaster.getJoinGameTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                mPlayerJoinGameTimerSet.cancelTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()));
            }
            if (playerDataMaster.getPendingKickTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled pending kick timeout: " << playerDataMaster.getPendingKickTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                mPlayerPendingKickTimerSet.cancelTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()));
            }
            if (playerDataMaster.getReservationTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled reservation timeout: " << playerDataMaster.getReservationTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                mPlayerReservationTimerSet.cancelTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()));
            }
            if (playerDataMaster.getQosValidationTimeout() != 0)
            {
                TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Canceled qos validation timeout: " << playerDataMaster.getQosValidationTimeout().getMicroSeconds() << " for player: " << playerInfo.getPlayerId());
                mPlayerQosValidationTimerSet.cancelTimer(PlayerInfoMaster::GamePlayerIdPair(playerInfo.getGameId(), playerInfo.getPlayerId()));
            }
            TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Erased player: " << playerInfo.getPlayerId() << " in game: " << playerInfo.getGameId());
        }

        TRACE_LOG("[GameManagerMasterImpl].onExportStorageRecord: Exported game: " << recordId);

        delete gameSessionMaster;
    }

    void GameManagerMasterImpl::onCommitStorageRecord(StorageRecordId recordId)
    {
        if (getConfig().getGameSession().getEnableGameUpdateEvents())
        {
            GameSessionMaster* game = getGameSessionMaster(recordId);
            if (game != nullptr)
            {
                game->sendGameUpdateEvent();
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief components are shut down in a specific (safe) order, so unregister from maps/session manager here
    ***************************************************************************************************/
    void GameManagerMasterImpl::onShutdown()
    {
        mValidateExistenceJobQueue.cancelQueuedJobs(); // cancel any outstanding validations, we don't need to wait for those to complete since they will be performed next time
        mValidateExistenceJobQueue.join();

        mExternalSessionJobQueue.join();

        if (mAuditUserDataGroupId != Fiber::INVALID_FIBER_GROUP_ID)
            Fiber::join(mAuditUserDataGroupId);
        
        if (mAuditCleanupTimer != INVALID_TIMER_ID)
        {
        // this timer is non-persistent
            gSelector->cancelTimer(mAuditCleanupTimer);
            mAuditCleanupTimer = INVALID_TIMER_ID;
        }

        // cancel all persistent timers, they will be reseeded on instance startup
        mMigrationTimerSet.cancelScheduledTimers();
        mLockForJoinsTimerSet.cancelScheduledTimers();
        mLockedAsBusyTimerSet.cancelScheduledTimers();
        mCCSRequestIssueTimerSet.cancelScheduledTimers();
        mCCSExtendLeaseTimerSet.cancelScheduledTimers();
        mPlayerJoinGameTimerSet.cancelScheduledTimers();
        mPlayerPendingKickTimerSet.cancelScheduledTimers();
        mPlayerReservationTimerSet.cancelScheduledTimers();
        mPlayerQosValidationTimerSet.cancelScheduledTimers();

        gUserSetManager->deregisterProvider(COMPONENT_ID);
        gUserSessionManager->removeSubscriber(*this);
        gController->deregisterDrainStatusCheckHandler(GameManager::GameManagerMaster::COMPONENT_INFO.name);

        gStorageManager->deregisterStorageTable(mGameTable);
    }

    /*! ************************************************************************************************/
    /*! \brief init config file parse for GameManagerMaster; also config the matchmaker.
    ***************************************************************************************************/
    bool GameManagerMasterImpl::onConfigure()
    {
        if (!GameReportingIdHelper::init())
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: failed initializing game reporting id helper");
            return false;
        }

        if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GameManagerMaster::COMPONENT_ID), GAMEMANAGER_IDTYPE_GAME) != ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: Failed reserving game id " << 
                RpcMakeSlaveId(GameManagerMaster::COMPONENT_ID) << " with unique id manager");
            return false;
        }

        bool success = GameManager::ExternalSessionUtil::createExternalSessionServices(mExternalSessionServices,
            getConfig().getGameSession().getExternalSessions(), getConfig().getGameSession().getPlayerReservationTimeout(), UINT16_MAX,
            &getConfig().getGameSession().getTeamNameByTeamIdMap());

        if (!success)
        {
            return false;   // (err already logged)
        }

        // check all the expression lists are valid
        if (!setUpGroupUedExpressionLists())
        {
            return false; // setUp method logs error 
        }

        // We check for ccs configuration and keep the results cached. We don't fail on it as it is not required.
        mIsValidCCSConfig = GameManager::ValidationUtils::hasValidCCSConfig(getConfig());

        // Load the audit data from the DB into our cache
        DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
        if (conn == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to obtain a database connection.");
            return false;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT * FROM `gm_user_connection_audit` WHERE `expiration`=0 OR `expiration`>$D", TimeValue::getTimeOfDay().getMicroSeconds());

        DbResultPtr dbResult;
        DbError dbErr = conn->executeQuery(query, dbResult);
        if (dbErr != DB_ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return false;
        }
        else
        {
        for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
        {
            const DbRow* row = *itr;
            mAuditedUsers[row->getInt64((uint32_t)0)] = TimeValue(row->getInt64(1));
        }
        }

        // Set up a job to periodically check for expired audits and remove them from the audit cache/DB
        if (mAuditCleanupTimer != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mAuditCleanupTimer);
            mAuditCleanupTimer = INVALID_TIMER_ID;
        }

        TimeValue auditCleanupPeriod = TimeValue::getTimeOfDay() + getConfig().getCheckAuditExpirationInterval();
        mAuditCleanupTimer = gSelector->scheduleFiberTimerCall(auditCleanupPeriod, this, &GameManagerMasterImpl::cleanupExpiredAudits, "GameManagerMasterImpl::onConfigure");

        configureGamePermissionSystem();


        if (getConfig().getLoadRedisAsPseudoGames())
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: The functionality is broken and deprecated.");
            return false;
        }
        return true;
    }
    
    /*! ************************************************************************************************/
    /*! \brief re-parse the config file
            - parse the game manager & matchmaking configuration sections and ensure no errors.
            - If there are no errors we set new configuration values.
        \param[out] true always.  In case of error, old configuration values are kept so the server
            keeps running in a known state.
    ***************************************************************************************************/
    bool GameManagerMasterImpl::onReconfigure()
    {
        mExternalSessionServices.onReconfigure(getConfig().getGameSession().getExternalSessions(), getConfig().getGameSession().getPlayerReservationTimeout());

        cleanUpGroupUedExpressionLists();

        setUpGroupUedExpressionLists();

        // We check for ccs configuration and keep the results cached. We don't fail on it as it is not required.
        mIsValidCCSConfig = GameManager::ValidationUtils::hasValidCCSConfig(getConfig());

        configureGamePermissionSystem();

        TRACE_LOG("[GameManagerMasterImpl].onReConfigure: reconfiguration of gamemanager complete.");
        return true;
    }

    bool GameManagerMasterImpl::onValidatePreconfig(GameManagerServerPreconfig& config, const GameManagerServerPreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        if (!mScenarioManager.validatePreconfig(config))
        {
            ERR_LOG("[GameManagerMasterImpl].onValidatePreconfig: Configuration problem while validating preconfig for ScenarioManager.");
            return false;
        }

        return true;
    }

    bool GameManagerMasterImpl::onPreconfigure()
    {
        TRACE_LOG("[GameManagerMaster] Preconfiguring gamemanager master component.");

        const GameManagerServerPreconfig& preconfigTdf = getPreconfig();

        // We still need to preconfig on the master, because we're still loading the Generics.
        if (!mScenarioManager.preconfigure(preconfigTdf))
        {
            ERR_LOG("[GameManagerMasterImpl].onPreconfigure: Configuration problem while preconfiguring the ScenarioManager.");
            return false;
        }
        return true;
    }

    bool GameManagerMasterImpl::onValidateConfig(GameManagerServerConfig& config, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        // Create a dummy config to compare against defaults:
        GameSessionServerConfig defaultConfig;
        if (config.getGameSession().getPendingKickTimeout() < defaultConfig.getPendingKickTimeout())
        {
            WARN_LOG("[GameManagerMasterImpl].onValidateConfig: pending kick timeout of '" << config.getGameSession().getPendingKickTimeout().getMillis() 
                      << "' ms is less than recommended minimum of " << defaultConfig.getPendingKickTimeout().getMillis() << " ms");
        }
        if (config.getGameSession().getPendingKickTimeout() <= config.getGameSession().getHostPendingKickTimeout())
        {
            ERR_LOG("[GameManagerMasterImpl].onValidateConfig: pending kick timeout of '" << config.getGameSession().getPendingKickTimeout().getMillis() 
                     << "' ms is less than or equal to the host kick timeout of " << config.getGameSession().getHostPendingKickTimeout().getMillis() << " ms. "
                     << "The host timeout must be shorter, or players may be incorrectly kicked in cases where the host disconnected.");
            return false;
        }

        // check all the expression lists are valid for the teamUedBalanceRuleMap, since they're used on the GM master to tiebreak team selection for FG-mm joins.
        if (!config.getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().empty())
        {
            TeamUEDBalanceRuleConfigMap::const_iterator teamUedBalanceRuleIter = config.getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().begin();
            TeamUEDBalanceRuleConfigMap::const_iterator teamUedBalanceRuleEnd = config.getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().end();
            for (; teamUedBalanceRuleIter != teamUedBalanceRuleEnd; ++teamUedBalanceRuleIter)
            {
                Matchmaker::GroupUedExpressionList tempGroupUedExpressionList;
                if (!teamUedBalanceRuleIter->second->getGroupAdjustmentFormula().empty())
                {
                    bool expressionInitResult = tempGroupUedExpressionList.initialize(teamUedBalanceRuleIter->second->getGroupAdjustmentFormula());
                    if (!expressionInitResult)
                    {
                        ERR_LOG("[GameManagerMasterImpl].onValidateConfig ERROR failed to initialize group UED expressions for TeamUedBalanceRule '" << teamUedBalanceRuleIter->first << "'.");
                        return false;
                    }
                }
            }
        }

        if (!config.getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().empty() &&
            (config.getMatchmakerSettings().getQosValidationTimeout() < config.getGameSession().getJoinTimeout()))
        {
            WARN_LOG("[GameManagerMasterImpl].onValidateConfig qos validation timeout (" << gGameManagerMaster->getMmQosValidationTimeout().getSec() << "s) was configured to be less than the recommended minimum which is the player join timeout (" << config.getGameSession().getJoinTimeout().getSec() << "s).");
        }

        auto& latencyQuantiles = config.getLatencyMetricQuantiles();
        if (!latencyQuantiles.empty())
        {
            static const uint32_t MAX_LATENCY = LATENCY_BUCKET_SIZE * (MAX_NUM_LATENCY_BUCKETS - 1);
            auto isValidLatencyQuantile = [](uint32_t latencyQuantile) {
                return ((latencyQuantile % LATENCY_BUCKET_SIZE) == 0) && (latencyQuantile <= MAX_LATENCY);
            };
            bool badQuantile = false;
            for (auto itr = latencyQuantiles.begin() + 1, end = latencyQuantiles.end(); itr != end; ++itr)
            {
                auto curr = *itr;
                if (!isValidLatencyQuantile(curr))
                {
                    badQuantile = true;
                    break;
                }
                auto prev = *(itr - 1);
                if (curr <= prev)
                {
                    ERR_LOG("[GameManagerMasterImpl].onValidateConfig: GameManagerServerConfig.latencyMetricQuantiles elements must be unique and sorted in ascending order.");
                    return false;
                }
            }
            if (badQuantile || !isValidLatencyQuantile(latencyQuantiles.front()))
            {
                ERR_LOG("[GameManagerMasterImpl].onValidateConfig: GameManagerServerConfig.latencyMetricQuantiles elements must be a multiple of the bucket size: " 
                    << LATENCY_BUCKET_SIZE << " and cannot exceed: " << MAX_LATENCY << ".");
                return false;
            }
        }

        auto& packetLossQuantiles = config.getPacketLossMetricQuantiles();
        if (!packetLossQuantiles.empty())
        {
            static const uint32_t MAX_PACKET_LOSS = 100;
            auto isValidPacketLossQuantile = [](uint32_t packetLossQuantile) {
                return packetLossQuantile <= MAX_PACKET_LOSS;
            };
            bool badQuantile = false;
            for (auto itr = packetLossQuantiles.begin() + 1, end = packetLossQuantiles.end(); itr != end; ++itr)
            {
                auto curr = *itr;
                if (!isValidPacketLossQuantile(curr))
                {
                    badQuantile = true;
                    break;
                }
                auto prev = *(itr - 1);
                if (curr <= prev)
                {
                    ERR_LOG("[GameManagerMasterImpl].onValidateConfig: GameManagerServerConfig.packetLossMetricQuantiles elements must be unique and sorted in ascending order.");
                    return false;
                }
            }
            if (badQuantile || !isValidPacketLossQuantile(packetLossQuantiles.front()))
            {
                ERR_LOG("[GameManagerMasterImpl].onValidateConfig: GameManagerServerConfig.packetLossMetricQuantiles elements cannot exceed: " << MAX_PACKET_LOSS << "%.");
                return false;
            }
        }

        // Validate Input Sanitizers:
        if (!inputSanitizerConfigValidation(config.getInputSanitizers(), validationErrors))
            return false;

        if (!validateGamePermissions(config, validationErrors))
        {
            ERR_LOG("[GameManagerMasterImpl].onValidateConfig: Configuration problem while validating preconfig for GamePermissions.");
            return false;
        }

        return true;
    }

    bool GameManagerMasterImpl::onPrepareForReconfigure(const GameManagerServerConfig& newConfig)
    {
        if (!mExternalSessionServices.onPrepareForReconfigure(newConfig.getGameSession().getExternalSessions()))
        {
            return false;
        }
        return true;
    }
    
    void GameManagerMasterImpl::addUserSessionGameInfo(const UserSessionId userSessionId, const GameSessionMaster& gameSession, bool isNowActive /*= false*/, UserSessionInfo* userInfo /*=nullptr*/)
    {
        if (userSessionId == INVALID_USER_SESSION_ID)
        {
            ASSERT_LOG("[GameManagerMasterImpl].addUserSessionGameInfo: attempt to insert invalid session mapping for game: " << gameSession.getGameId() << ".");
            return; // should never happen, want to catch it at testing time, but harmless otherwise
        }
        GameIdSet& gameIdSet = mGameIdSetByUserSessionMap[userSessionId];
        gameIdSet.insert(gameSession.getGameId());
        if (isNowActive)
        {
            GameIdSetByUserSessionMap& activeMap = (gameSession.getGameType() == GAME_TYPE_GROUP) ? mActiveGameIdSetByUserSessionMapsGameGroups : mActiveGameIdSetByUserSessionMapsGames;
            GameIdSet& activeGameIdSet = activeMap[userSessionId];
            const bool wasEmpty = activeGameIdSet.empty();
            activeGameIdSet.insert(gameSession.getGameId());
            if (wasEmpty)
            {
                // This metric breaks if users can change their best ping site while still in-game. 
                mMetrics.mInGamePlayerSessions.increment(1, gameSession.getGameType(), (userInfo ? userInfo->getBestPingSiteAlias() : "unknown_pingsite"), (userInfo ? userInfo->getProductName() : "unset"));
            }
        }
    }

    void GameManagerMasterImpl::removeUserSessionGameInfo(const UserSessionId userSessionId, const GameSessionMaster& gameSession, bool wasActive /*=false*/, UserSessionInfo* userInfo /*=nullptr*/)
    {
        if (userSessionId == INVALID_USER_SESSION_ID)
            return; // early out on invalid session, unused virtual ded servers have these

        GameIdSetByUserSessionMap::iterator it = mGameIdSetByUserSessionMap.find(userSessionId);
        if (it != mGameIdSetByUserSessionMap.end())
        {
            it->second.erase(gameSession.getGameId());
            if (it->second.empty())
                mGameIdSetByUserSessionMap.erase(userSessionId);
        }

        if (wasActive)
        {
            GameIdSetByUserSessionMap& activeMap = (gameSession.getGameType() == GAME_TYPE_GROUP) ? mActiveGameIdSetByUserSessionMapsGameGroups : mActiveGameIdSetByUserSessionMapsGames;
            it = activeMap.find(userSessionId);
            if (it != activeMap.end())
            {
                const bool success = it->second.erase(gameSession.getGameId()) > 0;
                if (it->second.empty())
                {
                    activeMap.erase(userSessionId);
                    if (success)
                    {
                        // This metric breaks if users can change their best ping site while still in-game. 
                        mMetrics.mInGamePlayerSessions.decrement(1, gameSession.getGameType(), (userInfo ? userInfo->getBestPingSiteAlias() : "unknown_pingsite"), (userInfo ? userInfo->getProductName() : "unset"));
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief when a user logs out of blaze, we kick them out of any games they're still in.
    *************************************************************************************************/
    void GameManagerMasterImpl::onUserSessionExtinction(const UserSession& userSession)
    {
        TRACE_LOG("[GameManagerMaster] Player(" << userSession.getBlazeId() << ") logging out");
        removeUserFromAllGames(userSession.getUserSessionId(), userSession.getBlazeId());
    }

    /*! ************************************************************************************************/
    /*! \brief remove a user from all of the games they're in and delete their UserSessionGameInfoMap entry
    ***************************************************************************************************/
    void GameManagerMasterImpl::removeUserFromAllGames(UserSessionId sessionId, BlazeId blazeId)
    {
        RemovePlayerMasterRequest removePlayerRequest;
        removePlayerRequest.setPlayerId(blazeId);
        removePlayerRequest.setPlayerRemovedReason(BLAZESERVER_CONN_LOST);

        GameIdSet processedGameIdSet; // keeps track of games already processed
        GameIdSetByUserSessionMap::iterator it;
        // Since master is now blocking, we need to be extra safe about how we iterate secondary indexes
        // while performing blocking operations. This loop ensures that we always purge all outstanding games
        // before ensuring that the mapping mGameIdSetByUserSessionMap[sessionId] = [...] itself is eliminated.
        while ((it = mGameIdSetByUserSessionMap.find(sessionId)) != mGameIdSetByUserSessionMap.end())
        {
            if (it->second.empty())
            {
                // shouldn't happen, but nothing to do
                break;
            }
            size_t skipped = 0;
            GameIdSet gameIdSet = it->second; // make local copy we can safely iterate
            for (GameIdSet::const_iterator i = gameIdSet.begin(), e = gameIdSet.end(); i != e; ++i)
            {
                GameId gameId = *i;

                if (!processedGameIdSet.insert(gameId).second)
                {
                    ++skipped;
                    continue;
                }

                GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
                if (EA_UNLIKELY(gameSessionMaster == nullptr))
                {
                    ERR_LOG("[GameManagerMaster] blaze id " << blazeId << 
                        " shows himself as a member of a non-existent game (gameId: " << gameId << ")!");
                    continue;
                }

                // leaving players can call removePlayer on themselves, but leaving dedicated server hosts and external owners, must call destroyGame
                bool isExternalOwnerUserSession = (gameSessionMaster->hasExternalOwner() && (sessionId == gameSessionMaster->getExternalOwnerSessionId()));
                bool isDedicatedServerHost = ( (sessionId == gameSessionMaster->getDedicatedServerHostSessionId()) && 
                    (gameSessionMaster->hasDedicatedServerHost()) );
                if (!isDedicatedServerHost && !isExternalOwnerUserSession)
                {
                    // Take no action on a reservation. They last until timed out or claimed.
                    PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(blazeId);
                    if (playerInfo != nullptr && playerInfo->isReserved())
                    {
                        continue;
                    }

                    // Also if player has multiple user sessions, only exit game if the logging out user session is the 
                    // player's main user session in the game. E.g. it could've reserve/queued via a HTTP/gRPC user session, but
                    // reservation-claimed via another user session later.
                    if ((playerInfo != nullptr) && (playerInfo->getPlayerSessionId() != sessionId))
                    {
                        continue;
                    }

                    // leave the game
                    removePlayerRequest.setGameId(gameId);
                    // Note: this alters the size of playerGameList
                    RemovePlayerMasterError::Error removePlayerError;
                    removePlayer(&removePlayerRequest, blazeId, removePlayerError);
                    if (removePlayerError != RemovePlayerMasterError::ERR_OK)
                    {
                        TRACE_LOG("[GameManagerMaster] Error removing player(" << blazeId << 
                            ") from game(" << gameId << "). Remove player error " << (ErrorHelp::getErrorName( (BlazeRpcError)removePlayerError)) << ".");
                    }
                }
                else
                {
                    if (gameSessionMaster->getGameSettings().getVirtualized())
                    {
                        // The dedicated topology host of a virtual game doesn't destroy the game, 
                        // but just ejects itself as the host. 
                        gameSessionMaster->ejectHost(sessionId, blazeId);
                    }
                    else
                    {
                        // destroy the game (dedicated server host can't call leave game; they're not a player)
                        BlazeRpcError destroyError = (BlazeRpcError)destroyGame(gameSessionMaster.get(), HOST_LEAVING);
                        if (destroyError != ERR_OK)
                        {
                            TRACE_LOG("[GameManagerMaster] Error removing host player(" << blazeId <<
                                ") from dedicated server game(" << gameId << "). Destroy game error " << (ErrorHelp::getErrorName(destroyError)) << ".");
                        }
                    }
                }
            }
            if (skipped == gameIdSet.size())
            {
                // finished processing all the games
                break;
            }
        }

        mGameIdSetByUserSessionMap.erase(sessionId); // safety net here, to ensure we can't possibly leave a mapping in mGameIdSetByUserSessionMap after leaving this method

        // another safety net, erasing from all the 'active' maps (should never actually erase anything)
        mActiveGameIdSetByUserSessionMapsGames.erase(sessionId);
        mActiveGameIdSetByUserSessionMapsGameGroups.erase(sessionId);
    }
    
    const GameSessionMaster* GameManagerMasterImpl::getReadOnlyGameSession(GameId id) const
    {
        return getGameSessionMaster(id);
    }

    GameSessionMasterPtr GameManagerMasterImpl::getWritableGameSession(GameId id, bool autoCommit)
    {
        GameSessionMaster* game = getGameSessionMaster(id);
        if (autoCommit && game != nullptr)
            game->setAutoCommitOnRelease();
        return game;
    }

    void GameManagerMasterImpl::eraseGameData(GameId gameId)
    {
        GameSessionMasterByIdMap::const_iterator it = mGameSessionMasterByIdMap.find(gameId);
        if (it != mGameSessionMasterByIdMap.end())
        {
            GameSessionMasterPtr gameSession = it->second;
            mGameSessionMasterByIdMap.erase(gameId);
            mGameTable.eraseRecord(gameId);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return the GameNetworkBehavior for a game topology.
    ***************************************************************************************************/
    GameNetworkBehavior* GameManagerMasterImpl::getGameNetworkBehavior(GameNetworkTopology gameNetworkTopology)
    {
        // GM_AUDIT: mildly inefficient - we should change the topology enum to be 0..3, and just use an array or vector

        GameNetworkBehaviorMap::iterator iter = mGameNetworkBehaviorMap.find(gameNetworkTopology);
        if (EA_UNLIKELY( iter == mGameNetworkBehaviorMap.end() ))
        {
            return nullptr;
        }

        return iter->second;
    }

    /*! ************************************************************************************************/
    /*! \brief return the next GameReportingId if game reporting enabled for the game type.
    ***************************************************************************************************/
    GameReportingId GameManagerMasterImpl::getNextGameReportingId() 
    {
        GameReportingId id = GameManager::INVALID_GAME_REPORTING_ID;
        BlazeRpcError rc = GameReportingIdHelper::getNextId(id);
        if (rc != ERR_OK)
        {
            WARN_LOG("[GameManagerMasterImpl].getNextGameReportingId: failed to get the next game reporting id. Err: " << ErrorHelp::getErrorName(rc));
        }
        return id;
    }

    /*! ************************************************************************************************/
    /*! \brief create game RPC. This RPC is blockable until finish doing the validation of persisted id
    ***************************************************************************************************/
    CreateGameMasterError::Error GameManagerMasterImpl::processCreateGameMaster(const CreateGameMasterRequest &request, 
        CreateGameMasterResponse &response, Blaze::GameManager::CreateGameMasterErrorInfo &error, const Message *message)
    {
        BlazeRpcError blazeErr = populateRequestCrossplaySettings(const_cast<CreateGameMasterRequest&>(request));
        if (blazeErr != Blaze::ERR_OK)
        {
            return CreateGameMasterError::commandErrorFromBlazeError(blazeErr);
        }

        // the creators reason to send back to the client as to why they joined the game.
        GameSetupReason creatorJoinContext;
        creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        creatorJoinContext.getDatalessSetupContext()->setSetupContext(CREATE_GAME_SETUP_CONTEXT);

        // PG member reasons to send back to the client for as to why they joined the game.
        GameSetupReason pgContext;
        pgContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
		pgContext.getIndirectJoinGameSetupContext()->setUserGroupId(request.getCreateRequest().getPlayerJoinData().getGroupId());       // WARNING:  This is wrong.  This will be the creator of the CG game, not the group that started matchmaking. 
        pgContext.getIndirectJoinGameSetupContext()->setRequiresClientVersionCheck(getConfig().getGameSession().getEvaluateGameProtocolVersionString());

        // Creators join game reason.
        UserSessionSetupReasonMap setupReasons;
        fillUserSessionSetupReasonMap(setupReasons, request.getUsersInfo(), &creatorJoinContext, &pgContext, nullptr);

        CreateGameMasterError::Error createErr = createGame(request, response, setupReasons, &error);
        if (createErr == CreateGameMasterError::Error::ERR_OK)
        {
            const GameSessionMaster* newGame = getReadOnlyGameSession(response.getCreateResponse().getGameId());

            if ((newGame != nullptr) && (newGame->getPlayerRoster()->getRosterPlayerCount() > 0))
            {
                RelatedPlayerSetsMap joiningPlayersLists;
                buildRelatedPlayersMap(newGame, request.getUsersInfo(), joiningPlayersLists);
                buildAndSendMatchJoinEvent(*newGame, setupReasons, joiningPlayersLists);
            }
        }
        return createErr;
    }

    /*! ************************************************************************************************/
    /*! \brief createGame helper method
    ***************************************************************************************************/
    CreateGameMasterError::Error GameManagerMasterImpl::createGame( const CreateGameMasterRequest& masterRequest, CreateGameMasterResponse& response, const UserSessionSetupReasonMap &setupReasons,
        Blaze::GameManager::CreateGameMasterErrorInfo *errorInfo)
    {
        const CreateGameRequest& request = masterRequest.getCreateRequest();
        if ((request.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION) && 
            (isPlayerHostedTopology(request.getGameCreationData().getNetworkTopology()) 
                || ((request.getGameCreationData().getNetworkTopology() == NETWORK_DISABLED) && getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions())))
        {
            return CreateGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
        }

        if ((request.getCommonGameData().getGameType() == GAME_TYPE_GROUP) && 
            isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology()))
        {
            return CreateGameMasterError::GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
        }

        OwnedSliverRef sliverRef = mGameTable.getSliverOwner().getLeastLoadedOwnedSliver();
        if (sliverRef == nullptr)
        {
            // This should never happen, SliverManager ensures that creaton RPCs never get routed to an owner that has no ready slivers
            ERR_LOG("[GameManagerMasterImpl].createGame: failed to obtain owned sliver ref.");
            return CreateGameMasterError::ERR_SYSTEM;
        }
        
        Sliver::AccessRef sliverAccess;
        if (!sliverRef->getPrioritySliverAccess(sliverAccess))
        {
            ERR_LOG("[GameManagerMasterImpl].createGame: Could not get priority sliver access for SliverId(" << sliverRef->getSliverId() << ")");
            return CreateGameMasterError::ERR_SYSTEM;
        }
        
        GameId gameId = GameManager::INVALID_GAME_ID;
        BlazeRpcError rc = gUniqueIdManager->getId(RpcMakeSlaveId(GameManagerMaster::COMPONENT_ID), GAMEMANAGER_IDTYPE_GAME, gameId);
        if (rc == ERR_OK)
        {
            gameId = BuildSliverKey(sliverRef->getSliverId(), gameId);
        }
        else
        {
            ERR_LOG("[GameManagerMasterImpl].createGame: failed to get the next game id. Err: " << ErrorHelp::getErrorName(rc));
            return CreateGameMasterError::ERR_SYSTEM;
        }

        if (IS_LOGGING_ENABLED(Logging::TRACE)) 
        {
            eastl::string attrStr;
            Collections::AttributeMap::const_iterator gameAttribIter = request.getGameCreationData().getGameAttribs().begin();
            Collections::AttributeMap::const_iterator gameAttribEnd = request.getGameCreationData().getGameAttribs().end();
            for(; gameAttribIter != gameAttribEnd; gameAttribIter++)
            {
                attrStr.append_sprintf("%s=%s ", gameAttribIter->first.c_str(), gameAttribIter->second.c_str());
            }

            TRACE_LOG("[GameManagerMaster] Creating game(" << gameId << ":" << request.getGameCreationData().getGameName() 
                << "), attributes( " << attrStr.c_str() << "), settings(" << SbFormats::HexLower(request.getGameCreationData().getGameSettings().getBits()) << ")");
        }

        GameReportingId gameReportId = getNextGameReportingId();
        if (gameReportId == GameManager::INVALID_GAME_REPORTING_ID)
        {
            ERR_LOG("[GameManagerMaster] Failed to generate game reporting id for game(" << gameId << ":" << request.getGameCreationData().getGameName() 
                << "), settings(" << SbFormats::HexLower(request.getGameCreationData().getGameSettings().getBits()) << ").");
            return CreateGameMasterError::ERR_SYSTEM;
        }

        // generate new persisted id and secret, or check if the uuid matches the secret if uuid in the request is non-empty, as needed.
        PersistedGameId persistedId;
        PersistedGameIdSecret persistedIdSecret;
        GameId existingGameId;
        // validate if the persisted game id exists.
        SliverId sliverId = sliverRef->getSliverId();
        BlazeRpcError validateErr = validateOrCreate(request, sliverId, persistedId, persistedIdSecret, existingGameId);
        if (validateErr != ERR_OK)
        {
            if (validateErr == GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE)
                errorInfo->setPreExistingGameIdWithPersistedId(existingGameId);

            return CreateGameMasterError::commandErrorFromBlazeError(validateErr);
        }

        // Disallow blocking after the persisted id validation finishes
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

        // cache off game id reference by persisted game id.
        if (request.getGameCreationData().getGameSettings().getEnablePersistedGameId() && (persistedId.c_str()[0] != '\0'))
        {
            GameIdByPersistedIdMap::insert_return_type ret = mGameIdByPersistedIdMap.insert(persistedId.c_str());
            if (!ret.second)
            {
                // If somehow crossed wire with another create using same persisted id, this attempt should fail or join instead.
                WARN_LOG("[GameManagerMaster] Failed to create new game, as a game(" << ret.first->second << ") already exists with the requested persisted id(" << persistedId.c_str() << "). This may be caused by edge timing conditions. Returning error code to internal caller, which may attempt to join the game instead.");
                if (errorInfo != nullptr)
                {
                    errorInfo->setPreExistingGameIdWithPersistedId(ret.first->second);
                }
                return CreateGameMasterError::GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE;
            }
            ret.first->second = gameId;
            // Note: immediately after mGameIdByPersistedIdMap's insert, call createAndLockGameSession below so the insert+lock
            // is atomic. This ensures no one tries persistedId-joining the game session before we finish creating it.
        }

        GameSessionMasterByIdMap::insert_return_type ret = mGameSessionMasterByIdMap.insert(gameId);
        if (!ret.second)
        {
            WARN_LOG("[GameManagerMaster] Failed to create new game, game(" << gameId << ") already exists.");
            return CreateGameMasterError::ERR_SYSTEM;
        }
        GameSessionMasterPtr gameSessionMaster = BLAZE_NEW GameSessionMaster(gameId, sliverRef);
        ret.first->second = gameSessionMaster;

        gameSessionMaster->setGameReportingId(gameReportId);
        gameSessionMaster->getReplicatedGameSession().setIsPseudoGame(masterRequest.getIsPseudoGame());
        if (masterRequest.getIsPseudoGame())
        {
            mPseudoGameIdList.push_back(gameId);
        }

        CreateGameMasterError::Error err;

        err = gameSessionMaster->initialize(masterRequest, setupReasons, persistedId, persistedIdSecret, response.getExternalSessionParams());
        
        if (EA_UNLIKELY( err != CreateGameMasterError::ERR_OK))
        {
            // destroyGame is aware of the gameSessionMaster's mem status and cleans up appropriately
            //   (depending on the outcome of init, the GSM either needs to be deleted OR erased from the replicated game map (the replicator deletes the item).
            TRACE_LOG("[GameManagerMaster] create game(" << gameSessionMaster->getGameId() << ") failed trying to initialize.  Error: \"" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << "\"");
            if (masterRequest.getFinalizingMatchmakingSessionId() != INVALID_MATCHMAKING_SESSION_ID)
            {
                gameSessionMaster->removeMatchmakingQosData(masterRequest.getUsersInfo());
            }

            destroyGame(gameSessionMaster.get(), SYS_CREATION_FAILED);
            return err;
        }
        else
        {
            // game created successfully (and stored in the replicated game map)

            // we also cache off dedicated servers in a separate container (for quick access when resetting a dedicated server)
            if (gameSessionMaster->hasDedicatedServerHost())
            {
                logDedicatedServerEvent("game created", *gameSessionMaster);
            }
        }

        gameSessionMaster->setAutoCommitOnRelease();
      
        // populate the response obj
        response.getCreateResponse().setGameId(gameSessionMaster->getGameId());

        bool isVirtualWithReservation = (request.getGameCreationData().getGameSettings().getVirtualized() && (request.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION) && !masterRequest.getUsersInfo().empty());

        // If the game is created by a user group with more than one member, join the other members
        // Or, if virtual with reservation, this list is 1 or more reservations needed joined:
        if (masterRequest.getUsersInfo().size() > 1 || isVirtualWithReservation)
        {
            EntryCriteriaError error;
            JoinGameByGroupMasterError::Error joinGameByGroupErr;

            JoinGameByGroupMasterRequest groupRequest;
            groupRequestFromCreateRequest(masterRequest, gameSessionMaster->getGameId(), groupRequest);

            if (isVirtualWithReservation)
            {
                // Ensure that no user is listed as the 'host', since in the virtual reservations case everyone should get a reservation. 
                for (auto& curUserInfo : groupRequest.getUsersInfo())
                    curUserInfo->setIsHost(false);
            }

            JoinGameMasterResponse groupResponse;

            // Pre: to ensure the group joins together, everything since leader joined must be atomic

            joinGameByGroupErr = gameSessionMaster->joinGameByGroupMembersOnly(groupRequest, groupResponse, error, setupReasons);

            if (EA_UNLIKELY(joinGameByGroupErr != Blaze::GameManager::JoinGameByGroupMasterError::ERR_OK))
            {
                // unlikely as the game group should always get in the new game.
                WARN_LOG("[GameManagerMaster].createGame: prejoinGroup failed with err " << joinGameByGroupErr << "(" 
                          << (ErrorHelp::getErrorName((BlazeRpcError)joinGameByGroupErr)) << "), failedCriteria(" << error.getFailedCriteria() << ")");
            }
            // add the joined external ids to the create response.
            externalSessionJoinParamsToOtherResponse(groupResponse.getExternalSessionParams(), response.getExternalSessionParams());
            joinedReservedPlayerIdentificationsToOtherResponse(groupResponse.getJoinResponse(), response.getCreateResponse());
        }

        if (!gameSessionMaster->isResetable())
            addGameToCensusCache(*gameSessionMaster);
        
        // set up MM QoS data as needed
        // build out our QoS tracking list
        if (!GameSessionMaster::isPerformQosValidationListEmpty(masterRequest.getUsersInfo()) && (masterRequest.getFinalizingMatchmakingSessionId() != INVALID_MATCHMAKING_SESSION_ID))
        {
            // lock down the game to handle verification of connection quality.
            gameSessionMaster->setGameState(CONNECTION_VERIFICATION);

        }

        return CreateGameMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Create a joinGameByGroupMasterRequest from a create request.
    ***************************************************************************************************/
    void GameManagerMasterImpl::groupRequestFromCreateRequest(const CreateGameMasterRequest& createRequest, GameId gameId, 
        JoinGameByGroupMasterRequest& groupRequest) const
    {
        // Even though this is just a group join, we still have to include the host information:
        createRequest.getUsersInfo().copyInto(groupRequest.getUsersInfo());
        createRequest.getExternalSessionData().copyInto(groupRequest.getExternalSessionData());

        groupRequest.getJoinRequest().setGameId(gameId);
        groupRequest.getJoinRequest().setJoinMethod(SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME);

        createRequest.getCreateRequest().getPlayerJoinData().copyInto(groupRequest.getJoinRequest().getPlayerJoinData());
        groupRequest.getJoinRequest().getCommonGameData().setGameProtocolVersionString(createRequest.getCreateRequest().getCommonGameData().getGameProtocolVersionString());
    }
    
    /*! ************************************************************************************************/
    /*! \brief Destroys the supplied game session. The user destroying the game must be an admin.

        \param[in]  request - Destroy game request.
        \return ERR_OK or a specific failure error
    ***************************************************************************************************/
    DestroyGameMasterError::Error GameManagerMasterImpl::processDestroyGameMaster( const DestroyGameRequest &request,
        DestroyGameResponse &response, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return DestroyGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // check player's admin rights, soft check on the batch destroy permission because we expect that most callers won't have that, and we do a hard check in the batch RPC on the slave
        if (!checkPermission(*gameSessionMaster, GameAction::DESTROY_GAME) && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_BATCH_DESTROY_GAMES, true))
        {
            return DestroyGameMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        response.setGameId(gameSessionMaster->getGameId());
        return destroyGame(gameSessionMaster.get(), request.getDestructionReason());
    }

    DestroyAllPseudoGamesMasterError::Error GameManagerMasterImpl::processDestroyAllPseudoGamesMaster(const Message *)
    {
        // Spawn a fiber to manage the deletes (since it's going to take several seconds):
        gFiberManager->scheduleCall<GameManagerMasterImpl, GameIdList*>(this, &GameManagerMasterImpl::destroyAllPseudoGamesFiber, &mPseudoGameIdList);
        mPseudoGameIdList.clear();

        return DestroyAllPseudoGamesMasterError::ERR_OK;
    }

    void GameManagerMasterImpl::destroyAllPseudoGamesFiber(GameIdList* tempIdList)
    {
        // Destroy games over time:
        GameIdList gameIdList;
        gameIdList.swap(*tempIdList);

        TimeValue startTime = TimeValue::getTimeOfDay();
        
        uint32_t gameRemoveCounter = 0;
        DestroyGameRequest req;
        DestroyGameResponse response;
        while (!gameIdList.empty())
        {
            req.setGameId(gameIdList.back());
            gameIdList.pop_back();
            processDestroyGameMaster(req, response, nullptr);

            // Destroy 100 games at a time, to avoid stalling the server:
            ++gameRemoveCounter;
            if (gameRemoveCounter > getConfig().getPseudoGamesConfig().getPseudoGamesDeleteRate())
            {
                gameRemoveCounter = 0;
                if (Fiber::sleep(0, "DestroyAllPseudoGames short sleep") == ERR_CANCELED)
                {
                    return;
                }
            }
        }

        TimeValue totalTime = TimeValue::getTimeOfDay() - startTime;
        INFO_LOG("GameManagerMasterImpl::destroyAllPseudoGamesThread: Removed " << gameIdList.size() << " pseudo games in " << totalTime.getSec() << " seconds" );
    }

    void GameManagerMasterImpl::removeGameFromConnectionMetrics(const GameId gameId)
    {
        mMetrics.mLatencyByConnGroups.erase(gameId);
        mMetrics.mCurrentPacketLossByConnGroup.erase(gameId);
        mMetrics.mConnectionStatsEventListByConnGroup.erase(gameId);
    }

    /*! ************************************************************************************************/
    /*! \brief Destroy the supplied game.  Note: admin rights are not checked.

        \param[in] gameSessionMaster - the game to destroy
        \param[in] destructionReason - the reason the game is being destroyed
        \param[in] removedPlayerId - if the game is being destroyed due to a player being removed, list the player 
                    so game reporting can treat them as a player who finished the game (instead of a dnf player)
        \return ERR_OK, or a specific failure reason.
    ***************************************************************************************************/
    DestroyGameMasterError::Error GameManagerMasterImpl::destroyGame(GameSessionMaster* gameSessionMaster,
        GameDestructionReason destructionReason, PlayerId removedPlayerId /* = 0 */)
    {
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return DestroyGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        const GameId gameId = gameSessionMaster->getGameId();

        // send game finished message to game reporting (if the game was created successfully)
        if (destructionReason != SYS_CREATION_FAILED)
        {
            //Any started game ends now
            gameSessionMaster->gameFinished(removedPlayerId);

            // send notification to topology host that game is removed
            if (gameSessionMaster->hasDedicatedServerHost())
            {
                NotifyGameRemoved notifyGameRemoved;
                notifyGameRemoved.setGameId(gameId);
                notifyGameRemoved.setDestructionReason(destructionReason);
                sendNotifyGameRemovedToUserSessionById(gameSessionMaster->getDedicatedServerHostSessionId(), &notifyGameRemoved);                
            }
            if (gameSessionMaster->hasExternalOwner())
            {
                NotifyGameRemoved notifyGameRemoved;
                notifyGameRemoved.setGameId(gameId);
                notifyGameRemoved.setDestructionReason(destructionReason);
                sendNotifyGameRemovedToUserSessionById(gameSessionMaster->getExternalOwnerSessionId(), &notifyGameRemoved);
            }
        }

        // clear persisted game id cache.
        gGameManagerMaster->removePersistedGameIdMapping(*gameSessionMaster);

        // update game list by attribute map (for census data)
        removeGameFromCensusCache(*gameSessionMaster);

        gameSessionMaster->destroyGame(destructionReason);
        
        return DestroyGameMasterError::ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief Reset a dedicated server (chosen at random from the available servers near my datacenter)

        \param[in]  request -  create game request which includes all settings for the dedicated server to be reset.
        \param[out] response - response after a dedicated server is reset. This includes gameid generated for the
                                game. A gameid will be the handle to further deal with any operations on the game session.

        \return - An error code is returned after finishing the operation. ERR_OK returned for successful dedicate server reset.
    ****************************************************************************************************/
    ResetDedicatedServerMasterError::Error GameManagerMasterImpl::processResetDedicatedServerMaster(
        const CreateGameMasterRequest& request, JoinGameMasterResponse& response, const Message *message)
    {
        BlazeRpcError blazeErr = populateRequestCrossplaySettings(const_cast<CreateGameMasterRequest&>(request));
        if (blazeErr != Blaze::ERR_OK)
        {
            return ResetDedicatedServerMasterError::commandErrorFromBlazeError(blazeErr);
        }

        GameSetupReason creatorJoinContext;
        if (request.getCreateRequest().getGameCreationData().getIsExternalCaller()) //for client to complete join, treat as indirect
        {
            creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
			creatorJoinContext.getIndirectJoinGameSetupContext()->setUserGroupId(request.getCreateRequest().getPlayerJoinData().getGroupId());       // WARNING:  This is wrong.  This will be the creator of the CG game, not the group that started matchmaking. 
        }
        else
            creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_RESETDEDICATEDSERVERSETUPCONTEXT);

        GameSetupReason pgContext;
        pgContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
		pgContext.getIndirectJoinGameSetupContext()->setUserGroupId(request.getCreateRequest().getPlayerJoinData().getGroupId());       // WARNING:  This is wrong.  This will be the creator of the CG game, not the group that started matchmaking. 

        UserSessionSetupReasonMap setupReasons;

        fillUserSessionSetupReasonMap(setupReasons, request.getUsersInfo(), &creatorJoinContext, &pgContext, nullptr);

        GameSessionMasterPtr gameSessionMaster;
        blazeErr = resetDedicatedServer(request, setupReasons, gameSessionMaster, response);
        if (blazeErr == ERR_OK)
        {
            response.getJoinResponse().setGameId(gameSessionMaster->getGameId());
            //build & submit mp_match_join event for all roster members during reset
            if (gameSessionMaster->getPlayerRoster()->getRosterPlayerCount() > 0)
            {
                RelatedPlayerSetsMap joiningPlayersLists;
                buildRelatedPlayersMap(gameSessionMaster.get(), request.getUsersInfo(), joiningPlayersLists);
                buildAndSendMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists);
            }
        }

        return ResetDedicatedServerMasterError::commandErrorFromBlazeError(blazeErr);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to select & reset an available dedicated server (the joining user's session will join the game)
    
        \param[in] UserSessionSetupReasonMap - the replication reason tdf to use when inserting the player into the map.
        \param[in,out] dedicatedServer - the dedicated server that's reset; if nullptr, we select a dedicated server randomly, if not we reset the provided server
    ***************************************************************************************************/
    BlazeRpcError GameManagerMasterImpl::resetDedicatedServer(const CreateGameMasterRequest& createGameRequest,         
        const UserSessionSetupReasonMap &setupReasons, GameSessionMasterPtr& dedicatedServer, JoinGameMasterResponse& joinResponse)
    {
        // If the dedicated server is null then we search for one.
        GameSessionMasterPtr availableGame;
        if (dedicatedServer == nullptr)
        {
            // find an available dedicated server
            availableGame = chooseAvailableDedicatedServer(createGameRequest.getGameIdToReset());
            if (EA_UNLIKELY(!availableGame))
            {
                // no dedicated servers are available for reset
                TRACE_LOG("[GameManagerMaster] no dedicated server is found.");
                return Blaze::GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
            }
        }
        else
        {
            // use the passed in dedicated server.
            availableGame = dedicatedServer;
        }

        PersistedGameId persistedId;
        PersistedGameIdSecret persistedIdSecret;
        GameId existingGameId;
        // validate if the persisted game id exists.
        SliverId sliverId = GetSliverIdFromSliverKey(availableGame->getGameId());
        BlazeRpcError validateErr = validateOrCreate(createGameRequest.getCreateRequest(), sliverId, persistedId, persistedIdSecret, existingGameId);
        if (validateErr != ERR_OK)
        {
            availableGame->returnDedicatedServerToPool();
            return validateErr;
        }

        // erase this games previous persisted id association.  Resetting will provide us with a new id.
        gGameManagerMaster->removePersistedGameIdMapping(*availableGame, true);

        // Disallow blocking after the persisted id validation/cleanup finishes
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

        // Associate this server with its new persisted game id.
        if (createGameRequest.getCreateRequest().getGameCreationData().getGameSettings().getEnablePersistedGameId() && (persistedId.c_str()[0] != '\0'))
        {
            GameIdByPersistedIdMap::insert_return_type ret = mGameIdByPersistedIdMap.insert(persistedId.c_str());
            if (!ret.second)
            {
                // If somehow crossed wire with another reset using same persisted id, this attempt should fail or join instead.
                WARN_LOG("[GameManagerMaster] Failed to reset game, as a game(" << ret.first->second << ") already exists with the requested persisted id(" << persistedId.c_str() << ").");
                return GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE;
            }
            ret.first->second = availableGame->getGameId();
        }

        // Accounts for pg members and reserved players.
        SlotTypeSizeMap slotTypeSizeMap;
        ValidationUtils::populateSlotTypeSizes(createGameRequest, slotTypeSizeMap);

        // This is a double check.  Would also get caught by the subsequent join
        validateErr = ValidationUtils::validateGameCapacity(slotTypeSizeMap, createGameRequest.getCreateRequest());
        if (EA_UNLIKELY(validateErr != ERR_OK))
        {
            return validateErr;
        }

        // Check teams, note that we do not account for reserved slots in the team capacity checks.
        // I'm allowed to reserve slots for players that may join another team, this is also done by Matchmaking.
        validateErr = ValidationUtils::validateTeamCapacity(slotTypeSizeMap, createGameRequest.getCreateRequest(), false);
        if (EA_UNLIKELY(validateErr != ERR_OK))
        {
            return validateErr;
        }
        
        // reset the dedicated server
        TRACE_LOG("[GameManagerMaster] Reseting dedicated server(" << availableGame->getGameId() << ":" << availableGame->getGameName() << ")");
        BlazeRpcError resetErr =  availableGame->resetDedicatedServer(createGameRequest, setupReasons, persistedId, persistedIdSecret);
        if ( resetErr != Blaze::ERR_OK )
        {
            // reset failed. Clean up the game and return it to the pool.
            availableGame->returnDedicatedServerToPool();
            return resetErr;
        }

        // Append player host to joined external ids list so caller can update external session.
        const UserSessionInfo* hostUserSessionInfo = getHostSessionInfo(createGameRequest.getUsersInfo());
        if (hostUserSessionInfo == nullptr)
        {
            WARN_LOG("[GameManagerMaster] Missing host player for call to reset dedicated server.");
            return GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER;
        }
        updateExternalSessionJoinParams(joinResponse.getExternalSessionParams(), *hostUserSessionInfo,
            (createGameRequest.getCreateRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION), *availableGame);

        // NOTE: roster should be replicated after the RESET notification.  Players are not expected
        // to be in a RESETABLE game.
        BlazeRpcError joinErr = joinPlayersToDedicatedServer(*availableGame, createGameRequest, setupReasons, joinResponse);
        if ( joinErr == Blaze::ERR_OK)
        {
            dedicatedServer = availableGame;
        }
        else
        {
            // clear up MM qos data
            if (createGameRequest.getFinalizingMatchmakingSessionId() != INVALID_MATCHMAKING_SESSION_ID)
            {
                availableGame->removeMatchmakingQosData(createGameRequest.getUsersInfo());
            }
            availableGame->returnDedicatedServerToPool();
        }

        return joinErr;
    }

    Blaze::BlazeRpcError GameManagerMasterImpl::joinPlayersToDedicatedServer( GameSessionMaster& availableGame, const CreateGameMasterRequest& masterRequest, 
        const UserSessionSetupReasonMap &setupReasons, JoinGameMasterResponse& joinResponse)
    {        
        const CreateGameRequest& createGameRequest = masterRequest.getCreateRequest();
        UserJoinInfoPtr hostUserJoinInfo = nullptr;
        const UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo(), &hostUserJoinInfo);
        if (hostSessionInfo == nullptr)
        {
            WARN_LOG("[GameManagerMaster] Missing host player for call to joinPlayersToDedicatedServer.");
            return GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER;
        }

        UserSessionId joiningSessionId = hostSessionInfo->getSessionId();
        BlazeId joiningBlazeId = hostSessionInfo->getUserInfo().getId();

        UserSessionSetupReasonMap::const_iterator sessIter = setupReasons.find(joiningSessionId);
        if (sessIter == setupReasons.end())
        {
            ERR_LOG("[GameSessionMaster] Reset dedicated server request with no setup reason entry for joining session(" << joiningSessionId << ")");
            return Blaze::ERR_SYSTEM;
        }
        auto& joinReason = *sessIter->second; 

        // join the resetting player to the newly reset game
        JoinGameMasterRequest joinRequest;
        joinRequest.setUserJoinInfo(*hostUserJoinInfo);
        masterRequest.getExternalSessionData().copyInto(joinRequest.getExternalSessionData());
        joinRequest.getJoinRequest().setGameId(availableGame.getGameId());
        joinRequest.getJoinRequest().setJoinMethod(SYS_JOIN_BY_RESETDEDICATEDSERVER); 
        createGameRequest.getCommonGameData().copyInto(joinRequest.getJoinRequest().getCommonGameData());       // ProtocolVersion
        createGameRequest.getPlayerJoinData().copyInto(joinRequest.getJoinRequest().getPlayerJoinData());
        
        if (createGameRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_CLAIM_RESERVATION)
        {
            joinRequest.getJoinRequest().getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
        }
        
        if (createGameRequest.getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION
            && createGameRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
        {
            // error if network address not present for active hosts
            ERR_LOG("[GameManagerMaster] joinPlayersToDedicatedServer for user (" << joiningBlazeId << ") and game (" << availableGame.getGameId() << ") failed, no host network addresses were available.");
            return Blaze::ERR_SYSTEM;
        }

        EntryCriteriaError error;
        JoinGameMasterError::Error joinError = availableGame.joinGame(joinRequest, joinResponse, error, joinReason);
        if ( EA_UNLIKELY(joinError != JoinGameMasterError::ERR_OK) )
        {
            TRACE_LOG("[GameSessionMaster] Reset dedicated server - resetter failed to join game with " << joinError << " (" << (ErrorHelp::getErrorName(static_cast<BlazeRpcError>(joinError))) << ")");
            // if player failed to join the reset server, roll back the reset or create...
            return Blaze::GAMEMANAGER_ERR_JOIN_PLAYER_FAILED;
        }

        // Join the game group members
        JoinGameByGroupMasterRequest groupRequest;
        groupRequestFromCreateRequest(masterRequest, availableGame.getGameId(), groupRequest);
        if (groupRequest.getUsersInfo().size() > 1)
        {
            groupRequest.getJoinRequest().setJoinMethod(SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER);

            // Pre: to ensure the group joins together, everything since leader joined must be atomic
            JoinGameByGroupMasterError::Error err = availableGame.joinGameByGroupMembersOnly(groupRequest, joinResponse, error, setupReasons);

            if (EA_UNLIKELY(err != JoinGameByGroupMasterError::ERR_OK))
            {
                // unlikely as the game group should always get in the new game.
                TRACE_LOG("[GameManagerMaster] PrejoinGroup failed with err " << err << "(" << (ErrorHelp::getErrorName(static_cast<BlazeRpcError>(err))) << "), failedCriteria(" << error.getFailedCriteria() << ")");
            }
        }

        return Blaze::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Returns a resettable dedicated server from the given list.

        \param[in] gamesToReset - list of dedicated servers that can be reset.
        \param[in] gamesWithMaxFitScore - the number of dedicated servers with the max fit score. These are the first to be chosen from.

        \return Returns a resettable dedicated server, or nullptr if none exist
    ***************************************************************************************************/
    GameSessionMasterPtr GameManagerMasterImpl::chooseAvailableDedicatedServer(GameId gameIdToReset)
    {
        mMetrics.mDedicatedServerLookups.increment();

        // This is a sorted list, and we want to know how many servers have matching fit scores:
        if (gameIdToReset == INVALID_GAME_ID)
        {
            mMetrics.mDedicatedServerLookupFailures.increment();
            mMetrics.mDedicatedServerLookupsNoGames.increment();
            WARN_LOG("[GameManagerMasterImpl] No available resettable dedicated servers found.");
            return nullptr;
        }


        GameId resetTargetGameId = gameIdToReset;
        GameSessionMasterPtr game = getWritableGameSession(resetTargetGameId);

        if (game == nullptr || game->getGameState() != RESETABLE)
        {
            TRACE_LOG("[GameManagerMasterImpl] Random choice resettable dedicated server was not accessible. The requesting Slave will continue randomly choosing servers.");
            game = nullptr;
            mMetrics.mDedicatedServerLookupFailures.increment();
        }

        return game;
    }

    /*! ************************************************************************************************/
    /*! \brief Calls chooseHostForInjection on all of the instances of the game manager master, passing in a list of games to try to inject into.

        \return bool - Was a game injected into?
    ***************************************************************************************************/
    bool GameManagerMasterImpl::getDedicatedServerForHostInjection(ChooseHostForInjectionRequest& request, ChooseHostForInjectionResponse &response, GameIdsByFitScoreMap& gamesToSend)
    {
        bool result = false;

        TRACE_LOG("GameManagerMasterImpl::getDedicatedServerForHostInjection: Attempting remote host injection with ("
            << gamesToSend.size() << ") possible fit scores.");

        // NOTE: fit score map is ordered by lowest score; therefore, iterate in reverse
        for (GameIdsByFitScoreMap::reverse_iterator gameIdsMapItr = gamesToSend.rbegin(), gameIdsMapEnd = gamesToSend.rend(); gameIdsMapItr != gameIdsMapEnd; ++gameIdsMapItr)
        {
            GameIdList* gameIds = gameIdsMapItr->second;
            // random walk gameIds
            while (!gameIds->empty())
            {
                GameIdList::iterator i = gameIds->begin() + Random::getRandomNumber((int32_t)gameIds->size());
                GameId gameId = *i;
                gameIds->erase_unsorted(i);

                request.setGameIdToReset(gameId);
                BlazeRpcError err = chooseHostForInjection(request, response);
                if (err == ERR_OK)
                {    
                    // found a host
                    if (gUserSessionManager->getSessionExists(response.getHostInfo().getSessionId()))
                    {
                        result = true;
                        break;
                    }

                    WARN_LOG("GameManagerMasterImpl::getDedicatedServerForHostInjection: Attempting remote host injection but user session (" << response.getHostInfo().getSessionId() << ") hosting game (" 
                        << response.getHostPreviousGameId() << ") did not exist.");
                }
                else
                {
                    TRACE_LOG("GameManagerMasterImpl::getDedicatedServerForHostInjection: Unable to choose host for game ("
                        << gameId << "), reason: " << ErrorHelp::getErrorName(err));
                }
            }

            if (result)
                break;
        }

        return result;
    }

    /*! ************************************************************************************************/
    /*! \brief helper - logs the number of available dedicated servers (in each data center)
    ***************************************************************************************************/
    void GameManagerMasterImpl::logAvailableDedicatedServerCounts(BlazeId joiningBlazeId, const char8_t* gameProtocolVersionString,
                                                                  DedicatedServerByPingSiteMap &availableDedicatedServerMap) const
    {
        // dump the available servers per pingSite
        eastl::string logMsg;
        logMsg.sprintf("Dedicated Servers are available for the following ping sites (with gameProtocolVersion:\"%s\" and not hosted by PlayerId: %u\n",
            gameProtocolVersionString, joiningBlazeId);

        DedicatedServerByPingSiteMap::const_iterator mapIter = availableDedicatedServerMap.begin();
        DedicatedServerByPingSiteMap::const_iterator mapEnd = availableDedicatedServerMap.end();
        while (mapIter != mapEnd)
        {
            const char8_t *currentPingSiteAlias = mapIter->first;
            GameSessionMaster *gameSessionMaster = mapIter->second;
            // skip any game we are hosting.
            if (joiningBlazeId == gameSessionMaster->getTopologyHostInfo().getPlayerId())
            {
                mapIter++;
                continue;
            }
            DedicatedServerByPingSiteMap::const_iterator upperBound = availableDedicatedServerMap.upper_bound(currentPingSiteAlias);
            uint32_t serverCount = (uint32_t) eastl::distance(mapIter, upperBound);
            logMsg.append_sprintf("    pingSite \"%s\" : %u servers available for reset\n", currentPingSiteAlias, serverCount);
            mapIter = upperBound;
        }

        TRACE_LOG(logMsg.c_str());
    }

    /*! ************************************************************************************************/
    /*! \brief RPC function to remove all group members from the specified game
    ***************************************************************************************************/
    LeaveGameByGroupMasterError::Error GameManagerMasterImpl::processLeaveGameByGroupMaster(
        const Blaze::GameManager::LeaveGameByGroupMasterRequest &request, 
        Blaze::GameManager::LeaveGameByGroupMasterResponse &response, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );

        // if it's not a valid game
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return LeaveGameByGroupMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // If cleaning up an external session failure for a join which the caller/leader was not a part of, don't permission-check caller here.
        BlazeId currentBlazeId = UserSession::getCurrentUserBlazeId();
        bool cleaningUpFailedJoinByUserList = (!request.getLeaveLeader() && (request.getPlayerRemovedReason() == PLAYER_JOIN_EXTERNAL_SESSION_FAILED));
        if (!cleaningUpFailedJoinByUserList)
        {
            if (currentBlazeId != INVALID_BLAZE_ID && removalKicksConngroup(request.getPlayerRemovedReason()) && 
                !gameSessionMaster->hasPermission(currentBlazeId, GameAction::KICK_PLAYERS))
            {
                // If owner session does not have permission to kick conn group, error out
                ERR_LOG("[GameManagerMaster] player(" <<  currentBlazeId << ") does not have privilege to kick players from game(" << gameSessionMaster->getGameId() << ") ");
                return LeaveGameByGroupMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }

        // populate our remove player request with data that doesn't change between the members
        GameAction::Actions playerLeavePermission = GameAction::INVALID_ACTION;
        RemovePlayerMasterRequest removePlayerRequest;
        removePlayerRequest.setGameId(request.getGameId());
        removePlayerRequest.setPlayerRemovedTitleContext(request.getPlayerRemovedTitleContext());
        removePlayerRequest.setTitleContextString(request.getTitleContextString());
        removePlayerRequest.setPlayerRemovedReason(request.getPlayerRemovedReason());

        if (request.getSessionIdList().size() == 1)
        {
            if (request.getPlayerRemovedReason() == GROUP_LEFT)
            {
                // connection groups will always call group leave, so we fake player leave response here.
                removePlayerRequest.setPlayerRemovedReason(PLAYER_LEFT);
            }
            else if (request.getPlayerRemovedReason() == GROUP_LEFT_MAKE_RESERVATION)
            {
                // connection groups will always call group leave, so we fake player leave response here.
                removePlayerRequest.setPlayerRemovedReason(PLAYER_LEFT_MAKE_RESERVATION);
            }
        }

        if (removePlayerRequest.getPlayerRemovedReason() == PLAYER_LEFT || removePlayerRequest.getPlayerRemovedReason() == PLAYER_LEFT_MAKE_RESERVATION || 
            removePlayerRequest.getPlayerRemovedReason() == PLAYER_LEFT_CANCELLED_MATCHMAKING || removePlayerRequest.getPlayerRemovedReason() == PLAYER_LEFT_SWITCHED_GAME_SESSION)
        {
            playerLeavePermission = GameAction::LEAVE_GAME; 
        }
        else if (removePlayerRequest.getPlayerRemovedReason() == GROUP_LEFT || removePlayerRequest.getPlayerRemovedReason() == GROUP_LEFT_MAKE_RESERVATION)
        {
            playerLeavePermission = GameAction::LEAVE_GAME_BY_GROUP;
        }

        PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(currentBlazeId);
        if (playerLeavePermission == GameAction::LEAVE_GAME && EA_UNLIKELY(playerInfo == nullptr))
        {
            // user trying to leave and is not a member of the game
            return LeaveGameByGroupMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        // If this was a normal leave (there are lots of abnormal leave conditions) then do a permission check:
        if (playerLeavePermission != GameAction::INVALID_ACTION && !gameSessionMaster->hasPermission(currentBlazeId, playerLeavePermission))
        {
            ERR_LOG("[GameManagerMaster] player(" <<  currentBlazeId << ") does not have privilege to leave " << (playerLeavePermission == GameAction::LEAVE_GAME_BY_GROUP ? "(by group)" : "") << 
                    " from game(" << gameSessionMaster->getGameId() << ") ");
            return LeaveGameByGroupMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }


        bool isBan = isRemovalReasonABan(request.getPlayerRemovedReason());

        if (isBan && !gameSessionMaster->hasPermission(currentBlazeId, GameAction::UPDATE_BANNED_PLAYERS))
        {
            ERR_LOG("[GameManagerMaster] player(" <<  currentBlazeId << ") does not have privilege to ban players game(" << gameSessionMaster->getGameId() << ") ");
            return LeaveGameByGroupMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        AccountId accountIdBeingBanned = isBan ? request.getBanAccountId() : Blaze::INVALID_ACCOUNT_ID;
        PlayerRemovedReason removeReasonForOthers = PLAYER_KICKED;
        if (isBan)
        {
            switch (request.getPlayerRemovedReason())
            {
            case PLAYER_KICKED_WITH_BAN: 
                removeReasonForOthers = PLAYER_KICKED; break;
            case PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN: 
                removeReasonForOthers = PLAYER_KICKED_CONN_UNRESPONSIVE; break;
            case PLAYER_KICKED_POOR_CONNECTION_WITH_BAN: 
                removeReasonForOthers = PLAYER_KICKED_POOR_CONNECTION; break;
            default:
                removeReasonForOthers = PLAYER_KICKED; break;
            }
        }
        
        GameId gameId = gameSessionMaster->getGameId();
        if (request.getSessionIdList().empty())
        {
            BlazeId leavingBlazeId = request.getPlayerId();
            removePlayerRequest.setPlayerId(leavingBlazeId);

            if (isBan)
            {
                removePlayerRequest.setPlayerRemovedReason(request.getPlayerRemovedReason());
                removePlayerRequest.setAccountId(request.getBanAccountId());
            }

            RemovePlayerMasterError::Error removePlayerError;
            TRACE_LOG("[GameManagerMaster] Removing PlayerId(" << leavingBlazeId << ") from GameId(" << gameId << ")");
            if (removePlayer(&removePlayerRequest, leavingBlazeId, removePlayerError, &response.getExternalSessionParams()) == REMOVE_PLAYER_GAME_DESTROYED)
            {
                ERR_LOG("[GameManagerMaster] Removing PlayerId(" << leavingBlazeId << ") triggered an unexpected game destruction for GameId(" << gameId << ")");
                return LeaveGameByGroupMasterError::ERR_OK;
            }
        }
        else
        {
            // go through the member in the list, remove each player from the game including the host (if applicable). Remove host at the end to avoid destructing the game in the middle otherwise
            // rest of the players will have wrong remove reason.
            BlazeId leavingHostBlazeId = INVALID_BLAZE_ID;
            for (UserSessionIdList::const_iterator sessionIdIter = request.getSessionIdList().begin(), sessionIdEnd = request.getSessionIdList().end(); sessionIdIter != sessionIdEnd; ++sessionIdIter)
            {
                BlazeId leavingBlazeId = gUserSessionManager->getBlazeId(*sessionIdIter);
                // ignore invalid sessions (might have logged out, etc)
                if (leavingBlazeId == INVALID_BLAZE_ID)
                {
                    continue;
                }

                if (gameSessionMaster->getTopologyHostInfo().getPlayerId() == leavingBlazeId)
                {
                    leavingHostBlazeId = leavingBlazeId;
                    continue;
                }

                TRACE_LOG("[GameManagerMaster] Removing PlayerId(" << leavingBlazeId << ") from GameId(" << gameId << ")");
                removePlayerRequest.setPlayerId(leavingBlazeId);

                if (isBan)
                {
                    AccountId acctId = gUserSessionManager->getAccountId(*sessionIdIter);
                    if (acctId == accountIdBeingBanned)
                    {
                        removePlayerRequest.setPlayerRemovedReason(request.getPlayerRemovedReason());
                        removePlayerRequest.setAccountId(request.getBanAccountId());
                    }
                    else
                    {
                        removePlayerRequest.setPlayerRemovedReason(removeReasonForOthers);
                    }
                }

                RemovePlayerMasterError::Error removePlayerError;
                if (removePlayer(&removePlayerRequest, leavingBlazeId, removePlayerError, &response.getExternalSessionParams()) == REMOVE_PLAYER_GAME_DESTROYED)
                {
                    // this shouldn't be reachable (since we've already found the game topology host and remove him last - he should be
                    //    the only player who's removal could destroy the game)
                    //   but just in case we need to guard against a crash.
                    ERR_LOG("[GameManagerMaster] Removing PlayerId(" << leavingBlazeId << ") triggered an unexpected game destruction for GameId(" << gameId << ")");
                    return LeaveGameByGroupMasterError::ERR_OK;
                }
            }

            // finally, remove the game host last (if the host is leaving)
            if (leavingHostBlazeId != INVALID_BLAZE_ID)
            {
                removePlayerRequest.setPlayerId(leavingHostBlazeId);
                RemovePlayerMasterError::Error removePlayerError;
                removePlayer(&removePlayerRequest, leavingHostBlazeId, removePlayerError, &response.getExternalSessionParams());
                // note: this player remove might've destroyed the game, but we don't use the game anymore
            }
        }
       
        return LeaveGameByGroupMasterError::ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief remove player RPC
    ***************************************************************************************************/
    RemovePlayerMasterError::Error GameManagerMasterImpl::processRemovePlayerMaster(
        const Blaze::GameManager::RemovePlayerMasterRequest &request, RemovePlayerMasterResponse& response, const Message *message)
    {
        RemovePlayerMasterError::Error err;
        removePlayer(&request, UserSession::getCurrentUserBlazeId(), err, &response.getExternalSessionParams());
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief A helper function to remove player with specific sessionId from the game

        \param[in] request         - RemovePlayerRequest
        \param[in] kickerBlazeId   -  id of player who is triggering this removePlayer function call
        \param[out] removePlayerError - error code to check
        \param[out] response       - if not nullptr, we fill out a response tdf
        \param[out] removedExternalUserInfos - if non nullptr, appends the removed user's external user infos for caller to update game's external session.
        \return the "outcome" of the player removal as far as the game is concerned; allows caller to handle
            game changes due to the player removal (game destruction, migration start)
    ***************************************************************************************************/
    GameManagerMasterImpl::RemovePlayerGameSideEffect GameManagerMasterImpl::removePlayer(const RemovePlayerMasterRequest* request, 
                                                                       BlazeId kickerBlazeId,
                                                                       RemovePlayerMasterError::Error &removePlayerError,
                                                                       LeaveGroupExternalSessionParameters *externalSessionParams /*= nullptr*/)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request->getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMaster] Game id " << request->getGameId() << " not found.");
            removePlayerError = RemovePlayerMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
            return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
        }

        // if it's a valid player
        PlayerId leavingPlayerId = request->getPlayerId();
        PlayerRemovedReason playerRemoveReason = request->getPlayerRemovedReason();
        PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(request->getPlayerId());
        if (EA_UNLIKELY(playerInfo == nullptr))
        {
            TRACE_LOG("[GameManagerMaster] Leaving player " << request->getPlayerId() << " not found in the game, this might be a bug ");
            removePlayerError = RemovePlayerMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        else
        {
            TRACE_LOG("[GameManagerMaster] Removing player(" << playerInfo->getPlayerId() << ":" << playerInfo->getPlayerName() 
                << ":conngroup=" << playerInfo->getConnectionGroupId() << ") from game(" << request->getGameId() << ") for " << PlayerRemovedReasonToString(request->getPlayerRemovedReason()));

            if ((gameSessionMaster->getGameNetworkTopology() == CLIENT_SERVER_DEDICATED)
                && ((playerRemoveReason == PLAYER_JOIN_TIMEOUT) || (playerRemoveReason == PLAYER_KICKED_CONN_UNRESPONSIVE) || (playerRemoveReason == PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN)))
            {
                addConnectionGroupJoinFailureMap(playerInfo->getConnectionGroupId(), request->getGameId());
            }

            removePlayerError = gameSessionMaster->processRemovePlayer(request, kickerBlazeId, externalSessionParams);
            if (removePlayerError != RemovePlayerMasterError::ERR_OK)
            {
                TRACE_LOG("[GameManagerMaster] Removing player(" << playerInfo->getPlayerId() << ":" << playerInfo->getPlayerName() 
                    << ") from game(" << request->getGameId() << ") failed with " << ErrorHelp::getErrorName(removePlayerError));
                return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
            }
        }

        // determine if we need to trigger host migration (or destroy the game)
        bool topologyHostLeft = (gameSessionMaster->getTopologyHostInfo().getPlayerId() == leavingPlayerId);
        bool platformHostLeft = (gameSessionMaster->getPlatformHostInfo().getPlayerId() == leavingPlayerId);
        bool gameRosterEmpty = !gameSessionMaster->getPlayerRoster()->hasRosterPlayers();

        if (topologyHostLeft || platformHostLeft ||
            (gameRosterEmpty && !isDedicatedHostedTopology(gameSessionMaster->getGameNetworkTopology()) && !gameSessionMaster->hasExternalOwner()))
        {
            // we've removed either the game's platform or topology host, so we need to trigger a host migration

            //   we always allow platformHostMigration in order to support a PC server with 360 clients.  
            //   It is possible that the platform host is currently also the topology host, in
            //   which case we trigger both.
            HostMigrationType migrationType = TOPOLOGY_HOST_MIGRATION;
            if (platformHostLeft && !topologyHostLeft) 
            {
                migrationType = PLATFORM_HOST_MIGRATION;
            }
            else if (platformHostLeft && topologyHostLeft)
            {
                migrationType = TOPOLOGY_PLATFORM_HOST_MIGRATION;
            }

            // migration is allowed if the game supports it, OR if this is only a platformHostMigration .
            bool isMigrationAllowed = ((gameSessionMaster->getGameSettings().getHostMigratable()) || (migrationType == PLATFORM_HOST_MIGRATION));
            
            if (isMigrationAllowed)
            {
                bool gameDestroyed = false;
                if (gameSessionMaster->startHostMigration(nullptr, migrationType, playerRemoveReason, gameDestroyed))
                {
                    removePlayerError = RemovePlayerMasterError::ERR_OK;
                    return REMOVE_PLAYER_MIGRATION_TRIGGERED;
                }
                else if (gameDestroyed)
                {
                    removePlayerError = RemovePlayerMasterError::ERR_OK;
                    return REMOVE_PLAYER_GAME_DESTROYED;
                }
                else if (gameSessionMaster->hasExternalOwner() && (migrationType == PLATFORM_HOST_MIGRATION))
                {
                    // empty game may persist until external owner logs out
                    TRACE_LOG("[GameSessionMaster] startHostMigration failed for game(" << gameSessionMaster->getGameId() << "). Has external owner(" << gameSessionMaster->getExternalOwnerBlazeId() << "), NOOP.");
                    return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
                }
                else
                {
                    // platform host migration fail on a virtual game... 
                    if (gameSessionMaster->getGameSettings().getVirtualized() && (migrationType == PLATFORM_HOST_MIGRATION))
                    {
                        if (gameSessionMaster->getDedicatedServerHostSessionExists())
                        {
                            // just eject host, the game is now invalid.
                            gameSessionMaster->ejectHost(gameSessionMaster->getDedicatedServerHostSessionId(), gameSessionMaster->getDedicatedServerHostBlazeId());
                            return REMOVE_PLAYER_GAME_DESTROYED;
                        }
                    }
                    if (gameSessionMaster->hasDedicatedServerHost() &&
                        (migrationType == PLATFORM_HOST_MIGRATION))
                    {
                        TRACE_LOG("[GameSessionMaster] startHostMigration failed for dedicated server game in INITIALIZING state, server will be returned to the pool.");
                        ReturnDedicatedServerToPoolMasterError::Error returnErr = gameSessionMaster->returnDedicatedServerToPool();
                        if (returnErr == ReturnDedicatedServerToPoolMasterError::ERR_OK)
                        {
                            return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
                        }
                        // If there was an error returning the game to pool we intentionally
                        // drop down from here to destroy the game.
                    }
                    TRACE_LOG("[GameManagerMaster] startHostMigration failed. game destroyed");
                    // fall though - we'll destroy the game
                }
            }

            // host migration not allowed (or it failed to start).  Transition state to post-game and nuke the game.
            // first advance to POST_GAME to allow clients to send trueSkill updates if needed.
            if ( gameSessionMaster->setGameState(POST_GAME) != ERR_OK )
            {
                TRACE_LOG("[GameManagerMaster] Unable to change game(" << gameSessionMaster->getGameId() << ") state to post game.");
            }
            
            // now destroy the game
            DestroyGameMasterError::Error err = destroyGame(gameSessionMaster.get(), HOST_LEAVING, leavingPlayerId);
            if (err == DestroyGameMasterError::ERR_OK)
            {
                removePlayerError = RemovePlayerMasterError::ERR_OK;
                return REMOVE_PLAYER_GAME_DESTROYED;
            }
            else
            {
                TRACE_LOG("[GameManagerMaster] Leaving player " << leavingPlayerId << " not found in the game, this might be a bug ");
                removePlayerError = RemovePlayerMasterError::GAMEMANAGER_ERR_FAILED_IN_GAME_DESTROY;
                return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
            }
        }

        // this was a regular player leave
        return REMOVE_PLAYER_NO_GAME_SIDE_EFFECT;
    }

    
    /*! ************************************************************************************************/
    /*! \brief ban a user from a game; user doesn't have to be a current member of the game (or even online).
            If they are a member, we kick them out.
    ***************************************************************************************************/
    BanPlayerMasterError::Error GameManagerMasterImpl::processBanPlayerMaster(const Blaze::GameManager::BanPlayerMasterRequest &request, Blaze::GameManager::BanPlayerMasterResponse &response, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return BanPlayerMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (request.getPlayerIds().size() > getBanListMax())
        {
            return BanPlayerMasterError::GAMEMANAGER_ERR_BANNED_LIST_MAX;
        }
        
        PlayerId banningPlayerId = UserSession::getCurrentUserBlazeId();
        bool hasUserSession = banningPlayerId != INVALID_BLAZE_ID;

        // banning requires admin rights on a game
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_BANNED_PLAYERS))
        {
            TRACE_LOG("[GameSessionMaster] session does not have privilege to ban the " << request.getPlayerIds().size() << " users from game " << request.getGameId() << ".");
            return BanPlayerMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        BlazeIdList::const_iterator blazeIt = request.getPlayerIds().begin();
        BlazeIdList::const_iterator blazeEnd = request.getPlayerIds().end();
        AccountIdList::const_iterator accIt = request.getAccountIds().begin();
        for (; blazeIt != blazeEnd; ++blazeIt, ++accIt)
        {
            if (hasUserSession && (*blazeIt == banningPlayerId))
            {
                TRACE_LOG("[GameSessionMaster] you cannot ban yourself from a game; trying to ban himself from game " << gameSessionMaster->getGameId());
                return BanPlayerMasterError::GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN;
            }
            
            TRACE_LOG("[GameSessionMaster] Ban requested for player (" << *blazeIt << ") from game " << request.getGameId() << ".");
             // When we ban a player, we kick the rest of the players in connection group. Cache the list prior to removing the player.
            PlayerInfoMaster *player = gameSessionMaster->getPlayerRoster()->getPlayer(*blazeIt);
            PlayerIdList playersToKick;
            if (player != nullptr && player->getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
            {
                if (gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(player->getConnectionGroupId()) != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
                {
                   PlayerIdList* original = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(player->getConnectionGroupId())->second;
                   if (original != nullptr)
                   {
                       // manually copy as the more concise version below is failing one of the tests.
                       for (PlayerIdList::const_iterator plIter = original->begin(), plEnd = original->end(); plIter != plEnd; ++plIter)
                       {
                           if (*plIter != *blazeIt)
                           {
                               playersToKick.push_back(*plIter);
                           }
                       }
                       /* The version below does not work for HostMigration_Ban_MigrationOn_CSPH. Investigate later. 
                       original->copyInto(playersToKick);
                       TRACE_LOG("[GameSessionMaster] From kick list, erase (" << *blazeIt << ") from game " << request.getGameId() << ".");
                       playersToKick.erase(eastl::find(playersToKick.begin(), playersToKick.end(),*blazeIt)); //remove the player being banned as he is dealt with explicitly.
                       for (PlayerIdList::const_iterator plIter = playersToKick.begin(), plEnd = playersToKick.end(); plIter != plEnd; ++plIter)
                       {
                           TRACE_LOG("[GameSessionMaster] As a result - kick (" << *plIter << ") from game " << request.getGameId() << ".");
                       }*/
                   }
                }
            }

            BanPlayerMasterError::Error error = gameSessionMaster->banUserFromGame(*blazeIt, *accIt, request.getPlayerRemovedTitleContext(), response);
            if (error != BanPlayerMasterError::ERR_OK)
            {
                return error;
            }
                
            for (PlayerIdList::const_iterator plIter = playersToKick.begin(), plEnd = playersToKick.end(); plIter != plEnd; ++plIter)
            {
                TRACE_LOG("[GameSessionMaster] Kick player(" << *plIter << ") from game(" << gameSessionMaster->getGameId() << ") due to "<< *blazeIt <<" being banned.");
                
                RemovePlayerMasterRequest removePlayerRequest;
                removePlayerRequest.setGameId(request.getGameId());
                removePlayerRequest.setPlayerId(*plIter);
                removePlayerRequest.setPlayerRemovedReason(PLAYER_KICKED);
                removePlayerRequest.setPlayerRemovedTitleContext(request.getPlayerRemovedTitleContext());

                RemovePlayerMasterError::Error removeError;
                gGameManagerMaster->removePlayer(&removePlayerRequest, banningPlayerId, removeError, &response.getExternalSessionParams());
                if (removeError != RemovePlayerMasterError::ERR_OK) 
                {
                    TRACE_LOG("[GameSessionMaster] Unable to remove player(" << *plIter << ") from game(" << gameSessionMaster->getGameId() << ").");
                    return BanPlayerMasterError::GAMEMANAGER_ERR_REMOVE_PLAYER_FAILED;
                }
            }
        }

        return BanPlayerMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief remove a user from a game banned list.
    ***************************************************************************************************/
    RemovePlayerFromBannedListMasterError::Error GameManagerMasterImpl::processRemovePlayerFromBannedListMaster(const Blaze::GameManager::RemovePlayerFromBannedListMasterRequest &request, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return RemovePlayerFromBannedListMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // removing a ban from banned list requires admin rights on a game
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_BANNED_PLAYERS))
        {
            TRACE_LOG("[GameSessionMaster] session does not have privilege to remove " << request.getPlayerId() << " user from the banned list of game " << request.getGameId() << ".");
            return RemovePlayerFromBannedListMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        return gameSessionMaster->removePlayerFromBannedList(request.getAccountId());
    }

    /*! ************************************************************************************************/
    /*! \brief clear a game banned list.
    ***************************************************************************************************/
    ClearBannedListMasterError::Error GameManagerMasterImpl::processClearBannedListMaster(const Blaze::GameManager::BannedListRequest &request, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return ClearBannedListMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }


        // clearing the banned list requires admin rights on a game
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_BANNED_PLAYERS))
        {
            TRACE_LOG("[GameSessionMaster] session does not have privilege to clear the banned list of game " << request.getGameId() << ".");
            return ClearBannedListMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        gameSessionMaster->clearBannedList();

        return ClearBannedListMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief get a banned list from a game.
    ***************************************************************************************************/
    GetBannedListMasterError::Error GameManagerMasterImpl::processGetBannedListMaster( const Blaze::GameManager::BannedListRequest &request, Blaze::GameManager::BannedListResponse &response, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return GetBannedListMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        gameSessionMaster->getBannedList(response.getBannedMembers());

        return GetBannedListMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief update 1 or more game attributes; requires admin rights
    ***************************************************************************************************/
    SetGameAttributesMasterError::Error GameManagerMasterImpl::processSetGameAttributesMaster(
        const SetGameAttributesRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetGameAttributesMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // Only admin can change the game attribute.
        if (checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_ATTRIBUTES)
            || gameSessionMaster->getGameSettings().getAllowMemberGameAttributeEdit())
        {
            //remove from census cache and read after the change
            removeGameFromCensusCache(*gameSessionMaster);

            uint32_t changedAttributeCount = 0;
            BlazeRpcError blazeRpcErr = gameSessionMaster->setGameAttributes(&request.getGameAttributes(), changedAttributeCount);
            if ((blazeRpcErr == ERR_OK) && (changedAttributeCount > 0))
            {
                //Send the attribute update notifications to all the sessions involved with the game
                NotifyGameAttribChange nGameAttrib;
                nGameAttrib.setGameId(gameSessionMaster->getGameId());
                nGameAttrib.getGameAttribs().insert( request.getGameAttributes().begin(), request.getGameAttributes().end() );
                GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
                sendNotifyGameAttribChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameAttrib);
            }
            
            addGameToCensusCache(*gameSessionMaster);

            return SetGameAttributesMasterError::commandErrorFromBlazeError(blazeRpcErr);
        }
        else
        {
            TRACE_LOG("[GameManagerMaster] non-admin user trying to set attributes for game(" << request.getGameId() << ")");
            
            return SetGameAttributesMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update 1 or more game attributes; requires admin rights
    ***************************************************************************************************/
    SetDedicatedServerAttributesMasterError::Error GameManagerMasterImpl::processSetDedicatedServerAttributesMaster(
        const SetDedicatedServerAttributesRequest &request, const Message *message)
    {
        // By default, only dedicated servers can make this request, and only for their current Game. 
        if (request.getGameIdList().size() > 1)
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_BATCH_UPDATE_DEDICATED_SERVER_ATTRIBUTES))
            {
                TRACE_LOG("[GameManagerMaster] non-admin user trying to set attributes for multiple games.");
                return SetDedicatedServerAttributesMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }

        for (auto& gameId : request.getGameIdList())
        {

            GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
            if (EA_UNLIKELY(gameSessionMaster == nullptr))
            {
                // We may be sent values for games that we don't own anymore.  Just ignore them. 
                continue;
            }

            // By default, only the DS itself can change its values.  If a util wants to change multiple values, it needs the correct permissions
            if (request.getGameIdList().size() > 1 || 
                (gCurrentUserSession != nullptr && gameSessionMaster->getDedicatedServerHostInfo().getUserSessionId() == gCurrentUserSession->getUserSessionId()) )
            {
                uint32_t changedAttributeCount = 0;
                BlazeRpcError blazeRpcErr = gameSessionMaster->setDedicatedServerAttributes(&request.getDedicatedServerAttributes(), changedAttributeCount);
                if ((blazeRpcErr == ERR_OK) && (changedAttributeCount > 0))
                {
                    //Send the attribute update notifications to all the sessions involved with the game
                    NotifyDedicatedServerAttribChange nGameAttrib;
                    nGameAttrib.setGameId(gameSessionMaster->getGameId());
                    nGameAttrib.getDedicatedServerAttribs().insert(request.getDedicatedServerAttributes().begin(), request.getDedicatedServerAttributes().end());
                    GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
                    sendNotifyDedicatedServerAttribChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameAttrib);
                }

                return SetDedicatedServerAttributesMasterError::commandErrorFromBlazeError(blazeRpcErr);
            }
            else
            {
                TRACE_LOG("[GameManagerMaster] non-dedicated server user trying to set attributes for game(" << gameId << ")");

                return SetDedicatedServerAttributesMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }

        return SetDedicatedServerAttributesMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief updates game mod register; requires admin rights
    ***************************************************************************************************/
    SetGameModRegisterMasterError::Error GameManagerMasterImpl::processSetGameModRegisterMaster(
        const Blaze::GameManager::SetGameModRegisterRequest &request, const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetGameModRegisterMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // Only admin can change the game attribute.
        if (checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_MOD_REGISTER))
        {
            GameModRegister oldRegister = gameSessionMaster->getGameModRegister();
            gameSessionMaster->setGameModRegister(request.getGameModRegister());
            if (oldRegister != request.getGameModRegister())
            {
                // we only update when the register has really changed
                NotifyGameModRegisterChanged notifyGameModRegisterChanged;
                notifyGameModRegisterChanged.setGameId(gameSessionMaster->getGameId());
                notifyGameModRegisterChanged.setGameModRegister(gameSessionMaster->getGameModRegister());
                GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
                sendNotifyGameModRegisterChangedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyGameModRegisterChanged);
            }
            
        }
        else
        {
            //Not an admin.
            TRACE_LOG("[GameManagerMaster] non-admin user trying to set game mod register for game(" << request.getGameId() << ")");
            
            return SetGameModRegisterMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        return SetGameModRegisterMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief updates game entry criteria; requires admin rights
    ***************************************************************************************************/
    SetGameEntryCriteriaMasterError::Error GameManagerMasterImpl::processSetGameEntryCriteriaMaster(
        const Blaze::GameManager::SetGameEntryCriteriaRequest &request, const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetGameEntryCriteriaMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // Only admin can change the game attribute.
        if (checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_ENTRY_CRITERIA))
        {
            BlazeRpcError blazeRpcError = gameSessionMaster->setGameEntryCriteria(request);

            if (blazeRpcError != ERR_OK)
            {
                return SetGameEntryCriteriaMasterError::commandErrorFromBlazeError(blazeRpcError);
            }

            //send a notification to everyone
            NotifyGameEntryCriteriaChanged notifyGameEntryCriteriaChanged;
            notifyGameEntryCriteriaChanged.setGameId(gameSessionMaster->getGameId());
            gameSessionMaster->getReplicatedGameSession().getEntryCriteriaMap().copyInto(notifyGameEntryCriteriaChanged.getEntryCriteriaMap());

            // build role criteria map
            for(RoleCriteriaMap::const_iterator roleCriteriaIter = gameSessionMaster->getRoleInformation().getRoleCriteriaMap().begin(), 
                roleCriteriaEnd = gameSessionMaster->getRoleInformation().getRoleCriteriaMap().end(); 
                roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
            {
                if(!roleCriteriaIter->second->getRoleEntryCriteriaMap().empty())
                {
                    // non-empty criteria, add to notification
                    // client knows to clear existing entry criteria for roles not specified in the notification
                    EntryCriteriaMap *roleEntryCriteria = notifyGameEntryCriteriaChanged.getRoleEntryCriteriaMap().allocate_element();

                    roleCriteriaIter->second->getRoleEntryCriteriaMap().copyInto(*roleEntryCriteria);
                    notifyGameEntryCriteriaChanged.getRoleEntryCriteriaMap()[roleCriteriaIter->first] = roleEntryCriteria;
                }
            }
            GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
            sendNotifyGameEntryCriteriaChangedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyGameEntryCriteriaChanged);


            return SetGameEntryCriteriaMasterError::ERR_OK;
        }
        else
        {
            //Not an admin.
            TRACE_LOG("[GameManagerMaster] non-admin user trying to set game entry criteria for game(" << request.getGameId() << ")");
            
            return SetGameEntryCriteriaMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief update a player's playerAttributes.  Requires admin rights to update someone else's attribs
    ***************************************************************************************************/
    SetPlayerAttributesMasterError::Error GameManagerMasterImpl::processSetPlayerAttributesMaster(
        const SetPlayerAttributesRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetPlayerAttributesMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        
        return gameSessionMaster->processSetPlayerAttributesMaster(request, UserSession::getCurrentUserBlazeId());
    }
        
    /*! ************************************************************************************************/
    /*! \brief add a player to a game's admin list - requires admin rights.
    ***************************************************************************************************/
    AddAdminPlayerMasterError::Error GameManagerMasterImpl::processAddAdminPlayerMaster(const UpdateAdminListRequest &request, const Message *message)
    {
        // GM_TODO 3.0 timeframe, look into consolidating to lists of added/removed admins.
        GameSessionMasterPtr gameSessionMaster;
        BlazeId currentBlazeId = UserSession::getCurrentUserBlazeId();
        BlazeRpcError blazeRpcError = validateAdminListUpdateRequest(request, currentBlazeId, gameSessionMaster);
        if (blazeRpcError != ERR_OK)
        {
            return AddAdminPlayerMasterError::commandErrorFromBlazeError(blazeRpcError);
        }

        if (gameSessionMaster->isAdminPlayer(request.getAdminPlayerId()))
        {
            WARN_LOG("[GameManagerMaster] Player " << request.getAdminPlayerId() << " in the request is already an admin.");
            return AddAdminPlayerMasterError::GAMEMANAGER_ERR_ALREADY_ADMIN;
        }

        gameSessionMaster->addAdminPlayer(request.getAdminPlayerId(), currentBlazeId);

        return AddAdminPlayerMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief remove a player from a game's admin list - requires admin rights.
    ***************************************************************************************************/
    RemoveAdminPlayerMasterError::Error GameManagerMasterImpl::processRemoveAdminPlayerMaster(const UpdateAdminListRequest &request, const Message *message)
    {
        // GM_TODO 3.0 timeframe, look into consolidating to lists of added/removed admins.
        GameSessionMasterPtr gameSessionMaster;
        BlazeId currentBlazeId = UserSession::getCurrentUserBlazeId();
        BlazeRpcError blazeRpcError = validateAdminListUpdateRequest(request, currentBlazeId, gameSessionMaster);
        if (blazeRpcError != ERR_OK)
        {
            return RemoveAdminPlayerMasterError::commandErrorFromBlazeError(blazeRpcError);
        }

        if (!gameSessionMaster->isAdminPlayer(request.getAdminPlayerId()))
        {
            WARN_LOG("[GameManagerMaster] Player " << request.getAdminPlayerId() << " in the request is not an admin.");
            return RemoveAdminPlayerMasterError::GAMEMANAGER_ERR_NOT_IN_ADMIN_LIST;
        }

        gameSessionMaster->removeAdminPlayer(request.getAdminPlayerId(), currentBlazeId, INVALID_CONNECTION_GROUP_ID);

        return RemoveAdminPlayerMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    
    /*! \brief validate common information for update admin list rpc; gameId, updatingSession is game admin, target is game member
        \param[in] request - the request to validate
    ***************************************************************************************************/
    BlazeRpcError GameManagerMasterImpl::validateAdminListUpdateRequest(const UpdateAdminListRequest &request, BlazeId sourceId,
                                                                        GameSessionMasterPtr &gameSessionMaster)
    {
        gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (!gameSessionMaster->hasPermission(sourceId, GameAction::UPDATE_ADMIN_LIST))
        {
            WARN_LOG("[GameManagerMaster] Only Admin can update admin list; user(" << sourceId << ") is not an admin.");
            return GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        // verify that the player having his admin rights changed is a game member
        // NOTE: this prevents the admin rights from being stripped from a dedicated server host.
        //     This only applies to the old logic where the topology host was set as the admin by default.
        if (gameSessionMaster->getGamePermissionSystem().mEnableAutomaticAdminSelection)
        {
            PlayerId updatedPlayerId = request.getAdminPlayerId();
            PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(updatedPlayerId);
            if (EA_UNLIKELY(playerInfo == nullptr))
            {
                bool isUpdatedPlayerDedicatedServerHost = ((gameSessionMaster->hasDedicatedServerHost()) &&
                                                            (gameSessionMaster->getTopologyHostInfo().getPlayerId() == updatedPlayerId));
                if (isUpdatedPlayerDedicatedServerHost)
                {
                    ERR_LOG("[GameManagerMaster] Player " << updatedPlayerId << " in the request is host of dedicated server, can't be removed.");
                    return GAMEMANAGER_ERR_DEDICATED_SERVER_HOST;
                }
                if (gameSessionMaster->hasExternalOwner() && //BlazeSDK may already disallow, but just in case
                    (gameSessionMaster->getExternalOwnerBlazeId() == updatedPlayerId))
                {
                    ERR_LOG("[GameManagerMasterImpl].validateAdminListUpdateRequest: User(" << updatedPlayerId << ") in the request is an external owner, can't be removed.");
                    return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
                }
            }
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief advance the game's GameState
    ***************************************************************************************************/
    AdvanceGameStateMasterError::Error GameManagerMasterImpl::processAdvanceGameStateMaster(const AdvanceGameStateRequest &request, 
                                                                                            const Message *message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return AdvanceGameStateMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }


        GameAction::Actions actionRequired = GameAction::INIT_GAME;
        switch (request.getNewGameState())
        {
        case IN_GAME:       actionRequired = GameAction::START_GAME;    break;
        case POST_GAME:     actionRequired = GameAction::END_GAME;      break;
        case DESTRUCTING:   actionRequired = GameAction::DESTROY_GAME;  break;
        case INITIALIZING:  actionRequired = GameAction::RESET_GAME;    break;
        case RESETABLE:     actionRequired = GameAction::RETURN_DEDICATED_SERVER_TO_POOL;  break;
        default:            break;
        }

        // check request sender's admin rights
        if (!checkPermission(*gameSessionMaster, actionRequired))
        {
            // player is not admin
            WARN_LOG("[GameManagerMaster] Only Admin player can advance the state of game.");
            return AdvanceGameStateMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }
        
        GameState oldGameState = gameSessionMaster->getGameState();
        GameState newGameState = request.getNewGameState();

        // we don't allow you to start or end migration using advanceGameState.
        if (EA_UNLIKELY(newGameState == MIGRATING) || EA_UNLIKELY(oldGameState == MIGRATING))
        {
            WARN_LOG("[GameManagerMaster] You cannot use the advanceGameState RPC to start or end host migration...");
            return AdvanceGameStateMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        TRACE_LOG("[GameManagerMaster] Advancing game(" << gameId << ") state " << GameStateToString(oldGameState) << " (" 
                  << oldGameState << ") => " << GameStateToString(newGameState) << " (" << newGameState << ")");
        if ( gameSessionMaster->setGameState(newGameState) == ERR_OK )
        {
            if (oldGameState == RESETABLE)
            {
                gameSessionMaster->getMetricsHandler().onDedicatedGameHostReset();
            }
            //Try to set the new game state but don't allow explicit transition to MIGRATING state
            if (newGameState == IN_GAME)
            {
                gameSessionMaster->gameStarted();
            }
            if (newGameState == POST_GAME)
            {
                gameSessionMaster->gameFinished();
            }
        }
        else
        {
            return AdvanceGameStateMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        return AdvanceGameStateMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief update the game's xnet nonce & session
    ***************************************************************************************************/
    FinalizeGameCreationMasterError::Error GameManagerMasterImpl::processFinalizeGameCreationMaster(
        const UpdateGameSessionRequest &request, const Message *message)
    {

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return FinalizeGameCreationMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }


        return gameSessionMaster->finalizeGameCreation(request, UserSession::getCurrentUserBlazeId());
    }

    /*! ************************************************************************************************/
    /*! \brief update the game's xnet nonce & xnet session
    ***************************************************************************************************/
    UpdateGameSessionMasterError::Error GameManagerMasterImpl::processUpdateGameSessionMaster(
        const UpdateGameSessionRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return UpdateGameSessionMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        return gameSessionMaster->updateGameSession(request, UserSession::getCurrentUserBlazeId());
    }

    /*! ************************************************************************************************/
    /*! \brief update the status of a game host migration
    ***************************************************************************************************/
    UpdateGameHostMigrationStatusMasterError::Error GameManagerMasterImpl::processUpdateGameHostMigrationStatusMaster(
        const UpdateGameHostMigrationStatusRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return UpdateGameHostMigrationStatusMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        return gameSessionMaster->updateGameHostMigrationStatus(request, UserSession::getCurrentUserBlazeId());
    }

    /*! ************************************************************************************************/
    /*! \brief join the specified game
    ***************************************************************************************************/
    JoinGameMasterError::Error GameManagerMasterImpl::processJoinGameMaster(
        const JoinGameMasterRequest& request, JoinGameMasterResponse& response, Blaze::EntryCriteriaError &error, const Message *message)
    {
        RelatedPlayerSetsMap joiningPlayersLists;
        UserJoinInfoList userJoinInfoList;
        UserJoinInfoPtr userJoinInfo = userJoinInfoList.pull_back();
        userJoinInfo->setUserGroupId(request.getJoinRequest().getPlayerJoinData().getGroupId());
        request.getUserJoinInfo().getUser().copyInto(userJoinInfo->getUser());

        if (request.getJoinRequest().getUser().getBlazeId() != INVALID_BLAZE_ID)
        {
            // tried to join a specific player, add it to the list
            joiningPlayersLists[request.getUserJoinInfo().getUser().getSessionId()].insert(request.getJoinRequest().getUser().getBlazeId());
        }

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getJoinRequest().getGameId());
        // looks up any group members in the game
        buildRelatedPlayersMap(gameSessionMaster.get(), userJoinInfoList, joiningPlayersLists);

        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, GAMEMANAGER_ERR_INVALID_GAME_ID);
            return JoinGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        if (gameSessionMaster->getGameType() == GAME_TYPE_GAMESESSION)
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_SESSION))
            {
                buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, ERR_AUTHORIZATION_REQUIRED);
                return JoinGameMasterError::ERR_AUTHORIZATION_REQUIRED;
            }
        }
        else if (gameSessionMaster->getGameType() == GAME_TYPE_GROUP)
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_GROUP))
            {
                buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, ERR_AUTHORIZATION_REQUIRED);
                return JoinGameMasterError::ERR_AUTHORIZATION_REQUIRED;
            }
        }

        response.getJoinResponse().setGameId(gameSessionMaster->getGameId());

        GameSetupReason playerJoinContext;
        playerJoinContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        if (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_CLAIM_RESERVATION)
        {
            playerJoinContext.getDatalessSetupContext()->setSetupContext(INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT);
        }
        else
        {
            playerJoinContext.getDatalessSetupContext()->setSetupContext(JOIN_GAME_SETUP_CONTEXT);
        }

        JoinGameMasterError::Error err;
       
        err = gameSessionMaster->joinGame(request, response, error, playerJoinContext);
        if (err == JoinGameMasterError::Error::ERR_OK)
        {
            UserSessionSetupReasonMap setupReasons;
            setupReasons[request.getUserJoinInfo().getUser().getSessionId()] = &playerJoinContext;

            buildAndSendMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists);
        }
        else
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, err);
        }
        return err;

    }

    /*! ************************************************************************************************/
    /*! \brief join a game and bring a game group along for the ride
    ***************************************************************************************************/
    JoinGameByGroupMasterError::Error GameManagerMasterImpl::processJoinGameByGroupMaster( const JoinGameByGroupMasterRequest& request,
        JoinGameMasterResponse& response, EntryCriteriaError& entryCriteriaError, const Message *message)
    {
        RelatedPlayerSetsMap joiningPlayersLists;

        if (request.getJoinRequest().getUser().getBlazeId() != INVALID_BLAZE_ID)
        {
            // tried to join a specific player, add it to the list for everyone
            for (UserJoinInfoList::const_iterator iter = request.getUsersInfo().begin(), end = request.getUsersInfo().end(); iter != end; ++iter)
            {
                UserSessionId currentSessionId = (*iter)->getUser().getSessionId();
                joiningPlayersLists[currentSessionId].insert(request.getJoinRequest().getUser().getBlazeId());
            }
            
        }

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getJoinRequest().getGameId());

        buildRelatedPlayersMap(gameSessionMaster.get(), request.getUsersInfo(), joiningPlayersLists);

        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, GAMEMANAGER_ERR_INVALID_GAME_ID);
            return JoinGameByGroupMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        Authorization::Permission joinPermission = (gameSessionMaster->getGameType() == GAME_TYPE_GAMESESSION) ? Authorization::PERMISSION_JOIN_GAME_SESSION : Authorization::PERMISSION_JOIN_GAME_GROUP;
       
        // Check for JoinLeader == false (JoinByUserList), and use a different permission check for that type of join:
        if (!request.getJoinLeader())
        {
            bool joinByUserListUserTypePermission = UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_BY_USERLIST, true);
            bool reservePlayersGamePermission = (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION  && checkPermission(*gameSessionMaster, GameAction::RESERVE_PLAYERS_BY_USERLIST));
            if (!joinByUserListUserTypePermission && !reservePlayersGamePermission)
            {
                UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_BY_USERLIST);
                if (!reservePlayersGamePermission)
                {
                    WARN_LOG("[GameSessionMaster].processJoinGameByGroupMaster: Player calling joinGameByUserList for game Id(" << gameSessionMaster->getGameId() << ") does not have RESERVE_PLAYERS_BY_USERLIST Game Permission, or is not attempting to make reservations.");
                }

                buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, ERR_AUTHORIZATION_REQUIRED);
                return JoinGameByGroupMasterError::ERR_AUTHORIZATION_REQUIRED;
            }
        }
        else if (!UserSession::isCurrentContextAuthorized(joinPermission))  // Normal join permission check:
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, ERR_AUTHORIZATION_REQUIRED);
            return JoinGameByGroupMasterError::ERR_AUTHORIZATION_REQUIRED;
        }

        response.getJoinResponse().setGameId(gameSessionMaster->getGameId());

        // If locked as busy, we may allow caller to suspend the join attempt till later.
        // Pre: this check is done before any checks against significant things that can change while locked.
        if (gameSessionMaster->getGameSettings().getLockedAsBusy())
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, GAMEMANAGER_ERR_GAME_BUSY);
            return JoinGameByGroupMasterError::GAMEMANAGER_ERR_GAME_BUSY;
        }

        // the creators reason to send back to the client as to why they joined the game.
        GameSetupReason creatorJoinContext;
        creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        if (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_CLAIM_RESERVATION)
        {
            creatorJoinContext.getDatalessSetupContext()->setSetupContext(INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT);
        }
        else
        {
            creatorJoinContext.getDatalessSetupContext()->setSetupContext(JOIN_GAME_SETUP_CONTEXT);
        }

        // GG member reasons to send back to the client for as to why they joined the game.
        GameSetupReason ggContext;
        if (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_CLAIM_RESERVATION)
        {
            ggContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
            ggContext.getDatalessSetupContext()->setSetupContext(INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT);
        }
        else
        {
            ggContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
            ggContext.getIndirectJoinGameSetupContext()->setUserGroupId(request.getJoinRequest().getPlayerJoinData().getGroupId());
            ggContext.getIndirectJoinGameSetupContext()->setRequiresClientVersionCheck(getConfig().getGameSession().getEvaluateGameProtocolVersionString());
        }

        // Reserved-list member reasons to send back to the client for as to why they joined the game.
        GameSetupReason reserveListContext;
        reserveListContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
        reserveListContext.getIndirectJoinGameSetupContext()->setRequiresClientVersionCheck(getConfig().getGameSession().getEvaluateGameProtocolVersionString());

        // Creators join game reason.
        UserSessionSetupReasonMap setupReasons;
        fillUserSessionSetupReasonMap(setupReasons, request.getUsersInfo(), &creatorJoinContext, &ggContext, &reserveListContext);

        JoinGameByGroupMasterError::Error err;
        if (request.getJoinLeader())
        {
            err = gameSessionMaster->joinGameByGroup(request, response, entryCriteriaError, setupReasons);
        }
        else
        {
            // If I'm in game, with a preferred join opt out map entry, remove the entry
            gameSessionMaster->eraseFromPendingPreferredJoinSet(UserSession::getCurrentUserBlazeId());

            err = gameSessionMaster->joinGameByGroupMembersOnly(request, response, entryCriteriaError, setupReasons);
        }
        
        if (err == JoinGameByGroupMasterError::Error::ERR_OK)
        {
            buildAndSendMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists);
        }
        else
        {
            buildAndSendMatchJoinFailedEvent(gameSessionMaster, request, joiningPlayersLists, err);
        }
        return err;
    }   

    /*! ************************************************************************************************/
    /*! \brief update the blazeServer with the status of two endpoints that connected.
    ***************************************************************************************************/
    MeshEndpointsConnectedMasterError::Error GameManagerMasterImpl::processMeshEndpointsConnectedMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return MeshEndpointsConnectedMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        
        if (request.getPlayerNetConnectionStatus() != CONNECTED)
        {
            WARN_LOG("[GameSessionMaster].processMeshEndpointsConnectedMaster called with wrong player connection status(" << PlayerNetConnectionStatusToString(request.getPlayerNetConnectionStatus()) << ")"
                << " sourceConnGrpId(" << request.getSourceConnectionGroupId() <<") targetConnGrpId(" << request.getTargetConnectionGroupId() <<") Game Id("<< gameId <<")");
            return MeshEndpointsConnectedMasterError::ERR_SYSTEM;
        }

        return gameSessionMaster->meshEndpointsConnected(request);
    }

    /*! ************************************************************************************************/
    /*! \brief update the blazeServer with the status of two endpoints that disconnected.
    ***************************************************************************************************/
    MeshEndpointsDisconnectedMasterError::Error GameManagerMasterImpl::processMeshEndpointsDisconnectedMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (gameSessionMaster == nullptr)
        {
            TRACE_LOG("[GameSessionMaster].processMeshEndpointsDisconnectedMaster called but Game ("<< gameId <<") is already destroyed. No-Op. sourceConnGrpId(" 
                << request.getSourceConnectionGroupId() <<") targetConnGrpId(" << request.getTargetConnectionGroupId() <<")");
            return MeshEndpointsDisconnectedMasterError::ERR_OK;
        }

        if ((request.getPlayerNetConnectionStatus() != DISCONNECTED && request.getPlayerNetConnectionStatus() != DISCONNECTED_PLAYER_REMOVED))
        {
            WARN_LOG("[GameSessionMaster].processMeshEndpointsDisconnectedMaster called with wrong player connection status(" << PlayerNetConnectionStatusToString(request.getPlayerNetConnectionStatus()) << ")"
                << " sourceConnGrpId(" << request.getSourceConnectionGroupId() <<") targetConnGrpId(" << request.getTargetConnectionGroupId() <<") Game Id("<< gameId <<")");
            return MeshEndpointsDisconnectedMasterError::ERR_SYSTEM;
        }

        if (gameSessionMaster->getGameNetworkTopology() == CLIENT_SERVER_DEDICATED)
        {
            updatePktReceivedMetrics(gameSessionMaster, request);
        }

        if (request.getPlayerNetConnectionStatus() == DISCONNECTED_PLAYER_REMOVED)
        {
            return MeshEndpointsDisconnectedMasterError::ERR_OK;
        }

        return gameSessionMaster->meshEndpointsDisconnected(request);
    }

    /*! ************************************************************************************************/
    /*! \brief update the blazeServer with the status of two endpoints that lost connection with each other.
    ***************************************************************************************************/
    MeshEndpointsConnectionLostMasterError::Error GameManagerMasterImpl::processMeshEndpointsConnectionLostMaster(const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request, const Message* message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (gameSessionMaster == nullptr)
        {
            TRACE_LOG("[GameSessionMaster].processMeshEndpointsConnectionLostMaster called but Game ("<< gameId <<") is already destroyed. No-Op. sourceConnGrpId(" 
                << request.getSourceConnectionGroupId() <<") targetConnGrpId(" << request.getTargetConnectionGroupId() <<")");
            return MeshEndpointsConnectionLostMasterError::ERR_OK;
        }

        if (request.getPlayerNetConnectionStatus() != DISCONNECTED)
        {
            WARN_LOG("[GameSessionMaster].processMeshEndpointsConnectionLostMaster called with wrong player connection status(" << PlayerNetConnectionStatusToString(request.getPlayerNetConnectionStatus()) << ")"
                << " sourceConnGrpId(" << request.getSourceConnectionGroupId() <<") targetConnGrpId(" << request.getTargetConnectionGroupId() <<") Game Id("<< gameId <<")");
            return MeshEndpointsConnectionLostMasterError::ERR_SYSTEM;
        }
                
        return gameSessionMaster->meshEndpointsConnectionLost(request);
    }


    /*! ************************************************************************************************/
    /*! \brief update the game's GameSettings bitfield (requires admin rights)
    ***************************************************************************************************/
    SetGameSettingsMasterError::Error GameManagerMasterImpl::processSetGameSettingsMaster(
        const SetGameSettingsRequest &request, const Message *message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetGameSettingsMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // check request sender's admin rights
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_SETTINGS))
        {
            // player is not admin
            return SetGameSettingsMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        GameSettings settings = request.getGameSettings();
        if (request.getGameSettingsMask().getBits() != 0)
        {
            // If a mask is used, use it to indicate what values should be taken from the request, and what should remain the same. 
            settings.setBits((settings.getBits() & request.getGameSettingsMask().getBits()) |
                             (gameSessionMaster->getGameSettings().getBits() & ~request.getGameSettingsMask().getBits()));
        }

        return gameSessionMaster->setGameSettings(&settings);
    }

    /*! ************************************************************************************************/
    /*! \brief resize a game's player capacity (requires admin rights)
    ***************************************************************************************************/
    SetPlayerCapacityMasterError::Error GameManagerMasterImpl::processSetPlayerCapacityMaster(
        const SetPlayerCapacityRequest &request, Blaze::GameManager::SwapPlayersErrorInfo &error, const Message *message)
    {
        SetPlayerCapacityMasterError::Error err = SetPlayerCapacityMasterError::ERR_OK;
        
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Could not locate game with id '%" PRIu64 "'.", request.getGameId());
            error.setErrMessage(msg);
            return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_PLAYER_CAPACITY))
        {
            TRACE_LOG("[GameSessionMaster] Only Admin player can set player capacity for the game, session is not an admin.");
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "You do not have permission to change the capacity of game '%" PRIu64 "'.", request.getGameId());
            error.setErrMessage(msg);
            return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        // Check for capacity changes, and if nothing will change, skip the RETE update:
        bool newCapacity = false;
        bool criteriaChanged = false;
        checkCapacityCriteriaChanges(request, *gameSessionMaster, newCapacity, criteriaChanged);

        TeamDetailsList newTeamRosters;
        newTeamRosters.reserve(request.getTeamDetailsList().size());
        bool wasOriginallyFull = gameSessionMaster->isGameAndQueueFull();
        uint16_t oldTotalPlayerCapacity = gameSessionMaster->getTotalPlayerCapacity();
        err = gameSessionMaster->processSetPlayerCapacityMaster(request, newTeamRosters, error);
        if (err == SetPlayerCapacityMasterError::ERR_OK)
        {
            if (newCapacity)
            {
                NotifyGameCapacityChange nCapacity;

                // start/restart timer for lock for preferred joins if game was made no-longer full. Or clear lock if full.
                if (getLockGamesForPreferredJoins() && (oldTotalPlayerCapacity != gameSessionMaster->getTotalPlayerCapacity()))
                {
                    if (gameSessionMaster->isGameAndQueueFull())
                        gameSessionMaster->clearLockForPreferredJoins();
                    else if (wasOriginallyFull)
                        nCapacity.setLockedForPreferredJoins(gameSessionMaster->lockForPreferredJoins());
                }

                //send all the players a notification
                nCapacity.setGameId(gameSessionMaster->getGameId());
                gameSessionMaster->getSlotCapacities().copyInto(nCapacity.getSlotCapacities());
                nCapacity.getTeamRosters().reserve(newTeamRosters.size());
                newTeamRosters.copyInto(nCapacity.getTeamRosters());
                gameSessionMaster->getRoleInformation().copyInto(nCapacity.getRoleInformation()); 
                GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
                sendNotifyGameCapacityChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nCapacity );

                // this is explicitly called after the replication of the new game size to ensure player-count related rules have accurate
                // data regarding game capacity before receiving queue promotion notifications
                gameSessionMaster->processQueueAfterSetPlayerCapacity();
            }
        }

        return err;
    }


    void GameManagerMasterImpl::checkCapacityCriteriaChanges(const SetPlayerCapacityRequest &request, GameSessionMaster& gameSessionMaster, bool& newCapacity, bool& criteriaChanged)
    {
        newCapacity = false;
        criteriaChanged = false;
        
        // Slot Changes
        if (request.getSlotCapacities().size() == MAX_SLOT_TYPE)
        {
            for (uint16_t curSlot = 0; curSlot < MAX_SLOT_TYPE; ++curSlot)
            {
                if (gameSessionMaster.getSlotCapacities()[curSlot] != request.getSlotCapacities()[curSlot])
                {
                    newCapacity = true;
                    break;
                }
            }
        }

        // Team Changes
        if (request.getTeamDetailsList().size() == 0)
        {
            // No team changes provided, no changes intended.
        }
        else if (request.getTeamDetailsList().size() != gameSessionMaster.getTeamCount())
        {
            newCapacity = true;
        }
        else
        {
            TeamDetailsList::const_iterator newTeamIter = request.getTeamDetailsList().begin();
            TeamDetailsList::const_iterator newTeamEnd  = request.getTeamDetailsList().end();
            TeamIdVector::const_iterator oldTeamIter = gameSessionMaster.getTeamIds().begin();
            TeamIdVector::const_iterator oldTeamEnd  = gameSessionMaster.getTeamIds().end();
            for(; (oldTeamIter != oldTeamEnd) && (newTeamIter != newTeamEnd); ++oldTeamIter, ++newTeamIter)
            {
                if (*oldTeamIter != (*newTeamIter)->getTeamId())
                {
                    newCapacity = true;
                    break;
                }
            }
        }


        // Role Changes
        const RoleInformation& oldRoleInformation = gameSessionMaster.getRoleInformation();
        const RoleInformation& newRoleInformation = request.getRoleInformation();

        if (oldRoleInformation.getRoleCriteriaMap().size() != newRoleInformation.getRoleCriteriaMap().size())
        {
            newCapacity = true;
        }
        else
        {
            RoleCriteriaMap::const_iterator oldRoleIter = oldRoleInformation.getRoleCriteriaMap().begin();
            RoleCriteriaMap::const_iterator oldRoleEnd  = oldRoleInformation.getRoleCriteriaMap().end(); 
            RoleCriteriaMap::const_iterator newRoleIter = newRoleInformation.getRoleCriteriaMap().begin(); 
            RoleCriteriaMap::const_iterator newRoleEnd  = newRoleInformation.getRoleCriteriaMap().end(); 
            for(; (oldRoleIter != oldRoleEnd) && (newRoleIter != newRoleEnd); ++oldRoleIter, ++newRoleIter)
            {
                if (blaze_stricmp(oldRoleIter->first.c_str(), newRoleIter->first.c_str()) != 0 ||
                    oldRoleIter->second->getRoleCapacity() != newRoleIter->second->getRoleCapacity())
                {
                    newCapacity = true;
                    break;
                }

                EntryCriteriaMap::const_iterator oldRoleCriteriaIter2 = oldRoleIter->second->getRoleEntryCriteriaMap().begin();
                EntryCriteriaMap::const_iterator oldRoleCriteriaEnd2  = oldRoleIter->second->getRoleEntryCriteriaMap().end(); 
                EntryCriteriaMap::const_iterator newRoleCriteriaIter2 = newRoleIter->second->getRoleEntryCriteriaMap().begin(); 
                EntryCriteriaMap::const_iterator newRoleCriteriaEnd2  = newRoleIter->second->getRoleEntryCriteriaMap().end(); 
                for(; (oldRoleCriteriaIter2 != oldRoleCriteriaEnd2) && (newRoleCriteriaIter2 != newRoleCriteriaEnd2); ++oldRoleCriteriaIter2, ++newRoleCriteriaIter2)
                {
                    if (blaze_stricmp(oldRoleCriteriaIter2->first.c_str(), newRoleCriteriaIter2->first.c_str()) != 0 || 
                        blaze_stricmp(oldRoleCriteriaIter2->second.c_str(), newRoleCriteriaIter2->second.c_str()) != 0)
                    {
                        criteriaChanged = true;
                        break;
                    }
                }
            }
        }  


        if (oldRoleInformation.getMultiRoleCriteria().size() != newRoleInformation.getMultiRoleCriteria().size())
        {
            criteriaChanged = true;
        }
        else
        {
            EntryCriteriaMap::const_iterator oldMultiRoleCriteriaIter = oldRoleInformation.getMultiRoleCriteria().begin();
            EntryCriteriaMap::const_iterator oldMultiRoleCriteriaEnd  = oldRoleInformation.getMultiRoleCriteria().end(); 
            EntryCriteriaMap::const_iterator newMultiRoleCriteriaIter = newRoleInformation.getMultiRoleCriteria().begin(); 
            EntryCriteriaMap::const_iterator newMultiRoleCriteriaEnd  = newRoleInformation.getMultiRoleCriteria().end(); 
            for(; (oldMultiRoleCriteriaIter != oldMultiRoleCriteriaEnd) && (newMultiRoleCriteriaIter != newMultiRoleCriteriaEnd); ++oldMultiRoleCriteriaIter, ++newMultiRoleCriteriaIter)
            {
                if (blaze_stricmp(oldMultiRoleCriteriaIter->first.c_str(), newMultiRoleCriteriaIter->first.c_str()) != 0 || 
                    blaze_stricmp(oldMultiRoleCriteriaIter->second.c_str(), newMultiRoleCriteriaIter->second.c_str()) != 0)
                {
                    criteriaChanged = true;
                    break;
                }
            }
        }        
    }


    /*! ************************************************************************************************/
    /*! \brief set a player's customData blob
    ***************************************************************************************************/
    SetPlayerCustomDataMasterError::Error GameManagerMasterImpl::processSetPlayerCustomDataMaster(
        const SetPlayerCustomDataRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetPlayerCustomDataMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
    
        return gameSessionMaster->processSetPlayerCustomDataMaster(request, UserSession::getCurrentUserBlazeId());
    }
        
    /*! ************************************************************************************************/
    /*! \brief replay the game (change the state back to pre_game from post game)
    ***************************************************************************************************/
    ReplayGameMasterError::Error GameManagerMasterImpl::processReplayGameMaster(
        const Blaze::GameManager::ReplayGameRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return ReplayGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        ReplayGameMasterError::Error err = gameSessionMaster->processReplayGameMaster(request, UserSession::getCurrentUserBlazeId());
        if ((err == ReplayGameMasterError::ERR_OK) && getConfig().getGameSession().getSyncExternalSessionsAtReplay())
        {
            TRACE_LOG("[GameManagerMasterImpl].processReplayGameMaster: checking if any external session requires resync for game(" << request.getGameId() << ")");
            Blaze::GameManager::ResyncExternalSessionMembersRequest resyncReq;
            resyncReq.setGameId(request.getGameId());
            processResyncExternalSessionMembers(resyncReq, nullptr);
        }
        return err;
    }


    /*! ************************************************************************************************/
    /*! \brief return a dedicated server to the pool of available servers (change its state back to RESETABLE and
            reset its GameProtocolVersionString (if needed).
    ***************************************************************************************************/
    ReturnDedicatedServerToPoolMasterError::Error GameManagerMasterImpl::processReturnDedicatedServerToPoolMaster(
        const ReturnDedicatedServerToPoolRequest &request, const Message *message)
    {
        
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // only admin player can change the state
        if (!gameSessionMaster->hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::RETURN_DEDICATED_SERVER_TO_POOL))
        {
            TRACE_LOG("[GameManagerMaster] Not admin player.  Can't return dedicated server to pool");
            return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        // This if mimics the if on the SDK.
        if ((gameSessionMaster->getGameState() != PRE_GAME) && (gameSessionMaster->getGameState() != INITIALIZING) && 
            (gameSessionMaster->getGameState() != POST_GAME) && (gameSessionMaster->getGameState() != UNRESPONSIVE))
        {
            TRACE_LOG("[GameManagerMaster] Unable to reset dedicated server " << request.getGameId() << " in the " <<
                GameStateToString(gameSessionMaster->getGameState()) << " state.");
            return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        // must be a dedicated server game
        if (!gameSessionMaster->hasDedicatedServerHost() || gameSessionMaster->getGameSettings().getVirtualized())
        {
            TRACE_LOG("[GameManagerMaster] You cannot return a non-dedicated server game[" << gameSessionMaster->getGameId() << "] to the available dedicated server pool.");
            return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION;
        }

       
        return gameSessionMaster->returnDedicatedServerToPool();

    }


    /*! ************************************************************************************************/
    /*! \brief client is triggering host migration (possibly suggesting a specific playerId to be the new host)
    ***************************************************************************************************/
    MigrateGameMasterError::Error GameManagerMasterImpl::processMigrateGameMaster(const MigrateHostRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(!gameSessionMaster))
        {
            return MigrateGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        
        // is migration supported by the game?
        if (!gameSessionMaster->getGameSettings().getHostMigratable() || gameSessionMaster->hasDedicatedServerHost())
        {
            TRACE_LOG("[GameManagerMaster] Host migration is not supported for game(" << gameSessionMaster->getGameId() << ").");
            return MigrateGameMasterError::GAMEMANAGER_ERR_MIGRATION_NOT_SUPPORTED;
        }

        // is caller a game admin
        if (!gameSessionMaster->hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::MIGRATE_HOST))
        {
            TRACE_LOG("[GameManagerMaster] Non admin player can't initiate host migration for game(" << gameSessionMaster->getGameId() << ").");
            return MigrateGameMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        // validate the suggested new host (if one is supplied)
        PlayerInfoMaster* suggestedHostPlayer = nullptr;
        if (request.getNewHostPlayer() != INVALID_BLAZE_ID)
        {
            suggestedHostPlayer = gameSessionMaster->getPlayerRoster()->getPlayer(request.getNewHostPlayer());
            if (suggestedHostPlayer == nullptr)
            {
                return MigrateGameMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
            }

            if (suggestedHostPlayer->getPlayerState() != ACTIVE_CONNECTED)
            {
                return MigrateGameMasterError::GAMEMANAGER_ERR_INVALID_NEWHOST;
            }

            if (!suggestedHostPlayer->hasHostPermission())
            {
                return MigrateGameMasterError::GAMEMANAGER_ERR_INVALID_NEWHOST;
            }
        }

        bool gameDestroyed = false;
        // if not a dedicated server, topology and platform host are the same, so we migrate both.
        if (!gameSessionMaster->startHostMigration(suggestedHostPlayer, TOPOLOGY_PLATFORM_HOST_MIGRATION, SYS_PLAYER_REMOVE_REASON_INVALID, gameDestroyed))
        {
            return MigrateGameMasterError::ERR_SYSTEM;
        }

        return MigrateGameMasterError::ERR_OK;
    }


    RequestPlatformHostMasterError::Error GameManagerMasterImpl::processRequestPlatformHostMaster(const MigrateHostRequest &request, const Message *message)
    {
        // This method was only for backward compatible clients, when server driven external sessions were disabled
        ERR_LOG("[GameManagerMaster].processRequestPlatformHostMaster: Player(" << request.getNewHostPlayer() << ") requested platform host for game(" << request.getGameId() << "). This flow is deprecated/unused when Blaze Server driven external sessions is enabled. No op.");
        return RequestPlatformHostMasterError::ERR_SYSTEM;
    }

    /*! ************************************************************************************************/
    /*! \brief Allows admin user to swap team for any players in the game
    ***************************************************************************************************/
    SwapPlayersMasterError::Error GameManagerMasterImpl::processSwapPlayersMaster(const Blaze::GameManager::SwapPlayersRequest &request, 
        Blaze::GameManager::SwapPlayersErrorInfo &error, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Could not locate game with id '%" PRIu64 "'.", request.getGameId());
            error.setErrMessage(msg);
            return SwapPlayersMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        return gameSessionMaster->processSwapPlayersMaster(request, UserSession::getCurrentUserBlazeId(), error);
    }

    /*! ************************************************************************************************/
    /*! \brief Allows admin user to change a team in the game
    ***************************************************************************************************/
    ChangeGameTeamIdMasterError::Error GameManagerMasterImpl::processChangeGameTeamIdMaster(const Blaze::GameManager::ChangeTeamIdRequest &request, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return ChangeGameTeamIdMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        return gameSessionMaster->processChangeGameTeamIdMaster(request, UserSession::getCurrentUserBlazeId());
    }

    /*! ************************************************************************************************/
    /*! \brief Allows for a admin user to migrate their admin rights to another player 
        (removing rights from themselves)
    ***************************************************************************************************/
    MigrateAdminPlayerMasterError::Error GameManagerMasterImpl::processMigrateAdminPlayerMaster(const Blaze::GameManager::UpdateAdminListRequest &request, const Message* message)
    {
        // GM_TODO 3.0 timeframe, look into consolidating to lists of added/removed admins.
        GameSessionMasterPtr gameSessionMaster;
        BlazeRpcError blazeRpcError = validateAdminListUpdateRequest(request, UserSession::getCurrentUserBlazeId(), gameSessionMaster);
        if (blazeRpcError != ERR_OK)
        {
            return MigrateAdminPlayerMasterError::commandErrorFromBlazeError(blazeRpcError);
        }

        if (gameSessionMaster->isAdminPlayer(request.getAdminPlayerId()))
        {
            TRACE_LOG("[GameManagerMaster] Player " << request.getAdminPlayerId() << " in the request is already an admin.");
            return MigrateAdminPlayerMasterError::GAMEMANAGER_ERR_ALREADY_ADMIN;
        }

        if ((gameSessionMaster->getDedicatedServerHostBlazeId() == UserSession::getCurrentUserBlazeId()))
        {
            ERR_LOG("[GameManagerMaster] Requesting Player(" << UserSession::getCurrentUserBlazeId() << ") in the request is host of dedicated server, can't strip his admin rights.");
            return MigrateAdminPlayerMasterError::GAMEMANAGER_ERR_DEDICATED_SERVER_HOST;
        }

        gameSessionMaster->migrateAdminPlayer(request.getAdminPlayerId(), UserSession::getCurrentUserBlazeId());

        return MigrateAdminPlayerMasterError::ERR_OK;
    }

    GetExternalDataSourceApiListByGameIdError::Error GameManagerMasterImpl::processGetExternalDataSourceApiListByGameId(
        const GetExternalDataSourceApiListRequest& request, GetExternalDataSourceApiListResponse &response, const Message* message)
    {
        const GameSessionMaster* gameSessionMaster = getReadOnlyGameSession(request.getGameId());
        if (gameSessionMaster == nullptr)
        {
            return GetExternalDataSourceApiListByGameIdError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        response.setDataSourceNameList(gameSessionMaster->getDataSourceNameList());

        return GetExternalDataSourceApiListByGameIdError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief update the game's name
    ***************************************************************************************************/
    UpdateGameNameMasterError::Error GameManagerMasterImpl::processUpdateGameNameMaster(
        const Blaze::GameManager::UpdateGameNameRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return UpdateGameNameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // rename the game name requires administrator rights on a game
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_NAME))
        {
            TRACE_LOG("[GameSessionMaster] session does not have privilege to rename the name of game " << request.getGameId() << ".");
            return UpdateGameNameMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        gameSessionMaster->updateGameName(request.getGameName());

        NotifyGameNameChange nGameName;
        nGameName.setGameId(gameSessionMaster->getGameId());
        nGameName.setGameName(request.getGameName());
        GameSessionMaster::SessionIdIteratorPair itPair = gameSessionMaster->getSessionIdIteratorPair();
        sendNotifyGameNameChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameName);

        return UpdateGameNameMasterError::ERR_OK;
    }

    SetPresenceModeMasterError::Error GameManagerMasterImpl::processSetPresenceModeMaster(const Blaze::GameManager::SetPresenceModeRequest &request,const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetPresenceModeMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // Presence mode changes requires administrator rights on a game
        if (!checkPermission(*gameSessionMaster, GameAction::UPDATE_GAME_PRESENCE_MODE))
        {
            TRACE_LOG("[GameSessionMaster] session does not have privilege to change the presence mode of game " << request.getGameId() << ".");
            return SetPresenceModeMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        if (request.getPresenceMode() == gameSessionMaster->getPresenceMode())
        {
            return SetPresenceModeMasterError::ERR_OK;
        }

        WARN_LOG("[GameSessionMaster] attempting to update game presence mode post-creation is not allowed on the current platform. Not updating presence mode for game(" << gameSessionMaster->getGameId() << ").");
        return SetPresenceModeMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION;
    }

    GetGameInfoSnapshotError::Error GameManagerMasterImpl::processGetGameInfoSnapshot(const Blaze::GameManager::GetGameInfoSnapshotRequest &request, Blaze::GameManager::GameInfo &response, const Message* message)
    {
        // lookup game based on game reporting id and construct game info from gamesessionmaster
        const GameSessionMaster* gameSessionMaster = getReadOnlyGameSession(request.getGameId());
        if (gameSessionMaster == nullptr)
        {
            return GetGameInfoSnapshotError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        gameSessionMaster->fillGameInfo(response);

        return GetGameInfoSnapshotError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Framework's entry point to collect healthcheck status for the GameMangerMaster component
    ***************************************************************************************************/
    void GameManagerMasterImpl::getStatusInfo(ComponentStatus& status) const
    {
        mMetrics.getStatuses(status);

        Blaze::ComponentStatus::InfoMap& map = status.getInfo();
        char8_t buf[64];

        // mMetrics.mActiveGames includes game groups, so its overall value isn't quite the "active game sessions" value we want for this status element (we don't want to include game groups), so add/replace it here...
        Metrics::TagPairList baseQuery;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t) getNumberOfActiveGameSessions(baseQuery));
        map["GMGauge_ACTIVE_GAMES"] = buf;

        // mMetrics.mAllGames doesn't include virtual games--which we want for this status element--so add/replace it here...
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t) mGameSessionMasterByIdMap.size());
        map["GMGauge_ALL_GAMES"] = buf;
    }

    /// @deprecated [metrics-refactor] getPSUByGameNetworkTopology RPC is polled for Graphite (which, in turn, is used by GOSCC) and can be removed when no longer used
    GetPSUByGameNetworkTopologyError::Error GameManagerMasterImpl::processGetPSUByGameNetworkTopology(const Blaze::GetMetricsByGeoIPDataRequest &request, GetPSUResponse& response, const Blaze::Message* message)
    {
        gController->tickDeprecatedRPCMetric("gamemanager_master", "getPSUByGameNetworkTopology", message);

        Metrics::SumsByTagValue sums;

        // active player counts by game network topology and country + timezone
        mMetrics.mActivePlayers.aggregate({ Metrics::Tag::network_topology }, sums);

        for (auto& entry : sums)
        {
            eastl::string metric = "GMGauge_ACTIVE_PLAYERS";
            for (auto& tagValue : entry.first)
            {
                metric += "_";
                metric += tagValue;
            }
            response.getPSUMap()[metric.c_str()] = static_cast<uint32_t>(entry.second);
        }

        return GetPSUByGameNetworkTopologyError::ERR_OK;
    }

    static void addTaggedSumsToConnectionMap(ConnectionMetricsMap& map, const char8_t* metricBaseName, const Metrics::SumsByTagValue& sums)
    {
        if (sums.empty())
            return;
        StringBuilder sb(metricBaseName);
        auto prefixLen = sb.length();
        auto& internalMap = map.asMap();
        // use eastl::vector_map::reserve() to avoid TdfMap::reserve() calling clear(), 
        // which is the default implementation of TdfMap :(
        internalMap.reserve(internalMap.size() + sums.size());
        for (auto& entry : sums)
        {
            for (auto& tagValue : entry.first)
            {
                sb.appendN("_", 1);
                sb.append(tagValue);
            }
            auto& added = internalMap.push_back_unsorted(); // NOTE: we don't sort until after all the push_back_unsorted()'s have been done
            auto len = sb.length();
            added.first.set(sb.c_str(), len);
            added.second = static_cast<int64_t>(entry.second);
            sb.trim(len - prefixLen); // trim back to common prefix length
        }
        // do a single batch sort, important for performance!
        map.fixupElementOrder();
    }

    // similar to addTaggedSumsToConnectionMap but with specific handling for 2 tags (cc_mode & connection_demangler_type)
    static void addTaggedSumsToConnectionMap_CCorDemangler(ConnectionMetricsMap& map, const char8_t* metricBaseName, const Metrics::SumsByTagValue& sums)
    {
        if (sums.empty())
            return;
        StringBuilder sb(metricBaseName);
        auto prefixLen = sb.length();
        auto& internalMap = map.asMap();
        // use eastl::vector_map::reserve() to avoid TdfMap::reserve() calling clear(), 
        // which is the default implementation of TdfMap :(
        internalMap.reserve(internalMap.size() + sums.size());
        // avoid allocator mismatch to avoid string copy realloc on construction
        EA::TDF::TdfString keyString(*internalMap.get_allocator().mpCoreAllocator);
        for (auto& entry : sums)
        {
            int tagIndex = 0;
            for (auto& tagValue : entry.first)
            {
                bool ccUsed = false;
                ++tagIndex;
                if (tagIndex == 7 /* cc_mode */)
                {
                    ccUsed = (blaze_strcmp(tagValue, "CC_UNUSED") != 0);
                    if (!ccUsed)
                        continue; // skip this tag
                }
                sb.appendN("_", 1);
                sb.append(tagValue);

                if (tagIndex == 7 /* cc_mode */ && ccUsed)
                    break; // skip any more tags
            }
            keyString.assignBuffer(sb.c_str());
            // using '+=' (instead of '=') because we're combining those special tags
            map[keyString] += static_cast<int64_t>(entry.second);
            sb.trim(sb.length() - prefixLen); // trim back to common prefix length
        }
    }

    /// @deprecated [metrics-refactor] getConnectionMetrics RPC is polled for Graphite (which, in turn, is used by GOSCC) and can be removed when no longer used
    GetConnectionMetricsError::Error GameManagerMasterImpl::processGetConnectionMetrics(Blaze::GameManager::GetConnectionMetricsResponse &response, const Message* message)
    {
        gController->tickDeprecatedRPCMetric("gamemanager_master", "getConnectionMetrics", message);

        ConnectionMetricsMap& map = response.getConnectionMetricsMap();

        // the aggregate queries should use the following order: (formerly found in GameManagerMetricHashes TDF)
        //   [tag = "a000"] ConnConciergeEligibleEnum ccEligible;
        //   [tag = "a001"] ConnectionJoinType joinType;
        //   [tag = "a002"] uint32_t index;                     // Used for metrics that store an array of values
        //   [tag = "a003"] GameType gameType;
        //   [tag = "a004"] GameNetworkTopology netTopology;
        //   [tag = "a005"] VoipTopology voipTopology;
        //   [tag = "a006"] JoinMethod joinMethod;
        //   [tag = "a007"] PlayerRemovedReason removeReason;
        //   [tag = "a008"] DemanglerConnectionHealthcheck dmConnHealthcheck;
        //   [tag = "a009"] PingSiteAlias pingSite;             // Since the ping sites aren't all known, we have to use a string.
        //   [tag = "a010"] Collections::AttributeValue gameMode; // populated by the well-known game mode attribute
        //   [tag = "a011"] string(MAX_ISP_LENGTH) geoIPData;
        //   [tag = "a012"] ConnSuccessMetricEnum success;
        //   [tag = "a013"] ConnImpactingMetricEnum impacting;
        //   [tag = "a014"] ConnConciergeModeMetricEnum cc;
        //   [tag = "a015"] ConnDemanglerMetricEnum demangler;
        //   [tag = "a016"] GameDestructionReason gameDestructionReason;

        Metrics::SumsByTagValue sums;

        // Connection Result Metrics ...

        // topology_pingsite_success
        mMetrics.mGameConns.aggregate({ Metrics::Tag::connection_join_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type }, sums);
        // topology_pingsite_success_impacting
        mMetrics.mGameConns.aggregate({ Metrics::Tag::connection_join_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::connection_impacting_type }, sums);

        addTaggedSumsToConnectionMap(map, "GMTotal_GAME_CONN", sums);

        sums.clear();
        // topology_pingsite_success_impacting_ccOrDemangler
        mMetrics.mGameConns.aggregate({ Metrics::Tag::cc_eligible_type, Metrics::Tag::connection_join_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::connection_impacting_type, Metrics::Tag::cc_mode, Metrics::Tag::connection_demangler_type }, sums);

        addTaggedSumsToConnectionMap_CCorDemangler(map, "GMTotal_GAME_CONN", sums);

        sums.clear();
        // topology_pingsite_success
        mMetrics.mVoipConns.aggregate({ Metrics::Tag::connection_join_type, Metrics::Tag::voip_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type }, sums);
        // topology_pingsite_success_impacting
        mMetrics.mVoipConns.aggregate({ Metrics::Tag::connection_join_type, Metrics::Tag::voip_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::connection_impacting_type }, sums);

        addTaggedSumsToConnectionMap(map, "GMTotal_VOIP_CONN", sums);

        sums.clear();
        // topology_pingsite_success_impacting_ccOrDemangler
        mMetrics.mVoipConns.aggregate({ Metrics::Tag::cc_eligible_type, Metrics::Tag::connection_join_type, Metrics::Tag::voip_topology, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::connection_impacting_type, Metrics::Tag::cc_mode, Metrics::Tag::connection_demangler_type }, sums);

        addTaggedSumsToConnectionMap_CCorDemangler(map, "GMTotal_VOIP_CONN", sums);

        // Connection Metrics ...

        StringBuilder sb;

        // Special helper used to efficiently push all the metrics into the map with minimal data copying and no sorting,
        // final sort of the map is performed at the end.
        auto pushMetricIntoMap = [&map, &sb](const Metrics::TagPairList& tagList, uint64_t value) {
            const auto prefixLen = sb.length();
            auto& internalMap = map.asMap();
            for (auto& tag : tagList)
            {
                sb.appendN("_", 1);
                sb.appendN(tag.second.c_str(), tag.second.size());
            }
            auto& added = internalMap.push_back_unsorted(); // NOTE: we don't sort until after all the push_back_unsorted()'s have been done
            const auto len = sb.length();
            added.first.set(sb.c_str(), len);
            added.second = value;
            sb.trim(len - prefixLen); // trim back to common prefix length
        };

        sb.reset().append("GMGauge_CURRENT_PACKET_LOSS");
        mMetrics.mCurrentPacketLoss.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { pushMetricIntoMap(tagList, value.get()); });

        sb.reset().append("GMTotal_MAX_CONNECTION_LATENCY");
        mMetrics.mMaxConnectionLatency.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_AVERAGE_CONNECTION_LATENCY");
        mMetrics.mAvgConnectionLatency.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_CONNECTION_LATENCY_SAMPLE");
        mMetrics.mConnectionLatencySample.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_CONNECTION_PACKET_LOSS");
        mMetrics.mConnectionPacketLoss.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_CONNECTION_PACKET_LOSS_SAMPLE");
        mMetrics.mConnectionPacketLossSample.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        // Non-Player Connection Metrics ...

        sb.reset().append("GMTotal_CONNECTION_LATENCY_SAMPLE_QOS_RULE");
        mMetrics.mConnectionLatencySampleQosRule.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_MAX_CONNECTION_LATENCY_QOS_RULE");
        mMetrics.mMaxConnectionLatencyQosRule.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_AVERAGE_CONNECTION_LATENCY_QOS_RULE");
        mMetrics.mAvgConnectionLatencyQosRule.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_CONNECTION_PACKET_LOSS_QOS_RULE");
        mMetrics.mConnectionPacketLossQosRule.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        sb.reset().append("GMTotal_CONNECTION_PACKET_LOSS_SAMPLE_QOS_RULE");
        mMetrics.mConnectionPacketLossSampleQosRule.iterate([&pushMetricIntoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { pushMetricIntoMap(tagList, value.getTotal()); });

        // IMPORTANT: Sorting the vector_map once *after* inserting all the elements with push_back_unsorted() is orders(!) of magnitude faster than keeping the map sorted during insertion of elements in random order
        // when the map gets big(map.size() > 1K), because N vector_map.push_back_unsorted's + one sort is O(N*Log(N)) while N vector_map.inserts is O(N^2). See: GOS-32916
        map.fixupElementOrder();

        return GetConnectionMetricsError::ERR_OK;
    }

    void GameManagerMasterImpl::clearGamePermissionSystem() 
    {
        mGlobalPermissionSystem.mHost.clear();
        mGlobalPermissionSystem.mAdmin.clear();
        mGlobalPermissionSystem.mParticipant.clear();
        mGlobalPermissionSystem.mSpectator.clear();
        // 
        eastl::map<eastl::string, GamePermissionSystemInternal*>::iterator curIter = mGamePermissionSystemMap.begin();
        eastl::map<eastl::string, GamePermissionSystemInternal*>::iterator endIter = mGamePermissionSystemMap.end();
        for (; curIter != endIter; ++curIter)
        {
            delete curIter->second;
        }
        mGamePermissionSystemMap.clear();
    }

    void addMultiActionsToActionSet(GamePermissionSystemInternal::GameActionSet& curSet)
    {
        if (curSet.find(GameAction::ALL_ACTIONS) != curSet.end())
        {
            curSet.insert(GameAction::ALL_ADMIN_ACTIONS);
            curSet.insert(GameAction::ALL_PLAYER_ACTIONS);
        }
        if (curSet.find(GameAction::ALL_ADMIN_ACTIONS) != curSet.end())
        {
            int32_t curAction = (int32_t)GameAction::START_OF_ADMIN_ACTIONS;
            while (++curAction != (int32_t)GameAction::END_OF_ADMIN_ACTIONS)     // Use pre-increment to skip the INVALID_ACTION.
            {
                curSet.insert((GameAction::Actions)curAction);
            }
        }
        if (curSet.find(GameAction::ALL_PLAYER_ACTIONS) != curSet.end())
        {
            int32_t curAction = (int32_t)GameAction::START_OF_PLAYER_ACTIONS;
            while (++curAction != (int32_t)GameAction::END_OF_PLAYER_ACTIONS)     // Use pre-increment to skip the INVALID_ACTION.
            {
                curSet.insert((GameAction::Actions)curAction);
            }
        }
    }

    void addMultiActionsToPermissionSystem(GamePermissionSystemInternal& curPermissionSystem)
    {
        addMultiActionsToActionSet(curPermissionSystem.mHost);
        addMultiActionsToActionSet(curPermissionSystem.mAdmin);
        addMultiActionsToActionSet(curPermissionSystem.mParticipant);
        addMultiActionsToActionSet(curPermissionSystem.mSpectator);
    }
    
    bool GameManagerMasterImpl::validateGameActionList(const GameActionList& actionList, const StringBuilder& listLocation, ConfigureValidationErrors& validationErrors) const
    {
        for (GameAction::Actions curAction : actionList)
        {
            // Invalid Actions:
            if ((int32_t)curAction <= (int32_t)GameAction::INVALID_ACTION ||
                curAction == GameAction::START_OF_ADMIN_ACTIONS ||
                curAction == GameAction::END_OF_ADMIN_ACTIONS ||
                curAction == GameAction::START_OF_PLAYER_ACTIONS ||
                (int32_t)curAction >= (int32_t)GameAction::END_OF_PLAYER_ACTIONS)
            {
                StringBuilder strBuilder;
                strBuilder << "Game Permission is invalid: " << GameAction::ActionsToString(curAction) <<
                    ", Location: '" << listLocation << "'.  If this permission is not used in your config file, and this occurred after a version update, please recompile and deploy all ConfigMaster and GameManager servers. (May be due to enum change.)";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }

        // Single Action sanity check (may occur if enums are altered and ALL_*_ACTIONS changes to something else. 
        if (actionList.size() == 1)
        {
            GameAction::Actions curAction = actionList.front();
            if (curAction != GameAction::ALL_ACTIONS &&
                curAction != GameAction::ALL_ADMIN_ACTIONS &&
                curAction != GameAction::ALL_PLAYER_ACTIONS)
            {
                // Technically this isn't invalid, but it's unexpected and could happen after an enum change:
                StringBuilder strBuilder;
                strBuilder << "Unexpected.  Single Game Permission used (not ALL_*_ACTIONS): " << GameAction::ActionsToString(curAction) <<
                    ", Location: '" << listLocation << "'.  If this permission is not used in your config file, and this occurred after a version update, please recompile and deploy all ConfigMaster and GameManager servers. (May be due to enum change.)" <<
                    " If you actually just want to use a single permission, contact GOS for a workaround." ;
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
        return (validationErrors.getErrorMessages().size() == 0);
    }

    bool GameManagerMasterImpl::validateGamePermissions(const GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors) const
    {
        StringBuilder listLocation;

        const GamePermissionSystem& defConfigPermission = config.getGameSession().getDefaultGamePermissionSystem();
        listLocation.reset();  listLocation << "Global Host";  validateGameActionList(defConfigPermission.getHost(), listLocation, validationErrors);
        listLocation.reset();  listLocation << "Global Admin"; validateGameActionList(defConfigPermission.getAdmin(), listLocation, validationErrors);
        listLocation.reset();  listLocation << "Global Participant"; validateGameActionList(defConfigPermission.getParticipant(), listLocation, validationErrors);
        listLocation.reset();  listLocation << "Global Spectator";   validateGameActionList(defConfigPermission.getSpectator(), listLocation, validationErrors);

        for (GamePermissionSystemByNameMap::const_iterator iter = config.getGameSession().getGamePermissionSystemConfig().begin(); iter != config.getGameSession().getGamePermissionSystemConfig().end(); ++iter)
        {
            const char8_t* systemName = iter->first.c_str();
            const GamePermissionSystem& configPermission = (*iter->second);

            listLocation.reset();  listLocation << systemName << " Host";  validateGameActionList(configPermission.getHost(), listLocation, validationErrors);
            listLocation.reset();  listLocation << systemName << " Admin"; validateGameActionList(configPermission.getAdmin(), listLocation, validationErrors);
            listLocation.reset();  listLocation << systemName << " Participant"; validateGameActionList(configPermission.getParticipant(), listLocation, validationErrors);
            listLocation.reset();  listLocation << systemName << " Spectator";   validateGameActionList(configPermission.getSpectator(), listLocation, validationErrors);
        }
        return (validationErrors.getErrorMessages().size() == 0);
    }



    void GameManagerMasterImpl::configureGamePermissionSystem() 
    {
        clearGamePermissionSystem();

        const GamePermissionSystem& defConfigPermission = getConfig().getGameSession().getDefaultGamePermissionSystem();
        mGlobalPermissionSystem.mHost.insert(defConfigPermission.getHost().begin(), defConfigPermission.getHost().end());
        mGlobalPermissionSystem.mAdmin.insert(defConfigPermission.getAdmin().begin(), defConfigPermission.getAdmin().end());
        mGlobalPermissionSystem.mParticipant.insert(defConfigPermission.getParticipant().begin(), defConfigPermission.getParticipant().end());
        mGlobalPermissionSystem.mSpectator.insert(defConfigPermission.getSpectator().begin(), defConfigPermission.getSpectator().end());
        mGlobalPermissionSystem.mEnableAutomaticAdminSelection = defConfigPermission.getEnableAutomaticAdminSelection();

        for (GamePermissionSystemByNameMap::const_iterator iter = getConfig().getGameSession().getGamePermissionSystemConfig().begin(); iter != getConfig().getGameSession().getGamePermissionSystemConfig().end(); ++iter)
        {
            const char8_t* systemName = iter->first.c_str();
            const GamePermissionSystem& configPermission = (*iter->second);

            GamePermissionSystemInternal* permissionSystem = BLAZE_NEW GamePermissionSystemInternal;
            permissionSystem->mHost.insert(configPermission.getHost().begin(), configPermission.getHost().end());
            permissionSystem->mAdmin.insert(configPermission.getAdmin().begin(), configPermission.getAdmin().end());
            permissionSystem->mParticipant.insert(configPermission.getParticipant().begin(), configPermission.getParticipant().end());
            permissionSystem->mSpectator.insert(configPermission.getSpectator().begin(), configPermission.getSpectator().end());
            permissionSystem->mEnableAutomaticAdminSelection = configPermission.getEnableAutomaticAdminSelection();

            addMultiActionsToPermissionSystem(*permissionSystem);

            mGamePermissionSystemMap[systemName] = permissionSystem;
        }

        if (mGamePermissionSystemMap.empty() &&
            mGlobalPermissionSystem.mHost.empty() &&
            mGlobalPermissionSystem.mAdmin.empty() &&
            mGlobalPermissionSystem.mParticipant.empty() &&
            mGlobalPermissionSystem.mSpectator.empty() &&
            mGlobalPermissionSystem.mEnableAutomaticAdminSelection == false)
        {
            INFO_LOG("[GameManagerMaster] Missing Game Permissions.  Adding default Global permission with all settings enabled for Admins.");

            mGlobalPermissionSystem.mAdmin.insert(GameAction::ALL_ADMIN_ACTIONS);
            mGlobalPermissionSystem.mParticipant.insert(GameAction::ALL_PLAYER_ACTIONS);
            mGlobalPermissionSystem.mSpectator.insert(GameAction::ALL_PLAYER_ACTIONS);
            mGlobalPermissionSystem.mEnableAutomaticAdminSelection = true;
        }

        // Add the global multi actions at the end in case the config had to add defaults:
        addMultiActionsToPermissionSystem(mGlobalPermissionSystem);
    }

    const GamePermissionSystemInternal& GameManagerMasterImpl::getGamePermissionSystem(const char8_t* permissionSystemName) const
    {
        eastl::map<eastl::string, GamePermissionSystemInternal*>::const_iterator iter = mGamePermissionSystemMap.find(permissionSystemName);
        if (iter == mGamePermissionSystemMap.end())
            return mGlobalPermissionSystem;

        return (*iter->second);
    }

    size_t GameManagerMasterImpl::getNumberOfActiveGameSessions(const Metrics::TagPairList& baseQuery) const
    {
        GameManagerMasterMetrics& metrics = const_cast<GameManagerMasterMetrics&>(mMetrics);

        // We only want the Games, not the GameGroups:
        Metrics::TagPairList noGameGroupQuery = { { Metrics::Tag::game_type, GameTypeToString(GAME_TYPE_GAMESESSION) } };
        noGameGroupQuery.insert(noGameGroupQuery.begin()+1, baseQuery.begin(), baseQuery.end());

        // This function is used in the Drain call, so it needs to get to 0 at the right time. 
        // Novel new approach - Just check the ActiveGames metric instead of doing manually recalculating it. 
        return (size_t)metrics.mActiveGames.get(noGameGroupQuery);
    }

    /*! ************************************************************************************************/
    /*! \brief Logs a dedicated server event for easy tracing of dedicated server information through the master logs

        \param[in] eventName - the type of event happening, ex: "game created", "game reset", etc.
        \param[in] dedicatedServerGame - the dedicated server game to draw logging info from
        \param[in] additionalOutput - any additional information that should be appended to the end of the log statement;
                                        can be used for event-specific additional logging ex: inserting information about the player resetting a dedicated server.
    ***************************************************************************************************/
    void GameManagerMasterImpl::logDedicatedServerEvent(const char8_t* eventName, const GameSessionMaster &dedicatedServerGame, const char8_t* additionalOutput /* = "" */) const
    {
        const UserSessionInfo& dedicatedHostInfo = dedicatedServerGame.getTopologyHostUserInfo();
        if (dedicatedServerGame.isInactiveVirtual())
        {
            INFO_LOG("[GameManagerMaster] Dedicated server event: " << eventName << ". Game Id (" << dedicatedServerGame.getGameId() << "), game is in INACTIVE_VIRTUAL state.");
            return;
        }
        else if (dedicatedHostInfo.getSessionId() == INVALID_USER_SESSION_ID)
        {
            WARN_LOG("[GameManagerMaster] Dedicated server event: " << eventName << ". Game Id (" << dedicatedServerGame.getGameId() << "), Game creator Session Id (" 
                    << dedicatedServerGame.getDedicatedServerHostSessionId() << ") - topologyHostUserInfo was invalid - possible bug.");
        }

        char8_t addressBuf[1024];
        addressBuf[0] = '\0';

        NetworkAddressList::const_iterator iter = dedicatedServerGame.getTopologyHostNetworkAddressList().begin();
        NetworkAddressList::const_iterator end = dedicatedServerGame.getTopologyHostNetworkAddressList().end();
        for (; iter != end; ++iter)
        {
            const NetworkAddress& address = **iter;

            switch(address.getActiveMember())
            {
                case NetworkAddress::MEMBER_HOSTNAMEADDRESS :
                    blaze_snzprintf(addressBuf, sizeof(addressBuf), "%sgame host address: '%s:%d' ", addressBuf, address.getHostNameAddress()->getHostName(), address.getHostNameAddress()->getPort());  
                    break;
                case NetworkAddress::MEMBER_IPADDRESS :
                    {                
                        InetAddress hostAddress(address.getIpAddress()->getIp(), address.getIpAddress()->getPort(), InetAddress::HOST);
                        blaze_snzprintf(addressBuf, sizeof(addressBuf), "%sgame host address: '%s:%d' ", addressBuf, hostAddress.getHostname(), hostAddress.getPort(InetAddress::HOST));  
                    }
                    break;
                case NetworkAddress::MEMBER_IPPAIRADDRESS :
                    {
                        InetAddress hostInternalAddress(address.getIpPairAddress()->getInternalAddress().getIp(), address.getIpPairAddress()->getInternalAddress().getPort(), InetAddress::HOST);
                        InetAddress hostExternalAddress(address.getIpPairAddress()->getExternalAddress().getIp(), address.getIpPairAddress()->getExternalAddress().getPort(), InetAddress::HOST);
                        blaze_snzprintf(addressBuf, sizeof(addressBuf), "%sgame host internal address: '%s:%d', game host external address: '%s:%d' ", 
                            addressBuf, hostInternalAddress.getHostname(), hostInternalAddress.getPort(InetAddress::HOST), hostExternalAddress.getHostname(), hostExternalAddress.getPort(InetAddress::HOST));  
                    }
                    break;
                default :
                    // got invalid, unset or xbox client address
                    blaze_snzprintf(addressBuf, sizeof(addressBuf), "%sgame session hostname unavailable, using NetworkAddress member index '%d' possible bug. ", 
                        addressBuf, address.getActiveMemberIndex());  
                    break;
            }
        }
        INFO_LOG("[GameManagerMaster] Dedicated server event: " << eventName << ". Game Id (" << dedicatedServerGame.getGameId() 
                 << "), Game creator Blaze Id (" << dedicatedServerGame.getTopologyHostInfo().getPlayerId() << "), game creator persona '" 
                 << dedicatedHostInfo.getUserInfo().getPersonaName() << "', game region '" << dedicatedServerGame.getBestPingSiteAlias() 
                 << "', status Url '" << dedicatedServerGame.getGameStatusUrl() << "', " << addressBuf << additionalOutput << ". ");
    }

    /*! ************************************************************************************************/
    /*! \brief Logs a dedicated server event's player info for easy tracing of dedicated server information through the master logs

    \param[in] eventName - the type of event happening, ex: "game created", "game reset", etc.
    \param[in] userSession - the user associated with this dedicated server event.
    \param[in] dedicatedServerGame - the dedicated server game to draw logging info from
    \param[in] additionalOutput - any additional information that should be appended to the end of the log statement;
                             can be used for adding more specific information to an event.
    ***************************************************************************************************/
    void GameManagerMasterImpl::logUserInfoForDedicatedServerEvent(const char8_t* eventName, BlazeId blazeId, const PingSiteLatencyByAliasMap &userLatencyMap, const GameSessionMaster &dedicatedServerGame, const char8_t* additionalOutput /* = "" */) const
    {
        // build the latency map
        size_t totalPingsiteNameChars = 0;
        PingSiteLatencyByAliasMap::const_iterator iter = userLatencyMap.begin();
        PingSiteLatencyByAliasMap::const_iterator end = userLatencyMap.end();
        for (; iter != end; ++iter)
        {
            totalPingsiteNameChars += iter->first.length();
        }
        static const char equalsStr[] = " = ";
        // 12 digits for MAX_QOS_LATENCY, 3 for the punctuation, " , ", and + 1 for '\0' in temporary buffer
        const size_t pingValueAndPunctuation = 12 + 3 + 1;
        const size_t bufSize = userLatencyMap.size() * (sizeof(equalsStr) + pingValueAndPunctuation) +
            totalPingsiteNameChars +
            1; // null terminator.

        char8_t* buf =  BLAZE_NEW_ARRAY(char8_t, bufSize);
        buf[0] = '\0';
        for (iter = userLatencyMap.begin(); iter != end; ++iter)
        {
            blaze_strnzcat(buf, iter->first.c_str(), bufSize);
            blaze_strnzcat(buf, equalsStr, bufSize);
            char8_t latency[pingValueAndPunctuation];
            blaze_snzprintf(latency, sizeof(latency), "%d , ", iter->second);
            blaze_strnzcat(buf, latency, bufSize);
        }

        INFO_LOG("[GameManagerMaster] Dedicated server event: " << eventName << ". Game Id (" << dedicatedServerGame.getGameId() 
                 << "), player Blaze Id (" << blazeId
                 << "), player ping site information: " << buf << additionalOutput << ".");

        delete [] buf;
    }

    AddQueuedPlayerToGameMasterError::Error GameManagerMasterImpl::processAddQueuedPlayerToGameMaster(const Blaze::GameManager::AddQueuedPlayerToGameRequest &request, const Message* message)
    {
        if (getConfig().getGameSession().getGameSessionQueueMethod() != GAME_QUEUE_ADMIN_MANAGED)
        {
            return AddQueuedPlayerToGameMasterError::GAMEMANAGER_ERR_INVALID_QUEUE_METHOD;
        }

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return AddQueuedPlayerToGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (!gameSessionMaster->hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::MANAGE_QUEUED_PLAYERS))
        {
            TRACE_LOG("[GameManagerMaster] Non admin player can't add queued players for game(" << gameSessionMaster->getGameId() << ").");
            return AddQueuedPlayerToGameMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        // admin managed queue has been pumped.
        gameSessionMaster->decrementQueuePumpsPending();

        PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(request.getPlayerId());
        if (EA_UNLIKELY(playerInfo == nullptr))
        {
            return AddQueuedPlayerToGameMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        
        BlazeRpcError addErr = gameSessionMaster->addPlayerFromQueue(*playerInfo, request.getPlayerTeamIndex(), request.getOverridePlayerRole() ? request.getPlayerRole() : nullptr);
        if (gameSessionMaster->isAnyQueueMemberPromotable() && !gameSessionMaster->areQueuePumpsPending())
        {
            // Send notification to admin if we still have someone in the queue that needs to be pumped (and no pumps were pending) (also triggers updatePromotabilityOfQueuedPlayers internally)
            gameSessionMaster->processPlayerAdminManagedQueue();
        }
        else
        {
            gameSessionMaster->updatePromotabilityOfQueuedPlayers();
        }

        return AddQueuedPlayerToGameMasterError::commandErrorFromBlazeError(addErr);
    }

    DemoteReservedPlayerToQueueMasterError::Error GameManagerMasterImpl::processDemoteReservedPlayerToQueueMaster(const Blaze::GameManager::DemoteReservedPlayerToQueueRequest &request, const Message* message)
    {
        if (getConfig().getGameSession().getGameSessionQueueMethod() != GAME_QUEUE_ADMIN_MANAGED)
        {
            return DemoteReservedPlayerToQueueMasterError::GAMEMANAGER_ERR_INVALID_QUEUE_METHOD;
        }

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession( request.getGameId() );
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return DemoteReservedPlayerToQueueMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (!gameSessionMaster->hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::MANAGE_QUEUED_PLAYERS))
        {
            TRACE_LOG("[GameManagerMaster] Non admin player can't queue reserved players for game(" << gameSessionMaster->getGameId() << ").");
            return DemoteReservedPlayerToQueueMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        if (gameSessionMaster->isQueueFull())
        {
            TRACE_LOG("[GameSessionMaster] Not allowing player(" << request.getPlayerId() <<
                ") to demote to queue in game(" << request.getGameId() << ") because the queue if full.");
            return DemoteReservedPlayerToQueueMasterError::GAMEMANAGER_ERR_QUEUE_FULL;
        }

        // admin managed queue needs to be pumped.
        gameSessionMaster->incrementQueuePumpsPending();

        PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(request.getPlayerId());
        if (EA_UNLIKELY(playerInfo == nullptr))
        {
            return DemoteReservedPlayerToQueueMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        
        BlazeRpcError demoteErr = gameSessionMaster->demotePlayerToQueue(*playerInfo);
        if (gameSessionMaster->isAnyQueueMemberPromotable() && !gameSessionMaster->areQueuePumpsPending())
        {
            // Send notification to admin if we still have someone in the queue that needs to be pumped (and no pumps were pending) (also triggers updatePromotabilityOfQueuedPlayers internally)
            gameSessionMaster->processPlayerAdminManagedQueue();
        }
        else
        {
            gameSessionMaster->updatePromotabilityOfQueuedPlayers();
        }

        return DemoteReservedPlayerToQueueMasterError::commandErrorFromBlazeError(demoteErr);
    }

    /*! ************************************************************************************************/
    /*! \brief Process a list of matching session to games, and attempt to join/create them.

        \param[in] request - the request containing the list of our matches.
        \param[in/out] response - a response object to fill out with any failures
        \param[in] message
    ***************************************************************************************************/
    MatchmakingFoundGameError::Error GameManagerMasterImpl::processMatchmakingFoundGame(const Blaze::GameManager::MatchmakingFoundGameRequest &request, 
        Blaze::GameManager::MatchmakingFoundGameResponse &response, const Message* message)
    {
        // Check for existence
        BlazeId ownerBlazeId = gUserSessionManager->getBlazeId(request.getOwnerUserSessionId());
        if (EA_UNLIKELY(ownerBlazeId == INVALID_BLAZE_ID))
        {
            // Bail out early if the user session owning the matchmaking session is no longer available.
            TRACE_LOG("[GameManagerMasterImpl] Matchmaking session owner user session(" << request.getOwnerUserSessionId() << ") not found");
            return MatchmakingFoundGameError::GAMEMANAGER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        }

        LogContextOverride logAudit(request.getOwnerUserSessionId());

        JoinGameMasterResponse joinResponse; // hold list of external ids joined for this game
        BlazeRpcError err = GAMEMANAGER_ERR_MATCHMAKING_NO_JOINABLE_GAMES;
        const MatchedGame& matchInfo = request.getMatchedGame();

        TeamIdRoleSizeMap requiredTeamRoles; 
        // same for the Qos list- we'll always be pulling in the same set of players
        PlayerIdList mmQosList;

        bool performQosValidation = false; // saved off from our last join request, as it may differ by topology.
        GameEntryType gameEntryType = GAME_ENTRY_TYPE_DIRECT; // also needed to determine if QoS is appropriate

        // reset the list of possible joined players.
        clearExternalSessionJoinParams(joinResponse.getExternalSessionParams());

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(matchInfo.getGameId(), false); // AutoCommit is off to avoid attempting noop commit when we early out
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMasterImpl] Matchmaking session matched game(" << matchInfo.getGameId() << ") that is no longer around.");
            return MatchmakingFoundGameError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // Declare the setup reasons map here so it's usable for building the join events
        UserSessionSetupReasonMap setupReasons;
        RelatedPlayerSetsMap joiningPlayersLists;
        TeamIdToTeamIndexMap idsToIndexes;
        const char8_t* leaderPersonaName = "";
        ClientPlatformType leaderClientPlatform = INVALID;
        if (request.getGameRequest().getJoinGameRequest() != nullptr)
        {
            // individual join
            JoinGameMasterRequest &joinRequest = const_cast<JoinGameMasterRequest &>(*( request.getGameRequest().getJoinGameRequest()));
            joinRequest.getJoinRequest().setGameId(matchInfo.getGameId());

            performQosValidation = joinRequest.getUserJoinInfo().getPerformQosValidation();
            gameEntryType = joinRequest.getJoinRequest().getPlayerJoinData().getGameEntryType();

            if (requiredTeamRoles.empty())
                buildTeamIdRoleSizeMapFromJoinRequest(joinRequest.getJoinRequest(), requiredTeamRoles, 1);

            if (mmQosList.empty())
            {
                // single user join
                mmQosList.push_back(joinRequest.getUserJoinInfo().getUser().getUserInfo().getId());
            }
                               
            // Matchmaking requests only require a user to provide a team id since we do not know which index
            // that team may exist at for any given game we match.
            // Determine the index of our requested team id in this particular game.
            const Matchmaker::GroupUedExpressionList* groupUedExpressionList = nullptr;
            if (request.getFoundTeam().getTeamUEDKey() != INVALID_USER_EXTENDED_DATA_KEY)
            {
                groupUedExpressionList= getGroupUedExpressionList(request.getFoundTeam().getTeamUEDRuleName());
            }

            // Since we're only testing with one player, we can just use the roleRequirement directly. 
            TeamId myTeamId = requiredTeamRoles.begin()->first;
            TeamIndex foundTeamIndex = UNSPECIFIED_TEAM_INDEX;
            err = gameSessionMaster->findOpenTeam(request.getFoundTeam(), requiredTeamRoles.begin(), groupUedExpressionList, foundTeamIndex);
            if (err != ERR_OK)
            {
                TRACE_LOG("[GameManagerMasterImpl] Matchmaking session(" << request.getMatchmakingSessionId() 
                << ") failed to find open team([" << foundTeamIndex << "]:" << myTeamId
                        << ") in game(" << matchInfo.getGameId() << "), error(" << err << ":" 
                        << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
                return MatchmakingFoundGameError::commandErrorFromBlazeError(err);
            }
            idsToIndexes[myTeamId] = foundTeamIndex;
            joinRequest.getJoinRequest().getPlayerJoinData().setDefaultTeamIndex(foundTeamIndex);
            leaderPersonaName = joinRequest.getUserJoinInfo().getUser().getUserInfo().getPersonaName();
            leaderClientPlatform = joinRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo().getClientPlatform();
                       

            // the creators reason to send back to the client as to why they joined the game.
            GameSetupReason creatorJoinContext;
            creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_MATCHMAKINGSETUPCONTEXT);
            creatorJoinContext.getMatchmakingSetupContext()->setFitScore(matchInfo.getFitScore());
            creatorJoinContext.getMatchmakingSetupContext()->setMatchmakingResult(SUCCESS_JOINED_EXISTING_GAME);
            creatorJoinContext.getMatchmakingSetupContext()->setSessionId(request.getMatchmakingSessionId());
            creatorJoinContext.getMatchmakingSetupContext()->setScenarioId(request.getMatchmakingScenarioId());
            creatorJoinContext.getMatchmakingSetupContext()->setUserSessionId(request.getOwnerUserSessionId());
            creatorJoinContext.getMatchmakingSetupContext()->setMaxPossibleFitScore(matchInfo.getMaxFitScore());
            creatorJoinContext.getMatchmakingSetupContext()->setTimeToMatch(matchInfo.getTimeToMatch());
            creatorJoinContext.getMatchmakingSetupContext()->setEstimatedTimeToMatch(matchInfo.getEstimatedTimeToMatch());
            creatorJoinContext.getMatchmakingSetupContext()->setMatchmakingTimeoutDuration(request.getMatchmakingTimeout());
            creatorJoinContext.getMatchmakingSetupContext()->setGameEntryType(gameEntryType);
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersOnline(request.getTotalUsersOnline());
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersInGame(request.getTotalUsersInGame());
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersInMatchmaking(request.getTotalUsersInMatchmaking());
            creatorJoinContext.getMatchmakingSetupContext()->setInitiatorId(ownerBlazeId);

            setupReasons[joinRequest.getUserJoinInfo().getUser().getSessionId()] = &creatorJoinContext;
            // no associated players for an individual MM session

            // set up MM QoS data as needed
            // build out our QoS tracking list
            if (performQosValidation && gameEntryType == GAME_ENTRY_TYPE_DIRECT)
            {
                gameSessionMaster->addMatchmakingQosData(mmQosList, request.getMatchmakingSessionId());
            }

            Blaze::EntryCriteriaError error;

            err = (BlazeRpcError)gameSessionMaster->joinGame(joinRequest, joinResponse, error, creatorJoinContext);

            if (err == ERR_OK)
            {
                // need the setup reasons, the PIN submission goes back to the MM
                buildMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists, response.getPinSubmission(), request.getTrackingTag());
            }
        }
        else if (request.getGameRequest().getJoinGameByGroupRequest() != nullptr)
        {
            // join by group
            JoinGameByGroupMasterRequest &joinRequest = const_cast<JoinGameByGroupMasterRequest &>(*( request.getGameRequest().getJoinGameByGroupRequest()));
            joinRequest.getJoinRequest().setGameId(matchInfo.getGameId());

            gameEntryType = joinRequest.getJoinRequest().getPlayerJoinData().getGameEntryType();

            uint16_t nonOptionalUserCount = 0;
            bool qosListWasEmpty = mmQosList.empty();
            performQosValidation = false;
            // Add the non-optional guys:
            UserJoinInfoList::const_iterator iter = joinRequest.getUsersInfo().begin();
            UserJoinInfoList::const_iterator end = joinRequest.getUsersInfo().end();
            for (; iter != end; ++iter)
            {
                // All non-optional members of the group join should have this set the same:
                if ((*iter)->getPerformQosValidation())
                {
                    performQosValidation = true;
                    if (qosListWasEmpty)
                    {
                        mmQosList.push_back((*iter)->getUser().getUserInfo().getId());
                    }
                }

                if (!(*iter)->getIsOptional())
                {
                    if (qosListWasEmpty)
                    {
                        mmQosList.push_back((*iter)->getUser().getUserInfo().getId());
                    }
                    ++nonOptionalUserCount;
                }
            }

            if (requiredTeamRoles.empty())
            {
                buildTeamIdRoleSizeMapFromJoinRequest(joinRequest.getJoinRequest(), requiredTeamRoles, nonOptionalUserCount);
            }
                            
            // Normal single-team matchmaking: 
            // Team UED rules only apply if we're a single team join (too complex otherwise).
            if (requiredTeamRoles.size() == 1)
            {
                // Matchmaking requests only require a user to provide a team id since we do not know which index
                // that team may exist at for any given game we match.
                // Determine the index of our requested team id in this particular game.
                const Matchmaker::GroupUedExpressionList* groupUedExpressionList = nullptr;
                if (request.getFoundTeam().getTeamUEDKey() != INVALID_USER_EXTENDED_DATA_KEY)
                {
                    groupUedExpressionList = getGroupUedExpressionList(request.getFoundTeam().getTeamUEDRuleName());
                }

                TeamId myTeamId = requiredTeamRoles.begin()->first;
                TeamIndex foundTeamIndex = UNSPECIFIED_TEAM_INDEX;
                err = gameSessionMaster->findOpenTeam(request.getFoundTeam(), requiredTeamRoles.begin(), groupUedExpressionList, foundTeamIndex);
                if (err != ERR_OK)
                {
                    TRACE_LOG("[GameManagerMasterImpl] Matchmaking session(" << request.getMatchmakingSessionId() 
                                << ") failed to find open team([" << foundTeamIndex << "]:" << myTeamId << ") in game(" 
                                << matchInfo.getGameId() << "), error(" << err << ":" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
                    return MatchmakingFoundGameError::commandErrorFromBlazeError(err);
                }
                idsToIndexes[myTeamId] = foundTeamIndex;
            }
            else
            {
                // If multiple teams are in the MM request, we want to just check that the team members can fit in the teams they requested.
                err = gameSessionMaster->findOpenTeams(requiredTeamRoles, idsToIndexes);
                if (err != ERR_OK)
                {
                    TRACE_LOG("[GameManagerMasterImpl] Matchmaking session(" << request.getMatchmakingSessionId() 
                                << ") failed to find open teams(" << requiredTeamRoles.size() << " teams required) in game(" 
                                << matchInfo.getGameId() << "), error(" << err << ":" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
                    return MatchmakingFoundGameError::commandErrorFromBlazeError(err);
                }
            }

            // Update the team index mappings for all user information and users: 
            UserJoinInfoList::const_iterator it = joinRequest.getUsersInfo().begin();
            UserJoinInfoList::const_iterator itEnd = joinRequest.getUsersInfo().end();
            for (; it != itEnd; ++it)
            {
                PerPlayerJoinData* playerData = lookupPerPlayerJoinData(joinRequest.getJoinRequest().getPlayerJoinData(), (*it)->getUser().getUserInfo());
                if (playerData != nullptr)
                {
                    TeamId playerTeamId = joinRequest.getJoinRequest().getPlayerJoinData().getDefaultTeamId();
                    if (playerData->getTeamId() != INVALID_TEAM_ID)
                        playerTeamId = playerData->getTeamId();
                                
                    TeamIndex newTeamIndex = idsToIndexes[playerTeamId];
                    playerData->setTeamIndex(newTeamIndex);
                }
            }
            joinRequest.getJoinRequest().getPlayerJoinData().setDefaultTeamIndex(idsToIndexes[joinRequest.getJoinRequest().getPlayerJoinData().getDefaultTeamId()]);

            const UserSessionInfo* hostSessionInfo = getHostSessionInfo(joinRequest.getUsersInfo());
            if (hostSessionInfo != nullptr)
            {
                leaderPersonaName = hostSessionInfo->getUserInfo().getPersonaName();
                leaderClientPlatform = hostSessionInfo->getUserInfo().getPlatformInfo().getClientPlatform();
            }

            // the creators reason to send back to the client as to why they joined the game.
            GameSetupReason creatorJoinContext;
            creatorJoinContext.switchActiveMember(GameSetupReason::MEMBER_MATCHMAKINGSETUPCONTEXT);
            creatorJoinContext.getMatchmakingSetupContext()->setFitScore(matchInfo.getFitScore());
            creatorJoinContext.getMatchmakingSetupContext()->setMatchmakingResult(SUCCESS_JOINED_EXISTING_GAME);
            creatorJoinContext.getMatchmakingSetupContext()->setSessionId(request.getMatchmakingSessionId());
            creatorJoinContext.getMatchmakingSetupContext()->setScenarioId(request.getMatchmakingScenarioId());
            creatorJoinContext.getMatchmakingSetupContext()->setUserSessionId(request.getOwnerUserSessionId());
            creatorJoinContext.getMatchmakingSetupContext()->setMaxPossibleFitScore(matchInfo.getMaxFitScore());
            creatorJoinContext.getMatchmakingSetupContext()->setTimeToMatch(matchInfo.getTimeToMatch());
            creatorJoinContext.getMatchmakingSetupContext()->setEstimatedTimeToMatch(matchInfo.getEstimatedTimeToMatch());
            creatorJoinContext.getMatchmakingSetupContext()->setMatchmakingTimeoutDuration(request.getMatchmakingTimeout());
            creatorJoinContext.getMatchmakingSetupContext()->setGameEntryType(gameEntryType);
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersOnline(request.getTotalUsersOnline());
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersInGame(request.getTotalUsersInGame());
            creatorJoinContext.getMatchmakingSetupContext()->setTotalUsersInMatchmaking(request.getTotalUsersInMatchmaking());
            creatorJoinContext.getMatchmakingSetupContext()->setInitiatorId(ownerBlazeId);

            // PG member reasons to send back to the client for as to why they joined the game.
            GameSetupReason pgContext;
            pgContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTMATCHMAKINGSETUPCONTEXT);
            pgContext.getIndirectMatchmakingSetupContext()->setFitScore(matchInfo.getFitScore());
            pgContext.getIndirectMatchmakingSetupContext()->setMatchmakingResult(SUCCESS_JOINED_EXISTING_GAME);
            pgContext.getIndirectMatchmakingSetupContext()->setSessionId(request.getMatchmakingSessionId());
            pgContext.getIndirectMatchmakingSetupContext()->setScenarioId(request.getMatchmakingScenarioId());
            pgContext.getIndirectMatchmakingSetupContext()->setUserSessionId(request.getOwnerUserSessionId());
            pgContext.getIndirectMatchmakingSetupContext()->setUserGroupId(joinRequest.getJoinRequest().getPlayerJoinData().getGroupId());
            pgContext.getIndirectMatchmakingSetupContext()->setMaxPossibleFitScore(matchInfo.getMaxFitScore());
            pgContext.getIndirectMatchmakingSetupContext()->setTimeToMatch(matchInfo.getTimeToMatch());
            pgContext.getIndirectMatchmakingSetupContext()->setEstimatedTimeToMatch(matchInfo.getEstimatedTimeToMatch());
            pgContext.getIndirectMatchmakingSetupContext()->setMatchmakingTimeoutDuration(request.getMatchmakingTimeout());
            pgContext.getIndirectMatchmakingSetupContext()->setRequiresClientVersionCheck(getConfig().getGameSession().getEvaluateGameProtocolVersionString());
            pgContext.getIndirectMatchmakingSetupContext()->setGameEntryType(gameEntryType);
            pgContext.getIndirectMatchmakingSetupContext()->setTotalUsersOnline(request.getTotalUsersOnline());
            pgContext.getIndirectMatchmakingSetupContext()->setTotalUsersInGame(request.getTotalUsersInGame());
            pgContext.getIndirectMatchmakingSetupContext()->setTotalUsersInMatchmaking(request.getTotalUsersInMatchmaking());
            pgContext.getIndirectMatchmakingSetupContext()->setInitiatorId(ownerBlazeId);

            // Creators join game reason.
            if (joinRequest.getJoinLeader())
            {
                fillUserSessionSetupReasonMap(setupReasons, joinRequest.getUsersInfo(), &creatorJoinContext, &pgContext, &pgContext);
            }
            else
            {  
                // If this was an indirect matchmaking attempt, make sure the leader does an indirect join as well:
                fillUserSessionSetupReasonMap(setupReasons, joinRequest.getUsersInfo(), &pgContext, &pgContext, &pgContext);
            }
            buildRelatedPlayersMap(gameSessionMaster.get(), joinRequest.getUsersInfo(), joiningPlayersLists);

            // set up MM QoS data as needed
            // build out our QoS tracking list
            if (performQosValidation && gameEntryType == GAME_ENTRY_TYPE_DIRECT)
            {
                gameSessionMaster->addMatchmakingQosData(mmQosList, request.getMatchmakingSessionId());
            }

            Blaze::EntryCriteriaError error;

            err = (BlazeRpcError)gameSessionMaster->joinGameByGroup(joinRequest, joinResponse, error, setupReasons);
            if (err == ERR_OK)
            {
                // need the setup reasons, the PIN submission goes back to the MM
                buildMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists, response.getPinSubmission(), request.getTrackingTag());
            }
        }

        // We joined this game.
        if (err == ERR_OK)
        {
            // Now we enable release to auto-commit to storage
            gameSessionMaster->setAutoCommitOnRelease();

            response.setJoinedGameId(matchInfo.getGameId());
            response.setMatchmakingSessionId(request.getMatchmakingSessionId());
            response.setMatchmakingScenarioId(request.getMatchmakingScenarioId());
            response.setOwnerUserSessionId(request.getOwnerUserSessionId());
            response.setIsAwaitingQosValidation(performQosValidation);
            response.setFoundGameParticipantCapacity(gameSessionMaster->getTotalParticipantCapacity());
            response.setFoundGamePlayerCount(gameSessionMaster->getPlayerRoster()->getPlayerCount(PlayerRoster::ACTIVE_PLAYERS));
            response.setFoundGameParticipantCount(gameSessionMaster->getPlayerRoster()->getParticipantCount());
            response.setMaxFitscore(matchInfo.getMaxFitScore());
            response.setFitScore(matchInfo.getFitScore());

            // Update the Ids of any teams that are now set:
            TeamIdToTeamIndexMap::const_iterator curIds = idsToIndexes.begin();
            TeamIdToTeamIndexMap::const_iterator endIds = idsToIndexes.end();
            for (; curIds != endIds; ++curIds)
            {
                TeamId newTeamId = curIds->first;
                TeamIndex foundTeamIndex = curIds->second;

                TeamId gameSessionTeamId = gameSessionMaster->getTeamIdByIndex(foundTeamIndex);
                if (gameSessionTeamId == ANY_TEAM_ID && gameSessionTeamId != newTeamId)
                    gameSessionMaster->changeGameTeamIdMaster(foundTeamIndex, newTeamId);
            }

            externalSessionJoinParamsToOtherResponse(joinResponse.getExternalSessionParams(), response.getExternalSessionParams());


            // submit matchmaking succeeded event
            SuccesfulMatchmakingSession succesfulMatchmakingSession;
            succesfulMatchmakingSession.setMatchmakingSessionId(request.getMatchmakingSessionId());
            succesfulMatchmakingSession.setMatchmakingScenarioId(request.getMatchmakingScenarioId());
            succesfulMatchmakingSession.setUserSessionId(request.getOwnerUserSessionId());
            succesfulMatchmakingSession.setPersonaName(leaderPersonaName);
            succesfulMatchmakingSession.setClientPlatform(leaderClientPlatform);
            succesfulMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(SUCCESS_JOINED_EXISTING_GAME));
            succesfulMatchmakingSession.setFitScore(matchInfo.getFitScore());
            succesfulMatchmakingSession.setMaxPossibleFitScore(matchInfo.getMaxFitScore());
            succesfulMatchmakingSession.setGameId(matchInfo.getGameId());

            gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_SUCCESFULMATCHMAKINGSESSIONEVENT), succesfulMatchmakingSession);
        }
        else
        {

            // remove MM QoS data, since the join failed
            // build out our QoS tracking list
            if (performQosValidation && gameEntryType == GAME_ENTRY_TYPE_DIRECT)
            {
                gameSessionMaster->removeMatchmakingQosData(mmQosList);
            }

            TRACE_LOG("[GameManagerMasterImpl] Matchmaking session(" << request.getMatchmakingSessionId() 
                << ") failed to process join game match for game(" << matchInfo.getGameId() << "), error(" << err << ":" 
                << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
        }

        return MatchmakingFoundGameError::commandErrorFromBlazeError(err);
    }

    /*! ************************************************************************************************/
    /*! \brief Process a list of matching session to games, and attempt to join/create them.

        \param[in] request - the request containing the list of our matches.
        \param[in/out] response - a response object to fill out with any failures
        \param[in] message
    ***************************************************************************************************/
    PackerFoundGameError::Error GameManagerMasterImpl::processPackerFoundGame(const Blaze::GameManager::PackerFoundGameRequest &request,
        Blaze::GameManager::PackerFoundGameResponse &response, const Message* message)
    {
        // Check for existence
        BlazeId ownerBlazeId = gUserSessionManager->getBlazeId(request.getOwnerUserSessionId());
        if (EA_UNLIKELY(ownerBlazeId == INVALID_BLAZE_ID))
        {
            // Bail out early if the user session owning the matchmaking session is no longer available.
            TRACE_LOG("[GameManagerMasterImpl] Finalization job(" << request.getFinalizationJobId() << ") could not find owner user session(" << request.getOwnerUserSessionId() << ")");
            return PackerFoundGameError::GAMEMANAGER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        }

        LogContextOverride logAudit(request.getOwnerUserSessionId());

        JoinGameMasterResponse joinResponse; // hold list of external ids joined for this game
        BlazeRpcError err = GAMEMANAGER_ERR_MATCHMAKING_NO_JOINABLE_GAMES;

        // reset the list of possible joined players.
        clearExternalSessionJoinParams(joinResponse.getExternalSessionParams());

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId(), false); // AutoCommit is off to avoid attempting noop commit when we early out
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMasterImpl] Finalization job(" << request.getFinalizationJobId() << ") matched game(" << request.getGameId() << ") that is no longer around.");
            return PackerFoundGameError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        response.setGameRecordVersion(mGameTable.getRecordVersion(request.getGameId()));

        // Declare the setup reasons map here so it's usable for building the join events
        UserSessionSetupReasonMap setupReasons;
        RelatedPlayerSetsMap joiningPlayersLists;
        TeamIdToTeamIndexMap idsToIndexes;
        const char8_t* leaderPersonaName = "";
        ClientPlatformType leaderClientPlatform = INVALID;

        // always "join by group"
        JoinGameByGroupMasterRequest &joinRequest = const_cast<JoinGameByGroupMasterRequest &>(request.getJoinGameByGroupRequest());
        joinRequest.getJoinRequest().setGameId(request.getGameId());

#if 0 // PACKER_TODO: need QoS in packerFoundGame?
        PlayerIdList mmQosList;
        bool qosListWasEmpty = mmQosList.empty();
        bool performQosValidation = false;
        GameEntryType gameEntryType = joinRequest.getJoinRequest().getPlayerJoinData().getGameEntryType();
#endif

        // Add the non-optional guys:
        uint16_t nonOptionalUserCount = 0;
        UserJoinInfoList::const_iterator iter = joinRequest.getUsersInfo().begin();
        UserJoinInfoList::const_iterator end = joinRequest.getUsersInfo().end();
        for (; iter != end; ++iter)
        {
#if 0 // PACKER_TODO: need QoS in packerFoundGame?
            // All non-optional members of the group join should have this set the same:
            if ((*iter)->getPerformQosValidation())
            {
                performQosValidation = true;
                if (qosListWasEmpty)
                {
                    mmQosList.push_back((*iter)->getUser().getUserInfo().getId());
                }
            }
#endif
            if (!(*iter)->getIsOptional())
            {
#if 0 // PACKER_TODO: need QoS in packerFoundGame?
                if (qosListWasEmpty)
                {
                    mmQosList.push_back((*iter)->getUser().getUserInfo().getId());
                }
#endif
                ++nonOptionalUserCount;
            }
        }

        TeamIdRoleSizeMap requiredTeamRoles;
        //if (requiredTeamRoles.empty())
        {
            buildTeamIdRoleSizeMapFromJoinRequest(joinRequest.getJoinRequest(), requiredTeamRoles, nonOptionalUserCount);
        }

        // Normal single-team matchmaking: 
        // Team UED rules only apply if we're a single team join (too complex otherwise).
        if (requiredTeamRoles.size() == 1)
        {
            // Matchmaking requests only require a user to provide a team id since we do not know which index
            // that team may exist at for any given game we match.
            // Determine the index of our requested team id in this particular game.
            const Matchmaker::GroupUedExpressionList* groupUedExpressionList = nullptr;
            if (request.getFoundTeam().getTeamUEDKey() != INVALID_USER_EXTENDED_DATA_KEY)
            {
                groupUedExpressionList = getGroupUedExpressionList(request.getFoundTeam().getTeamUEDRuleName());
            }

            TeamId myTeamId = requiredTeamRoles.begin()->first;
            TeamIndex foundTeamIndex = UNSPECIFIED_TEAM_INDEX;
            err = gameSessionMaster->findOpenTeam(request.getFoundTeam(), requiredTeamRoles.begin(), groupUedExpressionList, foundTeamIndex);
            if (err != ERR_OK)
            {
                TRACE_LOG("[GameManagerMasterImpl] Finalization job(" << request.getFinalizationJobId()
                    << ") failed to find open team([" << foundTeamIndex << "]:" << myTeamId << ") in game("
                    << request.getGameId() << "), error(" << err << ":" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
                return PackerFoundGameError::commandErrorFromBlazeError(err);
            }
            idsToIndexes[myTeamId] = foundTeamIndex;
        }
        else
        {
            // If multiple teams are in the MM request, we want to just check that the team members can fit in the teams they requested.
            err = gameSessionMaster->findOpenTeams(requiredTeamRoles, idsToIndexes);
            if (err != ERR_OK)
            {
                TRACE_LOG("[GameManagerMasterImpl] Finalization job(" << request.getFinalizationJobId()
                    << ") failed to find open teams(" << requiredTeamRoles.size() << " teams required) in game("
                    << request.getGameId() << "), error(" << err << ":" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
                return PackerFoundGameError::commandErrorFromBlazeError(err);
            }
        }

        const UserSessionInfo* hostSessionInfo = getHostSessionInfo(joinRequest.getUsersInfo());
        if (hostSessionInfo != nullptr)
        {
            leaderPersonaName = hostSessionInfo->getUserInfo().getPersonaName();
            leaderClientPlatform = hostSessionInfo->getUserInfo().getPlatformInfo().getClientPlatform();
        }

        if (request.getUserSessionGameSetupReasonMap().empty())
        {
            return PackerFoundGameError::commandErrorFromBlazeError(ERR_SYSTEM);
        }
        for (auto& reasonByUserSessionItr : request.getUserSessionGameSetupReasonMap())
        {
            setupReasons.insert(reasonByUserSessionItr);
        }

        buildRelatedPlayersMap(gameSessionMaster.get(), joinRequest.getUsersInfo(), joiningPlayersLists);

#if 0 // PACKER_TODO: need QoS in packerFoundGame?
        // set up MM QoS data as needed
        // build out our QoS tracking list
        if (performQosValidation && gameEntryType == GAME_ENTRY_TYPE_DIRECT)
        {
            gameSessionMaster->addMatchmakingQosData(mmQosList, request.getMatchmakingSessionId());
        }
#endif

        Blaze::EntryCriteriaError error;

        err = (BlazeRpcError)gameSessionMaster->joinGameByGroup(joinRequest, joinResponse, error, setupReasons);
        if (err == ERR_OK)
        {
            // need the setup reasons, the PIN submission goes back to the packer
            // NOTE: tracking tag intentionally omitted here, filled in on the packer master which actually submits the MPMatchJoinEvent event
            // associated with each scenario.
            buildMatchJoinEvent(*gameSessionMaster, setupReasons, joiningPlayersLists, response.getPinSubmission(), nullptr);
        }

        // We joined this game.
        if (err == ERR_OK)
        {
            // Now we enable release to auto-commit to storage
            gameSessionMaster->setAutoCommitOnRelease();

            response.setJoinedGameId(request.getGameId());

            // Update the Ids of any teams that are now set:
            TeamIdToTeamIndexMap::const_iterator curIds = idsToIndexes.begin();
            TeamIdToTeamIndexMap::const_iterator endIds = idsToIndexes.end();
            for (; curIds != endIds; ++curIds)
            {
                TeamId newTeamId = curIds->first;
                TeamIndex foundTeamIndex = curIds->second;

                TeamId gameSessionTeamId = gameSessionMaster->getTeamIdByIndex(foundTeamIndex);
                if (gameSessionTeamId == ANY_TEAM_ID && gameSessionTeamId != newTeamId)
                    gameSessionMaster->changeGameTeamIdMaster(foundTeamIndex, newTeamId);
            }

            externalSessionJoinParamsToOtherResponse(joinResponse.getExternalSessionParams(), response.getExternalSessionParams());
        }
        else
        {
#if 0 // PACKER_TODO: need QoS in packerFoundGame?
            // remove MM QoS data, since the join failed
            // build out our QoS tracking list
            if (performQosValidation && gameEntryType == GAME_ENTRY_TYPE_DIRECT)
            {
                gameSessionMaster->removeMatchmakingQosData(mmQosList);
            }
#endif
            TRACE_LOG("[GameManagerMasterImpl] Finalization job(" << request.getFinalizationJobId()
                << ") failed to process join game match for game(" << request.getGameId() << "), error(" << err << ":"
                << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
        }

        if (err != ERR_OK && request.getGameRecordVersion() == response.getGameRecordVersion())
        {
            ERR_LOG("[GameManagerMasterImpl].processPackerFoundGame: Finalization job(" << request.getFinalizationJobId() << ") members unexpectedly failed to enter game(" 
                << request.getGameId() << "), err(" << ErrorHelp::getErrorName(err) << ") despite matching game version(" << response.getGameRecordVersion() 
                << "), request -> " << request);

            // PACKER_TODO: Need to validate all user sessions at the beginning of this command because we do not want to signal a non-resumable error to the packer slave in case one of the members of the request logged out while this request was on the wire.
            err = GAMEMANAGER_ERR_UNEXPECTED_JOIN_FAILURE_GAME_VERSION;
        }
        
        return PackerFoundGameError::commandErrorFromBlazeError(err);
    }

    MatchmakingCreateGameWithPrivilegedIdError::Error GameManagerMasterImpl::processMatchmakingCreateGameWithPrivilegedId(const Blaze::GameManager::MatchmakingCreateGameRequest &request, Blaze::GameManager::CreateGameMasterResponse &response, const Message* message)
    {
        // we just call processMatchmakingCreateGame, this allows some requests to be sharded while standard ones are not.
        BlazeRpcError err = (BlazeRpcError)processMatchmakingCreateGame(request, response, message);
        return MatchmakingCreateGameWithPrivilegedIdError::commandErrorFromBlazeError(err);
    }

    BlazeRpcError GameManagerMasterImpl::populateRequestCrossplaySettings(CreateGameMasterRequest& masterRequest)
    {
        bool isCrossplayEnabled = false;
        BlazeRpcError err = ValidationUtils::validateRequestCrossplaySettings(isCrossplayEnabled, masterRequest.getUsersInfo(), 
            true, masterRequest.getCreateRequest().getGameCreationData(), getConfig().getGameSession(),
            masterRequest.getCreateRequest().getClientPlatformListOverride(), false);

        // Set the Crossplay setting to the correct value:  (We only disable Crossplay, never enable by default)
        // validation method above has set/corrected the proper client platform list for this request.
        masterRequest.setIsCrossplayEnabled(isCrossplayEnabled);

        if (err != ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].populateRequestCrossplaySettings:  Crossplay enabled user attempted to create a game with invalid settings.  Unable to just disable crossplay to fix this.");
            return err;
        }


 
        return err;
    }

    MatchmakingCreateGameError::Error GameManagerMasterImpl::processMatchmakingCreateGame(const Blaze::GameManager::MatchmakingCreateGameRequest &request, Blaze::GameManager::CreateGameMasterResponse &response, const Message* message)
    {
        BlazeRpcError blazeErr = populateRequestCrossplaySettings(const_cast<CreateGameMasterRequest&>(request.getCreateReq()));
        if (blazeErr != Blaze::ERR_OK)
        {
            return MatchmakingCreateGameError::commandErrorFromBlazeError(blazeErr);
        }

        UserSessionId hostSessionId = getHostUserSessionId(request.getCreateReq().getUsersInfo());
        TRACE_LOG("[GameManagerMasterImpl]::processMatchmakingCreateGame Matchmaking session owned by user session (" << hostSessionId << ") attempting to create game.");
        

        // convert setup reasons to hashmap
        auto& setupReasonMap = request.getUserSessionGameSetupReasonMap();
        
        if (!isDedicatedHostedTopology(request.getCreateReq().getCreateRequest().getGameCreationData().getNetworkTopology()))
        {
            BlazeRpcError createErr;
            createErr = (BlazeRpcError)createGame(request.getCreateReq(), response, setupReasonMap);

            if (createErr == ERR_OK)
            {
                
                const GameSessionMaster* newGame = getReadOnlyGameSession(response.getCreateResponse().getGameId());
                if (newGame != nullptr)
                {
                    RelatedPlayerSetsMap joiningPlayersLists;
                    buildRelatedPlayersMap(newGame, request.getCreateReq().getUsersInfo(), joiningPlayersLists);
                    buildMatchJoinEvent(*newGame, setupReasonMap, joiningPlayersLists, response.getPinSubmission(), request.getTrackingTag());
                    // put the events in the response to the MM to finish out
                    // we do it on the MM because we might have to QoS validate or something else before MM is 'done' and the join is complete
                }
            }
            return MatchmakingCreateGameError::commandErrorFromBlazeError(createErr);
        }
        else
        {
            // dedicated server, this is a reset            
            if (!gUserSessionManager->getSessionExists(hostSessionId))
            {
                // perhaps add a specific error code for this case
                return MatchmakingCreateGameError::ERR_SYSTEM;
            }

            GameSessionMasterPtr resetTarget = nullptr;
            if (request.getPrivilegedGameIdPreference() != GameManager::INVALID_GAME_ID)
            {
                GameSessionMasterPtr desiredGame = getWritableGameSession(request.getPrivilegedGameIdPreference());

                if (desiredGame != nullptr)
                {
                    if (desiredGame->hasDedicatedServerHost() && desiredGame->isResetable() && desiredGame->isGameCreationFinalized())
                    {
                        TRACE_LOG("[GameManagerMasterImpl]::processMatchmakingCreateGame Matchmaking session owned by user session " << hostSessionId << " - create game matchmaking session matched into preferred game " << request.getPrivilegedGameIdPreference() << ".");
                        resetTarget = desiredGame;
                    }
                }

                // If we specified a privileged id, we should get it. 
                if (request.getRequirePrivilegedGameId() && resetTarget == nullptr)
                {
                    TRACE_LOG("[GameManagerMasterImpl]::processMatchmakingCreateGame Unable to get the game id specified in request.  Session owned by user session " << hostSessionId << " - create game matchmaking session matched into preferred game " << request.getPrivilegedGameIdPreference() << ".");
                    return MatchmakingCreateGameError::ERR_SYSTEM;
                }
            }

            BlazeRpcError resetErr;
            JoinGameMasterResponse joinResponse;
            resetErr = (BlazeRpcError)resetDedicatedServer(request.getCreateReq(), setupReasonMap, resetTarget, joinResponse);
            if ((resetErr == ERR_OK) && (resetTarget != nullptr))
            {
                response.getCreateResponse().setGameId(resetTarget->getGameId());
                // transfer the joined external ids to the create response.
                // Pre: joins above have set all required external session params here. Add to rsp
                externalSessionJoinParamsToOtherResponse(joinResponse.getExternalSessionParams(), response.getExternalSessionParams());
                joinedReservedPlayerIdentificationsToOtherResponse(joinResponse.getJoinResponse(), response.getCreateResponse());

                RelatedPlayerSetsMap joiningPlayersLists;
                buildRelatedPlayersMap(resetTarget.get(), request.getCreateReq().getUsersInfo(), joiningPlayersLists);
                // put the events in the response to the MM to finish out
                buildMatchJoinEvent(*resetTarget, setupReasonMap, joiningPlayersLists, response.getPinSubmission(), request.getTrackingTag());
            }

            return MatchmakingCreateGameError::commandErrorFromBlazeError(resetErr);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Ejects the calling user as the host of the virtual game.
    
        \param[in] request - the ejection request.
        \return rpc error
    ***************************************************************************************************/
    EjectHostMasterError::Error GameManagerMasterImpl::processEjectHostMaster(const Blaze::GameManager::EjectHostRequest &request, const Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMaster] Attempt to eject host on game(" << request.getGameId() <<
                ") that doesn't exist. ");
            return EjectHostMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        if (EA_UNLIKELY(!gameSessionMaster->hasDedicatedServerHost()))
        {
            TRACE_LOG("[GameManagerMaster] Attempt to eject host on game(" << request.getGameId() <<
                ") which is not a dedicated server topology.");
            return EjectHostMasterError::GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION;
        }

        if (EA_UNLIKELY(gameSessionMaster->getDedicatedServerHostSessionId() != UserSession::getCurrentUserSessionId()))
        {
            TRACE_LOG("[GameManagerMaster] Attempt to eject host on game(" << request.getGameId() << 
                ") by user session(" << UserSession::getCurrentUserSessionId() << ") who is not the topology host.");
            return EjectHostMasterError::GAMEMANAGER_ERR_NOT_TOPOLOGY_HOST;
        }

        gameSessionMaster->ejectHost(UserSession::getCurrentUserSessionId(), UserSession::getCurrentUserBlazeId(), request.getReplaceHost());

        return EjectHostMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Fills out a UserSessionSetupReasonMap with the provided context for the game creator.  
        Loops over the members and adds their context as well.

        \param[in/out] setupReasons - the map of setup reasons to fill out
        \param[in] userInfos - list of user session info's of all joining users
        \param[in] creatorContext - the reason why the creator is knowing about this game
        \param[in] pgContext - the reason why the normal members are knowing about this game.
        \param[in] optContext - the reason why the optional members are knowing about this game.
    ***************************************************************************************************/
    void GameManagerMasterImpl::fillUserSessionSetupReasonMap(UserSessionSetupReasonMap& setupReasons, 
        const UserJoinInfoList& userInfos, GameSetupReason* creatorContext, GameSetupReason* pgContext, GameSetupReason* optContext) const  
    {
        // Populate all the users' join game reasons.
        for (UserJoinInfoList::const_iterator iter = userInfos.begin(), end = userInfos.end(); iter != end; ++iter)
        {
            GameSetupReason* contextToUse = nullptr;
            if ((*iter)->getIsHost())
            {
                contextToUse = creatorContext;
            }
            else if (!(*iter)->getIsOptional())
            {
                contextToUse = pgContext;
            }
            else
            {
                contextToUse = optContext;
            }
            
            if (contextToUse != nullptr)
            {
                setupReasons[(*iter)->getUser().getSessionId()] = contextToUse;
            }
        }
    }

   /*! **********************************************************************************************/
    /*! \brief Process DEMANGLER Connection Healthcheck-increase Count
     ***************************************************************************************************/
    void GameManagerMasterImpl::incrementDemanglerHealthCheckCounter(const GameDataMaster::ClientConnectionDetails& connDetails,
        DemanglerConnectionHealthcheck check, GameNetworkTopology topology, const char8_t* pingSite)
    {
        if (check != CONNECTION_WITHOUT)
        {
            // by spec, to see non-impacting successes that went on to *fail* QoS, only those get the 'QOS_RULE' prefix
            // (impacting successes that went on to *pass* QoS, are simply lumped with successes without the 'QOS_RULE')
            if (connDetails.getQosEnabled() && (!connDetails.getWasConnected() || !connDetails.getQosValidationPassed()))
                mMetrics.mQosRuleDemangler.increment(1, topology, pingSite, check);
            else
                mMetrics.mDemangler.increment(1, topology, pingSite, check);
        }
    }

    /*! **********************************************************************************************/
    /*! \brief update client connection result metrics
    ***************************************************************************************************/
    void GameManagerMasterImpl::incrementConnectionResultMetrics(const GameDataMaster::ClientConnectionDetails& connDetails,
        const GameSessionMaster& game, const char8_t* pingSite, PlayerRemovedReason updateReason)
    {
        ConnSuccessMetricEnum connSuccess = FAILED;
        if (connDetails.getWasConnected())
        {
            // If we managed to get fully connected, then things were successful
            connSuccess = SUCCESS;
        }
        else
        {
            if ((updateReason != PLAYER_CONN_LOST) && (updateReason != PLAYER_JOIN_TIMEOUT))
            {
                if (game.getGameType() == GAME_TYPE_GROUP)
                {
                    // For GameGroups, we also check the conn status, since leaving the group early will hit this code before the mesh is disconnected,
                    // but the reason will always be PLAYER_LEFT (for non-CCS fallback groups) because Voip failure does not trigger a game exit.
                    if (connDetails.getStatus() == ESTABLISHING_CONNECTION)
                        connSuccess = INCOMPLETE;
                }
                else
                {
                    // We're here because we were added to a network mesh, but didn't connect to an the target endpoint, and we were
                    // removed from the mesh for some reason other than PLAYER_CONN_LOST || PLAYER_JOIN_TIMEOUT.  All other reasons
                    // indicate a non-failure based removal.  So therefore, the connection didn't succeed or fail, but rather it
                    // did not get a chance to complete.
                    connSuccess = INCOMPLETE;
                }
            }
        }

        TRACE_LOG("[GameManagerMasterImpl].incrementConnectionResultMetrics: incrementing for connection(" << connDetails.getSourceConnectionGroupId() << " -> " << connDetails.getTargetConnectionGroupId()
            << ") in game(" << game.getGameId() << "), '" << (connDetails.getConnectionGame() ? "GAME_CONN" : "VOIP_CONN")
            << "', '" << ConnectionJoinTypeToString(connDetails.getJoinType())
            << "', '" << pingSite
            << "', '" << ConnSuccessMetricEnumToString(connSuccess)
            << "', '" << ConnConciergeModeMetricEnumToString(connDetails.getCCSInfo().getConnConciergeMode())
            << "', demangled '" << (connDetails.getFlags().getConnectionDemangled() ? "true" : "false")
            << "', impacting '" << (game.getPlayerConnectionMesh()->isConnectionPlayerImpacting(connDetails) ? "true" : "false")
            << "', leveragesCcs '" << (game.doesLeverageCCS() ? "true" : "false")
            << "'.");

        ConnImpactingMetricEnum impacting = game.getPlayerConnectionMesh()->isConnectionPlayerImpacting(connDetails) || connSuccess == INCOMPLETE ? IMPACTING : NONIMPACTING;
        if (connDetails.getConnectionGame())
        {
            mMetrics.mGameConns.increment(1
                , connDetails.getJoinType()
                , pingSite
                , connSuccess
                , game.getGameNetworkTopology()
                , impacting
                , connDetails.getCCSInfo().getConnConciergeMode()
                , connDetails.getFlags().getConnectionDemangled() ? WITH_DEMANGLER : WITHOUT_DEMANGLER
                , game.doesLeverageCCS() ? CC_ELIGIBLE : CC_INELIGIBLE
            );
        }
        else
        {
            mMetrics.mVoipConns.increment(1
                , connDetails.getJoinType()
                , pingSite
                , connSuccess
                , game.getVoipNetwork()
                , impacting
                , connDetails.getCCSInfo().getConnConciergeMode()
                , connDetails.getFlags().getConnectionDemangled() ? WITH_DEMANGLER : WITHOUT_DEMANGLER
                , game.doesLeverageCCS() ? CC_ELIGIBLE : CC_INELIGIBLE
            );
        }
    }

    /*
    returns true if the calling Blaze component has super user privilege for the given game session
        or if the UserSession::getCurrentUserBlazeId() is an admin player
    */
    bool GameManagerMasterImpl::checkPermission(const GameSessionMaster& gameSession, GameAction::Actions action, BlazeId blazeId /*= INVALID_BLAZE_ID*/) const
    {
        if (UserSession::hasSuperUserPrivilege() && gameSession.getGameSettings().getVirtualized())
        {
            TRACE_LOG("[GameManagerMaster] superUserPrivilege used to modify virtual game id (" << gameSession.getGameId() << ").");
            return true;
        }
        return gameSession.hasPermission( ((blazeId != INVALID_BLAZE_ID) ? blazeId : UserSession::getCurrentUserBlazeId()), action);
    }

    GetGameSessionCountError::Error GameManagerMasterImpl::processGetGameSessionCount(Blaze::GameManager::GetGameSessionCountResponse &response, const Blaze::Message *message)
    {
        // This function is DEPRECATED and unused in Blaze.  The value returned is the total Active Games count, combining all platforms. 
        Metrics::TagPairList baseQuery;

        // sending number of active sessions here to avoid count of inactive dedicated server games in total number of games
        response.setGameSessionCount(getNumberOfActiveGameSessions(baseQuery));
        // this counts players in multiple sessions as 1 player for each game that they're in.
        // this is appropriate since 1 player in 2 games is the same as 2 players in 1 game each from a load perspective
        response.setPlayerSessionCount((uint32_t)mMetrics.mActivePlayers.get());
        response.setInstanceId(getLocalInstanceId());
        

        return GetGameSessionCountError::ERR_OK;
    }

    bool GameManagerMasterImpl::isDrainComplete()
    { 
        Metrics::TagPairList baseQuery;
        return (getNumberOfActiveGameSessions(baseQuery) == 0 && !Fiber::isFiberGroupIdInUse(mAuditUserDataGroupId) && !mGameReportingRpcJobQueue.hasPendingWork());
    }


    ChooseHostForInjectionError::Error GameManagerMasterImpl::processChooseHostForInjection(const Blaze::GameManager::ChooseHostForInjectionRequest &request, Blaze::GameManager::ChooseHostForInjectionResponse &response, const Blaze::Message *message)
    {
        // first, find the requesting user's session        
        if (!gUserSessionManager->getSessionExists(request.getJoinerSessionInfo().getSessionId()))
        {
            return ChooseHostForInjectionError::ERR_SYSTEM;
        }

        GameSessionMasterPtr resettableGame = chooseAvailableDedicatedServer(request.getGameIdToReset());

        if (resettableGame == nullptr)
        {
            return ChooseHostForInjectionError::GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
        }

        TRACE_LOG("[GameSessionMaster] Selected the host of game (" << resettableGame->getGameId() << ") for remote master host injection.");


        resettableGame->getTopologyHostUserInfo().copyInto(response.getHostInfo());
        resettableGame->getReplicatedGameSession().getDedicatedServerHostNetworkAddressList().copyInto(response.getHostNetworkAddressList());
        response.setHostPreviousGameId(resettableGame->getGameId());

        destroyGame(resettableGame.get(), HOST_INJECTION);

        return ChooseHostForInjectionError::ERR_OK;
    }

    void GameManagerMasterImpl::addHostInjectionJob(const GameSessionMaster &virtualizedGameSession, const JoinGameMasterRequest& joinGameRequest)
    {
        HostInjectionJobMap::const_iterator jobIter = mHostInjectionJobMap.find(virtualizedGameSession.getGameId());
        if (jobIter == mHostInjectionJobMap.end())
        {
            TRACE_LOG("GameManagerMasterImpl::addHostInjectionJob - adding job for gameId (" << virtualizedGameSession.getGameId() << ").");
            ChooseHostForInjectionRequest request;
            joinGameRequest.getUserJoinInfo().getUser().copyInto(request.getJoinerSessionInfo());
            request.setNetworkTopology(virtualizedGameSession.getGameNetworkTopology());

            // if initialized with match any, use the joining user's GPVS for finding the ded server
            if (blaze_stricmp(virtualizedGameSession.getGameProtocolVersionString(), GameManager::GAME_PROTOCOL_VERSION_MATCH_ANY) == 0)
            {
                request.setGameProtocolVersionString(joinGameRequest.getJoinRequest().getCommonGameData().getGameProtocolVersionString());
            }
            else
            {
                request.setGameProtocolVersionString(virtualizedGameSession.getGameProtocolVersionString());
            }

            request.getGamePingSiteAliasList().push_back(virtualizedGameSession.getBestPingSiteAlias());
            request.setRequiredPlayerCapacity(virtualizedGameSession.getMaxPlayerCapacity());

            HostInjectionJob *job = 
                BLAZE_NEW HostInjectionJob(virtualizedGameSession.getGameId(), request);
            mHostInjectionJobMap.insert( eastl::make_pair(virtualizedGameSession.getGameId(), job) );

            job->start();
        }
        else
        {
            // this is fine, we can let the join continue and all users will get the game setup notification once the host is found
           TRACE_LOG("GameManagerMasterImpl::addHostInjectionJob - job already exists for gameId " << virtualizedGameSession.getGameId() << "!");
        }

    }

    void GameManagerMasterImpl::removeHostInjectionJob(const GameId &gameId)
    {
        HostInjectionJobMap::iterator jobIter = mHostInjectionJobMap.find(gameId);
        if (jobIter != mHostInjectionJobMap.end())
        {
            TRACE_LOG("GameManagerMasterImpl::removeHostInjectionJob - removing job for gameId (" << gameId << ").");
            HostInjectionJob* hostinjectJob = jobIter->second;
            mHostInjectionJobMap.erase(jobIter);

            // We delete after erasing, since removeHostInjectionJob may be called by the Job's destructor, which cancels the active Fiber. 
            delete hostinjectJob; 
        }
    }

    void GameManagerMasterImpl::removePersistedGameIdMapping(GameSessionMaster& gameSession, bool waitForRemoval /*= false*/)
    {
        if (gameSession.hasPersistedGameId())
        {
            // IMPORTANT: must make a copy of persistent id, we clear the game out below
            PersistedGameId persistedId = gameSession.getPersistedGameId();
            GameIdByPersistedIdMap::iterator gameSessionMapIter = mGameIdByPersistedIdMap.find(persistedId);
            if (gameSessionMapIter != mGameIdByPersistedIdMap.end())
            {
                if (gameSessionMapIter->second == gameSession.getGameId())
                {
                    mGameIdByPersistedIdMap.erase(gameSessionMapIter);

                    // NOTE: Pin gameSession from migrating until its persistent id has been removed from redis to avoid race condition where another instance could try to take the persistent id which still has an erasure pending
                    // IMPORTANT: (we currently cannot use a FiberJobQueue to process these because we'd need to ensure that all persisted id key redis inserts area also funneled through the same queue
                    // and we'd also need to add support for sharding incoming jobs into sub-queues within the FiberJobQueue to ensure that jobs for the same persisted id always get queued in order!)
                    Fiber::CreateParams params;
                    params.blocksCaller = waitForRemoval;
                    gSelector->scheduleFiberCall(this, &GameManagerMasterImpl::cleanupPersistedIdFromRedis, persistedId, GameSessionMasterPtr(&gameSession), "GameManagerMasterImpl::cleanupPersistedIdFromRedis", params);
                }
                else
                {
                    // something went wrong this should never happen!
                    WARN_LOG("[GameManagerMasterImpl].removePersistedGameIdMapping Tried to remove mapping of persisted id to game id, but the recorded game id didn't match the provided game id " 
                        << persistedId << ") of game(" << gameSession.getGameId() << ") was recorded as being associated with game id (" 
                    << gameSessionMapIter->second << ").");
                }
            }
            else
            {
                // should have been in the list.
                WARN_LOG("[GameManagerMasterImpl].removePersistedGameIdMapping Tried to remove mapping of persisted id to game id, but persisted game id " 
                    << persistedId << ") of game(" << gameSession.getGameId() << ") is not in the persisted game id map!");
            }
        }

        gameSession.clearPersistedGameIdAndSecret();
    }

    HasGameMappedByPersistedIdMasterError::Error GameManagerMasterImpl::processHasGameMappedByPersistedIdMaster(const Blaze::GameManager::HasGameMappedByPersistedIdMasterRequest &request, 
        Blaze::GameManager::HasGameMappedByPersistedIdMasterResponse &response, const Message* message)
    {
        GameIdByPersistedIdMap::iterator gameSessionMapIter = mGameIdByPersistedIdMap.find(request.getPersistedGameId());
        if (gameSessionMapIter != mGameIdByPersistedIdMap.end())
        {
            response.setGameId(gameSessionMapIter->second);
        }

        return HasGameMappedByPersistedIdMasterError::ERR_OK;
    }

    ReservePersistedIdMasterError::Error GameManagerMasterImpl::processReservePersistedIdMaster(const Blaze::GameManager::ReservePersistedIdMasterRequest &request, Blaze::GameManager::CreateGameMasterErrorInfo &errorResp, const Message* message)
    {
        BlazeRpcError err = ERR_OK;
        PersistedGameId persistedId = request.getPersistedGameId();
        SliverId sliverId = request.getPersistedIdOwnerSliverId();

        // lock it if not in progress
        FiberMutexByPersistedIdMap::insert_return_type insertRet = mFiberMutexByPersistedIdMap.insert(persistedId.c_str());
        if (insertRet.second)
            insertRet.first->second = BLAZE_NEW FiberMutex();
        FiberMutex& fiberMutex = *insertRet.first->second;

        err = fiberMutex.lock();
        if (err == ERR_OK)
        {
            // This will yield back to the signaling fiber so that it can carry on.
            Fiber::yield();

            // find the current owner in redis
            // check if the provided persisted game id exists in redis
            SliverId existingId = INVALID_SLIVER_ID;
            RedisError rc = mPersistedIdOwnerMap.find(persistedId, existingId);
            if (rc == REDIS_ERR_OK)
            {
                if (existingId != INVALID_SLIVER_ID)
                {
                    // We now found the persisted id exists in redis with a valid sliverId, now we need to check with the sliver manager to make sure that sliver actually owns it
                    RpcCallOptions opts;
                    opts.routeTo.setSliverIdentityFromSliverId(GameManager::GameManagerMaster::COMPONENT_ID, existingId);
                    HasGameMappedByPersistedIdMasterRequest hasGameMappedByPersistedIdRequest;
                    hasGameMappedByPersistedIdRequest.setPersistedGameId(persistedId);

                    HasGameMappedByPersistedIdMasterResponse hasGameMappedByPersistedIdResponse;
                    err = hasGameMappedByPersistedIdMaster(hasGameMappedByPersistedIdRequest, hasGameMappedByPersistedIdResponse, opts);

                    if (err == ERR_OK)
                    {
                        GameId foundGameId = hasGameMappedByPersistedIdResponse.getGameId();
                        if (foundGameId != INVALID_GAME_ID)
                        {
                            // Return the game id so the caller *can* do a joinGame() call if desired.
                            errorResp.setPreExistingGameIdWithPersistedId(foundGameId);
                            err = GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE; // the persisted id is in use
                        }
                        else
                        {
                            // if the sliver does not actually own this id, erase the entry from redis. We will reserve this persisted id
                            rc = mPersistedIdOwnerMap.erase(persistedId);
                            if (rc != REDIS_ERR_OK)
                            {
                                WARN_LOG("[GameManagerMasterImpl].processReservePersistedIdMaster : failed to erase persistedId (" << persistedId << "). Redis error: " << RedisErrorHelper::getName(rc));
                                err = ERR_SYSTEM;
                            }
                        }
                    }
                    else
                    {
                        WARN_LOG("[GameManagerMasterImpl].processReservePersistedIdMaster: Failed to check game mapped by persisted game id '" << persistedId << 
                            "' on sliver id=" << existingId << " with error " << ErrorHelp::getErrorName(err));
                        err = ERR_SYSTEM;
                    }
                }
                else
                {
                    // unlock
                    rc = mPersistedIdOwnerMap.erase(persistedId);
                    if (rc != REDIS_ERR_OK)
                    {
                        WARN_LOG("[GameManagerMasterImpl].processReservePersistedIdMaster : failed to erase persistedId (" << persistedId << "). Redis error: " << RedisErrorHelper::getName(rc));
                        err = ERR_SYSTEM;
                    }
                }
            }
            else if (rc != REDIS_ERR_NOT_FOUND)
            {
                WARN_LOG("[GameManagerMasterImpl].processReservePersistedIdMaster : failed to perform find operation on persistedId (" << persistedId << "). Redis error: " << RedisErrorHelper::getName(rc));
                err = ERR_SYSTEM;
            }

            if (err == ERR_OK)
            {
                // reserve the id in redis
                // insert this entry with the current sliver id,  continue
                rc = mPersistedIdOwnerMap.insert(persistedId, sliverId);
                if (rc != REDIS_ERR_OK)
                {
                    WARN_LOG("[GameManagerMasterImpl].processReservePersistedIdMaster : failed to insert persistedId (" << persistedId << "). Redis error: " << RedisErrorHelper::getName(rc));
                    err = ERR_SYSTEM;
                }
            }

            fiberMutex.unlock();
        }
        else
        {
            err = ERR_SYSTEM;
        }

        if (!fiberMutex.isInUse())
        {
            delete insertRet.first->second;
            mFiberMutexByPersistedIdMap.erase(persistedId.c_str());
        }

        return ReservePersistedIdMasterError::commandErrorFromBlazeError(err);
    }

    ProcessTelemetryReportError::Error GameManagerMasterImpl::processProcessTelemetryReport(const Blaze::GameManager::ProcessTelemetryReportRequest &request, const Message* message)
    {
        BlazeRpcError rc = ERR_OK;
        const TelemetryReportRequest& report = request.getReport();
        GameId gameId = report.getGameId();
        if (gameId == INVALID_GAME_ID)
        {
            TRACE_LOG("[GameManagerMasterImpl].processProcessTelemetryReport: Game id " << gameId << " not found.");
            return ProcessTelemetryReportError::commandErrorFromBlazeError(GAMEMANAGER_ERR_INVALID_GAME_ID);
        }

        if (report.getTelemetryReports().empty())
        {
            TRACE_LOG("[GameManagerMasterImpl].processProcessTelemetryReport: Telemetry Report list is empty.");
            return ProcessTelemetryReportError::commandErrorFromBlazeError(ERR_OK);
        }

        const GameSessionMaster* game = getReadOnlyGameSession(gameId);
        if (EA_UNLIKELY(game == nullptr))
        {
            TRACE_LOG("[GameManagerMasterImpl].processProcessTelemetryReport: Game id " << gameId << " not found.");
            return ProcessTelemetryReportError::commandErrorFromBlazeError(GAMEMANAGER_ERR_INVALID_GAME_ID);
        }

        // Only iterate player roster when not having dedicated server host, 
        // since for dedicated server topologies, the host will NOT be in the player roster.
        bool callerInGame = false;
        PingSiteAlias pingSite = "";
        ConnectionGroupId localConnGroupId = report.getLocalConnGroupId();
        if(game->hasDedicatedServerHost())
        {
            ConnectionGroupId groupId = game->getDedicatedServerHostInfo().getConnectionGroupId();
            if(groupId == localConnGroupId)
            {
                callerInGame = true;
                pingSite = game->getBestPingSiteAlias();
            }
        }
        
        if (!callerInGame)
        {
            const PlayerRoster::PlayerInfoList players = game->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
            PlayerRoster::PlayerInfoList::const_iterator playerIter = players.begin();
            PlayerRoster::PlayerInfoList::const_iterator playerEnd = players.end();
            for (; playerIter != playerEnd; ++playerIter)
            {
                PlayerInfo *info = *playerIter;
                ConnectionGroupId playerConnGroupId = info->getConnectionGroupId();
                if (playerConnGroupId == localConnGroupId)
                {
                    callerInGame = true;
                    pingSite = info->getUserInfo().getBestPingSiteAlias();
                    break;
                }
            }
        }

        if (!callerInGame)
        {
            TRACE_LOG("[GameManagerMasterImpl].processProcessTelemetryReport: Local player is not in game.");
            return ProcessTelemetryReportError::commandErrorFromBlazeError(ERR_SYSTEM);
        }

        TelemetryReportRequest::TelemetryReportList::const_iterator iter = report.getTelemetryReports().begin();
        TelemetryReportRequest::TelemetryReportList::const_iterator end = report.getTelemetryReports().end();
        for (; iter != end; ++iter)
        {
            rc = processTelemetryReport(report.getLocalConnGroupId(), report.getGameId(), **iter, pingSite);
            if (rc != ERR_OK)
                break;
        }

        return ProcessTelemetryReportError::commandErrorFromBlazeError(rc);
    }

    void GameManagerMasterImpl::buildTeamIdRoleSizeMapFromJoinRequest(const JoinGameRequest &request, TeamIdRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount) const
    {
        if (request.getPlayerJoinData().getPlayerDataList().empty())
        {
            ERR_LOG("[GameManagerMasterImpl.buildRoleSizeMapFromJoinRequest] Trying to build role size map to join game(" << request.getGameId() 
                << ") with empty role roster!");
            return;
        }
        buildTeamIdRoleSizeMap(request.getPlayerJoinData(), teamRoleRequirements, joiningPlayerCount, false);

    }
    void GameManagerMasterImpl::buildTeamIndexRoleSizeMapFromJoinRequest(const JoinGameRequest &request, TeamIndexRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount) const
    {
        if (request.getPlayerJoinData().getPlayerDataList().empty())
        {
            ERR_LOG("[GameManagerMasterImpl.buildRoleSizeMapFromJoinRequest] Trying to build role size map to join game(" << request.getGameId() 
                << ") with empty role roster!");
            return;
        }
        buildTeamIndexRoleSizeMap(request.getPlayerJoinData(), teamRoleRequirements, joiningPlayerCount, false);

    }

    BlazeRpcError GameManagerMasterImpl::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) 
    {
        GameId gameId = (GameId)bobjId.id;
        GameSessionMaster* game = getGameSessionMaster(gameId);
        if (game == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl.getSessionIds] Unable to find GameSessionMaster for game " << gameId);
            return ERR_SYSTEM;
        }

        GameManager::PlayerRoster::PlayerInfoList players = game->getPlayerRoster()->getPlayers(GameManager::PlayerRoster::ROSTER_PARTICIPANTS);
        for (auto& player : players)
        {
            ids.push_back(player->getPlayerSessionId());
        }
        return ERR_OK;
    }
    BlazeRpcError GameManagerMasterImpl::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
    {
        GameId gameId = (GameId)bobjId.id;
        GameSessionMaster* game = getGameSessionMaster(gameId);
        if (game == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl.getUserBlazeIds] Unable to find GameSessionMaster for game " << gameId);
            return ERR_SYSTEM;
        }

        GameManager::PlayerRoster::PlayerInfoList players = game->getPlayerRoster()->getPlayers(GameManager::PlayerRoster::ALL_PLAYERS);
        for (auto& player : players)
        {
            BlazeId blazeId = gUserSessionManager->getBlazeId(player->getPlayerSessionId());
            if (blazeId != INVALID_BLAZE_ID)
            {
                ids.push_back(blazeId);
            }
        }
        return ERR_OK;
    }
    BlazeRpcError GameManagerMasterImpl::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
    {
        GameId gameId = (GameId)bobjId.id;
        GameSessionMaster* game = getGameSessionMaster(gameId);
        if (game == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl.getUserIds] Unable to find GameSessionMaster for game " << gameId);
            return ERR_SYSTEM;
        }

        GameManager::PlayerRoster::PlayerInfoList players = game->getPlayerRoster()->getPlayers(GameManager::PlayerRoster::ALL_PLAYERS);
        for (auto& player : players)
        {
            UserIdentificationPtr userIdent = ids.pull_back();
            player->getPlatformInfo().copyInto(userIdent->getPlatformInfo());
            userIdent->setBlazeId(player->getPlayerId());
            userIdent->setExternalId(getExternalIdFromPlatformInfo(userIdent->getPlatformInfo()));
            userIdent->setName(player->getPlayerName());
            userIdent->setAccountLocale(player->getAccountLocale());
            userIdent->setAccountCountry(player->getAccountCountry());
            userIdent->setPersonaNamespace(player->getPlayerNameSpace());
        }
        return ERR_OK;
    }
    BlazeRpcError GameManagerMasterImpl::countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
    {
        GameId gameId = (GameId)bobjId.id;
        GameSessionMaster* game = getGameSessionMaster(gameId);
        if (game == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl.countSessions] Unable to find GameSessionMaster for game " << gameId);
            return ERR_SYSTEM;
        }

        // Just assuming that the each player has a unique session: (as per prior impl)
        count = game->getPlayerRoster()->getPlayerCount(GameManager::PlayerRoster::ROSTER_PARTICIPANTS);
        return ERR_OK;
    }
    BlazeRpcError GameManagerMasterImpl::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
    {
        GameId gameId = (GameId)bobjId.id;
        GameSessionMaster* game = getGameSessionMaster(gameId);
        if (game == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl.countUsers] Unable to find GameSessionMaster for game " << gameId );
            return ERR_SYSTEM;
        }

        count = game->getPlayerRoster()->getPlayerCount(GameManager::PlayerRoster::ROSTER_PARTICIPANTS);
        return ERR_OK;
    }


    void GameManagerMasterImpl::onUserSessionExistence(const UserSession& userSession)
    {
        GameIdSet gameIdSet; // cached

        switch (userSession.getPlatformInfo().getClientPlatform())
        {
        case pc:
            if (userSession.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
            {
                GameIdSetByOriginPersonaIdMap::const_iterator it = mGameIdSetByOriginPersonaIdMap.find(userSession.getPlatformInfo().getEaIds().getOriginPersonaId());
                if (it != mGameIdSetByOriginPersonaIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        case ps4:
        case ps5:
            if (userSession.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
            {
                GameIdSetByPsnAccountIdMap::iterator it = mGameIdSetByPsnAccountIdMap.find(userSession.getPlatformInfo().getExternalIds().getPsnAccountId());
                if (it != mGameIdSetByPsnAccountIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        case xone:
        case xbsx:
            if (userSession.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
            {
                GameIdSetByXblAccountIdMap::iterator it = mGameIdSetByXblAccountIdMap.find(userSession.getPlatformInfo().getExternalIds().getXblAccountId());
                if (it != mGameIdSetByXblAccountIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        case nx:
            if (!userSession.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
            {
                GameIdSetBySwitchIdMap::iterator it = mGameIdSetBySwitchIdMap.find(userSession.getPlatformInfo().getExternalIds().getSwitchId());
                if (it != mGameIdSetBySwitchIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        case steam:
            if (userSession.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
            {
                GameIdSetBySteamAccountIdMap::iterator it = mGameIdSetBySteamAccountIdMap.find(userSession.getPlatformInfo().getExternalIds().getSteamAccountId());
                if (it != mGameIdSetBySteamAccountIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        case stadia:
            if (userSession.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
            {
                GameIdSetByPsnAccountIdMap::iterator it = mGameIdSetByStadiaAccountIdMap.find(userSession.getPlatformInfo().getExternalIds().getStadiaAccountId());
                if (it != mGameIdSetByStadiaAccountIdMap.end())
                    gameIdSet.insert(it->second.begin(), it->second.end());
            }
            break;
        default:
            break;
        }

        eastl::string platformInfoStr;
        TRACE_LOG("[GameManagerMasterImpl.onUserSessionExistence] Transferring old place holder player object reservations to new player objects, for user(id=" << userSession.getBlazeId() << ",platformInfo=(" <<
            platformInfoToString(userSession.getPlatformInfo(), platformInfoStr) << ")) in total of " << gameIdSet.size() << " games.");

        for (GameIdSet::iterator i = gameIdSet.begin(), e = gameIdSet.end(); i != e; ++i)
        {
            GameSessionMasterPtr game = getWritableGameSession(*i);
            if (game == nullptr)
            {
                continue;
            }
            if (game->replaceExternalPlayer(userSession.getUserSessionId(), userSession) == nullptr)
            {
                ERR_LOG("[GameManagerMasterImpl.onUserSessionExistence] Failed to find or transfer reservation from any old place holder player object to new player object, for user(id=" << userSession.getBlazeId() << ",platformInfo=(" << platformInfoStr.c_str() << ")), for game(" << game->getGameId() << ")");
            }

            // Note: don't clean up the map entries for the user here. That is done when the PlayerInfoMaster
            // object is destroyed when removed from the game, triggered by the replace external player call above
        }
    }

    void GameManagerMasterImpl::insertIntoExternalPlayerToGameIdsMap(const UserInfoData& playerInfo, GameId gameId)
    {
        switch (playerInfo.getPlatformInfo().getClientPlatform())
        {
        case pc:
        {
            auto originId = playerInfo.getPlatformInfo().getEaIds().getOriginPersonaId();
            if (originId != INVALID_ORIGIN_PERSONA_ID)
            {
                if (mGameIdSetByOriginPersonaIdMap.insert(originId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added Origin player entry (originId="
                        << originId << "), for game(" << gameId << ").");
                }
            }
        }
        break;
        case ps4:
        case ps5:
        {
            auto psnId = playerInfo.getPlatformInfo().getExternalIds().getPsnAccountId();
            if (psnId != INVALID_PSN_ACCOUNT_ID)
            {
                if (mGameIdSetByPsnAccountIdMap[psnId].insert(gameId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added psn player entry (psnId="
                        << psnId << "), for game(" << gameId << ").");
                }
            }
        }
        break;
        case xone:
        case xbsx:
        {
            auto xblId = playerInfo.getPlatformInfo().getExternalIds().getXblAccountId();
            if (xblId != INVALID_XBL_ACCOUNT_ID)
            {
                if (mGameIdSetByXblAccountIdMap[xblId].insert(gameId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added xbl player entry (xblId="
                        << xblId << "), for game(" << gameId << ").");
                }
            }
        }
        break;
        case nx:
        {
            auto switchId = playerInfo.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
            if (!switchId.empty())
            {
                if (mGameIdSetBySwitchIdMap[switchId].insert(gameId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added switch player entry (switchId="
                        << switchId.c_str() << "), for game(" << gameId << ").");
                }
            }
        }
        break;
        case steam:
        {
            auto steamId = playerInfo.getPlatformInfo().getExternalIds().getSteamAccountId();
            if (steamId != INVALID_STEAM_ACCOUNT_ID)
            {
                if (mGameIdSetBySteamAccountIdMap[steamId].insert(gameId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added steam player entry (steamId="
                        << steamId << "), for game(" << gameId << ").");
                }
            }
        }
        break;
        case stadia:
        {
            auto stadiaId = playerInfo.getPlatformInfo().getExternalIds().getStadiaAccountId();
            if (stadiaId != INVALID_STADIA_ACCOUNT_ID)
            {
                if (mGameIdSetByStadiaAccountIdMap[stadiaId].insert(gameId).second)
                {
                    TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Added stadia player entry (stadiaId="
                        << stadiaId << "), for game(" << gameId << ").");
                }
            }
        }
        break; 
        default:
            break;
        }
    }

    bool GameManagerMasterImpl::eraseFromExternalPlayerToGameIdsMap(const UserInfoData& playerInfo, GameId gameId)
    {
        bool result = false;

        switch (playerInfo.getPlatformInfo().getClientPlatform())
        {
        case pc:
        {
            auto originId = playerInfo.getPlatformInfo().getEaIds().getOriginPersonaId();
            if (originId != INVALID_ORIGIN_PERSONA_ID)
            {
                auto it = mGameIdSetByOriginPersonaIdMap.find(originId);
                if (it != mGameIdSetByOriginPersonaIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (originId="
                            << originId << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetByOriginPersonaIdMap.erase(originId);
                }
            }
        }
        break;
        case ps4:
        case ps5:
        {
            auto psnId = playerInfo.getPlatformInfo().getExternalIds().getPsnAccountId();
            if (psnId != INVALID_PSN_ACCOUNT_ID)
            {
                auto it = mGameIdSetByPsnAccountIdMap.find(psnId);
                if (it != mGameIdSetByPsnAccountIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (psnId="
                            << psnId << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetByPsnAccountIdMap.erase(psnId);
                }
            }
        }
        break;
        case xone:
        case xbsx:
        {
            auto xblId = playerInfo.getPlatformInfo().getExternalIds().getXblAccountId();
            if (xblId != INVALID_XBL_ACCOUNT_ID)
            {
                auto it = mGameIdSetByXblAccountIdMap.find(xblId);
                if (it != mGameIdSetByXblAccountIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (xblId="
                            << xblId << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetByXblAccountIdMap.erase(xblId);
                }
            }
        }
        break;
        case nx:
        {
            auto switchId = playerInfo.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
            if (!switchId.empty())
            {
                auto it = mGameIdSetBySwitchIdMap.find(switchId);
                if (it != mGameIdSetBySwitchIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (switchId="
                            << switchId.c_str() << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetBySwitchIdMap.erase(switchId);
                }
            }
        }
        break;
        case steam:
        {
            auto steamId = playerInfo.getPlatformInfo().getExternalIds().getSteamAccountId();
            if (steamId != INVALID_STEAM_ACCOUNT_ID)
            {
                auto it = mGameIdSetBySteamAccountIdMap.find(steamId);
                if (it != mGameIdSetBySteamAccountIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (steamId="
                            << steamId << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetBySteamAccountIdMap.erase(steamId);
                }
            }
        }
        break;
        case stadia:
        {
            auto stadiaId = playerInfo.getPlatformInfo().getExternalIds().getStadiaAccountId();
            if (stadiaId != INVALID_STADIA_ACCOUNT_ID)
            {
                auto it = mGameIdSetByStadiaAccountIdMap.find(stadiaId);
                if (it != mGameIdSetByStadiaAccountIdMap.end())
                {
                    if (it->second.erase(gameId) > 0)
                    {
                        result = true;
                        TRACE_LOG("[GameManagerMasterImpl].insertToExternalPlayerToGameIdsMap: Removed external player entry (stadiaId="
                            << stadiaId << "), for game(" << gameId << ").");
                    }
                    if (it->second.empty())
                        mGameIdSetByStadiaAccountIdMap.erase(stadiaId);
                }
            }
        }
        break;
        default:
            break;
        }

        return result;
    }

    
    /*! ************************************************************************************************/
    /*! \brief entry point for the census data component to poll the GameManager slave for census info.
    ***************************************************************************************************/
    GetCensusDataError::Error GameManagerMasterImpl::processGetCensusData(GameManagerCensusData &response, const Message *message)
    {    
        TimeValue now = TimeValue::getTimeOfDay();

        if (now < mCensusCacheExpiry)
        {
            // Just return the cached response
            mCensusCache.copyInto(response);
            return GetCensusDataError::ERR_OK;
        }

        populateCensusData(response);
        mCensusCacheExpiry = now + (1 * 1000 * 1000);
        response.copyInto(mCensusCache);

        return GetCensusDataError::ERR_OK; 
    }

    void GameManagerMasterImpl::populateCensusData(GameManagerCensusData& curCensus)
    {
        Metrics::TagPairList baseQuery;
        // Number of games with people in them (everything but dedicated servers waiting to be reset)
        curCensus.setNumOfActiveGame(getNumberOfActiveGameSessions(baseQuery));
        curCensus.setNumOfJoinedPlayer((uint32_t)mMetrics.mInGamePlayerSessions.get({ { Metrics::Tag::game_type, GameTypeToString(GAME_TYPE_GAMESESSION) } }));
        curCensus.setNumOfGameGroup((uint32_t)mMetrics.mActiveGameGroups.get());
        curCensus.setNumOfPlayersInGameGroup((uint32_t)mMetrics.mInGamePlayerSessions.get({ { Metrics::Tag::game_type, GameTypeToString(GAME_TYPE_GROUP) } }));

        const PingSiteInfoByAliasMap& pingSiteInfoMap = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap();
        for (PingSiteInfoByAliasMap::const_iterator it = pingSiteInfoMap.begin(), end = pingSiteInfoMap.end(); it != end; ++it)
        {
            Metrics::TagPairList pingSiteQuery = { { Metrics::Tag::game_pingsite, it->first.c_str() } };
            curCensus.getNumOfActiveGamePerPingSite()[it->first] = getNumberOfActiveGameSessions(pingSiteQuery);
            curCensus.getNumOfJoinedPlayerPerPingSite()[it->first] = (uint32_t)mMetrics.mActivePlayers.get({ { Metrics::Tag::game_type, GameTypeToString(GAME_TYPE_GAMESESSION) },{ Metrics::Tag::game_pingsite, it->first.c_str() } });
        }

        // GM_GAME_ATTRIBUTE
        //   We return a count of games that have specific game attrib name/value pairs (as configured in the config file)
        //   Ex: 3 games have "mode" = "deathmatch"; 332 games have "mode" = "captureTheFlag"
        populateGameAttributeCensusData(curCensus.getGameAttributesData());
    }

    /*! ************************************************************************************************/
    /*! \brief populate the census data map with the configured info about games with specific game attrib values
        \param[out] censusDataByIndexMap the census data map to populate
    ***************************************************************************************************/
    void addEntryToCensus(GameManagerCensusData::GameAttributeCensusDataList& attrCensusData, const char8_t *attribName, const char8_t *attribValue, const CensusEntry& entry)
    {
        if (entry.gameCount > 0)
        {
            bool foundEntry = false;
            for (auto& attrData : attrCensusData)
            {
                if (attrData->getAttributeNameAsTdfString() == attribName && attrData->getAttributevalueAsTdfString() == attribValue)
                {
                    attrData->setNumOfGames(attrData->getNumOfGames() + entry.gameCount);
                    attrData->setNumOfPlayers(attrData->getNumOfPlayers() + entry.playerCount);
                    break;
                }
            }

            if (!foundEntry)
            {
                GameAttributeCensusData* gameAttributeCensusData = attrCensusData.pull_back();
                gameAttributeCensusData->setAttributeName(attribName);
                gameAttributeCensusData->setAttributevalue(attribValue);
                gameAttributeCensusData->setNumOfGames(entry.gameCount);
                gameAttributeCensusData->setNumOfPlayers(entry.playerCount);
            }
        }
    }

    void GameManagerMasterImpl::populateGameAttributeCensusData(GameManagerCensusData::GameAttributeCensusDataList& attrCensusData) const
    {
        for (auto attribMapIter : getConfig().getCensusData().getGameAttributes())
        {
            const char8_t *attribName = attribMapIter.first;
            const Collections::AttributeValueList *attribValueList = attribMapIter.second;
            if (attribValueList->empty())
                continue;

            CensusByAttrNameMap::const_iterator cacheNameIter = mCensusByAttrNameMap.find(attribName);
            if (cacheNameIter == mCensusByAttrNameMap.end())
                continue;

            Collections::AttributeValueList::const_iterator valueIter = attribValueList->begin();
            Collections::AttributeValueList::const_iterator valueEnd = attribValueList->end();
            if (isCensusAllValues(valueIter->c_str()))
            {
                CensusEntryByAttrValueMap::const_iterator cacheValueIter = cacheNameIter->second.begin();
                CensusEntryByAttrValueMap::const_iterator cacheValueEnd = cacheNameIter->second.end();
                for (; cacheValueIter != cacheValueEnd; ++cacheValueIter)
                {
                    addEntryToCensus(attrCensusData, attribName, cacheValueIter->first.c_str(), cacheValueIter->second);
                }
            }
            else
            {
                for (; valueIter != valueEnd; ++valueIter)
                {
                    const char8_t *attribValue = valueIter->c_str();
                    CensusEntryByAttrValueMap::const_iterator cacheValueIter = cacheNameIter->second.find(attribValue);
                    if (cacheValueIter != cacheNameIter->second.end())
                    {
                        addEntryToCensus(attrCensusData, attribName, attribValue, cacheValueIter->second);
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief get the number of game and players in game with specified game set

        \param[in] gameSet - the set of games
        \param[out] gameAttributeCensusData - data including number of games counted and number of active participants in games counted
    ***************************************************************************************************/
    void GameManagerMasterImpl::getInfoFromCensusCache(const GameIdSet& gameIdSet, GameAttributeCensusData& gameAttributeCensusData) const
    {
        uint32_t numOfPlayers = 0;

        GameIdSet::const_iterator gameIter = gameIdSet.begin();
        GameIdSet::const_iterator gameIterEnd = gameIdSet.end();
        for (; gameIter != gameIterEnd; ++gameIter)
        {
            GameId gameId = *gameIter;
            const GameSessionMaster* gameSessionMaster = getReadOnlyGameSession(gameId);
            if (gameSessionMaster != nullptr)
            {
                numOfPlayers += (uint32_t)gameSessionMaster->getPlayerRoster()->getPlayerCount(PlayerRoster::ACTIVE_PARTICIPANTS);
            }
        }

        gameAttributeCensusData.setNumOfPlayers(numOfPlayers);
    }


    /*! ************************************************************************************************/
    /*! \brief cache this game according to its game attributes (for census data collection)
        \param[in] gameSessionSlave - game to be added to the attribute map
    ***************************************************************************************************/
    void GameManagerMasterImpl::addGameToCensusCache(GameSessionMaster& gameSession)
    {
        if (gameSession.isResetable())
        {
            // Games in reset mode don't count - their game attribs will change once they're reset.
            return;
        }

        // scan the list of game attribs tracked by census, we need to cache this game for each tracked attrib it has
        for (auto censusAttribMapIter : getConfig().getCensusData().getGameAttributes())
        {
            const char8_t *trackedAttribName = censusAttribMapIter.first;
            const char8_t *gameAttribValue = gameSession.getGameAttrib(trackedAttribName);
            if (gameAttribValue != nullptr)
            {
                // game defines one of the attrib names tracked by census; check if the value is also tracked
                const Collections::AttributeValueList *trackedAttribValueList = censusAttribMapIter.second;
                Collections::AttributeValueList::const_iterator tempIter = trackedAttribValueList->begin();

                bool isTracked = isCensusAllValues(tempIter->c_str());
                if (!isTracked)
                {
                    for( ; tempIter != trackedAttribValueList->end(); tempIter++)
                    {
                        if(blaze_strcmp(*tempIter, gameAttribValue) == 0)
                        {
                            isTracked = true;
                            break;
                        }
                    }
                }
                if (isTracked)
                {
                    auto censusIter = mCensusByAttrNameMap.find(trackedAttribName);
                    if (censusIter == mCensusByAttrNameMap.end())
                    {
                        censusIter = mCensusByAttrNameMap.insert(trackedAttribName).first;
                        censusIter->second.set_allocator(BlazeStlAllocator("mCensusByAttrNameMapMap", COMPONENT_MEMORY_GROUP));
                    }

                    // game's attrib is tracked by census, add the game to the census map for this attrib name/value combo
                    CensusEntry& entry = censusIter->second[gameAttribValue];
                    gameSession.addToCensusEntry(entry);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief remove the game from the census data's game cache (cache games by attribute values)
        \param[in] gameSession - game needs to be deleted from the set
    ***************************************************************************************************/
    void GameManagerMasterImpl::removeGameFromCensusCache(GameSessionMaster& gameSession)
    {
        gameSession.removeFromCensusEntries();
    }

    /*! ************************************************************************************************/
    /*! \brief updates the specified game's external session's properties.
    ***************************************************************************************************/
    void GameManagerMasterImpl::handleUpdateExternalSessionProperties(GameId gameId, ExternalSessionUpdateInfoPtr origValues, ExternalSessionUpdateEventContextPtr context)
    {
        mExternalSessionServices.handleUpdateExternalSessionProperties(this, gameId, origValues, context);
    }

    void GameManagerMasterImpl::incrementExternalSessionFailuresNonImpacting(ClientPlatformType platform)
    {
        mMetrics.mExternalSessionFailuresNonImpacting.increment(1, platform);
    }


    /*! ************************************************************************************************/
    /*! \brief leaves the specified external id from the game's external session.
        \param[in] params parameters to pass to external service's leave() method
    ***************************************************************************************************/
    void GameManagerMasterImpl::handleLeaveExternalSession(LeaveGroupExternalSessionParametersPtr params)
    {
        if (params != nullptr)
        {
            ExternalSessionApiError apiErr;
            BlazeRpcError sessErr = mExternalSessionServices.leave(*params, apiErr);
            if (ERR_OK != sessErr)
            {
                for (const auto& platformErr : apiErr.getPlatformErrorMap())
                {
                    incrementExternalSessionFailuresNonImpacting(platformErr.first);
                    TRACE_LOG("[GameManagerMasterImpl].handleLeaveExternalSession: Failed to leave " << platformErr.second->getUsers().size()
                        << " players from external session " << toLogStr(params->getSessionIdentification()) << " error (" << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr())
                        << ") platform(" << ClientPlatformTypeToString(platformErr.first));
                }
            }
        }
        else
        {
            ERR_LOG("[GameManagerMasterImpl].handleLeaveExternalSession: Failed to leave players - nullptr params");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief remove the specified players if present, from the game's pending preferred joins set.
        Players that have already opted out or called join game by user list, are ignored
    ***************************************************************************************************/
    PreferredJoinOptOutMasterError::Error GameManagerMasterImpl::processPreferredJoinOptOutMaster(
        const PreferredJoinOptOutMasterRequest &request, const Message *message)
    {
        GameSessionMasterPtr game = getWritableGameSession(request.getGameId());
        if ((game != nullptr) && !game->isLockedForPreferredJoins())
        {
            // timer may have expired, or been cleared before the opt out request
            TRACE_LOG("[GameManagerMasterImpl].processPreferredJoinOptOutMaster: Game(" << request.getGameId() << ") is not currently locked for preferred joins. Ignoring attempt to opt out");
            return PreferredJoinOptOutMasterError::ERR_OK;
        }
        if (game == nullptr)
        {
            TRACE_LOG("[GameManagerMasterImpl].processPreferredJoinOptOutMaster: Failed to find game(" << request.getGameId() << "). May have been removed. No op.");
            return PreferredJoinOptOutMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        // caller must be in the game, before it can opt out
        const PlayerId callerPlayerId = UserSession::getCurrentUserBlazeId();
        if (EA_UNLIKELY(callerPlayerId == INVALID_BLAZE_ID))
            return PreferredJoinOptOutMasterError::ERR_AUTHORIZATION_REQUIRED;

        if (game->getPlayerRoster()->getPlayer(callerPlayerId) == nullptr)
        {
            TRACE_LOG("[GameManagerMasterImpl].processPreferredJoinOptOutMaster: player(blaze id: " << callerPlayerId << ") was not found in game(" << request.getGameId() << "). No op.");
            return PreferredJoinOptOutMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        // erase the ids if present
        for (UserSessionIdList::const_iterator itr = request.getSessionIdList().begin(); itr != request.getSessionIdList().end(); ++itr)
        {
            BlazeId blazeId = gUserSessionManager->getBlazeId(*itr);
            if (blazeId != INVALID_BLAZE_ID)
                game->eraseFromPendingPreferredJoinSet(blazeId);
        }

        return PreferredJoinOptOutMasterError::ERR_OK;
    }

     /*! ************************************************************************************************/
    /*! \brief if Blaze game and its external session got out of synce, remove from the external session
        its unexpected members
    ***************************************************************************************************/
    ResyncExternalSessionMembersError::Error GameManagerMasterImpl::processResyncExternalSessionMembers(
        const Blaze::GameManager::ResyncExternalSessionMembersRequest &request, const Blaze::Message *message)
    {
        GameSessionMasterPtr game = getWritableGameSession(request.getGameId());
        if (game == nullptr)
        {
            TRACE_LOG("[GameManagerMasterImpl].processResyncExternalSessionMembers: Failed to find game(" << request.getGameId() << ")");
            return ResyncExternalSessionMembersError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
       
        // This metric track first party sync/desync changes. The actual resync code might only execute on some platforms(xone at the moment) but we currently use the 
        // platform_list as the dimension rather than platform(The latter would have required creating a new metric type rather than using the one we have already.
        // Probably something to look back with Gen5 effort. 
        mMetrics.mExternalSessionSyncChecks.increment(1, game->getGameType(), GameSessionMetricsHandler::getGameModeAsMetric(game->getGameMode()), game->getGameNetworkTopology(), game->getBestPingSiteAlias(), game->getPlatformForMetrics());

        // guard vs redundant calls/fibers
        if (game->getIsPendingResyncExternalSession())
            return ResyncExternalSessionMembersError::ERR_OK;
        game->setIsPendingResyncExternalSession(true);

        mExternalSessionJobQueue.queueFiberJob(this, &GameManagerMasterImpl::handleResyncExternalSessionMembers, request.getGameId(), "GameManagerMasterImpl::handleResyncExternalSessionMembers");
        
        return ResyncExternalSessionMembersError::ERR_OK;
    }

    /*! \brief fiber method for resync'ing external session members */
    void GameManagerMasterImpl::handleResyncExternalSessionMembers(GameId gameId)
    {
        GameSessionMasterPtr game = getWritableGameSession(gameId);
        if (game == nullptr)
            return;

        
        // NOTE:  This function only removes external members of the session that are not currently in the Game,
        //        it does not remove players in the Game that are not (yet) members of the external session.  (Therefore it's safe to do all platforms at once, rather than individually.)
        ExternalSessionUtilManager::ExternalIdsByPlatform externalSessionMembers;
        ExternalSessionApiError getMemberApiErr;
        BlazeRpcError err = mExternalSessionServices.getMembers(((const GameSessionMaster&)*game).getExternalSessionIdentification(), externalSessionMembers, getMemberApiErr);

        TRACE_LOG("[GameManagerMasterImpl].handleResyncExternalSessionMembers: checking if require resync for game(" << gameId << ") external session.");
        if (err != ERR_OK)
        {
            for (const auto& platformErr: getMemberApiErr.getPlatformErrorMap() )
                incrementExternalSessionFailuresNonImpacting(platformErr.first);
        }
        // Get expected members - i.e. players in the Blaze game with external session join permission
        typedef eastl::hash_set<ExternalId> ExternalIdSet;
        typedef eastl::map<ClientPlatformType, ExternalIdSet> ExternalIdSetByPlatform;
        ExternalIdSetByPlatform gameRosterMembers;
        const PlayerRosterMaster::PlayerInfoList players = game->getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS);
        for (PlayerRosterMaster::PlayerInfoList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            ExternalId extId = getExternalIdFromPlatformInfo((*itr)->getPlatformInfo());
            ClientPlatformType platform = (*itr)->getPlatformInfo().getClientPlatform();

            bool isQueued = (*itr)->isInQueue();
            if ((*itr)->getUserInfo().getHasExternalSessionJoinPermission() && !isQueued)
                gameRosterMembers[platform].insert(extId);
        }

        // Get unexpected members. store to the external session leave params
        LeaveGroupExternalSessionParameters unexpectedExternalSessionMembers;
        for (const auto& aItr : externalSessionMembers) //iterate externalIds in external session by platform
        {
            if (getMemberApiErr.getPlatformErrorMap().find((aItr.first)) != getMemberApiErr.getPlatformErrorMap().end())
            {
                // we ran into an error talking to first party. Let's be conservative and not do any removal.
                continue;
            }
                
            ExternalIdSetByPlatform::const_iterator eItr = gameRosterMembers.find(aItr.first);//find the externalId list in the game for this platform
            if (eItr == gameRosterMembers.end()) // no player set for this platform found
            {
                for (const auto& uItr : aItr.second) // take all the external Ids and add them for removal.
                    updateExternalSessionLeaveParams(unexpectedExternalSessionMembers, uItr, game->getReplicatedGameSession(), aItr.first);
            }
            else
            {
                for (const auto& uItr : aItr.second) // iterate over each player in the external id list. If not found in the game session, put it up for removal. 
                {
                    if (eItr->second.find(uItr) == eItr->second.end())
                        updateExternalSessionLeaveParams(unexpectedExternalSessionMembers, uItr, game->getReplicatedGameSession(), aItr.first);
                }
            }
        }

        // Remove unexpected members.
        if (!unexpectedExternalSessionMembers.getExternalUserLeaveInfos().empty())
        {
            size_t unexpectedMemberCount = unexpectedExternalSessionMembers.getExternalUserLeaveInfos().size();
            mMetrics.mExternalSessionsDesynced.increment(1, game->getGameType(), GameSessionMetricsHandler::getGameModeAsMetric(game->getGameMode()), game->getGameNetworkTopology(), game->getBestPingSiteAlias(), game->getPlatformForMetrics());
            mMetrics.mExternalSessionsDesyncedMembers.increment(unexpectedMemberCount, game->getGameType(), GameSessionMetricsHandler::getGameModeAsMetric(game->getGameMode()), game->getGameNetworkTopology(), game->getBestPingSiteAlias(), game->getPlatformForMetrics());
                
            game->incrementTotalExternalSessionsDesynced();

            WARN_LOG("[GameManagerMasterImpl].handleResyncExternalSessionMembers: required resync for game(" << gameId << ") external session, attempting to remove " << unexpectedMemberCount
                << " unexpected members from the external session. Current total players in Blaze game: " << game->getPlayerRoster()->getRosterPlayerCount() << "/ " << game->getTotalPlayerCapacity()
                << "( participants: " << game->getPlayerRoster()->getParticipantCount() << "/" << game->getTotalParticipantCapacity()
                << ", spectators: " << game->getPlayerRoster()->getSpectatorCount() << "/" << game->getTotalSpectatorCapacity()
                << ", queued: " << game->getPlayerRoster()->getQueueCount() << "/" << game->getQueueCapacity() << " ). Number of times this game required resync's: " << game->getTotalExternalSessionsDesynced());

            ExternalSessionApiError apiErr;
            if (mExternalSessionServices.leave(unexpectedExternalSessionMembers, apiErr) != ERR_OK)
            {
                for (const auto& platformErr : apiErr.getPlatformErrorMap())
                    incrementExternalSessionFailuresNonImpacting(platformErr.first);
            }
        }

        game->setIsPendingResyncExternalSession(false);
    }

     /*! ************************************************************************************************/
    /*! \brief Set a game session as responsive or not.
    ***************************************************************************************************/
    SetGameResponsiveMasterError::Error GameManagerMasterImpl::processSetGameResponsiveMaster(const Blaze::GameManager::SetGameResponsiveRequest &request, const Message* message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return SetGameResponsiveMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        gameSessionMaster->setResponsive(request.getResponsive());
        return SetGameResponsiveMasterError::ERR_OK;
    }

    void GameManagerMasterImpl::sendQosDataToMatchmaker(GameSessionMasterPtr game, GameSessionConnectionCompletePtr mmQosData)
    {
        QosEvaluationResult resp;

        BlazeRpcError error;
        Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
            static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &error));
        if (matchmakerComponent == nullptr)
        {
            ERR_LOG("[[GameSessionMaster].sendQosDataToMatchmaker()].execute() - Unable to resolve matchmaker component.");
        }
        else
        {
            error = matchmakerComponent->gameSessionConnectionComplete(*mmQosData, resp);
        }

        if (error != ERR_OK)
        {
            // need to yield the fiber, because the call chain above assumes that we've blocked, and if the RPC call fast-fails, we won't.
            Fiber::yield();
        }

        if (game != nullptr)
        {
            TRACE_LOG("[GameManagerMasterImpl].sendQosDataToMatchmaker() : sent MatchmakingQosData in game(" << game->getGameId() 
                << ") for finalized matchmaking session(" << mmQosData->getMatchmakingSessionId() <<") error '" << ErrorHelp::getErrorName(error) << "'.");
            if ((matchmakerComponent != nullptr) && (error == ERR_OK))
            {
                if (resp.getDestroyCreatedGame() && (game->getGameState() == CONNECTION_VERIFICATION))
                {
                    // destroy the game and do nothing else
                    if (game->hasDedicatedServerHost())
                    {
                        game->returnDedicatedServerToPool();
                    }
                    else
                    {
                        destroyGame(game.get(), SYS_GAME_FAILED_CONNECTION_VALIDATION);
                    }
                }
                else
                {
                    // advance game state if needed
                    if (game->getGameState() == CONNECTION_VERIFICATION)
                    {
                        game->setGameState(INITIALIZING, true);
                    }
                    game->updateQosTestedPlayers(resp.getPassedValidationList(), resp.getFailedValidationList());
                }
            }
            else
            {
                // If lookup failed, we destroy the game, if appropriate
                if (game->getGameState() == CONNECTION_VERIFICATION)
                {
                    // destroy the game and do nothing else
                    if (game->hasDedicatedServerHost())
                    {
                        game->returnDedicatedServerToPool();
                    }
                    else
                    {
                        destroyGame(game.get(), SYS_GAME_FAILED_CONNECTION_VALIDATION);
                    }
                }
                else
                {
                    // game wasn't created by this MM attempt, clean up users
                    PlayerIdList tempPassedIdList;
                    PlayerIdList tempFailedIdList;

                    GameQosDataMap::const_iterator gameQosIter = mmQosData->getGameQosDataMap().begin();
                    GameQosDataMap::const_iterator gameQosEnd = mmQosData->getGameQosDataMap().end();
                    for (; gameQosIter != gameQosEnd; ++gameQosIter)
                    {
                        tempFailedIdList.push_back(gameQosIter->first);
                    }

                    // passed list is intentionally empty, don't want to kick anyone out of game if something went wrong in MM
                    game->updateQosTestedPlayers(tempPassedIdList, tempFailedIdList);
                }
            }
        }
    }

    BlazeRpcError GameManagerMasterImpl::processTelemetryReport(ConnectionGroupId localConnGroupId, GameId gameId, const TelemetryReport& report, PingSiteAlias pingSite)
    {
        ConnectionGroupId remoteConnGroupId = report.getRemoteConnGroupId();
        if (localConnGroupId == remoteConnGroupId)
        {
            SPAM_LOG("[GameManagerMasterImpl].processTelemetryReport: Ignoring report because remoteConnectionGroupId(" << remoteConnGroupId << ") is equal to the localConnectionGroupId.");
            return ERR_OK;
        }

        // We do not need to modify the game session, telemetry is not persistent
        const GameSessionMaster* gameSessionMaster = getReadOnlyGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMasterImpl].processTelemetryReport: Game id " << gameId << " not found.");
            return GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        GameNetworkTopology topology = gameSessionMaster->getGameNetworkTopology();
        if (EA_UNLIKELY(localConnGroupId == 0))
        {
            // shouldn't hit this as the command class guards it, but getConnectionGroupUserIds can assert if we pass in 0, so need to check here.
            ERR_LOG("[GameManagerMasterImpl].processTelemetryReport: incoming telemetry report for game with topology (" << topology << ") had a localConnGroupId of (0).");
            return ERR_SYSTEM;
        }
        
        // For dedicated server topologies, the host will not be in the player roster, so check if the current session is the host
        // and that he matches one of the connectionGroupIds. If so, then continue processing this report
        bool remotePlayerInGame = false;
        if (gameSessionMaster->hasDedicatedServerHost() && gameSessionMaster->getDedicatedServerHostSessionExists())
        {
            ConnectionGroupId groupId = gameSessionMaster->getDedicatedServerHostInfo().getConnectionGroupId();
            if (groupId == remoteConnGroupId)
                remotePlayerInGame = true;
        }
        
        if (!remotePlayerInGame)
        {
            const PlayerRoster::PlayerInfoList players = gameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
            PlayerRoster::PlayerInfoList::const_iterator iter = players.begin();
            PlayerRoster::PlayerInfoList::const_iterator end = players.end();
            for (; iter != end; ++iter)
            {
                PlayerInfo *info = *iter;
                ConnectionGroupId playerConnGroupId = info->getConnectionGroupId();
                if (playerConnGroupId == remoteConnGroupId)
                {
                    remotePlayerInGame = true;
                    break;
                }
            }
        }

        if (!remotePlayerInGame)
        {
            TRACE_LOG("[GameManagerMasterImpl].processTelemetryReport: Report received for player that is not in game id " << gameId << ". Skip this report.");
            return ERR_OK;
        }

        if (pingSite.empty())
            pingSite = UNKNOWN_PINGSITE;

        uint32_t packetsRecv = report.getLocalPacketsReceived();
        uint32_t packetsSent = report.getRemotePacketsSent();
        float packetLoss = (report.getPacketLossFloat() == 0.0f) ? (float)report.getPacketLossPercent() : report.getPacketLossFloat();
        uint8_t packetLossCeil = (uint8_t) ::ceil(packetLoss);

        if (packetLossCeil > 100)
        {
            WARN_LOG("[GameManagerMasterImpl].processTelemetryReport: reported packet loss(" << packetLossCeil << ") is outside of range 0-100%.");
            return ERR_SYSTEM;
        }

        if (packetsRecv > packetsSent)
        {
            WARN_LOG("[GameManagerMasterImpl].processTelemetryReport: reported packet received (" << packetsRecv << ") is greater than reported packets sent (" << packetsSent << ".");
            return ERR_SYSTEM;
        }

        GameManagerMasterMetrics::ConnectionGroupIdPair connGroupIdPair(localConnGroupId, remoteConnGroupId);
        GameManagerMasterMetrics::PacketLossValues* packetLossValues = nullptr;
        GameManagerMasterMetrics::PacketLossByConnGroupByGameIdMap::iterator gameIdPacketIter = mMetrics.mCurrentPacketLossByConnGroup.find(gameId);
        if (gameIdPacketIter != mMetrics.mCurrentPacketLossByConnGroup.end())
        {
            GameManagerMasterMetrics::PacketLossByConnGroupMap::iterator packetLossIter = gameIdPacketIter->second.find(connGroupIdPair);
            if (packetLossIter != gameIdPacketIter->second.end())
            {
                packetLossValues = &packetLossIter->second;
                uint8_t prevPacketLossCeil = packetLossValues->mCurrentPacketLossCeil;
                if (prevPacketLossCeil != packetLossCeil)
                {
                    const auto quantizedPrevPacketLossCeil = getQuantizedPacketLoss(prevPacketLossCeil);
                    const auto quantizedPacketLossCeil = getQuantizedPacketLoss(packetLossCeil);
                    mMetrics.mCurrentPacketLoss.decrement(1, quantizedPrevPacketLossCeil, topology, pingSite.c_str());
                    mMetrics.mCurrentPacketLoss.increment(1, quantizedPacketLossCeil, topology, pingSite.c_str());
                }

                // calculate the packet loss for this specific sample
                uint32_t packetsRecvDiff = packetsRecv - packetLossValues->mPrevPacketsRecv;
                uint32_t packetsSentDiff = packetsSent - packetLossValues->mPrevPacketsSent;

                if (packetsRecvDiff > packetsSentDiff)
                {
                    WARN_LOG("[GameManagerMasterImpl].processTelemetryReport: reported packet loss has decreased from previous report which should never happen.");
                    return ERR_SYSTEM;
                }

                uint8_t packetLossDiff = packetsSentDiff == 0 ? 0 : (uint8_t)(::ceil(((packetsSentDiff - packetsRecvDiff) / (double)packetsSentDiff) * 100.0f));
                ++packetLossValues->mTotalSamplePacketLosses[packetLossDiff];
            }
        }

        if (packetLossValues == nullptr)
        {
            GameManagerMasterMetrics::PacketLossByConnGroupMap& map = mMetrics.mCurrentPacketLossByConnGroup[gameId];
            packetLossValues = &map[connGroupIdPair];
            mMetrics.mCurrentPacketLoss.increment(1, packetLossCeil, topology, pingSite.c_str());
            ++packetLossValues->mTotalSamplePacketLosses[packetLossCeil];
        }

        packetLossValues->mCurrentPacketLoss = packetLoss;
        packetLossValues->mCurrentPacketLossCeil = packetLossCeil;
        packetLossValues->mPrevPacketsRecv = packetsRecv;
        packetLossValues->mPrevPacketsSent = packetsSent;

        uint32_t latency = report.getLatencyMs();
        uint16_t latencyIndex = UserSessionManager::getLatencyIndex(latency);

        GameManagerMasterMetrics::LatencyByConnGroupMap& gameIdLatencyMap = mMetrics.mLatencyByConnGroups[gameId];
        GameManagerMasterMetrics::ConnectionGroupLatencyValues& latencyByConnGroupCounts = gameIdLatencyMap[connGroupIdPair];

        if (latencyByConnGroupCounts.mMaxLatency < latency)
            latencyByConnGroupCounts.mMaxLatency = latency;
        if (latencyByConnGroupCounts.mMinLatency > latency)
            latencyByConnGroupCounts.mMinLatency = latency;
        latencyByConnGroupCounts.mTotalLatency += latency;
        ++latencyByConnGroupCounts.mLatencyCount;

        ++latencyByConnGroupCounts.mTotalSampleLatencies[latencyIndex / LATENCY_BUCKET_SIZE];

        // New Detailed Stats:
        if (!getConfig().getDisableDetailedConnectionStats())
        {

            auto& pinStatsEventList = mMetrics.mConnectionStatsEventListByConnGroup[gameId][connGroupIdPair];
            if (pinStatsEventList.empty())
            {
                PlayerInfoMaster *sourcePlayerInfo = nullptr;
                PlayerInfoMaster *targetPlayerInfo = nullptr;

                GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator connectionGroupPlayerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(localConnGroupId);
                const PlayerIdList* sourceConnectionGroupIdPlayerList = (connectionGroupPlayerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? connectionGroupPlayerIdListItr->second : nullptr;
                if (sourceConnectionGroupIdPlayerList != nullptr)
                    sourcePlayerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*sourceConnectionGroupIdPlayerList->begin());

                GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator targetPlayerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(remoteConnGroupId);
                const PlayerIdList* targetConnectionGroupIdPlayerList = (targetPlayerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? targetPlayerIdListItr->second : nullptr;
                if (targetConnectionGroupIdPlayerList != nullptr)
                    targetPlayerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*targetConnectionGroupIdPlayerList->begin());

                InetAddress sourceAddr, targetAddr;
                if (sourcePlayerInfo != nullptr)
                    sourceAddr.setHostnameAndPort(sourcePlayerInfo->getConnectionAddr());
                if (targetPlayerInfo != nullptr)
                    targetAddr.setHostnameAndPort(targetPlayerInfo->getConnectionAddr());

                // For dedicated server topologies, the host will not be in the player roster, so check if the host matches one of the playerIds
                if (sourcePlayerInfo == nullptr || targetPlayerInfo == nullptr)
                {
                    if (gameSessionMaster->hasDedicatedServerHost())
                    {
                        const UserSessionInfo& sessionInfo = gameSessionMaster->getTopologyHostUserInfo();
                        UserSessionId hostUserSessionId = sessionInfo.getSessionId();
                        if (hostUserSessionId != INVALID_USER_SESSION_ID)
                        {
                            if (sessionInfo.getConnectionGroupId() == localConnGroupId)
                                sourceAddr.setHostnameAndPort(sessionInfo.getConnectionAddr());
                            else if (sessionInfo.getConnectionGroupId() == remoteConnGroupId)
                                targetAddr.setHostnameAndPort(sessionInfo.getConnectionAddr());
                        }
                    }
                }
                eastl::string hostname = sourceAddr.getHostname();
                eastl::string targetHostname = targetAddr.getHostname();

                const char* statsNames[] = { "distproc",
                     "distwait",
                     "idpps",
                     "odpps",
                     "lplost",
                     "rplost",
                     "ipps",
                     "opps",
                     "jitter",
                     "lnaksent",
                     "rnaksent",
                     "lpsaved",
                     "late"
                    //"rpsaved"
                }; // all the stats names

                eastl::string curGameIdStr;
                curGameIdStr.sprintf("%" PRId64, gameId);
                for (auto curStatName : statsNames)
                {
                    RiverPoster::ConnectionStatsEventPtr curEvent = BLAZE_NEW RiverPoster::ConnectionStatsEvent;
                    pinStatsEventList.push_back(curEvent);

                    curEvent->setGameId(curGameIdStr.c_str());
                    curEvent->setTargetIpAddress(targetHostname.c_str());
                    curEvent->getHeaderCore().setClientIp(hostname.c_str());
                    curEvent->setType(curStatName);

                    curEvent->getHeaderCore().setEventName(RiverPoster::CONNECTION_STATS_EVENT_NAME);
                    curEvent->getHeaderCore().setGameType(gameSessionMaster->getPINGameType());
                    curEvent->getHeaderCore().setModeType(gameSessionMaster->getPINGameModeType());
                    curEvent->getHeaderCore().setGameMode(gameSessionMaster->getGameMode());   // Oh Joy, these can change.
                    if (*gameSessionMaster->getPINGameMap() != '\0')
                        curEvent->getHeaderCore().setMap(gameSessionMaster->getPINGameMap());     // Oh Joy, these can change.
                }
            }

            // Add the lastest values to the events:
            // This code assumes that the order of PIN events above (statsNames) matches this.
            auto curPinEvent = pinStatsEventList.begin();
            auto curNumEntries = (*curPinEvent)->getSamplesFloat().size();
            {
                auto& stats = report.getReportStats();
                (*curPinEvent)->getSamplesFloat().push_back(stats.getDistProcTime());   ++curPinEvent;
                (*curPinEvent)->getSamplesFloat().push_back(stats.getDistWaitTime());   ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getIdpps());            ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getOdpps());            ++curPinEvent;
                (*curPinEvent)->getSamplesFloat().push_back(stats.getLpackLost());      ++curPinEvent;
                (*curPinEvent)->getSamplesFloat().push_back(stats.getRpackLost());      ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getIpps());             ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getOpps());             ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getJitter());           ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getLnakSent());         ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getRnakSent());         ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getLpSaved());          ++curPinEvent;
                (*curPinEvent)->getSamplesInt().push_back(stats.getLatency());          ++curPinEvent;
                //(*curPinEvent)->getSamplesInt().push_back(stats.getRpSaved());        ++curPinEvent;
            }

            // Send off the event when the values are full:  (+1 for newest entries)
            if (curNumEntries+1 >= getConfig().getConnStatsMaxEntries())
            {
                // Send the event off.  This should be fine?  Or does it need another thread too? 
                sendPINConnStatsEvent(gameSessionMaster, localConnGroupId, pinStatsEventList);
            }
        }

        return ERR_OK;
    }

    void GameManagerMasterImpl::sendPINConnStatsEvent(const GameSessionMaster* gameSessionMaster, ConnectionGroupId sourceConnGroupId, GameManagerMasterMetrics::ConnStatsPINEventList& pinStatsEventList)
    {
        if (getConfig().getDisableDetailedConnectionStats())
            return;

        // Each entry should have values set 
        if (pinStatsEventList.empty() || (pinStatsEventList.front()->getSamplesFloat().empty() && pinStatsEventList.front()->getSamplesInt().empty()))
            return;

        // Build the connection sessions -> event
        PlayerUserSessionIdList sourcePlayerSessionIdList;
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator connectionGroupPlayerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(sourceConnGroupId);
        const PlayerIdList* sourceConnectionGroupIdPlayerList = (connectionGroupPlayerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? connectionGroupPlayerIdListItr->second : nullptr;
        if (sourceConnectionGroupIdPlayerList != nullptr)
        {
            for (auto playerId : *sourceConnectionGroupIdPlayerList)
            {
                // The Player is either going to be someone in the Game, or the Dedicated Server host itself:
                UserSessionId sessionId = INVALID_USER_SESSION_ID;
                const PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(playerId);
                if (playerInfo != nullptr)
                    sessionId = playerInfo->getPlayerSessionId();
                else if (playerId == gameSessionMaster->getDedicatedServerHostBlazeId())
                    sessionId = gameSessionMaster->getDedicatedServerHostSessionId();

                // fill out the pairs list and pass that around. 
                sourcePlayerSessionIdList.push_back(eastl::make_pair(playerId, sessionId));
            }
        }
        
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        addPinEventToSubmission(gameSessionMaster, &sourcePlayerSessionIdList, true, nullptr, pinStatsEventList, pinSubmission);
        gUserSessionManager->sendPINEvents(pinSubmission);
    }

    /*! ************************************************************************************************/
    /*! \brief update connection qos metrics and events.
        \param[in,out] pinSubmission If not nullptr, update appropriate PIN event data in this param.
    ***************************************************************************************************/
    void GameManagerMasterImpl::updateConnectionQosMetrics(const GameDataMaster::ClientConnectionDetails& connDetails, GameId gameId, const TimeValue& termTime, PlayerRemovedReason reason, PINSubmission* pinSubmission)
    {
        const ConnectionGroupId sourceConnGroupId = connDetails.getSourceConnectionGroupId();
        const ConnectionGroupId targetConnGroupId = connDetails.getTargetConnectionGroupId();

        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            TRACE_LOG("[GameManagerMasterImpl].updateConnectionQosMetrics: Game id " << gameId << " not found.");
            return;
        }

        const char8_t* gameMode = gameSessionMaster->getGameMode();

        //We need to grab a player from the connection group to retrieve information such as client type, qos data. This information is the same across all players in the game.

        PlayerInfoMaster *sourcePlayerInfo = nullptr;
        PlayerInfoMaster *targetPlayerInfo = nullptr;

        PlayerIdList sourcePlayerAuditList;
        PlayerUserSessionIdList sourcePlayerSessionIdList;

        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator connectionGroupPlayerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(sourceConnGroupId);
        PlayerIdList* sourceConnectionGroupIdPlayerList = (connectionGroupPlayerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? connectionGroupPlayerIdListItr->second : nullptr;
        if (sourceConnectionGroupIdPlayerList == nullptr)
        {
            ASSERT_COND_LOG(sourceConnectionGroupIdPlayerList != nullptr, "[GameManagerMasterImpl:].updateConnectionQosMetrics could not find sourceConnGroupId in PlayerIdListByConnectionGroupId");
        }
        else
        {
            sourcePlayerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*sourceConnectionGroupIdPlayerList->begin());
            for (auto playerId : *sourceConnectionGroupIdPlayerList)
            {
                // The Player is either going to be someone in the Game, or the Dedicated Server host itself:
                UserSessionId sessionId = INVALID_USER_SESSION_ID;
                const PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(playerId);
                if (playerInfo != nullptr)
                    sessionId = playerInfo->getPlayerSessionId();
                else if (playerId == gameSessionMaster->getDedicatedServerHostBlazeId())
                    sessionId = gameSessionMaster->getDedicatedServerHostSessionId();

                // fill out the pairs list and pass that around. 
                sourcePlayerSessionIdList.push_back(eastl::make_pair(playerId, sessionId));

                if (isUserAudited(playerId))
                {
                    sourcePlayerAuditList.push_back(playerId);
                }   
            }
        }

        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator targetPlayerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(targetConnGroupId);
        PlayerIdList* targetConnectionGroupIdPlayerList = (targetPlayerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? targetPlayerIdListItr->second : nullptr;
        if (targetConnectionGroupIdPlayerList == nullptr)
        {
            ASSERT_COND_LOG(targetConnectionGroupIdPlayerList != nullptr, "[GameManagerMasterImpl:].updateConnectionQosMetrics could not find targetConnGroupId in PlayerIdListByConnectionGroupId");
        }
        else
        {
            targetPlayerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*targetConnectionGroupIdPlayerList->begin());
        }

        char8_t sourceCountry[3] = "";
        char8_t targetCountry[3] = "";
        if (sourcePlayerInfo != nullptr)
            LocaleTokenCreateCountryString(sourceCountry, sourcePlayerInfo->getAccountCountry()); // account country is not locale-related, but re-using locale macro for convenience
        if (targetPlayerInfo != nullptr)
            LocaleTokenCreateCountryString(targetCountry, targetPlayerInfo->getAccountCountry()); // account country is not locale-related, but re-using locale macro for convenience

        BlazeId sourceBlazeId = (sourcePlayerInfo == nullptr ? INVALID_BLAZE_ID : sourcePlayerInfo->getUserInfo().getUserInfo().getId());
        BlazeId targetBlazeId = (targetPlayerInfo == nullptr ? INVALID_BLAZE_ID : targetPlayerInfo->getUserInfo().getUserInfo().getId());

        ClientType sourceClientType = (sourcePlayerInfo == nullptr ? CLIENT_TYPE_INVALID : sourcePlayerInfo->getUserInfo().getClientType());
        ClientType targetClientType = (targetPlayerInfo == nullptr ? CLIENT_TYPE_INVALID : targetPlayerInfo->getUserInfo().getClientType());

        const Util::NetworkQosData* sourceQosData = (sourcePlayerInfo == nullptr ? nullptr : &sourcePlayerInfo->getQosData());
        const Util::NetworkQosData* targetQosData = (targetPlayerInfo == nullptr ? nullptr : &targetPlayerInfo->getQosData());

        InetAddress sourceAddr;
        InetAddress targetAddr;
        if (sourcePlayerInfo != nullptr)
        {
            sourceAddr.setHostnameAndPort(sourcePlayerInfo->getConnectionAddr());
        }
        if (targetPlayerInfo != nullptr)
        {
            targetAddr.setHostnameAndPort(targetPlayerInfo->getConnectionAddr());
        }

        const ClientMetrics* sourceMetrics = (sourcePlayerInfo == nullptr ? nullptr : sourcePlayerInfo->getClientMetrics());
        const ClientMetrics* targetMetrics = (targetPlayerInfo == nullptr ? nullptr : targetPlayerInfo->getClientMetrics());

        bool clientInitiatedConnection = (sourcePlayerInfo == nullptr ? false : !sourcePlayerInfo->getPlayerData()->getJoinedViaMatchmaking());

        // For dedicated server topologies, the host will not be in the player roster, so check if the host matches one of the playerIds
        if (sourceBlazeId == INVALID_BLAZE_ID || targetBlazeId == INVALID_BLAZE_ID)
        {
            if (gameSessionMaster->hasDedicatedServerHost())
            {
                const UserSessionInfo& sessionInfo = gameSessionMaster->getTopologyHostUserInfo();
                UserSessionId hostUserSessionId = sessionInfo.getSessionId();
                if (hostUserSessionId != INVALID_USER_SESSION_ID)
                {
                    if (sessionInfo.getConnectionGroupId() == sourceConnGroupId)
                    {
                        sourceClientType = CLIENT_TYPE_DEDICATED_SERVER;
                        sourceBlazeId = sessionInfo.getUserInfo().getId();
                        LocaleTokenCreateCountryString(sourceCountry, sessionInfo.getUserInfo().getAccountCountry()); // account country is not locale-related, but re-using locale macro for convenience
                        sourceQosData = &sessionInfo.getQosData();
                        sourceAddr.setHostnameAndPort(sessionInfo.getConnectionAddr());
                        sourceMetrics = &sessionInfo.getClientMetrics();
                        clientInitiatedConnection = (targetPlayerInfo == nullptr ? false : targetPlayerInfo->getPlayerData()->getJoinedViaMatchmaking()); // Dedicated server hosts never explicitly connect into a game
                                                                                                 // If the source is the dedicated server host, use the remote player's value
                                                                                                 // to determine how this connection made it into the game
                    }
                    else if (sessionInfo.getConnectionGroupId() == targetConnGroupId)
                    {
                        targetBlazeId = sessionInfo.getUserInfo().getId();
                        targetClientType = CLIENT_TYPE_DEDICATED_SERVER;
                        LocaleTokenCreateCountryString(targetCountry, sessionInfo.getUserInfo().getAccountCountry()); // account country is not locale-related, but re-using locale macro for convenience
                        targetQosData = &sessionInfo.getQosData();
                        targetAddr.setHostnameAndPort(sessionInfo.getConnectionAddr());
                        targetMetrics = &sessionInfo.getClientMetrics();
                    }
                }
            }
        }

        if (sourceBlazeId == INVALID_BLAZE_ID || targetBlazeId == INVALID_BLAZE_ID)
        {
            TRACE_LOG("[GameManagerMasterImpl].updateConnectionQosMetrics: Attempted to update metrics for player(s) that is/are not in game id " << gameId << ". Rejecting update.");
            return;
        }

        if (sourceConnGroupId == targetConnGroupId)
        {
            TRACE_LOG("[GameManagerMasterImpl].updateConnectionQosMetrics: source ConnectionGroupId( " << sourceConnGroupId <<") is equal to target ConnectionGroupId in game id " << gameId <<". Rejecting update.");
            return;
        }

        PingSiteAlias pingSite = gameSessionMaster->getBestPingSiteForConnection(sourceConnGroupId);

        GameNetworkTopology topology = gameSessionMaster->getGameNetworkTopology();

        // update connection result metrics
        incrementConnectionResultMetrics(connDetails, *gameSessionMaster, pingSite.c_str(), reason);

        // metrics below don't get updated for game group/voip connections
        if (!connDetails.getConnectionGame())
        {
            return;
        }

        const bool connDemangled = connDetails.getFlags().getConnectionDemangled();
        DemanglerConnectionHealthcheck demanglerConnectionHealthcheck = CONNECTION_WITHOUT;
        if (connDetails.getWasConnected())
        {
            demanglerConnectionHealthcheck = connDemangled ? CONNECTION_SUCCESS_WITH_DEMANGLER : CONNECTION_SUCCESS_WITHOUT_DEMANGLER;
        }
        else
        {
            demanglerConnectionHealthcheck = connDemangled ? CONNECTION_FAILURE_WITH_DEMANGLER : CONNECTION_FAILURE_WITHOUT_DEMANGLER;
        }
        
        incrementDemanglerHealthCheckCounter(connDetails, demanglerConnectionHealthcheck, topology, pingSite.c_str());

        AuditedUserData commonAuditData;
        bool userIsAudited = !sourcePlayerAuditList.empty();
        if (userIsAudited)
        {
            commonAuditData.setLocalIp(sourceAddr.getIp(InetAddress::HOST));
            commonAuditData.setLocalPort(sourceAddr.getPort(InetAddress::HOST));
            commonAuditData.setRemoteIp(targetAddr.getIp(InetAddress::HOST));
            commonAuditData.setRemotePort(targetAddr.getPort(InetAddress::HOST));
            commonAuditData.setGameId(gameId);
            commonAuditData.setGameMode(gameMode);
            commonAuditData.setClientInitiatedConnection(clientInitiatedConnection);
            commonAuditData.setLocalClientType(sourceClientType);
            commonAuditData.setLocalCountry(sourceCountry);
            commonAuditData.setLocalNatType(sourceQosData->getNatType());
            commonAuditData.setLocalRouterInfo(sourceMetrics->getDeviceInfo());
            commonAuditData.setRemoteClientType(targetClientType);
            commonAuditData.setRemoteCountry(targetCountry);
            commonAuditData.setRemoteNatType(targetQosData->getNatType());
            commonAuditData.setRemoteRouterInfo(targetMetrics->getDeviceInfo());
            commonAuditData.setTopology(topology);
            commonAuditData.setConnInitTime(connDetails.getConnInitTime());
            commonAuditData.setConnTermTime(termTime);
            commonAuditData.setConnTermReason(reason);
        }

        DisconnectedEndpointData data;
        data.setGameNetworkTopology(topology);
        data.setPingSite(pingSite.c_str());
        data.setCountry(sourceCountry);
        data.setGameMode(gameMode);
        data.setClientType(sourceClientType);
        data.setClientInitiatedConnection(clientInitiatedConnection);
        data.setQosRuleEnabled(connDetails.getQosEnabled());
        data.setDisconnectReason(reason);
        data.setConnectionStatus(demanglerConnectionHealthcheck);
        data.setLocalIp(sourceAddr.getIp(InetAddress::HOST));
        data.setLocalPort(sourceAddr.getPort(InetAddress::HOST));
        data.setRemoteIp(targetAddr.getIp(InetAddress::HOST));
        data.setRemotePort(targetAddr.getPort(InetAddress::HOST));
        data.setJoinType(connDetails.getJoinType());
        data.setImpacting(gameSessionMaster->getPlayerConnectionMesh()->isConnectionPlayerImpacting(connDetails) ? IMPACTING : NONIMPACTING);
        data.setConnectionConciergeMode(connDetails.getCCSInfo().getConnConciergeMode());

        GameManagerMasterMetrics::ConnectionGroupIdPair connGroupPair(sourceConnGroupId, targetConnGroupId);

        // Start building the PIN connection event. We'll finish populating the event based on if the connection was successful and passed QoS validation or not.
        eastl::list<RiverPoster::ConnectionStatsEventPtr> pinStatsEventList = mMetrics.mConnectionStatsEventListByConnGroup[gameId][connGroupPair];  // empty
        RiverPoster::ConnectionEventPtr connectionEvent = nullptr;
        if (pinSubmission != nullptr)
        {
            // only generating an event for the source conn group, but if this is MLU, multiple events will be generated for the source, identical except for the header core
            connectionEvent = BLAZE_NEW RiverPoster::ConnectionEvent;

            eastl::string hostname = sourceAddr.getHostname();
            connectionEvent->getHeaderCore().setEventName(RiverPoster::CONNECTION_EVENT_NAME);
            connectionEvent->getHeaderCore().setClientIp(hostname.c_str());
            connectionEvent->getHeaderCore().setGameType(gameSessionMaster->getPINGameType());
            connectionEvent->getHeaderCore().setModeType(gameSessionMaster->getPINGameModeType());
            connectionEvent->getHeaderCore().setGameMode(gameMode);
            if (*gameSessionMaster->getPINGameMap() != '\0')
                connectionEvent->getHeaderCore().setMap(gameSessionMaster->getPINGameMap());

            eastl::string gameIdString;
            gameIdString.sprintf("%" PRIu64, gameId);
            connectionEvent->setGameId(gameIdString.c_str());
            connectionEvent->setPlayerPingSite(pingSite.c_str());
            connectionEvent->setTargetIpAddress(targetAddr.getHostname());
            PingSiteAlias targetPingSite = ((targetPlayerInfo != nullptr) ? targetPlayerInfo->getUserInfo().getBestPingSiteAlias() : gameSessionMaster->getBestPingSiteAlias());
            if (EA_UNLIKELY(targetPingSite.empty()))
            {
                targetPingSite = UNKNOWN_PINGSITE;
            }
            connectionEvent->setTargetPingSite(targetPingSite);

            PingSiteAlias gamePingSite = gameSessionMaster->getBestPingSiteAlias();
            if (EA_UNLIKELY(gamePingSite.empty()))
            {
                gamePingSite = UNKNOWN_PINGSITE;
            }
            connectionEvent->setGamePingSite(gamePingSite);

            connectionEvent->setNetworkTopology(GameNetworkTopologyToString(topology));
            // even though conn group members could join after the fact (and therefore could have 'direct joined' after the leader matchmade),
            // we're going to use the same join method for the entire conn group
            if (sourcePlayerInfo != nullptr)
            {
                switch (sourcePlayerInfo->getConnectionJoinType())
                {
                case CREATED:
                    connectionEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_CREATED);
                    break;
                case DIRECTJOIN:
                    connectionEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_DIRECT);
                    break;
                case MATCHMAKING:
                    connectionEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_MATCHMAKING);
                    break;
                default:
                    // don't set if we don't know what we had
                    break;
                }
            }

            connectionEvent->setClientType(ClientTypeToPinString(sourceClientType));
            connectionEvent->setLeaveReason(PlayerRemovedReasonToString(reason));
            connectionEvent->setConnectionType(connDetails.getConnectionGame() ? RiverPoster::CONNECTION_TYPE_GAME : RiverPoster::CONNECTION_TYPE_VOIP);
            if (connDetails.getFlags().getConnectionDemangled())
            {
                connectionEvent->setConnectionTechnology(RiverPoster::CONNECTION_TECHNOLOGY_DEMANGLER);
            }
            else if(connDetails.getCCSInfo().getConnConciergeMode() != CC_UNUSED)
            {
                connectionEvent->setConnectionTechnology(RiverPoster::CONNECTION_TECHNOLOGY_CONCIERGE);
                connectionEvent->setCcsPingSite(gameSessionMaster->getCcsPingSite());
            }
            else
            {
                connectionEvent->setConnectionTechnology(RiverPoster::CONNECTION_TECHNOLOGY_NORMAL);
            }

            connectionEvent->setImpacting(gameSessionMaster->getPlayerConnectionMesh()->isConnectionPlayerImpacting(connDetails));
        }

        if (!connDetails.getWasConnected())
        {
            // Submit what information we have
            addPinEventToSubmission(&*gameSessionMaster, &sourcePlayerSessionIdList, false, connectionEvent, pinStatsEventList, pinSubmission);

            gEventManager->submitBinaryEvent(static_cast<uint32_t>(GameManagerMetricsMaster::EVENT_ENDPOINTDISCONNECTEDEVENT), data);

            // Check if any source players are being audited
            if (userIsAudited)
            {
                Fiber::CreateParams params;
                params.groupId = mAuditUserDataGroupId;
                params.namedContext = "GameManagerMasterImpl::addConnectionMetricsToDb";
                PlayerIdList::iterator auditPlayerListItr =  sourcePlayerAuditList.begin();
                for (; auditPlayerListItr != sourcePlayerAuditList.end(); ++auditPlayerListItr)
                {
                    AuditedUserDataPtr auditData = BLAZE_NEW AuditedUserData;
                    commonAuditData.copyInto(*auditData);
                    auditData->setBlazeId(*auditPlayerListItr);
                    gFiberManager->scheduleCall<GameManagerMasterImpl, const AuditedUserDataPtr>(this, &GameManagerMasterImpl::addConnectionMetricsToDb, auditData, params);
                }
            }
            return; // If we never connected, we won't have any latency or packet loss data to process
        }

        // If this connection was rejected due to the MM QoS rule, use the qos data from MM for the metrics
        // instead of relying on telemetry data that may or may not be reported by the clients
        if (connDetails.getQosEnabled() && reason == PLAYER_CONN_POOR_QUALITY)
        {
            const uint16_t latencyIndex = UserSessionManager::getLatencyIndex(connDetails.getLatency());
            const uint8_t packetLossIndex = (uint8_t)(ceil(connDetails.getPacketLoss()));
            const auto quantizedLatency = getQuantizedLatency(latencyIndex);
            const auto quantizedPacketLoss = getQuantizedPacketLoss(packetLossIndex);
            mMetrics.mConnectionLatencySampleQosRule.increment(1, quantizedLatency, topology, pingSite.c_str());
            data.getTotalLatencySample().push_back(latencyIndex);
            mMetrics.mMaxConnectionLatencyQosRule.increment(1, quantizedLatency, topology, pingSite.c_str());
            data.setTotalLatencyMax((int64_t)connDetails.getLatency());
            mMetrics.mAvgConnectionLatencyQosRule.increment(1, quantizedLatency, topology, pingSite.c_str());
            data.setTotalLatencyAvg((int64_t)connDetails.getLatency());
            mMetrics.mConnectionPacketLossQosRule.increment(1, quantizedPacketLoss, topology, pingSite.c_str());
            data.setTotalPacketLoss((int16_t)connDetails.getPacketLoss());
            mMetrics.mConnectionPacketLossSampleQosRule.increment(1, quantizedPacketLoss, topology, pingSite.c_str());
            data.getTotalPacketLossSample().push_back(packetLossIndex);

            if (userIsAudited)
            {
                commonAuditData.setMaxLatency((int64_t)connDetails.getLatency());
                commonAuditData.setMinLatency((int64_t)connDetails.getLatency());
                commonAuditData.setAverageLatency((int64_t)connDetails.getLatency());
                commonAuditData.setPacketLoss((int16_t)connDetails.getPacketLoss());
            }

            if (pinSubmission != nullptr)
            {
                connectionEvent->setAverageLatency((float)connDetails.getLatency());
                connectionEvent->setMaximumLatency((float)connDetails.getLatency());
                connectionEvent->setPacketLoss(connDetails.getPacketLoss());
            }

            // Clean up any telemetry report data that may have been sent up
            {
                GameManagerMasterMetrics::LatencyByConnGroupByGameIdMap& gameIdMap = mMetrics.mLatencyByConnGroups;
                GameManagerMasterMetrics::LatencyByConnGroupByGameIdMap::iterator gameIdIter = gameIdMap.find(gameId);
                if (gameIdIter != gameIdMap.end())
                {
                    GameManagerMasterMetrics::LatencyByConnGroupMap &connGroupMap = gameIdIter->second;
                    connGroupMap.erase(connGroupPair);
                    if (connGroupMap.empty())
                    {
                        gameIdMap.erase(gameIdIter);
                    }
                }
            }

            {
                GameManagerMasterMetrics::PacketLossByConnGroupByGameIdMap& gameIdMap = mMetrics.mCurrentPacketLossByConnGroup;
                GameManagerMasterMetrics::PacketLossByConnGroupByGameIdMap::iterator gameIdIter = gameIdMap.find(gameId);
                if (gameIdIter != gameIdMap.end())
                {
                    GameManagerMasterMetrics::PacketLossByConnGroupMap &connGroupMap = gameIdIter->second;
                    GameManagerMasterMetrics::PacketLossByConnGroupMap::iterator connGroupIter = connGroupMap.find(connGroupPair);
                    if (connGroupIter != connGroupMap.end())
                    {
                        mMetrics.mCurrentPacketLoss.decrement(1, connGroupIter->second.mCurrentPacketLossCeil, topology, pingSite.c_str());

                        connGroupMap.erase(connGroupIter);
                        if (connGroupMap.empty())
                        {
                            gameIdMap.erase(gameIdIter);
                        }
                    }
                }
            }

            addPinEventToSubmission(&*gameSessionMaster, &sourcePlayerSessionIdList, true, connectionEvent, pinStatsEventList, pinSubmission);

            gEventManager->submitBinaryEvent(static_cast<uint32_t>(GameManagerMetricsMaster::EVENT_ENDPOINTDISCONNECTEDEVENT), data);

            // Check if any source players are being audited
            if (userIsAudited)
            {
                Fiber::CreateParams params;
                params.namedContext = "GameManagerMasterImpl::addConnectionMetricsToDb2";
                params.groupId = mAuditUserDataGroupId;
                PlayerIdList::iterator auditPlayerListItr =  sourcePlayerAuditList.begin();
                for (; auditPlayerListItr != sourcePlayerAuditList.end(); ++auditPlayerListItr)
                {
                    AuditedUserDataPtr auditData = BLAZE_NEW AuditedUserData;
                    commonAuditData.copyInto(*auditData);
                    auditData->setBlazeId(*auditPlayerListItr);
                    gFiberManager->scheduleCall<GameManagerMasterImpl, const AuditedUserDataPtr>(this, &GameManagerMasterImpl::addConnectionMetricsToDb, auditData, params);
                }
            }
            return;
        }
        
        {
            GameManagerMasterMetrics::LatencyByConnGroupByGameIdMap& gameIdMap = mMetrics.mLatencyByConnGroups;
            GameManagerMasterMetrics::LatencyByConnGroupByGameIdMap::iterator gameIdIter = gameIdMap.find(gameId);
            if (gameIdIter != gameIdMap.end())
            {
                GameManagerMasterMetrics::LatencyByConnGroupMap &connGroupMap = gameIdIter->second;
                GameManagerMasterMetrics::LatencyByConnGroupMap::iterator connGroupIter = connGroupMap.find(connGroupPair);
                if (connGroupIter != connGroupMap.end())
                {
                    const uint16_t maxLatencyIndex = UserSessionManager::getLatencyIndex(connGroupIter->second.mMaxLatency);
                    const auto quantizedMaxLatency = getQuantizedLatency(maxLatencyIndex);
                    mMetrics.mMaxConnectionLatency.increment(1, quantizedMaxLatency, topology, pingSite.c_str());
                    data.setTotalLatencyMax((int64_t)connGroupIter->second.mMaxLatency);
                    if (userIsAudited)
                    {
                        commonAuditData.setMaxLatency((int64_t)connGroupIter->second.mMaxLatency);
                        commonAuditData.setMinLatency((int64_t)connGroupIter->second.mMinLatency);
                    }

                    const uint32_t avgLatency = (uint32_t)(ceill((long double)connGroupIter->second.mTotalLatency / (long double)connGroupIter->second.mLatencyCount));
                    const uint16_t avgLatencyIndex = UserSessionManager::getLatencyIndex(avgLatency);
                    const auto quantizedAvgLatency = getQuantizedLatency(avgLatencyIndex);
                    mMetrics.mAvgConnectionLatency.increment(1, quantizedAvgLatency, topology, pingSite.c_str());
                    data.setTotalLatencyAvg((int64_t)avgLatency);
                    if (userIsAudited)
                        commonAuditData.setAverageLatency((int64_t)avgLatency);

                    // PIN DATA
                    if (pinSubmission != nullptr)
                    {
                        connectionEvent->setAverageLatency((float)avgLatency);
                        connectionEvent->setMaximumLatency((float)connGroupIter->second.mMaxLatency);
                    }

                    data.getTotalLatencySample().reserve(MAX_NUM_LATENCY_BUCKETS);
                    GameManagerMasterMetrics::LatencyArray::const_iterator latencyIter = connGroupIter->second.mTotalSampleLatencies.begin();
                    GameManagerMasterMetrics::LatencyArray::const_iterator latencyEnd = connGroupIter->second.mTotalSampleLatencies.end();
                    uint16_t latencyIndex = 0;
                    while (latencyIter != latencyEnd)
                    {
                        if (*latencyIter > 0)
                        {
                            const auto quantizedLatency = getQuantizedLatency(latencyIndex);
                            mMetrics.mConnectionLatencySample.increment(*latencyIter, quantizedLatency, topology, pingSite.c_str());
                        }
                        latencyIndex += LATENCY_BUCKET_SIZE;
                        data.getTotalLatencySample().push_back(*latencyIter);
                        ++latencyIter;
                    }

                    connGroupMap.erase(connGroupIter);
                    if (connGroupMap.empty())
                    {
                        gameIdMap.erase(gameIdIter);
                    }
                }
            }
        }

        {
            GameManagerMasterMetrics::PacketLossByConnGroupByGameIdMap& gameIdMap = mMetrics.mCurrentPacketLossByConnGroup;
            GameManagerMasterMetrics::PacketLossByConnGroupByGameIdMap::iterator gameIdIter = gameIdMap.find(gameId);
            if (gameIdIter != gameIdMap.end())
            {
                GameManagerMasterMetrics::PacketLossByConnGroupMap &connGroupMap = gameIdIter->second;
                GameManagerMasterMetrics::PacketLossByConnGroupMap::iterator connGroupIter = connGroupMap.find(connGroupPair);
                if (connGroupIter != connGroupMap.end())
                {
                    const auto quantizedPacketLossCeil = getQuantizedPacketLoss(connGroupIter->second.mCurrentPacketLossCeil);
                    mMetrics.mCurrentPacketLoss.decrement(1, quantizedPacketLossCeil, topology, pingSite.c_str());
                    mMetrics.mConnectionPacketLoss.increment(1, quantizedPacketLossCeil, topology, pingSite.c_str());
                    data.setTotalPacketLoss((int16_t)connGroupIter->second.mCurrentPacketLossCeil);
                    if (userIsAudited)
                        commonAuditData.setPacketLoss((int16_t)connGroupIter->second.mCurrentPacketLossCeil);

                    // PIN DATA
                    if (pinSubmission != nullptr)
                    {
                        connectionEvent->setPacketLoss(connGroupIter->second.mCurrentPacketLoss);
                    }

                    data.getTotalPacketLossSample().reserve(GameManagerMasterMetrics::PACKET_LOSS_ARRAY_SIZE);
                    GameManagerMasterMetrics::PacketLossArray::const_iterator packetLossIter = connGroupIter->second.mTotalSamplePacketLosses.begin();
                    GameManagerMasterMetrics::PacketLossArray::const_iterator packetLossEnd = connGroupIter->second.mTotalSamplePacketLosses.end();
                    uint16_t packetLoss = 0; // This always pushes PACKET_LOSS_ARRAY_SIZE values, even though many may be 0
                    while (packetLossIter != packetLossEnd)
                    {
                        if (*packetLossIter > 0)
                        {
                            const auto quantizedPacketLoss = getQuantizedPacketLoss(packetLoss);
                            mMetrics.mConnectionPacketLossSample.increment(*packetLossIter, quantizedPacketLoss, topology, pingSite.c_str());
                        }
                        ++packetLoss;
                        data.getTotalPacketLossSample().push_back(*packetLossIter);
                        ++packetLossIter;
                    }

                    connGroupMap.erase(connGroupIter);
                    if (connGroupMap.empty())
                    {
                        gameIdMap.erase(gameIdIter);
                    }
                }
            }
        }

        addPinEventToSubmission(&*gameSessionMaster, &sourcePlayerSessionIdList, true, connectionEvent, pinStatsEventList, pinSubmission);

        gEventManager->submitBinaryEvent(static_cast<uint32_t>(GameManagerMetricsMaster::EVENT_ENDPOINTDISCONNECTEDEVENT), data);

        // Check if any source players are being audited
        if (userIsAudited)
        {
            Fiber::CreateParams params;
            params.groupId = mAuditUserDataGroupId;
            params.namedContext = "GameManagerMasterImpl::addConnectionMetricsToDb3";
            PlayerIdList::iterator auditPlayerListItr =  sourcePlayerAuditList.begin();
            for (; auditPlayerListItr != sourcePlayerAuditList.end(); ++auditPlayerListItr)
            {
                AuditedUserDataPtr auditData = BLAZE_NEW AuditedUserData;
                commonAuditData.copyInto(*auditData);
                auditData->setBlazeId(*auditPlayerListItr);
                gFiberManager->scheduleCall<GameManagerMasterImpl, const AuditedUserDataPtr>(this, &GameManagerMasterImpl::addConnectionMetricsToDb, auditData, params);
            }
        }
    }

    AddUsersToConnectionMetricAuditMasterError::Error GameManagerMasterImpl::processAddUsersToConnectionMetricAuditMaster(const Blaze::GameManager::UserAuditInfoMasterRequest &request,
                                                                                                                          const Message* message)
    {
        if (mAuditedUsers.size() + request.getBlazeIds().size() > getConfig().getMaxAuditedUsers())
        {
            WARN_LOG("[GameManagerMasterImpl].processAddUsersToConnectionMetricAuditMaster: Maximum number of users to audit reached - unable to add more users.");
            return AddUsersToConnectionMetricAuditMasterError::ERR_SYSTEM;
        }

            // Does user have permission to add users to the audit list
        if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_CREATE_AUDIT))
        {
            ERR_LOG("[GameManagerMasterImpl].processAddUsersToConnectionMetricAuditMaster: User does not have permission to add user to connection metric audit.");
            return AddUsersToConnectionMetricAuditMasterError::commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
        }

        if (mAuditedUsers.size() + request.getBlazeIds().size() > getConfig().getMaxAuditedUsers())
        {
            WARN_LOG("[GameManagerMasterImpl].processAddUsersToConnectionMetricAuditMaster: Maximum number of users to audit reached - unable to add more users.");
            return AddUsersToConnectionMetricAuditMasterError::ERR_SYSTEM;
        }

        BlazeRpcError err = updateConnectionMetricAuditList(request, true);

        return AddUsersToConnectionMetricAuditMasterError::commandErrorFromBlazeError(err);
    }

    RemoveUsersFromConnectionMetricAuditMasterError::Error GameManagerMasterImpl::processRemoveUsersFromConnectionMetricAuditMaster(const Blaze::GameManager::UserAuditInfoMasterRequest &request,
                                                                                                                                    const Message* message)
    {
        // Does user have permission to add users to the audit list
        if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_CANCEL_AUDIT))
        {
            ERR_LOG("[GameManagerMasterImpl].processRemoveUsersFromConnectionMetricAuditMaster: User does not have permission to remove user from connection metric audit.");
            return RemoveUsersFromConnectionMetricAuditMasterError::commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
        }

        BlazeRpcError err = updateConnectionMetricAuditList(request, false);

        return RemoveUsersFromConnectionMetricAuditMasterError::commandErrorFromBlazeError(err);
    }

    BlazeRpcError GameManagerMasterImpl::updateConnectionMetricAuditList(const Blaze::GameManager::UserAuditInfoMasterRequest &request, bool doAdd)
    {
        TimeValue expiration = request.getExpiration();
        if (expiration != 0) // User has requested this audit expires
            expiration += TimeValue::getTimeOfDay();

        BlazeIdList::const_iterator idIter = request.getBlazeIds().begin();
        BlazeIdList::const_iterator idEnd = request.getBlazeIds().end();
        ConnectionMetricAuditMap tempAudits(BlazeStlAllocator("GameManagerMasterImpl::tempAudits", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        for (; idIter != idEnd; ++idIter)
        {
            BlazeId id = *idIter;
            ConnectionMetricAuditMap::iterator iter = mAuditedUsers.find(id);
            if (doAdd)
            {
                if (iter == mAuditedUsers.end())
                {
                    tempAudits[id] = expiration;
                }
                else
                {
                    INFO_LOG("[GameManagerMasterImpl].updateConnectionMetricAuditList: User " << id << " is already audited.");
                    return GAMEMANAGER_ERR_USER_ALREADY_AUDITED;
                }
            }
            else
            {
                if (iter != mAuditedUsers.end())
                {
                    tempAudits.insert(id);
                }
                else
                {
                    INFO_LOG("[GameManagerMasterImpl].updateConnectionMetricAuditList: User " << id << " is currently not audited.");
                    return GAMEMANAGER_ERR_USER_NOT_FOUND;
                }
            }
        }

        if (!tempAudits.empty())
        {
            // tempAudits is unusable after this call (data is swapped out)
            Fiber::CreateParams params;
            params.namedContext = "GameManagerMasterImpl::addConnectionMetricsToDb4";
            gFiberManager->scheduleCall<GameManagerMasterImpl, ConnectionMetricAuditMap&, bool>(this, &GameManagerMasterImpl::updateConnectionMetricAuditDb,
                tempAudits, doAdd, params);
        }

        
        return ERR_OK;
    }

    void GameManagerMasterImpl::updateConnectionMetricAuditDb(ConnectionMetricAuditMap& inputData, bool doAdd)
    {
        ConnectionMetricAuditMap data(BlazeStlAllocator("GameManagerMasterImpl::ConnectionMetricAuditMap", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        inputData.swap(data);

        DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
        if (conn == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl].updateConnectionMetricAuditList: Unable to obtain a database connection.");
            return;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);

        ConnectionMetricAuditMap tempAudits(BlazeStlAllocator("GameManagerMasterImpl::ConnectionMetricAuditMapdata", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        if (doAdd)
        {
            query->append("INSERT INTO `gm_user_connection_audit` (`blazeId`,`expiration`) VALUES ");
            ConnectionMetricAuditMap::const_iterator begin = data.begin();
            ConnectionMetricAuditMap::const_iterator end = data.end();
            for (; begin != end; ++begin)
            {
                query->append("($D, $D),", begin->first, begin->second.getMicroSeconds());
                tempAudits.insert(eastl::make_pair(begin->first, begin->second));
            }
            query->trim(1);
            query->append(" ON DUPLICATE KEY UPDATE `expiration`=VALUES(`expiration`)");
        }
        else
        {
            query->append("UPDATE `gm_user_connection_audit` SET `expiration`=$D WHERE `blazeId` IN (", TimeValue::getTimeOfDay().getMicroSeconds());
            ConnectionMetricAuditMap::const_iterator begin = data.begin();
            ConnectionMetricAuditMap::const_iterator end = data.end();
            for (; begin != end; ++begin)
            {
                query->append("$D,", begin->first);
                tempAudits.insert(begin->first);
            }
            query->trim(1);
            query->append(")");
        }

        DbResultPtr result;
        DbError dbErr = conn->executeQuery(query, result);
        if (dbErr != DB_ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].updateConnectionMetricAuditList: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return;
        }

        if (doAdd)
            mAuditedUsers.insert(tempAudits.begin(), tempAudits.end());
        else
        {
            ConnectionMetricAuditMap::iterator iter = tempAudits.begin();
            ConnectionMetricAuditMap::iterator end = tempAudits.end();
            for (; iter != end; ++iter)
                mAuditedUsers.erase(iter->first);
        }
    }

    FetchAuditedUsersMasterError::Error GameManagerMasterImpl::processFetchAuditedUsersMaster(Blaze::GameManager::FetchAuditedUsersResponse& response, const Message* message)
    {
        ConnectionMetricAuditMap::const_iterator iter = mAuditedUsers.begin();
        for (; iter != mAuditedUsers.end(); ++iter)
        {
            GameManager::AuditedUser* user = response.getAuditedUsers().pull_back();
            user->setExpiration(iter->second);
            // rest of core identification populated at slave
            user->getCoreIdentification().setBlazeId(iter->first);
        }
        return FetchAuditedUsersMasterError::ERR_OK;
    }

    void GameManagerMasterImpl::addConnectionMetricsToDb(const Blaze::GameManager::AuditedUserDataPtr data)
    {
        DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
        if (conn == nullptr)
        {
            ERR_LOG("[GameManagerMasterImpl].addConnectionMetricsToDb: Unable to obtain a database connection.");
            return;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("INSERT INTO `gm_user_connection_metrics` (`blazeid`, `gameid`, `local_ip`, `local_port`, `local_client_type`, `local_locale`, `remote_ip`, `remote_port`, `remote_client_type`, `remote_locale`, "
            "`game_mode`, `topology`, `max_latency`, `avg_latency`, `min_latency`, `packet_loss`, `conn_term_reason`, `conn_init_time`, `conn_term_time`, `client_init_conn`, `qos_rule_enabled`, `local_nat_type`, `remote_nat_type`, "
            "`local_router_info`, `remote_router_info`) VALUES ($D, $D, $u, $u, $u, '$s', $u, $u, $u, '$s', '$s', $u, $d, $d, $d, $d, $u, $D, $D, $u, $u, $u, $u, '$s', '$s')",
            data->getBlazeId(), data->getGameId(), data->getLocalIp(), data->getLocalPort(), data->getLocalClientType(), data->getLocalCountry(),
            data->getRemoteIp(), data->getRemotePort(), data->getRemoteClientType(), data->getRemoteCountry(),
            data->getGameMode(), data->getTopology(), data->getMaxLatency(), data->getAverageLatency(), data->getMinLatency(), data->getPacketLoss(),
            data->getConnTermReason(), data->getConnInitTime().getMicroSeconds(), data->getConnTermTime().getMicroSeconds(), data->getClientInitiatedConnection(), data->getQosRuleEnabled(),
            data->getLocalNatType(), data->getRemoteNatType(), data->getLocalRouterInfo(), data->getRemoteRouterInfo());

        DbError dbErr = conn->executeQuery(query);
        if (dbErr != DB_ERR_OK)
        {
            ERR_LOG("[GameManagerMasterImpl].addConnectionMetricsToDb: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return;
        }
    }

    void GameManagerMasterImpl::addConnectionGroupJoinFailureMap(ConnectionGroupId connectionGroupId, GameId gameId)
    {
        ConnectionGroupJoinFailureMap::iterator itr = mConnectionGroupJoinFailureMap.find(connectionGroupId);
        if (itr == mConnectionGroupJoinFailureMap.end())
        {
            ConnectionGroupJoinFailureInfo connectionGroupJoinFailureInfo;
            connectionGroupJoinFailureInfo.mExpiryTime = TimeValue::getTimeOfDay() + getConfig().getConnectionGroupJoinFailureMapExpiry();
            connectionGroupJoinFailureInfo.mGameId = gameId;
            mConnectionGroupJoinFailureMap[connectionGroupId] = connectionGroupJoinFailureInfo;
            startConnectionGroupJoinFailureTimer();
        }
    }

    void GameManagerMasterImpl::cleanConnectionGroupJoinFailureMap()
    {
        TimeValue now = TimeValue::getTimeOfDay();
        ConnectionGroupJoinFailureMap::iterator itr = mConnectionGroupJoinFailureMap.begin();
        ConnectionGroupJoinFailureMap::iterator end = mConnectionGroupJoinFailureMap.end();
        while(itr != end)
        {
            if (now >= itr->second.mExpiryTime)
            {
                PktReceivedMetricsByGameIdMap::iterator metricsItr = mPktReceivedMetricsByGameIdMap.find(itr->second.mGameId);
                PktReceivedMetrics* pktReceivedMetrics = nullptr;
                if (metricsItr == mPktReceivedMetricsByGameIdMap.end())
                {
                    pktReceivedMetrics = mPktReceivedMetricsByGameIdMap.allocate_element();
                    mPktReceivedMetricsByGameIdMap[itr->second.mGameId] = pktReceivedMetrics;
                }
                else
                {
                    pktReceivedMetrics = metricsItr->second;
                }

                if (!itr->second.mMeshDisconnectReceived.getDedicatedServer())
                {
                    pktReceivedMetrics->setDedicatedServerHostNoResp(pktReceivedMetrics->getDedicatedServerHostNoResp() + 1);
                    TRACE_LOG("[GameManagerMasterImpl].cleanConnectionGroupJoinFailureMap - blazeserver did not receive pkt received information for client (" << itr->first << ") from dedicated server for game id ("
                        << itr->second.mGameId << ")");
                }

                if (!itr->second.mMeshDisconnectReceived.getClient())
                {
                    pktReceivedMetrics->setClientNoResp(pktReceivedMetrics->getClientNoResp() + 1);
                    TRACE_LOG("[GameManagerMasterImpl].cleanConnectionGroupJoinFailureMap - blazeserver did not receive pkt received information for the dedicated server from client (" << itr->first << ")  for game id ("
                        << itr->second.mGameId << ")");
                }

                itr = mConnectionGroupJoinFailureMap.erase(itr);
            }
            else
            {
                itr++;
            }
        }

        if (mConnectionGroupJoinFailureMap.empty())
        {
            cancelConnectionGroupJoinFailureTimer();
        }
        else
        {
            resetConnectionGroupJoinFailureTimer();
        }
    }

    void GameManagerMasterImpl::startConnectionGroupJoinFailureTimer()
    {
        // we don't support multiple timers at a time
        if (mConnectionGroupJoinFailureTimer == INVALID_TIMER_ID)
        {
            mConnectionGroupJoinFailureTimer = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + getConfig().getConnectionGroupJoinFailureMapExpiry(),
                this, &GameManagerMasterImpl::cleanConnectionGroupJoinFailureMap,
                "GameManagerMasterImpl::cleanConnectionGroupJoinFailureMap");
        }
    }

    void GameManagerMasterImpl::cancelConnectionGroupJoinFailureTimer()
    {
        if (mConnectionGroupJoinFailureTimer != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mConnectionGroupJoinFailureTimer);
            mConnectionGroupJoinFailureTimer = INVALID_TIMER_ID;
        }
    }

    void GameManagerMasterImpl::resetConnectionGroupJoinFailureTimer()
    {
        cancelConnectionGroupJoinFailureTimer();
        startConnectionGroupJoinFailureTimer();
    }

    void GameManagerMasterImpl::updatePktReceivedMetrics(GameSessionMasterPtr gameSessionMaster, const Blaze::GameManager::UpdateMeshConnectionMasterRequest &request)
    {
        bool dedicatedServerSource = request.getSourceConnectionGroupId() == gameSessionMaster->getDedicatedServerHostInfo().getConnectionGroupId();
        ConnectionGroupJoinFailureMap::iterator connectionGroupItr;
        if(dedicatedServerSource)
        {
            connectionGroupItr = mConnectionGroupJoinFailureMap.find(request.getTargetConnectionGroupId());
            if (connectionGroupItr == mConnectionGroupJoinFailureMap.end())
            {
                // removed connection group id does not exist in connection group join failure map. No need to update metrics.
                return;
            }
            else
            {
                if (connectionGroupItr->second.mMeshDisconnectReceived.getDedicatedServer())
                {
                    // dedicated server already sent mesh update notification. Exit early.
                    return;
                }
                else
                {
                    connectionGroupItr->second.mMeshDisconnectReceived.setDedicatedServer();
                }
            }
        }
        else
        {
            connectionGroupItr = mConnectionGroupJoinFailureMap.find(request.getSourceConnectionGroupId());
            if (connectionGroupItr == mConnectionGroupJoinFailureMap.end())
            {
                // removed connection group id does not exist in connection group join failure map. No need to update metrics. 
                return;
            }
            else
            {
                if (connectionGroupItr->second.mMeshDisconnectReceived.getClient())
                {
                    // leaving client already sent mesh update notification. Exit early.
                    return;
                }
                else
                {
                    connectionGroupItr->second.mMeshDisconnectReceived.setClient();
                }
            }
        }
        
        PktReceivedMetricsByGameIdMap::iterator itr = mPktReceivedMetricsByGameIdMap.find(gameSessionMaster->getGameId());
        PktReceivedMetrics* pktReceivedMetrics = nullptr;
        if (itr == mPktReceivedMetricsByGameIdMap.end())
        {
            pktReceivedMetrics = mPktReceivedMetricsByGameIdMap.allocate_element();
            mPktReceivedMetricsByGameIdMap[gameSessionMaster->getGameId()] = pktReceivedMetrics;
        }
        else
        {
            pktReceivedMetrics = itr->second;
        }

        if (dedicatedServerSource)
        {
            if (request.getPlayerNetConnectionFlags().getConnectionPktReceived())
            {
                pktReceivedMetrics->setDedicatedServerHostPktReceived(pktReceivedMetrics->getDedicatedServerHostPktReceived() + 1);
                TRACE_LOG("[GameManagerMasterImpl].updatePktReceivedMetrics dedicated server(" << request.getSourceConnectionGroupId() << ") received packets from client (" 
                    << request.getTargetConnectionGroupId() <<") when attempting to establish a connection for game id(" << gameSessionMaster->getGameId() << ")");
            }
            else
            {
                pktReceivedMetrics->setDedicatedServerHostNoPktReceived(pktReceivedMetrics->getDedicatedServerHostNoPktReceived() + 1);
                TRACE_LOG("[GameManagerMasterImpl].updatePktReceivedMetrics dedicated server(" << request.getSourceConnectionGroupId() << ") did not receive packets from client (" 
                    << request.getTargetConnectionGroupId() <<") when attempting to establish a connection for game id(" << gameSessionMaster->getGameId() << ")");
            }
        }
        else
        {
            if (request.getPlayerNetConnectionFlags().getConnectionPktReceived())
            {
                pktReceivedMetrics->setClientPktReceived(pktReceivedMetrics->getClientPktReceived() + 1);
                TRACE_LOG("[GameManagerMasterImpl].updatePktReceivedMetrics client (" << request.getSourceConnectionGroupId() << ") received packets from the dedicated server (" 
                    << request.getTargetConnectionGroupId() <<") when attempting to establish a connection for game id(" << gameSessionMaster->getGameId() << ")");
            }
            else
            {
                pktReceivedMetrics->setClientNoPktReceived(pktReceivedMetrics->getClientNoPktReceived() + 1);
                TRACE_LOG("[GameManagerMasterImpl].updatePktReceivedMetrics client (" << request.getSourceConnectionGroupId() << ") did not receive packets from dedicated server (" 
                    << request.getTargetConnectionGroupId() <<") when attempting to establish a connection for game id(" << gameSessionMaster->getGameId() << ")");
            }
        }

        if (connectionGroupItr->second.mMeshDisconnectReceived.getClient() && connectionGroupItr->second.mMeshDisconnectReceived.getDedicatedServer())
        {
            // All notifications have been received, and metrics have been updated. Remove connection group mConnectionGroupJoinFailureMap
            // and cancel the timer if the map is empty.
            mConnectionGroupJoinFailureMap.erase(connectionGroupItr);
            if (mConnectionGroupJoinFailureMap.empty())
            {
                cancelConnectionGroupJoinFailureTimer();
            }
        }

    }

    void GameManagerMasterImpl::cleanupExpiredAudits()
    {
        TimeValue now = TimeValue::getTimeOfDay();
        ConnectionMetricAuditMap::const_iterator iter = mAuditedUsers.begin();
        BlazeIdList expiredAuditIds;
        for (; iter != mAuditedUsers.end();)
        {
            // Skip over audits without expiration as they never expire
            if (iter->second != 0 && iter->second < now)
                iter = mAuditedUsers.erase(iter);
            else
                ++iter;
        }

        mAuditCleanupTimer = gSelector->scheduleTimerCall(now + getConfig().getCheckAuditExpirationInterval(), this, &GameManagerMasterImpl::cleanupExpiredAudits, "GameManagerMasterImpl::cleanupExpiredAudits");
    }

    const Matchmaker::GroupUedExpressionList* GameManagerMasterImpl::getGroupUedExpressionList(const char8_t* ruleName) const
    {
        GroupUedExpressionListsMap::const_iterator expressionListIter = mGroupUedExpressionLists.find(ruleName);

        if (expressionListIter != mGroupUedExpressionLists.end())
        {
            return expressionListIter->second;

        }

        return nullptr;
    }

    void GameManagerMasterImpl::cleanUpGroupUedExpressionLists()
    {
        
        GroupUedExpressionListsMap::iterator expressionsMapIter = mGroupUedExpressionLists.begin();
        GroupUedExpressionListsMap::iterator expressionsMapEnd = mGroupUedExpressionLists.end();
        for (; expressionsMapIter != expressionsMapEnd; ++expressionsMapIter)
        {
            // clean up the memory
            delete expressionsMapIter->second;
        }

        // clear the expressions
        mGroupUedExpressionLists.clear();
    }

    bool GameManagerMasterImpl::setUpGroupUedExpressionLists()
    {
        bool allExpressionListsProcessed = true;
        // check all the expression lists are valid
        if (!getConfig().getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().empty())
        {
            TeamUEDBalanceRuleConfigMap::const_iterator teamUedBalanceRuleIter = getConfig().getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().begin();
            TeamUEDBalanceRuleConfigMap::const_iterator teamUedBalanceRuleEnd = getConfig().getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap().end();
            for (; teamUedBalanceRuleIter != teamUedBalanceRuleEnd; ++teamUedBalanceRuleIter)
            {
                if (!teamUedBalanceRuleIter->second->getGroupAdjustmentFormula().empty())
                {
                    Matchmaker::GroupUedExpressionList *tempGroupUedExpressionList = BLAZE_NEW Matchmaker::GroupUedExpressionList();
                    bool expressionInitResult = tempGroupUedExpressionList->initialize(teamUedBalanceRuleIter->second->getGroupAdjustmentFormula());
                    if (!expressionInitResult)
                    {
                        ERR_LOG("[GameManagerMasterImpl].setUpGroupUedExpressionLists ERROR failed to initialize group UED expressions for TeamUedBalanceRule '" << teamUedBalanceRuleIter->first << "'.");
                        delete tempGroupUedExpressionList;
                        allExpressionListsProcessed = false;
                        continue;
                    }

                    mGroupUedExpressionLists.insert(eastl::make_pair(teamUedBalanceRuleIter->first, tempGroupUedExpressionList));
                }
            }
        }

        return allExpressionListsProcessed;
    }

    /// @deprecated [metrics-refactor] getScenarioSubSessionGameMetrics() can be removed when getMatchmakingGameMetrics RPC is removed
    /*! ************************************************************************************************/
    /*!
        \brief Fetch Scenario and SubSession's Matchmaking Game Metrics

            If the Scenario/Variant/Version/SubSession doesn't already exist in the map, initialize it.

        \param[in]    tagList            - scenario information to fetch metric maps for (tags are SPECIFICALLY-defined and SPECIFICALLY-ordered)
        \param[in]    metricsMap         - Matchmaking Game Metrics Map to fetch from (can be modified)
        \param[in]    nonScenarioMetrics - use/return this metrics object if the information indicates non-scenario
        \param[out]   scenarioMetrics    - pointer to scenario metrics for requested scenario (pointer to nonScenarioMetrics if non-scenario)
        \param[out]   subSessionMetrics  - pointer to sub session metrics for requested scenario
    *************************************************************************************************/
    void getScenarioSubSessionGameMetrics(const Metrics::TagPairList& tagList, ScenarioMatchmakingGameMetricsMap& metricsMap, MatchmakingGameMetrics& nonScenarioMetrics, MatchmakingGameMetricsPtr& scenarioMetrics, MatchmakingGameMetricsPtr& subSessionMetrics)
    {
        ASSERT_COND_LOG(tagList.size() >= 4, "getScenarioSubSessionGameMetrics: tagList does not contain enough scenario information");
        const char8_t* scenarioName = tagList[0].second.c_str();
        if (blaze_strcmp(scenarioName, Metrics::Tag::NON_SCENARIO_NAME) == 0)
        {
            scenarioMetrics = &nonScenarioMetrics;
            return;
        }
        const char8_t* scenarioVariant = tagList[1].second.c_str();
        uint32_t scenarioVersion = 0;
        blaze_str2int(tagList[2].second.c_str(), &scenarioVersion);
        const char8_t* subSessionName = tagList[3].second.c_str();

        ScenarioMatchmakingGameMetricsMap::iterator itr = metricsMap.find(scenarioName);
        if (itr == metricsMap.end())
        {
            // No Scenario Found, initialize Scenario/Variant/Version/SubSession
            ScenarioVariantMatchmakingGameMetricsMap* scenarioVariantMatchmakingGameMetricsMap = metricsMap.allocate_element();
            ScenarioVersionMatchmakingGameMetricsMap* scenarioVersionMatchmakingGameMetricsMap = scenarioVariantMatchmakingGameMetricsMap->allocate_element();
            ScenarioMatchmakingGameMetricsPtr scenarioMatchmakingGameMetrics = scenarioVersionMatchmakingGameMetricsMap->allocate_element();
            scenarioMetrics = &scenarioMatchmakingGameMetrics->getMetrics();
            subSessionMetrics = scenarioMatchmakingGameMetrics->getSubSessionMetrics().allocate_element();

            scenarioMatchmakingGameMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
            (*scenarioVersionMatchmakingGameMetricsMap)[scenarioVersion] = scenarioMatchmakingGameMetrics;
            (*scenarioVariantMatchmakingGameMetricsMap)[scenarioVariant] = scenarioVersionMatchmakingGameMetricsMap;
            metricsMap[scenarioName] = scenarioVariantMatchmakingGameMetricsMap;
        }
        else
        {
            ScenarioVariantMatchmakingGameMetricsMap::iterator sceVarMatchmakingGameMetricsMapItr = itr->second->find(scenarioVariant);
            if (sceVarMatchmakingGameMetricsMapItr == itr->second->end())
            {
                // Variant for Scenario Not Found, initialize Variant/Version/SubSession
                ScenarioVersionMatchmakingGameMetricsMap* scenarioVersionMatchmakingGameMetricsMap = itr->second->allocate_element();
                ScenarioMatchmakingGameMetricsPtr scenarioMatchmakingGameMetrics = scenarioVersionMatchmakingGameMetricsMap->allocate_element();
                scenarioMetrics = &scenarioMatchmakingGameMetrics->getMetrics();
                subSessionMetrics = scenarioMatchmakingGameMetrics->getSubSessionMetrics().allocate_element();

                scenarioMatchmakingGameMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                (*scenarioVersionMatchmakingGameMetricsMap)[scenarioVersion] = scenarioMatchmakingGameMetrics;
                (*itr->second)[scenarioVariant] = scenarioVersionMatchmakingGameMetricsMap;
            }
            else
            {
                ScenarioVersionMatchmakingGameMetricsMap::iterator sceVerMatchmakingGameMetricsMap = sceVarMatchmakingGameMetricsMapItr->second->find(scenarioVersion);
                if (sceVerMatchmakingGameMetricsMap == sceVarMatchmakingGameMetricsMapItr->second->end())
                {
                    // Version for Scenario's Variant Not Found, initialize Version/SubSession
                    ScenarioMatchmakingGameMetricsPtr scenarioMatchmakingGameMetrics = sceVarMatchmakingGameMetricsMapItr->second->allocate_element();
                    scenarioMetrics = &scenarioMatchmakingGameMetrics->getMetrics();
                    subSessionMetrics = scenarioMatchmakingGameMetrics->getSubSessionMetrics().allocate_element();

                    scenarioMatchmakingGameMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                    (*sceVarMatchmakingGameMetricsMapItr->second)[scenarioVersion] = scenarioMatchmakingGameMetrics;
                }
                else
                {
                    scenarioMetrics = &sceVerMatchmakingGameMetricsMap->second->getMetrics();
                    SubSessionGameMetricsMap::iterator subSessionGameMetricsMap = sceVerMatchmakingGameMetricsMap->second->getSubSessionMetrics().find(subSessionName);
                    if (subSessionGameMetricsMap == sceVerMatchmakingGameMetricsMap->second->getSubSessionMetrics().end())
                    {
                        // SubSession for Scenario/Variant/Version Not Found, initialize SubSession
                        subSessionMetrics = sceVerMatchmakingGameMetricsMap->second->getSubSessionMetrics().allocate_element();
                        sceVerMatchmakingGameMetricsMap->second->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                    }
                    else
                    {
                        subSessionMetrics = subSessionGameMetricsMap->second;
                    }
                }
            }
        }
    }

    /// @deprecated [metrics-refactor] getMatchmakingGameMetrics RPC is polled for Graphite (which, in turn, is used by GOSCC) and can be removed when no longer used
    GetMatchmakingGameMetricsError::Error GameManagerMasterImpl::processGetMatchmakingGameMetrics(Blaze::GameManager::GetMatchmakingGameMetricsResponse& response, const Message* message)
    {
        gController->tickDeprecatedRPCMetric("gamemanager_master", "getMatchmakingGameMetrics", message);

        ScenarioMatchmakingGameMetricsMap& scenarioMmGameMetricsMap = response.getScenarioMatchmakingMetricsMap();
        MatchmakingGameMetrics& nonScenarioMmGameMetrics = response.getNonScenarioMetrics();

        mMetrics.mDurationInGameSession.iterate([&scenarioMmGameMetricsMap, &nonScenarioMmGameMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            MatchmakingGameMetricsPtr scenarioGameMetrics = nullptr;
            MatchmakingGameMetricsPtr subsessionGameMetrics = nullptr;
            getScenarioSubSessionGameMetrics(tagList, scenarioMmGameMetricsMap, nonScenarioMmGameMetrics, scenarioGameMetrics, subsessionGameMetrics);
            if (scenarioGameMetrics != nullptr)
            {
                scenarioGameMetrics->setTotalDurationInGameSession(value.getTotal());
            }
            if (subsessionGameMetrics != nullptr)
            {
                subsessionGameMetrics->setTotalDurationInGameSession(value.getTotal());
            }
        });

        mMetrics.mDurationInMatch.iterate([&scenarioMmGameMetricsMap, &nonScenarioMmGameMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            MatchmakingGameMetricsPtr scenarioGameMetrics = nullptr;
            MatchmakingGameMetricsPtr subsessionGameMetrics = nullptr;
            getScenarioSubSessionGameMetrics(tagList, scenarioMmGameMetricsMap, nonScenarioMmGameMetrics, scenarioGameMetrics, subsessionGameMetrics);
            if (scenarioGameMetrics != nullptr)
            {
                scenarioGameMetrics->setTotalDurationInMatch(value.getTotal());
            }
            if (subsessionGameMetrics != nullptr)
            {
                subsessionGameMetrics->setTotalDurationInMatch(value.getTotal());
            }
        });

        mMetrics.mPlayers.iterate([&scenarioMmGameMetricsMap, &nonScenarioMmGameMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            MatchmakingGameMetricsPtr scenarioGameMetrics = nullptr;
            MatchmakingGameMetricsPtr subsessionGameMetrics = nullptr;
            getScenarioSubSessionGameMetrics(tagList, scenarioMmGameMetricsMap, nonScenarioMmGameMetrics, scenarioGameMetrics, subsessionGameMetrics);
            if (scenarioGameMetrics != nullptr)
            {
                scenarioGameMetrics->setTotalPlayers((uint32_t)value.getTotal());
            }
            if (subsessionGameMetrics != nullptr)
            {
                subsessionGameMetrics->setTotalPlayers((uint32_t)value.getTotal());
            }
        });

        mMetrics.mLeftMatchEarly.iterate([&scenarioMmGameMetricsMap, &nonScenarioMmGameMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            MatchmakingGameMetricsPtr scenarioGameMetrics = nullptr;
            MatchmakingGameMetricsPtr subsessionGameMetrics = nullptr;
            getScenarioSubSessionGameMetrics(tagList, scenarioMmGameMetricsMap, nonScenarioMmGameMetrics, scenarioGameMetrics, subsessionGameMetrics);
            if (scenarioGameMetrics != nullptr)
            {
                scenarioGameMetrics->setTotalLeftMatchEarly((uint32_t)value.getTotal());
            }
            if (subsessionGameMetrics != nullptr)
            {
                subsessionGameMetrics->setTotalLeftMatchEarly((uint32_t)value.getTotal());
            }
        });

        mMetrics.mFinishedMatches.iterate([&scenarioMmGameMetricsMap, &nonScenarioMmGameMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            MatchmakingGameMetricsPtr scenarioGameMetrics = nullptr;
            MatchmakingGameMetricsPtr subsessionGameMetrics = nullptr;
            getScenarioSubSessionGameMetrics(tagList, scenarioMmGameMetricsMap, nonScenarioMmGameMetrics, scenarioGameMetrics, subsessionGameMetrics);
            if (scenarioGameMetrics != nullptr)
            {
                scenarioGameMetrics->setTotalFinishedMatches((uint32_t)value.getTotal());
            }
            if (subsessionGameMetrics != nullptr)
            {
                subsessionGameMetrics->setTotalFinishedMatches((uint32_t)value.getTotal());
            }
        });

        return GetMatchmakingGameMetricsError::ERR_OK;
    }

    /// @deprecated [metrics-refactor] getGameModeMetrics RPC is polled for Graphite (which, in turn, is used by GOSCC) and can be removed when no longer used
    GetGameModeMetricsError::Error GameManagerMasterImpl::processGetGameModeMetrics(Blaze::GameManager::GetGameModeMetrics& response, const Message* message)
    {
        gController->tickDeprecatedRPCMetric("gamemanager_master", "getGameModeMetrics", message);

        GameModeMetricsMap& gameModeMetricsMap = response.getMetrics();

        mMetrics.mGameModeStarts.iterate([&gameModeMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 2, "mGameModeStarts.iterate: tagList does not contain enough game mode information");
            GameModeByJoinMethodType joinType;
            if (ParseGameModeByJoinMethodType(tagList[1].second.c_str(), joinType))
            {
                const char8_t* gameMode = tagList[0].second.c_str();
                GameModeMetrics* gameModeMetrics = nullptr;
                GameModeMetricsMap::iterator itr = gameModeMetricsMap.find(gameMode);
                if (itr == gameModeMetricsMap.end())
                {
                    gameModeMetrics = gameModeMetricsMap.allocate_element();
                    gameModeMetricsMap[gameMode] = gameModeMetrics;
                }
                else
                {
                    gameModeMetrics = itr->second;
                }
                switch (joinType)
                {
                case GAMEMODE_BY_DIRECTJOIN:
                    gameModeMetrics->getDirectJoinGameModeMetrics().setTotalStarts((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_MATCHMAKING:
                    gameModeMetrics->getMatchMakingGameModeMetrics().setTotalStarts((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_REMATCH:
                    gameModeMetrics->getRematchGameModeMetrics().setTotalStarts((uint32_t)value.getTotal());
                    break;
                default:
                    break;
                }
            }
        });

        mMetrics.mGameModeMatchDuration.iterate([&gameModeMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 2, "mGameModeMatchDuration.iterate: tagList does not contain enough game mode information");
            GameModeByJoinMethodType joinType;
            if (ParseGameModeByJoinMethodType(tagList[1].second.c_str(), joinType))
            {
                const char8_t* gameMode = tagList[0].second.c_str();
                GameModeMetrics* gameModeMetrics = nullptr;
                GameModeMetricsMap::iterator itr = gameModeMetricsMap.find(gameMode);
                if (itr == gameModeMetricsMap.end())
                {
                    gameModeMetrics = gameModeMetricsMap.allocate_element();
                    gameModeMetricsMap[gameMode] = gameModeMetrics;
                }
                else
                {
                    gameModeMetrics = itr->second;
                }
                switch (joinType)
                {
                case GAMEMODE_BY_DIRECTJOIN:
                    gameModeMetrics->getDirectJoinGameModeMetrics().setTotalMatchDuration((int64_t)value.getTotal());
                    break;
                case GAMEMODE_BY_MATCHMAKING:
                    gameModeMetrics->getMatchMakingGameModeMetrics().setTotalMatchDuration((int64_t)value.getTotal());
                    break;
                case GAMEMODE_BY_REMATCH:
                    gameModeMetrics->getRematchGameModeMetrics().setTotalMatchDuration((int64_t)value.getTotal());
                    break;
                default:
                    break;
                }
            }
        });

        mMetrics.mGameModeLeftEarly.iterate([&gameModeMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 2, "mGameModeLeftEarly.iterate: tagList does not contain enough game mode information");
            GameModeByJoinMethodType joinType;
            if (ParseGameModeByJoinMethodType(tagList[1].second.c_str(), joinType))
            {
                const char8_t* gameMode = tagList[0].second.c_str();
                GameModeMetrics* gameModeMetrics = nullptr;
                GameModeMetricsMap::iterator itr = gameModeMetricsMap.find(gameMode);
                if (itr == gameModeMetricsMap.end())
                {
                    gameModeMetrics = gameModeMetricsMap.allocate_element();
                    gameModeMetricsMap[gameMode] = gameModeMetrics;
                }
                else
                {
                    gameModeMetrics = itr->second;
                }
                switch (joinType)
                {
                case GAMEMODE_BY_DIRECTJOIN:
                    gameModeMetrics->getDirectJoinGameModeMetrics().setTotalLeftEarly((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_MATCHMAKING:
                    gameModeMetrics->getMatchMakingGameModeMetrics().setTotalLeftEarly((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_REMATCH:
                    gameModeMetrics->getRematchGameModeMetrics().setTotalLeftEarly((uint32_t)value.getTotal());
                    break;
                default:
                    break;
                }
            }
        });

        mMetrics.mGameModeFinishedMatch.iterate([&gameModeMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 2, "mGameModeFinishedMatch.iterate: tagList does not contain enough game mode information");
            GameModeByJoinMethodType joinType;
            if (ParseGameModeByJoinMethodType(tagList[1].second.c_str(), joinType))
            {
                const char8_t* gameMode = tagList[0].second.c_str();
                GameModeMetrics* gameModeMetrics = nullptr;
                GameModeMetricsMap::iterator itr = gameModeMetricsMap.find(gameMode);
                if (itr == gameModeMetricsMap.end())
                {
                    gameModeMetrics = gameModeMetricsMap.allocate_element();
                    gameModeMetricsMap[gameMode] = gameModeMetrics;
                }
                else
                {
                    gameModeMetrics = itr->second;
                }
                switch (joinType)
                {
                case GAMEMODE_BY_DIRECTJOIN:
                    gameModeMetrics->getDirectJoinGameModeMetrics().setTotalFinishedMatch((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_MATCHMAKING:
                    gameModeMetrics->getMatchMakingGameModeMetrics().setTotalFinishedMatch((uint32_t)value.getTotal());
                    break;
                case GAMEMODE_BY_REMATCH:
                    gameModeMetrics->getRematchGameModeMetrics().setTotalFinishedMatch((uint32_t)value.getTotal());
                    break;
                default:
                    break;
                }
            }
        });

        return GetGameModeMetricsError::ERR_OK;
    }

    /// @deprecated [metrics-refactor] getPktReceivedMetrics RPC is apparently a custom RPC used for debugging by DICE(?) and should be removed when no longer needed/used
    GetPktReceivedMetricsError::Error GameManagerMasterImpl::processGetPktReceivedMetrics(const Blaze::GameManager::GetPktReceivedMetricsRequest &request, Blaze::GameManager::GetPktReceivedMetricsResponse &response, const Message* message)
    {
        if (request.getGameId() == INVALID_GAME_ID)
        {
            mPktReceivedMetricsByGameIdMap.copyInto(response.getPktReceivedMetricsByGameIdMap());
        }
        else
        {
            PktReceivedMetricsByGameIdMap::iterator itr = mPktReceivedMetricsByGameIdMap.find(request.getGameId());
            if (itr != mPktReceivedMetricsByGameIdMap.end())
            {
                PktReceivedMetrics* pktReceivedMetrics = response.getPktReceivedMetricsByGameIdMap().allocate_element();
                (*itr).second->copyInto(*pktReceivedMetrics);
                response.getPktReceivedMetricsByGameIdMap()[request.getGameId()] = pktReceivedMetrics;
            }
        }
        return GetPktReceivedMetricsError::ERR_OK;
    }

    GameSessionMaster* GameManagerMasterImpl::getGameSessionMaster(GameId gameId) const
    {
        GameSessionMaster* session = nullptr;
        GameSessionMasterByIdMap::const_iterator it = mGameSessionMasterByIdMap.find(gameId);
        if (it != mGameSessionMasterByIdMap.end())
            session = it->second.get();
        return session;
    }

    /*! ********************************************************************************************************************/
    /*!
        \brief If createGame request provides persisted id, check if it exists from both redis and corresponding sliver. 
               If exists in both redis and corresponding sliver's local cache, return id in use, otherwise, update the redis to take ownership of this id.
               Generate persistedId and secret if createGame request does not provide it.
               Note: this method is blockable.
    
        \param[in]    CreateGameRequest       create game request
        \param[in]    SliverId -              the sliver id which owns the current game
        \param[out]   PersistedGameId         the persisted game id of the game. Either provided in request or generate
        \param[out]   PersistedGameIdSecret   the persisted game id secret generates from the persisted id
    ************************************************************************************************************************/
    BlazeRpcError GameManagerMasterImpl::validateOrCreate(const CreateGameRequest& createGameRequest, SliverId curSliverId, PersistedGameId& persistedId, PersistedGameIdSecret& persistedIdSecret, GameId& existingGameId)
    {
        existingGameId = INVALID_GAME_ID;
        if (createGameRequest.getGameCreationData().getGameSettings().getEnablePersistedGameId())
        {
            RedisError rc = REDIS_ERR_OK;
            if (createGameRequest.getPersistedGameId()[0] == '\0')
            {
                // generate UUID
                UUID uuid;
                UUIDUtil::generateUUID(uuid);
                if (uuid.c_str()[0] == '\0')
                {
                    WARN_LOG("[GameManagerMasterImpl].validateOrCreate: Failed to generate UUID for the game.");
                    return ERR_SYSTEM;
                }
                persistedId = uuid.c_str();
                // generate secret
                UUIDUtil::generateSecretText(uuid, persistedIdSecret);

                int64_t result = 0;
                rc = mPersistedIdOwnerMap.insert(persistedId, curSliverId, &result);
                if (rc != REDIS_ERR_OK || result != 1)
                {
                    ERR_LOG("[GameManagerMasterImpl].validateOrCreate: Failed to insert persisted game id (" << persistedId << ") to redis. RedisError: " << RedisErrorHelper::getName(rc));
                    return ERR_SYSTEM;
                }
            }
            else
            {
                // check if the provided persisted game id exists in redis
                PersistedGameId providedId = createGameRequest.getPersistedGameId();
                int64_t result = 0;
                rc = mPersistedIdOwnerMap.insert(providedId, curSliverId, &result);
                if (rc != REDIS_ERR_OK)
                {
                    ERR_LOG("[GameManagerMasterImpl].validateOrCreate: Failed to insert persisted game id (" << providedId << ") to redis. RedisError: " << RedisErrorHelper::getName(rc));
                    return ERR_SYSTEM;
                }

                if (result == 1)
                {
                    // insert successfully
                    TRACE_LOG("[GameManagerMasterImpl].validateOrCreate: Insert persistedId (" << providedId << ") and sliverId (" << curSliverId << ") successfully into redis.");
                }
                else if (result == -1) // We now found the persisted id exists in redis, now we need to check with the sliver manager to make sure there is a sliver owns it
                {
                    // Hash the given persisted ID to a sliver id
                    SliverId sliverId = INVALID_SLIVER_ID;
                    uint32_t hash = EA::StdC::FNV1_String8(providedId, EA::StdC::kFNV1InitialValue, EA::StdC::kCharCaseLower);

                    uint32_t sliverCount = gSliverManager->getSliverCount(GameManager::GameManagerMaster::COMPONENT_ID);
                    if (sliverCount > 0)
                    {
                        // NOTE: This mapping means that the sliverCount can NEVER be reconfigurable on a live system, since this mapping would become invalid.
                        sliverId = static_cast<uint16_t>(hash % sliverCount);
                    }
                    else
                    {
                        ERR_LOG("[GameManagerMasterImpl].validateOrCreate: Sliver count is invalid.");
                        return ERR_SYSTEM;
                    }

                    // the master who owns this hashed sliverId is the ONLY authority to help reserve the persisted id
                    RpcCallOptions opts;
                    opts.routeTo.setSliverIdentityFromSliverId(GameManager::GameManagerMaster::COMPONENT_ID, sliverId);
                    ReservePersistedIdMasterRequest reservePersistedIdMasterRequest;
                    reservePersistedIdMasterRequest.setPersistedGameId(createGameRequest.getPersistedGameId());
                    reservePersistedIdMasterRequest.setPersistedIdOwnerSliverId(curSliverId); // the sliverId who wants to reserve the persisted id

                    CreateGameMasterErrorInfo reservePersistedIdMasterErrorResp;
                    BlazeRpcError validationError = reservePersistedIdMaster(reservePersistedIdMasterRequest, reservePersistedIdMasterErrorResp, opts);

                    if (validationError != ERR_OK)
                    {
                        if (validationError == GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE)
                            existingGameId = reservePersistedIdMasterErrorResp.getPreExistingGameIdWithPersistedId();

                        TRACE_LOG("[GameManagerMasterImpl].validateOrCreate: Failed to reserve persisted game id '" << providedId <<
                            "' by sliver id (" << sliverId << "), error " << ErrorHelp::getErrorName(validationError));
                        return validationError;
                    }
                }

                if (!isUUIDFormat(createGameRequest.getPersistedGameId()))
                {
                    // generate secret
                    UUIDUtil::generateSecretText(createGameRequest.getPersistedGameId(), persistedIdSecret);
                }
                else if (!isSecretMatchUUID(createGameRequest.getPersistedGameId(), createGameRequest.getPersistedGameIdSecret()))
                {
                    TRACE_LOG("[GameManagerMasterImpl].validateOrCreate: UUID " << createGameRequest.getPersistedGameId() << " for the game"
                        << " does not match the secret in the request.");
                    return GAMEMANAGER_ERR_INVALID_PERSISTED_GAME_ID_OR_SECRET;
                }

                persistedId = createGameRequest.getPersistedGameId();
                persistedIdSecret = createGameRequest.getPersistedGameIdSecret();
            }
        }
        return ERR_OK;
    }

    void GameManagerMasterImpl::cleanupPersistedIdFromRedis(PersistedGameId persistedId, GameSessionMasterPtr/*intentionally unused, this arg pins the game from migrating until erasure has completed*/)
            {
        //  clean up entry, and ignore response from redis
        mPersistedIdOwnerMap.erase(persistedId);
            }

    bool GameManagerMasterImpl::isUUIDFormat(const char8_t* persistedGameId)
    {
        if (persistedGameId != nullptr && persistedGameId[0] != '\0' && strlen(persistedGameId)+1 == MAX_UUID_LEN &&
            persistedGameId[8] == '-' && persistedGameId[13] == '-' && persistedGameId[18] == '-' && persistedGameId[23] == '-')
        {
            return true;
        }

        return false;
    }

    bool GameManagerMasterImpl::isSecretMatchUUID(const char8_t* uuid, const EA::TDF::TdfBlob& blob)
    {
        UUID uuidRetrieved;
        UUIDUtil::retrieveUUID(blob, uuidRetrieved);
        if (blaze_strcmp(uuid, uuidRetrieved.c_str()) == 0)
        {
            return true;
        }

        return false;
    }

    void GameManagerMasterImpl::validateUserExistence(UserSessionIdBlazeIdPair pair)
    {
        ValidateUserExistenceRequest req;
        req.setUserSessionId(pair.first);
        BlazeRpcError rc = gUserSessionManager->getMaster()->validateUserExistence(req);
        if (rc == ERR_OK)
        {
            TRACE_LOG("[GameManagerMaster].validateUserExistence: UserSession(" << pair.first << ")/BlazeId(" << pair.second << ") existence confirmed remotely.");
        }
        else if (rc == USER_ERR_SESSION_NOT_FOUND)
        {
            INFO_LOG("[GameManagerMaster].validateUserExistence: UserSession(" << pair.first << ")/BlazeId(" << pair.second << ") existence disconfirmed remotely, remove from all games.");
            removeUserFromAllGames(pair.first, pair.second);
        }
        else
        {
            WARN_LOG("[GameManagerMaster].validateUserExistence: Failed to remotely validate UserSession(" << pair.first << ")/BlazeId(" << pair.second << "), err:" << ErrorHelp::getErrorName(rc));
        }
    }

    void GameManagerMasterImpl::scheduleValidateUserSession(UserSessionId userSessionId, BlazeId blazeId, UserSessionIdSet& validatedSessionIdSet)
    {
        if (validatedSessionIdSet.insert(userSessionId).second)
        {
            if (gUserSessionManager->getSessionExists(userSessionId))
            {
                TRACE_LOG("[GameManagerMaster].scheduleValidateUserSession: UserSession(" << userSessionId << ")/BlazeId(" << blazeId << ") existence confirmed locally.");
                return;
            }
            mValidateExistenceJobQueue.queueFiberJob(this, &GameManagerMasterImpl::validateUserExistence, eastl::make_pair(userSessionId, blazeId), "GameManagerMasterImpl::validateUserExistence");
        }
    }

    uint32_t GameManagerMasterImpl::getNextCCSRequestId()
    {
        // The id is unique across the Blaze installation so that we can identify which instance issued the request (if needed).
        // Use a random start offset to make it easy to read the logs of CCS service especially in dev environment where we could be restarting 
        // Blaze server many times but not the CCS Service.
        static int startOffset = Random::getRandomNumber(5000);
        return BuildInstanceKey32(gController->getInstanceId(), (++mNextCCSRequestId + startOffset)); 
    }

    CcsConnectivityResultsAvailableError::Error GameManagerMasterImpl::processCcsConnectivityResultsAvailable(const Blaze::GameManager::CCSAllocateRequestMaster &request, const Message* message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return CcsConnectivityResultsAvailableError::ERR_SYSTEM;
        }

        return gameSessionMaster->ccsConnectivityResultsAvailable(request);
    }

    CcsLeaseExtensionResultsAvailableError::Error GameManagerMasterImpl::processCcsLeaseExtensionResultsAvailable(const Blaze::GameManager::CCSLeaseExtensionRequestMaster &request, const Message* message)
    {
        GameId gameId = request.getGameId();
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(gameId);
        if (EA_UNLIKELY(gameSessionMaster == nullptr))
        {
            return CcsLeaseExtensionResultsAvailableError::ERR_SYSTEM;
        }

        return gameSessionMaster->ccsLeaseExtensionResultsAvailable(request);
    }
    // PIN EVENTS
    /*! ************************************************************************************************/
    /*! \brief or sending MP match info events for round start/end
    ***************************************************************************************************/
    void GameManagerMasterImpl::buildAndSendMatchInfoEvent(const GameSessionMaster& gameSession, const char8_t* statusString) const
    {
        // build and send mp_match_info
        // status context doesn't impact the event fields
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        PINEventHelper::matchInfoForAllMembers(gameSession, *pinSubmission, statusString);
        gUserSessionManager->sendPINEvents(pinSubmission);
    }

    /*! ************************************************************************************************/
    /*! \brief or sending MP match info events for player leaving
    ***************************************************************************************************/
    void GameManagerMasterImpl::buildAndSendMatchLeftEvent(const GameSessionMaster& gameSession, const PlayerInfoMaster &leavingPlayer, const PlayerRemovalContext &removalContext) const
    {
        // build and send mp_match_info
        // status context doesn't impact the event fields
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        PINEventHelper::matchLeft(leavingPlayer, removalContext, gameSession, *pinSubmission);
        gUserSessionManager->sendPINEvents(pinSubmission);
    }

    /*! ************************************************************************************************/
    /*! \brief or sending MP match info events for round start/end/player leave
    ***************************************************************************************************/
    void GameManagerMasterImpl::buildAndSendMatchJoinEvent(const GameSessionMaster& gameSession, const UserSessionSetupReasonMap &setupReasons,
                                                           const RelatedPlayerSetsMap &relatedPlayerLists) const
    {
        // build and send mp_match_info
        // status context doesn't impact the event fields
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        buildMatchJoinEvent(gameSession, setupReasons, relatedPlayerLists, *pinSubmission, nullptr);

        gUserSessionManager->sendPINEvents(pinSubmission);

    }

    void GameManagerMasterImpl::buildAndSendMatchJoinFailedEvent(const GameSessionMasterPtr gameSession, const JoinGameByGroupMasterRequest& request,
                                                                 const RelatedPlayerSetsMap &relatedPlayerLists, BlazeRpcError errorCode) const
    {
        // build and send mp_match_info
        // status context doesn't impact the event fields
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        PINEventHelper::buildMatchJoinFailedEvent(request, errorCode, gameSession, relatedPlayerLists, *pinSubmission);

        gUserSessionManager->sendPINEvents(pinSubmission);

    }

    void GameManagerMasterImpl::buildAndSendMatchJoinFailedEvent(const GameSessionMasterPtr gameSession, const JoinGameMasterRequest& request, const RelatedPlayerSetsMap &relatedPlayerLists, BlazeRpcError errorCode) const
    {
        // build and send mp_match_info
        // status context doesn't impact the event fields
        PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
        PINEventHelper::buildMatchJoinFailedEvent(request, errorCode, gameSession, relatedPlayerLists, *pinSubmission);

        gUserSessionManager->sendPINEvents(pinSubmission);

    }

    void GameManagerMasterImpl::buildMatchJoinEvent(const GameSessionMaster& gameSession, const UserSessionSetupReasonMap &setupReasons, const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission, const char8_t* trackingTag) const
    {
        UserSessionSetupReasonMap::const_iterator iter = setupReasons.begin();
        UserSessionSetupReasonMap::const_iterator end = setupReasons.end();
        for (; iter != end; ++iter)
        {
            const PlayerInfoMaster *playerInfo = gameSession.getPlayerRoster()->getPlayer(gUserSessionManager->getBlazeId(iter->first));
            if (playerInfo != nullptr)
            {
                PINEventHelper::matchJoined(*playerInfo, *(iter->second), gameSession, relatedPlayerLists, trackingTag, pinSubmission);
            }
        }
    }

    void GameManagerMasterImpl::buildRelatedPlayersMap(const GameSessionMaster* gameSession, const UserJoinInfoList &joiningUsers, RelatedPlayerSetsMap &joiningPlayersLists )
    {

        for (UserJoinInfoList::const_iterator iter = joiningUsers.begin(), end = joiningUsers.end(); iter != end; ++iter)
        {
            UserSessionId currentSessionId = (*iter)->getUser().getSessionId();
            PlayerId currentBlazeId = (*iter)->getUser().getUserInfo().getId();
            UserGroupId currentGroupId = (*iter)->getUserGroupId();
            PlayerIdSet playerIds;
            for (UserJoinInfoList::const_iterator groupIter = joiningUsers.begin(), groupEnd = joiningUsers.end(); groupIter != groupEnd; ++groupIter)
            {
                if (((*groupIter)->getUser().getSessionId() != currentSessionId)
                    && (currentGroupId != EA::TDF::OBJECT_ID_INVALID)
                    && ((*groupIter)->getUserGroupId() == currentGroupId))
                {
                    // captures any group members I'm joining with
                    playerIds.insert((*groupIter)->getUser().getUserInfo().getId());
                }

                if (gameSession != nullptr)
                {
                    // check for any group members already in the game
                    PlayerIdList groupMembersInGame;
                    gameSession->getPlayerRoster()->getGroupMembersInGame((*iter)->getUserGroupId(), groupMembersInGame);
                    for (PlayerIdList::const_iterator groupMembersIter = groupMembersInGame.begin(), groupMembersEnd = groupMembersInGame.end(); groupMembersIter != groupMembersEnd; ++groupMembersIter)
                    {
                        if ((*groupMembersIter) != currentBlazeId)
                        {
                            playerIds.insert((*groupMembersIter));
                        }
                    }
                }
            }

            if (!playerIds.empty())
            {
                joiningPlayersLists.insert(eastl::make_pair(currentSessionId, playerIds));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Return game's current external session info
    ***************************************************************************************************/
    GetExternalSessionInfoMasterError::Error GameManagerMasterImpl::processGetExternalSessionInfoMaster(
        const GetExternalSessionInfoMasterRequest &request, GetExternalSessionInfoMasterResponse &response, const Blaze::Message *message)
    {
        UserIdentification tempCaller;
        request.getCaller().copyInto(tempCaller);
        convertToPlatformInfo(tempCaller);

        const GameSessionMaster* gameSessionMaster = getReadOnlyGameSession(request.getGameId() );
        if (gameSessionMaster == nullptr)
        {
            return GetExternalSessionInfoMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        gameSessionMaster->getExternalSessionInfoMaster(tempCaller, response);
        return GetExternalSessionInfoMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Track a user as being in game's external session
    ***************************************************************************************************/
    TrackExtSessionMembershipMasterError::Error GameManagerMasterImpl::processTrackExtSessionMembershipMaster(
        const TrackExtSessionMembershipRequest &request, GetExternalSessionInfoMasterResponse &response, 
        GetExternalSessionInfoMasterResponse &errorResponse, const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (gameSessionMaster == nullptr)
        {
            return TrackExtSessionMembershipMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        return gameSessionMaster->trackExtSessionMembership(request, response, errorResponse);
    }

    /*! ************************************************************************************************/
    /*! \brief Track a user as not being in game's external session
    ***************************************************************************************************/
    UntrackExtSessionMembershipMasterError::Error GameManagerMasterImpl::processUntrackExtSessionMembershipMaster(
        const UntrackExtSessionMembershipRequest &request, UntrackExtSessionMembershipResponse &response, const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (gameSessionMaster == nullptr)
        {
            TRACE_LOG("[GameManagerMasterImpl].processUntrackExtSessionMembershipMaster: game(" << request.getGameId() << ") no longer exists, no op.");
            return UntrackExtSessionMembershipMasterError::ERR_OK;
        }

        if (isExtSessIdentSet(request.getExtSessionToUntrackAll(), request.getExtSessType()))
        {
            // un-track all (cleanup after 1st party errors etc)
            gameSessionMaster->untrackAllExtSessionMemberships();
        }
        else
        {
            // un-track single user
            auto forType = request.getExtSessType().getType();
            gameSessionMaster->untrackExtSessionMembership(request.getUser().getUserIdentification().getBlazeId(), request.getExtSessType().getPlatform(), &forType);
        }
        response.setSimplifiedState(getSimplifiedGamePlayState(*gameSessionMaster));
        return UntrackExtSessionMembershipMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Update game's external session status
    ***************************************************************************************************/
    UpdateExternalSessionStatusMasterError::Error GameManagerMasterImpl::processUpdateExternalSessionStatusMaster(
        const Blaze::GameManager::UpdateExternalSessionStatusRequest &request, const Blaze::Message *message)
    {
        GameSessionMasterPtr gameSessionMaster = getWritableGameSession(request.getGameId());
        if (gameSessionMaster == nullptr)
        {
            return UpdateExternalSessionStatusMasterError::GAMEMANAGER_ERR_INVALID_GAME_ID;
        }
        return gameSessionMaster->updateExternalSessionStatus(request);
    }

    /*! ************************************************************************************************/
    /*! \brief Helper function to send off the Game Events to the External Tournament sources.
    ***************************************************************************************************/
    void GameManagerMasterImpl::sendExternalTournamentGameEvent(const ExternalHttpGameEventDataPtr data, eastl::string gameEventAddress,
        eastl::string gameEventUri, eastl::string gameEventContentType)
    {
        doSendTournamentGameEvent(data, gameEventAddress, gameEventUri, gameEventContentType, getConfig().getGameSession().getSendTournamentEventRetryLimit());
    }

    BlazeRpcError GameManagerMasterImpl::doSendTournamentGameEvent(const ExternalHttpGameEventDataPtr data, eastl::string gameEventAddress,
        eastl::string gameEventUri, eastl::string gameEventContentType, uint8_t retryLimit)
    {
        // Strip any http/https, since InetAddress ctor only expects hostname:port. If https was specified, set isSecure
        const char8_t* hostName = nullptr;
        bool isSecure = false;
        
        //Remove trailing slash from game event address
        gameEventAddress.erase(gameEventAddress.find_last_not_of('/') + 1);
        
        //Add slash if it doesn't have in the beginning of gameEventUri
        if (gameEventUri.length() > 0 && gameEventUri[0] != '/') 
        {
            gameEventUri.insert(0, "/");
        }
        
        HttpProtocolUtil::getHostnameFromConfig(gameEventAddress.c_str(), hostName, isSecure);

        InetAddress exteralAddress(hostName);
        OutboundHttpConnectionManager connMgr(gameEventAddress.c_str());
        connMgr.initialize(exteralAddress, 10, isSecure);

        RawBuffer buffer(1024);

        Encoder::Type encoderType = RestRequestBuilder::getEncoderTypeFromContentType(gameEventContentType.c_str());
        TdfEncoder* encoder = gController->getConnectionManager().getOrCreateEncoder(encoderType);
        if ((encoder == nullptr) || !encoder->encode(buffer, *data))
        {
            ERR_LOG("[GameManagerMasterImpl].sendExternalTournamentGameEvent: Error encoding TDF for the game(" << data->getGameId() << ").  This is unexpected.");
            return ERR_SYSTEM;
        }

        const char8_t* httpHeaders[1];

        ExternalTournamentEventResult result;
        BlazeRpcError error = ERR_OK;
        int64_t retriesRemaining = retryLimit;
        do
        {
            result.clearError();
            error = connMgr.sendRequest(getTournamentGameEventHttpMethod(),
                (const char8_t*)gameEventUri.c_str(), httpHeaders, 0, (OutboundHttpResult *)&result, gameEventContentType.c_str(),
                (const char8_t*)buffer.data(), (uint32_t)buffer.datasize(), Log::HTTP, (const char8_t*)nullptr, RpcCallOptions());

            // retry as needed:
        } while ((retriesRemaining-- > 0) && 
            (isTournamentEventRetryError(error, result)) &&
            (ERR_OK == (error = Fiber::sleep(2 * 1000 * 1000, "sendTournamentGameEvent retry"))));


        if (error != ERR_OK || result.hasError())
        {
            WARN_LOG("[GameManagerMasterImpl].sendExternalTournamentGameEvent: error(" << error << ") HTTP error(" << result.getError() << "), httpStatus(" << result.getHttpStatusCode() << "), sending event for game(" << data->getGameId() << ") to (" <<
                gameEventAddress.c_str() << gameEventUri.c_str() << "), contentType(" << gameEventContentType << "). Retries(" << (retryLimit - (retriesRemaining + 1)) << ").");
        }
        else
        {
            TRACE_LOG("[GameManagerMasterImpl].sendExternalTournamentGameEvent: httpStatus(" << result.getHttpStatusCode() << "), successfully sent event for game(" << data->getGameId() << "), to (" <<
                gameEventAddress.c_str() << gameEventUri.c_str() << "), contentType(" << gameEventContentType << "). Retries(" << (retryLimit - (retriesRemaining + 1)) << ").");
        }
        return (error != ERR_OK || result.hasError());
    }

    bool GameManagerMasterImpl::isTournamentEventRetryError(BlazeRpcError error, const ExternalTournamentEventResult& result)
    {
        auto httpCode = const_cast<ExternalTournamentEventResult&>(result).getHttpStatusCode();
        const bool isRetryHttpCode = (
            (httpCode == 401) || (httpCode == 404) || (httpCode == 408) || (httpCode == 409) || (httpCode == 425) || (httpCode == 429) ||
            (httpCode == 500) || (httpCode == 502) || (httpCode == 503) || (httpCode == 504) ||
            ((httpCode == 0) && ((result.getError() != ERR_OK) || (error != ERR_OK))));//may be 0 on curl/network error

        if ((error == ERR_TIMEOUT) || isRetryHttpCode)
        {
            TRACE_LOG("[GameManagerMasterImpl].isTournamentEventRetryError: retryable error(" << ErrorHelp::getErrorName(error) << ") HTTP error(" << result.getError() << "), httpStatus(" << httpCode << ")");
            return true;
        }
        return false;
    }

    uint32_t GameManagerMasterImpl::getQuantizedPacketLoss(uint32_t packetLossPct) const
    {
        auto& quantiles = getConfig().getPacketLossMetricQuantiles();
        return quantizeToNearestValue(packetLossPct, quantiles.begin(), quantiles.size());
    }

    uint32_t GameManagerMasterImpl::getQuantizedLatency(uint32_t latencyMs) const
    {
        auto& quantiles = getConfig().getLatencyMetricQuantiles();
        return quantizeToNearestValue(latencyMs, quantiles.begin(), quantiles.size());
    }

    void GameManagerMasterImpl::populateTournamentGameEventPlayer(ExternalHttpGameEventData::ExternalHttpGamePlayerEventDataList& playerDataList,
        SlotType slotType, const char8_t* encryptedBlazeId, BlazeId blazeId, PlayerState playerState)
    {
        // only participants need to sent to the tournament organizer
        if (isParticipantSlot(slotType))
        {
            // for security, only expose encrypted blaze ids, if present
            ExternalHttpGamePlayerEventData* playerData = playerDataList.pull_back();
            playerData->setEncryptedBlazeId(encryptedBlazeId);
            playerData->setBlazeId(encryptedBlazeId[0] != '\0' ? Blaze::INVALID_BLAZE_ID : blazeId);
            playerData->setPlayerState(playerState);
        }
    }

} // GameManager namespace
} // Blaze namespace
