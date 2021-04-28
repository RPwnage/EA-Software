/*************************************************************************************************/
/*!
    \file   countsessions_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CountSessionsCommand

    resolve count requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/userset/userset.h"
#include "framework/rpc/usersetslave/countsessions_stub.h"
namespace Blaze
{
    class CountSessionsCommand : public CountSessionsCommandStub
    {
    public:

        CountSessionsCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : CountSessionsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~CountSessionsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        CountSessionsCommandStub::Errors execute() override
        {
            uint32_t count = 0;
            BlazeRpcError result = mComponent->countLocalSessions(mRequest.getBlazeObjectId(), count);
            if (result == Blaze::ERR_OK)
                mResponse.setCount(count);
            return commandErrorFromBlazeError(result);
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_COUNTSESSIONS_CREATE_COMPNAME(UserSetManager)

} // Blaze
