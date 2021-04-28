/*! ************************************************************************************************/
/*!
    \file teamchoicerule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamchoicerule.h"
#include "gamemanager/matchmaker/rules/teaminfohelper.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamChoiceRule::TeamChoiceRule(const TeamChoiceRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(definition, matchmakingAsyncStatus),
          mTeamCount(0),
          mDuplicateTeamsAllowed(false),
          mUnspecifiedTeamCount(0)
    {
    }

    TeamChoiceRule::TeamChoiceRule(const TeamChoiceRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(other, matchmakingAsyncStatus),
          mTeamIdJoinSizes(other.mTeamIdJoinSizes),
          mTeamCount(other.mTeamCount),
          mDuplicateTeamsAllowed(other.mDuplicateTeamsAllowed),
          mUnspecifiedTeamCount(other.mUnspecifiedTeamCount)
    {
          other.mTeamIds.copyInto(mTeamIds);
    }


    TeamChoiceRule::~TeamChoiceRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamChoiceRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamChoiceRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for servers.
            return true;
        }

        eastl::string buf;

        mTeamCount = getCriteria().calcTeamCountFromRules(criteriaData, matchmakingSupplementalData);

        // This builds a list of team ids with sizes, sorted from largest to smallest (useful to sort this way when picking teams)
        buildSortedTeamIdSizeList(matchmakingSupplementalData.mTeamIdRoleSpaceRequirements, mTeamIdJoinSizes);
        if (matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.find(INVALID_TEAM_ID) != matchmakingSupplementalData.mTeamIdRoleSpaceRequirements.end())
        {
            err.setErrMessage("TeamChoiceRule's team id is invalid. You cannot join an invalid team id.");
            return false;
        }

        if (!matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            // Add a 0-player entry for the default team id, since this is a GB request:
            if (mTeamIdJoinSizes.empty())
            {
                TeamIdSize teamSize(matchmakingSupplementalData.mPlayerJoinData.getDefaultTeamId(), (uint16_t)0);
                mTeamIdJoinSizes.push_back(teamSize);
            }
        }


        if (isDisabled())
        {
            // Rule is disabled
            return true;
        }
        else
        {
            // Fill in the team vector with ANY_TEAM_ID:
            bool isDefaultTeamCount = false;
            uint16_t teamCount = getCriteria().calcTeamCountFromRules(criteriaData, matchmakingSupplementalData, &isDefaultTeamCount);
            teamCount = teamCount ? teamCount : 1;

            // If the team Ids were empty, fill them in with the data from what was being joined, and the TeamCountRule's desire. 
            if (matchmakingSupplementalData.mTeamIds.empty())
            {
                if (teamCount < mTeamIdJoinSizes.size())
                {
                    err.setErrMessage("TeamChoiceRule's team count is somehow less than the number of teams joining. Unable to fill out mTeamIds list.");
                    return false;
                }

                mUnspecifiedTeamCount = teamCount - mTeamIdJoinSizes.size();

                for (TeamIdSizeList::const_iterator teamSizes = mTeamIdJoinSizes.begin(), teamSizesEnd = mTeamIdJoinSizes.end(); teamSizes != teamSizesEnd; ++teamSizes)
                {
                    TeamId teamId = teamSizes->first;
                    if (teamId == ANY_TEAM_ID)
                    {
                        ++mUnspecifiedTeamCount;
                    }
                    else
                    {
                        mTeamIds.push_back(teamId); 
                    }
                }

                for (int i=0; i<mUnspecifiedTeamCount; ++i)
                {
                    mTeamIds.push_back(ANY_TEAM_ID); 
                }
            }
            else
            {
                // This error would occur if the team composition rule or team count rule specified a different size:
                if (!isDefaultTeamCount && matchmakingSupplementalData.mTeamIds.size() != (size_t)teamCount)
                {
                    err.setErrMessage(buf.sprintf("TeamChoiceRule's team ids list is different sizes from the size set by the current team rules (%d != %d).", matchmakingSupplementalData.mTeamIds.size(), teamCount).c_str());
                    return false;
                }
    
                eastl::set<TeamId> duplicateTeamMap;
                matchmakingSupplementalData.mTeamIds.copyInto(mTeamIds); 
                
                for (TeamIdVector::const_iterator myTeamIter = mTeamIds.begin(), endIter = mTeamIds.end(); myTeamIter != endIter; ++myTeamIter)
                {
                    TeamId teamId = *myTeamIter;
                    if (teamId == ANY_TEAM_ID)
                    {
                        ++mUnspecifiedTeamCount;
                    }
                    if (teamId == INVALID_TEAM_ID)
                    {
                        err.setErrMessage("TeamChoiceRule's team ids list contains an INVALID_TEAM_ID.  You cannot require an invalid team id.");
                        return false;
                    }
    
                    if (!mDuplicateTeamsAllowed && teamId != ANY_TEAM_ID)
                    {
                        if (duplicateTeamMap.insert(teamId).second == false)
                        {
                            err.setErrMessage(buf.sprintf("TeamChoiceRule does not support multiple teams with same teamId when create game settings don't allow it. Team '%u' is duplicated in capacity vector.", teamId).c_str());
                            return false;
                        }
                    }
                }


                // For every team we're joining, check if there is a team in-game that we can join:
                for (TeamIdSizeList::const_iterator teamSizes = mTeamIdJoinSizes.begin(), teamSizesEnd = mTeamIdJoinSizes.end(); teamSizes != teamSizesEnd; ++teamSizes)
                {
                    TeamId joinTeamId = teamSizes->first;

                    bool joinTeamPresent = false;
                    for (TeamIdVector::const_iterator gameTeamIter = mTeamIds.begin(), endIter = mTeamIds.end(); gameTeamIter != endIter; ++gameTeamIter)
                    {
                        // (if we explicitly set a team to join, that team must be in the game)
                        if (*gameTeamIter == joinTeamId || joinTeamId == ANY_TEAM_ID)
                        {
                            joinTeamPresent = true;
                        }
                    }

                    // check that myTeam had an entry in the team vector
                    if (!joinTeamPresent)
                    {
                        err.setErrMessage(buf.sprintf("TeamChoiceRule's team ids list doesn't contain an entry for joining teamId (%u)", joinTeamId).c_str());
                        return false;
                    }
                }
            }

            eastl::sort(mTeamIds.begin(), mTeamIds.end());
        }

        if (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            mDuplicateTeamsAllowed = matchmakingSupplementalData.mDuplicateTeamsAllowed;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    UpdateThresholdResult TeamChoiceRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // The rule never changes so we don't need to decay.
        mNextUpdate = UINT32_MAX;
        return NO_RULE_CHANGE;
    }


    /*! ************************************************************************************************/
    /*! \brief evaluate a matchmaking session's desired game COUNT against your desired value.  
        if single group match,  one group only wants to play against another group which meets the game COUNT rule.
        Returns NO_MATCH  if the session is not a match (otherwise it returns a weighted fit score).

        Note: we test desired vs desired, not desired vs actual pool COUNT

        \param[in]  session The other matchmaking session to evaluate this rule against.
    *************************************************************************************************/
    void TeamChoiceRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const TeamChoiceRule* otherRule = static_cast<const TeamChoiceRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));
        bool otherRuleIsDisabled = otherRule == nullptr || otherRule->isDisabled();

        // if both sessions have disabled teams, they match
        if (isDisabled() && otherRuleIsDisabled)
        {
            // rule is disabled, let 'em match
            sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
            sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);
            return;
        }

        if (!otherRuleIsDisabled && getDuplicateTeamsAllowed() != otherRule->getDuplicateTeamsAllowed())
        {
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("I %s duplicate teams, they %s duplicate teams", getDuplicateTeamsAllowed()?"allow":"disallow", otherRule->getDuplicateTeamsAllowed()?"allow":"disallow");
            
            return;
        }

        // Only do the team checks if we both specify teams:
        if (!isDisabled() && !otherRuleIsDisabled)
        {
            // We check that every team we're joining is part of the team ids list after this check. This check is just to make sure that we can join all of the teams.
            TeamIdSizeList::const_iterator teamSizes = mTeamIdJoinSizes.begin();
            TeamIdSizeList::const_iterator teamSizesEnd = mTeamIdJoinSizes.end();
            for (; teamSizes != teamSizesEnd; ++teamSizes)
            {
                TeamId joinTeamId = teamSizes->first;
                uint16_t joiningPlayerCount = teamSizes->second;

                if (joinTeamId == ANY_TEAM_ID)
                    continue;

                // for each joining team, check if the combined number of players on the teams will exceed the limit, 
                TeamIdSizeList::const_iterator otherTeamSizes = otherRule->mTeamIdJoinSizes.begin();
                TeamIdSizeList::const_iterator otherTeamSizesEnd = otherRule->mTeamIdJoinSizes.end();
                for (; otherTeamSizes != otherTeamSizesEnd; ++otherTeamSizes)
                {
                    if (otherTeamSizes->first == joinTeamId)
                    {
                        uint16_t otherJoiningPlayerCount =  otherTeamSizes->second;
                        uint16_t teamCapacity = getCriteria().getTeamCapacity();
                        if (!getDuplicateTeamsAllowed() && (joiningPlayerCount + otherJoiningPlayerCount) > teamCapacity)
                        {     
                            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("%u players > %u capacity on team %u", (joiningPlayerCount + otherJoiningPlayerCount), teamCapacity, joinTeamId);
                            return;
                        }
                        break;
                    }
                }
            }
        }

        // Check that team ids lists are compatible:
        TeamIdVector mismatchTeams = otherRule->getTeamIds();
        TeamIdVector::const_iterator myTeamIter = mTeamIds.begin(); 
        TeamIdVector::const_iterator endIter = mTeamIds.end(); 
        for (; myTeamIter != endIter; ++myTeamIter)
        {
            TeamIdVector::iterator mismatchIter = mismatchTeams.begin(); 
            TeamIdVector::iterator mismatchEnd = mismatchTeams.end(); 
            for (; mismatchIter != mismatchEnd; ++mismatchIter)
            {
                // Since the lists are sorted, the ANY team will appear at the end. So we check for a match, or an 'ANY' if we hit the end. 
                if (*myTeamIter == *mismatchIter || 
                    *myTeamIter == ANY_TEAM_ID   || *mismatchIter == ANY_TEAM_ID)
                {
                    mismatchTeams.erase(mismatchIter);
                    break;
                }
            }
        }

        if (!mismatchTeams.empty())
        {
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Team id lists have a %u team mismatch", (uint32_t)mismatchTeams.size());
                    
            return;
        }

        // 0,0 means matches at 0 fitscore, with 0 time offset
        sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
        sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);

        // sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("My team %u, other team %u", mTeamId, otherRuleIsDisabled ? 0 : otherRule->mTeamId);
        

        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Duplicate teams %s", getDuplicateTeamsAllowed()?"allowed":"disallowed");
        
        return;
    }

    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamChoiceRule::updateAsyncStatus()
    {
    }

    FitScore TeamChoiceRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        if (mDuplicateTeamsAllowed != gameSession.getGameSettings().getAllowSameTeamId())
        {
            return FIT_SCORE_NO_MATCH;
        }

        // Check if there is room for the current joining players on the desired team:
        uint16_t teamCapacity = gameSession.getTeamCapacity();
        uint16_t teamCount = gameSession.getTeamCount();

        // Check to make sure that we can join all of the teams.
        TeamIndexSet indexesToSkip;
        TeamIdSizeList::const_iterator teamSizes = mTeamIdJoinSizes.begin();
        TeamIdSizeList::const_iterator teamSizesEnd = mTeamIdJoinSizes.end();
        for (; teamSizes != teamSizesEnd; ++teamSizes)
        {
            uint16_t maxFreeSpace = 0;
            TeamId joinTeamId = teamSizes->first;
            uint16_t joiningPlayerCount = teamSizes->second;

            // add a WME for each existing team in the game.
            TeamIndex foundTeamIndex = 0;
            for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                if (indexesToSkip.find(teamIndex) != indexesToSkip.end())
                    continue;

                uint16_t teamSize = gameSession.getPlayerRoster()->getTeamSize(teamIndex);
                uint16_t teamFreeSpace = teamCapacity - teamSize;

                // guard to prevent excess WMEs from being added to the RETE tree if teamSize is ever inadvertently greater than capacity
                if ( EA_UNLIKELY(teamCapacity < teamSize))
                {
                    teamFreeSpace = 0;
                }

                uint16_t curTeamId = gameSession.getTeamIdByIndex(teamIndex);

                // Max free space if we don't care about our team id:
                if ((joinTeamId == ANY_TEAM_ID) ||
                    (curTeamId == ANY_TEAM_ID || curTeamId == UNSET_TEAM_ID || curTeamId == joinTeamId))
                {
                    maxFreeSpace = eastl::max(maxFreeSpace, teamFreeSpace);
                    foundTeamIndex = teamIndex;
                }
            }

            if (maxFreeSpace < joiningPlayerCount) 
                return FIT_SCORE_NO_MATCH;

            indexesToSkip.insert(foundTeamIndex);
        }

        return 0;
    }

    Rule* TeamChoiceRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamChoiceRule(*this, matchmakingAsyncStatus);
    }

    // RETE implementations
    bool TeamChoiceRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        const TeamChoiceRuleDefinition &myDefn = getDefinition();
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        uint16_t totalJoiningPlayerCount = 0;
        TeamIdSizeList::const_iterator teamSizes = mTeamIdJoinSizes.begin();
        TeamIdSizeList::const_iterator teamSizesEnd = mTeamIdJoinSizes.end();
        for (; teamSizes != teamSizesEnd; ++teamSizes)
        {
            TeamId joinTeamId = teamSizes->first;
            uint16_t joiningPlayerCount = teamSizes->second;
            totalJoiningPlayerCount += joiningPlayerCount;

            // My team choice:
            if (joinTeamId != ANY_TEAM_ID)
            {
                // My Team Choice:
                Rete::OrConditionList& sizeOrConditions = conditions.push_back();
                char8_t wmeNameBuf[32];
                blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TeamChoiceRuleDefinition::TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, joinTeamId);
                // 0 fit score (match/no-match), only the actual size affects the fit score.
                sizeOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(wmeNameBuf, joiningPlayerCount, myDefn.getMaxGameSize(), mRuleDefinition),
                    0, this 
                   ));

                // Unset Team Choice: (The ANY_TEAM_ID team with the largest # of free slots, this will be 0 if all teams have ids)
                blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TeamChoiceRuleDefinition::TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, UNSET_TEAM_ID);
                // 0 fit score (match/no-match), only the actual size affects the fit score.
                sizeOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(wmeNameBuf, joiningPlayerCount, myDefn.getMaxGameSize(), mRuleDefinition),
                    0, this
                   ));
            }
            else if (joiningPlayerCount != 0)
            {
                // Any Team Choice: (The team with the largest # of free slots)
                Rete::OrConditionList& sizeOrConditions = conditions.push_back();
                char8_t wmeNameBuf[32];
                blaze_snzprintf(wmeNameBuf, sizeof(wmeNameBuf), TeamChoiceRuleDefinition::TEAMCHOICERULE_TEAM_OPEN_SLOTS_KEY, ANY_TEAM_ID);
                // 0 fit score (match/no-match), only the actual size affects the fit score.
                sizeOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(wmeNameBuf, joiningPlayerCount, myDefn.getMaxGameSize(), mRuleDefinition),
                    0, this
                   ));
            }
        }

        // Joining player count is only 0 in the case where the GameBrowser is being used, and duplicate team data is unset. 
        if (totalJoiningPlayerCount != 0)
        {
            // Duplicate Factions
            Rete::OrConditionList& duplicateOrConditions = conditions.push_back();
            // 0 fit score (match/no-match), only the actual size affects the fit score.
            duplicateOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(TeamChoiceRuleDefinition::TEAMCHOICERULE_DUPLICATE_RETE_KEY, myDefn.getWMEBooleanAttributeValue(mDuplicateTeamsAllowed), mRuleDefinition),
                0, this
               ));
        }

        return true;
    }
    // end RETE

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics
    ****************************************************************************************************/
    void TeamChoiceRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        RuleDiagnosticSetupInfo setupInfo("desiredTeamIds");
        getDiagnosticDisplayString(setupInfo.mValueTag, mTeamIdJoinSizes);
        setupInfos.push_back(setupInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t TeamChoiceRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (isMatchAny())
        {
            return helper.getOpenToMatchmakingCount();
        }
        return helper.getTeamIdsGameCount(mTeamIdJoinSizes);
    }

    void TeamChoiceRule::getDiagnosticDisplayString(eastl::string& desiredValuesTag, const TeamIdSizeList& desiredTeamIds) const
    {
        for (auto itr = desiredTeamIds.begin(), end = desiredTeamIds.end(); itr != end; ++itr)
        {
            desiredValuesTag.append_sprintf("%u%s", itr->first, (itr != desiredTeamIds.begin() ? ", " : ""));
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
