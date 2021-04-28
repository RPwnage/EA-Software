/*************************************************************************************************/
/*!
    \file   sendmessage_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SendMessageCommand

    Send a message using the Messaging RPC interface.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
// arson includes
#include "arson/tdf/arson.h"
#include "messaging/tdf/messagingtypes.h"
#include "messaging/rpc/messagingslave.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/sendmessage_stub.h"


namespace Blaze
{
namespace Arson
{

    class SendMessageCommand : public SendMessageCommandStub
    {
    public:
        SendMessageCommand(Message* message, SendMessageRequest* request, ArsonSlaveImpl* componentImpl)
            : SendMessageCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~SendMessageCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SendMessageCommandStub::Errors execute() override
        {
            BlazeRpcError err;

            Messaging::MessagingSlave* component = static_cast<Messaging::MessagingSlave*>(gController->getComponent(Messaging::MessagingSlave::COMPONENT_ID));
            if (component == nullptr)
            {
                WARN_LOG("[SendMessageCommandStub].execute() - messaging component is not available");
                return ERR_SYSTEM;
            }

            // Set the gCurrentUserSession to nullptr and set fibre to a superUser in order to test that sendMessage does not require a userSession
            UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
            UserSession::SuperUserPermissionAutoPtr autoPtr(true);

            Messaging::ClientMessage req;
            req.setTargetType(ENTITY_TYPE_USER);
            req.getTargetIds().push_back(mRequest.getBlazeId());
            req.getAttrMap()[1] = mRequest.getMessage();
            
            // Testing only cares about persistent test.  Add to req if needed.
            req.getFlags().setPersistent();
            
            Messaging::SendMessageResponse resp;
            err = component->sendMessage(req, resp);

            return arsonErrorFromMessagingError(err);
        }

        static Errors arsonErrorFromMessagingError(BlazeRpcError error);

    private:
        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free

    };

    // static factory method impl
    DEFINE_SENDMESSAGE_CREATE()

    SendMessageCommandStub::Errors SendMessageCommand::arsonErrorFromMessagingError(BlazeRpcError error)
    {
        Errors result = ERR_SYSTEM;
        switch (error)
        {
            case ERR_OK: result = ERR_OK; break;
            case ERR_SYSTEM: result = ERR_SYSTEM; break;
            case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
            case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
            case ERR_AUTHENTICATION_REQUIRED: result = ERR_AUTHENTICATION_REQUIRED; break;
            default:
            {
                //got an error not defined in rpc definition, log it
                TRACE_LOG("[BanPlayerCommand].arsonErrorFromMessagingError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
                result = ERR_SYSTEM;
                break;
            }
        };

        return result;
    }

} // Arson
} // Blaze
