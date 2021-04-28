/*! ************************************************************************************************/
/*!
\file gamereteruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMERETERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_GAMERETERULEDEFINITION_H

#include "gamemanager/rete/reteruledefinition.h"
#include "gamemanager/rpc/gamemanagerslave.h"

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
}

namespace GameManager
{
namespace Matchmaker
{
    typedef Blaze::GameManager::Rete::WMEProvider<Search::GameSessionSearchSlave> GameReteRuleDefinition;
}
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMERETERULEDEFINITION_H
