/*! ************************************************************************************************/
/*!
    \file gamemanagerapi.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_MANAGER_API_H
#define BLAZE_GAMEMANAGER_GAME_MANAGER_API_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/networkmeshadapterlistener.h"

#include "BlazeSDK/censusdata/censusdataapi.h"
#include "BlazeSDK/component/censusdata/tdf/censusdata.h"

#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/gamemanager/game.h"
#include "BlazeSDK/gamemanager/gamebrowserlist.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"
#include "BlazeSDK/blaze_eastl/map.h"
#include "BlazeSDK/blaze_eastl/vector_set.h"

namespace Blaze
{
    class BlazeHub;
    class UserGroup;

namespace UserManager
{
    class UserManager;
}

namespace GameManager
{
    class Player;
    class GameManagerAPIListener;
    class MatchmakingConfigResponse;
    class GameManagerApiJob;
    class PlayerJoinDataHelper;

    extern const uint32_t INVALID_USER_INDEX;

    /*! **************************************************************************************************************/
    /*! \class GameManagerAPI
        \ingroup _mod_apis

        \brief 
        GameManagerAPI Entry point for Game Creation/Destruction/Manipulation, Game Joins, GameBrowsing, and Matchmaking.

        \details

        GameManagerAPI acts as an intermediary between the game session(s) on the blazeServer and the 
        local game objects(s) in the BlazeSDK.  GameManagerAPI uses the low-level GameManager component 
        to communicate with the blazeServer and a NetworkMeshAdapter implementation to establish peer 
        connections to other game clients.

        NOTE: GameManagerAPI does not (explicitly) provide a mechanism to send/broadcast game play data to the game server
        or other peers.  Use the NetworkMeshAdapter for communication to the game server or game peers; the GameManagerAPI
        communicates with the game session on the blaze server.

        One (or more) of your objects should implement the GameManagerAPIListener interface and register itself
        as a listener on the GameManagerAPI (see addListener).  This is how your title receives async notifications
        from the gameManager.

        \section Object Lifetimes
        It's safe to cache off GameManager object pointers and ids, as long as you're registered to listen for the notification
        that destroys/invalidates the object.  It's not safe to persist an object's index over subsequent calls to BlazeHub::idle(),
        however, as the index may shift as objects are removed or inserted into the containers.

        For example, it's safe to cache off the pointer to the MatchmakingScenario returned by the StartMatchmakingScenarioCb.
        The GameManagerAPI will dispatch GameManagerAPIListener::onMatchmakingScenarioFinished before destroying the object.
        

        \section Capabilities

        GameManagerAPI provides these capabilities:
        \li Create / Destroy games
        \li Modify game game attributes and settings (indirectly, via a Game object)
        \li Join Games
        \li Create / Destroy GameBrowser lists
        \li Start / Cancel matchmaking sessions
        \li Access / Iterate over your games, matchmaking sessions, and gameBrowserLists
    ******************************************************************************************************************/
    class BLAZESDK_API GameManagerAPI :
        public SingletonAPI,
        public Blaze::UserGroupProvider,
        public UserManager::PrimaryLocalUserListener,
        public Blaze::ConnectionManagerStateListener,
        private BlazeNetworkAdapter::NetworkMeshAdapterListener

    {
        //! \cond INTERNAL_DOCS
        friend class GameBrowserList; // GameBrowserList needs access to GameManagerAPI::destroyGameBrowserList & dispatch helpers
        friend class Game; // Game needs access to GameManagerAPI::destroyLocalGame & dispatch helpers
        friend class GameManagerApiJob;
        friend class MatchmakingScenario;
        //! \endcond

    public:

        /*! ****************************************************************************/
        /*! \struct GameManagerApiParams
            \brief Parameters struct for GameManagerAPI creation (see GameManagerAPI::createAPI)

            At the minimum, you must supply the NetworkMeshAdapter instance to use for peer connections
            (and the port to connect to).

            \section NetworkMeshAdapter
            \note You are responsible for deleting your network adapter after shutting down the BlazeHub.

            \section GameManagerApi Memory Pools

            You can also (optionally) specify memory pool limits for GameManager's collections of
            Games, GameBrowserLists, GameBrowserGames, and MatchmakingScenarios.

            Leaving a container's limit at zero (the default) lets the blazeSDK grow the container
            as needed.  If you supply a non-zero value, blaze allocates that many nodes up front
            and will overflow if you exceed that limit.  Container's initial reservation of memory and 
            overflow memory are allocated against the GameManagerAPI's MemoryGroupId.

            Note: these "size" limits are the number of objects we preallocate in the container
                (not the size of the object or container in bytes).

            For example, if mMaxMatchmakingScenarios is set to 3, you can have 3 matchmaking Scenarios
            running concurrently.  Creating a 4th Scenario will allocate the extra memory when
            the 4th Scenario is added. (since you have exceeded the limit set at creation in your memory pool).
        ********************************************************************************/
        struct BLAZESDK_API GameManagerApiParams
        {
            //! By default we init the adapter to nullptr. You must set a valid adapter before creating the GameManagerAPI.
            GameManagerApiParams(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP) : 
                        mNetworkAdapter(nullptr),
                        mMaxGameBrowserLists(0),
                        mMaxGameBrowserGamesInList(0),
                        mMaxGameManagerGames(0),
                        mGameProtocolVersionString(nullptr, memGroupId),
                        mPreventMultipleGameMembership(true),
                        mMaxMatchmakingScenarios(0)
            { 
                // Set the GameProtocolVersion to defaulted value. Currently, the default value is "".
                mGameProtocolVersionString.set(GAME_PROTOCOL_DEFAULT_VERSION);
            }

            /*! ****************************************************************************/
            /*! \brief Convenience constructor; initializes the struct's fields
                \param[in] networkAdapter            The NetworkMeshAdapter instance to use in the GameManagerAPI
                \param[in] maxMatchmakingScenarios    The initial size (number of MatchmakingScenario objects) of the matchmaking Scenario memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
                \param[in] maxGameBrowserLists       The initial size (number of GameBrowserList objects) of the game browser list memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
                \param[in] maxGameBrowserGamesInList The initial size (number of GameBrowserGame objects) of the game browser games memory pool. GameBrowserGames are games in a game browser list.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
                \param[in] maxGameManagerGames       The initial size (number of Game objects) of the game manager games memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
                \param[in] gameProtocolVersion       The game protocol version for this instance of the game manager api. Used for release versiond access control to Matchmaking, Game Browsing, and game access(joining, resetting, etc). Must be enabled in GameManager server configuration. Defaults to match any version.
                \param[in] memGroupId                Memory group to be used by this class for memory allocations
                \param[in] preventMultipleGameMembership If true, GameManager will automatically leave any pre-existing local game when a new NotifyGameSetup is delivered from the server. Default of 'false' retains legacy Blaze behavior, which is to allow multiple game membership for a local user, this is not supported on all platforms.
            ********************************************************************************/
            GameManagerApiParams(BlazeNetworkAdapter::NetworkMeshAdapter *networkAdapter,
                           uint32_t maxMatchmakingScenarios = 0,
                           uint32_t maxGameBrowserLists = 0,
                           uint32_t maxGameBrowserGamesInList = 0,
                           uint32_t maxGameManagerGames = 0,
                           const char8_t* gameProtocolVersion = GAME_PROTOCOL_DEFAULT_VERSION, /*the default value is ""*/
                           MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP,
                           bool preventMultipleGameMembership = true
                           );

            BlazeNetworkAdapter::NetworkMeshAdapter *mNetworkAdapter;   //!< the GameNetworkAdapter instance to use for the GameManagerAPI
            uint32_t mMaxGameBrowserLists;                              //!< The initial size (number of GameBrowserList objects) of the game browser list memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
            uint32_t mMaxGameBrowserGamesInList;                        //!< The initial size (number of GameBrowserGame objects) of the game browser games memory pool. GameBrowserGames are games in a game browser list.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
            uint32_t mMaxGameManagerGames;                              //!< The initial size (number of Game objects) of the game manager games memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.
            GameProtocolVersionString mGameProtocolVersionString;       //!< The game protocol version for this instance of the game manager api. Used for release versiond access control to Matchmaking, Game Browsing, and game access(joining, resetting, etc). Must be enabled in GameManager server configuration. Defaults to match any version.
            bool mPreventMultipleGameMembership;                        //!< If false, GameManagerAPI won't automatically leave a previous game when a new game is joined.
            uint32_t mMaxMatchmakingScenarios;                          //!< The initial size (number of MatchmakingScenario objects) of the matchmaking session memory pool.  If the pool is exhausted, overflow memory is allocated from GameManagerAPI's memory group.

            //! \cond INTERNAL_DOCS
            /*! ****************************************************************************/
            /*! \brief prints the parameters to the blaze log.  Called internally by BlazeSDK.
            ********************************************************************************/
            void logParameters() const;
            //! \endcond
        };

    public:

    /*! ****************************************************************************/
    /*! \name API Creation
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Creates the GameManagerAPI (and any blaze components that GameManagerAPI depends on)

            \param hub The hub to create the API on.
            \param params The settings to create the API with
            \param gameManagerAllocator optional allocator to use for gameManager objects; we use the default blaze allocator if omitted
            \param gameBrowserAllocator optional allocator to use for gameBrowser objects; we use the gameManagerAllocator if omitted.
        ********************************************************************************/            
        static void createAPI(BlazeHub &hub, const GameManagerApiParams &params, 
                EA::Allocator::ICoreAllocator* gameManagerAllocator = nullptr, EA::Allocator::ICoreAllocator* gameBrowserAllocator = nullptr);

        /*! ************************************************************************************************/
        /*! \brief Returns the GameManagerApiParams GameManagerAPI was created with.
            \return GameManagerApiParams data.
         ***************************************************************************************************/
        const GameManagerApiParams& getApiParams() const { return mApiParams; }

    /*! ****************************************************************************/
    /*! \name Creating New Games
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon createGame success or failure.

            If the blazeError is ERR_OK, we return a pointer to the locally created Game obj.  On error,
            the game pointer is nullptr.

            Once you've successfully created a game, you should register a GameListener on the 
            Game object to receive game event notifications.

            \param[in] BlazeError the error code for the createGame attempt
            \param[in] JobId the JobId of the create request.
            \param[in] Game* a pointer to the created game (or nullptr on error)
        ***************************************************************************************************/
        typedef Functor3<BlazeError, JobId, Game *> CreateGameCb;

        /*! ****************************************************************************/
        /*! \brief [DEPRECATED] Please update your code to use CreateGameTemplates.
             Create a new game session on the blazeServer.  If the session is not a dedicated server game,
                you (and your userGroup) join the game as part of the creation process.

            For a regular (non dedicated-server) game, you'll join the game as part of the
            game creation process.  If the game's network topology is dedicated server, however,
            you won't be a player in the game (although you will have admin rights over 
            the game in either case).

            If you supply a UserGroup, your group members will join the newly created game.  It's
            not possible to create a dedicated server game as a userGroup.
                              
            Once you've successfully created a game, you should register a GameListener on the 
            Game object (returned in the CreateGameCb) to receive game event notifications.  You'll
            also need to advance the game's state to PRE_GAME before other players are allowed to
            join the game (see Game::initGameComplete).

            \param[in] params The game's initial settings and attributes
            \param[in] callbackFunctor The functor dispatched upon success/failure
            \param[in] userGroup if supplied, your userGroup members will join your newly created game.
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                fields must be set in order to create external player objects in games for the user. See documentation for details.

            \return JobId, can be used to cancel game creation
        ********************************************************************************/
        JobId createGame(const Game::CreateGameParameters &params, const CreateGameCb &callbackFunctor, 
                const UserGroup* userGroup = nullptr,
                const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr);
        JobId createGame(const Game::CreateGameParametersBase &params, const PlayerJoinDataHelper& playerData, const CreateGameCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon joinGame or resetDedicatedServer success or failure.

            Once you've successfully joined a game, you should register a GameListener on the 
            Game object to receive game event notifications.

            If the blazeError is ERR_OK, we return a pointer to the joined Game instance.  On error,
            the game pointer is typically nullptr (with some exceptions). For example, if you failed to 
            join a game because you're already in the game we dispatch joinGameCb(GAMEMANAGER_ERR_ALREADY_GAME_MEMBER, existingGame).
            
            \param[in] BlazeError the error code for the joinGame attempt
            \param[in] JobId the jobId of the join request
            \param[in] Game* a pointer to the joined game (possibly nullptr on error)
            \param[in] EntryCriteria the string representation of the failed entry criteria if join game failed due to the criteria.
        ***************************************************************************************************/
        typedef Functor4<BlazeError, JobId, Game *, const char8_t*> JoinGameCb;

        /*! ************************************************************************************************/
        /*! \brief Attempt to create a game from a template using the supplied parameters.

            Dispatches the supplied callback with the result the create game call.

            Create Game templates can call createGame, createOrJoinGame, or resetGame, depending on the server config.

            \param[in] templateName       The name of the template to use, as defined in the server's config.
            \param[in] templateAttributes The parameters to use for this create game template, also defined by server config.
            \param[in] playerData         Data on the players that will join, including external players, and their roles and attributes.
            \param[in] callbackFunctor    the functor dispatched upon success/failure
            \return jobId the jobId to cancel the create game template with.
        ***************************************************************************************************/
        JobId createGameFromTemplate(const TemplateName& templateName, 
            const TemplateAttributes& templateAttributes,
            const PlayerJoinDataHelper& playerData, 
            const JoinGameCb &callbackFunctor);


    /*! ****************************************************************************/
    /*! \name Joining Existing Games
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Attempt to join a specific game by gameId (using a specific JoinMethod).

            \param[in] gameId The id of the game to join
            \param[in] joinMethod the entry method for to use for the game join.
            \param[in] callbackFunctor the functor dispatched upon success/failure
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                        For group joins, the player attributes are only applied to the group leader;
                        members must set their own attributes after joining the game.
            \param[in] userGroup the user group who would like to join the game together,
                        default nullptr omits. If userGroup and additionalUsersList are nullptr it's a single player join operation.
            \param[in] teamIndex the index of the team the player will attempt to join, 
                        defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                        directly enter, create a reservation, or claim a reservation
                        defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] additionalUsersList - blaze ids of other users who would like to join the game together,
                        default nullptr omits. Not to be used with userGroup
            \param[in] joiningRoles - map of RoleNames to player id list for assigning the joining players roles,
                        if omitted, the joining players will attempt to take the default player role
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return             JobId - for use when canceling join request
        ********************************************************************************/
        JobId joinGameById(GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup *userGroup = nullptr, 
            TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT,
            const Blaze::GameManager::PlayerIdList *additionalUsersList = nullptr, const RoleNameToPlayerMap *joiningRoles = nullptr,
            const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameById(GameId gameId, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor);

        /*! ****************************************************************************/
        /*! \brief Lookup the supplied user's current game and join it with the supplied joinMethod.

            You can get or lookup user objects from the UserManager.  If the user is a member of multiple games,
            we join the first one we find.
            
            Note: You can examine the list of games/game groups a user is in by via user's UserSessionExtendedData::getBlazeObjectIdList().
            Iterate over the list searching for BlazeObjectIds that have type ENTITY_TYPE_GAME/ENTITY_TYPE_GAMEGROUP.

            \param[in] user you can get or lookup users from the UserManager.
            \param[in] joinMethod the entry method for to use for the game join.
            \param[in] callbackFunctor the functor dispatched upon success/failure
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                        For group joins, the player attributes are only applied to the group leader;
                        members must set their own attributes after joining the game.
            \param[in] userGroup the user group who would like to join the game together, 
                        default nullptr omits. If userGroup and additionalUsersList are nullptr it's a single player join operation.

            \param[in] teamIndex the index of the team the player will attempt to join, 
                        defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                        directly enter, create a reservation, or claim a reservation
                        defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] additionalUsersList - blaze ids of other users who would like to join the game together,
                        default nullptr omits. Not to be used with userGroup
            \param[in] joiningRoles - map of RoleNames to player id list for assigning the joining players roles,
                        if omitted, the joining players will attempt to take the default player role
            \param[in] gameType - the game type to join. If the user is a member of multiple instances of the specified game type, join the first one found,
                        if omitted, the join game type will be game session
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return             JobId - for use when canceling join request
        ********************************************************************************/
        JobId joinGameByUser(const UserManager::User *user, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup *userGroup = nullptr, 
            TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT,
            const Blaze::GameManager::PlayerIdList *additionalUsersList = nullptr, const RoleNameToPlayerMap *joiningRoles = nullptr, 
            GameType gameType = GAME_TYPE_GAMESESSION, const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameByUser(const UserManager::User *user, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType = GAME_TYPE_GAMESESSION);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup the given user's current game and join it.

            \param[in] id The user's id.
            \param[in] joinMethod the entry method for to use for the game join.
            \param[in] callbackFunctor the functor dispatched upon success/failure
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                        For group joins, the player attributes are only applied to the group leader;
                        members must set their own attributes after joining the game.
            \param[in] userGroup the user group who would like to join the game together,
                        default nullptr omits. If userGroup and additionalUsersList are nullptr it's a single player join operation.
            \param[in] teamIndex the index of the team the player will attempt to join,
                        defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                        directly enter, create a reservation, or claim a reservation
                        defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] additionalUsersList - blaze ids of other users who would like to join the game together,
                        default nullptr omits. Not to be used with userGroup
            \param[in] joiningRoles - map of RoleNames to player id list for assigning the joining players roles,
                        if omitted, the joining players will attempt to take the default player role
            \param[in] gameType - the game type to join. If the user is a member of multiple instances of the specified game type, join the first one found,
                        if omitted, the join game type will be game session
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return             JobId - for use when canceling join request
        ********************************************************************************/
        JobId joinGameByBlazeId(BlazeId id, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup *userGroup = nullptr, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, 
            GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT, const Blaze::GameManager::PlayerIdList *additionalUsersList = nullptr,
            const RoleNameToPlayerMap *joiningRoles = nullptr, GameType gameType = GAME_TYPE_GAMESESSION,
            const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameByBlazeId(BlazeId id, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType = GAME_TYPE_GAMESESSION);

        /*! ****************************************************************************/
        /*! \brief Attempt to lookup the given user's current game and join it.

            \param[in] username The user's name.
            \param[in] joinMethod the entry method for to use for the game join.
            \param[in] callbackFunctor the functor dispatched upon success/failure
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                        For group joins, the player attributes are only applied to the group leader;
                        members must set their own attributes after joining the game.
            \param[in] userGroup the user group who would like to join the game together,
                        default nullptr omits. If userGroup and additionalUsersList are nullptr it's a single player join operation.
            \param[in] teamIndex the index of the team the player will attempt to join, 
                        defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                        directly enter, create a reservation, or claim a reservation
                        defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] additionalUsersList - blaze ids of other users who would like to join the game together,
                        default nullptr omits. Not to be used with userGroup
            \param[in] joiningRoles - map of RoleNames to player id list for assigning the joining players roles,
                        if omitted, the joining players will attempt to take the default player role
            \param[in] gameType - the game type to join. If the user is a member of multiple instances of the specified game type, join the first one found,
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                fields must be set in order to create external player objects in games for the user. See documentation for details.
           \return             JobId - for use when canceling join request
        ********************************************************************************/
        JobId joinGameByUsername(const char8_t *username, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup *userGroup = nullptr, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, 
            GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT, const Blaze::GameManager::PlayerIdList *additionalUsersList = nullptr,
            const RoleNameToPlayerMap *joiningRoles = nullptr, GameType gameType = GAME_TYPE_GAMESESSION,
            const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameByUsername(const char8_t *username, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType = GAME_TYPE_GAMESESSION);

        /*! ****************************************************************************/
        /*! \brief Attempt to join a game by GameId, attempting to join a specific slot. 
                    For internal dev use only.(using a specific JoinMethod).

            \param[in] gameId The id of the game to join
            \param[in] joinMethod the entry method for to use for the game join.
            \param[in] callbackFunctor the functor dispatched upon success/failure
            \param[in] slotId the slot to attempt to fill. Join will fail if this slot is occupied.
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                        For group joins, the player attributes are only applied to the group leader;
                        members must set their own attributes after joining the game.
            \param[in] userGroup the user group who would like to join the game together,
                        default nullptr omits. If userGroup and additionalUsersList are nullptr it's a single player join operation.
            \param[in] teamIndex the index of the team the player will attempt to join,
                        defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                        directly enter, create a reservation, or claim a reservation
                        defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] additionalUsersList - blaze ids of other users who would like to join the game together,
                        default nullptr omits. Not to be used with userGroup
            \param[in] joiningRoles - map of RoleNames to player id list for assigning the joining players roles,
                        if omitted, the joining players will attempt to take the default player role
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return             JobId - for use when canceling join request
        ********************************************************************************/
        JobId joinGameByIdWithSlotId(GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotId slotId, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup *userGroup = nullptr, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, 
            GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT, const Blaze::GameManager::PlayerIdList *additionalUsersList = nullptr, const RoleNameToPlayerMap *joiningRoles = nullptr, 
            const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameByIdWithSlotId(GameId gameId, JoinMethod joinMethod, SlotId slotId, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief Joins a local user at the give userIndex to the game.  Game must already be local
            and another local user must already be in the game.
        
            \param[in] userIndex - the local user index of the user to bring into the game.
            \param[in] gameId - the id of the game the user is to join.  
            \param[in] callbackFunctor - the method pointer to be called upon success/failure.
            \param[in] slotType the type of slot we request to be put into
            \param[in] playerAttributeMap the player attributes to initialize the joining player with.
                For group joins, the player attributes are only applied to the group leader;
                members must set their own attributes after joining the game.
            \param[in] teamIndex the index of the team the player will attempt to join,
                defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] entryType - the type of entry the player will use to enter the game.  The player can
                directly enter, create a reservation, or claim a reservation
                defaults to GAME_ENTRY_TYPE_DIRECT
            \param[in] roleName - the role the local user should request, defaults to PLAYER_ROLE_NAME_DEFAULT
            \param[in] gameType - the game type to join. If the user is a member of multiple instances of the specified game type, join the first one found,
                        if omitted, the join game type will be game session
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return JobId - for use when canceling the join request.
        ***************************************************************************************************/
        JobId joinGameLocalUser(uint32_t userIndex, GameId gameId, const JoinGameCb &callbackFunctor, SlotType slotType = SLOT_PUBLIC_PARTICIPANT, 
            const Collections::AttributeMap *playerAttributeMap = nullptr, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, 
            GameEntryType entryType = GAME_ENTRY_TYPE_DIRECT, const RoleName roleName = PLAYER_ROLE_NAME_DEFAULT, 
            GameType gameType = GAME_TYPE_GAMESESSION, const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameLocalUser(uint32_t userIndex, GameId gameId, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType = GAME_TYPE_GAMESESSION);

        /*! ************************************************************************************************/
        /*! \brief Makes reservations in your game for the specified list of other users.
                Caller must already be in the game before calling this method. Does not join the caller.

            \param[in] gameId - the id of the game the user is to join.  
            \param[in] callbackFunctor - the method pointer to be called upon success/failure.
            \param[in] reservedPlayerIdentifications objects identifying players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \param[in] slotType the type of slot we request to be put into
            \param[in] teamIndex the index of the team the player will attempt to join,
                defaults to UNSPECIFIED_TEAM_INDEX
            \param[in] gameType - the game type to join. If the user is a member of multiple instances of the specified game type, join the first one found,
                        if omitted, the join game type will be game session
            \param[in] userIdentificationJoiningRoles - map of RoleNames to external user identifications list for assigning joining reserved external players roles,
                        if omitted, the joining reserved external players will attempt to take the default player role
            \return JobId - for use when canceling the join request. Note: canceling will not remove already-joined
                users from the game.
        ***************************************************************************************************/
        JobId joinGameByUserList(GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor,
            const Blaze::UserIdentificationList& reservedPlayerIdentifications,
            SlotType slotType = SLOT_PUBLIC_PARTICIPANT, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX, GameType gameType = GAME_TYPE_GAMESESSION,
            const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles = nullptr);
        JobId joinGameByUserList(GameId gameId, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType = GAME_TYPE_GAMESESSION);

    /*! ****************************************************************************/
    /*! \name Resetting Dedicated Server Games
    ********************************************************************************/


        /*! ****************************************************************************/
        /*! \brief [DEPRECATED] Please update your code to use CreateGameTemplates.
            Find an available dedicated server and reset it using the supplied parameters.  You'll
                join the newly reset game with admin rights.

            If you supply a userGroup, your group members will join the newly reset game as well.

            An "available" dedicated server is a game session with RESETABLE gameState.  Blaze takes your
            network QOS info into account when choosing which dedicated server to reset (we prefer to reset
            a server in your nearest datacenter).

            Note: a dedicated server host should not call this function in an attempt to reset his own game.
            See Game::resetDedicatedServer instead.

            \param[in] params The new settings/attributes to reset the dedicated game server with
            \param[in] callbackFunctor Functor dispatched upon RPC success/failure
            \param[in] playerAttributeMap the set of player attributes you'll enter the game with.
                            Only applied to the player calling reset; group members will have to set their
                            player attribs after they've joined the game.
            \param[in] userGroup if supplied, your userGroup members also join the newly reset game.
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                        if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                        fields must be set in order to create external player objects in games for the user. See documentation for details.
            \return JobId, can be used to cancel game reset / join
        ********************************************************************************/
        JobId resetDedicatedServer(const Game::ResetGameParameters &params, const JoinGameCb &callbackFunctor,
                const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup* userGroup = nullptr,
                const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr);
        JobId resetDedicatedServer(const Game::ResetGameParametersBase &params, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor);

    /*! ****************************************************************************/
    /*! \name Creating New Games or Joining existing Games
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief [DEPRECATED] Please update your code to use CreateGameTemplates.
             Create a new game session or join an existing game on the blazeServer.  
                If the session is not a dedicated server game,
                you (and your userGroup) join the game as part of the creation process.

            For a regular (non dedicated-server) game, you'll join the game as part of the
            game creation process.  If the game's network topology is dedicated server, however,
            you won't be a player in the game (although you will have admin rights over 
            the game in either case).

            If you supply a UserGroup, your group members will join the newly created game.  It's
            not possible to create a dedicated server game as a userGroup.
                              
            Once you've successfully created a game, you should register a GameListener on the 
            Game object (returned in the CreateGameCb) to receive game event notifications.  You'll
            also need to advance the game's state to PRE_GAME before other players are allowed to
            join the game (see Game::initGameComplete).

            \param[in] params The game's initial settings and attributes
            \param[in] callbackFunctor The functor dispatched upon success/failure
            \param[in] playerAttributeMap the set of player attributes you'll enter the game with.
                            Only applied to the player calling reset; group members will have to set their
                            player attribs after they've joined the game.
            \param[in] userGroup if supplied, your userGroup members will join your newly created game.
            \param[in] reservedPlayerIdentifications objects identifying additional optional players to try reserving slots for,
                if there's spare room, after you and your userGroup have joined. Note, depending on the platform, certain
                fields must be set in order to create external player objects in games for the user. See documentation for details.

            \return JobId, can be used to cancel game creation
        ********************************************************************************/
        JobId createOrJoinGame(const Game::CreateGameParameters &params, const JoinGameCb &callbackFunctor, 
                const Collections::AttributeMap *playerAttributeMap = nullptr, const UserGroup* userGroup = nullptr,
                const Blaze::UserIdentificationList *reservedPlayerIdentifications = nullptr);
        JobId createOrJoinGame(const Game::CreateGameParametersBase &params, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor);

    /*! ****************************************************************************/
    /*! \name Accessing local games

        Note: Game pointers and ids are safe to persist over calls to BlazeHub::idle, as
        long as you invalidate games when they are destroyed (GameManagerAPIListener::onGameDestructing)
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Return the local Game obj with the supplied gameId (or nullptr if no game is found locally).

            \param[in] gameId the id of the game to get
            \return       The game, or nullptr if no game is found.
        ********************************************************************************/
        Game *getGameById(GameId gameId) const;

        /*! ****************************************************************************/
        /*! \brief Return the Game at a particular index (or nullptr if the index is out of bounds)

            Note: Game indices will change as games are created and destroyed, so you shouldn't
            persist an index over multiple calls to the BlazeHub idle function.

            \param[in] index The index of the game (between 0 and getGameCount()-1)
            \return       The game, or nullptr if no game is found.
        ********************************************************************************/
        Game *getGameByIndex(uint32_t index) const;

        /*! ****************************************************************************/
        /*! \brief Return the number of local Game objects in the GameManagerAPI
            \return The game count for the local user.
        ********************************************************************************/
        uint32_t getGameCount() const { return (uint32_t) mGameMap.size(); }

    /*! ****************************************************************************/
    /*! \name Matchmaking

        Note: MatchmakingScenario pointers and ids are safe to persist over calls to BlazeHub::idle, as
        long as you invalidate handle when the session/scenario is destroyed (GameManagerAPIListener::onMatchmakingScenarioFinished)
    ********************************************************************************/

        typedef Functor4<BlazeError /*error*/, JobId /*jobId*/, MatchmakingScenario* /*matchmakingScenario*/, const char8_t* /*errDetails*/> StartMatchmakingScenarioCb;
        /*! ************************************************************************************************/
        /*! \brief Attempt to start a matchmaking scenario using the supplied parameters.

            Dispatches the supplied callback with the result the matchmaking scenario.

            Once started successfully, a matchmaking scenario will run until it times out, is canceled,
            or it creates/joins a matched game.  When a scenario finishes, it dispatches
            GameManagerAPIListener::onMatchmakingScenarioFinished to all listeners (see addListener).

            If you supply a userGroupId, your group members will be included in your matchmaking session and
            you'll only join/create games as a group.

            \param[in] scenarioName       The name of the scenario to use, as defined in the server's config.
            \param[in] scenarioAttributes The parameters to use for this matchmaking scenario, also defined by server config.
            \param[in] playerData         Data on the players that will join, including external players, and their roles and attributes.
            \param[in] callbackFunctor    the functor dispatched upon success/failure
            \param[in] addLocalUsersToRequest  if set to false, no local users will be added, and matchmaking will make an indirect matchmaking attempt (requires PERMISSION_START_INDIRECT_MATCHMAKING_SCENARIO)
            \return jobId the jobId to cancel the matchmaking scenario with.
        ***************************************************************************************************/
        JobId startMatchmakingScenario(const ScenarioName& scenarioName, 
            const ScenarioAttributes& scenarioAttributes,
            const PlayerJoinDataHelper& playerData, 
            const StartMatchmakingScenarioCb &callbackFunctor,
            bool addLocalUsersToRequest = true);

        /*! ************************************************************************************************/
        /*! \brief Return the number of matchmaking scenarios this user is running.
            \return the number of matchmaking scenarios the user is running.
        ***************************************************************************************************/
        uint32_t getMatchmakingScenarioCount() const { return (uint32_t) mMatchmakingScenarioList.size(); }

        /*! ************************************************************************************************/
        /*! \brief Return the MatchmakingScenario at the supplied index (or nullptr, if the index is out of bounds).

            Note: MatchmakingScenario indices will change as scenarios are created and destroyed, so you shouldn't
            persist an index over multiple calls to the BlazeHub idle function.

            \param[in] index    The index of the matchmaking scenario to get (between 0 and getMatchmakingScenarioCount()-1).
            \return The matchmaking scenario at the supplied index, or nullptr if the index is out of bounds.
        ***************************************************************************************************/
        MatchmakingScenario* getMatchmakingScenarioByIndex(uint32_t index) const;

        /*! ************************************************************************************************/
        /*! \brief Return a MatchmakingScenario given its id (or nullptr, if no session exists with that id)

            \param[in] matchmakingScenarioId The id of the matchmaking scenario to get.
            \return The matchmaking scenario with the supplied id, or nullptr if no scenario was found.
        ***************************************************************************************************/
        MatchmakingScenario* getMatchmakingScenarioById(MatchmakingScenarioId matchmakingScenarioId) const;

        /*! ************************************************************************************************/
        /*!    \brief A callback functor dispatched upon getMatchmakingConfig success or failure.

            \param[in] error ERR_OK on success
            \param[in] jobId the job id for the RPC call
            \param[in] matchmakingConfig the details of the matchmaking rules defined on the blazeServer;
                    Note: this object is not cached locally and will be deleted when this callback exits.
        ***************************************************************************************************/
        typedef Functor3<BlazeError /*error*/, JobId /*jobId*/, const GetMatchmakingConfigResponse* /*matchmakingConfig*/> GetMatchmakingConfigCb;


        /*! ************************************************************************************************/
        /*! \brief Retrieve the current set of matchmaking rules defined on the blazeServer (in etc/component/gamemanager/matchamker.cfg)

            We don't retrieve the entire rule definition, just the portions that the client can modify.
            Typically used if you want to make your client's matchmaking UI data driven.
            
            \param[in] callbackFunctor    the functor dispatched upon success/failure
            \return JobId for the server request
        ***************************************************************************************************/
        JobId getMatchmakingConfig( const GetMatchmakingConfigCb &callbackFunctor );

    /*! ****************************************************************************/
    /*! \name GameBrowser
    ********************************************************************************/
        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon GetGameBrowserScenarioConfig success or failure.
        
        \param[in] error  ERR_OK on success; otherwise see the errDetails string
        \param[in] jobId  the job id for the GetGameBrowserScenarioConfig request.
        \param[in] gamebrowserScenarioAttributes A pointer to the game browser scenario attributes, null if fetching attributes failed.
        ***************************************************************************************************/
        typedef Functor3<BlazeError /*error*/, JobId /*jobId*/, const GetGameBrowserScenariosAttributesResponse * /*gamebrowserScenarioAttributes*/> GetGameBrowserScenarioConfigCb;

        /*! ************************************************************************************************/
        /*! \brief Attempt to fetch game browser scenario attributes;
        dispatches the supplied callback functor upon success/failure.

        \param[in] titleCb    the functor dispatched upon success/failure
        \return The JobId of the request
        ***************************************************************************************************/
        JobId getGameBrowserAttributesConfig(const GetGameBrowserScenarioConfigCb &titleCb);

        /*! ************************************************************************************************/
        /*! \brief returns the number GameBrowserLists stored on the client.
            \return returns the number of GameBrowserLists stored on the client
        ***************************************************************************************************/
        uint32_t getGameBrowserListCount() const { return (uint32_t)mGameBrowserListByClientIdMap.size(); }

        /*! ************************************************************************************************/
        /*! \brief Return the GameBrowserList at the given index.  Returns nullptr if index is out of bounds.

            \param[in] index The index of the GameBrowserList to retrieve.  Range: [0, getGameBrowserListCount())
            \return the GameBrowserList at index, or nullptr if the index is out of bounds.
        ***************************************************************************************************/
        GameBrowserList* getGameBrowserListByIndex(uint32_t index) const;


        /*! ************************************************************************************************/
        /*! \brief Return the GameBrowserList with the given id.  Returns nullptr if list not found.

            \param[in] listId The unique identifier of a GameBrowserList.
            \return the GameBrowserList with the supplied id, or nullptr if no list is found with the supplied id.
        ***************************************************************************************************/
        GameBrowserList* getGameBrowserListByClientId(GameBrowserListId listId) const;


        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon createGameBrowserList success or failure.

            Note: ERR_OK indicates that a gameBrowserList was created successfully; you'll soon get
            GameManagerListener::onGameBrowserListUpdated messages as the list populates itself with matching games.

            \param[in] error ERR_OK on success; otherwise see the errDetails string
            \param[in] jobId the job id for the create game browser list request.
            \param[in] gameBrowserList A pointer to the newly created GameBrowserList.  nullptr if list creation failed.
            \param[in] errDetails A detailed error message intended for the developer.  Not localized; do not expose to players.
        ***************************************************************************************************/
        typedef Functor4<BlazeError /*error*/, JobId /*JobId*/, GameBrowserList* /*gameBrowserList*/, const char8_t* /*errDetails*/> CreateGameBrowserListCb;
        

        /*! ************************************************************************************************/
        /*! \brief Attempt to create a GameBrowserList using the supplied parameters;
            dispatches the supplied callback functor upon success/failure.

            Once a list is created successfully, it recieves a number of async updates as the list fills with
            matching games (see GameManagerListener::onGameBrowserListUpdated).

            \param[in] parameters         Contains the params needed to create a GameBrowserList: the list's type, capacity, configName, and criteria.
            \param[in] playerJoinData     Holds the roles and teams requested. You do not need to specify a user id for players when making a browser request,
                                          instead you can simply create a dummy player for every role/team you require using PlayerJoinDataHelper.addDummyPlayer().
            \param[in] callbackFunctor    the functor dispatched upon success/failure
            \return The JobId of the request
        ***************************************************************************************************/
        JobId createGameBrowserList(const GameBrowserList::GameBrowserListParametersBase &parameters, const PlayerJoinDataHelper& playerJoinData, 
            const CreateGameBrowserListCb &callbackFunctor);
        JobId createGameBrowserList(const GameBrowserList::GameBrowserListParameters &parameters,
            const CreateGameBrowserListCb &callbackFunctor);
        
        /*! ************************************************************************************************/
        /*! \brief Attempt to start a GameBrowser scenario using the supplied parameters;
        dispatches the supplied callback functor upon success/failure.

        Once a list is created successfully, it recieves a number of async updates as the list fills with
        matching games (see GameManagerListener::onGameBrowserListUpdated).

        \param[in] gameBrowserScenarioName          specifies the game browser scenario to use.
        \param[in] gameBrowserScenarioAttributes    Contains the client send attributes for game browser scenario.
        \param[in] playerData                       Holds the roles and teams requested. You do not need to specify a user id for players when making a browser request,
                                                    instead you can simply create a dummy player for every role/team you require using PlayerJoinDataHelper.addDummyPlayer().
        \param[in] titleCb    the functor dispatched upon success/failure
        \return The JobId of the request
        ***************************************************************************************************/
        JobId createGameBrowserListByScenario(const ScenarioName& gameBrowserScenarioName, const ScenarioAttributes& gameBrowserScenarioAttributes, const PlayerJoinDataHelper& playerData, const CreateGameBrowserListCb &titleCb);

        /*! ************************************************************************************************/
        /*! \brief Attempt to create a GameBrowserList for a UserSet Game list. 
            dispatches the supplied callback functor upon success/failure.

            Once a list is created successfully, it receives a number of async updates as the list fills with
            matching games (see GameManagerListener::onGameBrowserListUpdated).

            \param[in] userSetId the id of the userset in concern
            \param[in] titleCb the functor dispatched upon success/failure
            \return The JobId of the request
        ***************************************************************************************************/
        JobId subscribeUserSetGameList(const UserGroupId& userSetId, const CreateGameBrowserListCb &titleCb);

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon lookupGamesByUser success or failure.

            \param[in] error ERR_OK on success; otherwise see the errDetails string
            \param[in] JobId Identifies the job that called the callback.
            \param[in] gameDataList A pointer to a GameBrowserDataList TDF.  To persist, clone this object.  nullptr if no games or on error.
        ***************************************************************************************************/
        typedef Functor3<BlazeError /*error*/, JobId /*jobId*/, const GameBrowserDataList* /*gameDataList*/> LookupGamesByUserCb;

        /*! ************************************************************************************************/
        /*! \brief Lookup a user's games (games they are a member of) and return a snapshot of the game information.
                The subset of game data returned is filtered using a GameBrowserList configuration.

            \param[in] user - the user to lookup
            \param[in] listConfigName the name of the game browser list configuration; filters the amount of game data returned for each game.
            \param[in] callbackFunctor    the functor dispatched upon success/failure to retrieve the game list.
            \return JobId of task.
        ***************************************************************************************************/
        JobId lookupGamesByUser(const UserManager::User* user, const GameBrowserListConfigName& listConfigName,
                    const LookupGamesByUserCb& callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief Lookup a user's games with reservations made for the user and 
                return a snapshot of the game information.

            \param[in] user - the user to lookup
            \param[in] listConfigName - the name of the game browser list configuration; filters the amount of game data returned for each game. 
                                        This will filter out the games that do not have the requested user in RESERVED player state so 
                                        the listConfig is required to have downloadPlayerRosterType as GAMEBROWSER_ROSTER_ALL.
            \param[in] callbackFunctor - the functor dispatched upon success/failure to retrieve the game list.
            \return JobId of task.
        ***************************************************************************************************/
        JobId lookupReservationsByUser(const UserManager::User* user, const GameBrowserListConfigName& listConfigName,
                    const LookupGamesByUserCb& callbackFunctor);

        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon cancelGameReservation success or failure.

            \param[in] error ERR_OK on success; otherwise see the errDetails string
            \param[in] JobId Identifies the job that called the callback.
        ***************************************************************************************************/
        typedef Functor2 < BlazeError /*error*/, JobId /*jobId*/ > CancelGameReservationCb;

        /*! ************************************************************************************************/
        /*! \brief Cancels a reservation in the given Game, or attempts to leave the Game if the player was joining (internally calls leaveGame).
                   Default permissions only allow the local Users to cancel reservations.  (By default, you can't cancel other player's reservations.)

            \param[in] user - the user to lookup
            \param[in] gameId - the Game to attempt to leave
            \param[in] callbackFunctor - the functor dispatched upon success/failure to cancel the reservation /  leave the game. 
            \return JobId of task.
        ***************************************************************************************************/
        JobId cancelGameReservation(const UserManager::User* user, GameId gameId,
                const CancelGameReservationCb& callbackFunctor);

    /*! ****************************************************************************/
    /*! \name GameManagerAPIListener methods
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive async notifications from the GameManagerAPI.

            Once an object adds itself as a listener, GameManagerAPI will dispatch
            async events to the object via the methods in GameManagerAPIListener.

            \param listener The listener to add.
        ********************************************************************************/
        void addListener(GameManagerAPIListener *listener) { mDispatcher.addDispatchee( listener ); }

        /*! ****************************************************************************/
        /*! \brief Removes a listener from the GameManagerAPI, preventing any further async
                notifications from being dispatched to the listener.

            \param listener The listener to remove.
        ********************************************************************************/
        virtual void removeListener(GameManagerAPIListener *listener) { mDispatcher.removeDispatchee( listener); }


     /*! ****************************************************************************/
    /*! \name ConnectionManagerStateListener methods
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been established.  Implies netconn connection as well
            as first ping response.
        ***************************************************************************************************/
        void onConnected() { mNetworkAdapter->addListener(this); }
        
        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been lost.
            \param error - the error code for why the disconnect happened.
        ***************************************************************************************************/
        void onDisconnected(BlazeError error) { mNetworkAdapter->removeListener(this); }


    /*! ****************************************************************************/
    /*! \name Misc Accessors
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief return the low level GameManager component associated with this API (not typically needed)

            You don't typically need to call low-level RPCs directly.
            \return the GameManager (component) associated with the GameManagerAPI's user
        ***************************************************************************************************/
        GameManagerComponent* getGameManagerComponent() const { return mGameManagerComponent; }

        /*! ************************************************************************************************/
        /*! \brief return the NetworkMeshAdapter the GameManager was registered with.
        
            NOTE: you don't need to interact with the adapter through it's interface; the GameManagerApi
                does that automatically.
        ***************************************************************************************************/
        BlazeNetworkAdapter::NetworkMeshAdapter* getNetworkAdapter() const { return mNetworkAdapter; }

        /*! ************************************************************************************************/
        /*! \brief return the UserManager.
            \return the UserManager (API) associated with the GameManagerAPI
        ***************************************************************************************************/
        UserManager::UserManager* getUserManager() const { return mUserManager; }

        /*! ************************************************************************************************/
        /*! \brief Set the mGameProtocolVersionString in GameManagerApiParams
            NOTE: Don't change it while in/joining a game, during matchmaking, or while you have a game browser list running.
            \param[in] versionString the new game protocol version
        ***************************************************************************************************/
        void setGameProtocolVersionString(const char8_t *versionString);

        /*! ************************************************************************************************/
        /*! \brief Get the mGameProtocolVersionString in GameManagerApiParams
        
        \return mGameProtocolVersionString string.
        ***************************************************************************************************/
        const char8_t* getGameProtocolVersionString() const;

    /*! ****************************************************************************/
    /*! \name GameManager Census Data
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief get the gameManager's current census data.  The data is only updated periodically by the CensusDataAPI;
                you should wait for a dispatch from CensusDataAPIListener::onNotifyCensusData before accessing
                the data.       
            
            You should avoid caching off pointers from inside the object, as they can potentially be invalidated
            during a call to BlazeHub::idle(). Holding a pointer longer than the time in between calls to idle() can
            cause the pointer to be invalidated.

            \return the current GameManagerCensusData, can be nullptr if the TDF is not found.
        ***************************************************************************************************/
        const GameManagerCensusData* getCensusData() const;

        //! \cond INTERNAL_DOCS

        // required impl for user group provider
        Game* getUserGroupById(const EA::TDF::ObjectId &bobjId) const;

        /*! ****************************************************************************/
        /*! \brief Logs any initialization parameters for this API to the log.
        ********************************************************************************/
        void logStartupParameters() const;

        // From BlazeStateEventHandler //

        virtual void onAuthenticated(uint32_t userIndex);

        virtual void onDeAuthenticated(uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief UserManager async notification that the primary local user index has changed.
            \param[in] primaryUserIndex - the new primary user index.
        ***************************************************************************************************/
        void onPrimaryLocalUserChanged(uint32_t primaryUserIndex);

        /*! ************************************************************************************************/
        /*! \brief Returns true if the API is managing at least one game group that does not have the ignored parameter
            \param[in] ignoreGameGroupId - the game id to ignore when looking at the game groups stored by the API.
        ***************************************************************************************************/
        bool hasLocalGameGroups(GameId ignoreGameGroupId) const;

        /*! ************************************************************************************************/
        /*! \brief Returns how many Jobs are currently running on Games, Groups, Scenarios, or Matchmaking Sessions. (Including join/create requests)
        \return The total job count
        ***************************************************************************************************/
        int getGameJobCount() const
        {
            int jobCount = 0;
            for (const auto& jobIter : mUserToGameToJobVector)
                jobCount += static_cast<int>(jobIter->size());

            return jobCount;
        }

        /*! ************************************************************************************************/
        /*! \brief Checks if any users are in any Active sessions, or have active requests pending:
        \return bool - true if a game, group, matchmaking session/scenario, game job, or join/create request are active. 
        ***************************************************************************************************/
        bool areUsersInActiveSession() const
        {
            return (getGameCount() || getGameJobCount() || getMatchmakingScenarioCount());
        }

        //! \endcond

        typedef Blaze::vector_set< const GameBrowserGame* > GameBrowserGameSet;
        typedef Blaze::vector_set< const GameBrowserPlayer* > GameBrowserPlayerSet;
        typedef Blaze::vector_map<BlazeId, GameBrowserGameSet* > GamesOfUsersMap;        

        typedef Blaze::vector_map<GameId, GameBrowserPlayerSet* > UsersOfGamesMap;

        GameId getActivePresenceGameId() const { return mActivePresenceGameId; }

#if defined(ARSON_BLAZE)
        BlazeError getLastPresenceSetupError() const { return mLastPresenceSetupError; }
#endif

    private:

        NON_COPYABLE(GameManagerAPI);

        GameManagerAPI(BlazeHub &blazeHub, const GameManagerApiParams &params, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
        virtual ~GameManagerAPI();

        Game* createLocalGame(const ReplicatedGameData &data, const ReplicatedGamePlayerList &gameRoster, const ReplicatedGamePlayerList& gameQueue, 
            const GameSetupReason& gameSetupReason, const TimeValue& telemetryInterval, const QosSettings& qosSettings, bool performQosOnActivePlayers,
            bool lockableForPreferredJoins, const char8_t* gameAttributeName);
        void destroyLocalGame(Game *game, const GameDestructionReason destructionReason, bool wasLocalPlayerHost, bool wasLocalPlayerActive, bool dispatchGameManagerApiJob = true);

        
        void sendMeshEndpointsConnected(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionFlags connectionFlags, uint32_t latencyMs = 0, float packetLoss = 0.0f) const;
        void sendMeshEndpointsDisconnected(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionStatus connectionStatus, PlayerNetConnectionFlags connectionFlags) const;
        void sendMeshEndpointsConnectionLost(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionFlags connectionFlags) const;
        
        void sendFinalizeGameCreation(GameId gameId, uint32_t userIndex);

        // NetworkMeshAdapterListener impls
        virtual void networkMeshCreated(const Blaze::Mesh *mesh, uint32_t userIndex, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void networkMeshDestroyed( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void connectedToEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void disconnectedFromEndpoint( const Blaze::MeshEndpoint *endpoint, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void connectionToEndpointLost( const Blaze::Mesh *mesh,  Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void connectedToVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void disconnectedFromVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void connectionToVoipEndpointLost( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void migratedTopologyHost( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void migratedPlatformHost( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );

        /* TO BE DEPRECATED - do not use - not MLU ready - receiving game data should be implemented against DirtySDk's NetGameLink api directly */
        virtual void receivedMessageFromEndpoint( const Blaze::MeshEndpoint *endpoint, const void *buf, size_t bufSize, BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error);

        bool setupPlayerJoinData(PlayerJoinDataHelper& playerData, uint32_t userIndex,
            SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, 
            TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/,
            const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, 
            const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/);

        BlazeError prepareCreateGameRequest(const Game::CreateGameParametersBase &params, const PlayerJoinDataHelper& playerData,
            CreateGameRequest& createGameRequest) const;
        BlazeError prepareCreateGameRequest(const Game::CreateGameParameters &params, const UserGroup *userGroup, 
            const Blaze::UserIdentificationList *reservedPlayerIdentifications, CreateGameRequest& createGameRequest) const;

        void prepareCommonGameData(CommonGameRequestData& commonData, GameType gameType) const;
        void prepareTeamsForServerRequest(PlayerJoinData& joiningPlayerData, const TeamIdVector& teamVector, TeamIdVector &requestTeamIdVector) const;
        JobId joinGameInternal(uint32_t userIndex, const UserIdentification* userIdentification, GameId gameId, JoinMethod joinMethod, 
            const PlayerJoinDataHelper& playerData, const JoinGameCb &titleCb, 
            SlotId slotId = DEFAULT_JOINING_SLOT,
            int32_t sessionStart = 0, int32_t sessionLength = 0, bool localUserJoin = false, GameType gameType = GAME_TYPE_GAMESESSION);
        
       typedef Functor4<const GameBrowserDataList *, BlazeError, JobId, const LookupGamesByUserCb&> InternalLookupGamesByUserCb;

        JobId internalLookupGamesByUser(const UserManager::User *user, const GameBrowserListConfigName &listConfigName, const LookupGamesByUserCb &titleCb, bool checkReservation);

        JobId scheduleCreateGameCb(const char8_t* jobName, const CreateGameCb &titleCb, BlazeError error);
        JobId scheduleJoinGameCb(const char8_t* jobName, const JoinGameCb &titleCb, BlazeError error);

        void internalStartMatchmakingScenarioCb( const StartMatchmakingScenarioResponse *startScenarioResponse,
            const MatchmakingCriteriaError *startMatchmakingError, BlazeError errorCode, JobId rpcJobId, JobId jobId);
        void internalStartMatchmakingCb( const StartMatchmakingResponse *startMatchmakingResponse,
            const MatchmakingCriteriaError *startMatchmakingError, BlazeError errorCode, JobId rpcJobId, JobId jobId);
        
        void internalGetMatchmakingConfigCb(const GetMatchmakingConfigResponse *matchmakingConfigResponse, 
            BlazeError errorCode, JobId jobId, GetMatchmakingConfigCb titleCb);

        void internalJoinGameCb(const JoinGameResponse *response, const Blaze::EntryCriteriaError *error, 
                BlazeError errorCode, JobId rpcJobId, JobId jobId);
        void internalJoinGameByUserListCb(const JoinGameResponse *response, const Blaze::EntryCriteriaError *error,
                BlazeError errorCode, JobId rpcJobId, JobId jobId);

        void internalCreateGameCb(const CreateGameResponse *response, BlazeError error, JobId rpcJobId, JobId jobId);
        void internalCreateOrJoinGameCb(const JoinGameResponse *response, BlazeError errorCode, JobId rpcJobId, JobId jobId);
        void internalResetDedicatedServerCb(const JoinGameResponse *response, BlazeError errorCode, JobId rpcJobId, JobId jobId);
        void internalFinalizeGameCreationCb(BlazeError error, JobId rpcJobId, GameId gameId, uint32_t userIndex);
        void internalAdvanceGameStateCb(Blaze::BlazeError blazeError, Game *game);
        void internalEjectHostCb(BlazeError error, JobId rpcJobId, GameId gameId);
        void internalCreateGameBrowserListByScenarioCb(const GetGameListResponse *getGameListResponse,
            const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, JobId jobId);
        void internalCreateGameListCb(const GetGameListResponse *getGameListResponse,
            const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, 
            JobId jobId, GameBrowserList::ListType listType, uint32_t listCapacity);

        void internalLookupGamesByUserCb(const GameBrowserDataList *response, BlazeError errorCode, JobId rpcJobId,  LookupGamesByUserCb titleCb, bool disconnectedReservation, const UserManager::User *user);

        void internalGetUserSetGameListSubscriptionCb(const GetGameListResponse *getGameListResponse,const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, UserGroupId userSetId, JobId jobId);

        GameBrowserList* getGameBrowserListByServerId(GameBrowserListId listId) const;

        void internalGetGameBrowserAttributesConfigCb(const GetGameBrowserScenariosAttributesResponse *gamebrowserattributes, BlazeError errorCode, JobId jobId, GetGameBrowserScenarioConfigCb titleCb);

        void verifyGameBrowserList(GameBrowserListId listId);
        void verifyGameBrowserListCb(BlazeError errorCode, JobId rpcJobId, GameBrowserListId listId);
        void resubscribeGamebrowserListCb(const GetGameListResponse *getGameListResponse, const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, GameBrowserListId listId);

        void destroyGameBrowserList(GameBrowserList *list);

        void resolveGameMembership(Game *newGame, uint32_t userIndex, bool usingSelfOwnedPresence = false);
        void preInitGameNetwork(Game *game, uint32_t userIndex);
        void initGameNetwork(Game *game, uint32_t userIndex);
        void createdGameNetwork(BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error, GameId gameId, uint32_t userIndex);
        void networkAdapterMigrateGame(BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error, GameId gameId);
        void networkAdapterMigrateHost(BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error, GameId gameId, HostMigrationType migrationType);

        void notifyDropWarn(const char8_t* notifyName, uint32_t userIndex, const Game* game, GameId gameId) const;

        // low level async notification handlers
        void onNotifyGameSetup(const NotifyGameSetup *notifyGameSetup, uint32_t userIndex);
        void onNotifyGameReset(const NotifyGameReset  *notification, uint32_t userIndex);
        void onNotifyGameStateChanged(const NotifyGameStateChange *notification, uint32_t userIndex);
        void onNotifyGameAttributeChanged(const NotifyGameAttribChange *notifyGameAttributeChanged, uint32_t userIndex);
        void onNotifyDedicatedServerAttributeChanged(const NotifyDedicatedServerAttribChange *notifyDedicatedServerAttributeChanged, uint32_t userIndex);
        void onNotifyGameSettingChanged (const NotifyGameSettingsChange *notification, uint32_t userIndex);
        void onNotifyGameModRegisterChanged (const NotifyGameModRegisterChanged *notification, uint32_t userIndex);
        void onNotifyGameEntryCriteriaChanged (const NotifyGameEntryCriteriaChanged *notification, uint32_t userIndex);
        void onNotifyGameCapacityChanged(const NotifyGameCapacityChange *notification, uint32_t userIndex);
        void onNotifyGameAdminListChange(const NotifyAdminListChange *notification, uint32_t userIndex);
        void onNotifyPlayerJoining(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex);
        void onNotifyPlayerJoiningQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex);
        void onNotifyPlayerClaimingReservation(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex);
        void onNotifyPlayerPromotedFromQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex);
        void onNotifyPlayerDemotedToQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex);
        void onNotifyJoiningPlayerInitiateConnections(const NotifyGameSetup *notifyGameSetup, uint32_t userIndex);
        void onNotifyPlayerJoinComplete(const NotifyPlayerJoinCompleted *notification, uint32_t userIndex);
        void onNotifyPlayerRemoved(const NotifyPlayerRemoved *notification, uint32_t userIndex);

        void onNotifyPlayerAttributeChanged(const NotifyPlayerAttribChange *notifyPlayerAttributeChanged, uint32_t userIndex);
        void onNotifyPlayerCustomDataChanged(const NotifyPlayerCustomDataChange *notifyPlayerCustomDataChanged, uint32_t userIndex);
        void onNotifyGamePlayerStateChanged(const NotifyGamePlayerStateChange *notification, uint32_t userIndex);
        void onNotifyGamePlayerTeamRoleSlotChange(const NotifyGamePlayerTeamRoleSlotChange *notification, uint32_t userIndex);
        void onNotifyGameTeamIdChanged(const NotifyGameTeamIdChange *notification, uint32_t userIndex);
        void onNotifyMatchmakingFailed(const NotifyMatchmakingFailed *notifyMatchmakingFailed, uint32_t userIndex);
        void onNotifyMatchmakingAsyncStatus(const NotifyMatchmakingAsyncStatus *notifyMatchmakingAsyncStatus, uint32_t userIndex);
        void onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated *notifyMatchmakingConnectionValidated, uint32_t userIndex);
        void onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess *notifyMatchmakingDebugged, uint32_t userIndex);
        void onNotifyMatchmakingScenarioPseudoSuccess(const NotifyMatchmakingScenarioPseudoSuccess *notifyMatchmakingDebugged, uint32_t userIndex);
        void onNotifyGameListUpdate(const NotifyGameListUpdate *notifyGameListUpdate, uint32_t userIndex);
        void onNotifyPlatformHostInitialized(const NotifyPlatformHostInitialized *notification, uint32_t userIndex);
        void onNotifyHostMigrationStart(const NotifyHostMigrationStart *notification, uint32_t userIndex);
        void onNotifyHostMigrationFinished(const NotifyHostMigrationFinished *notification, uint32_t userIndex);
        void onNotifyGameReportingIdChanged(const NotifyGameReportingIdChange *notification, uint32_t userIndex);
        void onNotifyGameRemoved(const NotifyGameRemoved *notification, uint32_t userIndex);
        void onNotifyProcessQueue(const NotifyProcessQueue *notification, uint32_t userIndex);
        void onNotifyGameNameChanged(const NotifyGameNameChange *notification, uint32_t userIndex);
        void onNotifyPresenceModeChanged(const NotifyPresenceModeChanged *notification, uint32_t userIndex);
        void onNotifyQueueChanged(const NotifyQueueChanged *notification, uint32_t userIndex);
        void onNotifyMatchmakingReservedExternalPlayers(const NotifyMatchmakingReservedExternalPlayers *notification, uint32_t userIndex);

        // event handlers for when some other user takes action on your behalf, such as entering mm or joining a game
        void onNotifyRemoteMatchmakingStarted(const NotifyRemoteMatchmakingStarted *notification, uint32_t userIndex);
        void onNotifyRemoteMatchmakingEnded(const NotifyRemoteMatchmakingEnded *notification, uint32_t userIndex);
        void onRemoteJoinFailed(const NotifyRemoteJoinFailed *notification, uint32_t userIndex);

        void onNotifyHostedConnectivityAvailable(const NotifyHostedConnectivityAvailable* notification, uint32_t userIndex);

        void setupNotificationHandlers();
        void clearNotificationHandlers();

        void dispatchNotifyMatchmakingScenarioFinished(MatchmakingScenario &matchmakingScenario, Game *matchedGame);
        void dispatchOnGameBrowserListUpdated(GameBrowserList *gameBrowserList, const GameBrowserList::GameBrowserGameVector &removedGames,
                const GameBrowserList::GameBrowserGameVector &updatedGames) const;
        void dispatchOnGameBrowserListUpdateTimeout(GameBrowserList *gameBrowserList) const;

        void destroyLocalData(uint32_t userIndex);

        // dispatches callback based on game setup reason.
        void dispatchGameSetupCallback(Game &game, BlazeError blazeError, const GameSetupReason& reason, uint32_t userIndex);

        void dispatchOnReservedPlayerIdentifications(Blaze::GameManager::Game *game, const UserIdentificationList &externalIdents, BlazeError blazeError, uint32_t userIndex);

        void linkGameToJob(uint32_t userIndex, GameId gameId, const GameManagerApiJob *job);
        void unlinkGameFromJob(uint32_t userIndex, GameId gameId);
        GameManagerApiJob* getJob(uint32_t userIndex, GameId gameId);

        inline static void setSlotCapacitiesVector(SlotCapacitiesVector &dest, const SlotCapacities &slotArray);        

        BlazeError validateTeamIndex( const TeamIdVector &teamIdVector, const PlayerJoinData &playerJoinData ) const;
        BlazeError validatePlayerJoinDataForCreateOrJoinGame(const PlayerJoinData& playerJoinData) const;
        bool validateProtocolVersion( const Game &game, GameManagerApiJob &jobToCancelIfFail ) const;

#if defined(EA_PLATFORM_PS4)
        /*! ****************************************************************************/
        /*! \brief Deprecated - just compare externalId instead
        ********************************************************************************/
        bool areSonyIdsEqual(const PrimarySonyId& id1, const PrimarySonyId& id2) const;
#endif

        BlazeError validateReservedPlayerArguments(const Blaze::UserIdentificationList *reservedPlayerIdentifications, bool hasUserIdentificationJoiningRoles) const;

        void updateGamePresenceForLocalUser(uint32_t userIndex, const Game& changedGame, UpdateExternalSessionPresenceForUserReason change, GameManagerApiJob* job, PlayerRemovedReason removeReason = SYS_PLAYER_REMOVE_REASON_INVALID);
        void internalUpdateGamePresenceForUserCb(const UpdateExternalSessionPresenceForUserResponse *response, const UpdateExternalSessionPresenceForUserErrorInfo *errInfo, BlazeError error, JobId rpcJobId, JobId jobId, GameId joinCompleteGameId, BlazeId blazeId);
        void dispatchSetupCallbacksOnGamePresenceUpdated(const JobId& jobId, GameId joinCompleteGameId, BlazeId blazeId);
        void onActivePresenceUpdated(GameId presenceGameId, const ExternalSessionIdentification& presenceSession, const char8_t* psnPushContextId, BlazeError error, uint32_t userIndex);
        void onActivePresenceCleared(bool isLocalGameDying, uint32_t userIndex);

        void validateGameSettings(const GameSettings& gameSettings) const;

        bool matchmakingScenarioHasLocalUser(const MatchmakingScenario& mmSession, GameId gameId) const;
        uint32_t getLocalUserIndex(BlazeId blazeId) const;
    private:
        GameManagerApiParams mApiParams;
        GameManagerComponent *mGameManagerComponent;
        UserManager::UserManager *mUserManager;

        typedef Blaze::hash_map<GameId, JobId, eastl::hash<GameId> > GameToJobMap;
        typedef Blaze::vector<GameToJobMap*> UserToGameToJobVector;
        UserToGameToJobVector mUserToGameToJobVector;

        // collection of games
        typedef Blaze::vector_map<GameId, Game *> GameMap;
        GameMap mGameMap;

        MemPool<Game> mGameMemoryPool;
        MemPool<MatchmakingScenario> mMatchmakingScenarioMemoryPool;
        MemPool<GameBrowserList> mGameBrowserMemoryPool;
        MemPool<GameToJobMap> mGameToJobMemoryPool;

        typedef Blaze::vector<MatchmakingScenario*> MatchmakingScenarioList;
        MatchmakingScenarioList mMatchmakingScenarioList;

        // collection of GameBrowserLists
        typedef Blaze::vector_map<GameBrowserListId, GameBrowserList *> GameBrowserListMap;
        GameBrowserListMap mGameBrowserListByClientIdMap;
        GameBrowserListMap mGameBrowserListByServerIdMap;

        GameBrowserListId mNextGameBrowserListId;

        BlazeNetworkAdapter::NetworkMeshAdapter *mNetworkAdapter;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

        typedef Dispatcher<GameManagerAPIListener> GameManagerApiDispatcher;
        GameManagerApiDispatcher mDispatcher;

        GameId mActivePresenceGameId;

#if defined(ARSON_BLAZE)
        BlazeError mLastPresenceSetupError;
#endif
    }; // class GameManagerApi

    /*! ****************************************************************************/
    /*! \class GameManagerAPIListener

        \brief classes that wish to receive async notifications about GameManagerAPI events implement
            this interface and add them self as a listener.

        Listeners must add themselves to each GameManagerAPI that they wish to get notifications about.
        See GameManagerAPI::addListener.
    ********************************************************************************/
    class BLAZESDK_API GameManagerAPIListener
    {
    public:

        /*! ****************************************************************************/
        /*! \brief Async notification that a game has been created by the API.  This is
            only a notice that they have learned about the game, and may still be 
            attempting to establish a network connection to the game. 

            Note that the game could be destroyed before the player connects to the network.

            \param createdGame - The game that was just created
        ********************************************************************************/
        virtual void onGameCreated(Blaze::GameManager::Game *createdGame);

        /*! ****************************************************************************/
        /*! \brief Async notification dispatched just before a game pointer is destroyed; 
            invalidate any cached pointers to the game at this time.

            Note: the destructing game has already been removed from the GameManagerAPI's game map
            by the time this is dispatched.

            \param gameManagerApi The api sending the notification.
            \param dyingGame  The game about to be deleted.
            \param reason The reason the game is being destructed.
        ********************************************************************************/
        virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI *gameManagerApi, 
            Blaze::GameManager::Game *dyingGame, Blaze::GameManager::GameDestructionReason reason) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a matchmaking scenario completes; just before destroying the scenario.

            Note: The matchmakingScenario obj is destroyed after this notification is dispatched; so you must
            invalidate any cached pointers to the matchmaking scenario.

            \param[in] matchmakingResult The result of the matchmaking scenario (the reason the session finished).
            \param[in] matchmakingScenario The scenario that finished; possibly contains fitScores and other info.  NOTE:
            the matchmaking scenario will be destroyed immediately after this callback exits.
            \param[in] game If the matchmaking scenario succeeded, this is the game you were matched into (otherwise nullptr).
        ***************************************************************************************************/
        virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, 
            const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, 
            Blaze::GameManager::Game *game) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification dispatched when a matchmaking v status is sent during matchmaking

            \param[in] matchmakingScenario The scenario which owns the status
            \param[in] matchmakingAsyncStatusList The status of matchmaking scenario on the server 
        ***************************************************************************************************/
        virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
                                                      const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification that a GameBrowserList has been updated.

            Note: The list (and contained games/players) have already been updated by the time this is dispatched.
            (the removed games and players are no longer present in the gameBrowserList.

            You must update any cached pointers to GameBrowserGame of GameBrowserPlayers during this update.
            All games within the removedGames vector will be deleted immediately after this function returns.

            We don't expose an explicit list of GameBrowserPlayers who are about to be deleted.  If you cache of
            GameBrowserPlayer pointers, you need to ensure that the player is still a member of its game (and
            ensure that the game is not in the removedGames list).


            Note: games are removed from list for a few different reasons.
            \li The game ended or was destroyed
            \li The game no longer meets the criteria for this GameBrowserList (no longer matches)
            \li The game's fitScore is not high enough to place is on this list

            \param[in] gameBrowserList the list that has been updated. Note: updates are already applied.
            \param[in] removedGames the games that were removed from the gameBrowserList by this update.
            \param[in] updatedGames the games that were added or updated on the gameBrowserList
        ***************************************************************************************************/
        virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *gameBrowserList,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *removedGames,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *updatedGames) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification that updates to a snapshot GameBrowserList have timed out.

            onGameBrowserListUpdateTimeout will be invoked in place of onGameBrowserListUpdated if a snapshot
            GameBrowserList's maxUpdateInterval elapses and BlazeSDK does not receive an expected update 
            notification from the server.

            \param[in] gameBrowserList the list for which updates have timed out
        ***************************************************************************************************/
        virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *gameBrowserList) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification that a GameBrowserList has been destroyed.

            Note: The list (and contained games/players) will be destroyed after receiving this notification.

            \param[in] gameBrowserList the list that will be destroyed.
        ***************************************************************************************************/
        virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList) = 0;

        /*! ************************************************************************************************/
        /*! \brief Async notification that you've been directed to join a game due to your game group leader
            joining the game.

            If you'd rather not join the game, call game->leaveGame.
            Otherwise you should register yourself as a listener of the game's events, and wait for your
            join to finish.

            \param[in] game The game you're joining.
            \param[in] player The player that is joining the game.
            \param[in] groupId The userGroup that you joined with.
        ***************************************************************************************************/    
        virtual void onUserGroupJoinGame(Blaze::GameManager::Game *game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId) = 0;


        /*! ************************************************************************************************/
        /*! \brief Async notification that you've been directed to become the topology host of a virtualized game.
            Only dedicated server clients will ever get this notification.
         
            You should register yourself as a listener of the game's events, and wait for onTopologyHostInjectionCompleted().
            At this point, the game's setup is incomplete, and the network is not yet set up.
         
            \param[in] game The game you're hosting.
        ***************************************************************************************************/   
        virtual void onTopologyHostInjectionStarted(Blaze::GameManager::Game *game);

        /*! ************************************************************************************************/
        /*! \brief Async notification that you're the topology host of a virtualized game.
            Only dedicated server clients will ever get this notification.
         
            At this point, the game creation is finalized and the game object is ready for use.
         
            \param[in] game The game you're hosting.
        ***************************************************************************************************/   
        virtual void onTopologyHostInjected(Blaze::GameManager::Game *game);

        /*! ************************************************************************************************/
        /*! \brief Async notification that you failed completing indirect join to game which you were pulled
            into by your matchmaking/user group. This maybe due to disconnect, timeout setting up your game
            network connections, or being explicitly removed before you could complete the join.

            \param[in] game The game you failed to join.
            \param[in] userIndex The user index of the player that failed to join
            \param[in] blazeError The reason the player failed to join
        ***************************************************************************************************/ 
        virtual void onIndirectJoinFailed(Blaze::GameManager::Game *game, uint32_t userIndex, Blaze::BlazeError blazeError);
         
        /*! ************************************************************************************************/
        /*! \brief Async notification that you successfully managed to reserve slots in the game for (at
            least some of) the optional reserved external players you originally specified in your create,
            join or reset dedicated server game request.

            Lets the user know which of its original reserved external players got reservations in the
            reservedPlayerIdentifications list.

            \param[in] game The game the players were reserved slots in.
            \param[in] userIndex The user index of the player that called create/join/reset game to make the reservations.
            \param[in] reservedPlayerIdentifications The list of external user info's of the players that got reservations.
        ***************************************************************************************************/ 
        virtual void onReservedPlayerIdentifications(Blaze::GameManager::Game *game, uint32_t userIndex, const Blaze::UserIdentificationList *reservedPlayerIdentifications);

        /*! ************************************************************************************************/
        /*! \brief Async notification that the game has become locked for preferred joins. The lock allows active
            players in the game a first chance at directly joining into open slots their friends/party members etc,
            when a player is removed or the game or its queue's size expanded.
            
            To do this, blaze will block finding or joining a game by matchmaking or game browsing, for up to the blaze
            server configured lock for preferred joins timeout. If all active players have called joinGameByUserList
            or preferredJoinOptOut in response to the lock or the game becomes full, the lock is removed.

            Locking games for preferred joins must be explicitly enabled via blaze server's game session configuration.

            \param[in] game The game that became locked for preferred joins.
            \param[in] playerId  The BlazeId for the removed player (may be INVALID_BLAZE_ID if external player or the log is triggered due to capacity change.)
            \param[in] playerExternalId The ExternalId for the removed player (may be INVALID_EXTERNAL_ID if log is triggered due to capacity change.)
        ***************************************************************************************************/ 
        virtual void onGameLockedForPreferredJoins(Blaze::GameManager::Game *game, Blaze::BlazeId playerId, Blaze::ExternalId playerExternalId);

        /*! ************************************************************************************************/
        /*! \brief Async notification that the local user has been pulled into matchmaking by someone else.
            NOTE: This notification will not be delivered if matchmaking was initiated before the local user logged into Blaze.

            \param[in] matchmakingStartedNotification - notification containing details about the matchmaking session 
                and the initiator of the matchmaking, including BlazeID and UserGroup ID, if any.
            \param[in] userIndex - the user index of the user pulled into matchmaking
        ***************************************************************************************************/ 
        virtual void onRemoteMatchmakingStarted(const Blaze::GameManager::NotifyRemoteMatchmakingStarted *matchmakingStartedNotification, uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief Async notification that the matchmaking session the user was pulled into has ended, and the result
             NOTE: This notification will not be delivered if matchmaking concluded before the local user logged into Blaze.

            \param[in] matchmakingEndedNotification - notification containing details about the matchmaking session
                and the initiator of the matchmaking, including BlazeID and UserGroup ID, if any, as well as the result (timeout, success, canceled, etc).
            \param[in] userIndex - the user index of the user pulled into matchmaking
        ***************************************************************************************************/ 
        virtual void onRemoteMatchmakingEnded(const Blaze::GameManager::NotifyRemoteMatchmakingEnded *matchmakingEndedNotification, uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief Async notification that the local user failed to join a game when the join was initiated by someone else.
            NOTE: This notification will not be delivered if the join was initiated before the local user logged into Blaze.

            \param[in] remoteJoinFailedNotification - notification containing details about the join attempt
                and the initiator of the matchmaking, including BlazeID and UserGroup ID, if any, as well as the game ID and join error
            \param[in] userIndex - the user index of the user pulled into matchmaking
        ***************************************************************************************************/ 
        virtual void onRemoteJoinFailed(const Blaze::GameManager::NotifyRemoteJoinFailed *remoteJoinFailedNotification, uint32_t userIndex);

        /*! ****************************************************************************/
        /*! \name Destructor
        ********************************************************************************/
        virtual ~GameManagerAPIListener() {};

    private:
        void defaultPreferredJoinOptOutTitleCb(Blaze::BlazeError error, Blaze::GameManager::Game *game, JobId id);
    }; // class GameManagerAPIListener

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_MANAGER_API_H
