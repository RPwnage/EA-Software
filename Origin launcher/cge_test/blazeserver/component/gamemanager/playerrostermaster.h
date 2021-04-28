/*! ************************************************************************************************/
/*!
    \file playerrostermaster.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_ROSTER_MASTER_H
#define BLAZE_GAMEMANAGER_PLAYER_ROSTER_MASTER_H

#include "gamemanager/playerroster.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/rpc/gamemanagermaster_stub.h" // for RPC error codes
#include "gamemanager/playerinfomaster.h"

namespace Blaze
{
namespace GameManager
{
    class GameManagerMasterImpl;

    /*! ************************************************************************************************/
    /*! \brief Data class for game roster list. This class will encapsulate the maps for each type of player
            state.
    ***************************************************************************************************/
    class PlayerRosterMaster : public PlayerRoster
    {
        NON_COPYABLE(PlayerRosterMaster);

    ///////////////////////////////////////////////////////////////////////////////
    // public methods
    //
    public:

        // GM_AUDIT: friendship w/ GameSessionMaster means we don't have a meaningful public interface, need to refactor to create one.

        PlayerRosterMaster();
        ~PlayerRosterMaster() override;
    
        // create player
        BlazeRpcError createNewPlayer(const PlayerId playerId, const PlayerState playerState, SlotType slotType, 
            TeamIndex teamIndex, const RoleName &roleName, const UserGroupId& groupId, const UserSessionInfo& playerInfo, const char8_t* encryptedBlazeId, PlayerInfoMasterPtr& newPlayer);
        BlazeRpcError createExternalPlayer(SlotType slotType, TeamIndex teamIndex, const char8_t *roleName, const UserGroupId& groupId, const UserSessionInfo& playerInfo, const char8_t* encryptedBlazeId, PlayerInfoMasterPtr& newPlayer);


        // add/remove player
        bool addNewPlayer(PlayerInfoMaster* newPlayerInfo, const GameSetupReason &playerJoinContext);
        void deletePlayer(PlayerInfoMaster *&player, const PlayerRemovalContext &playerRemovalContext, bool lockedForPreferredJoins = false, bool isRemovingAllPlayers = false);

        bool addNewPlayerToQueue(PlayerInfoMaster& newPlayerInfo, const GameSetupReason &playerJoinContext);
        bool addNewReservedPlayer(PlayerInfoMaster& newPlayerInfo, const GameSetupReason& context);

        // update players for queued and reserved players.
        bool promoteQueuedPlayer(PlayerInfoMaster *&playerInfo, const GameSetupReason &playerJoinContext);
        bool promoteReservedPlayer(PlayerInfoMaster* playerInfo, const GameSetupReason &playerJoinContext, const PlayerState playerState, bool previousTeamIndexUnspecified = false);

        bool demotePlayerToQueue(PlayerInfoMaster *&playerInfo);


        // retrieve player(s)
        PlayerInfoMaster* getPlayer(const PlayerId playerId) const;
        PlayerInfoMaster* getPlayer(const PlayerId playerId, const PlatformInfo& platformInfo) const;
        PlayerInfoMaster* getRosterPlayer(const PlayerId playerId) const;
        PlayerInfoMaster* getQueuePlayer(const PlayerId playerId) const;
        PlayerInfoMaster* getExternalPlayer(const PlatformInfo& platformInfo) const;
        PlayerInfoList getPlayers(const PlayerRosterType rosterType) const
        {
            // NOTE: Although this method looks less efficient than it's counterpart in PlayerRoster that takes PlayerInfoList by ref, it isn't!
            // This is because GCC will always perform return value optimization (even in debug mode!) which means the copy ctor never gets invoked, 
            // despite appearances.

            PlayerInfoList players;
            PlayerRoster::getPlayers(players, rosterType);
            return players;
        }

        BlazeRpcError updatePlayerStateNoReplication(PlayerInfoMaster* player, const PlayerState newState);
        BlazeRpcError updatePlayerState(PlayerInfoMaster* info, const PlayerState newstate);
        BlazeRpcError updatePlayerTeamRoleAndSlot(PlayerInfoMaster* player, const TeamIndex newTeamIndex, const RoleName& newRoleName, SlotType newSlotType);
        // this method behaves like updatePlayerTeamIndex, but uses a different replication reason, and skips updating the team player counts
        // it is only for use when a player is moved to a different team as part of a setPlayerCapacity RPC
        BlazeRpcError updatePlayerTeamIndexForCapacityChange(PlayerInfoMaster* player, TeamIndex newTeamIndex);

        void initPlayerStatesForHostMigrationStart(const PlayerInfo& newHostPlayer);
        void initPlayerStatesForHostReinjection();
        void resetConnectingMembersJoinTimer();

        PlayerInfoMaster* pickPossHostByTime(PlayerId oldPlayerId, ConnectionGroupId connectionGroupToSkip);

        void populateQueueEvent(PlayerIdList& eventPlayerList) const;

        bool hasPlayersOnConnection(const ConnectionGroupId connGroupId) const;
        bool hasConnectedPlayersOnConnection(const ConnectionGroupId connGroupId) const;
        bool hasActiveMigratingPlayers() const;
        bool hasQueuedPlayers() const;
        bool hasRosterPlayers() const;

        PlayerInfoMaster* replaceExternalPlayer(UserSessionId newSessionId, const BlazeId newBlazeId, const PlatformInfo& newPlatformInfo, const Locale newAccountLocale,
            const uint32_t newAccountCountry, const char8_t* serviceName, const char8_t* productName);

        void updateQueuedPlayersDataOnPlayerRemove(PlayerId playerId);
        void insertPlayerData(PlayerInfoMaster& info, bool isQueued);
        void updatePlayerData(PlayerInfoMaster& info);
        void erasePlayerData(PlayerInfoMaster& info);
        void upsertPlayerFields();

    private:
        GameId getGameId() const;
        void updateQueueIndexes(SlotId removedQueueIndex);
        bool shouldLeaveExternalSessionOnDeleteMember(PlayerRemovedReason reason, const PlayerInfoMaster* player) const;
        GameSessionMaster* getOwningGameSession();
        PlayerInfoMaster* getFirstEligibleNextAdmin(const PlayerInfoList& playerList, PlayerId oldAdminId, ConnectionGroupId connectionGroupToSkip) const;
        bool isEligibleToBeNextAdmin(PlayerId newAdminCandidateId, PlayerId oldAdminId, ConnectionGroupId connectionGroupToSkip) const;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_PLAYER_ROSTER_MASTER_H
