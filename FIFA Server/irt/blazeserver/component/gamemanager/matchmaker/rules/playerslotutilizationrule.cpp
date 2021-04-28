/*! ************************************************************************************************/
/*!
    \file   playerslotutilizationrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationrule.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    PlayerSlotUtilizationRule::PlayerSlotUtilizationRule(const PlayerSlotUtilizationRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRangeRule(definition, status),
        mDesiredPercentFull(0),
        mMinPercentFull(0),
        mMaxPercentFull(0),
        mMyGroupParticipantCount(0),
        mForAsyncStatusOnly(false)
    {
    }

    PlayerSlotUtilizationRule::PlayerSlotUtilizationRule(const PlayerSlotUtilizationRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRangeRule(other, status),
        mDesiredPercentFull(other.mDesiredPercentFull),
        mMinPercentFull(other.mMinPercentFull),
        mMaxPercentFull(other.mMaxPercentFull),
        mMyGroupParticipantCount(other.mMyGroupParticipantCount),
        mForAsyncStatusOnly(other.mForAsyncStatusOnly)
    {
    }


    bool PlayerSlotUtilizationRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        ERR_LOG("[PlayerSlotUtilizationRule].addConditions this rule is not enabled for RETE addConditions."); 
        return true;
    }

    bool PlayerSlotUtilizationRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        eastl::string buf;

        // This rule only applies to Find Game and Game Browser Searches
        if (!matchmakingSupplementalData.mMatchmakingContext.canSearchJoinableGames())
        {
            // PlayerSlotUtilizationRule needs special handling as it has async statuses to send but is only evaluated on search slave.
            // Only the MMslave (isSearchingPlayers true) can send async statuses. If current context can't, just return here.
            if (!matchmakingSupplementalData.mMatchmakingContext.isSearchingPlayers())
                return true;

            // Else, disable the evaluation but use this instance for status updates.
            mForAsyncStatusOnly = true;
        }

        const PlayerSlotUtilizationRuleCriteria& rulePrefs = criteriaData.getPlayerSlotUtilizationRuleCriteria();

        // Check to see if the rule is disabled.
        const char8_t* rangeOffsetListName = rulePrefs.getRangeOffsetListName();
        if(rangeOffsetListName[0] == '\0')
        {
            // NOTE: nullptr is expected when the client disables the rule
            mRangeOffsetList = nullptr;
            return true;
        }
        else
        {
            // validate the listName for the rangeOffsetList
            mRangeOffsetList = getDefinition().getRangeOffsetList(rangeOffsetListName);
            if (mRangeOffsetList == nullptr)
            {
                err.setErrMessage("PlayerSlotUtilizationRule requested range list does not exist.");
                ERR_LOG("[PlayerSlotUtilizationRule] Warning - Rule being initialized with nonexistant range list '" << rangeOffsetListName << "'!");
                return false;
            }
        }

        // desired, max, & min percent full
        mDesiredPercentFull = rulePrefs.getDesiredPercentFull();
        mMaxPercentFull = rulePrefs.getMaxPercentFull();
        mMinPercentFull = rulePrefs.getMinPercentFull();

        // validate percent full bounds for internal consistency.
        if (mDesiredPercentFull == 0)
        {
            err.setErrMessage("PlayerSlotUtilizationRule's DesiredPercentFull must be > 0 (since it includes yourself).");
            return false;
        }
        // min must be <= max
        if (mMinPercentFull > mMaxPercentFull)
        {
            err.setErrMessage(buf.sprintf("PlayerSlotUtilizationRule's minPercentFull(%u) must be <= maxPercentFull(%u).", mMinPercentFull, mMaxPercentFull).c_str());
            return false;
        }
        if(mMaxPercentFull > 100)
        {
            err.setErrMessage(buf.sprintf("PlayerSlotUtilizationRule's maxPercentFull(%u) must be <= 100.", mMaxPercentFull).c_str());
            return false;
        }
        // DesiredPercentFull cannot be outside the min, max bounds
        if ( (mDesiredPercentFull < mMinPercentFull) || (mDesiredPercentFull > mMaxPercentFull) )
        {
            err.setErrMessage(buf.sprintf("PlayerSlotUtilizationRule's desiredPercentFull(%u) must be between the minPercentFull(%u) and maxPercentFull(%u) (inclusive).", mDesiredPercentFull, mMinPercentFull, mMaxPercentFull).c_str());
            return false;
        }

        // matchmaking only validation/settings
        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            // store max total possible size from all rules if available, otherwise use global max
            const uint16_t rulesMaxPossSlots = getCriteria().calcMaxPossPlayerSlotsFromRules(criteriaData, matchmakingSupplementalData, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());

            // validate percent full is possible based on the group size and max total
            size_t userGroupSize = matchmakingSupplementalData.mJoiningPlayerCount;
            if (userGroupSize > rulesMaxPossSlots)
            {
                err.setErrMessage(buf.sprintf("PlayerSlotUtilizationRule's userGroupSize(%u) exceeds the max allowed participants in a game(%u) your overall matchmaking criteria could match.", userGroupSize, rulesMaxPossSlots).c_str());
                return false;
            }
            
            // check games can exist to meet the input percents, based on your group size, and max game size.
            const float groupMinPossPercentFull = ((float)userGroupSize / (float)rulesMaxPossSlots * 100);
            if ((float)mDesiredPercentFull < groupMinPossPercentFull)
            {
                err.setErrMessage(buf.sprintf("PlayerSlotUtilizationRule's desiredPercentFull(%u), is less than the calculated lowest possible value of (%.2f) percent, based on your group size (%u) and max game size based on your overall matchmaking criteria (%u).",
                    mDesiredPercentFull, groupMinPossPercentFull, userGroupSize, rulesMaxPossSlots).c_str());
                WARN_LOG("[PlayerSlotUtilizationRule].initialize Warning: " << buf.c_str());
            }

            // save off the session's usergroup size and userGroupId
            mMyGroupParticipantCount = (uint16_t)userGroupSize;
        }
        else
        {
            // For GB, 0. For MM, set to the searcher's actual group size above
            mMyGroupParticipantCount = 0;
        }

        updateCachedThresholds(0, true);
        updateAsyncStatus();
        
        return true;
    }

    void PlayerSlotUtilizationRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        const Blaze::GameManager::PlayerRoster& gameRoster = *gameSession.getPlayerRoster();
        uint16_t totalPossibleParticipants = gameRoster.getParticipantCount() + mMyGroupParticipantCount;
        uint16_t totalParticipantCapacity = gameSession.getTotalParticipantCapacity();

        float possiblePercentFull = ((float)totalPossibleParticipants / (float)totalParticipantCapacity * 100);

        uint16_t minAcceptedPercentFull = calcMinPercentFullAccepted();
        uint16_t maxAcceptedPercentFull = calcMaxPercentFullAccepted();

        //user specifies an integer for desired percent.
        //round game fullness to integer for matching vs desired value itself
        uint16_t possiblePercentFullRounded = (uint16_t)((possiblePercentFull - floor(possiblePercentFull) < 0.5)? floor(possiblePercentFull) : ceil(possiblePercentFull));
        if (minAcceptedPercentFull == maxAcceptedPercentFull)
        {
            possiblePercentFull = possiblePercentFullRounded;
        }

        //Check count conditions that would make us fail.
        if ((possiblePercentFull < minAcceptedPercentFull) ||
            (possiblePercentFull > maxAcceptedPercentFull))
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            debugRuleConditions.writeRuleCondition("Adding %u players makes game %.2f%% full, not in acceptable range %u%% ... %u%%", 
                mMyGroupParticipantCount, possiblePercentFull, minAcceptedPercentFull, maxAcceptedPercentFull);

            return;
        }

        calcFitPercentInternal(possiblePercentFullRounded, fitPercent, isExactMatch);
        debugRuleConditions.writeRuleCondition("Adding %u players makes game %.2f%% full, in acceptable range %u%% ... %u%%", 
            mMyGroupParticipantCount, possiblePercentFull, minAcceptedPercentFull, maxAcceptedPercentFull);
        
    }


    void PlayerSlotUtilizationRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // This rule is not evaluated on create game, no-op.
    }


    void PlayerSlotUtilizationRule::postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        // This rule is not evaluated on create game, no-op.
    }

    void PlayerSlotUtilizationRule::calcFitPercentInternal(uint16_t otherDesiredPercentFull, float &fitPercent, bool &isExactMatch) const
    {
        fitPercent = getDefinition().getFitPercent(otherDesiredPercentFull, mDesiredPercentFull);
        isExactMatch = (mDesiredPercentFull == otherDesiredPercentFull);
    }

    void PlayerSlotUtilizationRule::updateAcceptableRange()
    {
        // range's bounds are the total possible percent range
        mAcceptableRange.setRange(mDesiredPercentFull, *mCurrentRangeOffset, 0, 100);
    }

    UpdateThresholdResult PlayerSlotUtilizationRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        UpdateThresholdResult result = SimpleRangeRule::updateCachedThresholds(elapsedSeconds, forceUpdate);

        // if mForAsyncStatusOnly no change for evals, for simplicity return no change. The status updates will still be reflected in later async status notifications.
        if (result == RULE_CHANGED && mForAsyncStatusOnly)
            return NO_RULE_CHANGE;

        return result;
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void PlayerSlotUtilizationRule::updateAsyncStatus()
    {
        if (mCurrentRangeOffset != nullptr)
        {
            if (mCurrentRangeOffset->isExactMatchRequired())
            {
                // When exact match required, min/max are both desired percent
                mMatchmakingAsyncStatus->getPlayerSlotUtilizationRuleStatus().setMaxPercentFullAccepted(mDesiredPercentFull);
                mMatchmakingAsyncStatus->getPlayerSlotUtilizationRuleStatus().setMinPercentFullAccepted(mDesiredPercentFull); 
            }
            else 
            {
                // get the percents required for current threshold
                mMatchmakingAsyncStatus->getPlayerSlotUtilizationRuleStatus().setMaxPercentFullAccepted(calcMaxPercentFullAccepted());
                mMatchmakingAsyncStatus->getPlayerSlotUtilizationRuleStatus().setMinPercentFullAccepted(calcMinPercentFullAccepted());
            }
        }
    }

    uint8_t PlayerSlotUtilizationRule::calcMinPercentFullAccepted() const
    {
        if (mCurrentRangeOffset != nullptr)
        {
            uint8_t rangeOffsetDifference = (uint8_t)mCurrentRangeOffset->mMinValue;

            // Clamp the min percent full to the absolute minimum
            if ((mDesiredPercentFull - mMinPercentFull) <= rangeOffsetDifference)
            {
                return mMinPercentFull;
            }
            else
            {
                return ((rangeOffsetDifference <= mDesiredPercentFull)? (mDesiredPercentFull - rangeOffsetDifference) : 0);
            }
        }

        return 0;
    }

    uint8_t PlayerSlotUtilizationRule::calcMaxPercentFullAccepted() const
    {
        if (mCurrentRangeOffset != nullptr)
        {
            uint8_t rangeOffsetDifference = (uint8_t)mCurrentRangeOffset->mMaxValue;

            // Clamp the max percent full to the absolute max
            if (rangeOffsetDifference >= (mMaxPercentFull - mDesiredPercentFull))
            {
                return mMaxPercentFull;
            }
            else
            {
                return (mDesiredPercentFull + rangeOffsetDifference);
            }
        }

        return 0;
    }

    // Override this from Rule since we use range thresholds instead of fit thresholds
    void PlayerSlotUtilizationRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        const PlayerSlotUtilizationRule& otherPlayerSlotUtilizationRule = static_cast<const PlayerSlotUtilizationRule&>(otherRule);

        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[PlayerSlotUtilizationRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
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
            uint32_t matchSeconds = iter->first;
            const RangeList::Range &rangeOffset = iter->second;
            acceptableRange.setRange(mDesiredPercentFull, rangeOffset, 0, 100);
            if (acceptableRange.isInRange(otherPlayerSlotUtilizationRule.mDesiredPercentFull))
            {
                FitScore fitScore = static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight());
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }


    /*! ************************************************************************************************/
    /*! \brief provide bucketed search value diagnostic
    ***************************************************************************************************/
    void PlayerSlotUtilizationRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        RuleDiagnosticSetupInfo setupInfo("desiredPercentFull");
        getDefinition().getDiagnosticsBucketPartitions().getDisplayString(setupInfo.mValueTag, mDesiredPercentFull);

        setupInfos.push_back(setupInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief return bucketed value's game count
    ***************************************************************************************************/
    uint64_t PlayerSlotUtilizationRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (isMatchAny())
        {
            return helper.getOpenToMatchmakingCount();
        }
        return helper.getPlayerSlotUtilizationGameCount(calcMinPercentFullAccepted(), calcMaxPercentFullAccepted());
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
