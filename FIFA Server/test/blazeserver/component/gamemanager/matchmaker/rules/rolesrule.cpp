/*! ************************************************************************************************/
/*!
    \file   rolesrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/rolesrule.h"
#include "gamemanager/matchmaker/rules/rolesruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    RolesRule::RolesRule(const RolesRuleDefinition &definition, MatchmakingAsyncStatus* status)
        : Rule(definition, status),
        mTeamCount(0),
        mHasMultiRoleMembers(false)
    {
    }

    RolesRule::RolesRule(const RolesRule &other, MatchmakingAsyncStatus* status) 
        : Rule(other, status),
        mTeamIdRoleSpaceRequirements(other.mTeamIdRoleSpaceRequirements),
        mTeamIdPlayerRoleRequirements(other.mTeamIdPlayerRoleRequirements),
        mDesiredRoleSpaceRequirements(other.mDesiredRoleSpaceRequirements),
        mTeamCount(other.mTeamCount),
        mHasMultiRoleMembers(other.mHasMultiRoleMembers)
    {
    }

    Rule* RolesRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW RolesRule(*this, matchmakingAsyncStatus);
    }

    bool RolesRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for resettable servers.
            return true;
        }
        
        mTeamIdRoleSpaceRequirements = matchmakingSupplementalData.mTeamIdRoleSpaceRequirements;
        mTeamIdPlayerRoleRequirements = matchmakingSupplementalData.mTeamIdPlayerRoleRequirements;

        mTeamCount = getCriteria().calcTeamCountFromRules(criteriaData, matchmakingSupplementalData);

        if (!matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            if (mTeamIdRoleSpaceRequirements.empty())
            {
                // Add an empty entry using the default team index: (May need to update buildTeamIdRoleSizeMap to do this by default...)
                mTeamIdRoleSpaceRequirements.insert(matchmakingSupplementalData.mPlayerJoinData.getDefaultTeamId());
            }
            if (mTeamIdPlayerRoleRequirements.empty())
            {
                mTeamIdPlayerRoleRequirements.insert(matchmakingSupplementalData.mPlayerJoinData.getDefaultTeamId());
            }
        }

        // Fill out the mTeamIdPlayerRoleRequirements
        eastl::map<TeamId, RolesSizeMap> tempTeamRoleCounts;
        for (const auto& teamRoleItr : mTeamIdPlayerRoleRequirements)
        {
            const TeamId teamId = teamRoleItr.first;
            auto tempTeamItr = tempTeamRoleCounts.insert(teamId).first;

            for (const auto& playerRoleItr : teamRoleItr.second)
            {
                if (playerRoleItr.second.size() == 1)
                {
                    ++tempTeamItr->second[playerRoleItr.second]; // If we desire only a single role, we need to ensure that matching sessions have
                                                                 // room in that role
                }
                else
                {
                    mHasMultiRoleMembers = true;
                    tempTeamItr->second[playerRoleItr.second] = 1; // If we have more than one desired role, we aren't concerned with the count
                                                                   // Capacity validation will be done post-RETE
                }
            }
        }

        // Find the largest count for each role combination across all teams. This will be the value we set on the role condition
        for (const auto& tempTeamItr : tempTeamRoleCounts)
        {
            for (const auto& tempRolesItr : tempTeamItr.second)
            {
                RolesSizeMap::iterator desiredRoleIter = mDesiredRoleSpaceRequirements.find(tempRolesItr.first);
                if (desiredRoleIter == mDesiredRoleSpaceRequirements.end())
                {
                    mDesiredRoleSpaceRequirements[tempRolesItr.first] = tempRolesItr.second;
                }
                else
                {
                    desiredRoleIter->second = eastl::max(desiredRoleIter->second, tempRolesItr.second);
                }
            }
        }

        return true;
    }

    FitScore RolesRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        // this rule does a post-RETE eval to ensure the desired roles are available on the correct team without violating multi-role criteria
        FitScore rolesMatchingFitScore = 0;
         
        // TODO: Order by the largest teams first:
        TeamIndexSet teamsToSkip;
        TeamIdRoleSizeMap::const_iterator teamSizeIter = mTeamIdRoleSpaceRequirements.begin();
        TeamIdRoleSizeMap::const_iterator teamSizeEnd = mTeamIdRoleSpaceRequirements.end();
        for (; teamSizeIter != teamSizeEnd; ++teamSizeIter)
        {
            bool success = false;
            TeamId joiningTeamId = teamSizeIter->first;
            const RoleSizeMap& roleSpaceRequirements = teamSizeIter->second;

            const TeamIdVector &teamIds = gameSession.getTeamIds();
            bool foundDesiredTeam = false;
            bool foundJoinableUnsetTeam = false;
            TeamIndex joinableUnsetTeamIndex = 0;
            bool acceptAnyTeam = (joiningTeamId == ANY_TEAM_ID);
            bool gameAllowsDuplicateTeams = gameSession.getGameSettings().getAllowSameTeamId();
            for (TeamIndex i = 0; i < gameSession.getTeamCount(); ++i)
            {
                // Skip team indexes that we have already chosen for the other joining team(s).
                if (teamsToSkip.find(i) != teamsToSkip.end())
                    continue;

                foundDesiredTeam = (teamIds[i] == joiningTeamId);
                if ( foundDesiredTeam || acceptAnyTeam || ((teamIds[i] == ANY_TEAM_ID) && !foundJoinableUnsetTeam) )
                {
                    // only check joinability if this team is an exact match for us, or it's unset and we haven't already found a joinableUnsetTeam
                    if ( gameSession.checkJoinabilityByTeamIndex(roleSpaceRequirements, i) == ERR_OK )
                    {
                        // We passed joinability for all single role players, now check for players that desire multiple roles
                        // All the players that we validated above need to be included to correctly validate role capacity
                        TeamIndexRoleSizeMap roleCounts;
                        for (const auto& roleSize : roleSpaceRequirements)
                        {
                            if (blaze_stricmp(roleSize.first.c_str(), PLAYER_ROLE_NAME_ANY) != 0)
                            {
                                auto teamItr = roleCounts.insert(i).first;
                                teamItr->second.insert(eastl::make_pair(roleSize.first, roleSize.second));
                            }
                        }

                        TeamIdPlayerRoleNameMap::const_iterator roleReqItr = mTeamIdPlayerRoleRequirements.find(joiningTeamId);
                        if (gameSession.findOpenRole(roleReqItr->second, i, roleCounts) != ERR_OK)
                        {
                            TRACE_LOG("[RolesRule].evaluateForMMFindGame rule '" << getRuleName() << "' failed to find space for joining roles on TeamIndex ("
                                << i << "), TeamId(" << teamIds[i] << ") in game(" << gameSession.getGameId() << ").");
                            continue;
                        }

                        // we can stop searching if this was the team we requested, we'll take anything, or this is an unset team and the game allows duplicates
                        if (foundDesiredTeam || acceptAnyTeam || gameAllowsDuplicateTeams)
                        {
                            TRACE_LOG("[RolesRule].evaluateForMMFindGame rule '" << getRuleName() << "' found space for joining roles on TeamIndex ("
                                << i << "), TeamId(" << teamIds[i] << ") in game(" << gameSession.getGameId() << ").");

                            debugRuleConditions.writeRuleCondition("Found space for joining players roles on teamId %u(index:%u).", teamIds[i], i);

                            teamsToSkip.insert(i);
                            success = true;
                            break;
                        }
                     
                        foundJoinableUnsetTeam = true;
                        joinableUnsetTeamIndex = i;
                    }

                    if (!acceptAnyTeam && foundDesiredTeam && !gameAllowsDuplicateTeams)
                    {
                        // game doesn't allow duplicate teams, and we found the explicitly defined team ID we're searching for
                        // set foundUnsetTeam to prevent a join attempt that will fail
                        foundJoinableUnsetTeam = false;
                        joinableUnsetTeamIndex = i;
                        break;
                    }
                }
            }

            if (foundJoinableUnsetTeam)
            {
                 // found an unset team, and taking the slot won't violate a duplicate team prohibition for this game
                 TRACE_LOG("[RolesRule].evaluateForMMFindGame rule '" << getRuleName() << "' found space for joining roles on an unset team "
                     << "in game(" << gameSession.getGameId() << ").");
                 debugRuleConditions.writeRuleCondition("Found space for joining players roles on an unset teamIndex %u", joinableUnsetTeamIndex);
 
                 teamsToSkip.insert(joinableUnsetTeamIndex);
                 success = true;
            }


            if (!success)
            {
                debugRuleConditions.writeRuleCondition("No space for joining roles on team %u", joiningTeamId);
                return FIT_SCORE_NO_MATCH;
            }
        }

         return rolesMatchingFitScore;
     }

    void RolesRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
 
        if (EA_UNLIKELY(getMatchmakingSession() == nullptr))
        {
            return;
        }

        sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
        sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);

        const RoleInformation& myRoleInformation = getMatchmakingSession()->getRoleInformation();
        const RoleInformation& otherRoleInformation = otherSession.getRoleInformation();

        // Validate that the RoleInformation for both sessions matches (same Role capacities, same Multi-Role criteria).
        // If these aren't identical, NO_MATCH. Role entry criteria is allowed to differ.
        if (myRoleInformation.getMultiRoleCriteria().size() != otherRoleInformation.getMultiRoleCriteria().size())
        {
            // bad start, we've got differing Multi-Role criteria.
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Number of role criteria %" PRIsize " != %" PRIsize".", myRoleInformation.getMultiRoleCriteria().size(), otherRoleInformation.getMultiRoleCriteria().size());
            return;
        }

        EntryCriteriaMap::const_iterator myMultiRoleCriteriaIter = myRoleInformation.getMultiRoleCriteria().begin();
        EntryCriteriaMap::const_iterator myMultiRoleCriteriaEnd = myRoleInformation.getMultiRoleCriteria().end(); 
        EntryCriteriaMap::const_iterator otherMultiRoleCriteriaIter = otherRoleInformation.getMultiRoleCriteria().begin(); 
        EntryCriteriaMap::const_iterator otherMultiRoleCriteriaEnd = otherRoleInformation.getMultiRoleCriteria().end(); 
        for(; (myMultiRoleCriteriaIter != myMultiRoleCriteriaEnd) && (otherMultiRoleCriteriaIter != otherMultiRoleCriteriaEnd); ++myMultiRoleCriteriaIter, ++otherMultiRoleCriteriaIter)
        {
            if (blaze_stricmp(myMultiRoleCriteriaIter->first.c_str(), otherMultiRoleCriteriaIter->first.c_str()) != 0)
            {
                // bad start, we've got differing Multi-Role criteria.
                sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Role criteria name '%s' != '%s'", myMultiRoleCriteriaIter->first.c_str(), otherMultiRoleCriteriaIter->first.c_str());
                return;
            }

            if (blaze_stricmp(myMultiRoleCriteriaIter->second.c_str(), otherMultiRoleCriteriaIter->second.c_str()) != 0)
            {
                // bad start, we've got differing Multi-Role criteria.
                sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Role criteria '%s' != '%s'", myMultiRoleCriteriaIter->second.c_str(), otherMultiRoleCriteriaIter->second.c_str());
                return;
            }
        }
        
        // this is used to bypass role capacity checks if there's only one role required.
        bool isSingleRoleSession = (myRoleInformation.getRoleCriteriaMap().size() == 1);
        //first do the quick checks, make sure the same number of roles are defined
        if (myRoleInformation.getRoleCriteriaMap().size() != otherRoleInformation.getRoleCriteriaMap().size())
        {
            // bad start, we've got differing RoleCriteria sizes.
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Number of Roles %" PRIsize " != %" PRIsize".", myRoleInformation.getRoleCriteriaMap().size(), otherRoleInformation.getRoleCriteriaMap().size());
            
            return;
        }

        // now walk the role criteria maps in lockstep, ensure the roles defined AND the capacities match. 
        // Role entry criteria can mismatch, that's actually evaluated to ensure that both sessions meet each other's criteria.
        RoleCriteriaMap::const_iterator myRoleCriteriaIter = myRoleInformation.getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator myRoleCriteriaEnd = myRoleInformation.getRoleCriteriaMap().end(); 
        RoleCriteriaMap::const_iterator otherRoleCriteriaIter = otherRoleInformation.getRoleCriteriaMap().begin(); 
        RoleCriteriaMap::const_iterator otherRoleCriteriaEnd = otherRoleInformation.getRoleCriteriaMap().end(); 
        for(; (myRoleCriteriaIter != myRoleCriteriaEnd) && (otherRoleCriteriaIter != otherRoleCriteriaEnd); ++myRoleCriteriaIter, ++otherRoleCriteriaIter)
        {
            if (blaze_stricmp(myRoleCriteriaIter->first.c_str(), otherRoleCriteriaIter->first.c_str()) != 0)
            {
                // missing a role
                sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other doesn't have role %s", myRoleCriteriaIter->first.c_str());
                
                return;
            }

            
            if(!isSingleRoleSession)
            {
                // now check capacity, if we have more than one role
                // this rule requires an exact match for each role capacity if the sessions specify more than one role each
                // if there's only one role specified, we'll rely on the game capacity rules to ensure there's a capacity match
                if (myRoleCriteriaIter->second->getRoleCapacity() != otherRoleCriteriaIter->second->getRoleCapacity())
                {
                    // mismatched role capacity
                    sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                    sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                    sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Role capcity %u != %u", myRoleCriteriaIter->second->getRoleCapacity(), otherRoleCriteriaIter->second->getRoleCapacity());
                    
                    return;
                }
            }
        }

        const RolesRule* otherRule = static_cast<const RolesRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));
        // prevent crash if for some reason other rule is not defined
        if (otherRule == nullptr)
        {
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other not using Roles");
            return;
        }
        // Validate that given the team choice & role rosters of the MM sessions won't violate role capacities or multi-role criteria.
        // If role criteria will be violated, NO_MATCH.
        // Sessions are checked during init to ensure they don't violate their own role criteria.
        // We check as though both sessions will join the same team if the required team ids are the same (and not ANY_TEAM_ID), duplicate teams are disallowed OR we have fewer than 2 teams
        TeamIdRoleSizeMap::const_iterator teamSizeIter = mTeamIdRoleSpaceRequirements.begin();
        TeamIdRoleSizeMap::const_iterator teamSizeEnd = mTeamIdRoleSpaceRequirements.end();
        for (; teamSizeIter != teamSizeEnd; ++teamSizeIter)
        {
            TeamId joiningTeamId = teamSizeIter->first;
            const RoleSizeMap& roleSpaceRequirements = teamSizeIter->second;

            TeamIdRoleSizeMap::const_iterator otherTeamSizeIter = otherRule->mTeamIdRoleSpaceRequirements.find(joiningTeamId);
            bool requestingSameTeam = (joiningTeamId != ANY_TEAM_ID) && (otherTeamSizeIter != otherRule->mTeamIdRoleSpaceRequirements.end());
            if ( (requestingSameTeam && !getMatchmakingSession()->getCriteria().getDuplicateTeamsAllowed()) || (mTeamCount < 2))
            {
                // If we didn't find our team on the other rule, then we can only get here when team count == 1. (So just use the first/only team in the list).
                if (otherTeamSizeIter == otherRule->mTeamIdRoleSpaceRequirements.end())
                    otherTeamSizeIter = otherRule->mTeamIdRoleSpaceRequirements.begin();

                if (otherTeamSizeIter == otherRule->mTeamIdRoleSpaceRequirements.end())
                {
                    sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other not using Teams/Roles");
                    return;
                }

                // may join the same team
                RoleSizeMap totalRoleSizeMap;
                totalRoleSizeMap.assign(roleSpaceRequirements.begin(), roleSpaceRequirements.end());
                // copy over other role space requirements, since they're always more restrictive than mine (by player count).
                const RoleCriteriaMap &otherRoleCriteriaMap = otherRoleInformation.getRoleCriteriaMap();

                RoleSizeMap::const_iterator otherRoleSizeIter = otherTeamSizeIter->second.begin();
                RoleSizeMap::const_iterator otherRoleSizeEnd = otherTeamSizeIter->second.end();
                for(; otherRoleSizeIter != otherRoleSizeEnd; ++otherRoleSizeIter)
                {
                    uint16_t totalRoleSize;
                    const RoleName &roleName = otherRoleSizeIter->first;
                    RoleSizeMap::iterator totalRoleIter = totalRoleSizeMap.find(roleName);
                    if (totalRoleIter == totalRoleSizeMap.end())
                    {
                        totalRoleSizeMap[roleName] = otherRoleSizeIter->second;
                        totalRoleSize = otherRoleSizeIter->second;
                    }
                    else
                    {
                        totalRoleIter->second += otherRoleSizeIter->second;
                        totalRoleSize = totalRoleIter->second;
                    }
                    
                    uint16_t otherRoleCapacity = 0;
                    RoleCriteriaMap::const_iterator roleCriteriaIter =  otherRoleCriteriaMap.find(roleName);
                    if ( roleCriteriaIter != otherRoleCriteriaMap.end())
                    {
                        otherRoleCapacity = roleCriteriaIter->second->getRoleCapacity();
                    }
                    else if (blaze_stricmp(roleName, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
                    {
                        otherRoleCapacity = getMatchmakingSession()->getCriteria().getTeamCapacity();
                    }

                    if (totalRoleSize > otherRoleCapacity)
                    {
                        // exceeded individual role capacity, bail
                        sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                        sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Exceeded capacity for role %s, %u > %u", roleName.c_str(), totalRoleSize, otherRoleCapacity);
                    
                        return;
                    }
  
                }
            }
            else // we're joining different teams, make sure I don't exceed the other's role capacity with my group
            {
                const RoleCriteriaMap &otherRoleCriteriaMap = otherRoleInformation.getRoleCriteriaMap();

                RoleSizeMap::const_iterator myRoleSizeIter = roleSpaceRequirements.begin();
                RoleSizeMap::const_iterator myRoleSizeEnd = roleSpaceRequirements.end();
                for (; myRoleSizeIter != myRoleSizeEnd; ++myRoleSizeIter)
                {      
                    uint16_t myRoleSize = myRoleSizeIter->second;
                    const RoleName &roleName = myRoleSizeIter->first;

                    uint16_t otherRoleCapacity = 0;
                    RoleCriteriaMap::const_iterator roleCriteriaIter =  otherRoleCriteriaMap.find(roleName);
                    if ( roleCriteriaIter != otherRoleCriteriaMap.end())
                    {
                        otherRoleCapacity = roleCriteriaIter->second->getRoleCapacity();
                    }
                    else if (blaze_stricmp(roleName, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
                    {
                        otherRoleCapacity = getMatchmakingSession()->getCriteria().getTeamCapacity();
                    }

                    if (myRoleSize > otherRoleCapacity)
                    {
                        // exceeded individual role capacity, bail
                        sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                        sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Exceeded capacity for role %s, %u > %u", roleName.c_str(), myRoleSize, otherRoleCapacity);
                    
                        return;
                    }
                
                }
            }
        }

        // sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other joining team %u vs my team %u.", otherRule->mJoiningTeamId, mJoiningTeamId);
        
        
        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Number of Roles %" PRIsize " == %" PRIsize".", myRoleInformation.getRoleCriteriaMap().size(), otherRoleInformation.getRoleCriteriaMap().size());
        
        
        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Number of role criteria %" PRIsize " == %" PRIsize".", myRoleInformation.getMultiRoleCriteria().size(), otherRoleInformation.getMultiRoleCriteria().size());
        
        
        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Role capacities match.");
        
        // all criteria & capacities validated
    }

     UpdateThresholdResult RolesRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
     {
         // this rule doesn't decay
         mNextUpdate = UINT32_MAX;
         return NO_RULE_CHANGE;
     }


     bool RolesRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
     {
         Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
         // Get the size of each team
         uint16_t globalMaxSlots = static_cast<const RolesRuleDefinition&>(mRuleDefinition).getGlobalMaxSlots();
         uint16_t teamSize = ((mTeamCount == 0) ? globalMaxSlots : (globalMaxSlots / mTeamCount));

         for (const auto& roleSizeItr : mDesiredRoleSpaceRequirements)
         {
             const PlayerRoleList& roleNames = roleSizeItr.first;
             Rete::OrConditionList& roleOrConditions = conditions.push_back();
             for (const auto& role : roleNames)
             {
                 // skip the any role and default role, no filtering can be done
                 if (role.empty() || blaze_stricmp(role.c_str(), GameManager::PLAYER_ROLE_NAME_ANY) == 0)
                     continue;

                 // Create a wme name with our role name
                 char8_t roleWmeNameBuf[64];
                 blaze_snzprintf(roleWmeNameBuf, sizeof(roleWmeNameBuf), "%s_%s", RolesRuleDefinition::ROLES_RULE_ROLE_ATTRIBUTE_NAME, role.c_str());

                 // range rule - looking for needed size space for role to # of slots on team
                 // 0 fit score (match/no-match).
                 roleOrConditions.push_back(Rete::ConditionInfo(
                     Rete::Condition(roleWmeNameBuf, roleSizeItr.second, teamSize, mRuleDefinition),
                     0, this
                 ));
             }
             if (roleOrConditions.empty())
                 conditions.pop_back();
         }
         return true;
     }

     /*! ************************************************************************************************/
     /*! \brief Implemented to enable per rule criteria value break down diagnostics
     ****************************************************************************************************/
     void RolesRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
     {
         RuleDiagnosticSetupInfo setupInfo("desiredRoles");
         getDiagnosticDisplayString(setupInfo.mValueTag, mDesiredRoleSpaceRequirements);
         setupInfos.push_back(setupInfo);
     }

     /*! ************************************************************************************************/
     /*! \brief override since default can't handle count via RETE
     ***************************************************************************************************/
     uint64_t RolesRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
     {
         if (isMatchAny())
         {
             return helper.getOpenToMatchmakingCount();
         }
         return helper.getRolesGameCount(mDesiredRoleSpaceRequirements);
     }

     void RolesRule::getDiagnosticDisplayString(eastl::string& desiredValuesTag, const RolesSizeMap& desiredRoles) const
     {
         eastl::set<RoleName> roles; // Track processed roles to skip duplicates
         for (const auto& itr : desiredRoles)
         {
             for (const auto& role : itr.first)
             {
                 if (roles.find(role) == roles.end())
                 {
                     desiredValuesTag.append_sprintf("%s, ", role.c_str());
                     roles.insert(role);
                 }
             }
         }
         if (!desiredValuesTag.empty())
             desiredValuesTag.erase(desiredValuesTag.size() - 2); // Remove the extra ", "
     }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
