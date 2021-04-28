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
#include "associationlists/rpc/associationlistsslave/checklistcontainsmembers_stub.h"

namespace Blaze
{
namespace Association
{

class CheckListContainsMembersCommand : public CheckListContainsMembersCommandStub
{
public:
    CheckListContainsMembersCommand(Message* message, Blaze::Association::CheckListContainsMembersRequest* request, AssociationListsSlaveImpl* componentImpl)
        : CheckListContainsMembersCommandStub(message, request), 
        mComponent(componentImpl)
    {
    }

    ~CheckListContainsMembersCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    CheckListContainsMembersCommandStub::Errors execute() override
    {
        TRACE_LOG("[CheckListContainsMembersCommandStub].execute()");
        return commandErrorFromBlazeError(mComponent->checkListContainsMembers(mRequest.getListIdentification(), mRequest.getOwnerBlazeId(), mRequest.getMembersBlazeIds().getBlazeIds(), mResponse.getBlazeIds()));
    }

private:
    AssociationListsSlaveImpl* mComponent;  // memory owned by creator, don't free

};
// static factory method impl
DEFINE_CHECKLISTCONTAINSMEMBERS_CREATE()

} // Association
} // Blaze
