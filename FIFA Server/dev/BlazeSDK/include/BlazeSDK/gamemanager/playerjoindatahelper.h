/*! ************************************************************************************************/
/*!
    \file playerjoindatahelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_JOIN_DATA_HELPER_H
#define BLAZE_GAMEMANAGER_PLAYER_JOIN_DATA_HELPER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usercontainer.h"
#include "BlazeSDK/gamemanager/gamemanagerhelpers.h" // for SlotType helpers


namespace Blaze
{
namespace GameManager
{
    class Game;

    // 
    class BLAZESDK_API PlayerJoinDataHelper
    {
    public:
        friend class GameManagerAPI;

        PlayerJoinDataHelper() {}
        PlayerJoinDataHelper(const PlayerJoinData& playerJoinData) 
        {
            playerJoinData.copyInto(mPlayerJoinData);
        }

        // Helper constructor that calls the initFromGame* functions;
        PlayerJoinDataHelper(const Game* game, bool initForMatchmaking);

        /*! ***************************************************************/
        /*! \name initFromGameFor*
            \brief Copies information from all players. 
                   This can be used when you want to move the players from one game (or game group) into another game.
                   The Matchmaking version only takes active (non-reserved, non-queued) players (no spectators), because only the public participant slot is supported for MM. 
        *******************************************************************/
        void initFromGameForMatchmaking( const Game* game, bool copyTeams = true, bool copyRoles = true, bool copyPlayerAttributes = true);
        void initFromGameForCreateOrJoin(const Game* game, bool copyTeams = true, bool copyRoles = true, bool copyPlayerAttributes = true, bool copySlotType = true, bool includeSpectators = true);

        // Finds data based on UserIdentifier or BlazeId. Returns nullptr if the player does not already exist. 
        PerPlayerJoinData* findPlayer(BlazeId playerId) { return findPlayerJoinData(mPlayerJoinData, playerId); }
        PerPlayerJoinData* findPlayer(const UserIdentification& user) { return findPlayerJoinData(mPlayerJoinData, user); }

        // Add a player, overriding values if the player already exists:
        PerPlayerJoinData* addPlayer(BlazeId playerId, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr)  { return addPlayerJoinData(mPlayerJoinData, playerId, roleName, playerAttrs); }
        PerPlayerJoinData* addPlayer(UserManager::User* user, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr)  { return addPlayerJoinData(mPlayerJoinData, user, roleName, playerAttrs); }
        
        // Add a player that does not have a valid BlazeId. Used by GameBrowser requests, which do not require actual players. (Other functions will ignore dummy users.)
        PerPlayerJoinData* addDummyPlayer(const RoleName* roleName = nullptr, TeamId teamId = INVALID_TEAM_ID);

        // Add a player by external id (using an external id, or blob).  If the player is an optional user (such as XboxOne external party members), set isOptionalUser to true.
        PerPlayerJoinData* addExternalPlayer(const UserIdentification& user, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr, bool isOptionalUser = false) { return addPlayerJoinData(mPlayerJoinData, user, roleName, playerAttrs, isOptionalUser); }


        /*! ***************************************************************/
        /*! \name PlayerDataList
            \brief Player Join Data for the individual joining players, whose blaze id, or external ids, are known. Holds Role, and Attribute information.
        *******************************************************************/
        PerPlayerJoinDataList& getPlayerDataList() { return mPlayerJoinData.getPlayerDataList(); }
        const PerPlayerJoinDataList& getPlayerDataList() const { return mPlayerJoinData.getPlayerDataList(); }

        /*! ***************************************************************/
        /*! \name GroupId
            \brief Used for group join only, holding component,type and groupid information.
        *******************************************************************/
        UserGroupId getGroupId() const { return mPlayerJoinData.getGroupId(); }
        void setGroupId(UserGroupId val) { mPlayerJoinData.setGroupId(val); }
        // Helper function to convert a GameGroup's GameId to the UserGroupId used in matchmaking.
        void setGameGroupId(GameId ggId) { mPlayerJoinData.setGroupId(Blaze::GameManager::UserGroupId(Blaze::GameManager::ENTITY_TYPE_GAME_GROUP, static_cast<EA::TDF::EntityId>(ggId))); }


        /*! ***************************************************************/
        /*! \name SlotType
            \brief Slot type to join as. Public or private, participant or spectator.
        *******************************************************************/
        SlotType getSlotType() const { return mPlayerJoinData.getDefaultSlotType(); }
        void setSlotType(SlotType val) {  mPlayerJoinData.setDefaultSlotType(val); }

        /*! ***************************************************************/
        /*! \name GameEntryType
            \brief This value indicates if a player is joining, reserving, or claiming a reservation.
        *******************************************************************/
        GameEntryType getGameEntryType() const { return mPlayerJoinData.getGameEntryType(); }
        void setGameEntryType(GameEntryType val) {  mPlayerJoinData.setGameEntryType(val); }

        /*! ***************************************************************/
        /*! \name DefaultRole
            \brief Default role, used for players who cannot provide PerPlayerJoinData (those coming from a GroupId), or to set all player roles globally.
        *******************************************************************/
        const char8_t* getDefaultRole() const { return mPlayerJoinData.getDefaultRole(); }
        void setDefaultRole(const char8_t* val) {  mPlayerJoinData.setDefaultRole(val); }

        /*! ***************************************************************/
        /*! \name TeamId
            \brief The TeamId I choose to join. By default, ANY_TEAM_ID will be used in-game, since all games have at least one team. Do not set to INVALID_TEAM_ID.
        *******************************************************************/
        TeamId getTeamId() const { return mPlayerJoinData.getDefaultTeamId(); }
        void setTeamId(TeamId val) {  mPlayerJoinData.setDefaultTeamId(val); }


        /*! ***************************************************************/
        /*! \name TeamIndex
            \brief Default Team index used when multiple teams have the same team id. Not used by Matchmaking.
        *******************************************************************/
        TeamIndex getTeamIndex() const { return mPlayerJoinData.getDefaultTeamIndex(); }
        void setTeamIndex(TeamIndex val) {  mPlayerJoinData.setDefaultTeamIndex(val); }


        PlayerJoinData& getPlayerJoinData() { return mPlayerJoinData; }
        const PlayerJoinData& getPlayerJoinData() const { return mPlayerJoinData; }

        // Copy the data into a PlayerJoinData structure
        void copyInto(PlayerJoinData& playerJoinData) const
        {
            mPlayerJoinData.copyInto(playerJoinData);
        }


    // Static Helper Functions:
        static bool isUserIdDefault(const UserIdentification& id1);

        // Check if two UserIdentifications are equivalent 
        static bool areUserIdsEqual(const UserIdentification& id1, const UserIdentification& id2);
        
        // Update a partially complete UserIdentification with the newId values 
        static void updateUserId(const UserIdentification& newId, UserIdentification& outUserId);





    // Old Conversion functions:
        // Adds all of the given users and updates their roles
        static void loadFromPlayerIdList(PlayerJoinData& joinData, const PlayerIdList& playerList, const RoleName* defaultRoleName = nullptr);
        static void loadFromRoleNameToPlayerMap(PlayerJoinData& joinData, const RoleNameToPlayerMap& roleToPlayerMap);
        static void loadFromUserIdentificationList(PlayerJoinData& joinData, const UserIdentificationList& userList, const RoleName* defaultRoleName = nullptr, bool isOptionalUser = false);
        static void loadFromRoleNameToUserIdentificationMap(PlayerJoinData& joinData, const RoleNameToUserIdentificationMap& roleToUserMap, bool isOptionalUser = false);

        // These convert the data to the old formats:
        static void exportToUserIdentificationList(const PlayerJoinData& joinData, UserIdentificationList& outUserList);
        static void exportToUserIdentificationListAndRoleNameToUserIdentificationMap(const PlayerJoinData& joinData, UserIdentificationList& outUserList, RoleNameToUserIdentificationMap& outUserRoles);
        static void exportToRoleNameToPlayerMapAndRoleNameToUserIdentificationMap(const PlayerJoinData& joinData, RoleNameToPlayerMap& outPlayerRoles, RoleNameToUserIdentificationMap& outUserRoles);

        static PerPlayerJoinData* addPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr);
        static PerPlayerJoinData* addPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr, bool isOptionalUser = false);

    private:
        // Finds data based on UserIdentifier or BlazeId. Returns nullptr if the player does not already exist. 
        static PerPlayerJoinData* findPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId);
        static PerPlayerJoinData* findPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user);

        // Gets data based on UserIdentifier or BlazeId. Adds a new player if one did not already exist. (Returns nullptr if INVALID_BLAZE_ID is used).
        static PerPlayerJoinData* getPlayerJoinData(PlayerJoinData& joinData, BlazeId playerId);
        static PerPlayerJoinData* getPlayerJoinData(PlayerJoinData& joinData, const UserIdentification& user);

        // Add a player, overriding values if the player already exists:
        static PerPlayerJoinData* addPlayerJoinData(PlayerJoinData& joinData, UserManager::User* user, const RoleName* roleName = nullptr, const Collections::AttributeMap *playerAttrs = nullptr);

        PlayerJoinData mPlayerJoinData;
    };


} //namespace GameManager
} //namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYER_JOIN_DATA_HELPER_H
