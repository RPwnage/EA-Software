/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_STATSMODULE_H
#define BLAZE_STRESS_STATSMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "stressinstance.h"
#include "stats/statsconfig.h"
#include "stats/statscommontypes.h"
#include "stats/rpc/statsslave.h"

using namespace Blaze::Stats;

namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class StatsInstance;
class StatsModule;

// similar to LeaderboardCache's mechanism to walk thru the different keyscope combinations
typedef struct
{
    const char8_t* name;
    const ScopeStartEndValuesMap* valuesMap;
    ScopeStartEndValuesMap::const_iterator mapItr;
    ScopeValue aggregateValue; // KEY_SCOPE_VALUE_AGGREGATE if none
    ScopeValue numPossibleValues; // not including aggregate, if any
    ScopeValue currentValue;
} ScopeCell;

typedef eastl::vector<ScopeCell*> ScopeVector;

typedef eastl::vector_map<const StatCategory*, ScopeVector*> ScopeVectorMap;

typedef eastl::vector<const StatCategory*> CategoryList;

class StatsModule : public StressModule
{
    NON_COPYABLE(StatsModule);
    
public:

    ~StatsModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "stats";}
    bool parseConfig(const ConfigMap* config, const ConfigBootUtil* bootUtil);
    bool parseGameCategories(const ConfigMap* config, CategoryList* catList, const char8_t* name);
    
    int32_t getInt32(const ConfigMap* config, const char8_t* name, int32_t def, int32_t min, int32_t max);

    bool initScopeVectorMaps();
    void nextScopeNameValueMap(ScopeNameValueMap& scopeNameValueMap, const StatCategory* category, bool update = false);
    void getScopeNameValueMapByEntity(ScopeNameValueMap& scopeNameValueMap, const StatCategory* category, EntityId entity);

    static StressModule* create();

    const Stats::StatCategory* nextCategory();
    const Stats::StatLeaderboard* nextLeaderboard();
    EntityId getRandomEntityId();
    uint32_t getRandomPeriodOffset();
    int32_t getRandomPeriodType(const StatCategory* cat);

    const char8_t* nextLeaderboardFolderName();
    int32_t  getLBStartRank();
    const Stats::StatGroup* nextStatGroup();
    void fillLeaderboardRequestScopes(ScopeNameValueMap& scopeNameValueMap, const StatLeaderboard* lb);
    void fillStatGroupRequestScopes(ScopeNameValueMap& scopeNameValueMap, const StatGroup* sg);

    int32_t getRandSeed() {return mRandSeed;}
    int32_t getMaxPeriodOffset() {return mMaxPeriodOffset;}
    EntityId nextEntityId() { return ++mEntityId;}
    int32_t getRpcCount() {return mRpcCount;}
    const char8_t* getActionName() {return mActionName;}
    const Stats::StatsConfigData* getStatsConfig() const {return mStatsConfig;}

    uint32_t getMaxClubCount() {return mMaxClubCount;}
    uint32_t getMaxUserCount() {return mMaxUserCount;}


    const CategoryList* getNormalGames() { return &mNormalGames;}
    const CategoryList* getSoloGames() { return &mSoloGames;}
    const CategoryList* getOtpGames() { return &mOtpGames;}
    const CategoryList* getClubGames() { return &mClubGames;}
    const CategoryList* getSponsorEventGames() { return &mSponsorEventGames;}
    const char* getGlobalCategoryName() { return mGlobalCategoryName; }

private:
    friend class StatsInstance;
    typedef eastl::hash_map<int32_t,StatsInstance*> InstancesById;

    bool saveStats() override;

    ScopeVectorMap mScopeVectorMap;

    InstancesById mActiveInstances;
    int32_t mRandSeed;
    int32_t mMaxLbStartRank; 
    int32_t mMaxPeriodOffset;
    EntityId mEntityId;
    EntityId mMinEntityId;
    EntityId mMaxEntityId;
    int32_t mSeqEntityId;
    int32_t mRpcCount;
    
    int32_t mMaxCategoryCount;
    int32_t mSampleSize;
    int32_t mSampleIntervalCtr;
    uint32_t mMaxClubCount;
    uint32_t mMaxUserCount;

    const char* mGlobalCategoryName;

    char8_t mLogFileName[128];
    FILE* mLogFile;
    
    const char8_t* mActionName;
    
    CategoryList mNormalGames;
    CategoryList mSoloGames;
    CategoryList mOtpGames;
    CategoryList mClubGames;
    CategoryList mSponsorEventGames;

    const ConfigMap* mStatsConfigFile;
    Stats::StatsConfigData* mStatsConfig;
    Stats::CategoryMap::const_iterator mCatIter;
    Stats::CategoryMap::const_iterator mCatIterEnd;
    Stats::StatLeaderboardsMap::const_iterator  mLbIter;
    Stats::StatLeaderboardsMap::const_iterator  mLbIterEnd;
    Stats::StatGroupMap::const_iterator mStatGroupIter;
    Stats::StatGroupMap::const_iterator mStatGroupIterEnd;
    Stats::LeaderboardGroupsMap::const_iterator mLBGroupIter;
    Stats::LeaderboardGroupsMap::const_iterator mLBGroupIterEnd;
    Stats::LeaderboardFolderIndexMap::const_iterator mLBFolderIter;
    Stats::LeaderboardFolderIndexMap::const_iterator mLBFolderIterEnd;
    Stats::StatsConfig mStatsConfigTdf;

    StatsModule();
};

// ==================================================
class StatsInstance : public StressInstance
{
    NON_COPYABLE(StatsInstance);

public:
    StatsInstance(StatsModule* owner, StressConnection* connection, Login* login)
        : StressInstance(owner, connection, login, StatsSlave::LOGGING_CATEGORY),
          mOwner(owner),
          mProxy(BLAZE_NEW StatsSlave(*getConnection())),
          mRpcIndex(0)
    {
    }

    ~StatsInstance() override
    {
        delete mProxy;
    }
    
    void fillStats(UpdateStatsRequest* req, const StatCategory* cat, EntityId entity, ScopeNameValueMap* scopeOverride = nullptr);

    BlazeRpcError getLeaderboard(int32_t index);
    BlazeRpcError updateStats(int32_t index);
    BlazeRpcError deleteStats(int32_t index);
    BlazeRpcError getStatsByGroupBase(int32_t index, bool async);
    BlazeRpcError getStatsByGroup(int32_t index);
    BlazeRpcError getStatsByGroupAsync(int32_t index);
    BlazeRpcError getStats(int32_t index);
    BlazeRpcError getLeaderboardTree(int32_t index);
    BlazeRpcError getLeaderboardFolder(int32_t index);
    BlazeRpcError getStatGroup(int32_t index);
    BlazeRpcError getLeaderboardGroup(int32_t index);
    BlazeRpcError getCenteredLeaderboard(int32_t index);
    BlazeRpcError getFilteredLeaderboard(int32_t index);
    BlazeRpcError getUserSetLeaderboardBase(int32_t index, bool isInMemory); // getLeaderboard RPC using UserSets
    BlazeRpcError getUserSetLeaderboardCached(int32_t index);
    BlazeRpcError getUserSetLeaderboardUncached(int32_t index);
    BlazeRpcError getLeaderboardEntityCount(int32_t index);
    BlazeRpcError getEntityCount(int32_t index);
    BlazeRpcError updateNormalGameStats(int32_t index);
    BlazeRpcError updateClubGameStats(int32_t index);
    BlazeRpcError updateSoloGameStats(int32_t index);
    BlazeRpcError updateSponsorEventGameStats(int32_t index);
    BlazeRpcError updateOtpGameStats(int32_t index);
    BlazeRpcError getKeyScopesMap(int32_t index);
    BlazeRpcError getStatGroupList(int32_t index);
    BlazeRpcError getStatDescs(int32_t index);
    BlazeRpcError updateGlobalStats(int32_t index);
    BlazeRpcError getGlobalStats(int32_t index);

    const char8_t *getName() const override;

private:
    StatsModule* mOwner;
    StatsSlave* mProxy;
    int32_t mRpcIndex;
    BlazeRpcError execute() override;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_STATSMODULE_H

