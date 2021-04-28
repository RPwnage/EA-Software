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
#include "associationlists/rpc/associationlistsslave/removeusersfromlist_stub.h"


namespace Blaze
{
namespace Association
{

class RemoveUsersFromListCommand : public RemoveUsersFromListCommandStub
{
public:
    RemoveUsersFromListCommand(Message* message, Blaze::Association::UpdateListMembersRequest* request, AssociationListsSlaveImpl* componentImpl)
        : RemoveUsersFromListCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~RemoveUsersFromListCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    RemoveUsersFromListCommandStub::Errors execute() override
    {
        TRACE_LOG("[RemoveUsersFromListCommandStub].execute()");

        BlazeRpcError result = mComponent->checkUserEditPermission(mRequest.getBlazeId());
        if (result != Blaze::ERR_OK)
            return commandErrorFromBlazeError(result);

        for (auto member : mRequest.getListMemberIdVector())
            convertToPlatformInfo(member->getUser());

        UserInfoAttributesVector users;
        result = mComponent->validateListMemberIdVector(mRequest.getListMemberIdVector(), users);
        if (result == Blaze::ERR_OK)
        {

            BlazeId ownerBlazeId = mRequest.getBlazeId();
            if (ownerBlazeId == INVALID_BLAZE_ID)
                ownerBlazeId = gCurrentUserSession->getBlazeId();  // If requestor does not set user id, update self's lists
            
            AssociationListRef list = mComponent->findOrCreateList(mRequest.getListIdentification(), ownerBlazeId);
            if (list != nullptr)
            {
                result = list->removeMembers(users, mRequest.getValidateDelete(), mResponse, gCurrentUserSession->getClientPlatform());
            }
            else
            {
                result = Blaze::ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
            }
        }
        else if (result == Blaze::ASSOCIATIONLIST_ERR_CANNOT_INCLUDE_SELF || result == Blaze::ASSOCIATIONLIST_ERR_USER_NOT_FOUND)
        {
            result = Blaze::ASSOCIATIONLIST_ERR_MEMBER_NOT_FOUND_IN_THE_LIST;
        }

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free    

};
// static factory method impl
DEFINE_REMOVEUSERSFROMLIST_CREATE()
    
} // Association
} // Blaze
