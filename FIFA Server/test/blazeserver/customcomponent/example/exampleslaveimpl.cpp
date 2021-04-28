/*************************************************************************************************/
/*!
    \file   exampleslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ExampleSlaveImpl

    Example Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "exampleslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"

#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/storage/storagemanager.h"
#include "framework/uniqueid/uniqueidmanager.h"

// example includes
#include "example/rpc/examplemaster.h"
#include "example/tdf/exampletypes.h"

namespace Blaze
{
namespace Example
{

static const int32_t EXAMPLEITEM_IDTYPE = 0;

// static
ExampleSlave* ExampleSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ExampleSlaveImpl") ExampleSlaveImpl();
}


/*** Public Methods ******************************************************************************/
ExampleSlaveImpl::ExampleSlaveImpl()
    : mExampleItemDataTable(ExampleSlave::COMPONENT_ID, ExampleSlave::COMPONENT_MEMORY_GROUP, ExampleSlave::LOGGING_CATEGORY)
{
    EA_ASSERT(gExampleSlave == nullptr);
    gExampleSlave = this;
}

ExampleSlaveImpl::~ExampleSlaveImpl()
{
    gExampleSlave = nullptr;
}

bool ExampleSlaveImpl::onConfigure()
{
    if (gUniqueIdManager->reserveIdType(ExampleSlave::COMPONENT_ID, EXAMPLEITEM_IDTYPE) != ERR_OK)
    {
        return false;
    }

    TRACE_LOG("[ExampleSlaveImpl].configure: configuring component.");
    return true;
}

bool ExampleSlaveImpl::onResolve()
{
    mExampleItemDataTable.registerFieldPrefixes(VALID_EXAMPLE_ITEM_FIELD_PREFIXES);
    mExampleItemDataTable.registerRecordHandlers(
        ImportStorageRecordCb(this, &ExampleSlaveImpl::onImportStorageRecord),
        ExportStorageRecordCb(this, &ExampleSlaveImpl::onExportStorageRecord));
    mExampleItemDataTable.registerRecordCommitHandler(CommitStorageRecordCb(this, &ExampleSlaveImpl::onCommitStorageRecord));

    gStorageManager->registerStorageTable(mExampleItemDataTable);

    return true;
}

RequestToUserSessionOwnerError::Error ExampleSlaveImpl::processRequestToUserSessionOwner(const Message* message)
{
    // This RPC is called by our stress testing client.  It is meant to invoke a short delay, and issue in internal
    // request back to the coreSlave that owns the current UserSession, in order to test auxCluster to coreSlave communication
    // during a stress test.
    Fiber::sleep(100 * 1000, "ExampleSlaveImpl::processRequestToUserSessionOwner");

    UserSessionDataPtr userSession;
    BlazeRpcError err = gCurrentUserSession->fetchSessionData(userSession);

    UserInfoPtr userInfo;
    err = gCurrentUserSession->fetchUserInfo(userInfo);
    TRACE_LOG("ExampleSlaveImpl::processRequestToUserSessionOwner: called by " << gCurrentUserSession->getBlazeId() << " persona=" << (userInfo == nullptr ? "null" : userInfo->getPersonaName()));

    return RequestToUserSessionOwnerError::commandErrorFromBlazeError(err);
}


const char8_t PUBLIC_EXAMPLE_ITEM_FIELD[] = "pub.exampleItem";
const char8_t PRIVATE_EXAMPLE_ITEM_FIELD[] = "pri.exampleItem";
const char8_t* VALID_EXAMPLE_ITEM_FIELD_PREFIXES[] = { PUBLIC_EXAMPLE_ITEM_FIELD, PRIVATE_EXAMPLE_ITEM_FIELD, nullptr };

EA_THREAD_LOCAL ExampleSlaveImpl* gExampleSlave = nullptr;

void intrusive_ptr_add_ref(ExampleItem* ptr)
{
    if (ptr->mRefCount == 1)
    {
        // pin the sliver from migrating because we 'may' be writing the game object back
        bool result = ptr->mOwnedSliverRef->getPrioritySliverAccess(ptr->mOwnedSliverAccessRef);
        EA_ASSERT_MSG(result, "Failed to obtain owned sliver access!");
    }
    ++ptr->mRefCount;
}

void intrusive_ptr_release(ExampleItem* ptr)
{
    if (ptr->mRefCount > 0)
    {
        --ptr->mRefCount;
        if (ptr->mRefCount == 0)
            delete ptr;
        else if (ptr->mRefCount == 1)
        {
            if (ptr->mAutoCommit)
            {
                const uint64_t id = ptr->getExampleItemId();
                const ExampleItem* exampleItem = gExampleSlave->getReadOnlyExampleItem(id);
                if (exampleItem == ptr)
                {
                    // This means we are returning the ExampleItem back to the container, trigger a writeback to storage
                    gExampleSlave->getExampleItemDataTable().upsertField(id, PRIVATE_EXAMPLE_ITEM_FIELD, *ptr->mExampleItemData);
                }
                ptr->mAutoCommit = false;
            }
            // unpin the sliver and enable it to migrate once more (once the pending upserts have been flushed)
            ptr->mOwnedSliverAccessRef.release();
        }
    }
}


ExampleItem::ExampleItem(uint64_t id, const OwnedSliverRef& ownedSliverRef)
    : ExampleItem(id, BLAZE_NEW ExampleItemData(), ownedSliverRef)
{
}

ExampleItem::ExampleItem(uint64_t id, const ExampleItemDataPtr& exampleItemData, const OwnedSliverRef& ownedSliverRef)
    : mAutoCommit(false)
    , mRefCount(0)
    , mOwnedSliverRef(ownedSliverRef)
    , mExampleItemId(id)
    , mExampleItemData(exampleItemData)
    , mTimeToLiveTimerId(INVALID_TIMER_ID)
    , mBroadcastMessageTimerId(INVALID_TIMER_ID)
{
}


uint64_t ExampleSlaveImpl::getNextUniqueId()
{
    uint64_t id = 0;
    gUniqueIdManager->getId(ExampleSlave::COMPONENT_ID, EXAMPLEITEM_IDTYPE, id);
    return id;
}

ExampleItem* ExampleSlaveImpl::getExampleItem(uint64_t id) const
{
    ExampleItem* exampleItem = nullptr;
    ExampleItemPtrByIdMap::const_iterator it = mExampleItemPtrByIdMap.find(id);
    if (it != mExampleItemPtrByIdMap.end())
        exampleItem = it->second.get();
    return exampleItem;
}

const ExampleItem* ExampleSlaveImpl::getReadOnlyExampleItem(uint64_t id) const
{
    return getExampleItem(id);
}

ExampleItemPtr ExampleSlaveImpl::getWritableExampleItem(uint64_t id, bool autoCommit)
{
    ExampleItem* exampleItem = getExampleItem(id);
    if (autoCommit && (exampleItem != nullptr))
        exampleItem->setAutoCommitOnRelease();
    return exampleItem;
}

void ExampleSlaveImpl::eraseExampleItem(uint64_t id)
{
    ExampleItemPtrByIdMap::const_iterator it = mExampleItemPtrByIdMap.find(id);
    if (it != mExampleItemPtrByIdMap.end())
    {
        ExampleItemPtr exampleItem = it->second;
        mExampleItemPtrByIdMap.erase(id);
        mExampleItemDataTable.eraseRecord(id);
    }
}

void ExampleSlaveImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
{
    StorageRecordSnapshot::FieldEntryMap::const_iterator dataField;

    dataField = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_EXAMPLE_ITEM_FIELD);
    if (dataField == snapshot.fieldMap.end())
    {
        ERR_LOG("ExampleSlaveImpl.onImportStorageRecord: Field prefix=" << PRIVATE_EXAMPLE_ITEM_FIELD << " matches no fields in exampleItemDataTable record(" << snapshot.recordId << ").");
        mExampleItemDataTable.eraseRecord(snapshot.recordId);
        return;
    }

    ExampleItemDataPtr exampleItemData = static_cast<ExampleItemData*>(dataField->second.get());

    ExampleItemPtrByIdMap::insert_return_type inserted = mExampleItemPtrByIdMap.insert(snapshot.recordId);
    if (!inserted.second)
    {
        ASSERT_LOG("ExampleSlaveImpl.onImportStorageRecord: A ExampleItemData with id(" << snapshot.recordId << ") already exists.");
        return;
    }

    ExampleItem* exampleItem = BLAZE_NEW ExampleItem(snapshot.recordId, exampleItemData, &ownedSliver);
    inserted.first->second = exampleItem;

    TimerId timeToLiveTimerId = gSelector->scheduleFiberTimerCall(exampleItem->getData().getAbsoluteTimeToLive(), this, &ExampleSlaveImpl::exampleItemTimeToLiveHandler,
        exampleItem->getExampleItemId(), "ExampleSlaveImpl::exampleItemTimeToLiveHandler");
    exampleItem->setTimeToLiveTimerId(timeToLiveTimerId);

    TimerId broadcastMessageTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (5 * 1000000), gExampleSlave, &ExampleSlaveImpl::exampleItemBroadcastMessageHandler,
        exampleItem->getExampleItemId(), "ExampleSlaveImpl::exampleItemBroadcastMessageHandler");
    exampleItem->setBroadcastMessageTimerId(broadcastMessageTimerId);
}

void ExampleSlaveImpl::onExportStorageRecord(StorageRecordId recordId)
{
    ExampleItemPtrByIdMap::iterator it = mExampleItemPtrByIdMap.find(recordId);
    if (it == mExampleItemPtrByIdMap.end())
    {
        ASSERT_LOG("ExampleSlaveImpl.onExportStorageRecord: An ExampleItem with id(" << recordId << ") does not exist.");
        return;
    }

    // NOTE: Intentionally *not* using ExampleItemPtr here because it would attempt to aquire sliver access while sliver is locked for migration, which we want to avoid.
    // It is always safe to delete the ExampleItem object after export, because export is only triggered when there are no more outstanding sliver accesses on this sliver.
    ExampleItem* exampleItem = it->second.detach();
    mExampleItemPtrByIdMap.erase(it);

    // Cancel all scheduled timers for this object
    gSelector->cancelTimer(exampleItem->getTimeToLiveTimerId());
    gSelector->cancelTimer(exampleItem->getBroadcastMessageTimerId());

    delete exampleItem;
}

void ExampleSlaveImpl::onCommitStorageRecord(StorageRecordId recordId)
{
}

void ExampleSlaveImpl::exampleItemTimeToLiveHandler(uint64_t exampleItemId)
{
    INFO_LOG("ExampleSlaveImpl.exampleItemTimeToLiveHandler: getting writable ExampleItem with id " << exampleItemId);

    // This will pin the sliver associated with the game report (logic found in 'void intrusive_ptr_add_ref(ExampleItem* ptr)')
    ExampleItemPtr exampleItem = getWritableExampleItem(exampleItemId);
    if (exampleItem == nullptr)
    {
        ERR_LOG("ExampleSlaveImpl.exampleItemTimeToLiveHandler: exampleItemId " << exampleItemId << " already removed -- this should not have happened");
        return;
    }

    // Cancel all scheduled timers for this object
    gSelector->cancelTimer(exampleItem->getTimeToLiveTimerId());
    gSelector->cancelTimer(exampleItem->getBroadcastMessageTimerId());

    eraseExampleItem(exampleItem->getExampleItemId());
}

void ExampleSlaveImpl::exampleItemBroadcastMessageHandler(uint64_t exampleItemId)
{
    // This will pin the sliver associated with the game report (logic found in 'void intrusive_ptr_add_ref(ExampleItem* ptr)')
    ExampleItemPtr exampleItem = getWritableExampleItem(exampleItemId);
    if (exampleItem == nullptr)
    {
        ERR_LOG("ExampleSlaveImpl.exampleItemTimeToLiveHandler: exampleItemId " << exampleItemId << " already removed -- this should not have happened");
        return;
    }

    for (auto id : exampleItem->getData().getSubscribers())
    {
        ExampleItemState exampleItemState;
        exampleItemState.setStateMessage(exampleItem->getData().getStateMessage());

        sendNotifyExampleItemBroadcastToUserSessionById(id, &exampleItemState);
    }

    TimerId broadcastMessageTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (5 * 1000000), gExampleSlave, &ExampleSlaveImpl::exampleItemBroadcastMessageHandler,
        exampleItem->getExampleItemId(), "ExampleSlaveImpl::exampleItemBroadcastMessageHandler");
    exampleItem->setBroadcastMessageTimerId(broadcastMessageTimerId);
}

} // Example
} // Blaze
