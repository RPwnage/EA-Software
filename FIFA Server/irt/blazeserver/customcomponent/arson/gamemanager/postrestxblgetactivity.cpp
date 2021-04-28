/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/postrestxblgetactivity_stub.h"
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/protocol/restprotocolutil.h"

namespace Blaze
{
namespace Arson
{

//QA_TODO: THIS COMMAND IS DEPRECATED - SHOULD REMOVE. GetXblActivityForUserCommand now calls ExternalSessionUtilXboxOne::getPrimaryExternalSession() directly.

//      (side: we have this in ARSON, because blazeserver doesn't actually have a get xbl users method/rpc)
//      (note: also these tdfs cannot be reused for client, as they contain blaze server side only tdfs as members).
class PostRestXblGetActivityCommand : public PostRestXblGetActivityCommandStub
{
public:
    PostRestXblGetActivityCommand(Message* message, PostRestXblGetActivityRequest* request, ArsonSlaveImpl* componentImpl)
        : PostRestXblGetActivityCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~PostRestXblGetActivityCommand() override {}

private:

    PostRestXblGetActivityCommandStub::Errors execute() override
    {
        HttpConnectionManagerPtr connectionManager = mComponent->getXblHandlesConnectionManagerPtr();
        if (connectionManager == nullptr)
        {
            return ERR_SYSTEM;
        }
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(
            connectionManager, mComponent->getComponentId(), mCommandInfo.commandId,
            &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), true, &mResponse);

        // do any retries as recommended by MS for potential xbl/network temporary issues
        for (size_t i = 0; (mComponent->isArsonMpsdRetryError(err) && (i <= ArsonSlaveImpl::MAX_EXTERNAL_SESSION_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            TRACE_LOG("[Arson::PostRestXblGetActivityCommand].execute Microsoft service unavailable. Retry #" << i << " after sleeping " << ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS << "s and retrying.");
            if (ERR_OK != mComponent->waitSeconds(ArsonSlaveImpl::EXTERNAL_SESSION_SERVICE_RETRY_AFTER_SECONDS, "Sleeping Arson::PostRestXblGetActivityCommand"))
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

DEFINE_POSTRESTXBLGETACTIVITY_CREATE()

} // Arson
} // Blaze
