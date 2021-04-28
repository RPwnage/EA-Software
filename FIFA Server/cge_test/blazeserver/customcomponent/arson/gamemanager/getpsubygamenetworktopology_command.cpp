/*************************************************************************************************/
/*!
    \file   getpsubygamenetworktopology_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getpsubygamenetworktopology_stub.h"

#include "gamemanager/rpc/gamemanagermaster.h"

namespace Blaze
{
namespace Arson
{
class GetPSUByGameNetworkTopologyCommand : public GetPSUByGameNetworkTopologyCommandStub
{
public:
    GetPSUByGameNetworkTopologyCommand(
        Message* message, Blaze::Arson::GetMetricsByGeoIPDataRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPSUByGameNetworkTopologyCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetPSUByGameNetworkTopologyCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPSUByGameNetworkTopologyCommandStub::Errors execute() override
    {
        GameManager::GameManagerMaster* gmMaster = static_cast<GameManager::GameManagerMaster*>(gController->getComponent(GameManager::GameManagerMaster::COMPONENT_ID, false, true));

        if (gmMaster != nullptr)
        {
            Blaze::GetMetricsByGeoIPDataRequest getMetricsByGeoIPDataRequest;
            getMetricsByGeoIPDataRequest.setIncludeISP(mRequest.getIncludeISP());
            Blaze::GetPSUResponse getPSUResponse;

            BlazeRpcError err = gmMaster->getPSUByGameNetworkTopology(getMetricsByGeoIPDataRequest, getPSUResponse);

            getPSUResponse.getPSUMap().copyInto(mResponse.getPSUMap());

            //return back with the resulting rpc error code
            return commandErrorFromBlazeError(err);
        }
        else
        {
            ERR_LOG("[GetPSUByGameNetworkTopologyCommand::execute] GameManagerComponent not found '" << ErrorHelp::getErrorName(ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND) << "'.");
            return ERR_SYSTEM;
        }
    }
};

DEFINE_GETPSUBYGAMENETWORKTOPOLOGY_CREATE()
} //Arson
} //Blaze
