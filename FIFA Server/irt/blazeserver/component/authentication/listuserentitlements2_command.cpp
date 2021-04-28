/*************************************************************************************************/
/*!
    \file   listuserentitlements2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListUserEntitlements2Command 

    Retrieve Nucleus Entitlements for the current user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/listuserentitlements2_stub.h"
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
class ListUserEntitlements2Command : public ListUserEntitlements2CommandStub
{
public:
    ListUserEntitlements2Command(Message* message, ListUserEntitlements2Request *request, AuthenticationSlaveImpl* componentImpl);

    ~ListUserEntitlements2Command() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    ListEntitlementsUtil mListEntitlementsUtil;

    // States
    ListUserEntitlements2CommandStub::Errors execute() override;
};

DEFINE_LISTUSERENTITLEMENTS2_CREATE();

ListUserEntitlements2Command::ListUserEntitlements2Command(Message* message, ListUserEntitlements2Request *request, AuthenticationSlaveImpl* componentImpl)
    : ListUserEntitlements2CommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/
ListUserEntitlements2CommandStub::Errors ListUserEntitlements2Command::execute()
{
    BlazeRpcError error = mComponent->checkListEntitlementsRequest(mRequest.getGroupNameList(), mRequest.getPageSize(), mRequest.getPageNo()); 

    if (error != Blaze::ERR_OK)
    {
        if (error == Blaze::ERR_AUTHORIZATION_REQUIRED)
        {
            WARN_LOG("[ListUserEntitlements2Command].execute(): User[" << gCurrentUserSession->getBlazeId() << "] does not have permission to list all entitlements. "
                << "Failed with error[" << ErrorHelp::getErrorName(error) <<"]");
        }
        else
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
        }

        return commandErrorFromBlazeError(error);
    }

    // default account id is current user
    AccountId accountId = gCurrentUserSession->getAccountId();

    // look up account id of the user
    if (mRequest.getBlazeId() != INVALID_BLAZE_ID) // RESTRICT PERMISSION
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
            WARN_LOG("[ListUserEntitlements2Command].execute:  Account Id (" << accountId << ") attempting to look up nucleus id (" << userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() << ")'s entitlements without proper permissions.");
            error = Blaze::ERR_AUTHORIZATION_REQUIRED;

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
            return commandErrorFromBlazeError(error);;
        }

        accountId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();
    }

    error = mListEntitlementsUtil.listEntitlements2ForUser(accountId, mRequest, mResponse.getEntitlements(), this);

    if (error == Blaze::ERR_OK)
    {
        if (accountId != gCurrentUserSession->getAccountId())
        {
            TRACE_LOG("[ListUserEntitlements2Command].execute: User " << gCurrentUserSession->getBlazeId()
                << " got entitlements for nucleus id[" << accountId << "]." );
        }
    }
    else if (error == ERR_COULDNT_CONNECT)
    {
        gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
    }
   
    return  commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
