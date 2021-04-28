/*************************************************************************************************/
/*!
    \file   triggerblazetdfecho_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/oauth/accesstokenutil.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/validateaccesstokenformat_stub.h"

namespace Blaze
{
namespace Arson
{
class ValidateAccessTokenFormatCommand : public ValidateAccessTokenFormatCommandStub
{
public:
    ValidateAccessTokenFormatCommand( Message* message, Blaze::Arson::AccessTokenFormatValidationRequest* request, ArsonSlaveImpl* componentImpl)
        : ValidateAccessTokenFormatCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~ValidateAccessTokenFormatCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    bool isTokenJwt(const char8_t* token)
    {
        //Yes, a JWT contains two dots and first 3 chars are "eyJ" so that is what we use below to verify.
        
        eastl::string accessToken(token);
        size_t count = eastl::count_if(accessToken.begin(), accessToken.end(), [](char c) {bool isDot = (c == '.') ? true : false; return isDot; });

        if (count == 2 && (accessToken.substr(0, 3).compare("eyJ") == 0)) 
            return true;

        return false;
    }

    ValidateAccessTokenFormatCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_OK;

        if (gCurrentUserSession != nullptr)
        {
            INFO_LOG("[ValidateAccessTokenFormatCommand].execute: request (" << *getRequest() << ")");

            Blaze::OAuth::AccessTokenUtil serverTokenUtil;
            serverTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);

            switch (getRequest()->getServerTokenFormat())
            {
            case Blaze::OAuth::TOKEN_FORMAT_JWT:
                if (!isTokenJwt(serverTokenUtil.getAccessToken()))
                {
                    ERR_LOG("[ValidateAccessTokenFormatCommand].execute: server token (" << serverTokenUtil.getAccessToken() << ") is not jwt.");
                    return ERR_SYSTEM;
                }
                break;
            case Blaze::OAuth::TOKEN_FORMAT_OPAQUE:
                if (isTokenJwt(serverTokenUtil.getAccessToken()))
                {
                    ERR_LOG("[ValidateAccessTokenFormatCommand].execute: server token (" << serverTokenUtil.getAccessToken() << ") is jwt and not opaque.");
                    return ERR_SYSTEM;
                }
                break;
            default:
                ERR_LOG("[ValidateAccessTokenFormatCommand].execute: unknown server token format (" << TokenFormatToString(getRequest()->getServerTokenFormat()) << ").");
                return ERR_SYSTEM;
            }

            Blaze::OAuth::AccessTokenUtil userTokenUtil;
            userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);

            switch (getRequest()->getUserTokenFormat())
            {
            case Blaze::OAuth::TOKEN_FORMAT_JWT:
                if (!isTokenJwt(userTokenUtil.getAccessToken()))
                {
                    ERR_LOG("[ValidateAccessTokenFormatCommand].execute: user token (" << userTokenUtil.getAccessToken() << ") is not jwt.");
                    return ERR_SYSTEM;
                }
                break;
            case Blaze::OAuth::TOKEN_FORMAT_OPAQUE:
                if (isTokenJwt(userTokenUtil.getAccessToken()))
                {
                    ERR_LOG("[ValidateAccessTokenFormatCommand].execute: user token (" << userTokenUtil.getAccessToken() << ") is jwt and not opaque.");
                    return ERR_SYSTEM;
                }
                break;
            default:
                ERR_LOG("[ValidateAccessTokenFormatCommand].execute: unknown user token format (" << TokenFormatToString(getRequest()->getUserTokenFormat()) << ").");
                return ERR_SYSTEM;
            }
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_VALIDATEACCESSTOKENFORMAT_CREATE()

} //Arson
} //Blaze
