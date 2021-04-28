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
#include "associationlists/rpc/associationlistsslave/unsubscribefromlists_stub.h"


namespace Blaze
{
namespace Association
{

class UnsubscribeFromListsCommand : public UnsubscribeFromListsCommandStub
{
public:
    UnsubscribeFromListsCommand(Message* message, Blaze::Association::UpdateListsRequest* request, AssociationListsSlaveImpl* componentImpl)
    : UnsubscribeFromListsCommandStub(message, request), 
    mComponent(componentImpl)
    {
    }

    ~UnsubscribeFromListsCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:   
    UnsubscribeFromListsCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        AssociationListVector lists;
        result = mComponent->loadLists(mRequest.getListIdentificationVector(), gCurrentUserSession->getBlazeId(), lists);
        if (result == Blaze::ERR_OK)
        {        
            for (AssociationListVector::iterator itr = lists.begin(), end = lists.end(); itr != end; ++itr)
            {            
                (*itr)->unsubscribe();
            }
        }
        else
        {
            ERR_LOG("[UnsubscribeToListsCommandStub].execute: Failed to load the requested association list for user [" 
                << gCurrentUserSession->getBlazeId() << "]: ERROR [" << (ErrorHelp::getErrorName(result)) << "]");
        }

        return commandErrorFromBlazeError(result);
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_UNSUBSCRIBEFROMLISTS_CREATE()

} // Association
} // Blaze
