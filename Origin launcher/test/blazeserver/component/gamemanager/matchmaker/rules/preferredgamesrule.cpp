/*! ************************************************************************************************/
/*!
    \file    PreferredGamesRule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/preferredgamesrule.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in calcFitPercent()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    PreferredGamesRule::PreferredGamesRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mRequirePreferredGame(false)
    {
    }
    PreferredGamesRule::PreferredGamesRule(const PreferredGamesRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mRequirePreferredGame(otherRule.mRequirePreferredGame)
    {
    }
    PreferredGamesRule::~PreferredGamesRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    ***************************************************************************************************/
    bool PreferredGamesRule::initialize(const MatchmakingCriteriaData &criteria,
        const MatchmakingSupplementalData &mmSupplementalData, MatchmakingCriteriaError &err)
    {
        // This rule does not apply to Create Game
        if (!mmSupplementalData.mMatchmakingContext.canSearchJoinableGames())
        {
            return true;
        }

        // validate if requires a preferred game, the set of them cannot be empty
        const PreferredGamesRuleCriteria& ruleCriteria = criteria.getPreferredGamesRuleCriteria();
        mRequirePreferredGame = ruleCriteria.getRequirePreferredGame();
        if (mRequirePreferredGame && ruleCriteria.getPreferredList().empty())
        {
            err.setErrMessage("mRequirePreferredGame = true, but no preferred games were specified for matchmaking");
            return false;
        }

        // check if disabled
        if (ruleCriteria.getPreferredList().empty())
        {
            TRACE_LOG("[PreferredGamesRule] disabled, preferred gamess empty/disabled.");
            return true;
        }

        // Setup mGameIdSet
        GameIdList::const_iterator iter = ruleCriteria.getPreferredList().begin();
        for (; iter != ruleCriteria.getPreferredList().end(); ++iter)
        {
            if (mGameIdSet.size() < getDefinition().getMaxGamesUsed())
            {
                GameIdSet::const_iterator findIter = mGameIdSet.find(*iter);
                if (findIter == mGameIdSet.end())
                    mGameIdSet.insert(*iter);
            }
        }

        TRACE_LOG("[PreferredGamesRule].initialize preferred list has '" << mGameIdSet.size() << "' games, mRequirePreferredGame = " << mRequirePreferredGame);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: 100% match if it had a user in my preferred users or I have no
        users preferred, otherwise 0% match. If required preferred player but had none, no-match.
    ***************************************************************************************************/
    void PreferredGamesRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        GameIdSet::const_iterator findIter = mGameIdSet.find(gameSession.getGameId());
        if (findIter != mGameIdSet.end())
        {
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " is preferred", gameSession.getGameId());
            TRACE_LOG("[PreferredGamesRule].calcFitPercent found a preferred game in game id(" << gameSession.getGameId() << "). Max fit.");
            fitPercent = 1.0f;
            isExactMatch = true;
        }
        else
        {
            if (mRequirePreferredGame)
            {
                debugRuleConditions.writeRuleCondition("Game %" PRIu64 " is not preferred, require preferred games", gameSession.getGameId());
                TRACE_LOG("[PreferredGamesRule].calcFitPercent mRequirePreferredGame was true but game id(" << gameSession.getGameId() << ") not preferred. No match.");
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
            else
            {
                debugRuleConditions.writeRuleCondition("Game %" PRIu64 " is not preferred, preferred games not required", gameSession.getGameId());
                TRACE_LOG("[PreferredGamesRule].calcFitPercent Game id(" << gameSession.getGameId() << ") not preferred). Not adding fit.");
                fitPercent = ZERO_FIT;
                isExactMatch = mGameIdSet.empty(); // exact match only if no games preferred
            }
        }
        
    }


    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: 100% match if it had a user in my preferred users or I have no
        users preferred, otherwise 0% match. If required preferred player but had none, no-match.
    ***************************************************************************************************/
    void PreferredGamesRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // disabled
        TRACE_LOG("[PreferredGamesRule].calcFitPercent not evaluated for create game.");
        fitPercent = ZERO_FIT;
        isExactMatch = true;  
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t PreferredGamesRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            return (mRequirePreferredGame ? mGameIdSet.size() : helper.getTotalGamesCount());
        }
        ERR_LOG("[PreferredGamesRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

