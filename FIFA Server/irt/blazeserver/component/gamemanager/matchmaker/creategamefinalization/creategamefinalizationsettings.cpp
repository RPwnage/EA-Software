/*! ************************************************************************************************/
/*!
    \file   creategamefinalizationsettings.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"

#include "gamemanager/matchmaker/rules/expandedpingsiterule.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "framework/util/random.h"
#include "gamemanager/gamesession.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    CreateGameFinalizationSettings::CreateGameFinalizationSettings(const MatchmakingSession& finalizingSession)
        :
        mAutoJoinMemberInfo(&(finalizingSession.getHostMemberInfo())),
        mTeamIndexByMatchmakingSession(BlazeStlAllocator("mTeamIndexByMatchmakingSession", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mFinalizingMatchmakingSessionId(finalizingSession.getMMSessionId()),
        mTeamValidator(finalizingSession),
        mPlayerCapacity(finalizingSession.getCriteria().getParticipantCapacity()), 
        mPlayerSlotsRemaining(finalizingSession.getCriteria().getParticipantCapacity()),
        mTeamsTriedVectorMap(BlazeStlAllocator("mTeamsTriedVectorMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mDebugSessionMap(BlazeStlAllocator("mDebugSessionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        mMultiRoleEntryCriteriaEvaluator.setStaticRoleInfoSource(mRoleInformation);
        mMatchingSessionList.reserve( mPlayerCapacity ); // worst case, 1 player per MM session

        finalizingSession.getGameCreationData().getRoleInformation().copyInto(mRoleInformation);
        int32_t errorCount = mMultiRoleEntryCriteriaEvaluator.createCriteriaExpressions();
        if (errorCount > 0) 
        {
            // Since we already validated these rules for the matchmaking or create game request, they shouldn't be invalid now.
            ERR_LOG("[CreateGameFinalizationSettings].CreateGameFinalizationSettings found errors with role criteria.");
        }

        if (finalizingSession.getCriteria().getExpandedPingSiteRule() != nullptr)
        {
            size_t acceptedPingSiteCount = finalizingSession.getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap().size();
            mPlayerCountsByAcceptedPingSiteMap.reserve(acceptedPingSiteCount);
        }
    }

    bool CreateGameFinalizationSettings::canFinalizeWithSession(const MatchmakingSession *finalizingSession, const MatchmakingSession *owningSession, CreateGameFinalizationTeamInfo*& teamInfo)
    {
        // prevent pulling in of any locked down sessions
        if (owningSession->isLockedDown())
        {
            TRACE_LOG("[MatchmakingSession].canFinalizeWithSession:  finalizing Session(" << finalizingSession->getMMSessionId() 
                << ") skipped Session("<< owningSession->getMMSessionId() << ") for being locked down.");
            return false;
        }

        if ( (((!owningSession->isDebugCheckOnly() || !finalizingSession->isDebugCheckOnly()) || (!owningSession->isDebugCheckMatchSelf() || !finalizingSession->isDebugCheckMatchSelf()))) 
            && containsUserGroup(owningSession->getUserGroupId()))
        {
            TRACE_LOG("[MatchmakingSession].canFinalizeWithSession:  finalizing Session(" << finalizingSession->getMMSessionId() 
                << ") skipped Session("<< owningSession->getMMSessionId() << ") for group already present in match set.");
            return false;
        }

        if (containsScenario(owningSession->getMMScenarioId()))
        {
            // already have a MM session with this scenario, don't take another
            TRACE_LOG("[MatchmakingSession].canFinalizeWithSession: finalizing Session(" << finalizingSession->getMMSessionId() 
                << ") skipped Session("<< owningSession->getMMSessionId() << ") for scenario already present in match set.");
            return false;
        }

        if (!isSessionConnectable(owningSession, finalizingSession))
        {
            TRACE_LOG("[MatchmakingSession].canFinalizeWithSession:  finalizing Session(" << finalizingSession->getMMSessionId()  
                << ") skipped Session " << owningSession->getMMSessionId() << ") for not connectable.");
            return false;
        }

        // moving this to before selectTeams because selectTeams, if successful is a one-way trip.
        // If there's a 'best' ping site, we verify the owningSession we're about to add accepts it.
        // If not, we skip adding this session during finalization, if so, we continue on to select a team/role.
        if (finalizingSession->getCriteria().isExpandedPingSiteRuleEnabled())
        {
            PingSiteAlias pingSite;
            if (hasBestPingSite(pingSite))
            {
                if (owningSession->getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap().find(pingSite) ==
                    owningSession->getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap().end())
                {
                    TRACE_LOG("[MatchmakingSession].canFinalizeWithSession: finalizing Session(" << finalizingSession->getMMSessionId()
                        << ") skipped Session(" << owningSession->getMMSessionId() << ") because ping site " << pingSite << " is not acceptable.");
                    return false;
                }
            }
        }

        // if anything rejects owning session afterwards, we need to go back and clean up the team counts & ued values.
        // so we just return it's return value so it's not moved higher in the method without addressing the reversibility of the method.
        // select team logs why it fails, if it does
        return owningSession->selectTeams(*this, teamInfo);
    }

    void CreateGameFinalizationSettings::addDebugInfoForSessionInGameSessionList(const MatchmakingSessionMatchNode& node)
    {
        DebugSessionResult& debugResults = mDebugSessionMap[node.sMatchedToSession->getMMSessionId()];
        node.sRuleResultMap->copyInto(debugResults.getRuleResults());
        debugResults.setTimeToMatch(TimeValue::getTimeOfDay());
        debugResults.setOverallFitScore(node.sFitScore);
        debugResults.setMatchedSessionId(node.sMatchedToSession->getMMSessionId());
        debugResults.setOwnerPlayerName(node.sMatchedToSession->getOwnerPersonaName());
        debugResults.setOwnerBlazeId(node.sMatchedToSession->getOwnerBlazeId());
        debugResults.setPlayerCount(node.sMatchedToSession->getPlayerCount());
    }


    /*! ************************************************************************************************/
    /*! \brief Helper: add the supplied matching node to the finalizationSettings as a session that must explicitly
            join the game.  Asserts if the session is too large to fit

        \param[in] node - the match node, information about the session matched including debug results & fit score.
        \param[in,out] teamInfo - the team breakdown.
    ***************************************************************************************************/
    void CreateGameFinalizationSettings::addSessionToGameSessionList(const MatchmakingSessionMatchNode& node, CreateGameFinalizationTeamInfo* teamInfo)
    {
        addSessionToGameSessionList(*(node.sMatchedToSession), teamInfo);
        addDebugInfoForSessionInGameSessionList(node);
    }


    /*! ************************************************************************************************/
    /*! \brief Helper: add the supplied session to the finalizationSettings as a session that must explicitly
            join the game.  Asserts if the session is too large to fit

        \param[in] matchingSession the matching matchmaking session.  (could be sub session)
        \param[in,out] teamInfo - the team breakdown.
    ***************************************************************************************************/
    void CreateGameFinalizationSettings::addSessionToGameSessionList(const MatchmakingSession &matchingSession, CreateGameFinalizationTeamInfo* teamInfo)
    {
        EA_ASSERT( mPlayerSlotsRemaining >= matchingSession.getPlayerCount() );

        mMatchingSessionList.push_back(&matchingSession);
        mUserGroupIdSet.insert(matchingSession.getUserGroupId());

        if (matchingSession.getMMScenarioId() != INVALID_SCENARIO_ID)
        {
            // we only track the scenario ids for scenarios in use. 
            mScenarioIdSet.insert(matchingSession.getMMScenarioId());
        }
        else
        {
            ERR_LOG("[addSessionToGameSessionList] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        uint16_t sessionPlayerCount = matchingSession.getPlayerCount();
        mPlayerSlotsRemaining -= sessionPlayerCount;

        if (isTeamsEnabled())
        {
            if (teamInfo == nullptr)
            {
                ERR_LOG("[CreateGameFinalizationSettings].addSessionToGameSessionList Adding matchmaking session '" 
                    << matchingSession.getMMSessionId() << "' with nullptr team info and enabled teams");
                return;
            }

            UserExtendedDataValue sessionTeamUed = matchingSession.getCriteria().getTeamUEDContributionValue();
            TRACE_LOG("[CreateGameFinalizationSettings].addSessionToGameSessionList Session(" << mFinalizingMatchmakingSessionId << ") - pick number " << (mMatchingSessionList.size() - 1) << ", adding session " 
                << matchingSession.getMMSessionId() << " (Owner: " << matchingSession.getOwnerPersonaName() << ", playerId: " 
                << matchingSession.getOwnerBlazeId() << ", sessionSize: " << (int32_t)matchingSession.getPlayerCount() << ", NAT type: " 
                << (Util::NatTypeToString(matchingSession.getNetworkQosData().getNatType())) << ", sessionTeamUED: " << sessionTeamUed << "), to join. Team id '"
                << teamInfo->mTeamId << "' index '" << teamInfo->mTeamIndex << "' size '" << teamInfo->mTeamSize << "' capacity '" 
                << teamInfo->mTeamCapacity << "' teamUED '" << teamInfo->mUedValue << "'. " << mPlayerSlotsRemaining << " total player slots remaining.");
        }
        else
        {
            TRACE_LOG("[CreateGameFinalizationSettings]" << " Session(" << mFinalizingMatchmakingSessionId << ") - adding session " << matchingSession.getMMSessionId() << " (Owner: " 
                << matchingSession.getOwnerPersonaName() << ", playerId: " << matchingSession.getOwnerBlazeId() 
                << ", sessionSize: " << (int32_t)matchingSession.getPlayerCount() << ", NAT type: " 
                << (Util::NatTypeToString(matchingSession.getNetworkQosData().getNatType())) << ") to the game. " << mPlayerSlotsRemaining 
                << " player slots remaining.");
        }

        updateFinalizedNatStatus(matchingSession);
    }


    /*! ************************************************************************************************/
    /*! \brief return the total number of players in the sessions
        that have been chosen for finalization.  Includes group members.
    ***************************************************************************************************/
    uint16_t CreateGameFinalizationSettings::calcFinalizingPlayerCount() const
    {
        uint16_t playerCount = 0;

        MatchmakingSessionList::const_iterator sessionIter = mMatchingSessionList.begin();
        MatchmakingSessionList::const_iterator end = mMatchingSessionList.end();
        for ( ; sessionIter != end; ++sessionIter )
        {
            const MatchmakingSession *session = *sessionIter;
            playerCount = playerCount + session->getPlayerCount();
        }

        return playerCount;
    }


    CreateGameFinalizationTeamInfo* CreateGameFinalizationSettings::getSmallestAvailableTeam(TeamId teamId, eastl::set<CreateGameFinalizationTeamInfo*>& teamsToSkip)
    {
        CreateGameFinalizationTeamInfo* smallestTeam = nullptr;

        size_t teamCapacitiesVectorSize = mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            CreateGameFinalizationTeamInfo& teamInfo = mCreateGameFinalizationTeamInfoVector[i];

            if (teamsToSkip.find(&teamInfo) != teamsToSkip.end())
                continue;

            // Pick the smallest team for the team Id we have specified
            if (((teamId == ANY_TEAM_ID) || (teamInfo.mTeamId == teamId)) &&
                ((smallestTeam == nullptr) || (teamInfo.mTeamSize <= smallestTeam->mTeamSize)))
            {
                // if size is tied, use other balance criteria, else random, to break the tie.
                if ((smallestTeam != nullptr) && (teamInfo.mTeamSize == smallestTeam->mTeamSize))
                {
                    bool tieBreakChoosesFirst = ((smallestTeam->mUedValue == teamInfo.mUedValue)? (Random::getRandomNumber(2) == 0) //MM_AUDIT: potentialy pick a more eff way to 'randomly' choose, e.g. pseudo random
                        : (smallestTeam->mUedValue < teamInfo.mUedValue));
                    if (tieBreakChoosesFirst)
                        continue;
                }

                smallestTeam = &teamInfo;
            }
        }

        return smallestTeam;
    }


    TeamIndex CreateGameFinalizationSettings::getTeamIndexForMMSession(const MatchmakingSession& session, TeamId teamId) const
    {
        TeamMatchmakingSessions::const_iterator itr = mTeamIndexByMatchmakingSession.find(session.getMMSessionId());

        if (itr != mTeamIndexByMatchmakingSession.end())
        {
            TeamIdToTeamIndexMap::const_iterator teamIndex = itr->second.find(teamId);
            if (teamIndex != itr->second.end())
            {
                return teamIndex->second;
            }
        }

        return UNSPECIFIED_TEAM_INDEX;
    }


    void CreateGameFinalizationSettings::claimTeamIndexForMMSession(const MatchmakingSession& session, TeamId teamId, TeamIndex index)
    {
        mTeamIndexByMatchmakingSession[session.getMMSessionId()][teamId] = index;
    }


    /*! ************************************************************************************************/
    /*! \brief Once the teams have been populated by buildSessionList and mCreateGameFinalizationTeamInfoVector
        is populated, this method validates that our selections meet the finalizing sessions team requirements.
    *************************************************************************************************/
    bool CreateGameFinalizationSettings::areTeamSizesSufficientToCreateGame(const MatchmakingCriteria& criteria, uint16_t& teamSizeDifference, TeamIndex& largestTeam) const
    {
        // These are already the clamping values for the team size rules' acceptable ranges. 
        uint16_t maxTeamCapacity = criteria.getTeamCapacity();
        uint16_t minTeamCapacity = criteria.getAcceptableTeamMinSize();
        uint16_t teamRequirementsSize = criteria.getTeamCountFromRule();

        uint16_t maxTeamSize = 0;
        uint16_t minTeamSize = UINT16_MAX;

        const CreateGameFinalizationTeamInfoVector& finalTeamRequirementsVector = mCreateGameFinalizationTeamInfoVector;
        uint16_t finalTeamRequirementsSize = (uint16_t)finalTeamRequirementsVector.size();
        if (EA_UNLIKELY(teamRequirementsSize != finalTeamRequirementsSize))
        {
            TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final number of teams '" 
                      << teamRequirementsSize << "' doesn't match initial number of teams '" << finalTeamRequirementsSize << "'");
            return false;
        }

        // NOTE; that this assumes all teams have already been accounted for in the team requirements vector.
        // It should not be possible to get here without that being true.
        for (uint16_t j = 0; j < finalTeamRequirementsSize; j++)
        {
            const CreateGameFinalizationTeamInfo& finalTeamRequirements = finalTeamRequirementsVector[j];

            if (j != finalTeamRequirements.mTeamIndex)
            {
                ERR_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final team index mismatch for team '" 
                        << finalTeamRequirements.mTeamId << "', expected '" << j << "' but found '" << finalTeamRequirements.mTeamIndex << "'");
                return false;
            }

            // All team requirements have same max/min
            // The teams capcity.
            if ((finalTeamRequirements.mTeamCapacity > maxTeamCapacity) ||
                (finalTeamRequirements.mTeamCapacity < minTeamCapacity))
            {
                TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final player capacity '" 
                          << finalTeamRequirements.mTeamCapacity << "' out of bounds [" << minTeamCapacity 
                          << "," << maxTeamCapacity << "] for team Id '" << finalTeamRequirements.mTeamId 
                          << "', index '" << finalTeamRequirements.mTeamIndex << "'");
                return false;
            }

            // Check the teams size.
            if ((finalTeamRequirements.mTeamSize > maxTeamCapacity) ||
                (finalTeamRequirements.mTeamSize < minTeamCapacity))
            {
                TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final team size '" 
                          << finalTeamRequirements.mTeamSize << "' out of bounds [" << minTeamCapacity << "," 
                          << maxTeamCapacity << "] for teamId '" << finalTeamRequirements.mTeamId 
                          << "', index '" << finalTeamRequirements.mTeamIndex << "'");
                return false;
            }

            // Check the size vs the acceptable range.
            if ((finalTeamRequirements.mTeamSize > maxTeamCapacity) ||
                (finalTeamRequirements.mTeamSize < minTeamCapacity))
            {
                TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final team size '" 
                          << finalTeamRequirements.mTeamSize << "' not in current range [" 
                          << minTeamCapacity << "," 
                          << maxTeamCapacity << "] for team Id '" 
                          << finalTeamRequirements.mTeamId << "', index '" << finalTeamRequirements.mTeamIndex << "'");
                return false;
            }

            if (finalTeamRequirements.mTeamSize > finalTeamRequirements.mTeamCapacity)
            {
                TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame final team size '" 
                          << finalTeamRequirements.mTeamSize << "' greater than final team capacity '" << finalTeamRequirements.mTeamCapacity 
                          << "' for team Id '" << finalTeamRequirements.mTeamId << "', index '" << finalTeamRequirements.mTeamIndex << "'");
                return false;
            }

            maxTeamSize = eastl::max(finalTeamRequirements.mTeamSize, maxTeamSize);
            minTeamSize = eastl::min(finalTeamRequirements.mTeamSize, minTeamSize);

            // Track largest team, last one wins.
            if (maxTeamSize == finalTeamRequirements.mTeamSize)
            {
                largestTeam = finalTeamRequirements.mTeamIndex;
            }

            TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizesSufficientToCreateGame MATCH for team Id '" 
                      << finalTeamRequirements.mTeamId << "' index '" << finalTeamRequirements.mTeamIndex << "', size '" 
                      << finalTeamRequirements.mTeamSize << "' capacity '" << finalTeamRequirements.mTeamCapacity << "' in current range [" 
                      << minTeamCapacity << "," << maxTeamCapacity << "]");
        }

        // check that the team size difference is ok
        teamSizeDifference = maxTeamSize - minTeamSize;
        if (teamSizeDifference > criteria.getAcceptableTeamBalance())
        {
            TRACE_LOG("[CreateGameFinalizationSettings].areTeamSizeSufficientToCreateGame team sizes are too unbalanced required max size difference <= '" 
                      << criteria.getAcceptableTeamBalance() << "', but the current size difference is '" << teamSizeDifference << "'");
            return false;
        }

        return true;
    }

    
    bool CreateGameFinalizationSettings::areTeamsAcceptableToCreateGame(const MatchmakingCriteria& criteria, uint16_t& teamSizeDiff, UserExtendedDataValue& teamUedDiff) const
    {
        teamSizeDiff = 0;//for metrics
        teamUedDiff = 0;
        TeamIndex teamInd;
        const bool teamUedAcceptable = (!criteria.hasTeamUEDBalanceRuleEnabled() || criteria.getTeamUEDBalanceRule()->areFinalizingTeamsAcceptableToCreateGame(*this, teamUedDiff));
        return (areTeamSizesSufficientToCreateGame(criteria, teamSizeDiff, teamInd) && teamUedAcceptable &&
            (!criteria.hasTeamCompositionRuleEnabled() || criteria.getTeamCompositionRule()->areFinalizingTeamsAcceptableToCreateGame(*this)) &&
            (!criteria.hasTeamUEDPositionParityRuleEnabled() || criteria.getTeamUEDPositionParityRule()->areFinalizingTeamsAcceptableToCreateGame(*this)));
    }


    const char8_t* CreateGameFinalizationSettings::pickSequenceToLogStr(eastl::string& buf) const
    {
        eastl::string logBuf;
        buf.sprintf("finalization pick sequence: ");
        for (MatchmakingSessionList::const_iterator itr = mMatchingSessionList.begin(); itr != mMatchingSessionList.end(); ++itr)
        {
            const TeamIdSizeList* teamIdSizeList = (*itr)->getCriteria().getTeamIdSizeListFromRule();
            if (!teamIdSizeList)
                continue;

            TeamIdSizeList::const_iterator curTeamId = teamIdSizeList->begin();
            TeamIdSizeList::const_iterator endTeamId = teamIdSizeList->end();
            for (; curTeamId != endTeamId; ++curTeamId)
            {
                TeamId teamid = curTeamId->first;
                buf.append_sprintf("{MmSess%s,TeamId=%u,TeamIndex=%u},", TeamUEDBalanceRule::makeSessionLogStr(*itr, logBuf), teamid, getTeamIndexForMMSession(*(*itr), teamid));
            }
        }
        return buf.c_str();
    }


    /*! ************************************************************************************************/
    /*! \brief 'reset' all my sessions 'tried' flags to 'false' for all picks above the specified number.
    *************************************************************************************************/
    void CreateGameFinalizationSettings::clearAllSessionsTriedAfterIthPick(uint16_t ithPickNumber)
    {
        // for teams and buckets, clear *at* the current pick number
        const uint16_t ithMinusOne = ((ithPickNumber > 0)? ithPickNumber - 1 : 0);

        for (PickNumberToTeamsTriedVectorMap::iterator itr = mTeamsTriedVectorMap.begin(); itr != mTeamsTriedVectorMap.end(); ++itr)
        {
            if (itr->first > ithMinusOne)
                itr->second.clear();
        }

        for (CreateGameFinalizationTeamInfoVector::iterator itr = mCreateGameFinalizationTeamInfoVector.begin();
            itr != mCreateGameFinalizationTeamInfoVector.end(); ++itr)
        {
            // for sessions clear above the current pick number
            itr->clearSessionsTried(ithPickNumber);
            itr->clearBucketsTried(ithMinusOne);
        }
    }


    bool CreateGameFinalizationSettings::hasUntriedTeamsForCurrentPick(uint16_t pickNumber) const
    {
        if (!isTeamsEnabled())
            return false;

        PickNumberToTeamsTriedVectorMap::const_iterator itr = mTeamsTriedVectorMap.find(pickNumber);
        if ((itr == mTeamsTriedVectorMap.end()) || (itr->second.empty()))
        {
            //haven't tried the team at all
            return true;
        }
        const TeamsTriedVector& teamsTried = itr->second;
        return (eastl::find(teamsTried.begin(), teamsTried.end(), false) != teamsTried.end());
    }


    bool CreateGameFinalizationSettings::wasTeamTriedForCurrentPick(TeamIndex teamIndex, uint16_t pickNumber) const
    {
        PickNumberToTeamsTriedVectorMap::const_iterator itr = mTeamsTriedVectorMap.find(pickNumber);
        if (itr == mTeamsTriedVectorMap.end())
        {
            return false;
        }
        const TeamsTriedVector& teamsTried = itr->second;
        if (teamsTried.size() <= teamIndex)
        {
            // side: for efficiency vector may have been simply cleared when reseting to retry session pick sequence, don't ERR if empty here
            if (!teamsTried.empty())
            {
                ERR_LOG("[CreateGameFinalizationSettings].wasTeamTriedForCurrentPick: specified team index " << teamIndex << " was out of bounds of initialized team vector, for pick number " << pickNumber << ".");
            }
            return false;
        }
        return teamsTried[teamIndex];
    }


    void CreateGameFinalizationSettings::markTeamTriedForCurrentPick(TeamIndex teamIndex, uint16_t pickNumber)
    {
        TeamsTriedVector& teamsTried = mTeamsTriedVectorMap[pickNumber];
        // create the vector as needed
        if (teamsTried.empty())
        {
            teamsTried.reserve(mCreateGameFinalizationTeamInfoVector.size());
            teamsTried.insert(teamsTried.begin(), mCreateGameFinalizationTeamInfoVector.size(), false);
        }
        if (teamsTried.size() <= teamIndex)
        {
            ERR_LOG("[CreateGameFinalizationSettings].markTeamTriedForCurrentPick: specified team index " << teamIndex << " was out of bounds of initialized team vector, for pick number " << pickNumber << ". Enlarging vector.");
            teamsTried.insert(teamsTried.end(), (teamIndex - teamsTried.size() + 1), false);
        }
        teamsTried[teamIndex] = true;
    }


    /*! ************************************************************************************************/
    /*! \brief removes sessions from my finalizing list for all picks at and above the specified number.
    *************************************************************************************************/ 
    void CreateGameFinalizationSettings::removeSessionsFromTeamsFromIthPickOn(uint16_t ithPickNumber,
        const MatchmakingSession& pullingSession, const MatchmakingSession& hostSession)
    {
        uint16_t i = 0;
        MatchmakingSessionList sessionsToRemove;
        for (MatchmakingSessionList::const_iterator itr = mMatchingSessionList.begin(); itr != mMatchingSessionList.end(); ++itr)
        {
            if (i++ < ithPickNumber)
                continue;

            MatchmakingSessionId nextMmSessId = (*itr)->getMMSessionId();
            if ((nextMmSessId == pullingSession.getMMSessionId()) ||
                (nextMmSessId == hostSession.getMMSessionId()))
            {
                ERR_LOG("[CreateGameFinalizationSettings].removeSessionsFromTeamsFromIthPickOn: can't roll back the " << 
                    ((nextMmSessId == pullingSession.getMMSessionId())? "pulling" : "host") << " MM session(" << nextMmSessId << "). Not rolling this '" << (i - 1) << "'th pick back.");
                continue;
            }

            sessionsToRemove.push_back((*itr));
        }

        for (MatchmakingSessionList::const_iterator itr = sessionsToRemove.begin(); itr != sessionsToRemove.end(); ++itr)
        {
            removeSessionFromTeams((*itr));
            mUserGroupIdSet.erase((*itr)->getUserGroupId());
            mScenarioIdSet.erase((*itr)->getMMScenarioId());
        }

        // reset the Nat status based on who is left.
        mNatStatus.hasModerateNat = false;
        mNatStatus.hasStrictNat = false;
        for (MatchmakingSessionList::const_iterator itr = mMatchingSessionList.begin(); itr != mMatchingSessionList.end(); ++itr)
        {
            updateFinalizedNatStatus(**itr);
        }
    }

    CreateGameFinalizationTeamInfo* CreateGameFinalizationSettings::getTeamInfoByTeamIndex(TeamIndex teamIndex)
    { 
        size_t teamCapacitiesVectorSize = mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            CreateGameFinalizationTeamInfo& teamInfo = mCreateGameFinalizationTeamInfoVector[i];

            if (teamInfo.mTeamIndex == teamIndex)
                return &teamInfo;
        }
        return nullptr;
    }


    bool CreateGameFinalizationSettings::removeSessionFromTeams(const MatchmakingSession* session)
    {
        // Remove the user from the finalization list.
        // This is not efficient.  Issues with moving away from a vector, most of finalization depends on it
        // and is common code with other instances of the vector
        uint16_t iterIndex = 0;
        bool foundSession = false;
        MatchmakingSessionList::iterator iter = mMatchingSessionList.begin();
        MatchmakingSessionList::iterator end = mMatchingSessionList.end();
        for (; iter!=end; ++iter, ++iterIndex)
        {
            if ((*iter)->getMMSessionId() == session->getMMSessionId())
            {
                mUserGroupIdSet.erase((*iter)->getUserGroupId());
                mScenarioIdSet.erase((*iter)->getMMScenarioId());
                mMatchingSessionList.erase(iter);
                foundSession = true;
                break;
            }
        }

        // if for some unknown reason we couldn't find the session in the matched list, bail.
        if (!foundSession)
        {
            TRACE_LOG("[CreateGameFinalizationSettings].removeSessionFromTeam Session(" << session->getMMSessionId() << ") was not found in finalization matched list, failing finalizaiton.");
            return false;
        }

        TeamMatchmakingSessions::iterator teamIter = mTeamIndexByMatchmakingSession.find(session->getMMSessionId());
        if (teamIter != mTeamIndexByMatchmakingSession.end())
        {
            TeamIdToTeamIndexMap::iterator curTeamIdToIndex = teamIter->second.begin();
            TeamIdToTeamIndexMap::iterator endTeamIdToIndex = teamIter->second.end();
            for (; curTeamIdToIndex != endTeamIdToIndex; ++curTeamIdToIndex)
            {
                TeamId teamId = curTeamIdToIndex->first;
                TeamIndex teamIndex = curTeamIdToIndex->second;

                CreateGameFinalizationTeamInfo* teamInfoPtr = getTeamInfoByTeamIndex(teamIndex);
                if (teamInfoPtr == nullptr)
                    continue;
                CreateGameFinalizationTeamInfo& teamInfo = *teamInfoPtr;

                const TeamUEDBalanceRule* teamUEDRule = session->getCriteria().getTeamUEDBalanceRule();
                if ((teamUEDRule != nullptr) && !teamUEDRule->isDisabled())
                {
                    if (!teamUEDRule->getDefinition().recalcTeamUEDValue(teamInfo.mUedValue, teamInfo.mTeamSize,
                        teamUEDRule->getUEDValue(), teamUEDRule->getPlayerCount(), true, teamInfo.mMatchmakingSessionUedValues, teamInfo.mTeamIndex))
                    {
                        ERR_LOG("[CreateGameFinalizationSettings].removeSessionFromTeam failed to update ued value for team index " << teamInfo.mTeamIndex << ", for leaving session " << session->getMMSessionId());
                    }
                }

                // remove the session's UED values from the ranking
                const TeamUEDPositionParityRule* teamUEDPositionParityRule = session->getCriteria().getTeamUEDPositionParityRule();
                if ((teamUEDPositionParityRule != nullptr) && !teamUEDPositionParityRule->isDisabled())
                {
                    // remove this matchmaking session's member UED values from the team
                    if (!teamUEDPositionParityRule->removeSessionFromTeam(teamInfo.mMemberUedValues))
                    {
                        ERR_LOG("[CreateGameFinalizationSettings].removeSessionFromTeam failed to update ued positions for team index " << teamInfo.mTeamIndex << ", for leaving session " << session->getMMSessionId());
                    }
                }

                uint16_t playerCount = session->getTeamSize(teamId);

                // decrement by the session size.
                teamInfo.mTeamSize -= playerCount;

                TeamIdRoleSizeMap::const_iterator teamRoleIter = session->getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->find(teamId);

                // fix the role count
                if (teamRoleIter != session->getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->end())
                {
                    teamInfo.removeSessionFromTeam(*session, teamId);
                }

                // fix the composition count
                if (session->getCriteria().hasTeamCompositionRuleEnabled())
                {
                    teamInfo.clearTeamCompositionToFillGroupSizeFilled(session->getPlayerCount());
                }
            }

            mTeamIndexByMatchmakingSession.erase(teamIter);
            return true;
        }

        TRACE_LOG("[CreateGameFinalizationSettings].removeSessionFromTeam Unable to find session(" << session->getMMSessionId() << ") team index.");

        return false;
    }


    bool CreateGameFinalizationSettings::areAllFinalizingTeamSizesEqual() const
    {
        const CreateGameFinalizationTeamInfo* firstTeam = nullptr;
        size_t teamCapacitiesVectorSize = mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            const CreateGameFinalizationTeamInfo& teamInfo = mCreateGameFinalizationTeamInfoVector[i];
            if (firstTeam == nullptr)
            {
                firstTeam = &teamInfo;
            }
            else if (firstTeam->mTeamSize != teamInfo.mTeamSize)
                return false;
        }
        return true;
    }


    bool CreateGameFinalizationSettings::wasGameTeamCompositionsIdTried(GameTeamCompositionsId gameTeamCompositionsId) const
    {
        return (mGameTeamCompositionsIdTriedSet.find(gameTeamCompositionsId) != mGameTeamCompositionsIdTriedSet.end());
    }


    void CreateGameFinalizationSettings::markGameTeamCompositionsIdTried(GameTeamCompositionsId gameTeamCompositionsId)
    {
        if (!mGameTeamCompositionsIdTriedSet.insert(gameTeamCompositionsId).second)
        {
            WARN_LOG("[CreateGameFinalizationSettings].markGameTeamCompositionsTried attempted to mark as tried an already-marked game team composition id " << gameTeamCompositionsId);
        }
    }


    bool CreateGameFinalizationSettings::initTeamCompositionsFinalizationInfo(const GameTeamCompositionsInfo& gameTeamCompositionsInfo, const MatchmakingSession& pullingSession)
    {
        // pre-existing member should be just the pulling session, if using team compositions here.
        if (pullingSession.getCriteria().hasTeamCompositionRuleEnabled())
        {
            const int32_t expectedMemberCount = 1;
            if (getTotalSessionsCount() - expectedMemberCount >= 1)
            {
                ERR_LOG("[CreateGameFinalizationSettings].initTeamCompositionsFinalizationInfo internal error: unexpected excess number of membersalready assigned to teams before team compositions assigned to teams. Expected only " << expectedMemberCount << ", but there were " << getTotalSessionsCount() << ". Attempting to recover by removing " << (getTotalSessionsCount() - expectedMemberCount) << " excess members.");
                removeSessionsFromTeamsFromIthPickOn((uint16_t)expectedMemberCount, pullingSession, pullingSession);//recover
                mAutoJoinMemberInfo = &pullingSession.getHostMemberInfo();
            }
        }


        return mTeamCompositionsFinalizationInfo.initialize(*this, gameTeamCompositionsInfo, pullingSession, mCreateGameFinalizationTeamInfoVector);
    }


    uint16_t CreateGameFinalizationSettings::getOverallFirstRepickablePickNumber(const MatchmakingSession& pullingSession) const
    {
        // Pre: any auto join session that needs to be chosen up front, has already been assigned to mAutoJoinMemberInfo. Just in case guard
        if (mAutoJoinMemberInfo == nullptr)
        {
            WARN_LOG("[CreateGameFinalizationSettings].getOverallFirstRepickablePickNumber Assuming only pulling session(" << pullingSession.getMMSessionId() << ") is non-repickable, auto join session (unexpectedly) not yet initialized.");
            return 1;//assume pulling session goes in
        }
        return ((mAutoJoinMemberInfo->mMMSession.getMMSessionId() == pullingSession.getMMSessionId())?
            1 : 2);
    }


    /*! ************************************************************************************************/
    /*! \brief Metrics helper, gets max delta between top and bottom skilled player in the match
    *************************************************************************************************/ 
    void CreateGameFinalizationSettings::getMembersUedMaxDeltas(const MatchmakingCriteria& criteria,
        UserExtendedDataValue& gameMembersDelta, UserExtendedDataValue& teamMembersDelta) const
    {
        gameMembersDelta = 0;
        teamMembersDelta = 0;
        if (!criteria.hasTeamUEDBalanceRuleEnabled())
            return;

        const TeamIndex maxTeams = 4;//cap for metrics for efficiency.
        UserExtendedDataValue maxValByTeamInd[maxTeams] = {INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN};
        UserExtendedDataValue minValByTeamInd[maxTeams] = {INT64_MAX,INT64_MAX,INT64_MAX,INT64_MAX};

        UserExtendedDataValue maxVal = INT64_MIN;
        UserExtendedDataValue minVal = INT64_MAX;
        for (MatchmakingSessionList::const_iterator itr = getMatchingSessionList().begin(), end = getMatchingSessionList().end();
            itr != end; ++itr)
        {
            const TeamUEDBalanceRule* rule = (*itr)->getCriteria().getTeamUEDBalanceRule();
            if (EA_UNLIKELY((rule == nullptr) || rule->isDisabled()))
                continue;//side: unlikely since to get to finalization their rules should've been enabled also.

            // The team UED Balance rule only applies with a single team join:
            const TeamIdSizeList* teamIdSizeList = (*itr)->getCriteria().getTeamIdSizeListFromRule();
            if (teamIdSizeList == nullptr || teamIdSizeList->size() != 1)
                continue;

            const TeamIndex sessTeamInd = getTeamIndexForMMSession(**itr, teamIdSizeList->begin()->first);
            if (sessTeamInd >= maxTeams)
            {
                TRACE_LOG("[CreateGameFinalizationSettings].getMembersUedMaxDeltas note: for efficiency metrics will not account for team index " << sessTeamInd << " which is beyond max index " << maxTeams << ".");
                continue;
            }

            // update game max/min
            if (maxVal < rule->getMaxUserUEDValue())
                maxVal = rule->getMaxUserUEDValue();
            if (minVal > rule->getMinUserUEDValue())
                minVal = rule->getMinUserUEDValue();

            // update team max/min
            if (maxValByTeamInd[sessTeamInd] < rule->getMaxUserUEDValue())
                maxValByTeamInd[sessTeamInd] = rule->getMaxUserUEDValue();
            if (minValByTeamInd[sessTeamInd] > rule->getMinUserUEDValue())
                minValByTeamInd[sessTeamInd] = rule->getMinUserUEDValue();
            // avoid looping over teams again below, by calc'ing here. Pre: rule.mMax/MinUserUEDValue already init
            if (teamMembersDelta < (maxValByTeamInd[sessTeamInd] - minValByTeamInd[sessTeamInd]))
                teamMembersDelta = (maxValByTeamInd[sessTeamInd] - minValByTeamInd[sessTeamInd]);
        }

        if ((maxVal == INT64_MIN) || (minVal == INT64_MAX))
        {
            ERR_LOG("[CreateGameFinalizationSettings].getMembersUedMaxDeltas internal error: invalid or missing team ued rule for sessions in the match. Unable to retrieve the max/min user ued values for the match, assuming zero difference.");
            return;
        }
        gameMembersDelta = (maxVal - minVal);
    }


    /*! \brief return whether compositions exact match. false if rule disabled */ 
    bool CreateGameFinalizationSettings::areAllFinalizingTeamCompositionsEqual() const
    {
        return ((mTeamCompositionsFinalizationInfo.getCurrentGameTeamCompositionsInfo() == nullptr)? false :
            mTeamCompositionsFinalizationInfo.getCurrentGameTeamCompositionsInfo()->mIsExactMatch);
    }


    float CreateGameFinalizationSettings::getFinalizingTeamCompositionsFitPercent() const
    {
        return ((mTeamCompositionsFinalizationInfo.getCurrentGameTeamCompositionsInfo() == nullptr)? FIT_PERCENT_NO_MATCH :
            mTeamCompositionsFinalizationInfo.getCurrentGameTeamCompositionsInfo()->mCachedFitPercent);
    }


    bool CreateGameFinalizationSettings::areAllSessionsMakingReservations() const
    {
        bool areAllSessionsMakingReservation = true;
        for (MatchmakingSessionList::const_iterator itr = getMatchingSessionList().begin(), end = getMatchingSessionList().end(); itr != end; ++itr)
        {
            if ((*itr)->getGameEntryType() !=  GAME_ENTRY_TYPE_MAKE_RESERVATION)
            {
                areAllSessionsMakingReservation = false;
                break;
            }
        }
        return areAllSessionsMakingReservation;
    }


    /*! ************************************************************************************************/
    /*! \brief Uses helper class to track that the matched session's team ids
        are compatible with the finalizing session's team ids
        NOTE: called for every matched session.
    *************************************************************************************************/ 
    void CreateGameFinalizationSettings::trackTeamsWithMatchedSession(const MatchmakingSession& matchedSession)
    {
        // Add up the number of players requesting which teams from each matched session.
        mTeamValidator.tallyTeamInfo(matchedSession);
    }


    /*! ************************************************************************************************/
    /*! \brief returns true if the finalizing session potentially satisfies number of players needed
            to make minimum sized teams.  Also populates the teamInfo vector to be used in packing
            the game session during finalizaiton.

            Used to see if a createGame matchmaking session can be finalized; if the number of matched
            players is high enough to potentially create a game with.  Used as an optimization
            to weed out finalization attempts that just can't possibly work.
    
            To check here is potentially invalid as we have no idea how the teams will break down when
            we actually select who goes where.  Outside limitations like best host always selecting first,
            and owning session going first, group size sorting are just not accounted for here.
            We are only doing our best to determine we have met our minimum size requirements.

        \param[in] finalizingSession
        \return true if the playerCount is potentially sufficient to create a game
    ***************************************************************************************************/
    bool CreateGameFinalizationSettings::areTeamSizesSufficientToAttemptFinalize(const MatchmakingSession& finalizingSession) 
    {
        bool hasEnoughPlayers = true;
        const TeamIdVector *origTeamIds = finalizingSession.getCriteria().getTeamIds();

        // should never happen
        if (origTeamIds == nullptr)
        {
            return false;
        }

        size_t teamIdVectorSize = origTeamIds->size();
        uint16_t teamCapacity = finalizingSession.getCriteria().getTeamCapacity();
        for (size_t i = 0; i < teamIdVectorSize; ++i)
        {
            TeamId teamId = origTeamIds->at(i);
            TeamIndex teamIndex = i;

            // Initialize team infos for the finalization vector - to be used later when actually determining which matched sessions
            // get pulled into our game session.
            CreateGameFinalizationTeamInfo teamInfo(this, teamId, teamIndex, 
                teamCapacity, finalizingSession.getCriteria().getTeamUEDMinValueFromRule());
            mCreateGameFinalizationTeamInfoVector.push_back(teamInfo);

            // We need to loop through every team, but track if we've failed.
            bool currentTeamHasEnoughPlayers = mTeamValidator.hasEnoughPlayers(teamId, finalizingSession.getCriteria().getAcceptableTeamMinSize());
            // never set our global if one fails.  One fail equals all fail
            if (hasEnoughPlayers)
            {
                hasEnoughPlayers = currentTeamHasEnoughPlayers;
            }
        }

        return hasEnoughPlayers;
    }


    void CreateGameFinalizationSettings::updateFinalizedNatStatus( const MatchmakingSession& session )
    {
        if (session.getNetworkQosData().getNatType() >= Util::NAT_TYPE_STRICT_SEQUENTIAL)
        {
            mNatStatus.hasStrictNat = true;
        }
        else if (session.getNetworkQosData().getNatType() == Util::NAT_TYPE_MODERATE)
        {
            mNatStatus.hasModerateNat = true;
        }
    }


    bool CreateGameFinalizationSettings::isSessionConnectable( const MatchmakingSession* matchingSession, const MatchmakingSession* finalizingSession ) const
    {
        // Sanity
        if ( (matchingSession == nullptr) || (finalizingSession == nullptr) )
        {
            return false;
        }

        if (!matchingSession->getMatchmaker().getMatchmakingConfig().getServerConfig().getEvaluateConnectibilityDuringCreateGameFinalization())
        {
            return true;
        }

        Util::NatType matchedSessionNatType = matchingSession->getNetworkQosData().getNatType();
        if (finalizingSession->getGameCreationData().getNetworkTopology() == PEER_TO_PEER_FULL_MESH)
        {
            if ( matchingSession->getSupplementalData().mHasMultipleStrictNats
                || (mNatStatus.hasStrictNat && ( matchedSessionNatType >= Util::NAT_TYPE_MODERATE)) // moderate or worse can't connect to strict
                || (mNatStatus.hasModerateNat && (matchedSessionNatType >= Util::NAT_TYPE_STRICT_SEQUENTIAL))) // strict can't connect to moderate.
            {

                if (matchingSession->getSupplementalData().mHostSessionExternalIp == finalizingSession->getSupplementalData().mHostSessionExternalIp)
                {
                    TRACE_LOG("[MatchmakingSession].isSessionConnectable - matching session '" << matchingSession->getMMSessionId() 
                        << "' will be able to connect despite NAT type '" << (Util::NatTypeToString(matchedSessionNatType)) << "' as they are behind the same firewall.");
                    return true;
                }

                // can't join this game skip this session
                TRACE_LOG("[MatchmakingSession].isSessionConnectable - matching session '" << matchingSession->getMMSessionId() 
                    << "' will be unable to connect due to NAT type '" << (Util::NatTypeToString(matchedSessionNatType)) 
                    << "' Finalization currently hasStrict(" << mNatStatus.hasStrictNat << ") hasModerate(" << mNatStatus.hasModerateNat << ").");

                return false;
            }
        }
        return true;
    }

    void CreateGameFinalizationSettings::updatePingSitePlayerCount(const MatchmakingSession& finalizingSession, const MatchmakingSession& matchedSession)
    { 
        const ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap& myAcceptedPingSites = finalizingSession.getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap();
        ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap::const_iterator iter = myAcceptedPingSites.begin();
        ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap::const_iterator end = myAcceptedPingSites.end();
        for (; iter != end; ++iter)
        {
            const PingSiteAlias& pingSite = iter->first;
            const ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap& otherAcceptedPingSites = matchedSession.getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap();
            if (otherAcceptedPingSites.find(pingSite) != otherAcceptedPingSites.end())
            {
                PlayerCountsByAcceptedPingSiteMap::iterator countItr = mPlayerCountsByAcceptedPingSiteMap.find(pingSite);
                if (countItr == mPlayerCountsByAcceptedPingSiteMap.end())
                    mPlayerCountsByAcceptedPingSiteMap[pingSite] = finalizingSession.getPlayerCount();

                mPlayerCountsByAcceptedPingSiteMap[pingSite] += matchedSession.getPlayerCount();
            }
        }
    }

    void CreateGameFinalizationSettings::buildSortedPingSitePlayerCountList(const Blaze::Matchmaker::MatchmakerSlaveImpl& matchmakerSlave, const MatchmakingCriteria& criteria, bool& pingSitePlayerCountSufficient)
    {
        PlayerCountsByAcceptedPingSiteMap::iterator iter = mPlayerCountsByAcceptedPingSiteMap.begin();
        for (; iter != mPlayerCountsByAcceptedPingSiteMap.end();)
        {
            pingSitePlayerCountSufficient = true;
            pingSitePlayerCountSufficient = (iter->second >= matchmakerSlave.getMatchmaker()->getGlobalMinPlayerCountInGame());
            if (criteria.isPlayerCountRuleEnabled())
            {
                // Note that this now only checks ourself and not everyone else.   We only check the owning session list's
                // counts are sufficient after we decide how we are going to build the game.
                pingSitePlayerCountSufficient = criteria.getPlayerCountRule()->isParticipantCountSufficientToCreateGame(iter->second);
            }

            if (!pingSitePlayerCountSufficient)
                iter = mPlayerCountsByAcceptedPingSiteMap.erase(iter);
            else
            {
                mSortedAcceptedPingSitePlayerCounts.push_back(eastl::make_pair(iter->second, iter->first));
                ++iter;
            }
        }

        // Sort the list of ping sites by player count in descending order
        eastl::quick_sort(mSortedAcceptedPingSitePlayerCounts.begin(), mSortedAcceptedPingSitePlayerCounts.end(), eastl::greater<PingSitePlayerCountPair>());
    }

    void CreateGameFinalizationSettings::buildOrderedAcceptedPingSiteList(const ExpandedPingSiteRule& rule, const LatenciesByAcceptedPingSiteIntersection& pingSiteIntersection,
        OrderedPreferredPingSiteList& orderedPreferredPingSites)
    {
        rule.buildAcceptedPingSiteList(pingSiteIntersection, orderedPreferredPingSites);

        OrderedPreferredPingSiteList::const_iterator iter = orderedPreferredPingSites.begin();
        OrderedPreferredPingSiteList::const_iterator end = orderedPreferredPingSites.end();
        for (; iter != end; ++iter)
        {
            mOrderedPreferredPingSites.push_back(iter->second);
        }
    }

    bool CreateGameFinalizationSettings::hasBestPingSite(PingSiteAlias& pingSite)
    {
        PlayerCountsByAcceptedPingSiteList::iterator iter = mSortedAcceptedPingSitePlayerCounts.begin();
        if (iter != mSortedAcceptedPingSitePlayerCounts.end())
        {
            pingSite = (*iter).second;
            return true;
        }

        return false;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
