/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getplayersessioninvitations_stub.h"
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Arson
{

// HTTP pass through. (side: we have this in ARSON, because blazeserver doesn't actually have a get PSN invitations method/rpc)
// (note: also these tdfs cannot be reused for client, as they contain blaze server side only tdfs as members).
class GetPlayerSessionInvitationsCommand : public GetPlayerSessionInvitationsCommandStub
{
public:
    GetPlayerSessionInvitationsCommand(Message* message, GetPlayerSessionInvitationsRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPlayerSessionInvitationsCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPlayerSessionInvitationsCommand() override {}

private:

    GetPlayerSessionInvitationsCommandStub::Errors execute() override
    {
        if (!gCurrentLocalUserSession)
        {
            ERR_LOG("[GetPlayerSessionInvitationsCommand].execute: Internal test err: gCurrentLocalUserSession null.");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = ERR_OK;
        UserIdentification ident;
        UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), ident);
        ExternalUserAuthInfo authInfo;
        authInfo.setCachedExternalSessionToken(mRequest.getHeader().getAuthToken());
        authInfo.setServiceName(gCurrentLocalUserSession->getServiceName());
        mRequest.setAccountId(eastl::string(eastl::string::CtorSprintf(), "%" PRIi64 "", ident.getPlatformInfo().getExternalIds().getPsnAccountId()).c_str());

        ExternalSessionErrorInfo errInfo;

        //reuse ExternalSessionUtil's helpers
        err = mComponent->callPSNsessionManager(ident, mCommandInfo, authInfo, mRequest.getHeader(), mRequest, &mResponse, &errInfo);
        if (err != ERR_OK)
        {
            // translate back for caller, which uses the PSN type values for consistency
            mErrorResponse.getError().setMessage(errInfo.getMessage());
            mErrorResponse.getError().setCode(errInfo.getCode());
        }

        return commandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETPLAYERSESSIONINVITATIONS_CREATE()

} // Arson
} // Blaze
