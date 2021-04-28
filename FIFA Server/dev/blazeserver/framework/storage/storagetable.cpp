/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/redis/rediscommand.h"
#include "framework/redis/redismanager.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/storage/storagemanager.h"
#include "framework/storage/storagetable.h"

namespace Blaze
{

namespace Metrics
{
    namespace Tag
    {
        TagInfo<const char8_t*>* storage_prefix = BLAZE_NEW TagInfo<const char8_t*>("storage_prefix");
    }
}

// USAGE: <scriptbody> <numkeys> <hashmapkey> <num_fields_to_delete> <fields_to_delete> <field_value_pairs_to_upsert> 
// eg: eval "<scriptbody>" 1 myhash 3 field4 field5 field6 field1 value1 field2 value2 field3 value3
// tested working to delete and upsert (tested working with 0 fields to delete and with no upsertable field/value pairs)
// NOTE: the only issue with using unpack() is if the size/count of ARGV passed in is very large (eg: several MB)
// the script may fail because it will overflow the stack used by the Lua unpack() function.
RedisScript StorageTable::msUpdateRecordScript(1,
    "local delKeys = tonumber(ARGV[1]);"
    "if delKeys > 0 then"
    "  redis.call('HDEL', KEYS[1], unpack(ARGV,2,delKeys));"
    "end;"
    "if #ARGV > delKeys+1 then"
    "  redis.call('HMSET', KEYS[1], unpack(ARGV,delKeys+2));"
    "end;"
    "return redis.status_reply('OK');");


// Storage Table acts as a writethrough cache storing intrusive pointers to outgoing data that needs to be flushed to the
// backing store and replicated to any registered listeners.
// StorageTable acts as a server
// StorageListener acts as a client

StorageTable::StorageTable(const SliverNamespace& sliverNamespace, MemoryGroupId memoryGroupId, int32_t logCategory) : 
    mSliverNamespace(sliverNamespace),
    mSliverNamespaceStr(BlazeRpcComponentDb::getComponentNameById(mSliverNamespace)),
    mStorageSnapshotAllocator(*Blaze::Allocator::getAllocator(memoryGroupId)),
    mLogCategory(logCategory),
    mSliverOwner(sliverNamespace),
    mRecordKeyFieldMap(RedisId(mSliverNamespaceStr, BlazeRpcComponentDb::getComponentNameById(sliverNamespace)), true),
    mRecordBlockBySliverIdMap(BlazeStlAllocator("StorageTable::mRecordBlockBySliverIdMap", StorageManager::COMPONENT_MEMORY_GROUP)),
    mStorageWriterQueue("StorageTable::mStorageWriterQueue"),
    mPublicFieldPrefix(STORAGE_FIELD_PUBLIC_PREFIX),
    mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::sliver_namespace, mSliverNamespaceStr)),
    mFieldChanges(mMetricsCollection, "storage.fieldChanges", Metrics::Tag::fiber_context, Metrics::Tag::storage_prefix),
    mFieldUpdates(mMetricsCollection, "storage.fieldUpdates", Metrics::Tag::fiber_context, Metrics::Tag::storage_prefix),
    mFieldIgnores(mMetricsCollection, "storage.fieldIgnores", Metrics::Tag::fiber_context, Metrics::Tag::storage_prefix)

{
    mStorageWriterQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
}

StorageTable::~StorageTable()
{
    if (mStorageWriterQueue.hasPendingWork())
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].dtor: Table(" << getSliverNamespaceStr() << ") writer queue not empty.");
    }
    if (!mRecordBlockBySliverIdMap.empty())
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].dtor: Table(" << getSliverNamespaceStr() << ") contains unexported records.");
    }
}

void StorageTable::registerRecordHandlers(ImportStorageRecordCb importHandler, ExportStorageRecordCb exportHandler)
{
    EA_ASSERT(importHandler.isValid());
    EA_ASSERT(exportHandler.isValid());

    mImportRecordHandler = importHandler;
    mExportRecordHandler = exportHandler;
}

void StorageTable::registerFieldPrefixes(const char8_t** validFieldPrefixes)
{
    const char8_t** fieldPrefix = validFieldPrefixes;
    while (*fieldPrefix != nullptr)
    {
        const char8_t* overlappedPrefix = addNonOverlappingStorageFieldPrefixToSet(mValidFieldPrefixes, *fieldPrefix);
        if (overlappedPrefix != nullptr)
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].registerValidFieldPrefixes: Table(" << getSliverNamespaceStr() << ") "
                "failed to register field handler. New prefix: " << *fieldPrefix << " overlaps existing prefix: " 
                << overlappedPrefix);
            break;
        }
        
        ++fieldPrefix;
    }
}

void StorageTable::registerRecordCommitHandler(CommitStorageRecordCb commitHandler)
{
    EA_ASSERT(commitHandler.isValid());

    mCommitRecordHandler = commitHandler;
}

void StorageTable::upsertField(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue)
{
    const char8_t* fieldPrefix = getFieldPrefix(fieldName);
    EA_ASSERT_MSG(fieldPrefix != nullptr, "Field name must match a registered prefix!");
    EA_ASSERT_MSG(!fieldValue.isNotRefCounted(), "Field value must be ref-countable!");

    SliverId sliverId = GetSliverIdFromSliverKey(recordId);

    RecordBlockBySliverIdMap::iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr != mRecordBlockBySliverIdMap.end())
    {
        RecordBlock& recordBlock = *recordBlockItr->second;
        RecordMap::insert_return_type recRet = recordBlock.recordMap.insert(recordId);
        if (recRet.second)
        {
            if (!recordBlock.syncing)
            {
                recRet.first->second = BLAZE_NEW_MGID(StorageManager::COMPONENT_MEMORY_GROUP, "StorageTable::RecordEntry_Upsert") RecordEntry;
                recRet.first->second->recordId = recordId;
            }
            else
            {
                recordBlock.recordMap.erase(recordId);
                // Only known records can be changed during sync, otherwise records with yet unknown recordId could be synced subsequently thus silently losing changes thought to be applied locally.
                BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].upsertField: Table(" << getSliverNamespaceStr() << ") attempted to upsert field(" << fieldName << ") for an unsynced recordId(" << recordId << ") while syncing.");
                return;
            }
        }

        RecordEntry& recordEntry = *recRet.first->second;
        FieldEntryMap::insert_return_type fieldRet = recordEntry.fieldEntryMap.insert(fieldName);
        if (fieldRet.second)
            fieldRet.first->second.isPublic = blaze_strncmp(mPublicFieldPrefix.c_str(), fieldName, mPublicFieldPrefix.size()) == 0;
        fieldRet.first->second.fieldValue = &fieldValue;
        fieldRet.first->second.dirty = true;

        mFieldChanges.increment(1, Fiber::getCurrentContext(), fieldPrefix);

        TimeValue replicateAtTime = recordEntry.lastPublishTime + mSliverOwner.getUpdateCoalescePeriod();
        publishRecord(recordEntry, recordBlock, replicateAtTime);
    }
    else
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].upsertField: Table(" << getSliverNamespaceStr() << ") cannot upsert record field for a sliver that we do not own.");
    }
}

void StorageTable::eraseField(StorageRecordId recordId, const char8_t* fieldName)
{
    SliverId sliverId = GetSliverIdFromSliverKey(recordId);

    RecordBlockBySliverIdMap::iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr != mRecordBlockBySliverIdMap.end())
    {
        RecordBlock& recordBlock = *recordBlockItr->second;
        RecordMap::iterator recordEntryItr = recordBlock.recordMap.find(recordId);
        if (recordEntryItr != recordBlock.recordMap.end())
        {
            RecordEntry& recordEntry = *recordEntryItr->second;
            FieldEntryMap::iterator fieldItr = recordEntry.fieldEntryMap.find(fieldName);
            if (fieldItr != recordEntry.fieldEntryMap.end())
            {
                fieldItr->second.fieldValue.reset(); // clear the field marking it for erase
                fieldItr->second.dirty = true;

                TimeValue replicateAtTime = recordEntry.lastPublishTime + mSliverOwner.getUpdateCoalescePeriod();
                publishRecord(recordEntry, recordBlock, replicateAtTime);
            }
        }
        else if (recordBlock.syncing)
        {
            // Only known records can be changed during sync, otherwise records with yet unknown recordId could be synced subsequently thus silently losing changes thought to be applied locally.
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].eraseRecord: Table(" << getSliverNamespaceStr() << ") attempted to erase field(" << fieldName << ") from an unsynced recordId(" << recordId << ") while syncing.");
            return;
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].upsertField: Table(" << getSliverNamespaceStr() << ") cannot erase record field for a sliver that we do not own.");
    }
}

void StorageTable::eraseRecord(StorageRecordId recordId)
{
    SliverId sliverId = GetSliverIdFromSliverKey(recordId);

    RecordBlockBySliverIdMap::iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr != mRecordBlockBySliverIdMap.end())
    {
        RecordBlock& recordBlock = *recordBlockItr->second;

        RecordMap::iterator recordEntryItr = recordBlock.recordMap.find(recordId);
        if (recordEntryItr != recordBlock.recordMap.end())
        {
            RecordEntryPtr recordEntry = recordEntryItr->second;
            recordEntry->erased = true;
            recordBlock.recordMap.erase(recordId); // pull out record from the map to disallow all access
            publishRecord(*recordEntry, recordBlock, TimeValue::getTimeOfDay()); // erase is scheduled immediately
        }
        else if (recordBlock.syncing)
        {
            // Only known records can be changed during sync, otherwise records with yet unknown recordId could be synced subsequently thus silently losing changes thought to be applied locally.
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].eraseRecord: Table(" << getSliverNamespaceStr() << ") attempted to erase unsynced recordId(" << recordId << ") while syncing.");
            return;
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].upsertField: Table(" << getSliverNamespaceStr() << ") cannot erase record for a sliver that we do not own.");
    }
}

StorageRecordVersion StorageTable::getRecordVersion(StorageRecordId recordId)
{
    StorageRecordVersion recordVersion = 0;
    SliverId sliverId = GetSliverIdFromSliverKey(recordId);

    RecordBlockBySliverIdMap::iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr != mRecordBlockBySliverIdMap.end())
    {
        RecordBlock& recordBlock = *recordBlockItr->second;

        RecordMap::iterator recordEntryItr = recordBlock.recordMap.find(recordId);
        if (recordEntryItr != recordBlock.recordMap.end())
        {
            recordVersion = recordEntryItr->second->recordVersion;
        }
    }

    return recordVersion;
}

RawBufferPtr StorageTable::encodeFieldValue(const StorageFieldName& fieldName, const EA::TDF::Tdf& fieldValue) const
{
    RawBufferPtr buffer = BLAZE_NEW_MGID(StorageManager::COMPONENT_MEMORY_GROUP, "StorageTable::RawBuffer_Encode") RawBuffer(1024);

    // First store the TdfId
    EA::TDF::TdfId tdfId = fieldValue.getTdfId();
    uint8_t *buf = buffer->acquire(sizeof(tdfId));
    memcpy(buf, &tdfId, sizeof(tdfId));
    buffer->put(sizeof(tdfId));

    // Then encode the Tdf into the RawBuffer
    Heat2Encoder encoder;
    if (!encoder.encode(*buffer, fieldValue))
    {
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].encodeFieldValue: Table(" << getSliverNamespaceStr() << ") failed to encode field:" << fieldName.c_str());
        buffer.reset();
    }

    return buffer;
}

void StorageTable::commitAndReplicateRecord(RecordJobArgs args)
{
    RecordEntry& recordEntry = *args.recordEntryPtr;

    SliverId sliverId = GetSliverIdFromSliverKey(recordEntry.recordId);
    if (recordEntry.erased)
    {
        RedisError rc = mRecordKeyFieldMap.erase(recordEntry.recordId);
        if (rc != REDIS_ERR_OK)
        {
            BLAZE_WARN_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                "storage unavailable err=" << RedisErrorHelper::getName(rc) << ", deferred recordId: " 
                << recordEntry.recordId << " erase.");
        }

        InstanceIdSetBySliverIdMap::const_iterator instanceIdSetItr = mInstanceIdSetBySliverIdMap.find(sliverId);
        // send erase notification to listeners
        if (instanceIdSetItr != mInstanceIdSetBySliverIdMap.end() && !instanceIdSetItr->second.empty())
        {
            // For now we opt to notify all instances registered for this sliver that the record is gone (regardless which field prefix they subscribed for).
            // This works for Game Manager because only certain slaves will subscribe to certain slivers, and it works for User Manager because
            // all instances subscribe to existence replication.

            StorageRecordErase replicatedErase;
            replicatedErase.setSliverNamespace(mSliverNamespace);
            replicatedErase.setRecordId(recordEntry.recordId);
            gStorageManager->sendStorageRecordErasedToInstances(instanceIdSetItr->second, replicatedErase);
        }
        else if (BLAZE_IS_LOGGING_ENABLED(mLogCategory, Logging::TRACE))
        {
            bool hasPublicFields = false;
            for (auto& fieldItr : recordEntry.fieldEntryMap)
            {
                if (fieldItr.second.isPublic)
                {
                    hasPublicFields = true;
                    break;
                }
            }
            if (hasPublicFields)
            {
                BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                    "skipping public replication of recordId:" << recordEntry.recordId
                    << " record erase, no listeners registered for sliver = " << sliverId);
            }
        }

        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ")"
            << " recordId: " << recordEntry.recordId
            << " erased."); 
        
        return;
    }

    TimeValue commitAndReplicateDelay = args.replAtTime - TimeValue::getTimeOfDay();
    if (commitAndReplicateDelay > 0) 
    {
        auto ret = Fiber::sleep(commitAndReplicateDelay, "StorageTable::commitAndReplicateRecord commit delayed");
        if (ret != ERR_OK)
        {
            BLAZE_ERR_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ")"
                << " recordId: " << recordEntry.recordId << ". Failed to call Fiber::sleep.");
        }
    }

    recordEntry.timerId = INVALID_TIMER_ID;
    recordEntry.fiberId = Fiber::getCurrentFiberId();

    const StorageRecordVersion version = recordEntry.recordVersion;

    StorageManager::InstanceIdList candidateInstanceIdList;
    StorageManager::InstanceIdList affectedInstanceIdList;

    InstanceIdSetBySliverIdMap::const_iterator instanceIdSetItr = mInstanceIdSetBySliverIdMap.find(sliverId);
    if (instanceIdSetItr != mInstanceIdSetBySliverIdMap.end())
    {
        candidateInstanceIdList.reserve(instanceIdSetItr->second.size());
        candidateInstanceIdList.assign(instanceIdSetItr->second.begin(), instanceIdSetItr->second.end());
        affectedInstanceIdList.reserve(instanceIdSetItr->second.size());
    }

    if (BLAZE_IS_LOGGING_ENABLED(mLogCategory, Logging::TRACE) && 
        candidateInstanceIdList.empty())
    {
        bool hasPublicFields = false;
        for (auto& fieldItr : recordEntry.fieldEntryMap)
        {
            if (fieldItr.second.isPublic)
            {
                hasPublicFields = true;
                break;
            }
        }
        if (hasPublicFields)
        {
            BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                "skipping public replication of recordId: " << recordEntry.recordId
                << " record update, no listeners registered for sliver = " << sliverId);
        }
    }

    typedef eastl::vector<ptrdiff_t> FieldEntryIndexList;
    FieldEntryIndexList updatedFields, erasedFields;
    updatedFields.reserve(recordEntry.fieldEntryMap.size());
    erasedFields.reserve(recordEntry.fieldEntryMap.size());
    
    for (FieldEntryMap::iterator fieldItr = recordEntry.fieldEntryMap.begin(), fieldEnd = recordEntry.fieldEntryMap.end(); fieldItr != fieldEnd; ++fieldItr)
    {
        FieldEntry& fieldEntry = fieldItr->second;
        if (fieldEntry.dirty)
        {
            fieldEntry.dirty = false;
            EA::TDF::Tdf* tdf = fieldEntry.fieldValue;
            const ptrdiff_t fieldIndex = fieldItr - recordEntry.fieldEntryMap.begin();
            if (tdf != nullptr)
                updatedFields.push_back(fieldIndex);
            else
                erasedFields.push_back(fieldIndex);

            // determine the specific set of instances affected by this change walk 
            StorageManager::InstanceIdList::const_iterator instanceIdItr = candidateInstanceIdList.begin();
            while (instanceIdItr != candidateInstanceIdList.end())
            {
                bool found = false;
                InstanceId instanceId = *instanceIdItr;
                // check that the field name matches one of the filters for this instance
                FieldPrefixListByInstanceIdMap::const_iterator prefixListMapItr = mFieldPrefixListByInstanceIdMap.find(instanceId);
                if (prefixListMapItr != mFieldPrefixListByInstanceIdMap.end())
                {
                    for (FieldPrefixList::const_iterator prefixItr = prefixListMapItr->second.begin(), 
                        prefixEnd = prefixListMapItr->second.end(); prefixItr != prefixEnd; ++prefixItr)
                    {
                        if (blaze_strncmp(prefixItr->c_str(), fieldItr->first.c_str(), prefixItr->size()) == 0)
                        {
                            found = true;
                            break;
                        }
                    }
                }
                if (found)
                {
                    // one of the filters matched, we remove the instance from the list of candidates
                    // since we know we'll be including it in the set of affected instances now.
                    affectedInstanceIdList.push_back(instanceId);
                    instanceIdItr = candidateInstanceIdList.erase_unsorted(instanceIdItr);
                }
                else
                    ++instanceIdItr;
            }
        }
    }

    StorageRecordUpdate replicatedUpdate;
    replicatedUpdate.setSliverNamespace(mSliverNamespace);
    replicatedUpdate.setRecordId(recordEntry.recordId);
    replicatedUpdate.setRecordVersion(version);
    replicatedUpdate.getFields().reserve(updatedFields.size() + erasedFields.size());

    RedisCommand cmd;
    RedisResponsePtr resp;

    cmd.beginKey();
    cmd.add(eastl::make_pair(mRecordKeyFieldMap.getCollectionKey(sliverId), recordEntry.recordId));
    cmd.end();

    cmd.add(erasedFields.size());

    uint32_t erasedPublicFieldCount = 0;
    uint32_t erasedPrivateFieldCount = 0;

    for (FieldEntryIndexList::const_iterator i = erasedFields.begin(), e = erasedFields.end(); i != e; ++i)
    {
        FieldEntryMap::iterator fieldEntryItr = recordEntry.fieldEntryMap.begin() + *i;
        cmd.add(fieldEntryItr->first);
        if (fieldEntryItr->second.isPublic)
        {
            // only replicate erasure of public fields
            StorageRecordUpdate::FieldEntry* updateFieldEntry = replicatedUpdate.getFields().pull_back();
            updateFieldEntry->setFieldName(fieldEntryItr->first.c_str());
            ++erasedPublicFieldCount;
        }
        else
        {
            ++erasedPrivateFieldCount;
        }
    }

    typedef eastl::vector<RawBufferPtr> RawBufferPtrList;
    RawBufferPtrList updateFieldBufList; // pins the raw buffers to enable single encode of data for redis storage and replication
    updateFieldBufList.reserve(updatedFields.size());

    uint32_t publicFieldUpdateCount = 0;
    uint32_t privateFieldUpdateCount = 0;

    for (FieldEntryIndexList::const_iterator i = updatedFields.begin(), e = updatedFields.end(); i != e; ++i)
    {
        FieldEntryMap::iterator fieldEntryItr = recordEntry.fieldEntryMap.begin() + *i;
        RawBufferPtr rawPtr = encodeFieldValue(fieldEntryItr->first, *fieldEntryItr->second.fieldValue);
        if (rawPtr != nullptr)
        {
            uint8_t sha1Digest[SHA_DIGEST_LENGTH];
            memset(sha1Digest, 0, sizeof(sha1Digest));
            SHA1(rawPtr->data(), rawPtr->datasize(), sha1Digest);

            const char8_t* fieldPrefix = getFieldPrefix(fieldEntryItr->first.c_str());
            const bool dupFieldUpdate = memcmp(fieldEntryItr->second.lastPublishedHash, sha1Digest, sizeof(sha1Digest)) == 0;
            if (dupFieldUpdate)
            {
                mFieldIgnores.increment(1, Fiber::getCurrentContext(), fieldPrefix);
                continue;
            }
            memcpy(fieldEntryItr->second.lastPublishedHash, sha1Digest, sizeof(fieldEntryItr->second.lastPublishedHash));

            cmd.add(fieldEntryItr->first);
            cmd.add(*rawPtr);
            if (fieldEntryItr->second.isPublic)
            {
                updateFieldBufList.push_back(rawPtr); // pin the rawbuffer that will be used in the replicated update to the list
                // only replicate update of public fields
                StorageRecordUpdate::FieldEntry* updateFieldEntry = replicatedUpdate.getFields().pull_back();
                updateFieldEntry->setFieldName(fieldEntryItr->first.c_str());
                updateFieldEntry->getFieldValue().assignData(rawPtr->data(), rawPtr->datasize()); // point field entry to rawbuffer data
                ++publicFieldUpdateCount;
            }
            else
            {
                ++privateFieldUpdateCount;
            }

            mFieldUpdates.increment(1, Fiber::getCurrentContext(), fieldPrefix);
        }
        else
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: "
                "Failed to encode field: " << fieldEntryItr->first.c_str() << " for record " 
                << recordEntry.recordId);
        }
    }

    // it is now safe to remove the erased fields from the record entry, 
    // (iterate in reverse order to ensure removal doesn't invalidate cached indexes)
    for (FieldEntryIndexList::const_reverse_iterator ri = erasedFields.rbegin(), re = erasedFields.rend(); ri != re; ++ri)
    {
        FieldEntryMap::iterator fieldEntryItr = recordEntry.fieldEntryMap.begin() + *ri;
        recordEntry.fieldEntryMap.erase(fieldEntryItr);
    }

    bool recordHasPublicChanges = (erasedPublicFieldCount + publicFieldUpdateCount) > 0;
    bool recordHasPrivateChanges = (erasedPrivateFieldCount + privateFieldUpdateCount) > 0;
    if (recordHasPrivateChanges || recordHasPublicChanges)
    {
        StorageRecordVersionTdf recordVersion;
        recordVersion.setVersion(version);
        StorageFieldName fieldName = StorageRecordSnapshot::PUB_VERSION_FIELD;
        RawBufferPtr rawPtr = encodeFieldValue(fieldName, recordVersion);
        if (rawPtr != nullptr)
        {
            cmd.add(fieldName);
            cmd.add(*rawPtr);
        }
        else
        {
            // NOTE: Fiber::isFiberIdInUse() returns false when fiber has been 'frozen' due to assert, so we can always recover this way
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                "failed to encode field: " << fieldName.c_str() << " for recordId: " 
                << recordEntry.recordId);
        }

        recordEntry.lastPublishTime = TimeValue::getTimeOfDay();
        const StorageRecordVersion lastPublishedVersion = recordEntry.recordVersion;

        RedisCluster* cluster = gRedisManager->getRedisCluster(mSliverNamespaceStr); // things like gamemanager_master, or usermanager_master
        if (cluster != nullptr)
        {
            TimeValue lastCommitTime = TimeValue::getTimeOfDay();
            // NOTE: We *need* to use a script here to ensure that cumulative field upsert/erase operations that get applied are always consistent with the version number field
            RedisError rc = cluster->execScript(msUpdateRecordScript, cmd, resp);
            if (rc == REDIS_ERR_OK)
            {
                recordEntry.lastCommitTime = lastCommitTime;
                BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ")"
                    << " recordId: " << recordEntry.recordId
                    << " version: " << recordEntry.recordVersion
                    << " committed.");
            }
            else
            {
                BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                    "failed to upsert " << recordEntry.fieldEntryMap.size() << " fields for recordId: " 
                    << recordEntry.recordId << ", err=" << RedisErrorHelper::getName(rc));
            }
        }
        else
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].commitAndReplicateRecord: Table(" << getSliverNamespaceStr() << ") "
                "failed to get redis cluster.");
        }

        if (recordHasPublicChanges)
        {
            // replicate the update to registered sessions
            if (!affectedInstanceIdList.empty())
            {
                gStorageManager->sendStorageRecordUpdatedToInstances(affectedInstanceIdList, replicatedUpdate);
            }
        }

        if (recordEntry.recordVersion != lastPublishedVersion)
        {
            // We've accumulated some changes while writing out the entry to redis, re-schedule
            args.replAtTime = recordEntry.lastPublishTime + mSliverOwner.getUpdateCoalescePeriod();
            mStorageWriterQueue.queueFiberJob<StorageTable, RecordJobArgs>
                (this, &StorageTable::commitAndReplicateRecord, args, "StorageTable::commitAndReplicateRecord"); // upsertRecord/eraseRecord
        }

        if (mCommitRecordHandler.isValid())
            mCommitRecordHandler(recordEntry.recordId);
    }

    recordEntry.fiberId = Fiber::INVALID_FIBER_ID; // reset fiberId for completeness
}

void StorageTable::syncStorageSliverToInstance(const SyncStorageSliverRequest &request)
{
    InstanceId instanceId = request.getInstanceId();
    SlaveSession* slaveSession = gStorageManager->getSlaveSessionByInstanceId(instanceId);
    SliverId sliverId = request.getSliverId();

    SliverIdSetByInstanceIdMap::insert_return_type ret = mSliverIdSetByInstanceIdMap.insert(instanceId);
    if (ret.second)
    {
        if (slaveSession != nullptr)
        {
            slaveSession->addSessionListener(*this);
        }

        // MAYBE: validate that we are only asking for public field prefixes..

        // NOTE: currently these prefixes do not restrict what gets replicated to this session, only whether a change in a specified
        // field will trigger a replication to the instance that includes this field in its prefix list!
        mFieldPrefixListByInstanceIdMap[instanceId].assign(request.getFieldPrefixes().begin(), request.getFieldPrefixes().end());
    }

    SliverIdSet& sliverIdSet = ret.first->second;
    // track how many slivers this session subscribes for
    bool inserted = sliverIdSet.insert(sliverId).second;
    if (inserted)
    {
        mInstanceIdSetBySliverIdMap[sliverId].insert(instanceId);
        if (slaveSession != nullptr)
        {
            BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                "syncing instance: " << instanceId << ", conn[" << ConnectionSbFormat(slaveSession->getConnection()) << "], sliverId: " << sliverId); 
        }
        else
        {
            BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                "syncing instance: " << instanceId << ", conn[LocalInstance], sliverId: " << sliverId); 
        }
    }
    else
    {
        if (slaveSession != nullptr)
        {
            BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                "already synced instance: " << instanceId << ", conn[" << ConnectionSbFormat(slaveSession->getConnection()) << "], sliverId: " << sliverId); 
        }
        else
        {
            BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                "already synced instance: " << instanceId << ", conn[LocalInstance], sliverId: " << sliverId); 
        }
        return; // already synced
    }

    RecordBlockBySliverIdMap::const_iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr == mRecordBlockBySliverIdMap.end())
        return; // no data to sync

    const RecordBlock& recordBlock = *recordBlockItr->second;

    typedef eastl::hash_map<StorageRecordId, uint64_t> RecordVersionMap;
    RecordVersionMap versionMap;

    StorageRecordErase replicatedErase;
    replicatedErase.setSliverNamespace(mSliverNamespace);

    SyncStorageSliverRequest::RecordIdList::const_iterator recordIdItr = request.getHaveRecordIds().begin();
    SyncStorageSliverRequest::RecordIdList::const_iterator recordIdEnd = request.getHaveRecordIds().end();
    SyncStorageSliverRequest::RecordVersionList::const_iterator recordVersItr = request.getHaveRecordVersions().begin();
    SyncStorageSliverRequest::RecordVersionList::const_iterator recordVersEnd = request.getHaveRecordVersions().end();
    for (; (recordIdItr != recordIdEnd) && (recordVersItr != recordVersEnd); ++recordIdItr, ++recordVersItr)
    {
        StorageRecordId recordId = *recordIdItr;
        uint64_t version = *recordVersItr;
        versionMap[recordId] = version;
        if (recordBlock.recordMap.find(recordId) == recordBlock.recordMap.end())
        {
            replicatedErase.setRecordId(recordId);
            gStorageManager->sendStorageRecordErasedToInstanceById(instanceId, &replicatedErase, /*immediate=*/true); // replicate to slave
        }
    }

    for (RecordMap::const_iterator recordItr = recordBlock.recordMap.begin(), recordEnd = recordBlock.recordMap.end(); recordItr != recordEnd; ++recordItr)
    {
        uint64_t slaveVersion = 0;

        const RecordEntry& recordEntry = *recordItr->second;
        RecordVersionMap::const_iterator versionItr = versionMap.find(recordItr->first);
        if (versionItr != versionMap.end())
        {
            if (versionItr->second == recordEntry.recordVersion)
                continue; // skip identical record versions already stored on the slave
            if (versionItr->second > recordEntry.recordVersion)
                slaveVersion = versionItr->second;
        }

        StorageRecordUpdate replicatedUpdate;
        replicatedUpdate.setRecordId(recordEntry.recordId);
        replicatedUpdate.setRecordVersion(recordEntry.recordVersion);
        replicatedUpdate.setSliverNamespace(mSliverNamespace);
        
        StorageRecordUpdate::FieldEntryList& replicatedFieldList = replicatedUpdate.getFields();
        replicatedFieldList.reserve(recordEntry.fieldEntryMap.size());

        typedef eastl::vector<RawBufferPtr> RawBufferPtrList;
        RawBufferPtrList rawBufferList;
        rawBufferList.reserve(recordEntry.fieldEntryMap.size());
        
        // sync all public fields
        for (auto& recFieldItr : recordEntry.fieldEntryMap)
        {
            if (!recFieldItr.second.isPublic)
                continue;
            if (recFieldItr.second.fieldValue != nullptr)
            {
                RawBufferPtr fieldValue = encodeFieldValue(recFieldItr.first, *recFieldItr.second.fieldValue);
                if (fieldValue != nullptr)
                {
                    rawBufferList.push_back(fieldValue);
                    StorageRecordUpdate::FieldEntryPtr fieldEntry = replicatedFieldList.pull_back();
                    fieldEntry->setFieldName(recFieldItr.first.c_str());
                    EA::TDF::TdfBlob& fieldBlob = fieldEntry->getFieldValue();
                    fieldBlob.assignData(fieldValue->data(), fieldValue->datasize());
                }
                else
                {
                    BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                        "failed to encode field: " << recFieldItr.first.c_str() << " fields for record: "
                        << recordEntry.recordId);
                }
            }
            else
            {
                // field has been erased, but not yet replicated, replicate erasure anyway 
                // (if remote side already has an earlier version of the record the field must be erased)
                StorageRecordUpdate::FieldEntryPtr fieldEntry = replicatedFieldList.pull_back();
                fieldEntry->setFieldName(recFieldItr.first.c_str());
            }
        }

        if (!replicatedFieldList.empty())
        {
            if (slaveVersion > recordEntry.recordVersion)
            {
                BLAZE_WARN_LOG(mLogCategory, "[StorageTable].syncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
                    "has record recordId=" << recordEntry.recordId << " with table.version= " << recordEntry.recordVersion << " < listener.version=" << slaveVersion << " on instanceId = " << instanceId << 
                    "; force syncing listener record to match table version.");                   
            }
            gStorageManager->sendStorageRecordUpdatedToInstanceById(instanceId, &replicatedUpdate, /*immediate=*/true); // replicate to slave
        }
    }
}

void StorageTable::unsyncStorageSliverToInstance(const UnsyncStorageSliverRequest &request)
{
    InstanceId instanceId = request.getInstanceId();
    SlaveSession* slaveSession = gStorageManager->getSlaveSessionByInstanceId(instanceId);
    SliverId sliverId = request.getSliverId();

    if (slaveSession != nullptr)
    {
        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].unsyncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
            "unsyncing instance: " << instanceId << ", conn[" << ConnectionSbFormat(slaveSession->getConnection()) << "], sliverId: " << sliverId);
    }
    else
    {
        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].unsyncStorageSliverToInstance: Table(" << getSliverNamespaceStr() << ") "
            "unsyncing instance: " << instanceId << ", conn[LocalInstance], sliverId: " << sliverId);
    }

    SliverIdSetByInstanceIdMap::iterator sliverIdSetItr = mSliverIdSetByInstanceIdMap.find(instanceId);
    if (sliverIdSetItr != mSliverIdSetByInstanceIdMap.end())
    {
        sliverIdSetItr->second.erase(sliverId);
        if (sliverIdSetItr->second.empty())
        {
            mSliverIdSetByInstanceIdMap.erase(instanceId);
            // Session no longer subscribes to any slivers, remove prefix mapping as well
            mFieldPrefixListByInstanceIdMap.erase(instanceId);
            if (slaveSession != nullptr)
            {
                slaveSession->removeSessionListener(*this);
            }
        }
    }

    InstanceIdSetBySliverIdMap::iterator instanceIdSetItr = mInstanceIdSetBySliverIdMap.find(sliverId);
    if (instanceIdSetItr != mInstanceIdSetBySliverIdMap.end())
    {
        instanceIdSetItr->second.erase(instanceId);
        if (instanceIdSetItr->second.empty())
            mInstanceIdSetBySliverIdMap.erase(sliverId);
    }
}

void StorageTable::onSlaveSessionRemoved(SlaveSession& slaveSession)
{
    InstanceId instanceId = SlaveSession::getSessionIdInstanceId(slaveSession.getId());
    SliverIdSetByInstanceIdMap::iterator sliverIdSetItr = mSliverIdSetByInstanceIdMap.find(instanceId);
    if (sliverIdSetItr != mSliverIdSetByInstanceIdMap.end())
    {
        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].onSlaveSessionRemoved: Table(" << getSliverNamespaceStr() << ") "
            "detaching instance: " << instanceId << ", conn[" << ConnectionSbFormat(slaveSession.getConnection()) << "]");

        for (SliverIdSet::const_iterator i = sliverIdSetItr->second.begin(), e = sliverIdSetItr->second.end(); i != e; ++i)
        {
            SliverId sliverId = *i;
            InstanceIdSetBySliverIdMap::iterator instanceIdSetItr = mInstanceIdSetBySliverIdMap.find(sliverId);
            if (instanceIdSetItr != mInstanceIdSetBySliverIdMap.end())
            {
                instanceIdSetItr->second.erase(instanceId);
                if (instanceIdSetItr->second.empty())
                    mInstanceIdSetBySliverIdMap.erase(sliverId);
            }
        }
        mFieldPrefixListByInstanceIdMap.erase(instanceId);
        mSliverIdSetByInstanceIdMap.erase(instanceId);
    }
}

void StorageTable::publishRecord(RecordEntry &recordEntry, RecordBlock &recordBlock, TimeValue replicateAtTime)
{
    ++recordEntry.recordVersion; // always bump version

    if (recordEntry.refCount == 1)
    {
        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].publishRecord: Table(" << getSliverNamespaceStr() << ") "
            "recordId: " << recordEntry.recordId << " " << (recordEntry.erased ? "erase" : "commit") << " at version: " << recordEntry.recordVersion << " queued.");

        // This means that no jobs are pending for this record, schedule a job
        mStorageWriterQueue.queueFiberJob<StorageTable, RecordJobArgs>
            (this, &StorageTable::commitAndReplicateRecord, RecordJobArgs(recordEntry, recordBlock, replicateAtTime), "StorageTable::commitAndReplicateRecord"); // upsertRecord/eraseRecord
    }
    else
    {
        BLAZE_SPAM_LOG(mLogCategory, "[StorageTable].publishRecord: Table(" << getSliverNamespaceStr() << ") "
            "recordId: " << recordEntry.recordId << " commit already pending.");
    }
}

void StorageTable::onImportSliver(OwnedSliver& ownedSliver, BlazeRpcError& err)
{
    EA_ASSERT(mImportRecordHandler.isValid());
    EA_ASSERT(mExportRecordHandler.isValid());

    SliverId sliverId = ownedSliver.getSliverId();

    RecordBlockBySliverIdMap::insert_return_type ret = mRecordBlockBySliverIdMap.insert(sliverId);
    if (ret.second)
    {
        ret.first->second = BLAZE_NEW_MGID(StorageManager::COMPONENT_MEMORY_GROUP, "StorageTable::RecordBlock_Import") RecordBlock;
        BlazeStlAllocator alloc = mRecordBlockBySliverIdMap.get_allocator();
        alloc.set_name("StorageTable::RecordBlock::RecordMap");
        ret.first->second->recordMap.set_allocator(alloc);
    }
    else
    {
        // if sliver block already exists something is wrong with sliver manager
        // since it should never ask us to import a sliver it already knows we have!
        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].onImportSliver: Table(" << getSliverNamespaceStr() << ") sliver " << sliverId << " is already owned!");
        err = ERR_SYSTEM;
        return;
    }

    // newly inserted block
    RecordBlock& recordBlock = *ret.first->second;

    BlazeRpcError result = ERR_OK;

    EA_ASSERT(!recordBlock.syncing); // TODO: replace with error logging
    recordBlock.syncing = true;
    EA_ASSERT(recordBlock.ownedSliver == nullptr);
    recordBlock.ownedSliver = &ownedSliver;

    // pull data from redis
    eastl::pair<RedisCollection::CollectionKey, char8_t> matchKey = 
        eastl::make_pair(mRecordKeyFieldMap.getCollectionKey(sliverId), '*');

    uint64_t validRecordCount = 0;
    uint64_t totalRecordCount = 0;
    uint64_t ignoredScanRecordDupesCount = 0;

    BlazeStlAllocator fieldAlloc = mRecordBlockBySliverIdMap.get_allocator();
    fieldAlloc.set_name("StorageTable::FieldEntryMap");

    RedisCommand cmd;
    RedisResponsePtr scanResp;
    RedisDecoder rdecoder;
    const char8_t* cursor = "0"; // NOTE: Cursor returned by SCAN is always a string-formatted int
    do
    {
        cmd.reset();
        cmd.add("SCAN");
        cmd.add(cursor);
        cmd.add("MATCH");

        cmd.beginKey();
        cmd.add(matchKey);
        cmd.end();

        // NOTE: Count is a *hint* that governs how much 'work' the 'stride' length for each redis scan request, 
        // if the amount of data returned is 0, it is best to double the COUNT adaptively (until about 100000 elements)
        // to reduce the number of requests to redis and to enable each request to scan more elements before returning
        // any results. In other words, COUNT is *not* the number of elements to return, but the number of elements to 'visit'.
        // Example: when selecting a 1000 subset of 10M elements [populated with testMassiveKeyPopulation()] using the COUNT = 100000 
        // returns the results in 100 requests, which takes about 10 seconds. 
        // Without using COUNT, the default COUNT value of (10) is used which results in 1M requests
        // which takes about 11 minutes, to return the same 1000 elements!
        cmd.add("COUNT");
        cmd.add(REDIS_SCAN_STRIDE_MAX);
        cmd.setNamespace(mSliverNamespaceStr);

        // "scan 0 match k* count 100000"
        RedisError rc = gRedisManager->exec(cmd, scanResp);
        if (rc == REDIS_ERR_OK)
        {
            EA_ASSERT(scanResp->isArray() && scanResp->getArraySize() == REDIS_SCAN_RESULT_MAX);
            const RedisResponse& scanArray = scanResp->getArrayElement(REDIS_SCAN_RESULT_ARRAY);
            EA_ASSERT(scanArray.isArray());
            for (uint32_t i = 0, len = scanArray.getArraySize(); i < len; ++i)
            {
                ++totalRecordCount;

                const RedisResponse& scanKey = scanArray.getArrayElement(i);
                const char8_t* begin = scanKey.getString();
                const char8_t* end = begin + scanKey.getStringLen();
                eastl::pair<RedisCollection::CollectionKey, StorageRecordId> recordKey;
                if (rdecoder.decodeValue(begin, end, recordKey) != nullptr)
                {
                    cmd.reset();
                    cmd.add("HGETALL");
                    // populate command with raw keys obtained from the scan
                    cmd.beginKey();
                    cmd.add((const uint8_t*)scanKey.getString(), scanKey.getStringLen());
                    cmd.end();
                    cmd.setNamespace(mSliverNamespaceStr);
                    RedisResponsePtr itemResp;
                    rc = gRedisManager->exec(cmd, itemResp);
                    if (rc == REDIS_ERR_OK)
                    {
                        StorageRecordSnapshot snapshot(mStorageSnapshotAllocator, mLogCategory);
                        snapshot.recordId = recordKey.second;
                        if (snapshot.populateFromStorage(*itemResp))
                        {
                            RecordMap::insert_return_type recRet = recordBlock.recordMap.insert(snapshot.recordId);
                            if (recRet.second)
                            {
                                RecordEntry* recordEntry = BLAZE_NEW_MGID(StorageManager::COMPONENT_MEMORY_GROUP, "StorageTable::RecordEntry_Import") RecordEntry;
                                recordEntry->recordId = snapshot.recordId;
                                recordEntry->recordVersion = snapshot.recordVersion;
                                recordEntry->fieldEntryMap.set_allocator(fieldAlloc);
                                recordEntry->fieldEntryMap.reserve(snapshot.fieldMap.size());
                                recRet.first->second = recordEntry;

                                auto publicFieldPrefixLen = mPublicFieldPrefix.size();
                                for (StorageRecordSnapshot::FieldEntryMap::const_iterator fieldEntryItr = snapshot.fieldMap.begin(),
                                    fieldEntryEnd = snapshot.fieldMap.end(); fieldEntryItr != fieldEntryEnd; ++fieldEntryItr)
                                {
                                    FieldEntry& fieldEntry = recordEntry->fieldEntryMap[fieldEntryItr->first];
                                    fieldEntry.fieldValue = fieldEntryItr->second;
                                    fieldEntry.isPublic = blaze_strncmp(mPublicFieldPrefix.c_str(), fieldEntryItr->first.c_str(), publicFieldPrefixLen) == 0;
                                }

                                // trigger import handler
                                // IMPORTANT: During the import handler, it is possible (and allowed) that the called function will mutate the
                                //            recordBlock.recordMap eastl container.  Therefore, it is important that no further code paths in
                                //            this function assume that any held iterators for that container will still be valid.
                                mImportRecordHandler(ownedSliver, snapshot);
                                ++validRecordCount;
                            }
                            else if (recRet.first->second->recordVersion == snapshot.recordVersion)
                                ++ignoredScanRecordDupesCount;
                            else
                            {
                                // Occasionally scan will return duplicate keys, in this event
                                // the versions of the object read and the one we cached already must match!
                                // If the versions of a repeated read don't match, this means someone else 
                                // is writing records as we are trying take a snapshot of the records in order 
                                // to take ownership!
                                BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].onImportSliver: Table(" << getSliverNamespaceStr() << ") "
                                    "local version: " << recRet.first->second->recordVersion << "!= snapshot version: " << snapshot.recordVersion << " mismatch for record " 
                                    << snapshot.recordId);
                                result = ERR_SYSTEM;
                            }
                        }
                    }
                    else
                    {
                        BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].onImportSliver: Table(" << getSliverNamespaceStr() << ") "
                            "failed to retrieve fields for record: " << recordKey.second << ", error: " 
                            << RedisErrorHelper::getName(rc));
                        result = REDIS_ERR_SYSTEM;
                        break;
                    }
                }
                else
                {
                    BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].onImportSliver: "
                        "Failed to decode record key: " << eastl::string(begin, end));
                    result = REDIS_ERR_SYSTEM;
                    break;
                }
            }

            const RedisResponse& cursorResp = scanResp->getArrayElement(REDIS_SCAN_RESULT_CURSOR);
            EA_ASSERT(cursorResp.isString());
            cursor = cursorResp.getString();
        }
        else
        {
            BLAZE_ASSERT_LOG(mLogCategory, "[StorageTable].onImportSliver: Table(" << getSliverNamespaceStr() << ") failed to retrieve data for sliver: " << sliverId);
            result = REDIS_ERR_SYSTEM;
            break;
        }
    }
    while (strcmp(cursor, "0") != 0);

    if (result == ERR_OK)
    {
        BLAZE_TRACE_LOG(mLogCategory, "[StorageTable].syncOwnedSliver: Table(" << getSliverNamespaceStr() << ") took ownership of sliver: " 
            << "{" << sliverId << "}, " << validRecordCount 
            << " of " << totalRecordCount << " records "
            << "(ignored dupes: " << ignoredScanRecordDupesCount <<")");
    }
    else
    {
        BLAZE_ERR_LOG(mLogCategory, "[StorageTable].syncOwnedSliver: Table(" << getSliverNamespaceStr() << ") failed to take ownership of sliver: " 
            << "{" << sliverId << "}, " << validRecordCount 
            << " of " << totalRecordCount << " records "
            << "(ignored dupes: " << ignoredScanRecordDupesCount <<")");
    }

    // sync is done or failed
    recordBlock.syncing = false;

    if (result != ERR_OK)
    {
        // sync failed, trigger export callback
        for (RecordMap::const_iterator recordItr = recordBlock.recordMap.begin(), 
            recordEnd = recordBlock.recordMap.end(); recordItr != recordEnd; ++recordItr)
        {
            mExportRecordHandler(recordItr->first);
        }

        // get rid of the entire sliver (delete it too)
        auto iter = mRecordBlockBySliverIdMap.find(sliverId);
        if (iter != mRecordBlockBySliverIdMap.end())
        {
            delete iter->second;
            mRecordBlockBySliverIdMap.erase(iter);
        }
    }

    err = result;
}

void StorageTable::onExportSliver(OwnedSliver& ownedSliver, BlazeRpcError& err)
{
    err = ERR_OK;

    SliverId sliverId = ownedSliver.getSliverId();

    // get rid of any slave sessions that may be listening to us as the owner of this sliver to avoid flushing any state to them, they will get it from the new owner
    InstanceIdSetBySliverIdMap::iterator instanceIdSetItr = mInstanceIdSetBySliverIdMap.find(sliverId);
    if (instanceIdSetItr != mInstanceIdSetBySliverIdMap.end())
    {
        for (InstanceIdSet::const_iterator instanceIdItr = instanceIdSetItr->second.begin(), instanceIdEnd = instanceIdSetItr->second.end(); instanceIdItr != instanceIdEnd; ++instanceIdItr)
        {
            InstanceId instanceId = *instanceIdItr;
            SliverIdSetByInstanceIdMap::iterator sliverIdSetItr = mSliverIdSetByInstanceIdMap.find(instanceId);
            if (sliverIdSetItr != mSliverIdSetByInstanceIdMap.end())
            {
                sliverIdSetItr->second.erase(sliverId);
                if (sliverIdSetItr->second.empty())
                {
                    SlaveSession* slaveSession = gStorageManager->getSlaveSessionByInstanceId(instanceId);
                    if (slaveSession != nullptr)
                    {
                        slaveSession->removeSessionListener(*this);
                    }
                    mSliverIdSetByInstanceIdMap.erase(instanceId);
                }
            }
        }
        mInstanceIdSetBySliverIdMap.erase(sliverId);
    }

    RecordBlockBySliverIdMap::iterator recordBlockItr = mRecordBlockBySliverIdMap.find(sliverId);
    if (recordBlockItr != mRecordBlockBySliverIdMap.end())
    {
        RecordBlock& recordBlock = *recordBlockItr->second;

        EA_ASSERT(recordBlock.mPendingWriteCount == 0);

        // detach the record block, no further writes for the sliver are possible!
        mRecordBlockBySliverIdMap.erase(recordBlockItr);

        // trigger export of all locally owned data, we will no longer own it.
        for (RecordMap::const_iterator recordItr = recordBlock.recordMap.begin(), 
            recordEnd = recordBlock.recordMap.end(); recordItr != recordEnd; ++recordItr)
        {
            mExportRecordHandler(recordItr->first);
        }

        SAFE_DELETE_REF(recordBlock);
    }
}

const char8_t* StorageTable::getFieldPrefix(const char8_t* fieldName) const
{
    const char8_t* fieldPrefix = nullptr;
    StorageFieldPrefixSet::const_iterator fieldPrefixItr = mValidFieldPrefixes.lower_bound(fieldName);
    if (fieldPrefixItr != mValidFieldPrefixes.end())
    {
        if (blaze_strncmp(*fieldPrefixItr, fieldName, strlen(*fieldPrefixItr)) == 0)
            fieldPrefix = *fieldPrefixItr;
        else if (fieldPrefixItr != mValidFieldPrefixes.begin())
        {
            --fieldPrefixItr;
            if (blaze_strncmp(*fieldPrefixItr, fieldName, strlen(*fieldPrefixItr)) == 0)
                fieldPrefix = *fieldPrefixItr;
        }
    }
    else if (!mValidFieldPrefixes.empty())
    {
        StorageFieldPrefixSet::const_reverse_iterator fieldPrefixRitr = mValidFieldPrefixes.rbegin();
        if (blaze_strncmp(*fieldPrefixRitr, fieldName, strlen(*fieldPrefixRitr)) == 0)
            fieldPrefix = *fieldPrefixRitr;
    }
    return fieldPrefix;
}

void intrusive_ptr_add_ref(StorageTable::RecordEntry* ptr)
{
    ++ptr->refCount;
}

void intrusive_ptr_release(StorageTable::RecordEntry* ptr)
{
    if (ptr->refCount > 0)
    {
        --ptr->refCount;
        if (ptr->refCount == 0)
            delete ptr;
    }
}

void intrusive_ptr_add_ref(StorageTable::RecordBlock* ptr)
{
    if (ptr->mPendingWriteCount == 0)
    {
        bool result = ptr->ownedSliver->getPrioritySliverAccess(ptr->ownedSliverAccessRef); // pin the sliver from migrating
        EA_ASSERT_MSG(result, "Storage table failed to obtain owned sliver access!");
    }
    ++ptr->mPendingWriteCount;
}

void intrusive_ptr_release(StorageTable::RecordBlock* ptr)
{
    if (ptr->mPendingWriteCount > 0)
    {
        --ptr->mPendingWriteCount;
        if (ptr->mPendingWriteCount == 0)
        {
            ptr->ownedSliverAccessRef.release();
        }
    }
}

} // namespace Blaze

