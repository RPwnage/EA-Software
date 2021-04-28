/*! ************************************************************************************************/
/*!
\file gamestaterule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gamestaterule.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameStateRule::GameStateRule(const GameStateRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(definition, matchmakingAsyncStatus)
    {
    }

    GameStateRule::GameStateRule(const GameStateRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus)
    {
        other.mGameStateWhitelist.copyInto(mGameStateWhitelist);
    }


    bool GameStateRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

        GameStateList::const_iterator itr = mGameStateWhitelist.begin();
        GameStateList::const_iterator end = mGameStateWhitelist.end();
        for(; itr != end; ++itr)
        {
            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(GameStateRuleDefinition::GAMESTATERULE_RETE_KEY, getDefinition().getGameStateAttributeValue(*itr), mRuleDefinition), 
                0, this));
        }

        // need to put something into the conditions blocks if no states were matched to ensure correct whitelist behavior.
        // an empty condition block is ignored, essentially disabling the rule.
        // inserting a nonexistent game state prevents the rule from matching any games for an empty condition block
        if (baseOrConditions.empty())
        {
            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(GameStateRuleDefinition::GAMESTATERULE_RETE_KEY, getDefinition().getNoMatchAttributeValue(), mRuleDefinition), 
                0, this));
        }

        return true;

    }

    bool GameStateRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!matchmakingSupplementalData.mMatchmakingContext.isSearchingByMatchmaking() 
            && !matchmakingSupplementalData.mGameStateWhitelist.empty()) 
        {
            // a whitelist was provided, copy it over
            matchmakingSupplementalData.mGameStateWhitelist.copyInto(mGameStateWhitelist);
        }
        else
        {
            // if we're matchmaking, or the whitelist was empty & we're not ignoring game state,
            // we use the default game states in the search.
            mGameStateWhitelist.push_back(RESETABLE);
            mGameStateWhitelist.push_back(IN_GAME);
            mGameStateWhitelist.push_back(PRE_GAME);
            mGameStateWhitelist.push_back(INACTIVE_VIRTUAL);
            mGameStateWhitelist.push_back(GAME_GROUP_INITIALIZED);
        }
        return true;
    }


    Rule* GameStateRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GameStateRule(*this, matchmakingAsyncStatus);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
