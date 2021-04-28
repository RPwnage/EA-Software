/*************************************************************************************************/
/*!
\file   deleteexampleitem_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class DeleteExampleItemCommand

DeleteExampleItem the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "example/rpc/exampleslave/deleteexampleitem_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class DeleteExampleItemCommand : public DeleteExampleItemCommandStub
        {
        public:
            DeleteExampleItemCommand(Message* message, DeleteExampleItemRequest* request, ExampleSlave* componentImpl)
                : DeleteExampleItemCommandStub(message, request)
            { }

            ~DeleteExampleItemCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            DeleteExampleItemCommandStub::Errors execute() override
            {
                ExampleItemPtr exampleItem = gExampleSlave->getWritableExampleItem(mRequest.getExampleItemId());
                if (exampleItem == nullptr)
                {
                    WARN_LOG("DeleteExampleItemCommand.execute: Could not find an ExampleItem with id " << mRequest.getExampleItemId());
                    // They are asking for this object to be deleted, and we couldn't find it, so assume it's deleted, and return ERR_OK.
                    return ERR_OK;
                }

                // Cancel all scheduled timers for this object before erasing it.
                gSelector->cancelTimer(exampleItem->getTimeToLiveTimerId());
                gSelector->cancelTimer(exampleItem->getBroadcastMessageTimerId());

                gExampleSlave->eraseExampleItem(exampleItem->getExampleItemId());

                return ERR_OK;
            }
        };


        // static factory method impl
        DEFINE_DELETEEXAMPLEITEM_CREATE()

    } // Example
} // Blaze
