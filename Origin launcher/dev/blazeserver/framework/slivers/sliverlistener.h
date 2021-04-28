/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVERLISTENER_H
#define BLAZE_SLIVERLISTENER_H

/*** Include files *******************************************************************************/

#include "framework/redis/redisid.h"
#include "framework/util/dispatcher.h"
#include "framework/callback.h"
#include "framework/system/fibermutex.h"

#include "framework/rpc/sliversslave_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SliverListener
{
    NON_COPYABLE(SliverListener);
public:
    SliverListener(SliverNamespace sliverNamespace);
    virtual ~SliverListener();

    SliverNamespace getSliverNamespace() const { return mSliverNamespace; }
    const SliverPartitionInfo& getPartitionInfo() const { return mPartitionInfo; }

    typedef Functor2<SliverPtr, BlazeRpcError&> SyncSliverCb;
    typedef Functor2<SliverPtr, BlazeRpcError&> DesyncSliverCb;
    void setCallbacks(const SyncSliverCb& syncSliverCb, const DesyncSliverCb& desyncSliverCb) { mSyncSliverCb = syncSliverCb; mDesyncSliverCb = desyncSliverCb; }

    bool isSliverSynced(SliverId sliverId) const { return (mSyncedSliverIds.find(sliverId) != mSyncedSliverIds.end()); }

    void shutdown();

private:
    friend class SliverManager;

    void notifyUpdateAssignedSlivers(const SliverPartitionInfo& partitionInfo);
    void notifySliverInstanceIdChanged(SliverId sliverId);

    void syncSibling(SlaveSession& slaveSession);

    Fiber::FiberGroupId getSyncFiberGroupId()
    {
        if (mSyncFiberGroupId == Fiber::INVALID_FIBER_GROUP_ID)
            mSyncFiberGroupId = Fiber::allocateFiberGroupId();
        return mSyncFiberGroupId;
    }

private:
    SliverNamespace mSliverNamespace;
    SliverPartitionInfo mPartitionInfo;

    Fiber::FiberGroupId mSyncFiberGroupId;

    SyncSliverCb mSyncSliverCb;
    DesyncSliverCb mDesyncSliverCb;

    SliverIdSet mSyncedSliverIds;

    FiberMutex mUpdateAssignedSliversMutex;
    bool mStopCurrentUpdateOperation;
    bool mShutdown;
};

} // Blaze

#endif // BLAZE_SLIVERSETPARTITIONLISTENER_H
