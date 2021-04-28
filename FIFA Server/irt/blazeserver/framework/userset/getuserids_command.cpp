/*************************************************************************************************/
/*!
    \file   getuserids_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUserIdsCommand

    resolve externalId lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/userset/userset.h"
#include "framework/rpc/usersetslave/getuserids_stub.h"
#include "framework/rpc/usersetslave/getuserblazeids_stub.h"
#include "framework/rpc/usersetslave/getproviderroutingoptions_stub.h"
namespace Blaze
{
    class GetUserBlazeIdsCommand : public GetUserBlazeIdsCommandStub
    {
    public:

        GetUserBlazeIdsCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : GetUserBlazeIdsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetUserBlazeIdsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetUserBlazeIdsCommand::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->getLocalUserBlazeIds(mRequest.getBlazeObjectId(), mResponse.getBlazeIdList()));
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETUSERBLAZEIDS_CREATE_COMPNAME(UserSetManager)

    class GetProviderRoutingOptionsCommand : public GetProviderRoutingOptionsCommandStub
    {
    public:

        GetProviderRoutingOptionsCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : GetProviderRoutingOptionsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetProviderRoutingOptionsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetProviderRoutingOptionsCommand::Errors execute() override
        {
            RoutingOptions opts = mComponent->getProviderRoutingOptions(mRequest.getBlazeObjectId(), false);

            if (opts.getType() == RoutingOptions::INSTANCE_ID)
                mResponse.getId().setInstanceId(opts.getAsInstanceId());
            else if (opts.getType() == RoutingOptions::SLIVER_IDENTITY)
                mResponse.getId().setSliverIdentity(opts.getAsSliverIdentity());

            return commandErrorFromBlazeError(ERR_OK);
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETPROVIDERROUTINGOPTIONS_CREATE_COMPNAME(UserSetManager)

    class GetUserIdsCommand : public GetUserIdsCommandStub
    {
    public:

        GetUserIdsCommand(Message* message, Blaze::UserSetRequest* request, UserSetManager* componentImpl)
            : GetUserIdsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetUserIdsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetUserIdsCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->getLocalUserIds(mRequest.getBlazeObjectId(), mResponse.getUserIdList()));
        }

    private:
        UserSetManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETUSERIDS_CREATE_COMPNAME(UserSetManager)
} // Blaze
