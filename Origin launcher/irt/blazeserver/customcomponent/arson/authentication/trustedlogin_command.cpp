/*************************************************************************************************/
/*!
    \file   trustedlogin_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "component/authentication/authenticationimpl.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/trustedlogin_stub.h"

namespace Blaze
{
namespace Arson
{
class TrustedLoginCommand : public TrustedLoginCommandStub
{
public:
    TrustedLoginCommand(Message* message, Blaze::Arson::TrustedLoginRequest* request, ArsonSlaveImpl* componentImpl)
        : TrustedLoginCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~TrustedLoginCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    TrustedLoginCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;
        Blaze::Authentication::TrustedLoginRequest request;
        Blaze::Authentication::LoginResponse response;

        Blaze::Authentication::AuthenticationSlave * component = (Blaze::Authentication::AuthenticationSlave *)gController->getComponent(
            Blaze::Authentication::AuthenticationSlave::COMPONENT_ID);
        
        if(nullptr != component)
        {
            // Populate request
            request.setAccessToken(mRequest.getAccessToken());
            request.setId(mRequest.getId());
            request.setIdType(mRequest.getIdType());

            err = component->trustedLogin(request, response);

            response.copyInto(mResponse);
        }

        return arsonErrorFromAuthenticationError(err);
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);

};

DEFINE_TRUSTEDLOGIN_CREATE()

TrustedLoginCommandStub::Errors TrustedLoginCommand::arsonErrorFromAuthenticationError(BlazeRpcError error)
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
            TRACE_LOG("[TrustedLoginCommand].arsonErrorFromAuthenticationError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
