/*! ************************************************************************************************/
/*!
    \file    avoidgamesrule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/qosavoidgamesrule.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" 

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    QosAvoidGamesRule::QosAvoidGamesRule(const QosAvoidGamesRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus)
    {}

    QosAvoidGamesRule::QosAvoidGamesRule(const QosAvoidGamesRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus)
    {
        mGameIdSet = otherRule.mGameIdSet;
    }


    bool QosAvoidGamesRule::initialize(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
         // This rule only applies to Find Game
         if (!matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo() ||
             !matchmakingSupplementalData.mMatchmakingContext.canSearchJoinableGames())
         {
             return true;
         }
         // check if disabled
         if ((matchmakingSupplementalData.mQosGameIdAvoidList == nullptr) || matchmakingSupplementalData.mQosGameIdAvoidList->empty())
         {
             TRACE_LOG("[QosAvoidGamesRule].initialize: rule disabled, game id list empty.");
             return true;
         }
 
         gameIdListToSet(*(matchmakingSupplementalData.mQosGameIdAvoidList), mGameIdSet);
         TRACE_LOG("[QosAvoidGamesRule].initialize: avoiding " << mGameIdSet.size() << " total game ids.");
         return true;
    }
    
    void QosAvoidGamesRule::gameIdListToSet(const Blaze::GameManager::GameIdList& inputList, GameIdSet& result) const
    {
        uint32_t maxListSize = getDefinition().getMaxGameIdListSize();
        uint32_t gamesAdded = 0;

        GameIdList::const_iterator iter = inputList.begin();
        GameIdList::const_iterator itEnd = inputList.end();
        for (; iter != itEnd; ++iter)
        {
            if (gamesAdded >= maxListSize)
            {
                WARN_LOG("[QosAvoidGamesRule] Input game id list of size '" << inputList.size() << "' exceeds the configured max of '" << maxListSize << "'. Additional games will be ignored.");
                return;
            }

            result.insert(*iter);
            TRACE_LOG("[QosAvoidGamesRule].gameIdListToSet added game id(" << (*iter) << ") to game ids to avoid.");
            ++gamesAdded;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no match if game had a player in avoid set, exact match if in
            preferred set.  If none preferred, consider exact match unless player was to be avoided.
    ***************************************************************************************************/
    void QosAvoidGamesRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        if (mGameIdSet.find(gameSession.getGameId()) != mGameIdSet.end())
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            TRACE_LOG("[QosAvoidGamesRule].calcFitPercent game (id = " << gameSession.getGameId() << ") is in avoid list. No match.");

            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " is avoided due to QoS", gameSession.getGameId());
        }
        else 
        {
            fitPercent = 1.0f;
            isExactMatch = true;
            TRACE_LOG("[QosAvoidGamesRule].calcFitPercent game (id = " << gameSession.getGameId() << ") not in avoid list. Max match.");

            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " is not avoided due to QoS", gameSession.getGameId());
        }
        
    }


    void QosAvoidGamesRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // This rule is not evaluated on create game, no-op.
    }


    Rule* QosAvoidGamesRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW QosAvoidGamesRule(*this, matchmakingAsyncStatus);
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t QosAvoidGamesRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            // this is a 'good enough' snapshot calc, though won't sync games removed from MM while session is running
            uint64_t availableGames = helper.getTotalGamesCount();
            return (EA_LIKELY(availableGames >= mGameIdSet.size()) ?
                availableGames - mGameIdSet.size() :
                availableGames);
        }
        ERR_LOG("[QosAvoidGamesRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

