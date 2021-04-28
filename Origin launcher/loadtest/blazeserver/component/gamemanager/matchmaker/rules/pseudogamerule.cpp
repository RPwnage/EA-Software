/*! ************************************************************************************************/
/*!
    \file   pseudogamerule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/pseudogamerule.h"
#include "gamemanager/matchmaker/rules/pseudogameruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    PseudoGameRule::PseudoGameRule(const PseudoGameRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mIsPseudoRequest(false)
    {
    }

    PseudoGameRule::PseudoGameRule(const PseudoGameRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRule(other, status),
        mIsPseudoRequest(other.mIsPseudoRequest)
    {
    }

    bool PseudoGameRule::isDisabled() const
    {
        return false;
    }

    bool PseudoGameRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        const PseudoGameRuleDefinition &myDefn = getDefinition();

        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& baseOrTopologyConditions = baseConditions.push_back();
        baseOrTopologyConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(PseudoGameRuleDefinition::PSEUDO_GAME_RETE_KEY, 
                            myDefn.getWMEBooleanAttributeValue(false), mRuleDefinition),
            0, this
            ));
        if (mIsPseudoRequest)
        {
            baseOrTopologyConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(PseudoGameRuleDefinition::PSEUDO_GAME_RETE_KEY, 
                                myDefn.getWMEBooleanAttributeValue(true), mRuleDefinition),
                0, this
                ));
        }

        return true;
    }

    bool PseudoGameRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        mIsPseudoRequest = matchmakingSupplementalData.mIsPseudoRequest;
        return true;
    }

    void PseudoGameRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        if (!gameSession.isPseudoGame())
        {
            debugRuleConditions.writeRuleCondition("non-pseudo game found");
            fitPercent = 0;
            isExactMatch = true;
        }
        else if (mIsPseudoRequest)
        {
            debugRuleConditions.writeRuleCondition("pseudo game match found for pseudo request");
            fitPercent = 0;
            isExactMatch = true;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("skipping pseudo game found by non-pseudo request");

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
    }

    void PseudoGameRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const PseudoGameRule& otherTopRule = (const PseudoGameRule&)otherRule;
        if (otherTopRule.mIsPseudoRequest == mIsPseudoRequest)
        {
            debugRuleConditions.writeRuleCondition("pseudo game settings match");
            fitPercent = 0;
            isExactMatch = true;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("pseudo game settings mismatch");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
    }

    // Override this for games
    FitScore PseudoGameRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        return FitScore(fitPercent);
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
