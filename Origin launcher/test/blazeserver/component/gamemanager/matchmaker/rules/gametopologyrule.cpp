/*! ************************************************************************************************/
/*!
    \file   gametopologyrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gametopologyrule.h"
#include "gamemanager/matchmaker/rules/gametopologyruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    GameTopologyRule::GameTopologyRule(const GameTopologyRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mDesiredTopology(NETWORK_DISABLED)
    {
    }

    GameTopologyRule::GameTopologyRule(const GameTopologyRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRule(other, status),
        mDesiredTopology(other.mDesiredTopology)
    {
    }

    bool GameTopologyRule::isDisabled() const
    {
        return (mDesiredTopology == NETWORK_DISABLED);
    }

    bool GameTopologyRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        const GameTopologyRuleDefinition &myDefn = getDefinition();

        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& baseOrTopologyConditions = baseConditions.push_back();
        baseOrTopologyConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(GameTopologyRuleDefinition::GAME_NETWORK_TOPOLOGY_RETE_KEY, 
                            myDefn.getWMEInt32AttributeValue((int32_t)mDesiredTopology), mRuleDefinition),
            0, this
            ));

        return true;
    }

    bool GameTopologyRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Only enable this rule for dedicated server searches (until we figure out how to handle the default topology on FindGame)
            return true;
        }

        mDesiredTopology = matchmakingSupplementalData.mNetworkTopology;
        return true;
    }

    void GameTopologyRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        if (gameSession.getGameNetworkTopology() == mDesiredTopology)
        {
            debugRuleConditions.writeRuleCondition("topology match (%s)", GameNetworkTopologyToString(mDesiredTopology));
            fitPercent = 0;
            isExactMatch = true;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("topology mismatch (%s desired, %s found)", GameNetworkTopologyToString(mDesiredTopology), GameNetworkTopologyToString(gameSession.getGameNetworkTopology()));

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
    }

    void GameTopologyRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const GameTopologyRule& otherTopRule = (const GameTopologyRule&)otherRule;
        if (otherTopRule.mDesiredTopology == mDesiredTopology)
        {
            debugRuleConditions.writeRuleCondition("topology match (%s)", GameNetworkTopologyToString(mDesiredTopology));
            fitPercent = 0;
            isExactMatch = true;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("topology mismatch (%s desired, %s found)", GameNetworkTopologyToString(mDesiredTopology), GameNetworkTopologyToString(otherTopRule.mDesiredTopology));
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
    }

    // Override this for games
    FitScore GameTopologyRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        return FitScore(fitPercent);
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
