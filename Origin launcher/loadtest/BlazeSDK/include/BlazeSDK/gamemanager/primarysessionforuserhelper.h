/*! ************************************************************************************************/
/*!
    \file primarysessionforuserhelper.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PRIMARYSESSION_FOR_USER_HELPER_H
#define BLAZE_GAMEMANAGER_PRIMARYSESSION_FOR_USER_HELPER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/player.h"

namespace Blaze
{

namespace GameManager
{

    // misc helpers for working with primary sessions for users

    inline void populateGameActivity(GameActivity& activity, const Game& game, const Player* player, const char8_t* playerActivityHandleId)
    {
        activity.setGameId(game.getId());
        activity.setGameType(game.getGameType());
        activity.setPresenceMode(game.getPresenceMode());
        game.getExternalSessionIdentification().copyInto(activity.getSessionIdentification());
        activity.getPlayer().setPlayerState(player->getPlayerState());
        activity.getPlayer().setJoinedGameTimestamp(player->getJoinedGameTimestamp());
        activity.getPlayer().setTeamIndex(player->getTeamIndex());
        activity.getPlayer().setSlotType(player->getSlotType());
        
        // also set deprecated external session parameters. For backward compatibility with older servers.
        activity.setPlayerState(player->getPlayerState());
        activity.setJoinedGameTimestamp(player->getJoinedGameTimestamp());
        activity.setPresenceMode(game.getPresenceMode());
        activity.setExternalSessionName(game.getExternalSessionIdentification().getXone().getSessionName());
        activity.setExternalSessionTemplateName(game.getExternalSessionIdentification().getXone().getTemplateName());
    }


} //namespace GameManager
} //namespace Blaze

#endif // BLAZE_GAMEMANAGER_PRIMARYSESSION_FOR_USER_HELPER_H
