/*************************************************************************************************/
/*!
\file   readexampleitem_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ReadExampleItemCommand

ReadExampleItem the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "example/rpc/exampleslave/readexampleitem_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class ReadExampleItemCommand : public ReadExampleItemCommandStub
        {
        public:
            ReadExampleItemCommand(Message* message, ReadExampleItemRequest* request, ExampleSlave* componentImpl)
                : ReadExampleItemCommandStub(message, request)
            { }

            ~ReadExampleItemCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            ReadExampleItemCommandStub::Errors execute() override
            {
                const ExampleItem* exampleItem = gExampleSlave->getReadOnlyExampleItem(mRequest.getExampleItemId());
                if (exampleItem == nullptr)
                {
                    WARN_LOG("ReadExampleItemCommand.execute: Could not find an ExampleItem with id " << mRequest.getExampleItemId());
                    return ERR_SYSTEM;
                }

                mResponse.setStateMessage(exampleItem->getData().getStateMessage());

                INFO_LOG("ReadExampleItemCommand.execute: Finished reading ExampleItem with id " << exampleItem->getExampleItemId());
                return ERR_OK;
            }
        };


        // static factory method impl
        DEFINE_READEXAMPLEITEM_CREATE()

    } // Example
} // Blaze
