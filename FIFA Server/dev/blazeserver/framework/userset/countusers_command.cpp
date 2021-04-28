/*************************************************************************************************/
/*!
    \file   countusers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CountUsersCommand

    resolve count requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/userset/userset.h"
#include "framework/rpc/usersetslave/countusers_stub.h"
namespace Blaze
{
    class CountUsersCommand : public CountUsersCommandStub
    {
    public:

        CountUsersCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : CountUsersCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~CountUsersCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        CountUsersCommandStub::Errors execute() override
        {
            uint32_t count = 0;
            BlazeRpcError result = mComponent->countLocalUsers(mRequest.getBlazeObjectId(), count);
            if (result == Blaze::ERR_OK)
                mResponse.setCount(count);
            return commandErrorFromBlazeError(result);
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_COUNTUSERS_CREATE_COMPNAME(UserSetManager)

} // Blaze
