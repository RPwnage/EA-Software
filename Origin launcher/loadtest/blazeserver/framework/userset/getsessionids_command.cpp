/*************************************************************************************************/
/*!
    \file   getsessionids_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetSessionIdsCommand

    resolve externalId lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/userset/userset.h"
#include "framework/rpc/usersetslave/getsessionids_stub.h"
namespace Blaze
{
    class GetSessionIdsCommand : public GetSessionIdsCommandStub
    {
    public:

        GetSessionIdsCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : GetSessionIdsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetSessionIdsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetSessionIdsCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->getLocalSessionIds(mRequest.getBlazeObjectId(), mResponse.getUserSessionIdList()));
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETSESSIONIDS_CREATE_COMPNAME(UserSetManager)

} // Blaze
