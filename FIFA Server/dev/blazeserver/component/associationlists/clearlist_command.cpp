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
#include "associationlists/rpc/associationlistsslave/clearlists_stub.h"


namespace Blaze
{
namespace Association
{

class ClearListsCommand : public ClearListsCommandStub
{
public:
    ClearListsCommand(Message* message, Blaze::Association::UpdateListsRequest* request, AssociationListsSlaveImpl* componentImpl)
        : ClearListsCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~ClearListsCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    ClearListsCommandStub::Errors execute() override
    {
        TRACE_LOG("[ClearListsCommandStub].execute()");

        BlazeRpcError result = mComponent->checkUserEditPermission(mRequest.getBlazeId());
        if (result != Blaze::ERR_OK)
            return commandErrorFromBlazeError(result);                 

        BlazeId ownerBlazeId = mRequest.getBlazeId();
        if (ownerBlazeId == INVALID_BLAZE_ID)
            ownerBlazeId = gCurrentUserSession->getBlazeId();  // If requestor does not set user id, update self's lists

        for (ListIdentificationVector::const_iterator itr = mRequest.getListIdentificationVector().begin(), end = mRequest.getListIdentificationVector().end(); (itr != end) && (result == ERR_OK); ++itr)
        {           
            AssociationListRef list = mComponent->findOrCreateList(**itr, ownerBlazeId);
            if (list != nullptr)
            {
                result = list->clearMembers(gCurrentUserSession->getClientPlatform());
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
DEFINE_CLEARLISTS_CREATE()

} // Association
} // Blaze
