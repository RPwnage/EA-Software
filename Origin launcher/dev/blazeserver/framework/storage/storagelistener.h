/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STORAGE_LISTENER_H
#define BLAZE_STORAGE_LISTENER_H

/*** Include files *******************************************************************************/
#include "framework/slivers/sliverlistener.h"
#include "framework/storage/storagecommon.h"
#include "framework/storage/storagesnapshot.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class StorageManager;

/*! \brief Register all handlers before registering the listener with the StorageManager */
class StorageListener
{
public:
    struct FieldHandler
    {
        const char8_t* fieldNamePrefix;
        const char8_t* upsertFieldFiberContext;
        const char8_t* eraseFieldFiberContext;
        UpsertStorageFieldCb upsertFieldCb;
        EraseStorageFieldCb eraseFieldCb;
        FieldHandler() : fieldNamePrefix(""), upsertFieldFiberContext("StorageListener::onUpsertField"), eraseFieldFiberContext("StorageListener::onEraseField") {}
    };

    struct RecordHandler
    {
        EraseStorageRecordCb eraseRecordCb;
        const char8_t* eraseRecordFiberContext;
        RecordHandler() : eraseRecordFiberContext("StorageListener::onEraseRecord") {}
    };

    StorageListener(const SliverNamespace& sliverNamespace, MemoryGroupId memoryGroupId, int32_t logCategory);
    ~StorageListener();

    /*! \brief Incoming updates always trigger field handlers in registration order. */
    void registerFieldHandler(FieldHandler& fieldHandler);

    /*! \brief Record erasure never triggers any field handlers. */
    void registerRecordHandler(RecordHandler& recordHandler);

    StorageRecordVersion getRecordVersion(StorageRecordId recordId) const;

private:
    friend class StorageManager;

    struct FieldEntry
    {
        FieldEntry() {}
        EA::TDF::TdfPtr fieldValue;
    };
    typedef eastl::vector_map<StorageFieldName, FieldEntry> FieldEntryMap;

    struct RecordEntry
    {
        RecordEntry() : recordId(0), recordVersion(0) {}
        StorageRecordId recordId;
        StorageRecordVersion recordVersion;
        FieldEntryMap fieldMap;
    };
    typedef eastl::hash_map<StorageRecordId, RecordEntry> RecordMap;
    typedef eastl::hash_map<SliverId, RecordMap> RecordMapBySliverIdMap;
    typedef eastl::vector<StorageListener*> ListenerList;
    typedef eastl::hash_set<StorageRecordId> RecordIdSet;
    typedef eastl::vector<FieldHandler> StorageFieldHandlerList;

    void onSyncSliver(SliverPtr sliver, BlazeRpcError& err);
    void onDesyncSliver(SliverPtr sliver, BlazeRpcError& err);
    void processRecordUpdate(const StorageRecordSnapshot& snapshot);
    void processRecordErase(StorageRecordId recordId);
    const char8_t* getSliverNamespaceStr() const;
    void onUpsertField(const FieldHandler* fieldHandler, StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf* newValue);
    void onEraseField(const FieldHandler* fieldHandler, StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf* oldValue);
    void onEraseRecord(StorageRecordId recordId);
    bool isValidFieldHandler(FieldHandler& handler) const;
    bool isValidRecordHandler(RecordHandler& handler) const;

    SliverNamespace mSliverNamespace;
    Blaze::Allocator& mStorageSnapshotAllocator;
    int32_t mLogCategory;
    SliverListener mSliverListener;
    StorageFieldHandlerList mFieldHandlerList;
    RecordHandler mRecordHandler;
    RecordMapBySliverIdMap mRecordMapBySliverIdMap;
    Fiber::CreateParams mFiberOptions;
    uint64_t mTotalUpdateVersGtCount;
    uint64_t mTotalUpdateVersEqCount; // number of updates whose version was equal to that stored locally
    uint64_t mTotalUpdateVersLtCount; // number of updates whose versions was lower than that stored locally
    bool mIsRegistered;
};



} // Blaze


#endif // BLAZE_STORAGE_LISTENER_H
