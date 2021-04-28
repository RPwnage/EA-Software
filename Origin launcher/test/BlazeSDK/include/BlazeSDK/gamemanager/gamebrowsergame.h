/*! ************************************************************************************************/
/*!
    \file gamebrowsergame.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEBROWSER_GAME_H
#define BLAZE_GAMEBROWSER_GAME_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanager/tdf/gamebrowser.h"
#include "BlazeSDK/gamemanager/gamebase.h"
#include "BlazeSDK/gamemanager/gamebrowserplayer.h"
#include "BlazeSDK/jobid.h"

namespace Blaze
{

namespace GameManager
{
    class GameBrowserList;

    /*! ************************************************************************************************/
    /*! \brief representation of a game in a GameBrowserList object.  Contains the game's fitScore
    (against the list's matchmaking criteria), as well as game and player information.
    ***************************************************************************************************/
    class BLAZESDK_API GameBrowserGame : public GameBase
    {
        //! \cond INTERNAL_DOCS
        friend class GameBrowserList;
        friend class MemPool<GameBrowserGame>;
        NON_COPYABLE(GameBrowserGame);
        //! \endcond

    public:

        //! \brief an unsorted vector of GameBrowserPlayer pointers
        typedef Blaze::vector<GameBrowserPlayer*> GameBrowserPlayerVector;

        /*! ************************************************************************************************/
        /*! \brief Returns the actual fitScore of the GameBrowserList's criteria to this matched game.
            \return Returns the actual fitScore of the GameBrowserList's criteria to this matched game.
        ***************************************************************************************************/
        FitScore getFitScore() const { return mFitScore; }

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of players in the game (including player reservations).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getPlayerCount() const { return static_cast<uint16_t>( mParticipantCount + mSpectatorCount ); }

        // 'Using' is needed because the method name is being hidden due to the multiple overloads of the method name
        using GameBase::getPlayerCount;
        
        /*! **********************************************************************************************************/
        /*! \brief  returns the number of participants in the game (including reservations).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getParticipantCount() const { return mParticipantCount; }

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of spectators in the game (all spectator slot types, including reservations).
            \return    Returns the number of players in this game.
        **************************************************************************************************************/
        virtual uint16_t getSpectatorCount() const { return mSpectatorCount; }

        /*! **********************************************************************************************************/
        /*! \brief returns the current number of users in the game roster (all slot types, players and spectators, including reservations).
            \return    Returns the number of users in this game.
        **************************************************************************************************************/
        virtual uint16_t getRosterCount() const { return getPlayerCount(); }

        /*! **********************************************************************************************************/
        /*! \brief  returns the number of players in the game's queue
            \return    Returns the number of players in this game's queue.
        **************************************************************************************************************/
        virtual uint16_t getQueueCount() const { return mQueueCount; }

        /*! **********************************************************************************************************/
        /*! \brief  returns the game mode attribute value for this game.
            \return    Returns the game mode attribute value for this game.
        **************************************************************************************************************/
        virtual const char8_t* getGameMode() const { return mGameMode.c_str(); }


        /*! ************************************************************************************************/
        /*! \brief A callback functor dispatched upon downloadAllDataFields success or failure.

            ERR_OK indicates that this gameBrowserGame has been updated with all the data fields for the game.

            \param[in] error ERR_OK on success
            \param[in] gameBrowserList A pointer to the (updated) GameBrowserGame.
        ***************************************************************************************************/
        typedef Functor2<BlazeError /*error*/, GameBrowserGame* /*gameBrowserGame*/> DownloadAllDataFieldsCbFunctor;

        /*! ************************************************************************************************/
        /*! \brief get all game and player data for this game from the blazeServer. This overrides this list's
            configuration (pulls down all data, even data excluded in the config).

            Use this method to download/refresh all of the data for this game (including player roster and
            all player data).

            For example, many games omit the player roster from gameBrowser lists by default (in the list's
            configuration).  If a user double clicks on a game entry, however you can call this method
            to get the game's roster from the server.

            NOTE: any future async update for the game will obey the list's config, meaning that additional
            data can become stale.

            \param[in] callbackFunctor the callback functor dispatched when the RPC returns.
            \return a jobId, allowing you to cancel the RPC
        ***************************************************************************************************/
        JobId downloadAllDataFields(const DownloadAllDataFieldsCbFunctor &callbackFunctor);

        /*! **********************************************************************************************************/
        /*! \brief  Returns the GameBrowserPlayer object for the supplied BlazeId (or nullptr, if the no player exists for that blazeId)
            \param[in]    blazeId    The BlazeId (playerId) to get the player for.

            Note: dedicated server games are hosted by a blaze user (with a BlazeId), but that user is not
            a player in the game (there's no Player object for the dedicated server host).

            \return    Returns the GameBrowserPlayer representing the supplied user (or nullptr, if the blazeId isn't a game member).
        **************************************************************************************************************/
        GameBrowserPlayer* getPlayerById(BlazeId blazeId) const;

        /*! ************************************************************************************************/
        /*! \brief Return the unsorted set of Players & Spectators in this game.  NOTE: will be empty if this list's
            configuration doesn't download the player roster.

            We expose the vector of game players for each GameBrowserGame, allowing titles to sort the
            players when and how they see fit (typically using one of the eastl sort algorithms in EASTL\sort.h)

            We provide a default sort comparison struct to sort by playerName (ascending).
            See DefaultPlayerSortCompare.

            Note: you must use a sort comparison struct when sorting the player vector, otherwise you'll be
            sorting players based off of their location in memory (sorting by pointer value).

            \return Return the unsorted set of players in this game.
        ***************************************************************************************************/
        GameBrowserPlayerVector& getPlayerVector() { return mPlayerVector; }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Player object for the game's host player.  Returns nullptr if this game doesn't have a host
            player (a dedicated server game), or if this gameBrowser list didn't download the player roster.
            \return the game's host player (or nullptr if there is no host player, or the roster wasn't downloaded for this game)
        **************************************************************************************************************/
        GameBrowserPlayer* getTopologyHostPlayer() const { return getPlayerById(mTopologyHostId); }
        GameBrowserPlayer* getHostPlayer() const { return getPlayerById(mTopologyHostId); }

        /*! ************************************************************************************************/
        /*! \brief We provide a default sort comparison struct for the GameBrowserPlayerVector
            ( sort ascending by player name using stricmp ).

            See EASTL/sort.h, and the eastl FAQ for more info on sorting(in packages/eastl/version/doc)
        ***************************************************************************************************/
        struct BLAZESDK_API DefaultPlayerSortCompare
        {

            /*! ************************************************************************************************/
            /*! \brief An eastl Compare struct; operator() return true if itemA should come before itemB.
                We sort by: player's name (ascending, case-insensitive), then playerId.

                \param[in] a the first item to compare
                \param[in] b the second item to compare
                \return true if a should come before b
            ***************************************************************************************************/
            bool operator()(const GameBrowserPlayer *a, const GameBrowserPlayer *b) const;
        };

    private:

        /*! ************************************************************************************************/
        /*! \brief construct and update a GameBrowserGame from the supplied data.

            \param[in] gameBrowserList      the owning list
            \param[in] gameBrowserGameData  the gameBrowserGameData to init the obj from
            \param[in] memGroupId           mem group to be used by this class to allocate memory 
            ***************************************************************************************************/
            GameBrowserGame(GameBrowserList *gameBrowserList, const GameBrowserMatchData *gameBrowserMatchData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        //! \brief destructor
        ~GameBrowserGame();

        /*! ************************************************************************************************/
        /*! \brief update this GameBrowserGame with new GameBrowserGameData (game and roster info).
            Note: this can invalidate GameBrowserPlayer pointers.

            \param[in] gameBrowserGameData the data to init this game from
        ***************************************************************************************************/
        void update(const GameBrowserMatchData *gameBrowserMatchData);

        /*! ************************************************************************************************/
        /*! \brief update the game's player roster (and the cached game size var). 
            Note: removed players are queued for deletion, not deleted outright.

            \param[in] gameBrowserGameData the gameBrowserGameData to init the player roster from
        ***************************************************************************************************/
        void updatePlayerRoster(const GameBrowserGameData& gameBrowserGameData);

        /*! ************************************************************************************************/
        /*! \brief update the game's player roster (and the cached game size var). 
            Note: removed players are queued for deletion, not deleted outright.

            \param[in] replicatedPlayerList the full player data to init the player roster from
        ***************************************************************************************************/
        void updatePlayerRoster(const ReplicatedGamePlayerList *replicatedPlayerList);

        /*! ************************************************************************************************/
        /*! \brief template function to update the player roster from either a GameBroserGameData::GameRosterList
            or a ReplicatedGamePlayerList

            \param[in] playerDataList either a GameBroserGameData::GameRosterList or a ReplicatedGamePlayerList
            \param[in] totalPlayerCount the total number of players in the game
            \param[in] totalSpectatorCount the total number of spectators in the game
        ***************************************************************************************************/
        template<class PlayerDataList>
        void updatePlayerRoster(const PlayerDataList *playerDataList, uint16_t totalPlayerCount, uint16_t totalSpectatorCount);

        /*! ************************************************************************************************/
        /*! \brief clear the player roster, deleting all GameBrowserPlayers associated with this game.
        ***************************************************************************************************/
        void clearPlayerRoster();

        /*! ************************************************************************************************/
        /*! \brief internal callback for GetFullGameData RPC (see downloadAllDataFields)
        ***************************************************************************************************/
        void internalDownloadFullGameDataCb(const GetFullGameDataResponse *response, BlazeError error, JobId id, DownloadAllDataFieldsCbFunctor titleCb);

        /*! **********************************************************************************************************/
        /*! \brief Returns the game's network address list, used to initalize the game
        **************************************************************************************************************/
        virtual NetworkAddressList* getNetworkAddressList() { return &mNetworkAddressList; }


        private:
            FitScore mFitScore;
            uint16_t mParticipantCount; // Note: we can't just use the player container size, since we might not download the roster...
            uint16_t mSpectatorCount;
            uint16_t mQueueCount;
            GameBrowserPlayerVector mPlayerVector;
            GameBrowserList *mGameBrowserList;
            MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
            Collections::AttributeValue mGameMode; // Direct access to the mode attribute
            NetworkAddressList mNetworkAddressList;
        };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEBROWSER_GAME_H
