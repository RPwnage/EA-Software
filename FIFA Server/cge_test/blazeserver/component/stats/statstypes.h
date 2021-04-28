/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSTYPES_H
#define BLAZE_STATS_STATSTYPES_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/vector_set.h"
#include "EASTL/list.h"
#include "EASTL/map.h"
#include "EASTL/vector_map.h"
#include "EASTL/set.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/expression.h"
#include "stats/tdf/stats_commontypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{
// Forward delcarations
class Expression;
class StatsConfigData;
class KeyScopeLeaderboards;

typedef eastl::list<int32_t> PeriodIdList;

const char8_t STAT_PERIOD_NAMES[STAT_NUM_PERIODS][16] = {"alltime", "monthly", "weekly", "daily"};

enum StatType
{
    STAT_TYPE_INT = Blaze::EXPRESSION_TYPE_INT,
    STAT_TYPE_FLOAT = Blaze::EXPRESSION_TYPE_FLOAT,
    STAT_TYPE_STRING = Blaze::EXPRESSION_TYPE_STRING
};

/*************************************************************************************************/
/*!
    This class represents a stat or keyscope column type.  It contains the base type and the size
    of that type.  It is used to translate between internal blaze type usage and the actual
    database column type used to store the value.
*/
/*************************************************************************************************/
class StatTypeDb
{
public:
    StatTypeDb(StatType type, uint32_t size) : mType(type), mSize(size) { }

    StatTypeDb() : mType(STAT_TYPE_INT), mSize(4) { }

    bool operator==(const StatTypeDb& rhs) const
    {
        return ((mType == rhs.mType) && (mSize == rhs.mSize));
    }

    bool operator!=(const StatTypeDb& rhs) const
    {
        return !operator==(rhs);
    }

    void set(StatType type, uint32_t size)
    {
        mType = type;
        mSize = size;
    }
    bool isValidSize() const;

    StatType getType() const { return mType; }
    uint32_t getSize() const { return mSize; }
    char8_t* getDbTypeString(char8_t* buf, size_t len) const;

    static StatTypeDb getTypeFromDbString(const char8_t* type, uint32_t length);

private:
    StatType mType;
    uint32_t mSize;
};

const uint16_t INVALID_STAT_ID = 0;
static const float RANK_TABLE_EXTRA = 0.1f; // percentage based extra rows to cache for a leaderboard

const uint32_t STATS_DEFAULT_LRU_SIZE = 1024;
// May be used as a limit in db queries, a config variable may overwrite it
const uint32_t STATS_MAX_ROWS_TO_RETURN = 1024;
const uint32_t STATS_MAX_TABLE_NAME_LENGTH = 128;
const uint32_t LB_MAX_TABLE_NAME_LENGTH = STATS_MAX_TABLE_NAME_LENGTH + 3; //we append 3 chars to the stats table name to create the lb table name

// Forward declarations
class Stat;
class StatDesc;
class StatCategory;
class StatGroup;

typedef union 
{
    int64_t intVal;
    float_t floatVal;
} RankingStatValue;

typedef struct StatVal
{
    union
    {
        int64_t intVal;
        float_t floatVal;
        const char8_t* stringVal;
    };
    // whether or not this stat is included in a broadcast update
    bool changed; 
    // this type is used when the value in StatVal struct needs to be sent outside stats component.
    // stats component can decide the type based on stats config file, but other components need a type to decide the datatype.
    uint32_t type; 

    int32_t stringcmp(const char8_t* pString)
    {
        if (stringVal == nullptr)
        {
            if (pString != nullptr)
            {
                return -(*pString);
            }
            // else both are nullptr
            return 0;
        }
        else
        {
            if (pString == nullptr)
            {
                return *stringVal;
            }
        }
        return blaze_strcmp(stringVal, pString);
    }

    StatVal() : changed(false),type(STAT_TYPE_INT) { }
} StatVal;
typedef eastl::hash_map<const char8_t*, StatVal*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StatValMap;
typedef eastl::hash_map<const Stat*, StatVal*> StatPtrValMap;

typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StringSet;
typedef StringSet::const_iterator StringSetIter;

typedef struct StringStatValue
{
    char8_t buf[STATS_STATVALUE_LENGTH];

    StringStatValue()
    {
        buf[0] = '\0';
    }
} StringStatValue;
typedef eastl::list<StringStatValue> StringStatValueList;

typedef eastl::map<uint32_t, StatCategory*> CategoryIdMap;
typedef eastl::hash_map<const char8_t*, EA::TDF::tdf_ptr<Stat>, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StatMap;
typedef eastl::hash_map<const char8_t*, EA::TDF::tdf_ptr<StatCategory>, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > CategoryMap;
typedef eastl::hash_map<const char8_t*, EA::TDF::tdf_ptr<StatGroup>, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StatGroupMap;

typedef eastl::vector_set<ScopeName, EA::TDF::TdfStringCompareIgnoreCase> ScopeNameSet;

typedef eastl::vector_set<ScopeValue> ScopeValueSet;
typedef eastl::map<ScopeName, ScopeValueSet*> ScopeNameValueSetMap;

struct ScopeNameValueSetMapWrapper : public ScopeNameValueSetMap
{
    ~ScopeNameValueSetMapWrapper()
    {
        for (ScopeNameValueSetMap::const_iterator i = begin(), e = end(); i != e; ++i)
        {
            delete i->second;
        }
    }
};

bool validateFormat(StatType statType, const char8_t* format, int32_t typeSize, char8_t* rFormat , size_t rFormatSize);

// The DependencyMap is a map of category name keys to stat set values, it encapsulates all the stats that a given derived
// stat depends upon, we choose to key the data by category name for ease of figuring out the overall dependencies at a category level
typedef eastl::hash_set<const Stat*> StatSet;
typedef eastl::hash_map<const char8_t*, StatSet, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > DependencyMap;

// A simple category and stat name tuple
typedef struct CategoryStat
{
    const char8_t* categoryName;
    const char8_t* statName;
} CategoryStat;
typedef eastl::list<CategoryStat> CategoryStatList;

// This struct wraps two things that help us with multi-category derived stats, first a string set of category names,
// this is a collection of all categories whose stats need to be fetched together because of inter-category dependencies,
// these dependencies may be direct or indirect (e.g. category A has a stat that depends on category B which in turn has
// a stat that depends on category C).  Second, an ordered list of all derived stats that exist inside of the categories
// contained in catSet - they should be iterated over in order during stats update calls, and each formula that has a
// dependence on one or more stats modified during execution should be evaluated to determine the new derived value.
typedef struct CategoryDependency
{
    StringSet catSet;
    CategoryStatList catStatList;
} CategoryDependency;
typedef eastl::hash_set<CategoryDependency*> CategoryDependencySet;

class StatDependencyContainer : public DependencyContainer
{
public:
    StatDependencyContainer(DependencyMap* dependencyMap, const CategoryMap* categoryMap, const char8_t* categoryName)
    : mDependencyMap(dependencyMap), mCategoryMap(categoryMap), mCategoryName(categoryName) {}
    void pushDependency(void* mContext, const char8_t* nameSpaceName, const char8_t* name) override;

private:
    DependencyMap* mDependencyMap;
    const CategoryMap* mCategoryMap;
    const char8_t* mCategoryName;
};

struct scope_index_map_less : public eastl::binary_function<const ScopeNameValueMap*, const ScopeNameValueMap*, bool>
{
    bool operator()(const ScopeNameValueMap* a, const ScopeNameValueMap* b) const
    {
        // ScopeNameValueMap is a vector map with keys ordered alphabetically
        // and we may just compare them sequentually. 
        // Maps are for the same category i.e. keys are expected to be the same 
        // and this was validated during parsing
        // nullptr and empty map are equal and both implies no keyscopes
        if (a == nullptr || b == nullptr) 
            return false;
            
        if (a->size() == 0 || b->size() == 0)
            return false;

        ScopeNameValueMap::const_iterator ita = a->begin();
        ScopeNameValueMap::const_iterator itae = a->end();
        ScopeNameValueMap::const_iterator itb = b->begin();
        ScopeNameValueMap::const_iterator itbe = b->end();
        for (; ita != itae; ++ita, ++itb)
        {   // never should happen
            if (itb == itbe) 
                return false;
            if (ita->second > itb->second)
                return false;
            if (ita->second < itb->second)
                return true;
        }
                
        return false;
    }
};

class Stat : public StatData
{
public:
    Stat(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : StatData(allocator, debugName), mDerivedExpression(nullptr), mCategory(nullptr) { }

    ~Stat() override;

    void init(const StatCategory* category, const StatTypeDb& type, const char8_t* format,
        int64_t defaultIntVal, float_t defaultFloatVal, const char8_t* defaultStringVal);

    const StatCategory& getCategory() const { return *mCategory; }
    const char8_t* getQualifiedName() const { return mQualifiedName; }

    int32_t getTypeSize() const { return mTypeDb.getSize(); }
    const StatTypeDb& getDbType() const { return mTypeDb; }

    int64_t getDefaultIntVal() const { return mDefaultIntVal; }
    float_t getDefaultFloatVal() const { return mDefaultFloatVal; }
    const char8_t* getDefaultStringVal() const { return mDefaultStringVal; }

    const char8_t* getStatFormat() const { return mStatFormat; }

    DependencyMap* getDependencies() { return &mDependencies; }
    const DependencyMap* getDependencies() const { return &mDependencies; }

    void setDefaultIntVal(int64_t val) { mDefaultIntVal = val; initDefaultValAsString(); }
    void setDefaultFloatVal(float_t val) { mDefaultFloatVal = val; initDefaultValAsString(); }
    void setDefaultStringVal(const char8_t* val) { blaze_strnzcpy( mDefaultStringVal, val, sizeof(mDefaultStringVal)); initDefaultValAsString(); }
    const char8_t* getDefaultValAsString() const { return mDefaultValueAsFormattedString; } // Returns the cached default value of a stat.
    void extractDefaultRawVal(StatRawValue& statRawVal) const;

    bool isDerived() const { return mDerivedExpression != nullptr; }
    void setDerivedExpression(Blaze::Expression* derivedExpression) { mDerivedExpression = derivedExpression; }
    const Blaze::Expression* getDerivedExpression() const { return mDerivedExpression; }

    bool isValidIntVal(int64_t val) const;

private:
    
    char8_t mQualifiedName[STATS_QUALIFIED_STAT_NAME_LENGTH];  // a string like "<categoryName>:<statName>"
    StatTypeDb mTypeDb;  // data type used to store the stat in the DB
    int64_t mDefaultIntVal;  // default value for this stat only if it is an int
    float_t mDefaultFloatVal;  // default value for this stat only if it is a float
    char8_t mDefaultStringVal[STATS_STATVALUE_LENGTH];  // default value for this stat only if it is a string
    char8_t mStatFormat[STATS_FORMAT_LENGTH];  // stat format string
    char8_t mDefaultValueAsFormattedString[STATS_STATVALUE_LENGTH];  // default value for this stat as formatted string

    Blaze::Expression* mDerivedExpression;
    DependencyMap mDependencies;
    const StatCategory* mCategory;  // parent category of the stat

   
    void initDefaultValAsString();
    void clean();
    Stat(const Stat& rhs);
    Stat& operator=(const Stat& rhs);
};

class StatDesc : public StatDescData
{
public:
    StatDesc(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : StatDescData(allocator, debugName), mStat(nullptr), mGroup(nullptr), mScopeMap(nullptr) { }

    ~StatDesc() override;
    
    void init(const Stat* stat, const StatGroup* group, const ScopeNameValueMap* scopeMap,
        int32_t periodType, const char8_t* format, int32_t index);

    const Stat* getStat() const { return mStat; }
    StatType getType() const { return mStat->getDbType().getType(); }
    int32_t getIndex() const { return mIndex; }
    int32_t getStatPeriodType() const { return mStatPeriodType; }
    const char8_t* getStatFormat() const { return mStatFormat; }
    const StatGroup& getGroup() const { return *mGroup; }
    const ScopeNameValueMap* getScopeNameValueMap() const { return mScopeMap; }

private:
   

    const Stat* mStat;  // stat that this description describes
    const StatGroup* mGroup;  // parent stat group of this stat
    EA::TDF::tdf_ptr<const ScopeNameValueMap> mScopeMap;  //optional scope name value pairs associated with this stat desc
    int32_t mStatPeriodType;  // optional period type value associated with this stat desc
    int32_t mIndex;  // index value describing the order of stat descs within the group
    char8_t mStatFormat[STATS_FORMAT_LENGTH ];  // a printf style formatting string used to convert stat value to string

   
    void clean();
    StatDesc(const StatDesc& rhs);
    StatDesc& operator=(const StatDesc& rhs);
};

// ----------------------- Leaderboards ---------------------------
class GroupStat;
class LeaderboardGroup;
class StatLeaderboardTreeNode;
class StatLeaderboard;

typedef eastl::hash_map<const char8_t*, EA::TDF::tdf_ptr<LeaderboardGroup>, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > LeaderboardGroupsMap;
typedef eastl::hash_map<const char8_t*, uint32_t, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > LeaderboardIndexMap;
typedef eastl::hash_map<int32_t, StatLeaderboard*> StatLeaderboardsMap;

class GroupStat : public StatDescData
{
public:
    GroupStat(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "")
        : StatDescData(allocator, debugName), mStat(nullptr), mGroup(nullptr), mScopeMap(nullptr) { }

    ~GroupStat() override;

    void init(const Stat* stat, const LeaderboardGroup* group, const EA::TDF::tdf_ptr<ScopeNameValueListMap>& scopeMap,
        int32_t periodType, const char8_t* format, int32_t index);

    const Stat* getStat() const { return mStat; }
    int32_t getIndex() const { return mIndex; }
    const LeaderboardGroup& getGroup() const { return *mGroup; }
    int32_t getStatPeriodType() const { return mStatPeriodType; }
    const char8_t* getStatFormat() const { return mStatFormat; }
    const ScopeNameValueListMap* getScopeNameValueListMap() const { return mScopeMap; }

    GroupStat(const GroupStat& rhs);
    GroupStat& operator=(const GroupStat& rhs);

private:
    
    const Stat* mStat;  // stat that this description describes
    const LeaderboardGroup* mGroup;  // parent stat group of this stat
    EA::TDF::tdf_ptr<ScopeNameValueListMap> mScopeMap;  // optional scope name value pairs associated with this stat desc
    int32_t mStatPeriodType;  // optional period type value associated with this stat desc
    int32_t mIndex;  // index value describing the order of stat descs within the group
    char8_t mStatFormat[STATS_FORMAT_LENGTH];  // a printf style formatting string used to convert stat value to string
    
   
    void clean();
};

class StatLeaderboardData : public StatLeaderboardBaseData
{
public:    
    StatLeaderboardData(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugMemName = "")
        : StatLeaderboardBaseData(allocator, debugMemName), mScopeNameValueListMap(nullptr), mPeriodType(0),
        mSize(0), mExtra(0), mLevel(0), mParent(0), mFirstChild(0), mChildCount(0) { }
    
    ~StatLeaderboardData() override;
        
     void init(ScopeNameValueListMap* scopeNameValueListMap, int32_t periodType, int32_t size, int32_t extra, int32_t level);

    int32_t getLevel() const {return mLevel;}
    int32_t getParent() const {return mParent;}
    int32_t getPeriodType() const {return mPeriodType;}
    int32_t getSize() const {return mSize;}
    int32_t getExtra() const {return mExtra;}
    int32_t getFirstChild() const {return mFirstChild;}
    int32_t getChildCount() const {return mChildCount;}

    const ScopeNameValueListMap* getScopeNameValueListMap() const { return mScopeNameValueListMap;}
    void setScopeNameValueListMap(ScopeNameValueListMap* map) { mScopeNameValueListMap = map;}

    void setLevel(int32_t level) {mLevel = level;}
    void setParent(int32_t parent) {mParent = parent;}
    void setPeriodType(int32_t periodType) {mPeriodType = periodType;}
    void setSize(int32_t size) {mSize = size;}
    void setExtra(int32_t extra) {mExtra = extra;}
    void setFirstChild(int32_t firstchild) {mFirstChild = firstchild;}
    void setChildCount(int32_t count) {mChildCount = count;}

private:
    
    EA::TDF::tdf_ptr<ScopeNameValueListMap> mScopeNameValueListMap;
    int32_t mPeriodType;
    int32_t mSize;
    int32_t mExtra;
    int32_t mLevel;
    int32_t mParent;
    int32_t mFirstChild;
    int32_t mChildCount;

   
    void clean();
    StatLeaderboardData(const StatLeaderboardData& rhs);
    StatLeaderboardData& operator=(const StatLeaderboardData& rhs);
};

/**
 * ScopeNameIndex
 */
struct ScopeNameIndex
{
    ScopeName mScopeName;
    ScopeValue mScopeValue;
};

typedef eastl::vector<ScopeNameIndex>  RowScopesVector;
typedef eastl::vector<RowScopesVector> ScopesVectorForRows;

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSTYPES_H
