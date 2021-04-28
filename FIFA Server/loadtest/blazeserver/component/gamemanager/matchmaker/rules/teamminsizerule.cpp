/*! ************************************************************************************************/
/*!
    \file teamminsizerule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamminsizerule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamMinSizeRule::TeamMinSizeRule(const TeamMinSizeRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(definition, matchmakingAsyncStatus),
          mDesiredTeamMinSize(0),
          mAbsoluteTeamMinSize(0),
          mRangeOffsetList(nullptr),
          mCurrentRangeOffset(nullptr),
          mSmallestTeamJoiningPlayerCount(0)
    {
    }

    TeamMinSizeRule::TeamMinSizeRule(const TeamMinSizeRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(other, matchmakingAsyncStatus),
          mDesiredTeamMinSize(other.mDesiredTeamMinSize),
          mAbsoluteTeamMinSize(other.mAbsoluteTeamMinSize),
          mRangeOffsetList(other.mRangeOffsetList),
          mCurrentRangeOffset(other.mCurrentRangeOffset),
          mSmallestTeamJoiningPlayerCount(other.mSmallestTeamJoiningPlayerCount)
    {
    }


    TeamMinSizeRule::~TeamMinSizeRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamMinSizeRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamMinSizeRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
         if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
         {
             // Rule is disabled when searching for servers.
             return true;
         }

        const TeamMinSizeRulePrefs& rulePrefs = criteriaData.getTeamMinSizeRulePrefs();

        // validate the listName for the rangeOffsetList
        // NOTE: empty string is expected when the client disables the rule
        if (rulePrefs.getRangeOffsetListName()[0] == '\0')
        {
            // Rule is disabled (mRangeOffsetList == nullptr)
            return true;
        }
        else
        {
            uint16_t teamCountFromCriteria = getCriteria().calcTeamCountFromRules(criteriaData, matchmakingSupplementalData);
            
            if (teamCountFromCriteria != 0 && getDefinition().getGlobalMinTotalPlayerSlotsInGame() >= teamCountFromCriteria)
            {
                mAbsoluteTeamMinSize = getDefinition().getGlobalMinTotalPlayerSlotsInGame() / teamCountFromCriteria;
            }
            else
            {
                mAbsoluteTeamMinSize = 0;
            }

            mRangeOffsetList = getDefinition().getRangeOffsetList(rulePrefs.getRangeOffsetListName());
            if (mRangeOffsetList == nullptr)
            {
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Couldn't find a Range Offset List named \"%s\" in rule definition \"%s\".", rulePrefs.getRangeOffsetListName(), getRuleName());
                err.setErrMessage(msg);
                return false;
            }


            // validate joining player's teamId?
            mDesiredTeamMinSize = rulePrefs.getTeamMinSize();
        }

        mSmallestTeamJoiningPlayerCount = 0; // MM_AUDIT: need to provide hint for GB so we don't assume a single player joining.
        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            // Use the size of the smallest joining team:
            uint16_t smallestTeamSize = 0;
            TeamIdRoleSizeMap::const_iterator teamSizeIter = matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.begin();
            TeamIdRoleSizeMap::const_iterator teamSizeEnd = matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.end();
            for (; teamSizeIter != teamSizeEnd; ++teamSizeIter)
            {
                uint16_t curTeamSize = 0;
                RoleSizeMap::const_iterator roleSizeIter = teamSizeIter->second.begin();
                RoleSizeMap::const_iterator roleSizeEnd = teamSizeIter->second.end();
                for (; roleSizeIter != roleSizeEnd; ++roleSizeIter)
                {
                    curTeamSize += roleSizeIter->second;
                }

                if (curTeamSize < smallestTeamSize || smallestTeamSize == 0)
                    smallestTeamSize = curTeamSize;
            }
            mSmallestTeamJoiningPlayerCount = smallestTeamSize;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    UpdateThresholdResult TeamMinSizeRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
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
                mAcceptableSizeRange.setRange(mDesiredTeamMinSize, *mCurrentRangeOffset, mAbsoluteTeamMinSize, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());

            }

            updateAsyncStatus();
            return RULE_CHANGED;
        }

        return NO_RULE_CHANGE;
    }


    /*! ************************************************************************************************/
    /*! \brief MinSizeRule always allows MM to create games. (Later before finalization the number of players will be limited).

        \param[in]  session The other matchmaking session to evaluate this rule against.
    *************************************************************************************************/
    void TeamMinSizeRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        // 0,0 means matches at 0 fitscore, with 0 time offset
        sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
        sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamMinSizeRule::updateAsyncStatus()
    {
        TeamMinSizeRuleStatus &teamMinSizeRuleStatus = mMatchmakingAsyncStatus->getTeamMinSizeRuleStatus();

        teamMinSizeRuleStatus.setTeamMinSizeAccepted(getAcceptableTeamMinSize());
    }

    FitScore TeamMinSizeRule::calcWeightedFitScoreInternal(bool isExactMatch, float fitPercent, uint16_t actualTeamMinSize) const
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
                curAcceptableBalanceRange.setRange(mDesiredTeamMinSize, *range, mAbsoluteTeamMinSize, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
                if (actualTeamMinSize < (uint16_t)curAcceptableBalanceRange.mMinValue)
                {
                    return FIT_SCORE_NO_MATCH;
                }
            }
        }

        // success
        return static_cast<FitScore>(fitPercent * weight);
    }

    typedef eastl::pair<TeamIndex, uint16_t> TeamIndexSize;
    struct TeamIndexSizeSortComparitor
    {
        // Sort from smallest to largest:
        bool operator()(const TeamIndexSize &a, const TeamIndexSize &b) const
        {
            return (a.second < b.second);
        }
    };
    typedef eastl::vector<TeamIndexSize> TeamIndexSizeList;

    FitScore TeamMinSizeRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        if (isDisabled())
        {
            return 0;
        }
        
        const TeamIdSizeList* teamIdSizeList = getCriteria().getTeamIdSizeListFromRule();
        if (teamIdSizeList == nullptr)
        {
            return 0;
        }
        const GameManager::PlayerRoster& gameRoster = *(gameSession.getPlayerRoster());

        // Find the smallest team (id/index)
        TeamId minTeamId = 0;
        TeamIndex minTeamIndex = 0;
        uint16_t minTeamSize = getDefinition().getGlobalMaxTotalPlayerSlotsInGame();
        uint16_t teamCount = gameSession.getTeamCount();

        // For each team in the game: (Smallest to largest)
        TeamIndexSizeList inGameTeams;
        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            inGameTeams.push_back(eastl::make_pair(teamIndex, gameRoster.getTeamSize(teamIndex)));
        }
        eastl::sort(inGameTeams.begin(), inGameTeams.end(), TeamIndexSizeSortComparitor());


        TeamIdSet teamsAssigned;
        TeamIdSizeList::const_iterator curGameTeam = inGameTeams.begin();
        TeamIdSizeList::const_iterator endGameTeam = inGameTeams.end();
        for (; curGameTeam != endGameTeam; ++curGameTeam)
        {
            TeamIndex teamIndex = curGameTeam->first;
            TeamId teamId = gameSession.getTeamIdByIndex(teamIndex);
            uint16_t curTeamSize = curGameTeam->second;

            // Find the largest joining team that could join you:
            
            TeamIdSizeList::const_iterator curMyTeam = teamIdSizeList->begin();
            TeamIdSizeList::const_iterator endMyTeam = teamIdSizeList->end();
            for (; curMyTeam != endMyTeam; ++curMyTeam)
            {
                TeamId myTeamId = curMyTeam->first;
                uint16_t myJoiningPlayerCount = curMyTeam->second;

                if (teamsAssigned.find(myTeamId) != teamsAssigned.end())
                    continue;

                // Quickly check if my specified team exists in the game: 
                bool foundMyTeam = false;
                for (TeamIndex foundTeamIndex = 0; foundTeamIndex < teamCount; ++foundTeamIndex)
                {
                    TeamId foundTeamId = gameSession.getTeamIdByIndex(foundTeamIndex);
                    if (foundTeamId == myTeamId)
                    {
                        foundMyTeam = true;
                        break;
                    }
                }

                if ( teamId == myTeamId || myTeamId == ANY_TEAM_ID || 
                    (teamId == ANY_TEAM_ID && (!foundMyTeam || gameSession.getGameSettings().getAllowSameTeamId())) )
                {
                    teamsAssigned.insert(myTeamId);

                    // Increment the team size (assumes everyone could join that team):
                    curTeamSize += myJoiningPlayerCount;
                    break;
                }
            }


            if (curTeamSize < minTeamSize)
            {
                minTeamSize = curTeamSize;
                minTeamId = teamId;
                minTeamIndex = teamIndex;
            }
        }

        debugRuleConditions.writeRuleCondition("Smallest team Id %u(index:%u) of size %u, minimum size requirement of %u", minTeamId, minTeamIndex, minTeamSize, getAcceptableTeamMinSize());

        float fitPercent = getDefinition().getFitPercent(minTeamSize, gameSession.getTeamCapacity());
        bool isExactMatch = (minTeamSize == gameSession.getTeamCapacity());

        return calcWeightedFitScoreInternal(isExactMatch, fitPercent, minTeamSize);
    }

    Rule* TeamMinSizeRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamMinSizeRule(*this, matchmakingAsyncStatus);
    }

    // RETE implementations
    bool TeamMinSizeRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        const TeamMinSizeRuleDefinition &myDefn = getDefinition();
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        // Test smallest team (assume we'll join them)
        Rete::OrConditionList& capacityOrConditions = conditions.push_back();

        uint16_t minCount = 0; 
        if (getAcceptableTeamMinSize() > mSmallestTeamJoiningPlayerCount)
            minCount = getAcceptableTeamMinSize() - mSmallestTeamJoiningPlayerCount;
        uint16_t maxCount = getCriteria().getTeamCapacity();
        for (uint16_t curCap = minCount; curCap <= maxCount; curCap++)
        {
            // Always 0 fit in RETE, we rely on the post RETE evaluation for our fit score.
            FitScore zeroFit = 0;
            capacityOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(TeamMinSizeRuleDefinition::MIN_TEAM_SIZE_IN_GAME_KEY, myDefn.getWMEUInt16AttributeValue(curCap), mRuleDefinition),
                zeroFit, this
               ));
        }
        return true;
    }
    // end RETE
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
