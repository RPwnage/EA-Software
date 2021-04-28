/*! ************************************************************************************************/
/*!
    \file playerjoindatahelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"
#include "BlazeSDK/gamemanager/game.h"
#include "BlazeSDK/gamemanager/player.h"

namespace Blaze
{
namespace GameManager
{

PlayerJoinDataHelper::PlayerJoinDataHelper(const Game* game, bool initForMatchmaking)
{
    if (initForMatchmaking)
    {
        initFromGameForMatchmaking(game);
    }
    else
    {
        initFromGameForCreateOrJoin(game);
    }
}


void PlayerJoinDataHelper::initFromGameForMatchmaking(const Game* game, bool copyTeams, bool copyRoles, bool copyPlayerAttributes)
{ 
    initFromGameForCreateOrJoin(game, copyTeams, copyRoles, copyPlayerAttributes, false, false);
}

void PlayerJoinDataHelper::initFromGameForCreateOrJoin(const Game* game, bool copyTeams, bool copyRoles, bool copyPlayerAttributes, bool copySlotType, bool includeSpectators)
{
    uint16_t playerCount = game->getActivePlayerCount();
    for (uint16_t curPlayerIndex = 0; curPlayerIndex < playerCount; ++curPlayerIndex)
    {
        Player* player = game->getActivePlayerByIndex(curPlayerIndex);
        if (player->isQueued() || player->isReserved())
            continue;

        if (player->isSpectator() && includeSpectators == false)
            continue;

        PerPlayerJoinData* perPlayerJoinData = addPlayer(player->getId());
        if (copyRoles)
        {
            perPlayerJoinData->getRoles().push_back(player->getRoleName());
        }

        if (copyTeams)
        {
            TeamId teamId = game->getTeamIdByIndex(player->getTeamIndex());
            perPlayerJoinData->setTeamId(teamId);
            perPlayerJoinData->setTeamIndex(player->getTeamIndex());
        }
            
        if (copyPlayerAttributes)
        {
            player->getPlayerAttributeMap().copyInto(perPlayerJoinData->getPlayerAttributes());
        }

        if (copySlotType)
        {
            perPlayerJoinData->setSlotType(player->getSlotType());
        }
    }
}

PerPlayerJoinData* PlayerJoinDataHelper::findPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId)
{
    if (playerId == INVALID_BLAZE_ID)
        return nullptr;

    PerPlayerJoinDataList::iterator curIter = joinData.getPlayerDataList().begin();
    PerPlayerJoinDataList::iterator endIter = joinData.getPlayerDataList().end();
    for (; curIter != endIter; ++curIter)
    {
        if ((*curIter)->getUser().getBlazeId() == playerId)
        {
            return (*curIter);
        }
    }

    return nullptr;
}
PerPlayerJoinData* PlayerJoinDataHelper::findPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user)
{
    if (isUserIdDefault(user))
        return nullptr;

    PerPlayerJoinDataList::iterator curIter = joinData.getPlayerDataList().begin();
    PerPlayerJoinDataList::iterator endIter = joinData.getPlayerDataList().end();
    for (; curIter != endIter; ++curIter)
    {
        if (areUserIdsEqual((*curIter)->getUser(), user))
        {
            return (*curIter);
        }
    }

    return nullptr;
}
PerPlayerJoinData* PlayerJoinDataHelper::getPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId)
{
    if (playerId == INVALID_BLAZE_ID)
        return nullptr;

    PerPlayerJoinData* newPlayerData = findPlayerJoinData(joinData, playerId);
    if (newPlayerData == nullptr)
    {
        newPlayerData = joinData.getPlayerDataList().pull_back();
        newPlayerData->getUser().setBlazeId(playerId);
    }
    return newPlayerData;
}
PerPlayerJoinData* PlayerJoinDataHelper::getPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user)
{
    if (isUserIdDefault(user))
        return nullptr;

    PerPlayerJoinData* newPlayerData = findPlayerJoinData(joinData, user);
    if (newPlayerData == nullptr)
    {
        newPlayerData = joinData.getPlayerDataList().pull_back();
        user.copyInto(newPlayerData->getUser());
    }
    else
    {
        // Update any part of the data that may be incomplete (allows for external data to flow in)
        updateUserId(user, newPlayerData->getUser());
    }
    return newPlayerData;
}

PerPlayerJoinData* PlayerJoinDataHelper::addDummyPlayer(const RoleName* roleName, TeamId teamId)
{
    PerPlayerJoinDataPtr playerData = getPlayerDataList().pull_back();
    if (roleName != nullptr)
    {
        playerData->getRoles().push_back(*roleName);
    }
    playerData->setTeamId(teamId);

    return playerData;
}

PerPlayerJoinData* PlayerJoinDataHelper::addPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId, const RoleName* roleName /*= nullptr*/, const Collections::AttributeMap *playerAttrs /*= nullptr*/)
{
    UserIdentification userIdent;
    userIdent.setBlazeId(playerId);

    return addPlayerJoinData(joinData, userIdent, roleName, playerAttrs);
}
PerPlayerJoinData* PlayerJoinDataHelper::addPlayerJoinData(PlayerJoinData& joinData, UserManager::User* user, const RoleName* roleName, const Collections::AttributeMap *playerAttrs)
{
    if (user == nullptr)
    {
        return nullptr;
    }

    UserIdentification userIdent;
    user->copyIntoUserIdentification(userIdent);

    return addPlayerJoinData(joinData, userIdent, roleName, playerAttrs);
}
PerPlayerJoinData* PlayerJoinDataHelper::addPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user, const RoleName* roleName /*= nullptr*/, const Collections::AttributeMap *playerAttrs /*= nullptr*/, bool isOptionalUser /*= false*/)
{
    PerPlayerJoinData* newPlayerData = getPlayerJoinData(joinData, user);
    if (newPlayerData == nullptr)
        return nullptr;

    // Update the data:  (User data is already updated in getPlayerJoinData)
    if (roleName)
        newPlayerData->getRoles().push_back(*roleName);
    if (playerAttrs)
        playerAttrs->copyInto(newPlayerData->getPlayerAttributes());
    if (isOptionalUser)
        newPlayerData->setIsOptionalPlayer(isOptionalUser);         // Note: The only way to make a user non-optional once it's set to optional is to call getPlayerJoinData directly. 

    return newPlayerData;
}

bool PlayerJoinDataHelper::isUserIdDefault(const UserIdentification& id1)
{
    UserIdentification defaultValue;
    return areUserIdsEqual(id1, defaultValue);
}

bool PlayerJoinDataHelper::areUserIdsEqual(const UserIdentification& id1, const UserIdentification& id2)
{
    // The fields below should only be compared if they are not invalid. If they are invalid for even one of the ids, we ignore them as some times the information is incomplete. However, if they are valid, then
    // they should match for both the ids. If all the fields are invalid, the ids compare as equal (invalid ids). 

    bool id1Invalid = (id1.getBlazeId() == INVALID_BLAZE_ID) && (id1.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID) && (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID) 
        && (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        && (id1.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
        && (!id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty()) && (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID);
    bool id2Invalid = (id2.getBlazeId() == INVALID_BLAZE_ID) && (id2.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID) && (id2.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID) 
        && (id2.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
        && (id2.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
        && (!id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty()) && (id2.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID);

    // Two completely invalid ids are considered equal.
    if (id1Invalid && id2Invalid)
    {
        return true;
    }

    // Now, we ignore any field where one of the ids has it as invalid as we might be dealing with incomplete info. But if the field is valid in both the ids, they must be equal.
    // We also keep a counter to make sure that at least one of the fields matched. This is to avoid scenarios similar to following.
    //
    // Field        Id1     Id2
    // Field 1     invalid  valid
    // Field 2     valid    invalid
    // Field 3     invalid  valid
    // 
    // And we do not make an early return as soon as one field matches as we want to test all the fields if they are valid.

    uint16_t fieldsMatched = 0;
    if (id1.getBlazeId() != INVALID_BLAZE_ID && id2.getBlazeId() != INVALID_BLAZE_ID)
    {
        if (id1.getBlazeId() != id2.getBlazeId())
        {
            return false; // If both fields are valid and they do not compare as equal, the ids are not equal.
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
    {
        if (id1.getPlatformInfo().getExternalIds().getXblAccountId() != id2.getPlatformInfo().getExternalIds().getXblAccountId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
    {
        if (id1.getPlatformInfo().getExternalIds().getPsnAccountId() != id2.getPlatformInfo().getExternalIds().getPsnAccountId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
    {
        if (id1.getPlatformInfo().getExternalIds().getSteamAccountId() != id2.getPlatformInfo().getExternalIds().getSteamAccountId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID && id2.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
    {
        if (id1.getPlatformInfo().getExternalIds().getStadiaAccountId() != id2.getPlatformInfo().getExternalIds().getStadiaAccountId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (!id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty() && !id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
    {
        if (id1.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() != id2.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && id2.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
    {
        if (id1.getPlatformInfo().getEaIds().getNucleusAccountId() != id2.getPlatformInfo().getEaIds().getNucleusAccountId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    if (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID && id2.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
    {
        if (id1.getPlatformInfo().getEaIds().getOriginPersonaId() != id2.getPlatformInfo().getEaIds().getOriginPersonaId())
        {
            return false;
        }
        else
        {
            ++fieldsMatched;
        }
    }

    return (fieldsMatched > 0);
}
void PlayerJoinDataHelper::updateUserId(const UserIdentification& newId, UserIdentification& outUserId)
{
    // If two ids match, we may still need to update the external data: 
    if (newId.getBlazeId() != INVALID_BLAZE_ID && newId.getBlazeId() != outUserId.getBlazeId())
    {
        outUserId.setBlazeId(newId.getBlazeId());
    }

    if (newId.getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && newId.getPlatformInfo().getExternalIds().getXblAccountId() != outUserId.getPlatformInfo().getExternalIds().getXblAccountId())
    {
        outUserId.getPlatformInfo().getExternalIds().setXblAccountId(newId.getPlatformInfo().getExternalIds().getXblAccountId());
    }
    if (newId.getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && newId.getPlatformInfo().getExternalIds().getPsnAccountId() != outUserId.getPlatformInfo().getExternalIds().getPsnAccountId())
    {
        outUserId.getPlatformInfo().getExternalIds().setPsnAccountId(newId.getPlatformInfo().getExternalIds().getPsnAccountId());
    }
    if (newId.getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && newId.getPlatformInfo().getExternalIds().getSteamAccountId() != outUserId.getPlatformInfo().getExternalIds().getSteamAccountId())
    {
        outUserId.getPlatformInfo().getExternalIds().setSteamAccountId(newId.getPlatformInfo().getExternalIds().getSteamAccountId());
    }
    if (newId.getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID && newId.getPlatformInfo().getExternalIds().getStadiaAccountId() != outUserId.getPlatformInfo().getExternalIds().getStadiaAccountId())
    {
        outUserId.getPlatformInfo().getExternalIds().setStadiaAccountId(newId.getPlatformInfo().getExternalIds().getStadiaAccountId());
    }
    if (!newId.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty() && newId.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() != outUserId.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString())
    {
        outUserId.getPlatformInfo().getExternalIds().setSwitchId(newId.getPlatformInfo().getExternalIds().getSwitchId());
    }
    if (newId.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && newId.getPlatformInfo().getEaIds().getNucleusAccountId() != outUserId.getPlatformInfo().getEaIds().getNucleusAccountId())
    {
        outUserId.getPlatformInfo().getEaIds().setNucleusAccountId(newId.getPlatformInfo().getEaIds().getNucleusAccountId());
    }
    if (newId.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID && newId.getPlatformInfo().getEaIds().getOriginPersonaId() != outUserId.getPlatformInfo().getEaIds().getOriginPersonaId())
    {
        outUserId.getPlatformInfo().getEaIds().setOriginPersonaId(newId.getPlatformInfo().getEaIds().getOriginPersonaId());
    }
}

// This method sets the default role for each specified player if they don't already have any roles specified.
void PlayerJoinDataHelper::loadFromPlayerIdList(PlayerJoinData& joinData, const PlayerIdList& playerList, const RoleName* defaultRoleName /*= nullptr*/)
{
    PlayerIdList::const_iterator curUser = playerList.begin();
    PlayerIdList::const_iterator endUser = playerList.end();
    for (; curUser != endUser; ++curUser)
    {
        PerPlayerJoinData* playerData = getPlayerJoinData(joinData, *curUser);
        if (playerData == nullptr || !playerData->getRoles().empty())
            continue;

        if (defaultRoleName)
        {
            playerData->getRoles().push_back(*defaultRoleName);
        }
    }
}
void PlayerJoinDataHelper::loadFromRoleNameToPlayerMap(PlayerJoinData& joinData, const RoleNameToPlayerMap& roleToPlayerMap)
{
    if (roleToPlayerMap.size() == 1 && 
        roleToPlayerMap.begin()->second->front() == INVALID_BLAZE_ID)
    {
        joinData.setDefaultRole(roleToPlayerMap.begin()->first);
        return;
    }

    RoleNameToPlayerMap::const_iterator curPlayerRole = roleToPlayerMap.begin();
    RoleNameToPlayerMap::const_iterator endPlayerRole = roleToPlayerMap.end();
    for (; curPlayerRole != endPlayerRole; ++curPlayerRole)
    {
        const RoleName& role = curPlayerRole->first;

        PlayerIdList::const_iterator curPlayer = curPlayerRole->second->begin();
        PlayerIdList::const_iterator endPlayer = curPlayerRole->second->end();
        for (; curPlayer != endPlayer; ++curPlayer)
        {
            BlazeId playerId = *curPlayer;
            if (playerId != INVALID_BLAZE_ID)
            {
                PerPlayerJoinData* playerData = getPlayerJoinData(joinData, playerId);
                if (playerData == nullptr)
                    continue;

                playerData->getRoles().push_back(role);
            }
        }
    }
}
void PlayerJoinDataHelper::loadFromUserIdentificationList(PlayerJoinData& joinData, const UserIdentificationList& userList, const RoleName* defaultRoleName /*= nullptr*/, bool isOptionalUser /*= false*/)
{
    UserIdentificationList::const_iterator curUser = userList.begin();
    UserIdentificationList::const_iterator endUser = userList.end();
    for (; curUser != endUser; ++curUser)
    {
        PerPlayerJoinData* playerData = getPlayerJoinData(joinData, **curUser);
        if (playerData == nullptr)
            continue;

        if (defaultRoleName && playerData->getRoles().empty())
            playerData->getRoles().push_back(*defaultRoleName);
        if (isOptionalUser)
            playerData->setIsOptionalPlayer(true);
    }
}
void PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(PlayerJoinData& joinData, const RoleNameToUserIdentificationMap& roleToUserMap, bool isOptionalUser /*= false*/)
{
    if ((roleToUserMap.size() == 1) &&
        (roleToUserMap.begin()->second->front()->getBlazeId() == INVALID_BLAZE_ID) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getExternalIds().getSteamAccountId() == INVALID_STEAM_ACCOUNT_ID) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getExternalIds().getStadiaAccountId() == INVALID_STADIA_ACCOUNT_ID) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty()) &&
        (roleToUserMap.begin()->second->front()->getPlatformInfo().getEaIds().getNucleusAccountId() == INVALID_ACCOUNT_ID))
    {
        joinData.setDefaultRole(roleToUserMap.begin()->first);
        return;
    }

    RoleNameToUserIdentificationMap::const_iterator curRole = roleToUserMap.begin();
    RoleNameToUserIdentificationMap::const_iterator endRole = roleToUserMap.end();
    for (; curRole != endRole; ++curRole)
    {
        const RoleName& role = curRole->first;

        UserIdentificationList::const_iterator curUser = curRole->second->begin();
        UserIdentificationList::const_iterator endUser = curRole->second->end();
        for (; curUser != endUser; ++curUser)
        {
            PerPlayerJoinData* playerData = getPlayerJoinData(joinData, **curUser);
            if (playerData == nullptr)
                continue;

            playerData->getRoles().push_back(role);
            if (isOptionalUser)
                playerData->setIsOptionalPlayer(true);
        }
    }
}

void PlayerJoinDataHelper::exportToUserIdentificationList(const PlayerJoinData& joinData, UserIdentificationList& outUserList)
{
    UserIdentificationList tempReservedIdentificationList;
    PerPlayerJoinDataList::const_iterator curIter = joinData.getPlayerDataList().begin();
    PerPlayerJoinDataList::const_iterator endIter = joinData.getPlayerDataList().end();
    for (; curIter != endIter; ++curIter)
    {
        if ((*curIter)->getIsOptionalPlayer())
        {
            (*curIter)->getUser().copyInto(*outUserList.pull_back());
        }
    }
}
void PlayerJoinDataHelper::exportToUserIdentificationListAndRoleNameToUserIdentificationMap(const PlayerJoinData& joinData, UserIdentificationList& outUserList, RoleNameToUserIdentificationMap& outUserRoles)
{
    UserIdentificationList tempReservedIdentificationList;
    PerPlayerJoinDataList::const_iterator curIter = joinData.getPlayerDataList().begin();
    PerPlayerJoinDataList::const_iterator endIter = joinData.getPlayerDataList().end();
    for (; curIter != endIter; ++curIter)
    {
        RoleName role = (*curIter)->getRole();
        const UserIdentification& user = (*curIter)->getUser();
        if ((*curIter)->getIsOptionalPlayer())
        {
            if (role[0] != '\0' || joinData.getDefaultRole()[0] != '\0')
            {
                if (!outUserRoles[role])
                {
                    UserIdentificationList *localPlayerList = outUserRoles.allocate_element();
                    user.copyInto(*localPlayerList->pull_back());
                    outUserRoles[role] = localPlayerList;
                }
                else
                {
                    user.copyInto(*outUserRoles[role]->pull_back());
                }
            }

            user.copyInto(*outUserList.pull_back());
        }
    }
}
void PlayerJoinDataHelper::exportToRoleNameToPlayerMapAndRoleNameToUserIdentificationMap(const PlayerJoinData& joinData, RoleNameToPlayerMap& outPlayerRoles, RoleNameToUserIdentificationMap& outUserRoles)
{
    // Here, we apply the default role to all the players:
    bool defaultRoleExists = false;
    if (joinData.getDefaultRole()[0] != '\0')
    {
        // set up the joining role
        if (!outPlayerRoles[joinData.getDefaultRole()])
        {
            PlayerIdList *localPlayerList = outPlayerRoles.allocate_element();
            localPlayerList->push_back(INVALID_BLAZE_ID);
            outPlayerRoles[joinData.getDefaultRole()] = localPlayerList;
        }
        else
        {
            outPlayerRoles[joinData.getDefaultRole()]->push_back(INVALID_BLAZE_ID);
        }
        
        defaultRoleExists = true;
    }

    PerPlayerJoinDataList::const_iterator curIter = joinData.getPlayerDataList().begin();
    PerPlayerJoinDataList::const_iterator endIter = joinData.getPlayerDataList().end();
    for (; curIter != endIter; ++curIter)
    {
        RoleName role = (*curIter)->getRole();
        const UserIdentification& user = (*curIter)->getUser();
        if (role[0] == '\0' || isUserIdDefault(user))
            continue;

        if (user.getBlazeId() != INVALID_BLAZE_ID && !(*curIter)->getIsOptionalPlayer())
        {
            if (defaultRoleExists)
            {
                if (role != joinData.getDefaultRole())
                {
                    BLAZE_SDK_DEBUGF("[PlayerJoinDataHelper::exportToRoleNameToPlayerMapAndRoleNameToUserIdentificationMap()] Role name used (%s) differs from default role name (%s).  This means the test is going to fail because we didn't track that user's role properly.\n", role.c_str(), joinData.getDefaultRole());
                }
                continue;
            }

            if (!outPlayerRoles[role])
            {
                PlayerIdList *localPlayerList = outPlayerRoles.allocate_element();
                localPlayerList->push_back(user.getBlazeId());
                outPlayerRoles[role] = localPlayerList;
            }
            else
            {
                outPlayerRoles[role]->push_back(user.getBlazeId());
            }
        }
        else
        {
            if (!outUserRoles[role])
            {
                UserIdentificationList *localPlayerList = outUserRoles.allocate_element();
                user.copyInto(*localPlayerList->pull_back());
                outUserRoles[role] = localPlayerList;
            }
            else
            {
                user.copyInto(*outUserRoles[role]->pull_back());
            }
        }
    }
}


} // namespace GameManager
} // namespace Blaze

