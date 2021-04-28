/*! ************************************************************************************************/
/*!
\file   simplerule.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    SimpleRule::SimpleRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(ruleDefinition, matchmakingAsyncStatus)
    {
    }

    SimpleRule::SimpleRule(const SimpleRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(otherRule, matchmakingAsyncStatus)
    {
    }

    FitScore SimpleRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        return evaluateGame(gameSession, debugRuleConditions);
    }

    FitScore SimpleRule::evaluateGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        if ((isDisabled()) || (!isDedicatedServerEnabled() && gameSession.isResetable()))
            return getMaxFitScore();

        float fitPercent;
        bool isExactMatch;
        calcFitPercent(gameSession, fitPercent, isExactMatch, debugRuleConditions);

        // TODO: Figure out how to remove this.
        if (EA_UNLIKELY(mRuleEvaluationMode == RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD ))
        {
            // for session created game, we don't need to compare the
            // fitscore with mCurrentMinFitThreshold
            return static_cast<FitScore>(fitPercent * mRuleDefinition.getWeight());
        }

        FitScore fitScore = calcWeightedFitScore(isExactMatch, fitPercent);

        return fitScore;
    }


    FitScore SimpleRule::evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const
    {
        return evaluateGame(gameSession, debugRuleConditions);
    }

    void SimpleRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const SimpleRule* otherRule = static_cast<const SimpleRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));

        // prevent crash if for some reason other rule is not defined
        if (otherRule == nullptr)
        {
            ERR_LOG("[SimpleRule].evaluateForMMCreateGame Rule '" << getRuleName() << "' MM session '" 
                << getMMSessionId() << "': Unable to find other rule by id '" << getId() << "' for MM session '" 
                << otherSession.getMMSessionId() << "'");
            return;
        }

        if ( handleDisabledRuleEvaluation(*otherRule, sessionsMatchInfo, getMaxFitScore()) )
        {
            return;
        }

        if (!isDisabled())
            evaluateRule(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);

        if (!otherRule->isDisabled())
            otherRule->evaluateRule(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);

        fixupMatchTimesForRequirements(sessionsEvalInfo, otherRule->mMinFitThresholdList, sessionsMatchInfo, isBidirectional());

        // post eval provides ability to perform uni-directional evaluations
        // AUDIT: this can set NO_MATCH, which can be confusing from the logs, as they technically match,
        // but only one of the sessions can finalize for it to work.
        postEval(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
        otherRule->postEval(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
    }

    void SimpleRule::evaluateRule(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        evalInfo.mFitPercent = 0.0f;
        evalInfo.mIsExactMatch = true;

        if (!preEval(otherRule, evalInfo, matchInfo))
        {
            return;
        }

        //calc the matchInfo for [this vs other]
        calcFitPercent(otherRule, evalInfo.mFitPercent, evalInfo.mIsExactMatch, matchInfo.sDebugRuleConditions);
        calcSessionMatchInfo(otherRule, evalInfo, matchInfo); // note: minFitThresholdList is nullptr for DNF
    }

    FitScore SimpleRule::evaluateCreateGameRequest(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const
    {
        if (isDisabled())
            return getMaxFitScore();

        float fitPercent;
        bool isExactMatch;
        
        calcFitPercent(matchmakingCreateGameRequest, fitPercent, isExactMatch, ruleConditions);

        // modified from calcSessionMatchInfo
        // goal is to loosen the check if they will accept it eventually.
        RuleWeight weight = mRuleDefinition.getWeight();
        const MinFitThresholdList *minFitThresholdList = mMinFitThresholdList;

        if (minFitThresholdList == nullptr)
        {
            //If the fit threshold list is nullptr, it means we take the fit percentage at face value
            if (isFitPercentMatch(fitPercent))
            {
                //matches, so set the match immediately
                return static_cast<FitScore>(fitPercent * weight);
            }
            else
            {
                return FIT_SCORE_NO_MATCH;
            }
        }

        //Check exact match
        if (isExactMatch && minFitThresholdList->isExactMatchRequired())
        {
            return static_cast<FitScore>(fitPercent * weight);
        }

        uint32_t thresholdListSize = minFitThresholdList->getSize();
        for (uint32_t i = minFitThresholdList->isExactMatchRequired() ? 1 : 0; i < thresholdListSize; ++i)
        {
            float minFitThreshold = minFitThresholdList->getMinFitThresholdByIndex(i);

            if ( fitPercent >= minFitThreshold )
            {
                uint32_t matchSeconds = minFitThresholdList->getSecondsByIndex(i);
                TRACE_LOG("[SimpleRule].evaluateCreateGameRequest accepting create game request as it will match at time " << matchSeconds << ".");
                FitScore fitScore = static_cast<FitScore>(fitPercent * weight);
                return fitScore;
            }
        }

        return FIT_SCORE_NO_MATCH;
    }

    // Failing to add debug information just evaluates the fit percent.
    void SimpleRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        calcFitPercent(gameSession, fitPercent, isExactMatch);
    }

    // Dummy implementation for rules, everything matches, but does not add a fit score.
    void SimpleRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch) const
    {
        fitPercent = 0.0f;
        isExactMatch = true;
    }

    // Dummy implementation for rules, everything matches, but does not add a fit score.
    void SimpleRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const
    {
        fitPercent = 0.0f;
        isExactMatch = true;
    }

    void SimpleRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        calcFitPercent(otherRule, fitPercent, isExactMatch);
    }



    SimpleRangeRule::SimpleRangeRule(const RuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mRangeOffsetList(nullptr),
        mCurrentRangeOffset(nullptr)
    {}

    SimpleRangeRule::SimpleRangeRule(const SimpleRangeRule &otherRule, MatchmakingAsyncStatus* status)
    : SimpleRule(otherRule, status),
        mRangeOffsetList(otherRule.mRangeOffsetList),
        mCurrentRangeOffset(otherRule.mCurrentRangeOffset),
        mAcceptableRange(otherRule.mAcceptableRange)
    {}

    FitScore SimpleRangeRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        RuleWeight weight = mRuleDefinition.getWeight();

        // TODO: make the offset check generic here, for right now,
        // this check is done in the calcFitPercent for Games, but 
        // should be moved here so that ranges can be moved to the Rule Class.
        if (fitPercent == FIT_PERCENT_NO_MATCH)
            return FIT_SCORE_NO_MATCH;

        if (mRuleEvaluationMode != RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD)
        {
            const RangeList::Range* range = mCurrentRangeOffset;

            if (range != nullptr)
            {
                if (range->isExactMatchRequired())
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
            }
        }

        // success
        return static_cast<FitScore>(fitPercent * weight);
    }
    
    UpdateThresholdResult SimpleRangeRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // If the rule is disabled we don't need to decay.
        if (mRangeOffsetList == nullptr)
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        const RangeList::Range *originalRangeOffset = mCurrentRangeOffset;
        mCurrentRangeOffset = mRangeOffsetList->getRange(elapsedSeconds, &mNextUpdate);

        if ( (originalRangeOffset != mCurrentRangeOffset) || forceUpdate )
        {
            if (mCurrentRangeOffset != nullptr)
            {
                if (originalRangeOffset != nullptr)
                {
                    TRACE_LOG("[matchmakingrule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                        << "' elapsedSeconds '" << elapsedSeconds << "' previousRange: " << originalRangeOffset->mMinValue << ".." 
                        << originalRangeOffset->mMaxValue << " currentRange: " << mCurrentRangeOffset->mMinValue << ".." << mCurrentRangeOffset->mMaxValue);
                }
                else
                {
                    TRACE_LOG("[matchmakingrule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                        << "' elapsedSeconds '" << elapsedSeconds << "' currentRange: " << mCurrentRangeOffset->mMinValue << ".." 
                        << mCurrentRangeOffset->mMaxValue);
                }

                // range's bounds are the total possible percent range
                updateAcceptableRange();
            }

            updateAsyncStatus();

            return RULE_CHANGED;
        } 

        return NO_RULE_CHANGE;
    }

    FitScore SimpleRangeRule::evaluateGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        if (mRuleEvaluationMode == RULE_EVAL_MODE_LAST_THRESHOLD)
        {
            // Switch the acceptable range temporarily: 
            const RangeList::Range* prevRange = mCurrentRangeOffset;
            const RangeList::Range* range = (mRangeOffsetList ? mRangeOffsetList->getLastRange() : nullptr);

            SimpleRangeRule* tempThis = const_cast<SimpleRangeRule*>(this);
            tempThis->mCurrentRangeOffset = range;
            tempThis->updateAcceptableRange();

            FitScore fitScore = SimpleRule::evaluateGame(gameSession, debugRuleConditions);

            tempThis->mCurrentRangeOffset = prevRange;
            tempThis->updateAcceptableRange();

            return fitScore;
        }

        return SimpleRule::evaluateGame(gameSession, debugRuleConditions);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
