/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_PLAYER_H
#define GAME_PACKER_PLAYER_H

#include <EASTL/hash_map.h>
#include "gamepacker/common.h"

namespace Packer
{

struct Player
{
    PlayerId mPlayerId;
    PartyIdList mPartyIds; // list of parties the player is a member of

    Player()
    {
        mPlayerId = INVALID_PLAYER_ID;
    }
};


typedef eastl::hash_map<PlayerId, Player> PlayerMap;

}
#endif
