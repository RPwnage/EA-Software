/*! ************************************************************************************************/
/*!
    \file game.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_H
#define BLAZE_GAMEMANAGER_GAME_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/usergroup.h"

#include "BlazeSDK/networkmeshadapter.h" // for NetworkMeshAdapterListener
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/gamemanager/gamebase.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

#include "BlazeSDK/memorypool.h"
#include "BlazeSDK/mesh.h"

#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/hash_set.h"
#include "BlazeSDK/blaze_eastl/map.h"

namespace Blaze
{

namespace UserManager
{
    class User;
}

namespace GameManager
{
    class Player;
    class GameEndpoint;
    class GameListener;
    class GameManagerAPI;
    class GameManagerApiJob;
    class TelemetryReportRequest;

    /*! **********************************************************************************************************/
    /*! \class Game
        \brief Local representation of a game session on the blazeServer.


        Game object instances are not constructed directly; the GameManagerAPI creates a local game instance when
        you create or join a game.

        Note: Game implements the UserGroup interface; this allows you to take certain actions on the players in a game 
            (ex: sending a message to all the members).

        Game pointers can be persisted between calls to BlazeHub::idle; pointers are valid until the 
        GameManagerAPI dispatches GameManagerAPIListener::onGameDestructing.
    **************************************************************************************************************/
    class BLAZESDK_API Game : 
        public GameBase, 
        public Blaze::Mesh,
        public UserGroup
    {
        //! \cond INTERNAL_DOCS
        friend class GameManagerAPI;
        friend class Player;
        friend class GameManagerApiJob;
        friend class MemPool<Game>;
        //! \endcond
    public:
        //static const char8_t* GAME_PROTOCOL_DEFAULT_VERSION;   //defined here since it's being used as fnc default param and if BlazeSDK is being built as a Dll, title will require access to it

        /*! ****************************************************************************/
        /*! \brief contains a game's initial settings, attributes, and other data.
        ********************************************************************************/
        struct BLAZESDK_API CreateGameParametersBase
        {
            /*! ************************************************************************************************/
            /*! \brief Default constructor (open 8 player game, no attributes, no queue, etc...)

                \param[in] memGroupId   mem group to be used by this class to allocate memory

                Default values:
                    \li mGameName: ""
                    \li mGameSettings: open to browsing, invites, matchmaking. Integrators note: the mIgnoreEntryCriteriaWithInvite parameter available in older versions of Blaze has now been moved into mGameSettings.
                    \li mPresenceMode: PRESENCE_MODE_STANDARD
                    \li mNetworkTopology: CLIENT_SERVER_PEER_HOSTED
                    \li mVoipTopology: VOIP_DISABLED
                    \li mGameAttributes: NONE
                    \li mMeshAttributes: NONE
                    \li mMaxPlayerCapacity: 8
                    \li mMinPlayerCapacity: 1
                    \li mPlayerCapacity[SLOT_PUBLIC_PARTICIPANT]: 8
                    \li mPlayerCapacity[SLOT_PRIVATE_PARTICIPANT]: 0
                    \li mPlayerCapacity[SLOT_PUBLIC_SPECTATOR]: 0
                    \li mPlayerCapacity[SLOT_PRIVATE_SPECTATOR]: 0
                    \li mTeamCapacities : empty
                    \li mQueueCapacity: 0
                    \li mJoiningSlotType: SLOT_PUBLIC_PARTICIPANT
                    \li mJoiningTeamId: INVALID_TEAM_ID
                    \li mJoiningTeamIndex: UNSPECIFIED_TEAM_INDEX
                    \li mJoiningEntryType: GAME_ENTRY_TYPE_DIRECT
                    \li mEntryCriteriaMap: NONE
                    \li mGamePingSiteAlias: ""
                    \li mGameStatusUrl: ""
                    \li mReservedGameId: GameManager::INVALID_GAME_ID
                    \li mSessionTemplateName: ""


            ***************************************************************************************************/
            CreateGameParametersBase(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            

            GameName mGameName; //!< The game's name.  Note: uniqueness is not enforced.
            GameSettings mGameSettings; //!< The game's ranked, host migration, and game entry mode flags
            GameModRegister mGameModRegister; //!< Bitset indicating which special features (DLCs) game will use; clients must have these features themselves in order to matchmake against the game
            PresenceMode mPresenceMode; //!< Whether the game uses 1st party presence (defaults to PRESENCE_MODE_STANDARD)
            GameType mGameType; //!< The game type to use for the game (defaults to GAME_TYPE_GAMESESSION)
            GameNetworkTopology mNetworkTopology; //!< The network topology to use for the game (immutable after game creation)
            VoipTopology mVoipTopology; //!< Network Topology used for VOIP connections (immutable after game creation)
            Collections::AttributeMap mGameAttributes; //!< the initial game attributes of the game, can be updated after game creation
            Collections::AttributeMap mMeshAttributes; //!< the network mesh attributes for the game (typically set by a dedicated game server)
            uint16_t mMaxPlayerCapacity; //!< The upper limit on the maximum number of players (immutable after game creation).  Note: the blazeServer has a compile-time limit for this value (default blazeServer only allows up to 256 players in a game).  If you need larger games, contact your blaze producer.
            uint16_t mMinPlayerCapacity; //!< The lower limit on the maximum number of players (immutable after game creation).  (defaults to 1)
            SlotCapacities mPlayerCapacity; //!< An array of the initial number of available slots for different categories of players to join the game (sum of all types must be no more than max capacity)
            TeamIdVector mTeamIds; //!< A listing of the teams present in this game. GameManager requires that games have at least 1 team.
            uint16_t mTeamCapacity; //!< DEPRECATED - This value is ignored in favor of deriving the team capacity from the team count and total player capacity.
            uint16_t mQueueCapacity; //!< The initial capacity of the game queue.  If set to 0 (default), no players will be queued when the game is full.  Note: this value will be overridden on the blazeServer if it exceeds the configured queueCapacityMax value in /etc/components/gamemanager/gamemanager.cfg
            EntryCriteriaMap mEntryCriteriaMap; //!< Players must pass these entry criteria to join this game.
            PingSiteAlias mGamePingSiteAlias; //!< Indicates the game's data center. The game will determine it's own data center (using host's best ping site) if the alias is left as empty during game creation. Available locations can be found in server's configuration file: trunk/etc/component/util/util.cfg.
            GameStatusURL mGameStatusUrl; //!< The URL used to retrieve additional game status information by external tools interfacing with Blaze. Frequently used for games created by dedicated game servers, and used to specify a voip server URL.
            PersistedGameId mPersistedGameId; //!< Indicates the game's persisted game id. When it's empty, and the flag in game setting is true, server will generate one for the game.
            PersistedGameIdSecret mPersistedGameIdSecret; // !< the secret passphrase for the persisted game id.
            GameReportName mGameReportName; //!< GameReportName for the game - game types are defined inside gamereporting.cfg.  If this is not set, game reporting will not work for this match.
            bool mServerNotResetable; // !< Indicates that a dedicated server will not be allowed to enter the resetable state. - can be used for replay only type servers
            RoleInformation mRoleInformation; //!< Map of roles and their per-team max capacities for the game. The total max capacities of all roles must be >= the capacity of a team in the game session.
            GamePermissionSystemName mPermissionSystemName; //!< Name of the permissions system that will be used by the game.  Permission systems define what actions that players, spectators, the host, and admins can perform.  (See gamesession.cfg on the server)
 
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            Blaze::XblSessionTemplateName mSessionTemplateName; //!< The external session template name for initializing external game session (applicable to xbox only). If omitted, the default session template name configured on the server will be used
            Blaze::ExternalSessionCustomDataString mExternalSessionCustomDataString; //!< The external session custom data.
#endif
#if (defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)) && !defined(EA_PLATFORM_PS4_CROSSGEN)
            Blaze::ExternalSessionStatus mExternalSessionStatus; //!< The external session status.
#endif
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            Blaze::ExternalSessionCustomData mExternalSessionCustomData; //!< The external session custom data.
#endif
        };

        /*! ****************************************************************************/
        /*! \brief [DEPRECATED] Backwards compatibility class for use with the old GameManagerAPI::createGame function. 
                   New code should just use the CreateGameParametersBase, along with the PlayerJoinData class and functions.
        ********************************************************************************/
        struct BLAZESDK_API CreateGameParameters : public CreateGameParametersBase
        {
            CreateGameParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            // This data is now normally stored as part of the PlayerJoinData
            SlotType mJoiningSlotType; //!< When creating a non-dedicated server game, this specifies the SlotType for the game host to consume.
            TeamIndex mJoiningTeamIndex; //!< When creating a non-dedicated server game, this specifies which team joining players will use. Must be a team specified in TeamCapacites at the given index or UNSPECIFIED_TEAM_INDEX if teams are not enabled for this game
            GameEntryType mJoiningEntryType; //!< When creating a non-dedicated server game, this specifies how the joining players will enter the game.  Players enter either directly, or can only reserve a space to be claimed later.
            RoleNameToPlayerMap mPlayerRoles; //!< Map of roles to list of the joining players.
            RoleNameToUserIdentificationMap mUserIdentificationRoles; //!< Map of roles to list of the joining external players.
        };


        /*! ****************************************************************************/
        /*! \brief Contains new game data, settings, and attributes to reset a dedicated server game
        ********************************************************************************/
        struct BLAZESDK_API ResetGameParametersBase
        {
            /*!************************************************************************************************/
            /*! \brief Default constructor.

                \param[in] memGroupId  mem group to be used by this class to allocate memory
                
                Default values:
                    \li mGameName: ""
                    \li mGameSettings: open to browsing, invites, matchmaking. Integrators note: the mIgnoreEntryCriteriaWithInvite parameter available in older versions of Blaze has now been moved into mGameSettings.
                    \li mPresenceMode: PRESENCE_MODE_STANDARD
                    \li mGameAttributes: NONE
                    \li mPlayerCapacity[SLOT_PUBLIC_PARTICIPANT]: 8
                    \li mPlayerCapacity[SLOT_PRIVATE_PARTICIPANT]: 0
                    \li mPlayerCapacity[SLOT_PUBLIC_SPECTATOR]: 0
                    \li mPlayerCapacity[SLOT_PRIVATE_SPECTATOR]: 0
                    \li mTeamCapacities : empty
                    \li mQueueCapacity: 0
                    \li mJoiningSlotType: SLOT_PUBLIC_PARTICIPANT
                    \li mJoiningTeamId: INVALID_TEAM_ID
                    \li mJoiningTeamIndex: UNSPECIFIED_TEAM_INDEX
                    \li mEntryCriteriaMap: NONE
                    \li mSessionTemplateName: ""

            ***************************************************************************************************/
            ResetGameParametersBase(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            GameName mGameName; //!< The game's name.  Note: uniqueness is not enforced.
            GameSettings mGameSettings; //!< The game's ranked, host migration, and game entry mode flags
            GameModRegister mGameModRegister; //!< Bitset indicating which special features (DLCs) game will use; clients must have these features themselves in order to matchmake against the game
            PresenceMode mPresenceMode; //!< Whether the game uses 1st party presence (defaults to PRESENCE_MODE_STANDARD)
            GameNetworkTopology mNetworkTopology; //!< The network topology to use for the game (immutable after game creation)
            VoipTopology mVoipTopology;//!< Network Topology used for VOIP connections (immutable after game creation)
            Collections::AttributeMap mGameAttributes; //!< the initial game attributes of the game
            SlotCapacities mPlayerCapacity; //!< An array of the initial number of available slots for different categories of players to join the game (sum of all types must be no more than max capacity)
            TeamIdVector mTeamIds; //!< A listing of the teams present in this game. GameManager requires that games have at least 1 team.
            uint16_t mTeamCapacity; //!< DEPRECATED - The capacity for each team in the game, all teams must have the same capacity, and mTeamIds.size() * mTeamCapacities must be equal to mPlayerCapacity
            uint16_t mQueueCapacity; //!< The initial capacity of the game queue.  If set to 0 (default), no players will be queued when the game is full.
            EntryCriteriaMap mEntryCriteriaMap; //!< Players must pass these entry criteria to join this game.
            PersistedGameId mPersistedGameId; //!< Indicates the game's persisted game id. When it's empty, and the flag in game setting is true, server will generate one for the game.
            PersistedGameIdSecret mPersistedGameIdSecret; // !< the secret passphrase for the persisted game id.
            RoleInformation mRoleInformation; //!< Map of roles and their per-team max capacities for the game. The total max capacities of all roles must be >= the capacity of a team in the game session.
            GamePermissionSystemName mPermissionSystemName; //!< Name of the permissions system that will be used by the game.  Permission systems define what actions that players, spectators, the host, and admins can perform.  (See gamesession.cfg on the server)
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            Blaze::XblSessionTemplateName mSessionTemplateName; //!< The external session template name for initializing external game session (applicable to xbox only). If omitted, the default session template name configured on the server will be used
            Blaze::ExternalSessionCustomDataString mExternalSessionCustomDataString; //!< The external session custom data.
#endif
#if defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            Blaze::ExternalSessionStatus mExternalSessionStatus; //!< The external session status.
#endif
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            Blaze::ExternalSessionCustomData mExternalSessionCustomData; //!< The external session custom data.
#endif
        };

        /*! ****************************************************************************/
        /*! \brief [DEPRECATED] Backwards compatibility class for use with the old GameManagerAPI::resetGame function. 
                   New code should just use the ResetGameParametersBase, along with the PlayerJoinData class and functions.
        ********************************************************************************/
        struct BLAZESDK_API ResetGameParameters : public ResetGameParametersBase
        {
            ResetGameParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            // This data is now normally stored as part of the PlayerJoinData
            SlotType mJoiningSlotType; //!< When creating a non-dedicated server game, this specifies the SlotType for the game host to consume.
            TeamIndex mJoiningTeamIndex; //!< When creating a non-dedicated server game, this specifies which team joining players will use. Must be a team specified in TeamCapacites at the given index or UNSPECIFIED_TEAM_INDEX if teams are not enabled for this game
            GameEntryType mJoiningEntryType; //!< When creating a non-dedicated server game, this specifies how the joining players will enter the game.  Players enter either directly, or can only reserve a space to be claimed later.
            RoleNameToPlayerMap mPlayerRoles; //!< Map of roles to list of the joining players.
            RoleNameToUserIdentificationMap mUserIdentificationRoles; //!< Map of roles to list of the joining external players.
        };

    public:

    /*! ************************************************************************************************/
    /*! \name General Game Data Accessors

        Note: we also inherit a number of general game data accessors from GameBase.
        They are documented below in the section named "General Game Data Accessors (GameBase)"
    ***************************************************************************************************/


        /*! **********************************************************************************************************/
        /*! \brief Returns the game's shared seed
        \return game's shared seed
        **************************************************************************************************************/
        uint32_t getSharedSeed() const { return mSharedSeed; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's reporting id (a unique identifier used by the game reporting component)
            \return the game's reporting id
        **************************************************************************************************************/
        GameReportingId getGameReportingId() const { return mGameReportingId; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's reporting game type name (defined inside gamereporting.cfg)
            \return the game's reporting game type name
        **************************************************************************************************************/
        GameReportName getGameReportName() const { return mGameReportName; }

        /*! ************************************************************************************************/
        /*! \brief return the BlazeId (aka PlayerId) of the game's topology host.
        ***************************************************************************************************/
        BlazeId getTopologyHostId() const {return mTopologyHostId;}
        BlazeId getHostId() const { return getTopologyHostId(); }

        /*! ************************************************************************************************/
        /*! \brief return the BlazeId (aka PlayerId) of the game's dedicated server host.  NOTE: this is the only way to get
                the id of a dedicated server host , since there's no player object for a dedicated server host.
            \return the host's BlazeId (which is also their playerId)
        ***************************************************************************************************/
        BlazeId getDedicatedServerHostId() const {return mDedicatedServerHostId;}

         /*! ************************************************************************************************/
        /*! \brief (DEPRECATED) return the player object for the game's topology host, if there is one. Will return nullptr for dedicated server topologies.

            \deprecated  Use getTopologyHostEndpoint() instead, to address the host machine, rather than the individual player.
                    
            \return the host's BlazeId (which is also their playerId)
        ***************************************************************************************************/
        // platform host player and topology host player are always the same if we're not a dedicated server game.
        Player *getHostPlayer() const { return (!isDedicatedServerTopology())? getPlatformHostPlayer() : nullptr; }
       
        /*! **********************************************************************************************************/
        /*! \brief Determine the type of network topology the game is set to use

            \return    Returns a constant specifying the type of networking this game uses, one of the following:
                   \li CLIENT_SERVER_PEER_HOSTED (players connect only with the game host)
                   \li CLIENT_SERVER_DEDICATED (players connect to a dedicated server)
                \li PEER_TO_PEER_FULL_MESH (each player must connect with every other player and the game host)
        **************************************************************************************************************/
        GameNetworkTopology getNetworkTopology() const { return mNetworkTopology; }

        uint32_t getQosDuration() const { return mQosDurationMs; }
        uint32_t getQosInterval() const { return mQosIntervalMs; }
        uint32_t getQosPacketSize() const { return mQosPacketSize; }

        /*! **********************************************************************************************************/
        /*! \brief Helper function to determine if this game contains a dedicated server in its topology

            \return Returns true if the network topology contains a dedicated server.
        **************************************************************************************************************/
        bool isDedicatedServerTopology() const { return (mNetworkTopology == CLIENT_SERVER_DEDICATED); }
        /*DEPRECATED - Use isDedicatedServerTopology()*/
        bool isDedicatedServer() const { return isDedicatedServerTopology(); }

        /*! ************************************************************************************************/
        /*! \brief returns the VoipTopology defining the type of Voip network the game supports.
            \return VoipTopology describing the Voip network.
        ***************************************************************************************************/
        VoipTopology getVoipTopology() const { return mVoipTopology; }

        /*! ************************************************************************************************/
        /*! \brief Helper function to determine if this game group uses mesh connections and game networking setup
        ***************************************************************************************************/
        bool usesGamePlayNetworking() const { return getNetworkTopology() != NETWORK_DISABLED; }

        /*! ************************************************************************************************/
        /*! \brief Returns the pointer previously set by setTitleContextData.
        ***************************************************************************************************/
        void *getTitleContextData() const { return mTitleContextData; }

        /*! ************************************************************************************************/
        /*! \brief Returns current Game Mod Register
        ***************************************************************************************************/
        GameModRegister getGameModRegister() const { return mGameModRegister; }

        /*! ************************************************************************************************/
        /*! \brief Set the private data pointer the Title would like to associate with this object instance.

            Typically used to bind a 'game' class instance from the title's namespace to the blaze game instance.

            \param[in] titleContextData a pointer to any title specific data
        ***************************************************************************************************/
        void setTitleContextData(void *titleContextData) { mTitleContextData = titleContextData; }

    /*! **********************************************************************************************************/
    /*! \name Advancing the Game's GameState

        A game's state obeys a state machine, and certain game actions are only valid in some gameStates.  For example,
        you cannot join a game that's still in the INITIALIZING state.
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a game state changes success or failure.
                NOTE: the game's state hasn't been updated at the time of this dispatch, on success the game
                will dispatch one of the GameListener game state notifications (ex: GameListener::onPreGame,
                GameListener::onGameReset, etc).


            \param[in] BlazeError the error code for the GameState change attempt
            \param[in] Game* the game that attempted to update its state
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game*, JobId> ChangeGameStateJobCb;
        typedef Functor2<BlazeError, Game*> ChangeGameStateCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief Transition from INITIALIZING to PRE_GAME.  Games must call this before other players can join the game.
                NOTE: success triggers an async GameListener::onPreGame dispatch.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId initGameComplete(const ChangeGameStateJobCb &callbackFunctor);
        JobId initGameComplete(const ChangeGameStateCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Transition from PRE_GAME to IN_GAME.
                NOTE: success triggers an async GameListener::onGameStarted dispatch.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId startGame(const ChangeGameStateJobCb &callbackFunctor);
        JobId startGame(const ChangeGameStateCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Transition from IN_GAME to POST_GAME.
                NOTE: success triggers an async GameListener::onGameEnded dispatch.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId endGame(const ChangeGameStateJobCb &callbackFunctor);
        JobId endGame(const ChangeGameStateCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Transition from POST_GAME back to PRE_GAME.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId replayGame(const ChangeGameStateJobCb &callbackFunctor);
        JobId replayGame(const ChangeGameStateCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief Transition a dedicated server back to the RESETABLE state.  The server's current state 
                must be <= PRE_GAME, or POST_GAME.

            We don't allow a game to transition from IN_GAME to RESETABLE because this would skip the POST_GAME
            state (and prevent you from sending up a game report).

            \param[in] callbackFunctor a call back functor dispatched indicating success/failure
        ***************************************************************************************************/
        JobId returnDedicatedServerToPool(const ChangeGameStateJobCb &callbackFunctor);
        JobId returnDedicatedServerToPool(const ChangeGameStateCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief Transition a dedicated server from RESETABLE to INITIALIZING.  Only valid for dedicated
                servers.

            This provides a way for the dedicated server admin user to reset the server manually
            (instead of waiting for the blazeServer to send down a reset notification).
            Triggers GameListener::onGameReset.

            \param[in] callbackFunctor a call back functor dispatched indicating success/failure
        ***************************************************************************************************/
        JobId resetDedicatedServer(const ChangeGameStateJobCb &callbackFunctor);
        JobId resetDedicatedServer(const ChangeGameStateCb &callbackFunctor);

    typedef vector<Player *> PlayerVector;

    /*! **********************************************************************************************************/
    /*! \name Player Queue

        Games can set a player queue size upon creation (see CreateGameParameters::mQueueCapacity).  If enabled,
        players trying to join a full game will join the game's queue instead.  As active players leave the game, 
        the queued players will join in order (first in, first out).
    **************************************************************************************************************/

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied user (or nullptr, if the user isn't in the queue)
            \param    user The user to get the Player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the user isn't in the queue).
        **************************************************************************************************************/
        Player *getQueuedPlayerByUser(const UserManager::User *user) const;

        /*! **********************************************************************************************************/
        /*! \brief  Returns the Player object for the supplied BlazeId (or nullptr, if the no player exists for that blazeId)
            \param    blazeId    The BlazeId (playerId) to get the player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the blazeId isn't in the queue).
        **************************************************************************************************************/
        Player *getQueuedPlayerById(BlazeId blazeId) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied player name (persona name).  Case-insensitive.
            \param    name The (case-insensitive) name of the player.
            \return    Returns the Player representing the supplied user (or nullptr, if no player with the supplied name is in the queue).
        **************************************************************************************************************/
        Player *getQueuedPlayerByName(const char8_t *name) const;

        /*! **********************************************************************************************************/
        /*! \brief Return the Player at a particular index (or nullptr, if index is out of bounds)
                getQueuedPlayerCount() is the upper bound of the index.

            Note: Player indices will change as players join and leave, so you shouldn't
            persist an index over multiple calls to the BlazeHub idle function.

            \param    index The index of the player, 0 through (getQueuedPlayerCount() - 1)
            \return    Returns the Player at index, or nullptr if index is out of bounds
        **************************************************************************************************************/
        Player *getQueuedPlayerByIndex(uint16_t index) const;

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of players waiting in the game queue.
            \return    Returns the number of players in this game's queue.
        **************************************************************************************************************/
        uint16_t getQueuedPlayerCount() const { return static_cast<uint16_t>( mQueuedPlayers.size() ); }

        /*! **********************************************************************************************************/
        /*! \brief Return a player's current position in the queue (same as the player's index; 0 is the front of the queue).
            \return    Returns the player's queue position (index), or GameManager::INVALID_QUEUE_INDEX if the player is not queued).
        **************************************************************************************************************/
        uint16_t getQueuedPlayerIndex(const Player* player) const;

        /*! **********************************************************************************************************/
        /*! \brief fill the supplied PlayerVector with the queued players in the game 
            \param playerVector - the list of queued players in the game
        **************************************************************************************************************/
        void getQueuedParticipants( PlayerVector& playerVector) const;

        /*! **********************************************************************************************************/
        /*! \brief fill the supplied PlayerVector with the queued spectators in the game
            \param spectatorVector - the list of queued spectators in the game
        **************************************************************************************************************/
        void getQueuedSpectators( PlayerVector& spectatorVector) const;

        /*! ************************************************************************************************/
        /*! \brief A callback functor for addQueuedPlayer. 
        
            \param[out] the BlazeError returned by addQueuedPlayer
            \param[out] a pointer to the game on which addQueuedPlayer was called.
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const Player*, JobId> AddQueuedPlayerJobCb;
        typedef Functor3<BlazeError, Game *, const Player*> AddQueuedPlayerCb; // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief Attempt to add a player from the queue into the game.  This is an admin only action.
        
            \param[in] player - A pointer to the player to add to the game.  The player must be already queued in the game.
            \param[in] titleCb - A callback function.
        ***************************************************************************************************/
        JobId addQueuedPlayer(const Player *player, const AddQueuedPlayerJobCb& titleCb);
        JobId addQueuedPlayer(const Player *player, const AddQueuedPlayerCb& titleCb);

        /*! ************************************************************************************************/
        /*! \brief Attempt to add a player from the queue into the game.  This is an admin only action.
        
            \param[in] player - A pointer to the player to add to the game.  The player must be already queued in the game.
            \param[in] joiningTeamIndex - supply a team index to override the queued player's index. UNSPECIFIED_TEAM_INDEX will keep the user's existing value
            \param[in] joiningRoleName - supply a RoleName to override the queued player's Role. nullptr to keep the existing queued role.
            \param[in] titleCb - A callback function.
        ***************************************************************************************************/
        JobId addQueuedPlayer(const Player *player, TeamIndex joiningTeamIndex, const RoleName *joiningRoleName, const AddQueuedPlayerJobCb& titleCb);
        JobId addQueuedPlayer(const Player *player, TeamIndex joiningTeamIndex, const RoleName *joiningRoleName, const AddQueuedPlayerCb& titleCb);

        /*! ************************************************************************************************/
        /*! \brief Attempt to demote a reserved player from the game into the queue.  This is an admin only action.
        
            \param[in] player - A pointer to the player to demote from the game.  The player must be already reserved in the game.
            \param[in] titleCb - A callback function.
        ***************************************************************************************************/
        JobId demotePlayerToQueue(const Player *player, const AddQueuedPlayerJobCb& titleCb);
        JobId demotePlayerToQueue(const Player *player, const AddQueuedPlayerCb& titleCb);

    /*! **********************************************************************************************************/
    /*! \name Player Accessors

        Note: Player pointers and ids are safe to persist over calls to BlazeHub::idle, as
        long as you invalidate them as they are destroyed (see GameListener::onPlayerRemoved)
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Returns the Player object for the game's platform host.
        ***************************************************************************************************/
        Player* getPlatformHostPlayer() const { return mPlatformHostPlayer; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the local user.  NOTE: will be nullptr for a the dedicated server host
            (they're not a player in the game).   If multiple local users are allowed, returns the primary local user, 
            who is the "owner" of the endpoint.
        **************************************************************************************************************/
        Player* getLocalPlayer() const;

        /*! ************************************************************************************************/
        /*! \brief Returns the local player associated with the given user index.  Player can be null
            if there is no user at that index, or if the local user is the dedicated server host. (they
            are not a player in the game).
        
            \param[in] userIndex - the user index of the local player
            \return the player object of the local player at the given user index.
        ***************************************************************************************************/
        Player* getLocalPlayer(uint32_t userIndex) const;


        /*! **********************************************************************************************************/
        /*! \brief Checks if the current userIndex is the one that's handling notifications currently for the game. 
             if the primary user has left the game, and another user is still in-game,  then this function 
             will use the userIndex of the first local player still in-game. 
        **************************************************************************************************************/
        bool isNotificationHandlingUserIndex(uint32_t userIndex) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied user (or nullptr, if the user isn't a game member)
            Player could be inactive (queued or reserved).
            \param    user The user to get the Player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the user isn't a game member).
        **************************************************************************************************************/
        Player* getPlayerByUser(const UserManager::User *user) const;

        /*! **********************************************************************************************************/
        /*! \brief  Returns the Player object for the supplied BlazeId (or nullptr, if the no player exists for that blazeId)
            Player could be inactive (queued or reserved).
            \param    blazeId    The BlazeId (playerId) to get the player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the blazeId isn't a game member).
        **************************************************************************************************************/
        Player* getPlayerById(BlazeId blazeId) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied player name (persona name).  Case-insensitive.
            Player could be inactive (queued or reserved).
            \param    name The (case-insensitive) name of the player.
            \return    Returns the Player representing the supplied user (or nullptr, if no player has the supplied name).
        **************************************************************************************************************/
        Player* getPlayerByName(const char8_t *name) const;

        /*! ************************************************************************************************/
        /*! \brief Returns the first Player object for the supplied connection group id.
            \param[in] connectionGroupId - the connection group to look for
            \return the Player representing the connection group id ( or nullptr, if no player is found)
        ***************************************************************************************************/
        Player* getActivePlayerByConnectionGroupId(ConnectionGroupId connectionGroupId) const;

        /*! ************************************************************************************************/
        /*! \brief Populates a list of all players in a game session with the same connection group ID
            \param[in] connectionGroupId - the connection group to look for
            \param[in] playerVector - the list of players with the matching connection group Id
        ***************************************************************************************************/
        void getActivePlayerByConnectionGroupId(ConnectionGroupId connectionGroupId, PlayerVector& playerVector) const;

        /*! **********************************************************************************************************/
        /*! \brief fill the supplied PlayerVector with the active (not reserved, not queued) participants in the game 
            \param playerVector - the list of active players in the game
        **************************************************************************************************************/
        void getActiveParticipants( PlayerVector& playerVector) const;

        /*! **********************************************************************************************************/
        /*! \brief fill the supplied PlayerVector with the active (not reserved, not queued) spectators in the game
            \param spectatorVector - the list of active spectators in the game
        **************************************************************************************************************/
        void getActiveSpectators( PlayerVector& spectatorVector) const;

        /*! **********************************************************************************************************/
        /*! \brief Return the Player at a particular index (or nullptr, if index is out of bounds)
                getPlayerCount is the upper bound of the index. Player could be inactive (queued or reserved).

               Note: Player indices will change as players join and leave, so you shouldn't
               persist an index over multiple calls to the BlazeHub idle function.

            \param    index The index of the player, 0 through (getPlayerCount - 1)
            \return    Returns the Player at index, or nullptr if index is out of bounds
        **************************************************************************************************************/
        Player* getPlayerByIndex(uint16_t index) const;

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of users in the game (including spectators, player reservations and queued players).
                This method should be used when identifying the max index to be used when iterating over all users in a game by index.
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getPlayerCount() const 
            { return static_cast<uint16_t>( mRosterPlayers.size() + mQueuedPlayers.size() ); }

        // 'Using' is needed because the method name is being hidden due to the multiple overloads of the method name
        using GameBase::getPlayerCount;

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of users in the game (all slot types, players and spectators, including reservations).
                The return value of getRosterCount(), minus the return value of getSpectatorCount() is the number of non-spectator players in the game.
            \return    Returns the number of users in this game.
        **************************************************************************************************************/
        virtual uint16_t getRosterCount() const { return static_cast<uint16_t>( mRosterPlayers.size() ); }

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of users in the game (including player reservations and queued players).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getParticipantCount() const { return static_cast<uint16_t>( mPlayerSlotCounts[SLOT_PUBLIC_PARTICIPANT] + mPlayerSlotCounts[SLOT_PRIVATE_PARTICIPANT] ); }

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of users in the game (including player reservations and queued players).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getSpectatorCount() const { return static_cast<uint16_t>( mPlayerSlotCounts[SLOT_PUBLIC_SPECTATOR] + mPlayerSlotCounts[SLOT_PRIVATE_SPECTATOR] ); }

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of users in the queue (all slot types, players and spectators).
            \return    Returns the number of users in this game's queue.
        **************************************************************************************************************/
        virtual uint16_t getQueueCount() const { return static_cast<uint16_t>( mQueuedPlayers.size() ); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied user, if the player is active in the game (not queued)
            \param    user The user to get the Player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the user isn't an active game member).
        **************************************************************************************************************/
        Player* getActivePlayerByUser(const UserManager::User *user) const;

        /*! **********************************************************************************************************/
        /*! \brief  Returns the Player object for the supplied BlazeId, if the player is active in the game (not queued)
            \param    blazeId    The BlazeId (playerId) to get the player for.
            \return    Returns the Player representing the supplied user (or nullptr, if the blazeId isn't an active game member).
        **************************************************************************************************************/
        Player *getActivePlayerById(BlazeId blazeId) const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the supplied player name (persona name), if the player is active in the game (not queued).
                    Case-insensitive.
            \param    name The (case-insensitive) name of the player.
            \return    Returns the Player representing the supplied user (or nullptr, if no active player has the supplied name).
        **************************************************************************************************************/
        Player *getActivePlayerByName(const char8_t *name) const;

        /*! **********************************************************************************************************/
        /*! \brief Return the active Player at a particular index (or nullptr, if index is out of bounds)
            getActivePlayerCount is the upper bound of the index.

            Note: Player indices will change as players join and leave, so you shouldn't
            persist an index over multiple calls to the BlazeHub idle function.

            \param    index The index of the active player, 0 through (getActivePlayerCount - 1)
            \return    Returns the Player at index, or nullptr if index is out of bounds
        **************************************************************************************************************/
        Player *getActivePlayerByIndex(uint16_t index) const;

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of active users in the game (players who are not queued/reserved).
            \return    Returns the number of active players in this game.
        **************************************************************************************************************/
        uint16_t getActivePlayerCount() const { return static_cast<uint16_t>( mActivePlayers.size() ); }

    /*! **********************************************************************************************************/
    /*! \name Leaving Games
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a leaveGame's success or failure.
            NOTE: the player hasn't left the game at the time of this dispatch; on success,
            the game will dispatch GameListener::onPlayerRemoved (with an appropriate removal reason).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> LeaveGameJobCb;
        typedef Functor2<BlazeError, Game *> LeaveGameCb; // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief The local player leaves the game itself (or the game's queue).

            If the callback returns successfully, the blazeServer will remove you from the game and you'll
            receive a GameListener::onPlayerRemoved dispatch.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \param[in] userGroup - The user group who would like to leave the game together,
                                    default is nullptr, means it's a single player leaves operation
            \param[in] makeReservation - make disconnect reservation
        ***************************************************************************************************/
        JobId leaveGame(const LeaveGameJobCb &callbackFunctor, const UserGroup* userGroup = nullptr, bool makeReservation = false);
        JobId leaveGame(const LeaveGameCb &callbackFunctor, const UserGroup* userGroup = nullptr, bool makeReservation = false);

        /*! ************************************************************************************************/
        /*! \brief The local player specified by userIndex leaves the game.
        
            \param[in] userIndex - the index of the local player to leave the game. Cannot be
                the primary user index, that player should use leaveGame.
            \param[in] callbackFunctor - the functor dispatched indicating success/failure.
            \param[in] makeReservation - make disconnect reservation
        ***************************************************************************************************/
        JobId leaveGameLocalUser(uint32_t userIndex, const LeaveGameJobCb &callbackFunctor, bool makeReservation = false);
        JobId leaveGameLocalUser(uint32_t userIndex, const LeaveGameCb &callbackFunctor, bool makeReservation = false);


    /*! **********************************************************************************************************/
    /*! \name Kicking/Banning Players
    **************************************************************************************************************/

        enum KickReason
        {
            KICK_NORMAL,
            KICK_UNRESPONSIVE_PEER,
            KICK_POOR_CONNECTION
        };

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a kickPlayer's success or failure.
            NOTE: the player hasn't left the game at the time of this dispatch; on success,
            the game will dispatch GameListener::onPlayerRemoved (with an appropriate removal reason).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
            \param[in] Player* The player who you're trying to kick (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const Player *, JobId> KickPlayerJobCb;
        typedef Functor3<BlazeError, Game *, const Player *> KickPlayerCb; // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief Attempt to kick a player out of the game, optionally banning them from rejoining.  Requires admin
                rights for this game.  Note: you cannot kick yourself out of a game (use Game::leaveGame instead)

            If the callback returns successfully, the blazeServer will shortly remove the player and you'll
            receive a GameListener::onPlayerRemoved dispatch.

            \param[in] player The player to kick.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \param[in] banPlayer if true, the player is also banned from this game session, preventing reentry (see banUser for details).
            \param[in] titleContext a title-defined reason why the player was kicked. 
                            This value is broadcast to all game members via GameListener::onPlayerRemoved.
            \param[in] titleContextStr a title-defined reason why the player was kicked.  This is carried through to the server
                            for reporting and debugging connection issues 
        **************************************************************************************************************/
        JobId kickPlayer(const Player *player, const KickPlayerJobCb &callbackFunctor, bool banPlayer = false,
            PlayerRemovedTitleContext titleContext = 0, const char8_t* titleContextStr = nullptr, KickReason kickReason = KICK_NORMAL);
        JobId kickPlayer(const Player *player, const KickPlayerCb &callbackFunctor, bool banPlayer = false,
            PlayerRemovedTitleContext titleContext = 0, const char8_t* titleContextStr = nullptr, KickReason kickReason = KICK_NORMAL);

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a banUser's success or failure.
            NOTE: the user has been banned from the game at the time of this dispatch (but not kicked from the game).
            
            If the banned user is also being kicked from the game, the game will dispatch 
            GameListener::onPlayerRemoved (with an appropriate removal reason) shortly.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
            \param[in] BlazeId The BlazeId (aka PlayerId) of the user you're trying to ban (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, BlazeId, JobId> BanPlayerJobCb;
        typedef Functor3<BlazeError, Game *, BlazeId> BanPlayerCb; // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a banUser's success or failure.
            NOTE: the user has been banned from the game at the time of this dispatch (but not kicked from the game).
            
            If the banned user is also being kicked from the game, the game will dispatch 
            GameListener::onPlayerRemoved (with an appropriate removal reason) shortly.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
            \param[in] BlazeIdList The BlazeIdList (aka PlayerIdList) of the users you're trying to ban (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, PlayerIdList *, JobId> BanPlayerListJobCb;
        typedef Functor3<BlazeError, Game *, PlayerIdList *> BanPlayerListCb; // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief Attempt to ban a user's account from this game session.  If the user is also player in this game session,
                they are kicked from the game.  Requires admin rights for the game.
                
                NOTE: You cannot ban yourself from a game. 

                If blazeserver config "clearBannedListWithReset" to true, bans last for the duration of the game (until the game is reset 
                or destroyed). All bans are done at the account level, so banning a persona (blazeId) bans all personas owned by that account.

                Each game maintains a FIFO list of banned players; the list's capacity is configured by the blazeServer (see
                banListMax in the gameManager's server config file: /etc/components/gamemanager/gamemanager.cfg).
                If you ban a new player when the game's ban list is full, the blazeServer will remove the first banned
                player to make room for the newly banned player.

            \param[in] blazeId The user to ban from the game. NOTE: the player doesn't have to be in the game currently.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \param[in] titleContext only applies if the banned user is currently a game member.
                        a title-defined reason why the user was kicked from the game.
                        This value is broadcast to all game members via GameListener::onPlayerRemoved.
        **************************************************************************************************************/
        JobId banUser(const BlazeId blazeId, const BanPlayerJobCb &callbackFunctor, PlayerRemovedTitleContext titleContext = 0);
        JobId banUser(const BlazeId blazeId, const BanPlayerCb &callbackFunctor, PlayerRemovedTitleContext titleContext = 0);

        /*! **********************************************************************************************************/
        /*! \brief Attempt to ban a lot of user's account from this game session.  If the users are also players in this game session,
                they are kicked from the game.  Requires admin rights for the game.                

                NOTE: You cannot ban yourself from a game. Also, if one player in the list fails, the server errors out 
                at that point, the rest are not processed, and those already processed remain banned.

                If blazeserver config "clearBannedListWithReset" to true, bans last for the duration of the game (until the game is reset 
                or destroyed). All bans are done at the account level, so banning a persona (blazeId) bans all personas owned by that account.

                Each game maintains a FIFO list of banned players; the list's capacity is configured by the blazeServer (see
                banListMax in the gameManager's server config file: /etc/components/gamemanager/gamemanager.cfg).
                If you ban a new player when the game's ban list is full, the blazeServer will remove the first banned
                player to make room for the newly banned player.But the count that you ban users simultaneoulsy can't beyond the 
                max count - that config at /etc/components/gamemanager/gamemanager.cfg

            \param[in] playerIds the users to ban from the game. NOTE: the players don't have to be in the game currently.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \param[in] titleContext only applies if the banned user is currently a game member.
                        a title-defined reason why the user was kicked from the game.
                        This value is broadcast to all game members via GameListener::onPlayerRemoved.
        **************************************************************************************************************/
        JobId banUser(PlayerIdList *playerIds, const BanPlayerListJobCb &callbackFunctor, PlayerRemovedTitleContext titleContext = 0);
        JobId banUser(PlayerIdList *playerIds, const BanPlayerListCb &callbackFunctor, PlayerRemovedTitleContext titleContext = 0);

    /*! **********************************************************************************************************/
    /*! \name Unbanning player
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a unbanUser's success or failure.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
            \param[in] BlazeId The BlazeId (aka PlayerId) of the users you're trying to unban (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, BlazeId, JobId> UnbanPlayerJobCb;
        typedef Functor3<BlazeError, Game *, BlazeId> UnbanPlayerCb;  // DEPRECATED 

        /*! **********************************************************************************************************/
        /*! \brief remove player from the game banned list.

            \param[in] blazeId The user to remove from the game banned list.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId removePlayerFromBannedList(const BlazeId blazeId, const UnbanPlayerJobCb &callbackFunctor);
        JobId removePlayerFromBannedList(const BlazeId blazeId, const UnbanPlayerCb &callbackFunctor);

    /*! **********************************************************************************************************/
    /*! \name clearing banned list
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a clearing banned list success or failure.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ClearBannedListJobCb;
        typedef Functor2<BlazeError, Game *> ClearBannedListCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief clear the game banned list.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId clearBannedList(const ClearBannedListJobCb &callbackFunctor);
        JobId clearBannedList(const ClearBannedListCb &callbackFunctor);

    /*! **********************************************************************************************************/
    /*! \name getting banned list
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a getting banned list success or failure.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const BannedListResponse*, JobId> GetBannedListJobCb;
        typedef Functor3<const BannedListResponse*, BlazeError, Game *> GetBannedListCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief get the game banned list.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId getBannedList(const GetBannedListJobCb &callbackFunctor);
        JobId getBannedList(const GetBannedListCb &callbackFunctor);

    /*! **********************************************************************************************************/
    /*! \name renaming game name
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a renaming game name success or failure.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> UpdateGameNameJobCb;
        typedef Functor2<BlazeError, Game *> UpdateGameNameCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief rename the game name.

            \param[in] newName the new game name to rename from the game
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId updateGameName(const GameName &newName, const UpdateGameNameJobCb &callbackFunctor);
        JobId updateGameName(const GameName &newName, const UpdateGameNameCb &callbackFunctor);


    /*! **********************************************************************************************************/
    /*! \name Destroying Games
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a destroyGame's success or failure.
            NOTE: the game hasn't been destroyed at the time of this dispatch; on success,
            the game will dispatch GameManagerAPIListener::onGameDestructing.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> DestroyGameJobCb;
        typedef Functor2<BlazeError, Game *> DestroyGameCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief Destroys the game with a specific GameDestructionReason.  Requires admin rights.

            If the callback returns successfully, the blazeServer will shortly destroy the game and you'll
            receive a GameManagerAPIListener::onGameDestructing dispatch.

            Game destruction is final; it will not trigger host migration.

            \param[in] gameDestructionReason the 'reason' for destroying the game (can be customized per game)
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId destroyGame(GameDestructionReason gameDestructionReason, const DestroyGameJobCb &callbackFunctor);
        JobId destroyGame(GameDestructionReason gameDestructionReason, const DestroyGameCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief
        
            \param[in] BlazeError the error code for the ejction attempt
            \param[in] Game a pointer to the game the host was ejected from.
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game*, JobId> EjectHostJobCb;
        typedef Functor2<BlazeError, Game*> EjectHostCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief Removes the dedicated host as the host of this dedicated server game.
        ***************************************************************************************************/
        JobId ejectHost(const EjectHostJobCb &callbackFunctor, bool replaceHost = false);
        JobId ejectHost(const EjectHostCb &callbackFunctor, bool replaceHost = false);

    /*! **********************************************************************************************************/
    /*! \name Game Capacity

        Game admins can change a game's player capacity after creating the game.  The game's capacity can be shrunken
        down to the game's current playerCount, or enlarged until the capacity is equal to the game's maxPlayerCapacity.

        \sa
        \li members inherited from GameBase (documented below, in section "Game Capacity (GameBase)")
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a game capacity update's success or failure.
                NOTE: the game's capacity hasn't been updated at the time of this dispatch; on success,
                the game will dispatch GameListener::onPlayerCapacityUpdated.
        
            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
            \param[in] SwapPlayersErrorInfo * pointer to an error object with more details on the failure 
                (nullptr on ERR_OK, or if the error isn't related to the game roster)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const SwapPlayersErrorInfo *, JobId> ChangePlayerCapacityJobCb;
        typedef Functor3<BlazeError, Game *, const SwapPlayersErrorInfo *> ChangePlayerCapacityCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief set the game's player capacities for each slot type (public and private).  Requires game admin rights.

            You cannot shrink a game's capacity for a slotType below the game's current playerCount for that slot type.
            You cannot enlarge a game's total capacity (sum of the capacities for each slot type) above the game's
            maxPlayerCapacity.

            \sa
            \li getPlayerCount()
            \li GameBase::getMaxPlayerCapacity()

        \param[in] newSlotCapacities the new capacity array for the game.
        \param[in] titleCb the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const ChangePlayerCapacityJobCb &titleCb);
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const ChangePlayerCapacityCb &titleCb);

        /*! ************************************************************************************************/
        /*! \brief set the game's player capacities for each slot type (public and private).  Requires game admin rights.
        You cannot shrink a game's capacity for a slotType below the game's current playerCount for that slot type.
        You cannot enlarge a game's total capacity (sum of the capacities for each slot type) above the game's
        maxPlayerCapacity.
        You cannot change the game's total capacity to a value not evenly divisible by the number of teams in the session.
        Leaving newTeamsComposition empty will leave the FactionId and count of teams unchanged.
        Players who must be on a specific team after the re-size should be put in the corresponding PlayerIdList of the supplied newTeamsComposition. 
        If players are moved to another team, overall game capacity is reduced, or the number of teams in the game change, Blaze will make a best-effort to re-balance players between teams by player count.
        If a user specified in the request is no longer in the game session, the request is allowed to proceed as if the user was not mentioned.
 
 
        \param[in] newSlotCapacities the new capacity array for the game.
        \param[in] newTeamsComposition the the new faction IDs to use, and the players to assign to them. Omitted players are balanced between the teams as needed.
        \param[in] titleCb the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const ChangePlayerCapacityJobCb &titleCb);
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const ChangePlayerCapacityCb &titleCb);

        /*! ************************************************************************************************/
        /*! \brief set the game's player capacities for each slot type (public and private).  Requires game admin rights.
        You cannot shrink a game's capacity for a slotType below the game's current playerCount for that slot type.
        You cannot enlarge a game's total capacity (sum of the capacities for each slot type) above the game's
        maxPlayerCapacity.
        You cannot change the game's total capacity to a value not evenly divisible by the number of teams in the session.
        Leaving newTeamsComposition empty will leave the FactionId and count of teams unchanged.
        Players who must be on a specific team after the re-size should be put in the corresponding PlayerIdList of the supplied newTeamsComposition. 
        If players are moved to another team, overall game capacity is reduced, or the number of teams in the game change, Blaze will make a best-effort to re-balance players between teams by player count.
        If a user specified in the request is no longer in the game session, the request is allowed to proceed as if the user was not mentioned.
 
 
        \param[in] newSlotCapacities the new capacity array for the game.
        \param[in] newTeamsComposition the the new faction IDs to use, and the players to assign to them. Omitted players are balanced between the teams as needed.
        \param[in] roleInformation the role information for the game, including role capacities, role criteria, and multirole criteria. 
        \param[in] titleCb the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation &roleInformation, const ChangePlayerCapacityJobCb &titleCb);
        JobId setPlayerCapacity(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation &roleInformation, const ChangePlayerCapacityCb &titleCb);


    /*! **********************************************************************************************************/
    /*! \name Host Migration
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating if host migration started successfully. NOTE: on ERR_OK,
                wait for host migration to begin (the game will dispatch GameListener::onHostMigrationStarted)
        
            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> MigrateHostJobCb;
        typedef Functor2<BlazeError, Game *> MigrateHostCb;  // DEPRECATED
        
        /*! **********************************************************************************************************/
        /*! \brief Attempt to migrate the game's network topology host to a specific player.  Note: the blazeServer takes
                the player as a suggestion - they might not be a suitable host.

            Note: the blazeServer will attempt to honor your new host selection, but might choose another player
            if the selected player isn't able to host the game network mesh (if they're behind a strict firewall,
            for example).

            \param player The player suggested to be the new topology host.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId migrateHostToPlayer(const Player *player, const MigrateHostJobCb &callbackFunctor);
        JobId migrateHostToPlayer(const Player *player, const MigrateHostCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Determines whether or not topology host migration has been enabled for this game.

            \sa
            \li setGameSettings()
            \li CreateGameParameters::mGameSettings
            \li ResetGameParameters::mGameSettings

            \return    Returns True if topology host migration has been enabled for this game.
        **************************************************************************************************************/
        bool isHostMigrationEnabled() const { return mGameSettings.getHostMigratable(); }

        /*! **********************************************************************************************************/
        /*! \brief Determines whether or not the game is in the process of host migration (topology, platform, or both forms of migration).
            \return    Returns true if the game is in the process of host migration
        **************************************************************************************************************/
        bool isMigrating() const { return mIsMigrating; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the last host migration type, which may be TOPOLOGY, PLATFORM, or both.
            \return    Returns the last host migration type (not cleared when migration finishes)
        **************************************************************************************************************/
        HostMigrationType getHostMigrationType() const { return mHostMigrationType; }

        /*! **********************************************************************************************************/
        /*! \brief Determines whether or not the game is in the process of platform host migration.
            \return    Returns true if the game is in the process of platform host migration
        **************************************************************************************************************/
        bool isMigratingPlatformHost() const { return mIsMigratingPlatformHost; }

        /*! **********************************************************************************************************/
        /*! \brief Determines whether or not the game is in the process of topology host migration.
            \return    Returns true if the game is in the process of topology host migration
        **************************************************************************************************************/
        bool isMigratingTopologyHost() const { return mIsMigratingTopologyHost; }

    /*! **********************************************************************************************************/
    /*! \name Game Settings (joinBy flags (join-ability) and the game's ranked setting)
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Returns true if this game allows invited players to skip the game's entry criteria.
            \return Returns true if this game allows players to ignore the entry criteria with an invite.
        ***************************************************************************************************/
        bool ignoreEntryCriteriaWithInvite() const { return mGameSettings.getIgnoreEntryCriteriaWithInvite(); }

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's setGameModRegister attempt.  Success indicates that
                the game will soon dispatch GameListener::onGameModRegisterUpdated (with the updated mod register).

            Note: the game's mod register has NOT been updated at the time this callback is dispatched.  If the
                error code is ERR_OK, the game will dispatch GameListener::onGameModRegisterUpdated (once the mod register
                has been updated).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ChangeGameModRegisterJobCb;
        typedef Functor2<BlazeError, Game *> ChangeGameModRegisterCb;  // DEPRECATED

        JobId setGameModRegister(GameModRegister newModRegister, const ChangeGameModRegisterJobCb &callbackFunctor);
        JobId setGameModRegister(GameModRegister newModRegister, const ChangeGameModRegisterCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's setGameEntryCriteria attempt.  Success indicates that
                the game will soon dispatch GameListener::onGameEntryCriteriaUpdated (with the updated mod register).

            Note: the game's entry criteria has NOT been updated at the time this callback is dispatched.  If the
                error code is ERR_OK, the game will dispatch GameListener::onGameEntryCriteriaUpdated (once the entry criteria
                has been updated).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ChangeGameEntryCriteriaJobCb;
        typedef Functor2<BlazeError, Game *> ChangeGameEntryCriteriaCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief update the game entry criteria and invite entry setting in a single Blaze RPC. (Admin-only)
        
            Integrators note: the ignoreEntryCriteriaWithInvite parameter available in the setGameEntryCriteria() method in older versions
            of Blaze has now been removed. Callers should now set/clear the GameSettings.mIgnoreEntryCriteriaWithInvite via setGameSettings() instead.

            \param[in] entryCriteriaMap a map containing the criteria used to restrict entry into the game.
            \param[in] roleSpecificEntryCriteria - a map of role names to role-specific entry criteria maps, omit a role to clear its existing entry criteria
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId setGameEntryCriteria(const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap& roleSpecificEntryCriteria, const ChangeGameEntryCriteriaJobCb &callbackFunctor);
        JobId setGameEntryCriteria(const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap& roleSpecificEntryCriteria, const ChangeGameEntryCriteriaCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's setGameSettings attempt.  Success indicates that
                the game will soon dispatch GameListener::onGameSettingUpdated (with the updated settings).

            Note: the game's settings have NOT been updated at the time this callback is dispatched.  If the
                error code is ERR_OK, the game will dispatch GameListener::onGameSettingUpdated (once the settings
                have been updated).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ChangeGameSettingsJobCb;
        typedef Functor2<BlazeError, Game *> ChangeGameSettingsCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief update all game settings in a single Blaze RPC.

            typical usage:
              Blaze::GameManager::GameSettings newSettings = game->getGameSettings();
              newSettings.setRanked();
              newSettings.setOpenToBrowsing();
              game->setGameSettings(newSettings, someCallback);

            \note: you cannot change the ranked setting of an XBL game after game creation.

            Integrators note: the ignoreEntryCriteriaWithInvite parameter available in the setGameEntryCriteria() method in older versions
            of Blaze has now been removed. Callers should now set/clear the GameSettings.mIgnoreEntryCriteriaWithInvite via setGameSettings() instead.

            // GM_DOC_TODO: verify that if one flag is rejected, none are applied.  For example, if I try to change
            // the game's ranked flag in the middle of the game (not allowed), but also close the game to invites.

            \param[in] newSettings a tdf bitfield object specifying the game's join entry flags and ranked setting.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/

        JobId setGameSettings(const GameSettings &newSettings, const ChangeGameSettingsJobCb &callbackFunctor);
        JobId setGameSettings(const GameSettings &newSettings, const ChangeGameSettingsCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief update a masked set the game settings in a single Blaze RPC.
                   This can be safer than making multiple quick calls to the non-masked setGameSettings(),
                     or when different connections are updating the different settings at the same time. 

            usage:
                Blaze::GameManager::GameSettings newSetting, settingsMask;
                newSettings.setRanked();
                settingsMask.setRanked();
                newSettings.clearOpenToBrowsing();   // Not required, since the default is cleared.
                settingsMask.setOpenToBrowsing();

                game->setGameSettings(newSettings, settingsMask, someCallback);

            \param[in] newSettings a tdf bitfield specifying various game settings.
            \param[in] newSettings a tdf bitfield specifying the mask used to change the settings.  Only the masked settings will be changed.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId setGameSettings(const GameSettings &newSettings, const GameSettings &settingsMask, const ChangeGameSettingsJobCb &callbackFunctor);
        JobId setGameSettings(const GameSettings &newSettings, const GameSettings &settingsMask, const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief close game to all join methods (in a single Blaze RPC); other game settings flags are unchanged.

            This is equivalent to getting the game's current gameSettings, modifying the joinGameFlags,
            then calling setGameSettings with the modified settings.

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId closeGameToAllJoins(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            // clear the 'open to join by X' bits
            GameSettings newSettings, settingsMask;
            settingsMask.setOpenToBrowsing();
            settingsMask.setOpenToInvites();
            settingsMask.setOpenToJoinByPlayer();
            settingsMask.setOpenToMatchmaking();
            settingsMask.setEnablePersistedGameId();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId closeGameToAllJoins(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Determines if game is closed to all join methods.
            \return    Returns true if all join methods are disabled.
        **************************************************************************************************************/
        bool isClosedToAllJoins() const
        {
            return ( (!mGameSettings.getOpenToBrowsing()) && (!mGameSettings.getOpenToInvites()) && 
                (!mGameSettings.getOpenToJoinByPlayer()) && (!mGameSettings.getOpenToMatchmaking()) && 
                (!mGameSettings.getEnablePersistedGameId()) );
        }

        /*! **********************************************************************************************************/
        /*! \brief open game to all join methods (in a single Blaze RPC); other game settings flags are unchanged.

            This is equivalent to getting the game's current gameSettings, modifying the joinGameFlags,
            then calling setGameSettings with the modified settings.

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId openGameToAllJoins(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            // set the 'open to join by X' bits
            GameSettings newSettings, settingsMask;
            newSettings.setOpenToBrowsing();
            newSettings.setOpenToInvites();
            newSettings.setOpenToJoinByPlayer();
            newSettings.setOpenToMatchmaking();
            newSettings.setEnablePersistedGameId();

            settingsMask.setOpenToBrowsing();
            settingsMask.setOpenToInvites();
            settingsMask.setOpenToJoinByPlayer();
            settingsMask.setOpenToMatchmaking();
            settingsMask.setEnablePersistedGameId();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId openGameToAllJoins(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Determines if game is open to all join methods.
            \return    Returns true if all join methods are enabled.
        **************************************************************************************************************/
        bool isOpenToAllJoins() const
        {
            return ( (mGameSettings.getOpenToBrowsing()) && (mGameSettings.getOpenToInvites()) && 
                (mGameSettings.getOpenToJoinByPlayer()) && (mGameSettings.getOpenToMatchmaking()) && 
                (mGameSettings.getEnablePersistedGameId() || !hasPersistedGameId()) );
        }

        /*! **********************************************************************************************************/
        /*! \brief Enables joining by game browser (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setOpenToBrowsing(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.setOpenToBrowsing();
            settingsMask.setOpenToBrowsing();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId setOpenToBrowsing(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Disables joining by game browser (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId clearOpenToBrowsing(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.clearOpenToBrowsing();
            settingsMask.setOpenToBrowsing();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId clearOpenToBrowsing(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Determines if game is open to joins by game browser.
            \return    Returns true if game browser joins are enabled.
        **************************************************************************************************************/
        bool getOpenToBrowsing() const { return mGameSettings.getOpenToBrowsing(); }

        /*! **********************************************************************************************************/
        /*! \brief Enables joining by match making (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setOpenToMatchmaking(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.setOpenToMatchmaking();
            settingsMask.setOpenToMatchmaking();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId setOpenToMatchmaking(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Disables joining by match making (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId clearOpenToMatchmaking(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.clearOpenToMatchmaking();
            settingsMask.setOpenToMatchmaking();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId clearOpenToMatchmaking(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Determines if game is open to joins by matchmaking.
            \return    Returns true if matchmaking joins are enabled.
        **************************************************************************************************************/
        bool getOpenToMatchmaking() const { return mGameSettings.getOpenToMatchmaking(); }


        /*! **********************************************************************************************************/
        /*! \brief Enables joining by invitation (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setOpenToInvites(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.setOpenToInvites();
            settingsMask.setOpenToInvites();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId setOpenToInvites(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Disables joining by invitation (see setGameSettings to update multiple settings in a single rpc)

           \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
       **************************************************************************************************************/
        JobId clearOpenToInvites(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.clearOpenToInvites();
            settingsMask.setOpenToInvites();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId clearOpenToInvites(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Determines if game is open to joins by invitation.
            \return    Returns true if invitation joins are enabled.
        **************************************************************************************************************/
        bool getOpenToInvites() const { return mGameSettings.getOpenToInvites(); }

        /*! **********************************************************************************************************/
        /*! \brief Enables joining by player ('following' a player by finding/joining their current game).
                 (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setOpenToJoinByPlayer(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.setOpenToJoinByPlayer();
            settingsMask.setOpenToJoinByPlayer();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId setOpenToJoinByPlayer(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Disables joining by player ('following' a player by finding/joining their current game).
                 (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId clearOpenToJoinByPlayer(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.clearOpenToJoinByPlayer();
            settingsMask.setOpenToJoinByPlayer();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId clearOpenToJoinByPlayer(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Returns true if joining by player is enabled ('following' a player by finding/joining their current game).
            \return    Returns true if 'follow player' joins are enabled.
        **************************************************************************************************************/
        bool getOpenToJoinByPlayer() const { return mGameSettings.getOpenToJoinByPlayer(); }


        /*! **********************************************************************************************************/
        /*! \brief Marks this game as ranked.  Note: you cannot change the rank status of an XBL game after creation.
                (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setRanked(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.setRanked();
            settingsMask.setRanked();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId setRanked(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Marks this game as unranked.  Note: you cannot change the rank status of an XBL game after creation.
                (see setGameSettings to update multiple settings in a single rpc)

            \note: Subsequent calls to change settings before receiving the callback will not update the settings correctly.
            The client updates settings on the callback, so the subsequent calls would not have the changes from the first.

            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId clearRanked(const ChangeGameSettingsJobCb &callbackFunctor)
        {
            GameSettings newSettings, settingsMask;
            newSettings.clearRanked();
            settingsMask.setRanked();
            return setGameSettings(newSettings, settingsMask, callbackFunctor);
        }
        JobId clearRanked(const ChangeGameSettingsCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Returns true if this is a ranked game.
            \return return true if this is a ranked game.
        **************************************************************************************************************/
        bool getRanked() const { return mGameSettings.getRanked(); }

        /*! **********************************************************************************************************/
        /*! \brief (DEPRECATED) Returns true if this is a ranked game.
            \deprecated  Use getRanked instead; slated for removal in the next major BlazeSDK revision (~3.0).
            \return return true if this is a ranked game.
        **************************************************************************************************************/
        bool isRanked() const { return getRanked(); }

    /*! **********************************************************************************************************/
    /*! \name Change Team Id

        Game admins can change a team's id after starting the game. This action is invalid for a game without teams,
        a team cannot be switched to INVALID_TEAM_ID, and teams with players in them cannot be switched to ANY_TEAM_ID
        All players in a team that is switched have their individual TeamIds switched.

    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a TeamId change's success or failure.
                NOTE: the game's TeamId hasn't been updated at the time of this dispatch; on success,
                the game will dispatch GameListener::onNotifyGameTeamIdChanged.
        
            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ChangeTeamIdJobCb;
        typedef Functor2<BlazeError, Game *> ChangeTeamIdCb;

        //////////////////////////////////////////////////////////////////////////
        // Disabling changeTeamId as it is not working properly yet http://online.ea.com/jira/browse/GOS-5309
        //////////////////////////////////////////////////////////////////////////
                
        /*! ************************************************************************************************/
        /*! \brief change the id of one team in a game for another.

        You cannot add or remove teams from a game.
        You cannot change a TeamId to INVALID_TEAM_ID or to that of a team already present in the game.
        You cannot change a team with non-zero size to ANY_TEAM_ID.
        If there are multiple teams in a game with ANY_TEAM_ID, this method cannot guarantee which one will be switched.

        \param[in] teamIndex the index of the team to change the id of
        \param[in] newTeamId the team id to switch it to
        \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId changeTeamIdAtIndex(const TeamIndex &teamIndex, const TeamId &newTeamId, const ChangeTeamIdJobCb &callbackFunctor);
        JobId changeTeamIdAtIndex(const TeamIndex &teamIndex, const TeamId &newTeamId, const ChangeTeamIdCb &callbackFunctor);

     /*! **********************************************************************************************************/
    /*! \name Swap Team

        Game admins can change swap the players between teams after starting the game. This action is invalid for a game 
        without teams,a team cannot be switched to INVALID_TEAM_ID. If we can not accommodate the reassignment because of team sizes,
        an error is returned to the client.If a user is specified in the team change request that is no longer in the game session, 
        the request is allowed to proceed as if the user was not mentioned. The user could have left the game session just after 
        the admin sent the request.

    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a Team & Role swap's success or failure.
        
            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
            \param[in] SwapPlayersErrorInfo * pointer to an error object with more details on the failure (nullptr on ERR_OK)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const SwapPlayersErrorInfo*, JobId> SwapPlayerTeamsAndRolesJobCb;
        typedef Functor3<BlazeError, Game *, const SwapPlayersErrorInfo*> SwapPlayerTeamsAndRolesCb;

        /*! ************************************************************************************************/
        /*! \brief swap players between teams, roles, and/or slots

        Note: If we can not accommodate the reassignment because of team sizes, an error "GAMEMANAGER_ERR_TEAM_FULL"
        is returned to the client. If we can not accommodate the reassignment because of role capacities, an error "GAMEMANAGER_ERR_ROLE_FULL"
        is returned to the client. If any user is assigned to a role not valid for the game, "GAMEMANAGER_ERR_ROLE_NOT_ALLOWED" is returned. 
        If a user specified in the request is no longer in the game session, the request is allowed to proceed as if the user was not mentioned.
        Role entry criteria is not validated for the players being swapped.

        Titles using roles functionality must explicitly supply a role for each player in the request.

        \param[in] swapPlayerDataList the list of players' team id which will be swapped
        \param[in] callbackFunctor the functor dispatched indicating success/failure.
        ***************************************************************************************************/
        JobId swapPlayerTeamsAndRoles(const SwapPlayerDataList &swapPlayerDataList, const SwapPlayerTeamsAndRolesJobCb &callbackFunctor);
        JobId swapPlayerTeamsAndRoles(const SwapPlayerDataList &swapPlayerDataList, const SwapPlayerTeamsAndRolesCb &callbackFunctor);


     /*! **********************************************************************************************************/
    /*! \name Game Attribute Maps

        Game Attributes are stored on the blazeServer and are broadcasted to all game members.
    **************************************************************************************************************/

         /*! **********************************************************************************************************/
        /*! \brief  returns the game mode attribute value for this game.
        \return    Returns the game mode attribute value for this game.
        **************************************************************************************************************/
        virtual const char8_t* getGameMode() const;

         /*! **********************************************************************************************************/
        /*! \brief  returns the attribute name that corresponds to the game mode.
        \return    Rreturns the attribute name that corresponds to the game mode.
        **************************************************************************************************************/
        const char8_t* getGameModeAttributeName() const { return mGameModeAttributeName.c_str(); }
        
        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's AttributeMap update.  Success indicates that
                the game will soon dispatch GameListener::onGameAttributeUpdated.

            Note: the game's attribute map has NOT been updated at the time this callback is dispatched.  If the
                error code is ERR_OK, the game will dispatch GameListener::onGameAttributeUpdated (once the attributes
                have been updated).

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> ChangeGameAttributeJobCb;
        typedef Functor2<BlazeError, Game *> ChangeGameAttributeCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief create/update a single attribute for this game.  Requires admin rights.

            If no attribute exists with the supplied name, one is created; otherwise the existing value
                is updated.

            Note: use setGameAttributeMap to update multiple attributes in a single RPC.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
            \param[in] attributeValue the value to set the attribute to; must be < 256 characters
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        ***************************************************************************************************/
        JobId setGameAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangeGameAttributeJobCb &callbackFunctor);
        JobId setGameAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangeGameAttributeCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  create/update multiple attributes for this game in a single RPC.  Requires admin rights.
                NOTE: the server merges existing attributes with the supplied attributeMap (so omitted attributes retain their previous values).

            If no attribute exists with the supplied name, one is created; otherwise the existing value
            is updated.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] changingAttributes an attributeMap containing attributes to create/update on the server.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        **************************************************************************************************************/
        JobId setGameAttributeMap(const Collections::AttributeMap *changingAttributes, const ChangeGameAttributeJobCb &callbackFunctor);
        JobId setGameAttributeMap(const Collections::AttributeMap *changingAttributes, const ChangeGameAttributeCb &callbackFunctor);


        /*! ************************************************************************************************/
        /*! \brief create/update a single Dedicated Server attribute for this game.  Only usable by the Dedicated Server itself.

            If no attribute exists with the supplied name, one is created; otherwise the existing value
                is updated.

            Note: use setGameAttributeMap to update multiple attributes in a single RPC.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] attributeName the attribute name to get; case insensitive, must be < 32 characters
            \param[in] attributeValue the value to set the attribute to; must be < 256 characters
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        ***************************************************************************************************/
        JobId setDedicatedServerAttributeValue(const char8_t* attributeName, const char8_t* attributeValue, const ChangeGameAttributeJobCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  create/update multiple  Dedicated Server attributes for this game in a single RPC.  Only usable by the Dedicated Server itself.
                NOTE: the server merges existing attributes with the supplied attributeMap (so omitted attributes retain their previous values).

            If no attribute exists with the supplied name, one is created; otherwise the existing value
            is updated.

            Attribute names are case insensitive, and must be  < 32 characters long (see MAX_ATTRIBUTENAME_LEN)
            Attribute values must be < 256 characters long (see MAX_ATTRIBUTEVALUE_LEN)

            \param[in] changingAttributes an attributeMap containing attributes to create/update on the server.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        **************************************************************************************************************/
        JobId setDedicatedServerAttributeMap(const Collections::AttributeMap *changingAttributes, const ChangeGameAttributeJobCb &callbackFunctor);

    /*! **********************************************************************************************************/
    /*! \name Game Admin Rights

        Each game has a list of users who are allowed to take administrative actions on the game (players/users with admin rights).
        Certain actions can only be taken by a player with admin rights (for example, advancing the game's state or kicking a
            a player out of the game).

        Note: the host of a dedicated server is automatically a game admin, even though they are not a player in the game.

        \sa
        \li GameBase::isAdmin(BlazeId)const
        \li Player::isAdmin()const
    **************************************************************************************************************/


        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's admin list modification.  Success indicates that
                the game will soon dispatch GameListener::onAdminListChanged (once the list has been updated locally).

            Note: the game's admin list has NOT been updated at the time this callback is dispatched.  If the
                error code is ERR_OK, the game will dispatch GameListener::onAdminListChanged - at that time the
                game's admin list has been updated.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
            \param[in] Player* the player who was added/removed from the game's admin list (nullptr on error)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, Player *, JobId > UpdateAdminListJobCb;
        typedef Functor3<BlazeError, Game *, Player * > UpdateAdminListCb;  // DEPRECATED
        typedef Functor4<BlazeError, Game *, BlazeId, JobId >  UpdateAdminListBlazeIdJobCb;
        typedef Functor3<BlazeError, Game *, BlazeId >  UpdateAdminListBlazeIdCb;  // DEPRECATED
        
        /*! **********************************************************************************************************/
        /*! \brief Add a player to the game's admin list, grants the player admin rights on this game. Note: requires admin rights.

            \param[in] player - player to be added to the admin list of game
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId addPlayerToAdminList(Player *player, const UpdateAdminListJobCb &callbackFunctor);
        JobId addPlayerToAdminList(Player *player, const UpdateAdminListCb &callbackFunctor);
        JobId addPlayerToAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb &callbackFunctor);
        JobId addPlayerToAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Remove a player from the game's admin list, revoking the player's admin rights on this game. Note: requires admin rights.

            Note: removing a player's admin rights does not remove them from the game.

            \param[in] player - player to remove from the admin list
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId removePlayerFromAdminList(Player *player, const UpdateAdminListJobCb &callbackFunctor);
        JobId removePlayerFromAdminList(Player *player, const UpdateAdminListCb &callbackFunctor);
        JobId removePlayerFromAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb &callbackFunctor);
        JobId removePlayerFromAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdCb &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief Remove the local user as an admin in the game and migrate those admin privileges to the
            specified player. Note: requires admin rights.

            Note: removing the local player's admin rights does not remove them from the game.

            \param[in] player - player to remove from the admin list
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId migratePlayerAdminList(Player *player, const UpdateAdminListJobCb &callbackFunctor);
        JobId migratePlayerAdminList(Player *player, const UpdateAdminListCb &callbackFunctor);
        JobId migratePlayerAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdJobCb &callbackFunctor);
        JobId migratePlayerAdminList(BlazeId blazeId, const UpdateAdminListBlazeIdCb &callbackFunctor);

    /*! **********************************************************************************************************/
    /*! \name Opting out of preferred joins
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor indicating a preferred join opt out's success or failure.

            \param[in] BlazeError the error code for the opt out attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> PreferredJoinOptOutJobCb;
        typedef Functor2<BlazeError, Game *> PreferredJoinOptOutCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief Opt out the local player if the game is locked for preferred joins.

            The lock is cleared, if all active players in the game have called joinGameByUserList or preferredJoinOptOut
            in response to the lock.

            \param[in] titleCb the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId preferredJoinOptOut(const PreferredJoinOptOutJobCb &titleCb);
        JobId preferredJoinOptOut(const PreferredJoinOptOutCb &titleCb); // DEPRECATED

    /*! **********************************************************************************************************/
    /*! \name External Sessions
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's external session image update.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
            \param[in] ExternalSessionErrorInfo * pointer to an error object with more details on the failure (nullptr on ERR_OK)
        ***************************************************************************************************/
        typedef Functor4<BlazeError, Game *, const ExternalSessionErrorInfo *, JobId> UpdateExternalSessionImageJobCb;
        typedef Functor3<BlazeError, Game *, const ExternalSessionErrorInfo *> UpdateExternalSessionImageCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief update the external session image for this game.  Requires admin rights.

            \param[in] image the binary data for the image to set
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        ***************************************************************************************************/
        JobId updateExternalSessionImage(const Ps4NpSessionImage& image, const UpdateExternalSessionImageJobCb &callbackFunctor);
        JobId updateExternalSessionImage(const Ps4NpSessionImage& image, const UpdateExternalSessionImageCb &callbackFunctor); // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's external session status update.

            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (possibly nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game *, JobId> UpdateExternalSessionStatusJobCb;
        typedef Functor2<BlazeError, Game *> UpdateExternalSessionStatusCb;  // DEPRECATED

        /*! ************************************************************************************************/
        /*! \brief update the external session image for this game.  Requires admin rights.

            \param[in] status the status to set
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
            \return JobId for pending action.
        ***************************************************************************************************/
        JobId updateExternalSessionStatus(const ExternalSessionStatus& status, const UpdateExternalSessionStatusJobCb &callbackFunctor);
        JobId updateExternalSessionStatus(const ExternalSessionStatus& status, const UpdateExternalSessionStatusCb &callbackFunctor); // DEPRECATED


    /*! ****************************************************************************/
    /*! \name GameListener methods
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched with the result of a game's PresenceMode setting


            \param[in] BlazeError the error code for the update attempt
            \param[in] Game* a pointer to the game (nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, Game*, JobId> SetPresenceModeJobCb;
        typedef Functor2<BlazeError, Game*> SetPresenceModeCb;  // DEPRECATED

        /*! **********************************************************************************************************/
        /*! \brief Changes a game's presence mode, to take effect with the next replay. Note: requires admin rights.

            \param[in] presenceMode - the new presence mode for the game.
            \param[in] callbackFunctor the functor dispatched indicating success/failure.
        **************************************************************************************************************/
        JobId setPresenceMode(PresenceMode presenceMode, const SetPresenceModeJobCb& callbackFunctor);

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive async notifications from the game.

            Once an object adds itself as a listener, the game will dispatch
            async events to the object via the methods in GameListener.

            \param listener The listener to add.
        ********************************************************************************/
        void addListener(GameListener *listener) { mDispatcher.addDispatchee( listener ); }

        /*! ****************************************************************************/
        /*! \brief Removes a listener from the game, preventing any further async
                notifications from being dispatched to the listener.

            \param listener The listener to remove.
        ********************************************************************************/
        virtual void removeListener(GameListener *listener) { mDispatcher.removeDispatchee( listener); }

    /*! ****************************************************************************/
    /*! \name Misc Accessors
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief return the GameManagerAPI instance that contains this game object.
            \return the GameManagerAPI instance that contains this game.
        ***************************************************************************************************/
        GameManagerAPI *getGameManagerAPI() const { return &mGameManagerApi; }

    /*! **********************************************************************************************************/
    /*! \name Mesh Interface Implementation

    Games implement the Mesh interface so they can interact with a NetworkMeshAdapter instance.
    The Mesh interface corresponds to the game, while the MeshEndpoint interface corresponds to a Player.
        
    You can ignore these methods if you're using one of the existing network adapter objects.
    **************************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief return the name of this mesh (the game's name)
        ***************************************************************************************************/
        virtual const char8_t *getName() const {return mName; }

        /*! ************************************************************************************************/
        /*! \brief return the id of this mesh (the game's GameId)
        ***************************************************************************************************/
        virtual GameId getId() const { return mGameId; }

        /*! ************************************************************************************************/
        /*! \brief returns the UUID of the mesh (the game's UUID)
        ***************************************************************************************************/
        virtual const char8_t* getUUID() const { return mUUID.c_str(); }

        /*! ************************************************************************************************/
        /*! \brief return the platform specific network address for this game's topology host. 
        ***************************************************************************************************/
        virtual const NetworkAddress* getNetworkAddress() const { return getNetworkAddress(mTopologyHostMeshEndpoint); }

        /*! ************************************************************************************************/
        /*! \brief returns true if join in progress is specified as supported for the mesh.
        ***************************************************************************************************/
        virtual bool isJoinInProgressSupported() const { return mGameSettings.getJoinInProgressSupported(); }
        
#if defined(EA_PLATFORM_PS4)
        /*! ************************************************************************************************/
        /*! \brief returns the np session id of the mesh
        ***************************************************************************************************/    
        virtual const char8_t* getNpSessionId() const { return mExternalSessionIdentification.getPs4().getNpSessionId();}
#endif

        virtual const char *getScid() const { return mScid.c_str(); }
        virtual const char *getExternalSessionTemplateName() const { return mExternalSessionIdentification.getXone().getTemplateName(); }
        virtual const char *getExternalSessionName() const { return mExternalSessionIdentification.getXone().getSessionName(); }
        virtual const char *getExternalSessionCorrelationId() const { return mExternalSessionIdentification.getXone().getCorrelationId(); }


        const ExternalSessionIdentification& getExternalSessionIdentification() const { return mExternalSessionIdentification; }

        /*! ************************************************************************************************/
        /*! \brief returns the local push context id for this game
        ***************************************************************************************************/
        const char8_t* getPsnPushContextId() const { return mPsnPushContextId.c_str(); }


        /*! **********************************************************************************************************/
        /*! \brief Determines whether or not the local user is the topology host for this game.
            Integrators note: Older versions of Blaze had an isHosting() method that's been removed. Use isTopologyHost() instead.
            \return    Returns true if the local user is the topology host for this game
        **************************************************************************************************************/
        bool isTopologyHost() const;

        /*! ************************************************************************************************/
        /*! \brief returns true if the local endpoint is a dedicated server in the mesh
        ***************************************************************************************************/
        virtual bool isDedicatedServerHost() const { return (mDedicatedHostEndpoint == mLocalEndpoint); }

        /*! **********************************************************************************************************/
        /*! \brief Returns true if a local user is the platform host of this game.

            The platform host isn't always the same as the topology host. For example, when we're running a PC dedicated
            server for a console title, the PC is the topology host while one of the console acts as the platform host.

            \return    Returns true if the local user is the platform host of this game.
        **************************************************************************************************************/
        bool isPlatformHost() const;

        /*! ************************************************************************************************/
        /*! \brief whether the local user is external owner of this game.
        ***************************************************************************************************/
        bool isExternalOwner() const { return mIsExternalOwner; }

        const HostInfo& getExternalOwnerInfo() const { return mExternalOwnerInfo; }

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint of the network topology host.  
        
            \return the host MeshEndpoint.
        ***************************************************************************************************/
        virtual const MeshEndpoint *getTopologyHostMeshEndpoint() const;

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint of the dedicated topology host.  Will be nullptr if this is not a
                dedicated server.
        
            \return the host as a MeshEndpoint.
        ***************************************************************************************************/
        virtual const MeshEndpoint *getDedicatedServerHostMeshEndpoint() const; 

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint of the platform host.  (Session host player).  Will be nullptr if this
                is a dedicated server game and no user has been added.  (since dedicated server topology host
                is not in the game).
            \return the platform host player as a MeshEndpoint.
        ***************************************************************************************************/
        virtual const MeshMember *getPlatformHostMeshMember() const;

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint of this game's local player.
            \return the local player as a MeshEndpoint (nullptr if this is a dedicated server game).
        ***************************************************************************************************/
        virtual const MeshEndpoint *getLocalMeshEndpoint() const;

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint (active player) with a given blazeId.
            \param[in] blazeId the blazeId of the meshEndpoint to get.
            \return the MeshEndpoint, or nullptr if none was found.
        ***************************************************************************************************/
        virtual const MeshMember* getMeshMemberById(BlazeId blazeId) const;

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint (active player) with a given name (case-insensitive)
            \param[in] name the name of the meshEndpoint to get.
            \return the MeshEndpoint, or nullptr if none was found.
        ***************************************************************************************************/
        virtual const MeshMember* getMeshMemberByName(const char8_t *name) const;

        /*! ************************************************************************************************/
        /*! \brief returns the first member of the mesh that is part of the given connection group.
            \param[in] connectionGroupId - the connection group to look for.
            \return the MeshEndpoint, or nullptr if none was found.
        ***************************************************************************************************/
        virtual const MeshEndpoint* getMeshEndpointByConnectionGroupId(ConnectionGroupId connectionGroupId) const;

        /*! ************************************************************************************************/
        /*! \brief return the MeshEndpoint (console) at the supplied index.
            \param[in] index an index ranging from 0..(getMeshEndpointCount()-1)
            \return the MeshEndpoint, or nullptr if the index is out of bounds
        ***************************************************************************************************/
        virtual const MeshEndpoint* getMeshEndpointByIndex(uint16_t index) const;

        /*! ************************************************************************************************/
        /*! \brief return the number of mesh endpoints (machines) in this game.
            \return the number of mesh endpoints (active players) in this game
        ***************************************************************************************************/
        virtual uint16_t getMeshEndpointCount() const { return static_cast<uint16_t>( mActiveGameEndpoints.size() ); }

        /*! ************************************************************************************************/
        /*! \brief return the MeshMember (active player) at the supplied index.
            \param[in] index an index ranging from [0 - total active Players) 
            \return the MeshEndpoint, or nullptr if the index is out of bounds
        ***************************************************************************************************/
        virtual const MeshMember* getMeshMemberByIndex(uint16_t index) const;

        /*! ************************************************************************************************/
        /*! \brief return the number of mesh members (active players) in this game.
            \return the number of mesh endpoints (active players) in this game
        ***************************************************************************************************/
        virtual uint16_t getMeshMemberCount() const { return getActivePlayerCount(); }

        /*! ************************************************************************************************/
        /*! \brief Returns the public and private slot capacities of this mesh in the supplied arguments and 
                returns the total player capacity.
            \param[out] publicCapacity the mesh's public player slot capacity
            \param[out] privateCapacity the mesh's private player slot capacity
            \return returns the total player capacity (sum of all open player slots)
        ***************************************************************************************************/
        virtual uint16_t getMeshMemberCapacity(uint16_t &publicCapacity, uint16_t &privateCapacity) const
        {
            // we combine the public & private spectator slots with their respective player slots
            // at the mesh level, they're no different from player slots
            publicCapacity = static_cast<uint16_t>( getPlayerCapacity(SLOT_PUBLIC_PARTICIPANT) + getPlayerCapacity(SLOT_PUBLIC_SPECTATOR) );
            privateCapacity = static_cast<uint16_t>( getPlayerCapacity(SLOT_PRIVATE_PARTICIPANT) + getPlayerCapacity(SLOT_PRIVATE_SPECTATOR) );
            return getPlayerCapacityTotal();
        }

        /*! ************************************************************************************************/
        /*! \brief Returns the public and private slot capacities of this mesh in the supplied arguments and 
                returns the total player capacity for use by first-party.
                This does *not* include LIMITED_GAMEPLAY_USERS.

            \param[out] publicCapacity the mesh's public player slot capacity
            \param[out] privateCapacity the mesh's private player slot capacity
            \return returns the total player capacity (sum of all open player slots)
        ***************************************************************************************************/
        virtual uint16_t getFirstPartyCapacity(uint16_t &publicCapacity, uint16_t &privateCapacity) const;

        /*! ************************************************************************************************/
        /*! \brief return a map of attributes associated with this game's network mesh.  Note: this is a separate
                attribute map from the gameAttributes.

            Mesh attributes are specified when a game is created and are used by your network adapter.
            Mesh attributes cannot be altered once a game is created.  see CreateGameParameters::mMeshAttributes

            An example of a mesh attribute would be the address & port used by a dedicated 3rd party
            voip server associated with a specific dedicated server instance.

            \return return a map attributes associated with this game's network mesh.
        ***************************************************************************************************/
        virtual const Blaze::Collections::AttributeMap* getMeshAttributes() const { return &mMeshAttributes; }

         /*! **********************************************************************************************************/
        /*! \brief Returns the endpoint slot id for the game's topology host player's machine. 
            \return the game host's machine's slot id.
        **************************************************************************************************************/
        virtual SlotId getTopologyHostConnectionSlotId() const;

        /*! ************************************************************************************************/
        /*! \brief  Returns the topology hosts connection group id.
            \return The topology hosts connection group id
        ***************************************************************************************************/
        virtual uint64_t getTopologyHostConnectionGroupId() const;

        /*! **********************************************************************************************************/
        /*! \brief Returns the endpoint slot id for the game's platform host player.
                   If MLU is used, this corresponds to the player's individual connection.
            \return the game's host slot id.
        **************************************************************************************************************/
        virtual SlotId getPlatformHostSlotId() const { return mPlatformHostSlotId; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the endpoint slot id for the game's platform host player's machine.
                   If the game supports MLU, there may be multiple players with this connection slot id.
            \return the game's host's machine's  slot id.
        **************************************************************************************************************/
        virtual SlotId getPlatformHostConnectionSlotId() const { return mPlatformHostConnectionSlotId; }

    /*! **********************************************************************************************************/
    /*! End Mesh Interface Implementation
    **************************************************************************************************************/
        /*! ************************************************************************************************/
        /*! \brief return the platform specific network address for the given endpoint.
        ***************************************************************************************************/
        const NetworkAddress* getNetworkAddress(const GameEndpoint *endpoint) const;

        /*! ************************************************************************************************/
        /*! \brief Returns game id secret
            
            Note: only when the owner is host, the real secret will be returned, otherwise, it's empty
            The length of secrete is defined as SECRET_MAX_LENGTH in uuid.tdf, has to be multiple of 8

            \return returns game id secret
        ***************************************************************************************************/
        const PersistedGameIdSecret& getPersistedGameIdSecret() const { return mPersistedGameIdSecret; } 

        /*! ****************************************************************************/
        /*! \brief Gets the QOS statistics for a given endpoint

            \param endpoint The target endpoint
            \param qosData a struct to contain the qos values
            \return a bool true, if the call was successful
        ********************************************************************************/
        bool getQosStatisticsForEndpoint(const MeshEndpoint *endpoint, Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats = false) const;

        /*! ****************************************************************************/
        /*! \brief Gets the QOS statistics for the topology host

            \param qosData a struct to contain the qos values
            \return a bool true, if the call was successful
        ********************************************************************************/
        bool getQosStatisticsForTopologyHost(Blaze::BlazeNetworkAdapter::MeshEndpointQos &qosData, bool bInitialQosStats = false) const;

        /*! ************************************************************************************************/
        /*! \brief Returns the flag as specified in the create request.  Non resettable servers
            are not allowed to be returned back to the pool of available dedicated servers.
        ***************************************************************************************************/
        bool isServerNotResetable() const { return mServerNotResetable; }

        /*! ************************************************************************************************/
        /*! \brief Returns the default messaging entity type, which is for game sessions.
        ***************************************************************************************************/
        static EA::TDF::ObjectType getEntityType() { return ENTITY_TYPE_GAME; }
        static bool isValidMessageEntityType(const EA::TDF::ObjectType& entityType) { return ((entityType == ENTITY_TYPE_GAME) || (entityType == ENTITY_TYPE_GAME_GROUP)); }
        virtual EA::TDF::ObjectId getBlazeObjectId() const { return EA::TDF::ObjectId(((getGameType() == GAME_TYPE_GAMESESSION)? ENTITY_TYPE_GAME : ENTITY_TYPE_GAME_GROUP), static_cast<EntityId>(getId())); }

        bool isNetworkCreated() const { return mIsNetworkCreated; }

        bool isSettingUpNetwork() const { return (mIsLocalPlayerPreInitNetwork || mIsLocalPlayerInitNetwork); }

        /*! ************************************************************************************************/
        /*! \brief call during onResolveGameMembership to signal to a pre-existing game that the local player shouldn't leave.
        ***************************************************************************************************/
        void retainGameMembershipThroughResolve() { mRemoveLocalPlayerAfterResolve = false; }

        /*! ************************************************************************************************/
        /*! \brief the time when this game was created as tracked by Blaze Server
        ***************************************************************************************************/
        const TimeValue& getCreateTime() const { return mCreateTime; }
        
        void setLeavingPlayerConnFlags(uint32_t connFlags) { mLeavingPlayerConnFlags = connFlags; }

        uint32_t getLeavingPlayerConnFlags() const { return mLeavingPlayerConnFlags; }

        bool isLocalPlayerMapEmpty() const { return mLocalPlayerMap.empty(); }

#ifdef ARSON_BLAZE
        void setQosStatisticsDebug(ConnectionGroupId remoteConnGroupId, const Blaze::BlazeNetworkAdapter::MeshEndpointQos& debugQosData);
        void clearQosStatisticsDebug(ConnectionGroupId remoteConnGroupId);
#endif
        CCSMode getCCSMode() const { return mCCSMode;}
        bool doesLeverageCCS() const;

    private:

        NON_COPYABLE(Game); // disable copy ctor & assignment operator

        Game(GameManagerAPI &gameManagerAPI, const ReplicatedGameData *replicatedGameData, const ReplicatedGamePlayerList& gameRoster, 
             const ReplicatedGamePlayerList& gameQueue, const GameSetupReason &setupReason, const TimeValue& telemetryInterval, const QosSettings& qosSettings,
             bool performQosOnActivePlayers, bool locksForPreferredJoinsEnabled, const char8_t* gameAttributeName, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        virtual ~Game();

        void initRoster(const ReplicatedGamePlayerList& gameRoster, bool performQosOnActivePlayers);
        void initQueue(const ReplicatedGamePlayerList& gameQueue);
        bool initiatePlayerConnections(const Player* joiningPlayer, uint32_t userIndex);
        void setGameReportingId(GameReportingId gameReportingId) { mGameReportingId = gameReportingId; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's network address list, used to initalize the game
        **************************************************************************************************************/
        virtual NetworkAddressList* getNetworkAddressList();



        Player* addPlayer(const ReplicatedGamePlayer &playerData);
        Player* addPlayerToQueue(const ReplicatedGamePlayer &playerData);
        bool promotePlayerFromQueue(const ReplicatedGamePlayer &playerData);
        bool demotePlayerToQueue(const ReplicatedGamePlayer &playerData);
        bool claimPlayerReservation(const ReplicatedGamePlayer &playerData);

        /*! **********************************************************************************************************/
        /*! \brief Update the endpoint associated with a particular player
            \param[in] player - the player who may need a new endpoint
            \param[in] playerData - replicated player data to use for endpoint data
            \param[in] updateConnectionSlotId - if true, and there's an existing endpoint for the ConnectionGroupId 
                        specified in playerData, we'll update the slot ID stored in that existing endpoint.
                        This should only be needed when claiming a reservation while not queued, 
                        or when being promoted from the game queue.
        **************************************************************************************************************/
        void updatePlayerEndpoint(Player &player, const ReplicatedGamePlayer &playerData, bool updateConnectionSlotId );

        /*! **********************************************************************************************************/
        /*! \brief Put the given endpoint into the active endpoint list.
        **************************************************************************************************************/
        void activateEndpoint(GameEndpoint *endpoint);
        /*! **********************************************************************************************************/
        /*! \brief Remove the given endpoint from the active endpoint list.
        **************************************************************************************************************/
        void deactivateEndpoint(GameEndpoint *endpoint);


        void onPlayerRemoved(PlayerId leavingPlayerId, const PlayerRemovedReason playerRemovedReason, const PlayerRemovedTitleContext, uint32_t userIndex);
        void cleanupPlayerNetConnection(const Player* leavingPlayer, bool removeFromAllPlayers, const Player* localPlayer, bool isLocalPlayerHost, const PlayerRemovedReason playerRemovedReason);

        Player* getRosterPlayerById(BlazeId playerId) const;
        Player* getRosterPlayerByIndex(uint16_t index) const;
        Player* getRosterPlayerByName(const char8_t *personaName) const;
        Player* getRosterPlayerByUser(const UserManager::User *user) const;
        Player* getPlayerInternal(const PlayerId playerId) const;

        BlazeNetworkAdapter::NetworkMeshHelper& getNetworkMeshHelper() { return mNetworkMeshHelper; }

        bool resolveGameMembership(Game* newGame);
        void connectToDeferredJoiningPlayers();
        /*! ************************************************************************************************/
        /*!
        \return TeamInfo object describing team for the given Id. Returns nullptr if team out of range
        ********************************************************************************/
        GameBase::TeamInfo* getTeamByIndex(TeamIndex teamIndex);

        bool containsPlayer( const Player *player ) const;
        bool containsPlayer( const PlayerId playerId ) const;
        bool removePlayerFromRoster(Player *player);
        bool isPeerToPeer() const { return (mNetworkTopology == PEER_TO_PEER_FULL_MESH); }


        JobId advanceGameState(GameState state, const ChangeGameStateJobCb &titleCb);

        // Notification response helper functions
        void onNotifyGameStateChanged(GameState newGameState, uint32_t userIndex);
        void onNotifyGameReset(const ReplicatedGameData *gameData);
        void onNotifyGameAttributeChanged(const Collections::AttributeMap *newAttributeMap);
        void onNotifyDedicatedServerAttributeChanged(const Collections::AttributeMap *newAttributeMap);
        void onNotifyGameCapacityChanged(const SlotCapacitiesVector &newSlotCapacities, const TeamDetailsList& newTeamRosters, const RoleInformation& roleInformation);
        void onNotifyGameSettingsChanged(const GameSettings *newGameSettings);
        void onNotifyGameModRegisterChanged(GameModRegister newGameModRegister);
        void onNotifyGameEntryCriteriaChanged(const EntryCriteriaMap& entryCriteriaMap, const RoleEntryCriteriaMap &roleEntryCriteria);

        void onNotifyJoinGameQueue();
        void onNotifyPlayerJoining(const ReplicatedGamePlayer *joiningPlayerData, uint32_t userIndex, bool performQosDuringConnection);
        void onNotifyPlayerJoinComplete(const NotifyPlayerJoinCompleted *notifyPlayerJoinCompleted, uint32_t userIndex);
        void onNotifyPlayerAttributeChanged(PlayerId playerId, const Collections::AttributeMap *newAttributeMap);
        void onNotifyPlayerCustomDataChanged(PlayerId playerId, const EA::TDF::TdfBlob *newBlob);
        void onNotifyInactivePlayerJoining(const ReplicatedGamePlayer &playerData, uint32_t userIndex);
        void onNotifyInactivePlayerJoiningQueue(const ReplicatedGamePlayer &playerData, uint32_t useIndex);
        void onNotifyJoiningPlayerInitiateConnections(const ReplicatedGamePlayer &replicatedPlayer, const GameSetupReason& reason);
        void onNotifyPlayerClaimingReservation(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex, bool performQosDuringConnection);
        void onNotifyPlayerPromotedFromQueue(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex);
        void onNotifyPlayerDemotedToQueue(const ReplicatedGamePlayer& joiningPlayerData, uint32_t userIndex);
        void onNotifyLocalPlayerGoingActive(const ReplicatedGamePlayer &playerData, bool performQosDuringConnection);

        void onNotifyGameAdminListChange(PlayerId playerId, UpdateAdminListOperation updateOperation, PlayerId updaterId);
        void onNotifyGamePlayerStateChanged(const PlayerId playerId, const PlayerState state, uint32_t userIndex);
        void onNotifyGamePlayerTeamRoleSlotChanged(const PlayerId playerId, const TeamIndex newTeamIndex, const RoleName &newPlayerRole, SlotType newSlotType);
        void onNotifyGameTeamIdChanged(const TeamIndex teamIndex, const TeamId newTeamId);
        void onNotifyProcessQueue();
        void onNotifyQueueChanged(const PlayerIdList& playerIdlist);

        void onNotifyGameNameChanged(const GameName &newName);
        void onNotifyPresenceModeChanged(PresenceMode newPresenceMode);

        void onNotifyHostedConnectivityAvailable(const NotifyHostedConnectivityAvailable& notification, uint32_t userIndex);

        //voip status
        void gameVoipConnected(Blaze::ConnectionGroupId targetConnGroupId);
        void gameVoipLostConnection(Blaze::ConnectionGroupId targetConnGroupId);

        //host migration
        void initPlatformHostSlotId(const SlotId slotId, const PlayerId platformHostId);
        void onNotifyHostMigrationStart(const PlayerId playerId, const SlotId slotId, const SlotId newHostConnectionSlotId, HostMigrationType hostMigrationType, uint32_t userIndex);
        void onNotifyHostMigrationFinish();
        void onNotifyPlatformHostChanged(const PlayerId playerId);

        JobId setPlayerCapacityInternal(const SlotCapacities &newSlotCapacities, const TeamDetailsList &newTeamsComposition, const RoleInformation &roleInformation, const ChangePlayerCapacityJobCb &titleCb);

        // Helper calbacks for back-compat wrapping:
public:
        void helperCallback(BlazeError err, Game* game, JobId jobId);
        void helperCallback(BlazeError err, Game* game, BlazeId blazeId, JobId jobId);
        void helperCallback(BlazeError err, Game* game, Player* player, JobId jobId);
        void helperCallback(BlazeError err, Game* game, const Player* player, JobId jobId);
        void helperCallback(BlazeError err, Game* game, PlayerIdList * playeridList, JobId jobId);
        void helperCallback(BlazeError err, Game* game, const SwapPlayersErrorInfo * swapinfo, JobId jobId);
        void helperCallback(BlazeError err, Game* game, const ExternalSessionErrorInfo * extInfo, JobId jobId);
        void helperCallback(BlazeError err, Game* game, const BannedListResponse* bannedResp, JobId jobId);
private:

        // Callback functions
        void changeGameStateCb(BlazeError error, JobId id, ChangeGameStateJobCb titleCb);
        void internalSetGameSettingsCb(BlazeError error,  JobId id, ChangeGameSettingsJobCb titleCb);
        void internalSetGameAttributeCb(BlazeError error, JobId id, ChangeGameAttributeJobCb titleCb);
        void internalSetDedicatedServerAttributeCb(BlazeError error, JobId id, ChangeGameAttributeJobCb titleCb);
        void leaveGameCb(BlazeError error, JobId id, LeaveGameJobCb titleCb);
        void migrateHostCb(BlazeError error, JobId id,  MigrateHostJobCb titleCb);
        void destroyGameCb(const DestroyGameResponse *response, BlazeError error, JobId id, DestroyGameJobCb titleCb, GameDestructionReason destructionReason);
        void changeGameTeamIdCb(BlazeError error, JobId id, ChangeTeamIdJobCb titleCb);
        void internalSwapPlayersCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, SwapPlayerTeamsAndRolesJobCb titleCb);
        void ejectHostCb(BlazeError error, JobId id, EjectHostJobCb titleCb);
        
        void setPlayerCapacityCb(const SwapPlayersErrorInfo *swapPlayersError, BlazeError error, JobId id, ChangePlayerCapacityJobCb titleCb);
        void internalRemovePlayerCallback(BlazeError, JobId id, KickPlayerJobCb titleCb, PlayerId playerId);
        void internalBanUserCallback(BlazeError err, JobId id, BanPlayerJobCb titleCb, BlazeId blazeId);
        void internalBanUsersCallback(BlazeError err, JobId id, BanPlayerListJobCb titleCb, BlazeIdList* blazeIds);
        void internalUpdateAdminPlayerBlazeIdCb(BlazeError err, JobId id, UpdateAdminListBlazeIdJobCb titleCb, PlayerId playerId);
        void internalUpdateAdminPlayerCb(BlazeError err, JobId id, UpdateAdminListJobCb titleCb, PlayerId playerId);
        

        void internalAddQueuedPlayerToGameCb(BlazeError error, JobId id, PlayerId playerId, AddQueuedPlayerJobCb titleCb);
        void internalUnbanUserCallback(BlazeError err, JobId id, UnbanPlayerJobCb titleCb, BlazeId blazeId);
        void clearBannedListCallback(BlazeError err, JobId id, ClearBannedListJobCb titleCb);
        void getBannedListCallback(const BannedListResponse *response, BlazeError error, JobId id, GetBannedListJobCb titleCb);

        void internalSetPresenceModeCb(BlazeError error, JobId id, SetPresenceModeJobCb titleCb);
        void updateGameNameCallback(BlazeError err, JobId id, UpdateGameNameJobCb titleCb);
        void preferredJoinOptOutCb(BlazeError error, JobId id, PreferredJoinOptOutJobCb titleCb);

        void setNetworkCreated() { mIsNetworkCreated = true; }

        void internalUpdateSessionImageCb(const ExternalSessionErrorInfo *errorInfo, BlazeError error, JobId id, UpdateExternalSessionImageJobCb titleCb);
        void internalUpdateSessionStatusCb(BlazeError error, JobId id, UpdateExternalSessionStatusJobCb titleCb);
        void setNpSessionId(const char8_t* npSessionId) { mExternalSessionIdentification.getPs4().setNpSessionId(npSessionId); }

        bool incrementLocalTeamSize(const TeamIndex newTeamIndex, const RoleName& newRoleName);
        bool decrementLocalTeamSize(const TeamIndex previousTeamIndex, const RoleName& previousRoleName);

        /*! ************************************************************************************************/
        /*! \brief When host injection is completed by finding a host not local to the GM master hosting the 
             virtual game, players may have joined the session prior to the completion of the injection.
             This should be called when the Blaze server notifies the client that it is time to begin network 
             connections so the topology host information can be updated.
        ***************************************************************************************************/
        void completeRemoteHostInjection(const NotifyGameSetup *notifyGameSetup);

        /*! ************************************************************************************************/
        /*! \brief Update the host connection info. Used after an injection took place
        ***************************************************************************************************/
        void storeHostConnectionInfoPostInjection(const NotifyGameSetup &notifyGameSetup);

        /*! ************************************************************************************************/
        /*! \brief Finds an admin player of the game to use to send requests to the server.
        ***************************************************************************************************/
        GameManagerComponent* getAdminGameManagerComponent() const;

        GameManagerComponent* getPlayerGameManagerComponent(PlayerId playerId) const;

        void startTelemetryReporting();
        void stopTelemetryReporting();
        void reportGameTelemetry();
        void filloutTelemetryReport(Blaze::GameManager::TelemetryReportRequest* request, const MeshEndpoint *meshEndpoint);
        void reportTelemetryCb(BlazeError errorCode, JobId id);

        void cleanUpPlayer(Player *player);
        void cleanUpGameEndpoint(GameEndpoint *endpoint);

        void oldToNewExternalSessionIdentificationParams(const char8_t* sessionTemplate, const char8_t* externalSessionName, const char8_t* externalSessionCorrelationId, const char8_t* npSessionId, ExternalSessionIdentification& newParam);
        bool allConnectableCrossPlatformEndpointsHaveHostedConnectivity() const;
        bool allConnectableEndpointsHaveHostedConnectivity() const;
        bool shouldEndpointsConnect(const MeshEndpoint* endpoint1, const MeshEndpoint* endpoint2) const;
        bool isConnectionRequired(const MeshEndpoint* endpoint1, const MeshEndpoint* endpoint2) const;
        bool shouldSendMeshConnectionUpdateForVoip(Blaze::ConnectionGroupId targetConnGroupId) const;

        /*! ************************************************************************************************/
        /*! \brief return whether game will never have a topology host assigned to it. Only possible for NETWORK_DISABLED games
        ***************************************************************************************************/
        bool isHostless() const { return ((mTopologyHostId == INVALID_BLAZE_ID) && (mNetworkTopology == NETWORK_DISABLED)); }

        bool playerRequiresCCSConnectivity(const Player* joiningPlayer) const;
        bool hasFullCCSConnectivity() const;
        void handleDeferedCCSActions(uint32_t userIndex);

    private:
        UUID mUUID;

        typedef Dispatcher<GameListener> GameListenerDispatcher;
        GameListenerDispatcher mDispatcher;

        MemPool<Player> mPlayerMemoryPool;

        typedef vector_map<SlotId, Player *> PlayerRosterList;
        PlayerRosterList mActivePlayers;
        PlayerRosterList mRosterPlayers;
        PlayerRosterList mQueuedPlayers;

        typedef hash_map <PlayerId, Player *,
            eastl::hash<int64_t>, eastl::equal_to<int64_t> > PlayerMap;
        PlayerMap mPlayerRosterMap;

        typedef Blaze::hash_map<PlayerId, const Player*, eastl::hash<PlayerId> > DeferredJoiningPlayerMap;
        DeferredJoiningPlayerMap mDeferredJoiningPlayerMap;

        // since multiple players may be on the same mesh endpoint (console) we track these separately from the players

        MemPool<GameEndpoint> mGameEndpointMemoryPool;

        typedef vector_map<SlotId, GameEndpoint*> EndpointList;
        EndpointList mActiveGameEndpoints;

        typedef hash_map <ConnectionGroupId, GameEndpoint*,
            eastl::hash<int64_t>, eastl::equal_to<int64_t> > EndpointMap;
        EndpointMap mGameEndpointMap;

        BlazeNetworkAdapter::NetworkMeshHelper mNetworkMeshHelper;

        BlazeId mDedicatedServerHostId;
        GameEndpoint *mDedicatedHostEndpoint;
        GameEndpoint *mTopologyHostMeshEndpoint;
        Player *mPlatformHostPlayer;
        BlazeId mPlatformHostId;
        SlotId mPlatformHostSlotId;
        SlotId mPlatformHostConnectionSlotId;
        Collections::AttributeMap mMeshAttributes;
        

        // AUDI - consider moving the following to objects into a single data structure.
        PlayerVector mLocalPlayers;
        PlayerMap mLocalPlayerMap;
        GameEndpoint *mLocalEndpoint;

        GameManagerAPI &mGameManagerApi;
        GameNetworkTopology mNetworkTopology;

        // qos validation info
        uint32_t mQosDurationMs;
        uint32_t mQosIntervalMs;
        uint32_t mQosPacketSize;

        uint32_t mLeavingPlayerConnFlags;

        ExternalSessionIdentification mExternalSessionIdentification;

        XblScid mScid;
        PsnPushContextId mPsnPushContextId;

        bool mIsMigrating;
        bool mIsMigratingPlatformHost;
        bool mIsMigratingTopologyHost;

        bool mIsLocalPlayerPreInitNetwork;
        bool mIsLocalPlayerInitNetwork;
        
        bool mIsDirectlyReseting;  //!< Flag to indicate that the game was reset directly and not through the game manager api reset dedicated server flow.
        GameReportingId mGameReportingId;
        GameReportName mGameReportName;
        HostMigrationType mHostMigrationType;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
        void *mTitleContextData;

        PersistedGameIdSecret mPersistedGameIdSecret;
        uint32_t mSharedSeed; 

        bool mServerNotResetable;
        bool mIsNetworkCreated;
        bool mIsLockableForPreferredJoins;
        bool mRemoveLocalPlayerAfterResolve;
        TimeValue mTelemetryInterval;
        JobId mTelemetryReportingJobId;
        Collections::AttributeName mGameModeAttributeName; // Store this to allow access to the mode attribute
        TimeValue mCreateTime;
        CCSMode mCCSMode;
        bool mDelayingInitGameNetworkForCCS;
        bool mDelayingHostMigrationForCCS;

#ifdef ARSON_BLAZE
        typedef eastl::hash_map<ConnectionGroupId, Blaze::BlazeNetworkAdapter::MeshEndpointQos> RemoteConnGroupIdToQosStatsDebugMap;
        RemoteConnGroupIdToQosStatsDebugMap mQosStatisticsDebugMap;
#endif
        bool mIsExternalOwner;
        HostInfo mExternalOwnerInfo;
    };

    //!< Indicates whether game/1st party session is a user's primary joinable/invitable activity, for the 1st party UX
    enum PresenceActivityStatus
    {
        PRESENCE_INACTIVE, //!< user does not have it as his joinable/invitable activity.
        PRESENCE_ACTIVE    //!< user has the it as his joinable/invitable activity.
    };

    /*! ****************************************************************************/
    /*!
        \class GameListener

        \brief classes that wish to receive async notifications about Game events implement
        this interface, then add themselves as listeners.

        \brief Interface implemented by classes that wish to receive async notifications
            about GameManagerAPI events.

        Listeners must add themselves to each GameManagerAPI that they wish to get notifications about.
        See GameManagerAPI::addListener.
    ********************************************************************************/
    class BLAZESDK_API GameListener
    {
    public:

    /*! ****************************************************************************/
    /*! \name Game state related methods

        Note: all dispatches take place once the game enters the 'new' state.
        For example onPreGame fires off after the state is set to PRE_GAME.
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a game's state transitions into the
                GAME_GROUP_INITIALIZED state. Only dispatched for games of GameType GAME_TYPE_GROUP.

            \param game the game that's entered the GAME_GROUP_INITIALIZED state.
        ********************************************************************************/
        virtual void onGameGroupInitialized(Blaze::GameManager::Game *game);

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a game's state transitions into the
                PRE_GAME state.

            \param game the game that's entered the PRE_GAME state.
        ********************************************************************************/
        virtual void onPreGame(Blaze::GameManager::Game *game) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a game's state transitions into the
                IN_GAME state.

            \param game the game that's entered the IN_GAME state.
        ********************************************************************************/
        virtual void onGameStarted(Blaze::GameManager::Game *game) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a game's state transitions into the
                POST_GAME state.

            \param game the game that's entered the POST_GAME state.
        ********************************************************************************/
        virtual void onGameEnded(Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a dedicated server is reset ( its state
                transitions from RESETABLE to INITIALIZING).

            \param game the dedicated server that's being reset
        ***************************************************************************************************/
        virtual void onGameReset(Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification disptached when a dedicated server's state tramsits from POST_STATE,
            PRE_STATE or Initializing state to RESATABLE state

            \param[in] game the game with changed state
        ***************************************************************************************************/
        virtual void onReturnDServerToPool(Blaze::GameManager::Game *game) = 0;

    /*! ****************************************************************************/
    /*! \name Game Settings and Attribute updates
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game's attribute map changes.
                changedAttributeMap contains the changed or new attribute values and only those updated
                    values.

                At this point, the values have already been set into the game object and the old values
                    are lost.  The changedAttributeMap is only guaranteed to be valid until the call returns,
                    so copy off the map if you need to hold on to the information.


            \param[in] game the game with updated attributes
            \param[in] changedAttributeMap a map containing the new and updated attributes and values
        ***************************************************************************************************/
        virtual void onGameAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game dedicated server's attribute map changes.
                changedAttributeMap contains the changed or new attribute values and only those updated
                    values.

                At this point, the values have already been set into the game object and the old values
                    are lost.  The changedAttributeMap is only guaranteed to be valid until the call returns,
                    so copy off the map if you need to hold on to the information.


            \param[in] game the game with updated attributes
            \param[in] changedAttributeMap a map containing the new and updated attributes and values
        ***************************************************************************************************/
        virtual void onDedicatedServerAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap) {};

        
        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game's settings change.
                Call game->getGameSettings() for the updated values.

            Note: we don't provide a mechanism (yet) to determine which setting(s) changed.

            \param[in] game the game with updated settings.
        ***************************************************************************************************/
        virtual void onGameSettingUpdated(Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when the game's player capacity changes (as a result of the game being resized).

                The game's capacity has already been updated at the time of dispatch.  Call game->getPlayerCapacity() for the
                updated player capacity.

            \param[in] game the game object containing the updated player capacity.
        ***************************************************************************************************/
        virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when the game's team capacities change (as a result of a teamId being changed.)
            
            A seperate notification will be sent to update the TeamIds of any players affected by this change.

        \param[in] game the game object containing the updated team roster.
        \param[in] teamIndex the team that had its id changed
        \param[in] newTeamId the teamId that was switched
        ***************************************************************************************************/
        virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::TeamId newTeamId) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game's Game Mod Register changes.
                Call game->getGameModRegister() for the updated values.

            \param[in] game the game with updated Game Mod Register.
        ***************************************************************************************************/
        virtual void onGameModRegisterUpdated(Blaze::GameManager::Game *game);


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game's Game Entry Criteria changes.
                Call game->getGameEntryCriteria() for the updated values.

            \param[in] game the game with updated Game EntryCriteria.
        ***************************************************************************************************/
        virtual void onGameEntryCriteriaUpdated(Blaze::GameManager::Game *game);
        

    /*! ****************************************************************************/
    /*! \name Player related methods
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Async notification that a (non-local) player has started joining an existing game.

            Note: you don't get onPlayerJoining notifications for yourself (or other existing players)
            when you create or join a game.  You should iterate over the game roster
            to deal with your own (or other existing) game player(s).
            The joining Player can be identified as a spectator by calling the member method isSpectator().

            \param player The joining player (not fully connected yet).
        ********************************************************************************/
        virtual void onPlayerJoining(Blaze::GameManager::Player *player) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification that a (non-local) player has been placed into an existing game's queue.
            The queuing Player can be identified as a spectator by calling the member method isSpectator().

        \param player The joining player (waiting in the queue).
        ********************************************************************************/
        virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *player) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification that a player has fully connected to the game.
            The joining Player can be identified as a spectator by calling the member method isSpectator().

        \param player The fully connected player
        ********************************************************************************/
        virtual void onPlayerJoinComplete(Blaze::GameManager::Player *player) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification that a game's playerQueue has changed 
                (someone has joined, left, or the order has changed).
                
            \param game  The game object (containing the updated player queue).
        ********************************************************************************/
        virtual void onQueueChanged(Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification that player has joined the game from the queue.  For the local
                player this is called after the players network has been established.  For external players
                this just signifies that you learned they are joining the game.
            The dequeuing Player can be identified as a spectator by calling the member method isSpectator().

        \param[in] player The player who is joining the game from the queue.
        ***************************************************************************************************/ 
        virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *player) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification that a local player has been demoted to the queue from a reserved slot in the game from the queue. 
            The queued Player can be identified as a spectator by calling the member method isSpectator().

        \param[in] player The player who is joining the queue from the game.
        ***************************************************************************************************/ 
        virtual void onPlayerDemotedToQueue(Blaze::GameManager::Player *player);

        /*! ************************************************************************************************/
        /*! \brief Async notification that player has joined the game from claiming a reservation.  For the local
                player this is called after the players network has been established.  For external players
                this just signifies that you learned they are joining the game.
            The joining Player can be identified as a spectator by calling the member method isSpectator().

        \param[in] player The player who is joining the game from claiming a reservation.
        ***************************************************************************************************/ 
        virtual void onPlayerJoinedFromReservation(Blaze::GameManager::Player *player);

        /*! ****************************************************************************/
        /*! \brief Async notification that a player has been removed from the game roster and
                is about to be deleted.  The player's peer network connections will be torn down
                and the player object deleted immediately after this dispatch exits.
                
            Note: a player can be removed before they've fully connected to the game.

            If the local player is removed from the game, we destroy the game after this callback
                (which dispatches GameManagerAPIListener::onGameDestructing)
            The departing Player can be identified as a spectator by calling the member method isSpectator().

            \param game  The game the player has been removed from
            \param removedPlayer The removed player (who's about to be deleted).
            \param playerRemovedReason the reason the player was removed according to blaze
            \param titleContext  a title-specific reason why the player was removed (see kickPlayer and banUser).
        ********************************************************************************/
        virtual void onPlayerRemoved(Blaze::GameManager::Game *game,
                                     const Blaze::GameManager::Player *removedPlayer,
                                     Blaze::GameManager::PlayerRemovedReason playerRemovedReason,
                                     Blaze::GameManager::PlayerRemovedTitleContext titleContext) = 0;

        
        /*! ****************************************************************************/
        /*! \brief Async notification that the admin list for the game has been changed,
             a player is added or removed from the game's admin list;

             Note: Player object sent can possibly be nullptr on remove notifications if the player is lost 
             from the game before the admin change notification occurs. 

            \param game  The game whose admin list has been changed
            \param adminPlayer The player who has been added or removed from the admin list
            \param operation enum to indicate the player is added to or removed from the admin list
            \param updaterId The player id of the player who initiated the admin operation.
        ********************************************************************************/
        virtual void onAdminListChanged(Blaze::GameManager::Game *game,
                                        const Blaze::GameManager::Player *adminPlayer,
                                        Blaze::GameManager::UpdateAdminListOperation  operation,
                                        Blaze::GameManager::PlayerId updaterId) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's attribute map changes.

            \param[in] player the player with updated attributes
            \param[in] changedAttributeMap a map containing the new and updated attributes and values.
        ***************************************************************************************************/
        virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player *player, const Blaze::Collections::AttributeMap *changedAttributeMap) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's teamId changes.

        \param[in] player the player with updated team
        \param[in] previousTeamIndex the player's old team index
        ***************************************************************************************************/
        virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex previousTeamIndex) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's Role changes.

        \param[in] player the player with updated role
        \param[in] previousRoleName the player's old RoleName
        ***************************************************************************************************/
        virtual void onPlayerRoleUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::RoleName previousRoleName);
        
        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's SlotType changes. (Public, Private, Spectator or Not)
             
            This notification either indicates that a user in the game's roster has transitioned from spectator to player 
            or player to spectator, or that the player has switch to/from a public or private slot.

        \param[in] player the player with updated role
        \param[in] previousSlotType the player's old SlotType
        ***************************************************************************************************/
        virtual void onPlayerSlotUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::SlotType previousSlotType);


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's custom data changes.
            Call player->getCustomData for the updated values.

            \param[in] player the player with updated custom data
        ***************************************************************************************************/
        virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player *player) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a player's state changes.

            \param[in] previousPlayerState the player's previous state
        ***************************************************************************************************/
        virtual void onPlayerStateChanged(Blaze::GameManager::Player *player, Blaze::GameManager::PlayerState previousPlayerState);


    /*! ****************************************************************************/
    /*! \name Networking (Host Migration) related methods
    ********************************************************************************/


        /*! ****************************************************************************/
        /*! \brief Function called when host migration starts for a game the user is in.

            \param game The game.
        ********************************************************************************/
        virtual void onHostMigrationStarted(Blaze::GameManager::Game *game) = 0;

        /*! ****************************************************************************/
        /*! \brief Function called when host migration ends for a game the user is in.

            \param game The game.
        ********************************************************************************/
        virtual void onHostMigrationEnded(Blaze::GameManager::Game *game) = 0;

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a network for a game is created. 
                   
            \note  The meaning of this notification varies with the particular NetworkAapter 
                   that is in use. Usually it means that the handle of the network is available.
                   Please consult the documentation of the specific NetworkAapter.

        \param game the game in concern
        ********************************************************************************/
        virtual void onNetworkCreated(Blaze::GameManager::Game *game) = 0;


        /*! ****************************************************************************/
        /*! \brief DEPRECATED - in favor of using onActivePresenceUpdated().
            Async notification dispatched a game gets an NpSessionId, if one was not already available. 
                   
            \note  If the NpSessionId was already available when the Game was created, it can be found with 
                   Game::getNpSessionId();

        \param game the game in concern
        \param npSessionId the npSessionId now available 
        ********************************************************************************/
        virtual void onNpSessionIdAvailable(Blaze::GameManager::Game *game, const char8_t* npSessionId);

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched when a game (i.e. its 1st party session) becomes or is no longer the
            local user's joinable and invite-able activity for the 1st party UX. Titles may use this notification
            to appropriately enable or disable in game options allowing local user to send invites to the game etc.

            \param game The game. On Xbox, Game::getExternalSessionTemplateName() and getExternalSessionName()
                get its session template and name. On PS4, Game::getNpSessionId() returns its NpSessionId, or, if
                its not the user's 1st party UX activity, empty string.

            \param status Indicates whether game is the user's joinable/invite-able activity for the 1st party UX
        ********************************************************************************/
        virtual void onActivePresenceUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::PresenceActivityStatus status, uint32_t userIndex);

        /* TO BE DEPRECATED - will never be notified if you don't use GameManagerAPI::receivedMessageFromEndpoint() */
        virtual void onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize,
            Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when the server is running in Admin managed queues.
            The server has identified that this client is in the admin list, and is notifying it
            that a single slot has opened in the game and there are people in the queue.  
            Note that there could be other open slots in the game as well, if the queue has not
            been processed.
        
            \param[in] game - the game in which a slot has opened.
        ***************************************************************************************************/
        virtual void onProcessGameQueue(Blaze::GameManager::Game *game);

        /*! ****************************************************************************/
        /*! \brief Async notification that a game name is changed.

        \param game The game.
        \param newName The new game name.
        ********************************************************************************/
        virtual void onGameNameChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::GameName newName);

        /*! ****************************************************************************/
        /*! \brief Notifies when a game session voip connection has been made.

        \param game The game where the connection was made.
        \param connectedPlayersVector The game members a connection was made with. Listed players will all be members of the same connection group.
        ********************************************************************************/
        virtual void onVoipConnected(Blaze::GameManager::Game *game, Blaze::GameManager::Game::PlayerVector *connectedPlayersVector);

        /*! ****************************************************************************/
        /*! \brief Notifies when a game session voip connection has been lost or failed.

        \param game The game where the connection was lost.
        \param disconnectedPlayersVector the game member a connection was lost to. Listed players will all be members of the same connection group.
        ********************************************************************************/
        virtual void onVoipConnectionLost(Blaze::GameManager::Game *game, Blaze::GameManager::Game::PlayerVector *disconnectedPlayersVector);

        /*! ************************************************************************************************/
        /*! \brief Async notification that you've been joined to a new game while still a member of the current game
            
            Generally, titles shouldn't need to override this listener, 
            but calling retainGameMembershipThroughResolve() on the passed-in game will signal to GameManager to not remove the local player from the older game.
            Doing that is not supported on all platforms, due to 1st party presence session limitations.
            
            \param[in] game The pre-existing game.
            \param[in] newGame The newly-setup game.
        ***************************************************************************************************/   
        virtual void onResolveGameMembership(Blaze::GameManager::Game *game, Blaze::GameManager::Game *newGame);

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game transits into UNRESPONSIVE state.

            \param[in] game The game that has transited into UNRESPONSIVE state.
            \param[in] previousGameState the state prior to the transit into UNRESPONSIVE.
        ***************************************************************************************************/
        virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState);

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a game transits out of UNRESPONSIVE state
            (and back into its original pre-UNRESPONSIVE state)

            \param[in] game The game that has transited out of UNRESPONSIVE state.
        ***************************************************************************************************/
        virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game);

    /*! ****************************************************************************/
    /*! \name Destructor
    ********************************************************************************/
        virtual ~GameListener() {}
    };

} //namespace GameManager
} //namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_H
