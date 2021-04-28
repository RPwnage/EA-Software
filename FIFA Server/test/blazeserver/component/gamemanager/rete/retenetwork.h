/*! ************************************************************************************************/
/*!
    \file C:\p4\blaze\3.x-sb1\component\gamemanager\rete\retenetwork.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETENETWORK_H
#define BLAZE_GAMEMANAGER_RETENETWORK_H

#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/map.h"
#include "EASTL/list.h"
#include "EASTL/vector.h"

#include "framework/blazeallocator.h"
#include "gamemanager/rete/retetypes.h"
#include "gamemanager/rete/stringtable.h"
#include "gamemanager/tdf/retenetwork_config_server.h"
#include "gamemanager/tdf/matchmaker_types.h"
#include "gamemanager/rpc/gamemanagerslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class JoinNode;
    class ProductionNode;
    typedef eastl::list<JoinNode*> JoinNodeList;

    class WMEManager;
    class ProductionManager;
    class ReteNetwork;

    typedef uint64_t ProductionNodeId;
    static const ProductionNodeId INVALID_PRODUCTION_NODE_ID = 0;

    /*! ************************************************************************************************/
    /*! \brief ProductionNodes are end points in the rete network.  A ProductionListener listens
        to production nodes to find our match.
    ***************************************************************************************************/
    class ProductionNode final : public ProductionNodeIntrusiveListNode
    {
        NON_COPYABLE(ProductionNode);
        NODE_POOL_NEW_DELETE_OVERLOAD();
    public:
        ProductionNode(ReteNetwork* reteNetwork, JoinNode& parent, ProductionNodeId productionNodeId);
        ~ProductionNode();

        // When a new token comes in, update the GameBrowserList with the token & fit score.
        // Game Browser contains a priority queue of Game Sessions
        void notifyParentAddedToken(const ProductionToken& token);
        void notifyParentRemovedToken(const ProductionToken& token);
        void notifyParentUpsertToken(const ProductionToken& token);

        bool addProductionInfo(const ProductionInfo& info);
        bool removeProductionInfo(ProductionId id);

        JoinNode& getParent() const { return mParent; }
        size_t getTokenCount() const;
        const BetaMemory& getTokens() const;

        uint32_t getProductionCount() const { return mProductionInfoMap.size(); }

        ProductionNodeId getProductionNodeId() const { return mProductionNodeId; }

        // Recursively travels up and prints conditions
        void printConditions() const;

    private:
        JoinNode& mParent;

        // Set of Production Ids interested in this Join Node
        // Production Info Set which contains
        // Production Id, Fit Score, and ProductionListener.
        ProductionInfoMap mProductionInfoMap;

        // Pointer to the overall ReteNetwork
        ReteNetwork* mReteNetwork;

        // Unique id for the Node: 
        ProductionNodeId mProductionNodeId;
    };


    /*! ************************************************************************************************/
    /*! \brief ProductionNodeFactory handles creating and destroying memory of ProductionNodes.
    ***************************************************************************************************/
    class ProductionNodeFactory
    {
    public:
        ProductionNodeFactory() : mProductionNodeAllocator("MMAllocatorProductionNode", sizeof(ProductionNode), GameManagerSlave::COMPONENT_MEMORY_GROUP), mCurNodeId(INVALID_PRODUCTION_NODE_ID) {}
        virtual ~ProductionNodeFactory() {}

        ProductionNode& createProductionNode(ReteNetwork *reteNetwork, JoinNode& parent)
        {
            ++mCurNodeId;
            return *(new (getProductionNodeAllocator()) ProductionNode(reteNetwork, parent, mCurNodeId));
        }
        void deleteProductionNode(ProductionNode* productionNode)
        {
            productionNode->~ProductionNode();
            ProductionNode::operator delete (productionNode, getProductionNodeAllocator());
        }

        const NodePoolAllocator& getProductionNodeAllocator() const { return mProductionNodeAllocator; }

    private:
        NodePoolAllocator& getProductionNodeAllocator() { return mProductionNodeAllocator; }

    private:
        NodePoolAllocator mProductionNodeAllocator;
        ProductionNodeId mCurNodeId;
    };


    /*! ************************************************************************************************/
    /*! \brief The AlphaNetwork
    ***************************************************************************************************/
    class AlphaNetwork
    {
    public:
        class AlphaMemory final
        {
        public:
            typedef eastl::hash_map<WMEId, const WorkingMemoryElement*> WMEAlphaMemoryMap;
        public:
            AlphaMemory() : mWMEAlphaMemoryMap(BlazeStlAllocator("mWMEAlphaMemoryMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)), mHasWMEProvider(true) {}
            ~AlphaMemory() {}

            bool addWorkingMemoryElement(const WorkingMemoryElement& wme, int32_t logCategory);
            bool removeWorkingMemoryElement(const WorkingMemoryElement& wme, bool notifyListeners, int32_t logCategory);
            bool upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme, int32_t logCategory);

            bool containsWME(WMEId id) const { return mWMEAlphaMemoryMap.find(id) != mWMEAlphaMemoryMap.end(); }
            JoinNodeList::const_iterator registerListener(JoinNode& joinNode);
            void unregisterListener(JoinNodeList::const_iterator joinNodeItr);
            void addWMEsFromAlphaMemory(JoinNode& joinNode) const;

            size_t memorySize() const { return mWMEAlphaMemoryMap.size(); }

            bool isUnreferenced() { return mWMEAlphaMemoryMap.empty() && mJoinNodeList.empty(); }
            bool hasListeners() const { return mJoinNodeList.empty(); }

            void clearHasWMEProvider() { mHasWMEProvider = false; }
            bool hasWMEProvider() const { return mHasWMEProvider; }
            
            void eraseAllAlphaMemories(WMEManager& wmeManager);
        private:
            // Map of all of the WMEs in this Alpha Memory
            WMEAlphaMemoryMap mWMEAlphaMemoryMap;

            // List of all of the Productions listening to changes on this Alpha Memory
            JoinNodeList mJoinNodeList;

            bool mHasWMEProvider;
        };

        typedef eastl::pair<AlphaMemory*, JoinNodeList::const_iterator> AlphaMemoryAndNode; 

        struct WMEPair : public eastl::pair<WMEAttribute, WMEAttribute>
        {
            WMEPair(const eastl::pair<WMEAttribute, WMEAttribute>& other) { first = other.first; second = other.second; }
            bool operator<(const WMEPair& other)
            {
                if (first != other.first)   
                    return (first < other.first);
                if (second != other.second) 
                    return (second < other.second);
                return false;
            }
        };
        typedef eastl::map<WMEPair, AlphaMemory> WMEAttributeMap;

    public:
        AlphaNetwork(ReteNetwork* reteNetwork) : mReteNetwork(reteNetwork), mWMEAttributeHashMap(BlazeStlAllocator("mWMEAttributeHashMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)) {}
        virtual ~AlphaNetwork() {}

        void addWorkingMemoryElement(const WorkingMemoryElement& wme);
        void removeWorkingMemoryElement(const WorkingMemoryElement& wme);
        void upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme);

        void removeAlphaMemory(const WorkingMemoryElement& wme);

        void removeAlphaMemoryProvider(WMEAttribute name, WMEAttribute value);
        

        AlphaMemoryAndNode registerListener(WMEAttribute name, WMEAttribute value, JoinNode& joinNode);
        void unregisterListener(WMEAttribute name, WMEAttribute value, AlphaMemoryAndNode memoryAndNode);

        bool isInAlphaMemory(WMEAttribute name, WMEAttribute value) const { return (mWMEAttributeHashMap.find(eastl::make_pair(name, value)) != mWMEAttributeHashMap.end()); }

        size_t getAlphaMemorySize(WMEAttribute name, WMEAttribute value) const;

    private:
        inline WMEAttributeMap::iterator getAlphaMemoryItr(WMEAttribute name, WMEAttribute value)
        {
            WMEPair keyPair = eastl::make_pair(name, value);
            WMEAttributeMap::insert_return_type ret = mWMEAttributeHashMap.insert(keyPair);
            if (ret.second)
            {
                incrementHashReferences(name, value);
            }
            return ret.first;
        }

        void incrementHashReferences(WMEAttribute name, WMEAttribute value);

        void removeAllWMEsFromAlphaMemory(WMEAttributeMap::iterator alphaMemoryItr);

    private:
        ReteNetwork* mReteNetwork;

        WMEAttributeMap mWMEAttributeHashMap;
    };

    /*! ************************************************************************************************/
    /*! \brief ProductionBuildInfoFactory handles the creating and destroying of ProductionNodeInfos
    ***************************************************************************************************/
    class ProductionBuildInfoFactory
    {
    public:
        ProductionBuildInfoFactory() : mProductionBuildInfoAllocator("MMAllocatorProductionBuildInfo", sizeof(ProductionBuildInfo), GameManagerSlave::COMPONENT_MEMORY_GROUP) {}
        virtual ~ProductionBuildInfoFactory() {}

        ProductionBuildInfo& createProductionBuildInfo(JoinNode& parent, const ConditionBlock& conditionBlock,
            ConditionBlock::const_iterator itr, const ProductionInfo& productionInfo)
        {
            return *(new (getProductionBuildInfoAllocator()) ProductionBuildInfo(parent, conditionBlock, itr, productionInfo));
        }
        void deleteProductionBuildInfo(ProductionBuildInfo* node)
        {
            node->~ProductionBuildInfo();
            ProductionBuildInfo::operator delete (node, getProductionBuildInfoAllocator());
        }

        const NodePoolAllocator& getProductionBuildInfoAllocator() const { return mProductionBuildInfoAllocator; }

    private:
        NodePoolAllocator& getProductionBuildInfoAllocator() { return mProductionBuildInfoAllocator; }

    private:
        NodePoolAllocator mProductionBuildInfoAllocator;
    };


    /*! ************************************************************************************************/
    /*! \brief JoinNodes join the AlphaNetwork to the BetaNewtork. 
    ***************************************************************************************************/
    class JoinNode : public JoinNodeIntrusiveListNode
    {
        NON_COPYABLE(JoinNode);
        NODE_POOL_NEW_DELETE_OVERLOAD();
    public:

        typedef eastl::map<Condition, JoinNode*> JoinNodeMap;
        typedef eastl::map<WMEAttribute, AlphaNetwork::AlphaMemoryAndNode> AlphaMemoryMap;

    public:
        JoinNode(ReteNetwork* reteNetwork);
        JoinNode(ReteNetwork* reteNetwork, JoinNode& parent, const Condition& productionTest);
        virtual ~JoinNode();

        // Right Activations from Alpha Memory
        void notifyAddWMEToAlphaMemory(WMEId wmeId);
        void notifyRemoveWMEFromAlphaMemory(WMEId wmeId);
        void notifyUpsertWMEFromAlphaMemory(WMEId wmeId, WMEAttribute newValue);

        // Left Activations from Beta Memory
        void notifyAddWMEToParentBetaMemory(const ProductionToken& wme);
        void notifyRemoveWMEFromParentBetaMemory(const ProductionToken& wme);
        void notifyUpsertWMEFromParentBetaMemory(const ProductionToken &wme);

        void addWMEsFromParentsBetaMemory();

        const JoinNodeMap& getChildren() const { return mChildren; }
        JoinNodeMap& getChildren() { return mChildren; }
        bool hasTokens() const { return !mBetaMemory.empty(); }
        const BetaMemory& getBetaMemory() const { return mBetaMemory; }
        JoinNode* getParent() const { return mParent; }
        inline bool isHeadNode() const { return mParent == nullptr; }
        const Condition& getProductionTest() const { return mProductionTest; }

        inline bool hasProductionNode() const { return mPNode != nullptr; }
        ProductionNode& getProductionNode();
        void removeProductionNode();

        void setReteNetwork(ReteNetwork* network) { mReteNetwork = network; }
        ReteNetwork& getReteNetwork() const { return *mReteNetwork; }

        const ProductionToken* getTokenFromBetaMemory(WMEId id) const;

        void addProductionBuildInfo(const ConditionBlock& conditionBlock, ConditionBlock::const_iterator itr, ProductionInfo& productionInfo);
        void removeProductionBuildInfo(ProductionBuildInfo& productionBuildInfo);
        void removeProductionBuildInfo(const ProductionBuildInfoMapItrPair& itr);

        JoinNode& getOrCreateChild(const Condition& condition);
        void garbageCollectChild(const JoinNode& node);

        ProductionBuildInfoMap& getProductionBuildInfoMap() { return mProductionBuildInfoMap; }

        // Used for debug information only
        uint32_t getAlphaMemorySize() const;

        void printConditions() const;
        void getConditions(eastl::vector<eastl::string>& conditions, uint32_t depth = 0) const;

    private:
        void notifyChildrenAddToken(const ProductionToken& token);
        void notifyChildrenRemoveToken(const ProductionToken& token);
        void notifyChildrenUpsertToken(const ProductionToken& token);

        void addWME(WMEId wmeId);
        void removeWME(WMEId wmeId);
        void upsertWME(WMEId wmeId);

        void buildBetaTree();

    private:
        // Set of Production Ids interested in this Join Node
        ProductionBuildInfoMap mProductionBuildInfoMap;

        // Map of Children (Other Productions) keyed by the production test
        JoinNodeMap mChildren;

        // TODO: Currently this is a hash map, but it either needs to be a multi
        // map or hash off of the full WME, for our current use case 
        // All WME's are unique by WMEId (game Id) as long as the Beta Network
        // conditions don't search on keys for ANY_HASH.

        // This Beta Memory (all relevant Game matches).
        BetaMemory mBetaMemory;

        // Reference to Parent (To access the Beta Memory of Parent when right-activated)
        JoinNode* mParent;

        // Reference to Alpha Memory (To access Alpha Memory when left-activated)
        AlphaMemoryMap mAlphaMemoryMap;

        // Production Test (Attribute and Value)
        Condition mProductionTest;

        // P-Node, represents the complete set of matches for all production conditions.
        ProductionNode* mPNode;

        // Pointer to the overall ReteNetwork
        ReteNetwork* mReteNetwork;

        // Used avoid iterating mChildren to determine if we have any substring search children (usually 0)
        size_t mSubstringSearchChildrenCount;
    };


    /*! ************************************************************************************************/
    /*! \brief JoinNodeFactory handles creating and destroying of JoinNodes
    ***************************************************************************************************/
    class JoinNodeFactory
    {
    public:
        JoinNodeFactory() : mJoinNodeAllocator("MMAllocatorJoinNode", sizeof(JoinNode), GameManagerSlave::COMPONENT_MEMORY_GROUP) {}
        virtual ~JoinNodeFactory() {}

        JoinNode& createJoinNode(ReteNetwork* reteNetwork)
        {
            return *(new (getJoinNodeAllocator()) JoinNode(reteNetwork));
        }
        JoinNode& createJoinNode(ReteNetwork* reteNetwork, JoinNode& parent, const Condition& productionTest)
        {
            return *(new (getJoinNodeAllocator()) JoinNode(reteNetwork, parent, productionTest));
        }
        void deleteJoinNode(JoinNode* joinNode)
        {
            joinNode->~JoinNode();
            JoinNode::operator delete (joinNode, getJoinNodeAllocator());
        }

        const NodePoolAllocator& getJoinNodeAllocator() const { return mJoinNodeAllocator; }
    private:
        NodePoolAllocator& getJoinNodeAllocator() { return mJoinNodeAllocator; }

    private:
        NodePoolAllocator mJoinNodeAllocator;
    };


    /*! ************************************************************************************************/
    /*! \brief ProductionTokenFactory handles the creation and deletion of ProductionTokens
    ***************************************************************************************************/
    class ProductionTokenFactory
    {
    public:
        ProductionTokenFactory() : mJoinNodeAllocator("MMAllocatorProductionToken", sizeof(ProductionToken), GameManagerSlave::COMPONENT_MEMORY_GROUP) {}
        virtual ~ProductionTokenFactory() {}

        ProductionToken& createProductionToken(WMEId id)
        {
            return *(new (getProductionTokenAllocator()) ProductionToken(id));
        }
        void deleteProductionToken(ProductionToken* productionToken)
        {
            productionToken->~ProductionToken();
            ProductionToken::operator delete (productionToken, getProductionTokenAllocator());
        }

        const NodePoolAllocator& getProductionTokenAllocator() const { return mJoinNodeAllocator; }

    private:
        NodePoolAllocator& getProductionTokenAllocator() { return mJoinNodeAllocator; }

    private:
        NodePoolAllocator mJoinNodeAllocator;
    };

    class ReteSubstringTrie;
    class ReteSubstringTrieWordCounter;
    /*! ************************************************************************************************/
    /*! \brief The ReteNetwork
    ***************************************************************************************************/
    class ReteNetwork
    {
        NON_COPYABLE(ReteNetwork);
    public:
        typedef eastl::hash_map<uint32_t, uint64_t> JoinCountByRuleDefId;
        static const char8_t* RETENETWORK_CONFIG_KEY;

        ReteNetwork(Metrics::MetricsCollection& collection, int32_t logCategory);
        virtual ~ReteNetwork();

        void onShutdown();

        void hardReset();

        bool validateConfig(ReteNetworkConfig& config, const ReteNetworkConfig* referenceConfig, ConfigureValidationErrors& validationErrors) const;
        bool configure(const ReteNetworkConfig& config);
        bool reconfigure(const ReteNetworkConfig& config);

        const WMEManager& getWMEManager() const { return *mWMEManager; }
        WMEManager& getWMEManager() { return *mWMEManager; }

        void addWorkingMemoryElement(const WorkingMemoryElement& wme);
        void removeWorkingMemoryElement(const WorkingMemoryElement& wme);
        void upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme);

        void addWorkingMemoryElement(const WorkingMemoryElement& wme, const char8_t *stringKey);
        void removeWorkingMemoryElement(const WorkingMemoryElement& wme, const char8_t *stringKey);

        void removeAlphaMemory(const WorkingMemoryElement& wme);

        ProductionManager& getProductionManager() const { return *mProductionManager; }

        AlphaNetwork& getAlphaNetwork() { return *mAlphaNetwork; }

        // Rete substring search
        ReteSubstringTrie& getReteSubstringTrie() { return *mReteSubstringTrie; }
        // Separated substring search string tables, for easily tracking #users still needing an entry.
        // (Rete must clean entries when no longer used, as string search rule/defn may add 'infinite' poss values)
        StringTable& getSubstringTrieSearchTable() { return mSubstringTrieSearchTable; }
        StringTable& getSubstringTrieWordTable() { return mSubstringTrieWordTable; }
        // for reclaiming no-longer used hashes from mSubstringTrieWordTable
        ReteSubstringTrieWordCounter& getSubstringTrieWordCounter() { return *mSubstringTrieWordCounter; }

        JoinNode& getBetaNetwork() { return *mBetaNetwork; }

        uint64_t reteHash(const char8_t* str, bool garbageCollectable = false);
        const char8_t* getStringFromHash(uint64_t hash) const;
        const char8_t* getStringFromHashedConditionValue(const Condition& condition) const;

        JoinNodeFactory& getJoinNodeFactory() { return mJoinNodeFactory; }
        const JoinNodeFactory& getJoinNodeFactory() const { return mJoinNodeFactory; }
        ProductionNodeFactory& getProductionNodeFactory() { return mProductionNodeFactory; }
        const ProductionNodeFactory& getProductionNodeFactory() const { return mProductionNodeFactory; }
        ProductionTokenFactory& getProductionTokenFactory() { return mProductionTokenFactory; }
        const ProductionTokenFactory& getProductionTokenFactory() const { return mProductionTokenFactory; }
        ProductionBuildInfoFactory& getProductionBuildInfoFactory() { return mProductionBuildInfoFactory; }
        const ProductionBuildInfoFactory& getProductionBuildInfoFactory() const { return mProductionBuildInfoFactory; }

        const Metrics::TaggedGauge<GameManager::RuleName>& getRuleJoinCount() const { return mJoinCount; }
        const Metrics::TaggedCounter < GameManager::RuleName>& getRuleTotalJoinCount() const { return mTotalJoinCount; }
        const Metrics::TaggedCounter < GameManager::RuleName>& getRuleTotalUpserts() const { return mTotalUpserts; }
        const Metrics::TaggedGauge<GameManager::RuleName>& getRuleMaxChildJoinCount() const { return mMaxChildJoinCount; }

        // garbage collection
        void flagForGarbageCollection(ProductionNodeIntrusiveListNode& node);
        void clearForGarbageCollection(ProductionNodeIntrusiveListNode& node);
        void flagForGarbageCollection(JoinNodeIntrusiveListNode& node);
        void clearForGarbageCollection(JoinNodeIntrusiveListNode& node);
        void garbageCollect(uint32_t& pnodeCount, uint32_t& jnodeCount);
        bool garbageCollect(JoinNode& node);

        // idler for garbage collection
        void scheduleNextIdle(EA::TDF::TimeValue timeTillIdle = 0);
        bool idle();
        void idleTimerCallback();
        uint32_t getIdlePeriodMs() { return mReteConfig.getIdlePeriodMs(); }
        uint64_t getNumGarbageCollectableNodes() const { return mGarbageCollectableNodes; }

        void dumpBetaNetwork();

        void incRuleJoinCount(const char8_t* ruleName);
        void decRuleJoinCount(const char8_t* ruleName);
        void incRuleUpsertsCount(const char8_t* ruleName);
        void setRuleMaxChildJoinCount(const char8_t* ruleName, uint64_t val);

        size_t getNumGamesVisibleForCondition(const Rete::Condition& condition) const;
        int32_t getLogCategory() const { return mLogCategory; }

    // Helper Functions
    private:

        void deleteJoinNode(JoinNode* node);
        void logJoinNode(JoinNode& joinNode) const;

    // Members
    private:
        static const uint32_t MIN_IDLE_PERIOD_MS;

        AlphaNetwork* mAlphaNetwork;

        ReteSubstringTrie* mReteSubstringTrie;
        StringTable mSubstringTrieSearchTable;
        StringTable mSubstringTrieWordTable;
        ReteSubstringTrieWordCounter* mSubstringTrieWordCounter;

        JoinNode* mBetaNetwork; // The Head Join Node
        WMEManager* mWMEManager;
        ProductionManager* mProductionManager;

        JoinNodeFactory mJoinNodeFactory;
        ProductionNodeFactory mProductionNodeFactory;
        ProductionTokenFactory mProductionTokenFactory;
        ProductionBuildInfoFactory mProductionBuildInfoFactory;

        JoinNodeIntrusiveList mJoinNodeGCList;
        ProductionNodeIntrusiveList mProductionNodeGCList;

        ReteNetworkConfig mReteConfig;

        uint64_t mTotalIdles;
        TimerId mIdleTimerId;
        EA::TDF::TimeValue mLastIdleLength;
        uint64_t mGarbageCollectableNodes;
        FILE* mDebugOutput;
        int32_t mLogCategory;

        Metrics::MetricsCollection& mMetricsCollection;
        Metrics::TaggedGauge<GameManager::RuleName> mJoinCount;
        Metrics::TaggedCounter<GameManager::RuleName> mTotalJoinCount;
        Metrics::TaggedCounter<GameManager::RuleName> mTotalUpserts;
        Metrics::TaggedGauge<GameManager::RuleName> mMaxChildJoinCount;
    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_RETENETWORK_H
