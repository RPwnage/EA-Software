/*! ************************************************************************************************/
/*!
    \file teambalancerule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teambalancerule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamBalanceRule::TeamBalanceRule(const TeamBalanceRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(definition, matchmakingAsyncStatus),
          mDesiredTeamBalance(0),
          mRangeOffsetList(nullptr),
          mCurrentRangeOffset(nullptr),
          mJoiningPlayerCount(0)
    {
    }

    TeamBalanceRule::TeamBalanceRule(const TeamBalanceRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(other, matchmakingAsyncStatus),
          mDesiredTeamBalance(other.mDesiredTeamBalance),
          mRangeOffsetList(other.mRangeOffsetList),
          mCurrentRangeOffset(other.mCurrentRangeOffset),
          mJoiningPlayerCount(other.mJoiningPlayerCount)
    {
    }


    TeamBalanceRule::~TeamBalanceRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamBalanceRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamBalanceRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for servers.
            return true;
        }

        const TeamBalanceRulePrefs& rulePrefs = criteriaData.getTeamBalanceRulePrefs();

        // validate the listName for the rangeOffsetList
        // NOTE: empty string is expected when the client disables the rule
        if (rulePrefs.getRangeOffsetListName()[0] == '\0')
        {
            // Rule is disabled (mRangeOffsetList == nullptr)
            return true;
        }
        else
        {
            mRangeOffsetList = getDefinition().getRangeOffsetList(rulePrefs.getRangeOffsetListName());
            if (mRangeOffsetList == nullptr)
            {
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Couldn't find a Range Offset List named \"%s\" in rule definition \"%s\".", rulePrefs.getRangeOffsetListName(), getRuleName());
                err.setErrMessage(msg);
                return false;
            }

            // validate joining player's teamId?
            mDesiredTeamBalance = rulePrefs.getMaxTeamSizeDifferenceAllowed();
        }

        mJoiningPlayerCount = 0; // MM_AUDIT: need to provide hint for GB so we don't assume a single player joining.
        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            mJoiningPlayerCount = (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    UpdateThresholdResult TeamBalanceRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // If the rule is disabled we don't need to decay.
        if (isDisabled())
        {
            mNextUpdate = UINT32_MAX;
            return NO_RULE_CHANGE;
        }

        const RangeList::Range *originalRangeOffset = mCurrentRangeOffset;
        if (mRangeOffsetList != nullptr)
            mCurrentRangeOffset = mRangeOffsetList->getRange(elapsedSeconds, &mNextUpdate);

        if ((originalRangeOffset != mCurrentRangeOffset) || forceUpdate)
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

                // Search team requirements, only one for all teams
                mAcceptableBalanceRange.setRange(mDesiredTeamBalance, *mCurrentRangeOffset, 0, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());

            }

            updateAsyncStatus();
            return RULE_CHANGED;
        }

        return NO_RULE_CHANGE;
    }


    /*! ************************************************************************************************/
    /*! \brief evaluate a matchmaking session's desired game COUNT against your desired value.  
        if single group match,  one group only wants to play against another group which meets the game COUNT rule.
        Returns NO_MATCH  if the session is not a match (otherwise it returns a weighted fit score).

        Note: we test desired vs desired, not desired vs actual pool COUNT

        \param[in]  session The other matchmaking session to evaluate this rule against.
    *************************************************************************************************/
    void TeamBalanceRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const TeamBalanceRule* otherRule = static_cast<const TeamBalanceRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));
        bool otherRuleIsDisabled = (otherRule == nullptr) || otherRule->isDisabled();

        // If both players don't care about balance, then we're good.
        if (isDisabled() && otherRuleIsDisabled)
        {
            // 0,0 means matches at 0 fitscore, with 0 time offset
            sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
            sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);

            return;
        }

        // If only one is disabled, then the non-disabled one is the only one that can match (and setup the game)
        if (isDisabled() != otherRuleIsDisabled)
        {
            sessionsMatchInfo.sMyMatchInfo.setMatch(isDisabled() ? FIT_SCORE_NO_MATCH : 0,0);
            sessionsMatchInfo.sOtherMatchInfo.setMatch(otherRuleIsDisabled ? FIT_SCORE_NO_MATCH : 0,0);

            if (!isDisabled())
            {
                sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other disabled team balance");
                
            }
            return;
        }
        
        // The session with the lesser balance matches (or both if they're equal balance):
        if (getAcceptableTeamBalance() > otherRule->getAcceptableTeamBalance())
        {
            sessionsMatchInfo.sMyMatchInfo.setMatch(FIT_SCORE_NO_MATCH,0);
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other team balance %u < %u", otherRule->getAcceptableTeamBalance(), getAcceptableTeamBalance());
            
        }
        else
        {
            sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);

            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other team balance %u > %u", otherRule->getAcceptableTeamBalance(), getAcceptableTeamBalance());
            
        }
        
        sessionsMatchInfo.sOtherMatchInfo.setMatch(otherRule->getAcceptableTeamBalance() > getAcceptableTeamBalance() ? FIT_SCORE_NO_MATCH : 0,0);
    }

    FitScore TeamBalanceRule::calcWeightedFitScoreInternal(bool isExactMatch, float fitPercent, uint16_t actualTeamBalance) const
    {
        RuleWeight weight = mRuleDefinition.getWeight();
        if (fitPercent == FIT_PERCENT_NO_MATCH)
            return FIT_SCORE_NO_MATCH;

        if (mRuleEvaluationMode != RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD)
        {
            const RangeList::Range* range = (mRuleEvaluationMode != RULE_EVAL_MODE_LAST_THRESHOLD) ? mCurrentRangeOffset : (mRangeOffsetList ? mRangeOffsetList->getLastRange() : nullptr);
            if (range != nullptr)
            {
                RangeList::Range curAcceptableBalanceRange;
                curAcceptableBalanceRange.setRange(mDesiredTeamBalance, *range, 0, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
                if (actualTeamBalance > (uint16_t)curAcceptableBalanceRange.mMaxValue) 
                {
                    return FIT_SCORE_NO_MATCH;
                }
            }
        }

        // success
        return static_cast<FitScore>(fitPercent * weight);
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamBalanceRule::updateAsyncStatus()
    {
        TeamBalanceRuleStatus &teamBalanceRuleStatus = mMatchmakingAsyncStatus->getTeamBalanceRuleStatus();
        teamBalanceRuleStatus.setMaxTeamSizeDifferenceAccepted(getAcceptableTeamBalance());
    }

    FitScore TeamBalanceRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        
        if (isDisabled())
            return 0;

        if (gameSession.getTeamCount() == 1 || mJoiningPlayerCount == 0)
        {
            debugRuleConditions.writeRuleCondition("Only 1 team, balance always acceptable.");
            return 0;
        }

        const TeamIdSizeList* myTeamIds = getCriteria().getTeamIdSizeListFromRule();
        if (!myTeamIds)
            return 0;


        eastl::vector<uint16_t> teamSizes;
        const GameManager::PlayerRoster& gameRoster = *(gameSession.getPlayerRoster());
        uint16_t teamCount = gameSession.getTeamCount();
        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            teamSizes.push_back(gameRoster.getTeamSize(teamIndex));
        }


        uint16_t minTeamIndex = 0;
        uint16_t maxTeamIndex = 0;
        uint16_t minTeamSize = getDefinition().getGlobalMaxTotalPlayerSlotsInGame();
        uint16_t maxTeamSize = 0;

        TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
        TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
        for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
        {
            TeamId myTeamId = teamIdSize->first;
            uint16_t myTeamJoinCount = teamIdSize->second;

            uint16_t minMyTeamSize = getDefinition().getGlobalMaxTotalPlayerSlotsInGame();
            uint16_t minMyTeamIndex = 0;

            // Check the teams, find out which one this group could/would join, calc imba afterwards


            // See if our team id is in the game, and if we could join an ANY team id:
            bool foundMyTeam = false;
            for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                TeamId teamId = gameSession.getTeamIdByIndex(teamIndex);
                if (teamId == myTeamId)
                {
                    foundMyTeam = true;
                    break;
                }
            }
            bool canUseAnyTeam = (!foundMyTeam || gameSession.getGameSettings().getAllowSameTeamId());

            // We find the two teams with the largest and smallest sizes:
            for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                TeamId curTeamId = gameSession.getTeamIdByIndex(teamIndex);
                uint16_t curTeamSize = teamSizes[teamIndex];
                if (curTeamSize < minTeamSize)
                {
                    minTeamSize = curTeamSize;
                    minTeamIndex = teamIndex;
                }
                if (curTeamSize > maxTeamSize)
                {
                    maxTeamSize = curTeamSize;
                    maxTeamIndex = teamIndex;
                }

                // Find the smallest team I can join:
                if (myTeamId == ANY_TEAM_ID || curTeamId == myTeamId || (curTeamId == ANY_TEAM_ID && canUseAnyTeam))
                {
                    if (curTeamSize < minMyTeamSize)
                    {
                        minMyTeamSize = curTeamSize;
                        minMyTeamIndex = teamIndex;
                    }
                }
            }

            // Add my join count to the smallest team I can join, 
            minMyTeamSize += myTeamJoinCount;
            teamSizes[minMyTeamIndex] += myTeamJoinCount;

            // Check if we need to Update the max after I join my team.
            if (minMyTeamSize > maxTeamSize)
            {
                maxTeamSize = minMyTeamSize;
                maxTeamIndex = minMyTeamIndex;
            }

            // If I could join the smallest team, I'll try to, so:
            if (minMyTeamIndex == minTeamIndex)
            {
                // find the team with 2nd least size:
                uint16_t minTeamIndex2 = minTeamIndex;
                uint16_t minTeamSize2 = getDefinition().getGlobalMaxTotalPlayerSlotsInGame();
                for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
                {
                    uint16_t curTeamSize = teamSizes[teamIndex];
                    if (curTeamSize < minTeamSize2 && teamIndex != minTeamIndex)
                    {
                        minTeamSize2 = curTeamSize;
                        minTeamIndex2 = teamIndex;
                    }
                }
            
                // Update the min (either the team I was joining, or the 2nd smallest team)
                if (minTeamSize2 < minMyTeamSize)
                {
                    minTeamSize = minTeamSize2;
                    minTeamIndex = minTeamIndex2;
                }
                else
                {
                    // have to set the size here so that we account for myself joining.
                    minTeamSize = minMyTeamSize;
                    minTeamIndex = minMyTeamIndex;
                }
            }
        }
        uint16_t actualTeamBalance = maxTeamSize - minTeamSize;

        debugRuleConditions.writeRuleCondition("Largest team %u of size %u, allowed %u balance, from smallest team %u of size %u", 
            gameSession.getTeamIdByIndex(maxTeamIndex), maxTeamSize, getAcceptableTeamBalance(), gameSession.getTeamIdByIndex(minTeamIndex), minTeamSize);
        
        float fitPercent = getDefinition().getFitPercent(actualTeamBalance, 0);
        bool isExactMatch = (actualTeamBalance == 0);

        return calcWeightedFitScoreInternal(isExactMatch, fitPercent, actualTeamBalance);
    }

    Rule* TeamBalanceRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamBalanceRule(*this, matchmakingAsyncStatus);
    }

    // RETE implementations
    bool TeamBalanceRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        // Imbalance Rule:
        uint16_t maxImbalance = getAcceptableTeamBalance() + mJoiningPlayerCount;  // (This assumes all players join the same team)
        if (maxImbalance > getCriteria().getTeamCapacity())
            maxImbalance = getCriteria().getTeamCapacity();

        Rete::OrConditionList& capacityOrConditions = conditions.push_back();

        // Our RETE evaluation accounts for 0 fit, we will take the value determined by the POST-RETE
        // evaluation.
        FitScore zeroFit = 0;
        capacityOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(TeamBalanceRuleDefinition::TEAM_IMBALANCE_GAME_KEY,  0, maxImbalance, mRuleDefinition),
            zeroFit, this
            ));

        return true;
    }
    // end RETE
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
