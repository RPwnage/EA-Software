/*! ************************************************************************************************/
/*!
\file productionmanager.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "EAStdC/EAHashCRC.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/productionmanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    ProductionManager::ProductionManager(ReteNetwork* reteNetwork)
        : mReteNetwork(reteNetwork),
        mPbiCount(0),
        mApCount(0)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    ProductionManager::~ProductionManager()
    {
    }

    void ProductionManager::addProduction(LegacyProductionListener& pl)
    {
        addProduction(pl, pl);
    }

    void ProductionManager::addProduction(ProductionListener& pl, ReteRuleProvider& rP)
    {
        // Build out the beta network
        // For each test in production
        // - Lookup children by Attr/Value(s) @ current join node
        // - if exists add the production id to the ids list of this join node
        // - else add this production to children @ current join node & link in alpha memory to alpha network
        // - navigate to child and repeat with next test in production.

        // p-nodes
        // After iterating over all tests, add a p-node as the last child which contains the full set of complete matches
        // for this production.  Upon acquiring a complete match p-nodes signals this to the system.  Potentially
        // the p-node could run other rules that don't fit into the rete network, do final fit score calculations, and attempt to finalize a game.

        // Update Alpha Memory
        // For each new alpha memory added iterate all games & add matching games to alpha memory
        // NOTE: Potentially alpha memory for a game can be entirely built out based on configuration to avoid this step.
        // Trade off is less dynamic for speed in beta productions, will need to take into account a reconfigure.

        /////////////////////////////////////////////////////

        TimeValue blank;
        mPbiCount = 0;
        mPbiTime = blank;
        mApCount = 0;
        mApTime = blank;
        mJnWalked = 0;
        mJnBeta = 0;
        mJnAlpha = 0;
        mJnCreated = 0;
        mJnCreatedTime = blank;

        const ConditionBlockList& conditionsList = rP.getConditions();
        ConditionBlockList::const_iterator conditionsListItr = conditionsList.begin();
        ConditionBlockList::const_iterator conditionsListEnd = conditionsList.end();

        // Note: were assuming this is the first enum...
        ConditionBlockId blockId = CONDITION_BLOCK_BASE_SEARCH;

        for (; conditionsListItr != conditionsListEnd; ++conditionsListItr)
        {
            const ConditionBlock& conditionBlock = *conditionsListItr;
            ConditionBlock::const_iterator conditionBlockItr = conditionBlock.begin();
            ConditionBlock::const_iterator conditionBlockEnd = conditionBlock.end();

            JoinNode& headJoinNode = mReteNetwork->getBetaNetwork();

            ProductionInfo productionInfo(pl.getProductionId(), 0, &pl, blockId);
            buildReteTree(headJoinNode, conditionBlock, conditionBlockItr, conditionBlockEnd, productionInfo);

            blockId = (ConditionBlockId)(blockId + 1);
        }
        BLAZE_SPAM_LOG(mReteNetwork->getLogCategory(), "[ProductionManager].addProduction: Search times: pbi(" << mPbiCount << ") " << mPbiTime.getMicroSeconds() << " us, ap("
            << mApCount << ") " << mApTime.getMicroSeconds() << " us, jnWalked(" << mJnWalked << "), jnCreated(" << mJnCreated << ") "
            << mJnCreatedTime.getMicroSeconds() << " us, jnAlpha(" << mJnAlpha << "), jnBeta(" << mJnBeta << ")");
    }

    void ProductionManager::expandProduction(ProductionListener& pl) const
    {
        // RETE_TODO: production manager intelligent expansion of ReteTree.
    }

    /*! ************************************************************************************************/
    /*! \brief  Remove the production listener from any production nodes it has been listening to.
            We rely on garbage collection to clean up any JoinNodes that have been orphaned.
        \param[in] pl - the production listener that is removing itself
    *************************************************************************************************/  
    void ProductionManager::removeProduction(ProductionListener& pl) const
    {
        BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(),"[ProductionManager].removeProduction: Removing Production Listener with id (" << pl.getProductionId() << ").");

        // NOTES: GarbageCollection - We may want to go through the entire production tree and remove all
        // Join Nodes that are no longer in use, but for now just removing the
        // Production Info from the Production Nodes.
        ProductionNodeList& pnl = pl.getProductionNodeList();
        ProductionNodeList::iterator itr = pnl.begin();
        ProductionNodeList::iterator end = pnl.end();

        for (; itr != end; ++itr)
        {
            if(*itr == nullptr)
            {
                // this shouldn't happen
                BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[ProductionManager].removeProduction: Production Listener with id (" << pl.getProductionId() << ") had nullptr production node.");
                // we continue so we can clean up any valid production node entries
                continue;
            }
            ProductionNode& node = **itr;
            if (!node.removeProductionInfo(pl.getProductionId()))
            {
                BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[ProductionManager].removeProduction: Failed to remove production info for Production Listener with id (" << pl.getProductionId() << ").");
            }
        }
        pnl.clear();

        ProductionBuildInfoList& pbil = pl.getProductionBuildInfoList();

        while (!pbil.empty())
        {
            ProductionBuildInfo& info = pbil.front();
            info.getParent().removeProductionBuildInfo(info); // This removes the info from pbil since it is an intrusive list.
        }
    }

    void ProductionManager::buildReteTree(JoinNode& currentJoinNode, const ConditionBlock& conditionBlock,
        ConditionBlock::const_iterator itr, ConditionBlock::const_iterator end, ProductionInfo& productionInfo)
    {
        TimeValue startTime = TimeValue::getTimeOfDay();
        ++mJnWalked;
        
        if (!currentJoinNode.hasTokens())
        {
            currentJoinNode.addProductionBuildInfo(conditionBlock, itr, productionInfo);

            ++mPbiCount;
            mPbiTime += TimeValue::getTimeOfDay() - startTime;
        }
        else
        {
            if (itr != end)
            {
                const OrConditionList& orConditions = *itr;

                if (orConditions.empty())
                {
                    // This can occur if a rule is not disabled but doesn't add any conditions
                    // but creates an orConditions search tree.  The rules shouldn't do this.
                    // but if it happens then skip this or conditions block.
                    BLAZE_ERR_LOG(mReteNetwork->getLogCategory(), "[ProductionManager].buildReteTree: ERROR found empty orConditions list, skipping.");
                    buildReteTree(currentJoinNode, conditionBlock, itr + 1, end, productionInfo);
                }

                // Build out the ConditionInfo/JoinNodes based on the Conditions that were provided:  (via pushNode/getOrCreateChild)
                OrConditionList::const_iterator orItr = orConditions.begin();
                OrConditionList::const_iterator orEnd = orConditions.end();
                for (; orItr != orEnd; ++orItr)
                {
                    const ConditionInfo& conditionInfo = *orItr;

                    productionInfo.mListener->pushNode(conditionInfo, productionInfo);
                    // track the maximum expansion any one particular branch of this rules joinNodes has.
                    mReteNetwork->setRuleMaxChildJoinCount(conditionInfo.condition.getRuleDefinitionName(), currentJoinNode.getChildren().size());
                    JoinNode& nextJoinNode = currentJoinNode.getOrCreateChild(conditionInfo.condition);
                    buildReteTree(nextJoinNode, conditionBlock, itr + 1, end, productionInfo);
                    productionInfo.mListener->popNode(conditionInfo, productionInfo);
                }
            }
            else
            {
                // At an endpoint of the tree, so build the P-Node.
                ProductionNode& pNode = currentJoinNode.getProductionNode();

                // Adds the p-node into the productionInfo's list of known p-nodes.
                if (!pNode.addProductionInfo(productionInfo))
                {
                    BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[ProductionManager].buildReteTree: Ignoring attempt to add production id(" << productionInfo.id << ") due to multiple attempts.");
                }
                
                ++mApCount;
                mApTime += TimeValue::getTimeOfDay() - startTime;
            }
        }
    }


} // Rete
} // namespace GameManager
} // namespace Blaze
