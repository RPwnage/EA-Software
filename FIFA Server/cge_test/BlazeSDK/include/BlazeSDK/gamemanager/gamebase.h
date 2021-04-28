/*! ************************************************************************************************/
/*!
    \file gamebase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_BASE_H
#define BLAZE_GAMEMANAGER_GAME_BASE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "BlazeSDK/blaze_eastl/vector.h"


namespace Blaze
{
namespace GameManager
{

    /*! \brief A typedef to represent a set of capacities for player slots */
    typedef uint16_t SlotCapacities[MAX_SLOT_TYPE];
    
    /*! ************************************************************************************************/
    /*! \brief GameBase is an abstract class containing shared data and getter methods for blaze games.

        GameBase is a common base class for Games and GameBrowserGames.  Note that we don't have player
        getters here, since Games contain Players while GameBrowserGames contain GameBrowserPlayers.
    ***************************************************************************************************/
    class BLAZESDK_API GameBase
    {
        NON_COPYABLE(GameBase);
    public:

    /*! **********************************************************************************************************/
    /*! \name General Game Data Accessors (GameBase)
    **************************************************************************************************************/

        /*! **********************************************************************************************************/
        /*! \brief Returns the blazeId of this game.
        **************************************************************************************************************/
        GameId getId() const { return mGameId; }

        /*! ************************************************************************************************/
        /*! \brief Returns the current game's persisted game id
            
            The persisted game id can be used as bookmark by game team. It only works if the enablePersistedGameId
            in GameSetting is enabled.

            Note: the persisted game id is CASE_SENSITIVE
            
            \return returns the reference of persisted game id
        ***************************************************************************************************/
        const PersistedGameId& getPersistedGameId() const { return mPersistedGameId; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if the game has a non-empty Persisted Game Id
        ***************************************************************************************************/
        bool hasPersistedGameId() const { return mPersistedGameId.c_str()[0] != '\0'; }

        /*! ************************************************************************************************/
        /*! \brief Returns the current game's CCS pool name

            The CCS pool is a group of resources used for establishing a connection between two peers.
            A CCS pool will only be specified if it has been overridden for this game's game mode.

            \return returns CCS pool name
        ***************************************************************************************************/
        const char8_t* getCCSPool() const { return mCCSPool; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the name of this game. (Note: not unique, not profanity filtered)

            Note: game names are not unique, and are not passed through the profanity filter.
        **************************************************************************************************************/
        const char *getName() const { return mName.c_str(); }

        /*! ************************************************************************************************/
        /*! \brief return the game's GameProtocolVersionString.

            The GameProtocolVersionString isolates games that cannot communicate with each other due to version differences.
            You cannot join (or find via matchmaking) a game unless its protocol version matches yours.

            This setting is also commonly used during development to isolate developers from each other (by setting
                each dev's game protocol version to a unique value, such as a phone extension).
        
            Note: the blazeServer contains a setting to universally enable/disable GameProtocolVersionString checking;
                by default it is ENABLED.

            \return the game's GameProtocolVersionString
        ***************************************************************************************************/
        const char8_t* getGameProtocolVersionString() const { return mGameProtocolVersionString.c_str(); }

         /*! **********************************************************************************************************/
        /*! \brief Returns this game's 1st party presence mode
        **************************************************************************************************************/
        PresenceMode getPresenceMode() const { return mPresenceMode; }

        /*! **********************************************************************************************************/
        /*! \brief Returns this game's VOIP network topology
        **************************************************************************************************************/
        VoipTopology getVoipTopology() const { return mVoipTopology; }

        /*! **********************************************************************************************************/
        /*! \brief Returns this game's game type
        **************************************************************************************************************/
        GameType getGameType() const { return mGameType; }

        /*! ************************************************************************************************/
        /*! \brief Returns true if this is a game group
        ***************************************************************************************************/
        bool isGameTypeGroup() const { return mGameType == GAME_TYPE_GROUP; }

        /*! ************************************************************************************************/
        /*! \brief Return the game's current GameState (INITIALIZING, PRE_GAME, IN_GAME, etc).
        ***************************************************************************************************/
        GameState getGameState() const { return mState; }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if the game is in progress (the host has already called 
                   GameManagerHostedGame::StartGame but has not yet called GameManagerHostedGame::EndGame).

               While the concept of "in progress" is generic, the primary purpose of using GameManagerHostedGame::StartGame
               is to coordinate Xbox 360 arbitration for TCR compliance

        **************************************************************************************************************/
        bool isGameInProgress() const { return mState == IN_GAME; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's platform dependent end point address.
        **************************************************************************************************************/
        virtual const NetworkAddress* getNetworkAddress() const;

        /*! ************************************************************************/
        /*!    \brief  returns the BlazeId of the game's network topology host.
                    Note: the host of a dedicated server game is not a player in the game.

            \return  The BlazeId of the user who is hosting this game.
        ****************************************************************************/
        BlazeId getTopologyHostId() const { return mTopologyHostId; }

        /*! **********************************************************************************************************/
        /*! \brief returns the BlazeId of the game's network topology host.
                Note: the host of a dedicated server game is not a player in the game.
            \deprecated use getTopologyHostId()
            \return  The BlazeId of the user who is hosting this game.
        **************************************************************************************************************/
        BlazeId getHostId() const { return getTopologyHostId(); }


    /*! **********************************************************************************************************/
    /*! \name Game Capacity (GameBase)
    **************************************************************************************************************/

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of players in the game (all player slot types).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getPlayerCount() const = 0;

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of participants in the game (all spectator slot types).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getParticipantCount() const = 0;

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of spectators in the game (all spectator slot types).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getSpectatorCount() const = 0;

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of users in the game roster (all slot types, players and spectators, including reservations).
            \return    Returns the number of users in this game.
        **************************************************************************************************************/
        virtual uint16_t getRosterCount() const = 0;

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of users in the queue (all slot types, players and spectators).
            \return    Returns the number of users in this game's queue.
        **************************************************************************************************************/
        virtual uint16_t getQueueCount() const = 0;

        /*! **********************************************************************************************************/
        /*! \brief returns the number of players in the game of a specific slot type.
            \param[in] slotType the type of slot requested.
            \return    the number of players in the slot type
        **************************************************************************************************************/
        uint16_t getPlayerCount(SlotType slotType) const
        {
            BlazeAssert(slotType < MAX_SLOT_TYPE);
            return mPlayerSlotCounts[slotType];
        }

        /*! **********************************************************************************************************/
        /*! \brief Return the player capacity of a specific slotType.

            The playerCapacity dictates the max number of players (or a specific slotType) who can be in the game.
            Note: if a player joins a game using a private slot, they will consume a public slot if all the private
            slots are taken.

            \param[in] slotType the slot type (public/private) to get the capacity for
        **************************************************************************************************************/
        uint16_t getPlayerCapacity(SlotType slotType) const
        {
            BlazeAssert(slotType < MAX_SLOT_TYPE);
            return mPlayerCapacity[slotType];
        }

        /*! **********************************************************************************************************/
        /*! \brief Returns the total player capacity of the game (sum of each player slotType's capacity)

            The playerCapacity dictates the max number of player who can be in the game.
            This value is always <= the maxPlayerCapacity (see getMaxPlayerCapacity())
        **************************************************************************************************************/
        uint16_t getPlayerCapacityTotal() const
        {
            uint16_t totalCapacity = 0;

            for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_PARTICIPANT; slotType != MAX_SLOT_TYPE; ++slotType)
            {
                // cast needed due to warnings treated as errors
                totalCapacity = static_cast<uint16_t>(totalCapacity + mPlayerCapacity[slotType]);
            }

            return totalCapacity;
        }

        /*! **********************************************************************************************************/
        /*! \brief Returns the total participant capacity of the game (sum of each participant slotType's capacity)

            The playerCapacity dictates the max number of participants who can be in the game.
            This value is always <= the maxPlayerCapacity (see getMaxPlayerCapacity())
        **************************************************************************************************************/
        uint16_t getParticipantCapacityTotal() const
        {
            uint16_t totalCapacity = 0;

            for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_PARTICIPANT; slotType != MAX_PARTICIPANT_SLOT_TYPE; ++slotType)
            {
                // cast needed due to warnings treated as errors
                totalCapacity = static_cast<uint16_t>(totalCapacity + mPlayerCapacity[slotType]);
            }

            return totalCapacity;
        }

        /*! **********************************************************************************************************/
        /*! \brief Returns the total spectator capacity of the game (sum of each spectator slotType's capacity)

            The spectator capacity dictates the max number of spectators who can be in the game.
            This value is always <= the maxPlayerCapacity (see getMaxPlayerCapacity())
        **************************************************************************************************************/
        uint16_t getSpectatorCapacityTotal() const
        {
            uint16_t totalCapacity = 0;

            for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_SPECTATOR; slotType != MAX_SLOT_TYPE; ++slotType)
            {
                // cast needed due to warnings treated as errors
                totalCapacity = static_cast<uint16_t>(totalCapacity + mPlayerCapacity[slotType]);
            }

            return totalCapacity;
        }

        /*! **********************************************************************************************************/
        /*! \brief  returns the max possible player count for this game (inclusive of participants and spectators; 
                this is a hard cap established at game creation and cannot be changed).

            Note: The BlazeServer has a compile-time limit for this value (default blazeServer only allows up to 256 players in a game).
            If you need larger games, contact your blaze producer.

            \sa
            \li Game::CreateGameParameters::mMaxPlayerCapacity
            \li Game::setPlayerCapacity()

            \return    Returns the maximum number of players possible for this game.
        **************************************************************************************************************/
        uint16_t getMaxPlayerCapacity() const { return mMaxPlayerCapacity; }

        /*! **********************************************************************************************************/
        /*! \brief  Get min player capacity of the game
        \return    Returns the minimum number of active participants and spectators this game's slots totals can be set to.
        **************************************************************************************************************/
        uint16_t getMinPlayerCapacity() const { return mMinPlayerCapacity; }

        /*! **********************************************************************************************************/
        /*! \brief  Get capacity of the join queue.
        \return    Returns the maximum number of active participants and spectators this game can have.
        **************************************************************************************************************/
        uint16_t getQueueCapacity() const { return mQueueCapacity; }

    /*! **********************************************************************************************************/
    /*! \name Game Team Information
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief return the number of teams in the game.
        ***************************************************************************************************/
        uint16_t getTeamCount() const { return (uint16_t) mTeamInfoVector.size(); }

        /*! ************************************************************************************************/
        /*! \brief return the teamId at a given index (range is from 0..(getTeamIdCount()-1)).  returns INVALID_TEAM_ID
        if the index is out of bounds.
        \param[in] index the id of the team requested.
        \return    the TeamId of the requested team, INVALID_TEAM_ID if it isn't in the game.
        ***************************************************************************************************/
        TeamId getTeamIdByIndex(TeamIndex index) const;

        /*! **********************************************************************************************************/
        /*! \brief returns the number of players in the game for a specific teamId.
        \param[in] teamIndex the id of the team requested.
        \return    the number of players in the team, 0 if team doesn't exist.
        **************************************************************************************************************/
        uint16_t getTeamSizeByIndex(TeamIndex teamIndex) const;

        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Return the player capacity of a specific team; returns 0 if team not present in the game.

        The playerCapacity dictates the max number of players (of a specific team) who can be in the game.
        \deprecated use getTeamCapacity instead.

        \param[in] teamIndex the id of the team capacity to lookup
        \return the capacity of the given team; 0 if team is not present in game.
        **************************************************************************************************************/
        uint16_t getTeamCapacityByIndex(TeamIndex teamIndex) const;

        /*! **********************************************************************************************************/
        /*! \brief Return the player capacity for teams in this game
            Integrators note: this method should be used in place of the currently deprecated getTeamCapacityByIndex() method.
            \return teams' capacity
        **************************************************************************************************************/
        uint16_t getTeamCapacity() const;

        /*! **********************************************************************************************************/
        /*! \brief Return the player capacity for a role in the game
            \return the capacity of the given role, 0 for roles that aren't allowed in the game
        **************************************************************************************************************/
        uint16_t getRoleCapacity(const RoleName& roleName) const;

        /*! **********************************************************************************************************/
        /*! \brief returns the number of players in the game, with a specific role, for a specific teamId.
        \param[in] teamIndex the id of the team requested.
        \param[in] roleName the role to get the current size of
        \return    the number of players in the team, with this role, 0 if team doesn't exist.
        **************************************************************************************************************/
        uint16_t getRoleSizeForTeamAtIndex(TeamIndex teamIndex, const RoleName& roleName) const;

        /*! **********************************************************************************************************/
        /*! \brief returns the role information and capacites for this game session
        \return    The game session's role information.
        **************************************************************************************************************/
        const RoleInformation& getRoleInformation() const { return mRoleInformation; }
    /*! **********************************************************************************************************/
    /*! \name Game Admin Rights (GameBase)
    **************************************************************************************************************/

        /*! **********************************************************************************************************/
        /*! \brief Returns true if the supplied playerId is an admin for this game.
            \param    playerId the BlazeId (playerId) to check for admin rights
            \return    True if this is an admin player.
        **************************************************************************************************************/
        bool isAdmin(BlazeId playerId) const;

        /*! **********************************************************************************************************/
        /*! \brief get the list of user ids (BlazeId / PlayerId) who have admin rights on this game.

            \sa
            \li bool GameBase::isAdmin(BlazeId) const
            \li bool Player::isAdmin() const

            \return    The list of user ids (BlazeId / PlayerId) who have admin rights on this game.
        **************************************************************************************************************/
        const PlayerIdList& getAdminList() const { return mAdminList; }

        /*! **********************************************************************************************************/
        /*! \brief  Get the network topology of the game.
        \return    Returns the network topology of the gamee.
        **************************************************************************************************************/
        const GameNetworkTopology& getNetworkTopology() const { return mNetworkTopology; }

        /*! **********************************************************************************************************/
        /*! \brief  Get the ping site of the game.
        \return    Returns the ping site of the game.
        **************************************************************************************************************/
        const PingSiteAlias& getPingSite() const { return mPingSite; }

    /*! **********************************************************************************************************/
    /*! \name Game Settings (GameBase)
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Returns the current GameSettings TDF object (which contains the game's join flags and ranked setting.

            Note: helper for each of the individual settings are provided on the game object.
            For example, see isClosedToAllJoins() or isRanked()

            \return returns the game settings object.
        ***************************************************************************************************/
        const GameSettings& getGameSettings() const { return mGameSettings; }

        /*! **********************************************************************************************************/
        /*! \brief Determines whether game was created as ranked. Sets whether game is marked as Ranked. 
                   This is generally meaningful only for games on Xbox 360.
            \return    Returns True if this game was created as ranked, else returns False.
        **************************************************************************************************************/
        bool isRanked() const { return mGameSettings.getRanked(); }

        /*! **********************************************************************************************************/
        /*! \brief GameModRegister is bitset where each bit represents special feature (DLC) that this game requires
                   client to have to be able to matchmake against
            \return    Returns GameModReigister bitset
        **************************************************************************************************************/
        GameModRegister getGameModRegister() const { return mGameModRegister; }
    
    /*! **********************************************************************************************************/
    /*! \name Game Attribute Maps (GameBase)
    **************************************************************************************************************/

         /*! **********************************************************************************************************/
         /*! \brief  returns the game mode attribute value for this game.
                     Note: virtual because GB game data has a dedicated member to cache off the game mode,
                     to speed mode checking when dealing with large numbers of game objects. Local game sessions don't
                     cache the mode separately from the GameAttributes, and do a lookup to return the mode.
             \return    Returns the game mode attribute value for this game.
        **************************************************************************************************************/
        virtual const char8_t* getGameMode() const = 0;

        /*! ************************************************************************************************/
        /*! \brief Returns the value of the supplied game attribute (or nullptr, if no attribute exists with that name).

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            Note: You should avoid caching off pointers to values returned as they can be invalidated
                when game attributes are updated.  Pointers will possibly be invalidated during a call to
                BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
                cause the pointer to be invalidated.

            \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
            \return the value of the supplied attribute name (or nullptr, if the attib doesn't exist).
        ***************************************************************************************************/
        const char8_t* getGameAttributeValue(const char8_t* attributeName) const;

        /*! ************************************************************************************************/
        /*! \brief Returns the value of the supplied dedicated server attribute (or nullptr, if no attribute exists with that name).

        Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
        Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

        Note: You should avoid caching off pointers to values returned as they can be invalidated
        when game attributes are updated.  Pointers will possibly be invalidated during a call to
        BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
        cause the pointer to be invalidated.

        \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
        \return the value of the supplied attribute name (or nullptr, if the attib doesn't exist).
        ***************************************************************************************************/
        const char8_t* getDedicatedServerAttributeValue(const char8_t* attributeName) const;

        /*! ************************************************************************************************/
        /*! \brief Returns a const reference to the game's attribute map.
            
            Note: You should avoid caching off pointers to values inside the map as they can be invalidated
                when game attributes are updated.  Pointers will possibly be invalidated during a call to
                BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
                cause the pointer to be invalidated.
            
            \return Returns a const reference to the game's attribute map.
        ***************************************************************************************************/
        const Blaze::Collections::AttributeMap& getGameAttributeMap() const { return mGameAttributeMap; }

        /*! ************************************************************************************************/
        /*! \brief Returns a const reference to the dedicated server's attribute map.

        Note: You should avoid caching off pointers to values inside the map as they can be invalidated
        when game attributes are updated.  Pointers will possibly be invalidated during a call to
        BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
        cause the pointer to be invalidated.

        \return Returns a const reference to the dedicated server's attribute map.
        ***************************************************************************************************/
        const Blaze::Collections::AttributeMap& getDedicatedServerAttributeMap() const { return mDedicatedServerAttributeMap; }

    /*! **********************************************************************************************************/
    /*! \name Game Entry Criteria (GameBase)
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief get this game's EntryCriteria map.

            Each game can be created with a map of named EntryCriteria rules.  These rules are evaluated
            when a player tries to join the game.  A player must pass the entry criteria to join the game (each formula
            must evaluate to true for the player).

            The EntryCriteria syntax is shared with the Stats system (for derived statistics).

            Entry Criteria are initialized when a game is created or reset, and can only be changed if the game is reset
            again.

            \sa
            \li CreateGameParameters::mEntryCriteriaMap
            \li ResetGameParameters::mEntryCriteriaMap

            \return Returns a const reference to the game's entry criteria map.
        ***************************************************************************************************/
        const Blaze::EntryCriteriaMap& getEntryCriteriaMap() const { return mEntryCriteria; }


        bool isCrossplayEnabled() const { return mIsCrossplayEnabled; }

        /*! **********************************************************************************************************/
        /*! \brief List of platform restrictions set for Crossplay purposes:
                   May be empty, if no restrictions were explicitly set for the Game.

                   Although the underlying value on the Server may dynamically update as new platforms enter the Game, this value does not currently. 
        **************************************************************************************************************/
        const ClientPlatformTypeList& getClientPlatformOverrideList() const { return mClientPlatformList; }


    //! \cond INTERNAL_DOCS

        // NOTE: the base class doesn't provide an interface for getting players because we don't know the type of player
        //  to return (Player* vs GameBrowserPlayer*).

    protected:

        /*! ************************************************************************************************/
        /*! \brief Init the GameBase data from the supplied replicatedGameData obj.
            
            \param[in] replicatedGameData the obj to init the game's base data from
            \param[in] memGroupId mem group to be used by this class to allocate memory 
        ***************************************************************************************************/
        GameBase(const ReplicatedGameData *replicatedGameData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        /*! ************************************************************************************************/
        /*! \brief Init the GameBase data from the supplied gameBrowserGameData obj.
            
            \param[in] gameBrowserGameData the obj to init the game's base data from
            \param[in] memGroupId mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        GameBase(const GameBrowserGameData &gameBrowserGameData, MemoryGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        //! \brief destructor
        virtual ~GameBase() {}

        /*! ************************************************************************************************/
        /*! \brief initialize the base game data from the supplied replicatedGameData object.  All old values are wiped.
            \param[in] replicatedGameData the new data for the game
        ***************************************************************************************************/
        void initGameBaseData(const ReplicatedGameData *replicatedGameData);

        /*! ************************************************************************************************/
        /*! \brief initialize the base game data from the supplied gameBrowserGameData object. Note: we only update fields that we gameBrowserGameData knows about.
            \param[in] gameBrowserGameData the new data for the game
        ***************************************************************************************************/
        void initGameBaseData(const GameBrowserGameData &gameBrowserGameData);

        

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's network address list
        **************************************************************************************************************/
        virtual NetworkAddressList* getNetworkAddressList() = 0;

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's network address list
        **************************************************************************************************************/
        const NetworkAddressList* getNetworkAddressList() const { return const_cast<GameBase*>(this)->getNetworkAddressList(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the network address for a given network address list
        **************************************************************************************************************/
        const NetworkAddress* getNetworkAddress(const NetworkAddressList* networkAddressList) const;





    protected:

        typedef Blaze::vector_map<RoleName, uint16_t, CaseInsensitiveStringLessThan> RoleSizeMap;
        //TeamInfo stores the size and id for each team in a game
        struct TeamInfo
        {
            TeamId mTeamId;
            uint16_t mTeamSize;
            RoleSizeMap mRoleSizeMap;

            TeamInfo(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP)
                : mTeamId(ANY_TEAM_ID),
                mTeamSize(0),
                mRoleSizeMap(memGroupId, MEM_NAME(memGroupId, "GameBase::TeamInfo.mRoleSizeMap"))
            {}
        };

        typedef Blaze::vector_map<TeamId, TeamIndex> TeamIndexMap;
        typedef Blaze::vector<TeamInfo> TeamInfoVector;

        // Note: GameBase doesn't include player data, since we don't know if we're actually a game or a GameBrowserGame

        GameId mGameId;
        GameName mName;
        PresenceMode mPresenceMode;
        GameState mState;
        GameSettings mGameSettings;
        GameModRegister mGameModRegister;
        SlotCapacitiesVector mPlayerCapacity;
        SlotCapacitiesVector mPlayerSlotCounts; // how many players do we have in each kind of slot
        TeamIndexMap mTeamIndexMap;
        TeamInfoVector mTeamInfoVector;
        RoleInformation mRoleInformation;
        uint16_t mMaxPlayerCapacity;
        uint16_t mMinPlayerCapacity;
        uint16_t mQueueCapacity;
        GameProtocolVersionString mGameProtocolVersionString;
        Collections::AttributeMap mGameAttributeMap;
        Collections::AttributeMap mDedicatedServerAttributeMap;
        ClientPlatformTypeList mClientPlatformList;
        bool mIsCrossplayEnabled;
        PlayerIdList mAdminList;

        EntryCriteriaMap mEntryCriteria;

        PingSiteAlias mPingSite;
        GameNetworkTopology mNetworkTopology;
        GameType mGameType;

        VoipTopology mVoipTopology;
        BlazeId mTopologyHostId;

        PersistedGameId mPersistedGameId;
        char8_t mCCSPool[64];

    //! \endcond

    };

} //namespace GameManager
} //namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_BASE_H
