/*************************************************************************************************/
/*!
    \file   reconfigure_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"

#include "framework/tdf/controllertypes_server.h"
#include "framework/rpc/blazecontrollerslave/reconfigure_stub.h"

#include "framework/controller/controller.h"
#include "framework/system/fibermanager.h"


namespace Blaze
{

class ReconfigureCommand : public ReconfigureCommandStub
{
public:
    ReconfigureCommand(Message *message, ReconfigureRequest *request, Controller *controller);
    ~ReconfigureCommand() override {}

    ReconfigureCommandStub::Errors execute() override;

private:
    Controller *mController;
};

DEFINE_RECONFIGURE_CREATE_COMPNAME(Controller);

ReconfigureCommand::ReconfigureCommand(Message *message, ReconfigureRequest *request, Controller *controller)
    : ReconfigureCommandStub(message, request),
    mController(controller)
{
}

ReconfigureCommandStub::Errors ReconfigureCommand::execute()
{
    // check if the current user has the permission to reload config
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_RELOAD_CONFIG))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "User [" << UserSession::getCurrentUserBlazeId() << "] attempted to reconfigure, no permission!");
        return ReconfigureCommandStub::ERR_AUTHORIZATION_REQUIRED;
    }

    BlazeControllerMaster *masterController = mController->getMaster();
    if (masterController == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ReconfigureCommand].execute() : Master controller not available.");
        return ReconfigureCommandStub::ERR_SYSTEM;
    }

    BlazeRpcError err = masterController->reconfigureMaster(mRequest);
    if (err != Blaze::ERR_OK)
    {
        return commandErrorFromBlazeError(err);
    }

    //  sleep fiber until the controller signals validation has completed.
    err = mController->waitForReconfigureValidation(mErrorResponse.getValidationErrorMap());

    if (err == Blaze::ERR_OK)
    {
        if (gCurrentUserSession != nullptr)
        {
            BLAZE_INFO_LOG(Log::CONTROLLER, "User [" << gCurrentUserSession->getBlazeId() << "] reconfigured, had permission.");
        }
        else
        {
            BLAZE_INFO_LOG(Log::CONTROLLER, "Internal reconfigure completed successfully.");
        }
    }
    return commandErrorFromBlazeError(err);
}

} // Blaze
