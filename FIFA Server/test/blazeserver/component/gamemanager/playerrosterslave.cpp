/*! ************************************************************************************************/
/*!
    \file playerrosterslave.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/playerrosterslave.h"
#include "gamemanager/playerinfo.h"

namespace Blaze
{
namespace GameManager
{

    PlayerRosterSlave::PlayerRosterSlave()
    {
    }

    PlayerRosterSlave::~PlayerRosterSlave()
    {
    }

    PlayerRoster::PlayerInfoList PlayerRosterSlave::getPlayers(const PlayerRosterType rosterType) const
    {
        // NOTE: Although this method looks less efficient than it's counterpart in PlayerRoster that takes PlayerInfoList by ref, it isn't!
        // This is because GCC will always perform return value optimization (even in debug mode!) which means the copy ctor never gets invoked, 
        // despite appearances.

        PlayerInfoList players;
        PlayerRoster::getPlayers(players, rosterType);
        return players;
    }

} // namespace GameManager
} // namespace Blaze
