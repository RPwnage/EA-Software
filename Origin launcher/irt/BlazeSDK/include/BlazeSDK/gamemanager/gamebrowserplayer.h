/*! ************************************************************************************************/
/*!
    \file gamebrowserplayer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEBROWSER_PLAYER_H
#define BLAZE_GAMEBROWSER_PLAYER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanager/tdf/gamebrowser.h"
#include "BlazeSDK/gamemanager/playerbase.h"
#include "BlazeSDK/memorypool.h"

namespace Blaze
{


namespace GameManager
{
    class GameBrowserGame;
    class GameBrowserList;

    /*! ************************************************************************************************/
    /*! \brief representation of a Player in a GameBrowserGame.  Players are created and destroyed automatically
            as the blazeServer updates the GameBrowserList that owns the game.
    ***************************************************************************************************/
    class BLAZESDK_API GameBrowserPlayer : public PlayerBase
    {
        //! \cond INTERNAL_DOCS
        friend class GameBrowserGame;
        friend class GameBrowserList;
        friend class MemPool<GameBrowserPlayer>;

        NON_COPYABLE(GameBrowserPlayer);
        //! \endcond

    public:
        virtual bool isQueued() const { return mIsQueued; }
        void setIsQueued(bool queued) { mIsQueued = queued; }

    private:

        /*! ************************************************************************************************/
        /*! \brief init the GameBrowserPlayer from the supplied gameBrowserPlayerData and the owning game manager.
        
            \param[in] gameManagerApi        the GM Api is used to access the userManager.
            \param[in] gameBrowserPlayerData the gameBrowserPlayerData to init from
            \param[in] memGroupId                    mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        GameBrowserPlayer(GameManagerAPI *gameManagerApi, const GameBrowserPlayerData *gameBrowserPlayerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        /*! ************************************************************************************************/
        /*! \brief init the GameBrowserPlayer from the supplied replicatedPlayerData and the owning game manager.
        
            \param[in] gameManagerApi       the GM Api is used to access the userManager.
            \param[in] replicatedPlayerData the data to init from
            \param[in] memGroupId                   mem group to be used by this class to allocate memory
        ***************************************************************************************************/
        GameBrowserPlayer(GameManagerAPI *gameManagerApi, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        //! \brief destructor
        ~GameBrowserPlayer();

        /*! ************************************************************************************************/
        /*! \brief update the player's custom data blob & player attribute map
            \param[in] replicatedPlayerData the update player data
        ***************************************************************************************************/
        void updatePlayerData(const ReplicatedGamePlayer *replicatedPlayerData);

        /*! ************************************************************************************************/
        /*! \brief update the player's player attribute map from the supplied gameBrowserPlayerData
            \param[in] gameBrowserPlayerData the update player data
        ***************************************************************************************************/
        void updatePlayerData(const GameBrowserPlayerData *gameBrowserPlayerData);

        bool mIsQueued;
    };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEBROWSER_PLAYER_H
