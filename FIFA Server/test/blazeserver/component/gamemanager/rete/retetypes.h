/*! ************************************************************************************************/
/*!
\file retetypes.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETETYPES_H
#define BLAZE_GAMEMANAGER_RETETYPES_H

#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/vector_set.h"
#include "framework/blazeallocator.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class ProductionNode;
    class ReteRule;

    typedef uint64_t ProductionId;
    typedef uint64_t WMEId;
    typedef uint64_t WMEAttribute;

    typedef eastl::list<WMEAttribute> WMEAttributeList;
    typedef eastl::list<ProductionNode*> ProductionNodeList;
    typedef eastl::vector<const ReteRule*> MatchAnyRuleList;

    // re define this here to full separate RETE from matchmaking
    typedef uint32_t FitScore;

    class JoinNodeIntrusiveListNode : public eastl::intrusive_list_node 
    {
    public:
        JoinNodeIntrusiveListNode() : mIsGarbageCollectible(false) { mpNext = nullptr; mpPrev = nullptr; }
        bool isGarbageCollectible() { return mIsGarbageCollectible; }
        void setGarbageCollectible() 
        { 
            mIsGarbageCollectible = true; 
            mTimeStamp = EA::TDF::TimeValue::getTimeOfDay();
        }
        void clearGarbageCollectible() 
        { 
            mIsGarbageCollectible = false; 
            mTimeStamp = 0;
        }
        EA::TDF::TimeValue getTimeStamp() const { return mTimeStamp; }
    private:
        bool mIsGarbageCollectible;
        EA::TDF::TimeValue mTimeStamp;
    };
    typedef eastl::intrusive_list<JoinNodeIntrusiveListNode> JoinNodeIntrusiveList;

    class ProductionNodeIntrusiveListNode : public eastl::intrusive_list_node
    {
    public:
        ProductionNodeIntrusiveListNode() : mIsGarbageCollectible(false) { mpNext = nullptr; mpPrev = nullptr; }
        bool isGarbageCollectible() { return mIsGarbageCollectible; }
        void setGarbageCollectible() 
        { 
            mIsGarbageCollectible = true; 
            mTimeStamp = EA::TDF::TimeValue::getTimeOfDay();
        }
        void clearGarbageCollectible() 
        { 
            mIsGarbageCollectible = false; 
            mTimeStamp = 0;
        }
        EA::TDF::TimeValue getTimeStamp() const { return mTimeStamp; }
    private:
        bool mIsGarbageCollectible;
        EA::TDF::TimeValue mTimeStamp;
    };
    typedef eastl::intrusive_list<ProductionNodeIntrusiveListNode> ProductionNodeIntrusiveList;


    const uint64_t WME_ANY_HASH = UINT64_MAX;
    const uint64_t WME_UNSET_HASH = UINT64_MAX - 1;

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.
#define NODE_POOL_NEW_DELETE_OVERLOAD() public:\
    /* Node Pool New */ \
    void * operator new (size_t size, NodePoolAllocator& pool) { return pool.allocate(size); } \
    /* Node Pool Delete */ \
    void operator delete(void* ptr, NodePoolAllocator& pool) { if(ptr) pool.deallocate(ptr); } \
private: \
    /* don't allow WME's to be created except by the node pool. */ \
    void * operator new (size_t size) { EA_FAIL(); return ::operator new(size); } \
    void operator delete(void* ptr) { EA_FAIL(); ::operator delete(ptr); }

    class WorkingMemoryElement
    {
        NON_COPYABLE(WorkingMemoryElement);
        NODE_POOL_NEW_DELETE_OVERLOAD();
    public:
        WorkingMemoryElement() : mId(0), mName(WME_ANY_HASH), mValue(WME_ANY_HASH) {}
        WorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value)
            : mId(id),
            mName(name),
            mValue(value)
        { }

        bool equals(const WorkingMemoryElement& rhs) const { return (getId() == rhs.getId()) && (getName() == rhs.getName()) && (getValue() && rhs.getValue()); }

        WMEId getId() const { return mId; }
        void setId(WMEId id) { mId = id; }

        WMEAttribute getName() const { return mName; }
        void setName(WMEAttribute name) { mName = name; }

        WMEAttribute getValue() const { return mValue; }
        void setValue(WMEAttribute value) { mValue = value; }

    private:
        WMEId mId;
        WMEAttribute mName;
        WMEAttribute mValue;
    };


    class ConditionProvider
    {
    public:
        virtual ~ConditionProvider() {}
        virtual const char8_t* getProviderName() const = 0;
        virtual const uint32_t getProviderSalience() const = 0;
        virtual const FitScore getProviderMaxFitScore() const = 0;
    };


    class ConditionProviderDefinition
    {
    public:
        virtual ~ConditionProviderDefinition() {}
        virtual uint32_t getID() const = 0;
        virtual const char8_t* getName() const = 0;

        virtual uint64_t reteHash(const char8_t* str, bool garbageCollectable = false) const = 0;
        virtual const char8_t* reteUnHash(WMEAttribute value) const = 0;

        virtual WMEAttribute getCachedWMEAttributeName(const char8_t* attributeName) const = 0;
    };

    const int64_t INVALID_VALUE = INT64_MAX;

    // Simple rolling unique id.  Not unique across servers. 
    typedef uint64_t ConditionId;
    static const ConditionId INVALID_CONDITION_ID = 0;
    class Condition
    {

    public:
        Condition() {}

        /*! ************************************************************************************************/
        /*! \brief Constructor for hashed value conditions with negate and substring options.
            Hashed conditions rely on the integrating code (in MM's case, the rules) to hash incoming values to a WMEAttribute
            Hashed values are the simply equality compared.
        ***************************************************************************************************/
        Condition(const char8_t* attributeName, WMEAttribute value, const ConditionProviderDefinition &providerDefinition, bool negate = false, bool substringSearch = false, bool useUnset = false)
            : mName(0), mValue(value), mMinValue(INVALID_VALUE), mMaxValue(INVALID_VALUE), mIsRangeCondition(false)
            , mNegate(negate), mSubstringSearch(substringSearch)
            , mUseUnset(useUnset)
            , mRuleDefinitionId(providerDefinition.getID())
            , mRuleDefinitionName(providerDefinition.getName())
            , mUserData(nullptr)
        {
            mConditionId = getNextConditionId();
            mName = getWmeName(attributeName, providerDefinition);
            initialize(value, providerDefinition);
        }

        /*! ************************************************************************************************/
        /*! \brief Constructor for range conditions with negate and substring options 
            Ranged conditions directly take int64's to specify the range of values to be matched.
        ***************************************************************************************************/
        Condition(const char8_t* attributeName, int64_t minValue, int64_t maxValue, const ConditionProviderDefinition &providerDefinition, bool negate = false, bool substringSearch = false, bool useUnset = false)
            :  mName(0), mValue(INVALID_VALUE), mMinValue(minValue), mMaxValue(maxValue), mIsRangeCondition(true)
            , mNegate(negate), mSubstringSearch(substringSearch)
            , mUseUnset(useUnset)
            , mRuleDefinitionId(providerDefinition.getID())
            , mRuleDefinitionName(providerDefinition.getName())
            , mUserData(nullptr)
        {
            mConditionId = getNextConditionId();
            mName = getWmeName(attributeName, providerDefinition);
            initialize(minValue, maxValue, providerDefinition);
        }


        /*! ************************************************************************************************/
        /*! brief Copy constructor
            We are currently assuming this class is copyable... for our rules to create ConditionInfo's
            Until we change that pattern, we can't remove this copy ctor.  
            We copy during:
                * each rule's addConditions
                * every time we create a JoinNode
                * In the maps we key off Condition (JoinNodeMap, ConditionInfoMap)
                * every time we copy ConditionInfo
        ***************************************************************************************************/
        Condition(const Condition &condition)
            : mName(condition.getName())
            , mValue(condition.getValue())
            , mMinValue(condition.getMinValue())
            , mMaxValue(condition.getMaxValue())
            , mIsRangeCondition(condition.getIsRangeCondition())
            , mNegate(condition.getNegate())
            , mSubstringSearch(condition.getSubstringSearch())
            , mUseUnset(condition.getUseUnset())
            , mRuleDefinitionId(condition.getRuleDefinitionId())
            , mRuleDefinitionName(condition.getRuleDefinitionName())
            , mToString(condition.toString())
            , mConditionId(condition.getConditionId())
            , mUserData(condition.getUserData())
        {
        }

        ~Condition() {}

        /*! ************************************************************************************************/
        /*! \brief Converts the string attribute name into a WMEAttribute name for use in RETE. 
        ***************************************************************************************************/
        static WMEAttribute getWmeName(const char8_t* attributeName, const ConditionProviderDefinition& providerDefinition, bool isGarbageCollectable = false)
        {
            WMEAttribute cachedValue = providerDefinition.getCachedWMEAttributeName(attributeName);
            if (cachedValue != WME_ANY_HASH && cachedValue != WME_UNSET_HASH)
            {
                return cachedValue;
            }

            eastl::string outputName;

            if ( (providerDefinition.getName() != nullptr) || (attributeName != nullptr) )
            {
                outputName.append_sprintf("%s_%s", providerDefinition.getName(), attributeName);
            }
            else
            {
                WARN_LOG("[Rete::Condition] Rule provider " << providerDefinition.getID() << " doesn't have a name, or added a nullptr condition attribute, logging will be affected.");
            }

            return providerDefinition.reteHash(outputName.c_str(), isGarbageCollectable);
        }


        /*! ************************************************************************************************/
        /*! \brief initialize overload for range conditions.
        ***************************************************************************************************/
        void initialize(int64_t minValue, int64_t maxValue, const ConditionProviderDefinition &providerDefinition)
        {
            if (minValue == maxValue)
            {
                mToString.append_sprintf("%s %s %" PRId64,
                    providerDefinition.reteUnHash(mName),
                    (mNegate ? "!=" : "=="),
                    minValue);
            }
            else
            {
                auto* condSign = (mNegate ? ">" : "<=");
                mToString.append_sprintf("%" PRId64 " %s %s %s %" PRId64,
                    minValue,
                    condSign,
                    providerDefinition.reteUnHash(mName),
                    condSign,
                    maxValue);
            }
        }

        
        /*! ************************************************************************************************/
        /*! \brief initialize overload for hashed value conditions
        ***************************************************************************************************/
        void initialize(WMEAttribute value, const ConditionProviderDefinition &providerDefinition)
        {
            mToString.append_sprintf("%s %s %s", 
                providerDefinition.reteUnHash(mName), 
                (mNegate ? "!=" : "=="),
                providerDefinition.reteUnHash(value));
        }


        /*! ************************************************************************************************/
        /*! \brief The conditions test.   Called to see if the attribute value meets this condition.
        ***************************************************************************************************/
        bool test(WMEAttribute attribute) const 
        { 
            if (getValue() != INVALID_VALUE)
            {
                if (getUseUnset() && attribute == WME_UNSET_HASH)
                {
                    TRACE_LOG("[Condition].test: Condition(" << mToString << ") accepts UNSET value for the attribute");
                    return true;
                }
                return  attribute == mValue;
            }
            else
            {
                return isInRange(static_cast<int64_t>(attribute));
            }
        }

        WMEAttribute getName() const { return mName; }

        /*! ************************************************************************************************/
        /*! \brief Returns the WMEAttribute value for a hashed condition.  INVALID_VALUE returned for range conditions.
        ***************************************************************************************************/
        WMEAttribute getValue() const { return mValue; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this condition has the negate option.  Used for != comparisons.
        ***************************************************************************************************/
        inline bool getNegate() const { return mNegate; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this condition is using mValue or WME_UNSET_HASH value.  Used for != comparisons.
        ***************************************************************************************************/
        inline bool getUseUnset() const { return mUseUnset; }

        /*! ************************************************************************************************/
        /*! \brief Returns the unique condition id used
        ***************************************************************************************************/
        inline ConditionId getConditionId() const { return mConditionId; }

        /*! ************************************************************************************************/
        /*! \brief Returns the custom UserData value.  Data lifetime not managed by Condition.
        ***************************************************************************************************/
        inline void* getUserData() const { return mUserData; }

        /*! ************************************************************************************************/
        /*! \brief Sets the custom UserData value.  Data lifetime not managed by Condition.
        ***************************************************************************************************/
        inline void setUserData(void* userData) { mUserData = userData; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this condition is used for a substring search (game name rule)
        ***************************************************************************************************/
        bool getSubstringSearch() const { return mSubstringSearch; }

        /*! ************************************************************************************************/
        /*! \brief Returns the id of the rule that provided this condition.
        ***************************************************************************************************/
        uint32_t getRuleDefinitionId() const { return mRuleDefinitionId; }

        /*! ************************************************************************************************/
        /*! \brief Returns the name of the rule that provided this condition.
        ***************************************************************************************************/
        const char8_t* getRuleDefinitionName() const { return mRuleDefinitionName.c_str(); }

        /*! ************************************************************************************************/
        /*! \brief Return a string representation of the condition this checks.
            NOTE: This value is only set if TRACE logging is enabled.
        ***************************************************************************************************/
        const char8_t* toString() const { return mToString.c_str(); }

        bool getIsRangeCondition() const { return (mIsRangeCondition); }

        // We used to cache off all WME values over the range in a list, but that proved too expensive.   So the min
        int64_t getMinRangeValue() const 
        { 
            if (!getIsRangeCondition())
            {
                return static_cast<int64_t>(getValue());
            }
            else
            {
                 return getMinValue();
            }
        }

        int64_t getMaxRangeValue() const 
        {
            if (!getIsRangeCondition())
            {
                return static_cast<int64_t>(getValue());
            }
            else
            {
                return getMaxValue();
            }
        }

        /*! ************************************************************************************************/
        /*! \brief equality operator.
                   The JoinNode and ProductionListener use this to store Conditions as the 'Keys' for some maps.
                   This is how the code ensures that duplicate Conditions are not added to the maps. 
        ***************************************************************************************************/
        bool operator==( const Condition& condition ) const
        {
            return ( (getName() == condition.getName()) && (getValue() == condition.getValue()) && 
                (getMinValue() == condition.getMinValue()) && (getMaxValue() == condition.getMaxValue())) &&
                (getNegate() == condition.getNegate()) && (getSubstringSearch() == condition.getSubstringSearch()) && 
                (getRuleDefinitionId() == condition.getRuleDefinitionId())  && (getUseUnset() == condition.getUseUnset());
        }

        /*! ************************************************************************************************/
        /*! \brief < operator used for putting condition into maps.
            Comparison based on if we're a range condition or not.
        ***************************************************************************************************/
        bool operator<( const Condition& condition) const
        {
            if  (getName() != condition.getName())
                return (getName() < condition.getName());

            if (getValue() != condition.getValue())
                return (getValue() < condition.getValue());

            int64_t rangeA = (getMaxValue() - getMinValue());
            int64_t rangeB = (condition.getMaxValue() - condition.getMinValue());
            if (rangeA != rangeB)
                return (rangeA < rangeB);

            if (getMinValue() != condition.getMinValue())
                return (getMinValue() < condition.getMinValue());

            if (getMaxValue() != condition.getMaxValue())
                return (getMaxValue() < condition.getMaxValue());

            // Checking to see if negate from this object is different from condition being provided in the argument.
            if (getNegate() != condition.getNegate())
                return (getNegate() < condition.getNegate());

            if (getSubstringSearch() != condition.getSubstringSearch())
                return (getSubstringSearch() < condition.getSubstringSearch());

            if (getRuleDefinitionId() != condition.getRuleDefinitionId())
                return (getRuleDefinitionId() < condition.getRuleDefinitionId());

            if (getUseUnset() != condition.getUseUnset())
                return (getUseUnset() < condition.getUseUnset());
            return false;
        }


    protected:

        int64_t getMinValue() const { return mMinValue; }
        int64_t getMaxValue() const { return mMaxValue; }

        bool isInRange(int64_t value) const { return ((value >= mMinValue) && (value <= mMaxValue)); }

        // Id generating function:
        inline ConditionId getNextConditionId() const 
        { 
            static EA_THREAD_LOCAL ConditionId gConditionId = INVALID_CONDITION_ID;
            
            ++gConditionId;
            return gConditionId;
        }

    private:
        WMEAttribute mName;
        // Tests using mValue assume ==, but we can add a ComparisonType for <, >.
        WMEAttribute mValue;
        // Tests using a range.
        int64_t mMinValue;
        int64_t mMaxValue;
        bool mIsRangeCondition;

        bool mNegate;
        bool mSubstringSearch;
        bool mUseUnset;
        uint32_t mRuleDefinitionId;
        eastl::string mRuleDefinitionName;      // These are bad and probably the source of a large amount of memory growth, 
        eastl::string mToString;                // 

        // Tests using mValue or WME_UNSET_HASH
        ConditionId mConditionId;
        void* mUserData;
    };

    struct ConditionInfo
    {
        ConditionInfo() {}
        ConditionInfo(Condition c, FitScore f, const ConditionProvider* p) : condition(c), fitScore(f), provider(p) {}

        Condition condition;
        FitScore fitScore;
        const ConditionProvider *provider;
    };

    class ProductionToken
    {
        NON_COPYABLE(ProductionToken);
        NODE_POOL_NEW_DELETE_OVERLOAD();
    public:
        ProductionToken(WMEId id) : mId(id) {}

        WMEId mId;
    };


    typedef eastl::hash_map<WMEId, const ProductionToken*> BetaMemory;

    enum ConditionBlockId
    {
        CONDITION_BLOCK_BASE_SEARCH,
        CONDITION_BLOCK_MAX_SIZE,
        CONDITION_BLOCK_DEDICATED_SERVER_SEARCH // DEPRECATED
    };
 
    class ProductionListener;
    struct ProductionInfo
    {
        ProductionInfo() : id(0), fitScore(0), mListener(nullptr), conditionBlockId(CONDITION_BLOCK_BASE_SEARCH), isTracked(false), pNode(nullptr), numActiveTokens(0) {}
        ProductionInfo(ProductionId i, FitScore f, ProductionListener* pl, ConditionBlockId blockId) 
            : id(i), fitScore(f), mListener(pl), conditionBlockId(blockId), isTracked(false), pNode(nullptr), numActiveTokens(0)
        {}

        ProductionId id;
        FitScore fitScore;
        ProductionListener* mListener;
        ConditionBlockId conditionBlockId;
        bool isTracked;
        ProductionNode* pNode;
        uint32_t numActiveTokens;
    };

    typedef eastl::hash_map<ProductionId, ProductionInfo> ProductionInfoMap;

    class JoinNode;
    class ProductionBuildInfo;
    typedef eastl::hash_multimap<ProductionId, ProductionBuildInfo*> ProductionBuildInfoMap;
    typedef eastl::pair<ProductionBuildInfoMap::iterator,ProductionBuildInfoMap::iterator> ProductionBuildInfoMapItrPair;
    typedef eastl::intrusive_list<ProductionBuildInfo> ProductionBuildInfoList;

    typedef eastl::vector<ConditionInfo> OrConditionList;
    typedef eastl::vector<OrConditionList> ConditionBlock;
    // ConditionBlockList is indexed by ConditionBlockId
    typedef eastl::vector<ConditionBlock> ConditionBlockList;


    class ProductionBuildInfo : public eastl::intrusive_list_node
    {
        NON_COPYABLE(ProductionBuildInfo);
        NODE_POOL_NEW_DELETE_OVERLOAD();
    public:
        ProductionBuildInfo(JoinNode& parent, const ConditionBlock& conditionBlock,
            ConditionBlock::const_iterator itr, const ProductionInfo& productionInfo)
            : mParent(parent),
            mConditionBlock(conditionBlock),
            mItr(itr),
            mProductionInfo(productionInfo)
        { }
        virtual ~ProductionBuildInfo() { }

        JoinNode& getParent() { return mParent; }
        const JoinNode& getParent() const { return mParent; }
        const ConditionBlock& getCondiitonBlock() const { return mConditionBlock; }
        ConditionBlock::const_iterator& getConditionBlockItr() { return mItr; }
        ProductionInfo& getProductionInfo() { return mProductionInfo; }
        const ProductionInfo& getProductionInfo() const { return mProductionInfo; }

    private:
        JoinNode& mParent;
        const ConditionBlock& mConditionBlock;
        ConditionBlock::const_iterator mItr;
        ProductionInfo mProductionInfo;
    };

//     struct WMEEqualTo
//     {
// 
//         bool operator()(const WorkingMemoryElement* a, const WorkingMemoryElement* b) const
//         {
//             return a->getId() == b->getId()
//                 && a->getName() == b->getName()
//                 && a->getValue() == b->getValue();
//         }
//     };
// 
//     struct WMETestHash
//     {
//         // Basic Hash Function.
//         // TODO: Review Hash Function for fewest collisions.
//         size_t operator()(const WorkingMemoryElement* wme) const
//         {
//             //const int32_t SEED = 23;
//             const int32_t ODD_PRIME = 37;
//             int32_t result = 851; //SEED * ODD_PRIME;
// 
//             result = (result + wme->getId()) * ODD_PRIME;
//             result = (result + wme->getName()) * ODD_PRIME;
//             result = (result + wme->getValue()) * ODD_PRIME;
// 
//             return result;
//         }
//     };
//     typedef eastl::hash_set<const WorkingMemoryElement*, WMETestHash, WMEEqualTo> WMEHashSet;
//     typedef eastl::hash_map<WMEId, WMEHashSet> WMEIdHashMap;

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_RETETYPES_H

