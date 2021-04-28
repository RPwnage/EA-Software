/*! ************************************************************************************************/
/*!
\file environmentcapture_commands.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagermaster/capturegamemanagerenvironmentmaster_stub.h"
#include "gamemanager/rpc/gamemanagermaster/isgamecapturedonemaster_stub.h"
#include "gamemanager/rpc/gamemanagermaster/getredisdumplocationsmaster_stub.h"
#include "gamemanager/gamemanagermasterimpl.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Command version of captureGameManagerEnvironmentMaster so that it can be used while blocking.
    *************************************************************************************************/
    class CaptureGameManagerEnvironmentMasterCommand : public CaptureGameManagerEnvironmentMasterCommandStub
    {
    public:

        CaptureGameManagerEnvironmentMasterCommand(
            Message* message, GameManagerMasterImpl* componentImpl)
            :   CaptureGameManagerEnvironmentMasterCommandStub(message),
            mComponent(*componentImpl)
        {
        }

        ~CaptureGameManagerEnvironmentMasterCommand() override {}

    private:

        CaptureGameManagerEnvironmentMasterCommand::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent.processCaptureGameManagerEnvironmentMaster(mResponse, mMessage));
        }

    private:

        GameManagerMasterImpl &mComponent;
    };

    DEFINE_CAPTUREGAMEMANAGERENVIRONMENTMASTER_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Command version of IsGameCaptureDoneMasterCommand so that it can be used while blocking.
    *************************************************************************************************/
    class IsGameCaptureDoneMasterCommand : public IsGameCaptureDoneMasterCommandStub
    {
    public:

        IsGameCaptureDoneMasterCommand(
            Message* message, GameCaptureResponse *request, GameManagerMasterImpl* componentImpl)
            :   IsGameCaptureDoneMasterCommandStub(message, request),
            mComponent(*componentImpl)
        {
        }

        ~IsGameCaptureDoneMasterCommand() override {}

    private:

        IsGameCaptureDoneMasterCommand::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent.processIsGameCaptureDoneMaster(mRequest, mResponse, mMessage));
        }

    private:

        GameManagerMasterImpl &mComponent;
    };

    DEFINE_ISGAMECAPTUREDONEMASTER_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Command version of getRedisDumpLocationsMaster so that it can be used while blocking.
    *************************************************************************************************/
    class GetRedisDumpLocationsMasterCommand : public GetRedisDumpLocationsMasterCommandStub
    {
    public:

        GetRedisDumpLocationsMasterCommand(
            Message* message, GameManagerMasterImpl* componentImpl)
            :   GetRedisDumpLocationsMasterCommandStub(message),
            mComponent(*componentImpl)
        {
        }

        ~GetRedisDumpLocationsMasterCommand() override {}

    private:

        GetRedisDumpLocationsMasterCommand::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent.processGetRedisDumpLocationsMaster(mResponse, mMessage));
        }

    private:

        GameManagerMasterImpl &mComponent;
    };

    DEFINE_GETREDISDUMPLOCATIONSMASTER_CREATE()

} // namespace GameManager
} // namespace Blaze
