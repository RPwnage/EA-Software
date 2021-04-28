/*! ************************************************************************************************/
/*!
    \file usersessiongameset.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "EASTL/set.h" 
#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
    namespace GameManager
    {
        typedef eastl::set<GameId> UserSessionGameSet; // used in mUserSessionGamesSlaveMap, and Matchmaker::PlayersRule 
    }
}

