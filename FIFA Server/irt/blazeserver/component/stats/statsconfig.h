/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSCONFIG_H
#define BLAZE_STATS_STATSCONFIG_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector_map.h"
#include "shard.h"
#include "statscommontypes.h"
#include "stats/tdf/stats_server.h"
#include "framework/util/shared/blazestring.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
// Forward declarations
class StatCategory;

typedef eastl::hash_map<const char8_t*, StatCategoryPtr, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > CategoryMap;

struct str_less_than : public eastl::binary_function<char8_t*, char8_t*, bool>
{
    bool operator()(const char8_t* a, const char8_t* b) const
        { return blaze_stricmp(a, b) < 0; }
};    
typedef eastl::vector_map<const char8_t*, uint32_t, str_less_than>  LeaderboardFolderIndexMap;

// a bitwise enum used by different cases to show which reserved keyword(s) is(are) supported 
enum ScopeReservedValue
{
    SCOPE_RESERVED_TOTAL             = 0x01,
    SCOPE_RESERVED_EVERYTHING        = 0x02,
    SCOPE_RESERVED_USER_SPECIFIED    = 0x04,
    SCOPE_RESERVED_ALL               = SCOPE_RESERVED_TOTAL|SCOPE_RESERVED_EVERYTHING|SCOPE_RESERVED_USER_SPECIFIED
};

class StatsConfigData
{
public:

    StatsConfigData()
    : mHasMultiCategoryDerivedStats(false),
    mLeaderboardGroupsMap(nullptr),
    mStatLeaderboardsMap(nullptr),
    mStatLeaderboardTree(nullptr),
    mConfig(nullptr)
    {
    }

    ~StatsConfigData();
    void cleanStatsConfig();

    // parse all
    bool parseStatsConfig(const StatsConfig& config);
    bool parseStatsConfig(const StatsConfig& config, ConfigureValidationErrors& validationErrors);

    bool parseCategoryStats(const StatCategory& category, StatMap& statMap, StatList& stats, ConfigureValidationErrors& validationErrors);
    bool parseDerivedStats(ConfigureValidationErrors& validationErrors);
    bool parseCategories(ConfigureValidationErrors& validationErrors);
    bool parseStatDescs(const StatGroup& group, StatDescList& descDataList, ConfigureValidationErrors& validationErrors);
    bool parseStatGroups(ConfigureValidationErrors& validationErrors);

    //shards
    bool initShards();

    // keyscopes
    bool parseKeyScopes(const StatsConfig&  configMap);
    bool parseKeyScopes(ConfigureValidationErrors& validationErrors);
    bool parseKeyScopeItem(const char8_t* keyScopeItemName, KeyScopeItem& keyScopeItem, ConfigureValidationErrors& validationErrors);
    bool parseKeyScopeValues(ScopeStartEndValuesMap& keyScopeStartEndValuesMap, ConfigureValidationErrors& validationErrors);
    bool parseKeyScopeLeaderboardValues(const char8_t* keyScopeItemName, const ScopeValueList& keyScopeValuesSequence, ScopeStartEndValuesMap& keyScopeStartEndValuesMap, ConfigureValidationErrors& validationErrors);

    // Leaderboards
    int32_t parseType(const char8_t* type);
    bool parseLeaderboardStatDescs(const LeaderboardGroup& group, LbStatList& descDataList, ConfigureValidationErrors& validationErrors);
    bool parseLeaderboardGroups(ConfigureValidationErrors& validationErrors);
    bool parseLeaderboardHierarchy(ConfigureValidationErrors& validationErrors);

    const LeaderboardGroup* getLeaderboardGroup(const char8_t* name) const;
    const LeaderboardGroupsMap* getLeaderboardGroupsMap() const {return mLeaderboardGroupsMap;}
    const StatLeaderboardsMap* getStatLeaderboardsMap() const { return mStatLeaderboardsMap;}
    
    typedef eastl::vector<Blaze::Stats::StatLeaderboardTreeNodePtr> StatLeaderboardTree;
    int32_t parseNextSequence(int32_t level, StatLeaderboardTree* treeList, const LeaderboardHierarchyList& prevSequence, ConfigureValidationErrors& validationErrors);

    StatLeaderboardTreeNodePtr* getStatLeaderboardTree() const {
        return mStatLeaderboardTree->data();
    }
    uint32_t getStatLeaderboardTreeSize() const {
        return static_cast<uint32_t>(mStatLeaderboardTree->size());
    }

    const StatLeaderboard* getStatLeaderboard(uint32_t boardId, const char8_t* boardName) const;

    static bool validateConfigLeaderboardHierarchy(const StatsConfig& config, const StatsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors);

    uint32_t getLruSize() const {return mConfig->getLruSize();}
    uint32_t getMaxRowsToReturn() const { return mConfig->getMaxRowsToReturn();}
    uint32_t getMaxKeyScopeGeneratedLeaderboards() const { return mConfig->getMaxKeyScopeGeneratedLeaderboards();}
    TimeValue getStatUpdateSequenceTimeout() const { return mConfig->getStatUpdateSequenceTimeout(); }
    TimeValue getStatUpdateSweepInterval() const { return mConfig->getStatUpdateSweepInterval(); }
    TimeValue getTrimPeriodsMaxProcessingTime() const { return mConfig->getTrimPeriodsMaxProcessingTime(); }
    TimeValue getTrimPeriodsTimeout() const { return mConfig->getTrimPeriodsTimeout(); }

    const CategoryIdMap* getCategoryIdMap() const { return &mCategoryIdMap; }
    const CategoryMap* getCategoryMap() const { return &mCategoryMap; }
    const StatGroupMap* getStatGroupMap() const { return &mGroupMap; }
    const StatGroupsList& getStatGroupList() const { return mConfig->getStatGroups(); }
    const KeyScopesMap& getKeyScopeMap() const { return mConfig->getKeyscopes();}

    bool hasMultiCategoryDerivedStats() const { return mHasMultiCategoryDerivedStats; }

    const LeaderboardFolderIndexMap* getLeaderboardFolderIndexMap() const {
        return &mFolderIndexMap;
    }

    const ShardConfiguration& getShardConfiguration() const { return mShardConfiguration; }

    // keyscopes helper functions
    // ST_TODO may optimize via cache all values when we parsing the config
    ScopeValue getAggregateKeyValue(const char8_t* scopeName) const;
    EA::TDF::tdf_ptr<ScopeNameValueMap> parseUserStatsScope(const ScopeMap& scopeMap, ConfigureValidationErrors& validationErrors);
    StatTypeDb getScopeDbType(const char8_t* scopeName) const;

    // keyscopes validation helper functions
    bool isNumber(const char8_t* value);
    bool isValidScopeValue(const char8_t* scopeName, const ScopeValue keyScopeValue) const;
    ScopeValue getKeyScopeSingleValue(const ScopeStartEndValuesMap& scopeValues) const;

    void setStatsConfig(const StatsConfig& config) { config.copyInto(mConfigCopy); mConfig = &mConfigCopy; }

private:
    EA::TDF::tdf_ptr<ScopeNameValueMap> parseScopeSetting(const ScopeMap& scopeMap, const uint16_t validReservedKeywords, ConfigureValidationErrors& validationErrors);
    EA::TDF::tdf_ptr<ScopeNameValueMap> parseStatGroupScope(const ScopeMap&  scopeMap, ConfigureValidationErrors& validationErrors);
    EA::TDF::tdf_ptr<ScopeNameValueListMap> parseLeaderboardScope(const ScopeMap& scopeMap, const uint16_t validReservedKeywords, ConfigureValidationErrors& validationErrors);
    EA::TDF::tdf_ptr<ScopeNameValueListMap> parseLeaderboardScopes(const ScopesMap& scopeMap, ConfigureValidationErrors& validationErrors);
    void resolveStatType(const char8_t* nameSpace, const char8_t* name, void* categoryName, Blaze::ExpressionValueType& type);
    void resolveStatDefaultValue(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
                                 const void* context, Blaze::Expression::ExpressionVariableVal& val);
    bool testNodeName(StatLeaderboardTree* statLeaderboardTree, const char8_t* name);
    static bool validateConfigLeaderboardHierarchyNode(const StatLeaderboardTreeNode* referNode, const LeaderboardHierarchyList& lbList, ConfigureValidationErrors& validationErrors);
    bool isIdentifierChar(char8_t ch);

private:

    ShardConfiguration mShardConfiguration;

    // category
    CategoryIdMap mCategoryIdMap;
    CategoryMap mCategoryMap;
    StatGroupMap mGroupMap;

    bool mHasMultiCategoryDerivedStats;
    CategoryDependencySet mCategoryDependencySet;
    
    // Leaderboards
    LeaderboardGroupsMap* mLeaderboardGroupsMap;
    StatLeaderboardsMap* mStatLeaderboardsMap; 
    
    StatLeaderboardTree *mStatLeaderboardTree;
    LeaderboardIndexMap mLeaderboardIndexMap;

    // LeaderboardFolders
    LeaderboardFolderIndexMap mFolderIndexMap;

    // config
    StatsConfig mConfigCopy;    // Internally held copy because we decided to modify the values for some reason. 
    StatsConfig* mConfig;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSCONFIG_H
