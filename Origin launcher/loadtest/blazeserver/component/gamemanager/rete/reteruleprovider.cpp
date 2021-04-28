/*! ************************************************************************************************/
/*!
\file reteruleprovider.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rete/reteruleprovider.h"
#include "gamemanager/rete/reterule.h"
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
    ReteRuleProvider::ReteRuleProvider()
        : mUnevaluatedDedicatedServerFitScore(0)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    ReteRuleProvider::~ReteRuleProvider()
    {
    }

    void ReteRuleProvider::initializeReteProductions(Rete::ReteNetwork& reteNetwork, ProductionListener* pL)
    {
        TimeValue startTime = TimeValue::getTimeOfDay(), initTime, searchTime;

        BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].initializeReteProductions: Initializing " << Rete::CONDITION_BLOCK_MAX_SIZE << " condition blocks, removing known productionBuildInfo's.")
        // Remove any remaining production build info.
        ProductionBuildInfoList& pbil = pL->getProductionBuildInfoList();

        while (!pbil.empty())
        {
            ProductionBuildInfo& info = pbil.front();
            info.getParent().removeProductionBuildInfo(info); // This removes the info from pbil since it is an intrusive list.
        }

        // clear the conditionBlockList
        getConditions().clear();
        getConditions().resize(Rete::CONDITION_BLOCK_MAX_SIZE);

        // clear the unevaluated fit score.
        mUnevaluatedDedicatedServerFitScore = 0;



        const Rete::ReteRuleContainer& reteRuleContainer = getReteRules();
        Rete::ReteRuleContainer::const_iterator i = reteRuleContainer.begin();
        Rete::ReteRuleContainer::const_iterator e = reteRuleContainer.end();

        getMatchAnyRuleList().clear();
        for (; i != e; ++i)
        {
            const Rete::ReteRule* rule = *i;
            if (!rule->isDisabled())
            {
                if (rule->isDedicatedServerEnabled())
                {
                    addUnevaluatedDedicatedServerFitScore(rule->getMaxFitScore());
                }

                // if no conditions were added by the rule, we push the rule onto the any list of rules
                if (!(rule->addConditions(getConditions())))
                {
                    getMatchAnyRuleList().push_back(rule);
                }
                else
                {
                    // the rule added conditions, we should check if it added any empty conditions blocks.
                    // This will only catch the last orConditions added by a given rule being empty, but few rules add multiple conditions blocks.
                    // Rules with multiple conditions blocks that misbehave will get caught in ProductionManager::buildReteTree() where we don't have the rule name available.
                    if ((!getConditions().at(CONDITION_BLOCK_BASE_SEARCH).empty())
                        && EA_UNLIKELY(getConditions().at(CONDITION_BLOCK_BASE_SEARCH).back().empty()))
                    {
                        BLAZE_ERR_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].initializeReteProductions: id '" << getProductionId() << "' rule '"
                            << rule->getRuleName() << "' has added an empty orConditions list to block " << CONDITION_BLOCK_BASE_SEARCH << ".");
                    }
                }
            }
        }

        printConditions(reteNetwork);

        initTime = TimeValue::getTimeOfDay();
        reteNetwork.getProductionManager().addProduction(*pL, *this);

        // Game Browser no longer receives tokenAdded notifications when
        // calling addProduction.  Instead it saves off a list of production
        // nodes to pull from once it has them in sorted order.
        initialSearchComplete();

        searchTime = TimeValue::getTimeOfDay();

        SPAM_LOG("[ReteRuleProvider].initializeReteProductions: id '" << getProductionId() << "' Durations Total '"
            << searchTime.getMillis() - startTime.getMillis() << "' ms (Init '" << initTime.getMillis() - startTime.getMillis() 
            << "' ms, Search '" << searchTime.getMillis() - initTime.getMillis() << "' ms)");

        //reteNetwork.logBetaNetwork();
    }

    void ReteRuleProvider::printConditions(Rete::ReteNetwork& reteNetwork)
    {
        if (!BLAZE_IS_LOGGING_ENABLED(reteNetwork.getLogCategory(), Logging::TRACE))
        {
            return;
        }

        BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].printConditions: ===================== Searcher(" << getProductionId() << ") ===================================");
        printConditionBlock(reteNetwork, "CONDITION_BLOCK_BASE_SEARCH", getConditions().at(Rete::CONDITION_BLOCK_BASE_SEARCH));
        BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].printConditions: ===================== FINISHED CONDITIONS! ===================================");
    }

    void ReteRuleProvider::printConditionBlock(Rete::ReteNetwork& reteNetwork, const char8_t* name, Rete::ConditionBlock& block) const
    {
        Rete::ConditionBlock::iterator orItr = block.begin();
        Rete::ConditionBlock::iterator orEnd = block.end();
        BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].printConditionBlock: " << name << " [");

        uint32_t i = 1;
        for (; orItr != orEnd; ++orItr)
        {
            Rete::OrConditionList& orList = *orItr;
            Rete::OrConditionList::iterator conditionItr = orList.begin();
            Rete::OrConditionList::iterator conditionEnd = orList.end();

            eastl::string conditionBlockStr("'");
            for (; conditionItr != conditionEnd; ++conditionItr)
            {
                if (conditionItr != orList.begin())
                {
                    conditionBlockStr.append(", '");
                }
                Rete::ConditionInfo& info = *conditionItr;
                conditionBlockStr.append(info.condition.toString());
                conditionBlockStr.append("'(");

                char8_t buf[8];
                blaze_snzprintf(buf, sizeof(buf), "%u", info.fitScore);
                conditionBlockStr.append(buf);
                conditionBlockStr.append(")");
            }
            BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].printConditionBlock: " << name << ", level(" << i << ") " << conditionBlockStr.c_str());
            ++i;
        }

        BLAZE_TRACE_LOG(reteNetwork.getLogCategory(), "[ReteRuleProvider].printConditionBlock: " << name << "]");
    }

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
