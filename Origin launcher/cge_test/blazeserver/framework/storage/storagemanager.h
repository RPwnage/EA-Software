/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STORAGE_MANAGER_H
#define BLAZE_STORAGE_MANAGER_H

/*** Include files *******************************************************************************/
#include "framework/storage/storagelistener.h"
#include "framework/tdf/storage_server.h"
#include "framework/rpc/storageslave_stub.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class StorageTable;
class StorageRecordSnapshot;

class StorageManager : public StorageSlaveStub
{
    NON_COPYABLE(StorageManager);
public:
    typedef eastl::vector<InstanceId> InstanceIdList;

    StorageManager();
    ~StorageManager() override; // NOTE: Does not delete any tables, checks that they have all been released
    
    void registerStorageTable(StorageTable& table);
    void deregisterStorageTable(StorageTable& table);
    
    void registerStorageListener(StorageListener& listener);
    void deregisterStorageListener(StorageListener& listener);

    void sendStorageRecordUpdatedToInstances(const InstanceIdList& instanceIdList, const StorageRecordUpdate& update);
    void sendStorageRecordErasedToInstances(const InstanceIdList& instanceIdList, const StorageRecordErase& erase);

    SlaveSession* getSlaveSessionByInstanceId(InstanceId instanceId) { return mSlaveList.getSlaveSessionByInstanceId(instanceId); }

    GetStorageMetricsError::Error processGetStorageMetrics(GetStorageMetricsResponse &response, const Message*) override;

private:
    typedef eastl::hash_map<SliverNamespace, StorageTable*> StorageTableMap;
    typedef eastl::hash_map<SliverNamespace, StorageListener*> StorageListenerMap;

    SyncStorageSliverError::Error processSyncStorageSliver(const SyncStorageSliverRequest &request, const Message*) override;
    UnsyncStorageSliverError::Error processUnsyncStorageSliver(const UnsyncStorageSliverRequest &request, const Message*) override;
    void getStatusInfo(ComponentStatus& status) const override;
    void onStorageRecordUpdated(const StorageRecordUpdate& data, UserSession *associatedUserSession) override;
    void onStorageRecordErased(const StorageRecordErase& data, UserSession *associatedUserSession) override;

    StorageTableMap mStorageTableMap;
    StorageListenerMap mStorageListenerMap;
};

extern EA_THREAD_LOCAL StorageManager* gStorageManager;

} // Blaze


#endif // BLAZE_STORAGE_MANAGER_H
