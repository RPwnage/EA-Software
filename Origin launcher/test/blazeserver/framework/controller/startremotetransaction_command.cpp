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
#include "framework/rpc/blazecontrollerslave/startremotetransaction_stub.h"

#include "framework/controller/controller.h"


namespace Blaze
{
    class StartRemoteTransactionCommand : public StartRemoteTransactionCommandStub
    {
    public:
        StartRemoteTransactionCommand(Message *message, StartRemoteTransactionRequest *request, Controller *controller);
        ~StartRemoteTransactionCommand() override {}

        StartRemoteTransactionCommandStub::Errors execute() override;

    private:
        Controller *mController;
    };

    DEFINE_STARTREMOTETRANSACTION_CREATE_COMPNAME(Controller);

    StartRemoteTransactionCommand::StartRemoteTransactionCommand(
        Message *message, 
        StartRemoteTransactionRequest *request, 
        Controller *controller) 
        : StartRemoteTransactionCommandStub(message, request), mController(controller)
    {
    }

    StartRemoteTransactionCommandStub::Errors StartRemoteTransactionCommand::execute()
    {
        
        // locate the component
        Component *component = mController->getComponent(mRequest.getComponentId(), false);
        if (component == nullptr)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, 
                "[Controller].StartRemoteTransactionCommand:"
                " Could not find component: " << BlazeRpcComponentDb::getComponentNameById(mRequest.getComponentId())
                << ", [id] " << mRequest.getComponentId());
            return ERR_SYSTEM;
        }

        // start the transaction
        TransactionContextHandlePtr contextHandlePtr; 
        BlazeRpcError result = component->startTransaction(contextHandlePtr, mRequest.getCustomData(), mRequest.getTimeout());

        if (result != Blaze::ERR_OK || contextHandlePtr == nullptr)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, 
                "[Controller].StartRemoteTransactionCommand:"
                " Unable to start transaction on component component: " << component->getName());
            return ERR_SYSTEM;
        }

        // this handle is remote and not bound to local fibers
        contextHandlePtr->clearOwningFiber();

        // this handle's lifetime will not be associated with the transaction
        contextHandlePtr->dissociate();

        mResponse.setTransactionId(*contextHandlePtr);

        return ERR_OK;
    }

} // Blaze
