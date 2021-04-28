/*************************************************************************************************/
/*!
    \file   arsongetpktreceivedmetricsbygameidmap_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsongetpktreceivedmetrics_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class ArsonGetPktReceivedMetricsCommand : public ArsonGetPktReceivedMetricsCommandStub
{
public:
    ArsonGetPktReceivedMetricsCommand(
        Message* message,  Blaze::Arson::GetPktReceivedMetricsRequest* request, ArsonSlaveImpl* componentImpl)
        : ArsonGetPktReceivedMetricsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~ArsonGetPktReceivedMetricsCommand() override { }

private:
    ArsonSlaveImpl* mComponent;
    ArsonGetPktReceivedMetricsCommandStub::Errors execute() override
    {
        GameManager::GameManagerMaster* gameManagerComponent = nullptr;
        BlazeRpcError error;
        gameManagerComponent = static_cast<GameManager::GameManagerMaster*>(gController->getComponent(GameManager::GameManagerMaster::COMPONENT_ID, false, true, &error));

        Blaze::GameManager::GetPktReceivedMetricsResponse blazeResponse;
        Blaze::GameManager::GetPktReceivedMetricsRequest blazeRequest;
        blazeRequest.setGameId(mRequest.getGameId());
        BlazeRpcError err = gameManagerComponent->getPktReceivedMetrics(blazeRequest, blazeResponse);
        Blaze::GameManager::PktReceivedMetricsByGameIdMap& blazeMap = blazeResponse.getPktReceivedMetricsByGameIdMap();

        Blaze::GameManager::PktReceivedMetricsByGameIdMap::iterator blazeMapIter = blazeMap.begin();
        Blaze::Arson::PktReceivedMetricsByGameIdMap arsonMap;

        for(;blazeMapIter != blazeMap.end(); ++blazeMapIter)
        {
            
            Blaze::Arson::PktReceivedMetrics* metrics = arsonMap.allocate_element();
            metrics->setClientNoPktReceived(blazeMapIter->second->getClientNoPktReceived());
            metrics->setClientNoResp(blazeMapIter->second->getClientNoResp());
            metrics->setClientPktReceived(blazeMapIter->second->getClientPktReceived());
            metrics->setDedicatedServerHostNoPktReceived(blazeMapIter->second->getDedicatedServerHostNoPktReceived());
            metrics->setDedicatedServerHostNoResp(blazeMapIter->second->getDedicatedServerHostNoResp());
            metrics->setDedicatedServerHostPktReceived(blazeMapIter->second->getDedicatedServerHostPktReceived());
            arsonMap[blazeMapIter->first] = metrics;
        }

        arsonMap.copyInto(mResponse.getPktReceivedMetricsByGameIdMap());
        return arsonErrorFromGameManagerError(err);
    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_ARSONGETPKTRECEIVEDMETRICS_CREATE()

ArsonGetPktReceivedMetricsCommandStub::Errors ArsonGetPktReceivedMetricsCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[ArsonGetPktReceivedMetricsCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
