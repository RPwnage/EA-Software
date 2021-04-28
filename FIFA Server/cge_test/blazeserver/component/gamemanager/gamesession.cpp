/*! ************************************************************************************************/
/*!
    \file gamesession.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesession.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/commoninfo.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/groupvalueformulas.h" // for getCalcGroupFunction in findOpenTeam
#include "gamemanager/matchmaker/teamcompositionscommoninfo.h" // for calcNumberOfGroupsInTeamCompositionList in doesGameViolateGameTeamCompositions
#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{

    bool isGameProtocolVersionStringCompatible(bool doEvaluate, const char8_t* gp1, const char8_t* gp2)
    {
        if (!doEvaluate || (blaze_strcmp(gp1, gp2) == 0) || (blaze_strcmp(gp1, GAME_PROTOCOL_VERSION_MATCH_ANY) == 0) || (blaze_strcmp(gp2, GAME_PROTOCOL_VERSION_MATCH_ANY) == 0))
        {
            return true;
        }

        return false;
    }

    // This ctor is used to create a brand new GameSession
    GameSession::GameSession(GameId gameId)
        : mGameId(gameId), mExpressionMap(BlazeStlAllocator("GameSession::mExpressionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mGameData(BLAZE_NEW ReplicatedGameDataServer),
        mRoleEntryCriteriaEvaluators(BlazeStlAllocator("GameSession::mRoleCriteriaEvaluators", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        mMultiRoleEntryCriteriaEvaluator.setMutableRoleInfoSource(*this);
        getGameData().setGameId(gameId);
        getGameData().setSharedSeed((uint32_t)Random::getRandomNumber(RAND_MAX)^(uint32_t)TimeValue::getTimeOfDay().getSec());
    }

    // This ctor is used to create a GameSession given an already valid and fully populated replicatedGameSession
    GameSession::GameSession(ReplicatedGameDataServer &replicatedGameSession)
        : mGameId(replicatedGameSession.getReplicatedGameData().getGameId()),
          mExpressionMap(BlazeStlAllocator("GameSession::mExpressionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mGameData(&replicatedGameSession),
          mRoleEntryCriteriaEvaluators(BlazeStlAllocator("GameSession::mRoleCriteriaEvaluators", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        mMultiRoleEntryCriteriaEvaluator.setMutableRoleInfoSource(*this);
    }

    GameSession::~GameSession()
    {
        // free any parsed game entry criteria expressions
        clearCriteriaExpressions();
        cleanUpRoleEntryCriteriaEvaluators();
    }

    eastl::string GameSession::getPlatformForMetrics() const
    {
        eastl::string platformStr = "";
        bool firstEntry = true;


        // iterate over possible platforms in enum order to guarantee consistency in the metrics
        for (auto& platformValue : GetClientPlatformTypeEnumMap().getNamesByValue())
        {
            const EA::TDF::TdfEnumInfo& info = (const EA::TDF::TdfEnumInfo&)(platformValue);
            if (getCurrentlyAcceptedPlatformList().findAsSet((ClientPlatformType)info.mValue) != getCurrentlyAcceptedPlatformList().end())
            {
                if (!firstEntry)
                {
                    platformStr += "-";
                }

                platformStr += info.mName;
                firstEntry = false;
            }
        }

        if (platformStr.empty())
        {
            platformStr += ClientPlatformTypeToString(INVALID);
        }
        

        return platformStr;
    }
    
    bool GameSession::getTopologyHostSessionExists() const
    {
        return getTopologyHostSessionId() != INVALID_USER_SESSION_ID && gUserSessionManager->getSessionExists(getTopologyHostSessionId());
    }

    bool GameSession::getDedicatedServerHostSessionExists() const
    {
        return getDedicatedServerHostSessionId() != INVALID_USER_SESSION_ID && gUserSessionManager->getSessionExists(getDedicatedServerHostSessionId());
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve game attribute value using given attribute name

        \param[in] name - game attribute name
        \return game attribute value for the given attribute, nullptr if the attribute is not found
    ***************************************************************************************************/
    const char8_t* GameSession::getGameAttrib(const char8_t *name) const
    {
        Collections::AttributeMap::const_iterator it = getGameAttribs().find(name);
        if( it != getGameAttribs().end())
        {
            return it->second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve mesh attribute value using given attribute name

        \param[in] name - mesh attribute name
        \return mesh attribute value for the given attribute, nullptr if the attribute is not found
    ***************************************************************************************************/
    const char8_t* GameSession::getMeshAttrib(const char8_t *name) const
    {
        Collections::AttributeMap::const_iterator it = getMeshAttribs().find(name);
        if( it != getMeshAttribs().end())
        {
            return it->second;
        }

        return nullptr;
    }

    const char8_t* GameSession::getDedicatedServerAttrib(const char8_t* name) const
    {
        Collections::AttributeMap::const_iterator it = getDedicatedServerAttribs().find(name);
        if (it != getDedicatedServerAttribs().end())
        {
            return it->second;
        }

        return nullptr;
    }

    uint16_t GameSession::getTotalParticipantCapacity() const
    {
        uint16_t capacity = 0;
        const SlotCapacitiesVector &slotCapacities = getGameData().getSlotCapacities();
        for(size_t slot = 0; slot < (uint16_t)MAX_PARTICIPANT_SLOT_TYPE; slot++)
        {
            capacity += slotCapacities[slot];
        }
        return capacity;
    }

    uint16_t GameSession::getTotalSpectatorCapacity() const
    {
        uint16_t capacity = 0;
        const SlotCapacitiesVector &slotCapacities = getGameData().getSlotCapacities();
        for(size_t slot = (uint16_t)MAX_PARTICIPANT_SLOT_TYPE; slot < MAX_SLOT_TYPE; slot++)
        {
            capacity += slotCapacities[slot];
        }
        return capacity;
    }

    uint16_t GameSession::getTotalPlayerCapacity() const
    {
        return getTotalParticipantCapacity() + getTotalSpectatorCapacity();
    }

    uint16_t GameSession::getTeamCapacity() const
    {
        return getGameData().getTeamIds().empty() ? 0 : getTotalParticipantCapacity() / (uint16_t) getGameData().getTeamIds().size();
    }


    /*! ************************************************************************************************/
    /*! \brief Checks if the user group can join a single group match game.  A gruop is allowed to join
        the game if sgm is enabled and the group is already in the game or there is <= 1 groups in the game
        currently.

        \param[in] groupId - the user group id to check
        \return true if the provided group id is allowed to join the game.
    ***************************************************************************************************/
    bool GameSession::isGroupAllowedToJoinSingleGroupGame(const UserGroupId& groupId) const
    {
        if (!getGameData().getGameSettings().getEnforceSingleGroupJoin())
        {
            return false; // only valid for single group games
        }

        for (SingleGroupMatchIdList::iterator i = mGameData->getSingleGroupMatchIds().begin(), e = mGameData->getSingleGroupMatchIds().end(); i != e; ++i)
            if (*i == groupId)
                return true; // group is in the list already

        // otherwise, see if there's room for the group
        return (mGameData->getSingleGroupMatchIds().size() <= 1); // ok to join, as long as there aren't 2 groups in game already
    }

    TeamId GameSession::getTeamIdByIndex(const TeamIndex& teamIndex) const
    {
        const TeamIdVector &teamIds = getGameData().getTeamIds();
        if(teamIndex < teamIds.size())
        {
            return teamIds[teamIndex];
        }

        return INVALID_TEAM_ID;
    }

    /*! ************************************************************************************************/
    /*! \brief set up role-specific entry criteria
            NOTE: entry criteria setup will early-out if any of the criteria fails to process.

    \return ERR_OK if role-specific entry criteria was successfully parsed.
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSession::updateRoleEntryCriteriaEvaluators()
    {
        // delete the existing role criteria evaluators
        cleanUpRoleEntryCriteriaEvaluators();
        // iterate over the game roles, and create the expression maps
        RoleCriteriaMap::const_iterator roleCriteriaIter = getGameData().getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator roleCriteriaEnd = getGameData().getRoleInformation().getRoleCriteriaMap().end();
        for(; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            const RoleName& roleName = roleCriteriaIter->first;
            RoleEntryCriteriaEvaluator *roleEntryCriteriaEvaluator = BLAZE_NEW RoleEntryCriteriaEvaluator(roleName, roleCriteriaIter->second->getRoleEntryCriteriaMap());
            mRoleEntryCriteriaEvaluators.insert(eastl::make_pair(roleName, roleEntryCriteriaEvaluator));
            int32_t errorCount = roleEntryCriteriaEvaluator->createCriteriaExpressions();
            if (errorCount > 0)
            {
                TRACE_LOG("[GameSession] Failed to parse role-specific entry criteria for game(" << getGameId() << ") role '" << roleName << "'.");
                // clean up the failed role entry criteria
                cleanUpRoleEntryCriteriaEvaluators();
                return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
            }

        }

        // Do the same for the multi-role criteria: (We've already set the RoleInformation reference, so there's no need to set it again)
        int32_t errorCount = mMultiRoleEntryCriteriaEvaluator.createCriteriaExpressions();
        if (errorCount > 0)
        {
            TRACE_LOG("[GameSession] Failed to parse multirole entry criteria for game(" << getGameId() << ").");
            // clean up the failed role entry criteria
            cleanUpRoleEntryCriteriaEvaluators();
            return GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_INVALID;
        }

        return ERR_OK;
    }

    void GameSession::cleanUpRoleEntryCriteriaEvaluators()
    {
        RoleEntryCriteriaEvaluatorMap::iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.begin();
        RoleEntryCriteriaEvaluatorMap::iterator roleCriteriaEnd = mRoleEntryCriteriaEvaluators.end();
        for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            delete roleCriteriaIter->second;
            roleCriteriaIter->second = nullptr;
        }

        mMultiRoleEntryCriteriaEvaluator.clearCriteriaExpressions();
        mRoleEntryCriteriaEvaluators.clear();
    }

    
    BlazeRpcError GameSession::checkJoinability(const TeamIndexRoleSizeMap &teamRoleSpaceRequirements) const
    {
        // Check all the teams individually:
        TeamIndexRoleSizeMap::const_iterator teamIter = teamRoleSpaceRequirements.begin();
        TeamIndexRoleSizeMap::const_iterator teamEnd = teamRoleSpaceRequirements.end();
        for (; teamIter != teamEnd; ++teamIter)
        {
            BlazeRpcError err = checkJoinabilityByTeamIndex(teamIter->second, teamIter->first);
            if (err != ERR_OK)
                return err;
        }

        return ERR_OK;
    }

    BlazeRpcError GameSession::checkJoinabilityByTeamIndex(const RoleSizeMap &roleSpaceRequirements, const TeamIndex& teamIndex) const
    {
        const PlayerRoster *playerRoster = getPlayerRoster();
        if (!playerRoster)
        {
            ERR_LOG("[GameSession::checkJoinabilityByTeamIndex()] getPlayerRoster() returned nullptr for game(" << getGameId() << ").");
            return ERR_SYSTEM;
        }

        RoleSizeMap::const_iterator roleIter = roleSpaceRequirements.begin();
        RoleSizeMap::const_iterator roleEnd = roleSpaceRequirements.end();
        for (; roleIter != roleEnd; ++roleIter)
        {
            if (blaze_stricmp(roleIter->first, GameManager::PLAYER_ROLE_NAME_ANY) != 0)
            {
                // Check that the role is valid:
                if (getRoleInformation().getRoleCriteriaMap().find(roleIter->first) == getRoleInformation().getRoleCriteriaMap().end())
                {
                    TRACE_LOG("[GameSession::checkJoinabilityByTeamIndex()]  game(" << getGameId() << ") does not have role '" << roleIter->first.c_str() << "'.");
                    return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                }

                uint16_t totalRoleSize = roleIter->second + playerRoster->getRoleSize(teamIndex, roleIter->first);
                uint16_t roleCapacity = getRoleCapacity(roleIter->first);
                if (totalRoleSize > roleCapacity)
                {
                    TRACE_LOG("[GameSession::checkJoinabilityByTeamIndex()]  game(" << getGameId()
                        << ") did not have space in role '" << roleIter->first.c_str() << "' on team(" << teamIndex << ") for (" << roleIter->second << ") players.");
                    return GAMEMANAGER_ERR_ROLE_FULL;
                }
            }
        }

        EntryCriteriaName failedName;
        if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(getPlayerRoster(), roleSpaceRequirements, teamIndex, failedName))
        {
            TRACE_LOG("[GameSession::checkJoinabilityByTeamIndex()]  game(" << getGameId()
                << ") failed the multirole criteria (" << failedName.c_str() << ") for team(" << teamIndex << ") for joining players.");
            return GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED;
        }

        return ERR_OK;
    }

    
    BlazeRpcError GameSession::findOpenTeams(const TeamIdRoleSizeMap& teamRoleSizes, TeamIdToTeamIndexMap& foundTeamIndexes) const
    {
        // We need to check from largest group to smallest, and then skip any teams that are already in use:
        // This is obnoxious, but it should still be valid: 
        TeamIdSizeList sortedTeamList;
        buildSortedTeamIdSizeList(teamRoleSizes, sortedTeamList);

        TeamIndexSet indexesToSkip;
        eastl::vector<TeamIdSize>::const_iterator curTeamId = sortedTeamList.begin();
        eastl::vector<TeamIdSize>::const_iterator endTeamId = sortedTeamList.end();
        for (; curTeamId != endTeamId; ++curTeamId)
        {
            TeamId teamId = curTeamId->first;
            TeamSelectionCriteria teamSelectionCriteria;
            teamSelectionCriteria.setRequiredSlotCount((uint16_t)curTeamId->second);

            TeamIndex foundTeamIndex;
            TeamIdRoleSizeMap::const_iterator roleSizes = teamRoleSizes.find(teamId);
            BlazeRpcError err = findOpenTeam(teamSelectionCriteria, roleSizes, nullptr, foundTeamIndex, &indexesToSkip);
            if (err != ERR_OK)
                return err;
            
            indexesToSkip.insert(foundTeamIndex);
            foundTeamIndexes[teamId] = foundTeamIndex;
        }

        return ERR_OK;
    }
    /*! ************************************************************************************************/
    /*! \brief finds a team based on team selection criteria and roles.

        \param[in] teamSelectionCriteria - parameters for selecting the team to join, including the team id,
            required space on team (the total number of players that are joining (for user groups), etc.
            If team id is specified (not ANY_TEAM_ID), finds team of the requested team id.  Otherwise will
            find any available team regardless of team id.
        \param[in] requiredRoleSizes - the mapping of roles to required slots for the joiners
        \param[in] groupUedAdjustmentExpressions - group UED expressions to use when calculating team UED
        \param[in/out] foundTeamIndex - the index of the team found.
            Can be an ANY_TEAM_ID's index, if explicit requestedTeamid not in game (to set it).
        \param[in] teamsToSkip - optional list of team indexes to skip
        \return ERR_OK if acceptable team was found.
    ***************************************************************************************************/
    BlazeRpcError GameSession::findOpenTeam(const TeamSelectionCriteria& teamSelectionCriteria,
        TeamIdRoleSizeMap::const_iterator requiredTeamIdRoleSizes,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions,
        TeamIndex& foundTeamIndex, TeamIndexSet* teamsToSkip) const
    {
        return findOpenTeam(teamSelectionCriteria,
            requiredTeamIdRoleSizes->second, requiredTeamIdRoleSizes->first,
            groupUedAdjustmentExpressions, foundTeamIndex, teamsToSkip);
    }

    BlazeRpcError GameSession::findOpenTeam(const TeamSelectionCriteria& teamSelectionCriteria,
        const RoleSizeMap& requiredRoleSizes, TeamId requestedTeamId,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions,
        TeamIndex& foundTeamIndex, TeamIndexSet* teamsToSkip) const
    {
        const uint16_t requiredSlotCount = teamSelectionCriteria.getRequiredSlotCount();

        // to save us iteration, first check if there are even enough spaces in the game as a whole
        if ((requiredSlotCount + getPlayerRoster()->getParticipantCount()) > getTotalParticipantCapacity())
        {
            return GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL;
        }

        // backup plan: change an ANY_TEAM_ID on game to explicit requestedTeamId
        bool foundOpenUnsetTeam = false;
        uint16_t smallestUnsetTeamSize = UINT16_MAX;
        TeamIndex smallestUnsetTeamIndex = 0;
        BlazeRpcError result = GAMEMANAGER_ERR_TEAM_FULL;

        const TeamIdVector &teamIdVector = getTeamIds();
        // Check to see if we should accept ANY_TEAM_ID teams:
        bool allowDuplicateTeamIds = getGameSettings().getAllowSameTeamId();
        bool foundRequestedTeamId = false;
        for (TeamIndex i = 0; i < teamIdVector.size(); ++i)
        {
            if (teamsToSkip != nullptr && teamsToSkip->find(i) != teamsToSkip->end())
                continue;

            if (teamIdVector[i] == requestedTeamId)
                foundRequestedTeamId = true;
        }

        // first we find the index of this team, need to iterate over all teams in case of repeated team
        bool foundOpenTeam = false;
        uint16_t smallestTeamSize = UINT16_MAX;
        TeamIndex smallestTeamIndex = 0;
        for (TeamIndex i = 0; i < teamIdVector.size(); ++i)
        {
            if (teamsToSkip != nullptr && teamsToSkip->find(i) != teamsToSkip->end())
                continue;

            uint16_t currentTeamSize = getPlayerRoster()->getTeamSize(i);
            SPAM_LOG("[GameSession].findOpenTeam currentTeamSize: " << currentTeamSize << " currentTeamIndex: " << i << " teamCapacity " << getTeamCapacity());
            if ( (requestedTeamId == ANY_TEAM_ID)
                || (teamIdVector[i] == requestedTeamId) )
            {
                if ( (requiredSlotCount + currentTeamSize) <= getTeamCapacity())
                {
                    Blaze::BlazeRpcError err = checkJoinabilityByTeamIndex(requiredRoleSizes, i);
                    if (err == ERR_OK)
                    {
                        if ((currentTeamSize < smallestTeamSize) ||
                            ((currentTeamSize == smallestTeamSize) && chooseBetweenOpenTeams(i, smallestTeamIndex, teamSelectionCriteria, groupUedAdjustmentExpressions))
                           )
                        {
                            SPAM_LOG("[GameSession].findOpenTeam updating smallest team - previous smallestTeamSize: " << smallestTeamSize << " smallestTestIndex: " << smallestTeamIndex);
                            foundOpenTeam = true;
                            smallestTeamIndex = i;
                            smallestTeamSize = currentTeamSize;
                        }
                    }
                    else
                    {
                        // no room for required roles, or invalid role type
                        result = err;
                    }
                }
            }
            else if ( !foundOpenTeam
                && (teamIdVector[i] == ANY_TEAM_ID)
                && (allowDuplicateTeamIds || !foundRequestedTeamId) )
            {
                // backup plan: change this ANY_TEAM_ID to explicit requestedTeamId
                if ( (requiredSlotCount + currentTeamSize) <= getTeamCapacity())
                {
                    Blaze::BlazeRpcError err = checkJoinabilityByTeamIndex(requiredRoleSizes, i);
                    if (err == ERR_OK)
                    {
                        if ((currentTeamSize < smallestUnsetTeamSize) ||
                            ((currentTeamSize == smallestTeamSize) && chooseBetweenOpenTeams(i, smallestTeamIndex, teamSelectionCriteria, groupUedAdjustmentExpressions))
                           )
                        {
                            foundOpenUnsetTeam = true;
                            smallestUnsetTeamIndex = i;
                            smallestUnsetTeamSize = currentTeamSize;
                        }

                    }
                    else
                    {
                        // no room for required roles, or invalid role type
                        result = err;
                    }
                }
            }
        }

        TeamId foundTeamId = INVALID_TEAM_ID;
        if (foundOpenTeam)
        {
            foundTeamIndex = smallestTeamIndex;
            foundTeamId = getTeamIdByIndex(smallestTeamIndex);
            result = ERR_OK;
        }
        else if (foundOpenUnsetTeam)
        {
            foundOpenTeam = true;
            foundTeamIndex = smallestUnsetTeamIndex;
            foundTeamId = requestedTeamId;
            result = ERR_OK;
        }
        SPAM_LOG("[GameSession].foundOpenTeam foundTeamIndex: " << foundTeamIndex << " foundTeamId " << foundTeamId << "requiredSlotCount: " << requiredSlotCount << "requestedTeamId: " << requestedTeamId);
        return result;
    }


    /*! ************************************************************************************************/
    /*! \brief return whether to pick the first team over the second, based on team selection criteria
    ***************************************************************************************************/
    bool GameSession::chooseBetweenOpenTeams(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions) const
    {
        return chooseBetweenOpenTeamsByNextPriority(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, 0);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to select the next team by the next priority selection criteria
    ***************************************************************************************************/
    bool GameSession::chooseBetweenOpenTeamsByNextPriority(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const
    {
        // By default return true if t1 smaller, if ran out of explicitly specified priorities and prior ones all tied.
        if (currPrioritiesIndex >= teamSelectionCriteria.getCriteriaPriorityList().size())
        {
            uint16_t t1size = getPlayerRoster()->getTeamSize(t1);
            uint16_t t2size = getPlayerRoster()->getTeamSize(t2);
            SPAM_LOG("[GameSession].chooseBetweenOpenTeamsByNextPriority defaulting to size - team1Size: " << t1size << " team2Size: " << t2size << " currentPriorityIndex " << currPrioritiesIndex << " listSize: " << teamSelectionCriteria.getCriteriaPriorityList().size());
            return (t1size < t2size);
        }
        switch (teamSelectionCriteria.getCriteriaPriorityList()[currPrioritiesIndex])
        {
        case TEAM_SELECTION_PRIORITY_TYPE_TEAM_BALANCE:
            return chooseBetweenOpenTeamsByBalance(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, currPrioritiesIndex);
        case TEAM_SELECTION_PRIORITY_TYPE_TEAM_UED_BALANCE:
            return chooseBetweenOpenTeamsByUedBalance(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, currPrioritiesIndex);
        case TEAM_SELECTION_PRIORITY_TYPE_TEAM_COMPOSITION:
            return chooseBetweenOpenTeamsByCompositions(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, currPrioritiesIndex);
        default:
            ERR_LOG("[GameSession].chooseBetweenOpenTeamsByNextPriority internal error: unhandled team selection priority type " << TeamSelectionCriteriaPriorityToString(teamSelectionCriteria.getCriteriaPriorityList()[currPrioritiesIndex]));
        };
        return (getPlayerRoster()->getTeamSize(t1) < getPlayerRoster()->getTeamSize(t2));
    }

    /*! ************************************************************************************************/
    /*! \brief helper to select the next team by the team balance
    ***************************************************************************************************/
    bool GameSession::chooseBetweenOpenTeamsByBalance(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const
    {
        uint16_t t1size = getPlayerRoster()->getTeamSize(t1);
        uint16_t t2size = getPlayerRoster()->getTeamSize(t2);
        // tie break by next priority
        SPAM_LOG("[GameSession].chooseBetweenOpenTeamsByBalance team1 size: " << t1size << " team2 size: " << t2size);
        return ((t1size != t2size)? (t1size < t2size) : chooseBetweenOpenTeamsByNextPriority(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, (currPrioritiesIndex+1)));
    }

    /*! ************************************************************************************************/
    /*! \brief return whether to consider team 1 lower-valued than team 2.
    ***************************************************************************************************/
    bool GameSession::chooseBetweenOpenTeamsByUedBalance(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const
    {
        UserExtendedDataKey uedKey = teamSelectionCriteria.getTeamUEDKey();
        GroupValueFormula groupFormula = teamSelectionCriteria.getTeamUEDFormula();
        if (uedKey == INVALID_USER_EXTENDED_DATA_KEY)// disabled
            return chooseBetweenOpenTeamsByNextPriority(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, (currPrioritiesIndex+1));

        // calculate the team UED of the two teams and pick the bottom valued one
        // NOTE: unlike at MM sessions, no need to normalize, this is only used to choose a team consistently.
        UserExtendedDataValue ued1;
        UserExtendedDataValue ued2;
        if (groupUedAdjustmentExpressions)
        {
            ued1 = calcTeamUEDValue(t1, uedKey, groupFormula, *groupUedAdjustmentExpressions);
            ued2 = calcTeamUEDValue(t2, uedKey, groupFormula, *groupUedAdjustmentExpressions);
        }
        else
        {
            Matchmaker::GroupUedExpressionList emptyGroupUedExpressionList;
            ued1 = calcTeamUEDValue(t1, uedKey, groupFormula, emptyGroupUedExpressionList);
            ued2 = calcTeamUEDValue(t2, uedKey, groupFormula, emptyGroupUedExpressionList);
        }

        eastl::string ruleNameKey;
        ruleNameKey.append_sprintf("%u:%s:%s", (uint32_t)currPrioritiesIndex, (teamSelectionCriteria.getTeamUEDRuleName()[0] != '\0' ?
            teamSelectionCriteria.getTeamUEDRuleName() : gUserSessionManager->getUserExtendedDataName(uedKey)), GroupValueFormulaToString(groupFormula));

        // const cast needed to update this cache
        TeamIndexToTeamUedMap &teamUeds = const_cast<TeamIndexToTeamUedMap &>(mTeamUeds);
        teamUeds[t1][ruleNameKey.c_str()] = ued1;
        teamUeds[t2][ruleNameKey.c_str()] = ued2;

        if (ued1 != ued2)
        {
            SPAM_LOG("[GameSession].chooseBetweenOpenTeamsByUedBalance ued1: " << ued1 << " ued2: " << ued2 << " groupFormula: " << GroupValueFormulaToString(groupFormula));
            const bool shouldPickHigherValuedTeam = ((groupFormula == GROUP_VALUE_FORMULA_MIN) ||
                ((groupFormula == GROUP_VALUE_FORMULA_SUM) && (teamSelectionCriteria.getTeamUEDJoiningValue() < 0)) ||
                ((groupFormula == GROUP_VALUE_FORMULA_AVERAGE) && (teamSelectionCriteria.getTeamUEDJoiningValue() < eastl::min(ued1,ued2))));
            return !shouldPickHigherValuedTeam? (ued1 < ued2) : (ued1 > ued2);
        }
        // tie break by next priority
        return chooseBetweenOpenTeamsByNextPriority(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, (currPrioritiesIndex+1));
    }

    /*! ************************************************************************************************/
    /*! \brief helper to select the next team by team composition priority
    ***************************************************************************************************/
    bool GameSession::chooseBetweenOpenTeamsByCompositions(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria,
        const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPriorityQueueIndex) const
    {
        // Try each of the game team compositions until find one that both fits this game, and either of the 2 teams
        // Pre: ordered by non-ascending preference
        const uint16_t joiningSize = teamSelectionCriteria.getRequiredSlotCount();
        for (size_t i = 0; i < teamSelectionCriteria.getAcceptableGameTeamCompositionsList().size(); ++i)
        {
            const GroupSizeCountMapByTeamList& gtcGroupSizeCountsByTeam = *teamSelectionCriteria.getAcceptableGameTeamCompositionsList()[i];
            if (doesGameViolateGameTeamCompositions(gtcGroupSizeCountsByTeam, joiningSize))
                continue;

            const GroupSizeCountMap& team1CompositionGroupSizeCounts = *(gtcGroupSizeCountsByTeam[t1]);
            const GroupSizeCountMap& team2CompositionGroupSizeCounts = *(gtcGroupSizeCountsByTeam[t2]);

            // Now accounting for my join size, see if each team would fit its composition. If both could, tie break by next priority below
            bool t1AndComp1FitsMe = !doesTeamViolateTeamComposition(t1, team1CompositionGroupSizeCounts, joiningSize);
            bool t2AndComp2FitsMe = !doesTeamViolateTeamComposition(t2, team2CompositionGroupSizeCounts, joiningSize);
            if (t1AndComp1FitsMe != t2AndComp2FitsMe)
            {
                eastl::string buf;
                TRACE_LOG("[GameSession].chooseBetweenOpenTeamsByCompositions Chose team(TeamIndex=" << (t1AndComp1FitsMe? t1:t2) << ") out of (TeamIndex1=" << t1 << ", TeamIndex2=" << t2 <<") based on game team compositions criteria " << groupSizeCountMapByTeamListToLogStr(gtcGroupSizeCountsByTeam, buf) << ".");
                return t1AndComp1FitsMe;
            }
            else if (t1AndComp1FitsMe && t2AndComp2FitsMe)
                break;//tie break below
            // else retry on another game team compositions that might fit.
        }

        // tie break by next priority
        TRACE_LOG("[GameSession].chooseBetweenOpenTeamsByCompositions no preferred team found out of (TeamIndex1=" << t1 << ", TeamIndex2=" << t2 <<") based on game team compositions criteria. Continuing to try to tie break by next team selection criteria.");
        return chooseBetweenOpenTeamsByNextPriority(t1, t2, teamSelectionCriteria, groupUedAdjustmentExpressions, (currPriorityQueueIndex+1));
    }

    /*! ************************************************************************************************/
    /*! \brief check whether game's current team's group sizes violate the specified GameTeamCompositions criteria.
        Does not account for future joiners if game not full (ignores *potential* future violating group sizes entirely).

        \param[in] gtcGroupSizeCountsByTeamList group size counts for the team compositions. Game's actual counts must be <=.
        \param[in] joiningSize If non-zero, your joining group size to be accounted for.
        \param[in] joiningTeamIndex If specified, your joining group size is accounted for as joining this team.
    ***************************************************************************************************/
    bool GameSession::doesGameViolateGameTeamCompositions(const GroupSizeCountMapByTeamList& gtcGroupSizeCountsByTeamList,
        uint16_t joiningSize, TeamIndex joiningTeamIndex /*= UNSPECIFIED_TEAM_INDEX*/) const
    {
        if (gtcGroupSizeCountsByTeamList.size() != getTeamCount())
            return true;

        // Pre: each index lines up with its corresponding team index in game
        for (TeamIndex tIndex = 0; tIndex < gtcGroupSizeCountsByTeamList.size(); ++tIndex)
        {
            if (doesTeamViolateTeamComposition(tIndex, *(gtcGroupSizeCountsByTeamList[tIndex]), ((joiningTeamIndex == tIndex)? joiningSize : 0)))
            {
                if (IS_LOGGING_ENABLED(Logging::SPAM))
                {
                    eastl::string buf;
                    SPAM_LOG("[GameSession].doesGameViolateGameTeamCompositions Game(" << getGameId() << ") violates team compositions. Game's team(index=" << tIndex << ")'s with joining player's group size(" << joiningSize << "). Game Team Compositions criteria " << groupSizeCountMapByTeamListToLogStr(gtcGroupSizeCountsByTeamList, buf) << ".");
                }
                return true;
            }
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether Team has any group size present or of count greater than on the team composition's group size counts
    ***************************************************************************************************/
    bool GameSession::doesTeamViolateTeamComposition(TeamIndex teamIndex, const GroupSizeCountMap& teamCompositionGroupSizeCounts,
        uint16_t joiningSize) const
    {
        if (getTeamCapacity() - getPlayerRoster()->getTeamSize(teamIndex) < joiningSize)
            return true;

        GroupSizeCountMap teamActualGroupSizeCounts;
        getPlayerRoster()->getTeamGroupSizeCountMap(teamActualGroupSizeCounts, teamIndex);

        if (joiningSize > 0)
            teamActualGroupSizeCounts[joiningSize] += 1;

        for (GroupSizeCountMap::const_iterator itr = teamActualGroupSizeCounts.begin(); itr != teamActualGroupSizeCounts.end(); ++itr)
        {
            if (itr->second > findGroupSizeCountInMap(itr->first, teamCompositionGroupSizeCounts))
            {
                SPAM_LOG("[GameSession].doesTeamViolateTeamComposition Game(" << getGameId() << ")'s team(index=" << teamIndex << ")'s group size(" << itr->first << ")'s count(" << itr->second << ") " << ", violates composition's group size(" << itr->first << ")'s max count(" << findGroupSizeCountInMap(itr->first, teamCompositionGroupSizeCounts) << "). Team: " << getPlayerRoster()->getTeamAttributesLogStr(teamIndex) << ".");
                return true;
            }
        }

        return false;
    }


    // Return a team's UED value calculated using the specified group value formula, or INVALID_USER_EXTENDED_DATA_VALUE if n/a.
    Blaze::UserExtendedDataValue GameSession::calcTeamUEDValue(TeamIndex teamIndex, UserExtendedDataKey uedKey,
        GroupValueFormula groupFormula, const Matchmaker::GroupUedExpressionList &groupAdjustmentFormulaList, UserExtendedDataValue defaultValue /*= 0*/) const
    {
        PlayerRoster::PlayerInfoList playerList;
        getPlayerRoster()->getPlayers(playerList, PlayerRoster::ACTIVE_PARTICIPANTS);
        if (playerList.empty())
        {
            return defaultValue;
        }

        // get arbitrary map, as team leader is effectively just max
        UserSessionInfoList infoList(DEFAULT_BLAZE_ALLOCATOR, "TempUserInfoList");
        infoList.reserve(playerList.size());
        for (PlayerRoster::PlayerInfoList::iterator itr = playerList.begin(), end = playerList.end(); itr != end; ++itr)
        {
            PlayerInfo* playerInfo = (*itr);
            if (playerInfo->getTeamIndex() != teamIndex)
                continue;
            uint16_t groupSize = getPlayerRoster()->getSizeOfGroupOnTeam(teamIndex, playerInfo->getUserGroupId());
            UserSessionInfo& sessionInfo = playerInfo->getUserInfo();
            sessionInfo.setGroupSize(groupSize);
            infoList.push_back(&sessionInfo);
        }

        // calc UED value for team.
        UserExtendedDataValue teamUEDValue = defaultValue;
        if (!infoList.empty())
        {
             if (!(*Matchmaker::GroupValueFormulas::getCalcGroupFunction(groupFormula))(uedKey, groupAdjustmentFormulaList, infoList.front()->getUserInfo().getId(), infoList.front()->getGroupSize(), infoList, teamUEDValue)) /*lint !e613 */
             {
                 WARN_LOG("[GameSession].calcTeamUEDValue Game(" << getGameId() << ") team(index=" << teamIndex << ") UED Value for UED key " << uedKey << " failed to be calculated. Team members may not have had valid UED values on their user sessions available.");
             }
        }
        return teamUEDValue;
    }

    /*!************************************************************************************************/
    /*! \brief finds a role that can be assigned as the 'any role' for the player that desires multiple roles
            We only handle players that desire multiple roles here as single role players are validated prior to this
            and players that desire ANY role don't require this validation.
        \param[in] playerRoles - A mapping of PlayerIds and their desired roles
        \param[in] joiningTeamIndex - Team index for the joining team
        \param[in/out] roleCounts - Tracks how many players have been 'added' to the game and at what team/role
            such that role capacity and criteria validation works
    ***************************************************************************************************/
    BlazeRpcError GameSession::findOpenRole(const PlayerToRoleNameMap& playerRoles, TeamIndex joiningTeamIndex, TeamIndexRoleSizeMap& roleCounts) const
    {
        const RoleCriteriaMap &roleCriteriaMap = getRoleInformation().getRoleCriteriaMap();

        for (const auto& roles : playerRoles)
        {
            PlayerRoleList desiredRoles;
            const PlayerId playerId = roles.first;
            if (roles.second.size() > 1)
            {
                desiredRoles.insert(desiredRoles.begin(), roles.second.begin(), roles.second.end()); // Need a copy so we can shuffle it randomly

                if (!desiredRoles.empty())
                {
                    // Randomize our desired roles so that we don't inadvertently prefer some roles over others
                    // There's a case to be made here for ordering the roles based on which roles are least desired amongst the group
                    // such that we're more likely to choose roles that won't compete with other players
                    // This can be looked as a future enhancement should a random selection prove unsatisfactory
                    eastl::random_shuffle(desiredRoles.begin(), desiredRoles.end(), Random::getRandomNumber);

                    auto desiredRoleItr = desiredRoles.begin();
                    for (; desiredRoleItr != desiredRoles.end(); ++desiredRoleItr)
                    {
                        RoleCriteriaMap::const_iterator roleCriteriaItr = roleCriteriaMap.find(*desiredRoleItr);
                        if (roleCriteriaItr != roleCriteriaMap.end())
                        {
                            if (validateOpenRole(playerId, joiningTeamIndex, roleCriteriaItr, roleCounts))
                                break;
                        }
                    }

                    if (desiredRoleItr == desiredRoles.end())
                        return GAMEMANAGER_ERR_ROLE_FULL;
                }
            }
        }

        return ERR_OK;
    }

    bool GameSession::validateOpenRole(PlayerId playerId, TeamIndex joiningTeamIndex, const RoleCriteriaMap::const_iterator& roleCriteriaItr, TeamIndexRoleSizeMap& roleCounts) const
    {
        const PlayerRoster *playerRoster = getPlayerRoster();
        const RoleName &currentRoleName = roleCriteriaItr->first;
        const RoleCriteria *roleCriteria = roleCriteriaItr->second;
        const uint16_t roleCapacity = roleCriteria->getRoleCapacity();

        auto reqItr = roleCounts[joiningTeamIndex].insert(eastl::make_pair<const RoleName&, uint16_t>(currentRoleName, 0)).first;
        uint16_t numInRole = playerRoster->getRoleSize(joiningTeamIndex, currentRoleName) + reqItr->second;

        // ensure enough room for possible remaining joining group members, before picking this role as an any role
        if (roleCapacity > numInRole)
        {
            // make sure that this role selection doesn't violate multirole criteria
            // Role entry criteria is checked later on as part of isGamePotentiallyJoinable()
            ++reqItr->second;
            EntryCriteriaName failedEntryCriteria;
            if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(playerRoster, roleCounts[joiningTeamIndex], joiningTeamIndex, failedEntryCriteria))
            {
                SPAM_LOG("[GameSession].validateOpenRole: Open role " << currentRoleName << " failed multirole entry criteria to enter game(" << getGameId()
                    << "), failedEntryCriteria(" << failedEntryCriteria << "), trying next role.");
            }
            else
            {
                TRACE_LOG("[GameSession].validateOpenRole: Role(" << currentRoleName << ") found for player(" << playerId << "), in game(" << getGameId() << ").");
                return true;
            }

            if (reqItr->second > 0)
                --reqItr->second;
        }

        TRACE_LOG("[GameSession].validateOpenRole: Role(" << currentRoleName << ") is not available for player(" << playerId << "), in game(" << getGameId() << ").");
        return false;
    }

    bool GameSession::isPresenceDisabledForPlatform(ClientPlatformType platform) const
    {
        for (const auto& plat : getPresenceDisabledList())
        {
            if (plat == platform)
                return true;
        }
        return false;
    }

}// namespace GameManager
}// namespace Blaze
