/*************************************************************************************************/
/*!
    \file   postentitlement_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GrantEntitlementCommand

    Sends Nucleus an email and password for authentication.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/grantentitlement_stub.h"
#include "authentication/util/grantentitlementutil.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GrantEntitlementCommand : public GrantEntitlementCommandStub
{
public:
    GrantEntitlementCommand(Message* message, Blaze::Authentication::PostEntitlementRequest* request, AuthenticationSlaveImpl* componentImpl);
    ~GrantEntitlementCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GrantEntitlementUtil mGrantEntitlementUtil;

    // States
    GrantEntitlementCommandStub::Errors execute() override;
};

DEFINE_GRANTENTITLEMENT_CREATE()

GrantEntitlementCommand::GrantEntitlementCommand(Message* message, Blaze::Authentication::PostEntitlementRequest* request, AuthenticationSlaveImpl* componentImpl)
    : GrantEntitlementCommandStub(message, request),
    mComponent(componentImpl),
    mGrantEntitlementUtil(gCurrentLocalUserSession->getPeerInfo(),
        &(componentImpl->getNucleusBypassList()), componentImpl->isBypassNucleusEnabled(gCurrentUserSession->getClientType()))
{
}

/* Private methods *******************************************************************************/
GrantEntitlementCommandStub::Errors GrantEntitlementCommand::execute()
{
    const char8_t* productName = gCurrentUserSession->getProductName();

    if (!mComponent->passClientGrantWhitelist(productName, mRequest.getGroupName()))
    {
        WARN_LOG("[GrantEntitlementCommand].execute() - white list check failed. Groupname(" 
            << mRequest.getGroupName() << ") not defined in ClientEntitlementGrantWhitelist");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_WHITELIST), RiverPoster::entitlement_grant_failed);
        return AUTH_ERR_WHITELIST;
    }

    BlazeRpcError error = mGrantEntitlementUtil.grantEntitlementLegacy(
                                    gCurrentUserSession->getAccountId(),
                                    gCurrentUserSession->getBlazeId(),
                                    mRequest.getProductId(),
                                    mRequest.getEntitlementTag(),
                                    mRequest.getGroupName(), mRequest.getProjectId(), 
                                    mComponent->getEntitlementSource(gCurrentUserSession->getProductName()),
                                    gCurrentUserSession->getProductName(),
                                    Blaze::Nucleus::EntitlementType::DEFAULT, Blaze::Nucleus::EntitlementStatus::ACTIVE, mRequest.getWithPersona());

    if (error == AUTH_ERR_LINK_PERSONA)
    {
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_LINK_PERSONA), RiverPoster::entitlement_grant_failed);
    }
    else if (error == ERR_COULDNT_CONNECT)
    {
        gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
    } 

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
