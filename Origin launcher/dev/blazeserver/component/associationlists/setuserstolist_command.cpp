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
#include "associationlists/rpc/associationlistsslave/setuserstolist_stub.h"

namespace Blaze
{
namespace Association
{

class SetUsersToListCommand : public SetUsersToListCommandStub
{
public:
    SetUsersToListCommand(Message* message, Blaze::Association::UpdateListMembersRequest* request, AssociationListsSlaveImpl* componentImpl)
        : SetUsersToListCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~SetUsersToListCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    SetUsersToListCommandStub::Errors execute() override
    {      
        BlazeRpcError result = mComponent->checkUserEditPermission(mRequest.getBlazeId());
        if (result != Blaze::ERR_OK)
            return commandErrorFromBlazeError(result);       

        for (auto member : mRequest.getListMemberIdVector())
            convertToPlatformInfo(member->getUser());

        BlazeId ownerBlazeId = mRequest.getBlazeId();
        if (ownerBlazeId == INVALID_BLAZE_ID)
            ownerBlazeId = gCurrentUserSession->getBlazeId();  // If requester does not set user id, update self's lists

        UserInfoAttributesVector users;
        ListMemberIdVector extMembers;
        result = mComponent->validateListMemberIdVector(mRequest.getListMemberIdVector(), users, ownerBlazeId, &extMembers);
        if (result == Blaze::ERR_OK)
        {
            AssociationListRef list = mComponent->findOrCreateList(mRequest.getListIdentification(), ownerBlazeId);
            if (list != nullptr)
            {
                result = list->setMembers(users, extMembers, mRequest.getMemberHash(), mRequest.getAttributesMask(), mResponse, gCurrentUserSession->getClientPlatform());
            }
            else
            {
                result = Blaze::ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
            }
        }       

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free
};

// static factory method impl
DEFINE_SETUSERSTOLIST_CREATE()

} // Association
} // Blaze

