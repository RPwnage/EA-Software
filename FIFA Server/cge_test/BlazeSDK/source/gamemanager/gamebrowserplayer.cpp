/*! ************************************************************************************************/
/*!
    \file gamebrowserplayer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/gamebrowserplayer.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief init the GameBrowserPlayer from the supplied gameBrowserPlayerData and the owning game manager.
    
        \param[in] gameManagerApi         the GM Api is used to access the userManager.
        \param[in] gameBrowserPlayerData  the gameBrowserPlayerData to init from
        \param[in] memGroupId             mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBrowserPlayer::GameBrowserPlayer(GameManagerAPI *gameManagerApi, const GameBrowserPlayerData *gameBrowserPlayerData, MemoryGroupId memGroupId)
        : PlayerBase(gameManagerApi, gameBrowserPlayerData, memGroupId),
          mIsQueued( gameBrowserPlayerData->getPlayerState() == QUEUED )
    {
    }

    /*! ************************************************************************************************/
    /*! \brief init the GameBrowserPlayer from the supplied replicatedPlayerData and the owning game manager.
    
        \param[in] gameManagerApi        the GM Api is used to access the userManager.
        \param[in] replicatedPlayerData  the data to init from
        \param[in] memGroupId            mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBrowserPlayer::GameBrowserPlayer(GameManagerAPI *gameManagerApi, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId)
        : PlayerBase(gameManagerApi, replicatedPlayerData, memGroupId),
          mIsQueued( replicatedPlayerData->getPlayerState() == QUEUED )
    {
    }


    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameBrowserPlayer::~GameBrowserPlayer()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief update the player's custom data blob & player attribute map
        \param[in] replicatedPlayerData the update player data
    ***************************************************************************************************/
    void GameBrowserPlayer::updatePlayerData(const ReplicatedGamePlayer *replicatedPlayerData)
    {
        BlazeAssert(mId == replicatedPlayerData->getPlayerId());
        replicatedPlayerData->getPlayerAttribs().copyInto(mPlayerAttributeMap);
        replicatedPlayerData->getCustomData().copyInto(mCustomData);
        mPlayerState = replicatedPlayerData->getPlayerState();
        mTeamIndex = replicatedPlayerData->getTeamIndex();
        mRoleName = replicatedPlayerData->getRoleName();
        mSlotType = replicatedPlayerData->getSlotType();
        mIsQueued = replicatedPlayerData->getPlayerState() == QUEUED;
    }

    /*! ************************************************************************************************/
    /*! \brief update the player's player attribute map from the supplied gameBrowserPlayerData
        \param[in] gameBrowserPlayerData the update player data
    ***************************************************************************************************/
    void GameBrowserPlayer::updatePlayerData(const GameBrowserPlayerData *gameBrowserPlayerData)
    {
        BlazeAssert(mId == gameBrowserPlayerData->getPlayerId());
        gameBrowserPlayerData->getPlayerAttribs().copyInto(mPlayerAttributeMap);
        mPlayerState = gameBrowserPlayerData->getPlayerState();
        mTeamIndex = gameBrowserPlayerData->getTeamIndex();
        mRoleName = gameBrowserPlayerData->getRoleName();
        mSlotType = gameBrowserPlayerData->getSlotType();
        mIsQueued = gameBrowserPlayerData->getPlayerState() == QUEUED;
    }

} // namespace GameManager
} // namespace Blaze

