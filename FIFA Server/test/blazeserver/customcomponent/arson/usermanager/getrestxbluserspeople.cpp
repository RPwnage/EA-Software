/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getrestxbluserspeople_stub.h"
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/protocol/restprotocolutil.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because blazeserver doesn't actually have a get xbl users method/rpc)
//      (note: also these tdfs cannot be reused for client, as they contain blaze server side only tdfs as members).
class GetRestXblUsersPeopleCommand : public GetRestXblUsersPeopleCommandStub
{
public:
    GetRestXblUsersPeopleCommand(Message* message, GetRestXblUsersPeopleRequest* request, ArsonSlaveImpl* componentImpl)
        : GetRestXblUsersPeopleCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetRestXblUsersPeopleCommand() override {}

private:

    GetRestXblUsersPeopleCommandStub::Errors execute() override
    {
        HttpConnectionManagerPtr connectionManager = mComponent->getXblSocialConnectionManagerPtr();
        if (connectionManager == nullptr)
        {
            return ERR_SYSTEM;
        }
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(
            connectionManager, mCommandInfo.restResourceInfo, 
            &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), &mResponse);

        // do any retries as recommended by MS for potential xbl/network temporary issues
        for (size_t i = 0; (mComponent->isArsonMpsdRetryError(err) && (i <= ArsonSlaveImpl::MAX_EXTERNAL_SESSION_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            TRACE_LOG("[Arson::GetRestXblUsersPeopleCommand].execute Microsoft service unavailable. Retry #" << i << " after sleeping " << ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS << "s and retrying.");
            if (ERR_OK != mComponent->waitSeconds(ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS, "Sleeping Arson::GetRestXblUsersPeopleCommand"))
                return ERR_SYSTEM;
            err = RestProtocolUtil::sendHttpRequest(
                connectionManager, mComponent->getComponentId(), mCommandInfo.commandId,
                &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), true, &mResponse);
        }

        return commandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETRESTXBLUSERSPEOPLE_CREATE()

} // Arson
} // Blaze
