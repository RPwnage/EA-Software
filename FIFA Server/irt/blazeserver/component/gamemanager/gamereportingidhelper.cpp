/*! ************************************************************************************************/
/*!
    \file gamereportingidhelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/util/random.h"

#include "gamemanager/gamereportingidhelper.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "gamemanager/tdf/gamemanager_server.h"
#ifdef TARGET_gamereporting
#include "gamereporting/rpc/gamereportingslave.h"
#endif

namespace Blaze
{
namespace GameManager
{

/*! ************************************************************************************************/
/*! \brief initialize the helper; to be called in any component's onConfigure() method
***************************************************************************************************/
bool GameReportingIdHelper::init()
{
    if (gUniqueIdManager->reserveIdType(GameManagerSlave::COMPONENT_ID, GAMEMANAGER_IDTYPE_GAME) != ERR_OK)
    {
        ERR_LOG("[GameReportingIdHelper].init: failed reserving game id " << GameManagerSlave::COMPONENT_ID << " with unique id manager");
        return false;
    }
    return true;
}

/*! ************************************************************************************************/
/*! \brief fetch the next GameReportingId. Used for game reports and uniquely identifying game rounds
***************************************************************************************************/
BlazeRpcError GameReportingIdHelper::getNextId(GameReportingId& id)
{
    BlazeError err = gUniqueIdManager->getId(GameManagerSlave::COMPONENT_ID, GAMEMANAGER_IDTYPE_GAME, id);
#ifdef TARGET_gamereporting
    // game reporting is enabled, so id must also be a sliver shard key
    if (err == ERR_OK)
    {
        // Deliberately only get the bare bones component here because all we need is the apis that allow access 
        // to instance load and sliver ownership info which is broadcast by the framework to all instances.
        auto* component = gController->getComponent(GameReporting::GameReportingSlave::COMPONENT_ID, false, false, nullptr);
        if (component != nullptr)
        {
            SliverId sliverId = component->getSliverIdByLoad();
            if (sliverId != INVALID_SLIVER_ID)
            {
                id = BuildSliverKey(sliverId, id);
            }
            else
            {
                ERR_LOG("[GameReportingIdHelper].getNextId: Gamereporting component slivers unavailable.");
                err = ERR_SYSTEM;
            }
        }
        else
        {
            // Always following this code path means that the integrator isn't using game reporting at all; however, GM flows still require a "valid" GR ID.
            // Otherwise, there should be other problem indicators if we occasionally enter here, e.g. failing to connect to grSlave.
            id = BuildSliverKey(INVALID_SLIVER_ID, id);
            TRACE_LOG("[GameReportingIdHelper].getNextId: non-sliver-sharded game reporting id " << id);
            err = ERR_OK;
        }
    }
#endif
    return err;
}

} // namespace GameManager
} // namespace Blaze
