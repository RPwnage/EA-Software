/*! ************************************************************************************************/
/*!
\file uedrule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/rules/uedrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    UEDRule::UEDRule(const UEDRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(ruleDefinition, matchmakingAsyncStatus),
          mUEDSearchValue(INVALID_USER_EXTENDED_DATA_VALUE),
          mUEDValue(INVALID_USER_EXTENDED_DATA_VALUE),
          mAcceptEmptyGames(false)
    {
    }

    UEDRule::UEDRule(const UEDRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRangeRule(otherRule, matchmakingAsyncStatus),
        mUEDSearchValue(otherRule.mUEDSearchValue),
        mUEDValue(otherRule.mUEDValue),
        mAcceptEmptyGames(otherRule.mAcceptEmptyGames)
    {
        // We check for empty rule names on UED rules here because DNF & Skill are UED rules, but
        // they do not add UED status, they have their own.
        if ((!otherRule.isDisabled()) && (otherRule.mUEDRuleStatus.getRuleName()[0] != '\0'))
        {
            mMatchmakingAsyncStatus->getUEDRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mUEDRuleStatus));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    UEDRule::~UEDRule()
    {
        mMatchmakingAsyncStatus->getUEDRuleStatusMap().erase(getRuleName());
    }

    bool UEDRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
         if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
         {
             // Rule is disabled when searching for servers.
             return true;
         }

         if (matchmakingSupplementalData.mMatchmakingContext.isSearchingByMatchmaking())
         {
             // if we're coming from MM, we should accept games with no players as a match.
             mAcceptEmptyGames = true;
         }

        const UEDRuleCriteriaMap& uedCriteriaMap = criteriaData.getUEDRuleCriteriaMap();

        UEDRuleCriteriaMap::const_iterator itr = uedCriteriaMap.find(getRuleName());

        // initialize the UED value regardless because other rules that aren't
        // omit or disabled will need to search against this value.
        UserExtendedDataValue calculatedValue = INVALID_USER_EXTENDED_DATA_VALUE;
        if (!getDefinition().calcUEDValue(calculatedValue, matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getId(), matchmakingSupplementalData.mMembersUserSessionInfo->size(), *matchmakingSupplementalData.mMembersUserSessionInfo))
        {
            // Missing key name indicates that there's an issue with the config:
            if (getDefinition().getUEDKey() == INVALID_USER_EXTENDED_DATA_KEY)
            {
                ERR_LOG("[UEDRule].initialize: UedRule Name '" << ((getRuleName() != nullptr) ? getRuleName() : "<unavailable>")
                    << "' uses an unknown UED key '" << ((getDefinition().getUEDName() != nullptr) ? getDefinition().getUEDName() : "(missing)")
                    << "', check for typos in your config.  Disabling rule.");
                return true;
            }

            // Failure to calculate the UED value is a system failure.  
            // This is normally caused by the UED being misconfigured.
            // log a warning and disable the rule but don't fail the init.
            TRACE_LOG("[UEDRule].initialize SYS_ERR UED Value for Session(" 
                      << matchmakingSupplementalData.mPrimaryUserInfo->getSessionId()
                      << ") not found, disabling rule(" << (getRuleName() != nullptr ? getRuleName() : "<nullptr>")
                      << ") for UED(" << (getDefinition().getUEDName() != nullptr ? getDefinition().getUEDName() : "<nullptr>") << ").");
            return true;
        }
        mUEDValue = calculatedValue;
        getDefinition().normalizeUEDValue(mUEDValue);

        mUEDSearchValue = calculatedValue;
        getDefinition().normalizeUEDValue(mUEDSearchValue);

        if (itr != uedCriteriaMap.end())
        {
            const UEDRuleCriteria& uedRuleCriteria = *itr->second;

            if (uedRuleCriteria.getClientUEDSearchValue() != INVALID_USER_EXTENDED_DATA_VALUE)
            {
                mUEDSearchValue = uedRuleCriteria.getClientUEDSearchValue();
                getDefinition().normalizeUEDValue(mUEDSearchValue);
            }

            if (uedRuleCriteria.getOverrideUEDValue() != INVALID_USER_EXTENDED_DATA_VALUE)
            {
                mUEDValue = uedRuleCriteria.getOverrideUEDValue();
                getDefinition().normalizeUEDValue(mUEDValue);

            }

            // lookup the minFitThreshold list we want to use
            const char8_t* thresholdName = uedRuleCriteria.getThresholdName();

            // validate the listName for the rangeOffsetList
            // NOTE: nullptr is expected when the client disables the rule
            mRangeOffsetList = getDefinition().getRangeOffsetList( thresholdName );

            // If the rule is specified by the client it must contain a threshold list.
            if (mRangeOffsetList == nullptr)
            {
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Rule Name '\"%s\"' criteria contains invalid threshold name '\"%s\"', disabling rule.", ((getRuleName() != nullptr)? getRuleName() : "<unavailable>"), ((thresholdName != nullptr)? thresholdName : "<rangeOffsetList name null>"));
                err.setErrMessage(msg);
                return false;
            }

            mMatchmakingAsyncStatus->getUEDRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mUEDRuleStatus));
            updateAsyncStatus();

            TRACE_LOG("[UEDRule].initialize Rule Name '" << getRuleName() << "', thresh '" << thresholdName << "', searchValue (" 
                      << mUEDSearchValue << "), UEDValue (" << mUEDValue << ")");
        }
        
        return true;
    }

    void UEDRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        UserExtendedDataValue gameUEDValue = gameSession.getMatchmakingGameInfoCache()->getCachedGameUEDValue(getDefinition());
        bool isGameEmpty = gameSession.getPlayerRoster()->getParticipantCount() == 0;

        
        if ( mAcceptableRange.isInRange(gameUEDValue) )
        {
            debugRuleConditions.writeRuleCondition("%" PRId64 " minimum required < %" PRId64 " game UED value < %" PRId64 " maximum required", mAcceptableRange.mMinValue, gameUEDValue, mAcceptableRange.mMaxValue);

            calcFitPercentInternal(gameUEDValue, fitPercent, isExactMatch);
        }
        else if (isGameEmpty && mAcceptEmptyGames)
        {
            debugRuleConditions.writeRuleCondition("Game was empty, fit score will be 0.");

            fitPercent = 0.0;
            isExactMatch = false;
        }
        else
        {
            if (gameUEDValue < mAcceptableRange.mMinValue)
            {
                debugRuleConditions.writeRuleCondition("%" PRId64 " game UED value < %" PRId64 " minimum required", gameUEDValue, mAcceptableRange.mMinValue);
            }
            else
            {
                debugRuleConditions.writeRuleCondition(" %" PRId64 " game UED value > %" PRId64 " maximum required", gameUEDValue, mAcceptableRange.mMaxValue);
            }

            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;
        }
    }

    void UEDRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const UEDRule& otherUEDRule = static_cast<const UEDRule&>(otherRule);
        calcFitPercentInternal(otherUEDRule.mUEDValue, fitPercent, isExactMatch);

        
        if ( mAcceptableRange.isInRange(otherUEDRule.mUEDValue) )
        {
            debugRuleConditions.writeRuleCondition("%" PRId64 " minimum required < %" PRId64 " other UED value < %" PRId64 " maximum required", mAcceptableRange.mMinValue, otherUEDRule.mUEDValue, mAcceptableRange.mMaxValue);

        }
        else
        {
            if (otherUEDRule.mUEDValue < mAcceptableRange.mMinValue)
            {
                debugRuleConditions.writeRuleCondition("%" PRId64 " other UED value < %" PRId64 " minimum required", otherUEDRule.mUEDValue, mAcceptableRange.mMinValue);
            }
            else
            {
                debugRuleConditions.writeRuleCondition(" %" PRId64 " other UED value > %" PRId64 " maximum required", otherUEDRule.mUEDValue, mAcceptableRange.mMaxValue);
            }

        }

    }

    // get the fit score of me vs other
    void UEDRule::calcFitPercentInternal(const UserExtendedDataValue& otherUEDValue, float &fitPercent, bool &isExactMatch) const
    {
        fitPercent = getDefinition().getFitPercent(mUEDSearchValue, otherUEDValue);
        isExactMatch = (mUEDSearchValue == otherUEDValue);
    }

    Rule* UEDRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW UEDRule(*this, matchmakingAsyncStatus);
    }

    bool UEDRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {

        if (isMatchAny())
        {
            // If we match the entire range, no need to filter
            return false;
        }

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& uedValuesOrConditions = conditions.push_back();

        if (mAcceptableRange.mMinValue <= mAcceptableRange.mMaxValue)
        {
            // 0 out the fit score if we will be doing a post RETE eval.
            uedValuesOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(getDefinition().getUEDWMEAttributeNameStr(), mAcceptableRange.mMinValue, mAcceptableRange.mMaxValue, mRuleDefinition), 
                0, this));
        }

        if (mAcceptEmptyGames)
        {
            // empty game contributes 0 fit score
            uedValuesOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(getDefinition().getUEDWMEAttributeNameStr(), getDefinition().getWMEInt64AttributeValue(INVALID_USER_EXTENDED_DATA_VALUE), mRuleDefinition), 
                0, this));
        }

        return true; // TODO: for bucketed ranges, consider get precision back by doing a post eval...?
    }

    bool UEDRule::fitFunction(int64_t value, FitScore& fitScore) const
    {
        bool isExactMatch;
        float fitPercent;
        // We need to calculate the fit score based off the actual value the game will have once we join
        calcFitPercentInternal(value, fitPercent, isExactMatch);

        fitScore = calcWeightedFitScore(isExactMatch, fitPercent);

        return isFitScoreMatch(fitScore);
    }

    void UEDRule::updateAcceptableRange()
    {
        mAcceptableRange.setRange(mUEDSearchValue, *mCurrentRangeOffset, getDefinition().getMinRange(), getDefinition().getMaxRange());
    }

    void UEDRule::updateAsyncStatus()
    {
        mUEDRuleStatus.setRuleName(getRuleName());

        mUEDRuleStatus.setMyUEDValue(mUEDValue);

        mUEDRuleStatus.setMinUEDAccepted(mAcceptableRange.mMinValue);
        mUEDRuleStatus.setMaxUEDAccepted(mAcceptableRange.mMaxValue);
    }

    // Override this from Rule since we use range thresholds instead of fit thresholds
    void UEDRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        const UEDRule& otherUEDRule = static_cast<const UEDRule&>(otherRule);

        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[UEDRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
            EA_ASSERT(mRangeOffsetList != nullptr);
            return;
        }

        //Check exact match
        if (evalInfo.mIsExactMatch &&
            mCurrentRangeOffset != nullptr &&
            mCurrentRangeOffset->isExactMatchRequired())
        {
            matchInfo.setMatch(static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight()), 0);
            return;
        }

        RangeList::Range acceptableRange;
        const RangeList::RangePairContainer &rangePairContainer = mRangeOffsetList->getContainer();
        RangeList::RangePairContainer::const_iterator iter = rangePairContainer.begin();
        RangeList::RangePairContainer::const_iterator end = rangePairContainer.end();
        for ( ; iter != end; ++iter )
        {
            // note: we could pre-calc this, but we'd have to store off a list per team, since each team has a different desired size...
            uint32_t matchSeconds = iter->first;
            const RangeList::Range &rangeOffset = iter->second;
            acceptableRange.setRange(mUEDSearchValue, rangeOffset, getDefinition().getMinRange(), getDefinition().getMaxRange());
            if (acceptableRange.isInRange(otherUEDRule.mUEDValue))
            {
                FitScore fitScore = static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight());
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }


    /*! ************************************************************************************************/
    /*! \brief local helper
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void UEDRule::postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        // Ensure a session with this rule disabled can't pull in a session with it enabled
        // This check is not executed when evaluating bidirectionally.
        const UEDRule& otherUEDRule = static_cast<const UEDRule&>(otherRule); // Need to cast here in order to call the protected method getMMSessionId()
                                                                              // as C++ doesn't allow calling protected methods on an instance of a base class
        if (isDisabled() && (!otherRule.isDisabled()))
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("Session (%" PRIu64 ") with rule disabled can't add session (%" PRIu64 ") with rule enabled.", getMMSessionId(), otherUEDRule.getMMSessionId());
            matchInfo.setNoMatch();
            return;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief provide bucketed search value diagnostic
    ***************************************************************************************************/
    void UEDRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        if (getDefinition().isDiagnosticsDisabled())
        {
            return;
        }

        RuleDiagnosticSetupInfo setupInfo("uedSearchValue");
        if (mUEDSearchValue == INVALID_USER_EXTENDED_DATA_VALUE)
        {
            setupInfo.mValueTag = "<NoUed>";
        }
        else
        {
            getDefinition().getDiagnosticBucketPartitions().getDisplayString(setupInfo.mValueTag, mUEDSearchValue);
        }
        setupInfos.push_back(setupInfo);
    }
    
    /*! ************************************************************************************************/
    /*! \brief return bucketed value's game count
    ***************************************************************************************************/
    uint64_t UEDRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (getDefinition().isDiagnosticsDisabled())
        {
            return 0;
        }

        // get from definition's cache, unless its match any in which case can just return total games
        return (isMatchAny() ? helper.getOpenToMatchmakingCount() :
            helper.getUEDGameCount(getDefinition().getName(), mAcceptableRange.mMinValue, mAcceptableRange.mMaxValue));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
