/*! ************************************************************************************************/
/*!
    \file playerroster.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_ROSTER_H
#define BLAZE_GAMEMANAGER_PLAYER_ROSTER_H

#include "EASTL/fixed_vector.h"
#include "gamemanager/rpc/gamemanagermaster_stub.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/commoninfo.h"

namespace Blaze
{
namespace GameManager
{
    class PlayerInfo;

    /*! ************************************************************************************************/
    /*! \brief Data class for game roster list. This class will encapsulate the maps for each type of player
            state.
    ***************************************************************************************************/
    class PlayerRoster
    {
        NON_COPYABLE(PlayerRoster);

    protected:

        /*! \brief  map tracking group sizes by group id */
        typedef eastl::hash_map<UserGroupId, uint16_t> GroupIdToSizeMap;

    public:
        enum RosterType
        {
            QUEUE_ROSTER,
            PLAYER_ROSTER
        };

        enum PlayerCountAdjustment
        {
            PLAYER_COUNT_INCREMENT,
            PLAYER_COUNT_DECREMENT
        };

        // We track the roster in different ways for various needs.
        // These are mostly based on player state and which roster the player is in.
        // These are mostly based on player state and which roster the player is in.
        // Some rosters break out a subset of the users in a particular state who are also participants (not spectators) in the game.
        // This is important for doing things like determining a new game host, or promoting a new admin,
        // since we specifically avoid picking spectators to take on these duties
        enum PlayerRosterType
        {
            // All queued players (even reserved)
            QUEUED_PLAYERS,
            // All roster participants (even reserved)
            ROSTER_PARTICIPANTS,
            // All players in the roster (participants & spectators, even reserved)
            ROSTER_PLAYERS,
            // Players with PERMISSION_HOST_GAME_SESSION in the ACTIVE_CONNECTING player state in the roster.
            ACTIVE_CONNECTING_PLAYERS_WITH_HOST_PERMISSION, 
            // Players with PERMISSION_HOST_GAME_SESSION in the ACTIVE_CONNECTED player state in the roster.
            ACTIVE_CONNECTED_PLAYERS_WITH_HOST_PERMISSION, 
            // Players with PERMISSION_HOST_GAME_SESSION in the ACTIVE_MIGRATING player state in the roster.
            ACTIVE_MIGRATING_PLAYERS_WITH_HOST_PERMISSION, 
            // All players in the ACTIVE_CONNECTING player state in the roster.
            ACTIVE_CONNECTING_PLAYERS, 
            // Players in the ACTIVE_CONNECTED player state in the roster.
            ACTIVE_CONNECTED_PLAYERS, 
            // Players in the ACTIVE_MIGRATING player state in the roster.
            ACTIVE_MIGRATING_PLAYERS, 
            // All active participants in the roster. (not reserved & queued). Used for items related to the network connections that need to exclude spectators.
            ACTIVE_PARTICIPANTS, 
            // All active players in the roster. (not reserved & queued). Used for items related to the network connections that need to touch both participants and spectators.
            ACTIVE_PLAYERS, 
            // Every player the game knows about.
            ALL_PLAYERS,
            // roster type count
            PLAYER_ROSTER_TYPE_MAX
        };

        typedef eastl::fixed_vector<PlayerInfo*, 64> PlayerInfoListBase;
        class PlayerInfoList : public PlayerInfoListBase
        {
        public:
            PlayerInfoList() {}
            PlayerInfoList(const PlayerInfoListBase::overflow_allocator_type& overflowAllocator) : PlayerInfoListBase(overflowAllocator) {}
            void remove(PlayerInfo* player)
            {
                erase(eastl::remove(begin(), end(), player), end());
            }
        };

        virtual ~PlayerRoster() {};

        // NEW INTERFACE
        PlayerInfo* getPlayer(const PlayerId playerId) const;
        PlayerInfoPtr getPlayerInfoPtr(const PlayerId playerId) const;
        void getPlayers(PlayerInfoList& playerList, const PlayerRosterType rosterType) const;
        void getPlayers(PlayerInfoList& playerList, const PlayerState* playerState = nullptr, const bool* hostPermission = nullptr, const bool* isParticipant = nullptr) const;
        /*! \brief Get the number of non-queued players in game */
        uint16_t getRosterPlayerCount() const;
        uint16_t getPlayerCount(const PlayerRosterType rosterType) const;
        uint16_t getPlayerCount(const PlayerState* playerState, const bool* hostPermission = nullptr, const bool* isParticipant = nullptr, const TeamIndex* teamIndex = nullptr) const;
        uint16_t getParticipantCount() const;
        uint16_t getSpectatorCount() const;
        uint16_t getPlayerCount(SlotType slotType) const;
        uint16_t getPlayerCount(ClientPlatformType platform, bool includeQueue = true) const;
        void getSlotPlayerCounts(SlotCapacitiesVector& capacities) const;
        uint16_t getQueueCount() const;// returns the total size of the queue, players and spectators
        bool isQueueEmptyForSlotType(SlotType slotType) const;// returns true if there are no users queued for a given type of slot (player vs spectator)
        uint16_t getTeamSize(TeamIndex teamIndex) const;
        uint16_t getRoleSize(TeamIndex teamIndex, const RoleName &roleName) const;
        void getRoleSizeMap(RoleSizeMap& roleSizeMap, TeamIndex teamIndex) const;
        uint16_t getSizeOfGroupOnTeam(TeamIndex teamIndex, UserGroupId groupId) const;
        uint16_t getSizeOfGroupInGame(UserGroupId groupId) const;
        void getGroupMembersInGame(UserGroupId groupId, PlayerIdList &groupMemberList) const;
        void getTeamGroupSizeCountMap(GroupSizeCountMap& groupSizeCountMap, TeamIndex teamIndex) const;
        const char8_t* getTeamAttributesLogStr() const;
        const char8_t* getTeamAttributesLogStr(TeamIndex teamIndex) const;

        void setOwningGameSession(GameSession& session);

        void insertPlayer(PlayerInfo& player);
        bool erasePlayer(PlayerId playerId);

    protected:
        PlayerRoster();

    protected:
        PlayerInfoMap mPlayerInfoMap;

        GameSession* mOwningGameSession;

    private:
        mutable StringBuilder mTeamAttributesBuf;
        mutable StringBuilder mTeamIndexAttributesBuf;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_PLAYER_ROSTER_H
