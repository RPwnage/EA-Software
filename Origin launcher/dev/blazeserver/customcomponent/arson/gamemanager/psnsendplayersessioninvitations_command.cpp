/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/sendplayersessioninvitations_stub.h"
#include "arsonslaveimpl.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Arson
{

// HTTP pass through. (side: we have this in ARSON, because blazeserver doesn't actually have a get PSN users method/rpc)
// (note: also these tdfs cannot be reused for client, as they contain blaze server side only tdfs as members).
class SendPlayerSessionInvitationsCommand : public SendPlayerSessionInvitationsCommandStub
{
public:
    SendPlayerSessionInvitationsCommand(Message* message, SendPlayerSessionInvitationsRequest* request, ArsonSlaveImpl* componentImpl)
        : SendPlayerSessionInvitationsCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~SendPlayerSessionInvitationsCommand() override {}

private:

    SendPlayerSessionInvitationsCommandStub::Errors execute() override
    {
        if (!gCurrentLocalUserSession)
        {
            ERR_LOG("[SendPlayerSessionInvitationsCommand].execute: Internal test err: gCurrentLocalUserSession null.");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = ERR_OK;
        UserIdentification ident;
        UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), ident);
        ExternalUserAuthInfo authInfo;
        authInfo.setCachedExternalSessionToken(mRequest.getHeader().getAuthToken());
        authInfo.setServiceName(gCurrentLocalUserSession->getServiceName());
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

DEFINE_SENDPLAYERSESSIONINVITATIONS_CREATE()

} // Arson
} // Blaze
