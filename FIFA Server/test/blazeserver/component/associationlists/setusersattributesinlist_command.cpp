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
#include "associationlists/rpc/associationlistsslave/setusersattributesinlist_stub.h" 

namespace Blaze
{
namespace Association
{

    class SetUsersAttributesInListCommand : public SetUsersAttributesInListCommandStub
    {
    public:
        SetUsersAttributesInListCommand(Message* message, Blaze::Association::UpdateListMembersRequest* request, AssociationListsSlaveImpl* componentImpl)
            : SetUsersAttributesInListCommandStub(message, request), 
              mComponent(componentImpl)
        {
        }

        ~SetUsersAttributesInListCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        // this will be called by the generic request handler called in execute()
        

        SetUsersAttributesInListCommandStub::Errors execute() override
        {
            TRACE_LOG("[SetUsersAttributesInListCommandStub].execute()");

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
                    ownerBlazeId = gCurrentUserSession->getBlazeId();  // If requester does not set user id, update self's lists

                AssociationListRef list = mComponent->findOrCreateList(mRequest.getListIdentification(), ownerBlazeId);
                if (list != nullptr)
                {
                    result = list->setMembersAttributes(users, mRequest.getAttributesMask(), mResponse, gCurrentUserSession->getClientPlatform());
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
    DEFINE_SETUSERSATTRIBUTESINLIST_CREATE()
    
} // Association
} // Blaze
