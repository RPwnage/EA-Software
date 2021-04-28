/*************************************************************************************************/
/*!
    \file   getuserxbltoken_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuserxbltoken_stub.h"

// authentication includes
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth.h"

namespace Blaze
{
namespace Arson
{
class GetUserXblTokenCommand : public GetUserXblTokenCommandStub
{
public:
    GetUserXblTokenCommand( Message* message, Blaze::OAuth::GetUserXblTokenRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserXblTokenCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetUserXblTokenCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserXblTokenCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::OAuth::OAuthSlave *oAuthComponent = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
            Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);

        if (oAuthComponent == nullptr)
        {
            ERR_LOG("[GetUserXblTokenCommandStub].execute: AuthenticationSlave was unavailable, XBL token lookup failed.");
            return ERR_SYSTEM;
        }

        err = oAuthComponent->getUserXblToken(mRequest, mResponse);
        if (err != ERR_OK)
        {
            switch (mRequest.getRetrieveUsing().getActiveMember())
            {
                case Blaze::OAuth::XblTokenOwnerId::MEMBER_UNSET:
                    ERR_LOG("[GetUserXblTokenCommandStub].execute: Failed to retrieve xbl auth token, no external/persona id set, with error '" << ErrorHelp::getErrorName(err) <<"'.");
                    break;
                case Blaze::OAuth::XblTokenOwnerId::MEMBER_PERSONAID:
                    ERR_LOG("[GetUserXblTokenCommandStub].execute: Failed to retrieve xbl auth token for persona id: " << mRequest.getRetrieveUsing().getPersonaId() << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
                    break;
                case Blaze::OAuth::XblTokenOwnerId::MEMBER_EXTERNALID:
                    ERR_LOG("[GetUserXblTokenCommandStub].execute: Failed to retrieve xbl auth token for external id: " << mRequest.getRetrieveUsing().getExternalId() << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
                    break;
            }
            return arsonErrorFromAuthenticationError(err);
        }

        return ERR_OK;
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GETUSERXBLTOKEN_CREATE()

GetUserXblTokenCommandStub::Errors GetUserXblTokenCommand::arsonErrorFromAuthenticationError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case OAUTH_ERR_INVALID_USER: result = ARSON_ERR_INVALID_USER; break;
        case OAUTH_ERR_NO_XBLTOKEN: result = ARSON_ERR_NO_XBLTOKEN; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[GetUserXblTokenCommand].arsonErrorFromAuthenticationError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
