/*************************************************************************************************/
/*!
    \file   completeremotetransaction_command.cpp


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
#include "framework/rpc/blazecontrollerslave/completeremotetransaction_stub.h"

#include "framework/controller/controller.h"
#include "framework/controller/transaction.h"


namespace Blaze
{
    class CompleteRemoteTransactionCommand : public CompleteRemoteTransactionCommandStub
    {
    public:
        CompleteRemoteTransactionCommand(Message *message, CompleteRemoteTransactionRequest *request, Controller *controller);
        ~CompleteRemoteTransactionCommand() override {}

        CompleteRemoteTransactionCommandStub::Errors execute() override;

    private:
        Controller *mController;
    };

    DEFINE_COMPLETEREMOTETRANSACTION_CREATE_COMPNAME(Controller);

    CompleteRemoteTransactionCommand::CompleteRemoteTransactionCommand(
        Message *message, 
        CompleteRemoteTransactionRequest *request, 
        Controller *controller) 
        : CompleteRemoteTransactionCommandStub(message, request), mController(controller)
    {
    }

    CompleteRemoteTransactionCommandStub::Errors CompleteRemoteTransactionCommand::execute()
    {
        
        // locate the component
        Component *component = mController->getComponent(mRequest.getComponentId(), false);
        if (component == nullptr)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, 
                "[Controller].CompleteRemoteTransactionCommand:"
                " Could not find component: " << BlazeRpcComponentDb::getComponentNameById(mRequest.getComponentId())
                << ", component id: " << mRequest.getComponentId());
            return ERR_SYSTEM;
        }

        // complete the transaction
        BlazeRpcError result = Blaze::ERR_OK;

        if (mRequest.getCommitTransaction())
            result = component->commitTransaction(mRequest.getTransactionId()); 
        else
            result = component->rollbackTransaction(mRequest.getTransactionId());

        return commandErrorFromBlazeError(result);
    }

} // Blaze
