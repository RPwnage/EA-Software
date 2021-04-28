/*************************************************************************************************/
/*!
    \file   startremotetransaction_command.cpp


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
#include "framework/rpc/blazecontrollerslave/drain_stub.h"

#include "framework/controller/controller.h"


namespace Blaze
{
    class DrainCommand : public DrainCommandStub
    {
    public:
        DrainCommand(Message *message, DrainRequest *request, Controller *controller) : DrainCommandStub(message, request) {}
        ~DrainCommand() override {}

        DrainCommand::Errors execute() override;
    };

    DEFINE_DRAIN_CREATE_COMPNAME(Controller);

    DrainCommandStub::Errors DrainCommand::execute()
    {
        // check if the current user has the permission to server maintenance
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[DrainCommand].processDrain: User [" << UserSession::getCurrentUserBlazeId() << "] attempted to drain, no permission!");
            return DrainError::ERR_AUTHORIZATION_REQUIRED;
        }

        //convert DrainReq to ShutdownReq
        Blaze::ShutdownRequest shutdownReq;

        shutdownReq.setRestart(false);

        switch (mRequest.getTarget())
        {
            case DrainRequest::DRAIN_TARGET_LOCAL:
                shutdownReq.setTarget(Blaze::ShutdownTarget::SHUTDOWN_TARGET_LOCAL);
                break;
            case DrainRequest::DRAIN_TARGET_LIST:
                shutdownReq.setTarget(Blaze::ShutdownTarget::SHUTDOWN_TARGET_LIST);
                mRequest.getInstanceIdList().copyInto(shutdownReq.getInstanceIds());
                break;
            default:
                shutdownReq.setTarget(Blaze::ShutdownTarget::SHUTDOWN_TARGET_ALL);
        }

        return commandErrorFromBlazeError(gController->shutdown(shutdownReq));
    }
} // Blaze
