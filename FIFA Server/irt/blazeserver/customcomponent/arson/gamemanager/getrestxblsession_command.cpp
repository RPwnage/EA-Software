/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getrestxblsession_stub.h"
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/protocol/restprotocolutil.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because blazeserver xblservicesconfigs.rpc doesn't actually have a 'get' external session method/rpc)
//      (note: also these tdfs cannot be reused for client, as they contain blaze server side only tdfs as members).
class GetRestXblSessionCommand : public GetRestXblSessionCommandStub
{
public:
    GetRestXblSessionCommand(Message* message, GetRestXblSessionRequest* request, ArsonSlaveImpl* componentImpl)
        : GetRestXblSessionCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetRestXblSessionCommand() override {}

private:

    GetRestXblSessionCommandStub::Errors execute() override
    {
        BlazeRpcError error = ERR_SYSTEM;

        HttpConnectionManagerPtr connectionManager = mComponent->getXblMpsdConnectionManagerPtr();
        if (connectionManager != nullptr)
        {
            error = RestProtocolUtil::sendHttpRequest(connectionManager, mCommandInfo.restResourceInfo, 
                &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), &mResponse);

            // do any retries as recommended by MS for potential xbl/network temporary issues
            for (size_t i = 0; (mComponent->isArsonMpsdRetryError(error) && (i <= ArsonSlaveImpl::MAX_EXTERNAL_SESSION_SERVICE_UNAVAILABLE_RETRIES)); ++i)
            {
                TRACE_LOG("[Arson::GetRestXblSessionCommand].execute Microsoft service unavailable. Retry #" << i << " after sleeping " << ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS << "s and retrying.");
                if (ERR_OK != mComponent->waitSeconds(ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS, "Sleeping Arson::GetRestXblSessionCommand"))
                    return ERR_SYSTEM;
                error = RestProtocolUtil::sendHttpRequest(
                    connectionManager, mComponent->getComponentId(), mCommandInfo.commandId,
                    &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), true, &mResponse);
            }
        }
        return (commandErrorFromBlazeError(error));
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETRESTXBLSESSION_CREATE()

} // Arson
} // Blaze
