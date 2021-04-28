/*! ************************************************************************************************/
/*!
    \file C:\p4\blaze\3.x-sb1\component\gamemanager\rete\retenetwork.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include <EAStdC/EAHashCRC.h>
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "matchmaker/rules/ruledefinition.h"
#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/retesubstringtrie.h" // for mReteSubstringTrie
#include "gamemanager/rpc/searchslave.h"
#include "gamemanager/matchmaker/matchmakermetrics.h"

#define RETE_FORMAT_WME(wme) mReteNetwork->getStringFromHash(wme) << "(" << wme << ")"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    // minimum set to be the default length of slave matchmaking idle.
    const uint32_t ReteNetwork::MIN_IDLE_PERIOD_MS = 250;
    const char8_t* ReteNetwork::RETENETWORK_CONFIG_KEY = "rete";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    ReteNetwork::ReteNetwork(Metrics::MetricsCollection& collection, int32_t logCategory)
        : mTotalIdles(0)
        , mIdleTimerId(INVALID_TIMER_ID)
        , mLastIdleLength(0)
        , mGarbageCollectableNodes(0)
        , mDebugOutput(nullptr)
        , mLogCategory(logCategory)
        , mMetricsCollection(collection)
        , mJoinCount(mMetricsCollection, "rete.joinNodes", Metrics::Tag::rule_name)
        , mTotalJoinCount(mMetricsCollection, "rete.joinCount", Metrics::Tag::rule_name)
        , mTotalUpserts(mMetricsCollection, "rete.upserts", Metrics::Tag::rule_name)
        , mMaxChildJoinCount(mMetricsCollection, "rete.maxChildJoinCount", Metrics::Tag::rule_name)
    {
        mWMEManager = BLAZE_NEW WMEManager(this);
        mProductionManager = BLAZE_NEW ProductionManager(this);
        mAlphaNetwork = BLAZE_NEW AlphaNetwork(this);
        mReteSubstringTrie = BLAZE_NEW ReteSubstringTrie(getSubstringTrieSearchTable(), *mWMEManager);
        mSubstringTrieWordCounter = BLAZE_NEW ReteSubstringTrieWordCounter();
        mBetaNetwork = &getJoinNodeFactory().createJoinNode(this);
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    ReteNetwork::~ReteNetwork()
    {
        if (mIdleTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mIdleTimerId);
            mIdleTimerId = INVALID_TIMER_ID;
        }
        
        deleteJoinNode(mBetaNetwork);
        // clean up everything now that we've removed all the join nodes.
        
        delete mWMEManager;
        mWMEManager = nullptr;
        delete mProductionManager;
        mProductionManager = nullptr;
        delete mAlphaNetwork;
        mAlphaNetwork = nullptr;

        if (mReteSubstringTrie != nullptr)
        {
            delete mReteSubstringTrie;
            mReteSubstringTrie = nullptr;
        }
        if (mSubstringTrieWordCounter != nullptr)
        {
            delete mSubstringTrieWordCounter;
            mSubstringTrieWordCounter = nullptr;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Resets the BetaNetwork.  Assumes any productions have already been canceled.
    ***************************************************************************************************/
    void ReteNetwork::hardReset()
    {
        if (mIdleTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mIdleTimerId);
            mIdleTimerId = INVALID_TIMER_ID;
        }

        // recreate the beta network
        deleteJoinNode(mBetaNetwork);
        mBetaNetwork = &getJoinNodeFactory().createJoinNode(this);
    }

    void ReteNetwork::onShutdown()
    {
        if (mIdleTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mIdleTimerId);
            mIdleTimerId = INVALID_TIMER_ID;
        }
    }

    bool ReteNetwork::validateConfig(ReteNetworkConfig& config, const ReteNetworkConfig* referenceConfig, ConfigureValidationErrors& validationErrors) const
    {
        if (config.getIdlePeriodMs() < MIN_IDLE_PERIOD_MS)
        {
            eastl::string msg;
            msg.sprintf("[ReteNetwork].validateConfig: - Idle period %u must be less than minimum idle period %u.",
                config.getIdlePeriodMs(), MIN_IDLE_PERIOD_MS);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        return validationErrors.getErrorMessages().empty();
    }

    /*! ************************************************************************************************/
    /*! \brief configure the rete network
    
        \param[in] config - the configuration to read from
        \return true if successfully read configuration values, otherwise false
    ***************************************************************************************************/
    bool ReteNetwork::configure(const ReteNetworkConfig& config)
    {
        config.copyInto(mReteConfig);

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief reconfigure the reteNetwork
    
        \param[in] configMap - the configuration map to read values from
        \return always return true on reconfigure, should never leave server in bad state
    ***************************************************************************************************/
    bool ReteNetwork::reconfigure(const ReteNetworkConfig& config)
    {
        config.copyInto(mReteConfig);

        // Reschedule idle as config timer may have changed:
        if (mIdleTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mIdleTimerId);
            mIdleTimerId = INVALID_TIMER_ID;
            scheduleNextIdle();
        }

        return true;
    }



    /*! ************************************************************************************************/
    /*! \brief Deletes a join node and all its children
    
        \param[in] node - the node to delete
    ***************************************************************************************************/
    void ReteNetwork::deleteJoinNode(JoinNode* node)
    {
        EA_ASSERT(node != nullptr);
        if (EA_UNLIKELY(node == nullptr)) return;

        // Remove the node from any (garbage collection) list it may still be part of: 
        JoinNodeIntrusiveListNode* listNode = node;
        if (listNode->mpNext != nullptr && listNode->mpPrev != nullptr)
        {
            JoinNodeIntrusiveList::remove(*listNode);
            listNode->mpNext = nullptr;
            listNode->mpPrev = nullptr;
        }

        JoinNode::JoinNodeMap& joinNodes = node->getChildren();
        while (!joinNodes.empty())
        {
            JoinNode::JoinNodeMap::iterator itr = joinNodes.begin();
            deleteJoinNode(itr->second);
            joinNodes.erase(itr);
        }

        getJoinNodeFactory().deleteJoinNode(node);
    }

    uint64_t ReteNetwork::reteHash(const char8_t* str, bool garbageCollectable /*= false*/)
    {
        return mWMEManager->getStringTable().reteHash(str, garbageCollectable);
    }

    const char8_t* ReteNetwork::getStringFromHash(uint64_t hash) const
    {
        return mWMEManager->getStringTable().getStringFromHash(hash);
    }
    /*! ************************************************************************************************/
    /*! \brief returns unhashed string for the condition's value (wordHash key) from appropriate string table
    ***************************************************************************************************/
    const char8_t* ReteNetwork::getStringFromHashedConditionValue(const Condition& condition) const
    {
        if (condition.getSubstringSearch())
        {
            return mSubstringTrieSearchTable.getStringFromHash(condition.getValue()); 
        }
        else
        {
            return getStringFromHash(condition.getValue());
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Flags a production node for garbage collection
    
        \param[in] node - the node
    ***************************************************************************************************/
    void ReteNetwork::flagForGarbageCollection(ProductionNodeIntrusiveListNode& node)
    {
        ++mGarbageCollectableNodes;
        node.setGarbageCollectible();
        mProductionNodeGCList.push_back(node);
        scheduleNextIdle(mReteConfig.getGarbageCollectionTime());
    }

    /*! ************************************************************************************************/
    /*! \brief clears the garbage collection flag on this production node
    
        \param[in] node the production node
    ***************************************************************************************************/
    void ReteNetwork::clearForGarbageCollection(ProductionNodeIntrusiveListNode& node)
    {
        if (mGarbageCollectableNodes > 0)
        {
            --mGarbageCollectableNodes;
        }
        node.clearGarbageCollectible();
        mProductionNodeGCList.remove(node);
        node.mpNext = nullptr;
        node.mpPrev = nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief sets the garbage collection flag on this join node.
    
        \param[in] node - the join node
    ***************************************************************************************************/
    void ReteNetwork::flagForGarbageCollection(JoinNodeIntrusiveListNode& node)
    {
        ++mGarbageCollectableNodes;
        node.setGarbageCollectible();
        mJoinNodeGCList.push_back(node);
        scheduleNextIdle(mReteConfig.getGarbageCollectionTime());
    }

    /*! ************************************************************************************************/
    /*! \brief clears the garbage collection flag on this join node
    
        \param[in] node - the join node
    ***************************************************************************************************/
    void ReteNetwork::clearForGarbageCollection(JoinNodeIntrusiveListNode& node)
    {
        if (mGarbageCollectableNodes > 0)
        {
            --mGarbageCollectableNodes;
        }
        node.clearGarbageCollectible();
        mJoinNodeGCList.remove(node);
        node.mpNext = nullptr;
        node.mpPrev = nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Take out the trash.  Cleans production and join nodes that are no longer being searched
        for. Recurses upwards searching for parents that arent being used as well.
    ***************************************************************************************************/
    void ReteNetwork::garbageCollect(uint32_t& pnodeCount, uint32_t& jnodeCount)
    {
        // To consider: This will only track the number of nodes that have been flagged.
        // But we recurse upwards deleting parents, do we want to count those in this counter.
        // May also want to have time values associated with the delete, and only delete
        // after a period has elapsed.
        pnodeCount = 0;
        jnodeCount = 0;

        const JoinNodeFactory& jnFactory = getJoinNodeFactory();
        const ProductionNodeFactory& pnFactory = getProductionNodeFactory();

        if (BLAZE_IS_LOGGING_ENABLED(mLogCategory, Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mLogCategory, "[ReteNetwork].garbageCollect: Garbage collection - pre size JoinNodeAllocator:" << jnFactory.getJoinNodeAllocator().getUsedSizeBytes() 
                      << ", ProductionNodeAllocator:" << pnFactory.getProductionNodeAllocator().getUsedSizeBytes());
        }

        while (!mProductionNodeGCList.empty() && pnodeCount < mReteConfig.getGarbageCollectionLimit())
        {
            ProductionNode& node = static_cast<ProductionNode&>(*(mProductionNodeGCList.begin()));

            if (TimeValue::getTimeOfDay() - node.getTimeStamp() < mReteConfig.getGarbageCollectionTime())
            {
                break;
            }

            // we clear it as garbage collectible.  
            // This removes it from the intrusive list.
            bool stillCollectible = node.isGarbageCollectible();
            clearForGarbageCollection(node);
            // Should always be true, but just in case someone changes the boolean
            // on the node itself (instead of in reteNetwork), we double check here.
            if (stillCollectible)
            {
                JoinNode& parent = node.getParent();
                parent.removeProductionNode();
                ++pnodeCount;
                // starts recursing up.
                garbageCollect(parent);
            }
        }

        // Separate counters for join nodes, so we don't starve out their GC.
        while (!mJoinNodeGCList.empty() && jnodeCount < mReteConfig.getGarbageCollectionLimit())
        {
            JoinNode& node = static_cast<JoinNode&>(*(mJoinNodeGCList.begin()));

            if (TimeValue::getTimeOfDay() - node.getTimeStamp() < mReteConfig.getGarbageCollectionTime())
            {
                break;
            }

            // we clear it as garbage collectible.  
            // This removes it from the intrusive list.
            bool stillCollectible = node.isGarbageCollectible();
            clearForGarbageCollection(node);
            // Should always be true, but just in case someone changes the boolean
            // on the node itself (instead of in reteNetwork), we double check here.
            if (stillCollectible)
            {
                // starts recursing up.
                garbageCollect(node);
                ++jnodeCount;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Take out the trash.  Cleans join Nodes that are no longer in use.  Also recurses
        to our parent.
    
        \param[in] node - the starting join node.
    ***************************************************************************************************/
    bool ReteNetwork::garbageCollect(JoinNode& node)
    {
        // Head node is never removed
        if (node.isHeadNode())
        {
            return false;
        }

        JoinNode *parent = node.getParent();
        // Shouldn't be possible, but just in case
        if (EA_UNLIKELY(parent == nullptr))
        {
            ERR_LOG("[ReteNetwork].garbageCollect: We never should be trying to garbage collect the head join node.");
            return false;
        }

        // We do not delete this node if there are any references remaining
        // It has references if
        // - If it has a pNode
        // - If it has children
        // - If it has production build info.
        if (node.hasProductionNode() || !node.getChildren().empty() || !node.getProductionBuildInfoMap().empty())
        {
            BLAZE_TRACE_LOG(mLogCategory, "[ReteNetwork].garbageCollect: Unable to garbage collect JoinNode:" << &node 
                << " '" << node.getProductionTest().toString()
                << "'. Pnode:" << (node.hasProductionNode()?"true":"false") 
                << ", children:" << node.getChildren().size() << ", PBuildInfos:" << node.getProductionBuildInfoMap().size());
            return false;
        }
        
        // find and remove ourselves from our parent. Also deletes the node
        parent->garbageCollectChild(node);
        garbageCollect(*parent);

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Adds a working memory element to the alphanetwork
    
        \param[in] wme - the working memory element to add
    ***************************************************************************************************/
    void ReteNetwork::addWorkingMemoryElement(const WorkingMemoryElement& wme)
    {
        mAlphaNetwork->addWorkingMemoryElement(wme);
    }

    /*! ************************************************************************************************/
    /*! \brief Removes a working memory element from the alpha network.
    
        \param[in] wme - the working memory element to remove
    ***************************************************************************************************/
    void ReteNetwork::removeWorkingMemoryElement(const WorkingMemoryElement& wme)
    {
        mAlphaNetwork->removeWorkingMemoryElement(wme);
    }

    void ReteNetwork::addWorkingMemoryElement(const WorkingMemoryElement& wme, const char8_t *stringKey)
    {
        getReteSubstringTrie().addSubstringFacts(wme, stringKey);

        // update count for the word
        getSubstringTrieWordCounter().incrementWordCount(wme.getValue());
    }

    void ReteNetwork::removeWorkingMemoryElement(const WorkingMemoryElement& wme, const char8_t *stringKey)
    {
        getReteSubstringTrie().removeSubstringFacts(wme, stringKey);

        // remove hash if none else needs (avoid unbounded string table growth)
        if (0 == getSubstringTrieWordCounter().decrementWordCount(wme.getValue()))
        {
            BLAZE_SPAM_LOG(mLogCategory, "[ReteNetwork].removeWorkingMemoryElement: no more instances of word in trie: cleaning hash tables for key " << wme.getValue() 
                << ", for: wme(wmename=" << getStringFromHash(wme.getName()) << ",id=" << wme.getId() << ",str=" << getSubstringTrieWordTable().getStringFromHash(wme.getValue()) << ")");
            getSubstringTrieWordTable().removeHash(wme.getValue());
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Removes a working memory element with the old value from the alpha network.  Uperts the new
            working memory element into the alpha network.  Note that the old value must have been
            registered with the same wmeId.

        \param[in] oldValue - the old value the working memory element had
        \param[in] wme - the working memory element with the new value to upsert
    ***************************************************************************************************/
    void ReteNetwork::upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme)
    {
        mAlphaNetwork->upsertWorkingMemoryElement(oldValue, wme);
    }


    /*! ************************************************************************************************/
    /*! \brief Removes all working memory elements by a given id.

        \param[in] wme - the common id of the working memory elements to remove
  
    ***************************************************************************************************/
    void ReteNetwork::removeAlphaMemory(const WorkingMemoryElement& wme)
    {
        // Cleanup alpha memory only
        // The clean up the beta memories already occurred when we unregistered the wme any/any
        mAlphaNetwork->removeAlphaMemory(wme);

    }


    /*! ************************************************************************************************/
    /*! \brief Schedules the next idle call
    ***************************************************************************************************/
    void ReteNetwork::scheduleNextIdle(TimeValue timeTillIdle /*=0*/)
    {
        // skip if already scheduled
        if (mIdleTimerId == INVALID_TIMER_ID)
        {
            uint32_t timeExtMs = getIdlePeriodMs();
            if (timeTillIdle != 0)
            {
                timeExtMs = (uint32_t)timeTillIdle.getMillis();
            }
            mIdleTimerId = gSelector->scheduleTimerCall(
                TimeValue::getTimeOfDay() + (timeExtMs * 1000),
                this, &ReteNetwork::idleTimerCallback,
                "ReteNetwork::idleTimerCallback");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief idle for the RETE network to handle garbage collection
    ***************************************************************************************************/
    bool ReteNetwork::idle()
    {
        ++mTotalIdles;

        TimeValue idleStartTime = TimeValue::getTimeOfDay();

        uint32_t jnodeCount;
        uint32_t pnodeCount;
        garbageCollect(pnodeCount, jnodeCount);

        TimeValue endTime = TimeValue::getTimeOfDay();
        mLastIdleLength = endTime - idleStartTime;

        if (BLAZE_IS_LOGGING_ENABLED(mLogCategory, Logging::TRACE))
        {
            const JoinNodeFactory& jnFactory = getJoinNodeFactory();
            const ProductionNodeFactory& pnFactory = getProductionNodeFactory();
            BLAZE_TRACE_LOG(mLogCategory, "[ReteNetwork].idle: Exiting idle " << mTotalIdles << "; idle duration " << mLastIdleLength.getMillis() << "ms. Deleted " << pnodeCount << " pnodes " 
                << jnodeCount << " jnodes. Size JoinNodeAllocator:" << jnFactory.getJoinNodeAllocator().getUsedSizeBytes() 
                << ", ProductionNodeAllocator:" << pnFactory.getProductionNodeAllocator().getUsedSizeBytes() << ".");
        }

        return (!mJoinNodeGCList.empty() || !mProductionNodeGCList.empty());
    }

    /*! ************************************************************************************************/
    /*! \brief Callback for our idle's scheduled timer call.
    ***************************************************************************************************/
    void ReteNetwork::idleTimerCallback()
    {
        mIdleTimerId = INVALID_TIMER_ID;
        if (idle())
        {
            scheduleNextIdle();
        }
    }

    void ReteNetwork::dumpBetaNetwork()
    {
        JoinNode *headNode = mBetaNetwork;
        if (headNode != nullptr)
        {
            eastl::string fileName;
            fileName.sprintf("reteBetaNetwork_%s.dot", gController->getInstanceName());
            mDebugOutput = fopen(fileName.c_str(), "w");
            if (mDebugOutput != nullptr)
            {
                fprintf(mDebugOutput, "digraph {\ngraph [ranksep=0.25, fontname=Arial, nodesep=0.125];\nnode [fontname=Arial, style=filled, height=0, width=0, shape=box, fontcolor=white];\nedge [fontname=Arial];\n");
                logJoinNode(*headNode);
                fprintf(mDebugOutput, "}");
                fclose(mDebugOutput);
                mDebugOutput = nullptr;
            }
        }
    }

    void ReteNetwork::logJoinNode(JoinNode& joinNode) const
    {
        uint32_t tokens = (uint32_t)joinNode.getBetaMemory().size();
        uint32_t searchersWaiting = (uint32_t)joinNode.getProductionBuildInfoMap().size();
        uint32_t memorySize = joinNode.getAlphaMemorySize();

        uint32_t searchers = 0;
        uint32_t nodeColor = 128; // #000080, aka navy
        if (joinNode.hasProductionNode())
        {
            searchers = joinNode.getProductionNode().getProductionCount();
            nodeColor = 8388608; // #800000, aka maroon
        }
        else if (searchersWaiting > 0)
        {
            searchers = searchersWaiting;
            nodeColor = 32768; // #008000, aka green
        }


        fprintf(mDebugOutput, "\"JoinNode:%p\" [label=\"%s\\nMatches: %u of %u\\nSearchers: %u\" color=\"#%06x\" fontcolor=\"#ffffff\"];\n", &joinNode, joinNode.getProductionTest().toString(), tokens, memorySize, searchers, nodeColor);
        JoinNode::JoinNodeMap::const_iterator iter = joinNode.getChildren().begin();
        JoinNode::JoinNodeMap::const_iterator end = joinNode.getChildren().end();
        for (; iter != end; ++iter)
        {
            JoinNode *childNode = iter->second;
            if (childNode != nullptr)
            {
                fprintf(mDebugOutput, "\"JoinNode:%p\" -> \"JoinNode:%p\";\n", &joinNode, childNode);
                logJoinNode(*childNode);
            }
        }
    }

    void ReteNetwork::incRuleJoinCount(const char8_t* ruleName)
    {
        mJoinCount.increment(1, ruleName);
        mTotalJoinCount.increment(1, ruleName);
    }

    void ReteNetwork::decRuleJoinCount(const char8_t* ruleName)
    {
        mJoinCount.decrement(1, ruleName);
    }

    void ReteNetwork::incRuleUpsertsCount(const char8_t* ruleName)
    {
        mTotalUpserts.increment(1, ruleName);
    }

    void ReteNetwork::setRuleMaxChildJoinCount(const char8_t* ruleName, uint64_t val)
    {
        if (mMaxChildJoinCount.get({ { Metrics::Tag::client_type, ruleName } }) < val)
        {
            mMaxChildJoinCount.set(val, ruleName);
        }
    }

    size_t ReteNetwork::getNumGamesVisibleForCondition(const Rete::Condition& condition) const
    {
        ASSERT_COND_LOG(mAlphaNetwork != nullptr, "[ReteNetwork].getNumGamesVisibleForCondition: internal error: alpha network nullptr, returning 0.");
        if (mAlphaNetwork == nullptr)
        {
            return 0;
        }
        size_t count = 0;
        if (!condition.getIsRangeCondition())
        {
            count = mAlphaNetwork->getAlphaMemorySize(condition.getName(), condition.getValue());
        }
        else
        {
            for (auto i = condition.getMinRangeValue(); i <= condition.getMaxRangeValue(); ++i)
            {
                count += mAlphaNetwork->getAlphaMemorySize(condition.getName(), i);
            }
        }

        BLAZE_SPAM_LOG(mLogCategory, "[ReteNetwork].getNumGamesVisibleForCondition: (" << count << ") games for condition (wmename=" <<
            getStringFromHash(condition.getName()) << ":" << condition.getName() << ":value=" <<
            getStringFromHash(condition.getValue()) << "/" << condition.getValue() << ":min=" <<
            condition.getMinRangeValue() << ":max=" << condition.getMaxRangeValue() << "), for rule(definitionId=" <<
            condition.getRuleDefinitionId() << ").");
        return count;
    }

    size_t AlphaNetwork::getAlphaMemorySize(WMEAttribute name, WMEAttribute value) const
    {
        WMEAttributeMap::const_iterator itr = mWMEAttributeHashMap.find(eastl::make_pair(name, value));
        return (itr != mWMEAttributeHashMap.end() ? itr->second.memorySize() : 0);
    }


    void AlphaNetwork::addWorkingMemoryElement(const WorkingMemoryElement& wme)
    {
        if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].addWorkingMemoryElement: WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue()) << "] from alphaMemory.");
        }

        WMEAttributeMap::iterator alphaMemoryItr = getAlphaMemoryItr(wme.getName(), wme.getValue());
        if (!alphaMemoryItr->second.addWorkingMemoryElement(wme, mReteNetwork->getLogCategory()))
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[AlphaNetwork].addWorkingMemoryElement: ERROR adding WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue()) << "] to alphaMemory.");
        }
    }
    
    
    bool AlphaNetwork::AlphaMemory::addWorkingMemoryElement(const WorkingMemoryElement& wme, int32_t logCategory)
    {
        if (mWMEAlphaMemoryMap.insert(eastl::make_pair(wme.getId(), &wme)).second == false)
        {
            BLAZE_ERR_LOG(logCategory, "[AlphaMemory].addWorkingMemoryElement: dropping duplicate WME [id(" << wme.getId() << "), name(" << wme.getName() << "), value(" 
                    << wme.getValue() << ")] to alphaMemory.");
            EA_FAIL();
            return false;
        }

        if (BLAZE_IS_LOGGING_ENABLED(logCategory, Logging::SPAM))
        {
            BLAZE_SPAM_LOG(logCategory, "[RETE][AlphaMemory].addWorkingMemoryElement: ADDING ELEMENT FOR THE WME ID. WME[id("
                     << wme.getId() << "), name(" << wme.getName() <<"), value(" << wme.getValue() << ")] to alphaMemory.");
        }

        // Notify All productions that a new WME was added.
        for (auto& itr : mJoinNodeList)
        {
            JoinNode* joinNode = itr;
            joinNode->notifyAddWMEToAlphaMemory(wme.getId());
        }

        return true;
    }


    void AlphaNetwork::removeWorkingMemoryElement(const WorkingMemoryElement& wme)
    {
        if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].removeWorkingMemoryElement: WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue()) << "] from alphaMemory.");
        }

        WMEAttributeMap::iterator alphaMemoryItr = getAlphaMemoryItr(wme.getName(), wme.getValue());
        if (!alphaMemoryItr->second.removeWorkingMemoryElement(wme, true, mReteNetwork->getLogCategory()))
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].removeWorkingMemoryElement: ERROR removing WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue()) << "] from alphaMemory.");
        }

        // This WME name/value pair is no longer referenced by anything, so remove it from the map.
        if (alphaMemoryItr->second.isUnreferenced())
        {
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(wme.getName());
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(wme.getValue());
            mWMEAttributeHashMap.erase(alphaMemoryItr);
        }
    }


    bool AlphaNetwork::AlphaMemory::removeWorkingMemoryElement(const WorkingMemoryElement& wme, bool notifyListeners, int32_t logCategory)
    {
        WMEAlphaMemoryMap::iterator itr = mWMEAlphaMemoryMap.find(wme.getId());

        if (itr == mWMEAlphaMemoryMap.end())
        {
            BLAZE_ERR_LOG(logCategory, "[AlphaMemory].removeWorkingMemoryElement: attempting to remove non existant WME [id(" << wme.getId() << "), name(" 
                    << wme.getName() << "), value(" << wme.getValue() << ")].");
            EA_FAIL();
            return false;
        }

        mWMEAlphaMemoryMap.erase(itr);

        if (BLAZE_IS_LOGGING_ENABLED(logCategory, Logging::SPAM))
        {
            BLAZE_SPAM_LOG(logCategory, "[RETE][AlphaMemory].removeWorkingMemoryElement: REMOVING WME ID FROM TREE. WME[id("
                     << wme.getId() << "), name(" << wme.getName() << "), value(" << wme.getValue() << ")].");
        }

        if (notifyListeners)
        {
            // Notify All productions that a WME was removed.
            for (auto& joinNodeItr : mJoinNodeList)
            {
                JoinNode* production = joinNodeItr;
                production->notifyRemoveWMEFromAlphaMemory(wme.getId());
            }
        }

        return true;
    }


    void AlphaNetwork::upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme)
    {
        // Find the old alpha memory, and attempt to upsert our new wme.
        WMEAttributeMap::iterator oldAlphaMemoryItr = getAlphaMemoryItr(wme.getName(), oldValue);
        if (!oldAlphaMemoryItr->second.upsertWorkingMemoryElement(oldValue, wme, mReteNetwork->getLogCategory()))
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[AlphaNetwork].upsertWorkingMemoryElement: ERROR adding WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue())
                << ", oldValue " << RETE_FORMAT_WME(oldValue) << "]");
        }

        if (oldAlphaMemoryItr->second.isUnreferenced())
        {
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(oldValue);
            mWMEAttributeHashMap.erase(oldAlphaMemoryItr);
        }

        // Add to the new alpha memory.  If we successfully upserted the old join node, our add will early out
        // after a single hash lookup.  This is still more efficient than completely removing the token
        // and having that propagate all the way through the tree.
        WMEAttributeMap::iterator newAlphaMemoryItr = getAlphaMemoryItr(wme.getName(), wme.getValue());
        if (!newAlphaMemoryItr->second.addWorkingMemoryElement(wme, mReteNetwork->getLogCategory()))
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[AlphaNetwork].upsertWorkingMemoryElement: ERROR adding WME[id(" << wme.getId() << "), name " << RETE_FORMAT_WME(wme.getName()) << ", value " << RETE_FORMAT_WME(wme.getValue())
                << ", oldValue " << RETE_FORMAT_WME(oldValue) << "]");
        }
    }


    bool AlphaNetwork::AlphaMemory::upsertWorkingMemoryElement(WMEAttribute oldValue, const WorkingMemoryElement& wme, int32_t logCategory)
    {
        WMEAlphaMemoryMap::iterator itr = mWMEAlphaMemoryMap.find(wme.getId());

        if (itr == mWMEAlphaMemoryMap.end())
        {
            BLAZE_ERR_LOG(logCategory, "[AlphaMemory].upsertWorkingMemoryElement: attempting to remove non existant WME [id(" << wme.getId() << "), name(" 
                << wme.getName() << "), value(" << oldValue << ")].");
            EA_FAIL();
            return false;
        }

        mWMEAlphaMemoryMap.erase(itr);

        if (BLAZE_IS_LOGGING_ENABLED(logCategory, Logging::SPAM))
        {
            BLAZE_SPAM_LOG(logCategory, "[RETE][AlphaMemory].upsertWorkingMemoryElement: REMOVING WME ID FROM TREE. WME[id("
                << wme.getId() << "), name(" << wme.getName() << "), value(" << oldValue << ")].");
        }


        // Notify All productions that a WME was removed.
        for (auto& joinNodeItr : mJoinNodeList)
        {
            JoinNode* production = joinNodeItr;

            // wme.getValue() is our new upserted value.
            production->notifyUpsertWMEFromAlphaMemory(wme.getId(), wme.getValue());
        }

        return true;
    }

    void AlphaNetwork::removeAlphaMemory(const WorkingMemoryElement& wme)
    {
        if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].removeAlphaMemory: WME[id(" << wme.getId() << "), name(" 
                << mReteNetwork->getStringFromHash(wme.getName()) << "), value(" 
                << mReteNetwork->getStringFromHash(wme.getValue()) << ")" << wme.getValue() << "] from alphaMemory.");
        }

        WMEAttributeMap::iterator alphaMemoryItr = getAlphaMemoryItr(wme.getName(), wme.getValue());
        bool notify = false;
        if (!alphaMemoryItr->second.removeWorkingMemoryElement(wme, notify, mReteNetwork->getLogCategory()))
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].removeAlphaMemory: ERROR removing WME[id(" << wme.getId() << "), name(" 
                << mReteNetwork->getStringFromHash(wme.getName()) << "), value(" 
                << mReteNetwork->getStringFromHash(wme.getValue()) << ")" << wme.getValue() << "] from alphaMemory only.");
        }

        if (alphaMemoryItr->second.isUnreferenced())
        {
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(wme.getName());
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(wme.getValue());
            mWMEAttributeHashMap.erase(alphaMemoryItr);
        }
    }

    void AlphaNetwork::removeAlphaMemoryProvider(WMEAttribute name, WMEAttribute value)
    {
        WMEAttributeMap::iterator alphaMemoryItr = getAlphaMemoryItr(name, value);
        alphaMemoryItr->second.clearHasWMEProvider();

        if (alphaMemoryItr->second.hasListeners())
            return;

        // no more listeners, clean this up
        removeAllWMEsFromAlphaMemory(alphaMemoryItr);

        // might be unreferenced now! 
        if (alphaMemoryItr->second.isUnreferenced())
        {
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(name);
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(value);
            mWMEAttributeHashMap.erase(alphaMemoryItr);
        }
    }

    void AlphaNetwork::incrementHashReferences(WMEAttribute name, WMEAttribute value)
    {
        // we've got a new alpha memory, increase the refcounts in the string table
        mReteNetwork->getWMEManager().getStringTable().referenceHash(name);
        mReteNetwork->getWMEManager().getStringTable().referenceHash(value);
    }

    void AlphaNetwork::removeAllWMEsFromAlphaMemory(WMEAttributeMap::iterator alphaMemoryItr)
    {
        // walk the WMEs in the alpha memory, pass to the WME manager for deletion
        alphaMemoryItr->second.eraseAllAlphaMemories(mReteNetwork->getWMEManager());
    }

    void AlphaNetwork::AlphaMemory::eraseAllAlphaMemories(WMEManager& wmeManager)
    {
        while (!mWMEAlphaMemoryMap.empty())
        {
            auto alphaMemoryIter = mWMEAlphaMemoryMap.begin();
            const WorkingMemoryElement* wme = alphaMemoryIter->second;
            wmeManager.eraseWorkingMemoryElement(wme->getId(), wme->getName(), wme->getValue());
            mWMEAlphaMemoryMap.erase(alphaMemoryIter);
        }
    }

    AlphaNetwork::AlphaMemoryAndNode AlphaNetwork::registerListener(WMEAttribute name, WMEAttribute value, JoinNode& joinNode)
    {
        AlphaNetwork::AlphaMemoryAndNode retVal;
        WMEAttributeMap::iterator alphaMemoryItr = getAlphaMemoryItr(name, value);
        JoinNodeList::const_iterator joinNodeItr = alphaMemoryItr->second.registerListener(joinNode);

        BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].registerListener: name " << RETE_FORMAT_WME(name) << ", value " << RETE_FORMAT_WME(value) << ", joinNode: " << &joinNode << " (Parent:" << joinNode.getParent() << ")");

        retVal.first = &alphaMemoryItr->second;
        retVal.second = joinNodeItr;
        return retVal;
    }

    void AlphaNetwork::unregisterListener(WMEAttribute name, WMEAttribute value, AlphaMemoryAndNode memoryAndNode)
    {
        BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][AlphaNetwork].unregisterListener: name " << RETE_FORMAT_WME(name) << ", value " << RETE_FORMAT_WME(value) << ", joinNode: " << *memoryAndNode.second << " (Parent:" << (*memoryAndNode.second)->getParent() << ")");

        memoryAndNode.first->unregisterListener(memoryAndNode.second);

        if (!memoryAndNode.first->hasWMEProvider() && !memoryAndNode.first->hasListeners())
        {
            // we were waiting on the listeners to clean up orphaned alpha memories
            removeAllWMEsFromAlphaMemory(getAlphaMemoryItr(name, value));
        }

        if (memoryAndNode.first->isUnreferenced())
        {
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(name);
            mReteNetwork->getWMEManager().getStringTable().dereferenceHash(value);
            mWMEAttributeHashMap.erase(eastl::make_pair(name, value));
        }
    }


    JoinNodeList::const_iterator AlphaNetwork::AlphaMemory::registerListener(JoinNode& joinNode)
    {
        return mJoinNodeList.insert(mJoinNodeList.begin(), &joinNode);
    }


    void AlphaNetwork::AlphaMemory::addWMEsFromAlphaMemory(JoinNode& joinNode) const
    {
        WMEAlphaMemoryMap::const_iterator itr = mWMEAlphaMemoryMap.begin();
        WMEAlphaMemoryMap::const_iterator end = mWMEAlphaMemoryMap.end();

        for (; itr != end; ++itr)
        {
            const WorkingMemoryElement& wme = *(itr->second);
            joinNode.notifyAddWMEToAlphaMemory(wme.getId());
        }
    }

    void AlphaNetwork::AlphaMemory::unregisterListener(JoinNodeList::const_iterator joinNodeItr)
    {
        mJoinNodeList.erase(joinNodeItr);
    }

    /*! ************************************************************************************************/
    /*! \brief JoinNode constructor for a head node
    
        \param[in] reteNetwork - pointer to overall reteNetwork.
    ***************************************************************************************************/
    JoinNode::JoinNode(ReteNetwork* reteNetwork)
        : JoinNodeIntrusiveListNode(),
        mProductionBuildInfoMap(BlazeStlAllocator("JoinNode::mProductionBuildInfoMap", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mChildren(BlazeStlAllocator("JoinNode::mChildren", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mBetaMemory(BlazeStlAllocator("JoinNode::mBetaMemory", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mParent(nullptr), 
        mAlphaMemoryMap(BlazeStlAllocator("JoinNode::mAlphaMemoryMap", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mProductionTest(WMEManager::WMEAnyProvider::WME_ANY_STRING, WME_ANY_HASH, WMEManager::WMEAnyProvider()),
        mPNode(nullptr),
        mReteNetwork(reteNetwork),
        mSubstringSearchChildrenCount(0)
    {
        const AlphaNetwork::AlphaMemoryAndNode& alphaMemory = reteNetwork->getAlphaNetwork().registerListener(mProductionTest.getName(), mProductionTest.getValue(), *this);
        alphaMemory.first->addWMEsFromAlphaMemory(*this);
        mAlphaMemoryMap[mProductionTest.getValue()] = alphaMemory;

        // NOTE: we intentionally don't do mReteNetwork->incRuleJoinCount(mProductionTest.mRuleDefinitionId) here
        // because this is the head node of the beta tree, and we don't need to count it.
    }

    /*! ************************************************************************************************/
    /*! \brief JoinNode constructor for a node in the tree (not a head node)
        Start listening for wme updates and populating beta memory.
    
        \param[in] reteNetwork - pointer to overall reteNetwork
        \param[in] parent - parent node for this node
        \param[in] productionTest - Condition for this node.
    ***************************************************************************************************/
    JoinNode::JoinNode(ReteNetwork* reteNetwork, JoinNode& parent, const Condition& productionTest)
        : JoinNodeIntrusiveListNode(),
        mProductionBuildInfoMap(BlazeStlAllocator("JoinNode::mProductionBuildInfoMap", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mChildren(BlazeStlAllocator("JoinNode::mChildren", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mBetaMemory(BlazeStlAllocator("JoinNode::mBetaMemory", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mParent(&parent), 
        mAlphaMemoryMap(BlazeStlAllocator("JoinNode::mAlphaMemoryMap", Search::SearchSlave::COMPONENT_MEMORY_GROUP)),
        mProductionTest(productionTest),
        mPNode(nullptr), 
        mReteNetwork(reteNetwork),
        mSubstringSearchChildrenCount(0)
    {   
        if (productionTest.getSubstringSearch())
        {
            // Hook this join Node up to it's substring matches provider.
            const char8_t* searchString = mReteNetwork->getStringFromHashedConditionValue(mProductionTest);
            mReteNetwork->getReteSubstringTrie().registerListener(searchString, *this);
                        
            // Find initial substring matches and add appropriate wmes to beta memory.
            ++mReteNetwork->getProductionManager().mJnAlpha;
            mReteNetwork->getReteSubstringTrie().addWMEsFromFindMatches(searchString, *this);
        }
        else
        {
            // Hook this join Node up to it's corresponding Alpha Memory.
            if (!getProductionTest().getIsRangeCondition())
            {
                const AlphaNetwork::AlphaMemoryAndNode& alphaMemory = mReteNetwork->getAlphaNetwork().registerListener(getProductionTest().getName(), getProductionTest().getValue(), *this);
                mAlphaMemoryMap[getProductionTest().getValue()] = alphaMemory;
            }
            else
            {
                for (int64_t i = productionTest.getMinRangeValue(); i <= productionTest.getMaxRangeValue(); ++i)
                {
                    // until we have various typed wme's this is unfortunately required.
                    WMEAttribute wmeValue = static_cast<WMEAttribute>(i);
                    const AlphaNetwork::AlphaMemoryAndNode& alphaMemory = reteNetwork->getAlphaNetwork().registerListener(productionTest.getName(), wmeValue, *this);
                    mAlphaMemoryMap[wmeValue] = alphaMemory;
                }
            }

            if (getProductionTest().getUseUnset())
            {
                const AlphaNetwork::AlphaMemoryAndNode& alphaMemory = mReteNetwork->getAlphaNetwork().registerListener(getProductionTest().getName(), WME_UNSET_HASH, *this);
                mAlphaMemoryMap[WME_UNSET_HASH] = alphaMemory;
            }

            ++mReteNetwork->getProductionManager().mJnBeta;
            addWMEsFromParentsBetaMemory();
        }

        mReteNetwork->incRuleJoinCount(mProductionTest.getRuleDefinitionName());
    }

    /*! ************************************************************************************************/
    /*! \brief Destructor for a JoinNode, will also delete the production node, if we have one.
    ***************************************************************************************************/
    JoinNode::~JoinNode()
    {
        if (getProductionTest().getSubstringSearch())
        {
            mReteNetwork->getReteSubstringTrie().unregisterListener(mReteNetwork->getStringFromHashedConditionValue(getProductionTest()), *this);
        }
        else
        {
            WMEAttribute wmeName = getProductionTest().getName();
            // UN-Hook this join Node from it's corresponding Alpha Memory.
            if (!getProductionTest().getIsRangeCondition())
            {
                WMEAttribute wmeValue = getProductionTest().getValue();
                mReteNetwork->getAlphaNetwork().unregisterListener(wmeName, wmeValue, mAlphaMemoryMap[wmeValue]);
            }
            else
            {
                for (int64_t i = getProductionTest().getMinRangeValue(); i <= getProductionTest().getMaxRangeValue(); ++i)
                {
                    // until we have various typed wme's this is unfortunately required.
                    WMEAttribute wmeValue = static_cast<WMEAttribute>(i);
                    mReteNetwork->getAlphaNetwork().unregisterListener(wmeName, wmeValue, mAlphaMemoryMap[wmeValue]);
                }
            }

            if (getProductionTest().getUseUnset())
            {
                mReteNetwork->getAlphaNetwork().unregisterListener(wmeName, WME_UNSET_HASH, mAlphaMemoryMap[WME_UNSET_HASH]);
            }
        }

        mReteNetwork->decRuleJoinCount(mProductionTest.getRuleDefinitionName());
        removeProductionNode();
    }

    void JoinNode::garbageCollectChild(const JoinNode& childNode)
    {
        if (!childNode.getChildren().empty())
        {
            BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[JoinNode] Atempting to garbage collect JoinNode:" << &childNode << ", which has children.");
            return;
        }

        JoinNode::JoinNodeMap::iterator childItr = mChildren.find(childNode.getProductionTest());

        if (childItr != mChildren.end())
        {
            JoinNode *child = childItr->second;

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[JoinNode].garbageCollectChild: Condition(" << mProductionTest.toString() << ") Garbage collection is removing child JoinNode:" << child << " '" 
                          << child->getProductionTest().toString() << "'");
            }
            if (childItr->first.getSubstringSearch() && mSubstringSearchChildrenCount > 0)
            {
                --mSubstringSearchChildrenCount;
            }
            mChildren.erase(childItr);
            mReteNetwork->getJoinNodeFactory().deleteJoinNode(child);
        }
    }

    const ProductionToken* JoinNode::getTokenFromBetaMemory(WMEId id) const
    {
        BetaMemory::const_iterator itr = mBetaMemory.find(id);
        if (itr != mBetaMemory.end())
        {
            return itr->second;
        }

        return nullptr;
    }

    void JoinNode::notifyChildrenAddToken(const ProductionToken& token)
    {
        const bool findMatchingChildren = (mSubstringSearchChildrenCount == 0) && mChildren.size() > 0;
        if (findMatchingChildren)
        {
            // We have to loop over all children to test the condition of each child.
            for (auto itr : mChildren)
            {
                JoinNode* joinNode = itr.second;
                auto attrName = itr.first.getName();

                auto* valueMap = mReteNetwork->getWMEManager().getWorkingMemoryElements(token.mId, attrName);
                if (valueMap != nullptr)
                {
                    for (auto valueItr : *valueMap)
                    {
                        if (itr.first.test(valueItr.second->getValue()))
                        {
                            joinNode->notifyAddWMEToParentBetaMemory(token);
                            break;
                        }
                    }
                }
                else if (itr.first.getNegate())
                {
                    // negate conditions need to be notified about the addition/removal of tokens in case the token was never in the alpha memory for the condition
                    joinNode->notifyAddWMEToParentBetaMemory(token);
                }
            }
        }
        else
        {
            // Currently string matching uses a Trie which breaks the invariant that matching WME.getValue() == childJoinNode.mProductionTest.getValue()
            // (This seems to be because WME.getValue() is a hash of 'GAME', while childJoinNode.mProductionTest.getValue() is a hash of 'GAM')
            // Until such time when we can determine *all* the JoinNode children that could be affected by a single WME node, 
            // the above optimization cannot be used and we must iterate them all.
            for (auto itr : mChildren)
            {
                itr.second->notifyAddWMEToParentBetaMemory(token);
            }
        }
    }

    void JoinNode::notifyChildrenRemoveToken(const ProductionToken& token)
    {
        const bool findMatchingChildren = (mSubstringSearchChildrenCount == 0) && mChildren.size() > 0;
        if (findMatchingChildren)
        {
            // We have to loop over all children to test the condition of each child.
            for (auto itr : mChildren)
            {
                JoinNode* joinNode = itr.second;
                auto attrName = itr.first.getName();

                auto* valueMap = mReteNetwork->getWMEManager().getWorkingMemoryElements(token.mId, attrName);
                if (valueMap != nullptr)
                {
                    for (auto valueItr : *valueMap)
                    {
                        if (itr.first.test(valueItr.second->getValue()))
                        {
                            joinNode->notifyRemoveWMEFromParentBetaMemory(token);
                            break;
                        }
                    }
                }
                else if (itr.first.getNegate())
                {
                    // negate conditions need to be notified about the addition/removal of tokens in case the token was never in the alpha memory for the condition
                    joinNode->notifyRemoveWMEFromParentBetaMemory(token);
                }
            }
        }
        else
        {
            // Currently string matching uses a Trie which breaks the invariant that matching WME.getValue() == childJoinNode.mProductionTest.getValue()
            // (This seems to be because WME.getValue() is a hash of 'GAME', while childJoinNode.mProductionTest.getValue() is a hash of 'GAM')
            // Until such time when we can determine *all* the JoinNode children that could be affected by a single WME node, 
            // the above optimization cannot be used and we must iterate them all.
            for (auto itr : mChildren)
            {
                itr.second->notifyRemoveWMEFromParentBetaMemory(token);
            }
        }
    }

    void JoinNode::notifyChildrenUpsertToken(const ProductionToken& token)
    {
        // we just need to traverse the tree to tell pnodes below us to update.
        JoinNodeMap::iterator itr = mChildren.begin();
        JoinNodeMap::iterator end = mChildren.end();
        for (; itr != end; ++itr)
        {
            JoinNode* joinNode = itr->second;
            joinNode->notifyUpsertWMEFromParentBetaMemory(token);
        }
    }

    void JoinNode::addWMEsFromParentsBetaMemory()
    {
        // Walk the beta memory of our parent
        BetaMemory::const_iterator iter = getParent()->getBetaMemory().begin();
        BetaMemory::const_iterator end = getParent()->getBetaMemory().end();

        for (; iter != end; ++iter)
        {
            const ProductionToken* token = iter->second;
            notifyAddWMEToParentBetaMemory(*token);
        }
    }

    void JoinNode::notifyAddWMEToAlphaMemory(WMEId wmeId)
    {
        if (mProductionTest.getNegate())
        {
            removeWME(wmeId);
        }
        else
        {
            addWME(wmeId);
        }
    }

    void JoinNode::addWME(WMEId wmeId)
    {
        const ProductionToken* token = getTokenFromBetaMemory(wmeId);

        // We don't have the token in our memory, so check our parents memory.
        if (token == nullptr)
        {
            // We don't have a token in our beta memory and our parent is the head node so create
            // else search the parents memory for a token.
            if (isHeadNode())
            {
                if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
                {
                    BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].addWME: Condition(" << mProductionTest.toString() << ") - CREATING NEW PRODUCTION TOKEN AT HEAD NODE wme[id(" << wmeId << ")]");
                }

                token = &mReteNetwork->getProductionTokenFactory().createProductionToken(wmeId);
            }
            else
            {
                if (mParent != nullptr)
                    token = mParent->getTokenFromBetaMemory(wmeId);
            }

            // A token was found so add it to our beta memory and inform our children
            // including p-nodes.
            if (token != nullptr)
            {
                mBetaMemory.insert(eastl::make_pair(token->mId, token));

                if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
                {
                    BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].addWME: - ADDING MATCHED ITEM Id(" 
                        << token->mId << ") meeting Condition(" << mProductionTest.toString() << ").");
                }

                if (hasProductionNode())
                {
                    getProductionNode().notifyParentAddedToken(*token);
                }

                // Found a WME so continue building the Beta Tree
                if (!getProductionBuildInfoMap().empty())
                {
                    buildBetaTree();
                }
                notifyChildrenAddToken(*token);
            }
            else
            {
                // omit adding token to this node if was not on parent
                if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::SPAM))
                {
                    BLAZE_SPAM_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].addWME: Condition(" << mProductionTest.toString() << ") - MATCHED ITEM NOT FOUND ON PARENT '" 
                              << mParent->getProductionTest().toString()
                              << "', NO OP. id(" << wmeId << ").");
                }
            }
        }
        // else:  token was already on this node, NO-OP
    }

    void JoinNode::buildBetaTree()
    {
        ProductionBuildInfoMap::iterator itr = getProductionBuildInfoMap().begin();
        ProductionBuildInfoMap::iterator end = getProductionBuildInfoMap().end();

        for (;itr != end; ++itr)
        {
            ProductionBuildInfo& info = *itr->second;

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].buildBetaTree: Build production(" << info.getProductionInfo().id
                    << ") info building out found tree for parent Join Node:" << &info.getParent() << "("
                    << info.getParent().getProductionTest().toString() << ").");
            }

            mReteNetwork->getProductionManager().buildReteTree(info.getParent(), info.getCondiitonBlock(), info.getConditionBlockItr(),
                info.getCondiitonBlock().end(), info.getProductionInfo());
        }

        removeProductionBuildInfo(ProductionBuildInfoMapItrPair(getProductionBuildInfoMap().begin(), getProductionBuildInfoMap().end())); // Post Increment the iterator and pass in the old value which will delete the iterator
    }

    void JoinNode::notifyRemoveWMEFromAlphaMemory(WMEId wmeId)
    {
        if (mProductionTest.getNegate())
        {
            addWME(wmeId);
        }
        else
        {
            removeWME(wmeId);
        }
    }


    void JoinNode::notifyUpsertWMEFromAlphaMemory(WMEId wmeId, WMEAttribute newValue)
    {
        // Test if the new value for this wme id still meets our condition.
        // Meeting our condition test means that our token is still valid, no need to modify the tree.
        if (mProductionTest.getNegate())
        {
            if (mProductionTest.test(newValue))
            {
                // we did meet the condition, but are a negate
                addWME(wmeId);
                mReteNetwork->incRuleUpsertsCount(mProductionTest.getRuleDefinitionName());
            }
        }
        else
        {
            if (!(mProductionTest.test(newValue)))
            {
                // we did not meet the condition, we need to just remove the WME
                removeWME(wmeId);
                mReteNetwork->incRuleUpsertsCount(mProductionTest.getRuleDefinitionName());
            }
        }
    }


    void JoinNode::removeWME(WMEId wmeId)
    {
        BetaMemory::iterator itr = mBetaMemory.find(wmeId);

        if (itr != mBetaMemory.end())
        {
            const ProductionToken& token = *itr->second;
            mBetaMemory.erase(itr);

            notifyChildrenRemoveToken(token);

            if (hasProductionNode())
            {
                getProductionNode().notifyParentRemovedToken(token);
            }

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].removeWME: - REMOVE MATCHED ITEM id(" 
                    << wmeId << ") meeting Condition(" << mProductionTest.toString() << ").");
            }

            // check to see if we created this token.
            if (isHeadNode())
            {
                if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::SPAM))
                {
                    BLAZE_SPAM_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].removeWME: Condition(" << mProductionTest.toString() << ") - Head Node Delete matched item id(" << token.mId << ")");
                }

                // Const cast is required here because we store off the memory
                // internally as a const *.  Production Tokens are immutable
                // and are never modified except when they are destroyed.
                mReteNetwork->getProductionTokenFactory().deleteProductionToken(const_cast<ProductionToken*>(&token));
            }
        }
        // else: Token not in beta memory, NO-OP
    }


    // If the token has a WME that is in this productions Alpha Memory
    // Add it to this Beta Memory and notify it's children.
    void JoinNode::notifyAddWMEToParentBetaMemory(const ProductionToken& token)
    {
        // If we already know about the token, we can skip this
        BetaMemory::iterator itr = mBetaMemory.find(token.mId);
        if (itr != mBetaMemory.end())
        {
            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::SPAM))
            {
                BLAZE_SPAM_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyAddWMEToParentBetaMemory: Condition(" << mProductionTest.toString() << ") - ALREADY TRACKING MATCHED ITEM Id(" << token.mId << ") in beta memory");
            }
            return;
        }

        // if node's alpha memory/activater has wme, do updates as needed
        bool alphaMemoryContainsWME = false;
        if (mProductionTest.getSubstringSearch()) 
        {
            alphaMemoryContainsWME = mReteNetwork->getReteSubstringTrie().substringMatchesWME(mReteNetwork->getStringFromHashedConditionValue(mProductionTest), token.mId);
        }
        else
        {
            AlphaMemoryMap::const_iterator iter = mAlphaMemoryMap.begin();
            AlphaMemoryMap::const_iterator end = mAlphaMemoryMap.end();
            for (; iter != end; ++iter)
            {
                const AlphaNetwork::AlphaMemory* alphaMemory = iter->second.first;
                if ( (alphaMemory != nullptr) && (alphaMemory->containsWME(token.mId)))
                {
                    // only need a token from one value in the list.
                    alphaMemoryContainsWME = true;
                    break;
                }
            }
        }

        if ((!mProductionTest.getNegate() && alphaMemoryContainsWME) || (mProductionTest.getNegate() && !alphaMemoryContainsWME))
        {
            mBetaMemory.insert(eastl::make_pair(token.mId, &token));

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyAddWMEToParentBetaMemory: - ADDING MATCHED ITEM Id(" << token.mId << ") meeting Condition(" << mProductionTest.toString() << ").");
            }

            // Found a WME so continue building the Beta Tree
            if (!getProductionBuildInfoMap().empty())
            {
                buildBetaTree();
            }

            notifyChildrenAddToken(token);

            // Only notify our pNode if we didn't just build out our beta tree
            // thus creating our pNode and already notifying it.
            if (hasProductionNode())
            {
                getProductionNode().notifyParentAddedToken(token);
            }
        }
        else
        {
            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyAddWMEToParentBetaMemory: Condition(" << mProductionTest.toString() << ") - DID NOT ADD TO MATCHED ITEM id(" << token.mId 
                          << ") to beta memory [alphaMemoryContainsWME=" << (alphaMemoryContainsWME ? "true" : "false") << "]");
            }
        }
    }


    // if we contain the wme in our memory, remove it from our beta memory
    // and notify our children that it has been removed.
    void JoinNode::notifyRemoveWMEFromParentBetaMemory(const ProductionToken& token)
    {   
        BetaMemory::iterator itr = mBetaMemory.find(token.mId);
        if (itr != mBetaMemory.end())
        {
            // Need to erase this before notifying the production listeners
            // so that they can potentially look for another token in
            // the production node.
            mBetaMemory.erase(itr);

            notifyChildrenRemoveToken(token);

            if (hasProductionNode())
            {
                getProductionNode().notifyParentRemovedToken(token);
            }

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyRemoveWMEFromParentBetaMemory: - REMOVE MATCHED ITEM id(" 
                    << token.mId << ") meeting Condition '" << mProductionTest.toString() << "'.");
            }
        }
        else
        {
            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyRemoveWMEFromParentBetaMemory: Condition(" << mProductionTest.toString() << ") - ITEM NOT IN BETA MEMORY. DID NOT REMOVE. id(" << token.mId << ")");
            }
        }
    }

    void JoinNode::upsertWME(WMEId wmeId)
    {
        BetaMemory::iterator itr = mBetaMemory.find(wmeId);

        // Find the beta memory token
        if (itr != mBetaMemory.end())
        {
            const ProductionToken& token = *itr->second;

            notifyChildrenUpsertToken(token);

            if (hasProductionNode())
            {
                getProductionNode().notifyParentUpsertToken(token);
            }

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].upsertWME: - Updated MATCHED ITEM id("
                    << wmeId << ") meeting Condition(" << mProductionTest.toString() << ").");
            }
        }
    }

    void JoinNode::notifyUpsertWMEFromParentBetaMemory(const ProductionToken& token)
    {
        BetaMemory::iterator itr = mBetaMemory.find(token.mId);
        if (itr != mBetaMemory.end())
        {
            notifyChildrenUpsertToken(token);

            if (hasProductionNode())
            {
                getProductionNode().notifyParentUpsertToken(token);
            }

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyRemoveWMEFromParentBetaMemory: - REMOVE MATCHED ITEM id("
                    << token.mId << ") meeting Condition(" << mProductionTest.toString() << ").");
            }
        }
        else
        {
            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].notifyRemoveWMEFromParentBetaMemory: Condition(" << mProductionTest.toString() << ") - ITEM NOT IN BETA MEMORY. DID NOT REMOVE. id(" << token.mId << ")");
            }
        }
    }


    void JoinNode::addProductionBuildInfo(const ConditionBlock& conditionBlock, 
        ConditionBlock::const_iterator itr, ProductionInfo& productionInfo)
    {

        ProductionBuildInfo& info = mReteNetwork->getProductionBuildInfoFactory().createProductionBuildInfo(*this, conditionBlock, itr, productionInfo);

        getProductionBuildInfoMap().insert(eastl::make_pair(productionInfo.id, &info));
        uint64_t depth = itr - conditionBlock.begin();

        if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].addProductionBuildInfo: Condition(" << mProductionTest.toString() << ") - NO MATCHED ITEMS for searcher id(" << productionInfo.id 
                << ") matching this condition currently at depth (" << depth <<").");
        }

        productionInfo.mListener->addProductionBuildInfo(info, depth, mReteNetwork->getLogCategory());
        if (isGarbageCollectible())
        {
            mReteNetwork->clearForGarbageCollection(*this);
        }
    }

    void JoinNode::removeProductionBuildInfo(ProductionBuildInfo& productionBuildInfo)
    {
        ProductionBuildInfoMapItrPair itrPair = getProductionBuildInfoMap().equal_range(productionBuildInfo.getProductionInfo().id);

        if (itrPair.first != itrPair.second)
        {
            removeProductionBuildInfo(itrPair);
        }
        else if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].removeProductionBuildInfo: Condition(" << mProductionTest.toString() << ") ERROR productionBuildInfo not found for id(" 
                    << productionBuildInfo.getProductionInfo().id << ")");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Removes all production build infos indicated by the iter pair into the multimap.
        Removes from both the productionListener and this join node.
    
        \param[in] itrPair - the iterator pair intot he multimap of production Id keyd productionbuildinfos
        we are removing
    ***************************************************************************************************/
    void JoinNode::removeProductionBuildInfo(const ProductionBuildInfoMapItrPair& itrPair)
    {
        for (ProductionBuildInfoMap::iterator itr = itrPair.first; itr != itrPair.second; ++itr)
        {
            ProductionBuildInfo& productionBuildInfo = *itr->second;

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].removeProductionBuildInfo: for searcher id("
                          << productionBuildInfo.getProductionInfo().id << "), Condition(" << mProductionTest.toString() << ")");
            }

            productionBuildInfo.getProductionInfo().mListener->getProductionBuildInfoList().remove(productionBuildInfo);
            mReteNetwork->getProductionBuildInfoFactory().deleteProductionBuildInfo(&productionBuildInfo);
        }
        getProductionBuildInfoMap().erase(itrPair.first, itrPair.second);

        // If this isn't the head node and there are no more references
        // Children, pNode, productionBuildInfo, etc.  Then this node is ready
        // for garbage collection.
        if (getProductionBuildInfoMap().empty() && getChildren().empty() && !hasProductionNode() && !isHeadNode())
        {
            mReteNetwork->flagForGarbageCollection(*this);
        }
    }

    uint32_t JoinNode::getAlphaMemorySize() const 
    {
        uint32_t memorySize = 0;
        AlphaMemoryMap::const_iterator iter = mAlphaMemoryMap.begin();
        for (; iter != mAlphaMemoryMap.end(); ++iter)
        {
            const AlphaNetwork::AlphaMemory* alpha = iter->second.first;
            memorySize += alpha->memorySize();
        }
        return memorySize;
    }

    void JoinNode::printConditions() const
    {
        if (isHeadNode())
            return;

        mParent->printConditions();
        if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode:" << this << "] Condition: '" << mProductionTest.toString());
        }
    }

    void JoinNode::getConditions(eastl::vector<eastl::string>& conditions, uint32_t depth) const
    {
        if (isHeadNode())
        {
            return;
        }

        mParent->getConditions(conditions, depth + 1);
        conditions.push_back(mProductionTest.toString());
    }
    
    size_t ProductionNode::getTokenCount() const
    {
        return mParent.getBetaMemory().size();
    }

    const BetaMemory& ProductionNode::getTokens() const
    {
        return mParent.getBetaMemory();
    }

    void ProductionNode::printConditions() const
    {
        mParent.printConditions();
    }

    void ProductionNode::notifyParentAddedToken(const ProductionToken& token)
    {
        ProductionInfoMap::iterator itr = mProductionInfoMap.begin();
        ProductionInfoMap::iterator end = mProductionInfoMap.end();

        for (; itr != end; ++itr)
        {
            ProductionInfo& info = itr->second;
            bool notifyOnUpdate = info.mListener->notifyOnTokenAdded(info);

            // If this is the first token notify the listeners
            // that in case they want to change their notification
            // status.
            if (mParent.getBetaMemory().size() == 1)
            {
                info.mListener->onProductionNodeHasTokens(*this, info);
            }

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].notifyParentAddedToken: - ADD MATCH id(" << token.mId << ") for searcher id("
                          << info.id << ") with fitScore (" << info.fitScore << "), notify=(" << (notifyOnUpdate ? "true" : "false") << ")");
            }

            if (notifyOnUpdate)
            {
                info.mListener->onTokenAdded(token, info);
            }
        }
    }

    void ProductionNode::notifyParentRemovedToken(const ProductionToken& token)
    {
        ProductionInfoMap::iterator itr = mProductionInfoMap.begin();
        ProductionInfoMap::iterator end = mProductionInfoMap.end();

        for (; itr != end; ++itr)
        {
            ProductionInfo& info = itr->second;
            bool notifyOnUpdate = info.mListener->notifyOnTokenRemoved(info);

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].notifyParentRemovedToken: - REMOVE MATCH id(" << token.mId
                          << ") for Production id=(" << info.id << ") with fitScore (" << info.fitScore << "), notify=(" << (notifyOnUpdate ? "true" : "false") << ")");
            }
            if (notifyOnUpdate)
            {
                info.mListener->onTokenRemoved(token, info);
            }

            // If this is the last token notify the listeners
            if (mParent.getBetaMemory().empty())
            {
                info.mListener->onProductionNodeDepletedTokens(*this);
            }
        }
    }


    void ProductionNode::notifyParentUpsertToken(const ProductionToken& token)
    {
        ProductionInfoMap::iterator itr = mProductionInfoMap.begin();
        ProductionInfoMap::iterator end = mProductionInfoMap.end();

        for (; itr != end; ++itr)
        {
            ProductionInfo& info = itr->second;
            info.mListener->onTokenUpdated(token, info);
        }
    }

    bool ProductionNode::addProductionInfo(const ProductionInfo& info)
    {
        // TODO: Production info should be slab allocated and passed in as a 
        // const reference to avoid the copy into the production info map.

        eastl::pair<ProductionInfoMap::iterator, bool> insertPair = mProductionInfoMap.insert(ProductionInfoMap::value_type(info.id, info));
        bool success = insertPair.second;
        if (isGarbageCollectible())
        {
            mParent.getReteNetwork().clearForGarbageCollection(*this);
        }

        if (success)
        {
            ProductionInfo& pInfo = insertPair.first->second;
            pInfo.pNode = this;
            bool notifyOnUpdate = pInfo.mListener->notifyOnTokenAdded(pInfo);

            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].addProductionInfo: Searcher(" << pInfo.id << ") reached end of RETE tree with "
                << mParent.getBetaMemory().size() << " matched items having fitScore(" 
                << pInfo.fitScore << "), notifySearcher(" << (notifyOnUpdate ? "true" : "false") << ")");
            pInfo.mListener->getProductionNodeList().push_back(this);

            if (!mParent.getBetaMemory().empty())
            {
                pInfo.mListener->onProductionNodeHasTokens(*this, pInfo);
                if (notifyOnUpdate)
                {
                    // Notify this production of all current matches, we've reached a matching endpoint.
                    BetaMemory::const_iterator itr = mParent.getBetaMemory().begin();
                    BetaMemory::const_iterator end = mParent.getBetaMemory().end();
                    for (; itr != end; ++itr)
                    {
                        const ProductionToken& token = *itr->second;
                        BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].addProductionInfo: Searcher(" << pInfo.id << ") HAS_MATCH id(" << token.mId 
                                  << ") with fitScore(" << info.fitScore << ")");
                        if (!pInfo.mListener->onTokenAdded(token, pInfo))
                        {
                            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].addProductionInfo: Searcher (" << pInfo.id 
                                      << ") doesn't want any more matches.");
                            break;
                        }
                    }
                }
                else
                {
                    BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].addProductionInfo: Searcher " << pInfo.id << " skipping matched items.");
                }
            }
        }
        else
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].addProductionInfo: Searcher(" << info.id 
                      << ") already exists, not inserted.");
        }
        return success;
    }

    /*! ************************************************************************************************/
    /*! \brief Removes the given production id from the production info map of listeners
        at this production node.
    
        \param[in] id - the production id to remove
        \return true if the production info was removed, false otherwise
    ***************************************************************************************************/
    bool ProductionNode::removeProductionInfo(ProductionId id)
    {
        ProductionInfoMap::iterator itr = mProductionInfoMap.find(id);

        if (itr != mProductionInfoMap.end())
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].removeProductionInfo removing info for id (" << id << ").");
            mProductionInfoMap.erase(itr);

            // No one else is left listening at this production node. flag for GC
            if (mProductionInfoMap.empty())
            {
                mParent.getReteNetwork().flagForGarbageCollection(*this);
                // We only add ourselves as collectible, and then wait for the idle
                // to traverse the tree and delete any nodes.
            }

            return true;
        }

        BLAZE_WARN_LOG(mReteNetwork->getLogCategory(), "[RETE][ProductionNode].removeProductionInfo info for id (" << id << ") was not found.");
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Constructor, sets our parent node
    
        \param[in] parent - the parent join node for this production node
    ***************************************************************************************************/
    ProductionNode::ProductionNode(ReteNetwork *reteNetwork, JoinNode& parent, ProductionNodeId productionNodeId)
        : ProductionNodeIntrusiveListNode(), 
        mParent(parent),
        mProductionInfoMap(BlazeStlAllocator("ProductionNode::mProductionInfoMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mReteNetwork(reteNetwork),
        mProductionNodeId(productionNodeId)
    {
        mpNext = nullptr;
        mpPrev = nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Destructor.
    ***************************************************************************************************/
    ProductionNode::~ProductionNode()
    {
    }

    ProductionNode& JoinNode::getProductionNode()
    {
        if (mPNode == nullptr)
        {
            mPNode = &mReteNetwork->getProductionNodeFactory().createProductionNode(mReteNetwork, *this);
        }

        return *mPNode;
    }

    /*! ************************************************************************************************/
    /*! \brief Removes a production node from this join node
    ***************************************************************************************************/
    void JoinNode::removeProductionNode()
    {
        if (hasProductionNode())
        {
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[JoinNode] Removing ProductionNode:" << mPNode << "PnodeId: " << mPNode->getProductionNodeId());

            // Remove the node from any (garbage collection) list it may still be part of: 
            if (mPNode->mpNext != nullptr && mPNode->mpPrev != nullptr)
            {
                ProductionNodeIntrusiveList::remove(*mPNode);
                mPNode->mpNext = nullptr;
                mPNode->mpPrev = nullptr;
            }

            mReteNetwork->getProductionNodeFactory().deleteProductionNode(mPNode);
            mPNode = nullptr;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Get the Join Node for this condition.  If none exists, create a new one.

        \param[in] condition - 
        \return the joinNode child
    *************************************************************************************************/
    JoinNode& JoinNode::getOrCreateChild(const Condition& condition)
    {
        JoinNode* joinNode = nullptr;
        JoinNode::JoinNodeMap::iterator childItr = mChildren.find(condition);

        if (childItr != mChildren.end())
        {
            joinNode = childItr->second;
            // a node that has been flagged to be deleted was requested. Clear the flag so we don't delete
            // while trying to build out the tree
            if (joinNode->isGarbageCollectible())
            {
                mReteNetwork->clearForGarbageCollection(*joinNode);
            }
        }
        else
        {
            TimeValue startTime = TimeValue::getTimeOfDay();

            joinNode = &mReteNetwork->getJoinNodeFactory().createJoinNode(mReteNetwork, *this, condition);

            mChildren[condition] = joinNode;

            if (BLAZE_IS_LOGGING_ENABLED(mReteNetwork->getLogCategory(), Logging::TRACE))
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[RETE][JoinNode].getJoinNode: NEW Child Join Node: " << joinNode << " '"
                          << condition.toString() << "' (Parent: " << joinNode->mParent << ").");
            }

            if (condition.getSubstringSearch())
                ++mSubstringSearchChildrenCount;

            ++mReteNetwork->getProductionManager().mJnCreated;
            mReteNetwork->getProductionManager().mJnCreatedTime += TimeValue::getTimeOfDay() - startTime;
        }

        return *joinNode;
    }

} // namespace Rete
} // namespace GameManager
} // namespace Blaze

