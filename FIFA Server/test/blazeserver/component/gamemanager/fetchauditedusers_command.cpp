/*! ************************************************************************************************/
/*!
    \file fetchauditedusers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/fetchauditedusers_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Look up a user (possibly from the db, if they aren't in game or logged in), then
            forward ban cmd to master with the banned user's blazeId.
    ***************************************************************************************************/
    class FetchAuditedUsersCommand : public FetchAuditedUsersCommandStub
    {
    public:

        FetchAuditedUsersCommand(Message* message, FetchAuditedUsersRequest *request, GameManagerSlaveImpl* componentImpl)
            : FetchAuditedUsersCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~FetchAuditedUsersCommand() override
        {
        }

    private:

        FetchAuditedUsersCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent.fetchAuditedUsers(mRequest, mResponse));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_FETCHAUDITEDUSERS_CREATE()

}
}
