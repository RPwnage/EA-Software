/*************************************************************************************************/
/*!
    \file   getaccount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAccountCommand

    Gets the Account details.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

#include "authentication/authenticationimpl.h"
#include "framework/oauth/nucleus/error_codes.h"
#include "authentication/rpc/authenticationslave/getaccount_stub.h"
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{
namespace Authentication
{

class GetAccountCommand : public GetAccountCommandStub
{
public:
    GetAccountCommand(Message* message, AuthenticationSlaveImpl* componentImpl)
        : GetAccountCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GetAccountCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;
    
    /* Private methods *******************************************************************************/
    GetAccountCommandStub::Errors execute() override
    {
        BlazeId blazeId = gCurrentUserSession->getBlazeId();

        OAuth::AccessTokenUtil tokenUtil;
        BlazeRpcError rc = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[GetAccountCommand].execute : Failed to retrieve access token for user " << blazeId);
            return commandErrorFromBlazeError(rc);
        }

        NucleusIdentity::GetPidRequest req;
        NucleusIdentity::GetAccountResponse resp;
        NucleusIdentity::IdentityError error;
        req.setPid(gCurrentUserSession->getAccountId());
        req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
        req.getAuthCredentials().setClientId(tokenUtil.getClientId());
        req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(this).getIpAsString());

        NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));

        rc = ident->getPid(req, resp, error);
        if (rc != ERR_OK)
        {
            rc = AuthenticationUtil::parseIdentityError(error, rc);
            ERR_LOG("[GetAccountCommand].execute: getPid failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return commandErrorFromBlazeError(rc);
        }

        mResponse.setAccountId((int64_t)resp.getPid().getPidId());   // Is this correct?  PidId == AccountId?
        
         Blaze::Nucleus::EmailStatus::Code emailStatus =  Blaze::Nucleus::EmailStatus::UNKNOWN;
        if (resp.getPid().getEmailStatus()[0] != '\0' && !Blaze::Nucleus::EmailStatus::ParseCode(resp.getPid().getEmailStatus(), emailStatus))
        {
            WARN_LOG("[GetAccountCommand].execute: Received unexpected enum EmailStatus value " << resp.getPid().getEmailStatus() << " from Nucleus");
        }
        mResponse.setEmailStatus(emailStatus);

        mResponse.setCountry(resp.getPid().getCountry());
        mResponse.setLanguage(resp.getPid().getLanguage());

        Blaze::Nucleus::AccountStatus::Code status = Blaze::Nucleus::AccountStatus::UNKNOWN;
        if (resp.getPid().getStatus()[0] != '\0' && !Blaze::Nucleus::AccountStatus::ParseCode(resp.getPid().getStatus(), status))
        {
            WARN_LOG("[GetAccountCommand].execute: Received unexpected enum AccountStatus value " << resp.getPid().getStatus() << " from Nucleus");
        }
        mResponse.setStatus(status);

        Blaze::Nucleus::StatusReason::Code reasonCode = Blaze::Nucleus::StatusReason::UNKNOWN;
        if (resp.getPid().getReasonCode()[0] != '\0' && !Nucleus::StatusReason::ParseCode(resp.getPid().getReasonCode(), reasonCode))
        {
            WARN_LOG("[GetAccountCommand].execute: Received unexpected enum StatusReason value " << resp.getPid().getReasonCode() << " from Nucleus");
        }
        mResponse.setReasonCode(reasonCode);

        mResponse.setThirdPartyOptin(resp.getPid().getThirdPartyOptin());
        mResponse.setGlobalOptin(resp.getPid().getGlobalOptin());
        mResponse.setDateCreated(resp.getPid().getDateCreated());
        mResponse.setLastAuth(resp.getPid().getLastAuthDate());
        mResponse.setAnonymousUser(resp.getPid().getAnonymousPid());
        mResponse.setUnderageUser(resp.getPid().getUnderagePid());

        // Return results.
        return ERR_OK;
    }

};
DEFINE_GETACCOUNT_CREATE()


} // Authentication
} // Blaze
