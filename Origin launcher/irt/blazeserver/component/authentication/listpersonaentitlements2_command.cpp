/*************************************************************************************************/
/*!
    \file   listpersonaentitlements2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListPersonaEntitlements2Command 

    Retrieve Nucleus Entitlements for the current user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/listpersonaentitlements2_stub.h"
#include "authentication/util/listentitlementsutil.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class ListPersonaEntitlements2Command : public ListPersonaEntitlements2CommandStub
{
public:
    ListPersonaEntitlements2Command(Message* message, ListPersonaEntitlements2Request *request, AuthenticationSlaveImpl* componentImpl);

    ~ListPersonaEntitlements2Command() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    ListEntitlementsUtil mListEntitlementsUtil;

    // States
    ListPersonaEntitlements2CommandStub::Errors execute() override;
};

DEFINE_LISTPERSONAENTITLEMENTS2_CREATE();

ListPersonaEntitlements2Command::ListPersonaEntitlements2Command(Message* message, ListPersonaEntitlements2Request *request, AuthenticationSlaveImpl* componentImpl)
    : ListPersonaEntitlements2CommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/
ListPersonaEntitlements2CommandStub::Errors ListPersonaEntitlements2Command::execute()
{
    BlazeRpcError error = AuthenticationSlaveImpl::checkListEntitlementsRequest(mRequest.getGroupNameList(), mRequest.getPageSize(), mRequest.getPageNo());

    if (error != Blaze::ERR_OK)
    {
        if (error == Blaze::ERR_AUTHORIZATION_REQUIRED)
        {
            WARN_LOG("[ListPersonaEntitlements2Command].execute(): User[" << gCurrentUserSession->getBlazeId() << "] does not have permission to list all entitlements. "
                << "Failed with error[" << ErrorHelp::getErrorName(error) <<"]");
        }
        else
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
        }

        return commandErrorFromBlazeError(error);
    }

    // default account id is current user
    PersonaId personaId = gCurrentUserSession->getBlazeId();

    // look up user and get nucleus id of the user
    if (mRequest.getPersonaId() != INVALID_ACCOUNT_ID)  // RESTRICT BASED ON PERMISSION
    {
        if (mRequest.getPersonaId() != personaId && 
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_LIST_OTHER_USER_ENTITLEMENTS))
        {
            WARN_LOG("[ListPersonaEntitlements2Command].execute:  Persona " << personaId << " attempting to look up persona's " << mRequest.getPersonaId() << " entitlements without proper permissions.");
            error = Blaze::ERR_AUTHORIZATION_REQUIRED;

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
            return commandErrorFromBlazeError(error);;
        }

        personaId = mRequest.getPersonaId();
    }

    error = mListEntitlementsUtil.listEntitlements2ForPersona(personaId, mRequest, mResponse.getEntitlements(), this);

    if (error == Blaze::ERR_OK)
    {
        if (personaId != gCurrentUserSession->getBlazeId())
        {
            INFO_LOG("[ListPersonaEntitlements2Command].execute(): User " << gCurrentUserSession->getBlazeId()
                << " got entitlements for persona[" << mRequest.getPersonaId() << "]." );
        }
    }
    else if (error == ERR_COULDNT_CONNECT)
    {
        gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
    }

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
