/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STORAGE_TABLE_H
#define BLAZE_STORAGE_TABLE_H

/*** Include files *******************************************************************************/
#include "EASTL/intrusive_ptr.h"
#include "framework/system/fiberjobqueue.h"
#include "framework/slivers/sliverowner.h"
#include "framework/storage/storagecommon.h"
#include "framework/storage/storagesnapshot.h"
#include "framework/redis/rediskeyfieldmap.h"
#include "framework/redis/redisscriptregistry.h"
#include <openssl/sha.h>
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

using StoragePrefix = const char8_t*;
namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<const char8_t*>* storage_prefix;
    }
}

class StorageManager;
class SyncStorageSliverRequest;

typedef Functor1<StorageRecordId> CommitStorageRecordCb;

class StorageTable : private SlaveSessionListener
{
    NON_COPYABLE(StorageTable);
public:
    StorageTable(const SliverNamespace& sliverNamespace, MemoryGroupId memoryGroupId, int32_t logCategory);
    ~StorageTable() override;

    /** \brief Register all field handlers before registering the table with the StorageManager */
    void registerRecordHandlers(ImportStorageRecordCb importHandler, ExportStorageRecordCb exportHandler);
    /** \brief Register a nullptr terminated array of valid field prefixes (used for reporting field change metrics) */
    void registerFieldPrefixes(const char8_t** validFieldPrefixes);
    /** \brief Triggered whenever a record update is committed to persistent storage. Useful for generating rate limited state change events. Handler is always invoked even if the write to storage fails. */
    void registerRecordCommitHandler(CommitStorageRecordCb commitHandler);

    void upsertField(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue);
    void eraseField(StorageRecordId recordId, const char8_t* fieldName);
    void eraseRecord(StorageRecordId recordId);
    StorageRecordVersion getRecordVersion(StorageRecordId recordId);

    SliverNamespace getSliverNamespace() const { return mSliverNamespace; }
    const char8_t* getSliverNamespaceStr() const { return mSliverNamespaceStr; }
    SliverOwner& getSliverOwner() { return mSliverOwner; }
    bool isFirstRegistration() const { return mRecordKeyFieldMap.isFirstRegistration(); }

private:
    friend class StorageManager;
    
    struct FieldEntry
    {
        FieldEntry() : dirty(false), isPublic(false) { memset(lastPublishedHash, 0, sizeof(lastPublishedHash)); }
        EA::TDF::TdfPtr fieldValue;
        uint8_t lastPublishedHash[SHA_DIGEST_LENGTH];
        bool dirty;
        bool isPublic;
    };
    typedef eastl::vector_map<StorageFieldName, FieldEntry> FieldEntryMap; // intentionally an ordered map to enable fast search by field prefix

    struct RecordEntry
    {
        RecordEntry() 
            : refCount(0),
            recordId(0),
            recordVersion(0), 
            erased(false),
            timerId(INVALID_TIMER_ID),
            fiberId(Fiber::INVALID_FIBER_ID)
        {
            // always set last publish time when creating a record to ensure we respect coalescing thresholds
            lastPublishTime = EA::TDF::TimeValue::getTimeOfDay();
        }
        uint32_t refCount;
        StorageRecordId recordId;
        StorageRecordVersion recordVersion;
        FieldEntryMap fieldEntryMap; // NOTE: fields to erase store no rawbuffer, just the key and an empty value, even if the field was created/erased before the data was flushed, we leave the field entry in the hashmap because that way the receiving client is guaranteed to ignore it or to erase it.
        bool erased;

        TimerId timerId;
        Fiber::FiberId fiberId;
        EA::TDF::TimeValue lastPublishTime;
        EA::TDF::TimeValue lastCommitTime; // currently used for debugging only
    };
    friend void intrusive_ptr_add_ref(RecordEntry* ptr);
    friend void intrusive_ptr_release(RecordEntry* ptr);

    typedef eastl::intrusive_ptr<RecordEntry> RecordEntryPtr; // this pointer controls record entry lifetime
    typedef eastl::hash_map<StorageRecordId, RecordEntryPtr> RecordMap;

    struct RecordBlock
    {
        RecordBlock()
            : syncing(false), mPendingWriteCount(0), ownedSliver(nullptr), ownedSliverAccessRef(false) {}

        bool syncing;
        uint32_t mPendingWriteCount;
        OwnedSliver* ownedSliver;
        Sliver::AccessRef ownedSliverAccessRef;
        RecordMap recordMap;

        NON_COPYABLE(RecordBlock);
    };

    typedef eastl::intrusive_ptr<RecordBlock> RecordBlockRef; // this reference is used to track pending writes to a record block; it does not control record block lifetime

    friend void intrusive_ptr_add_ref(RecordBlock* ptr);
    friend void intrusive_ptr_release(RecordBlock* ptr);

    struct RecordJobArgs
    {
        RecordJobArgs(RecordEntry& recordEntry, RecordBlock& recordBlock, EA::TDF::TimeValue replAt) : recordEntryPtr(&recordEntry), recordBlockRef(&recordBlock), replAtTime(replAt) {}
        RecordEntryPtr recordEntryPtr;
        RecordBlockRef recordBlockRef;
        EA::TDF::TimeValue replAtTime;
    };

    typedef eastl::vector<eastl::string> FieldPrefixList;
    typedef eastl::hash_map<InstanceId, FieldPrefixList> FieldPrefixListByInstanceIdMap;
    typedef eastl::hash_map<SliverId, RecordBlock*> RecordBlockBySliverIdMap;
    typedef eastl::vector_set<InstanceId> InstanceIdSet;
    typedef eastl::vector_set<SliverId> SliverIdSet;
    typedef eastl::hash_map<InstanceId, SliverIdSet> SliverIdSetByInstanceIdMap;
    typedef eastl::hash_map<SliverId, InstanceIdSet> InstanceIdSetBySliverIdMap;
    typedef eastl::hash_map<const char8_t*, uint64_t> CountByFiberContextMap;
    typedef eastl::hash_map<const char8_t*, uint64_t> CountByFieldPrefixMap;
    typedef RedisKeyFieldMap<StorageRecordId, StorageFieldName, RawBuffer> RecordFieldMap;
    
    RawBufferPtr encodeFieldValue(const StorageFieldName& fieldName, const EA::TDF::Tdf& fieldValue) const;
    void publishRecord(RecordEntry &recordEntry, RecordBlock &recordBlock, EA::TDF::TimeValue replicateAtTime);
    void commitAndReplicateRecord(RecordJobArgs args);
    void syncStorageSliverToInstance(const SyncStorageSliverRequest &request);
    void unsyncStorageSliverToInstance(const UnsyncStorageSliverRequest &request);
    void onSlaveSessionRemoved(SlaveSession& slaveSession) override;
    void onImportSliver(OwnedSliver& ownedSliver, BlazeRpcError& err);
    void onExportSliver(OwnedSliver& ownedSliver, BlazeRpcError& err);
    const char8_t* getFieldPrefix(const char8_t* fieldName) const;

    SliverNamespace mSliverNamespace;
    const char8_t* mSliverNamespaceStr;
    Blaze::Allocator& mStorageSnapshotAllocator;
    int32_t mLogCategory;
    SliverOwner mSliverOwner;
    RecordFieldMap mRecordKeyFieldMap;

    FieldPrefixListByInstanceIdMap mFieldPrefixListByInstanceIdMap;
    SliverIdSetByInstanceIdMap mSliverIdSetByInstanceIdMap;
    InstanceIdSetBySliverIdMap mInstanceIdSetBySliverIdMap;

    RecordBlockBySliverIdMap mRecordBlockBySliverIdMap;
    FiberJobQueue mStorageWriterQueue;
    eastl::string mPublicFieldPrefix;
    StorageFieldPrefixSet mValidFieldPrefixes;
    Metrics::MetricsCollection& mMetricsCollection;
    Metrics::TaggedCounter<Metrics::FiberContext, StoragePrefix> mFieldChanges;
    Metrics::TaggedCounter<Metrics::FiberContext, StoragePrefix> mFieldUpdates;
    Metrics::TaggedCounter<Metrics::FiberContext, StoragePrefix> mFieldIgnores;
    UpsertStorageRecordCb mImportRecordHandler;
    EraseStorageRecordCb mExportRecordHandler;
    CommitStorageRecordCb mCommitRecordHandler;
    static RedisScript msUpdateRecordScript;
};

void intrusive_ptr_add_ref(StorageTable::RecordBlock* ptr);
void intrusive_ptr_release(StorageTable::RecordBlock* ptr);

void intrusive_ptr_add_ref(StorageTable::RecordEntry* ptr);
void intrusive_ptr_release(StorageTable::RecordEntry* ptr);

} // Blaze


#endif // BLAZE_STORAGE_TABLE_H
