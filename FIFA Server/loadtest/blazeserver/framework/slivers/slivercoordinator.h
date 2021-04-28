/*************************************************************************************************/
/*!
    \file
        slivercoordinator.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVERCOORDINATOR_H
#define BLAZE_SLIVERCOORDINATOR_H

/*** Include files *******************************************************************************/

#include "framework/redis/redisid.h"
#include "framework/tdf/slivermanagertypes_server.h"
#include "framework/controller/controller.h"

namespace Blaze
{
struct RedisScript;
class SliverNamespaceImpl;

class SliverCoordinator
{
    NON_COPYABLE(SliverCoordinator);
public:

    SliverCoordinator();
    virtual ~SliverCoordinator();

    void activate();
    void shutdown();
    void reconfigure();

    bool isActivating() const { return mIsActivating; }
    bool isDesignatedCoordinator() const;
    bool hasOngoingMigrationTasks();

    InstanceId fetchActiveInstanceId();

    BlazeRpcError migrateSliver(const MigrateSliverRequest& request, MigrateSliverResponse& response);
    BlazeRpcError ejectSlivers(const EjectSliversRequest& request);
    BlazeRpcError rebalanceSlivers(const RebalanceSliversRequest& request);
    BlazeRpcError getSlivers(const GetSliversRequest& request, GetSliversResponse& response);

private:
    friend class SliverManager;
    friend class SliverNamespaceImpl;

    SliverNamespaceImpl* getOrCreateSliverNamespaceImpl(SliverNamespace sliverNamespace, bool canCreate = false);
    SliverNamespaceImpl* getOrCreateSliverNamespaceImpl(const char8_t* sliverNamespaceStr, bool canCreate = false);

    BlazeRpcError updateSliverNamespaceStates(InstanceId instanceId, const StateBySliverNamespaceMap& states);

    void handleSlaveSessionAdded(InstanceId instanceId);
    void handleSlaveSessionRemoved(InstanceId instanceId);

    void scheduleRefreshSliverCoordinatorRedisKey(EA::TDF::TimeValue relativeTimeout = EA::TDF::TimeValue());
    void refreshSliverCoordinatorRedisKey();

    void addPendingSlaveInstanceId(InstanceId instanceId);
    void removePendingSlaveInstanceId(InstanceId instanceId);

    bool isDrainComplete();

private:
    typedef eastl::hash_map<SliverNamespace, SliverNamespaceImpl*> SliverNamespaceImplBySliverNamespace;
    SliverNamespaceImplBySliverNamespace mSliverNamespaceImplBySliverNamespace;

    typedef eastl::vector_set<InstanceId> InstanceIdSet;
    InstanceIdSet mPendingSubmissionInstanceIdSet;
    Fiber::WaitList mIsReadyWaitList;
    bool mIsActivating;

    InstanceId mCurrentInstanceId;
    TimerId mResolverTimerId;
    Fiber::FiberId mResolverFiberId;
    Fiber::FiberGroupId mResolverFiberGroupId;
    FiberMutex mResolverFiberMutex;

    static RedisScript msSliverCoordinatorScript;
    static const char8_t* msSliverCoordinatorRedisKey;
};

}

#endif // BLAZE_SLIVERCOORDINATOR_H
