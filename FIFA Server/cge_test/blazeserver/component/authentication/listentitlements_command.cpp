/*************************************************************************************************/
/*!
    \file   listentitlement2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListEntitlementsCommand 

    Retrieve Nucleus Entitlements for the current user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/listentitlements_stub.h"
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
class ListEntitlementsCommand : public ListEntitlementsCommandStub
{
public:
    ListEntitlementsCommand(Message* message, ListEntitlementsRequest *request, AuthenticationSlaveImpl* componentImpl);

    ~ListEntitlementsCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    ListEntitlementsUtil mListEntitlementsUtil;

    // States
    ListEntitlementsCommandStub::Errors execute() override;

};
DEFINE_LISTENTITLEMENTS_CREATE()

ListEntitlementsCommand::ListEntitlementsCommand(Message* message, ListEntitlementsRequest *request, AuthenticationSlaveImpl* componentImpl)
    : ListEntitlementsCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/
ListEntitlementsCommandStub::Errors ListEntitlementsCommand::execute()
{
    if (mRequest.getGroupNameList().empty())
    {
        TRACE_LOG("[ListEntitlementsCommand].execute has no group name in the list.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_GROUPNAME_REQUIRED), RiverPoster::entitlement_access_failed);
        return AUTH_ERR_GROUPNAME_REQUIRED;
    }

    BlazeRpcError error = mComponent->checkListEntitlementsRequest(mRequest.getGroupNameList(), mRequest.getPageSize(), mRequest.getPageNo());

    if (error != Blaze::ERR_OK)
    {
        if ((error == AUTH_ERR_PAGENO_ZERO) || (error == AUTH_ERR_PAGESIZE_ZERO))
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
        }

        return commandErrorFromBlazeError(error);
    }
    // default account id is current user
    AccountId accountId = gCurrentUserSession->getAccountId();
    PersonaId personaId =  gCurrentUserSession->getBlazeId();

    // look up user and get nucleus id of the user
    if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
    {
        UserInfoPtr userInfo;
        error = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
        if (error != Blaze::ERR_OK)
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_INVALID_USER), RiverPoster::entitlement_access_failed);
            return AUTH_ERR_INVALID_USER;
        }

        if (userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() != accountId &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_LIST_OTHER_USER_ENTITLEMENTS))
        {
            WARN_LOG("[ListEntitlementsCommand].execute:  Account Id (" << accountId << ") attempting to look up nucleus id (" << userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() << ")'s entitlements without proper permissions.");
            error = Blaze::ERR_AUTHORIZATION_REQUIRED;

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
            return commandErrorFromBlazeError(error);;
        }

        accountId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();
        personaId = userInfo->getId();
    }

    if (mRequest.getEntitlementSearchFlag().getIncludePersonaLink())
    {
        error = mListEntitlementsUtil.listEntitlementsForPersona(personaId, mRequest.getGroupNameList(), mRequest.getPageSize(), mRequest.getPageNo(), mResponse.getEntitlements(), this);
        if (error != Blaze::ERR_OK)
        {
            if (error == ERR_COULDNT_CONNECT)
            {
                gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
            }
            return commandErrorFromBlazeError(error);
        }
    }

    if (mRequest.getEntitlementSearchFlag().getIncludeUnlinked())
    {
        error = mListEntitlementsUtil.listEntitlementsForUser(accountId, mRequest.getGroupNameList(), mRequest.getPageSize(), mRequest.getPageNo(), mResponse.getEntitlements(), this, false);
        if (error != Blaze::ERR_OK)
        {
            if (error == ERR_COULDNT_CONNECT)
            {
                gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
            }
            return commandErrorFromBlazeError(error);
        }
    }
    
    return  commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
