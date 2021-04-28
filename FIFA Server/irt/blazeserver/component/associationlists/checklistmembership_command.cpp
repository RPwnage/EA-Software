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

#include "associationlists/tdf/associationlists.h"
#include "associationlistsslaveimpl.h"
#include "associationlists/rpc/associationlistsslave/checklistmembership_stub.h"

namespace Blaze
{
namespace Association
{

class CheckListMembershipCommand : public CheckListMembershipCommandStub
{
public:
    CheckListMembershipCommand(Message* message, Blaze::Association::CheckListMembershipRequest* request, AssociationListsSlaveImpl* componentImpl)
        : CheckListMembershipCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~CheckListMembershipCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    CheckListMembershipCommandStub::Errors execute() override
    {
        TRACE_LOG("[CheckListMembershipCommandStub].execute()");
        return commandErrorFromBlazeError(mComponent->checkListMembership(mRequest.getListIdentification(), mRequest.getOwnersBlazeIds().getBlazeIds(), mRequest.getMemberBlazeId(), mResponse.getBlazeIds()));
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_CHECKLISTMEMBERSHIP_CREATE()

} // Association
} // Blaze
