/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/storage/storagemanager.h"
#include "framework/storage/storagelistener.h"
#include "framework/controller/controller.h"
#include "framework/system/fibermanager.h"

namespace Blaze
{

StorageListener::StorageListener(const SliverNamespace& sliverNamespace, MemoryGroupId memoryGroupId, int32_t logCategory) :
    mSliverNamespace(sliverNamespace),
    mStorageSnapshotAllocator(*Blaze::Allocator::getAllocator(memoryGroupId)),
    mLogCategory(logCategory),
    mSliverListener(sliverNamespace),
    mFieldHandlerList(BlazeStlAllocator("StorageListener::mFieldHandlerList", StorageManager::COMPONENT_MEMORY_GROUP)),
    mRecordMapBySliverIdMap(BlazeStlAllocator("StorageListener::mRecordMapBySliverIdMap", StorageManager::COMPONENT_MEMORY_GROUP)),
    mTotalUpdateVersGtCount(0),
    mTotalUpdateVersEqCount(0), // number of updates whose version was equal to that stored locally
    mTotalUpdateVersLtCount(0), // number of updates whose versions was lower than that stored locally
    mIsRegistered(false)
{
    mFiberOptions.blocking = false;
    mFiberOptions.stackSize = Fiber::STACK_LARGE;
}

StorageListener::~StorageListener()
{
}

void StorageListener::registerFieldHandler(FieldHandler& fieldHandler)
{
    EA_ASSERT_MSG(!mIsRegistered, "Listener is already registered to receive updates, some may have been missed!");

    if (!isValidFieldHandler(fieldHandler))
        return;

    // construct a sorted set of existing, unique, non-overlapping field prefixes
    StorageFieldPrefixSet fieldPrefixSet;
    fieldPrefixSet.reserve(mFieldHandlerList.size());
    for (StorageFieldHandlerList::const_iterator i = mFieldHandlerList.begin(), e = mFieldHandlerList.end(); i != e; ++i)
        fieldPrefixSet.insert(i->fieldNamePrefix);

    const char8_t* overlappedPrefix = addNonOverlappingStorageFieldPrefixToSet(fieldPrefixSet, fieldHandler.fieldNamePrefix);
    if (overlappedPrefix == nullptr)
    {
        mFieldHandlerList.push_back(fieldHandler);
    }
    else
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].registerFieldHandlers: "
            "Failed to register field handler. New prefix: " << fieldHandler.fieldNamePrefix << " overlaps existing prefix: " 
            << overlappedPrefix);
    }
}

void StorageListener::registerRecordHandler(RecordHandler& recordHandler)
{
    EA_ASSERT_MSG(!mIsRegistered, "Listener is already registered to receive updates, some may have been missed!");
    if (!isValidRecordHandler(recordHandler))
        return;

    mRecordHandler = recordHandler;
}

StorageRecordVersion StorageListener::getRecordVersion(StorageRecordId recordId) const
{
    StorageRecordVersion version = 0;
    SliverId sliverId = GetSliverIdFromSliverKey(recordId);
    auto recordMapItr = mRecordMapBySliverIdMap.find(sliverId);
    if (recordMapItr != mRecordMapBySliverIdMap.end())
    {
        auto& recordMap = recordMapItr->second;
        auto recordItr = recordMap.find(recordId);
        if (recordItr != recordMap.end())
        {
            version = recordItr->second.recordVersion;
        }
    }
    return version;
}

void StorageListener::onSyncSliver(SliverPtr sliver, BlazeRpcError& err)
{
    err = ERR_OK;

    SliverId sliverId = sliver->getSliverId();
    EA_ASSERT(sliverId != INVALID_SLIVER_ID);

    InstanceId instanceId = sliver->getSliverInstanceId();
    if (instanceId == INVALID_INSTANCE_ID)
    {
        BLAZE_TRACE_LOG(gStorageManager->getLogCategory(), "[StorageListener].onSyncSliver: "
            "Skipping sync sliver: " << sliverId << " from table: " << getSliverNamespaceStr() << ", currently not owned by any instance.");
        return; // no instance owns this sliver right now
    }

    SyncStorageSliverRequest req;
    req.setInstanceId(gController->getInstanceId());
    req.setSliverNamespace(mSliverNamespace);
    req.setSliverId(sliverId);

    // construct a group of unique prefix handlers
    StorageFieldPrefixSet groupFieldPrefixSet;
    for (StorageFieldHandlerList::const_iterator fieldHandlerItr = mFieldHandlerList.begin(), 
        fieldHandlerEnd = mFieldHandlerList.end(); fieldHandlerItr != fieldHandlerEnd; ++fieldHandlerItr)
    {
        groupFieldPrefixSet.insert(fieldHandlerItr->fieldNamePrefix);
    }

    EA_ASSERT_MSG(!groupFieldPrefixSet.empty(), "Storage listener must specify fields!");

    for (StorageFieldPrefixSet::const_iterator i = groupFieldPrefixSet.begin(), e = groupFieldPrefixSet.end(); i != e; ++i)
    {
        req.getFieldPrefixes().push_back(*i);
    }

    RecordMapBySliverIdMap::const_iterator recordMapItr = mRecordMapBySliverIdMap.find(sliverId);
    if (recordMapItr != mRecordMapBySliverIdMap.end())
    {
        req.getHaveRecordIds().reserve(recordMapItr->second.size());
        req.getHaveRecordVersions().reserve(recordMapItr->second.size());

        // submit all the records available locally
        for (RecordMap::const_iterator i = recordMapItr->second.begin(), e = recordMapItr->second.end(); i != e; ++i)
        {
            req.getHaveRecordIds().push_back(i->first);
            req.getHaveRecordVersions().push_back(i->second.recordVersion);
        }
    }

    RpcCallOptions options;
    options.routeTo.setInstanceId(instanceId);
    err = gStorageManager->syncStorageSliver(req, options);
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(mLogCategory, "[StorageListener].onSyncSliver: failed to sync storage sliver: " << sliverId << " for namespace: " << getSliverNamespaceStr() << ", err = " << ErrorHelp::getErrorName(err));
    }
}

void StorageListener::onDesyncSliver(SliverPtr sliver, BlazeRpcError& err)
{
    err = ERR_OK;

    SliverId sliverId = sliver->getSliverId();

    if (sliverId == INVALID_SLIVER_ID)
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].onDesyncSliver: Attempted to desync from invalid sliver for namespace: " << getSliverNamespaceStr());
        return;
    }

    InstanceId instanceId = sliver->getSliverInstanceId();
    if (instanceId != INVALID_INSTANCE_ID)
    {
        UnsyncStorageSliverRequest req;
        req.setInstanceId(gController->getInstanceId());
        req.setSliverNamespace(mSliverNamespace);
        req.setSliverId(sliverId);

        RpcCallOptions options;
        options.routeTo.setInstanceId(instanceId);
        err = gStorageManager->unsyncStorageSliver(req, options);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(mLogCategory, "[StorageListener].onDesyncSliver: failed to unsync storage sliver: " << sliverId << " for namespace: " << getSliverNamespaceStr() << ", err = " << ErrorHelp::getErrorName(err))
        }
    }

    RecordMapBySliverIdMap::iterator recordMapsIt = mRecordMapBySliverIdMap.find(sliverId);
    if (recordMapsIt != mRecordMapBySliverIdMap.end())
    {
        Fiber::CreateParams fiberParams = mFiberOptions;
        const RecordMap& recordMap = recordMapsIt->second;
        // sweep all the records since this sliver is being detached without replacement
        for (RecordMap::const_iterator recordMapIt = recordMap.begin(), recordMapEnd = recordMap.end(); recordMapIt != recordMapEnd; ++recordMapIt)
        {
            fiberParams.namedContext = mRecordHandler.eraseRecordFiberContext;
            gFiberManager->scheduleCall(this, &StorageListener::onEraseRecord, recordMapIt->first, fiberParams);
        }

        mRecordMapBySliverIdMap.erase(sliverId);
    }
}

void StorageListener::processRecordUpdate(const StorageRecordSnapshot& snapshot)
{
    SliverId sliverId = GetSliverIdFromSliverKey(snapshot.recordId);
    RecordMapBySliverIdMap::insert_return_type ret = mRecordMapBySliverIdMap.insert(sliverId);
    if (ret.second)
    {
        BlazeStlAllocator alloc = mRecordMapBySliverIdMap.get_allocator();
        alloc.set_name("StorageListener::RecordMap");
        ret.first->second.set_allocator(alloc);
    }
    RecordMap::insert_return_type recRet = ret.first->second.insert(snapshot.recordId);
    bool inserted = recRet.second;
    RecordEntry& recordEntry = recRet.first->second;

    if (inserted)
    {
        recordEntry.recordId = snapshot.recordId;
        ++mTotalUpdateVersGtCount;
    }
    else if (snapshot.recordVersion == recordEntry.recordVersion)
    {
        ++mTotalUpdateVersEqCount; // grow when master replicates unnecessary data due to race conditions that can occur when listeners register to master during sliver migration
        return; // ignore update
    }
    else if (snapshot.recordVersion < recordEntry.recordVersion)
        ++mTotalUpdateVersLtCount; // grow when master has to roll back records when being restarted after losing backing store (redis down)

    recordEntry.recordVersion = snapshot.recordVersion;

    typedef eastl::vector_set<ptrdiff_t> FieldIndexSet;
    FieldIndexSet changedFields, erasedFields, addedFields;
    changedFields.reserve(snapshot.fieldMap.size());
    erasedFields.reserve(snapshot.fieldMap.size());
    addedFields.reserve(snapshot.fieldMap.size());

    Fiber::CreateParams fiberParams = mFiberOptions;

    for (StorageFieldHandlerList::const_iterator fieldHandlerItr = mFieldHandlerList.begin(), 
        fieldHandlerEnd = mFieldHandlerList.end(); fieldHandlerItr != fieldHandlerEnd; ++fieldHandlerItr)
    {
        // find field entry in the snapshot
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr = getStorageFieldByPrefix(snapshot.fieldMap, fieldHandlerItr->fieldNamePrefix);
        if (snapFieldItr == snapshot.fieldMap.end())
            continue;
        do
        {
            // There is a deliberate policy whereby the handlers are always invoked before the corresponding record update is committed to the local
            // record cache. The upsert field handlers are passed the incoming field value which enables them to perform a delta state computation by looking up
            // existing state and comparing it to the new passed in state. The erase field handlers are passed the existing current field value for convenience
            // since erase handling often involves removing of existing secondary indexes built upon the data in the field about to be removed.
            const ptrdiff_t snapshotFieldIndex = snapFieldItr - snapshot.fieldMap.begin();
            bool found = false;
            if (!inserted)
            {
                // this finds a matching record's field entry
                FieldEntryMap::iterator recFieldItr = recordEntry.fieldMap.find(snapFieldItr->first);
                if (recFieldItr != recordEntry.fieldMap.end())
                {
                    // we are updating a record
                    // handle upserted field
                    if (snapFieldItr->second != nullptr)
                    {
                        // handle upserted field
                        if (fieldHandlerItr->upsertFieldCb.isValid())
                        {
                            fiberParams.namedContext = fieldHandlerItr->upsertFieldFiberContext;
                            gFiberManager->scheduleCall(this, &StorageListener::onUpsertField, fieldHandlerItr, snapshot.recordId, snapFieldItr->first.c_str(), snapFieldItr->second.get()/*new value*/, fiberParams);
                        }
                        changedFields.insert(snapshotFieldIndex);
                    }
                    else
                    {
                        // handle removed field, pass up current field value
                        if (fieldHandlerItr->eraseFieldCb.isValid())
                        {
                            fiberParams.namedContext = fieldHandlerItr->eraseFieldFiberContext;
                            gFiberManager->scheduleCall(this, &StorageListener::onEraseField, fieldHandlerItr, snapshot.recordId, snapFieldItr->first.c_str(), recFieldItr->second.fieldValue.get()/*old value*/, fiberParams);
                        }
                        erasedFields.insert(snapshotFieldIndex);
                    }
                    found = true;
                }
            }

            if (!found)
            {
                if (snapFieldItr->second != nullptr)
                {
                    // handle field addition
                    if (fieldHandlerItr->upsertFieldCb.isValid())
                    {
                        fiberParams.namedContext = fieldHandlerItr->upsertFieldFiberContext;
                        gFiberManager->scheduleCall(this, &StorageListener::onUpsertField, fieldHandlerItr, snapshot.recordId, snapFieldItr->first.c_str(), snapFieldItr->second.get()/*new value*/, fiberParams);
                    }
                    addedFields.insert(snapshotFieldIndex);
                }
                // else nop non-existing field removal
            }
            ++snapFieldItr;
            if (snapFieldItr == snapshot.fieldMap.end())
                break;
        }
        while (blaze_strncmp(snapFieldItr->first.c_str(), fieldHandlerItr->fieldNamePrefix, strlen(fieldHandlerItr->fieldNamePrefix)) == 0);
    }

    for (FieldIndexSet::iterator i = erasedFields.begin(), e = erasedFields.end(); i != e; ++i)
    {
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr = snapshot.fieldMap.begin() + *i;
        recordEntry.fieldMap.erase(snapFieldItr->first);
    }

    for (FieldIndexSet::iterator i = changedFields.begin(), e = changedFields.end(); i != e; ++i)
    {
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr = snapshot.fieldMap.begin() + *i;
        FieldEntryMap::iterator recFieldItr = recordEntry.fieldMap.find(snapFieldItr->first);
        if (recFieldItr != recordEntry.fieldMap.end())
        {
            recFieldItr->second.fieldValue = snapFieldItr->second;
        }
        else
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].processRecordUpdate: Changed field " << snapFieldItr->first.c_str() <<  " must exist! ");
        }
    }

    for (FieldIndexSet::iterator i = addedFields.begin(), e = addedFields.end(); i != e; ++i)
    {
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr = snapshot.fieldMap.begin() + *i;
        FieldEntryMap::insert_return_type feRet = recordEntry.fieldMap.insert(snapFieldItr->first);
        if (feRet.second)
        {
            feRet.first->second.fieldValue = snapFieldItr->second;
        }
        else
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].processRecordUpdate: Added field " << snapFieldItr->first.c_str() <<  " must not exist! ");
        }
    }
}

void StorageListener::processRecordErase(StorageRecordId recordId)
{
    SliverId sliverId = GetSliverIdFromSliverKey(recordId);
    RecordMapBySliverIdMap::iterator recordMapItr = mRecordMapBySliverIdMap.find(sliverId);
    if (recordMapItr != mRecordMapBySliverIdMap.end())
    {
        Fiber::CreateParams fiberParams = mFiberOptions;
        RecordMap& recordMap = recordMapItr->second;
        RecordMap::iterator recordItr = recordMap.find(recordId);
        if (recordItr != recordMap.end())
        {
            fiberParams.namedContext = mRecordHandler.eraseRecordFiberContext;
            gFiberManager->scheduleCall(this, &StorageListener::onEraseRecord, recordId, fiberParams);

            recordMap.erase(recordId); // remove the cached record entry and all its fields
        }
        if (recordMap.empty())
            mRecordMapBySliverIdMap.erase(sliverId); // clean up empty map
    }
}

const char8_t* StorageListener::getSliverNamespaceStr() const
{
    return BlazeRpcComponentDb::getComponentNameById(mSliverNamespace);
}

void StorageListener::onUpsertField(const FieldHandler* fieldHandler, StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf* newValue)
{
    fieldHandler->upsertFieldCb(recordId, fieldName, *newValue);
}

void StorageListener::onEraseField(const FieldHandler* fieldHandler, StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf* oldValue)
{
    fieldHandler->eraseFieldCb(recordId, fieldName, *oldValue);
}

void StorageListener::onEraseRecord(StorageRecordId recordId)
{
    mRecordHandler.eraseRecordCb(recordId);
}

bool StorageListener::isValidFieldHandler(FieldHandler& handler) const
{
    if (handler.fieldNamePrefix[0] == '\0')
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidFieldHandler: Invalid field name prefix! ");
        return false;
    }

    if (handler.upsertFieldCb.isValid())
    {
        if (handler.eraseFieldFiberContext[0] == '\0')
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidFieldHandler: Invalid upsert field named context!");
            return false;
        }
    }
    else if (handler.eraseFieldCb.isValid())
    {
        if (handler.eraseFieldFiberContext[0] == '\0')
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidFieldHandler: Invalid erase field named context!");
            return false;
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidFieldHandler: Both upsert and erase callbacks are invalid!");
        return false;
    }

    return true;
}

bool StorageListener::isValidRecordHandler(RecordHandler& handler) const
{
    if (!handler.eraseRecordCb.isValid())
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidRecordHandler: Invalid erase record callback!");
        return false;
    }
    else if (handler.eraseRecordFiberContext[0] == '\0')
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageListener].isValidFieldHandler: Invalid erase record fiber context!");
        return false;
    }

    return true;
}


} // namespace Blaze

