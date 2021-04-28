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
#include "associationlists/rpc/associationlistsslave/getmemberhash_stub.h"


namespace Blaze
{
namespace Association
{

class GetMemberHashCommand : public GetMemberHashCommandStub
{
public:
    GetMemberHashCommand(Message* message, Blaze::Association::GetMemberHashRequest* request, AssociationListsSlaveImpl* componentImpl)
        : GetMemberHashCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~GetMemberHashCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    GetMemberHashCommandStub::Errors execute() override
    {
        BlazeRpcError result = mComponent->checkUserEditPermission(mRequest.getBlazeId());
        if (result != Blaze::ERR_OK)
            return commandErrorFromBlazeError(result);                 

        BlazeId ownerBlazeId = mRequest.getBlazeId();
        if (ownerBlazeId == INVALID_BLAZE_ID)
            ownerBlazeId = gCurrentUserSession->getBlazeId();  // If requestor does not set user id, update self's lists

        AssociationListRef list = mComponent->findOrCreateList(mRequest.getListIdentification(), ownerBlazeId);
        if (list != nullptr)
        {
            MemberHash hashValue;
            result = list->getMemberHash(hashValue);
            if (result == ERR_OK)
            {
                mResponse.setMemberHash(hashValue);
            }
        }
        else
        {
            result = Blaze::ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
        }  

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_GETMEMBERHASH_CREATE()

} // Association
} // Blaze
