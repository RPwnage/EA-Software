/*! ************************************************************************************************/
/*!
    \file   totalplayerslotsrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/totalplayerslotsrule.h"
#include "gamemanager/matchmaker/rules/totalplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TotalPlayerSlotsRule::TotalPlayerSlotsRule(const TotalPlayerSlotsRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRangeRule(definition, status),
        mDesiredTotalParticipantSlots(0),
        mMinTotalParticipantSlots(0),
        mMaxTotalParticipantSlots(0),
        mMyGroupParticipantCount(0)
    {
    }

    TotalPlayerSlotsRule::TotalPlayerSlotsRule(const TotalPlayerSlotsRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRangeRule(other, status),
        mDesiredTotalParticipantSlots(other.mDesiredTotalParticipantSlots),
        mMinTotalParticipantSlots(other.mMinTotalParticipantSlots),
        mMaxTotalParticipantSlots(other.mMaxTotalParticipantSlots),
        mMyGroupParticipantCount(other.mMyGroupParticipantCount)
    {
    }


    bool TotalPlayerSlotsRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
 
        uint16_t minCount = 0;
        uint16_t maxCount = 0;

        Rete::OrConditionList& capacityOrConditions = conditions.push_back();
        // Participant capacity 
        // We loop over all possible max capacities that can fit us restricted by our max desired.

        minCount = (uint16_t)((mMyGroupParticipantCount < mMinTotalParticipantSlots) ? mMinTotalParticipantSlots : mMyGroupParticipantCount);
        maxCount = (uint16_t) mMaxTotalParticipantSlots;
        if (!isMatchAny())
        {
            // match any matches over our entire allowed range
            // So update our min/max here based off current decay.
            uint16_t minAcceptableCount = (uint16_t)mAcceptableRange.mMinValue;
            uint16_t maxAcceptableCount = (uint16_t)mAcceptableRange.mMaxValue;
            if (minAcceptableCount > minCount && minAcceptableCount <= maxCount)
            {
                minCount = minAcceptableCount;
            }
            if (maxAcceptableCount < maxCount && maxAcceptableCount >= minCount)
            {
                maxCount = maxAcceptableCount;
            }
        }

        capacityOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(TotalPlayerSlotsRuleDefinition::TOTALPLAYERSLOTSRULE_CAPACITY_RETE_KEY, minCount, maxCount, mRuleDefinition),
            0, this
            ));

        return true;
    }

    bool TotalPlayerSlotsRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
         if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
         {
             // Rule is disabled when searching for servers.
             return true;
         }

        eastl::string buf;
        const TotalPlayerSlotsRuleCriteria& totalSlotRulePrefs = criteriaData.getTotalPlayerSlotsRuleCriteria();
        
        // set desired, max, & min game sizes
        const char8_t* rangeOffsetListName = totalSlotRulePrefs.getRangeOffsetListName();
        if (rangeOffsetListName[0] != '\0')
        {
            mDesiredTotalParticipantSlots = totalSlotRulePrefs.getDesiredTotalPlayerSlots();
            mMaxTotalParticipantSlots = totalSlotRulePrefs.getMaxTotalPlayerSlots();
            mMinTotalParticipantSlots = totalSlotRulePrefs.getMinTotalPlayerSlots();
        }

        // Check to see if the rule is disabled.
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
                buf.sprintf("TotalPlayerSlotsRule requested range list(%s) does not exist.", rangeOffsetListName);
                err.setErrMessage(buf.c_str());
                ERR_LOG("[TotalPlayerSlotsRule] " << buf.c_str());
                return false;
            }
        }

        // min must be > 0 (since desired size includes yourself)
        if (mMinTotalParticipantSlots == 0)
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's minTotalParticipantSlots must be > 0 (since it includes yourself).", mMinTotalParticipantSlots).c_str());
            return false;
        }
        // min must be <= max
        if (mMinTotalParticipantSlots > mMaxTotalParticipantSlots)
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's minTotalParticipantSlots(%u) must be <= maxTotalParticipantSlots(%u).", mMinTotalParticipantSlots, mMaxTotalParticipantSlots).c_str());
            return false;
        }
        // ensure min no less than global configured min
        if (mMinTotalParticipantSlots < getDefinition().getGlobalMinPlayerCountInGame())
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's minTotalParticipantSlots(%u) must be >= configured range minimum(%u).", mMinTotalParticipantSlots, getDefinition().getGlobalMinPlayerCountInGame()).c_str());
            return false;
        }
        // ensure max no more than global configured max
        if (mMaxTotalParticipantSlots > getDefinition().getGlobalMaxTotalPlayerSlotsInGame())
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's maxTotalParticipantSlots(%u) must be <= configured range maximum(%u).", mMaxTotalParticipantSlots, getDefinition().getGlobalMaxTotalPlayerSlotsInGame()).c_str());
            return false;
        }
        // desiredTotalPlayerSlots cannot be outside the min, max bounds
        if ( (mDesiredTotalParticipantSlots < mMinTotalParticipantSlots) || (mDesiredTotalParticipantSlots > mMaxTotalParticipantSlots) )
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's desiredTotalParticipantSlots(%u) must be between the minTotalParticipantSlots(%u) and maxTotalParticipantSlots(%u) (inclusive).", mDesiredTotalParticipantSlots, mMinTotalParticipantSlots, mMaxTotalParticipantSlots).c_str());
            return false;
        }

        // matchmaking only validation/settings
        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            // validate total slots info vs the group size (if a group is present)
            size_t userGroupSize = matchmakingSupplementalData.mJoiningPlayerCount;
            if (mDesiredTotalParticipantSlots < userGroupSize)
            {
                // not enough room in the game for the userGroup (group).
                err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's desiredTotalParticipantSlots(%u) must exceed the number of participants in the group(%u)", mDesiredTotalParticipantSlots, userGroupSize).c_str());
                return false;
            }
            mMyGroupParticipantCount = (uint16_t)userGroupSize;
        }
        else
        {
            // For GB where we don't have player data, assume either 0 or 1 player
            mMyGroupParticipantCount = 0;
        }

        // ensure min not below min possible based on other rules
        uint16_t rulesMinPossParticipantCount = 
            getCriteria().calcMinPossPlayerCountFromRules(criteriaData, matchmakingSupplementalData, getDefinition().getGlobalMinPlayerCountInGame());
        if(mMinTotalParticipantSlots < rulesMinPossParticipantCount)
        {
            err.setErrMessage(buf.sprintf("TotalPlayerSlotsRule's minTotalParticipantSlots(%u) must be >= min possible participant count based on other participant count rule's criteria (%u).", mMinTotalParticipantSlots, rulesMinPossParticipantCount).c_str());
            return false;
        }

        updateCachedThresholds(0, true);
        updateAsyncStatus();
        
        return true;
    }

    void TotalPlayerSlotsRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        uint16_t totalParticipantCapacity = gameSession.getTotalParticipantCapacity();

        //Check count conditions that would make us fail.
        if (totalParticipantCapacity < mMinTotalParticipantSlots)
        {
            debugRuleConditions.writeRuleCondition("%hu participants < %hu minimum required", totalParticipantCapacity, mMinTotalParticipantSlots);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }
        else if (totalParticipantCapacity > mMaxTotalParticipantSlots)
        {
            debugRuleConditions.writeRuleCondition("%hu participants > %hu maximum required", totalParticipantCapacity, mMaxTotalParticipantSlots);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }

        if (!mAcceptableRange.isInRange(totalParticipantCapacity))
        {
            debugRuleConditions.writeRuleCondition("%" PRIi64 " minimum range > %hu participants < %" PRIi64 " maximum range", mAcceptableRange.mMinValue, totalParticipantCapacity, mAcceptableRange.mMaxValue);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }

        debugRuleConditions.writeRuleCondition("%" PRIi64 " minimum required <= %hu participants <= %" PRIi64 " maximum required",  mAcceptableRange.mMinValue, totalParticipantCapacity, mAcceptableRange.mMaxValue);
        

        calcFitPercentInternal(totalParticipantCapacity, fitPercent, isExactMatch);
    }


    void TotalPlayerSlotsRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const TotalPlayerSlotsRule& otherTotalPlayerSlotsRule = static_cast<const TotalPlayerSlotsRule&>(otherRule);

        // For total slots rule calculate the fit percent based on the other session's desired total slots
        // against my desired total slots.
        uint16_t otherDesiredTotalParticipantSlots = otherTotalPlayerSlotsRule.mDesiredTotalParticipantSlots;
        

        // finally, we still want to check that we're not matching a group that's too large for our game
        uint16_t groupSize = mMyGroupParticipantCount + otherTotalPlayerSlotsRule.mMyGroupParticipantCount;
        if(groupSize > mMaxTotalParticipantSlots)
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("%hu joining players > %hu", groupSize, mMaxTotalParticipantSlots);

            return;
        }

        debugRuleConditions.writeRuleCondition("%u <= %hu joining players <= %hu", mMinTotalParticipantSlots, groupSize, mMaxTotalParticipantSlots);
        

        calcFitPercentInternal(otherDesiredTotalParticipantSlots, fitPercent, isExactMatch);
    }


    void TotalPlayerSlotsRule::postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        const TotalPlayerSlotsRule& otherTotalPlayerSlotsRule = static_cast<const TotalPlayerSlotsRule&>(otherRule);

        // Note: we also need to do a check of the other session's min/max values (vs our own).
        //   This is to prevent me from sucking someone into my pool in violation of their min/max values.
        //   So you'll only pull another session into your own if your min/max range is inside of their range.
        // check other player's min/max cap (vs mine)
        if ( (otherTotalPlayerSlotsRule.mMinTotalParticipantSlots > mMinTotalParticipantSlots) || (otherTotalPlayerSlotsRule.mMaxTotalParticipantSlots < mMaxTotalParticipantSlots) )
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("Cannot pull other session into my pool; their total particpant slots requirements (min: %hu, max: %hu) are more restrictive than mine (min: %hu, max: %hu).",
               otherTotalPlayerSlotsRule.mMinTotalParticipantSlots, otherTotalPlayerSlotsRule.mMaxTotalParticipantSlots, mMinTotalParticipantSlots, mMaxTotalParticipantSlots);
            matchInfo.setNoMatch();
            return;
        }
    }
    
    void TotalPlayerSlotsRule::calcFitPercentInternal(uint16_t otherDesiredTotalPlayerSlots, float &fitPercent, bool &isExactMatch) const
    {
        fitPercent = getDefinition().getFitPercent(otherDesiredTotalPlayerSlots, mDesiredTotalParticipantSlots);
        isExactMatch = (mDesiredTotalParticipantSlots == otherDesiredTotalPlayerSlots);
    }

    uint16_t TotalPlayerSlotsRule::calcCreateGameParticipantCapacity() const
    {
        if (isDisabled())
        {
            WARN_LOG("[TotalPlayerSlotsRule].calcCreateGameParticipantCapacity Total Slots Rule is disabled.");
            return 0;
        }

        if (mCurrentRangeOffset == nullptr)
            return 0;

        return calcCreateGameParticipantCapacity(*mCurrentRangeOffset, mDesiredTotalParticipantSlots, mMaxTotalParticipantSlots, mMinTotalParticipantSlots);
    }

    uint16_t TotalPlayerSlotsRule::calcCreateGameParticipantCapacity(const RangeList::Range& desiredRange,
        uint16_t desiredTotalParticipantSlots, uint16_t maxTotalParticipantSlots, uint16_t minTotalParticipantSlots)
    {
        if (desiredRange.isExactMatchRequired())
            return desiredTotalParticipantSlots;

        if (desiredRange.isMatchAny())
        {
            return maxTotalParticipantSlots;
        }

        uint16_t calculatedPlayerCapacity = (uint16_t)(desiredTotalParticipantSlots + desiredRange.mMaxValue);
        if (calculatedPlayerCapacity > maxTotalParticipantSlots)
            return maxTotalParticipantSlots;

        if (calculatedPlayerCapacity < minTotalParticipantSlots)
            return minTotalParticipantSlots;

        return calculatedPlayerCapacity;
    }

    bool TotalPlayerSlotsRule::calcParticipantCapacityAtInitialAndFinalTimes(const TotalPlayerSlotsRuleCriteria& totalSlotRuleCriteria,
        const RuleDefinitionCollection& ruleDefinitionCollection, uint16_t& capacityAtInitialTime, uint16_t& capacityAtFinalTime)
    {
        if (totalSlotRuleCriteria.getRangeOffsetListName()[0] == '\0')
            return false;

        const TotalPlayerSlotsRuleDefinition* defn = ruleDefinitionCollection.getRuleDefinition<TotalPlayerSlotsRuleDefinition>();
        if ((defn == nullptr) || defn->isDisabled())
        {
            WARN_LOG("[TotalPlayerSlotsRule].calcParticipantCapacityAtInitialAndFinalTimes: internal error could not TotalPlayerSlotsRule enabled but its definition was not found. Calculation/validations being skipped.");
            return false;
        }
        const RangeList * rangeOffsetList = defn->getRangeOffsetList(totalSlotRuleCriteria.getRangeOffsetListName());
        if ((rangeOffsetList == nullptr) || rangeOffsetList->getContainer().empty())
        {
            WARN_LOG("[TotalPlayerSlotsRule].calcParticipantCapacityAtInitialAndFinalTimes: internal error could not calculate TotalPlayerSlotsRule participant capacities. Calculation/validations being skipped.");
            return false;
        }

        capacityAtInitialTime = calcCreateGameParticipantCapacity(rangeOffsetList->getContainer()[0].second,
            totalSlotRuleCriteria.getDesiredTotalPlayerSlots(), totalSlotRuleCriteria.getMaxTotalPlayerSlots(),
            totalSlotRuleCriteria.getMinTotalPlayerSlots());

        capacityAtFinalTime = calcCreateGameParticipantCapacity(rangeOffsetList->getContainer()[rangeOffsetList->getContainer().size() - 1].second,
            totalSlotRuleCriteria.getDesiredTotalPlayerSlots(), totalSlotRuleCriteria.getMaxTotalPlayerSlots(),
            totalSlotRuleCriteria.getMinTotalPlayerSlots());
        return true;
    }

    void TotalPlayerSlotsRule::updateAcceptableRange()
    {
        mAcceptableRange.setRange(mDesiredTotalParticipantSlots, *mCurrentRangeOffset, getDefinition().getGlobalMinPlayerCountInGame(), getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
    }

    void TotalPlayerSlotsRule::updateAsyncStatus()
    {
        if (mCurrentRangeOffset != nullptr)
        {
            if (mCurrentRangeOffset->isExactMatchRequired())
            {
                // When exact match required, min/max all both desired total slots
                mMatchmakingAsyncStatus->getTotalPlayerSlotsRuleStatus().setMaxTotalPlayerSlotsAccepted(mDesiredTotalParticipantSlots);
                mMatchmakingAsyncStatus->getTotalPlayerSlotsRuleStatus().setMinTotalPlayerSlotsAccepted(mDesiredTotalParticipantSlots); 
            }
            else 
            {
                // get the total slots accepted for current threshold
                uint16_t maxTotalParticipantSlotsAccepted = calcCreateGameParticipantCapacity();
                uint16_t minTotalParticipantSlotsAccepted = calcMinTotalParticipantSlotsAccepted();
                mMatchmakingAsyncStatus->getTotalPlayerSlotsRuleStatus().setMaxTotalPlayerSlotsAccepted(maxTotalParticipantSlotsAccepted);
                mMatchmakingAsyncStatus->getTotalPlayerSlotsRuleStatus().setMinTotalPlayerSlotsAccepted(minTotalParticipantSlotsAccepted);
            }
        }
    }

    uint16_t TotalPlayerSlotsRule::calcMinTotalParticipantSlotsAccepted() const
    {
        if (mCurrentRangeOffset != nullptr)
        {
            uint16_t rangeOffsetDifference = (uint16_t)mCurrentRangeOffset->mMinValue;

            // Clamp the min total slots to the absolute minimum
            if (mDesiredTotalParticipantSlots - mMinTotalParticipantSlots <= rangeOffsetDifference)
            {
                return mMinTotalParticipantSlots;
            }
            else
            {
                return mDesiredTotalParticipantSlots - rangeOffsetDifference;
            }
        }

        return 0;
    }

    // Override this from Rule since we use range thresholds instead of fit thresholds
    void TotalPlayerSlotsRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        const TotalPlayerSlotsRule& otherTotalPlayerSlotsRule = static_cast<const TotalPlayerSlotsRule&>(otherRule);

        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[TotalPlayerSlotsRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
            EA_ASSERT(mRangeOffsetList != nullptr);
            return;
        }

        //Check exact match
        if (evalInfo.mIsExactMatch &&
            mCurrentRangeOffset != nullptr && 
            mCurrentRangeOffset->isExactMatchRequired())
        {
            matchInfo.setMatch(calcWeightedFitScore(evalInfo.mIsExactMatch, evalInfo.mFitPercent), 0);
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
            acceptableRange.setRange(mDesiredTotalParticipantSlots, rangeOffset, getDefinition().getGlobalMinPlayerCountInGame(), getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
            if (acceptableRange.isInRange(otherTotalPlayerSlotsRule.mDesiredTotalParticipantSlots))
            {
                FitScore fitScore = calcWeightedFitScore(evalInfo.mIsExactMatch, evalInfo.mFitPercent);
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
