/*************************************************************************************************/
/*!
\file   pinexampleitem_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class PinExampleItemCommand

PinExampleItem the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "example/rpc/exampleslave/pinexampleitem_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class PinExampleItemCommand : public PinExampleItemCommandStub
        {
        public:
            PinExampleItemCommand(Message* message, PinExampleItemRequest* request, ExampleSlave* componentImpl)
                : PinExampleItemCommandStub(message, request)
            { }

            ~PinExampleItemCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            PinExampleItemCommandStub::Errors execute() override
            {
                ExampleItemPtr exampleItem = gExampleSlave->getWritableExampleItem(mRequest.getExampleItemId());
                if (exampleItem == nullptr)
                {
                    WARN_LOG("PinExampleItemCommand.execute: Could not find an ExampleItem with id " << mRequest.getExampleItemId());
                    return ERR_SYSTEM;
                }

                // The Blaze framework automatically obtains SliverAccess before calling command handlers, the sliver to which this
                // ExampleItem is bound will be prevented from migrating for the duration of the sleep.

                INFO_LOG("PinExampleItemCommand.execute: Sleeping for " << mRequest.getDurationInSeconds() << " seconds with ExampleItemId " << mRequest.getExampleItemId() << " pinned.");
                Fiber::sleep(mRequest.getDurationInSeconds() * 1000000, "PinExampleItemCommand::execute");
                INFO_LOG("PinExampleItemCommand.execute: Woke up from sleeping for " << mRequest.getDurationInSeconds() << " seconds with ExampleItemId " << mRequest.getExampleItemId() << " pinned.");

                return ERR_OK;
            }
        };


        // static factory method impl
        DEFINE_PINEXAMPLEITEM_CREATE()

    } // Example
} // Blaze
