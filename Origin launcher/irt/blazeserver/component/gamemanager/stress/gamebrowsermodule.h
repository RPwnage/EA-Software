/*************************************************************************************************/
/*!
    \file


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEBROWSERMODULE_H
#define BLAZE_STRESS_GAMEBROWSERMODULE_H

#include "stressmodule.h"
#include "stressinstance.h"
#include "gamemanagerutil.h"
#include "matchmakerrulesutil.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/rpc/gamemanagerslave.h"

#include "EASTL/vector.h"
#include "EASTL/hash_set.h"

using namespace Blaze::GameManager;

namespace Blaze
{
namespace Stress
{

class StressInstance;
class StressConnection;
class GameManagerUtil;
class GameManagerUtilInstance;
class GameBrowserInstance;
typedef eastl::hash_set<GameId> GameBrowserGameSet;

/*************************************************************************************************/
/*
    GameBrowserModule
*/
/*************************************************************************************************/
class GameBrowserModule : public StressModule
{
    NON_COPYABLE(GameBrowserModule);

public:
    enum Action
    {
        ACTION_INVALID = -1,
        ACTION_NOOP, 
        ACTION_GET_GAME_DATA
    };

    enum Metric
    {
        NOTIFY_UPDATES = 0,
        NOTIFY_REMOVALS,
        NOTIFICATIONS,
        NUM_METRICS
    };

    enum DumpStats
    {
        DUMP_STATS_FULL,
        DUMP_STATS_AVG,
        DUMP_STATS_NONE
    };
    bool parseDumpStats(DumpStats& dumpStats, const char8_t* dumpStatsStr);

    class PossibleValue
    {
    public:
        PossibleValue()
            : mValue(),
            mCreateChance(0),
            mBrowseChance(0)
        { }
        virtual ~PossibleValue() { }

        const Collections::AttributeValue& getValue() const { return mValue; }
        void setValue(const char8_t* value) { mValue = value; }

        uint32_t getCreateChance() const { return mCreateChance; }
        void setCreateChance(uint32_t chance) { mCreateChance = chance; }

        uint32_t getBrowseChance() const { return mBrowseChance; }
        void setBrowseChance(uint32_t chance) { mBrowseChance = chance; }

    private:
        Collections::AttributeValue mValue;
        uint32_t mCreateChance;
        uint32_t mBrowseChance;
    };

    typedef eastl::vector<PossibleValue> PossibleValues;

    class StressGameAttribute
    {
    public:
        StressGameAttribute()
            : mPrefix(),
            mAutoSuffixCount(0),
            mCount(0),
            mThresholdName()
        { }
        virtual ~StressGameAttribute() { }

        const Collections::AttributeName& getPrefix() const { return mPrefix; }
        void setPrefix(const char8_t* prefix) { mPrefix = prefix; }

        uint32_t getAutoSuffixCount() const { return mAutoSuffixCount; }
        void setAutoSuffixCount(uint32_t count) { mAutoSuffixCount = count; }

        const MinFitThresholdName& getThresholdName() const { return mThresholdName; }
        void setThresholdName(const char8_t* name) { mThresholdName = name; }

        PossibleValues& getPossibleValues() { return mPossibleValues; }
        const char8_t* chooseCreateValue() const { return chooseValue(CREATE); }
        const char8_t* chooseBrowseValue() const  { return chooseValue(BROWSE); }

        const char8_t* getAttribName(char8_t* name, uint32_t len, uint32_t ndx) const;

    private:
        enum SelectionMode { BROWSE, CREATE };
        const char8_t* chooseValue(SelectionMode mode) const;

    private:
        Collections::AttributeName mPrefix;
        uint32_t mAutoSuffixCount;
        uint32_t mCount;
        MinFitThresholdName mThresholdName;
        PossibleValues mPossibleValues;

    };
    typedef eastl::vector<StressGameAttribute> GameAttributes;

    ~GameBrowserModule() override;

    /*
        Stress Module methods
    */
    static StressModule* create();

    // Perform any necessary configuration prior to starting the stress test
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;

    // Called by the core system to create stress instances for this module
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "gamebrowser";}

    // Called when the stress tester determines how many connections the module will use out of the total pool
    void setConnectionDistribution(uint32_t conns) override;

    bool saveStats() override;

    /*
        GameBrowserModule methods
    */
    void releaseInstance(StressInstance *inst);

    bool parseConfig(const ConfigMap& config);

    Action getAction() const
    {
        return mAction;
    }

    void addMetric(Metric metric);

    GameManagerUtil* getGameManagerUtil() const { return mGameManager; }

    float getGameCreationFreq() const { return mCreationConnDist; }
    float getSearchFreq() const { return mSearchFrequency; }

    char8_t* getListConfigName() { return mListConfigName; }

    uint32_t getListLifespan() const { return mListLife; }

    float getListDestroyChance() const { return mListDestroyChance; }

    uint32_t getListUpdateMaxInterval() const { return mListUpdateMaxInterval; }

    bool getSnapshotListSync() const { return mSnapshotListSync; }

    bool getSubscriptonLists() const { return mSubscriptionLists; }

    float getListGetDataChance() const { return mListGetDataChance; }

    uint32_t getGameBrowserDelay() const { return mGameBrowserDelay; }

    int32_t getNumGameAttribs() const { return mNumGameAttribs; }

    uint32_t getGameListSize() const { return mGameListSize; }

    const GameAttributes& getGameAttributes() const { return mGameAttributes; }

    MatchmakerRulesUtil* getMatchmakerRulesUtil() const { return mMatchmakerRulesUtil; }

    void initCreateGameRequestOverride();
    void initUpdateGameNameRequestOverride(GameManagerUtilInstance &gameManagerInstance);
    
protected:
    GameBrowserModule();

    void printGBStat(const GameBrowserInstance &inst);
    void printAvgGBStats();
    void printAllGBStats();

    void initJoinGameRequestOverride();

private:
    typedef eastl::hash_map<int32_t, GameBrowserInstance*> InstanceMap;
    InstanceMap mInstances;

    float mCreationConnDist;
    float mSearchFrequency;
    uint32_t mRemainingCreatorCount;
    char8_t mListConfigName[100];
    uint32_t mListLife;
    float mListDestroyChance;
    uint32_t mListUpdateMaxInterval;
    bool mSnapshotListSync;
    bool mSubscriptionLists;
    float mListGetDataChance;
    int32_t mNumGameAttribs;
    uint32_t mGameListSize;

    uint32_t mGameBrowserDelay;

    Action mAction;

    GameManagerUtil *mGameManager;
    CreateGameRequest mCreateOverride;
    JoinGameRequest mJoinOverride;

    uint64_t mMetricCount[NUM_METRICS];

    GameAttributes mGameAttributes;

    DumpStats mDumpStats;

    MatchmakerRulesUtil *mMatchmakerRulesUtil;

}; // End GameBrowserModule Class


/*************************************************************************************************/
/*
    GameBrowserInstance
*/
/*************************************************************************************************/
class GameBrowserInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(GameBrowserInstance);
    friend class GameBrowserModule;

public:
    ~GameBrowserInstance() override;

    // This is the entry point for a stress instance to start running
    void start() override;

    //  asynchronous notification event handler.
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

    void onLogout(BlazeRpcError result) override;

    /*
        General accessors
    */
    GameBrowserGameSet getGameSet() const { return mGameSet; }

    GameBrowserListId getListId() const { return mListId; }

protected:
    GameBrowserInstance(GameBrowserModule *owner, StressConnection* connection, Login* login);

    void onDisconnected() override;
    void onLogin(BlazeRpcError result) override {}

    //Override this method to do your task
    BlazeRpcError execute() override;

    const char8_t *getName() const override;

    /*
        Game Browser RPC function wrappers
    */

    BlazeRpcError createGameBrowserList();

    BlazeRpcError destroyGameBrowserList();

    BlazeRpcError getGameData();

    BlazeRpcError getFullGameData();

    uint32_t getNumGamesInFirstUpdate() const { return mNumGamesInFirstUpdate; }
    bool hasReceivedFirstUdpate() const { return !mFirstUpdate; }
    uint64_t getFirstUpdateTimeMs() const { return mFirstUpdateTime.getMillis(); }


private:
    GameBrowserModule *mOwner;
    GameManagerSlave *mProxy;

    GameBrowserListId mListId;
    GameBrowserGameSet mGameSet;

    uint32_t mCurrentDelay;

    GameManagerUtilInstance *mGameManagerInstance;

    TimeValue mStartTime;
    TimeValue mListCreationTime;
    TimeValue mFirstUpdateTime;
    TimeValue mLastUpdateTime; // used for checking heartbeat
    uint32_t mNumGamesInFirstUpdate;
    bool mFirstUpdate;

    bool mIsSearcherInstance;

}; // End GameBrowserInstance Class


} // End Stress namespace
} // End Blaze namespace

#endif // BLAZE_STRESS_GAMEBROWSERMODULE_H
