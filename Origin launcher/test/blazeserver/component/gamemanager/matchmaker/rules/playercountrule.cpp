/*! ************************************************************************************************/
/*!
    \file   playercountrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/playercountrule.h"
#include "gamemanager/matchmaker/rules/playercountruledefinition.h"
#include "gamemanager/matchmaker/rules/rostersizeruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    PlayerCountRule::PlayerCountRule(const PlayerCountRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRangeRule(definition, status),
        mDesiredParticipantCount(0),
        mMinParticipantCount(0),
        mMaxParticipantCount(0),
        mMyGroupParticipantCount(0),
        mIsSingleGroupMatch(false),
        mIsSimulatingRosterSizeRule(false)
    {
    }

    PlayerCountRule::PlayerCountRule(const PlayerCountRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRangeRule(other, status),
        mDesiredParticipantCount(other.mDesiredParticipantCount),
        mMinParticipantCount(other.mMinParticipantCount),
        mMaxParticipantCount(other.mMaxParticipantCount),
        mMyGroupParticipantCount(other.mMyGroupParticipantCount),
        mIsSingleGroupMatch(other.mIsSingleGroupMatch),
        mUserGroupId(other.mUserGroupId),
        mIsSimulatingRosterSizeRule(other.mIsSimulatingRosterSizeRule)
    {
    }


    bool PlayerCountRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
 
        uint16_t minCount = 0;
        uint16_t maxCount = 0;

        Rete::OrConditionList& sizeOrConditions = conditions.push_back();

        // Game size
        // When we add our nodes to the RETE tree we loop over all possible games that our in our range, minus
        // the number of players we are bringing in.
        // A hard cap based on our passed in required min/max
        minCount = ((mMinParticipantCount - mMyGroupParticipantCount) <= 0 ? 0 : mMinParticipantCount - mMyGroupParticipantCount);
        maxCount = ((mMaxParticipantCount - mMyGroupParticipantCount) <= 0 ? 0 : mMaxParticipantCount - mMyGroupParticipantCount);
        if (!isMatchAny())
        {
            // match any matches over our entire allowed range
            // So update our min/max here based off current decay.
            uint16_t minAcceptableCount = (((uint16_t)mAcceptableRange.mMinValue - mMyGroupParticipantCount) <= 0 ? 0 :  (uint16_t)mAcceptableRange.mMinValue - mMyGroupParticipantCount);
            uint16_t maxAcceptableCount = (((uint16_t)mAcceptableRange.mMaxValue - mMyGroupParticipantCount) <= 0 ? 0 : (uint16_t)mAcceptableRange.mMaxValue - mMyGroupParticipantCount);
            if (minAcceptableCount > minCount && minAcceptableCount <= maxCount)
            {
                minCount = minAcceptableCount;
            }
            if (maxAcceptableCount < maxCount && maxAcceptableCount >= minCount)
            {
                maxCount = maxAcceptableCount;
            }
        }

        sizeOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(PlayerCountRuleDefinition::PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY, minCount, maxCount, mRuleDefinition),
            0, this
            ));

        return true;
    }

    bool PlayerCountRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Player Count not needed when doing resettable games only
            return true;
        }

        eastl::string buf;

        const PlayerCountRuleCriteria& playerCountRulePrefs = criteriaData.getPlayerCountRuleCriteria();

        const RosterSizeRulePrefs& rosterRulePrefs = criteriaData.getRosterSizeRulePrefs();
        mIsSimulatingRosterSizeRule = ((rosterRulePrefs.getMaxPlayerCount() != INVALID_ROSTER_SIZE_RULE_MAX_SIZE) && !matchmakingSupplementalData.mMatchmakingContext.isSearchingPlayers());

        // set desired, max, & min game sizes
        const char8_t* rangeOffsetListName = playerCountRulePrefs.getRangeOffsetListName();
        const char8_t* countRuleSource = "PlayerCountRule";
        if (rangeOffsetListName[0] != '\0')
        {
            // we disallow specifying PlayerCountRule and RosterSizeRule simultaneously
            if (mIsSimulatingRosterSizeRule)
            {
                buf.sprintf("Specifying both PlayerCountRule and RosterSizeRule simultaneously is not allowed.");
                err.setErrMessage(buf.c_str());
                ERR_LOG("[PlayerCountRule] " << buf.c_str());
                return false;
            }
            mDesiredParticipantCount = playerCountRulePrefs.getDesiredPlayerCount();
            mMaxParticipantCount = playerCountRulePrefs.getMaxPlayerCount();
            mMinParticipantCount = playerCountRulePrefs.getMinPlayerCount();
            mIsSingleGroupMatch = playerCountRulePrefs.getIsSingleGroupMatch() > 0 ? true : false;
        }
        else if (mIsSimulatingRosterSizeRule)
        {
            rangeOffsetListName = RosterSizeRuleDefinition::ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME;
            mDesiredParticipantCount = rosterRulePrefs.getMaxPlayerCount();
            mMaxParticipantCount = rosterRulePrefs.getMaxPlayerCount();
            mMinParticipantCount = rosterRulePrefs.getMinPlayerCount();
            mIsSingleGroupMatch = false;
            countRuleSource = "RosterSizeRule";
            TRACE_LOG("[PlayerCountRule].initialize Using appropriate settings from " << rosterRulePrefs.getClassName() << ", rangeOffsetListName(" << rangeOffsetListName << "), mMinParticipantCount(" << mMinParticipantCount << ", mDesiredParticipantCount (" << mDesiredParticipantCount << "), mMaxParticipantCount ("  << mMaxParticipantCount << "), singleGroupMatch (" << mIsSingleGroupMatch << ")");
        }

        if(rangeOffsetListName[0] == '\0')
        {
            // NOTE: nullptr is expected when the client disables the rule, and, did not intend to use game size rule.
            mRangeOffsetList = nullptr;
            return true;
        }
        else
        {
            // validate the listName for the rangeOffsetList
            mRangeOffsetList = getDefinition().getRangeOffsetList(rangeOffsetListName);
            if (mRangeOffsetList == nullptr)
            {
                buf.sprintf("%s requested range list(%s) does not exist.", countRuleSource, rangeOffsetListName);
                err.setErrMessage(buf.c_str());
                ERR_LOG("[PlayerCountRule] " << buf.c_str());
                return false;
            }
        }


        // min must be > 0 (since desired size includes yourself)
        if ((mMinParticipantCount == 0) && (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo()))
        {
            err.setErrMessage(buf.sprintf("%s's minParticipantCount must be > 0 (since it includes yourself).", countRuleSource, mMinParticipantCount).c_str());
            return false;
        }
        // min must be <= max
        if (mMinParticipantCount > mMaxParticipantCount)
        {
            err.setErrMessage(buf.sprintf("%s's minParticipantCount(%u) must be <= maxParticipantCount(%u).", countRuleSource, mMinParticipantCount, mMaxParticipantCount).c_str());
            return false;
        }
        // ensure min no less than global configured min
        if (mMinParticipantCount < getDefinition().getGlobalMinPlayerCountInGame())
        {
            err.setErrMessage(buf.sprintf("%s's minParticipantCount(%u) must be >= configured range minimum(%u).", countRuleSource, mMinParticipantCount, getDefinition().getGlobalMinPlayerCountInGame()).c_str());
            return false;
        }
        // ensure max no more than global configured max
        if (mMaxParticipantCount > getDefinition().getGlobalMaxTotalPlayerSlotsInGame())
        {
            err.setErrMessage(buf.sprintf("%s's maxParticipantCount(%u) must be <= configured range maximum(%u).", countRuleSource, mMaxParticipantCount, getDefinition().getGlobalMaxTotalPlayerSlotsInGame()).c_str());
            return false;
        }
        // desiredPlayerCount cannot be outside the min, max bounds
        if ( (mDesiredParticipantCount < mMinParticipantCount) || (mDesiredParticipantCount > mMaxParticipantCount) )
        {
            err.setErrMessage(buf.sprintf("%s's desiredParticipantCount(%u) must be between the minParticipantCount(%u) and maxParticipantCount(%u) (inclusive).", countRuleSource, mDesiredParticipantCount, mMinParticipantCount, mMaxParticipantCount).c_str());
            return false;
        }
       
        // we ensure max not over max possible based on other rules
        const uint16_t rulesMaxPossSlots = getCriteria().calcMaxPossPlayerSlotsFromRules(criteriaData, matchmakingSupplementalData, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
        if (mMaxParticipantCount > rulesMaxPossSlots)
        {
            err.setErrMessage(buf.sprintf("%s's maxParticipantCount(%u) must be <= max possible total slots based on other total player slots rule's criteria (%u).", countRuleSource, mMaxParticipantCount, rulesMaxPossSlots).c_str());
            return false;
        }

        // matchmaking only validation/settings
        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo() && !mIsSimulatingRosterSizeRule)
        {
            // validate game size info vs the group size (if a group is present)
            size_t userGroupSize = matchmakingSupplementalData.mJoiningPlayerCount;
            if (mDesiredParticipantCount < userGroupSize)
            {
                // not enough room in the game for the userGroup (group).
                err.setErrMessage(buf.sprintf("%s's desiredPlayerCount(%u) must exceed the number of players in the group(%u)", countRuleSource, mDesiredParticipantCount, userGroupSize).c_str());
                return false;
            }
            mMyGroupParticipantCount = (uint16_t)userGroupSize;

            // save off the session's userGroupId
            mUserGroupId = matchmakingSupplementalData.mPlayerJoinData.getGroupId();
        }
        else
        {
            // For GB where we don't have player data, assume 0 players
            mMyGroupParticipantCount = 0;
        }

        updateCachedThresholds(0, true);
        updateAsyncStatus();
        
        return true;
    }

    void PlayerCountRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const Blaze::GameManager::PlayerRoster& gameRoster = *gameSession.getPlayerRoster();
        uint16_t totalPossibleParticipants = gameRoster.getParticipantCount() + mMyGroupParticipantCount;

        
        //Check count conditions that would make us fail.
        if (totalPossibleParticipants < mMinParticipantCount)
        {
            debugRuleConditions.writeRuleCondition("%u player count < %u minimum required", totalPossibleParticipants, mMinParticipantCount);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }
        else if (totalPossibleParticipants > mMaxParticipantCount)
        {
            debugRuleConditions.writeRuleCondition("%u player count > %u maximum required", totalPossibleParticipants, mMaxParticipantCount);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }
        if (!mAcceptableRange.isInRange(totalPossibleParticipants))
        {
            debugRuleConditions.writeRuleCondition("%" PRIi64 " minimum range < %u player count > %" PRIi64 " maximum range", mAcceptableRange.mMinValue, totalPossibleParticipants, mAcceptableRange.mMaxValue);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }

        //Check single group match logic
        if ((mIsSingleGroupMatch && !gameSession.isResetable()) &&
             ((!gameSession.getGameSettings().getEnforceSingleGroupJoin()) ||
              (!gameSession.isGroupAllowedToJoinSingleGroupGame(mUserGroupId))))
        { 
            debugRuleConditions.writeRuleCondition("%s group id not allowed due to single group match", mUserGroupId.toString().c_str());

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }

        debugRuleConditions.writeRuleCondition("%" PRIi64 " minimum range <= %u player count <= %" PRIi64 " maximum range", mAcceptableRange.mMinValue, totalPossibleParticipants, mAcceptableRange.mMaxValue);
        if (mIsSimulatingRosterSizeRule)
        {
            fitPercent = 100.0f;
            isExactMatch = true;
            if (gameRoster.getParticipantCount() < mMinParticipantCount)
            {
                //RosterSizeRule also doesn't not allow mMyGroupParticipantCount to count towards satisfying min.
                debugRuleConditions.writeRuleCondition("%u player count < %u roster size minimum required", gameRoster.getParticipantCount(), mMinParticipantCount);
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
            }
            return;
        }
        calcFitPercentInternal(totalPossibleParticipants, fitPercent, isExactMatch);
    }


    void PlayerCountRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const PlayerCountRule& otherPlayerCountRule = static_cast<const PlayerCountRule&>(otherRule);
        

        uint16_t otherDesiredPlayerCount;
        if (mIsSingleGroupMatch)
        {
            // For a single group match calculate the fit percent based on my group size + the other sessions
            // group size against my desired player count.
            otherDesiredPlayerCount = mMyGroupParticipantCount + otherPlayerCountRule.mMyGroupParticipantCount;

            // desired player count should not be less than the min player count for this rule.
            if (otherDesiredPlayerCount < mMinParticipantCount)
            {
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;

                debugRuleConditions.writeRuleCondition("%u other desired < %u minium required", otherDesiredPlayerCount, mMinParticipantCount);
    
                return;
            }
            else if (otherDesiredPlayerCount > mMaxParticipantCount)
            {
                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;

                debugRuleConditions.writeRuleCondition("%u other desired > %u maximum allowed", otherDesiredPlayerCount, mMaxParticipantCount);
    
                return;
            }
        }
        else
        {
            // For game size rule calculate the fit percent based on the other session's desired player count
            // against my desired player count.
            otherDesiredPlayerCount = otherPlayerCountRule.mDesiredParticipantCount;
        }


        // finally, we still want to check that we're not matching a group that's too large for our game
        uint16_t groupSize = mMyGroupParticipantCount + otherPlayerCountRule.mMyGroupParticipantCount;
        if(groupSize > mMaxParticipantCount)
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("%u player count > %u maximum allowed", groupSize, mMaxParticipantCount);

            return;
        }

        debugRuleConditions.writeRuleCondition("%u minimum required <= %u player count <= %u maximum required", mMinParticipantCount, groupSize, mMaxParticipantCount);
        

        debugRuleConditions.writeRuleCondition("Other desired %u players vs my desired %u players", otherDesiredPlayerCount, mDesiredParticipantCount);
        

        calcFitPercentInternal(otherDesiredPlayerCount, fitPercent, isExactMatch);
    }


    void PlayerCountRule::postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        const PlayerCountRule& otherPlayerCountRule = static_cast<const PlayerCountRule&>(otherRule);

        // Session of single group match can't be pulled into createGame by normal session
        // This check is not executed when evaluating bidirectionally.
        if ((!mIsSingleGroupMatch) && (otherPlayerCountRule.mIsSingleGroupMatch))
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("Non singleGroupMatch session can't add a singleGroupMatch session");
            matchInfo.setNoMatch();
            return;
        }

        // Note: we also need to do a check of the other session's min/max values (vs our own).
        //   This is to prevent me from sucking someone into my pool in violation of their min/max values.
        //   So you'll only pull another session into your own if your min/max range is inside of their range.
        // check other player's min/max cap (vs mine)
        if ( (otherPlayerCountRule.mMinParticipantCount > mMinParticipantCount) || (otherPlayerCountRule.mMaxParticipantCount < mMaxParticipantCount) )
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("cannot pull other session into my pool; their game size requirements (min: %u, max: %u) are more restrictive than mine (min: %u, max: %u).",
                otherPlayerCountRule.mMinParticipantCount, otherPlayerCountRule.mMaxParticipantCount, mMinParticipantCount, mMaxParticipantCount);
            matchInfo.setNoMatch();
            return;
        }
    }
    
    void PlayerCountRule::calcFitPercentInternal(uint16_t otherDesiredPlayerCount, float &fitPercent, bool &isExactMatch) const
    {
        fitPercent = getDefinition().getFitPercent(otherDesiredPlayerCount, mDesiredParticipantCount);
        isExactMatch = (mDesiredParticipantCount == otherDesiredPlayerCount);
    }

    bool PlayerCountRule::isParticipantCountSufficientToCreateGame(size_t potentialParticipantCount) const
    {
        if (isDisabled())
        {
            WARN_LOG("[PlayerCountRule].isParticipantCountSufficientToCreateGame: rule disabled.");
            return true;
        }

        // too few players
        if (potentialParticipantCount < mMinParticipantCount)
            return false;

        // more than enough players
        if (potentialParticipantCount >= mDesiredParticipantCount)
            return true;

        // not an ideal amount, but enough
        float fitPercent = getDefinition().getFitPercent( (uint16_t) potentialParticipantCount, mDesiredParticipantCount);

        TRACE_LOG("[PlayerCountRule].isParticipantCountSufficientToCreateGame[min(" << mMinParticipantCount << ") .. max(" << mMaxParticipantCount << ")] Matched players:" << potentialParticipantCount 
            << " Desired: " << mDesiredParticipantCount << ", fitPercent '" << fitPercent << "', acceptable range '" 
                  << mAcceptableRange.mMinValue << "'..'" << mAcceptableRange.mMaxValue << "'");

        // exact match would've already been caught above, by >= desired
        if (mCurrentRangeOffset != nullptr)
        {
            if (mCurrentRangeOffset->isExactMatchRequired())
            {
                return false;
            }
            else
            {
                return mAcceptableRange.isInRange(potentialParticipantCount);
            }
        }
       
        return false;
    }

    void PlayerCountRule::updateAcceptableRange()
    {
        mAcceptableRange.setRange(mDesiredParticipantCount, *mCurrentRangeOffset, getDefinition().getGlobalMinPlayerCountInGame(), getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void PlayerCountRule::updateAsyncStatus()
    {
        if (mCurrentRangeOffset != nullptr)
        {
            if (mCurrentRangeOffset->isExactMatchRequired())
            {
                // When exact match required, min/max all both desired player count
                mMatchmakingAsyncStatus->getPlayerCountRuleStatus().setMaxPlayerCountAccepted(mDesiredParticipantCount);
                mMatchmakingAsyncStatus->getPlayerCountRuleStatus().setMinPlayerCountAccepted(mDesiredParticipantCount);
            }
            else 
            {
                // get the play capacity required for current threshold
                uint16_t maxPlayerCountAccepted = calcMaxPlayerCountAccepted();
                uint16_t minPlayerCountAccepted = calcMinPlayerCountAccepted();
                mMatchmakingAsyncStatus->getPlayerCountRuleStatus().setMaxPlayerCountAccepted(maxPlayerCountAccepted);
                mMatchmakingAsyncStatus->getPlayerCountRuleStatus().setMinPlayerCountAccepted(minPlayerCountAccepted);
            }
        }
    }

    uint16_t PlayerCountRule::calcMinPlayerCountAccepted() const
    {
        if (mCurrentRangeOffset != nullptr)
        {
            uint16_t playerCountDifference = (uint16_t)mCurrentRangeOffset->mMinValue;

            // Clamp the min player count to the absolute minimum
            if (mDesiredParticipantCount - mMinParticipantCount <= playerCountDifference)
            {
                return mMinParticipantCount;
            }
            else
            {
                return mDesiredParticipantCount - playerCountDifference;
            }
        }

        return 0;
    }

    uint16_t PlayerCountRule::calcMaxPlayerCountAccepted() const
    {
        if (mCurrentRangeOffset != nullptr)
        {
            uint16_t rangeOffsetDifference = (uint16_t)mCurrentRangeOffset->mMaxValue;

            // Clamp the max percent full to the absolute max
            if (rangeOffsetDifference >= (mMaxParticipantCount - mDesiredParticipantCount))
            {
                return mMaxParticipantCount;
            }
            else
            {
                return (mDesiredParticipantCount + rangeOffsetDifference);
            }
        }

        return 0;
    }



    // Override this from Rule since we use range thresholds instead of fit thresholds
    void PlayerCountRule::calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const
    {
        const PlayerCountRule& otherPlayerCountRule = static_cast<const PlayerCountRule&>(otherRule);

        if (mRangeOffsetList == nullptr)
        {
            ERR_LOG("[PlayerCountRule].calcSessionMatchInfo is being called by rule '" << getRuleName() << "' with no range offset list.");
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
            acceptableRange.setRange(mDesiredParticipantCount, rangeOffset, getDefinition().getGlobalMinPlayerCountInGame(), getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
            if (acceptableRange.isInRange(otherPlayerCountRule.mDesiredParticipantCount))
            {
                FitScore fitScore = static_cast<FitScore>(evalInfo.mFitPercent * getDefinition().getWeight());
                matchInfo.setMatch(fitScore, matchSeconds);
                return;
            }
        }

        matchInfo.setNoMatch();
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
