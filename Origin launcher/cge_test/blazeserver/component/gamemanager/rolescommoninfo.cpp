/*! ************************************************************************************************/
/*!
\file   rolescommoninfo.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/rolescommoninfo.h"
#include "gamemanager/playerinfo.h" // for isParticipantSlot()
#include "framework/usersessions/usersessionmanager.h" // for gUserSessionManager in lookupPlayerRoleName()
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/rpc/gamemanager_defines.h"

namespace Blaze
{
namespace GameManager
{

    uint16_t lookupRoleCount(const RoleNameToPlayerMap& roleRosters, const RoleName& name, uint16_t numJoiningPlayers)
    {
        if ((roleRosters.size()== 1) && roleRosters.begin()->first == name && (roleRosters.begin()->second->front() == INVALID_BLAZE_ID))
        {
            // only one role in map, with no player assignments, this is shortcut to assign all joiners to same role,
            // which means you need to know the number of players.
            return numJoiningPlayers;
        }

        // Technically, the role roster may contain ids that aren't part of the joining player list. That should be verified elsewhere.
        RoleNameToPlayerMap::const_iterator roleIter = roleRosters.find(name);
        if (roleIter != roleRosters.end())
            return roleIter->second->size();

        return 0;
    }

    const RoleName* lookupPlayerRoleName(const RoleNameToPlayerMap& roleRosters, PlayerId playerId)
    {
        if ((roleRosters.size()== 1) && (roleRosters.begin()->second->front() == INVALID_BLAZE_ID))
        {
            // only one role in map, with no player assignments, this is shortcut to assign all joiners to same role
            return &(roleRosters.begin()->first);
        }
        //iterate over the roleRosters to find the player
        RoleNameToPlayerMap::const_iterator roleIter = roleRosters.begin();
        RoleNameToPlayerMap::const_iterator roleEnd = roleRosters.end();
        for (; roleIter != roleEnd; ++roleIter)
        {
            // check this role
            PlayerIdList::const_iterator playerIter = roleIter->second->begin();
            PlayerIdList::const_iterator playerEnd = roleIter->second->end();
            for (; playerIter != playerEnd; ++playerIter)
            {
                if (*playerIter == playerId)
                {
                    return &(roleIter->first);
                }
            }
        }

        // couldn't locate player
        return nullptr;
    }

    const char8_t* lookupPlayerRoleName(const PlayerJoinData& playerJoinData, PlayerId playerId)
    {
        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            // If player is found, return a non-nullptr value: 
            if ((*playerIter)->getUser().getBlazeId() == playerId)
            {
                if ((*playerIter)->getRole()[0] != '\0')
                {
                    return (*playerIter)->getRole();
                }
                else
                {
                    return playerJoinData.getDefaultRole();
                }
            }
        }

        // We don't return nullptr because there are too many edge cases where Group members may be missing information.
        // (That means that any calls to lookupPlayerRoleName need to actually use valid playerIds)
        return playerJoinData.getDefaultRole();
    }

    const PerPlayerJoinData* lookupPerPlayerJoinDataConst(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData)
    {
        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            // compare non-INVALID_BLAZE_IDs or try external identifications.
            if (((userInfoData.getId() != INVALID_BLAZE_ID) && ((*playerIter)->getUser().getBlazeId() == userInfoData.getId()))
                || isExternalUserIdentificationForPlayer((*playerIter)->getUser(), userInfoData))
            {
                return (*playerIter);
            }
        }
       
        // couldn't locate player, just use the default role if no-one else has a role.
        return nullptr;
    }

    PerPlayerJoinData* lookupPerPlayerJoinData(PlayerJoinData& playerJoinData, const UserInfoData& userInfoData)
    {
        return const_cast<PerPlayerJoinData*>(lookupPerPlayerJoinDataConst(playerJoinData, userInfoData));
    }

    const char8_t* lookupExternalPlayerRoleName(const PlayerJoinData& playerJoinData,
        const UserInfoData& playerUserInfo)
    {
        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            if (isExternalUserIdentificationForPlayer((*playerIter)->getUser(), playerUserInfo))
            {
                // Player was found, so return a non-nullptr value:
                if ((*playerIter)->getRole()[0] != '\0')
                {
                    return (*playerIter)->getRole();
                }
                else
                {
                    return playerJoinData.getDefaultRole();
                }
            }
        }

        // We don't return nullptr because there are too many edge cases where Group members may be missing information.
        // (That means that any calls to lookupPlayerRoleName need to actually use valid playerIds)
        return playerJoinData.getDefaultRole();
    }

    /*! \brief Retrieve user's role from the request's external or non-external players role roster depending on whether its an external user */
    const char8_t* lookupPlayerRoleName(const PlayerJoinData &playerJoinData, const UserSessionInfo &joiningUserInfo)
    {
        const PerPlayerJoinData* playerData = lookupPerPlayerJoinDataConst(playerJoinData, joiningUserInfo.getUserInfo());
        if (playerData != nullptr && playerData->getRole()[0] != '\0')
            return playerData->getRole();

        return playerJoinData.getDefaultRole();
    }

    void buildSortedTeamIdSizeList(const TeamIdRoleSizeMap &teamRoleSizes, TeamIdSizeList& sortedTeamList)
    {
        TeamIdRoleSizeMap::const_iterator curTeam = teamRoleSizes.begin();
        TeamIdRoleSizeMap::const_iterator endTeam = teamRoleSizes.end();
        for (; curTeam != endTeam; ++curTeam)
        {
            TeamIdSize teamSize(curTeam->first, (uint16_t)0);

            RoleSizeMap::const_iterator roleIter = curTeam->second.begin();
            RoleSizeMap::const_iterator roleEnd = curTeam->second.end();
            for (; roleIter != roleEnd; ++roleIter)
                teamSize.second += roleIter->second;

            sortedTeamList.push_back(teamSize);
        }
        // pair comparison sorts by first, then second. 
        eastl::sort(sortedTeamList.begin(), sortedTeamList.end(), TeamIdSizeSortComparitor());
    }

    SlotType getSlotType(const PlayerJoinData& playerJoinData, SlotType memberSlotType)
    {
        return (memberSlotType == INVALID_SLOT_TYPE) ? playerJoinData.getDefaultSlotType() : memberSlotType;
    }


    SlotType getSlotType(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData)
    {
        const PerPlayerJoinData* playerData = lookupPerPlayerJoinDataConst(playerJoinData, userInfoData);
        return getSlotType(playerJoinData, playerData ? playerData->getSlotType() : INVALID_SLOT_TYPE);
    }


    // note: for non-MM joins optional players are never required, so includeOptionalPlayers should be set to false.
    // For MM joins, their roles are required, so includeOptionalPlayers should be used. 
    void buildTeamIdRoleSizeMap(const PlayerJoinData &playerJoinData, TeamIdRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers)
    {
        if (playerJoinData.getPlayerDataList().empty())
        {
            return;
        }

        RoleName defaultRole = playerJoinData.getDefaultRole();
        TeamId defaultTeamId = playerJoinData.getDefaultTeamId();

        // Number of players that we've set data for:
        uint16_t playerCountSet = 0;

        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            const char* role = (*playerIter)->getRole();
            TeamId teamId = (*playerIter)->getTeamId();
            SlotType slotType = getSlotType(playerJoinData, (*playerIter)->getSlotType());
            if (!isParticipantSlot(slotType))
                continue;

            // Optional players are used in MM requirements, but not in normal join requirements.
            if (includeOptionalPlayers || !(*playerIter)->getIsOptionalPlayer())
            {
                if (role == nullptr || role[0] == '\0')
                    role = defaultRole;
                    
                if (teamId == UNSPECIFIED_TEAM_INDEX)
                    teamId = defaultTeamId;

                teamRoleRequirements[teamId][role] += 1;
                ++playerCountSet;
            }
        }

        if (joiningPlayerCount > playerCountSet)
        {
            teamRoleRequirements[defaultTeamId][defaultRole] += (joiningPlayerCount - playerCountSet);
        }
        else if (joiningPlayerCount < playerCountSet && !includeOptionalPlayers)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "buildTeamIdRoleSizeMap: unexpected internal error: supplied joining player count (" << joiningPlayerCount << ") is less than the per-player join data count (" << playerCountSet << ") (not including optional users).");
        }
    }

    // Build a map tracking which roles each player requires. Used by the roles rule to determine our RETE conditions
    void buildTeamIdPlayerRoleMap(const PlayerJoinData &playerJoinData, TeamIdPlayerRoleNameMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers)
    {
        if (playerJoinData.getPlayerDataList().empty())
        {
            return;
        }

        RoleName defaultRole = playerJoinData.getDefaultRole();
        TeamId defaultTeamId = playerJoinData.getDefaultTeamId();

        // Number of players that we've set data for:
        uint16_t playerCountSet = 0;

        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            const char* role = (*playerIter)->getRole();
            const RoleNameList& roles = (*playerIter)->getRoles();
            TeamId teamId = (*playerIter)->getTeamId();
            PlayerId playerId = (*playerIter)->getUser().getBlazeId();
            SlotType slotType = getSlotType(playerJoinData, (*playerIter)->getSlotType());
            if (!isParticipantSlot(slotType))
                continue;

            // Optional players are used in MM requirements, but not in normal join requirements.
            if (includeOptionalPlayers || !(*playerIter)->getIsOptionalPlayer())
            {
                if (role == nullptr || role[0] == '\0')
                    role = defaultRole;

                if (teamId == UNSPECIFIED_TEAM_INDEX)
                    teamId = defaultTeamId;

                PlayerToRoleNameMap& playerRoles = teamRoleRequirements[teamId];
                PlayerToRoleNameMap::iterator it = playerRoles.find(playerId);
                PlayerRoleList* roleList = nullptr;
                if (it == playerRoles.end())
                {
                    auto inserter = playerRoles.insert(playerId).first;
                    roleList = &inserter->second;
                }
                else
                    roleList = &it->second;

                if (blaze_stricmp(role, PLAYER_ROLE_NAME_ANY) != 0 || roles.empty())
                {
                    roleList->push_back(role);
                }
                else
                {
                    roleList->insert(roleList->begin(), roles.begin(), roles.end());
                }

                // Sort the roles in alphabetical order so that we'll generate the correct keys as part of the role rule diagnostics.
                eastl::sort(roleList->begin(), roleList->end(), CaseInsensitiveStringLessThan());

                ++playerCountSet;
            }
        }

        if (joiningPlayerCount < playerCountSet && !includeOptionalPlayers)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "buildTeamIdPlayerRoleMap: unexpected internal error: supplied joining player count (" << joiningPlayerCount << ") is less than the per-player join data count (" << playerCountSet << ") (not including optional users).");
        }
    }

    void buildTeamIndexRoleSizeMap(const PlayerJoinData &playerJoinData, TeamIndexRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers)
    {
        if (playerJoinData.getPlayerDataList().empty())
        {
            return;
        }

        RoleName defaultRole = playerJoinData.getDefaultRole();
        TeamIndex defaultTeamIndex = playerJoinData.getDefaultTeamIndex();

        // Number of players that we've set data for:
        uint16_t playerCountSet = 0;

        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            const char* role = (*playerIter)->getRole();
            TeamIndex teamIndex = (*playerIter)->getTeamIndex();
            SlotType slotType = getSlotType(playerJoinData, (*playerIter)->getSlotType());
            if (!isParticipantSlot(slotType))
                continue;

            // Optional players are used in MM requirements, but not in normal join requirements.
            if (includeOptionalPlayers || !(*playerIter)->getIsOptionalPlayer())
            {
                if (role == nullptr || role[0] == '\0')
                    role = defaultRole;
                    
                if (teamIndex == UNSPECIFIED_TEAM_INDEX)
                    teamIndex = defaultTeamIndex;

                teamRoleRequirements[teamIndex][role] += 1;
                ++playerCountSet;
            }
        }

        if (joiningPlayerCount > playerCountSet)
        {
            teamRoleRequirements[defaultTeamIndex][defaultRole] += (joiningPlayerCount - playerCountSet);
        }
        else if (joiningPlayerCount < playerCountSet && !includeOptionalPlayers)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "buildTeamIndexRoleSizeMap: unexpected internal error: supplied joining player count (" << joiningPlayerCount << ") is less than the per-player join data count (" << playerCountSet << ") (not including optional users).");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief if a role join roster is empty, this will fill it out with the default role setup.
          Single RoleName, "" and one entry for BlazeId, INVALID_BLAZE_ID

        \param[in\out] roleRosters - the joining role roster to set up
    ***************************************************************************************************/
    void setUpDefaultJoiningRoles(RoleNameToPlayerMap &roleRosters)
    {
        if (roleRosters.empty())
        {
            PlayerIdList *defaultRoleRoster = roleRosters.allocate_element();
            defaultRoleRoster->push_back(INVALID_BLAZE_ID);
            roleRosters[PLAYER_ROLE_NAME_DEFAULT] = defaultRoleRoster;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief if a role join roster is empty, this will fill it out with the default role setup.
          Single RoleName, "" with capacity equaling the provided teamCapacity

        \param[in\out] roleInformation - the RoleInformation to set up if uninitalized
        \param[in] teamCapacity - the size of teams in the game session
    ***************************************************************************************************/
    void setUpDefaultRoleInformation(RoleInformation &roleInformation, uint16_t teamCapacity)
    {
        if (roleInformation.getRoleCriteriaMap().empty())
        {
            RoleCriteria *defaultRoleCriteria = roleInformation.getRoleCriteriaMap().allocate_element();
            uint16_t defaultRoleSize = teamCapacity;
            defaultRoleCriteria->setRoleCapacity(defaultRoleSize);
            roleInformation.getRoleCriteriaMap()[GameManager::PLAYER_ROLE_NAME_DEFAULT] = defaultRoleCriteria;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief if the user with the specified id has its role specified in the input role join roster,
            copy/add its entry to the other roster.
    ***************************************************************************************************/
    void copyPlayerRoleRosterEntry(BlazeId playerId, const RoleNameToPlayerMap& orig, PlayerJoinData& playerJoinData)
    {
        const RoleName* roleName = lookupPlayerRoleName(orig, playerId);
        if (roleName)
        {
            bool foundPlayer = false;
            PerPlayerJoinDataList::iterator curIter = playerJoinData.getPlayerDataList().begin();
            PerPlayerJoinDataList::iterator endIter = playerJoinData.getPlayerDataList().end();
            for (; curIter != endIter; ++curIter)
            {
                if ((*curIter)->getUser().getBlazeId() == playerId)
                {
                    (*curIter)->setRole(*roleName);
                    foundPlayer = true;
                    break;
                }
            }

            if (!foundPlayer)
            {
                PerPlayerJoinData* newPlayerData = playerJoinData.getPlayerDataList().pull_back();
                newPlayerData->getUser().setBlazeId(playerId);
                newPlayerData->setRole(*roleName);
            }

        }
    }

    void copyPlayerJoinDataEntry(const UserInfoData& userInfo, const PlayerJoinData& origData, PlayerJoinData& newJoinData)
    {
        // Players may not be part of the original PlayerJoinData if they were in a joining group. In that case, add the BlazeId alone. 
        const PerPlayerJoinData* origPlayerData = lookupPerPlayerJoinDataConst(origData, userInfo);
        PerPlayerJoinData* newPlayerData = lookupPerPlayerJoinData(newJoinData, userInfo);
        if (newPlayerData == nullptr)
        {
            newPlayerData = newJoinData.getPlayerDataList().pull_back();
        }

        if (origPlayerData)
        {
            origPlayerData->copyInto(*newPlayerData);
        }
        else
        {
            newPlayerData->getUser().setBlazeId(userInfo.getId());
            newPlayerData->setRole(origData.getDefaultRole());
        }
    }



    /*! ************************************************************************************************/
    /*! \brief if the user with the specified id has its role specified in the external join roster,
            copy/add its entry to the non-external role roster. Used to track a user originally specified
            in an external players list, in the same maps as 'regular player' for all role roster cap/criteria checks.
        \return pointer to the join roster player's entry got added to. nullptr, if it had no role found in original roster.
    ***************************************************************************************************/
    PerPlayerJoinData* externalRoleRosterEntryToNonExternalRoster(const UserInfoData& userInfo, PlayerJoinData& playerJoinData)
    {
        PerPlayerJoinDataList::iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            if (isExternalUserIdentificationForPlayer((*playerIter)->getUser(), userInfo))
            {
                (*playerIter)->setIsOptionalPlayer(false);
                return *playerIter;
            }
        }
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Ensures that the role information present in the join request is valid and
            updates the role information for use internally
        \return error indicating if validation was successful
    ***************************************************************************************************/
    BlazeRpcError fixupPlayerJoinDataRoles(PlayerJoinData& playerJoinData, bool singleRoleRequired, int32_t logCategory, EntryCriteriaError* errorMsg)
    {
        PerPlayerJoinDataList &ppJoinDataList = playerJoinData.getPlayerDataList();
        eastl::string msg;
        for (auto& ppJoinData : ppJoinDataList)
        {
            if (!ppJoinData->getRoles().empty())
            {
                if (ppJoinData->getRole()[0] != '\0')
                {
                    // Legacy Role and RoleNameList are mutually exclusive
                    msg.append("Both RoleName and RoleNames list cannot be specified!");
                    BLAZE_WARN_LOG(logCategory, msg.c_str());
                    if (errorMsg != nullptr)
                        errorMsg->setFailedCriteria(msg.c_str());
                    return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
                }

                size_t roleListSize = ppJoinData->getRoles().size();
                if (singleRoleRequired && roleListSize > 1)
                {
                    // For non-MM requests, only a single role is supported
                    msg.append("Multiple roles are not supported outside of Matchmaking!");
                    BLAZE_WARN_LOG(logCategory, msg.c_str());
                    if (errorMsg != nullptr)
                        errorMsg->setFailedCriteria(msg.c_str());
                    return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
                }

                if (roleListSize == 1)
                {
                    // Single role, set the singular role name and clear the list
                    ppJoinData->setRole(ppJoinData->getRoles()[0]);
                    ppJoinData->getRoles().clear();
                }
                else
                {
                    // Check that if there are multiple roles, PLAYER_ROLE_NAME_ANY isn't one of them
                    for (const auto& role : ppJoinData->getRoles())
                    {
                        if (blaze_stricmp(role, PLAYER_ROLE_NAME_ANY) == 0)
                        {
                           msg.append_sprintf("%s can not be specified with other roles!", PLAYER_ROLE_NAME_ANY);
                            BLAZE_WARN_LOG(logCategory, msg.c_str());
                            if (errorMsg != nullptr)
                                errorMsg->setFailedCriteria(msg.c_str());
                            return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
                        }
                    }

                    // Multiple roles, set the singular role name to PLAYER_ROLE_NAME_ANY
                    ppJoinData->setRole(PLAYER_ROLE_NAME_ANY);
                }
            }
        }

        return ERR_OK;
    }

} // namespace GameManager
} // namespace Blaze

