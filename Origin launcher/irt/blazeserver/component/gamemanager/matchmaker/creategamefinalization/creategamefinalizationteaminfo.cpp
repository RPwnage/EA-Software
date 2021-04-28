/*! ************************************************************************************************/
/*!
    \file   creategamefinalizationteaminfo.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteaminfo.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"

#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    CreateGameFinalizationTeamInfo::CreateGameFinalizationTeamInfo(CreateGameFinalizationSettings* p,
        TeamId id, TeamIndex index, uint16_t capacity, UserExtendedDataValue initialUedValue)
        : mParent(p),
        mTeamId(id),
        mTeamIndex(index),
        mTeamCapacity(capacity),
        mTeamSize(0),
        mUedValue(initialUedValue),
        mTeamHasMultiRoleMembers(false),
        mPickNumberToMmSessionIdsTriedMap(BlazeStlAllocator("mPickNumberToMmSessionIdsTriedMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mPickNumberToBucketsTriedMap(BlazeStlAllocator("mPickNumberToBucketsTriedMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mTeamCompositionToFill(nullptr)
    {

        if (EA_LIKELY(mParent))
        {
            // build out the role size map so we don't have to add roles as we attempt to assign users to a team.
            RoleCriteriaMap::const_iterator roleIter = mParent->getRoleInformation().getRoleCriteriaMap().begin();
            RoleCriteriaMap::const_iterator roleEnd = mParent->getRoleInformation().getRoleCriteriaMap().end();
            for (; roleIter != roleEnd; ++roleIter)
            {
                mRoleSizeMap[roleIter->first] = 0;
            }
        }
        else
        {
            ERR_LOG("[CreateGameFinalizationTeamInfo] was created with nullptr mParent CreateGameFinalizationSettings.");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief checks if session has not joined a team, and can fit into this team's size and roles
    ***************************************************************************************************/
    bool CreateGameFinalizationTeamInfo::canPotentiallyJoinTeam(const MatchmakingSession& session, bool failureAsError, TeamId joiningTeamId) const
    {
        // check if we've already joined the finalization
        TeamIndex teamIdx = mParent->getTeamIndexForMMSession(session, joiningTeamId);
        if (teamIdx != UNSPECIFIED_TEAM_INDEX)
        {
            if (failureAsError)
            {
                ERR_LOG("[CreateGameFinalizationTeamInfo].canPotentiallJoinTeam matchmaking session '" << session.getMMSessionId() << "' team id " << joiningTeamId << " size '"
                    << session.getTeamSize(joiningTeamId) << "' already on team index '" << teamIdx << "'");
            }
            return false;
        }

        // double check the team ids
        const TeamIdSizeList* myTeamIds = session.getCriteria().getTeamIdSizeListFromRule();
        if (myTeamIds == nullptr)
        {
            TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' missing TeamIdSizeList for team '"
                << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize << "'.");
            return false;
        }

        TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
        TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
        for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
        {
            TeamId sessionTeamId = teamIdSize->first;
            if (sessionTeamId != joiningTeamId)
                continue;

            uint16_t myTeamJoinCount = teamIdSize->second;
            if ((sessionTeamId != ANY_TEAM_ID) && (mTeamId != ANY_TEAM_ID) && (sessionTeamId != mTeamId))
                continue;

            if (myTeamJoinCount + mTeamSize > mTeamCapacity)
            {
                TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' size '" 
                        << myTeamJoinCount << "' is too big for team '" << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize 
                        << "' capacity '" << mTeamCapacity << "'");
                continue;
            }

            if (session.getCriteria().getTeamIdRoleSpaceRequirementsFromRule() == nullptr)
            {
                TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' missing RoleSpaceRequirementsFromRule for team '"
                    << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize << "'.");
                continue;
            }

            TeamIdRoleSizeMap::const_iterator teamRoleSize = session.getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->find(sessionTeamId);
            if (teamRoleSize == session.getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->end())
            {
                TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' missing TeamIdSizeList for team '"
                    << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize << "'.");
                continue;
            }

            if (!checkJoiningTeamCompositions(session.getCriteria().getAcceptableGameTeamCompositionsInfoVectorFromRule(), session.getTeamSize(joiningTeamId), session))
            {
                TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' violated team composition criteria for team '"
                    << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize << "'. Other session's acceptable game team compositions: " << (session.getCriteria().hasTeamCompositionRuleEnabled()? session.getCriteria().getTeamCompositionRule()->getCurrentAcceptableGtcVectorInfoCacheLogStr() : "<team compositions criteria n/a>") << ".");
                continue;
            }

            // this is a lighter role check, skip over it if we've got multirole members, as it's not effective in that case
            if (!mTeamHasMultiRoleMembers && !session.getCriteria().getRolesRule()->hasMultiRoleMembers() && !checkJoiningRoles(teamRoleSize->second))
            {
                TRACE_LOG("[CreateGameFinalizationTeamInfo].canPotentiallyJoinTeam matchmaking session '" << session.getMMSessionId() << "' violated role criteria for team '"
                    << mTeamId << "' index '" << teamIdx << "' size '" << mTeamSize << "'.");
                continue;
            }

            // If we get here, then one of the teams of the session would fit in this CG finalization team info struct. (Yay)
            return true;
        }

        // If we get here, then none of the teams will fit in this team.
        return false;
    }

    bool CreateGameFinalizationTeamInfo::joinTeam(const MatchmakingSession& session, TeamId joiningTeamId)
    {
        if (!canPotentiallyJoinTeam(session, true, joiningTeamId))
            return false;//logged

        if (session.getCriteria().getTeamIdRoleSpaceRequirementsFromRule() == nullptr)
        {
            ERR_LOG("[CreateGameFinalizationTeamInfo].joinTeam unexpected error, missing RoleSpaceRequirementsFromRule for " << toLogStr() << ", when joining session " << session.getMMSessionId() << ".");
            return false;
        }

        // update our team UED if we're enabling TeamUEDBalanceRule (can just check joining session since if anyone in match has such a rule enabled, all must by create game evaluation).
        const TeamUEDBalanceRule* teamUEDRule = session.getCriteria().getTeamUEDBalanceRule();
        if ((teamUEDRule != nullptr) && !teamUEDRule->isDisabled())
        {
            if (!teamUEDRule->getDefinition().recalcTeamUEDValue(mUedValue, mTeamSize, teamUEDRule->getUEDValue(),
                teamUEDRule->getPlayerCount(), false, mMatchmakingSessionUedValues, mTeamIndex))
            {
                ERR_LOG("[CreateGameFinalizationTeamInfo].joinTeam unexpected error, failed to update ued value for " << toLogStr() << ", when joining session " << session.getMMSessionId() << ". See preceding server log error messages for details.");
                return false;
            }
        }

       
        // now validate role capacities, accounting for multi-role seeking members
        // if everyone only wants a single role, or accepts any role, we don't have to do this
        if (session.getCriteria().getRolesRule()->hasMultiRoleMembers() || mTeamHasMultiRoleMembers)
        {
            // we haven't generated a CG request yet, but we can get the role info from the roles rule
            CreateGameMasterRequestPtr tempCreateGameRequest = BLAZE_NEW CreateGameMasterRequest();
            session.getGameCreationData().copyInto(tempCreateGameRequest->getCreateRequest().getGameCreationData());

            // first put everyone who's on this team into the list
            PerPlayerJoinDataList& finalizingPlayerJoinDataList = tempCreateGameRequest->getCreateRequest().getPlayerJoinData().getPlayerDataList();
            finalizingPlayerJoinDataList.reserve(mParent->getPlayerCapacity());
            for (auto matchingSession: mParent->getMatchingSessionList())
            {
                // only add the players if they're on this team
                if ((mParent->getTeamIndexForMMSession(*matchingSession, mTeamId) == mTeamIndex) || (mParent->getTeamIndexForMMSession(*matchingSession, ANY_TEAM_ID) == mTeamIndex))
                {
                    for (auto playerJoinData : matchingSession->getPlayerJoinData().getPlayerDataList())
                    {
                        // (ensure don't use orig mm session's copy!:)
                        playerJoinData->copyInto(*finalizingPlayerJoinDataList.pull_back());
                        // need to assign the right team index at this stage, since it's not in the join data right now.
                        finalizingPlayerJoinDataList.back()->setTeamIndex(mTeamIndex);
                    }
                }
            }

            // now add our new players
            for (auto playerJoinData : session.getPlayerJoinData().getPlayerDataList())
            {
                // only add the player if they're on this team
                playerJoinData->copyInto(*finalizingPlayerJoinDataList.pull_back());
                // need to assign the right team index at this stage, since it's not in the join data right now.
                finalizingPlayerJoinDataList.back()->setTeamIndex(mTeamIndex);
            }

            // must now find role assignments, for the any/multi-role mm session members.
            // To optimize chances, first sort the mm session members by role count
            UserRoleCounts<PerPlayerJoinDataPtr> perPlayerJoinDataRoleCounts;
            session.sortPlayersByDesiredRoleCounts(finalizingPlayerJoinDataList, perPlayerJoinDataRoleCounts);

            TeamIndexRoleSizeMap roleCounts; // Temporary map to keep track of the current count for each role
            uint64_t highestRoleCount = 1;
            // now that we're sorted, try to find roles for everyone
            for (auto& ppJoinDataPair : perPlayerJoinDataRoleCounts)
            {
                // we're not assigning newRole anywhere because finalization may not occur, and we don't want to change the data on any running mm session
                RoleName newRole;
                if (ppJoinDataPair.second > highestRoleCount)
                {
                    highestRoleCount = ppJoinDataPair.second;
                }
                BlazeRpcError err = session.findOpenRole(*tempCreateGameRequest, *ppJoinDataPair.first, newRole, roleCounts);
                if (err != ERR_OK)
                {
                    LogContextOverride logAudit(session.getOwnerUserSessionId());

                    if (IS_LOGGING_ENABLED(Logging::TRACE))
                    {
                        eastl::string rolesBuf = "";
                        bool firstRole = true;
                        for (auto& desiredRoles : ppJoinDataPair.first->getRoles())
                        {
                            if (!firstRole)
                            {
                                rolesBuf += ", ";
                            }

                            rolesBuf += desiredRoles;
                            firstRole = false;
                        }

                        if (rolesBuf.empty())
                        {
                            rolesBuf = ppJoinDataPair.first->getRole();
                        }

                        TRACE_LOG("[CreateGameFinalizationTeamInfo].joinTeam: Failed to find open role from set (" << rolesBuf
                            << ") for player " << ppJoinDataPair.first->getUser().getBlazeId() << ", when attempting to check if session " << session.getMMSessionId() << " can be finalized with.");
                    }
                    return false;
                }
                else
                {
                    // keep the game built up as we iterate all the potential team members.
                    ppJoinDataPair.first->setRole(newRole);
                    ppJoinDataPair.first->getRoles().clear();
                    SPAM_LOG("[CreateGameFinalizationTeamInfo].joinTeam: Role(" << ppJoinDataPair.first->getRole() <<
                        ") chosen for player(" << ppJoinDataPair.first->getUser().getBlazeId() << "), on teamIndex(" << mTeamIndex << ")");
                }
            }
            // update the roleSizeMap to reflect our lookup results
            mRoleSizeMap.clear();
            mRoleSizeMap.assign(roleCounts[mTeamIndex].begin(), roleCounts[mTeamIndex].end());
            if (highestRoleCount > 1)
            {
                mTeamHasMultiRoleMembers = true;
            }
            else
            {
                mTeamHasMultiRoleMembers = false;
            }
        }

        addSessionToTeam(session, joiningTeamId);

        // update UED rankings if TeamUEDPositionParityRule is enabled (can just check joining session since if anyone in match has such a rule enabled, all must by create game evaluation).
        const TeamUEDPositionParityRule* teamUEDPostitionParityRule = session.getCriteria().getTeamUEDPositionParityRule();
        if ((teamUEDPostitionParityRule != nullptr) && !teamUEDPostitionParityRule->isDisabled())
        {
            mMemberUedValues.insert(mMemberUedValues.end(), teamUEDPostitionParityRule->getUEDValueList().begin(), teamUEDPostitionParityRule->getUEDValueList().end());
            // sort the team vectors as we add members
            eastl::sort(mMemberUedValues.begin(), mMemberUedValues.end(), eastl::greater<UserExtendedDataValue>());
        }

        uint16_t joiningTeamSize = session.getTeamSize(joiningTeamId);
        bool filledTeamCompositionGroup = markTeamCompositionToFillGroupSizeFilled(joiningTeamSize);
        ASSERT_COND_LOG(((getTeamCompositionToFill() == nullptr) || filledTeamCompositionGroup), "[CreateGameFinalizationTeamInfo].joinTeam: team(" << mTeamIndex << ") group of size(" << joiningTeamSize << ") unexpectedly not markable as (newly) filled for current team composition.");// canPotentiallyJoinTeam()/hasUnfilledGroupSize() should've prevented being here

        mParent->claimTeamIndexForMMSession(session, joiningTeamId, mTeamIndex);
        mTeamSize += joiningTeamSize;

        
        

        // Update the TeamId to match the joining team (if we're ANY team)
        if (mTeamId == ANY_TEAM_ID) 
        {
            mTeamId = joiningTeamId;
        }

        return true;
    }

    bool CreateGameFinalizationTeamInfo::checkJoiningRoles( const RoleSizeMap& joiningRoleSizeMap ) const
    {
        // build out a temp role size map that combines the struct's current role size map with the role size map from the joining session
        RoleSizeMap potentialRoleMap;
        potentialRoleMap.assign(mRoleSizeMap.begin(), mRoleSizeMap.end());

        // then validate the role capacities in the provided RoleInformation.
        RoleSizeMap::const_iterator joiningIter = joiningRoleSizeMap.begin();
        RoleSizeMap::const_iterator joiningEnd = joiningRoleSizeMap.end();
        for (; joiningIter != joiningEnd; ++joiningIter)
        {
            potentialRoleMap[joiningIter->first] += joiningIter->second;

            if (blaze_stricmp(joiningIter->first, GameManager::PLAYER_ROLE_NAME_ANY) != 0)
            {
                RoleCriteriaMap::const_iterator roleIter = mParent->getRoleInformation().getRoleCriteriaMap().find(joiningIter->first);
                if (EA_UNLIKELY(roleIter == mParent->getRoleInformation().getRoleCriteriaMap().end()))
                {
                    // role not allowed, should never happen (this session would have been rejected as a match earlier)
                    ERR_LOG("[CreateGameFinalizationTeamInfo].checkJoiningRoles matchmaking session trying to join role '"
                        << joiningIter->first.c_str() << "' which isn't allowed for the finalizing session.");
                    return false;
                }

                if (potentialRoleMap[joiningIter->first] > roleIter->second->getRoleCapacity())
                {
                    TRACE_LOG("[CreateGameFinalizationTeamInfo].checkJoiningRoles matchmaking session exceeded role capacity '" << roleIter->second->getRoleCapacity() << "' of role '"
                        << joiningIter->first << "' when trying to add '" << joiningIter->second << "' players.");
                    return false;
                }
            }
        }

        EntryCriteriaName failedEntryCriteria;
        if (!mParent->getMultiRoleEntryCriteraEvaluator().evaluateEntryCriteria(potentialRoleMap, failedEntryCriteria))
        {
            TRACE_LOG("[CreateGameFinalizationTeamInfo].checkJoiningRoles Players failed multirole entry criteria ("
                    << failedEntryCriteria.c_str() << ")");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief we do joinability as well bidirectional checking here up front (full checking with all
        teams and sessions against team composition criteria is at near finalization attempt's end)
    ***************************************************************************************************/
    bool CreateGameFinalizationTeamInfo::checkJoiningTeamCompositions(const GameTeamCompositionsInfoVector* otherAcceptableGtcInfoVector,
        uint16_t sessionSize, const MatchmakingSession& session) const
    {
        if (getTeamCompositionToFill() == nullptr)//I've disabled or not yet initialized team compositions info
        {
            if (otherAcceptableGtcInfoVector != nullptr)
            {
                TRACE_LOG("[TeamCompositionTriedInfo].checkJoiningTeamCompositions: my team composition info not (yet) available, even though session(sessId=" << session.getMMSessionId() << ",blazeId=" << session.getOwnerBlazeId() << ") enabled team compositions.");
            }
            return true;
        }
        // I've enabled, other should have also
        if (otherAcceptableGtcInfoVector == nullptr)
        {
            ERR_LOG("[CreateGameFinalizationTeamInfo].checkJoiningTeamCompositions possible internal error: other session(sessId=" << session.getMMSessionId() << ",blazeId=" << session.getOwnerBlazeId() << ") has disabled team compositions. Ignoring its bidirectionality check.");
            return true;
        }

        // verify the other session's size can currently fit into the current team compositions
        if (!getTeamCompositionToFill()->hasUnfilledGroupSize(sessionSize))
            return false;

        // we always try one GameTeamCompositions at a time. Thus to match check the other must have the same one as the one we're trying now.
        for (size_t i = 0; i < otherAcceptableGtcInfoVector->size(); ++i)
        {
            if ((*otherAcceptableGtcInfoVector)[i].mGameTeamCompositionsId == getTeamCompositionToFill()->getGameTeamCompositionsId())
            {
                SPAM_LOG("[CreateGameFinalizationTeamInfo].checkJoiningTeamCompositions other session(groupSize=" << sessionSize << ") can join the team(index=" << mTeamIndex << "), with our current GameTeamCompositionsId(" << getTeamCompositionToFill()->getGameTeamCompositionsId() << ") as it is also in that session(sessId=" << session.getMMSessionId() << ",blazeId=" << session.getOwnerBlazeId() << ")'s acceptable list. Team composition to join is " << getTeamCompositionToFill()->getParentLogStr() << ".");
                return true;
            }
        }

        TRACE_LOG("[CreateGameFinalizationTeamInfo].checkJoiningTeamCompositions other session(groupSize=" << sessionSize << ") can NOT join the team(index=" << mTeamIndex << "), as our current GameTeamCompositionsId(" << getTeamCompositionToFill()->getGameTeamCompositionsId() << ") is NOT in the other session(sessId=" << session.getMMSessionId() << ",blazeId=" << session.getOwnerBlazeId() << ")'s acceptable list. Team composition tried to join is " << getTeamCompositionToFill()->getParentLogStr() << ".");
        return false;
    }

    void CreateGameFinalizationTeamInfo::addSessionToTeam(const MatchmakingSession& addedSession, TeamId joiningTeamId)
    {
        if (!mTeamHasMultiRoleMembers)
        {
            // we can add to the role size maps directly if there are no users who accept multiple roles
            const RoleSizeMap& roleSizeMap = addedSession.getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->find(joiningTeamId)->second;
            addSessionRolesToTeam(roleSizeMap);
        }
        // team assignment for the mm session is tracked in the parent CreateGameFinalization
    }

    void CreateGameFinalizationTeamInfo::addSessionRolesToTeam(const RoleSizeMap& joiningRoleSizeMap)
    {
        RoleSizeMap::const_iterator joiningIter = joiningRoleSizeMap.begin();
        RoleSizeMap::const_iterator joiningEnd = joiningRoleSizeMap.end();
        for (; joiningIter != joiningEnd; ++joiningIter)
        {
            mRoleSizeMap[joiningIter->first] += joiningIter->second;
        }
    }

    void CreateGameFinalizationTeamInfo::removeSessionFromTeam(const MatchmakingSession& removedSession, TeamId removingTeamId)
    {
        TeamIndex removingTeamIndex = mParent->getTeamIndexForMMSession(removedSession, removingTeamId);

        if (removingTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            if (!mTeamHasMultiRoleMembers)
            {
                // we can subtract from the role size maps directly if there are no users who accept multiple roles
                const RoleSizeMap& roleSizeMap = removedSession.getCriteria().getTeamIdRoleSpaceRequirementsFromRule()->find(removingTeamId)->second;
                removeSessionRolesFromTeam(roleSizeMap);
            }
            else
            {
                // wipe the role counts, we'll rebuild on the next add
                mRoleSizeMap.clear();
            }
        }

    }

    void CreateGameFinalizationTeamInfo::removeSessionRolesFromTeam(const RoleSizeMap& removingRoleSizeMap)
    {
        RoleSizeMap::const_iterator removingIter = removingRoleSizeMap.begin();
        RoleSizeMap::const_iterator removingEnd = removingRoleSizeMap.end();
        for (; removingIter != removingEnd; ++removingIter)
        {
            mRoleSizeMap[removingIter->first] -= removingIter->second;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief checks if session already 'tried' for the current pick of the finalization
    ***************************************************************************************************/
    bool CreateGameFinalizationTeamInfo::wasSessionTriedForCurrentPick(MatchmakingSessionId mmSessionId, uint16_t pickNumber) const
    {
        PickNumberToMmSessionIdsMap::const_iterator itr = mPickNumberToMmSessionIdsTriedMap.find(pickNumber);

        if (itr == mPickNumberToMmSessionIdsTriedMap.end())
        {
            SPAM_LOG("[CreateGameFinalizationTeamInfo].wasSessionTriedForCurrentPick: never tried any picks for pick number " << pickNumber << " for " << toLogStr());
            return false;
        }
        bool found = (eastl::find(itr->second.begin(), itr->second.end(), mmSessionId) != itr->second.end());
        if (found)
        {
            TRACE_LOG("[CreateGameFinalizationTeamInfo].wasSessionTriedForCurrentPick: MM session '" << mmSessionId << "' was already tried and will be skipped for pick number " << pickNumber << " for " << toLogStr());
        }
        else
        {
            SPAM_LOG("[CreateGameFinalizationTeamInfo].wasSessionTriedForCurrentPick: MM session '" << mmSessionId << "' was not tried yet for pick number " << pickNumber << " for " << toLogStr());
        }
        return found;
    }

    bool CreateGameFinalizationTeamInfo::wasBucketTriedForCurrentPick(MatchedSessionsBucketingKey bucketKey, uint16_t pickNumber) const
    {
        PickNumberToBucketKeyListMap::const_iterator itr = mPickNumberToBucketsTriedMap.find(pickNumber);

        // true if no bucket was ever added
        if (itr == mPickNumberToBucketsTriedMap.end())
        {
            SPAM_LOG("[CreateGameFinalizationTeamInfo].wasBucketTriedForCurrentPick: never tried any picks for pick number " << pickNumber << " for bucket '" << bucketKey << "' for " << toLogStr());
            return false;
        }
        bool found = (eastl::find(itr->second.begin(), itr->second.end(), bucketKey) != itr->second.end());
        if (found)
        {
            TRACE_LOG("[CreateGameFinalizationTeamInfo].wasBucketTriedForCurrentPick: bucket '" << bucketKey << "' was already tried and will be skipped for pick number " << pickNumber << " for " << toLogStr());
        }
        else
        {
            SPAM_LOG("[CreateGameFinalizationTeamInfo].wasBucketTriedForCurrentPick: bucket '" << bucketKey << "' was never, or not completedly, tried yet for pick number " << pickNumber << " for " << toLogStr());
        }
        return found;
    }

    void CreateGameFinalizationTeamInfo::markSessionTriedForCurrentPick(MatchmakingSessionId mmSessionId, uint16_t pickNumber)
    {
        mPickNumberToMmSessionIdsTriedMap[pickNumber].push_back(mmSessionId);
        SPAM_LOG("[CreateGameFinalizationTeamInfo] markSessionTried: marked session " << mmSessionId << " as tried for pick number " << pickNumber << " for " << toLogStr());
    }
    void CreateGameFinalizationTeamInfo::markBucketTriedForCurrentPick(MatchedSessionsBucketingKey bucketKey, uint16_t pickNumber)
    {
        mPickNumberToBucketsTriedMap[pickNumber].push_back(bucketKey);
        SPAM_LOG("[CreateGameFinalizationTeamInfo] markBucketsTried: marked bucket " << bucketKey << " as completely tried for pick number " << pickNumber << " for " << toLogStr());
    }

    void CreateGameFinalizationTeamInfo::clearSessionsTried(uint16_t abovePickNumber)
    {
        SPAM_LOG("[CreateGameFinalizationTeamInfo] clearSessionsTried: clearing all sessions 'tried' above pick number '" << abovePickNumber << "' for " << toLogStr());
        for (PickNumberToMmSessionIdsMap::iterator itr = mPickNumberToMmSessionIdsTriedMap.begin(); itr != mPickNumberToMmSessionIdsTriedMap.end(); ++itr)
        {
            if (itr->first > abovePickNumber)
                itr->second.clear();
        }
    }
    void CreateGameFinalizationTeamInfo::clearBucketsTried(uint16_t abovePickNumber)
    {
        SPAM_LOG("[CreateGameFinalizationTeamInfo] clearBucketsTried: clearing all buckets 'tried' above pick number '" << abovePickNumber << "' for " << toLogStr());
        for (PickNumberToBucketKeyListMap::iterator itr = mPickNumberToBucketsTriedMap.begin(); itr != mPickNumberToBucketsTriedMap.end(); ++itr)
        {
            if (itr->first > abovePickNumber)
                itr->second.clear();
        }
    }

    void CreateGameFinalizationTeamInfo::setTeamCompositionToFill(TeamCompositionFilledInfo* teamCompositionToFill)
    {
        mTeamCompositionToFill = teamCompositionToFill;
        TRACE_LOG("[CreateGameFinalizationTeamInfo].setTeamCompositionToFill set finalization for team(index=" << mTeamIndex <<")'s team composition to fill " << ((teamCompositionToFill != nullptr)? teamCompositionToFill->toLogStr(false) : "nullptr") << ".");
    }

    bool CreateGameFinalizationTeamInfo::markTeamCompositionToFillGroupSizeFilled(uint16_t groupSize)
    {
        if (getTeamCompositionToFill() != nullptr)
        {
            return getTeamCompositionToFill()->markGroupSizeFilled(groupSize);
        }
        else
        {
            TRACE_LOG("[CreateGameFinalizationTeamInfo].markTeamCompositionToFillGroupSizeFilled No op. No team composition assigned to the team, cannot mark the requested group size(" << groupSize << ") as filled, Team: " << toLogStr());
            return false;
        }
    }
    // clears a single instance of the groupSize from being marked as filled
    void CreateGameFinalizationTeamInfo::clearTeamCompositionToFillGroupSizeFilled(uint16_t groupSize)
    {
        if (getTeamCompositionToFill() != nullptr)
        {
            getTeamCompositionToFill()->clearGroupSizeFilled(groupSize);
        }
        else
        {
            ERR_LOG("[CreateGameFinalizationTeamInfo].clearTeamCompositionToFillGroupSizeFilled No op. No team composition assigned to the team, cannot unmark the requested group size(" << groupSize << ") as filled, Team: " << toLogStr());
        }
    }

    const char8_t* CreateGameFinalizationTeamInfo::toLogStr() const
    {
        mLogBuf.clear();
        mLogBuf.sprintf("team(index=%u,size=%u,cap=%u,ued=%" PRIi64 ",mTeamId=%u)", mTeamIndex, mTeamSize, mTeamCapacity, mUedValue, mTeamId);
        return mLogBuf.c_str();
    }

    UserExtendedDataValue CreateGameFinalizationTeamInfo::getRankedUedValue(uint16_t rank) const
    {
        if (rank < mMemberUedValues.size())
        {
            return mMemberUedValues[rank];
        }

        return INVALID_USER_EXTENDED_DATA_VALUE;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
