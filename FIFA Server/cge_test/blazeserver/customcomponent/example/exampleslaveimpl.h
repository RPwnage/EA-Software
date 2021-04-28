/*************************************************************************************************/
/*!
    \file   exampleslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EXAMPLE_SLAVEIMPL_H
#define BLAZE_EXAMPLE_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/controller/controller.h"
#include "example/rpc/exampleslave_stub.h"
#include "example/rpc/examplemaster.h"
#include "example/tdf/exampletypes_server.h"
#include "framework/replication/replicationcallback.h"

#include "framework/storage/storagetable.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Example
{

class ExampleItem
{
public:
    ExampleItem(uint64_t id, const OwnedSliverRef& ownedSliverRef);
    ExampleItem(uint64_t id, const ExampleItemDataPtr& exampleItemData, const OwnedSliverRef& ownedSliverRef);

    uint64_t getExampleItemId() const { return mExampleItemId; }
    ExampleItemData& getData() const { return *mExampleItemData; }

    void setAutoCommitOnRelease() { mAutoCommit = true; }

    void setTimeToLiveTimerId(TimerId val) { mTimeToLiveTimerId = val; }
    TimerId getTimeToLiveTimerId() const { return mTimeToLiveTimerId; }
    void setBroadcastMessageTimerId(TimerId val) { mBroadcastMessageTimerId = val; }
    TimerId getBroadcastMessageTimerId() const { return mBroadcastMessageTimerId; }

private:
    bool mAutoCommit;
    uint32_t mRefCount;
    OwnedSliverRef mOwnedSliverRef;
    Sliver::AccessRef mOwnedSliverAccessRef;

    uint64_t mExampleItemId;
    ExampleItemDataPtr mExampleItemData;

    TimerId mTimeToLiveTimerId;
    TimerId mBroadcastMessageTimerId;

    friend void intrusive_ptr_add_ref(ExampleItem* ptr);
    friend void intrusive_ptr_release(ExampleItem* ptr);
};

void intrusive_ptr_add_ref(ExampleItem* ptr);
void intrusive_ptr_release(ExampleItem* ptr);

typedef eastl::intrusive_ptr<ExampleItem> ExampleItemPtr;
typedef eastl::hash_map<uint64_t, ExampleItemPtr> ExampleItemPtrByIdMap;

class ExampleSlaveImpl : public ExampleSlaveStub
{
public:
    ExampleSlaveImpl();
    ~ExampleSlaveImpl() override;

public:
    uint64_t getNextUniqueId();
    ExampleItemPtrByIdMap& getExampleItemPtrByIdMap() { return mExampleItemPtrByIdMap; }
    StorageTable& getExampleItemDataTable() { return mExampleItemDataTable; }
    ExampleItem* getExampleItem(uint64_t id) const;
    const ExampleItem* getReadOnlyExampleItem(uint64_t id) const;
    ExampleItemPtr getWritableExampleItem(uint64_t id, bool autoCommit = true);
    void eraseExampleItem(uint64_t id);

    // Migration and storage callbacks
    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    void onCommitStorageRecord(StorageRecordId recordId);

    void exampleItemTimeToLiveHandler(uint64_t exampleItemId);
    void exampleItemBroadcastMessageHandler(uint64_t exampleItemId);

protected:
    RequestToUserSessionOwnerError::Error processRequestToUserSessionOwner(const Message* message) override;

private:
    bool onConfigure() override;
    bool onResolve() override;

private:
    ExampleItemPtrByIdMap mExampleItemPtrByIdMap;

    StorageTable mExampleItemDataTable;

    struct UniqueIdGenerator
    {
    public:
        UniqueIdGenerator() : counter(0) {}
        uint64_t getNext()
        {
            UniqueId uniqueId;
            uniqueId.u.compound.timev = TimeValue::getTimeOfDay().getSec();
            uniqueId.u.compound.counter = counter++;
            uniqueId.u.compound.instance_id = gController->getInstanceId();
            return uniqueId;
        }
    private:
        uint64_t counter : 14;
        struct UniqueId
        {
            UniqueId(const UniqueId& other) { u.value = other.u.value; }
            UniqueId(const uint64_t& value) { u.value = value; }
            UniqueId() { u.value = 0; }

            operator uint64_t() { return u.value; }

            struct UniqueIdCompound
            {
                uint64_t timev : 40;        // 34 Year of time in millisecond resolution
                uint64_t counter : 14;      // 16,384 unique ids per-millisecond
                uint64_t instance_id : 10;  // 1,024 instances
            };
            union UniqueIdUnion
            {
                UniqueIdCompound compound;
                uint64_t value;
            };
            UniqueIdUnion u;
        };
    };

    UniqueIdGenerator mUniqueIdGenerator;
};

extern EA_THREAD_LOCAL ExampleSlaveImpl* gExampleSlave;

extern const char8_t PUBLIC_EXAMPLE_ITEM_FIELD[];
extern const char8_t PRIVATE_EXAMPLE_ITEM_FIELD[];
extern const char8_t* VALID_EXAMPLE_ITEM_FIELD_PREFIXES[];

} // Example
} // Blaze

#endif // BLAZE_EXAMPLE_SLAVEIMPL_H

