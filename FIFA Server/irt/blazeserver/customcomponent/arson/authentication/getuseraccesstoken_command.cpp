
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "framework/rpc/oauthslave.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuseraccesstoken_stub.h"

namespace Blaze
{
namespace Arson
{
class GetUserAccessTokenCommand : public GetUserAccessTokenCommandStub
{
public:
    GetUserAccessTokenCommand(Message* message, Blaze::OAuth::GetUserAccessTokenRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserAccessTokenCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~GetUserAccessTokenCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserAccessTokenCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::OAuth::OAuthSlave* component = (Blaze::OAuth::OAuthSlave*)gController->getComponent(
            Blaze::OAuth::OAuthSlave::COMPONENT_ID);
        
        if(nullptr != component)
        {
            err = component->getUserAccessToken(mRequest, mResponse);
            BLAZE_INFO_LOG(Log::USER, "Retrieved Access Token " << mResponse.getAccessToken());
        }
        
        return arsonErrorFromAuthenticationError(err);
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GETUSERACCESSTOKEN_CREATE()

GetUserAccessTokenCommandStub::Errors GetUserAccessTokenCommand::arsonErrorFromAuthenticationError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case OAUTH_ERR_INVALID_USER: result = ARSON_ERR_INVALID_USER; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[GetUserAccessTokenViaArsonCommand].arsonErrorFromAuthenticationError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
