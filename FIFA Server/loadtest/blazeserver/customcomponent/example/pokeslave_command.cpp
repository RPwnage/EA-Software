/*************************************************************************************************/
/*!
\file   pokeslave_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class PokeSlaveCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "example/rpc/exampleslave/poke_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class PokeCommand : public PokeCommandStub
        {
        public:
            PokeCommand(Message* message, ExampleRequest* request, ExampleSlaveImpl* componentImpl)
                : PokeCommandStub(message, request), mComponent(componentImpl)
            { }

            ~PokeCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            PokeCommandStub::Errors execute() override
            {
                TRACE_LOG("[PokeCommand].execute() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() << ")");

                TRACE_LOG("[PokeCommand].execute() - " << mRequest);

                ExampleMaster* master = mComponent->getMaster();
                BlazeRpcError errorCode = master->pokeMaster(mRequest, mResponse);

                TRACE_LOG("[PokeCommand].gotResponse()");

                // Response
                if (errorCode == Blaze::ERR_OK)
                {
                    mResponse.setInstanceName(gController->getInstanceName());
                    return ERR_OK; 
                }
                // Error.
                else
                {
                    WARN_LOG("[PokeCommand].gotResponse() - Master replied with error (" << (ErrorHelp::getErrorName(errorCode)) 
                             << "): " << (ErrorHelp::getErrorDescription(errorCode)));
                    mErrorResponse.setMessage(ErrorHelp::getErrorDescription(errorCode));
                    return commandErrorFromBlazeError(errorCode);
                }
            }

        private:
            // Not owned memory.
            ExampleSlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_POKE_CREATE()

    } // Example
} // Blaze
