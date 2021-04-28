/*************************************************************************************************/
/*!
    \file
        slivercoordinator.cpp"

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"

#include "framework/connection/selector.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/redis/redismanager.h"
#include "framework/redis/rediscluster.h"
#include "framework/slivers/slivercoordinator.h"
#include "framework/slivers/slivermanager.h"
#include "framework/slivers/slivernamespaceimpl.h"

namespace Blaze
{ 
    // argv[1] is instanceId or -1, 
    // argv[2] is coord expiry , 
    // argv[3] is activationTimeout
const char8_t* SliverCoordinator::msSliverCoordinatorRedisKey = "slivers.coordinator.instanceId";
RedisScript SliverCoordinator::msSliverCoordinatorScript(1,
    "local result = false;"
    "if (tonumber(ARGV[1]) ~= -1) then" // if instanceId is not -1(local coordinator allowed), set our instanceId with activation timeout as key expiry but only if key does not exist.  
    "    result = redis.call('SET', KEYS[1], ARGV[1], 'PX', ARGV[3], 'NX');"
    "end;"
    "if (result == false) then" // if some other instance or somehow redis failed,
    "    local value = redis.call('GET', KEYS[1]);"// get the id, 
    "    if (value == ARGV[1]) then"// if id is same as our instance id 
    "        redis.call('PEXPIRE', KEYS[1], ARGV[2]);"//, set up key expiry to a shorter time.
    "    end;"
    "    return tonumber(value);"
    "else"
    "    return tonumber(ARGV[1]);"
    "end;");

SliverCoordinator::SliverCoordinator() :
    mIsActivating(false),
    mCurrentInstanceId(INVALID_INSTANCE_ID),
    mResolverTimerId(INVALID_TIMER_ID),
    mResolverFiberId(Fiber::INVALID_FIBER_ID),
    mResolverFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID)
{
}

SliverCoordinator::~SliverCoordinator()
{
    for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
    {
        delete it->second;
    }
}

void SliverCoordinator::activate()
{
    mResolverFiberGroupId = Fiber::allocateFiberGroupId();
    scheduleRefreshSliverCoordinatorRedisKey();
}

void SliverCoordinator::shutdown()
{
    if (Fiber::isFiberIdInUse(mResolverFiberId))
    {
        BlazeRpcError rc = Fiber::join(mResolverFiberGroupId);
        if (rc != ERR_OK)
        {
            Fiber::cancelGroup(mResolverFiberGroupId, rc);
        }
    }

    if (mResolverTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mResolverTimerId);
        mResolverTimerId = INVALID_TIMER_ID;
    }
}

void SliverCoordinator::reconfigure()
{
    if (isDesignatedCoordinator())
    {
        for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
        {
            EA_ASSERT(it->second != nullptr);

            SliverNamespaceImpl& namespaceImpl = *it->second;

            SliverManager::SliverNamespaceData* namespaceData = gSliverManager->getOrCreateSliverNamespace(namespaceImpl.getSliverNamespace());
            if (namespaceData != nullptr)
                namespaceImpl.configure(namespaceData->mInfo.getConfig());
        }
    }
}

SliverNamespaceImpl* SliverCoordinator::getOrCreateSliverNamespaceImpl(SliverNamespace sliverNamespace, bool canCreate)
{
    SliverNamespaceImpl* namespaceImpl = nullptr;

    SliverNamespaceImplBySliverNamespace::iterator namespaceImplIt = mSliverNamespaceImplBySliverNamespace.find(sliverNamespace);
    if (namespaceImplIt != mSliverNamespaceImplBySliverNamespace.end())
    {
        namespaceImpl = namespaceImplIt->second;
    }
    else if (canCreate)
    {
        bool isKnown = false;
        const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(sliverNamespace, &isKnown);
        if (isKnown)
        {
            SliverNamespaceConfigMap::const_iterator it = gSliverManager->getConfig().getSliverNamespaces().find(componentName);
            if (it == gSliverManager->getConfig().getSliverNamespaces().end())
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.migrateSliver: 'Known' sliver namespace for component (" << componentName << ") not found.");
                return nullptr;
            }

            namespaceImpl = BLAZE_NEW SliverNamespaceImpl(*this, sliverNamespace);
            mSliverNamespaceImplBySliverNamespace[sliverNamespace] = namespaceImpl;

            namespaceImpl->configure(*it->second.get());


            if (!isActivating())
                namespaceImpl->activate();
        }
    }

    return namespaceImpl;
}

SliverNamespaceImpl* SliverCoordinator::getOrCreateSliverNamespaceImpl(const char8_t* sliverNamespaceStr, bool canCreate)
{
    SliverNamespaceImpl* namespaceImpl = nullptr;

    SliverNamespace sliverNamespace = BlazeRpcComponentDb::getComponentIdByName(sliverNamespaceStr);
    if (sliverNamespace != Component::INVALID_COMPONENT_ID)
        namespaceImpl = getOrCreateSliverNamespaceImpl(sliverNamespace, canCreate);

    return namespaceImpl;
}

InstanceId SliverCoordinator::fetchActiveInstanceId()
{
    FiberAutoMutex autoMutex(mResolverFiberMutex);
    if (autoMutex.hasLock())
    {
        InstanceId prevInstanceId = mCurrentInstanceId;

        while (!gController->isShuttingDown() && !Fiber::isCurrentlyCancelled())
        {
            InstanceId activeInstanceId = INVALID_INSTANCE_ID;

            bool localCoordinatorAllowed;
            if (hasOngoingMigrationTasks())
                localCoordinatorAllowed = true;
            else if (!gController->isReadyForService() || gController->isDraining())
                localCoordinatorAllowed = false;
            else if (gSliverManager->getConfig().getPreferredCoordinatorTypes().empty())
                localCoordinatorAllowed = true;
            else
            {
                localCoordinatorAllowed = false;
                SliversConfig::InstanceTypeNameList::const_iterator it = gSliverManager->getConfig().getPreferredCoordinatorTypes().begin();
                SliversConfig::InstanceTypeNameList::const_iterator end = gSliverManager->getConfig().getPreferredCoordinatorTypes().end();
                for (; it != end; ++it)
                {
                    const char8_t* preferredInstanceTypeName = it->c_str();

                    if (blaze_stricmp(gController->getBaseName(), preferredInstanceTypeName) == 0)
                    {
                        localCoordinatorAllowed = true;
                        break;
                    }

                    Component::InstanceIdList remoteInstances;
                    gSliverManager->getComponentInstances(remoteInstances, false);
                    bool foundPreferredType = false;
                    for (Component::InstanceIdList::iterator remoteIt = remoteInstances.begin(), remoteEnd = remoteInstances.end(); remoteIt != remoteEnd; ++remoteIt)
                    {
                        const RemoteServerInstance* remoteServerInstance = gController->getRemoteServerInstance(*remoteIt);
                        if ((remoteServerInstance != nullptr) && (blaze_stricmp(remoteServerInstance->getInstanceTypeName(), preferredInstanceTypeName) == 0))
                        {
                            foundPreferredType = true;
                            break;
                        }
                    }

                    if (foundPreferredType)
                        break;
                }
            }

            RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
            if (cluster != nullptr)
            {
                RedisCommand cmd;
                cmd.addKey(msSliverCoordinatorRedisKey);
                cmd.add(localCoordinatorAllowed ? gController->getInstanceId() : -1);
                cmd.add(gSliverManager->getConfig().getCoordinatorExpiryPeriod().getMillis());
                // The additional 5 second padding is somewhat arbitrary.  It's meant to give just a small additional
                // grace-period to allow for unexpected delays in talking to redis during cluster startup.
                cmd.add(gSliverManager->getConfig().getCoordinatorActivationTimeout().getMillis() + (5 * 1000));

                RedisResponsePtr resp;
                RedisError redisErr = cluster->execScript(msSliverCoordinatorScript, cmd, resp);
                if ((redisErr == REDIS_ERR_OK) && resp->isInteger())
                {
                    activeInstanceId = (InstanceId)resp->getInteger();
                }
            }

            // If there has been no change to which instanceId is the current coordinator, then just break;
            if (mCurrentInstanceId == activeInstanceId)
                break;

            if (activeInstanceId == gController->getInstanceId())
            {
                if (!localCoordinatorAllowed)
                {
                    Fiber::sleep(1 * 1000 * 1000, "SliverCoordinator::fetchActiveInstanceId");
                    continue;
                }

                mIsActivating = true;

                StateBySliverNamespaceMap localStates;
                gSliverManager->filloutSliverNamespaceStates(localStates);
                updateSliverNamespaceStates(gController->getInstanceId(), localStates);

                for (SlaveSessionList::iterator it = gSliverManager->mSlaveList.begin(), end = gSliverManager->mSlaveList.end(); it != end; ++it)
                {
                    SlaveSession& slaveSession = *it->second;
                    handleSlaveSessionAdded(slaveSession.getInstanceId());
                }

                BlazeRpcError err = ERR_OK;
                if (!mPendingSubmissionInstanceIdSet.empty())
                    err = mIsReadyWaitList.wait("SliverCoordinator::fetchActiveInstanceId", TimeValue::getTimeOfDay() + gSliverManager->getConfig().getCoordinatorActivationTimeout());

                mIsActivating = false;

                if (err == ERR_OK)
                    mCurrentInstanceId = activeInstanceId;
                else
                {
                    BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.fetchActiveInstanceId: Did not get a response from all members of the cluster, err(" << ErrorHelp::getErrorName(err) << ")");
                }

                continue;
            }

            mCurrentInstanceId = activeInstanceId;
            break;
        }

        if (prevInstanceId != mCurrentInstanceId)
        {
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.fetchActiveInstanceId: The designated SliverCoordinator has been updated from " << RoutingOptions(prevInstanceId) << " to " << RoutingOptions(mCurrentInstanceId));

            for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
            {
                EA_ASSERT(it->second != nullptr);
                SliverNamespaceImpl& namespaceImpl = *it->second;

                // We've just become the cluster's sliver coordinator, activate all the namespaces now that we should
                // have received all of the state from the cluster members.  Note, this call to activate does not have an
                // immediate effect, it schedule a job in the namespace's queue that will enable and perform rebalancing once
                // the other updateMasterSliverState() jobs (created by the rest of the cluster "reporting in" with their state)
                // have been processed.
                if (isDesignatedCoordinator())
                    namespaceImpl.activate();
                else
                    namespaceImpl.deactivate();

                namespaceImpl.updatePartitionCount();
            }
        }
    }
    else
    {
        mCurrentInstanceId = INVALID_INSTANCE_ID;
    }

    if (mResolverTimerId != INVALID_TIMER_ID)
        gSelector->updateTimer(mResolverTimerId, TimeValue::getTimeOfDay() + gSliverManager->getConfig().getCoordinatorRefreshInterval());

    return mCurrentInstanceId;
}

void SliverCoordinator::scheduleRefreshSliverCoordinatorRedisKey(TimeValue relativeTimeout)
{
    Fiber::CreateParams params;
    params.groupId = mResolverFiberGroupId;
    mResolverTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + relativeTimeout, this, &SliverCoordinator::refreshSliverCoordinatorRedisKey, "SliverCoordinator::refreshSliverCoordinatorRedisKey");
}

void SliverCoordinator::refreshSliverCoordinatorRedisKey()
{
    mResolverFiberId = Fiber::getCurrentFiberId();

    gSelector->cancelTimer(mResolverTimerId);
    mResolverTimerId = INVALID_TIMER_ID;

    InstanceId activeInstanceId = fetchActiveInstanceId();
    if (activeInstanceId == INVALID_INSTANCE_ID)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.refreshSliverCoordinatorRedisKey: No SliverCoordinator is currently designated as the authority.");
    }

    scheduleRefreshSliverCoordinatorRedisKey(gSliverManager->getConfig().getCoordinatorRefreshInterval());

    mResolverFiberId = Fiber::INVALID_FIBER_ID;
}

void SliverCoordinator::addPendingSlaveInstanceId(InstanceId instanceId)
{
    if (mIsActivating)
        mPendingSubmissionInstanceIdSet.insert(instanceId);
}

void SliverCoordinator::removePendingSlaveInstanceId(InstanceId instanceId)
{
    if (mIsActivating)
    {
        mPendingSubmissionInstanceIdSet.erase(instanceId);
        if (mPendingSubmissionInstanceIdSet.empty())
            mIsReadyWaitList.signal(ERR_OK);
    }
}

void SliverCoordinator::handleSlaveSessionAdded(InstanceId instanceId)
{
    addPendingSlaveInstanceId(instanceId);

    if (isActivating() || isDesignatedCoordinator())
    {
        SliverCoordinatorInfo info;
        info.setInstanceId(gController->getInstanceId());
        gSliverManager->sendSliverCoordinatorActivatedToInstanceById(instanceId, &info);

        for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
        {
            SliverNamespaceImpl& namespaceImpl = *it->second;
            if ((namespaceImpl.getConfig().getPartitionCount() == 0) && (namespaceImpl.getCurrentSliverPartitionCount() != 0))
            {
                SliverListenerPartitionCountUpdate update;
                update.setSliverNamespace(namespaceImpl.getSliverNamespace());
                update.setPartitionCount(namespaceImpl.getCurrentSliverPartitionCount());
                gSliverManager->sendUpdateSliverListenerPartitionCountToInstanceById(instanceId, &update, true);
            }
        }
    }
}

void SliverCoordinator::handleSlaveSessionRemoved(InstanceId instanceId)
{
    if (isActivating() || isDesignatedCoordinator())
    {
        for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
        {
            SliverNamespaceImpl& namespaceImpl = *it->second;
            namespaceImpl.updateSliverManagerInstanceState(instanceId, nullptr);
        }
    }

    removePendingSlaveInstanceId(instanceId);
}

BlazeRpcError SliverCoordinator::updateSliverNamespaceStates(InstanceId instanceId, const StateBySliverNamespaceMap& states)
{
    if (!isActivating() && !isDesignatedCoordinator())
        return ERR_SYSTEM;

    StateBySliverNamespaceMap::const_iterator it = states.begin();
    StateBySliverNamespaceMap::const_iterator end = states.end();
    for (; it != end; ++it)
    {
        SliverNamespaceImpl* namespaceImpl = getOrCreateSliverNamespaceImpl(it->first, true);
        if (namespaceImpl != nullptr)
            namespaceImpl->updateSliverManagerInstanceState(instanceId, it->second);
    }

    removePendingSlaveInstanceId(instanceId);

    return ERR_OK;
}

BlazeRpcError SliverCoordinator::migrateSliver(const MigrateSliverRequest& request, MigrateSliverResponse& response)
{
    InstanceId activeInstanceId = fetchActiveInstanceId();
    if (activeInstanceId == INVALID_INSTANCE_ID)
        return ERR_SYSTEM;

    if (activeInstanceId != gController->getInstanceId())
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(activeInstanceId);
        return gSliverManager->migrateSliver(request, response, opts);
    }

    Fiber::BlockingSuppressionAutoPtr suppressBlocking;

    SliverNamespaceImpl* namespaceImpl = getOrCreateSliverNamespaceImpl(request.getSliverNamespaceStr());
    if (namespaceImpl == nullptr)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.migrateSliver: Sliver namespace(" << request.getSliverNamespaceStr() << ") not found.");
        return ERR_SYSTEM;
    }

    namespaceImpl->migrateSliver(request.getSliverId(), request.getToInstanceId());
    return ERR_OK;
}

BlazeRpcError SliverCoordinator::ejectSlivers(const EjectSliversRequest& request)
{
    InstanceId activeInstanceId = fetchActiveInstanceId();
    if (activeInstanceId == INVALID_INSTANCE_ID)
        return ERR_SYSTEM;

    if (activeInstanceId != gController->getInstanceId())
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(activeInstanceId);
        return gSliverManager->ejectSlivers(request, opts);
    }

    Fiber::BlockingSuppressionAutoPtr suppressBlocking;

    SliverNamespaceImpl* namespaceImpl = getOrCreateSliverNamespaceImpl(request.getSliverNamespace());
    if (namespaceImpl == nullptr)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.ejectSlivers: SliverNamespace(" << request.getSliverNamespace() << ") not found.");
        return ERR_SYSTEM;
    }

    namespaceImpl->ejectAllSlivers(request.getFromInstanceId());
    return ERR_OK;
}

BlazeRpcError SliverCoordinator::rebalanceSlivers(const RebalanceSliversRequest& request)
{
    InstanceId activeInstanceId = fetchActiveInstanceId();
    if (activeInstanceId == INVALID_INSTANCE_ID)
        return ERR_SYSTEM;

    if (activeInstanceId != gController->getInstanceId())
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(activeInstanceId);
        return gSliverManager->rebalanceSlivers(request, opts);
    }

    Fiber::BlockingSuppressionAutoPtr suppressBlocking;

    SliverNamespaceImpl* namespaceImpl = getOrCreateSliverNamespaceImpl(request.getSliverNamespace());
    if (namespaceImpl == nullptr)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.rebalanceSlivers: SliverNamespace(" << request.getSliverNamespace() << ") not found.");
        return ERR_SYSTEM;
    }

    namespaceImpl->rebalanceSliverOwners();
    return ERR_OK;
}

BlazeRpcError SliverCoordinator::getSlivers(const GetSliversRequest& request, GetSliversResponse& response)
{
    InstanceId activeInstanceId = fetchActiveInstanceId();
    if (activeInstanceId == INVALID_INSTANCE_ID)
        return ERR_SYSTEM;

    if (activeInstanceId != gController->getInstanceId())
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(activeInstanceId);
        return gSliverManager->getSlivers(request, response, opts);
    }

    Fiber::BlockingSuppressionAutoPtr suppressBlocking;

    SliverNamespaceImpl* namespaceImpl = getOrCreateSliverNamespaceImpl(request.getSliverNamespace());
    if (namespaceImpl == nullptr)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverCoordinator.getSlivers: SliverNamespace(" << request.getSliverNamespace() << ") not found.");
        return ERR_SYSTEM;
    }

    SliverIdList& sliverIdList = response.getSliverIdList();
    sliverIdList.reserve(namespaceImpl->getConfig().getSliverCount());
    for (SliverId sliverId = SLIVER_ID_MIN; sliverId <= namespaceImpl->getConfig().getSliverCount(); ++sliverId)
    {
        if (namespaceImpl->isKnownOwnerOfSliver(sliverId, request.getFromInstanceId()))
        {
            sliverIdList.push_back(sliverId);
        }
    }

    return ERR_OK;
}

bool SliverCoordinator::isDrainComplete()
{
    return !mIsActivating && !hasOngoingMigrationTasks() && (mCurrentInstanceId != gController->getInstanceId());
}

bool SliverCoordinator::isDesignatedCoordinator() const
{
    return (!mIsActivating && (mCurrentInstanceId == gController->getInstanceId()));
}

bool SliverCoordinator::hasOngoingMigrationTasks()
{
    for (SliverNamespaceImplBySliverNamespace::iterator it = mSliverNamespaceImplBySliverNamespace.begin(), end = mSliverNamespaceImplBySliverNamespace.end(); it != end; ++it)
    {
        SliverNamespaceImpl* namespaceImpl = it->second;
        if (!namespaceImpl->isDrainComplete())
            return true;
    }

    return false;
}

} //namespace Blaze
