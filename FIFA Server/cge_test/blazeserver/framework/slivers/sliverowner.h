/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVEROWNER_H
#define BLAZE_SLIVEROWNER_H

/*** Include files *******************************************************************************/

#include "framework/redis/redisid.h"
#include "framework/util/dispatcher.h"
#include "framework/callback.h"
#include "framework/system/fibermutex.h"
#include "framework/slivers/ownedsliver.h"
#include "framework/slivers/slivermetrics.h"

#include "framework/rpc/sliversslave_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class OwnedSliver;

class SliverOwner
{
    NON_COPYABLE(SliverOwner);

    static const uint32_t EJECT_RETRY_INTERVAL_MS = 5000;

public:
    SliverOwner(SliverNamespace sliverNamespace);
    virtual ~SliverOwner();

    typedef Functor2<OwnedSliver&, BlazeRpcError&> ImportSliverCb;
    typedef Functor2<OwnedSliver&, BlazeRpcError&> ExportSliverCb;
    void setCallbacks(const ImportSliverCb& importCallback, const ExportSliverCb& exportCallback);

    SliverNamespace getSliverNamespace() const { return mSliverNamespace; }
    const char8_t* getSliverNamespaceStr() const;

    OwnedSliver* getOwnedSliver(SliverId sliverId);

    /* Returns a SliverId that has the least number of objects assigned to it. */
    OwnedSliver* getLeastLoadedOwnedSliver();

    int64_t getOwnedSliverCount() const { return mOwnedSliverBySliverIdMap.size(); }
    int64_t getOwnedObjectCount() const;

    TimeValue getUpdateCoalescePeriod() const { return mUpdateCoalescePeriod; }

private:
    friend class SliverManager;

    BlazeRpcError importSliver(SliverId sliverId, SliverRevisionId revisionId);
    void doImportSliver(SliverId sliverId, SliverRevisionId revisionId);
    BlazeRpcError exportSliver(SliverId sliverId, SliverRevisionId revisionId);
    void doExportSliver(OwnedSliver* ownedSliver, SliverRevisionId revisionId);

    void syncSibling(SlaveSession& slaveSession);
    void repairSibling(InstanceId siblingInstanceId, const SliverIdList& sliverIdList);

    void beginDraining();
    void executeDrainTasks();

    void setUpdateCoalescePeriod(TimeValue period);

    typedef eastl::hash_map<SliverId, OwnedSliver*> OwnedSliverBySliverIdMap;
    const OwnedSliverBySliverIdMap& getOwnedSliverBySliverIdMap() const { return mOwnedSliverBySliverIdMap; }

private:
    SliverNamespace mSliverNamespace;

    ImportSliverCb mImportCallback;
    ExportSliverCb mExportCallback;

    OwnedSliverBySliverIdMap mOwnedSliverBySliverIdMap;
    OwnedSliverByPriorityMap mOwnedSliverByPriorityMap;

    Fiber::FiberId mHandleDrainRequestFiberId;
    TimerId mHandleDrainRequestTimerId;

    typedef eastl::hash_map<SliverId, Fiber::WaitList> FiberWaitListBySliverId;
    FiberWaitListBySliverId mImportFiberWaitListBySliverId;

    TimeValue mUpdateCoalescePeriod;

    SliverOwnerMetrics mMetrics;
};

} // Blaze

#endif // BLAZE_SLIVEROWNER_H
