/*************************************************************************************************/
/*!
\file   updateexampleitem_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class UpdateExampleItemCommand

UpdateExampleItem the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "example/rpc/exampleslave/updateexampleitem_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class UpdateExampleItemCommand : public UpdateExampleItemCommandStub
        {
        public:
            UpdateExampleItemCommand(Message* message, UpdateExampleItemRequest* request, ExampleSlave* componentImpl)
                : UpdateExampleItemCommandStub(message, request)
            { }

            ~UpdateExampleItemCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            UpdateExampleItemCommandStub::Errors execute() override
            {
                ExampleItemPtr exampleItem = gExampleSlave->getWritableExampleItem(mRequest.getExampleItemId());
                if (exampleItem == nullptr)
                {
                    WARN_LOG("UpdateExampleItemCommand.execute: Could not find an ExampleItem with id " << mRequest.getExampleItemId());
                    return ERR_SYSTEM;
                }

                exampleItem->getData().setStateMessage(mRequest.getState());

                bool alreadySubscribed = false;
                for (auto id : exampleItem->getData().getSubscribers())
                {
                    if (id == UserSession::getCurrentUserSessionId())
                    {
                        alreadySubscribed = true;
                        break;
                    }
                }

                if (!alreadySubscribed)
                    exampleItem->getData().getSubscribers().push_back(UserSession::getCurrentUserSessionId());

                return ERR_OK;
            }
        };


        // static factory method impl
        DEFINE_UPDATEEXAMPLEITEM_CREATE()

    } // Example
} // Blaze
