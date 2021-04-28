/*! ************************************************************************************************/
/*!
    \file   gamepackerslaveimpl.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/


#include "framework/blaze.h"
#include "EAStdC/EAHashString.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/redis/redismanager.h"
#include "framework/controller/controller.h"
#include "gamemanager/tdf/gamepacker_server.h"
#include "gamemanager/tdf/gamepacker_internal_config_server.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/gamepacker/gamepackerslaveimpl.h"

#include "matchmaker/rules/propertiesrules/propertyrule.h"
#include "matchmaker/rules/propertiesrules/propertyruledefinition.h"

#include "framework/util/random.h"
#include "framework/usersessions/usersessionmanager.h"
#include "gamemanager/rpc/gamemanagermaster.h"
#include "gamemanager/commoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h" // PACKER_TODO: only for GameManager::lookupExternalUserByUserIdentification() once removed, get rid of it
#include "gamemanager/pinutil/pinutil.h"
#include "gamemanager/matchmakingfiltersutil.h"
#include "gamemanager/templateattributes.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "gamemanager/externalsessions/externalsessionutil.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamepacker/evalcontext.h"

#pragma push_macro("LOGGER_CATEGORY")
#undef LOGGER_CATEGORY
#define LOGGER_CATEGORY Blaze::BlazeRpcLog::gamepacker
#define LOG_JOB(job) "FinJob(" << job.mJobId << "), PackerSilo(" << job.mGamePackerSiloId << ")"

namespace Blaze
{

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<GameManager::ScenarioName>* scenario_name;
        extern TagInfo<GameManager::SubSessionName>* subsession_name;

        Metrics::TagInfo<GamePacker::PackerGameTemplateName>* packer_game_template_name = BLAZE_NEW TagInfo<GamePacker::PackerGameTemplateName>("packer_game_template_name");
        Metrics::TagInfo<GamePacker::RemovalReason>* packer_removal_reason = BLAZE_NEW TagInfo<GamePacker::RemovalReason>("packer_removal_reason");
        Metrics::TagInfo<const char8_t*>* packer_provisional_game = BLAZE_NEW TagInfo<const char8_t*>("packer_provisional_game");
        Metrics::TagInfo<uint32_t>* packer_game_score = BLAZE_NEW TagInfo<uint32_t>("packer_game_score", Blaze::Metrics::uint32ToString);
        Metrics::TagInfo<GameManager::PackerFinalizationTrigger>* packer_finalization_trigger = BLAZE_NEW TagInfo<GameManager::PackerFinalizationTrigger>("packer_finalization_trigger", [](const GameManager::PackerFinalizationTrigger& value, Metrics::TagValue&) { return GameManager::PackerFinalizationTriggerToString(value); });
        Metrics::TagInfo<const char8_t*>* packer_finalization_result = BLAZE_NEW TagInfo<const char8_t*>("packer_finalization_result");
        Metrics::TagInfo<const char8_t*>* packer_found_game_error = BLAZE_NEW TagInfo<const char8_t*>("packer_found_game_error");
        Metrics::TagInfo<const char8_t*>* packer_current_context = BLAZE_NEW TagInfo<const char8_t*>("packer_current_context");
        TagInfo<eastl::string>* packer_name = BLAZE_NEW TagInfo<eastl::string>("packer_name", [](const eastl::string& value, Metrics::TagValue&) { return value.c_str(); });
        TagInfo<eastl::string>* factor_name = BLAZE_NEW TagInfo<eastl::string>("factor_name", [](const eastl::string& value, Metrics::TagValue&) { return value.c_str(); });
        TagInfo<uint32_t>* score_percent = BLAZE_NEW TagInfo<uint32_t>("score_percent", Blaze::Metrics::uint32ToString);
    }
}

namespace GamePacker
{

enum AcquireWorkerSlotScriptResult
{
    CLAIM_RESULT_SLOT_ID,
    CLAIM_RESULT_PREV_SLOT_ID,
    CLAIM_RESULT_SLOT_VERSION_ID,
    CLAIM_RESULT_SLOT_COUNT,
    CLAIM_RESULT_MAX
};

static const char8_t* GAME_PACKER_LIST_NAME = "packerGameList"; // hardcoded for now

static const uint32_t MAX_GB_LIST_CAPACITY = 10; // PACKER_TODO: Replace with configurable value

static const char8_t* TIMED_JOB_STAGE[] = {
    "JOB_QUEUED",
    "JOB_LOCK_SCENARIOS",
    "JOB_CREATE_GAME",
    "JOB_FIND_GAME",
    "JOB_FIND_DS",
    "JOB_JOIN_EXTERNAL_SESSION",
    "JOB_TIMED_STAGES_MAX" // Sentinel
};

static EA_THREAD_LOCAL GamePackerSlaveImpl* gPackerSlaveImpl = nullptr;

// script that atomically obtains a worker slot identifier for the current instance. NOTE: path is relative to etc
RedisScript sAcquireWorkerSlotScript(1, "../component/gamemanager/db/redis/claim.lua", true);

bool isPlaceholderSessionId(PackerSessionId sessionId)
{
    return GetSliverIdFromSliverKey(sessionId) == INVALID_SLIVER_ID;
}

/**
 * A 'provisional' game is a game locally owned to the packer slave. It is not visible outside the instance.
 */
bool isProvisionalGameId(GameManager::GameId gameId)
{
    return GetSliverIdFromSliverKey(gameId) == INVALID_SLIVER_ID;
}

static GameManager::GameId generateUniqueProvisionalGameId()
{
    GameManager::GameId provisionalGameId = 0;
    GameManager::GameId uniqueIdBase = 0;
    auto idType = (uint16_t) GameManager::GAMEMANAGER_IDTYPE_GAME;
    BlazeError errRet = gUniqueIdManager->getId(RpcMakeSlaveId(GameManager::GameManagerMaster::COMPONENT_ID), idType, uniqueIdBase, SLIVER_KEY_BASE_MIN);
    if (errRet == Blaze::ERR_OK)
    {
        provisionalGameId = BuildSliverKey(INVALID_SLIVER_ID, uniqueIdBase); // Needed to ensure uniqueness because active games and provisional games both share id space
    }
    else
    {
        ERR_LOG("[GamePackerSlaveImpl].generateUniqueProvisionalGameId: failed to generate game id, got error '" << errRet << "'.");
    }
    return provisionalGameId;
}

static Blaze::GameManager::GameManagerMaster* getGmMaster()
{
    auto* component = static_cast<Blaze::GameManager::GameManagerMaster*>(gController->getComponent(Blaze::GameManager::GameManagerMaster::COMPONENT_ID, false, true));
    return component;
}

static Blaze::Search::SearchSlave* getSearchSlave()
{
    auto* component = static_cast<Blaze::Search::SearchSlave*>(gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, false, true));
    return component;
}

// static factory method
GamePackerSlave* GamePackerSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GamePackerSlaveImpl") GamePackerSlaveImpl();
}

float calculateWeightedGeometricMean(size_t count, const float* scores, const float* weights = nullptr)
{
    if (count == 0) return 0;

    if (weights == nullptr)
    {
        // unweighted geometric mean
        float result = scores[0];
        for (uint32_t i = 1; i < count; ++i)
        {
            result *= scores[i];
        }
        float divisor = 1.0f / count;
        result = pow(result, divisor);
        return result;
    }
    return 0;
}

GamePackerSlaveImpl::GamePackerSlaveImpl()
    : GamePackerSlaveStub()
    , mFinalizationJobQueue("GamePackerSlaveImpl::mFinalizationJobQueue")
    , mSessionsActive(getMetricsCollection(), "sessionsActive", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
    , mSessionsAcquired(getMetricsCollection(), "sessionsAcquired", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
    , mSessionsAborted(getMetricsCollection(), "sessionsAborted", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
    , mSessionsAcquireNone(getMetricsCollection(), "sessionsAcquireNone")
    , mSessionsAcquireFailed(getMetricsCollection(), "sessionsAcquireFailed")
    , mSessionsRemoved(getMetricsCollection(), "sessionsRemoved", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name, Metrics::Tag::packer_removal_reason)
    , mPlaceholderSessionsActive(getMetricsCollection(), "placeholderSessionsActive", Metrics::Tag::packer_game_template_name)
    , mPlaceholderSessionsAcquired(getMetricsCollection(), "placeholderSessionsAcquired", Metrics::Tag::packer_game_template_name)
    , mPlaceholderSessionsRemoved(getMetricsCollection(), "placeholderSessionsRemoved", Metrics::Tag::packer_game_template_name, Metrics::Tag::packer_removal_reason)
    , mGamesFinalized(getMetricsCollection(), "gamesFinalized", Metrics::Tag::packer_game_template_name, Metrics::Tag::packer_provisional_game, Metrics::Tag::packer_game_score, Metrics::Tag::packer_finalization_trigger, Metrics::Tag::packer_finalization_result)
    , mGamesPacked(getMetricsCollection(), "gamesPacked", Metrics::Tag::packer_game_template_name, Metrics::Tag::packer_provisional_game, Metrics::Tag::packer_game_score, Metrics::Tag::packer_finalization_trigger)
    , mPackerPlayersGauge(getMetricsCollection(), "players", Metrics::Tag::packer_name)
    , mPackerPartiesGauge(getMetricsCollection(), "parties", Metrics::Tag::packer_name)
    , mPackerViableGamesGauge(getMetricsCollection(), "viableGames", Metrics::Tag::packer_name)
    , mPackerGamesGauge(getMetricsCollection(), "games", Metrics::Tag::packer_name)
    , mPackerSilosGauge(getMetricsCollection(), "silos", Metrics::Tag::packer_name)
    , mPackerSilosCreatedCounter(getMetricsCollection(), "silosCreated", Metrics::Tag::packer_name)
    , mPackerSilosDestroyedCounter(getMetricsCollection(), "silosDestroyed", Metrics::Tag::packer_name)
    , mGamesCreatedCounter(getMetricsCollection(), "gamesCreated", Metrics::Tag::packer_name)
    , mGamesReapedCounter(getMetricsCollection(), "gamesReaped", Metrics::Tag::packer_name)
    , mViableGamesReapedCounter(getMetricsCollection(), "viableGamesReaped", Metrics::Tag::packer_name)
    , mFactorScoreFreq(getMetricsCollection(), "factorScoreFrequency", Metrics::Tag::packer_name, Metrics::Tag::factor_name, Metrics::Tag::score_percent)
    , mPackerFoundGameFailed(getMetricsCollection(), "packerFoundGameFailed", Metrics::Tag::packer_game_template_name, Metrics::Tag::packer_found_game_error)
    , mTotalPackingTime(getMetricsCollection(), "packingTime", Metrics::Tag::packer_game_template_name, Metrics::Tag::packer_current_context)
{
    mFinalizationJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
    gPackerSlaveImpl = this;

    mWorkSlotUpdateTimerId = INVALID_TIMER_ID;
    mWorkSlotUpdateFiberId = Fiber::INVALID_FIBER_ID;
    mWorkSlotCount = 0;
    mAssignedWorkSlotId = 0;
    mAssignedWorkSlotVersion = 0;
    mProductionId = EA::StdC::FNV64_String8(COMPONENT_INFO.name);
    mSweepEmptySilosTimerId = INVALID_TIMER_ID;

    EA::TDF::TypeDescriptionSelector<GameManager::PropertyValueMap>::get(); // PACKER_TODO: hacky registration, fix this
    EA::TDF::TypeDescriptionSelector<GameManager::PropertyValueMapList>::get(); // PACKER_TODO: hacky registration, fix this
}

GamePackerSlaveImpl::~GamePackerSlaveImpl()
{
    EA_ASSERT(mSweepEmptySilosTimerId == INVALID_TIMER_ID);
    gPackerSlaveImpl = nullptr;
    delete mExternalGameSessionServices;
    delete mReteNetwork;
    delete mRuleDefinitionCollection;
}

// PACKER_TODO: We may be able to push the relinquish timeout handling down to the packer library, this way we would have a single unified flow of timing out sessions that could only occur when reaping the games due to expiry, or nonviability. We would have to add Packer library support for both expiry and non-viability timeouts for each party, this may be a bit confusing given that timeouts should really be governed outside of the packersilo because multiple silos could technically share a single party reference!
void GamePackerSlaveImpl::migrateSession(PackerSessionId packerSessionId)
{
    EA_ASSERT(!isPlaceholderSessionId(packerSessionId));
    // PACKER_TODO: Add support for correctly handling multi-silo migration
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    if (packerSessionItr == mPackerSessions.end())
        return;

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());
    EA_ASSERT_MSG(packerSession.mFinalizationJobId == 0, "Migration timer should always be disabled when session is part of a finalization job!");

    packerSession.mMigrateSessionTimerId = INVALID_TIMER_ID;

    // PACKER_TODO: mTotalNewPartyCount is per template, not per silo because a single session may add parties in *many* silos, but it would be useful to determine this information in debug RPC that dumps all the sessions and their silos, etc.
    auto newTemplateSessionsSinceStart = packerSession.mGameTemplate.mMetrics.mTotalNewPartyCount - packerSession.mParentMetricsSnapshot.mTotalNewPartyCount;
    if (isRootLayer())
    {
        INFO_LOG("[GamePackerSlaveImpl].migrateSession: session(" << packerSessionId << ") exceeded viability deadline time quantum(" << packerSession.getMigrateUnfulfilledTime().getMillis() << "ms) on layer(" << getAssignedLayerId() << "), inPackerSilos(" << packerSession.mMatchedPackerSiloIdSet.size() << "), newCandidatesSessionsSinceStart(" << newTemplateSessionsSinceStart << "); retain session"); // PACKER_TODO: change to trace
    }
    else
    {
        INFO_LOG("[GamePackerSlaveImpl].migrateSession: session(" << packerSessionId << ") exceeded viability deadline time quantum(" << packerSession.getMigrateUnfulfilledTime().getMillis() << "ms) on layer(" << getAssignedLayerId() << "), inPackerSilos(" << packerSession.mMatchedPackerSiloIdSet.size() << "), newCandidatesSessionsSinceStart(" << newTemplateSessionsSinceStart << "); relinquish session to owner"); // PACKER_TODO: change to trace
        
        Fiber::BlockingSuppressionAutoPtr blockingSuppression; // this function doesn't block, all rpc calls made from it must be ignoreReply=true
        RpcCallOptions opts;
        opts.ignoreReply = true;
        Blaze::GameManager::WorkerMigratePackerSessionRequest request;
        request.setPackerSessionId(packerSessionId);
        BlazeError rc = getMaster()->workerMigratePackerSession(request, opts);
        if (rc == ERR_OK)
        {
            TRACE_LOG("[GamePackerSlaveImpl].migrateSession: Released session(" << packerSessionId << ") to owner for migration");
        }
        else
        {
            ERR_LOG("[GamePackerSlaveImpl].migrateSession: Failed to release session(" << packerSessionId << ") to owner for migration, error: " << rc);
        }
        cleanupSession(packerSessionId, GameManager::WORKER_SESSION_MIGRATED);
    }
}

void GamePackerSlaveImpl::abortSession(PackerSessionId packerSessionId, GameManager::MatchmakingScenarioId originatingScenarioId)
{
    RpcCallOptions opts;
    opts.ignoreReply = true;
    Blaze::GameManager::WorkerAbortPackerSessionRequest abortReq;
    abortReq.setPackerSessionId(packerSessionId);
    BlazeError rc = getMaster()->workerAbortPackerSession(abortReq, opts);
    if (rc != ERR_OK)
    {
        WARN_LOG("[GamePackerSlaveImpl].abortSession: Failed to abort session(" << packerSessionId << "), scenario(" << originatingScenarioId << "), error: " << rc);
    }
}

void GamePackerSlaveImpl::cleanupSession(PackerSessionId packerSessionId, GameManager::WorkerSessionCleanupReason removalReason, bool repack)
{
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    auto* removalReasonStr = GameManager::WorkerSessionCleanupReasonToString(removalReason);
    if (packerSessionItr == mPackerSessions.end())
    {
        WARN_LOG("[GamePackerSlaveImpl].cleanupSession: Expected session(" << packerSessionId << ") not found, removalReason(" << removalReasonStr << ")");
        return;
    }

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    if (packerSession.mFinalizationJobId != 0)
    {
        ASSERT_LOG("[GamePackerSlaveImpl].cleanupSession: Cleanup failed, session(" << packerSessionId << ") still finalizing, removalReason(" << removalReasonStr << ")");
        return;
    }

    packerSession.setCleanupReason(removalReasonStr);

    TRACE_LOG("[GamePackerSlaveImpl].cleanupSession: Cleanup session(" << packerSessionId << ") in scenario(" << packerSession.getOriginatingScenarioId() << "), remove from packer silos count(" << packerSession.mMatchedPackerSiloIdSet.size() << ")");
    EA_ASSERT(packerSession.mState == PackerSession::PACKING || packerSession.mState == PackerSession::AWAITING_RELEASE);
    auto* gameTemplateName = packerSession.mGameTemplate.mGameTemplateName.c_str();
    if (isPlaceholderSessionId(packerSessionId))
    {
        EA_ASSERT_MSG(!packerSession.mRegisteredWithRete, "Placeholder sessions currently never registered with rete directly."); // NOTE: This may need to change if we add support for negative filter conditions...
        mPlaceholderSessionsActive.decrement(1, gameTemplateName);
        mPlaceholderSessionsRemoved.increment(1, gameTemplateName, removalReasonStr);

        for (auto packerSiloId : packerSession.mMatchedPackerSiloIdSet)
        {
            auto* packerSilo = getPackerSilo(packerSiloId);
            if (packerSilo == nullptr)
            {
                TRACE_LOG("[GamePackerSlaveImpl].cleanupSession: session(" << packerSessionId << ") not present in PackerSilo(" << packerSiloId << ")");
                continue;
            }
            removeSessionFromPackerSilo(packerSession, *packerSilo, removalReasonStr);
            if (repack)
                packGames(packerSiloId); // PACKER_TODO: replace this with a pack on release auto ptr that will pack once last write reference to the silo is released
            else
            {
                // PACKER_TODO: This is a temporary state of affairs, cleanup should never directly call packGames in the future, instead we'll do one of two options:
                // 1) Create a new object called PackerSiloChangeSet that tracks packer silos that were modified by operations such as removing a party, and ensure such operations always trigger after the last reference to the changeset is released.
                // 2) Manually separate silo mutation operations into a separate stage and ensure that any silo operation goes through a standardized state machine which will explicitly call packGames() after every silo modifying operation...
                TRACE_LOG("[GamePackerSlaveImpl].cleanupSession: session(" << packerSessionId << ") in PackerSilo(" << packerSiloId << ") will be removed without auto-repacking");
            }
        }
    }
    else
    {
        // PACKER_TODO: Inline this, its only called here!
        packerSession.removeFromReteIndex(); // calls removeSessionFromPackerSilo(), packGames()

        auto* hostJoinInfo = packerSession.getHostJoinInfo();
        if (hostJoinInfo == nullptr) // PACKER_TODO: this should be changed into an ASSERT, should always have host join info for "real" packer sessions?
        {
            WARN_LOG("[GamePackerSlaveImpl].cleanupSession: Cleanup session(" << packerSessionId << ") in scenario(" << packerSession.getOriginatingScenarioId()
                << ") using template(" << gameTemplateName << ") does not have host info needed for metric tags");
        }
        else
        {
            auto& scenarioInfo = hostJoinInfo->getScenarioInfo();
            mSessionsActive.decrement(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
            mSessionsRemoved.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName(), removalReasonStr);
        }
    }

    mPackerSessions.erase(packerSessionItr); // yank it from the map to avoid insertion code trying to do the same thing

    delete &packerSession;
}

void GamePackerSlaveImpl::sendSessionStatus(PackerSessionId packerSessionId)
{
    EA_ASSERT(!isPlaceholderSessionId(packerSessionId));
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    if (packerSessionItr == mPackerSessions.end())
        return;

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    EA_ASSERT_MSG(packerSession.mFinalizationJobId == 0, "Session status sending should be disabled when part of finalization job");
    packerSession.mSendUpdateTimerId = INVALID_TIMER_ID;

    // build a session status update message and send it to the master
    Blaze::GameManager::WorkerUpdatePackerSessionStatusRequest request;
    request.setPackerSessionId(packerSessionId);
    auto& factorScores = request.getGameFactorScores();
    const Packer::Game* currentGame = nullptr;
    // find the best factor scores of all the p-games the session(ie: party) is currently part of
    for (auto packerSiloId : packerSession.mMatchedPackerSiloIdSet)
    {
        auto* packer = getPackerSilo(packerSiloId);
        EA_ASSERT(packer != nullptr);
        auto* party = packer->getPackerParty(packerSessionId);
        EA_ASSERT(party != nullptr);
        auto gameItr = packer->getGames().find(party->mGameId);
        EA_ASSERT(gameItr != packer->getGames().end());
        auto& game = gameItr->second;
        if (factorScores.size() < packer->getFactors().size())
            factorScores.resize(packer->getFactors().size(), 0.0f);
        bool replace = false;
        for (auto& factor : packer->getFactors())
        {
            auto newScore = game.mFactorScores[factor.mFactorIndex];
            auto curScore = factorScores[factor.mFactorIndex];
            if (replace || newScore > curScore)
            {
                if (!replace)
                {
                    if (currentGame != &game)
                    {
                        if (currentGame != nullptr)
                        {
                            TRACE_LOG("[GamePackerSlaveImpl].sendSessionStatus: session(" << packerSessionId << ") quality factor(" << factor.mFactorIndex << ":" << factor.mFactorName << ") new score(" << newScore << ") in game(" << game.mGameId << ") supercedes previous best score(" << curScore << ") in game(" << currentGame->mGameId << ")");
                        }
                        currentGame = &game;
                    }
                    replace = true;
                }
                factorScores[factor.mFactorIndex] = newScore;
            }
            else if (newScore < curScore)
                break;
        }
    }

    auto overallScore = calculateWeightedGeometricMean((uint32_t)factorScores.size(), factorScores.begin(), nullptr);
    request.setGameQualityScore(overallScore);

    RpcCallOptions callOpts;
    callOpts.ignoreReply = true;
    getMaster()->workerUpdatePackerSessionStatus(request, callOpts);
}

void GamePackerSlaveImpl::handlePackerImproveGame(const Packer::Game& game, const Packer::Party& incomingParty)
{
    auto& packer = game.mPacker;
    GameManager::PackerGamePartyAdded partyAddedToGame;
    partyAddedToGame.setPackerSiloId(packer.getPackerSiloId());
    partyAddedToGame.setPackerGameId(game.mGameId);
    partyAddedToGame.setJoinedPartyId(incomingParty.mPartyId);
    auto& factorScores = partyAddedToGame.getGameFactorScores();
    factorScores.reserve(game.mFactorScores.size());
    StringBuilder sb;
    for (auto score : game.mFactorScores)
    {
        if (IS_LOGGING_ENABLED(Logging::TRACE))
            sb << score << ",";
        factorScores.push_back(score);
    }
    if (!sb.isEmpty())
        sb.trim(1); // trim last comma

    TRACE_LOG("[GamePackerSlaveImpl].handlePackerImproveGame: update game(" << game.mGameId << ") quality factor scores(" << sb << ")");
    TRACE_LOG("[GamePackerSlaveImpl].handlePackerImproveGame: " << partyAddedToGame);

    auto packerSessionItr = mPackerSessions.find(incomingParty.mPartyId);
    EA_ASSERT(packerSessionItr != mPackerSessions.end());
    packerSessionItr->second->mParentMetricsSnapshot.mAddedToGameNewPartyCount = packerSessionItr->second->mGameTemplate.mMetrics.mTotalNewPartyCount; // PACKER_TODO: have this happen per silo at the packer lib
    auto partyCount = game.getPartyCount(Packer::GameState::PRESENT);
    bool nowViable = (game.mViableGameQueueItr.mpNode != nullptr);
    auto now = TimeValue::getTimeOfDay();

    for (int32_t partyIndex = 0; partyIndex < partyCount; ++partyIndex)
    {
        auto& gameParty = game.mGameParties[partyIndex];
        auto partyId = gameParty.mPartyId;
        if (gameParty.mImmutable)
        {
            packerSessionItr = mPackerSessions.find(partyId);
            EA_ASSERT(packerSessionItr != mPackerSessions.end());
            continue; // skip immutable parties, they do not track improvements
        }

        packerSessionItr = mPackerSessions.find(partyId);
        EA_ASSERT(packerSessionItr != mPackerSessions.end());

        auto& packerSession = *packerSessionItr->second;
        if (packerSession.mSendUpdateTimerId == INVALID_TIMER_ID)
        {
            packerSession.mSendUpdateTimerId = gSelector->scheduleTimerCall(now + PackerSession::SEND_UPDATE_PERIOD_MS, this, &GamePackerSlaveImpl::sendSessionStatus, packerSession.mPackerSessionId, "GamePackerSlaveImpl::sendSessionStatus");
        }

        if (packerSession.mMigrateSessionTimerId == INVALID_TIMER_ID)
        {
            if (!nowViable)
            {
                auto curSiloId = game.mPacker.getPackerSiloId();
                bool otherViable = false;
                for (auto packerSiloId : packerSession.mMatchedPackerSiloIdSet)
                {
                    if (curSiloId == packerSiloId)
                        continue; // skip current silo
                    otherViable = isPackerPartyInViableGame(packerSiloId, partyId);
                    if (otherViable)
                        break;
                }
                if (!otherViable)
                {
                    const auto migrateAtTime = packerSession.mSessionStartTime + packerSession.getMigrateUnfulfilledTime();
                    packerSession.mMigrateSessionTimerId = gSelector->scheduleTimerCall(migrateAtTime, this, &GamePackerSlaveImpl::migrateSession, packerSession.mPackerSessionId, "GamePackerSlaveImpl::migrateSession");
                }
            }
        }
        else if (nowViable)
        {
            packerSession.cancelMigrateTimer();
        }
    }
}

void GamePackerSlaveImpl::handlePackerRetireGame(const Packer::Game& game)
{
    // Empty for now
}

void GamePackerSlaveImpl::handlePackerEvictParty(const Packer::Game& game, const Packer::Party& evictedParty, const Packer::Party& incomingParty)
{
    auto& packer = game.mPacker;
    GameManager::PackerGamePartyEvicted partyEvictedFromGame;
    partyEvictedFromGame.setPackerSiloId(packer.getPackerSiloId());
    partyEvictedFromGame.setPackerGameId(game.mGameId);
    partyEvictedFromGame.setEvictedPartyId(evictedParty.mPartyId);
    partyEvictedFromGame.setEvictorPartyId(incomingParty.mPartyId);
    TRACE_LOG("[GamePackerSlaveImpl].handlePackerEvictParty: " << partyEvictedFromGame);
}

void GamePackerSlaveImpl::handlePackerReapGameAux(const Packer::Game& game, bool viable)
{
    static const uint32_t NUM_PERCENT = 10;

    auto& packer = game.mPacker;
    auto& packerSiloName = packer.getPackerName();

    TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: PackerSilo(" << packer.getPackerSiloId() << "), game(" << game.mGameId << "), gameViable(" << viable << ")");

    auto packerDetailedMetricsItr = mPackerDetailedMetricsByGameTemplateMap.find(packerSiloName);
    if (packerDetailedMetricsItr == mPackerDetailedMetricsByGameTemplateMap.end())
    {
        // PACKER_TODO(metrics): log error
        EA_FAIL();
        return;
    }
    auto& detailedMetrics = packerDetailedMetricsItr->second;
    for (auto& factor : packer.getFactors())
    {
        auto scorePercent = (uint32_t)(game.mFactorScores[factor.mFactorIndex] * 100.0f);
        if (scorePercent > 100)
            scorePercent = 100; // safety net

        auto& allGameFreqByScorePercentile = detailedMetrics.mFactorScoreBreakdownForAllGames[factor.mFactorName];
        ++allGameFreqByScorePercentile[scorePercent]; // PACKER_TODO(metrics): BAD NAME

        if (viable)
        {
            auto& viableGameFreqsByScorePercentile = detailedMetrics.mFactorScoreBreakdownForReapedGames[factor.mFactorName];
            ++viableGameFreqsByScorePercentile[scorePercent];

            // bucket percent scores
            uint32_t roundToNearest = (scorePercent / NUM_PERCENT) * NUM_PERCENT;
            mFactorScoreFreq.increment(1, packerSiloName, factor.mFactorName, roundToNearest);
        }
    }

    if (viable)
    {
        // NOTE: Reaped viable game parties wont have their packerReapParty() callback called, therefore we must process them here.
        auto now = TimeValue::getTimeOfDay();
        auto partyCount = game.getPartyCount(Packer::GameState::PRESENT);
        for (auto partyIndex = 0; partyIndex < partyCount; ++partyIndex)
        {
            auto& gameParty = game.mGameParties[partyIndex];
            if (gameParty.mImmutable)
                continue; // skip immutable parties as they are just placeholders
            auto partyId = gameParty.mPartyId;
            auto* party = packer.getPackerParty(partyId);
            EA_ASSERT(party != nullptr);
            TimeValue timeSpentInViableGames = party->mTotalTimeInViableGames;
            if (game.mViableTimestamp > party->mAddedToGameTimestamp)
            {
                timeSpentInViableGames += (now - game.mViableTimestamp);
            }
            else
            {
                timeSpentInViableGames += (now - party->mAddedToGameTimestamp);
            }

            // PACKER_TODO: this may very well be problematic for quantities that can grow without bound (like eviction count) since it may result in arbitrarily large number of entries in the detailed metrics. We probably need to store all quantities in a log-quantized form to avoid taking too much space.
            auto timeSpentInViableGameSec = timeSpentInViableGames.getSec();
            auto timeInPacker = now - party->mCreatedTimestamp;
            auto timeInPackerSec = timeInPacker.getSec();
            detailedMetrics.mTimeSpentInViableGamesByParties[timeSpentInViableGameSec]++;
            detailedMetrics.mTimeSpentInViableGamesBySuccessfulParties[timeSpentInViableGameSec]++;
            detailedMetrics.mTimeSpentInPackerByParty[timeInPackerSec]++;
            detailedMetrics.mTimeSpentInPackerBySuccessfulParty[timeInPackerSec]++;
            detailedMetrics.mEvictionCountByParties[party->mEvictionCount]++;
            detailedMetrics.mEvictionCountBySuccessfulParties[party->mEvictionCount]++;
        }

        mViableGamesReapedCounter.increment(1, packerSiloName); // PACKER_TODO: make reaped counter use tags, viable, and total
    }

    mGamesReapedCounter.increment(1, packerSiloName);
}

void GamePackerSlaveImpl::handlePackerReapGame(const Packer::Game& game, bool viable)
{
    handlePackerReapGameAux(game, viable);
    
    if (!viable)
    {
        return; // Currently skip games that were not viable when reaped, we already handle expired/recycled parties in a different callback
    }
    const auto partyCount = game.getPartyCount(Packer::GameState::PRESENT);
    const auto playerCount = game.getPlayerCount(Packer::GameState::PRESENT);
    const auto& packer = game.mPacker;
    const auto* gameTemplateName = packer.getPackerName().c_str();
    const auto packerSiloId = packer.getPackerSiloId();
    const auto gameScore = calculateWeightedGeometricMean(game.mFactorScores.size(), game.mFactorScores.begin());
    GameManager::PackerGameCompleted gameCompletedEvent;
    gameCompletedEvent.setPackerSiloId(packer.getPackerSiloId());
    gameCompletedEvent.setCompletedGameId(game.mGameId);
    gameCompletedEvent.setEvictedParties(game.mEvictedCount);
    gameCompletedEvent.getParties().reserve(partyCount);
    gameCompletedEvent.getPlayerIds().reserve(playerCount);
    gameCompletedEvent.getGameFactorScores().assign(game.mFactorScores.begin(), game.mFactorScores.end());
    gameCompletedEvent.setGameScore(gameScore);

    eastl::vector<PackerSession*> pendingSessions;      // These are the Sessions that have Matched and are being Packed into the Game. 
    pendingSessions.reserve(partyCount);
    BlazeId hostUserId = INVALID_BLAZE_ID;
    PackerSession* ownerSession = nullptr;
    GameManager::MatchmakingCreateGameRequestPtr createGameRequest = BLAZE_NEW GameManager::MatchmakingCreateGameRequest;
    uint64_t gameRecordVersion = 0;
    int32_t immutablePartyCount = 0;
    GameManager::PingSiteAliasList pingSiteAliases;

    for (auto partyIndex = 0; partyIndex < partyCount; ++partyIndex)
    {
        // NOTE: We deliberately add trace events for immutable parties, to provide more clarity about the state of the game when it is reaped
        const auto partyId = game.mGameParties[partyIndex].mPartyId;
        const auto* party = packer.getPackerParty(partyId);
        const auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, Packer::GameState::PRESENT);
        EA_ASSERT(party != nullptr);
        EA_ASSERT(teamIndex != -1);
        {
            // party trace event entry
            auto& eventParty = *gameCompletedEvent.getParties().pull_back();
            eventParty.setPartyId(partyId);
            eventParty.setLastImprovedTime(party->mAddedToGameTimestamp - party->mCreatedTimestamp); // relative to creation
            eventParty.setWaitTime(TimeValue::getTimeOfDay().getMicroSeconds() - party->mCreatedTimestamp); // relative to current time
            eventParty.setTeamIndex((uint32_t)teamIndex);
            for (auto& partyPlayer : party->mPartyPlayers)
                gameCompletedEvent.getPlayerIds().push_back(partyPlayer.mPlayerId);
        }
        auto packerSessionItr = mPackerSessions.find(partyId);
        if (packerSessionItr == mPackerSessions.end())
        {
            EA_FAIL(); // PACKER_TODO: handle removal of invalid packer sessions, even though this should never happen!
            continue;
        }
        auto& packerSession = *packerSessionItr->second;
        if (party->mImmutable)
        {
            EA_ASSERT_MSG(packerSession.mDelayCleanupTimestamp == 0, "Placeholder sessions currently don't exist in multiple silos so they cannot have already been cleaned up.");
            EA_ASSERT(packerSession.mImmutableGameId == (GameManager::GameId) game.mGameId);
            // The packer sessions associated with immutable parties are not indexed in RETE, and do not participate in finalization, therefore they can be removed immediately.
            TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: queue cleanup for placeholder session(" << partyId << ") in scenario(" << packerSession.getOriginatingScenarioId() << ") from game(" << game.mGameId << ") prior to finalization");
            packerSession.mState = PackerSession::AWAITING_RELEASE;
            packerSession.setCleanupReason("viable");
            packerSession.mDelayCleanupTimestamp = TimeValue::getTimeOfDay();
            if (gameRecordVersion == 0)
            {
                gameRecordVersion = packerSession.mImmutableGameVersion;
            }
            else
            {
                EA_ASSERT(gameRecordVersion == packerSession.mImmutableGameVersion);
            }
            mDelayCleanupSessions.push_back(partyId);
            ++immutablePartyCount;
            continue; // skip immutable parties, since they are already in game on the GM master
        }
        packerSession.mRequeuedCount += party->mRequeuedCount; 
        packerSession.mUnpackedCount += party->mUnpackedCount;
        packerSession.mEvictedCount += party->mEvictionCount; // snapshot number of times party was evicted in this particular silo; useful for troubleshooting. // PACKER_TODO: Due to the fact that a finalizing session will always remove itself from all other silos we currently lose metrics like cumulative eviction counts that occurred for a particular session in a particular silo that was not used for finalization. Figure out whether this matters...
        packerSession.mAddedToGameTimestamp = party->mAddedToGameTimestamp;
        packerSession.mTotalTimeInViableGames += party->mTotalTimeInViableGames;
        packerSession.mAddedToJobNewPartyCount = packerSession.mGameTemplate.mMetrics.mTotalNewPartyCount; // PACKER_TODO: party count should be stored per silo not per template, need to solve issue with silo going away while sessions are finalizing, that should not happen until after all packer sessions referencing the silo are truly deleted, that way there is continuity.
        LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());
        auto& cgMasterRequest = createGameRequest->getCreateReq();
        auto& cgGameRequest = cgMasterRequest.getCreateRequest();

        auto* packerSiloInfo = getPackerSiloInfo(packerSiloId);
        if (pendingSessions.empty())
        {
            GameManager::TemplateAttributes attributes;
            GameManager::PropertyNameMap properties;
            EA_ASSERT(packerSiloInfo != nullptr);

            // Copy Properties from the Base request, then add the values from the Factors:
            packerSiloInfo->mPackerSiloProperties.copyInto(properties);

            // Gather the Attributes from the Packer, and fill in the values on the CG request: 
            bool appliedAttributes = false;
            const auto& templateItr = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().find(gameTemplateName);
            if (templateItr != getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end())
            {
                const auto& templateConfig = *templateItr->second;
                for (auto& factor : packer.getFactors())
                {
                    auto* inputPropertyDescriptor = factor.getInputPropertyDescriptor();
                    if (inputPropertyDescriptor == nullptr)
                        continue; // skip factors that don't take any input properties, since they do not choose property fields

                    auto* inputPropertyName = inputPropertyDescriptor->mPropertyName.c_str();

                    // PACKER_TODO: fix this hardcoding that determines whether the factor is driving the
                    // game.pingSite property, the solution is to allow any factor to output an ordered list of
                    // fields into a game template attribute (if/when the packer finalization stage that performs
                    // reset dedicated server processing can accept a template attribute list that defines the 
                    // order of pingsites to try for resetting the dedicated server, this will not be needed...)
                    if (blaze_strnicmp(inputPropertyName, GameManager::PROPERTY_NAME_PING_SITE_LATENCIES, strlen(GameManager::PROPERTY_NAME_PING_SITE_LATENCIES)) == 0)
                    {
                        Packer::FieldScoreList fieldScoreList;
                        factor.computeSortedFieldScores(fieldScoreList, game);
                        pingSiteAliases.reserve(fieldScoreList.size());
                        for (auto& fieldScore : fieldScoreList)
                        {
                            auto& fieldName = fieldScore.getFieldName();
                            pingSiteAliases.push_back(fieldName.c_str());
                            TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: Write gqf(" << factor.mFactorName
                                << ") field output to game.pingSiteAliasList[" << pingSiteAliases.size() - 1 << "]=" << fieldName << ", used to reset dedicated server");
                        }
                    }
                    else if (blaze_strnicmp(inputPropertyName, GameManager::PROPERTY_NAME_GAME_ATTRIBUTES, strlen(GameManager::PROPERTY_NAME_GAME_ATTRIBUTES)) == 0)
                    {
                        EA_ASSERT(game.mFactorFields.size() > factor.mFactorIndex);
                        auto* fieldDesc = game.mFactorFields[factor.mFactorIndex];
                        if (fieldDesc != nullptr)
                        {
                            auto& outputPropertyName = factor.mOutputPropertyName;
                            // Add the Property if a field was chosen:
                            auto ret = properties.insert(outputPropertyName.c_str());
                            if (ret.second)
                            {
                                auto& fieldName = fieldDesc->mFieldName;
                                ret.first->second = properties.allocate_element();
                                ret.first->second->set(fieldName.c_str());
                                TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: Write gqf(" << factor.mFactorName
                                    << ") field output to " << outputPropertyName << "=" << fieldName << ", used to fill create game template");
                            }
                        }
                    }
                }

                // Set the Base attributes based on (random player's) values: 
                // (This means that we rely on Scenario AttributeOverrides to prevent hacked clients from setting Attributes that are not allowed.)
                packerSession.getAttributeMap().copyInto(attributes);

                // Build the GameCreationData again.
                // Some functionality is missing here: 
                //  It's not possible to override values in the GCD based on Scenario overrides.  This mostly applies to the GlobalAttributes (which in turn are mostly for common game data). 

                // PACKER_TODO:  Rather than overriding values that are stored in the master request, it would be better to not store any of the original request info (except what is required).
                GameManager::CreateGameRequest cgReq = templateConfig.getBaseRequest();
                const char8_t* failingAttr = "";
                StringBuilder componentDescription;
                componentDescription << "In GamePackerSlaveImpl::handlePackerReapGame for cgTemplate " << gameTemplateName;

                BlazeRpcError overallResult = applyTemplateAttributes(EA::TDF::TdfGenericReference(cgReq), templateConfig.getAttributes(), attributes, properties, failingAttr, componentDescription.get(), true, &mPropertyManager);
                if (overallResult == ERR_OK)
                {
                    appliedAttributes = true;

                    // The Data Source List comes from either Scenarios or CG Templates.  This goes against Packer design (since we shouldn't change Game properties based on Scenario data without separate Packers), but I
                    // PACKER_TODO:  Revise the logic to only set the DataSourceNameList from the CG Template config.  (after copying over GameCreationData)
                    GameManager::DataSourceNameList tempList = cgGameRequest.getGameCreationData().getDataSourceNameList();

                    // Copy over to real location: 
                    cgReq.getGameCreationData().copyInto(cgGameRequest.getGameCreationData());

                    // PACKER_TODO: Just use templateConfig.getExternalDataSourceApiList()
                    tempList.copyInto(cgGameRequest.getGameCreationData().getDataSourceNameList());
                }
                else
                {
                    ERR_LOG("[GamePackerSlaveImpl].handlePackerReapGame:  create game, failed because required attribute (" << failingAttr << ") was missing for CG template " << gameTemplateName);
                    return;
                }
            }

            ownerSession = &packerSession;
            TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: session(" << partyId << ") is finalization job initiator for game(" << game.mGameId << ")");
            // this is assumed to be acceptable for everyone involved
            cgGameRequest.getPlayerJoinData().setGameEntryType(packerSession.mMmInternalRequest->getRequest().getPlayerJoinData().getGameEntryType());
            cgMasterRequest.setCreateGameTemplateName(gameTemplateName);
            cgMasterRequest.setFinalizingMatchmakingSessionId(partyId);
            {
                // We've already filled in the basic cgGameRequest.getGameCreationData(); after applying the attributes
                packerSession.mMmInternalRequest->getRequest().getCommonGameData().copyInto(cgGameRequest.getCommonGameData());
                packerSession.mMmInternalRequest->getGameExternalSessionData().copyInto(cgMasterRequest.getExternalSessionData());

                // gm still uses these deprecated slot capacities in validation, so keeping for now
                uint16_t totalSlotCapacity = 0;
                for (auto& slotCapacityIter : cgGameRequest.getGameCreationData().getSlotCapacitiesMap())
                {
                    cgGameRequest.getSlotCapacities()[slotCapacityIter.first] = slotCapacityIter.second;
                    cgGameRequest.getSlotCapacitiesMap().insert(slotCapacityIter);
                    totalSlotCapacity += slotCapacityIter.second;
                }
                auto maxPlayerCapacity = cgGameRequest.getGameCreationData().getMaxPlayerCapacity();
                if (totalSlotCapacity > maxPlayerCapacity)
                {
                    // PACKER_TODO: rather than forcing capacity here, we really need to validate that our requests are well formed when we first receive them.
                    // All this checking/massaging can be done by the packer master since it knows all this information, and can unilaterally reject invalid requests at the source.
                    WARN_LOG("[GamePackerSlaveImpl].handlePackerReapGame: Required totalSlotCapacity(" << totalSlotCapacity << ") exceeds maxPlayerCapacity(" << maxPlayerCapacity << ") for initiator session(" << partyId << "), forcing increase maxPlayerCapacity to accomodate request");
                    cgGameRequest.getGameCreationData().setMaxPlayerCapacity(totalSlotCapacity);
                }
            }

            // also need to set up roles if not specified in template: PACKER_TODO: templates should do this automatically if unspecified
            if (cgGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap().empty())
            {
                GameManager::RoleCriteriaPtr roleCriteria = cgGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap().allocate_element();
                auto slotCapacitiesPlayerCount = GameManager::getTotalParticipantCapacity(cgGameRequest.getSlotCapacities()); // use slot capacities filled in above
                auto teamCount = cgGameRequest.getGameCreationData().getTeamIds().size();
                auto roleCapacity = (teamCount == 0) ? slotCapacitiesPlayerCount : (slotCapacitiesPlayerCount / teamCount);
                roleCriteria->setRoleCapacity(roleCapacity);
                cgGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap()[""] = roleCriteria;
            }
        }
        else
        {
            TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: session(" << partyId << ") is finalization job participant for game(" << game.mGameId << ")");
        }

        // Setup platforms list based on what is supported by the silo.  Should be just a list of values based on the PlatformFilter, but may need conversion. 
        {
            // PlatformFilter
            getPropertyRuleDefinition()->updatePlatformList(packerSession.getFilterMap(), packerSiloInfo->mPackerSiloProperties, cgGameRequest.getClientPlatformListOverride());
        }

        pendingSessions.push_back(&packerSession);
        EA_ASSERT(!party->mPartyPlayers.empty());
        for (auto& partyPlayer : party->mPartyPlayers)
        {
            bool foundPlayerJoinData = false;
            auto& playerJoinData = packerSession.mMmInternalRequest->getRequest().getPlayerJoinData();
            for (auto& playerDataFrom : playerJoinData.getPlayerDataList())
            {
                // NOTE: PlayerJoinData contains the original set of player info sent by the client, it does not get modified even if some of the members of the party end up leaving while the master keeps the packer session alive. This means we need to be careful *not* to use the PlayerJoinData as the 'source of truth' for determining which players will be added to the game.
                if (playerDataFrom->getUser().getBlazeId() == partyPlayer.mPlayerId)
                {
                    // override team and role info from the packed game details
                    auto playerDataTo = cgGameRequest.getPlayerJoinData().getPlayerDataList().pull_back();
                    playerDataFrom->copyInto(*playerDataTo);
                    playerDataTo->setTeamIndex((GameManager::TeamIndex)teamIndex);
                    playerDataTo->setRole(GameManager::PLAYER_ROLE_NAME_ANY);

                    auto userInfoTo = cgMasterRequest.getUsersInfo().pull_back();
                    for (auto& userInfoFrom : packerSession.mMmInternalRequest->getUsersInfo())
                    {
                        if (userInfoFrom->getUser().getUserInfo().getId() == partyPlayer.mPlayerId)
                        {
                            userInfoFrom->copyInto(*userInfoTo);
                            if (hostUserId == INVALID_BLAZE_ID)
                            {
                                hostUserId = partyPlayer.mPlayerId;
                                userInfoTo->setIsHost(true);
                            }
                            else
                            {
                                // each mm session has one user marked as host. If we've picked one, make sure we don't have another.
                                userInfoTo->setIsHost(false);
                            }
                            break;
                        }
                    }
                    foundPlayerJoinData = true;
                    break;
                }
            }

            if (!foundPlayerJoinData)
            {
                ERR_LOG("[GamePackerSlaveImpl].handlePackerReapGame: session(" << partyId << "), didn't have player join data for player(" << partyPlayer.mPlayerId << ") when building CreateGame Request!");
                EA_FAIL(); // PACKER_TODO: must handle removal of invalid packer sessions since we are just bailing!
                return;
            }
        }
    }

    if (pendingSessions.empty())
    {
        if (immutablePartyCount == partyCount)
        {
            TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: game(" << game.mGameId << ") in PackerSilo(" << packerSiloId << ") contains only immutable parties(" << immutablePartyCount << "), nothing to finalize.");
        }
        else
        {
            EA_FAIL_MSG("Mutable parties exist, must have pending sessions!");
        }
        return;
    }
    EA_ASSERT_MSG(ownerSession != nullptr, "Finalization owner must exist!");

    TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: PackerSilo(" << packerSiloId << ") initiate finalize session(" << ownerSession->mPackerSessionId << ")");

    PackerFinalizationJobId finalizationJobId = 0;
    BlazeError errRet = gUniqueIdManager->getId(GamePacker::GamePackerSlave::COMPONENT_ID, (uint16_t) Blaze::GameManager::GAMEPACKER_IDTYPE_FINALIZATION_JOB, finalizationJobId);
    if (errRet != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].handlePackerReapGame: Failed to obtain fin job id when reaping game(" << game.mGameId << "), error: " << errRet);
        return;
    }
    EA_ASSERT(finalizationJobId != 0);

    PackerSessionByScenarioIdMap packerSessionByScenarioIdMap;
    packerSessionByScenarioIdMap.reserve(pendingSessions.size());

    auto now = TimeValue::getTimeOfDay();
    bool atLeastOneSessionExpired = false;

    for (auto& session : pendingSessions)
    {
        auto& packerSession = *session;
        packerSession.cancelMigrateTimer();
        packerSession.cancelUpdateTimer();
        // PACKER_MAYBE: We don't want to do this here(move it to complete job), because we are not sure that we will actually enter finalization, that should only happen after we've successfully locked all the sessions otherwise its just spam.
        auto packerSessionId = packerSession.mPackerSessionId;
        sendSessionStatus(packerSessionId);
    }

    eastl::hash_set<PackerSiloId> repackSiloIdSet; // NOTE: All packer silos affected by the removal should be repacked at the end to avoid churn whereby sessions are repeatedly removed/repacked
    // NOTE: these sessions will be automatically yanked from the game.mPacker, which means that they do not exist in this particular packer silo, they are still associated with the production node (packer silo id) in RETE however, also they may still exist in any other packer silos as well, therefore we need to ensure to remove them from there also, while we are waiting for the master to create the game for us.
    // 1) yank them out of rete as well, so that new sessions don't end up wasting time matching these sessions.
    for (auto& session : pendingSessions)
    {
        auto& packerSession = *session;
        if (packerSession.mSessionExpirationTime <= now)
            atLeastOneSessionExpired = true;

        packerSession.mState = PackerSession::AWAITING_JOB_START;
        packerSession.mFinalizationJobId = finalizationJobId; // link to job
        ++packerSession.mFinalizationAttempts;

        for (auto otherPackerSiloId : packerSession.mMatchedPackerSiloIdSet)
        {
            if (otherPackerSiloId == packerSiloId)
            {
                // we are inside reap, for this silo, it will remove the parties itself.
                continue;
            }

            // remove from any other packer silos
            auto* otherPackerSilo = getPackerSilo(otherPackerSiloId);
            EA_ASSERT(otherPackerSilo != nullptr);

            // NOTE: We remove the party from *all* silos, the games it was a participant in need to be repacked in order for the remaining parties to be reassigned to new game(s)
            removeSessionFromPackerSilo(packerSession, *otherPackerSilo, "reap_viable_sibling");

            repackSiloIdSet.insert(otherPackerSiloId);
        }

        auto* hostJoinInfo = packerSession.getHostJoinInfo();
        auto scenarioId = hostJoinInfo->getOriginatingScenarioId();
        auto ret = packerSessionByScenarioIdMap.insert(scenarioId);
        EA_ASSERT_MSG(ret.second, "Sibling sub-sessions must never be packed into a single game!");
        ret.first->second = &packerSession;
    }

    for (auto repackPackerSiloId : repackSiloIdSet)
    {
        // PACKER_TODO: This pack/schedulereap invocation is making the flow more complex than I would like because it is currently synchronously triggered from within handlePackerReapGame() which is itself a callback already. It is also less efficient than we would want. What we would ideally want to do is queue silo packing requests to be processed at the end of this phase to avoid spending CPU time immediately before kicking off the job below (which will block several times and thus should be less impacted by the CPU overhead from repacking other silos inline.
        packGames(repackPackerSiloId);
    }

    auto ret = mFinalizationJobs.insert(finalizationJobId);
    EA_ASSERT(ret.second);
    auto& submitJob = ret.first->second;
    submitJob.mJobId = finalizationJobId;
    submitJob.mInitiatorPackerSessionId = ownerSession->mPackerSessionId;
    submitJob.mInitiatorScenarioId = ownerSession->getOriginatingScenarioId();
    submitJob.mGmMasterRequest = createGameRequest;
    submitJob.mPackerSessionByScenarioIdMap = eastl::move(packerSessionByScenarioIdMap);
    submitJob.mGameFactorScores = game.mFactorScores;
    const float wgeomean = calculateWeightedGeometricMean(game.mFactorScores.size(), game.mFactorScores.begin()); // PACKER_TODO: already calculated, no need to do it again?
    submitJob.mGameScore = (uint32_t) roundf(wgeomean * 100.0f); // always 0..100%
    submitJob.mGameId = game.mGameId;
    submitJob.mGameRecordVersion = gameRecordVersion;
    submitJob.mGameImproveCount = game.mImprovedCount;
    submitJob.mGamePackerSiloId = packerSiloId;
    submitJob.mGameCreatedTimestamp = game.mCreatedTimestamp;
    submitJob.mGameImprovedTimestamp = game.mImprovedTimestamp;
    submitJob.mJobCreatedTimestamp = TimeValue::getTimeOfDay();
    submitJob.mPingsiteAliasList.swap(pingSiteAliases); // take ownership without allocating

    if (packer.isIdealGame(game))
        submitJob.mFinalizationTrigger = GameManager::PackerFinalizationTrigger::OPTIMAL;
    else if (atLeastOneSessionExpired)
        submitJob.mFinalizationTrigger = GameManager::PackerFinalizationTrigger::DEADLINE;
    else
        submitJob.mFinalizationTrigger = GameManager::PackerFinalizationTrigger::COOLDOWN;

    mGamesPacked.increment(1, gameTemplateName, isProvisionalGameId(submitJob.mGameId) ? "yes" : "no", getBucketFromScore(wgeomean), submitJob.mFinalizationTrigger);

    mFinalizationJobQueue.queueFiberJob(this, &GamePackerSlaveImpl::finalizePackedGame, finalizationJobId, "GamePackerSlaveImpl::finalizePackedGame");

    gameCompletedEvent.setFinJobId(finalizationJobId);

    TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapGame: " << gameCompletedEvent);
}

void GamePackerSlaveImpl::handlePackerReapPartyAux(const Packer::Game& game, const Packer::Party& party, bool expired)
{
    auto& packer = game.mPacker;

    TRACE_LOG("[GamePackerSlaveImpl].handlePackerReapParty: PackerSilo(" << packer.getPackerSiloId() << "), game(" << game.mGameId << "), partyExpired(" << expired << ")");

    auto packerDetailedMetricsItr = mPackerDetailedMetricsByGameTemplateMap.find(packer.getPackerName()); // PACKER_TODO(metrics): #BUG check iterator!
    if (packerDetailedMetricsItr == mPackerDetailedMetricsByGameTemplateMap.end())
    {
        // PACKER_TODO(metrics): log error
        EA_FAIL();
        return;
    }
    auto& detailedMetrics = packerDetailedMetricsItr->second;
    TimeValue timeInViableGames = party.mTotalTimeInViableGames;
    auto timeInViableGamesSec = timeInViableGames.getSec();
    auto timeInPacker = TimeValue::getTimeOfDay() - party.mCreatedTimestamp;
    auto timeInPackerSec = timeInPacker.getSec();
    detailedMetrics.mTimeSpentInViableGamesByParties[timeInViableGamesSec]++;
    detailedMetrics.mTimeSpentInPackerByParty[timeInPackerSec]++;
    detailedMetrics.mEvictionCountByParties[party.mEvictionCount]++;
}

void GamePackerSlaveImpl::handlePackerReapParty(const Packer::Game& game, const Packer::Party& party, bool expired)
{
    handlePackerReapPartyAux(game, party, expired);
    if (expired)
    {
        INFO_LOG("[GamePackerSlaveImpl].handlePackerReapParty: PackerSilo(" << game.mPacker.getPackerSiloId() << "), game(" << game.mGameId << "), party(" << party.mPartyId << "), party expired."); // PACKER_TODO: Add verbosity, more useful info such as number of times evicted, last improved, num times finalization attempted, time acquired, etc.

        EA_ASSERT(!game.isViable());

        auto packerSessionItr = mPackerSessions.find(party.mPartyId);
        EA_ASSERT_MSG(packerSessionItr != mPackerSessions.end(), "PackerSession owns the party, must exist!");
        auto& packerSession = *packerSessionItr->second;
        // expired reaped sessions need to have their update timers canceled immediately since their Party object is getting removed from the silo, and all game quality factor score info (needed by the send update code) will not be available after that.
        packerSession.cancelUpdateTimer();
        packerSession.cancelMigrateTimer();
        if (packerSession.mDelayCleanupTimestamp == 0) // NOTE: Only add to delayed cleanup queue once (same partyId is used within all silos that need to clean up this party)
        {
            packerSession.mDelayCleanupTimestamp = TimeValue::getTimeOfDay();
            packerSession.mState = PackerSession::AWAITING_RELEASE;
            packerSession.setCleanupReason("expired");
            mDelayCleanupSessions.push_back(party.mPartyId);
        }
    }
}

/*
    General error handling flow of execution
    --lock participating sessions stage--
    if failed to lock with unexpected error (ERR_SYSTEM, ERR_TIMEOUT, ERR_DISCONNECT)
        complete all locked sessions as failed (auto-unlock)
    else if failed to lock with expected error (SESSION_NOT_FOUND, ALREADY_LOCKED) or
        unlock all locked sessions, and reinsert back into packer (with exponential backoff)

    --create game/reset dedicated server stage--
    if failed with error
        complete all locked sessions as failed (auto-unlock)
    else if (one of the sessions changed/logged out)
        cleanup game
        unlock all locked sessions and reinsert back into packer

    --join external game session stage--
    if failed with error
        cleanup game
        trigger remote external game sync on gm master
        complete all locked sessions as failed (auto-unlock)
    else if (one of the sessions changed/logged out)
        cleanup game
        unlock all locked sessions, and reinsert back into packer
 */
void GamePackerSlaveImpl::finalizePackedGame(PackerFinalizationJobId jobId)
{
    auto jobItr = mFinalizationJobs.find(jobId);
    if (jobItr == mFinalizationJobs.end())
    {
        WARN_LOG("[GamePackerSlaveImpl].finalizePackedGame: job(" << jobId << ") not found");
        return;
    }

    // PACKER_TODO: find the ownerSession for LogContextOverride (need to determine the owner of the job)
    auto& job = jobItr->second;
    job.mFinStageTimes[JOB_QUEUED] = TimeValue::getTimeOfDay() - job.mJobCreatedTimestamp;

    EA_ASSERT(!job.mPackerSessionByScenarioIdMap.empty());
    if (!isJobValid(job, PackerSession::AWAITING_JOB_START))
    {
        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated while in the queue, remaining valid sessions will be resubmitted to packer");
        completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
        return;
    }

    auto* gmMaster = getGmMaster();
    if (gmMaster == nullptr)
    {
        ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to resolve game manager master, aborting job");
        completeJob(ERR_SYSTEM, jobId);
        return;
    }

    TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " started");

    BlazeError overallResult = ERR_SYSTEM;

    auto privilegedGameIdPreference = GameManager::INVALID_GAME_ID;
    TimeValue before;
    // VERY IMPORTANT: Sibling subsessions originating from a single scenario are likely to migrate to multiple
    // different packer slaves and may end up competing to lock their respective scenarios when finalizing. 
    // In order to avoid deadlocks the locks need to be taken in globally consistent order by ScenarioId. 
    // This is what we do here.
    for (auto& packerSessByScenarioIdItr : job.mPackerSessionByScenarioIdMap)
    {
        auto scenarioId = packerSessByScenarioIdItr.first;
        auto& jobPackerSession = *packerSessByScenarioIdItr.second;
        auto packerSessionId = jobPackerSession.mPackerSessionId;
        // Don't bother validating packer sessions here, because it can become invalid anytime and we still have to handle it the same way.
        Blaze::GameManager::WorkerLockPackerSessionRequest req;
        req.setPackerSessionId(packerSessionId);
        jobPackerSession.mState = PackerSession::AWAITING_LOCK;
        before = TimeValue::getTimeOfDay();
        overallResult = getMaster()->workerLockPackerSession(req);
        job.mFinStageTimes[JOB_LOCK_SCENARIOS] += TimeValue::getTimeOfDay() - before;
        if (overallResult == ERR_OK)
        {
            TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " locked session(" << packerSessionId << "), scenario(" << scenarioId << ") for finalization");
            jobPackerSession.mState = PackerSession::LOCKED;
        }
        else
        {
            // NOTE: These are the cases that can cause the lock to fail:
            // 1) Lock is already held by a different worker
            // 2) The session has been canceled (via member logout, or direct cancelation)
            // 3) RPC fails (ERR_TIMEOUT, ERR_DISCONNECT, ERR_SYSTEM, ERR_CANCELED)

            // QUESTION: What is it best to handle ERR_CANCELED? in theory that means that this fiber is preventing 
            // the server from completing a disorderly shutdown; therefore, re-queueing work is just a waste of time
            // and we should just exit as fast as possible, after possibly logging some stuff.

            // lock could not be taken, ERR log in case of major error, trace in case of expected lock collision.
            if (overallResult == GAMEPACKER_ERR_SCENARIO_ALREADY_FINALIZING)
            {
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to lock session(" << packerSessionId << ") in scenario(" << scenarioId << ") while finalizing due to lock collision, session will be suspended by owning master, remaining sessions(" << job.mPackerSessionByScenarioIdMap.size()-1 << ") will be unlocked if needed and resubmitted into packer");
                // lock collisions are handled by suspending the colliding session on the master and requeuing the remaining non-orphaned sessions on the worker
            }
            else if (overallResult == GAMEPACKER_ERR_SESSION_NOT_FOUND)
            {
                if (jobPackerSession.mOrphaned)
                {
                    TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to lock session(" << packerSessionId << ") in scenario(" << scenarioId << ") while finalizing because owner orphaned the session, remaining sessions(" << job.mPackerSessionByScenarioIdMap.size() - 1 << ") will be unlocked (if needed) and resubmitted to packer");
                }
                else
                {
                    // PACKER_TODO: Investigate why this happens!
                    //EA_FAIL_MSG("Master should always send the removal notification first!");
                    WARN_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to lock session(" << packerSessionId << ") in scenario(" << scenarioId << ") while finalizing because owner did not find the session, remaining sessions(" << job.mPackerSessionByScenarioIdMap.size() - 1 << ") will be unlocked (if needed) and resubmitted to packer");
                }
                overallResult = GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED;
            }
            else
            {
                ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to lock session(" << packerSessionId << ") in scenario(" << scenarioId << ") for finalization due to unexpected err: " << overallResult << ", remaining sessions will be unlocked and resubmitted to packer");
            }

            completeJob(overallResult, jobId);
            return;
        }

        // remember any gameId override
        auto& callerPropertyMap = jobPackerSession.mMmInternalRequest->getMatchmakingPropertyMap();
        auto callerPropItr = callerPropertyMap.find(GameManager::PROPERTY_NAME_DEDICATED_SERVER_OVERRIDE_GAME_ID);
        if (callerPropItr != callerPropertyMap.end())
        {
            auto gameIdOverride = callerPropItr->second->getValue().asUInt64();
            if (privilegedGameIdPreference != gameIdOverride)
            {
                // any gameId overrides found here must *all* be the same (or INVALID_GAME_ID)
                if (gameIdOverride != GameManager::INVALID_GAME_ID)
                {
                    EA_ASSERT(privilegedGameIdPreference == GameManager::INVALID_GAME_ID);
                    privilegedGameIdPreference = gameIdOverride;
                }
            }
        }
    }

    if (!isJobValid(job))
    {
        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated while was locking the batch, remaining sessions will be resubmitted into packer");
        completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
        return;
    }

    auto initiatorPackerSessionItr = mPackerSessions.find(job.mInitiatorPackerSessionId);
    if (initiatorPackerSessionItr == mPackerSessions.end())
    {
        ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " logged out bypassing handling");
        completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
        return;
    }

    auto& initiatorPackerSession = *initiatorPackerSessionItr->second;
    eastl::string gameTemplateName = initiatorPackerSession.mGameTemplate.mGameTemplateName;
    auto pauseDuration = initiatorPackerSession.mGameTemplate.mCreateGameTemplateConfig->getPackerConfig().getPauseDurationAfterAcquireFinalizationLocks();
    // NOTE: Pausing finalization should only be used as an aid during testing finalization, never in prod :)
    if (pauseDuration > 0)
    {
        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " paused for(" << pauseDuration.getMillis() << "ms) after locking the batch");
        BlazeError rc = Fiber::sleep(pauseDuration, "GamePackerSlaveImpl.finalizePackedGame_Paused");
        if (rc == ERR_OK)
        {
            if (isJobValid(job))
            {
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " resumed following intentional pause after locking the batch");
            }
            else
            {
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated while intentionally paused after locking the batch, remaining sessions will be resubmitted into packer");
                completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
                return;
            }
        }
        else
        {
            ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " pause failed while while pausing the batch, error(" << rc << ")");
            completeJob(rc, jobId);
            return;
        }
    }

    auto* hostJoinInfo = initiatorPackerSession.getHostJoinInfo(); // PACKER_TODO: legacy mmsession uses 'host' and 'primary user' and 'initiator' interchangeably, we need to decide on a convention and finally stick with it!!!
    if (hostJoinInfo == nullptr)
    {
        EA_FAIL(); // PACKER_TODO: need to figure out whether its possible for a 'host' a.k.a 'primary' user session to not exist at all.
        // Currently this is basically a fatal error, but we really need to validate this stuff on the packer_master, when we first accept the request!
        completeJob(ERR_SYSTEM, jobId);
        return;
    }
    const bool isProvisionalGame = isProvisionalGameId(job.mGameId);
    job.mGmMasterRequest->getCreateReq().setFinalizingMatchmakingSessionId(job.mInitiatorPackerSessionId); // PACKER_TODO: this should be set already?
    job.mGmMasterRequest->setTrackingTag(initiatorPackerSession.mMmInternalRequest->getTrackingTag()); // PACKER_TODO: this should be set already?
    static const auto MAX_GAME_QUALITY_SCORE = 100; // always 100%
    {
        auto finalizationInitiatorBlazeId = hostJoinInfo->getUser().getUserInfo().getId();
        auto& setupReasonMap = job.mGmMasterRequest->getUserSessionGameSetupReasonMap();
        setupReasonMap.reserve(initiatorPackerSession.mMmInternalRequest->getRequest().getGameCreationData().getMaxPlayerCapacity());
        auto requiredProtocolVersionString = getConfig().getGameSession().getEvaluateGameProtocolVersionString(); 
        for (auto& packerSessByScenarioIdItr : job.mPackerSessionByScenarioIdMap)
        {
            auto& packerSession = *packerSessByScenarioIdItr.second;
            auto packerSessionId = packerSession.mPackerSessionId;
            GameManager::MatchmakingScenarioId scenarioId = packerSession.getOriginatingScenarioId();
            auto& packerSessionMmRequest = *packerSession.mMmInternalRequest;
            auto gameEntryType = packerSessionMmRequest.getRequest().getPlayerJoinData().getGameEntryType();
            auto groupId = packerSessionMmRequest.getRequest().getPlayerJoinData().getGroupId();
            // NOTE: ownerUserSessionId may be a completely separate user session that is not going to join the game itself
            // e.g: S2S proxy tools user acting on behalf of the players.
            auto ownerUserSessionId = packerSessionMmRequest.getOwnerUserSessionInfo().getSessionId();
            auto matchmakingResult = GameManager::SUCCESS_JOINED_NEW_GAME;
            if (!isProvisionalGame)
                matchmakingResult = GameManager::SUCCESS_JOINED_EXISTING_GAME;
            else if (packerSessionId == job.mInitiatorPackerSessionId)
                matchmakingResult = GameManager::SUCCESS_CREATED_GAME;

            for (auto userSessionId : packerSession.mUserSessionIds)
            {
                auto* setupReason = setupReasonMap.allocate_element();
                if (userSessionId == ownerUserSessionId)
                {
                    setupReason->switchActiveMember(GameManager::GameSetupReason::MEMBER_MATCHMAKINGSETUPCONTEXT);
                    auto& setupContext = *setupReason->getMatchmakingSetupContext();
                    setupContext.setSessionId(packerSessionId);
                    setupContext.setScenarioId(scenarioId);
                    setupContext.setUserSessionId(ownerUserSessionId);
                    setupContext.setMatchmakingResult(matchmakingResult);
                    setupContext.setFitScore(job.mGameScore);
                    setupContext.setMaxPossibleFitScore(MAX_GAME_QUALITY_SCORE);
                    setupContext.setGameEntryType(gameEntryType);
                    //setupContext.setTimeToMatch(getMatchmakingRuntime()); // PACKER_TODO: this *could* be how long it took the session to matchmake, we probably would like the time of how long it took the scenario to match given that the session may be delayed after a scenario started...
                    //setupContext.setEstimatedTimeToMatch(getEstimatedTimeToMatch()); // PACKER_TODDO: do we need this?
                    setupContext.setMatchmakingTimeoutDuration(packerSessionMmRequest.getScenarioTimeoutDuration());
                    setupContext.setTotalUsersOnline(packerSessionMmRequest.getTotalUsersOnline());
                    setupContext.setTotalUsersInGame(packerSessionMmRequest.getTotalUsersInGame());
                    setupContext.setTotalUsersInMatchmaking(packerSessionMmRequest.getTotalUsersInMatchmaking());
                    setupContext.setInitiatorId(finalizationInitiatorBlazeId);
                    setupContext.setFinalizationJobId(jobId);
                }
                else
                {
                    setupReason->switchActiveMember(GameManager::GameSetupReason::MEMBER_INDIRECTMATCHMAKINGSETUPCONTEXT);
                    auto& setupContext = *setupReason->getIndirectMatchmakingSetupContext();
                    setupContext.setSessionId(packerSessionId);
                    setupContext.setScenarioId(scenarioId);
                    setupContext.setUserSessionId(ownerUserSessionId);
                    setupContext.setMatchmakingResult(matchmakingResult);
                    setupContext.setUserGroupId(groupId);
                    setupContext.setFitScore(job.mGameScore);
                    setupContext.setMaxPossibleFitScore(MAX_GAME_QUALITY_SCORE);
                    setupContext.setRequiresClientVersionCheck(requiredProtocolVersionString);
                    setupContext.setGameEntryType(gameEntryType);
                    //setupContext.setTimeToMatch(getMatchmakingRuntime()); // PACKER_TODO: this *could* be how long it took the session to matchmake, we probably would like the time of how long it took the scenario to match given that the session may be delayed after a scenario started...
                    //setupContext.setEstimatedTimeToMatch(getEstimatedTimeToMatch()); // PACKER_TODDO: do we need this?
                    setupContext.setMatchmakingTimeoutDuration(packerSessionMmRequest.getScenarioTimeoutDuration());
                    setupContext.setTotalUsersOnline(packerSessionMmRequest.getTotalUsersOnline());
                    setupContext.setTotalUsersInGame(packerSessionMmRequest.getTotalUsersInGame());
                    setupContext.setTotalUsersInMatchmaking(packerSessionMmRequest.getTotalUsersInMatchmaking());
                    setupContext.setInitiatorId(finalizationInitiatorBlazeId);
                    setupContext.setFinalizationJobId(jobId);
                }
                setupReasonMap.insert(eastl::make_pair(userSessionId, setupReason));
            }
        }
    }

    // External Session Data for the users needs to be setup prior to creating the Game.  (So that XBL info will be available in Crossplay titles.)
    BlazeRpcError extSessErr = Blaze::GameManager::initRequestExternalSessionData(job.mGmMasterRequest->getCreateReq().getExternalSessionData(),
        job.mGmMasterRequest->getCreateReq().getCreateRequest().getGameCreationData().getExternalSessionIdentSetup(),
        job.mGmMasterRequest->getCreateReq().getCreateRequest().getCommonGameData().getGameType(),
        job.mGmMasterRequest->getCreateReq().getUsersInfo(),
        getConfig().getGameSession(), *mExternalGameSessionServices);
    if (extSessErr != ERR_OK)
    {
        completeJob(extSessErr, jobId);
        return;
    }

    if (isProvisionalGame)
    {
        const auto networkTopology = job.mGmMasterRequest->getCreateReq().getCreateRequest().getGameCreationData().getNetworkTopology();
        // only do game lookups if we don't have a privileged game id
        if (isDedicatedHostedTopology(networkTopology))
        {
            if (privilegedGameIdPreference != GameManager::INVALID_GAME_ID)
            {
                // If we have a game that we have been told to use for debugging purposes, find it and tell
                // master to reset that one (if it can)
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " -> create game with privilegedId(" << privilegedGameIdPreference << ")");
                job.mGmMasterRequest->setPrivilegedGameIdPreference(privilegedGameIdPreference);
                // tell gamemanager to not reset anything if the target game is unavailable
                job.mGmMasterRequest->setRequirePrivilegedGameId(true);
                before = TimeValue::getTimeOfDay();
                // let's try to create the game on the server that owns our target game, this is sharded by privilegedGameIdPreference
                overallResult = gmMaster->matchmakingCreateGameWithPrivilegedId(*job.mGmMasterRequest, job.mGmMasterResponse);
                job.mFinStageTimes[JOB_CREATE_GAME] += TimeValue::getTimeOfDay() - before;
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " <- create game with privelegedId(" << privilegedGameIdPreference << "), result(" << overallResult << ")");
            }
            else
            {
                auto* createGameTemplateName = job.mGmMasterRequest->getCreateReq().getCreateGameTemplateName();
                GameManager::GameIdsByFitScoreMap gamesToSend;
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " -> find dedicated server");
                before = TimeValue::getTimeOfDay();
                overallResult = findDedicatedServerHelper(
                    nullptr,
                    gamesToSend,
                    true,
                    &job.mPingsiteAliasList,
                    // &mCreateGameFinalizationSettings->mOrderedPreferredPingSites,
                    job.mGmMasterRequest->getCreateReq().getCreateRequest().getCommonGameData().getGameProtocolVersionString(),
                    job.mGmMasterRequest->getCreateReq().getCreateRequest().getGameCreationData().getMaxPlayerCapacity(),
                    networkTopology,
                    createGameTemplateName,
                    &initiatorPackerSession.mMmInternalRequest->getFindDedicatedServerRulesMap(),
                    &hostJoinInfo->getUser(),
                    job.mGmMasterRequest->getCreateReq().getCreateRequest().getClientPlatformListOverride()
                );
                job.mFinStageTimes[JOB_FIND_DS] += TimeValue::getTimeOfDay() - before;
                if (overallResult == ERR_OK)
                {
                    overallResult = GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND; // will be reset on success
                    if (!gamesToSend.empty())
                    {
                        // walk games to send and attempt to create/reset each one

                        bool done = false;
                        // NOTE: fit score map is ordered by lowest score; therefore, iterate in reverse
                        for (auto gameIdsMapItr = gamesToSend.rbegin(), gameIdsMapEnd = gamesToSend.rend(); gameIdsMapItr != gameIdsMapEnd; ++gameIdsMapItr)
                        {
                            GameManager::GameIdList* gameIds = gameIdsMapItr->second;
                            // random walk gameIds
                            while (!gameIds->empty())
                            {
                                if (!isJobValid(job))
                                {
                                    TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " reset dedicated server could not proceed because job was invalidated, remaining valid sessions will be resubmitted to packer");
                                    completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
                                    return;
                                }

                                auto gameItr = gameIds->begin() + Random::getRandomNumber((int32_t) gameIds->size());
                                GameManager::GameId gameId = *gameItr;
                                gameIds->erase_unsorted(gameItr);

                                job.mGmMasterRequest->getCreateReq().setGameIdToReset(gameId);
                                RpcCallOptions opts;
                                opts.routeTo.setSliverIdentityFromKey(GameManager::GameManagerMaster::COMPONENT_ID, gameId);
                                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " will reset dedicated server game(" << gameId << ")");
                                before = TimeValue::getTimeOfDay();
                                BlazeError err = gmMaster->matchmakingCreateGame(*job.mGmMasterRequest, job.mGmMasterResponse, opts);
                                job.mFinStageTimes[JOB_CREATE_GAME] += TimeValue::getTimeOfDay() - before;
                                if (err == ERR_OK)
                                {
                                    overallResult = ERR_OK;
                                    done = true;
                                    break;
                                }
                                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to reset dedicated server when, reason: " << err);
                            }

                            if (done)
                                break;
                        }
                    }
                }
                else
                {
                    WARN_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " -> find dedicated server, result(" << overallResult << ")");
                }
            }
        }
        else
        {
            // create game on any master
            TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " -> create game");
            RpcCallOptions opts;
            opts.routeTo.setSliverNamespace(GameManager::GameManagerMaster::COMPONENT_ID); // IMPORTANT! This is currently required for RPCs that require sliver access, but are not targeted to a particular sliver. Without it, RPC may be sent even when the master has not yet gone in-service and thus has not yet obtained (any!) slivers, which would cause operations like: mGameTable.getSliverOwner().getLeastLoadedOwnedSliver() to return nullptr, which would cause RPC to fail unexpectedly.
            before = TimeValue::getTimeOfDay();
            overallResult = gmMaster->matchmakingCreateGame(*job.mGmMasterRequest, job.mGmMasterResponse, opts);
            job.mFinStageTimes[JOB_CREATE_GAME] += TimeValue::getTimeOfDay() - before;
            if (overallResult == ERR_OK)
            {
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " <- create game, result(" << overallResult << ")");
            }
            else
            {
                ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " <- create game, result(" << overallResult << ")");
            }
        }

        if (overallResult != ERR_OK)
        {
            // If we failed to reset a dedicated server. Disqualify the finalizing session and all pulled in sessions that support FG mode from further CG evaluation.

            //bool failedToResetDedicatedServer = ((createErrCode == Blaze::GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND)
            //    || (createErrCode == Blaze::GAMEBROWSER_ERR_INVALID_CRITERIA)
            //    || (createErrCode == Blaze::GAMEBROWSER_ERR_SEARCH_ERR_OVERLOADED)
            //    || (createErrCode == Blaze::GAMEBROWSER_ERR_EXCEEDED_MAX_REQUESTS));

            // If fatal error, we must abort all sessions as failed creation.
            completeJob(overallResult, jobId);
            return;
        }
    }
    else
    {
        // found game, attempt to join it
        auto ownerUserSessionId = initiatorPackerSession.getHostJoinInfo()->getUser().getSessionId(); // PACKER_TODO: is this right? seems like matchmaker got it from foundGames.getOwnerUserSessionId()...
        //auto packerSessionId = initiatorPackerSession.mPackerSessionId;
        auto& commonMmInternalRequest = *initiatorPackerSession.mMmInternalRequest; // used for data common to all the sessions

        GameManager::PackerFoundGameRequest foundGameRequest;
        foundGameRequest.setFinalizationJobId(jobId);
        foundGameRequest.setOwnerUserSessionId(ownerUserSessionId); // This is really the initiator user session, it is the session that can be audited.
        GameManager::JoinGameByGroupMasterRequest& joinByGroupRequest = foundGameRequest.getJoinGameByGroupRequest();
        // IMPORTANT: pull all the grouped data in one shot.
        joinByGroupRequest.setUsersInfo(job.mGmMasterRequest->getCreateReq().getUsersInfo());
        // fill out join sub-request
        joinByGroupRequest.getJoinRequest().setJoinMethod(GameManager::JOIN_BY_MATCHMAKING);
        // IMPORTANT: pull all the grouped data in one shot.
        job.mGmMasterRequest->getUserSessionGameSetupReasonMap().copyInto(foundGameRequest.getUserSessionGameSetupReasonMap());
        job.mGmMasterRequest->getCreateReq().getCreateRequest().getPlayerJoinData().copyInto(joinByGroupRequest.getJoinRequest().getPlayerJoinData());
        commonMmInternalRequest.getRequest().getCommonGameData().copyInto(joinByGroupRequest.getJoinRequest().getCommonGameData());
        commonMmInternalRequest.getGameExternalSessionData().copyInto(joinByGroupRequest.getExternalSessionData());
        foundGameRequest.setGameId(job.mGameId);
        foundGameRequest.setGameRecordVersion(job.mGameRecordVersion);

        GameManager::PackerFoundGameResponse foundGameResponse;

        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " -> found game");

        before = TimeValue::getTimeOfDay();

        overallResult = gmMaster->packerFoundGame(foundGameRequest, foundGameResponse);

        job.mFinStageTimes[JOB_FIND_GAME] += TimeValue::getTimeOfDay() - before;

        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " <- found game, result(" << overallResult << ")");

        if (overallResult != ERR_OK)
        {
            // PACKER_TODO: Here are the most frequently returned errors from matchmakingFoundGame on various Battelfield Titles, these are likely the top initial candidates for handling as 'expected' errors (below). Some may be fatal, others are definitely resumable.
            // Search "matchmakingFoundGame" (6 hits in 1 file)
            //  gamemanager_master    matchmakingFoundGame    15,379,542    3,845,028    25.00%    GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL
            //  gamemanager_master    matchmakingFoundGame    15,379,542    205,188    1.33%    GAMEMANAGER_ERR_GAME_BUSY
            //  gamemanager_master    matchmakingFoundGame    15,379,542    1,144    .01%    GAMEMANAGER_ERR_RESERVATION_ALREADY_EXISTS
            //  gamemanager_master    matchmakingFoundGame    15,379,542    828    .01%    GAMEMANAGER_ERR_TEAM_FULL
            //  gamemanager_master    matchmakingFoundGame    15,379,542    592    .00%    GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION
            //  gamemanager_master    matchmakingFoundGame    15,379,542    65    .00%    GAMEMANAGER_ERR_ALREADY_IN_QUEUE
            // Search "matchmakingFoundGame" (6 hits in 1 file)
            //  gamemanager_master    matchmakingFoundGame    537,092    3,257    .61%    GAMEMANAGER_ERR_GAME_BUSY
            //  gamemanager_master    matchmakingFoundGame    537,092    962    .18%    GAMEMANAGER_ERR_TEAM_FULL
            //  gamemanager_master    matchmakingFoundGame    537,092    608    .11%    GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL
            //  gamemanager_master    matchmakingFoundGame    537,092    487    .09%    GAMEMANAGER_ERR_RESERVATION_ALREADY_EXISTS
            //  gamemanager_master    matchmakingFoundGame    537,092    238    .04%    GAMEMANAGER_ERR_JOIN_METHOD_NOT_SUPPORTED
            //  gamemanager_master    matchmakingFoundGame    537,092    5    .00%    GAMEMANAGER_ERR_GAME_IN_PROGRESS
            // Search "matchmakingFoundGame" (11 hits in 1 file)
            //  gamemanager_master    matchmakingFoundGame    755,605    3,284    .43%    GAMEMANAGER_ERR_GAME_BUSY
            //  gamemanager_master    matchmakingFoundGame    755,605    1,864    .25%    GAMEMANAGER_ERR_TEAM_FULL
            //  gamemanager_master    matchmakingFoundGame    755,605    1,691    .22%    GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL
            //  gamemanager_master    matchmakingFoundGame    755,605    445    .06%    GAMEMANAGER_ERR_JOIN_METHOD_NOT_SUPPORTED
            //  gamemanager_master    matchmakingFoundGame    755,605    375    .05%    GAMEMANAGER_ERR_RESERVATION_ALREADY_EXISTS
            //  gamemanager_master    matchmakingFoundGame    755,605    11    .00%    GAMEMANAGER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND
            //  gamemanager_master    matchmakingFoundGame    755,605    4    .00%    GAMEMANAGER_ERR_UNRESPONSIVE_GAME_STATE
            //  gamemanager_master    matchmakingFoundGame    755,605    1    .00%    GAMEMANAGER_ERR_GAME_IN_PROGRESS
            //  gamemanager_master    matchmakingFoundGame    755,605    1    .00%    GAMEMANAGER_ERR_INVALID_GAME_ID
            //  gamemanager_master    matchmakingFoundGame    755,605    1    .00%    GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION
            //  gamemanager_master    matchmakingFoundGame    755,605    1    .00%    GAMEMANAGER_ERR_QUEUE_FULL
            switch (overallResult)
            {
            case GAMEMANAGER_ERR_GAME_BUSY:
            case GAMEMANAGER_ERR_TEAM_FULL:
            case GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL:
            case GAMEMANAGER_ERR_GAME_IN_PROGRESS:
            case GAMEMANAGER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND:
            case GAMEMANAGER_ERR_INVALID_GAME_ID:
            case GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION:
            case GAMEMANAGER_ERR_UNRESPONSIVE_GAME_STATE:
            case GAMEMANAGER_ERR_JOIN_METHOD_NOT_SUPPORTED:
            case GAMEMANAGER_ERR_QUEUE_FULL:
                // PACKER_TODO: 
                // a) validate that this covers all expected errors, and test to make sure that they all get handled correctly(Arson)
                // b) If the master would return an error response including the game version it has, the packer slave would be able to treat this case as a special indicator that the entry failed due to replication lag(due to game being updated on the master but replication delay not making it yet to the packer slave). This is a useful metric to know  because it gives us a good proxy for how much contention there is between various packer instances (and game browser joins) as related to the rate of success of find game matchmaking.
                // c) add aggregate metrics to track resumable errors
                // d) add counter that tracks resumable errors per packer session, for historical debugging purposes so that we can audit when a specific session had issues with this
                TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " unable to join game(" << job.mGameId << "), resumable err(" << overallResult << ") aborting finalization, remaining active sessions will be repacked");
                completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_JOIN_GAME_FAILED, jobId);
                break;
            default:
                ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " failed to join game(" << job.mGameId << "), non-resumable err(" << overallResult << ") aborting finalization, remaining sessions will be aborted");
                completeJob(overallResult, jobId);
            }
            mPackerFoundGameFailed.increment(1, gameTemplateName.c_str(), overallResult.c_str());
            return;
        }

        if (!isJobValid(job))
        {
            TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated while was joining game(" << job.mGameId  << "), remaining valid sessions will be resubmitted into packer");
            completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
            return;
        }

        EA_ASSERT(job.mGameId == foundGameResponse.getJoinedGameId());

        // job.mGmMasterResponse.getPinSubmission() // PACKER_TODO: need to implement PIN stuff somehow...
        foundGameResponse.getExternalSessionParams().copyInto(job.mGmMasterResponse.getExternalSessionParams());
        foundGameResponse.getPinSubmission().copyInto(job.mGmMasterResponse.getPinSubmission());
        job.mGmMasterResponse.getCreateResponse().setGameId(foundGameResponse.getJoinedGameId());
    }
    
    const auto gameId = job.mGmMasterResponse.getCreateResponse().getGameId();
        
    if (!isJobValid(job))
    {
        
        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated after " << (isProvisionalGameId(job.mGameId) ? "creating" : "joining") << " game(" << gameId << "), corrective action will be taken, remaining sessions will be resubmitted to packer");
        completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
        return;
    }

    // update external session
    before = TimeValue::getTimeOfDay();
    ExternalSessionJoinInitError joinInitErr;
    ExternalSessionApiError apiErr;
    overallResult = mExternalGameSessionServices->join(job.mGmMasterResponse.getExternalSessionParams(), nullptr, true, joinInitErr, apiErr);
    job.mFinStageTimes[JOB_JOIN_EXTERNAL_SESSION] += TimeValue::getTimeOfDay() - before;
    if (overallResult != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " failed to join external session " << Blaze::GameManager::toLogStr(job.mGmMasterResponse.getExternalSessionParams().getSessionIdentification()) << " after successful match into game(" << gameId << "), external service error: " << overallResult);
        completeJob(overallResult, jobId, GameManager::SESSION_ERROR_GAME_SETUP_FAILED, GameManager::PLAYER_JOIN_EXTERNAL_SESSION_FAILED);
        return;
    }

    if (!isJobValid(job))
    {
        TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " was invalidated while was trying to join external game session for game(" << gameId << "). Game will be destroyed, remaining sessions will be resumitted to packer");
        // cleanup players from game, need to also delete the external session!
        completeJob(GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED, jobId);
        return;
    }

    TRACE_LOG("[GamePackerSlaveImpl].finalizePackedGame: " LOG_JOB(job) " successfully created new game("
        << gameId << ")");

    completeJob(overallResult, jobId, isProvisionalGameId(job.mGameId) ? GameManager::SUCCESS_CREATED_GAME : GameManager::SUCCESS_JOINED_EXISTING_GAME);
}

void GamePackerSlaveImpl::completeJob(BlazeError overallResult, PackerFinalizationJobId jobId, GameManager::MatchmakingResult result, GameManager::PlayerRemovedReason reason /*= GameManager::PLAYER_LEFT_CANCELLED_MATCHMAKING*/)
{
    auto jobItr = mFinalizationJobs.find(jobId); // need to fetch jobitr again, since hashtable might resize while we are blocked above
    if (jobItr == mFinalizationJobs.end())
    {
        TRACE_LOG("[GamePackerSlaveImpl].completeJob: job(" << jobId << ") not found");
        return;
    }

    RpcCallOptions ignoreReplyOpts;
    ignoreReplyOpts.ignoreReply = true;

    enum JobResult
    {
        JOB_SUCCESS,
        JOB_RESUMABLE_ERROR,
        JOB_FAILURE
    } jobResult = JOB_FAILURE;

    GameManager::UserJoinInfoPtr hostJoinInfo;
    eastl::string gameTemplateName;
    uint32_t scoreBucket = 0;

    auto& job = jobItr->second;
    auto gameId = job.mGmMasterResponse.getCreateResponse().getGameId();
    if (overallResult == ERR_OK)
    {
        EA_ASSERT_MSG(gameId != GameManager::INVALID_GAME_ID, "Game must be valid!");
        jobResult = JOB_SUCCESS;

        // find the "owner" for later setup of PIN submission
        auto initiatorPackerSessionItr = mPackerSessions.find(job.mInitiatorPackerSessionId);
        if (initiatorPackerSessionItr != mPackerSessions.end())
        {
            auto& initiatorPackerSession = *initiatorPackerSessionItr->second;
            hostJoinInfo = initiatorPackerSession.getHostJoinInfo();
        }
    }
    else
    {
        switch (overallResult)
        {
        case GAMEPACKER_ERR_SCENARIO_ALREADY_FINALIZING:
        case GAMEPACKER_ERR_FINALIZATION_JOB_INVALIDATED:
        case GAMEPACKER_ERR_FINALIZATION_JOB_JOIN_GAME_FAILED:
            jobResult = JOB_RESUMABLE_ERROR;
            break;
        }

        if (gameId != GameManager::INVALID_GAME_ID)
        {
#if 0 // PACKER_TODO: figure out if we need to handle this specially depending on the type of matchmaking (ie: create vs join)
            if (isProvisionalGameId(job.mGameId))
#endif
            {
                auto* gmMaster = getGmMaster();
                EA_ASSERT(gmMaster != nullptr);
                // cleanup game's external session if we had a join failure
                if ((reason == GameManager::PLAYER_JOIN_EXTERNAL_SESSION_FAILED) &&
                    (Blaze::GameManager::shouldResyncExternalSessionMembersOnJoinError(overallResult.code)))
                {
                    GameManager::ResyncExternalSessionMembersRequest resyncRequest;
                    resyncRequest.setGameId(gameId);
                    BlazeError resyncErr = gmMaster->resyncExternalSessionMembers(resyncRequest, ignoreReplyOpts);
                    if (resyncErr == ERR_OK)
                    {
                        TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " resynced external members for game(" << gameId << ")");
                    }
                    else
                    {
                        ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to resync game(" << gameId << ")'s external session members with error: " << resyncErr);
                    }
                }

                auto& externalUserInfos = job.mGmMasterResponse.getExternalSessionParams().getExternalUserJoinInfos();
                TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " cleanup after failing to join external session for game(" << gameId << ")");

                GameManager::LeaveGameByGroupMasterRequest leaveRequest;
                GameManager::LeaveGameByGroupMasterResponse leaveResponse;
                // PACKER_TODO: Get rid of this blocking loop! We should not need this, since the master should be able to remove everyone using the external user infos that it itself provided us with!
                // {{
                auto& sessionIds = leaveRequest.getSessionIdList();
                for (ExternalUserJoinInfoList::const_iterator itr = externalUserInfos.begin(); itr != externalUserInfos.end(); ++itr)
                {
                    UserInfoPtr userInfo;
                    if (GameManager::lookupExternalUserByUserIdentification((*itr)->getUserIdentification(), userInfo) == ERR_OK)
                    {
                        UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(userInfo->getId());
                        if (primarySessionId != UserSession::INVALID_SESSION_ID)
                            sessionIds.push_back(primarySessionId);
                    }
                }
                //}}

                leaveRequest.setGameId(gameId);
                // leave leader is false as all users being joined/removed are in the session id list for this request
                leaveRequest.setLeaveLeader(false); // PACKER_TODO: Having looked at the master code this should probably be true, because the leader is always the one on whose behalf we are kicking the group, but since its not necessarily us who is the player leaving the game... We really need to be careful to ensure that if the current player is already logged out on the master this RPC must still succeed!!!
                leaveRequest.setPlayerRemovedReason(reason);
                UserSession::SuperUserPermissionAutoPtr permissionPtr(true);
                BlazeError leaveErr = gmMaster->leaveGameByGroupMaster(leaveRequest, leaveResponse, ignoreReplyOpts);
                if (leaveErr == ERR_OK)
                {
                    TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " completed leave game(" << gameId << ") by group");
                }
                else
                {
                    ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to leave game(" << gameId << ") by group, error: " << leaveErr);
                }
            }
#if 0 // PACKER_TODO: figure out if we need to handle this specially
            else
            {
                // PACKER_TODO: Need to figure how to correctly handle the case when packerFoundGame fails
                // a) with errors such as ERR_TIMEOUT, or ERR_SYSTEM where packer slave may need to issue an *explicit* removal call to remove the parties from the game
                // b) with explicit non-fatal GM master errors like TEAM_FULL, where we shoundn't need to issue an explicit removal call
                // c) with explicit fatal GM master errors like INVALID_ENTRY_CRITERIA, where we shoundn't need to issue an explicit removal call, but should probably still abort all the sessions and avoid repacking them...
                WARN_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " joined leave game(" << gameId << ")");
            }
#endif
        }
    }

    for (auto& packerSessByScenarioIdItr : job.mPackerSessionByScenarioIdMap)
    {
        GameManager::WorkerSessionCleanupReason cleanupReason = GameManager::WORKER_SESSION_COMPLETED;
        auto scenarioId = packerSessByScenarioIdItr.first;
        auto& jobPackerSession = *packerSessByScenarioIdItr.second;
        auto packerSessionId = jobPackerSession.mPackerSessionId;
        jobPackerSession.mFinalizationJobId = 0; // reset job id, because in all cases session shall no longer be associated with this job
        if (gameTemplateName.empty())
            gameTemplateName = jobPackerSession.mGameTemplate.mGameTemplateName;

        // update finalization time metrics for this session
        for (eastl_size_t i = 0, size = jobPackerSession.mFinStageTimes.size(); i < size; ++i)
        {
            if (job.mFinStageTimes[i] > 0)
                jobPackerSession.mFinStageTimes[i] += job.mFinStageTimes[i];
        }
        switch (jobResult)
        {
            case JOB_SUCCESS:
            {
                // release session that succeeded
                GameManager::WorkerCompletePackerSessionRequest request;
                request.setGameId(gameId);
                request.setPackerSessionId(packerSessionId);
                request.setInitiatorPackerSessionId(job.mInitiatorPackerSessionId);
                auto overallScore = calculateWeightedGeometricMean(job.mGameFactorScores.size(), job.mGameFactorScores.begin(), nullptr);
                scoreBucket = getBucketFromScore(overallScore);
                request.setGameQualityScore(overallScore);
                request.getGameFactorScores().assign(job.mGameFactorScores.begin(), job.mGameFactorScores.end());
                PINSubmission pinSubmission;
                pinSubmission.getEventsMap().reserve(jobPackerSession.mUserSessionIds.size());
                // setup PIN submission for this packer session -- which is a subset of all the users in job.mGmMasterResponse.getPinSubmission()
                for (auto userSessionId : jobPackerSession.mUserSessionIds)
                {
                    auto pinEventsMapItr = job.mGmMasterResponse.getPinSubmission().getEventsMap().find(userSessionId);
                    if (pinEventsMapItr == job.mGmMasterResponse.getPinSubmission().getEventsMap().end())
                    {
                        // job.mGmMasterResponse.getPinSubmission() should contain all the users for every packer session, but ...
                        WARN_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " completed session(" << packerSessionId << ") in scenario(" << scenarioId << ") is missing user session(" << userSessionId << ") for PIN submission");
                        continue;
                    }

                    // it's ok to share the PINEventList data; this "job" will be done after this method
                    pinSubmission.getEventsMap()[userSessionId] = pinEventsMapItr->second;

                    if (hostJoinInfo != nullptr && hostJoinInfo->getUser().getSessionId() == userSessionId)
                    {
                        GameManager::PINEventHelper::addScenarioAttributesToMatchJoinEvent(*(pinEventsMapItr->second), hostJoinInfo->getScenarioInfo().getScenarioAttributes());
                    }
                }

                request.setPinSubmission(pinSubmission);
                BlazeError rc = getMaster()->workerCompletePackerSession(request, ignoreReplyOpts); // successful completion always auto-unlocks
                if (rc == ERR_OK)
                {
                    TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " completed session(" << packerSessionId << ") in scenario(" << scenarioId << ") to owner");
                }
                else
                {
                    ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to complete session(" << packerSessionId << ") in scenario(" << scenarioId << ") to owner, reason: " << rc);
                }
                jobPackerSession.mState = PackerSession::AWAITING_RELEASE;
                break;
            }
            case JOB_RESUMABLE_ERROR:
            {
                if (jobPackerSession.mOrphaned)
                {
                    cleanupReason = GameManager::WORKER_SESSION_TERMINATED;
                    // master already gave up on the session, just clean it up
                    TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " cleanup session(" << packerSessionId << ") in scenario(" << scenarioId << "), orphaned by owner");
                    jobPackerSession.mState = PackerSession::AWAITING_RELEASE;
                }
                else if (jobPackerSession.mState == PackerSession::AWAITING_LOCK)
                {
                    cleanupReason = GameManager::WORKER_SESSION_SUSPENDED;
                    INFO_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " cleanup session(" << packerSessionId << ") in scenario(" << scenarioId << ") due to scenario sibling finalization lock collision, session auto-suspended, owner will resume if necessary"); // PACKER_TODO: should this be trace?
                    jobPackerSession.mState = PackerSession::AWAITING_RELEASE;
                }
                else if (jobPackerSession.mState == PackerSession::AWAITING_RELEASE)
                {
                    cleanupReason = GameManager::WORKER_SESSION_RELINQUISHED;
                    Blaze::GameManager::WorkerRelinquishPackerSessionRequest request;
                    request.setPackerSessionId(packerSessionId);
                    BlazeError rc = getMaster()->workerRelinquishPackerSession(request, ignoreReplyOpts);
                    if (rc == ERR_OK)
                    {
                        TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " relinquished session(" << packerSessionId << ") in scenario(" << scenarioId << ") as requested by owner, owner will reissue session(if necessary)");
                    }
                    else
                    {
                        ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to relinquish session(" << packerSessionId << ") in scenario(" << scenarioId << ") as requested by owner, err: " << rc);
                    }
                    EA_ASSERT(jobPackerSession.mState == PackerSession::AWAITING_RELEASE);
                }
                else
                {
                    if (jobPackerSession.mState == PackerSession::LOCKED)
                    {
                        // This session has been successfully locked and now needs to be unlocked before it can be resubmitted back to the packer
                        Blaze::GameManager::WorkerUnlockPackerSessionRequest req;
                        req.setPackerSessionId(packerSessionId);
                        BlazeError unlockErr = getMaster()->workerUnlockPackerSession(req, ignoreReplyOpts);
                        if (unlockErr == ERR_OK)
                        {
                            TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " unlocked session(" << packerSessionId << ") in scenario(" << scenarioId << ")");
                        }
                        else
                        {
                            ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to unlock session/scenario(" << packerSessionId << ") in scenario(" << scenarioId << "), reason: " << unlockErr);
                        }
                    }
                    StringBuilder sb;
                    for (eastl_size_t i = 0, size = jobPackerSession.mFinStageTimes.size(); i < size; ++i)
                    {
                        auto time = jobPackerSession.mFinStageTimes[i].getMicroSeconds();
                        if (time > 0)
                            sb << TIMED_JOB_STAGE[i] << ":" << time << ", ";
                    }
                    if (!sb.isEmpty())
                        sb.trim(2);
                    // resubmit to packer if the session hasn't expired yet
                    auto now = TimeValue::getTimeOfDay();
                    if (now < jobPackerSession.mSessionExpirationTime)
                    {
                        INFO_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " resumable finalization attempt(" << jobPackerSession.mFinalizationAttempts << ") failed(" << overallResult << ") to enter game(" << job.mGameId << ") resubmit session(" << packerSessionId << "), scenario(" << scenarioId << ") to packer; gameVersion(" << job.mGameRecordVersion << "), gameLifetime(" << (now-job.mGameCreatedTimestamp).getMicroSeconds() << "), gameImproved(" << job.mGameImproveCount << "), gameImprovedTime(" << (job.mGameImprovedTimestamp - now).getMicroSeconds() << "), gameScore(" << job.mGameScore << "), sessionEvictedTimes(" << jobPackerSession.mEvictedCount << "), partyRequeuedTimes(" << jobPackerSession.mRequeuedCount << "), partyUnpackedTimes(" << jobPackerSession.mUnpackedCount << "), newSessionsSinceAddedToGame(" << jobPackerSession.mAddedToJobNewPartyCount-jobPackerSession.mParentMetricsSnapshot.mAddedToGameNewPartyCount << "), sessionTimeInGame(" << (now-jobPackerSession.mAddedToGameTimestamp).getMicroSeconds() << "), sessionTimeInViableGames(" << jobPackerSession.mTotalTimeInViableGames.getMicroSeconds() << "), sessionTTL(" << (jobPackerSession.mSessionExpirationTime - now).getMicroSeconds() << "), sessionFinalizationTime(" << sb <<")");

                        jobPackerSession.mState = PackerSession::PACKING;

                        for (auto packerSiloId : jobPackerSession.mMatchedPackerSiloIdSet)
                        {
                            if (!addSessionToPackerSilo(jobPackerSession.mPackerSessionId, packerSiloId))
                            {
                                EA_FAIL_MSG("Failed to readd packer session on job completion!");
                            }
                            packGames(packerSiloId); // PACKER_TODO: Simplify this to occur when all the jobSessions have been processed, this would also be more fair since we need to account for expiration priority order of sessions
                        }
                    }
                    else
                    {
                        WARN_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " resumable finalization attempt(" << jobPackerSession.mFinalizationAttempts << ") failed(" << overallResult << ") to enter game(" << job.mGameId << ") expired session(" << packerSessionId << "), scenario(" << scenarioId << ") from packer; gameVersion(" << job.mGameRecordVersion << "), gameLifetime(" << (now-job.mGameCreatedTimestamp).getMicroSeconds() << "), gameImproved(" << job.mGameImproveCount << "), gameImprovedTime(" << (job.mGameImprovedTimestamp - now).getMicroSeconds() << "), gameScore(" << job.mGameScore << "), sessionEvictedTimes(" << jobPackerSession.mEvictedCount << "), partyRequeuedTimes(" << jobPackerSession.mRequeuedCount << "), partyUnpackedTimes(" << jobPackerSession.mUnpackedCount << "), newSessionsSinceAddedToGame(" << jobPackerSession.mAddedToJobNewPartyCount-jobPackerSession.mParentMetricsSnapshot.mAddedToGameNewPartyCount << "), sessionTimeInGame(" << (now-jobPackerSession.mAddedToGameTimestamp).getMicroSeconds() << "), sessionTimeInViableGames(" << jobPackerSession.mTotalTimeInViableGames.getMicroSeconds() << "), sessionTTL(" << (jobPackerSession.mSessionExpirationTime - now).getMicroSeconds() << "), sessionFinalizationTime(" << sb <<")");

                        cleanupReason = GameManager::WORKER_SESSION_EXPIRED;
                        jobPackerSession.mState = PackerSession::AWAITING_RELEASE;
                        // expired already, just clean it up, master will take care of it by itself
                    }
                }
                break;
            }
            case JOB_FAILURE:
            {
                if (jobPackerSession.mOrphaned)
                {
                    cleanupReason = GameManager::WORKER_SESSION_TERMINATED;
                    TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed, result(" << overallResult << "), cleanup orphaned session(" << packerSessionId << ") in scenario(" << scenarioId << ")");
                }
                else
                {
                    cleanupReason = GameManager::WORKER_SESSION_FINALIZE_FAILED;
                    WARN_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed, result(" << overallResult << "), abort failed session(" << packerSessionId << ")");
                    GameManager::WorkerAbortPackerSessionRequest request;
                    request.setPackerSessionId(packerSessionId);
                    request.setInitiatorPackerSessionId(job.mInitiatorPackerSessionId);
                    BlazeError rc = getMaster()->workerAbortPackerSession(request, ignoreReplyOpts); // always auto-unlocks
                    if (rc == ERR_OK)
                    {
                        TRACE_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " aborted session(" << packerSessionId << ") in scenario(" << scenarioId << ") to owner");
                    }
                    else
                    {
                        ERR_LOG("[GamePackerSlaveImpl].completeJob: " LOG_JOB(job) " failed to abort session/scenario(" << packerSessionId << ") in scenario(" << scenarioId << ") to owner, reason: " << rc);
                    }
                }

                jobPackerSession.mState = PackerSession::AWAITING_RELEASE;
                break;
            }
        }
        if (jobPackerSession.mState == PackerSession::AWAITING_RELEASE)
            cleanupSession(packerSessionId, cleanupReason);
    }

    mGamesFinalized.increment(1, gameTemplateName.c_str(), isProvisionalGameId(job.mGameId) ? "yes" : "no", scoreBucket, job.mFinalizationTrigger, jobResult == JobResult::JOB_SUCCESS ? "success" : "error");

    mFinalizationJobs.erase(jobId);
}

Packer::Time GamePackerSlaveImpl::utilPackerMeasureTime()
{
    return TimeValue::getTimeOfDay().getMicroSeconds();
}

Packer::Time GamePackerSlaveImpl::utilPackerRetirementCooldown(Packer::PackerSilo& packer)
{
    const auto gameTemplateItr = mGameTemplateMap.find(packer.getPackerName());
    EA_ASSERT(gameTemplateItr != mGameTemplateMap.end());
    return gameTemplateItr->second->mCreateGameTemplateConfig->getPackerConfig().getIdealGameCooldownThreshold().getMicroSeconds();
}

Packer::GameId GamePackerSlaveImpl::utilPackerMakeGameId(Packer::PackerSilo& packer)
{
    return generateUniqueProvisionalGameId();
}

void GamePackerSlaveImpl::utilPackerUpdateMetric(Packer::PackerSilo& packer, Packer::PackerMetricType type, int64_t delta)
{
    switch (type)
    {
    case Packer::PackerMetricType::GAMES:
        if (delta > 0)
            mGamesCreatedCounter.increment(delta, packer.getPackerName());
        mPackerGamesGauge.increment(delta, packer.getPackerName());
        break;
    case Packer::PackerMetricType::VIABLE_GAMES:
        mPackerViableGamesGauge.increment(delta, packer.getPackerName());
        break;
    case Packer::PackerMetricType::PARTIES:
        mPackerPartiesGauge.increment(delta, packer.getPackerName());
        break;
    case Packer::PackerMetricType::PLAYERS:
        mPackerPlayersGauge.increment(delta, packer.getPackerName());
        break;
    default:
        EA_FAIL_MSG("Unhandled type!");
    }
}

void GamePackerSlaveImpl::utilPackerLog(Packer::PackerSilo& packer, Packer::LogLevel level, const char8_t* fmt, va_list args)
{
    switch (level)
    {
    case Packer::LogLevel::TRACE:
    {
        StringBuilder sb;
        sb.appendV(fmt, args, 0);
        TRACE_LOG("[GamePackerSlaveImpl].utilPackerLog: PackerSilo(" << packer.getPackerSiloId() << "): " << sb);
        break;
    }
    case Packer::LogLevel::ERR:
    {
        StringBuilder sb;
        sb.appendV(fmt, args, 0);
        ERR_LOG("[GamePackerSlaveImpl].utilPackerLog: PackerSilo(" << packer.getPackerSiloId() << "): " << sb);
        break;
    }
    default:
        break;
    }
}

bool GamePackerSlaveImpl::utilPackerTraceEnabled(Packer::TraceMode) const
{
    return IS_LOGGING_ENABLED(Logging::TRACE);
}

Packer::PackerSilo* GamePackerSlaveImpl::getPackerSilo(PackerSiloId packerSiloId)
{
    Packer::PackerSilo* packerSilo = nullptr;
    auto packerMapItr = mPackerSiloInfoMap.find(packerSiloId);
    if (packerMapItr != mPackerSiloInfoMap.end())
    {
         packerSilo = &packerMapItr->second.mPackerSilo;
    }
    return packerSilo;
}

GamePackerSlaveImpl::PackerSiloInfo* GamePackerSlaveImpl::getPackerSiloInfo(PackerSiloId packerSiloId)
{
    PackerSiloInfo* packerSiloInfo = nullptr;
    auto packerMapItr = mPackerSiloInfoMap.find(packerSiloId);
    if (packerMapItr != mPackerSiloInfoMap.end())
    {
        packerSiloInfo = &packerMapItr->second;
    }
    return packerSiloInfo;
}

bool GamePackerSlaveImpl::isPackerPartyInViableGame(PackerSiloId packerSiloId, Packer::PartyId partyId)
{
    bool isInViableGame = false;

    auto packer = getPackerSilo(packerSiloId);
    auto& partys = packer->getParties();
    auto partyItr = partys.find(partyId);
    if (partyItr != partys.end())
    {
        auto& games = packer->getGames();
        auto gameItr = games.find(partyItr->second.mGameId);
        if (gameItr != games.end())
        {
            auto& game = gameItr->second;
            isInViableGame = game.isViable();
        }
    }

    return isInViableGame;
}

void GamePackerSlaveImpl::markPackerSiloForRemoval(PackerSiloId packerSiloId)
{    
    if (getPackerSilo(packerSiloId) == nullptr)
    {
        TRACE_LOG("[GamePackerSlaveImpl].markPackerSiloForRemoval: PackerSilo(" << packerSiloId << ") does not exist.  Not adding for removal.");
        return;
    }

    if (mPackersMarkedForRemoval.insert(packerSiloId).second)
    {
        if (mSweepEmptySilosTimerId == INVALID_TIMER_ID)
        {
            mSweepEmptySilosTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay(), this, &GamePackerSlaveImpl::sweepEmptyPackerSilos, "GamePackerSlaveImpl::sweepEmptyPackerSilos");
        }
    }
}

bool GamePackerSlaveImpl::unmarkPackerSiloForRemoval(PackerSiloId packerSiloId)
{
    if (mPackersMarkedForRemoval.erase(packerSiloId) > 0)
    {
        if (mPackersMarkedForRemoval.empty() && mSweepEmptySilosTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mSweepEmptySilosTimerId);
            mSweepEmptySilosTimerId = INVALID_TIMER_ID;
        }
        return true;
    }

    WARN_LOG("[GamePackerSlaveImpl].unmarkPackerSiloForRemoval: PackerSilo(" << packerSiloId << ") was not in the removal list.  Possibly already removed.");
    return false;
}

void GamePackerSlaveImpl::sweepEmptyPackerSilos()
{
    // Local copy of the data, so that the iterator doesn't get invalidated while sweeping:
    PackerIdSet packersMarkedForRemoval = mPackersMarkedForRemoval;

    mPackersMarkedForRemoval.clear();
    mSweepEmptySilosTimerId = INVALID_TIMER_ID;
    for (auto packerSiloId : packersMarkedForRemoval)
    {
        auto packerSiloInfo = getPackerSiloInfo(packerSiloId);
        if (packerSiloInfo == nullptr)
        {
            WARN_LOG("[GamePackerSlaveImpl].sweepEmptyPackerSilos: PackerSilo(" << packerSiloId << ") found.  Possibly already removed.");
            continue;
        }

        auto& packer = packerSiloInfo->mPackerSilo;
        auto& immutableParties = packer.getParties();
        if (immutableParties.empty())
        {
            TRACE_LOG("[GamePackerSlaveImpl].sweepEmptyPackerSilos: PackerSilo(" << packerSiloId << "), is empty");
        }
        else
        {
            TRACE_LOG("[GamePackerSlaveImpl].sweepEmptyPackerSilos: PackerSilo(" << packerSiloId << "), contains only immutable: games(" << packer.getGames().size() << "), parties(" << packer.getParties().size() << "), players(" << packer.getParties().size() << ")");
            while (!immutableParties.empty())
            {
                auto partyId = immutableParties.begin()->first;
                cleanupSession(partyId, GameManager::WORKER_SESSION_EMPTY_SILO_REMOVED, false);
            }
        }

        // Metrics: 
        auto* packerSiloName = packer.getPackerName().c_str();
        GameManager::PackerSiloDestroyed packerSiloDestroyed;
        packerSiloDestroyed.setDestroyedPackerSiloId(packerSiloId);
        packerSiloDestroyed.setTotalParties(packer.getTotalPartiesCount());
        packerSiloDestroyed.setTotalPlayers(packer.getTotalPlayersCount());
        packerSiloDestroyed.setTotalGames(packer.getTotalGamesCreatedCount());
        //packerSiloEvent.setTotalIssuedGames(packer.getTotalReapedViableCount());
        packerSiloDestroyed.setTotalPartiesEvicted(packer.getEvictedPartiesCount());
        packerSiloDestroyed.setTotalPlayersEvicted(packer.getEvictedPlayersCount());

        //int64_t totalTimeAlive = TimeValue::getTimeOfDay().getMicroSeconds() - packer.getCreationTime();
        //packerSiloEvent.setTotalTimeAlive(totalTimeAlive);
        TRACE_LOG("[GamePackerSlaveImpl].sweepEmptyPackerSilos: " << packerSiloDestroyed);

        mPackerSilosDestroyedCounter.increment(1, packerSiloName);
        mPackerSilosGauge.decrement(1, packerSiloName);
        
        // Remove from other lists: 
        mProdNodeIdToSiloIdMap.erase(packerSiloInfo->mProductionNodeId);
        packerSiloInfo->mPackerSiloProperties.clear();  // This should be redundant, due to the subsequent destructor call. 
        mPackerSiloInfoMap.erase(packerSiloId);
    }
}

Packer::Time GamePackerSlaveImpl::getPackerPartyExpiryTime(PackerSiloId packerSiloId, Packer::PartyId partyId)
{
    Packer::Time expiryTime = -1LL;

    auto packer = getPackerSilo(packerSiloId);
    auto& partys = packer->getParties();
    auto partyItr = partys.find(partyId);
    if (partyItr != partys.end())
    {
        expiryTime = partyItr->second.mExpiryTime;
    }

    return expiryTime;
}

void GamePackerSlaveImpl::getDetailedPackerMetrics(GameManager::GetDetailedPackerMetricsResponse & response)
{
    // PACKER_TODO(metrics): #IMPROVE We likely want the option to track scores per template *and* per silo, for instance most frequent 100 used silos, and least frequent 100 used silos
    for (auto& detailedPackerMetricsPtr : mPackerDetailedMetricsByGameTemplateMap)
    {
        auto* packerSiloName = detailedPackerMetricsPtr.first.c_str();
        auto& detailedPackerMetrics = detailedPackerMetricsPtr.second;

        auto* templateMetrics = response.getGameTemplatePackerMetrics().allocate_element();// .insert(packerSiloName);
        response.getGameTemplatePackerMetrics()[packerSiloName] = templateMetrics;

        for (auto& factorScoreBreakdownItr : detailedPackerMetrics.mFactorScoreBreakdownForAllGames)
        {
            auto factorName = factorScoreBreakdownItr.first;
            auto* factorMetricsResp = templateMetrics->getFactorScoresByPercentileForGames().allocate_element();
            auto& factorScoreBreakdownArr = factorScoreBreakdownItr.second;
            for (uint64_t i = 0; i < factorScoreBreakdownArr.size(); ++i)
            {
                if (factorScoreBreakdownArr[i] != 0)
                {
                    (*factorMetricsResp)[i] = factorScoreBreakdownArr[i];
                }
            }
            templateMetrics->getFactorScoresByPercentileForGames()[factorName.c_str()] = factorMetricsResp;
        }

        for (auto& factorScoreBreakdownItr : detailedPackerMetrics.mFactorScoreBreakdownForReapedGames)
        {
            auto factorName = factorScoreBreakdownItr.first;
            auto* factorMetricsResp = templateMetrics->getFactorScoresByPercentileForSuccessfulGames().allocate_element();
            auto& factorScoreBreakdownArr = factorScoreBreakdownItr.second;
            for (uint64_t i = 0; i < factorScoreBreakdownArr.size(); ++i)
            {
                if (factorScoreBreakdownArr[i] != 0)
                {
                    (*factorMetricsResp)[i] = factorScoreBreakdownArr[i];
                }
            }
            templateMetrics->getFactorScoresByPercentileForSuccessfulGames()[factorName.c_str()] = factorMetricsResp;
        }

        for (auto& timeSpentItr : detailedPackerMetrics.mTimeSpentInViableGamesByParties)
        {
            templateMetrics->getTimeSpentInViableGamesByParties()[timeSpentItr.first] = timeSpentItr.second;
        }

        for (auto& timeSpentItr : detailedPackerMetrics.mTimeSpentInViableGamesBySuccessfulParties)
        {
            templateMetrics->getTimeSpentInViableGamesBySuccessfulParties()[timeSpentItr.first] = timeSpentItr.second;
        }

        for (auto& timeSpentItr : detailedPackerMetrics.mTimeSpentInPackerByParty)
        {
            templateMetrics->getTimeSpentInPackerByParties()[timeSpentItr.first] = timeSpentItr.second;
        }

        for (auto& timeSpentItr : detailedPackerMetrics.mTimeSpentInPackerBySuccessfulParty)
        {
            templateMetrics->getTimeSpentInPackerBySuccessfulParties()[timeSpentItr.first] = timeSpentItr.second;
        }

        for (auto& evictionCountItr : detailedPackerMetrics.mEvictionCountBySuccessfulParties)
        {
            templateMetrics->getEvictionCountBySuccessfulParties()[evictionCountItr.first] = evictionCountItr.second;
        }

        for (auto& evictionCountItr : detailedPackerMetrics.mEvictionCountByParties)
        {
            templateMetrics->getEvictionCountByParties()[evictionCountItr.first] = evictionCountItr.second;
        }

    }
}
/**
 * A 'immutable' packer session is a used to track the user group of participants returned from the search query
 */
bool GamePackerSlaveImpl::isMutableParty(Packer::PartyId partyId)
{
    return GetSliverIdFromSliverKey(partyId) != INVALID_SLIVER_ID;
}

void GamePackerSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    auto& parentStatusMap = status.getInfo();

    mPackerPlayersGauge.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerGaugePlayers", tagList, value.get()); });
    mPackerPartiesGauge.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerGaugeParties", tagList, value.get()); });
    mPackerGamesGauge.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerGaugeGames", tagList, value.get()); });
    mPackerViableGamesGauge.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerGaugeViableGames", tagList, value.get()); });
    mPackerSilosGauge.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerGaugeSilos", tagList, value.get()); });
    mFactorScoreFreq.iterate([&parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, "PackerTotalFactorScoreFreq", tagList, value.getTotal()); });
}

bool GamePackerSlaveImpl::initPackerSilo(Packer::PackerSilo& packer, PackerSession& packerSession, const Blaze::GameManager::CreateGameTemplate& cfg)
{
    // IMPORTANT: This method must currently use the indirect packer.errorLog() method of outputting errors because they need to be captured as strings for returning as part of a result during component config validation.
    EA_ASSERT(!packer.getPackerName().empty());
    auto* packerSiloName = packer.getPackerName().c_str();
    const auto& packerSiloCfg = cfg.getPackerConfig();

    // Add all the factors: 
    eastl::string fullGamePropertyName;
    for (const auto& qualityFactorCfg : packerSiloCfg.getQualityFactors())
    {
        Packer::GameQualityFactorConfig config;

        fullGamePropertyName = qualityFactorCfg->getGameProperty();
        EA::TDF::TdfPtr scoreShaperPtr;

        // PACKER_TODO - Hook up with Properties from PackerSilo as per-Riasat's commit. 
        const char8_t* failingAttr = nullptr;
        BlazeRpcError blazeErr = applyTemplateTdfAttributes(scoreShaperPtr, qualityFactorCfg->getScoreShaper(), packerSession.getAttributeMap(), packerSession.getPropertyMap(), failingAttr, "[GamePackerSlaveImpl].initPackerSilo: ");
        if (blazeErr != Blaze::ERR_OK)
        {
            ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: ScoreShaper for quality factor(" << fullGamePropertyName.c_str() << "), unable to apply value from config for the attribute " << failingAttr << ".");
            return false;
        }

        auto* factorSpec = Packer::GameQualityFactorSpec::getFactorSpecFromConfig(fullGamePropertyName, qualityFactorCfg->getTransform());
        if (factorSpec == nullptr)
        {
            ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: Transform("<< qualityFactorCfg->getTransform() <<") for quality factor(" << fullGamePropertyName.c_str() << ") has no matching spec");
            return false;
        }

        if (factorSpec->mRequiresProperty)
        {
            eastl::string inputPropertyName;
            auto& scoringMap = qualityFactorCfg->getScoringMap();
            // NOTE: Here we determine the name of the player properties that supply the per/player input data for the GQF.
            auto* scoringPropertyName = scoringMap.getPropertyName();
            if (*scoringPropertyName != '\0')
            {
                if (!GameManager::PropertyManager::configGamePropertyNameToFactorInputPropertyName(scoringPropertyName, inputPropertyName))
                {
                    ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: PackerSilo(" << packerSiloName << ":" << packer.getPackerSiloId() << ") scoring map property(" << scoringPropertyName << ") is invalid!");
                    return false;
                }
            }
            else
            {
#if 0
                // PACKER_TODO: Think about whether we need to do something special for attribute and literal specifiers that define the value of the scoring map.
                // The current thinking is: no, the code below already handles everything, because those are both essentially context'less values, their name carries little or no meaning of itself,
                // the only way in which it has meaning is in their application to determine the parameterization utilized as per playe input fields fed into the packer.
                if (*(scoringMap.getAttrName()) != '\0')
                {
                    // handle the case when the scoring map is a scenario attribute(as in the case when the quality factor game.attributes[something] factor, the name could be caller.attributes[something_desired_values]
                    EA_FAIL_MSG("Not implemented yet!"); // PACKER_TODO: implement it
                }
                else if (scoringMap.isDefaultSet())
                {
                    // PACKER_TODO: Need to ensure that default scoring map can be named sensibly somehow
                    EA_FAIL_MSG("Not implemented yet!"); // PACKER_TODO: implement it
                }
#endif
                // Currently this case means that the game property 'encodes' the player source property syntax within itself: eg: game.teams.players.uedValues[skill]
                GameManager::PropertyManager::configGamePropertyNameToFactorInputPropertyName(fullGamePropertyName.c_str(), inputPropertyName);
            }

            EA_ASSERT(!inputPropertyName.empty());

            TRACE_LOG("[GamePackerSlaveImpl].initPackerSilo: PackerSilo(" << packerSiloName << ":" << packer.getPackerSiloId() << ") mapped factor game property(" << fullGamePropertyName << ") -> input player property(" << inputPropertyName << ")");

            packer.addProperty(inputPropertyName); // this upserts the property if its not added already, noop if property exists

            config.mPropertyName = eastl::move(inputPropertyName);
        }
        config.mOutputPropertyName = fullGamePropertyName;
        config.mTransform = qualityFactorCfg->getTransform();
        config.mTeamAggregate = (Packer::PropertyAggregate) qualityFactorCfg->getTeamMergeOp(); // default = "range"  - PACKER_TODO - Make sure names match with MERGE_OPs, ensure set to NONE for factors that don't operate on teams?!

        if (scoreShaperPtr->getTdfId() == GameManager::DefaultShaper::TDF_ID)
        {
            auto& shaper = (GameManager::DefaultShaper&)*scoreShaperPtr;
            config.mBestValue = shaper.getBestValue();
            config.mGoodEnoughValue = shaper.getGoodEnoughValue();
            config.mViableValue = shaper.getViableValue();
            config.mWorstValue = shaper.getWorstValue();
            config.mGranularity = shaper.getGranularity();
        }

        if (!packer.addFactor(config))
        {
            ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: PackerSilo(" << packerSiloName << ":" << packer.getPackerSiloId() << ") failed to add factor(" << fullGamePropertyName << ")");
            return false;
        }
    }

    // Add the fixed hardcoded Variables:
    // PACKER_TODO - Manage these via a simple mapping the global Packer config 
    for (auto gamePropertyName : mPropertyManager.getRequiredPackerGameProperties())
    {
        auto iter = packerSession.getPropertyMap().find(gamePropertyName.c_str());
        if (iter != packerSession.getPropertyMap().end())
        {
            int32_t varValue = 0;
            EA::TDF::TdfGenericReference tempRef(varValue);
            if (!iter->second->convertToResult(tempRef))
            {
                ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: Missing, or unable to parse, Property (" << gamePropertyName << ") required for GamePacker to work. ");
                return false;
            }

            if (!packer.setVarByName(gamePropertyName.c_str(), varValue))
            {
                ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: PackerSilo(" << packerSiloName << ":" << packer.getPackerSiloId() << ") failed to set variable(" << gamePropertyName.c_str() << ") to value(" << varValue << ")");
                return false;
            }
        }
    }
    
    // PACKER_TODO: This validation should be implemented by the derived property system...
    if (packer.getVar(Packer::GameTemplateVar::TEAM_CAPACITY) <= 0)
    {
        ERR_LOG("[GamePackerSlaveImpl].initPackerSilo: PackerSilo(" << packerSiloName << ":" << packer.getPackerSiloId() << ") team capacity must be greater than 0");
        return false;
    }

    packer.setViableGameCooldownThreshold(packerSiloCfg.getViableGameCooldownThreshold().getMicroSeconds());
    packer.setMaxWorkingSet(packerSiloCfg.getMaxWorkingSet());
    
    return true;
    
}

bool GamePackerSlaveImpl::addPackerSilo(PackerSession& packerSession, PackerSiloId packerSiloId, Blaze::GameManager::Rete::ProductionNode* node)
{
    const char8_t* packerSiloName = packerSession.mGameTemplate.mGameTemplateName.c_str();
    auto cfgItr = packerSession.mGameTemplate.mCreateGameTemplateConfig;
    if (cfgItr == nullptr)
    {
        ERR_LOG("[GamePackerSlaveImpl].addPackerSilo: PackerSilo(" << packerSiloId << ") could not be created because template(" << packerSiloName << ") does not exist.");
        return false;
    }

    auto productionNodeId = node->getProductionNodeId();

    // Add PackerSiloInfo,  or do Lookup if it already exists: 
    auto packerSiloRet = mPackerSiloInfoMap.insert(packerSiloId);
    auto& packerSiloInfo = packerSiloRet.first->second;
    auto& packer = packerSiloInfo.mPackerSilo;
    if (!packerSiloRet.second)
    {
        if (packerSiloInfo.mProductionNodeId != productionNodeId)
            WARN_LOG("[GamePackerSlaveImpl].addPackerSilo: Incoming prod node (" << productionNodeId << ") was not in use by existing packer silo "<< packerSiloId <<". Value used (" << packerSiloInfo.mProductionNodeId << ").");

        unmarkPackerSiloForRemoval(packerSiloId); // make sure to erase from the marked set in case we are readding it
        return true;
    }

    // Add the Prod Node mapping: 
    packerSiloInfo.mProductionNodeId = node->getProductionNodeId();
    mProdNodeIdToSiloIdMap[packerSiloInfo.mProductionNodeId] = packerSiloId;

    // For the PlatformFilter, just add the Allowed list as the value that was set by the Session - Since it should be the exact match.  (Assumes that the session that matches first is the one with the conditions.)
    getPropertyRuleDefinition()->convertProdNodeToSiloProperties(packerSession.getFilterMap(), node, packerSiloInfo.mPackerSiloProperties);


    //if (!formattedConditions.empty())
    //    formattedConditions.pop_back();

    packer.setPackerSiloId(packerSiloId);
    //packer.setCreationTime(creationTime.getMicroSeconds());
    packer.setPackerName(packerSiloName);
    //packer.setPackerDetails(formattedConditions.c_str());
    packer.setParentObject(this);
    packer.setUtilityMethods(*this);
    if (initPackerSilo(packer, packerSession, *cfgItr))
    {
        // TDF created solely for logging output: 
        if ((BLAZE_IS_LOGGING_ENABLED(getLogCategory(), Blaze::Logging::TRACE)))
        {
            GameManager::PackerSiloCreated packerSiloCreated;
            packerSiloCreated.setGameTemplateName(packerSiloName);
            packerSiloCreated.setCreatedPackerSiloId(packerSiloId);
            eastl::string factors;
            for (auto& factor : packer.getFactors())
            {
                factors += factor.mFactorName;
                factors += ';';
            }
            if (!factors.empty())
                factors.pop_back();
            packerSiloCreated.setGameQualityFactors(factors.c_str());
            //packerSiloCreated.setPackerSiloConditions(formattedConditions.empty() ? "MATCH_ANY" : formattedConditions.c_str());
            TRACE_LOG("[GamePackerSlaveImpl].addPackerSilo: " << packerSiloCreated);
        }

        mPackerDetailedMetricsByGameTemplateMap.insert(packerSiloName); // Noop if already there
        mPackerSilosCreatedCounter.increment(1, packerSiloName);
        mPackerSilosGauge.increment(1, packerSiloName);
        return true;
    }
    else
    {
        ERR_LOG("[GamePackerSlaveImpl].addPackerSilo: Failed to initialize PackerSilo(" << packerSiloId << ")");

        // PACKER_TODO(metrics): log error here

        // remove map entries if the above call added them
        mProdNodeIdToSiloIdMap.erase(packerSiloInfo.mProductionNodeId);
        if (packerSiloRet.second)
        {
            packerSiloInfo.mPackerSiloProperties.clear();
            mPackerSiloInfoMap.erase(packerSiloRet.first);
        }
        mPackerSilosDestroyedCounter.increment(1, packerSiloName);
        mPackerSilosGauge.decrement(1, packerSiloName);

        return false;
    }
}

const char8_t* GamePackerSlaveImpl::getInstanceName(InstanceId instanceId, const eastl::string& instanceName)
{
    auto* instance = gController->getRemoteServerInstance(instanceId);
    if (instance == nullptr)
    {
        const_cast<eastl::string&>(instanceName).sprintf("%u", (uint32_t) instanceId);
        return instanceName.c_str();
    }
    return instance->getInstanceName();
}

bool GamePackerSlaveImpl::isJobValid(GamePackerSlaveImpl::FinalizationJob& job, PackerSession::PackerSessionState state) const
{
    for (auto& packerSessByScenarioIdItr : job.mPackerSessionByScenarioIdMap)
    {
        auto& packerSession = *packerSessByScenarioIdItr.second;
        if (packerSession.mOrphaned || packerSession.mState != state)
        {
            return false;
        }
    }
    return true;
}

void GamePackerSlaveImpl::onNotifyWorkerPackerSessionAvailable(const Blaze::GameManager::NotifyWorkerPackerSessionAvailable& data, ::Blaze::UserSession *)
{
    auto instanceId = data.getInstanceId();
    TRACE_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionAvailable: New work available on owner(" << getInstanceName(instanceId) << ") on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << ")");
    scheduleObtainSession(instanceId);
}

void GamePackerSlaveImpl::onNotifyWorkerPackerSessionRelinquish(const Blaze::GameManager::NotifyWorkerPackerSessionRelinquish& data, ::Blaze::UserSession *)
{
    auto packerSessionId = data.getPackerSessionId();
    EA_ASSERT(!isPlaceholderSessionId(packerSessionId));
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    if (packerSessionItr == mPackerSessions.end())
    {
        WARN_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionRelinquish: Expected session(" << packerSessionId << ") not found!");
        return;
    }

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    if (packerSession.mFinalizationJobId != 0)
    {
        // PACKER_TODO: add metric
        TRACE_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionRelinquish: session(" << packerSessionId << ") has been updated by owner, while assigned to job(" << packerSession.mFinalizationJobId << "), deferring resubmission until job completion");
        packerSession.mState = PackerSession::AWAITING_RELEASE;
        // PACKER_TODO: If the Finalization job fiber ends up taking an exception(which prevents cleaning it up) we should validate that the job still exists, and then perform a forced cleanup on it here instead of returning...
        return;
    }

    RpcCallOptions opts;
    opts.ignoreReply = true;
    Blaze::GameManager::WorkerRelinquishPackerSessionRequest request;
    request.setPackerSessionId(packerSessionId);
    BlazeError rc = getMaster()->workerRelinquishPackerSession(request, opts);
    if (rc == ERR_OK)
    {
        TRACE_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionRelinquish: Relinquished session(" << packerSessionId << ") to owner");
    }
    else
    {
        ERR_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionRelinquish: Failed to relinquish session(" << packerSessionId << ") to owner, error: " << rc);
    }

    cleanupSession(packerSessionId, GameManager::WORKER_SESSION_RELINQUISHED);
}

void GamePackerSlaveImpl::onNotifyWorkerPackerSessionTerminated(const Blaze::GameManager::NotifyWorkerPackerSessionTerminated& data, ::Blaze::UserSession *)
{
    auto packerSessionId = data.getPackerSessionId();
    EA_ASSERT(!isPlaceholderSessionId(packerSessionId));
    const auto packerSessionItr = mPackerSessions.find(packerSessionId);
    const auto reason = data.getTerminationReason();
    if (packerSessionItr == mPackerSessions.end())
    {
        if (reason == GameManager::PackerScenarioData::SCENARIO_TIMED_OUT)
        {
            // NOTE: workers already utilize timeout information for the session in order to prioritize packing and to avoid getting overloaded in case master is offline; therefore they are expected to time out/cleanup the session locally, but the master still sends a termination notification for completeness.
            INFO_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionTerminated: session(" << packerSessionId << ") not found, already timed out locally");
        }
        else
        {
            WARN_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionTerminated: expected session(" << packerSessionId << ") not found, terminated by owner, reason(" << GameManager::PackerScenarioData::TerminationReasonToString(reason) << ")");
        }
        return;
    }

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    if (packerSession.mFinalizationJobId != 0)
    {
        // PACKER_TODO: add metric
        INFO_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionTerminated: session(" << packerSessionId << ") terminated by owner, reason(" << GameManager::PackerScenarioData::TerminationReasonToString(reason) << ") while assigned to job(" << packerSession.mFinalizationJobId << "), cleanup deferred until job completion"); // PACKER_TODO: trace?
        packerSession.mOrphaned = true;
        // PACKER_TODO: If the Finalization job fiber ends up taking an exception(which prevents cleaning it up) we should validate that the job still exists, and then perform a forced cleanup on it here instead of returning...
        return;
    }

    INFO_LOG("[GamePackerSlaveImpl].onNotifyWorkerPackerSessionTerminated: session(" << packerSessionId << "), scenario(" << packerSession.getOriginatingScenarioId() << ") terminated by owner, reason(" << GameManager::PackerScenarioData::TerminationReasonToString(reason) << "), cleanup session"); // PACKER_TODO: trace?

    cleanupSession(packerSessionId, GameManager::WORKER_SESSION_TERMINATED);
}

bool GamePackerSlaveImpl::isDrainComplete()
{
    return mPackerSessions.empty();
}

bool GamePackerSlaveImpl::onValidateConfig(GamePackerConfig& config, const GamePackerConfig* referenceConfigPtr, ::Blaze::ConfigureValidationErrors& validationErrors) const
{
    auto& workerSettings = getConfig().getWorkerSettings();
    TimeValue lowestMaxClaimDuration("1s", TimeValue::TIME_FORMAT_INTERVAL);
    if (workerSettings.getMaxSlotClaimDuration().getSec() < lowestMaxClaimDuration.getSec())
    {
        StringBuilder sb;
        sb << "[GamePackerSlaveImpl].onValidateConfig: invalid max slot claim duration(" << workerSettings.getMaxSlotClaimDuration().getSec() << ") must be at least(" << lowestMaxClaimDuration.getSec() << "seconds)";
        validationErrors.getErrorMessages().push_back(sb.get());
    }

    TimeValue lowestTimeBetweenClaims("50ms", TimeValue::TIME_FORMAT_INTERVAL);
    if (workerSettings.getMinTimeBetweenSlotClaims().getMillis() < lowestTimeBetweenClaims.getMillis())
    {
        StringBuilder sb;
        sb << "[GamePackerSlaveImpl].onValidateConfig: invalid min time between claims(" << workerSettings.getMinTimeBetweenSlotClaims().getMillis() << ") must be at least(" << lowestTimeBetweenClaims.getMillis() << "millis)";
        validationErrors.getErrorMessages().push_back(sb.get());
    }

    if (workerSettings.getMaxSlotClaimDuration() < 2 * workerSettings.getMinTimeBetweenSlotClaims())
    {
        StringBuilder sb;
        sb << "[GamePackerSlaveImpl].onValidateConfig: invalid max slot claim duration(" << workerSettings.getMaxSlotClaimDuration() << ") must be greater than 2 x min time between claims(" << workerSettings.getMinTimeBetweenSlotClaims() << ")";
        validationErrors.getErrorMessages().push_back(sb.get());
    }

    mPropertyManager.validateConfig(config.getProperties(), validationErrors);



    for (auto& templateItr : config.getCreateGameTemplatesConfig().getCreateGameTemplates())
    {
        const auto& templateName = templateItr.first;
        const auto& packerConfig = templateItr.second->getPackerConfig();

        for (auto& curPacker : packerConfig.getQualityFactors())
        {
            // Sanity check all ScoreShapers: 
            Blaze::GameManager::AttributeToTypeDescMap attrToTypeDesc;
            eastl::set<EA::TDF::TdfId> acceptableTdfs = { Blaze::GameManager::DefaultShaper::TDF_ID };

            // Verify that all of the data types actually exist.
            // This is required because the config decoder now has logic to load generics into default data types (string, list<string>).
            const Blaze::GameManager::PropertyName& propertyName = curPacker->getGameProperty();

            // For each mapped element, we do the same thing that the old code did:
            StringBuilder errorPrefix;
            errorPrefix << "[GamePackerSlaveImpl].onValidateConfig: Template: "<< templateName.c_str() <<" Quality factor: " << propertyName.c_str() << " ";
            validateTemplateTdfAttributeMapping(curPacker->getScoreShaper(), attrToTypeDesc, validationErrors, errorPrefix.get(), acceptableTdfs);
        }
    }
    return validationErrors.getErrorMessages().empty();
}

bool GamePackerSlaveImpl::onConfigure()
{
    // IMPORTANT: Need to use matchmaker id type because we don't want to overlap with matchmaker ids
    // PACKER_TODO: Investigate if the above is actually true (or if it's time to use PACKER_IDTYPE_SESSION and PACKER_IDTYPE_SCENARIO, etc.
    // it may now be ok to overlap the packer session identifiers and matchmaker session identifiers because neither core slaves nor search slaves
    // should ever overlap legacy and modern matchmaking requests in any way...
    if (gUniqueIdManager->reserveIdType(Matchmaker::MatchmakerSlave::COMPONENT_ID, GameManager::MATCHMAKER_IDTYPE_SESSION) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving matchmaker session id " <<
            Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID << " with unique id manager");
        return false;
    }

    if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GameManager::GameManagerMaster::COMPONENT_ID), Blaze::GameManager::GAMEMANAGER_IDTYPE_GAME) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving game id " <<
            RpcMakeSlaveId(GameManager::GameManagerMaster::COMPONENT_ID) << " with unique id manager");
        return false;
    }

    //Setup unique id management for game external session updates
    if (gUniqueIdManager->reserveIdType(GameManager::GameManagerSlave::COMPONENT_ID, GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving component id " <<
            GameManager::GameManagerSlave::COMPONENT_ID << ", idType " << GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION << " with unique id manager");
        return false;
    }

    if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GameManager::GameManagerSlave::COMPONENT_ID), Blaze::GameManager::GAMEMANAGER_IDTYPE_GAMEBROWSER_LIST) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving game browser id " <<
            RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID) << " with unique id manager");
        return false;
    }

    if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID), Blaze::GameManager::GAMEPACKER_IDTYPE_FINALIZATION_JOB) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving finalization job id " <<
            RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID) << " with unique id manager");
        return false;
    }

    if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID), Blaze::GameManager::GAMEPACKER_IDTYPE_PACKER_SILO) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving packer silo id " <<
            RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID) << " with unique id manager");
        return false;
    }

    //Setup unique id management for game external session updates
    if (gUniqueIdManager->reserveIdType(RpcMakeSlaveId(GameManager::GameManagerSlave::COMPONENT_ID), GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION) != ERR_OK)
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving component id " <<
            RpcMakeSlaveId(GameManager::GameManagerSlave::COMPONENT_ID) << ", idType " << GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION << " with unique id manager");
        return false;
    }

    const GamePackerConfig& config = getConfig();

    mPropertyManager.onConfigure(config.getProperties());

    // Temp Sanity Check to ensure that 
    if (mPropertyManager.getRequiredPackerGameProperties().empty())
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed reserving packer silo id " <<
            RpcMakeSlaveId(GamePacker::GamePackerSlave::COMPONENT_ID) << " with unique id manager");
        return false;
    }
    mExternalGameSessionServices = BLAZE_NEW GameManager::ExternalSessionUtilManager;

    // NOTE: We deliberately avoid passing in the maxMemberCount parameter because it is only used for validation which is already performed in gamemanager and we want to avoid creating an unnecessary dependency 
    // on the matchmaking_settings configuration that currently stores the maximum supported game capacity value in: config.getMatchmakerSettings().getGameSizeRange().getMax()
    if (!GameManager::ExternalSessionUtil::createExternalSessionServices(*mExternalGameSessionServices, config.getGameSession().getExternalSessions(), config.getGameSession().getPlayerReservationTimeout()))
    {
        return false;   // (err already logged)
    }

    // Initialize ReteNetwork and RuleDefinitionCollection
    // PACKER_TODO: Figure out if we can make use of a single shared RuleDefinitionCollection among all the rete networks, in particular we are interested in investigating whether we could get perf/memory gains by leveraging the single string table across all the rete networks.
    mReteNetwork = BLAZE_NEW GameManager::Rete::ReteNetwork(getMetricsCollection(), GamePackerSlave::LOGGING_CATEGORY);
    if (!mReteNetwork->configure(config.getRete()))
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed to configure RETE network.");
        return false;
    }
    mRuleDefinitionCollection = BLAZE_NEW GameManager::Matchmaker::RuleDefinitionCollection(getMetricsCollection(), mReteNetwork->getWMEManager().getStringTable(), getLogCategory(), true);
    if (!mRuleDefinitionCollection->initFilterDefinitions(config.getFilters(), config.getGlobalFilters(), config.getProperties()))
    {
        ERR_LOG("[GamePackerSlaveImpl].onConfigure: Failed to initialize filter rule definition.");
        return false;
    }

    auto& createTemplatesMap = config.getCreateGameTemplatesConfig().getCreateGameTemplates();
    for (auto& templateItr : createTemplatesMap)
    {
        auto* createGameTemplateName = templateItr.first.c_str();
        if (templateItr.second->getPackerConfig().getQualityFactors().empty())
            continue; // if there are no quality factors then we don't need to do filtering!
        auto ret = mGameTemplateMap.insert(createGameTemplateName);
        if (!ret.second)
        {
            ERR_LOG("[GamePackerSlaveImpl].onConfigure: Duplicate template("<< createGameTemplateName <<")");
            return false;
        }
        ret.first->second = BLAZE_NEW GameTemplate(createGameTemplateName);
        auto& gameTemplate = *ret.first->second;
        gameTemplate.mCreateGameTemplateConfig = templateItr.second; // link to config object
    }

    return true;
}

bool GamePackerSlaveImpl::onPrepareForReconfigure(const GamePackerConfig& newConfig)
{
    if (!mExternalGameSessionServices->onPrepareForReconfigure(newConfig.getGameSession().getExternalSessions()))
    {
        return false;
    }

    return true;
}

bool GamePackerSlaveImpl::onReconfigure()
{
    const GamePackerConfig& config = getConfig();

    // PACKER_TODO: Actually robustly implement Reconfig support, not this involves ensuring that templates that have 'live' packer sessions cannot be removed as we are doing here
    if (mPackerSessions.empty())
    {
        auto& createTemplatesMap = config.getCreateGameTemplatesConfig().getCreateGameTemplates();
        for (auto& templateItr : createTemplatesMap)
        {
            auto* createGameTemplateName = templateItr.first.c_str();
            if (templateItr.second->getPackerConfig().getQualityFactors().empty())
                continue; // if there are no quality factors then we don't need to do filtering!
            auto ret = mGameTemplateMap.insert(createGameTemplateName);
            if (ret.second)
            {
                ret.first->second = BLAZE_NEW GameTemplate(createGameTemplateName);
            }
            ret.first->second->mCreateGameTemplateConfig = templateItr.second; // link/update config object
        }
    }
    else
    {
        WARN_LOG("[GamePackerSlaveImpl].onReconfigure: Live packer sessions(" << mPackerSessions.size() << "), reconfigure skipped!");
    }

    auto& gameSessionConfig = config.getGameSession();
    mExternalGameSessionServices->onReconfigure(gameSessionConfig.getExternalSessions(), gameSessionConfig.getPlayerReservationTimeout());

    return true;
}

bool GamePackerSlaveImpl::onResolve()
{
    if (!registerForSearchSlaveNotifications())
    {
        FAIL_LOG("[GamePackerSlaveImpl].onResolve: Failed to register broker for search slave notifications!");
        return false;
    }

    gController->registerDrainStatusCheckHandler(COMPONENT_INFO.name, *this);

    // TODO: replace this with a callback that can be triggered after a slave calls subscribeNotifications() to its master...(idea: add component virtual function called (onNotificationSubscribeToInstance(InstanceId instanceId)
    // Question: should we move this into onActivate(), also we should probably ensure that we add known components as well.
    // NOTE: we can have these callbacks maintain our mClaimInfoByInstanceMap. Whenever one of these is dispatched
    getMaster()->addNotificationListener(*this);
    getMaster()->registerNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb(this, &GamePackerSlaveImpl::onSubscribedForNotificationsForMaster));

    return true;
}

bool GamePackerSlaveImpl::onCompleteActivation()
{
    scheduleClaimWorkSlotIdAndNotifyMasters(TimeValue::getTimeOfDay());
    return true;
}

void GamePackerSlaveImpl::onShutdown()
{
    if (getMaster())
        getMaster()->deregisterNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb(this, &GamePackerSlaveImpl::onSubscribedForNotificationsForMaster));

    if (mWorkSlotUpdateTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mWorkSlotUpdateTimerId);
        mWorkSlotUpdateTimerId = INVALID_TIMER_ID;
    }

    //Fiber::join(mFiberGroupId); // wait for all scheduled fibers to exit

    for (auto& itr : mPackerSessions)
    {
        delete itr.second;
    }

    mPackerSessions.clear();
    if (mReteNetwork)
    {
        mReteNetwork->onShutdown();
    }

    for (auto& itr : mGameTemplateMap)
    {
        delete itr.second;
    }

    mGameTemplateMap.clear();

    if (mSweepEmptySilosTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mSweepEmptySilosTimerId);
        mSweepEmptySilosTimerId = INVALID_TIMER_ID;
        sweepEmptyPackerSilos();
    }

    //gUserSessionManager->removeSubscriber(*this);
    //gUserSessionManager->getQosConfig().removeSubscriber(*this);
    gController->deregisterDrainStatusCheckHandler(COMPONENT_INFO.name);
}

void GamePackerSlaveImpl::onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data, ::Blaze::UserSession *associatedUserSession)
{
    // PACKER_TODO: annoyingly we dont have a way to quickly look up added/removed games from list here, therefore we need to first index all the games.
    eastl::hash_map<GameManager::GameId, GameManager::GameBrowserGameData*> gameDataIndex;
    if (data.getGameDataMap().empty())
    {
        // no data updates
        TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: list(" << GAME_PACKER_LIST_NAME << ") contains contains only removals.");
    }
    else
    {
        auto gameDataItr = data.getGameDataMap().find(GAME_PACKER_LIST_NAME);
        if (gameDataItr != data.getGameDataMap().end())
        {
            if (data.getGameDataMap().size() > 1)
            {
                WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: unexpected multiple subscription lists, only list(" << gameDataItr->first.c_str() << ") results will be used!");
            }

            TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: list(" << gameDataItr->first << ") found active/updated game matches(" << gameDataItr->second->size() << ")");
            gameDataIndex.reserve(gameDataItr->second->size());
            for (auto& gameData : *gameDataItr->second)
            {
                auto valueItr = gameData->getPropertyNameMap().find("game.id");
                if (valueItr == gameData->getPropertyNameMap().end())
                {
                    WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing 'game.id' property!");
                    continue;
                }
                GameManager::GameId gameId = valueItr->second->getValue().asUInt64(); // PACKER_TODO - Manually converting like this sorta defeats the purpose of storing things generically. 

                auto ret = gameDataIndex.insert(gameId);     // Kept around even if Properties are used.
                EA_ASSERT(ret.second);
                ret.first->second = gameData.get();
            }
        }
        else
        {
            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: expected list(" << GAME_PACKER_LIST_NAME << ") not found!");
        }
    }

    auto unpackGameAndCleanupPlaceholderSessions = [this](const Packer::Game* packerGame, GameManager::WorkerSessionCleanupReason reason) {
        uint32_t unpackedPlayersBelongingToMutableParties = 0;
        const auto partyCount = packerGame->getPartyCount();
        const auto gameId = packerGame->mGameId;
        Packer::GamePartyList copiedGameParties(packerGame->mGameParties.begin(), packerGame->mGameParties.begin() + partyCount); // need to copy here becuse removeSessionFromPackerSilo destroys packerGame...
        packerGame = nullptr; // invalidate to avoid accidental access
        for (auto& existingGameParty : copiedGameParties)
        {
            auto existingPackerSessionId = existingGameParty.mPartyId;
            if (existingGameParty.mImmutable)
            {
                auto immutablePackerSessionItr = mPackerSessions.find(existingPackerSessionId);
                EA_ASSERT_MSG(immutablePackerSessionItr != mPackerSessions.end(), "Session must exist if the party is present!");
                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: remove placeholder session(" << existingPackerSessionId << ") before " << reason << " active game(" << gameId << ")");
                // NOTE: We remove the party corresponding to packerSessionId from its silo (which triggers the *destruction* of the game and requeueing of the remaining parties, which will be cleaned up in turn).

                cleanupSession(existingPackerSessionId, reason, false);
            }
            else
            {
                unpackedPlayersBelongingToMutableParties += existingGameParty.mPlayerCount;

                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: requeue session(" << existingPackerSessionId << ") prior to repacking");
            }
        }
        return unpackedPlayersBelongingToMutableParties;
    };

    for (auto& listUpdate : data.getGameListUpdate())
    {
        const auto gameListId = listUpdate->getListId();
        auto activeGameQueryItr = mActiveGameQueryByListIdMap.find(gameListId);
        if (activeGameQueryItr == mActiveGameQueryByListIdMap.end())
        {
            TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Untracked active game query(" << gameListId << "), skip");
            continue;
        }
        auto& gameTemplate = *activeGameQueryItr->second->mGameTemplate;
        const auto* gameTemplateName = gameTemplate.mGameTemplateName.c_str();

        EA_ASSERT(listUpdate->getAddedGameFitScoreList().size() <= MAX_GB_LIST_CAPACITY); // PACKER_TODO: #DEBUGGING Replace this check with warning

        TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: GameTemplate(" << gameTemplateName << "), game query(" << gameListId << ") found game matches, added(" << listUpdate->getAddedGameFitScoreList().size() << "), updated(" << listUpdate->getUpdatedGameFitScoreList().size() << "), removed(" << listUpdate->getRemovedGameList().size() << ")");
        for (auto packerSiloId : activeGameQueryItr->second->mInitiatorSiloIdSet)
        {
            Packer::PackerSilo* packerSilo = getPackerSilo(packerSiloId);
            if (packerSilo == nullptr)
            {
                EA_FAIL(); // PACKER_TODO: log warning?
                continue;
            }
            EA_ASSERT_MSG(!packerSilo->hasPendingParties(), "Should not have unpacked parties!");

            uint32_t unpackedPlayersBelongingToMutableParties = 0;
            // process removed games
            for (auto gameId : listUpdate->getRemovedGameList())
            {
                const Packer::Game* packerGame = packerSilo->getPackerGame(gameId);
                if (packerGame == nullptr)
                {
                    // This can happen only if the slave has already packed and issued or reaped this active game
                    TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") removed untracked active game(" << gameId << "), skip");
                    continue;
                }
                // Handle active game removal
                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") removed tracked active game(" << gameId << ")");

                // Remove the Game + kill the immutable players.
                unpackedPlayersBelongingToMutableParties += unpackGameAndCleanupPlaceholderSessions(packerGame, GameManager::WORKER_SESSION_ACTIVE_GAME_REMOVED);
                packerGame = nullptr; // invalidate to avoid accidental access
                EA_ASSERT(packerSilo->getPackerGame(gameId) == nullptr); // game must be gone!
            }

            uint32_t openSeatsInActiveGames = 0;
            const auto gameCapacity = (uint32_t)packerSilo->getVar(Packer::GameTemplateVar::GAME_CAPACITY);
            // process added/updated games
            Search::GameFitScoreInfoList* matchedGameLists[] = { &listUpdate->getUpdatedGameFitScoreList(), &listUpdate->getAddedGameFitScoreList()};
            for (auto& matchedGameList : matchedGameLists)
            {
                bool isNewIncomingGame = (matchedGameList == &listUpdate->getAddedGameFitScoreList());
                auto* incomingListType = isNewIncomingGame ?  "new" : "updated";
                GameManager::GameId prevGameId = GameManager::INVALID_GAME_ID;
                for (auto matchedGameItr = matchedGameList->begin(), matchedGameEnd = matchedGameList->end(); matchedGameItr != matchedGameEnd;)
                {
                    GameManager::GameId incomingGameId = (*matchedGameItr)->getGameId();
                    ++matchedGameItr; // deliberately increment here instead of in the for loop, because we may need to repeat a game when repacking
                    auto gameItr = gameDataIndex.find(incomingGameId);
                    EA_ASSERT(gameItr != gameDataIndex.end());
                    const bool repackGame = (incomingGameId == prevGameId);

                        // Load Values required from the PropertyMap: 
                    auto& propertyNameMap = gameItr->second->getPropertyNameMap();
                    auto valueItr = propertyNameMap.find("game.version");
                    if (valueItr == propertyNameMap.end())
                    {
                        WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing 'game.version' property!");
                        continue;
                    }
                    uint64_t gameRecordVersion = valueItr->second->getValue().asUInt64();


                    auto* packerGame = packerSilo->getPackerGame(incomingGameId);
                    if (packerGame == nullptr)
                    {
                        // active game not yet tracked in this packer silo, populate it
                        if (repackGame)
                        {
                            TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") repack known active game(" << incomingGameId << ")");
                            // PACKER_TODO: Add debug metric
                        }
                        else
                        {
                            if (isNewIncomingGame)
                            {
                                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") matched(new) active game(" << incomingGameId << ")");
                            }
                            else
                            {
                                // This can occur when a new game was previously deliberately skipped by the packer due to being full or not meeting criteria, it's not an error but nice to know when debugging
                                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") matched(" << incomingListType << ") active game(" << incomingGameId << "), expected(new)");
                            }

                            // PACKER_TODO: Add debug metric
                        }


                        valueItr = propertyNameMap.find("players.userGroupIds");
                        if (valueItr == propertyNameMap.end())
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing game.players.userGroupIds' property!");
                            continue;
                        }
                        GameManager::UserGroupIdList userGroupIds = *reinterpret_cast<GameManager::UserGroupIdList*>(valueItr->second->getValue().asAny()); // List<ObjectId>

                        // Debatably, this info should probably be combined with other pieces of info that are used (like rolename and player attributes)
                        valueItr = propertyNameMap.find("players.teamIndexes");
                        if (valueItr == propertyNameMap.end())
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing game.players.teamIndexes' property!");
                            continue;
                        }
                        GameManager::TeamIndexList teamIndexes = *reinterpret_cast<GameManager::TeamIndexList*>(valueItr->second->getValue().asAny()); // List<ObjectId>

                        valueItr = propertyNameMap.find("players.participantIds");
                        if (valueItr == propertyNameMap.end())
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing game.players.participantIds' property!");
                            continue;
                        }
                        GameManager::PlayerIdList playerIds = *reinterpret_cast<GameManager::PlayerIdList*>(valueItr->second->getValue().asAny()); // List<ObjectId>

                        valueItr = propertyNameMap.find("players.userSessionIds");
                        if (valueItr == propertyNameMap.end())
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Missing game.players.userSessionIds' property!");
                            continue;
                        }
                        UserSessionIdList sessionIds = *reinterpret_cast<UserSessionIdList*>(valueItr->second->getValue().asAny()); // List<ObjectId>


                        auto gameRosterSize = userGroupIds.size(); // matchedGame->getGameRoster().size()

                        if (gameRosterSize >= gameCapacity)
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") matched(" << incomingListType << ") active game(" << incomingGameId << ") has members(" << gameRosterSize << "), capacity(" << gameCapacity << "), skipping due to insufficient capacity.");
                            continue;
                        }


                        // NOTE: A valid UserGroupId is *required* for members of fetched games in order to specify stable parties that are to be processed
                        // by the packer as a unit. In matchmaking, designating parties with a UserGroupId is optional since by default all members of a
                        // single scenario always form an atomic party in the packer.
                        eastl::vector_map<GameManager::UserGroupId, PackerSession*> immutableSessionByGroupMap;
                        immutableSessionByGroupMap.reserve(gameRosterSize);
                        uint32_t numActivePlayers = 0;
                        // for (auto& playerData : matchedGame->getGameRoster())
                        for (int32_t userIndex = 0; userIndex < userGroupIds.size(); ++userIndex)
                        {
                            auto curUserGroupId = userGroupIds[userIndex];
                            auto curTeamIndex = teamIndexes[userIndex];
                            auto curPlayerId = playerIds[userIndex];
                            auto curSessionId = sessionIds[userIndex];
                            
                            auto fakeUserGroupId = curUserGroupId;
                            if (fakeUserGroupId == EA::TDF::OBJECT_ID_INVALID)
                            {
                                fakeUserGroupId.id = curSessionId;
                                ERR_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: active game(" << incomingGameId << ") incoming member(" << curPlayerId << ") has unspecified user group(" << curUserGroupId << "), use synthetic id(" << fakeUserGroupId << ")");
                            }
                            else
                            {
                                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: active game(" << incomingGameId << ") incoming member(" << curPlayerId << ") has user group id(" << fakeUserGroupId << ")");
                            }
                            auto ret = immutableSessionByGroupMap.insert(fakeUserGroupId);
                            if (ret.second)
                            {
                                // PACKER_TODO:  Storing Active Players in the same maps as the packer sessions is very wasteful.
                                //   This is going to exhaust matchmaking id pools very quickly, since this is using (# silos * # parties in-game) ids. 
                                //   These immutable players should just be held on the Packer as part of the Game.  There's no value to creating and holding a fake matchmaking session on the Slave. 

                                uint64_t uniqueIdBase = 0;
                                auto idType = Blaze::GameManager::MATCHMAKER_IDTYPE_SESSION;
                                BlazeError errRet = gUniqueIdManager->getId(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, (uint16_t) idType, uniqueIdBase, SLIVER_KEY_BASE_MIN);
                                if (errRet != Blaze::ERR_OK)
                                {
                                    ERR_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: couldn't get matchmaking session Id, got error '" << errRet << "'.");
                                    immutableSessionByGroupMap.erase(ret.first);
                                    continue;
                                }
                                PackerSessionId newPackerSessionId = BuildSliverKey(INVALID_SLIVER_ID, uniqueIdBase); // Needed to ensure uniqueness because legacy mm sessions and packer sessions both share id space
                                PackerSession& newPackerSession = *(BLAZE_NEW PackerSession(gameTemplate));
                                newPackerSession.mPackerSessionId = newPackerSessionId;
                                
                                ret.first->second = &newPackerSession; // add to active map
                                mPackerSessions[newPackerSessionId] = &newPackerSession;        // add to main session map
                                
                                newPackerSession.mImmutableTeamIndex = curTeamIndex;
                                newPackerSession.mImmutableGameId = incomingGameId;
                                newPackerSession.mImmutableGameListId = gameListId;
                                newPackerSession.mImmutableGameVersion = gameRecordVersion;
                                newPackerSession.mImmutableGroupId = curUserGroupId;            // always set group id as originally specified in the request
                                newPackerSession.mSessionStartTime = TimeValue::getTimeOfDay();
                                // PACKER_TODO: Need to account for per sessions expiration time as well as scenario expiration time
                                newPackerSession.mSessionExpirationTime = INT64_MAX; // never expire
                                // PACKER_TODO: Mutable parties care more about how many 'positions' they were considered for, rather than the number of participants in active games that were known over the duration of the mutable party's session.
                                newPackerSession.mParentMetricsSnapshot.mTotalNewPartyCount = newPackerSession.mGameTemplate.mMetrics.mTotalNewPartyCount;

                                // Build the Property Information that will fill in the SourceProperties of the Players.
                                // PACKER_TODO:  We shouldn't require holding the internal request for anything.
                                //    Remove it, and put the UserSessionInfo and PropertyMap data somewhere else. 
                                newPackerSession.mMmInternalRequest = BLAZE_NEW Blaze::GameManager::StartMatchmakingInternalRequest(); 

                                // Required Property for adding Players:
                                auto& sessionPropertyMap = newPackerSession.getPropertyMap();
                                auto participantIdsIter = sessionPropertyMap.allocate_element();
                                GameManager::PlayerIdList myList;
                                myList.push_back(curPlayerId);
                                participantIdsIter->set(myList);
                                sessionPropertyMap[GameManager::PropertyManager::PROPERTY_PLAYER_PARTICIPANT_IDS] = participantIdsIter;

                                // Okay, so we need to handle the following:
                                //   gamePropertyName     --  DataType returned by SearchSlave
                                //  'game.uedValue[FOO]'  --    list<uint64> (per-player)
                                //  'game.pingSite'       --      string
                                //  'game.attrName[FOO]'  --      string
                                // So, we handle the conversions in a hacky way, until I can think of a better way to do things:
                                // If ScoreMap/Keys are provided and the gameProperty returned is a string, fake it:
                                // Otherwise, convert the list of values into a single value for the current player. 

                                // Gather the PackerSilo and lookup the GameTemplate from the Packer/GameTemplateName.  Then get the config from that. 
                                newPackerSession.mImmutableScoringInfoByFactor.resize(packerSilo->getFactors().size());
                                for (const auto& factor : packerSilo->getFactors())
                                {
                                    auto* propertyDescriptor = factor.getInputPropertyDescriptor();
                                    // For each factor, check if keys or scoreMap are set, and if they don't have default then we'll need to fill them in. 
                                    if (propertyDescriptor == nullptr)
                                        continue; // skip internal property factors

                                    auto* outputPropertyName = factor.mOutputPropertyName.c_str();

                                    // All of the Game Properties should have been found, unless they're not real properties (ex. "game.players.count')
                                    auto propIter = propertyNameMap.find(outputPropertyName);
                                    if (propIter == propertyNameMap.end())
                                        continue;

                                    // PACKER_TODO: Need to make this more robust, and handle cases where a factor with a scalar output (like game.pingSite, still needs to have its scoring values filled in, such as may be the case for P2P games that still need to be evaluated based upon the ping information of each player, unlike DS games where the per/player ping info for active players is irrelevant to the scoring of the game).
                                    // PACKER_TODO: #OPTIMIZATION This currently populates values per factor, but some factors may operate on common property values, so those should be stored once by doing a pass on all property values, and writing those out once.
                                    // ANOTHER ISSUE: How do we handle blank game.pingsite, or for that matter any empty property field key that does not match anything in our silo? We need to omit such results from our packer, and trace log this.
                                    // ANOTHER ISSUE: For various propeties that are not explicitly called out in the packer silo property filter values, we may still need to add additional filtering, because we still need a way to ensure that games from a certain pingsite do not get omitted simply because games from another pingsite are more abundant! if the player accepts multiple pingsites we 'probably' should be making an effort to return back a list of games from *ALL* the pingsites, regardless of how small the GB list capacity is...
                                    auto& scoringInfo = newPackerSession.mImmutableScoringInfoByFactor[factor.mFactorIndex];
                                    if (propIter->second->getValue().getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
                                    {
                                        // Ex. uedValues
                                        // If this is a list, we assume the value can be converted to a single int64:
                                        EA::TDF::TdfGenericReferenceConst sourceRef;
                                        propIter->second->getValue().asList().getValueByIndex((size_t)userIndex, sourceRef);
                                        EA_ASSERT_MSG(sourceRef.isValid(), "Property values must be filled for all the players!");

                                        // PACKER_TODO: For now we stuff all the data from the property into the scoring value map, but for efficiency we should ideally be querying the GB to fetch only the fields the packer silo is currently operating on, this would ensure that the data we are pulling into the packer is limited only to a set of 'allowlisted' fields (e.g: ued values being used for packing, or player latency data related to the pingsite of the DS game), rather than pulling in all the values and trimming the set inside the packer silo(which is what happens now).
                                        EA::TDF::TdfGenericReference ref(scoringInfo.mScoringValueMap);
                                        if (!sourceRef.convertToResult(ref))
                                        {
                                            ERR_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Failed to convert property(" << outputPropertyName << ") value for active game(" << incomingGameId << ").");
                                            continue;
                                        }
                                    }
                                    else
                                    {
                                        // ex. game.pingsite
                                        // If this is not a list, we assume that the value can be converted to a String:
                                        auto& key = scoringInfo.mKeys.push_back();
                                        if (!propIter->second->convertToResult(key))
                                        {
                                            ERR_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: Failed to convert property(" << outputPropertyName <<") value for active game(" << incomingGameId << ").");
                                            continue;
                                        }
                                    }   
                                }
                            }
                            else
                            {
                                EA_ASSERT_MSG(ret.first->second->mImmutableTeamIndex == curTeamIndex, "Team index must match!");
                            }
                            ret.first->second->mUserSessionIds.push_back(curSessionId); // PackerSession takes a permanent ref to UserSessionInfo object
                            ++numActivePlayers;
                        } // for playerData in gameData.roster

                        for (auto& immutableSessionItr : immutableSessionByGroupMap)
                        {
                            auto& immutableSession = immutableSessionItr.second;
                            auto immutablePackerSessionId = immutableSession->mPackerSessionId;
                            if (!addSessionToPackerSilo(immutablePackerSessionId, packerSiloId))
                            {
                                EA_FAIL_MSG("Failed to add immutable session to packer silo!");
                            }
                            mPlaceholderSessionsAcquired.increment(1, gameTemplate.mGameTemplateName.c_str());
                            mPlaceholderSessionsActive.increment(1, gameTemplate.mGameTemplateName.c_str());
                            // PACKER_TODO: We would like to do at least a soft check to make sure this game is actually viable :), not required but nice to know, a full score breakdown with all metrics would be even better. This way we can tell the quality of the games we are getting from the search slaves...
                        }

                        if (numActivePlayers < gameCapacity)
                            openSeatsInActiveGames += gameCapacity - numActivePlayers;
                    }
                    else
                    {
                        // game is already tracked in the packer silo
                        TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") matched(known) active game(" << incomingGameId << ")");
                        if (isNewIncomingGame)
                        {
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") game query(" << gameListId << ") matched(" << incomingListType << ") active game(" << incomingGameId << "), expected(updated)");
                        }

                        // first check that the version of the game is actually changed, otherwise nothing to do
                        uint64_t existingGameVersion = 0;
                        // IMPORTANT: Deliberatey do not iterate using for ( ... : packerGame->mGameParties) directly, because it can include extra elements that get tegmporarily assigned during
                        // packing evaluation, all elements < partyCount are guaranteed to always be valid however.
                        const auto partyCount = packerGame->getPartyCount();
                        for (auto gamePartyItr = packerGame->mGameParties.begin(), gamePartyEnd = gamePartyItr + partyCount; gamePartyItr != gamePartyEnd; ++gamePartyItr)
                        {
                            // NOTE: Performance clarification, due to the specifics of packer library implementation immutable parties are always stored before mutable ones; therefore, this loop should never perform more than one step.
                            if (gamePartyItr->mImmutable)
                            {
                                auto existingImmutablePackerSessionItr = mPackerSessions.find(gamePartyItr->mPartyId);
                                EA_ASSERT_MSG(existingImmutablePackerSessionItr != mPackerSessions.end(), "Session must exist if the party is present!");
                                existingGameVersion = existingImmutablePackerSessionItr->second->mImmutableGameVersion;
                                break;
                            }
                        }
                        EA_ASSERT_MSG(existingGameVersion != 0, "Existing game version cannot be 0!");

                        if (existingGameVersion == gameRecordVersion)
                        {
                            // This is a warning because we don't expect notifications for games that are already known, may indicate inefficiencies...
                            WARN_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: known active game(" << incomingGameId << ") recordVersion(" << existingGameVersion << ") unchanged, skipping game.");
                            continue;
                        }

                        TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: known active game(" << incomingGameId << ") recordVersion(" << existingGameVersion << ") older than new recordVersion(" << gameRecordVersion << ") in update, repacking game.");

                        --matchedGameItr; // back up the iterator so that we can repack the game again

                        unpackedPlayersBelongingToMutableParties += unpackGameAndCleanupPlaceholderSessions(packerGame, GameManager::WORKER_SESSION_ACTIVE_GAME_UPDATED);

                        packerGame = nullptr; // invalidate to avoid accidental access
                    } // ... else if (gameId not in packerSilo)
                } // foreach matchedGame in matchedGameList
            } // foreach matchedGameList in matchedGameLists { new, updated }

            if (!packerSilo->hasPendingParties())
            {
                // This avoids churn in case the update produced no actionable changes for this silo, so skip repacking it to avoid unnecessary churn, and to avoid unintentional refreshing the improvement cooldown timer. The latter could unnecessarily delay session migration.
                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: game query(" << gameListId << ") produced no actionable changes in PackerSilo(" << packerSiloId << "), games(" << packerSilo->getGames().size() << "), parties(" << packerSilo->getParties().size() << "), skip");
                continue;
            }

            // Now unpack some parties so that newly vacated seats (due to added games or updated games) can be filled. NOTE: We should account for any parties/players already added to the pending queue due to games being removed from the potential set. Algoithm: walk games in ascending order of score, and unpack them while tracking the number of players that are freed up by this pass. Stop once the number of freed players exceeds the number of newly vacated seats in this silo.

            // NOTE: This algorithm is approximate and best effort. It does not currently guarantee that the parties being unpacked would necessarily fit the open seats in active games. This can happen when for example when the open game seats do not match the capacity requirement of the party being unpacked (game has spot for 1 player but party being unpacked has 2 players, etc.) For a more complete solution we would need a much more elaborate evaluation of options, and the ROI on this needs to be considered.
            
            // PACKER_TODO: #PERFORMANCE could be a large number of items potentially so we would like to keep a configurable cap on this, to avoid stalling this process too much, since non-viable games can still improve via create flow.
            for (auto packerGameItr = packerSilo->getGameByScoreSet().begin(); packerGameItr != packerSilo->getGameByScoreSet().end(); )
            {
                if (unpackedPlayersBelongingToMutableParties >= openSeatsInActiveGames)
                {
                    TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: PackerSilo(" << packerSiloId << ") unpacked(" << unpackedPlayersBelongingToMutableParties << ") as candidates to fill(" << openSeatsInActiveGames << ") open seats in active games");
                    break; // unpacked *sufficient* number of players to potentially fill all the open seats in active games
                }

                auto& packerGame = **packerGameItr;

                ++packerGameItr; // advance iterator before destructive action (unpackAndRequeueParties())

                auto gameId = packerGame.mGameId;
                if (!isProvisionalGameId(gameId))
                {
                    if (packerSilo->isIdealGame(packerGame))
                    {
                        TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: skip unpacking ideal active game(" << gameId << ")");
                        continue;
                    }
                }

                const auto packerGamePartyCount = packerGame.getPartyCount(Packer::GameState::PRESENT);
                uint32_t unpackedMutablePlayers = 0;
                StringBuilder sb;
                for (auto partyIndex = 0; partyIndex < packerGamePartyCount; ++partyIndex)
                {
                    auto& existingGameParty = packerGame.mGameParties[partyIndex];
                    const auto* partyType = "immutable";
                    if (!existingGameParty.mImmutable)
                    {
                        partyType = "mutable";
                        unpackedMutablePlayers += existingGameParty.mPlayerCount;
                    }
                    if (IS_LOGGING_ENABLED(Logging::TRACE))
                    {
                        sb << "\n" "unpack " << partyType << " party(" << existingGameParty.mPartyId << ")";
                    }
                } // foreach party in game to be unpacked

                if (unpackedMutablePlayers == 0)
                {
                    // PACKER_TODO: #PERFORMANCE We don't want to iterate games that don't have any mutable parties at all!
                    TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: skip unpacking game(" << gameId << "), contains only immutable parties -->" << sb);
                    continue; // skip games that have only immutable parties, since they cant change spaces...
                }

                TRACE_LOG("[GamePackerSlaveImpl].onNotifyGameListUpdate: unpack game(" << gameId << ") and requeue parties(" << packerGame.getPartyCount() << "), players(" << packerGame.getPlayerCount() << ") -->" << sb);

                unpackedPlayersBelongingToMutableParties += unpackedMutablePlayers;

                packerSilo->unpackGameAndRequeueParties(gameId); // NOTE: This does not remove parties, only unassign them from their games and requeue them
            }

            packGames(packerSiloId); // finally its time to pack all the games in one pass

        } // foreach packerSilo in activeGameQuery.mInitiatorSiloIdSet
    } // foreach listUpdate in gameListUpdates
}

GetDetailedMetricsError::Error GamePackerSlaveImpl::processGetDetailedMetrics(Blaze::GameManager::GetDetailedPackerMetricsResponse &response, const ::Blaze::Message* message)
{
    getDetailedPackerMetrics(response);
    return GetDetailedMetricsError::ERR_OK;
}

EA::TDF::TimeValue GamePackerSlaveImpl::getTimeBetweenWorkSlotClaims() const
{
    auto& workerSettings = getConfig().getWorkerSettings();
    TimeValue timeBetweenClaims = workerSettings.getMaxSlotClaimDuration() / 2;
    if (timeBetweenClaims < workerSettings.getMinTimeBetweenSlotClaims())
        timeBetweenClaims = workerSettings.getMinTimeBetweenSlotClaims(); // clamp to min
    return timeBetweenClaims;
}

void GamePackerSlaveImpl::onSubscribedForNotificationsForMaster(InstanceId instanceId)
{
    auto* instance = gController->getRemoteServerInstance(instanceId);
    if (instance == nullptr)
    {
        // bad news bears
        FAIL_LOG("[GamePackerSlaveImpl].onSubscribedForNotificationsForMaster: Remote instance(" << instanceId << ") must exist!");
        return;
    }

    auto ret = mSlotClaimByInstanceMap.insert(instanceId);
    auto& claim = ret.first->second;
    if (ret.second)
    {
        INFO_LOG("[GamePackerSlaveImpl].onSubscribedForNotificationsForMaster: Inserted new pending work slot claim for master(" << instance->getInstanceName() << "), registrationId(" << instance->getRegistrationId() << ")");
    }
    else
    {
        INFO_LOG("[GamePackerSlaveImpl].onSubscribedForNotificationsForMaster: Reset work slot claim(" << claim.mSlotId << "), version(" << claim.mSlotVersion << "), for master(" << instance->getInstanceName() << "), registrationId(" << claim.mInstanceRegistrationId << "->" << instance->getRegistrationId() << ")");
        claim.mSlotId = 0;
        claim.mSlotVersion = 0;
    }
    // always update identifying information
    claim.mInstanceRegistrationId = instance->getRegistrationId();
    claim.mInstanceName = instance->getInstanceName();

    scheduleClaimWorkSlotIdAndNotifyMasters(TimeValue::getTimeOfDay());
}

void GamePackerSlaveImpl::scheduleClaimWorkSlotIdAndNotifyMasters(EA::TDF::TimeValue newDeadline)
{
    if (gController->isShuttingDown())
    {
        SPAM_LOG("[GamePackerSlaveImpl].scheduleClaimWorkSlotIdAndNotifyMasters: No-op on shutdown");
        return;
    }

    const bool shortenDeadline = (mWorkSlotUpdateDeadline == 0) || (newDeadline < mWorkSlotUpdateDeadline);
    if (shortenDeadline)
    {
        mWorkSlotUpdateDeadline = newDeadline;
        if (mWorkSlotUpdateTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mWorkSlotUpdateTimerId);
            mWorkSlotUpdateTimerId = INVALID_TIMER_ID;
        }
        else if (Fiber::isFiberIdInUse(mWorkSlotUpdateFiberId))
        {
            return; // fiber already running
        }
    }
    else if ((mWorkSlotUpdateTimerId != INVALID_TIMER_ID) || Fiber::isFiberIdInUse(mWorkSlotUpdateFiberId))
    {
        return; // timer with earlier claim already pending or claim fiber already running
    }
    else if (mWorkSlotUpdateFiberId != Fiber::INVALID_FIBER_ID)
    {
        // This should only be seen if something extraordinary happens(e.g: fiber exception in claimWorkSlotIdAndNotifyMasters())
        WARN_LOG("[GamePackerSlaveImpl].scheduleClaimWorkSlotIdAndNotifyMasters: Previous claim stalled on fiber(" << mWorkSlotUpdateFiberId << "), forcing new claim at(" << mWorkSlotUpdateDeadline << ")");
    }

    // NOTE: If we waited longer than the claim duration that means that our claim has expired anyway, might as well time out the whole thing
    Fiber::CreateParams fiberParams(Fiber::STACK_LARGE);
    mWorkSlotUpdateTimerId = gSelector->scheduleFiberTimerCall(mWorkSlotUpdateDeadline, this, &GamePackerSlaveImpl::claimWorkSlotIdAndNotifyMasters, "GamePackerSlaveImpl::claimWorkSlotIdAndNotifyMasters", fiberParams);
}

void GamePackerSlaveImpl::claimWorkSlotIdAndNotifyMasters()
{
    // IMPORTANT: We reset the timer id and deadline because we want to allow mWorkSlotUpdateDeadline to be refreshed if any urgent operation requires it while this fiber is performing blocking operations. We use the fiber id to ensure that such operations are aware of the pending work and do not attempt to take over rescheduling another claim fiber while this one is still pending, the fiber id also serves as a fallback in case this fiber were to get frozen by an exception, since it enables such operations to reschedule the claim fiber.
    mWorkSlotUpdateFiberId = Fiber::getCurrentFiberId();
    mWorkSlotUpdateTimerId = INVALID_TIMER_ID;
    mWorkSlotUpdateDeadline = 0;

    auto timeBetweenClaims = getTimeBetweenWorkSlotClaims();
    auto maxClaimDuration = getConfig().getWorkerSettings().getMaxSlotClaimDuration();
    auto startClaimTime = TimeValue::getTimeOfDay();
    RedisCluster* cluster = gRedisManager->getRedisCluster(getComponentInfo().baseInfo.name);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        RedisResponsePtr resp;
        cmd.addKey("{gamepacker.workers}"); // needed to ensure that the lua script is confined to as specific redis instance and to ensure naming. PACKER_TODO: Use a registered redis collection to ensure that we are allowed to use this key...
        cmd.add(gController->getInstanceId());
        cmd.add(maxClaimDuration.getSec());
        RedisError rc = cluster->execScript(sAcquireWorkerSlotScript, cmd, resp);
        if (rc == REDIS_ERR_OK)
        {
            auto size = resp->getArraySize();
            EA_ASSERT(size == CLAIM_RESULT_MAX);
            mAssignedWorkSlotId = (uint32_t)resp->getArrayElement(CLAIM_RESULT_SLOT_ID).getInteger();
            auto previousSlotId = (uint32_t)resp->getArrayElement(CLAIM_RESULT_PREV_SLOT_ID).getInteger();
            mAssignedWorkSlotVersion = (uint64_t)resp->getArrayElement(CLAIM_RESULT_SLOT_VERSION_ID).getInteger();
            auto newSlotCount = (uint32_t)resp->getArrayElement(CLAIM_RESULT_SLOT_COUNT).getInteger();
            // TODO: If newSlotCount changes enough that the number of layers changes as well, we'll need to update the PackerSession::mRelinquishTimerId because it was computed based on the number of worker layers.
            mWorkSlotCount = newSlotCount;
            SPAM_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Obtained slotId(" << mAssignedWorkSlotId << "), version(" << mAssignedWorkSlotVersion << "), prevSlotId(" << previousSlotId << ")");
        }
        else
        {
            WARN_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Unable to claim slotId from redis, error: " << RedisErrorHelper::getName(rc) << "; will retry");
        }
    }
    else
    {
        EA_FAIL_MSG("Must be able to talk to redis!"); // TODO: add logging
    }

    Component::InstanceIdList refreshInstanceIdList; // set of instances that actually need to be refreshed
    refreshInstanceIdList.reserve(mSlotClaimByInstanceMap.size());

    // sweep any slot claims that refer to non-existent instances
    for (auto slotClaimItr = mSlotClaimByInstanceMap.begin(); slotClaimItr != mSlotClaimByInstanceMap.end(); )
    {
        auto instanceId = slotClaimItr->first;
        auto& claimInfo = slotClaimItr->second;
        auto* remoteInstance = gController->getRemoteServerInstance(instanceId);
        if (remoteInstance == nullptr)
        {
            // This can happen if the instance went away between calls to claimWorkSlotIdAndNotifyMasters(), this stale entry is normal expected behavior that has no side effects since we only use mSlotClaimByInstanceMap within this procedure.
            TRACE_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Remove stale slotId(" << claimInfo.mSlotId << ") claim info for disconnected master(" << claimInfo.mInstanceName << ")");
            slotClaimItr = mSlotClaimByInstanceMap.erase(slotClaimItr);
            continue;
        }
        EA_ASSERT_MSG(remoteInstance->getRegistrationId() == claimInfo.mInstanceRegistrationId, "registration id is not allowed to mutate!");
        if (claimInfo.mSlotId != mAssignedWorkSlotId)
        {
            INFO_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Refresh slotId(" << claimInfo.mSlotId << ") to slotId(" << mAssignedWorkSlotId << ") sending claim info to connected master(" << claimInfo.mInstanceName << ")"); // MAYBE: change to trace? This should not happen often though...
            refreshInstanceIdList.push_back(instanceId);
        }
        ++slotClaimItr;
    }

    if (!refreshInstanceIdList.empty())
    {
        Blaze::GameManager::WorkerLayerAssignmentRequest req;
        eastl::random_shuffle(refreshInstanceIdList.begin(), refreshInstanceIdList.end(), Random::getRandomNumber); // randomize calls to avoid prioritizing a single instance
        Component::MultiRequestResponseHelper<Blaze::GameManager::WorkerLayerAssignmentResponse, void> responses(refreshInstanceIdList, false, GamePackerSlave::getMemGroupId(), "WorkerLayerAssignmentResponse::multiResponse");
        // IMPORTANT: We have the slave send up only the slotid and have the master assign it the layerid via a pure deterministic function, that means that the layers are defined globally, this does mean that a specific master may temporarily only have comms with slaves on certain layers; however, this ensures a globally consistent view of the worker slave hierarchy from the perspective of each master.
        req.setLayerSlotId(mAssignedWorkSlotId);
        req.setLayerSlotAssignmentVersion(mAssignedWorkSlotVersion);
        req.setInstanceName(gController->getInstanceName());
        req.setInstanceTypeName(gController->getBaseName());
        auto err = getMaster()->sendMultiRequest(Blaze::GamePacker::GamePackerMaster::CMD_INFO_WORKERCLAIMLAYERASSIGNMENT, &req, responses);
        if (err == ERR_OK)
        {
            for (auto& respItr : responses)
            {
                if (respItr->error == ERR_OK)
                {
                    auto& resp = static_cast<Blaze::GameManager::WorkerLayerAssignmentResponse&>(*respItr->response);
                    if (mAssignedWorkSlotVersion == resp.getLayerSlotAssignmentVersion())
                    {
                        auto slotClaimItr = mSlotClaimByInstanceMap.find(respItr->instanceId);
                        if (slotClaimItr != mSlotClaimByInstanceMap.end())
                        {
                            auto& claimInfo = slotClaimItr->second;
                            claimInfo.mSlotId = mAssignedWorkSlotId;
                            claimInfo.mSlotVersion = mAssignedWorkSlotVersion;
                        }
                        else
                        {
                            ASSERT_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Claimed slotId(" << mAssignedWorkSlotId << ") version(" << mAssignedWorkSlotVersion << "), master(" << respItr->instanceId << "), but instance mapping unexpectedly mutated!");
                        }
                    }
                    else
                    {
                        // The only way we can get here is if an instance that claimed a slotid assignment for itself has gotten aged out of redis before the master could process its workerLayerAssignment, thereby giving another instance a chance to acquire/claim the same slotid (with a new version number of course).
                        auto slotClaimItr = mSlotClaimByInstanceMap.find(respItr->instanceId);
                        if (slotClaimItr != mSlotClaimByInstanceMap.end())
                        {
                            INFO_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Claimed existing slotId(" << mAssignedWorkSlotId << ") version(" << mAssignedWorkSlotVersion << ") out of date, master(" << slotClaimItr->second.mInstanceName << ") version(" << resp.getLayerSlotAssignmentVersion() << ") triggering immediate slot refresh");
                            mSlotClaimByInstanceMap.erase(slotClaimItr);
                        }
                        else
                        {
                            INFO_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Claimed slotId(" << mAssignedWorkSlotId << ") version(" << mAssignedWorkSlotVersion << ") out of date, master(" << respItr->instanceId << ") version(" << resp.getLayerSlotAssignmentVersion() << ") triggering immediate slot refresh");
                        }
                        timeBetweenClaims = getConfig().getWorkerSettings().getMinTimeBetweenSlotClaims(); // speed up claim since there is no point in waiting as we know our claim has been superceded and is now out of date
                    }
                }
                else
                {
                    //log all instance's errors for debugging network etc
                    auto slotClaimItr = mSlotClaimByInstanceMap.find(respItr->instanceId);
                    if (slotClaimItr != mSlotClaimByInstanceMap.end())
                    {
                        WARN_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Failed to claim existing slotId(" << mAssignedWorkSlotId << ") for master(" << slotClaimItr->second.mInstanceName << "), error(" << ErrorHelp::getErrorName(respItr->error) << "), will retry");
                        mSlotClaimByInstanceMap.erase(slotClaimItr);
                    }
                    else
                    {
                        WARN_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Failed to claim slotId(" << mAssignedWorkSlotId << ") for master(" << respItr->instanceId << "), error(" << ErrorHelp::getErrorName(respItr->error) << "), will retry");
                    }
                }
            }
        }
        else
        {
            ERR_LOG("[GamePackerSlaveImpl].claimWorkSlotIdAndNotifyMasters: Failed sending multi-request to claim slotId(" << mAssignedWorkSlotId << "), error(" << ErrorHelp::getErrorName(err) << "), will retry");
        }
    }

    mWorkSlotUpdateFiberId = Fiber::INVALID_FIBER_ID;
    scheduleClaimWorkSlotIdAndNotifyMasters(startClaimTime + timeBetweenClaims);
}

void GamePackerSlaveImpl::scheduleObtainSession(InstanceId instanceId)
{
    auto ret = mFetcherFiberByInstanceMap.insert(instanceId);
    if (!ret.second)
    {
        if (Fiber::isFiberIdInUse(ret.first->second))
        {
            // already operating on this instance
            TRACE_LOG("[GamePackerSlaveImpl].scheduleObtainSession: Requests already pending for master(" << getInstanceName(instanceId) << ") on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << ")");
            return;
        }
    }
    Fiber::CreateParams fiberParams(Fiber::STACK_LARGE);  // PACKER_TODO:  Remove getConfig().getWorkerSettings().getMaxSlotClaimDuration().
    gSelector->scheduleFiberCall(this, &GamePackerSlaveImpl::obtainSession, instanceId, "GamePackerSlaveImpl::obtainSession", fiberParams);
}

uint32_t GamePackerSlaveImpl::getWorkerLayerCount() const
{
    // PACKER_TODO: this code is a duplicate of the code in uint32_t MatchmakerMasterImpl::LayerInfo::layerIdFromSlotId(uint32_t slotId), factor out those data structures and share them...
    return getLayerIdFromSlotId(mWorkSlotCount);
}

/**  This worker fiber goes as fast as it can fetching data from instance until the instance signals it has no available work or, until we receive an error.*/
void GamePackerSlaveImpl::obtainSession(InstanceId instanceId)
{
    if (gController->isShuttingDown() || gController->isDraining())
        return;

    auto fiberId = Fiber::getCurrentFiberId();
    mFetcherFiberByInstanceMap[instanceId] = fiberId;

    RpcCallOptions callOpts;
    callOpts.routeTo.setInstanceId(instanceId);
    Blaze::GameManager::WorkerObtainPackerSessionRequest request;
    Blaze::GameManager::WorkerObtainPackerSessionResponse response;
    BlazeError rc = getMaster()->workerObtainPackerSession(request, response, callOpts);
    auto acquireMoreWork = false;
    if (rc == ERR_OK)
    {
        auto packerSessionId = response.getPackerSessionId();
        if (packerSessionId != 0)
        {
            EA_ASSERT(!isPlaceholderSessionId(packerSessionId));
            acquireMoreWork = true; // if we successful acquired a session from the master we always try again, because it might have more work.

            // PACKER_TODO: needed cast until we remove dependency on StartMatchmakingInternalRequest
            auto& mmInternalRequest = *static_cast<GameManager::StartMatchmakingInternalRequest*>(response.getInternalRequest());
            EA_ASSERT(mmInternalRequest.getTdfId() == GameManager::StartMatchmakingInternalRequest::TDF_ID);
            EA_ASSERT(!mmInternalRequest.getUsersInfo().empty());
            LogContextOverride logAudit(mmInternalRequest.getOwnerUserSessionInfo().getSessionId());

            auto& scenarioInfo = mmInternalRequest.getUsersInfo().front()->getScenarioInfo();
            mSessionsAcquired.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
            const auto originatingScenarioId = mmInternalRequest.getUsersInfo().front()->getOriginatingScenarioId();
            const auto* createGameTemplateName = mmInternalRequest.getCreateGameTemplateName();
            const auto gameTemplateItr = mGameTemplateMap.find(createGameTemplateName);
            if (gameTemplateItr != mGameTemplateMap.end())
            {
                auto& gameTemplate = *gameTemplateItr->second;
                const auto now = TimeValue::getTimeOfDay();
                const auto minTTL = gameTemplate.mCreateGameTemplateConfig->getPackerConfig().getMinTimeToLiveThreshold();
                const auto cutoff = now + minTTL;
                if (response.getExpiryDeadline() >= cutoff)
                {
                    const auto* propertyRuleDef = getPropertyRuleDefinition();
                    if (propertyRuleDef != nullptr)
                    {
                        auto& packerSession = *(BLAZE_NEW PackerSession(gameTemplate));
                        packerSession.mMmInternalRequest = &mmInternalRequest; // packer session needs to own the mmInternalRequest (needed for calling createGame on the GM master)
                        packerSession.mPackerSessionId = packerSessionId;
                        packerSession.mSessionStartTime = now;
                        // PACKER_TODO: Validate we correctly handle sessions that expire before scenario expiry
                        packerSession.mSessionExpirationTime = response.getExpiryDeadline();
                        packerSession.mUserSessionIds.reserve(mmInternalRequest.getUsersInfo().size());
                        for (auto& joinInfoItr : mmInternalRequest.getUsersInfo())
                        {
                            packerSession.mUserSessionIds.push_back(joinInfoItr->getUser().getSessionId());
                        }

                        GameManager::Matchmaker::PropertyRule* propertyRule = (GameManager::Matchmaker::PropertyRule*)propertyRuleDef->createRule(nullptr);
                        EA_ASSERT_MSG(propertyRule != nullptr, "Property rule must exist!");
                        packerSession.mReteRules.push_back((GameManager::Rete::ReteRule*)propertyRule);
                        GameManager::MatchmakingCriteriaError criteriaError;
                        // NOTE: This just copies the contents of the MatchmakingFilterCriteriaMap into the PropertyRule and enables packerSession.initializeReteProductions() to generate search conditions
                        if (propertyRule->initializeInternal(packerSession.getFilterMap(), criteriaError, true))
                        {
                            mSessionsActive.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
                            mPackerSessions[packerSessionId] = &packerSession;
                            packerSession.mParentMetricsSnapshot.mTotalNewPartyCount = gameTemplate.mMetrics.mTotalNewPartyCount++;
                            TRACE_LOG("[GamePackerSlaveImpl].obtainSession: Started session(" << packerSessionId << "), scenario(" << originatingScenarioId << "), on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << ")");
                            // NOTE: This is where the PropertyRule.addConditions() uses the game property filters(specific to this session) to generate condition=value pairs that define the search criteria that this session expects to match.
                            packerSession.initializeReteProductions(*mReteNetwork, this);
                            // NOTE: This is where the PropertyRuleDefinition::insertWorkingMemoryElements() should iterate *ALL* filters referenced by *ANY* scenario configs in order to build a union of all relevant game properties. Then it should extract the corresponding game properties active on the current session and populate the resulting WME attribute=value pairs into RETE.
                            packerSession.addToReteIndex(); 
                        }
                        else
                        {
                            ERR_LOG("[GamePackerSlaveImpl].obtainSession: Failed to start session(" << packerSessionId << "), scenario(" << originatingScenarioId << ") on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << "), failed to initialize matchmaking criteria: " << criteriaError.getErrMessage());
                            cleanupSession(packerSessionId, GameManager::WORKER_SESSION_CREATE_FAILED);
                            abortSession(packerSessionId, originatingScenarioId);
                            mSessionsAborted.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
                        }
                    }
                    else
                    {
                        ERR_LOG("[GamePackerSlaveImpl].obtainSession: Failed obtain property rule definition for session(" << packerSessionId << "), scenario(" << originatingScenarioId << ")");
                        abortSession(packerSessionId, originatingScenarioId);
                        mSessionsAborted.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
                        // PACKER_TODO: handle this error below by returning the session to the master.
                    }
                }
                else
                {
                    mSessionsAborted.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
                    WARN_LOG("[GamePackerSlaveImpl].obtainSession: session(" << packerSessionId << ") ttl(" << (now - response.getExpiryDeadline()).getMillis() << "ms) below threshold(" << minTTL.getMicroSeconds() << "), scenario(" << originatingScenarioId << "), on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << "), refusing session back to owner.");
                    abortSession(packerSessionId, originatingScenarioId);
                }
            }
            else
            {
                mSessionsAborted.increment(1, scenarioInfo.getScenarioName(), scenarioInfo.getSubSessionName());
                ERR_LOG("[GamePackerSlaveImpl].obtainSession: Failed to create session(" << packerSessionId << "), from owner(" << getInstanceName(instanceId) << "), unknown gameTemplate(" << createGameTemplateName << ")");
                abortSession(packerSessionId, originatingScenarioId);
            }
        }
        else
        {
            mSessionsAcquireNone.increment();
            TRACE_LOG("[GamePackerSlaveImpl].obtainSession: Received EOF from owner(" << getInstanceName(instanceId) << "), suspend fetching work until next availability signal received");
            // no more work to do on this instance
        }
    }
    else
    {
        mSessionsAcquireFailed.increment();
        ERR_LOG("[GamePackerSlaveImpl].obtainSession: Failed to aquire work from owner(" << getInstanceName(instanceId) << ") on layer(" << getLayerIdFromSlotId(mAssignedWorkSlotId) << "), error(" << rc << "), suspend fetching from owner until next availability signal received");
    }
    mFetcherFiberByInstanceMap.erase(instanceId);

    if (acquireMoreWork)
    {
        scheduleObtainSession(instanceId);
    }
}

bool GamePackerSlaveImpl::addSessionToPackerSilo(PackerSessionId packerSessionId, PackerSiloId packerSiloId)
{
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    if (packerSessionItr == mPackerSessions.end())
    {
        EA_FAIL(); // should never happen!
        return false;
    }

    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    bool immutable = !isProvisionalGameId(packerSession.mImmutableGameId);
    auto* gameTemplateName = packerSession.mGameTemplate.mGameTemplateName.c_str();
    auto& gameTemplates = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates();
    const auto createGameTemplateItr = gameTemplates.find(gameTemplateName);
    if (createGameTemplateItr == gameTemplates.end())
    {
        //ASSERT_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: internal error: Game Packer MM sessions require a valid configured scenario w/createGameTemplate. Can't find in create game templates config, template name(" << gameTemplateName << "), specified for scenario(" << mmSession->getScenarioInfo().getScenarioName() << "), subsession(" << mmSession->getScenarioInfo().getSubSessionName() << "). Unable to submit to packer, MMsession(" << mmSession->getMMSessionId() << "), MMscenarioId(" << mmSession->getMMScenarioId() << ")");
        EA_FAIL(); // PACKER_TODO: remove, should never happen!
        return false;
    }
    auto* packerSilo = getPackerSilo(packerSiloId);
    if (packerSilo == nullptr)
    {
        ASSERT_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: internal error: unexpected state, no packer silo. Unable to submit to packer");
        return false;
    }

    auto* existingParty = packerSilo->getPackerParty(packerSessionId);
    if (existingParty != nullptr)
    {
        EA_ASSERT(packerSession.mMatchedPackerSiloIdSet.find(packerSiloId) != packerSession.mMatchedPackerSiloIdSet.end());
        TRACE_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: PackerSilo(" << packerSiloId << ") already contains party(" << packerSessionId << "), skipping");
        return false;
    }

    auto playerIdListIter = packerSession.getPropertyMap().find(GameManager::PropertyManager::PROPERTY_PLAYER_PARTICIPANT_IDS);  // Need to indicate that this value is always required.   PACKER_TODO - Add to the code that builds the required Filters/Packer properties. 
    Blaze::GameManager::PlayerIdList playerIdList;
    if (playerIdListIter == packerSession.getPropertyMap().end() || !playerIdListIter->second->convertToResult(playerIdList))
    {
        ERR_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: PackerSilo(" << packerSiloId << ") missing 'player.participantIds' or incorrectly set.  This value should always be set!");
        EA_FAIL(); // PACKER_TODO - remove
        return false;
    }

    EA_ASSERT(!packerSilo->isPacking() && !packerSilo->isReaping());

    auto& packer = *packerSilo;
    auto partyId = packerSessionId;
    TimeValue creationTime = TimeValue::getTimeOfDay(); // PACKER_TODO: we may want to reuse creation time of the packer session, instead of updating the creation time of the party, but its not used by the packer lib right now so its ok...

    packerSession.mMatchedPackerSiloIdSet.insert(packerSiloId); // associate session with silo
    // PACKER_TODO: if we did not insert party into silo here, that means its already associated with the silo, this can happen when a party is removed from the silo (while its locked for finalization, and then readded back)
    // we should make this behavior a bit more clean

    GameManager::PackerSiloPartyAdded partyAddedToPackerSilo;
    if (IS_LOGGING_ENABLED(Logging::TRACE))
    {
        partyAddedToPackerSilo.setAddedPartyId(partyId);
        partyAddedToPackerSilo.setPackerSiloId(packer.getPackerSiloId());
        // NOTE: These values need to be set before packer.addPackerToParty() because they take a snapshot of the packer silo state before the new party is added
        partyAddedToPackerSilo.setPackerSiloGames(packer.getGames().size());
        partyAddedToPackerSilo.setPackerSiloParties(packer.getParties().size());
        partyAddedToPackerSilo.setPackerSiloPlayers(packer.getPlayers().size());
        partyAddedToPackerSilo.setPackerSiloViableGames(packer.getViableGameQueue().size());
        // PACKER_TODO: Add setReasonAdded(). We can set it
    }

    packer.addPackerParty(partyId, creationTime.getMicroSeconds(), packerSession.mSessionExpirationTime.getMicroSeconds(), immutable);
    auto& party = *packer.getPackerParty(partyId);
    if (immutable)
    {
        party.mGameId = packerSession.mImmutableGameId;
        party.mFaction = packerSession.mImmutableTeamIndex;
    }
    else
    {
        // PACKER_TODO: Currently we only unmark the silo for removal in case we added a mutable party, but this opens us to the possibility that a set of active games added to the silo would cause the silo to be removed immediately after due to not having any mutable parties in them yet. This may or may not be ideal... Need to investigate.
        unmarkPackerSiloForRemoval(packerSiloId);
    }

    // Add all the Players that will be required:  (We'll fill in their Properties below)
    for (const GameManager::PlayerId& participantId : playerIdList)
    {
        Packer::PartyPlayerIndex playerPartyIndex = packer.addPackerPlayer((Packer::PlayerId)participantId, party);

        if (IS_LOGGING_ENABLED(Logging::TRACE))
        {
            auto& evPlayer = *partyAddedToPackerSilo.getPlayers().pull_back();      // TDF value that isn't actually used by Packer internally, but duplicates the data so Blaze can use it.    PACKER_TODO:  Unify data types.
            evPlayer.setPlayerId(participantId);
        }

        if (playerPartyIndex != (Packer::PartyPlayerIndex)(party.mPartyPlayers.size()-1))
        {
            ERR_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: Player Indexes do not match with expected order in party.");
            continue;
        }
    }

    const bool isPlaceholderSession = isPlaceholderSessionId(packerSessionId);
    const auto& packerConfig = createGameTemplateItr->second->getPackerConfig();

    // We rely on the mapping of the config to the packer quality factors to be 1 to 1.  
    EA_ASSERT(packerConfig.getQualityFactors().size() == packer.getFactors().size());
    for (const auto& qualityFactor : packer.getFactors())
    {
        auto* boundPropertyDescriptor = qualityFactor.getInputPropertyDescriptor();
        if (boundPropertyDescriptor == nullptr)
            continue; // factor not using input properties (eg: size, teamsize)

        GameManager::ListString keys;
        GameManager::PropertyValueMapList scoringMapList; // Even if the entry is just a single value, it should still convert over automatically (value -> list[0]=value)

        if (isPlaceholderSession)
        {
            auto& immutableScoringInfo = packerSession.mImmutableScoringInfoByFactor[qualityFactor.mFactorIndex];
            immutableScoringInfo.mKeys.copyInto(keys);
            scoringMapList.push_back(&immutableScoringInfo.mScoringValueMap);
            EA_ASSERT(scoringMapList.empty() || party.getPlayerCount() == scoringMapList.size());
        }
        else
        {
            const auto& qualityFactorCfg = packerConfig.getQualityFactors()[qualityFactor.mFactorIndex];

            EA::TDF::TdfGenericReference keysRef(keys);
            const char8_t* outFailingAttribute = nullptr;
            BlazeRpcError err = applyTemplateAttributeDefinition(keysRef, qualityFactorCfg->getKeys(), packerSession.getAttributeMap(), packerSession.getPropertyMap(), outFailingAttribute, "Lookup Keys");
            if (err != ERR_OK)
            {
                // Failed to lookup keys (and keys was set)
                ERR_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: GQF(" << qualityFactor.mFactorName << ") expects keys configuration expects a value from property/attribute/default, missing property/attribute(" << outFailingAttribute << ").");
                continue;
            }

            EA::TDF::TdfGenericReference scoringMapListRef(scoringMapList);
            err = applyTemplateAttributeDefinition(scoringMapListRef, qualityFactorCfg->getScoringMap(), packerSession.getAttributeMap(), packerSession.getPropertyMap(), outFailingAttribute, "Lookup Scoring Map List");
            if (err != ERR_OK)
            {
                // Failed to lookup scoring map
                ERR_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: ScoringMap expect to set a value, but no value was provided.");
                continue;
            }
            EA_ASSERT(party.getPlayerCount() == scoringMapList.size());
        }

        for (Packer::PartyPlayerIndex partyPlayerIndex = 0; partyPlayerIndex < (Packer::PartyPlayerIndex)party.getPlayerCount(); ++partyPlayerIndex)
        {
            auto setPlayerFieldValueFunc = [&packer, &party, partyPlayerIndex, &boundPropertyDescriptor, &partyAddedToPackerSilo] (const char8_t* fieldName, const int64_t* fieldValue) {
                auto& fieldDescriptor = packer.addPropertyField(boundPropertyDescriptor->mPropertyName, fieldName);
                party.setPlayerPropertyValue(&fieldDescriptor, partyPlayerIndex, fieldValue);
                if (IS_LOGGING_ENABLED(Logging::TRACE))
                {
                    EA::TDF::TdfString fieldNameString; // use explicitly to avoid allocations/copies
                    fieldNameString.assignBuffer(fieldDescriptor.mFieldName.c_str()); // safe to avoid copying data, since field descriptors are immutable well past the end of this method
                    auto& evPlayer = *partyAddedToPackerSilo.getPlayers()[partyPlayerIndex];
                    if (fieldValue)
                        evPlayer.getProperties()[fieldNameString] = *fieldValue;
                    else
                        evPlayer.getProperties()[fieldNameString] = -1;
                }
            };

            auto& fieldValueMap = scoringMapList[partyPlayerIndex];
            if (keys.empty())
            {
                for (const auto& fieldMapItr : *fieldValueMap)
                    setPlayerFieldValueFunc(fieldMapItr.first, &fieldMapItr.second);
            }
            else
            {
                for (const auto& key : keys)
                {
                    auto fieldMapItr = fieldValueMap->find(key);
                    if (fieldMapItr != fieldValueMap->end())
                    {
                        setPlayerFieldValueFunc(fieldMapItr->first, &fieldMapItr->second);
                    }
                    else if (isPlaceholderSession)
                    {
                        setPlayerFieldValueFunc(key, nullptr);
                    }
                }
            }
        }
    }

    TRACE_LOG("[GamePackerSlaveImpl].addSessionToPackerSilo: " << partyAddedToPackerSilo); // output this before we pack since packing will add the party to a p-game

    return true;
}

void GamePackerSlaveImpl::removeSessionFromPackerSilo(PackerSession& packerSession, Packer::PackerSilo& packerSilo, const char8_t* removalReason)
{
    EA_ASSERT_MSG(!packerSilo.isReaping() && !packerSilo.isPacking(), "Party removal not allowed while packing/reaping!"); // only packer itself can remove parties at this point!
    auto partyId = packerSession.mPackerSessionId;
    auto matchedItr = packerSession.mMatchedPackerSiloIdSet.find(packerSilo.getPackerSiloId());
    EA_ASSERT(matchedItr != packerSession.mMatchedPackerSiloIdSet.end());
  
    auto* removedParty = packerSilo.getPackerParty(partyId);
    if (removedParty == nullptr)
    {
        TRACE_LOG("[GamePackerSlaveImpl].removeSessionFromPackerSilo: session(" << partyId << ") has no party in PackerSilo(" << packerSilo.getPackerSiloId() << "), removalReason(" << removalReason << ")"); // output this before we pack since packing will add the party to a p-game
        return;
    }

    packerSession.setRemoveFromSiloReason(removalReason); // PACKER_TODO: #DEBUGGING Get rid of this, or inline it

    // PACKER_TODO: add the game score of the game we were in when removed, eviction count, last update time, etc.
    GameManager::PackerSiloPartyRemoved partyRemovedFromPackerSilo;
    partyRemovedFromPackerSilo.setRemovedPartyId(partyId);
    partyRemovedFromPackerSilo.setPackerSiloId(packerSilo.getPackerSiloId());
    partyRemovedFromPackerSilo.setPackerSiloGames(packerSilo.getGames().size());
    partyRemovedFromPackerSilo.setPackerSiloParties(packerSilo.getParties().size());
    partyRemovedFromPackerSilo.setPackerSiloPlayers(packerSilo.getPlayers().size());
    partyRemovedFromPackerSilo.setPackerSiloViableGames(packerSilo.getViableGameQueue().size());
    partyRemovedFromPackerSilo.setPartyRemovalReason(removalReason);

    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    TRACE_LOG("[GamePackerSlaveImpl].removeSessionFromPackerSilo: " << partyRemovedFromPackerSilo); // output this before we pack since packing will add the party to a p-game

    // PACKER_TODO: Replace this with: unpackGame(), requeues everyone back (except specific session), add eraseGame(), must be empty.
    auto gameId = removedParty->mGameId;
    packerSilo.eraseParty(partyId);
    packerSilo.unpackGameAndRequeueParties(gameId);
}

void GamePackerSlaveImpl::packGames(PackerSiloId packerSiloId)
{
    auto* siloInfo = getPackerSiloInfo(packerSiloId);
    if (siloInfo == nullptr)
    {
        ERR_LOG("[GamePackerSlaveImpl].packGames: internal error: unexpected state, no packer silo. Unable to submit to packer");
        return;
    }

    TimeValue packingTime = siloInfo->mPackerSilo.packGames(this);

    // update per template metrics
    siloInfo->mMetrics.mTotalAddPartyPackingTime += packingTime.getMicroSeconds();
    mTotalPackingTime.increment(packingTime.getMicroSeconds(), siloInfo->mPackerSilo.getPackerName().c_str(), Fiber::getCurrentContext());

    scheduleReapGames(packerSiloId);
}

void GamePackerSlaveImpl::scheduleReapGames(PackerSiloId packerSiloId)
{
    auto* siloInfo = getPackerSiloInfo(packerSiloId);
    if (siloInfo == nullptr)
        return;

    EA::TDF::TimeValue deadline = siloInfo->mPackerSilo.getNextReapDeadline();
    if (deadline < 0)
    {
        EA_ASSERT(siloInfo->mPackerSilo.getGames().empty() && siloInfo->mPackerSilo.getParties().empty());
        auto& reapData = siloInfo->mReapData;
        if (reapData.mReapPackingsTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(reapData.mReapPackingsTimerId);
            reapData.mReapPackingsTimerId = INVALID_TIMER_ID;
        }
    }
    else
    {
        auto& reapData = siloInfo->mReapData;
        if (reapData.mReapPackingsTimerId == INVALID_TIMER_ID)
        {
            reapData.mNextReapDeadline = deadline;
        }
        else if (deadline != reapData.mNextReapDeadline)
        {
            gSelector->cancelTimer(reapData.mReapPackingsTimerId);
            reapData.mNextReapDeadline = deadline;
        }
        else
            return; // deadline is unchanged

        reapData.mReapPackingsTimerId = gSelector->scheduleFiberTimerCall(reapData.mNextReapDeadline, this, &GamePackerSlaveImpl::reapGames, packerSiloId, "GamePackerSlaveImpl::reapGames");

        TRACE_LOG("[GamePackerSlaveImpl].scheduleReapGames: PackerSilo(" << packerSiloId << "): Scheduled reap in(" << (deadline - TimeValue::getTimeOfDay()).getMillis() << "ms)");
    }
}

void GamePackerSlaveImpl::reapGames(PackerSiloId packerSiloId)
{
    auto now = TimeValue::getTimeOfDay();

    auto* siloInfo = getPackerSiloInfo(packerSiloId);
    if (siloInfo == nullptr)
        return;

    auto& reapData = siloInfo->mReapData;
    reapData.mReapCount++;
    reapData.mLastReapTime = now;
    reapData.mReapPackingsTimerId = INVALID_TIMER_ID;

    auto& packerSilo = siloInfo->mPackerSilo;

    packerSilo.reapGames(this); // PACKER_TODO: Note currently reap can actually result in packGames getting invoked! UUGH! This is due to the poor choice of using callbacks to trigger events from the packer. We need to fix all this with our shiny new design by buffering state instead of invoking callbacks, and using an explicit pumping loop and breaking all operations of significance into atomic units. IE: 
    // New Future packing API:
    // Party* peekNextParty() // used to record party and game state prior to packing
    // void packNextParty(evictedParties*, count);
    // void reapNextParty(evictedParties*, count);
    // Party* getQueuedParties();
    // Party* getEvictedParties();
    // etc...

    TimeValue packingTime = packerSilo.packGames(this);

    // update metrics
    siloInfo->mMetrics.mTotalReapGamePackingTime += packingTime.getMicroSeconds();
    mTotalPackingTime.increment(packingTime.getMicroSeconds(), siloInfo->mPackerSilo.getPackerName().c_str(), Fiber::getCurrentContext());

    scheduleReapGames(packerSiloId);

    while (!mDelayCleanupSessions.empty())
    {
        decltype(mDelayCleanupSessions) delayCleanupSessions = eastl::move(mDelayCleanupSessions);
        for (auto packerSessionId : delayCleanupSessions)
        {
            TRACE_LOG("[GamePackerSlaveImpl].reapGames: cleanup delayed session(" << packerSessionId << ")");
            cleanupSession(packerSessionId, GameManager::WORKER_SESSION_COMPLETED); // PACKER_TODO: #SIMPLIFY This is necessary for now to avoid the complexity involved with trying to destroy PackerSession directly from within PackerSilo::reapGames() and cleaning up the associated Packer::Party,Packer::Game,Packer::Player objects in *all* the corresponding silos while correctly updating metrics... Solution: Do not use callbacks!!!! Change to multi-pass approach.
        }
    }
}

uint32_t GamePackerSlaveImpl::getAssignedLayerId() const
{
    return getLayerIdFromSlotId(mAssignedWorkSlotId);
}

bool GamePackerSlaveImpl::isRootLayer() const
{
    return getLayerIdFromSlotId(mAssignedWorkSlotId) == 1;
}

uint32_t GamePackerSlaveImpl::getLayerIdFromSlotId(uint32_t slotId)
{
    // PACKER_TODO: this code is a duplicate of the code in uint32_t MatchmakerMasterImpl::LayerInfo::layerIdFromSlotId(uint32_t slotId), factor out those data structures and share them...
    uint32_t count = 0;
    uint32_t numSlots = slotId;
    while (numSlots > 0)
    {
        ++count;
        numSlots = numSlots >> 1;
    }
    return count;
}

GamePackerSlaveImpl::GameTemplate::GameTemplate(const char8_t* gameTemplateName)
{
    mGameTemplateName = gameTemplateName;
    TRACE_LOG("[GameTemplate].ctor: Create GameTemplate(" << mGameTemplateName << ")");
}

GamePackerSlaveImpl::GameTemplate::~GameTemplate()
{
    TRACE_LOG("[GameTemplate].dtor: Destroy GameTemplate(" << mGameTemplateName << ")");
}

Blaze::GameManager::Rete::ProductionId GamePackerSlaveImpl::getProductionId() const
{
    return mProductionId;
}

bool GamePackerSlaveImpl::notifyOnTokenAdded(GameManager::Rete::ProductionInfo& info) const
{
    return true;
}

GameManager::Matchmaker::PropertyRuleDefinition* GamePackerSlaveImpl::getPropertyRuleDefinition()
{
    return (GameManager::Matchmaker::PropertyRuleDefinition*) mRuleDefinitionCollection->getRuleDefinitionById(GameManager::Matchmaker::PropertyRuleDefinition::getRuleDefinitionId());
}

bool GamePackerSlaveImpl::onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
    const auto packerSessionId = token.mId;
    if (!isMutableParty(packerSessionId))
    {
        ERR_LOG("[GamePackerSlaveImpl].onTokenAdded: Only mutable parties should be indexed with RETE!");
        return false;
    }

    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    if (packerSessionItr == mPackerSessions.end())
    {
        ERR_LOG("[GamePackerSlaveImpl].onTokenAdded: Packer Session " << packerSessionId << " no longer exists!");
        return false;
    }
    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    GameManager::Rete::ProductionNodeId prodNodeId = info.pNode->getProductionNodeId();

    {
        auto* joinNode = &info.pNode->getParent();
        size_t depth = 0;
        while (!joinNode->isHeadNode()) 
        {
            depth++;
            joinNode = joinNode->getParent();
        }

        auto& conditionBlock = packerSession.getConditions().at(GameManager::Rete::CONDITION_BLOCK_BASE_SEARCH);
        if (depth < conditionBlock.size())
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: ProdNodeId(" << prodNodeId << ") referenced by PackerSession(" << packerSessionId << ") is not terminal, skip.");
            return true;
        }

        // Collect game properties from the the RETE branch of the current join node by traversing nodes up the tree
        eastl::hash_set<GameManager::Rete::WMEAttribute> reteProperties;
        joinNode = &info.pNode->getParent();
        while (!joinNode->isHeadNode())
        {
            reteProperties.insert(joinNode->getProductionTest().getName());
            joinNode = joinNode->getParent();
        }

        // Make sure all game properties referenced from the current session are contained in the RETE branch of the current join node
        for (auto& orConditionList : conditionBlock)
        {
            auto& condition = orConditionList.at(0).condition;
            if (reteProperties.find(condition.getName()) == reteProperties.end())
            {
                TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: ProdNodeId(" << prodNodeId << ") conditions don't meet PackerSession(" << packerSessionId << ") filter criteria, skip.");
                return true;
            }
        }

        TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: ProdNodeId(" << prodNodeId << ") referenced by PackerSession(" << packerSessionId << ") is a terminal node, continue.");
    }

    auto iter = mProdNodeIdToSiloIdMap.find(prodNodeId);
    bool isNewProductionNode = (iter == mProdNodeIdToSiloIdMap.end());
    PackerSiloId packerSiloId = GameManager::INVALID_PACKER_SILO_ID;
    if (isNewProductionNode)
    {
        BlazeError errRet = gUniqueIdManager->getId(GamePacker::GamePackerSlave::COMPONENT_ID, (uint16_t) Blaze::GameManager::GAMEPACKER_IDTYPE_PACKER_SILO, packerSiloId);
        if (errRet == ERR_OK)
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Add PackerSilo(" << packerSiloId << ")");
            if (!addPackerSilo(packerSession, packerSiloId, info.pNode))
            {
                ERR_LOG("[GamePackerSlaveImpl].onTokenAdded: ProdNodeId(" << prodNodeId << ") referenced by PackerSession(" << packerSessionId << ") was unable to add a Packer Silo.");
                return true;
            }
        }
        else
        {
            ERR_LOG("[GamePackerSlaveImpl].onTokenAdded: Failed to obtain PackerSilo id when creating silo for session(" << packerSessionId << "), silo creation failed, error: " << errRet);
            return true;
        }
    }
    else
    {
        packerSiloId = iter->second;
    }

    auto* packerSilo = getPackerSilo(packerSiloId);
    if (packerSilo == nullptr)
    {
        EA_FAIL_MESSAGE("[GamePackerSlaveImpl].onTokenAdded: Getting here indicates that a PackerSilo was mapped to the ProdNodeId, but has been removed somehow. ");
        return true;
    }

    if (!isNewProductionNode)
    {
        if (packerSilo->getParties().empty())
        {
            if (unmarkPackerSiloForRemoval(packerSiloId))
            {
                // need to unmark it so that it can be used again
                TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Reinstate empty PackerSilo(" << packerSiloId << ") marked for removal");
            }
            else
            {
                // This can happen when the silo is empty but still present because some of its non-placeholder sessions are locked in a finalization job (and are still in rete)
                TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Reuse empty PackerSilo(" << packerSiloId << ") with pending fin jobs");
            }
        }
        else
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Use existing PackerSilo(" << packerSiloId << "), parties(" << packerSilo->getParties().size() << "), games(" << packerSilo->getGames().size() << ")");
        }
    }
    // PACKER_TODO: Some of the code below is snipped out from gamebrowser.cpp, we may want to readd it as a common static function that can be used by both gamebrowser and packer...

    // PACKER_TODO: #ARCHITECTURE The initial implementation below assumes that a failure to perform an active game query for *any* reason results in a hard session abort (including both FIND/CREATE session modes). The first improvement that we'll need to make(once FIND/CREATE session modes are explicitly supported by the packer) is to ensure that only the FIND packer silo packing gets bypassed in this case. The ultimate improvement we may consider is to ensure that instead of terminating or aborting trackers for silos that need them but cannot construct them, we'd end up creating them still and adding a healthcheck system that would attempt to re-issue the active game queries when the underlying conditions change. (Search slave cluster becomes healthy, etc.)

    bool queryActiveGames = packerSession.mMmInternalRequest->getRequest().getSessionData().getSessionMode().getFindGame(); 
    if (queryActiveGames)
    {
        auto result = mActiveGameQueryBySiloIdMap.insert(packerSiloId);
        auto inserted = result.second;
        auto& gameQueryTracker = result.first->second;
        if (inserted)
        {
            Fiber::BlockingSuppressionAutoPtr blockingSuppress; // Guard to ensure that we never block below

            gameQueryTracker.mGameTemplate = &packerSession.mGameTemplate;
            GameManager::GameBrowserListId gameBrowserListId = 0;
            BlazeError error = gUniqueIdManager->getId(RpcMakeSlaveId(GameManager::GameManagerSlave::COMPONENT_ID), GameManager::GAMEMANAGER_IDTYPE_GAMEBROWSER_LIST, gameBrowserListId);
            if (error != ERR_OK)
            {
                mActiveGameQueryBySiloIdMap.erase(result.first);
                abortSession(packerSessionId, packerSession.getOriginatingScenarioId()); // PACKER_TODO: add abort reason, in this case because we couldn't allocate an active query gb list id, in theory the CREATE flow should still proceed (we'll need to check if we have a CREATE option and avoid aborting in this case...)
                EA_FAIL();  // PACKER_TODO: add error handling, and logging
                return true;
            }
            EA_ASSERT(gameBrowserListId != 0);
            gameBrowserListId = BuildInstanceKey64(gController->getInstanceId(), gameBrowserListId);
            //set the id so it routes to us in a sharded situation
            gameQueryTracker.mGameBrowserListId = gameBrowserListId;

            auto ret = mActiveGameQueryByListIdMap.insert(gameBrowserListId);
            EA_ASSERT_MSG(ret.second, "List id is uniquely generated!");
            ret.first->second = &gameQueryTracker;

            // PACKER_TODO: We need need to respond to the case when # of search slaves changes or they become reconfigured/out of service, etc.
            getFullCoverageSearchSet(gameQueryTracker.mResolvedSearchSlaveList);

            if (gameQueryTracker.mResolvedSearchSlaveList.empty())
            {
                WARN_LOG("[GamePackerSlaveImpl].onTokenAdded: Add PackerSilo(" << packerSiloId << "), no search slaves available, active game flow disabled for session(" << packerSessionId << ")");
                mActiveGameQueryByListIdMap.erase(gameBrowserListId);
                mActiveGameQueryBySiloIdMap.erase(result.first);
                // PACKER_TODO: add error handling, and logging
                abortSession(packerSessionId, packerSession.getOriginatingScenarioId()); // PACKER_TODO: add abort reason, in this case because we couldn't allocate an active query gb list id, in theory the CREATE flow should still proceed (we'll need to check if we have a CREATE option and avoid aborting in this case...)
                EA_FAIL();  // PACKER_TODO: add error handling, and logging
                return true;
            }
                
            auto* searchSlave = getSearchSlave();
            if (searchSlave == nullptr)
            {
                WARN_LOG("[GamePackerSlaveImpl].onTokenAdded: Add PackerSilo(" << packerSiloId << "), seach component unavailable, active game flow disabled for session(" << packerSessionId << ")");
                mActiveGameQueryByListIdMap.erase(gameBrowserListId);
                mActiveGameQueryBySiloIdMap.erase(result.first);
                // PACKER_TODO: add error handling, and logging
                abortSession(packerSessionId, packerSession.getOriginatingScenarioId()); // PACKER_TODO: add abort reason, in this case because we couldn't allocate an active query gb list id, in theory the CREATE flow should still proceed (we'll need to check if we have a CREATE option and avoid aborting in this case...)
                EA_FAIL();  // PACKER_TODO: add error handling, and logging
                return true;
            }
                
            // PACKER_TODOD: #ARCHITECTURE Figure out the following details:
            // 1. To implement find/create properly FIND/CREATE packer session membership needs to be handled within 2 disjoint silos. The sessions that want find, must only match other sessions that want find, 
            //    or sessions that want find/create. The sessions that want create must match only other sesions that want create, or find/create.
            // 1.a Note: FIND only sessions must only be packed into ACTIVE games. Create only sessions should only be packed into PROVISIONAL games. This means that a silo containing only FIND sessions will only be packed when it contains ACTIVE games. 
            //    We'll need to add support for reaping expired parties that never got packed into a game(because there was no active game to pack them into).
            // 2. How to handle the case when a party contains multiple members and must find only active games that have sufficient capacity to accomodate all the members. 
            //    Typically majority of requests would require at least 1 open seat in an active game, however some requests may require multiple open seats on one or more teams. 
            //    These are obviously a subset of all games that have at least one open seat. In theory the selection of games with multiple open seats can be left to the packer, however if the number of games returned from search is very large,
            //    it would be inefficient to get a large number of games that don't match capacity wise. This means that we probably would want to perform some kind of siloing on the minimum number of open seats required per active game. 
            //    This way we would need to specify different lists for games that 
            // 3. Need to determine whether a GB list has been destroyed on the search slave due to disconnect from the packer slave, thus would need to be recreated. 
            // 3.a Packer slave needs to perform recreate request in case connectivity to a search slave is lost.


            // for non-snapshot list we need list of hosting slaves so that we
            // can terminate the list once it's not needed anymore
            //if (!isSnapshot)
            //    list.setListOfHostingSearchSlaves(resolvedSearchSlaveList);

            RpcCallOptions opts;
            //opts.timeoutOverride = gPackerSlaveImpl->getConfig().getMatchmakerConfig().getMatchmakerSettings().getSearchMultiRpcTimeout().getMicroSeconds(); // PACKER_TODO: don't need timeout because we don't block
            opts.ignoreReply = true; // non-blocking

            Search::CreateGameListRequest gameBrowserRequest;
            gameBrowserRequest.setGameBrowserListId(gameBrowserListId);
            gameBrowserRequest.setIsSnapshot(false);
            gameBrowserRequest.setIsUserless(true);
            gameBrowserRequest.getGetGameListRequest().setListConfigName(GAME_PACKER_LIST_NAME);

            
            // Fill in the request with the Properties that this Silo cares about: 
            GameManager::PropertyNameList& propNames = gameBrowserRequest.getGetGameListRequest().getPropertyNameList();
            mPropertyManager.getGameBrowserPackerProperties().copyInto(propNames);

            for (auto& factor : packerSilo->getFactors())
            {
                auto* propertyDescriptor = factor.getInputPropertyDescriptor();
                if (propertyDescriptor == nullptr)
                    continue;

                propNames.push_back() = factor.mOutputPropertyName.c_str();
            }

            // Convert the Packer Session's Filters into something that the Packer Silo can use:
            const auto* propertyRuleDef = getPropertyRuleDefinition();
            auto& siloProperties = getPackerSiloInfo(packerSiloId)->mPackerSiloProperties;
            propertyRuleDef->convertPackerSessionFiltersToSiloFilters(packerSession.getFilterMap(), siloProperties, gameBrowserRequest.getFilterMap());
            
            gameBrowserRequest.getGetGameListRequest().setListCapacity(MAX_GB_LIST_CAPACITY);

            Component::MultiRequestResponseHelper<Search::CreateGameListResponse, Blaze::GameManager::MatchmakingCriteriaError> gameBrowserResponses(gameQueryTracker.mResolvedSearchSlaveList);
            BlazeError rc = searchSlave->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_CREATEGAMELIST, &gameBrowserRequest, gameBrowserResponses, opts);
            if (rc == ERR_OK)
            {
                TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Add new query list(" << gameQueryTracker.mGameBrowserListId << ") for PackerSilo(" << packerSiloId << ")");
            }
            else
            {
                EA_FAIL(); // PACKER_TODO: add an error log
            }
        }
        else
        {
            // game query tracker inserted
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Reuse existing query list(" << gameQueryTracker.mGameBrowserListId << ") for PackerSilo(" << packerSiloId << ")");
        }
        if (gameQueryTracker.mInitiatorSiloIdSet.insert(packerSiloId).second)
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: PackerSilo(" << packerSiloId << ") added to query list(" << gameQueryTracker.mGameBrowserListId << "), initiators(" << gameQueryTracker.mInitiatorSiloIdSet.size() << ")");
        }
        else
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: PackerSilo(" << packerSiloId << ") already in query list(" << gameQueryTracker.mGameBrowserListId << "), initiators(" << gameQueryTracker.mInitiatorSiloIdSet.size() << ")");
        }
    }
    else
    {
        TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Packer session(" << packerSessionId << ") in PackerSilo(" << packerSiloId << ") uses CreateGame session mode, skip active game query.");
    }

    TRACE_LOG("[GamePackerSlaveImpl].onTokenAdded: Add party(" << packerSessionId << ") to PackerSilo(" << packerSiloId << "), existing parties(" << packerSilo->getParties().size() << "), filterMap -> " << SbFormats::TdfObject(packerSession.getFilterMap(), false, 1, '\n') << ", propertyMap -> " << SbFormats::TdfObject(packerSession.mMmInternalRequest->getMatchmakingPropertyMap(), false, 1, '\n'));

    // PACKER_TODO: here we need to queue the search request for this silo (if the create game template allows join in progress)

    bool result = addSessionToPackerSilo(packerSessionId, packerSiloId);
    if (!result)
    {
        EA_FAIL();
        // PACKER_TODO: Add Log. Failed to submit mm session to packer.
        return true;
    }

    packGames(packerSiloId);

    return true;
}

bool GamePackerSlaveImpl::notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& info)
{
    return true;
}

void GamePackerSlaveImpl::onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
    const auto packerSessionId = token.mId;
    EA_ASSERT_MSG(isMutableParty(packerSessionId), "Only mutable parties should be indexed with RETE!");
    auto packerSessionItr = mPackerSessions.find(packerSessionId);
    EA_ASSERT(packerSessionItr != mPackerSessions.end());
    auto& packerSession = *packerSessionItr->second;
    LogContextOverride logAudit(packerSession.getLogContextOverrideUserSessionId());

    EA_ASSERT(info.pNode->getProductionNodeId() != GameManager::Rete::INVALID_PRODUCTION_NODE_ID);
    auto productionNodeId = info.pNode->getProductionNodeId();
    auto siloIdIter = mProdNodeIdToSiloIdMap.find(productionNodeId);
    if (siloIdIter == mProdNodeIdToSiloIdMap.end())
    {
        ERR_LOG("[GamePackerSlaveImpl].onTokenRemoved: ProdNodeId(" << productionNodeId << ") not mapped to any PackerSilo - found when removing PackerSession(" << packerSessionId << ")!");
        EA_FAIL();
        return;
    }

    PackerSiloId packerSiloId = siloIdIter->second;
    Packer::PackerSilo* packerSilo = getPackerSilo(packerSiloId);
    if (packerSilo == nullptr)
    {
        ERR_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") not found when removing PackerSession(" << packerSessionId << ")!");
        EA_FAIL();
        return;
    }

    // onTokenAdded() gets called for sessions that fully match search condition block and sessions that partially match condition block, the latter do not get added to the silo
    bool isSessionInSilo = (packerSession.mMatchedPackerSiloIdSet.find(packerSiloId) != packerSession.mMatchedPackerSiloIdSet.end());

    if (info.pNode->getParent().getBetaMemory().empty())
    {
        // removed last session from packer silo, silo needs to be marked for removal
        auto gameQueryItr = mActiveGameQueryBySiloIdMap.find(packerSiloId);
        if (gameQueryItr != mActiveGameQueryBySiloIdMap.end())
        {
            if (gameQueryItr->second.mInitiatorSiloIdSet.erase(packerSiloId) > 0)
            {
                const auto gameBrowserListId = gameQueryItr->second.mGameBrowserListId;
                if (gameQueryItr->second.mInitiatorSiloIdSet.empty())
                {
                    EA_ASSERT(gameBrowserListId != 0 && !gameQueryItr->second.mResolvedSearchSlaveList.empty());

                    auto* searchSlave = getSearchSlave();
                    if (searchSlave != nullptr)
                    {
                        RpcCallOptions opts;
                        opts.ignoreReply = true;
                        Component::MultiRequestResponseHelper<void, void> responses(gameQueryItr->second.mResolvedSearchSlaveList);
                        Search::TerminateGameListRequest request;
                        request.setListId(gameBrowserListId);
                        BlazeError rc = searchSlave->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_TERMINATEGAMELIST, &request, responses, opts);
                        if (rc == ERR_OK)
                        {
                            TRACE_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") empty, mark for removal, terminate associated list(" << gameBrowserListId << ")");
                        }
                        else
                        {
                            ERR_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") empty, mark for removal, terminate associated list(" << gameBrowserListId << "), error(" << ErrorHelp::getErrorName(rc) << ")");
                        }
                    }
                    else
                    {
                        EA_FAIL(); // PACKER_TODO: add logging
                    }
                    
                    mActiveGameQueryByListIdMap.erase(gameBrowserListId);
                    mActiveGameQueryBySiloIdMap.erase(gameQueryItr);
                }
                else
                {
                    TRACE_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") empty, mark for removal, deref associated list(" << gameBrowserListId << ")");
                }
            }
            else
            {
                TRACE_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId<< ") empty, matches condition but not tracking active games, mark for removal");
            }
        }
        else
        {
            TRACE_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") beta memory empty, mark for removal");
        }
        EA_ASSERT(packerSession.mLastCleanupReason != nullptr);
        if (isSessionInSilo)
        {
            removeSessionFromPackerSilo(packerSession, *packerSilo, packerSession.mLastCleanupReason);
            packGames(packerSiloId); // PACKER_TODO: Always pack after removal, because silo may get unmarked for removal later. Once we implement automatic silo cleanup, this can be removed
        }
        markPackerSiloForRemoval(packerSiloId);
        mProdNodeIdToSiloIdMap.erase(info.pNode->getProductionNodeId()); // dis-associate this id, since the node may stick around in rete, and we don't want to have any chance of reusing the id after the packer silo has been destroyed.
    }
    else
    {
        TRACE_LOG("[GamePackerSlaveImpl].onTokenRemoved: PackerSilo(" << packerSiloId << ") beta memory not empty, games(" << packerSilo->getGames().size() << "), parties(" << packerSilo->getParties().size() << ")");
        EA_ASSERT(packerSession.mLastCleanupReason != nullptr);
        if (isSessionInSilo)
        {
            removeSessionFromPackerSilo(packerSession, *packerSilo, packerSession.mLastCleanupReason);
            packGames(packerSiloId);
        }
    }
}

void GamePackerSlaveImpl::onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
    // We do not currently expect to WME updates, nonetheless it would be good to trace if they were to unexpectedly occur
    WARN_LOG("[GamePackerSlaveImpl].onTokenUpdated: PackerSilo(" << info.pNode->getProductionNodeId() << ") unhandled update for session(" << token.mId << ")");
}

GamePackerSlaveImpl::PackerSession::PackerSession(GameTemplate& gameTemplate) 
    : mGameTemplate(gameTemplate)
{
}

GamePackerSlaveImpl::PackerSession::~PackerSession()
{
    EA_ASSERT_MSG(mFinalizationJobId == 0, "Packer session still belongs to finalization job!");
    EA_ASSERT_MSG(!mRegisteredWithRete, "Still registered with Rete!");
    for (auto* rule : mReteRules)
        delete rule;
    mReteRules.clear();
    cancelUpdateTimer(); // PACKER_MAYBE: just replace this with an assert to ensure we always cancel explicitly?
    cancelMigrateTimer(); // PACKER_MAYBE: just replace this with an assert to ensure we always cancel explicitly?
}

/** triggers addToPackerSilo() */
void GamePackerSlaveImpl::PackerSession::addToReteIndex()
{
    if (mRegisteredWithRete)
    {
        ERR_LOG("[GamePackerSlaveImpl].addToReteIndex: Packer session (id: " << this->mPackerSessionId << ") is attempting to add to RETE when already indexed.");
        return;
    }
    mRegisteredWithRete = true;


    //this->initializeReteProductions(*gPackerSlaveImpl->mReteNetwork, &mGameTemplate); // PACKER_TODO: Investigate, do we need this when re-registering...
    auto& wmeManager = gPackerSlaveImpl->mReteNetwork->getWMEManager();
    wmeManager.registerMatchAnyWME(mPackerSessionId);

    // Normally this would be where we iterate over all Rules in use in RETE, but since we only use one Rule (PropertyFitler) we just look it up directly. 
    gPackerSlaveImpl->getPropertyRuleDefinition()->addPackerSessionWMEs(mPackerSessionId, getFilterMap(), mMmInternalRequest->getMatchmakingPropertyList(), gPackerSlaveImpl->mReteNetwork->getWMEManager());      // Data used in the Filters WMEs should be in the config defined structure.
}

/** triggers removeFromPackerSilo() */
void GamePackerSlaveImpl::PackerSession::removeFromReteIndex()
{
    if (!mRegisteredWithRete)
        return;

    mRegisteredWithRete = false; // set immediately
    
    auto& wmeManager = gPackerSlaveImpl->mReteNetwork->getWMEManager();
    bool usingFilters = !getFilterMap().empty();
    wmeManager.unregisterMatchAnyWME(mPackerSessionId);
    if (usingFilters)
    {
        // Since there's only one Rule (PropertyFilter) in use, we just remove all the WMEs here: 
        wmeManager.removeAllWorkingMemoryElements(mPackerSessionId);
    }
    wmeManager.unregisterWMEId(mPackerSessionId);

    gPackerSlaveImpl->mReteNetwork->idle();
}

void GamePackerSlaveImpl::PackerSession::cancelUpdateTimer()
{
    if (mSendUpdateTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mSendUpdateTimerId);
        mSendUpdateTimerId = INVALID_TIMER_ID;
    }
}

void GamePackerSlaveImpl::PackerSession::cancelMigrateTimer()
{
    if (mMigrateSessionTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mMigrateSessionTimerId);
        mMigrateSessionTimerId = INVALID_TIMER_ID;
    }
}

void GamePackerSlaveImpl::PackerSession::setCleanupReason(const char8_t* reason)
{
    EA_ASSERT(reason != nullptr);
    mCleanupTimestamp = TimeValue::getTimeOfDay();
    mLastCleanupReason = reason;
}

void GamePackerSlaveImpl::PackerSession::setRemoveFromSiloReason(const char8_t* reason)
{
    EA_ASSERT(reason != nullptr);
    mRemovalTimestamp = TimeValue::getTimeOfDay();
    mLastRemovalReason = reason;
}

Blaze::UserSessionId GamePackerSlaveImpl::PackerSession::getLogContextOverrideUserSessionId() const
{
    if (gLogContext->isAudited())
    {
        // PACKER_TODO: (See hansoft: hansoft://eadp-hansoft.rws.ad.ea.com;EADP$20Projects;5519c001/Task/845834?ID=2501) The log context override system as it exists today cannot work correctly in an additive way when an audited context is overridden with another context, because the overridden context ceases to apply and therefore when an owner of a finalization job is being audited, even though his actions should cause logs to be written to his log file, they may not be when those actions take the code down paths where the log context is overridden by subordinate user sessions that are not audited themselves! To avoid this issue, we need to ensure that log context overrides used in the packer slave(and probably everywhere) should only override the current log context when the current context is not audited.If the concept of an audited 'set' was fully implemented in the framework, then we would be able to uniformly specify the log context everywhere, and would be sure that audit logging is correctly output for all cases.Until 'active set' audit logging is correctly, consistently and efficiently implemented in the framework, we must resort to allowing only a single audit at a time, to avoid losing valuable audit log information due to audit flags getting toggled off unintentionally.
        return INVALID_USER_SESSION_ID;
    }
    if (mMmInternalRequest != nullptr)
        return mMmInternalRequest->getOwnerUserSessionInfo().getSessionId();
    EA_ASSERT(isPlaceholderSessionId(mPackerSessionId));
    EA_ASSERT(!mUserSessionIds.empty());
    return mUserSessionIds.front();
}

Blaze::GameManager::UserJoinInfo* GamePackerSlaveImpl::PackerSession::getHostJoinInfo() const
{
    if (mMmInternalRequest != nullptr)
    {
        for (auto& userJoinInfo : mMmInternalRequest->getUsersInfo())
        {
            if (userJoinInfo->getIsHost())
                return userJoinInfo.get();
        }
    }
    else
        EA_ASSERT(mImmutableGameId != 0);
    return nullptr;
}

GameManager::MatchmakingScenarioId GamePackerSlaveImpl::PackerSession::getOriginatingScenarioId() const
{
    auto* host = getHostJoinInfo();
    return host ? host->getOriginatingScenarioId() : GameManager::INVALID_SCENARIO_ID;
}

EA::TDF::TimeValue GamePackerSlaveImpl::PackerSession::getMigrateUnfulfilledTime() const
{
#if 1
    // PACKER_TODO: #ARCHITECTURE
    // This hardcoded interval is currently used to aid in determining the best way to migrate sessions between layers.
    // The proper fix would likely be at least configurable...
    return 250 * 1000;
#else
    uint32_t numLayers = mMatchmakerSlave.getWorkerLayerCount();
    if (numLayers > 1)
        return mTotalSessionDuration / numLayers;
    else
        return mTotalSessionDuration;
#endif
}

uint32_t GamePackerSlaveImpl::getBucketFromScore(float score)
{
    static const uint32_t BUCKET_SIZE_PERCENT = 10;
    uint32_t scorePercent = (uint32_t)(score * 100.0f);
    if (scorePercent > 100)
        scorePercent = 100;
    return (scorePercent / BUCKET_SIZE_PERCENT) * BUCKET_SIZE_PERCENT;
}

GamePackerSlaveImpl::FinalizationJob::~FinalizationJob()
{
    // 'resume' any remaining sessions to the packer
}

GamePackerSlaveImpl::PackerSiloInfo::~PackerSiloInfo()
{
    if (mReapData.mReapPackingsTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mReapData.mReapPackingsTimerId);
        mReapData.mReapPackingsTimerId = INVALID_TIMER_ID;
    }
}

} // GamePacker namespace
} // Blaze namespace

#pragma pop_macro("LOGGER_CATEGORY")

