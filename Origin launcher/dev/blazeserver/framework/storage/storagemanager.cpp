/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/storage/storagesnapshot.h"
#include "framework/storage/storagetable.h"
#include "framework/storage/storagemanager.h"
#include "framework/slivers/slivermanager.h"

namespace Blaze
{

StorageManager::StorageManager() 
{
    EA_ASSERT(gStorageManager == nullptr);
    gStorageManager = this;
}

StorageManager::~StorageManager()
{
    gStorageManager = nullptr;
}

SyncStorageSliverError::Error StorageManager::processSyncStorageSliver(const SyncStorageSliverRequest &request, const Message*)
{
    SyncStorageSliverError::Error result = SyncStorageSliverError::ERR_SYSTEM;

    EA_ASSERT(request.getHaveRecordIds().size() == request.getHaveRecordVersions().size());

    StorageTableMap::const_iterator tableItr = mStorageTableMap.find(request.getSliverNamespace());
    if (tableItr != mStorageTableMap.end())
    {
        tableItr->second->syncStorageSliverToInstance(request);
        result = SyncStorageSliverError::ERR_OK;
    }

    return result;
}

UnsyncStorageSliverError::Error StorageManager::processUnsyncStorageSliver(const UnsyncStorageSliverRequest &request, const Message*)
{
    UnsyncStorageSliverError::Error result = UnsyncStorageSliverError::ERR_SYSTEM;

    StorageTableMap::const_iterator tableItr = mStorageTableMap.find(request.getSliverNamespace());
    if (tableItr != mStorageTableMap.end())
    {
        tableItr->second->unsyncStorageSliverToInstance(request);
        result = UnsyncStorageSliverError::ERR_OK;
    }

    return result;
}

GetStorageMetricsError::Error StorageManager::processGetStorageMetrics(GetStorageMetricsResponse &response, const Message*)
{
    GetStorageMetricsError::Error result = GetStorageMetricsError::ERR_OK;

    response.getTableMetrics().reserve(mStorageTableMap.size());

    for (StorageTableMap::const_iterator i = mStorageTableMap.begin(), e = mStorageTableMap.end(); i != e; ++i)
    {
        const StorageTable* table = i->second;
        StorageTableMetricsPtr tableMetrics = response.getTableMetrics().pull_back();
        tableMetrics->setSliverNamespaceStr(table->getSliverNamespaceStr());

        Metrics::SumsByTagValue sums;
        table->mFieldChanges.aggregate({ Metrics::Tag::fiber_context }, sums);
        tableMetrics->getChangesByFiber().reserve(sums.size());
        for(auto& entry : sums)
            tableMetrics->getChangesByFiber()[entry.first[0]] = entry.second;

        sums.clear();
        table->mFieldChanges.aggregate({ Metrics::Tag::storage_prefix }, sums);
        tableMetrics->getChangesByField().reserve(sums.size());
        for(auto& entry : sums)
            tableMetrics->getChangesByField()[entry.first[0]] = entry.second;

        sums.clear();
        table->mFieldUpdates.aggregate({ Metrics::Tag::storage_prefix }, sums);
        tableMetrics->getUpdatesByField().reserve(sums.size());
        for(auto& entry : sums)
            tableMetrics->getUpdatesByField()[entry.first[0]] = entry.second;

        sums.clear();
        table->mFieldIgnores.aggregate({ Metrics::Tag::storage_prefix }, sums);
        tableMetrics->getIgnoresByField().reserve(sums.size());
        for(auto& entry : sums)
            tableMetrics->getIgnoresByField()[entry.first[0]] = entry.second;

        tableMetrics->setTotalFieldChanges(table->mFieldChanges.getTotal());
        tableMetrics->setTotalFieldUpdates(table->mFieldUpdates.getTotal());
        tableMetrics->setTotalFieldIgnores(table->mFieldIgnores.getTotal());
    }

    return result;
}

void StorageManager::getStatusInfo(ComponentStatus& status) const
{
    uint64_t totalFieldChanges = 0;
    uint64_t totalFieldUpdates = 0;
    uint64_t totalFieldIgnores = 0;

    for (StorageTableMap::const_iterator i = mStorageTableMap.begin(), e = mStorageTableMap.end(); i != e; ++i)
    {
        const StorageTable* table = i->second;
        totalFieldChanges += table->mFieldChanges.getTotal();
        totalFieldUpdates += table->mFieldUpdates.getTotal();
        totalFieldIgnores += table->mFieldIgnores.getTotal();
    }

    // coalescing ratio for field updates = totalFieldChanges/(totalFieldIgnores+totalFieldUpdates)

    char8_t value[32];

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, totalFieldChanges);
    status.getInfo()["TotalFieldChanges"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, totalFieldIgnores); // this is a useful metric that tracks unnecessary field submissions
    status.getInfo()["TotalFieldIgnores"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, totalFieldUpdates);
    status.getInfo()["TotalFieldUpdates"] = value;
}

void StorageManager::onStorageRecordUpdated(const StorageRecordUpdate& data, UserSession *)
{
    StorageListenerMap::iterator listenerItr = mStorageListenerMap.find(data.getSliverNamespace());
    if (listenerItr != mStorageListenerMap.end())
    {
        StorageRecordSnapshot snapshot(listenerItr->second->mStorageSnapshotAllocator, listenerItr->second->mLogCategory);
        snapshot.populateFromReplication(data);
        listenerItr->second->processRecordUpdate(snapshot);
    }
}

void StorageManager::onStorageRecordErased(const StorageRecordErase& data, UserSession *)
{
    StorageListenerMap::iterator listenerItr = mStorageListenerMap.find(data.getSliverNamespace());
    if (listenerItr != mStorageListenerMap.end())
    {
        listenerItr->second->processRecordErase(data.getRecordId());
    }
}

void StorageManager::registerStorageTable(StorageTable& table)
{
    EA_ASSERT_MSG(gStorageManager != nullptr, "StorageManager is nullptr!");

    StorageTableMap::insert_return_type ret = mStorageTableMap.insert(table.mSliverNamespace);

    if (ret.second)
    {
        ret.first->second = &table;
    }
    else
    {
        EA_ASSERT_MSG(ret.first->second == &table, "Table id collision, different table already registered for this id.");
        return;
    }

    bool result = table.mRecordKeyFieldMap.registerCollection();
    if (result)
    {
        table.mSliverOwner.setCallbacks(
            SliverOwner::ImportSliverCb(&table, &StorageTable::onImportSliver),
            SliverOwner::ExportSliverCb(&table, &StorageTable::onExportSliver));
        gSliverManager->registerSliverOwner(table.mSliverOwner);
    }
    else
    {
        EA_ASSERT_MSG(result, "Failed to register collection!");
    }
}

void StorageManager::deregisterStorageTable(StorageTable& table)
{
    StorageTableMap::iterator tableItr = mStorageTableMap.find(table.mSliverNamespace);
    if (tableItr != mStorageTableMap.end())
    {
        if (&table == tableItr->second)
        {
            gSliverManager->deregisterSliverOwner(table.mSliverOwner);
            mStorageTableMap.erase(tableItr);
        }
        else
        {
            EA_FAIL_MSG("Table id collision, this table is not registered for this id.");
        }
    }
}

void StorageManager::registerStorageListener(StorageListener& listener)
{
    EA_ASSERT_MSG(gStorageManager != nullptr, "StorageManager is nullptr!");
    StorageListenerMap::insert_return_type ret = mStorageListenerMap.insert(listener.mSliverNamespace);
    if (ret.second)
    {
        ret.first->second = &listener;
    }
    else
    {
        EA_ASSERT_MSG(ret.first->second == &listener, "Table id collision, different listener already registered for this id.");
        return;
    }

    BlazeRpcError rc = notificationSubscribe();
    if (rc == ERR_OK)
    {
        listener.mSliverListener.setCallbacks(
            SliverListener::SyncSliverCb(&listener, &StorageListener::onSyncSliver),
            SliverListener::DesyncSliverCb(&listener, &StorageListener::onDesyncSliver));

        gSliverManager->registerSliverListener(listener.mSliverListener);

        listener.mIsRegistered = true;
    }
    else
    {
        BLAZE_ASSERT_LOG(gStorageManager->getLogCategory(), "[StorageManager].registerStorageListener: "
            "Failed to register for notifications for: " << listener.getSliverNamespaceStr() << ", err: " << ErrorHelp::getErrorName(rc)); 
    }
}

void StorageManager::deregisterStorageListener(StorageListener& listener)
{
    StorageListenerMap::iterator listenerItr = mStorageListenerMap.find(listener.mSliverNamespace);
    if (listenerItr != mStorageListenerMap.end())
    {
        if (&listener == listenerItr->second)
        {
            gSliverManager->deregisterSliverListener(listener.mSliverListener);
            mStorageListenerMap.erase(listenerItr);
            listener.mIsRegistered = false;
        }
        else
        {
            EA_FAIL_MSG("Table id collision, this listener is not registered for this id.");
        }
    }
}

struct InstanceIdFetcher
{
    InstanceIdFetcher() {}
    InstanceId operator()(InstanceId instanceId)
    {
        return instanceId;
    }
};

void StorageManager::sendStorageRecordUpdatedToInstances(const InstanceIdList& instanceIdList, const StorageRecordUpdate& update)
{
    sendStorageRecordUpdatedToInstancesById(instanceIdList.begin(), instanceIdList.end(), InstanceIdFetcher(), &update, true);
}

void StorageManager::sendStorageRecordErasedToInstances(const InstanceIdList& instanceIdList, const StorageRecordErase& erase)
{
    sendStorageRecordErasedToInstancesById(instanceIdList.begin(), instanceIdList.end(), InstanceIdFetcher(), &erase, true);
}

EA_THREAD_LOCAL StorageManager* gStorageManager = nullptr;

// static
StorageSlave* StorageSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "StorageManager") StorageManager;
}

} // namespace Blaze

