/*************************************************************************************************/
/*!
\file   createexampleitem_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class CreateExampleItemCommand

CreateExampleItem the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "example/rpc/exampleslave/createexampleitem_stub.h"

// global includes

// example includes
#include "example/rpc/examplemaster.h"
#include "exampleslaveimpl.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
    namespace Example
    {

        class CreateExampleItemCommand : public CreateExampleItemCommandStub
        {
        public:
            CreateExampleItemCommand(Message* message, CreateExampleItemRequest* request, ExampleSlave* componentImpl)
                : CreateExampleItemCommandStub(message, request)
            { }

            ~CreateExampleItemCommand() override { }

            /* Private methods *******************************************************************************/
        private:

            CreateExampleItemCommandStub::Errors execute() override
            {
                OwnedSliverRef sliverRef = gExampleSlave->getExampleItemDataTable().getSliverOwner().getLeastLoadedOwnedSliver();
                if (sliverRef == nullptr)
                {
                    // This should never happen, SliverManager ensures that creation RPCs never get routed to an owner that has no ready slivers
                    ERR_LOG("CreateExampleItemCommand.execute: failed to obtain owned sliver ref.");
                    return ERR_SYSTEM;
                }

                Sliver::AccessRef sliverAccess;
                if (!sliverRef->getPrioritySliverAccess(sliverAccess))
                {
                    // This should never happen unless a blocking call was made after getting an OwnedSliver from a SliverOwner.
                    ERR_LOG("CreateExampleItemCommand.execute: Could not get priority sliver access for SliverId(" << sliverRef->getSliverId() << ")");
                    return ERR_SYSTEM;
                }

                uint64_t exampleItemId = BuildSliverKey(sliverRef->getSliverId(), gExampleSlave->getNextUniqueId());

                ExampleItemPtrByIdMap::insert_return_type inserted = gExampleSlave->getExampleItemPtrByIdMap().insert(exampleItemId);
                if (!inserted.second)
                {
                    // Unexpected!.. Something wrong with the UniqueIdGenerator algorithm? Log an error, ignore this request and carry on.
                    ERR_LOG("CreateExampleItemCommand.execute: Cannot create ExampleItem with id " << exampleItemId << " because an item with that id already exists.");
                    return ERR_SYSTEM;
                }

                ExampleItemPtr exampleItem = BLAZE_NEW ExampleItem(exampleItemId, sliverRef);
                inserted.first->second = exampleItem;

                // When this is set, it will trigger the ExampleItemPtr intrusive_ptr release function to commit the changes.
                exampleItem->setAutoCommitOnRelease();

                // Initialize the state of the new ExampleItem.
                exampleItem->getData().setStateMessage(mRequest.getState());
                exampleItem->getData().setAbsoluteTimeToLive(TimeValue::getTimeOfDay() + (mRequest.getTimeToLiveInSeconds() * 1000000));
                exampleItem->getData().getSubscribers().push_back(UserSession::getCurrentUserSessionId());

                TimerId timeToLiveTimerId = gSelector->scheduleFiberTimerCall(exampleItem->getData().getAbsoluteTimeToLive(), gExampleSlave, &ExampleSlaveImpl::exampleItemTimeToLiveHandler,
                    exampleItem->getExampleItemId(), "ExampleSlaveImpl::exampleItemTimeToLiveHandler");
                exampleItem->setTimeToLiveTimerId(timeToLiveTimerId);

                TimerId broadcastMessageTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (5 * 1000000), gExampleSlave, &ExampleSlaveImpl::exampleItemBroadcastMessageHandler,
                    exampleItem->getExampleItemId(), "ExampleSlaveImpl::exampleItemBroadcastMessageHandler");
                exampleItem->setBroadcastMessageTimerId(broadcastMessageTimerId);

                // Tell the caller we've done what they asked for.
                INFO_LOG("CreateExampleItemCommand.execute: Created ExampleItem with id " << exampleItem->getExampleItemId());

                mResponse.setExampleItemId(exampleItem->getExampleItemId());

                return ERR_OK;
            }
        };


        // static factory method impl
        DEFINE_CREATEEXAMPLEITEM_CREATE()

    } // Example
} // Blaze
