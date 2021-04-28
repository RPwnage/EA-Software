/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_MATCHMAKERMODULE_H
#define BLAZE_STRESS_MATCHMAKERMODULE_H

#include "stressmodule.h"
#include "stressinstance.h"
#include "gamemanagerutil.h"
#include "gamegroupsutil.h"
#include "matchmakerrulesutil.h" // for defines SizeRuleType, PredefinedRuleConfiguration, GenericRuleDefinitionList etc
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/tdf/scenarios.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/config/config_map.h"
#include "uedutil.h"

using namespace Blaze::GameManager;

namespace Blaze
{
namespace Stress
{

class StressInstance;
class StressConnection;
class GameManagerUtil;
class GameManagerUtilInstance;
class MatchmakerInstance;
class AttrValueUpdateParams;
/*************************************************************************************************/
/*
    MatchmakerModule
*/
/*************************************************************************************************/
class MatchmakerModule : public StressModule
{
    NON_COPYABLE(MatchmakerModule);

public:


    ~MatchmakerModule() override;

    /*
        Stress Module methods
    */
    static StressModule* create();

    enum Action
    {
        ACTION_INVALID = -1,
        ACTION_NOOP, 
        ACTION_CREATE_GAME_SESSION,
        ACTION_FIND_GAME_SESSION,
        ACTION_FIND_OR_CREATE_GAME_SESSION
    };

    // Perform any necessary configuration prior to starting the stress test
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;

    // Called by the core system to create stress instances for this module
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "matchmaker";}

    // Called when the stress tester determines how many connections the module will use out of the total pool
    void setConnectionDistribution(uint32_t conns) override;

    bool saveStats() override;

    Action getAction() const
    {
        return mAction;
    }

    /*
        MatchmakerModule methods
    */
    void releaseInstance(StressInstance *inst);

    bool parseConfig(const ConfigMap& config);

    uint32_t getMatchmakerDelay() const { return mMatchmakerDelay; }
    uint32_t getMatchmakingSessionTimeout() const { return mMatchmakingSessionTimeout; }
    int32_t getLogoutChance() const { return mLogoutChance; }


    GameManagerUtil* getGameManagerUtil() const { return mGameUtil; }
    GamegroupsUtil* getGamegroupsUtil() const { return mGamegroupsUtil; }

    int32_t getNumGameAttribs() const { return mNumGameAttribs; }

    uint32_t getGameStateInitializingDuration() const { return mGameStateInitializingDurationMs; }
    uint32_t getGameStatePreGameDuration() const { return mGameStatePreGameDurationMs; }
    uint32_t getGameStateInGameDuration() const { return mGameStateInGameDurationMs; }
    uint32_t getGameStateInGameDeviation() const { return mGameStateInGameDeviationMs; }
    uint32_t getGameStatePostGameDuration() const { return mGameStatePostGameDurationMs; }

    
    uint16_t getGamegroupLeaderFreq() const { return mGamegroupLeaderFreq; }
    uint16_t getGamegroupJoinerFreq() const { return mGamegroupJoinerFreq; }
    float getGamegroupsFreq() const { return mGamegroupsFreq; }
    
    void incTotalMMSessions() { ++mTotalMMSessionsSinceRateChange; }
    uint32_t getTotalMMSessionsSinceRateChange() const { return mTotalMMSessionsSinceRateChange; }
    uint64_t getMMStartTime() const { return mMatchmakingStartTime; }
    uint64_t getMMRateUpdateTime() const { return mLastMatchmakingSessionRateIncreaseMs; }
    uint32_t getForcedMMRetryCount() const { return mForcedMmRetryCount; }
    uint32_t getForcedMMRetryChance() const { return mForcedMmRetryChance; }
    uint32_t getMaxReloginDelaySec() const { return mMaxReloginDelaySec; }
    uint32_t getMinPlayersForInGameTransition() const { return mMinPlayersForInGameTransition; }

    MatchmakerRulesUtil* getMatchmakerRulesUtil() const { return mMatchmakerRulesUtil; }
    const UEDUtil& getClientUEDUtil() const { return mClientUEDUtil; }
    const char8_t* getScenarioName() const { return mScenarioName.c_str(); }

protected:
    MatchmakerModule();

private:

    typedef eastl::hash_map<int32_t, MatchmakerInstance*> InstanceMap;
    InstanceMap mInstances;
    
    uint32_t mRemainingCreatorCount;//remaining instances to create/join MM sessions
    uint32_t mRemainingPlayersInGgRequired;
    int32_t mNumGameAttribs;

    uint32_t mMatchmakerDelay;
    uint32_t mMatchmakingSessionTimeout;
    int32_t mLogoutChance;

    int32_t mRandSeed;

    Action mAction;
    eastl::string mScenarioName;

    GameManagerUtil *mGameUtil;
    CreateGameRequest mCreateOverride;
    JoinGameRequest mJoinOverride;

    GamegroupsUtil *mGamegroupsUtil;
    uint16_t mGamegroupLeaderFreq;
    uint16_t mGamegroupJoinerFreq;
    float mGamegroupsFreq;


    mutable uint64_t mLastMatchmakingSessionRateIncreaseMs;
    mutable uint32_t mTotalMMSessionsSinceRateChange;
    uint64_t mMatchmakingStartTime;
    GenericRuleDefinitionList mGenericRuleDefinitions;
    uint32_t mGameStateInitializingDurationMs;
    uint32_t mGameStateInGameDeviationMs;
    uint32_t mGameStatePreGameDurationMs;
    uint32_t mGameStateInGameDurationMs;
    uint32_t mGameStatePostGameDurationMs;
    uint32_t mLeaderCount;
    uint32_t mJoinerCount;
    uint32_t mForcedMmRetryCount; // # of times to retry MM after success, to simulate real client behavior of refusing a match.
    uint32_t mForcedMmRetryChance; // % chance user will retry MM even in case of successful match.
    uint32_t mMaxReloginDelaySec; // number of seconds to wait since last logout since attempting another one (0 to disable logouts)
    uint32_t mMinPlayersForInGameTransition; // minimum number of players required for host to transition to IN_GAME state (0 to disable)

    FILE* mLogFile;
    MatchmakerRulesUtil *mMatchmakerRulesUtil;
    UEDUtil mClientUEDUtil;
}; // End MatchmakerModule Class



/*************************************************************************************************/
/*
    MatchmakerInstance
*/
/*************************************************************************************************/
class MatchmakerInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(MatchmakerInstance);
    friend class MatchmakerModule;

public:
    ~MatchmakerInstance() override;

    // This is the entry point for a stress instance to start running
    void start() override;

    //  asynchronous notification event handler.
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

    void onLogin(BlazeRpcError result) override;

    uint64_t getTotalMMSessionTime() { return mTotalMatchmakingSessionTime; }
    
    uint32_t getTotalMMSessions() { return mTotalMatchmakingRequests; }
    uint32_t getTotalMatchmakingRequestsFailed() { return mTotalMatchmakingRequestsFailed; }
    uint32_t getTotalMatchmakingSuccessCreateGame() { return mTotalMatchmakingSuccessCreateGame; }
    uint32_t getTotalMatchmakingSuccessJoinNewGame() { return mTotalMatchmakingSuccessJoinNewGame; }
    uint32_t getTotalMatchmakingSuccessJoinExistingGame() { return mTotalMatchmakingSuccessJoinExistingGame; }
    uint32_t getTotalIndirectMatchmakingSuccessCreateGame() { return mTotalIndirectMatchmakingSuccessCreateGame; }
    uint32_t getTotalIndirectMatchmakingSuccessJoinNewGame() { return mTotalIndirectMatchmakingSuccessJoinNewGame; }
    uint32_t getTotalIndirectMatchmakingSuccessJoinExistingGame() { return mTotalIndirectMatchmakingSuccessJoinExistingGame; }
    uint32_t getTotalMatchmakingFailedTimeout() { return mTotalMatchmakingFailedTimeout; }
    uint32_t getTotalMatchmakingCanceled() { return mTotalMatchmakingCanceled; }
    // isInActive is a terrible, terrible function name.
    bool isRunningMatchmakingSession() { return mMatchmakingScenarioId != 0; }

    const Blaze::GameManager::NotifyGameSetup &getLastNotifyGameSetup() { return mLastNotifyGameSetup; }
    
protected:
    MatchmakerInstance(MatchmakerModule *owner, StressConnection* connection, Login* login);

    void onDisconnected() override;
    void onLogout(BlazeRpcError result) override;

    //Override this method to do your task
    BlazeRpcError execute() override;

    BlazeRpcError buildRequestAndStartMatchmaking();

    const char8_t *getName() const override;

    void setGgRole(GamegroupsUtilInstance::Role role);

private:
    typedef BlazeRpcError (MatchmakerInstance::*AttrUpdateFn)();
    // StressConnection::ConnectionHandler
    BlazeRpcError startMatchmakingSession(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest& scenarioReq, StartMatchmakingResponse &response);
    BlazeRpcError updateAttributes(const AttrValueUpdateParams& params, AttrUpdateFn update, const char* context);
    BlazeRpcError updateGameAttributes();
    BlazeRpcError updatePlayerAttributes();
    BlazeRpcError updateGamegroupAttributes();
    BlazeRpcError updateGgMemberAttributes();
    void advanceGameState();
    void kickMemberFromGame(GameId gameId);
    void setPendingRelogin();
    void doRelogin();
    void onGameRemovedCallback(EA::TDF::Tdf* gameRemovedNotificationTdf);
    void onPlayerRemovedCallback(EA::TDF::Tdf* playerRemovedNotificationTdf);
    void onGameStateChangeCallback(EA::TDF::Tdf* gameStateChangeNotificationTdf);
    void onNotifyGameSetup(EA::TDF::Tdf* tdfNotificaiton);
    void onNotifyMatchmakingFailed(EA::TDF::Tdf* failedTdf);
    uint32_t getGameDuration() const;

private:
    MatchmakerModule *mOwner;
    GameManagerSlave *mProxy;

    GameManagerUtilInstance *mGameManagerInstance;
    GamegroupsUtilInstance *mGamegroupsInstance;

    TimeValue mStartTime;

    MatchmakingScenarioId mMatchmakingScenarioId;
    TimeValue mMatchmakingStartTime;
    uint32_t mTotalMatchmakingRequests;
    uint32_t mTotalMatchmakingRequestsFailed;
    uint32_t mTotalMatchmakingSuccessCreateGame;
    uint32_t mTotalMatchmakingSuccessJoinNewGame;
    uint32_t mTotalMatchmakingSuccessJoinExistingGame;
    uint32_t mTotalIndirectMatchmakingSuccessCreateGame;
    uint32_t mTotalIndirectMatchmakingSuccessJoinNewGame;
    uint32_t mTotalIndirectMatchmakingSuccessJoinExistingGame;
    uint32_t mTotalMatchmakingFailedTimeout;
    uint32_t mTotalMatchmakingCanceled;
    uint32_t mForcedMmRetryCount;
    TimeValue mLastLogout;
    uint64_t mTotalMatchmakingSessionTime;
    TimeValue mLongestSessionTimestamp;
    uint64_t mLongestMatchmakingSession;

    Blaze::GameManager::NotifyGameSetup mLastNotifyGameSetup;

    TimeValue mGameEntryTime;
    bool mGameMember;
    bool mGameHost;
    bool mPendingRelogin;
    PlayerId mPid;
    GameId mGid;

    GameManager::GameState mLastGameState;
    GameManager::GameState mCurrentGameState;
    int64_t mInGameTimer;

    BlazeId mUserBlazeId;

}; // End MatchmakerInstance Class


} // End Stress namespace
} // End Blaze namespace

#endif // BLAZE_STRESS_MATCHMAKERMODULE_H
