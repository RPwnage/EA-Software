/*! ************************************************************************************************/
/*!
    \file playerrosterslave.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYER_ROSTER_SLAVE_H
#define BLAZE_GAMEMANAGER_PLAYER_ROSTER_SLAVE_H

#include "gamemanager/playerroster.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Roster impl - we cache the players into a number of lists (by player state) so we can
            access groups we care about quickly.
    ***************************************************************************************************/
    class PlayerRosterSlave : public PlayerRoster
    {
        NON_COPYABLE(PlayerRosterSlave);

    public:
        PlayerRosterSlave();
        ~PlayerRosterSlave() override;

        PlayerRoster::PlayerInfoList getPlayers(const PlayerRosterType rosterType) const;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_PLAYER_ROSTER_SLAVE_H
