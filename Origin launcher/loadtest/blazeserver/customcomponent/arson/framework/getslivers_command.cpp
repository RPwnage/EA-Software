/*************************************************************************************************/
/*!
    \file   getslivers_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getslivers_stub.h"

namespace Blaze
{
namespace Arson
{
class GetSliversCommand : public GetSliversCommandStub
{
public:
    GetSliversCommand(Message* message, GetSliversRequest* request, ArsonSlaveImpl* componentImpl)
        : GetSliversCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetSliversCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetSliversCommand::Errors execute() override
    {
        SliversSlave* sliversSlaveComponent = nullptr;
        sliversSlaveComponent = static_cast<SliversSlave*>(gController->getComponent(SliversSlave::COMPONENT_ID, false, true));

        if (sliversSlaveComponent == nullptr)
        {
            ERR_LOG("[GetSliversCommand] SliversSlave not found.");
            return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
        }

        Blaze::GetSliversRequest request;
        Blaze::GetSliversResponse response;

        request.setSliverNamespace(mRequest.getSliverNamespace());
        request.setFromInstanceId(mRequest.getFromInstanceId());

        BlazeRpcError err = sliversSlaveComponent->getSlivers(request, response);
        TRACE_LOG("[GetSliversCommand] returned error code '" << ErrorHelp::getErrorName(err) << "'.");

        if(err == Blaze::ERR_OK)
        {
            response.getSliverIdList().copyInto(mResponse.getSliverIdList());
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_GETSLIVERS_CREATE()


} //Arson
} //Blaze
