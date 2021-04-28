/*! ************************************************************************************************/
/*!
    \file   playerroster.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "EASTL/algorithm.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/gamesession.h" // for GameSession in getTeamAttributesLogStr

namespace Blaze
{
namespace GameManager
{

    PlayerRoster::PlayerRoster()
    {
        mOwningGameSession = nullptr;
    }

    // NEW INTERFACE

    PlayerInfo* PlayerRoster::getPlayer( const PlayerId playerId ) const
    {
        PlayerInfoMap::const_iterator it = mPlayerInfoMap.find(playerId);
        PlayerInfo* playerInfo = nullptr;
        if (it != mPlayerInfoMap.end())
            playerInfo = it->second.get();
        return playerInfo;
    }

    PlayerInfoPtr PlayerRoster::getPlayerInfoPtr(const PlayerId playerId) const
    {
        PlayerInfoMap::const_iterator it = mPlayerInfoMap.find(playerId);
        PlayerInfoPtr playerInfo = nullptr;
        if (it != mPlayerInfoMap.end())
            playerInfo = it->second;
        return playerInfo;
    }

    void PlayerRoster::getPlayers(PlayerInfoList& playerList, const PlayerState* playerState, const bool* hostPermission, const bool* isParticipant) const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (!player->isInRoster())
                continue;
            if (playerState && *playerState != player->getPlayerState())
                continue;
            if (hostPermission && *hostPermission != player->hasHostPermission())
                continue;
            if (isParticipant && *isParticipant != player->isParticipant())
                continue;
            playerList.push_back(player);
        }
    }

    static bool lessThanPlayerQueueOrderCompare(const PlayerInfo* a, const PlayerInfo* b)
    {
        return a->getSlotId() < b->getSlotId();
    }

    void PlayerRoster::getPlayers( PlayerInfoList& playerList, const PlayerRosterType rosterType ) const
    {
        switch (rosterType)
        {
        case QUEUED_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (!player->isInRoster())
                    playerList.push_back(player);
            }
            // NOTE: We must sort the players before returning because they get dequeued in the order of insertion
            eastl::sort(playerList.begin(), playerList.end(), &lessThanPlayerQueueOrderCompare);
            break;
        case ROSTER_PARTICIPANTS:
            {
                bool isParticipant = true;
                getPlayers(playerList, nullptr, nullptr, &isParticipant);
            }
            break;
        case ROSTER_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster())
                    playerList.push_back(player);
            }
            break;
        case ACTIVE_CONNECTING_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_CONNECTING;
                bool hasPermission = true;
                getPlayers(playerList, &state, &hasPermission);
            }
            break;
        case ACTIVE_CONNECTED_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_CONNECTED;
                bool hasPermission = true;
                getPlayers(playerList, &state, &hasPermission);
            }
            break;
        case ACTIVE_MIGRATING_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_MIGRATING;
                bool hasPermission = true;
                getPlayers(playerList, &state, &hasPermission);
            }
            break;
        case ACTIVE_CONNECTING_PLAYERS:
            {
                PlayerState state = ACTIVE_CONNECTING;
                getPlayers(playerList, &state);
            }
            break;
        case ACTIVE_CONNECTED_PLAYERS:
            {
                PlayerState state = ACTIVE_CONNECTED;
                getPlayers(playerList, &state);
            }
            break;
        case ACTIVE_MIGRATING_PLAYERS:
            {
                PlayerState state = ACTIVE_MIGRATING;
                getPlayers(playerList, &state);
            }
            break;
        case ACTIVE_PARTICIPANTS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && player->isParticipant() && player->isActive())
                {
                    playerList.push_back(player);
                }
            }
            break;
        case ACTIVE_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && player->isActive())
                {
                    playerList.push_back(player);
                }
            }
            break;
        case ALL_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                playerList.push_back(player);
            }
            break;
        default: 
            break;
        };
    }

    /*! \brief Get the number of non-queued players in game */
    uint16_t PlayerRoster::getRosterPlayerCount() const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster())
                ++count;
        }

        return count;
    }

    uint16_t PlayerRoster::getPlayerCount( const PlayerRosterType rosterType ) const
    {
        uint16_t count = 0;

        switch (rosterType)
        {
        case QUEUED_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (!player->isInRoster())
                    ++count;
            }
            break;
        case ROSTER_PARTICIPANTS:
            {
                bool isParticipant = true;
                count = getPlayerCount(nullptr, nullptr, &isParticipant);
            }
            break;
        case ROSTER_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster())
                    ++count;
            }
            break;
        case ACTIVE_CONNECTING_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_CONNECTING;
                bool hasPermission = true;
                count = getPlayerCount(&state, &hasPermission);
            }
            break;
        case ACTIVE_CONNECTED_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_CONNECTED;
                bool hasPermission = true;
                count = getPlayerCount(&state, &hasPermission);
            }
            break;
        case ACTIVE_MIGRATING_PLAYERS_WITH_HOST_PERMISSION:
            {
                PlayerState state = ACTIVE_MIGRATING;
                bool hasPermission = true;
                count = getPlayerCount(&state, &hasPermission);
            }
            break;
        case ACTIVE_CONNECTING_PLAYERS:
            {
                PlayerState state = ACTIVE_CONNECTING;
                count = getPlayerCount(&state);
            }
            break;
        case ACTIVE_CONNECTED_PLAYERS:
            {
                PlayerState state = ACTIVE_CONNECTED;
                count = getPlayerCount(&state);
            }
            break;
        case ACTIVE_MIGRATING_PLAYERS:
            {
                PlayerState state = ACTIVE_MIGRATING;
                count = getPlayerCount(&state);
            }
            break;
        case ACTIVE_PARTICIPANTS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && player->isParticipant() && player->isActive())
                {
                    ++count;
                }
            }
            break;
        case ACTIVE_PLAYERS:
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && player->isActive())
                {
                    ++count;
                }
            }
            break;
        case ALL_PLAYERS:
            count = static_cast<uint16_t>(mPlayerInfoMap.size());
            break;
        default: 
            break;
        };

        return count;
    }

    uint16_t PlayerRoster::getPlayerCount(const PlayerState* playerState, const bool* hostPermission, const bool* isParticipant, const TeamIndex* teamIndex) const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (!player->isInRoster())
                continue;
            if (playerState && *playerState != player->getPlayerState())
                continue;
            if (hostPermission && *hostPermission != player->hasHostPermission())
                continue;
            if (isParticipant && *isParticipant != player->isParticipant())
                continue;
            if (teamIndex && *teamIndex != player->getTeamIndex())
                continue;
            ++count;
        }
        return count;
    }

    uint16_t PlayerRoster::getPlayerCount( SlotType slotType ) const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && slotType == player->getSlotType())
                ++count;
        }
        return count;
    }

    uint16_t PlayerRoster::getPlayerCount(ClientPlatformType platform, bool includeQueue /* = true */) const
    {
        uint16_t count = 0;
        for (auto playerIter : mPlayerInfoMap)
        {
            PlayerInfoPtr player = playerIter.second;
            if ((includeQueue || player->isInRoster()) && (platform == player->getClientPlatform()))
                ++count;
        }
        return count;
    }

    uint16_t PlayerRoster::getParticipantCount() const
    {
        uint16_t count = 0;
        bool isParticipant = true;
        count = getPlayerCount(nullptr, nullptr, &isParticipant);
        return count;
    }

    uint16_t PlayerRoster::getSpectatorCount() const
    {
        uint16_t count = 0;
        bool isParticipant = false;
        count = getPlayerCount(nullptr, nullptr, &isParticipant);
        return count;
    }

    void PlayerRoster::getSlotPlayerCounts( SlotCapacitiesVector& capacities ) const
    {
        capacities.assign(MAX_SLOT_TYPE, 0); // required to size the vector appropriately

        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster())
                ++capacities[player->getSlotType()];
        }
    }

    uint16_t PlayerRoster::getQueueCount() const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (!player->isInRoster())
                ++count;
        }

        return count;
    }

    bool PlayerRoster::isQueueEmptyForSlotType( SlotType slotType ) const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (!player->isInRoster() && slotType == player->getSlotType())
                return false;
        }

        return true;
    }

    uint16_t PlayerRoster::getTeamSize( TeamIndex teamIndex ) const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()))
                ++count;
        }
        return count;
    }

    uint16_t PlayerRoster::getRoleSize( TeamIndex teamIndex, const RoleName &roleName ) const
    {
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()) && blaze_strcmp(roleName.c_str(), player->getRoleName()) == 0)
                ++count;
        }
        return count;
    }

    void PlayerRoster::getRoleSizeMap( RoleSizeMap& roleSizeMap, TeamIndex teamIndex ) const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()))
                roleSizeMap.insert(eastl::make_pair(player->getRoleName(), (uint16_t)0)).first->second++;
        }
    }

    uint16_t PlayerRoster::getSizeOfGroupOnTeam( TeamIndex teamIndex, UserGroupId groupId ) const
    {
        if (groupId == EA::TDF::OBJECT_ID_INVALID)
        {
            // treat this user as an individual
            return 1;
        }
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()) && groupId == player->getUserGroupId())
                ++count;
        }
        return count;
    }

    uint16_t PlayerRoster::getSizeOfGroupInGame( UserGroupId groupId ) const
    {
        if (groupId == EA::TDF::OBJECT_ID_INVALID)
        {
            // treat this user as an individual
            return 1;
        }
        uint16_t count = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && groupId == player->getUserGroupId() && isParticipantSlot(player->getSlotType()))
                ++count;
        }

        return count;
    }

    void PlayerRoster::getGroupMembersInGame(UserGroupId groupId, PlayerIdList& groupMemberList) const
    {
        EA_ASSERT(groupMemberList.empty());
        if (groupId == EA::TDF::OBJECT_ID_INVALID)
        {
            // no group.
            return;
        }

        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && groupId == player->getUserGroupId() && isParticipantSlot(player->getSlotType()))
                groupMemberList.push_back(player->getPlayerId());
        }

        return;
    }

    void PlayerRoster::getTeamGroupSizeCountMap( GroupSizeCountMap& groupSizeCountMap, TeamIndex teamIndex ) const
    {
        GroupIdToSizeMap gidToSizeMap(BlazeStlAllocator("GameManagerMasterImpl::gidToSizeMap", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        uint16_t groupsOfOne = 0;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()))
            {
                if (player->getUserGroupId() == EA::TDF::OBJECT_ID_INVALID)
                    ++groupsOfOne; // invalid group ids count as a separate group of 1
                else
                    gidToSizeMap.insert(eastl::make_pair(player->getUserGroupId(), (uint16_t)0)).first->second++;
            }
        }
        
        if (groupsOfOne > 0)
            groupSizeCountMap[1] = groupsOfOne;

        for (GroupIdToSizeMap::const_iterator i = gidToSizeMap.begin(), e = gidToSizeMap.end(); i != e; ++i)
        {
            groupSizeCountMap.insert(eastl::make_pair(i->second, (uint16_t)0)).first->second++;
        }
    }

    const char8_t* PlayerRoster::getTeamAttributesLogStr(TeamIndex teamIndex) const
    {
        uint16_t teamSize = getTeamSize(teamIndex);
        TeamId teamId = mOwningGameSession->getTeamIdByIndex(teamIndex);

        mTeamIndexAttributesBuf.reset();
        mTeamIndexAttributesBuf << "teamIndex=" << teamIndex;
        mTeamIndexAttributesBuf << ",id=" << teamId;
        mTeamIndexAttributesBuf << ",size=" << teamSize;
        mTeamIndexAttributesBuf << ",groups='";

        typedef eastl::set<eastl::pair<TeamIndex, UserGroupId> > TeamIdxGroupIdSet;
        TeamIdxGroupIdSet existingGroupIdsByTeamId;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            TeamIdxGroupIdSet::key_type key(player->getTeamIndex(), player->getUserGroupId());
            if (player->isInRoster() && teamIndex == player->getTeamIndex() && isParticipantSlot(player->getSlotType()) &&
                existingGroupIdsByTeamId.find(key) == existingGroupIdsByTeamId.end())
            {
                uint16_t groupSize = getSizeOfGroupOnTeam(player->getTeamIndex(), player->getUserGroupId());
                existingGroupIdsByTeamId.insert(key);
                mTeamIndexAttributesBuf << "{grpId=" << player->getUserGroupId().toString().c_str();
                mTeamIndexAttributesBuf << ",size=" << groupSize;
                mTeamIndexAttributesBuf << ",";
            }
        }

        if (!existingGroupIdsByTeamId.empty())
            mTeamIndexAttributesBuf.trim(1); // trim the trailing ','
        else
            mTeamIndexAttributesBuf << "<None>";

        mTeamIndexAttributesBuf << "'";
        return mTeamIndexAttributesBuf.get();
    }

    const char8_t* PlayerRoster::getTeamAttributesLogStr() const
    {
        typedef eastl::hash_set<TeamIndex> UniqueTeamIndicies;
        UniqueTeamIndicies uniqueTeamIndicies;
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && isParticipantSlot(player->getSlotType()))
                uniqueTeamIndicies.insert(player->getTeamIndex());
        }

        mTeamAttributesBuf.reset();
        mTeamAttributesBuf << "teamCount=" << uniqueTeamIndicies.size() << ":";
        for (UniqueTeamIndicies::const_iterator i = uniqueTeamIndicies.begin(), e = uniqueTeamIndicies.end(); i != e; ++i)
        {
            mTeamAttributesBuf << "[" << getTeamAttributesLogStr(*i) << "],";
        }

        mTeamAttributesBuf.trim(1); // trim the trailing ','

        return mTeamAttributesBuf.get();
    }

    void PlayerRoster::setOwningGameSession(GameSession& session)
    {
        EA_ASSERT(mOwningGameSession == nullptr);
        mOwningGameSession = &session;
    }

    
    void PlayerRoster::insertPlayer(PlayerInfo& player)
    {
        mPlayerInfoMap[player.getPlayerId()] = &player;
    }

    bool PlayerRoster::erasePlayer(PlayerId playerId)
    {
        return (mPlayerInfoMap.erase(playerId) > 0);
    }

}
}
