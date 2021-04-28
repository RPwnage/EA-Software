/*! ************************************************************************************************/
/*!
    \file    gamenamerule.cpp


    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gamenamerule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in evaluateGameAgainstAny()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    GameNameRule::GameNameRule(const GameNameRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(ruleDefinition, matchmakingAsyncStatus),
        mSearchString("")
    {}

    GameNameRule::GameNameRule(const GameNameRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(otherRule, matchmakingAsyncStatus),
        mSearchString(otherRule.mSearchString)
    {}


    bool GameNameRule::initialize(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames() ||
            !matchmakingSupplementalData.mMatchmakingContext.isSearchingGames())
        {
            return true;
        }

        // NOTE: empty string is expected when the client disables the rule
        if(criteriaData.getGameNameRuleCriteria().getSearchString()[0] == '\0')
        {
            TRACE_LOG("[GameNameRule] disabled, search substring was set to default of empty string"); 
            return true;
        }
        
        // we normalize here up front once to check length (side: its done again when accessing the substring trie. The dupe op could be optimized away if needed)
        char8_t normalizedSearch[Blaze::GameManager::MAX_GAMENAME_LEN];
        if (!getDefinition().normalizeStringAndValidateLength(criteriaData.getGameNameRuleCriteria().getSearchString(), normalizedSearch))
        {
            // notify game name search criteria fails length requirements
            char8_t errBuffer[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(errBuffer, sizeof(errBuffer), "search string('%s',normalized:'%s') did not pass rule length requirement checks. Required min length:%u",
                criteriaData.getGameNameRuleCriteria().getSearchString(), normalizedSearch, getDefinition().getSearchStringMinLength());
            err.setErrMessage(errBuffer);
            return false;
        }
        mSearchString.set(normalizedSearch);
        TRACE_LOG("[GameNameRule].initialize: SearchString(" << mSearchString.c_str() << ",normalized:" << normalizedSearch << ")");
        return true;
    }

    FitScore GameNameRule::evaluate(const Search::GameSessionSearchSlave& game, ReadableRuleConditions& debugRuleConditions) const
    {
        // side note: currently this method effectively ignored, given the rule disabled for non-rete/non-GameBrowser.
        if (blaze_strstr(game.getGameName(), mSearchString.c_str()) != nullptr)
        {
            return getMaxFitScore();
        }

        return FIT_SCORE_NO_MATCH;
    }


    bool GameNameRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if(!isDisabled())
        {
            const Rete::WMEAttribute wmeValue = getDefinition().reteHash(mSearchString.c_str());

            Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
            Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

            const FitScore fitScore = 0; // 0 fit score (match/no-match), only the actual match affects the fit score.
            const bool doSubstringSearch = true;

            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(GameNameRuleDefinition::GAMENAMERULE_RETE_KEY, wmeValue, mRuleDefinition, false, doSubstringSearch), 
                fitScore, this));

            return true;
        }

        return false;
    }

    FitScore GameNameRule::evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const
    {
        // Games are not ranked by fit score for this rule
        return getMaxFitScore();
    }

    FitScore GameNameRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        return evaluate(gameSession, debugRuleConditions);
    }

    void GameNameRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        // This rule is not evaluated on create game, no-op.
        sessionsMatchInfo.sMyMatchInfo.setMatch(0, 0);
        sessionsMatchInfo.sOtherMatchInfo.setMatch(0, 0);
    }

    Rule* GameNameRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GameNameRule(*this, matchmakingAsyncStatus);
    }

    void GameNameRule::updateAsyncStatus()
    {
        // NO_OP. rule is a filter w/o decay
    }

    bool GameNameRule::isDisabled() const
    {
        if(mSearchString.length() == 0)
        {
            return true;
        }
        return false;
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
