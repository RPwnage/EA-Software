/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"

// associationlists includes
#include "associationlists/tdf/associationlists.h"
#include "associationlistsslaveimpl.h"
#include "associationlists/rpc/associationlistsslave/getlistforuser_stub.h"


namespace Blaze
{
namespace Association
{

class GetListForUserCommand : public GetListForUserCommandStub
{
public:
    GetListForUserCommand(Message* message, Blaze::Association::GetListForUserRequest* request, AssociationListsSlaveImpl* componentImpl)
        : GetListForUserCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~GetListForUserCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    GetListForUserCommandStub::Errors execute() override
    {
        BlazeId curBlazeId = gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID;
        ClientPlatformType curClientPlatform = gCurrentUserSession != nullptr ? gCurrentUserSession->getClientPlatform() : ClientPlatformType::INVALID;
        BlazeId reqBlazeId = mRequest.getBlazeId();
        if (reqBlazeId == INVALID_BLAZE_ID)
        {
            // default if no user specified in request
            reqBlazeId = curBlazeId;
        }

        if (reqBlazeId == INVALID_BLAZE_ID)
        {
            // early out because lookupUserById will fail
            return ASSOCIATIONLIST_ERR_USER_NOT_FOUND;
        }

        // check if the current user has the permission to retrieve assoc list for another user
        if (reqBlazeId != curBlazeId && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_USER_ASSOC_LIST))
        {
            WARN_LOG("[GetListForUserCommand] User [" << curBlazeId << "] attempted to get [" << reqBlazeId << "]s list [" << mRequest.getListIdentification().getListType() << "], no permission!");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        BlazeRpcError result = Blaze::ERR_OK;
        
        UserInfoPtr userInfo;
        result = gUserSessionManager->lookupUserInfoByBlazeId(reqBlazeId, userInfo);

        if (result == Blaze::ERR_OK)
        {
            AssociationListRef list = mComponent->findOrCreateList(mRequest.getListIdentification(), reqBlazeId);
            if (list != nullptr)
            {
                list->getInfo().copyInto(mResponse.getInfo());
                mResponse.getInfo().setBlazeObjId(EA::TDF::ObjectId(AssociationListsSlave::COMPONENT_ID, list->getType(), reqBlazeId));

                if ((result = list->fillOutMemberInfoVector(mResponse.getListMemberInfoVector(), curClientPlatform, mRequest.getOffset(), mRequest.getMaxResultCount())) == ERR_OK)
                {
                    mResponse.setTotalCount(list->getTotalCount());
                    mResponse.setOffset(list->getCurrentOffset());
                }
            }
            else
            {
                result = ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;  
            }
        }
        else if (result == Blaze::USER_ERR_USER_NOT_FOUND)
        {
            result = Blaze::ASSOCIATIONLIST_ERR_USER_NOT_FOUND;
        }

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_GETLISTFORUSER_CREATE()
    
} // Association
} // Blaze
