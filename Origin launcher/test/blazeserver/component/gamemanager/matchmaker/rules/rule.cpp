/*! ************************************************************************************************/
/*!
    \file   rule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    Rule::Rule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : mCriteria(nullptr), 
          mRuleDefinition(ruleDefinition), 
          mMatchmakingAsyncStatus(matchmakingAsyncStatus),
          mRuleEvaluationMode(RULE_EVAL_MODE_NORMAL),
          mMinFitThresholdList(nullptr),
          mCurrentMinFitThreshold(0.0f),
          mNextUpdate(UINT32_MAX),
          mRuleIndex(0),
          mCachedRuleDiagnosticPtr(nullptr)
    { }

    Rule::Rule(const Rule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : mCriteria(otherRule.mCriteria), 
          mRuleDefinition(otherRule.mRuleDefinition),
          mMatchmakingAsyncStatus(matchmakingAsyncStatus),
          mRuleEvaluationMode(otherRule.mRuleEvaluationMode),
          mMinFitThresholdList(otherRule.mMinFitThresholdList),
          mCurrentMinFitThreshold(otherRule.mCurrentMinFitThreshold),
          mNextUpdate(otherRule.mNextUpdate),
          mRuleIndex(0),
          mCachedRuleDiagnosticPtr(nullptr)
    {
    }

    MatchmakingSessionId Rule::getMMSessionId() const
    { 
        const MatchmakingSession* session = getMatchmakingSession();
        if (session) 
            return session->getMMSessionId();
        else
            return (MatchmakingSessionId)INVALID_MATCHMAKING_SESSION_ID; 
    }

    const MatchmakingSession* Rule::getMatchmakingSession() const 
    { 
        return mCriteria ? mCriteria->getMatchmakingSession() : nullptr; 
    }


    UpdateThresholdResult Rule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // If the rule is disabled we don't need to decay.
        if (isDisabled())
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        // if the MinFitThresholdList is null then there is no decay.
        if (mMinFitThresholdList == nullptr)
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        float originalThreshold = mCurrentMinFitThreshold;
        mCurrentMinFitThreshold = mMinFitThresholdList->getMinFitThreshold(elapsedSeconds, &mNextUpdate);
      
        if (forceUpdate || originalThreshold != mCurrentMinFitThreshold)
        {
            TRACE_LOG("[matchmakingrule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                      << "' elapsedSeconds '" << elapsedSeconds << "' originalThreshold '" << originalThreshold << "' currentThreshold '" 
                      << mCurrentMinFitThreshold << "'");

            updateAsyncStatus();
            return RULE_CHANGED; // note: specific rules override this method (ex: generic attribs can invalidate subSessions too)
        }

        return NO_RULE_CHANGE;
    }

    void Rule::fixupMatchTimesForRequirements(const SessionsEvalInfo &sessionsEvalInfo, const MinFitThresholdList *otherMinFitThresholdList,
        SessionsMatchInfo &sessionsMatchInfo, bool isBidirectionalRule) const
    {

        uint32_t mySessionAge = sessionsEvalInfo.mMyEvalInfo.mSession.getSessionAgeSeconds();
        uint32_t myRequirementDecayAge = (mMinFitThresholdList != nullptr) ? mMinFitThresholdList->getRequiresExactMatchDecayAge() : 0;

        uint32_t otherSessionAge = sessionsEvalInfo.mOtherEvalInfo.mSession.getSessionAgeSeconds();
        uint32_t otherRequirementDecayAge = (otherMinFitThresholdList != nullptr) ? otherMinFitThresholdList->getRequiresExactMatchDecayAge() : 0;

        // fixup each match to take the other session's required exact match status into account
        fixupMatchTimeForRequiredExactMatch(mySessionAge, otherSessionAge, otherRequirementDecayAge, 
            sessionsEvalInfo.mOtherEvalInfo.mIsExactMatch, sessionsMatchInfo.sMyMatchInfo.sMatchTimeSeconds);

        fixupMatchTimeForRequiredExactMatch(otherSessionAge, mySessionAge, myRequirementDecayAge, 
            sessionsEvalInfo.mMyEvalInfo.mIsExactMatch, sessionsMatchInfo.sOtherMatchInfo.sMatchTimeSeconds);

        // fixup each match to take the other session's bidirectional requirements into account
        if (isBidirectionalRule)
        {
            int64_t myMatchTime = (int64_t) sessionsMatchInfo.sMyMatchInfo.sMatchTimeSeconds - mySessionAge;
            int64_t otherMatchTime = (int64_t) sessionsMatchInfo.sOtherMatchInfo.sMatchTimeSeconds - otherSessionAge;
            int64_t bidirMatchTime = eastl::max(myMatchTime, otherMatchTime);
            clampMatchAgeToMinimumTime(myMatchTime, bidirMatchTime, mySessionAge, sessionsMatchInfo.sMyMatchInfo.sMatchTimeSeconds);
            clampMatchAgeToMinimumTime(otherMatchTime, bidirMatchTime, otherSessionAge, sessionsMatchInfo.sOtherMatchInfo.sMatchTimeSeconds);
        }
    }

    void Rule::fixupMatchTimeForRequiredExactMatch(uint32_t mySessionAge, uint32_t otherSessionAge, uint32_t otherRequirementDecayAge,
        bool otherIsExactMatch, uint32_t &sessionMatchAge) const
    {
        bool otherRequiresExactMatch = (otherRequirementDecayAge > otherSessionAge);
        bool sessionMatchesOther = (sessionMatchAge < NO_MATCH_TIME_SECONDS);
        if ( otherRequiresExactMatch && !otherIsExactMatch && sessionMatchesOther)
        {
            // defer our match until after the other session has decayed from a requirement to a preference
            int64_t requirementDecayTime = (int64_t)otherRequirementDecayAge - otherSessionAge;
            int64_t matchTime = (int64_t)sessionMatchAge - mySessionAge;
            clampMatchAgeToMinimumTime(matchTime, requirementDecayTime, mySessionAge, sessionMatchAge);
        }
    }


    // given a matchTime & a minimumMatchTime, clamp the matchAge to the minimum (if needed) 
    void Rule::clampMatchAgeToMinimumTime(int64_t matchTime, int64_t minMatchTime, uint32_t sessionAge, uint32_t &matchAge) const
    {
        if (matchTime < minMatchTime)
        {
            // clamp matchTime to min and convert from a time back to a matchAge
            int64_t newMatchAge = sessionAge + minMatchTime;
            if (newMatchAge < UINT32_MAX)
            {
                if(matchAge < newMatchAge)
                {
                    //prevent a rule that matches sooner from causing premature finalization
                    matchAge = (uint32_t) newMatchAge;
                }
            }
            else
            {
                TRACE_LOG("[MatchmakingRule] Clamp Match age setting NO_MATCH for rule " << getRuleName() << " matchTime: " << matchTime << " minMatchTime: " << minMatchTime << " sessionAge: " << sessionAge << " matchAge: " << matchAge << ".");
                matchAge = UINT32_MAX; // indicates no match
            }
        }
    }

    void Rule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        RuleWeight weight = mRuleDefinition.getWeight();
        const MinFitThresholdList *minFitThresholdList = mMinFitThresholdList;

        // MM_TODO: ideally, we'd debug3 log each of the rule evals (success), and log the failure for debug2.
        // pass strings or something in for the values?

        if (minFitThresholdList == nullptr)
        {
            //If the fit threshold list is nullptr, it means we take the fit percentage at face value
            if (isFitPercentMatch(evalInfo.mFitPercent))
            {
                //matches, so set the match immediately
                matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * weight), 0);
            }
            else
            {
                matchInfo.setNoMatch();
            }
            return;
        }

        //Check exact match
        if (evalInfo.mIsExactMatch && minFitThresholdList->isExactMatchRequired())
        {
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * weight), 0);
            return;
        }

        uint32_t thresholdListSize = minFitThresholdList->getSize();
        for (uint32_t i = minFitThresholdList->isExactMatchRequired() ? 1 : 0; i < thresholdListSize; ++i)
        {
            float minFitThreshold = minFitThresholdList->getMinFitThresholdByIndex(i);

            if ( evalInfo.mFitPercent >= minFitThreshold )
            {
                uint32_t matchSeconds = minFitThresholdList->getSecondsByIndex(i);
                FitScore fitScore = static_cast<FitScore>(evalInfo.mFitPercent * weight);
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }

    FitScore Rule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        RuleWeight weight = mRuleDefinition.getWeight();

        if (mRuleEvaluationMode != RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD)
        {
            bool thresholdSet = false;
            float threshold = 0.0f;
            if (mRuleEvaluationMode != RULE_EVAL_MODE_LAST_THRESHOLD)
            {
                threshold = mCurrentMinFitThreshold;
                thresholdSet = true;
            }
            else if (mMinFitThresholdList != nullptr)
            {
                threshold = mMinFitThresholdList->getLastMinFitThreshold();
                thresholdSet = true;
            }

            if (MinFitThresholdList::isThresholdExactMatchRequired(threshold))
            {
                if (isExactMatch)
                {
                    return static_cast<FitScore>(fitPercent * weight);
                }
                else
                {
                    // log exact match fail msg
                    return FIT_SCORE_NO_MATCH;
                }
            }

            if (thresholdSet && fitPercent < threshold)
            {
                // log threshold fail message
                return FIT_SCORE_NO_MATCH;
            }
        }

        // success
        return static_cast<FitScore>(fitPercent * weight);

    }
 
     bool Rule::initMinFitThreshold( const char8_t* minFitListName, const RuleDefinition& ruleDef,
         MatchmakingCriteriaError& err )
     {
         if (minFitListName[0] != '\0' && !ruleDef.isDisabled())
         {
             // try looking up the minFitThresholdList
             mMinFitThresholdList = ruleDef.getMinFitThresholdList( minFitListName );
             if (mMinFitThresholdList == nullptr)
             {
                 // error: minFitThreshold name unknown
                 char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                 blaze_snzprintf(msg, sizeof(msg), "Couldn't find a MinFitThreshold list named \"%s\" in rule definition \"%s\".", minFitListName, getRuleName());
                 err.setErrMessage(msg);
                 return false;
             }
         }
         else
         {
             // the minFitThresholdList name is empty, indicating that this rule should be disabled
             // disable the rule (not an error)
             mMinFitThresholdList = nullptr;
         }
 
         return true;
     }

    bool Rule::handleDisabledRuleEvaluation(const Rule &otherRule, SessionsMatchInfo &sessionsMatchInfo, FitScore matchingWeightedFitScore) const
    {
        // if both rules are disabled, they match
        if (isDisabled() && otherRule.isDisabled())
        {
            sessionsMatchInfo.sMyMatchInfo.setMatch(0, 0);
            sessionsMatchInfo.sOtherMatchInfo.setMatch(0, 0);
            return true;
        }
        else
        {
            // MM_AUDIT: Some rules should still evaluate even if the other sessions
            // rule is disabled (ex. DNF, Skill, anything on the UED).
            // 1 or neither rule is disabled
            if (isDisabled())
            {
                sessionsMatchInfo.sMyMatchInfo.setMatch(0, 0);
                sessionsMatchInfo.sOtherMatchInfo.setMatch(matchingWeightedFitScore, 0);
                return !isEvaluateDisabled(); // For Rules that evaluate disabled rules, keep evaluating. IE skill rules still evaluate if one is enabled.
            }
            if (otherRule.isDisabled())
            {
                sessionsMatchInfo.sMyMatchInfo.setMatch(matchingWeightedFitScore, 0);
                sessionsMatchInfo.sOtherMatchInfo.setMatch(0, 0);
                return !isEvaluateDisabled(); // For Rules that evaluate disabled rules, keep evaluating. IE skill rules still evaluate if one is enabled.
            }
        }

        return false; // eval not handled 
    }

  
    /*! ************************************************************************************************/
    /*! \brief returns setup infos, names/tags, for the diagnostics to add for the rule.
        Used by both create and find mode
    ***************************************************************************************************/
    const RuleDiagnosticSetupInfoList& Rule::getDiagnosticsSetupInfos() const
    {
        // if already cached, return it
        if (!mCachedDiagnosticSetupInfos.empty())
        {
            return mCachedDiagnosticSetupInfos;
        }

        // get rule's per value diagnostics, if it has any
        getRuleValueDiagnosticSetupInfos(mCachedDiagnosticSetupInfos);

        bool isRuleTotalEnabled = isRuleTotalDiagnosticEnabled();

        // if disabled, return an empty list
        if (!isRuleTotalEnabled && mCachedDiagnosticSetupInfos.empty())
        {
            return mCachedDiagnosticSetupInfos;
        }

        // the common totals/summary goes in front so it may appear first in responses
        if (isRuleTotalEnabled)
        {
            mCachedDiagnosticSetupInfos.push_front(RuleDiagnosticSetupInfo(RuleDiagnosticSetupInfo::CATEGORY_RULE_SUMMARY, "total"));
        }

        return mCachedDiagnosticSetupInfos;
    }


    /*! ************************************************************************************************/
    /*! \brief tally the MM session's rule's diagnostics, for create game
        \param[in] ruleEvalResult Specified if tallying after a CG evaluation, to tally its diagnostics
    ***************************************************************************************************/
    void Rule::tallyRuleDiagnosticsCreateGame(MatchmakingSubsessionDiagnostics& sessionDiagnostics, const MatchInfo *ruleEvalResult) const
    {
        if (isDisabled())
        {
            return;
        }

        auto& ruleDiagnosticSetupInfos = getDiagnosticsSetupInfos();
        if (ruleDiagnosticSetupInfos.empty())
        {
            // rule has no diagnostics
            return;
        }

        DiagnosticsByRuleCategoryMapPtr ruleDiagnostics = getOrAddRuleDiagnostic(sessionDiagnostics.getRuleDiagnostics());
        if (ruleDiagnostics == nullptr)
        {
            return;//logged
        }

        // tally each of the rule's diagnostics
        for (auto& diagnosticSetupInfo : ruleDiagnosticSetupInfos)
        {
            MatchmakingRuleDiagnosticCountsPtr tally = getOrAddRuleSubDiagnostic(*ruleDiagnostics, diagnosticSetupInfo);
            if (tally == nullptr)
            {
                continue;//logged
            }

            //set if not already:
            tally->setSessions(1);

            // CG simply increments the count for all the rule's sub diagnostics, by default, below
            if (ruleEvalResult != nullptr)
            {
                tally->setCreateEvaluations(tally->getCreateEvaluations() + 1);

                if (ruleEvalResult->sFitScore != FIT_SCORE_NO_MATCH)
                {
                    tally->setCreateEvaluationsMatched(tally->getCreateEvaluationsMatched() + 1);
                }
            }

            SPAM_LOG("[Rule].tallyRuleDiagnosticsCreateGame: rule(" << (getRuleName() != nullptr ? getRuleName() : "<nullptr>") << ") diagnostics updated. Evaluations were(" <<
                ((ruleEvalResult != nullptr) ? eastl::string().sprintf("createEvals incremented to:%" PRIu64 ", matched to:%" PRIu64 "").c_str() : "<NOT EVALUATED (YET)>") << ")");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief tally the MM session's rule's diagnostics, for find game
        \param[in] conditionsBlockList The session's RETE conditions. Used by some rules to count games
        \param[in] helper Used by some rules to count games
    ***************************************************************************************************/
    void Rule::tallyRuleDiagnosticsFindGame(MatchmakingSubsessionDiagnostics& sessionDiagnostics,
        const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (isDisabled())
        {
            return;
        }
        auto& ruleDiagnosticSetupInfos = getDiagnosticsSetupInfos();
        if (ruleDiagnosticSetupInfos.empty())
        {
            return;
        }

        DiagnosticsByRuleCategoryMapPtr ruleDiagnostics = getOrAddRuleDiagnostic(sessionDiagnostics.getRuleDiagnostics());
        if (ruleDiagnostics == nullptr)
        {
            return;//logged
        }

        // tally each of the rule's diagnostics
        for (auto& diagnosticSetupInfo : ruleDiagnosticSetupInfos)
        {
            MatchmakingRuleDiagnosticCountsPtr tally = getOrAddRuleSubDiagnostic(*ruleDiagnostics, diagnosticSetupInfo);
            if (tally == nullptr)
            {
                continue;//logged
            }

            //set if not already:
            tally->setSessions(1);
            tally->setFindRequests(1);

            // FG games visible count can be calculated differently, depending on the rule, via this virtual fn:
            auto gamesVisible = getDiagnosticGamesVisible(diagnosticSetupInfo, sessionConditions, helper);

            tally->setFindRequestsGamesVisible(gamesVisible);

            if (tally->getFindRequestsGamesVisible() > 0)
            {
                tally->setFindRequestsHadGames(1);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return MM session's count of visible games for the specified diagnostic
        \param[in] diagnosticInfo Info on the diagnostic to get count for
        \param[in] conditionsBlockList The session's RETE conditions. Used by some rules to count games
        \param[in] helper Used by some rules to count games
    ***************************************************************************************************/
    uint64_t Rule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        // Default implementation: tallies visible games from RETE. Match any assumes all MM games visible.
        return ((isMatchAny() || !isReteCapable()) ?
            helper.getOpenToMatchmakingCount()
            : helper.getNumGamesVisibleForRuleConditions(getRuleName(), sessionConditions));
    }


    /*! ************************************************************************************************/
    /*! \brief from load tests: below caches pointers, to avoid map lookups during CG evaluation, 
            this improves performance significantly
    ***************************************************************************************************/
    DiagnosticsByRuleCategoryMapPtr Rule::getOrAddRuleDiagnostic(DiagnosticsByRuleMap& diagnosticsMap) const
    {
        if (mCachedRuleDiagnosticPtr == nullptr)
        {
            mCachedRuleDiagnosticPtr = DiagnosticsTallyUtils::getOrAddRuleDiagnostic(diagnosticsMap, getRuleName());
        }
        return mCachedRuleDiagnosticPtr;
    }

    MatchmakingRuleDiagnosticCountsPtr Rule::getOrAddRuleSubDiagnostic(DiagnosticsByRuleCategoryMap& ruleDiagnostic,
        const RuleDiagnosticSetupInfo& subDiagnosticSetupInfo) const
    {
        if (subDiagnosticSetupInfo.mCachedSubDiagnosticPtr == nullptr)
        {
            subDiagnosticSetupInfo.mCachedSubDiagnosticPtr = DiagnosticsTallyUtils::getOrAddRuleSubDiagnostic(
                ruleDiagnostic, subDiagnosticSetupInfo.mCategoryTag.c_str(), subDiagnosticSetupInfo.mValueTag.c_str());
        }
        return subDiagnosticSetupInfo.mCachedSubDiagnosticPtr;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

