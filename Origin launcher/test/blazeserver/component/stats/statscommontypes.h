/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSCOMMONTYPES_H
#define BLAZE_STATS_STATSCOMMONTYPES_H

/*** Include files *******************************************************************************/
#include "stats/tdf/stats_configtypes_server.h"
#include "stats/statstypes.h"
#include "stats/maxrankingstats.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{
/*************************************************************************************************/
/*!
    \brief StatCategory

    StatCategory encapsulates all of the data for a stat category.

*/
/*************************************************************************************************/
class StatCategory : public StatCategoryData
{
public:
    StatCategory(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : StatCategoryData(allocator, debugName), mStatMap(nullptr), mScopeNameSet(nullptr), mCategoryDependency(nullptr) { }

    ~StatCategory() override;

    void init(uint32_t id, EA::TDF::ObjectType entityType, StatMap* statMap, ScopeNameSet* scopeNameSet);

    uint32_t getId() const { return mCategoryId; }
    EA::TDF::ObjectType getCategoryEntityType() const { return mCategoryEntityType; }

    bool isValidPeriod(int32_t periodType) const  // bit field, lowest bit is alltime, then monthly, weekly, daily, etc.
    {
        return (periodType >= 0 && periodType < STAT_NUM_PERIODS) && (mPeriodTypes & (1 << periodType)) != 0;
    }

    StatMap* getStatsMap() { return mStatMap; }
    const StatMap* getStatsMap() const { return mStatMap; } 

    ///////////////////////////////////////////////////////
    // scope related
    //

    // if the category support scope
    bool hasScope() const { return (mScopeNameSet != nullptr); };
    bool isValidScopeNameSet(const ScopeNameValueMap* scopeNameValueMap) const;
    bool isValidScopeNameSet(const ScopeNameValueListMap* scopeNameValueListMap) const;
    bool isValidScopeNameSet(const ScopeNameValueListMap* scopeNameValueListMap, const ScopeNameValueListMap* inheritedScopeNameValueListMap) const;
    bool isValidScopeName(const char8_t* scopeName) const;

    const ScopeNameSet* getScopeNameSet() const { return mScopeNameSet;}

    /// @todo rename validateScopeForCategory() to isValidScopes()
    bool validateScopeForCategory(const StatsConfigData& config, const ScopeNameValueMap& scopeNameValueMap) const;

    const char8_t* getTableName(int32_t periodType) const { return mTableNames[periodType]; }

    CategoryDependency* getCategoryDependency() { return mCategoryDependency; }
    const CategoryDependency* getCategoryDependency() const { return mCategoryDependency; }
    void setCategoryDependency(CategoryDependency* catDep) { mCategoryDependency = catDep; }

private:

    uint32_t mCategoryId;
    EA::TDF::ObjectType mCategoryEntityType;  // type of entities in this category (users, clubs, etc.)
    StatMap* mStatMap;  // a map of Stat objects keyed by the name of the stat
    ScopeNameSet* mScopeNameSet; // collection of scope names for this category, null if a category does not have scope
    CategoryDependency* mCategoryDependency; // pointer because multiple categories may point to the same underlying object

    char8_t mTableNames[STAT_NUM_PERIODS][STATS_MAX_TABLE_NAME_LENGTH];

    void clean();
    StatCategory(const StatCategory& rhs);
    StatCategory& operator=(const StatCategory& rhs);
};

typedef EA::TDF::tdf_ptr<StatCategory> StatCategoryPtr;

/*************************************************************************************************/
/*!
    \brief StatGroup

    StatGroup encapsulates the core properties of multiple stats
    that are obtained either from the primary category, or the specific categories
    specified within each individual stat desc element stored within mStatDescs list.

*/
/*************************************************************************************************/
class StatGroup : public StatGroupData
{
public:
    StatGroup(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : StatGroupData(allocator, debugName), mStatCategory(nullptr) { }

    ~StatGroup() override;

    typedef eastl::vector<const StatDesc*> DescList;
        
    void init(const StatCategory* category, ScopeNameValueMap* scopeNameValueMap);

    struct Select
    {
        bool isCompatibleWith(const StatDesc& desc) const;
        int32_t getPeriodType() const;

        const StatCategory* mCategory; // never owned, never nullptr
        const ScopeNameValueMap* mScopes; // never owned, can be nullptr
        DescList mDescs; // never owned
    };

    typedef eastl::vector<Select> SelectList;

    void buildSelectableCategories();

    const StatCategory* getStatCategory() const { return mStatCategory; }
    const ScopeNameValueMap* getScopeNameValueMap() const { return mScopeNameValueMap; }
    const SelectList& getSelectList() const { return mSelectList; }

private:

    const StatCategory* mStatCategory;  // primary category this group is associated with
    /// @todo cleanup: replace ptr with object ???
    EA::TDF::tdf_ptr<ScopeNameValueMap> mScopeNameValueMap;  // list of scope name value pairs

    SelectList mSelectList;
    
    void clean();
    StatGroup(const StatGroup& rhs);
    StatGroup& operator=(const StatGroup& rhs);
};

typedef EA::TDF::tdf_ptr<StatGroup> StatGroupPtr;

class LeaderboardGroup : public LeaderboardGroupData
{
public:
    LeaderboardGroup(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : LeaderboardGroupData(allocator, debugName), mStatCategory(nullptr) { }

    ~LeaderboardGroup() override;

    typedef eastl::vector<const GroupStat*> GroupStatList;

    struct Select
    {
        Select() : mPrimary(false) {}
        bool isCompatibleWith(const GroupStat& desc) const;
        int32_t getPeriodType() const;

        const StatCategory* mCategory; // never owned, never nullptr
        const ScopeNameValueListMap* mScopes; // never owned, can be nullptr
        GroupStatList mDescs; // never owned

        bool mPrimary;
    };

    typedef eastl::vector<Select> SelectList;
    typedef eastl::hash_map<const GroupStat*, Select*> SelectMap;
    
    void init(const StatCategory* category);

    const StatCategory* getStatCategory() const { return mStatCategory; }

    void buildSelectableCategories();
    const SelectList& getSelectList() const { return mSelectList; }
    const SelectMap& getSelectMap() const { return mSelectMap; }

private:

    const StatCategory* mStatCategory;

    SelectList mSelectList; // each Select structure used to generate a single stat provider result set
    SelectMap mSelectMap; // map to find the stat in which select result set

    void clean();
    LeaderboardGroup(const LeaderboardGroup& rhs);
    LeaderboardGroup& operator=(const LeaderboardGroup& rhs);
};

typedef EA::TDF::tdf_ptr<LeaderboardGroup> LeaderboardGroupPtr;

class StatLeaderboard
{
public:

    StatLeaderboard(int32_t boardId,
                    const LeaderboardGroup* leaderboardGroup,
                    const StatLeaderboardTreeNode* leaderboardTreeNode,
                    const Stat* stats[MAX_RANKING_STATS],
                    bool cutoffStatValueDefined,
                    RankingStatValue cutoffStatValue
                    )
    :
    mLeaderboardGroup(leaderboardGroup),
    mLeaderboardTreeNode(leaderboardTreeNode),
    mBoardId(boardId),
    mKeyScopeLeaderboards(nullptr),
    mCutoffStatValueDefined(cutoffStatValueDefined),
    mCutoffStatValue(cutoffStatValue)
    {
        memcpy( mStats, stats, sizeof( mStats ) );
    }
        
    ~StatLeaderboard() {}

    const StatCategory* getStatCategory() const {return mLeaderboardGroup->getStatCategory();}
    const LeaderboardGroup* getLeaderboardGroup() const {return mLeaderboardGroup;}
    int32_t getPeriodType() const {return mLeaderboardTreeNode->getData().getPeriodType();}
    const char8_t* getDBTableName() const;
    const char8_t* getStatName() const {return mStats[0]->getName();}
    const char8_t* getRankingStatName(int i) const {return mStats[i]->getName();}
    const char8_t* getMetadata() const {return mLeaderboardTreeNode->getMetadata();}
    const char8_t* getBoardName() const {return mLeaderboardTreeNode->getName();}
    const char8_t* getDescription() const {return mLeaderboardTreeNode->getDesc();}
    const char8_t* getShortDescription() const {return mLeaderboardTreeNode->getShortDesc();}

    bool isAscending() const {return mLeaderboardTreeNode->getAscending();}
    bool isTiedRank() const {return mLeaderboardTreeNode->getTiedRank();}
    bool isInMemory() const {return mLeaderboardTreeNode->getInMemory();}
    uint32_t getSize() const {return mLeaderboardTreeNode->getData().getSize();}
    uint32_t getExtra() const {return mLeaderboardTreeNode->getData().getExtra();}
    const ScopeNameValueListMap* getScopeNameValueListMap() const { return mLeaderboardTreeNode->getData().getScopeNameValueListMap();}
    const Stat* getStat() const {return mStats[0];}
    const Stat* getRankingStat(int i) const {return mStats[i];}
    const RankingStatList& getRankingStats() const { return mLeaderboardTreeNode->getMultiStatRanking(); }
    int32_t getRankingStatsSize() const { return getRankingStats().size(); }
    bool hasSecondaryRankings() const { return getRankingStats().size() > 1; }
    bool getHasScopes() const {return mLeaderboardTreeNode->getData().getScopeNameValueListMap() != nullptr && mLeaderboardTreeNode->getData().getScopeNameValueListMap()->size() != 0;}
    int32_t getBoardId() const {return mBoardId;}
    KeyScopeLeaderboards* getKeyScopeLeaderboards() {return mKeyScopeLeaderboards;}
    const KeyScopeLeaderboards* getKeyScopeLeaderboards() const {return mKeyScopeLeaderboards;}

    bool appendScopeToDBQuery(char8_t* scopeQuery, size_t destLen);
    void setKeyScopeLeaderboards(KeyScopeLeaderboards* kslb) {mKeyScopeLeaderboards = kslb;}

    bool isCutoffStatValueDefined() const {return mCutoffStatValueDefined;}
    int64_t getCutoffStatValueInt() const {return mCutoffStatValue.intVal;}
    float_t getCutoffStatValueFloat() const {return mCutoffStatValue.floatVal;}
    
    bool isIntCutoff(int64_t val) const
    { 
        if (!mCutoffStatValueDefined) return false;
        if (isAscending())
        {
            if (val > mCutoffStatValue.intVal) return true;
        }
        else
        {
            if (val < mCutoffStatValue.intVal) return true;
        }        
        
        return false;
    }
    
    bool isFloatCutoff(float_t val) const
    { 
        if (!mCutoffStatValueDefined) return false;
        if (isAscending())
        {
            if (val > mCutoffStatValue.floatVal) return true;
        }
        else
        {
            if (val < mCutoffStatValue.floatVal) return true;
        }        
        
        return false;
    }
    
private:
    const LeaderboardGroup* mLeaderboardGroup;
    const StatLeaderboardTreeNode* mLeaderboardTreeNode;
    const Stat* mStats[MAX_RANKING_STATS];
    int32_t mBoardId;
    KeyScopeLeaderboards* mKeyScopeLeaderboards;
    bool mCutoffStatValueDefined;
    RankingStatValue mCutoffStatValue;     
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSCOMMONTYPES_H
