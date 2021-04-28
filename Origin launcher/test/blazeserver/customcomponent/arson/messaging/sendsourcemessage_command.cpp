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
#include "messaging/tdf/messagingtypes.h"
#include "messaging/rpc/messagingslave.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/sendsourcemessage_stub.h"


namespace Blaze
{
namespace Arson
{

    class SendSourceMessageCommand : public SendSourceMessageCommandStub
    {
    public:
        SendSourceMessageCommand(Message* message, Blaze::Messaging::SendSourceMessageRequest* request, ArsonSlaveImpl* componentImpl)
            : SendSourceMessageCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~SendSourceMessageCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SendSourceMessageCommandStub::Errors execute() override
        {
            BlazeRpcError err;

            Messaging::MessagingSlave* messaging = (Messaging::MessagingSlave*) 
                gController->getComponent(Messaging::MessagingSlave::COMPONENT_ID, false, true, &err);
           
            
            err = messaging->sendSourceMessage(mRequest, mResponse);
                       

            return arsonErrorFromMessagingError(err);
        }

        static Errors arsonErrorFromMessagingError(BlazeRpcError error);

    private:
        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free

    };
    SendSourceMessageCommandStub::Errors SendSourceMessageCommand::arsonErrorFromMessagingError(BlazeRpcError error)
    {

        Errors result = ERR_SYSTEM;
        switch (error)
        {
            case ERR_OK: result = ERR_OK; break;
            case ERR_SYSTEM: result = ERR_SYSTEM; break;
            case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
            case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
            case ERR_AUTHENTICATION_REQUIRED: result = ERR_AUTHENTICATION_REQUIRED; break;
            case MESSAGING_ERR_UNKNOWN: result = ARSON_ERR_UNKNOWN;break;
            case MESSAGING_ERR_FEATURE_DISABLED: result = ARSON_ERR_FEATURE_DISABLED; break;
            case MESSAGING_ERR_MAX_ATTR_EXCEEDED: result = ARSON_ERR_MAX_ATTR_EXCEEDED; break;
            case MESSAGING_ERR_DATABASE: result = ARSON_ERR_DATABASE; break;
            case MESSAGING_ERR_TARGET_NOT_FOUND: result = ARSON_ERR_TARGET_NOT_FOUND; break;
            case MESSAGING_ERR_TARGET_TYPE_INVALID: result = ARSON_ERR_TARGET_TYPE_INVALID; break;
            case MESSAGING_ERR_TARGET_INBOX_FULL: result = ARSON_ERR_TARGET_INBOX_FULL; break;
            default:
            {
                //got an error not defined in rpc definition, log it 
                TRACE_LOG("[SendSourceMessageCommand].arsonErrorFromMessagingError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
                result = ERR_SYSTEM;
                break;
            }
        };
        return result;
    }
    // static factory method impl
    DEFINE_SENDSOURCEMESSAGE_CREATE();

} // Arson
} // Blaze
