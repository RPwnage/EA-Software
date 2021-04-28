/*! ************************************************************************************************/
/*!
    \file fetchaudituserdata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/fetchauditeduserdata_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Look up a user (possibly from the db, if they aren't in game or logged in), then
            forward ban cmd to master with the banned user's blazeId.
    ***************************************************************************************************/
    class FetchAuditedUserDataCommand : public FetchAuditedUserDataCommandStub
    {
    public:

        FetchAuditedUserDataCommand(Message* message, FetchAuditedUserDataRequest *request, GameManagerSlaveImpl* componentImpl)
            : FetchAuditedUserDataCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~FetchAuditedUserDataCommand() override
        {
        }

    private:

        FetchAuditedUserDataCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent.fetchAuditedUserData(mRequest, mResponse));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_FETCHAUDITEDUSERDATA_CREATE()

}
}
