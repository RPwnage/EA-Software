/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"

#include "matchmakermodule.h"
#include "loginmanager.h"
#include "blazerpcerrors.h"

#include "gamemanager/tdf/scenarios.h"

#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/tdf/attributes.h"

#define SET_SCENARIO_ATTRIBUTE(scenarioReq, attributeName, value) \
    if (scenarioReq.getScenarioAttributes()[attributeName] == nullptr) \
    { \
        scenarioReq.getScenarioAttributes()[attributeName] = scenarioReq.getScenarioAttributes().allocate_element(); \
    } \
    scenarioReq.getScenarioAttributes()[attributeName]->set(value);


namespace Blaze
{
namespace Stress
{

    static const char8_t* MM_ACTION_STRINGS[] =  {
        "noop",
        "createGameSession",
        "findGameSession",
        "findOrCreateGameSession"
    };

/*************************************************************************************************/
/*
    MatchmakerModule methods
*/
/*************************************************************************************************/


MatchmakerModule::MatchmakerModule()
{
    mRemainingCreatorCount = 0;
    mRemainingPlayersInGgRequired = 0;
    mTotalMMSessionsSinceRateChange = 0;
    mMatchmakingStartTime = 0;
    mGameUtil = BLAZE_NEW GameManagerUtil();
    mGamegroupsUtil = BLAZE_NEW GamegroupsUtil();
    mLogoutChance = 0;
    mRandSeed = -1;
    mLogFile = nullptr;
    mMatchmakerRulesUtil = BLAZE_NEW MatchmakerRulesUtil();
    mLeaderCount = 0;
    mJoinerCount = 0;
    mForcedMmRetryCount = 0;
    mForcedMmRetryChance = 0;
    mMaxReloginDelaySec = 0;
    mMinPlayersForInGameTransition = 0;
}


MatchmakerModule::~MatchmakerModule()
{
    if (mLogFile != nullptr) fclose(mLogFile);

    delete mGameUtil;
    delete mGamegroupsUtil;
    delete mMatchmakerRulesUtil;
}


StressModule* MatchmakerModule::create()
{
    return BLAZE_NEW MatchmakerModule();
}


bool MatchmakerModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if(!StressModule::initialize(config, bootUtil))
        return false;

    if(!parseConfig(config))
        return false;

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].initialize: " << MM_ACTION_STRINGS[mAction] << " action selected.");

    // Initialize the game manager utility
    if (!mGameUtil->initialize(config, bootUtil))
        return false;
    if (!mGamegroupsUtil->initialize(config, bootUtil))
        return false;

    // Seed the random number generator (required by Blaze::Random)
    mRandSeed = config.getInt32("randSeed", -1);

    (mRandSeed < 0) ? srand((unsigned int)TimeValue::getTimeOfDay().getSec()) : srand(mRandSeed);

    if (getGamegroupsFreq() > 0.f && getGamegroupLeaderFreq() > 0 && getGamegroupJoinerFreq() >= 0)
    {
        mRemainingPlayersInGgRequired = static_cast<uint32_t>(mRemainingCreatorCount * getGamegroupsFreq());
    }

    return true;
}


StressInstance* MatchmakerModule::createInstance(StressConnection* connection, Login* login)
{
    MatchmakerInstance *instance = BLAZE_NEW MatchmakerInstance(this, connection, login);
    GamegroupsUtilInstance::Role role = GamegroupsUtilInstance::ROLE_NONE;


    if (getGamegroupsFreq() > 0.f && getGamegroupLeaderFreq() > 0 && getGamegroupJoinerFreq() >= 0)
    {
        float joinersPerLeader = (float)getGamegroupJoinerFreq() / (float)getGamegroupLeaderFreq();

        // base chance this instance uses a GG, on remaining # GGs needed and remaining instances avail
        if ((mRemainingPlayersInGgRequired > 0) && (mRemainingCreatorCount <= mRemainingPlayersInGgRequired || //this line ensuring same # GGs across runs
             Random::getRandomFloat() <= ((float)mRemainingPlayersInGgRequired / (float)mRemainingCreatorCount)))
        {
            if (((float)mLeaderCount)*joinersPerLeader <= (float)mJoinerCount)
            {
                ++mLeaderCount;
                role = GamegroupsUtilInstance::ROLE_LEADER;
            }
            else
            {
                ++mJoinerCount;
                role = GamegroupsUtilInstance::ROLE_JOINER;
            }
            --mRemainingPlayersInGgRequired;
        }
    }
    instance->setGgRole(role);

    InstanceMap::insert_return_type inserter = mInstances.insert(instance->getIdent());
    if (inserter.first == mInstances.end())
    {
        delete instance;
        instance = nullptr;
    }
    else
    {
        inserter.first->second = instance;
    }

    if (mRemainingCreatorCount > 0)
    {
        --mRemainingCreatorCount;
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].createInstance: Internal test error: creating more instances than originally allocated.");
    }
    return instance;
}


void MatchmakerModule::setConnectionDistribution(uint32_t conns)
{
    mRemainingCreatorCount = (uint32_t)(conns);
    StressModule::setConnectionDistribution(conns);
}


bool MatchmakerModule::saveStats()
{

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: instances(" << mInstances.size() << "), games(" << (uint32_t)mGameUtil->getGameCount() << ")");

    InstanceMap::const_iterator iter = mInstances.begin();
    InstanceMap::const_iterator end = mInstances.end();

    uint32_t totalMMSessions = 0;
    uint64_t totalMMTime = 0;
    uint32_t totalMMSessionsTimeout = 0;
    uint32_t totalMMSessionsCanceled = 0;
    uint32_t totalMMRequestsFailed = 0;
    uint32_t totalMMRequestsCreatedGame = 0;
    uint32_t totalMMRequestsJoinedNewGame = 0;
    uint32_t totalMMRequestsJoinedExistingGame = 0;
    uint32_t totalMMSessionsActive = 0;
    uint32_t totalIMMCreatedGame = 0;
    uint32_t totalIMMJoinedNewGame = 0;
    uint32_t totalIMMJoinedExistingGame = 0;

    for (; iter != end; ++iter)
    {
        MatchmakerInstance *inst = iter->second;
        uint32_t instanceMMSessions = inst->getTotalMMSessions();
        uint64_t instanceMMTime = inst->getTotalMMSessionTime();
        if(instanceMMSessions > 0)
        {
            totalMMSessions += instanceMMSessions;
            totalMMTime += instanceMMTime;
            totalMMSessionsTimeout += inst->getTotalMatchmakingFailedTimeout();
            totalMMSessionsCanceled += inst->getTotalMatchmakingCanceled();
            totalMMRequestsFailed += inst->getTotalMatchmakingRequestsFailed();
            totalMMRequestsCreatedGame += inst->getTotalMatchmakingSuccessCreateGame();
            totalMMRequestsJoinedNewGame += inst->getTotalMatchmakingSuccessJoinNewGame();
            totalMMRequestsJoinedExistingGame += inst->getTotalMatchmakingSuccessJoinExistingGame();
            totalIMMCreatedGame += inst->getTotalIndirectMatchmakingSuccessCreateGame();
            totalIMMJoinedNewGame += inst->getTotalIndirectMatchmakingSuccessJoinNewGame();
            totalIMMJoinedExistingGame += inst->getTotalIndirectMatchmakingSuccessJoinExistingGame();
            if(inst->isRunningMatchmakingSession())
            {
                ++totalMMSessionsActive;
            }
        }
        else
        {
        }
        
    }

    if(totalMMSessions > 0)
    {
        TimeValue currentTime = TimeValue::getTimeOfDay();
        uint64_t matchmakingDuration = TimeValue::getTimeOfDay().getMillis() - mMatchmakingStartTime;
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: Matchmaking has been running for " << matchmakingDuration << "ms.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << totalMMSessionsActive << " active MM sessions, average rate = " << (getTotalMMSessionsSinceRateChange()/((currentTime.getMillis() - mLastMatchmakingSessionRateIncreaseMs) / 1000.0f))); 
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: Last rate update " << (currentTime.getMillis() - mLastMatchmakingSessionRateIncreaseMs) << " ms ago, " << getTotalMMSessionsSinceRateChange() << " new sessions since.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: Total " << totalMMSessions << " MM Sessions of average length " << totalMMTime/totalMMSessions <<" ms");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << (totalMMRequestsCreatedGame * 100.0)/ totalMMSessions << " %% of MM sessions created new game.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << (totalMMRequestsJoinedNewGame * 100.0)/ totalMMSessions << " %% of MM sessions joined new game.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << (totalMMRequestsJoinedExistingGame * 100.0)/ totalMMSessions << " %% of MM sessions joined existing game.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << (totalMMSessionsTimeout * 100.0)/ totalMMSessions << " %% of MM sessions timed out.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << (totalMMSessionsCanceled * 100.0)/ totalMMSessions << " %% of MM sessions were canceled.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: " << totalMMRequestsFailed << " start matchmaking requests failed.");
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].saveStats: Indirect MM -- " << totalIMMCreatedGame << " created new, " << totalIMMJoinedNewGame << " joined new, " 
                       << totalIMMJoinedExistingGame <<" joined existing.");


        if (mLogFile != nullptr)
        {
            char8_t timestamp[32]; 
            uint32_t year; 
            uint32_t month; 
            uint32_t day; 
            uint32_t hour; 
            uint32_t min; 
            uint32_t sec; 
            uint32_t msec; 
            TimeValue::getLocalTimeComponents( 
            TimeValue::getTimeOfDay(), &year, &month, &day, &hour, &min, &sec, &msec); 
            blaze_snzprintf(timestamp, sizeof(timestamp), "%d/%02d/%02d-%02d:%02d:%02d.%03d", 
                    year, month, day, hour, min, sec, msec);

            fprintf(mLogFile, "%s +----------------------------+--------+---------+-----------+-----------+--------+--------+--------+---------+--------+----------+\n", timestamp);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: Matchmaking has been running for %" PRIu64 "ms.\n", matchmakingDuration); 
            fprintf(mLogFile, "[MatchmakerModule].saveStats: Currently %u active MM sessions, entering MM at an average rate of %f sessions/second.\n", totalMMSessionsActive, getTotalMMSessionsSinceRateChange()/((TimeValue::getTimeOfDay().getMillis() - mLastMatchmakingSessionRateIncreaseMs) / 1000.0f)); 
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d of MM sessions created new game. \n", totalMMRequestsCreatedGame);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d of MM sessions joined new game.\n", totalMMRequestsJoinedNewGame);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d of MM sessions joined existing game. \n", totalMMRequestsJoinedExistingGame);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d of MM sessions timed out. \n", totalMMSessionsTimeout);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d of MM sessions were canceled. \n", totalMMSessionsCanceled);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: %d start matchmaking requests failed. \n", totalMMRequestsFailed);
            fprintf(mLogFile, "[MatchmakerModule].saveStats: Indirect MM -- %d created new, %d joined new, %d joined existing. \n",
                totalIMMCreatedGame, totalIMMJoinedNewGame, totalIMMJoinedExistingGame);
            fflush(mLogFile);
        }

        //Note: this section can be commented back in, to validate game's created settings (disabled to avoid excess clutter)
        /*if (!mInstances.empty())
        {
            MatchmakerInstance *inst = mInstances.begin()->second;
            BLAZE_INFO_LOG(BlazeRpcLog::stats, "[MatchmakerModule] Sample setup game(id " << inst->getLastNotifyGameSetup().getGameData().getGameId()
                           <<  "): Teams' capacity: " << inst->getLastNotifyGameSetup().getGameData().getTeamCapacity());

            const Blaze::GameManager::TeamIdVector &currentTeams = inst->getLastNotifyGameSetup().getGameData().getTeamIds();
            Blaze::GameManager::TeamIdVector::const_iterator itCurTeam = currentTeams.begin();
            Blaze::GameManager::TeamIdVector::const_iterator itEndTeam = currentTeams.end();
            for (uint16_t teamInd = 0; itCurTeam != itEndTeam; ++itCurTeam, ++teamInd)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::stats, "[MatchmakerModule] Sample setup game(id " << inst->getLastNotifyGameSetup().getGameData().getGameId()
                              << "): TeamId[" << teamInd << "]: " << *itCurTeam);
            }
        }*/
    }

    mGameUtil->dumpStats(this);

    return false;
}


bool MatchmakerModule::parseConfig(const ConfigMap& config)
{
    const char8_t* action = config.getString("action", "");

    if (blaze_stricmp(action, MM_ACTION_STRINGS[ACTION_NOOP]) == 0)
    {
        mAction = ACTION_NOOP;
    }     
    else if (blaze_stricmp(action, MM_ACTION_STRINGS[ACTION_CREATE_GAME_SESSION]) == 0)
    {
        mAction = ACTION_CREATE_GAME_SESSION;
    }     
    else if (blaze_stricmp(action, MM_ACTION_STRINGS[ACTION_FIND_GAME_SESSION]) == 0)
    {
        mAction = ACTION_FIND_GAME_SESSION;
    }  
    else if (blaze_stricmp(action, MM_ACTION_STRINGS[ACTION_FIND_OR_CREATE_GAME_SESSION]) == 0)
    {
        mAction = ACTION_FIND_OR_CREATE_GAME_SESSION;
    }  
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].parseConfig: Unrecognized action: '" << action << "'");
        return false;
    }

    mScenarioName = config.getString("scenarioName", "ILT");

    mGamegroupLeaderFreq = config.getUInt16("ggLeaderFreq", 0);
    mGamegroupJoinerFreq = config.getUInt16("ggJoinerFreq", 0);
    mGamegroupsFreq = (float)config.getDouble("ggGamegroupsFreq", 0);

    // game host state change timings
    mGameStateInitializingDurationMs = config.getUInt32("initStateDurationMs", 1000);
    mGameStatePreGameDurationMs = config.getUInt32("preGameStateDurationMs", 1000) + mGameStateInitializingDurationMs;
    mGameStateInGameDurationMs = config.getUInt32("inGameStateDurationMs", 1000) + mGameStatePreGameDurationMs;
    mGameStateInGameDeviationMs = config.getUInt32("inGameStateDeviationMs", 0);
    mGameStatePostGameDurationMs = config.getUInt32("postGameStateDurationMs", 1000);

    // logout chance
    mLogoutChance = config.getInt32("logoutChance", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].parseConfig: logoutChance = " << mLogoutChance);

    // instance delay
    mMatchmakerDelay = config.getUInt32("matchmakerDelay", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].parseConfig: matchmakerDelay = " << mMatchmakerDelay);

    // matchmaking session timeout
    mMatchmakingSessionTimeout = config.getUInt32("matchmakingTimeout", 10000);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerModule].parseConfig: matchmakingTimeout = " << mMatchmakingSessionTimeout);

    // Initialize the matchmaking rules
    mMatchmakerRulesUtil->parseRules(config);

    // Initialize Client specified UED used by UED based rules
    if (!mClientUEDUtil.parseClientUEDSettings(config, BlazeRpcLog::gamemanager))
        return false;

    mMatchmakingStartTime = TimeValue::getTimeOfDay().getMillis() + mMatchmakerDelay;
    mLastMatchmakingSessionRateIncreaseMs = mMatchmakingStartTime;

    uint32_t tyear, tmonth, tday, thour, tmin, tsec;
    TimeValue tnow = TimeValue::getTimeOfDay(); 
    tnow.getGmTimeComponents(tnow,&tyear,&tmonth,&tday,&thour,&tmin,&tsec);
    char8_t fName[256];
    blaze_snzprintf(fName, sizeof(fName), "%s/%s_stress_%02d%02d%02d%02d%02d%02d.log",(Logger::getLogDir()[0] != '\0')?Logger::getLogDir():".",getModuleName(), tyear, tmonth, tday, thour, tmin, tsec);

    mForcedMmRetryCount = config.getUInt32("forcedMmRetryCount", 1);
    mForcedMmRetryChance = config.getUInt32("forcedMmRetryChance", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchMakerModule].parseConfig(): forcedMmRetryChance = " << mForcedMmRetryChance << ", forcedMmRetryCount = " << mForcedMmRetryCount);
    if (mForcedMmRetryChance > 100)
    {
        mForcedMmRetryChance = 100;
    }
    if (mForcedMmRetryChance > 0 && mForcedMmRetryCount < 1)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchMakerModule].parseConfig(): forcedMmRetryChance overrides forcedMMRetryCount, forcing retry count to 1.");
        mForcedMmRetryCount = 1;
    }
    mMaxReloginDelaySec = config.getUInt32("maxReloginDelaySec", 0);
    mMinPlayersForInGameTransition = config.getUInt32("minPlayersForInGameTransition", 0);

    mLogFile = fopen(fName, "wt");
    if (mLogFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,"[MatchMakerModule].parseConfig: Could not create log file("<<fName<<") in folder("<<Logger::getLogDir()<<")");
        return false;
    }


    return true;
}

void MatchmakerModule::releaseInstance(StressInstance *inst)
{
    auto it = mInstances.find(inst->getIdent());
    if (it != mInstances.end())
    {
        if (it->second->mGamegroupsInstance != nullptr)
        {
            switch (it->second->mGamegroupsInstance->getRole())
            {
            case GamegroupsUtilInstance::ROLE_LEADER:
                mLeaderCount--;
                mRemainingPlayersInGgRequired++;
                break;
            case GamegroupsUtilInstance::ROLE_JOINER:
                mJoinerCount--;
                mRemainingPlayersInGgRequired++;
                break;
            default:
                break;
            }
        }
        mRemainingCreatorCount++;
        mInstances.erase(it);
    }
}

/*************************************************************************************************/
/*
MatchmakerInstance methods
*/
/*************************************************************************************************/


MatchmakerInstance::MatchmakerInstance(MatchmakerModule *owner, StressConnection* connection, Login* login) :
    StressInstance(owner, connection, login, BlazeRpcLog::gamemanager),
    mOwner(owner),
    mProxy(BLAZE_NEW GameManagerSlave(*getConnection())),
    mMatchmakingScenarioId(0),
    mTotalMatchmakingRequests(0),
    mTotalMatchmakingRequestsFailed(0),
    mTotalMatchmakingSuccessCreateGame(0),
    mTotalMatchmakingSuccessJoinNewGame(0),
    mTotalMatchmakingSuccessJoinExistingGame(0),
    mTotalIndirectMatchmakingSuccessCreateGame(0),
    mTotalIndirectMatchmakingSuccessJoinNewGame(0),
    mTotalIndirectMatchmakingSuccessJoinExistingGame(0),
    mTotalMatchmakingFailedTimeout(0),
    mTotalMatchmakingCanceled(0),
    mForcedMmRetryCount(0),
    mTotalMatchmakingSessionTime(0),
    mLongestMatchmakingSession(0),
    mGameMember(false),
    mGameHost(false),
    mPendingRelogin(false),
    mPid(0),
    mGid(0),
    mLastGameState(GameManager::NEW_STATE),
    mCurrentGameState(GameManager::NEW_STATE),
    mInGameTimer(0)
{
    mGameManagerInstance = BLAZE_NEW GameManagerUtilInstance(mOwner->getGameManagerUtil());
    mGameManagerInstance->setStressInstance(this);
    mGameManagerInstance->setRole(GameManagerUtil::ROLE_NONE);

    //set up game group role
    mGamegroupsInstance = BLAZE_NEW GamegroupsUtilInstance(owner->getGamegroupsUtil());
    mGamegroupsInstance->setStressInstance(this);
    mGamegroupsInstance->setRole(GamegroupsUtilInstance::ROLE_NONE);

    // set up notification CBs
    mGameManagerInstance->setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED, GameManagerUtilInstance::AsyncNotifyCb(this, &MatchmakerInstance::onGameRemovedCallback));
    mGameManagerInstance->setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, GameManagerUtilInstance::AsyncNotifyCb(this, &MatchmakerInstance::onPlayerRemovedCallback));
    mGameManagerInstance->setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, GameManagerUtilInstance::AsyncNotifyCb(this, &MatchmakerInstance::onGameStateChangeCallback));
    mGameManagerInstance->setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, GameManagerUtilInstance::AsyncNotifyCb(this, &MatchmakerInstance::onNotifyGameSetup));
    mGameManagerInstance->setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED, GameManagerUtilInstance::AsyncNotifyCb(this, &MatchmakerInstance::onNotifyMatchmakingFailed));
    // Add async handlers -- These should be removed in place of using the custom notification handler above.
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGASYNCSTATUS, this);
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);

    mLastLogout = mStartTime = TimeValue::getTimeOfDay();
}


MatchmakerInstance::~MatchmakerInstance()
{
    getConnection()->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGASYNCSTATUS, this);
    getConnection()->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);
    
    mOwner->releaseInstance(this);// releaseInstance uses internal members of MatchmakerInstance, do not delete them before calling releaseInstance()
    if (mGameManagerInstance != nullptr)
    {
        mGameManagerInstance->setStressInstance(nullptr);
        delete mGameManagerInstance;
    }
    if (mGamegroupsInstance != nullptr)
    {
        delete mGamegroupsInstance;
    }
    delete mProxy;
}

void MatchmakerInstance::setGgRole(GamegroupsUtilInstance::Role role)
{
    mGamegroupsInstance->setRole(role);
}

void MatchmakerInstance::start()
{
    StressInstance::start();
}

void MatchmakerInstance::onLogin(BlazeRpcError result)
{
    if (result != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onLogin: Login failed");       
    }

    //saves off the player's blazeId when login is successful
    mUserBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();

    // set Client UED for skill/UEDRule/TeamUEDBalanceRule's as needed
    ((MatchmakerModule*)getOwner())->getClientUEDUtil().initClientUED(getUserSessionsSlave(), BlazeRpcLog::gamemanager);

    // if UseServerQosSettings is enabled, updateNetworkInfo() will be called by the base StressInstance on login
    if (!getOwner()->getUseServerQosSettings())
        updateNetworkInfo();
}


BlazeRpcError MatchmakerInstance::execute()
{
    BlazeRpcError error = ERR_OK;
    if (mPendingRelogin)
    {
        doRelogin();
    }
    if (!isLoggedIn())
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Unable to process Execute() for instance.  User is not logged in.");
        return ERR_OK;
    }
    error = mGameManagerInstance->execute();
    if (error != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Error in game manager execute.  Error(" << Blaze::ErrorHelp::getErrorName(error) 
                       << ") "<< Blaze::ErrorHelp::getErrorDescription(error));
        return error;
    }
    
    if(mPid == 0)
    {
        mPid = (PlayerId)mUserBlazeId;
    }

    GamegroupsUtilInstance::Role ggRole = mGamegroupsInstance->getRole();
    if (ggRole != GamegroupsUtilInstance::ROLE_NONE)
    {
        const bool executeGamegroupUtil = (!isRunningMatchmakingSession()) && (mGid == 0);
        if (executeGamegroupUtil)
        {
            error = mGamegroupsInstance->execute();
            if (error != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Error in game groups execute.  Error(" 
                    << Blaze::ErrorHelp::getErrorName(error) << ") " << Blaze::ErrorHelp::getErrorDescription(error));
                return error;
            }
        }

        if (ggRole == GamegroupsUtilInstance::ROLE_JOINER)
        {
            // joiner doesn't do any matchmaking or game state transitions
            return ERR_OK;
        }
    }

    // Start a MM session
    if (mGameHost)
    {
        if (mGid != 0)
            advanceGameState();

        if (mCurrentGameState == POST_GAME)
        {
            //mGameHost = false;
            // we need to expire the game group so that a new one gets created next time
            if (mGamegroupsInstance->getRole() == GamegroupsUtilInstance::ROLE_LEADER)
            {
                mGamegroupsInstance->clearCycles();
            }
        }
        else
        {
            mGameManagerInstance->updateGameExternalStatus();
        }
    }
    else if (!mGameMember && !isRunningMatchmakingSession() && mOwner->getAction() != MatchmakerModule::ACTION_NOOP)
    {
        //check MM entry rate
        bool readyForMatchmaking = false;
        if (mGamegroupsInstance != nullptr)
        {
            if (mGamegroupsInstance->getRole() == GamegroupsUtilInstance::ROLE_LEADER)
            {
                const GamegroupData* gd = mGamegroupsInstance->getGamegroupData();
                if (gd != nullptr)
                {
                    size_t memberCount = gd->getMemberCount() + gd->getPendingMemberCount();
                    float halfCapacity = mGamegroupsInstance->getUtil()->getGamegroupMemberCapacity() / 2.0f;
                    if (memberCount >= halfCapacity)
                    {
                        // game group leader only starts to matchmake when at least half the member capacity has been filled
                        readyForMatchmaking = true;
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Ready to MM with game group(" << gd->getGamegroupId() << "). Member count: " << memberCount << "/" << mGamegroupsInstance->getUtil()->getGamegroupMemberCapacity());
                    }
                    else
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Waiting for game group(" <<  gd->getGamegroupId() << ") to fill to half-capacity. Member count: " << memberCount << "/" << mGamegroupsInstance->getUtil()->getGamegroupMemberCapacity());
                    }
                }
                else
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: Waiting to create game group");
                }
            }
            else if (mGamegroupsInstance->getRole() == GamegroupsUtilInstance::ROLE_NONE)
            {
                // this means we dont have leaders, everyone matchmakes
                readyForMatchmaking = true;
            }
        }
        if (readyForMatchmaking)
        {
            // cooldown guard needed to avoid spamming MM in the event of MM failure
            int64_t mmCooldownTime = (mMatchmakingStartTime.getMillis() + getGameDuration()) - TimeValue::getTimeOfDay().getMillis();
            if (mmCooldownTime <= 0)
            {
                error = buildRequestAndStartMatchmaking();
            }
            else
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmkerInstance].execute: MM request delayed by: " << mmCooldownTime << " ms cooldown(based on game duration).");
                setNextSleepDelay((uint32_t)mmCooldownTime);
            }
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent()
                << "] not ready for MM " << "" << "");
        }
    }

    return error;
}

uint32_t MatchmakerInstance::getGameDuration() const 
{
    // NOTE: Right now we have a goofy mechanism whereby ingame duration actually *includes* pregame duration...
    if (mOwner->getGameStateInGameDuration() > mOwner->getGameStatePreGameDuration())
        return mOwner->getGameStateInGameDuration() - mOwner->getGameStatePreGameDuration();
    return 0;
}

BlazeRpcError MatchmakerInstance::startMatchmakingSession(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest& scenarioReq, StartMatchmakingResponse &response)
{
    BlazeRpcError err;
    MatchmakingCriteriaError criteriaError;
    mMatchmakingStartTime = TimeValue::getTimeOfDay();
    ++mTotalMatchmakingRequests;
    mOwner->incTotalMMSessions();

    if ((getIdent() == 0) && (mTotalMatchmakingRequests == 1))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].startMatchmakingSession: Scenario Request: \n" << scenarioReq);
    }
    StartMatchmakingScenarioResponse scenarioResp;
    err = mProxy->startMatchmakingScenario(scenarioReq, scenarioResp, criteriaError);
    if (err == ERR_OK)
        mMatchmakingScenarioId = scenarioResp.getScenarioId();
   
    if(err == ERR_OK)
    {
        eastl::string sessionType;
        if (request.getSessionData().getSessionMode().getFindGame())
        {
            sessionType = "/FIND";
        }
        if (request.getSessionData().getSessionMode().getCreateGame())
        {
            sessionType += "/CREATE";
        }
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].startMatchmakingSession:[" << getIdent() << "] Started " << sessionType.c_str() << " game MM scenario(" << mMatchmakingScenarioId << ") Request #" << mTotalMatchmakingRequests);
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].startMatchmakingSession: Start MM error:" 
            << ErrorHelp::getErrorName(err) << ", criteria error: " << criteriaError.getErrMessage());
        ++mTotalMatchmakingRequestsFailed;
    }

    return err;
}



const char8_t *MatchmakerInstance::getName() const
{
    return MM_ACTION_STRINGS[mOwner->getAction()];;
}

void MatchmakerInstance::kickMemberFromGame(GameId gameId)
{
    if (gameId != mGid)
    {
        // Validate that game id is still what it was, because we only want to perform a kick once entire game duration.
        return;
    }
    auto* gameData = mGameManagerInstance->getGameData();
    if (gameData == nullptr)
        return; // game data invalid
    if (gameData->getPlayerMap().size() <= 1)
        return; // not enough players to kick

    PlayerId kickVictimPlayerId = 0;
    TimeValue kickVictimAddedTime; // always evict player least recently added to avoid unfair churn
    for (auto& itr : gameData->getPlayerMap())
    {
        if (itr.first != mPid) // never try to kick self
        {
            if (kickVictimPlayerId == 0 || kickVictimAddedTime < itr.second.mTimeAdded)
            {
                kickVictimPlayerId = itr.first;
                kickVictimAddedTime = itr.second.mTimeAdded;
            }
        }
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "] "
        << "Host Player(" << mPid << ") kicking non-host Player(" << kickVictimPlayerId << ") from Game(" << mGid << ").");
    BlazeRpcError err;
    RemovePlayerRequest req;
    req.setGameId(mGid);
    req.setPlayerId(kickVictimPlayerId);
    req.setPlayerRemovedReason(PLAYER_KICKED);
    err = mProxy->removePlayer(req);
    if (err == ERR_OK)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "] "
            << "Non-host Player(" << kickVictimPlayerId << ") kicked from Game(" << req.getGameId() << ")");
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "] "
            << "Host kicking Player(" << kickVictimPlayerId << ") from Game(" << req.getGameId() << ") failed, err: " << ErrorHelp::getErrorName(err) << ".");
    }
}

void MatchmakerInstance::setPendingRelogin()
{
    // The main purpose of relogin is to avoid creating too many internal replication UserSession subscriptions on the server,
    // by regularly dropping usersessions and creating new ones. This also tests various caching mechanisms.
    if (!mPendingRelogin && mOwner->getMaxReloginDelaySec() > 0)
    {
        TimeValue now = TimeValue::getTimeOfDay();
        uint32_t delta = (uint32_t)(now - mLastLogout).getSec();
        if (isLoggedIn() && delta > mOwner->getMaxReloginDelaySec())
        {
            mPendingRelogin = true;

            // wake up immediately since we want to relogin
            wakeFromSleep();
        }
    }
}

void MatchmakerInstance::doRelogin()
{
    TimeValue now = TimeValue::getTimeOfDay();
    uint32_t delta = (uint32_t)(now - mLastLogout).getSec();

    // This relogin is used to avoid creating too many internal replication UserSession subscriptions on the server
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].doRelogin: Triggering logout after " << mTotalMatchmakingRequests << " executions and " << delta << " sec.");

    mLastLogout = now;

    logout();

    // intentionally re-login immediately to avoid dropping the PSU due to pause between calls to EXECUTE

    login();

    if (!isLoggedIn())
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].doRelogin: Failed re-login after logout!");
    }

    mPendingRelogin = false;
}

void MatchmakerInstance::onNotifyGameSetup(EA::TDF::Tdf* tdfNotification)
{
    if (tdfNotification == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyGameSetup: nullptr notification sent to onNotifyGameSetup!!");
        return;
    }

    GameManager::NotifyGameSetup* notifyGameSetup = (GameManager::NotifyGameSetup*)tdfNotification;

    // cache off game for stats logging
    notifyGameSetup->copyInto(mLastNotifyGameSetup);

    mGid = notifyGameSetup->getGameData().getGameId();
    mGameMember = true;
    mGameEntryTime = TimeValue::getTimeOfDay();

    if(mPid == notifyGameSetup->getGameData().getTopologyHostInfo().getPlayerId() 
        || mPid == notifyGameSetup->getGameData().getPlatformHostInfo().getPlayerId())
    {
        mGameHost = true;
    }
    else
    {
        mInGameTimer = mGameEntryTime.getMillis() + mOwner->getGameStateInGameDuration();
    }

    mCurrentGameState = notifyGameSetup->getGameData().getGameState();

    // Matchmaking result
    if (notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext() != nullptr)
    {
        const auto& mmSetupContext = *notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext();
        TimeValue mmCompletedTime = TimeValue::getTimeOfDay();
        uint64_t mmSessionLength = mmCompletedTime.getMillis() - mMatchmakingStartTime.getMillis();
        if(mmSessionLength > mLongestMatchmakingSession)
        {
            mLongestMatchmakingSession = mmSessionLength;
            mLongestSessionTimestamp = mmCompletedTime;
        }
        mTotalMatchmakingSessionTime += mmSessionLength;
        if (mMatchmakingScenarioId == mmSetupContext.getScenarioId())
        {
            MatchmakingResult result = mmSetupContext.getMatchmakingResult();
            switch(result)
            {
            case GameManager::SUCCESS_CREATED_GAME:
                ++mTotalMatchmakingSuccessCreateGame;
                break;
            case GameManager::SUCCESS_JOINED_NEW_GAME:
                ++mTotalMatchmakingSuccessJoinNewGame;
                break;
            case GameManager::SUCCESS_JOINED_EXISTING_GAME:
                ++mTotalMatchmakingSuccessJoinExistingGame;
                break;
            default:
                break;
            }

            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyGameSetup: " 
                << "Direct MM scenario(" << mMatchmakingScenarioId << "), Result(" 
                << MatchmakingResultToString(result) << "), FitScore(" << mmSetupContext.getFitScore() << "/" << mmSetupContext.getMaxPossibleFitScore()
                << "), FinJob(" << mmSetupContext.getFinalizationJobId() << "), Game(" << mGid << "), Topology(" << GameNetworkTopologyToString(notifyGameSetup->getGameData().getNetworkTopology()) << "), Players(" 
                << notifyGameSetup->getGameRoster().size() << "/" << notifyGameSetup->getGameData().getMaxPlayerCapacity() << ")"
                << (mGameHost ? ", host" : ""));

            mMatchmakingScenarioId = 0;
        }
    }
    else if (notifyGameSetup->getGameSetupReason().getIndirectMatchmakingSetupContext() != nullptr)
    {
        const auto& mmSetupContext = *notifyGameSetup->getGameSetupReason().getIndirectMatchmakingSetupContext();
        auto scenarioId = mmSetupContext.getScenarioId();
        auto result = mmSetupContext.getMatchmakingResult();
        switch(result)
        {
        case GameManager::SUCCESS_CREATED_GAME:
            ++mTotalIndirectMatchmakingSuccessCreateGame;
            break;
        case GameManager::SUCCESS_JOINED_NEW_GAME:
            ++mTotalIndirectMatchmakingSuccessJoinNewGame;
            break;
        case GameManager::SUCCESS_JOINED_EXISTING_GAME:
            ++mTotalIndirectMatchmakingSuccessJoinExistingGame;
            break;
        default:
            break;  
        }
        
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyGameSetup: " 
            << "Indirect MM scenario(" << scenarioId << "), Result(" 
            << MatchmakingResultToString(result) << "), FitScore(" << mmSetupContext.getFitScore() << "/" << mmSetupContext.getMaxPossibleFitScore()
            << "), FinJob(" << mmSetupContext.getFinalizationJobId() << "), Game(" << mGid << "), Topology(" << GameNetworkTopologyToString(notifyGameSetup->getGameData().getNetworkTopology()) << "), Players(" 
            << notifyGameSetup->getGameRoster().size() << "/" << notifyGameSetup->getGameData().getMaxPlayerCapacity() << ")"
            << (mGameHost ? ", host" : ""));

    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyGameSetup: Unexpected join game context " << notifyGameSetup->getGameSetupReason().getActiveMemberIndex());
    }

    if (mGameHost)
    {
        const auto retryChanceThreshold = mOwner->getForcedMMRetryChance();
        const auto retryCount = mOwner->getForcedMMRetryCount();
        if (retryChanceThreshold > 0 && retryCount > 0)
        {
            const uint32_t retryChance = (uint32_t) Random::getRandomNumber(100);
            if (retryChance <= retryChanceThreshold)
            {
                for (uint32_t i = 0; i < retryCount; ++i)
                {
                    const auto gameDurationMs = mOwner->getGameStateInGameDuration() + mOwner->getGameStatePreGameDuration();
                    TimeValue randomKickTimeWithinGameTimeWindow;
                    randomKickTimeWithinGameTimeWindow.setMillis(Random::getRandomNumber(gameDurationMs));
                    const auto deadline = TimeValue::getTimeOfDay() + randomKickTimeWithinGameTimeWindow;

                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent()
                        << "] rolled(" << retryChance << ") <= retry chance threshold(" << retryChanceThreshold << "), scheduling kick non-host member from Game(" << mGid << ") in(" << randomKickTimeWithinGameTimeWindow.getMillis() << "ms), kicked member will resume MM immediately.");

                    // schedule a time 
                    gSelector->scheduleFiberTimerCall(deadline, this, &MatchmakerInstance::kickMemberFromGame, mGid, "MatchmakerInstance::kickMemberFromGame");
                }
            }
        }
    }

    // do not allow delaying, just start matchmaking immediately
    setNextSleepDelay(0);

    // if instance fiber was sleeping, wake it up to resume game activity without delay
    wakeFromSleep();
}


void MatchmakerInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component != GameManagerSlave::COMPONENT_ID)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onAsync: Unwanted component id: " << component);
        return;
    }

    Heat2Decoder decoder;
    

    switch (type)
    {

    case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART:
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onAsync: Unwanted host migration event!");
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGASYNCSTATUS:
        {
            // matchmaking has completed
            GameManager::NotifyMatchmakingAsyncStatus notifyMatchmakingAsyncStatus;
            if (decoder.decode(*payload, notifyMatchmakingAsyncStatus) == ERR_OK)
            {
                //right now we don't do anything with a mm async notify...
                //should ultimately look at config to determine what to do.
            }
            break;
        }
    default:
        break;
    }
}

void MatchmakerInstance::onNotifyMatchmakingFailed(EA::TDF::Tdf* failedTdf)
{
    TimeValue mmCompletedTime = TimeValue::getTimeOfDay();
    uint64_t mmSessionLength = mmCompletedTime.getMillis() - mMatchmakingStartTime.getMillis();
    if(mmSessionLength > mLongestMatchmakingSession)
    {
        mLongestMatchmakingSession = mmSessionLength;
        mLongestSessionTimestamp = mmCompletedTime;
    }
    mTotalMatchmakingSessionTime += mmSessionLength;

    // matchmaking has completed
    GameManager::NotifyMatchmakingFailed *notifyMatchmakingFailed = (GameManager::NotifyMatchmakingFailed*)failedTdf;
    if (notifyMatchmakingFailed != nullptr && 
        (mMatchmakingScenarioId == notifyMatchmakingFailed->getScenarioId()))
    {
        MatchmakingResult result = notifyMatchmakingFailed->getMatchmakingResult();
        bool ok = false;
        switch(result)
        {
        case GameManager::SESSION_TIMED_OUT:
            ++mTotalMatchmakingFailedTimeout;
            break;
        case GameManager::SESSION_CANCELED:
            ok = true;
            ++mTotalMatchmakingCanceled;
            break;
        default:
            break;  
        }
        if (ok)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyMatchmakingFailed:[" << getIdent() << 
                "] MM scenario(" << mMatchmakingScenarioId << ") failed: " << MatchmakingResultToString(result));
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onNotifyMatchmakingFailed:[" << getIdent() << 
                "] MM scenario(" << mMatchmakingScenarioId << ") failed: " << MatchmakingResultToString(result));
        }

        if (mGamegroupsInstance->getRole() != GamegroupsUtilInstance::ROLE_NONE)
        {
            // if game group based matchmaking fails
            mGamegroupsInstance->clearCycles();
        }

        mMatchmakingScenarioId = 0;
    }

    // failed MM, may as well relogin
    //setPendingRelogin();
}


void MatchmakerInstance::onGameRemovedCallback(EA::TDF::Tdf* gameRemovedNotificationTdf)
{
    GameManager::NotifyGameRemoved *notification = (GameManager::NotifyGameRemoved*) gameRemovedNotificationTdf;
    if( notification->getGameId() == mGid)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onGameRemovedCallback: Game(" << notification->getGameId() << ") has been removed.");
        mGameHost = false;
        mGameMember = false;
        mGid = 0;
        mCurrentGameState = NEW_STATE;
        mGamegroupsInstance->clearCycles();
        // do not allow delaying, just start matchmaking immediately
        setNextSleepDelay(0);

        // if instance fiber was sleeping, wake it up to resume matchmaking without delay
        wakeFromSleep();
    }
}

void MatchmakerInstance::onPlayerRemovedCallback(EA::TDF::Tdf* playerRemovedNotificationTdf)
{
    GameManager::NotifyPlayerRemoved *notification = (GameManager::NotifyPlayerRemoved*) playerRemovedNotificationTdf;
    if(notification->getPlayerId() == mPid)
    {
        mGameHost = false;
        mGameMember = false;
        mGid = 0;
        mCurrentGameState = NEW_STATE;
        mGamegroupsInstance->clearCycles();

        auto retryChanceThreshold = mOwner->getForcedMMRetryChance();
        if (retryChanceThreshold > 0 && notification->getPlayerRemovedReason() == PLAYER_KICKED)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "]"
                << " Player(" << mPid << ") was removed from Game(" << notification->getGameId()
                << "), reason: " << GameManager::PlayerRemovedReasonToString(notification->getPlayerRemovedReason()) << ", deliberately to resume MM immediately, due to configured retry chance threshold");

        }
        else if (notification->getPlayerRemovedReason() == GAME_DESTROYED)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "]"
                << " Player(" << mPid << ") was removed from Game(" << notification->getGameId()
                << "), reason: " << GameManager::PlayerRemovedReasonToString(notification->getPlayerRemovedReason()) << ", deliberately resume MM immediately, due to game end");
        }
        else
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "]"
                << " Player(" << mPid << ") was removed from Game(" << notification->getGameId()
                << "), reason: " << GameManager::PlayerRemovedReasonToString(notification->getPlayerRemovedReason()) << ", unexpectedly will wait for cooldown before resuming MM to avoid spamming the server in case of failure.");
            return;
        }

        // NOTE: deliberately reset the start mm time(used as mm rate governor) to ensure that this instance goes right back into MM ASAP.
        // This does increase the MM rate over the alternative of letting the instance wait its full MM period between requests;
        // however, in this case we deliberately want to exercise the flow of leaving the game and rejoining it (or another active game)
        // as soon as possible to keep the games maximally full to ensure that the instance has a chance to claim its old spot in the game
        // (or a new spot in another game) thus keeping the max number of players in game at all times.
        mMatchmakingStartTime = 0;

        // do not allow delaying, just start matchmaking immediately
        setNextSleepDelay(0);

        // if instance fiber was sleeping, wake it up to resume game activity without delay
        wakeFromSleep();
    }
}

void MatchmakerInstance::onGameStateChangeCallback(EA::TDF::Tdf* gameStateChangeNotificationTdf)
{
    GameManager::NotifyGameStateChange *notification = (GameManager::NotifyGameStateChange*)gameStateChangeNotificationTdf;

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "]"
        << " Game(" << notification->getGameId()
        << ") state change, currentGameState(" << GameManager::GameStateToString(mCurrentGameState) << ") -> newGameState(" << GameManager::GameStateToString(notification->getNewGameState()) << ")");

    mCurrentGameState = notification->getNewGameState();
    mGameManagerInstance->clearIsWaitingForGameStateNotification();

    // do not allow delaying, just continue immediately
    setNextSleepDelay(0);

    // if instance fiber was sleeping, wake it up to resume matchmaking without delay
    wakeFromSleep();
}

void MatchmakerInstance::advanceGameState()
{
    if (mLastGameState != NEW_STATE && mLastGameState == mCurrentGameState)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid 
            << ") advancing game state without change: " << GameManager::GameStateToString(mCurrentGameState));
    }
    mLastGameState = mCurrentGameState;
    TimeValue curTime = TimeValue::getTimeOfDay();
    //TODO:: Need to make time configurable
    if(!mGameManagerInstance->isWaitingForGameStateNotification())
    {
        int64_t waitTime = 0;
        switch(mCurrentGameState)
        {
            case NEW_STATE:
                break;
            case INITIALIZING:
                waitTime = (mGameEntryTime.getMillis() + mOwner->getGameStateInitializingDuration()) - curTime.getMillis();
                if (waitTime <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") moving to PRE_GAME after "
                                   << (curTime.getMillis() - mGameEntryTime.getMillis()) << " ms");
                    mGameManagerInstance->advanceGameState(mGid, PRE_GAME);
                }
                break;
            case PRE_GAME:
                waitTime = (mGameEntryTime.getMillis() + mOwner->getGameStatePreGameDuration()) - curTime.getMillis();
                if (waitTime <= 0)
                {
                    size_t playerCount = 0;
                    const GameData* data = mGameManagerInstance->getGameData();
                    if (data != nullptr)
                    {
                        playerCount = data->getNumPlayers();
                    }
                    if (playerCount < mOwner->getMinPlayersForInGameTransition())
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") lingering in PRE_GAME due to insufficient " 
                            << "game.rosterSize(" << playerCount << ") < MinPlayersForInGameTransition(" << mOwner->getMinPlayersForInGameTransition() << ")");
                        return;
                    }
                    const AttrValueUpdateParams* params = &mGameManagerInstance->getUtil()->getGameAttrUpdateParams();
                    if (params->mPreGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGameAttributes, "game");
                    }
                    params = &mGameManagerInstance->getUtil()->getPlayerAttrUpdateParams();
                    if (params->mPreGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updatePlayerAttributes, "player");
                    }
                    params = &mGamegroupsInstance->getUtil()->getGamegroupAttrUpdateParams();
                    if (params->mPreGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGamegroupAttributes, "gamegroup");
                    }
                    params = &mGamegroupsInstance->getUtil()->getMemberAttrUpdateParams();
                    if (params->mPreGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGgMemberAttributes, "ggMember");
                    }
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") moving to IN_GAME after "
                                   << (curTime.getMillis() - mGameEntryTime.getMillis()) << " ms");
                    mGameManagerInstance->advanceGameState(mGid, IN_GAME);

                    // Adjust the game length by a random plus or minus 20 percent factor to avoid resonance issues that have been seen in ILT
                    uint32_t randAdjust = ((uint32_t)rand() % (mOwner->getGameStateInGameDuration() / 5));
                    if ( ((uint32_t)rand() % 2) == 0 )
                        mInGameTimer = mGameEntryTime.getMillis() + mOwner->getGameStateInGameDuration() + randAdjust;
                    else
                        mInGameTimer = mGameEntryTime.getMillis() + mOwner->getGameStateInGameDuration() - randAdjust;
                    waitTime = mInGameTimer - TimeValue::getTimeOfDay().getMillis();
                }
                break;
            case IN_GAME:
                waitTime = mInGameTimer - curTime.getMillis();
                if (waitTime <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") moving to POST_GAME after "
                                   << (curTime.getMillis() - mGameEntryTime.getMillis()) << " ms");

                    GameSettings newGameSettings;
                    const GameData *gameData = mOwner->getGameManagerUtil()->getGameData(mGid);
                    if (gameData != nullptr)
                    {
                        newGameSettings.setBits(gameData->getGameSettings()->getBits());
                    }
                    newGameSettings.clearOpenToMatchmaking();
                    mGameManagerInstance->setGameSettings(&newGameSettings);
                    mGameManagerInstance->advanceGameState(mGid, POST_GAME);
                }
                break;
            case POST_GAME:
                waitTime = mInGameTimer + mOwner->getGameStatePostGameDuration() - curTime.getMillis();
                if (waitTime <= 0)
                {
                    const AttrValueUpdateParams* params = &mGameManagerInstance->getUtil()->getGameAttrUpdateParams();
                    if (params->mPostGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGameAttributes, "game");
                    }
                    params = &mGameManagerInstance->getUtil()->getPlayerAttrUpdateParams();
                    if (params->mPostGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updatePlayerAttributes, "player");
                    }
                    params = &mGamegroupsInstance->getUtil()->getGamegroupAttrUpdateParams();
                    if (params->mPostGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGamegroupAttributes, "gamegroup");
                    }
                    params = &mGamegroupsInstance->getUtil()->getMemberAttrUpdateParams();
                    if (params->mPostGame)
                    {
                        updateAttributes(*params, &MatchmakerInstance::updateGgMemberAttributes, "ggMember");
                    }
                    const MatchmakerRulesUtil *matchmakerRulesUtil = mOwner->getMatchmakerRulesUtil();
                    BlazeRpcError err = Blaze::ERR_OK;

                    if (matchmakerRulesUtil->getNetworkTopology() == CLIENT_SERVER_DEDICATED)
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") returning dedicated server to pool "
                                       << (curTime.getMillis() - mGameEntryTime.getMillis()) << " ms");
                        ReturnDedicatedServerToPoolRequest request;
                        request.setGameId(mGid);
                        err = mProxy->returnDedicatedServerToPool(request);

                        if(err != ERR_OK)
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Failed to return dedicated server to pool: game(" << mGid 
                                          << ") Err(" << Blaze::ErrorHelp::getErrorName(err) << ")");
                        }
                    }
                    else
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Game(" << mGid << ") destroying after "
                                       << (curTime.getMillis() - mGameEntryTime.getMillis()) << " ms");
                        GameManager::DestroyGameRequest destroyRequest;
                        GameManager::DestroyGameResponse destroyResponse;
                        destroyRequest.setGameId(mGid);
                        destroyRequest.setDestructionReason(GameManager::HOST_LEAVING);
                        err = mProxy->destroyGame(destroyRequest, destroyResponse);

                        if(err != ERR_OK)
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].advanceGameState: Unable to destroy game(" << mGid 
                                          << ") Err(" << Blaze::ErrorHelp::getErrorName(err) << ")");
                        }
                    }

                    if (err == ERR_OK)
                        mGid = 0; // NOTE: Clear gameid only if we successfully delete/reset the game, otherwise we should retry until we succeed, like with the other states.
                }
                break;
            case MIGRATING:
                break;
            case DESTRUCTING:
                break;
            case RESETABLE:
                break;
            default:
                break;
         }

         if (waitTime > 0)
             setNextSleepDelay((uint32_t)waitTime);
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance][" << getIdent() << "] Game(" << mGid
        << ") waiting for game state change from : " << GameManager::GameStateToString(mCurrentGameState));
    }
}

BlazeRpcError MatchmakerInstance::updateAttributes(const AttrValueUpdateParams& params, MatchmakerInstance::AttrUpdateFn update, const char* context)
{
    BlazeRpcError rc = ERR_OK;
    for (uint32_t i = 0; i < params.mUpdateCount;)
    {
        rc = (this->*(update))();
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].updateAttributes: Game(" 
                << mGid << ") failed to update " << context << " attributes err: " << ErrorHelp::getErrorName(rc));
            break;
        }
        if (++i > 0)
        {
            uint32_t sleepMs = ((uint32_t)rand() % (params.mUpdateDelayMaxMs + 1));
            if (sleepMs < params.mUpdateDelayMinMs)
                sleepMs = params.mUpdateDelayMinMs;
            sleep(sleepMs);
        }
    }
    return rc;
}

BlazeRpcError MatchmakerInstance::updateGameAttributes()
{
    BlazeRpcError rpcResult = ERR_OK;
    const GameData* gameData = mGameManagerInstance->getGameData();
    // only host updates the game attributes
    if (gameData != nullptr && !mGameManagerInstance->getUtil()->getGameAttrValues().empty() && gameData->getHostId() == mPid)
    {
        SetGameAttributesRequest req;
        req.setGameId(mGid);
        AttrValueUtil::fillAttributeMap(mGameManagerInstance->getUtil()->getGameAttrValues(), req.getGameAttributes());
        rpcResult = mProxy->setGameAttributes(req);
    }
    return rpcResult;
}

BlazeRpcError MatchmakerInstance::updatePlayerAttributes()
{
    BlazeRpcError rpcResult = ERR_OK;
    const GameData* gameData = mGameManagerInstance->getGameData();
    if (gameData != nullptr && !mGameManagerInstance->getUtil()->getPlayerAttrValues().empty())
    {
        SetPlayerAttributesRequest req;
        req.setGameId(mGid);
        req.setPlayerId(mPid);
        AttrValueUtil::fillAttributeMap(mGameManagerInstance->getUtil()->getPlayerAttrValues(), req.getPlayerAttributes());
        rpcResult = mProxy->setPlayerAttributes(req);
    }
    return rpcResult;
}

BlazeRpcError MatchmakerInstance::updateGamegroupAttributes()
{
    BlazeRpcError rpcResult = ERR_OK;
    const GamegroupData* ggData = mGamegroupsInstance->getGamegroupData();
    // only admins update the game group attributes
    if (ggData != nullptr && !mGamegroupsInstance->getUtil()->getMemberAttrValues().empty() && ggData->isAdmin(mPid))
    {
        SetGameAttributesRequest req;
        req.setGameId(mGamegroupsInstance->getGamegroupId());
        AttrValueUtil::fillAttributeMap(mGamegroupsInstance->getUtil()->getGamegroupAttrValues(), req.getGameAttributes());
        rpcResult = mGamegroupsInstance->getSlave()->setGameAttributes(req);
    }
    return rpcResult;
}

BlazeRpcError MatchmakerInstance::updateGgMemberAttributes()
{
    BlazeRpcError rpcResult = ERR_OK;
    const GamegroupData* ggData = mGamegroupsInstance->getGamegroupData();
    if (ggData != nullptr && !mGamegroupsInstance->getUtil()->getMemberAttrValues().empty())
    {
        SetPlayerAttributesRequest req;
        req.setGameId(mGamegroupsInstance->getGamegroupId());
        req.setPlayerId(mPid);
        AttrValueUtil::fillAttributeMap(mGamegroupsInstance->getUtil()->getMemberAttrValues(), req.getPlayerAttributes());
        rpcResult = mGamegroupsInstance->getSlave()->setPlayerAttributes(req);
    }
    return rpcResult;
}

void MatchmakerInstance::onLogout(BlazeRpcError result)
{
    mGameHost = false;
    mGameMember = false;
    mGid = 0;
    mPid = 0;
    mMatchmakingScenarioId= 0;
    mCurrentGameState = GameManager::NEW_STATE;
    mInGameTimer = 0;

    if (mGameManagerInstance != nullptr)
    {
        mGameManagerInstance->onLogout();
    }
    if (mGamegroupsInstance != nullptr)
    {
        mGamegroupsInstance->onLogout();
    }

    StressInstance::onLogout(result);
}


void MatchmakerInstance::onDisconnected()
{
    onLogout(ERR_OK);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerInstance].onDisconnected: MatchmakerInstance disconnected!");
}

Blaze::BlazeRpcError MatchmakerInstance::buildRequestAndStartMatchmaking()
{
    StartMatchmakingScenarioRequest scenarioReq;
    
    scenarioReq.setScenarioName(mOwner->getScenarioName());

    StartMatchmakingRequest request;
    StartMatchmakingResponse response;

    TimeValue sessionTimeout;
    sessionTimeout.setMillis((int64_t)mOwner->getMatchmakingSessionTimeout());
    request.getSessionData().setSessionDuration(sessionTimeout);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SESSION_DURATION", sessionTimeout);

    const MatchmakerRulesUtil *matchmakerRulesUtil = mOwner->getMatchmakerRulesUtil();
    
    // set network topology
    request.getGameCreationData().setNetworkTopology(matchmakerRulesUtil->getNetworkTopology());
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "NET_TOPOLOGY", matchmakerRulesUtil->getNetworkTopology());

    // game protocol version rule
    if(matchmakerRulesUtil->getUseRandomizedGameProtocolVersionString())
    {
        //use the configured name + the persona name
        char8_t gameProtocolVersionString[64];
        blaze_strnzcpy(gameProtocolVersionString, matchmakerRulesUtil->getGameProtocolVersionString(), 64);
        blaze_strnzcat(gameProtocolVersionString, getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName(), 64);  
        request.getCommonGameData().setGameProtocolVersionString(gameProtocolVersionString);
        scenarioReq.getCommonGameData().setGameProtocolVersionString(gameProtocolVersionString);
    }
    else
    {
        request.getCommonGameData().setGameProtocolVersionString(matchmakerRulesUtil->getGameProtocolVersionString());
        scenarioReq.getCommonGameData().setGameProtocolVersionString(matchmakerRulesUtil->getGameProtocolVersionString());
    }

    //set the MatchmakingCriteriaData (MM rules etc)
    const uint16_t myGgSize = ((mGamegroupsInstance->getGamegroupData() == nullptr)? 0 : mGamegroupsInstance->getGamegroupData()->getMemberCount());
    const uint16_t mySessionSize = ((myGgSize == 0)? 1: myGgSize);
    matchmakerRulesUtil->buildMatchmakingCriteria(request.getCriteriaData(), scenarioReq, mySessionSize);

    matchmakerRulesUtil->buildRoleSettings(request, scenarioReq, mGamegroupsInstance->getGamegroupData(), mUserBlazeId);

    // Add the player to the game, if no roles are being used:
    if (request.getPlayerJoinData().getPlayerDataList().size() == 0)
    {
        Blaze::GameManager::PerPlayerJoinData* data = request.getPlayerJoinData().getPlayerDataList().pull_back();
        data->getUser().setBlazeId(mUserBlazeId);

        data = scenarioReq.getPlayerJoinData().getPlayerDataList().pull_back();
        data->getUser().setBlazeId(mUserBlazeId);
    }

    // Set the prefer/avoid rules:
    matchmakerRulesUtil->buildAvoidPreferSettings(request, scenarioReq, mGamegroupsInstance->getGamegroupData(), mUserBlazeId);


    //set the startMMreq's actual player's attribs
    //side note: this section used for MatchmakerModule, but not used for GameBrowserModule (not added to common MatchmakerRulesUtil).
    const GenericRuleDefinitionList *genericRuleList = matchmakerRulesUtil->getGenericRuleDefinitionList();
    GenericRuleDefinitionList::const_iterator genericIter = genericRuleList->begin();
    GenericRuleDefinitionList::const_iterator genericEnd = genericRuleList->end();
    for(; genericIter != genericEnd; ++genericIter)
    {
            //set playerAttribs
            if(genericIter->sIsPlayerAttribRule)
            {
                int32_t playerAttribRoll = rand() % 100;
                int32_t playerAttribFreq = 0;
                PossibleValuesList::const_iterator playerAttribIter = genericIter->sAttributePossibleValues.begin();
                PossibleValuesList::const_iterator playerAttribEnd = genericIter->sAttributePossibleValues.end();
                for(; playerAttribIter != playerAttribEnd; ++playerAttribIter)
                {
                    playerAttribFreq += playerAttribIter->sValueFreq;
                    if(playerAttribRoll < playerAttribFreq)
                    {
                        Blaze::Collections::AttributeMap &playerAttribs = (*request.getPlayerJoinData().getPlayerDataList().begin())->getPlayerAttributes();
                        playerAttribs[genericIter->sAttribName] = *(playerAttribIter->sValueList.begin());

                        Blaze::Collections::AttributeMap &playerAttribs2 = (*scenarioReq.getPlayerJoinData().getPlayerDataList().begin())->getPlayerAttributes();
                        playerAttribs2[genericIter->sAttribName] = *(playerAttribIter->sValueList.begin());
                        break;
                    }
                }
            }
    }//for

    // set create game AllowDuplicateTeamIds setting
    GameSettings gameSettings;
    if (matchmakerRulesUtil->getAllowDuplicateTeamIds())
    {
        gameSettings.setAllowSameTeamId();
    }
    else
    {
        gameSettings.clearAllowSameTeamId();
    }

    const char8_t* attrValue = AttrValueUtil::pickAttrValue(mOwner->getGameManagerUtil()->getGameAttrValues(), mOwner->getGameManagerUtil()->getGameModeAttributeName());
    request.getGameCreationData().getGameAttribs()[mOwner->getGameManagerUtil()->getGameModeAttributeName()] = attrValue;
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "GAME_MODE", attrValue);

    gameSettings.setOpenToBrowsing();
    gameSettings.setOpenToMatchmaking();
    gameSettings.setOpenToJoinByPlayer();
    gameSettings.setOpenToInvites();
    gameSettings.setJoinInProgressSupported();
    request.getGameCreationData().getGameSettings() = gameSettings;
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "GAME_SETTINGS", gameSettings);

    BlazeObjectId groupId;
    if (mGamegroupsInstance->getGamegroupId() != INVALID_GAME_ID)
    {
        groupId = EA::TDF::ObjectId(ENTITY_TYPE_GAME_GROUP, (Blaze::EntityId)mGamegroupsInstance->getGamegroupId());
        // do not allow any more simulated mm instances to join this game group since we already started matchmaking
        // and further added members will be ignored by the matchmaker, and the game manager.
        mGamegroupsInstance->setJoinable(false);
    }
    else
    {
        // We do not have to set anything for the group id when it's just a single user.
        // If we want to support MLU for ILT, it will require joining multiple players to the Local connection group, and using that instead.
    }

    // Always set a group id (either the game group, or the local user group)
    request.getPlayerJoinData().setGroupId(groupId);
    scenarioReq.getPlayerJoinData().setGroupId(groupId);

    MatchmakingSessionMode sessionMode;
    MatchmakerModule::Action action = mOwner->getAction();
    switch(action)
    {
    case MatchmakerModule::ACTION_CREATE_GAME_SESSION:
        sessionMode.setCreateGame();
        break;
    case MatchmakerModule::ACTION_FIND_GAME_SESSION:
        sessionMode.setFindGame();
        break;
    case MatchmakerModule::ACTION_FIND_OR_CREATE_GAME_SESSION:
        sessionMode.setCreateGame();
        sessionMode.setFindGame();
        break;
    default:
        break;
    }
    
    request.getSessionData().getSessionMode() = sessionMode;
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SESSION_MODE", sessionMode);

    // Not really sure where this hack was being done before...
    if (request.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
    {
        // need even a garbage network address here to avoid error on game create
        NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
        hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);

        hostAddress = &scenarioReq.getCommonGameData().getPlayerNetworkAddress();
        hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
    }

    return startMatchmakingSession(request, scenarioReq, response);
}


} // Stress end
} // Blaze end

#undef SET_SCENARIO_ATTRIBUTE
