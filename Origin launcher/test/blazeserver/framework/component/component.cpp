#include "framework/blaze.h"
#include "framework/system/fibermanager.h"
#include "framework/connection/selector.h"
#include "framework/component/component.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/component/rpcproxyresolver.h" 
#include "framework/component/componentmanager.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/config/configmerger.h"
#include "framework/replication/replicator.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/util/random.h"
#include "framework/slivers/slivermanager.h"
#include "EASTL/string.h"
#include "EASTL/scoped_ptr.h"


namespace Blaze
{

const ComponentId Component::INVALID_COMPONENT_ID = RPC_COMPONENT_UNKNOWN;
const CommandId Component::INVALID_COMMAND_ID = 0xFFFF;
static const double DEFAULT_RESTART_TIMEOUT_FRACTION = 2.0/3.0;

InstanceId Component::getControllerInstanceId()
{
    return gController->getInstanceId();
}

Component::Component(const ComponentData& info, RpcProxyResolver& resolver, const InetAddress& externalAddress)
    : mResolver(resolver),
     mExternalAddress(externalAddress),
     mComponentInfo(info),
     mAllocator(*Allocator::getAllocator(getMemGroupId())),
     mRestartingInstanceMap(BlazeStlAllocator("Component::mRestartingInstanceMap", getMemGroupId())),
     mAutoRegInstancesForNotifications(false),
     mAutoRegInstancesForReplication(false),
     mLastContextId(0)
{
}

Component::~Component()
{
    // Cancel all the timers queued for the restarting instances
    RestartInstanceToEntryMap::iterator it = mRestartingInstanceMap.begin();
    RestartInstanceToEntryMap::iterator end = mRestartingInstanceMap.end();
    for (; it != end; ++it)
    {
        RestartEntry& entry = it->second;
        gSelector->cancelTimer(entry.restartingInstanceTimerId);
    }
}

BlazeRpcError Component::waitForInstanceId(InstanceId instanceId, const char8_t* context, TimeValue absoluteTimeout)
{
    if (mHasSentNotificationSubscribe.find(instanceId) != mHasSentNotificationSubscribe.end())
        return ERR_OK;

    return mWaitListByInstanceIdMap[instanceId].wait(context, absoluteTimeout);
}

/*! ***************************************************************************/
/*! \brief  Adding the same component twice is a NOOP that returns true, 
            whereas adding a different component with the same instance id
            is a NOOP that returns false.
*******************************************************************************/
BlazeRpcError Component::addInstance(InstanceId instanceId)
{
    BlazeRpcError rc = ERR_OK;

    EA_ASSERT(instanceId != INVALID_INSTANCE_ID);

    //inserting twice is an no-op
    if (mInstanceIds.insert(instanceId).second)
    {
        mNonDrainingInstanceIds.insert(instanceId);
        rc = sendReplicaAndNotifForInstance(instanceId);
    }
    else
    {
        RestartInstanceToEntryMap::iterator itr = mRestartingInstanceMap.find(instanceId);
        if (itr != mRestartingInstanceMap.end())
        {
            rc = sendReplicaAndNotifForInstance(instanceId);
            if (rc == ERR_OK)
            {
                // need second lookup to ensure iterator still valid after blocking call: GOS-28860
                itr = mRestartingInstanceMap.find(instanceId);
                if (itr != mRestartingInstanceMap.end())
                {
                    // The restart instance is back in time, cancel the instance cleanup timer, and signal all the events in the map
                    EventList waitingRpcList;
                    waitingRpcList.swap(itr->second.waitingRpcList);
                    gSelector->cancelTimer(itr->second.restartingInstanceTimerId);
                    mRestartingInstanceMap.erase(itr);
                    for (EventList::const_iterator i = waitingRpcList.begin(), e = waitingRpcList.end(); i != e; ++i)
                    {
                        Fiber::signal(*i, ERR_OK);
                    }
                }
            }
        }
    }

    mWaitListByInstanceIdMap[instanceId].signal(rc);

    return rc;
}

BlazeRpcError Component::sendReplicaAndNotifForInstance(InstanceId instanceId)
{
    BlazeRpcError rc = ERR_OK;
    if (mAutoRegInstancesForReplication)
    {
        //If we're getting replication for this component, subscribe for this instance's replication
        rc = sendReplicationSubscribeForInstance(instanceId);
        if (rc != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].addInstance: Unable to sync replication for component " << getName() << " on instance " << instanceId);
        }
    }

    if (rc == ERR_OK)
    {
        if (mAutoRegInstancesForNotifications)
        {
            //If we're getting notifications for this component, subscribe for this instance's replication
            rc = sendNotificationSubscribeForInstance(instanceId);
            if (rc != ERR_OK)
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].addInstance: Unable to subscribe for notifications from component " << getName() << " on instance " << instanceId);
            }
        }
    } 
    return rc;
}

void Component::setInstanceDraining(InstanceId instanceId, bool draining)
{
    //only act on instance ids we know about
    if (hasInstanceId(instanceId))
    {
        if (draining)
        {
            mNonDrainingInstanceIds.erase(instanceId);
        }
        else
        {
            mNonDrainingInstanceIds.insert(instanceId);
        }
    }
}

int64_t Component::getInstanceRestartTimeoutUsec() const
{
    int64_t timeout = gController->getFrameworkConfigTdf().getConnectionManagerConfig().getRemoteInstanceRestartTimeout().getMicroSeconds();
    if (timeout == 0)
    {
        // Assuming average blazeserver is utilizing at least 50% CPU, after the blazeserver is back after n seconds, 
        // it needs n/2 seconds to process the held RPCs in full speed (100% CPU) and back to normal usage. 
        // We want the sum of rpc holding and processing time less than RPC timeout (n + n/2 < RPC_TIMEOUT), so we set the restart timeout to be 2/3 of rpc timeout.
        if (gController->getInternalEndpoint() != nullptr)
            timeout = static_cast<int64_t>(gController->getInternalEndpoint()->getCommandTimeout().getMicroSeconds() * DEFAULT_RESTART_TIMEOUT_FRACTION);
        else
            timeout = (10 * 1000 * 1000); // Use 10 seconds if no other settings can be found.
    }
    return timeout;
}

/*! ***************************************************************************/
/*! \brief  Removing a different component with same instance id
            is a NOOP and returns false.
*******************************************************************************/
bool Component::removeInstance(InstanceId instanceId)
{
    // Always erase subscribe trackers when instance is truly gone, ZDT or no, we'll need to re-subscribe when it's up
    mWaitListByInstanceIdMap[instanceId].signal(ERR_CANCELED);
    mHasSentNotificationSubscribe.erase(instanceId);
    mHasSentReplicationSubscribe.erase(instanceId);

    // immediately erase from sets (prevents RPCs from being held by the framework)
    eraseInstanceData(instanceId);
    
    return true;
}

void Component::eraseInstanceData(InstanceId id)
{
    // clean up other sets
    mNonDrainingInstanceIds.erase(id);
    mInstanceIds.erase(id);
}

InstanceId Component::selectInstanceId() const
{
    if (!mInstanceIds.empty())
    {
        if (gCurrentUserSession != nullptr && !gCurrentUserSession->getInstanceIds().empty())
        {
            // Determine which server instances (if any) the user has been mapped to, then select the first one
            // NOTE: This is a generic implementation that must work on core and aux servers; therefore, we always
            // iterate the *entire* list of instance ids, even though all but one should have valid assignments.
            for (InstanceIdList::const_iterator it = gCurrentUserSession->getInstanceIds().begin(), end = gCurrentUserSession->getInstanceIds().end(); it != end; ++it)
            {                
                if (mInstanceIds.find(*it) != mInstanceIds.end())
                {
                    return *it;
                }
            }
        }

        const InstanceIdSet &instanceSet = mNonDrainingInstanceIds;
        if (instanceSet.empty())
        {
            BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].selectInstanceId: No instance is available.");     
            return INVALID_INSTANCE_ID;
        }
        else
        {
            return getAnyInstanceId();
        }
    }
    else
    {
        return INVALID_INSTANCE_ID;
    }
}

SliverId Component::getSliverIdByLoad() const
{
    SliverId sliverId = INVALID_SLIVER_ID;
    // Pre-assign sliver based on weighted least loaded instance that covers the slivers
    auto instanceId = getAnyInstanceId();
    if (instanceId != INVALID_INSTANCE_ID)
    {
        sliverId = gSliverManager->getAnySliverIdByInstanceId(getId(), instanceId);
        if (sliverId == INVALID_SLIVER_ID)
        {
            BLAZE_WARN_LOG(getLogCategory(), "[Component].getSliverIdByLoad: Component(" << getName() << ") failed to get sliver from instance(" << instanceId << ") selected via load-based assignment, fallback to non load-based assignment.");
        }
    }
    else
    {
        BLAZE_WARN_LOG(getLogCategory(), "[Component].getSliverIdByLoad: Component(" << getName() << ") instances unavailable, fallback to non load-based assignment.");
    }

    if (sliverId == INVALID_SLIVER_ID)
    {
        // If all instances are down we still want to assign some sliver id; therefore, we fallback to non load based assignment in this case.
        auto sliverCount = gSliverManager->getSliverCount(getId());
        if (sliverCount > 0)
        {
            sliverId = static_cast<SliverId>(gRandom->getRandomNumber(sliverCount) + 1);
        }
        else
        {
            BLAZE_ERR_LOG(getLogCategory(), "[Component].getSliverIdByLoad: Component(" << getName() << ") has no slivers.");
        }
    }

    return sliverId;
}

InstanceId Component::getLocalInstanceId() const
{
    return gController->getInstanceId();
}


void Component::getComponentInstances(Component::InstanceIdList& instances, bool randomized, bool includeDrainingInstances) const
{
    const InstanceIdSet& instanceIdSet = includeDrainingInstances ? mInstanceIds : mNonDrainingInstanceIds;

    if (!randomized || instanceIdSet.size() <= 1)
    {
        instances = instanceIdSet;
    }
    else
    {
        size_t size = instanceIdSet.size();
        instances.clear();
        instances.resize(size, INSTANCE_ID_MAX);

        size_t startIndex = Random::getRandomNumber(size);
        size_t endIndex = startIndex + size;
        for (size_t counter = 0; startIndex < endIndex; ++counter, ++startIndex)
        {
            instances[counter] = (instanceIdSet[startIndex % size]);
        }
    }
}


InstanceId Component::getAnyInstanceId() const
{
    InstanceId result = INVALID_INSTANCE_ID;
    const InstanceIdSet& instanceSet = mNonDrainingInstanceIds;
    auto size = instanceSet.size();
    if (size == 1)
        result = instanceSet.front();
    else if (size > 1)
    {
        uint32_t inverseLoadSum = 0;
        eastl::vector_map<uint32_t, InstanceId> instanceIdByInverseLoadMap;
        instanceIdByInverseLoadMap.reserve(instanceSet.size());
        for (auto instanceId : instanceSet)
        {
            auto* remoteServer = gController->getRemoteServerInstance(instanceId);
            if (remoteServer)
            {
                // NOTE: Intentionally use user load(vs. total load) to load balance because it is statistically much more directly correlated to amount of work the instance is actually performing itself
                auto inverseLoad = RemoteServerInstance::MAX_LOAD - eastl::min(RemoteServerInstance::MAX_LOAD, remoteServer->getUserLoad());
                if (inverseLoad == 0)
                    inverseLoad = 1; // Always add one since we don't want to avoid 100% loaded instances entirely if they are all that is left
                inverseLoadSum += inverseLoad; 
                instanceIdByInverseLoadMap.emplace(inverseLoadSum, instanceId);
            }
        }

        if (!instanceIdByInverseLoadMap.empty())
        {
            uint32_t startIndex = Random::getRandomNumber(inverseLoadSum); // randomly index into load map, to avoid too aggressively targeting the least loaded intance
            auto loadItr = instanceIdByInverseLoadMap.lower_bound(startIndex);
            if (loadItr != instanceIdByInverseLoadMap.end())
                result = loadItr->second;
            else
                result = instanceIdByInverseLoadMap.back().second;
        }
    }

    return result;
}

void Component::onRemoteInstanceRestartTimeout(InstanceId id, TimeValue restartingTimeout)
{
    eraseInstanceData(id);

    RestartInstanceToEntryMap::iterator itr = mRestartingInstanceMap.find(id);
    if (itr != mRestartingInstanceMap.end())
    {
        RestartEntry& entry = itr->second;
        if (!entry.waitingRpcList.empty())
        {
            //Signal the held rpc requests
            EventList& events = entry.waitingRpcList;
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].onRemoteInstanceRestartTimeout: Instance: "<< id << " away for " << restartingTimeout.getSec() << "s, canceling "
                << events.size() << " held " << getName() << " RPCs.");

            while (!events.empty())
            {
                Fiber::EventHandle ev = events.back();
                events.pop_back();
                Fiber::signal(ev, ERR_TIMEOUT);
            }
        }
        mRestartingInstanceMap.erase(id);
    }
}

/*! ***************************************************************************/
/*! \brief Subscribes for notifications to all remote components tracked by
            this instance with the added robustness guarantee that components
            may be added or removed to the map during the subscription process.
*******************************************************************************/
BlazeRpcError Component::notificationSubscribe()
{
    BlazeRpcError rc = ERR_OK;
    if (!mAutoRegInstancesForNotifications)
    {
        mAutoRegInstancesForNotifications = true;

        //Add our local instance to the subscribed list
        if (isLocal())
        {
            mHasSentNotificationSubscribe.insert(getLocalInstanceId());
            mWaitListByInstanceIdMap[getLocalInstanceId()].signal(ERR_OK);
        }

        InstanceIdSet instanceIds = mInstanceIds; // take snapshot we can iterate safely
        for (InstanceIdSet::const_iterator i = instanceIds.begin(), e = instanceIds.end(); i != e; ++i)
        {
            if (mHasSentNotificationSubscribe.count(*i) > 0)
                continue;
            // Intentionally swallow disconnect errors when subscribing to all instances since some may be about to die and get cleaned up and we need to continue to proceed despite that.
            BlazeRpcError result = sendNotificationSubscribeForInstance(*i);
            if ((result != ERR_OK) && (result != ERR_DISCONNECTED))
            {
                BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].notificationSubscribe: Failed to subscribe for instance " << *i << ", result = " << ErrorHelp::getErrorName(result));     
                rc = result;
                break;
            }
        }
    }

    return rc;
}

bool Component::hasSentNotificationSubscribeToInstance(InstanceId instanceId)
{
    return mHasSentNotificationSubscribe.find(instanceId) != mHasSentNotificationSubscribe.end();
}

void Component::registerNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb callback)
{
    for (auto& cb : mSubscribedForNotificationsCbList)
    {
        if (cb == callback)
            return;
    }

    for (auto& instanceId : mHasSentNotificationSubscribe)
    {
        // Already sent notification subscribe for these instances; therefore, we must signal the immediately to avoid it missing those notifications
        callback(instanceId);
    }

    mSubscribedForNotificationsCbList.push_back(callback);
}

void Component::deregisterNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb callback)
{
    for (auto itr = mSubscribedForNotificationsCbList.begin(); itr != mSubscribedForNotificationsCbList.end(); ++itr)
    {
        if (*itr == callback)
        {
            mSubscribedForNotificationsCbList.erase(itr);
            return;
        }
    }
}

BlazeRpcError Component::sendNotificationSubscribeForInstance(InstanceId id)
{
    BlazeRpcError result = ERR_COMMAND_NOT_FOUND;

    const CommandInfo* cmdInfo = mComponentInfo.getCommandInfo(Component::NOTIFICATION_SUBSCRIBE_CMD);
    if (cmdInfo != nullptr)
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(id);
        opts.ignoreReply = true; // Intentionally do not wait for reply, RemoteInstance::activateInternal() needs to issue notification sub for *all* components without blocking before the remote side starts sending down updates for any given component
        result = sendRequest(*cmdInfo, nullptr, nullptr, nullptr, opts);
        if (result == ERR_OK)
        {
            mHasSentNotificationSubscribe.insert(id);
            mWaitListByInstanceIdMap[id].signal(ERR_OK);
            for (size_t i = 0; i < mSubscribedForNotificationsCbList.size(); ++i)
            {
                mSubscribedForNotificationsCbList[i](id);
            }
        }
    }

    return result;
}

/*! ***************************************************************************/
/*! \brief Subscribes for replication to all remote components tracked by
            this instance with the added robustness guarantee that components
            may be added or removed to the map during the subscription process.
*******************************************************************************/
BlazeRpcError Component::replicationSubscribe(InstanceId selectedInstanceId /* = INVALID_INSTANCE_ID */)
{
    BlazeRpcError rc = ERR_OK;
    if (!mAutoRegInstancesForReplication)
    {                
        //add our local instance into the sync'd list and subscribed list automatically
        if (isLocal())
        {
            mHasSentReplicationSubscribe.insert(getLocalInstanceId());       
        }

        if (selectedInstanceId == INVALID_INSTANCE_ID)
        {
            mAutoRegInstancesForReplication = true;
            InstanceIdSet instanceIds = mInstanceIds; // take snapshot we can iterate safely
            for (InstanceIdSet::const_iterator i = instanceIds.begin(), e = instanceIds.end(); i != e; ++i)
            {
                if (mHasSentReplicationSubscribe.count(*i) > 0)
                    continue;
                // Intentionally swallow disconnect errors when subscribing to all instances since some may be about to die and get cleaned up and we need to continue to proceed despite that.
                BlazeRpcError result = sendReplicationSubscribeForInstance(*i);
                if ((result != ERR_OK) && (result != ERR_DISCONNECTED))
                {
                    BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].replicationSubscribe: Failed to subscribe for instance " << *i << ", result = " << ErrorHelp::getErrorName(result));     
                    rc = result;
                    break;
                }
            }
        }
        else
        {
            if (mInstanceIds.count(selectedInstanceId) > 0)
            {
                if (mHasSentReplicationSubscribe.count(selectedInstanceId) == 0)
                {
                    rc = sendReplicationSubscribeForInstance(selectedInstanceId);
                }
            }
            else
            {
                //should never happen, we had to know about the instance to call this.
                BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].replicationSubscribe: Method was asked for specific id " << selectedInstanceId << " but no component with this instance id exists.");     
                rc = ERR_SYSTEM;
            }
        }
    }

    return rc;
}

BlazeRpcError Component::sendReplicationSubscribeForInstance(InstanceId id)
{
    BlazeRpcError result = ERR_COMMAND_NOT_FOUND;

    const CommandInfo* cmdInfo = mComponentInfo.getCommandInfo(Component::REPLICATION_SUBSCRIBE_CMD);
    if (cmdInfo != nullptr)
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(id);
        opts.ignoreReply = true; // Intentionally do not wait for reply, RemoteInstance::activateInternal() needs to issue replication sub for *all* components without blocking before the remote side starts sending down updates for any given component
        ReplicationSubscribeRequest req;
        ComponentStub* stub = gController->getComponentManager().getLocalComponentStub(mComponentInfo.baseInfo.id);
        if (stub != nullptr && !stub->getPartialReplicationTypes().empty())
        {
            req.getPartialReplicationTypes().reserve(stub->getPartialReplicationTypes().size());
            for (ComponentStub::ReplicatedMapTypes::const_iterator i = stub->getPartialReplicationTypes().begin(), e = stub->getPartialReplicationTypes().end(); i != e; ++i)
            {
                req.getPartialReplicationTypes().push_back(*i);
            }
        }
        result = sendRequest(*cmdInfo, &req, nullptr, nullptr, opts);
        if (result == ERR_OK)
        {
            mHasSentReplicationSubscribe.insert(id);
        }
    }

    return result;
}

BlazeRpcError Component::replicationUnsubscribe(InstanceId selectedInstanceId /*= INVALID_INSTANCE_ID*/)
{
    BlazeRpcError rc = ERR_OK;
    if (selectedInstanceId == INVALID_INSTANCE_ID)
    {
        mAutoRegInstancesForReplication = false;

        while (!mHasSentReplicationSubscribe.empty())
        {
            InstanceId nextId = mHasSentReplicationSubscribe.back();  //with a vector its most efficient to go from the back.
            mHasSentReplicationSubscribe.pop_back();
            BlazeRpcError result = sendReplicationUnsubscribeForInstance(nextId);

            if ((result != ERR_OK) && (result != ERR_DISCONNECTED))
            {
                BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].replicationUnsubscribe: Unsubscribe call to instance " << nextId  << " failed, result = " << ErrorHelp::getErrorName(result));     
            }
        }
    }
    else
    {
        InstanceIdSet::iterator itr = mHasSentReplicationSubscribe.find(selectedInstanceId);
        if (itr != mHasSentReplicationSubscribe.end())
        {
            mHasSentReplicationSubscribe.erase(itr);
            rc = sendReplicationUnsubscribeForInstance(selectedInstanceId);
            if ((rc != ERR_OK) && (rc != ERR_DISCONNECTED))
            {
                BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].replicationUnsubscribe: Unsubscribe call to instance " << selectedInstanceId  << " failed, result = " << ErrorHelp::getErrorName(rc));     
            }
        }
    }

    return rc;
}

BlazeRpcError Component::sendReplicationUnsubscribeForInstance(InstanceId id)
{
    BlazeRpcError result = ERR_COMMAND_NOT_FOUND;

    const CommandInfo* cmdInfo = mComponentInfo.getCommandInfo(Component::REPLICATION_UNSUBSCRIBE_CMD);
    if (cmdInfo != nullptr)
    {
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(id);
        result = sendRequest(*cmdInfo, nullptr, nullptr, nullptr, opts);
    }

    return result;
}

// check if the instance is valid.  If the instance is not int the set, it means the instance has shutdown and passed the restarting time period.
bool Component::checkInstanceIsValid(InstanceId instanceId)
{
    return instanceId != INVALID_INSTANCE_ID && mNonDrainingInstanceIds.find(instanceId) != mNonDrainingInstanceIds.end();
}

BlazeRpcError Component::waitForInstanceRestart(InstanceId instanceId)
{
    Fiber::EventHandle eventHandle = Fiber::getNextEventHandle();
    mRestartingInstanceMap[instanceId].waitingRpcList.push_back(eventHandle);
    
    int64_t timeout = gController->getFrameworkConfigTdf().getConnectionManagerConfig().getRemoteInstanceRestartTimeout().getMicroSeconds();
    if (timeout == 0)
    {
        // Assuming average blazeserver is utilizing at least 50% CPU, after the blazeserver is back after n seconds, 
        // it needs n/2 seconds to process the held RPCs in full speed (100% CPU) and back to normal usage. 
        // We want the sum of rpc holding and processing time less than RPC timeout (n + n/2 < RPC_TIMEOUT), so we set the restart timeout to be 2/3 of rpc timeout.
        if (gController->getInternalEndpoint() != nullptr)
            timeout = static_cast<int64_t>(gController->getInternalEndpoint()->getCommandTimeout().getMicroSeconds() * DEFAULT_RESTART_TIMEOUT_FRACTION);
    }

    TimeValue restartTimeout(timeout);
    BlazeRpcError waitErr = Fiber::wait(eventHandle, "sendRequest", TimeValue::getTimeOfDay() + restartTimeout);
    return waitErr;
}

BlazeRpcError Component::executeRemoteCommand(const RpcCallOptions &options, RpcProxySender* sender, const CommandInfo &cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf* responseTdf,
    RawBuffer* responseRaw, EA::TDF::Tdf* errorTdf, InstanceId& movedTo)
{
    BlazeRpcError rc = ERR_DISCONNECTED;
    if (EA_LIKELY(!options.ignoreReply))
    {
        rc = sender->sendRequest(getId(), cmdInfo.commandId, request, responseTdf, errorTdf, movedTo, getLogCategory(), options, responseRaw);
    }
    else
    {
        MsgNum msgNum;
        rc = sender->sendRequestNonblocking(getId(), cmdInfo.commandId, request, msgNum, getLogCategory(), options, responseRaw);
    } 
    return rc;
}

BlazeRpcError Component::sendRequest(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, const RpcCallOptions& options,
                                     Message* origMsg/* = nullptr*/, InstanceId* movedTo/* = nullptr*/, RawBuffer* responseRaw/* = nullptr*/, bool obfuscateResponse/* = false*/)
{
    if (cmdInfo.useShardKey && options.routeTo.isValid() && options.followMovedTo)
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "RpcCallOptions::routeTo and RpcCallOptions::followMovedTo are mutually exclusive when the command is sharded");
        return ERR_SYSTEM;
    }
    if (cmdInfo.useShardKey && (request == nullptr))
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "A command that is marked as sharded must be able to provide a shard key from a request TDF");
        return ERR_SYSTEM;
    }

    RoutingOptions resolvedRoute = options.routeTo;
    if (!resolvedRoute.isValid())
    {
        // If no route has been provided, try getting it from the request (sharded behavior)
        if (cmdInfo.useShardKey)
        {
            // The EA_ASSERT_MSG above makes sure that we would have a non-nullptr request ptr.
            ObjectId shardKey;
            if (getShardKeyFromRequest(cmdInfo.commandId, *request, shardKey))
            {
                if (isShardedBySlivers())
                    resolvedRoute.setSliverIdentityFromKey(getId(), shardKey);
                else
                    resolvedRoute.setInstanceIdFromKey(shardKey);
            }
            else
            {
                BLAZE_ASSERT_LOG(getLogCategory(), "Component.sendRequest: getShardKeyFromRequest() failed for commandId(" << cmdInfo.commandId << "), request tdf:" << *request);
                return ERR_SYSTEM;
            }
        }
        else
        {
            if (isLocal())
                resolvedRoute.setInstanceId(gController->getInstanceId());
            else
                resolvedRoute.setInstanceId(selectInstanceId());
        }
    }

    InstanceId instanceId = INVALID_INSTANCE_ID;
    BlazeRpcError rc;
    do
    {
        rc = ERR_OK;

        // Sliver Access needs to stay within this scope because the access needs to be held for the duration of the RPC call.
        Sliver::AccessRef sliverAccess;

        if (Sliver::isValidIdentity(resolvedRoute.getAsSliverIdentity()))
        {
            // This code path can be executed by a client, such as stress, which use component proxy apis.  In such a case,
            // there will not be a SliverManager.
            if (gSliverManager != nullptr)
            {
                rc = gSliverManager->waitForSliverAccess(resolvedRoute.getAsSliverIdentity(), sliverAccess, Fiber::getCurrentContext(),
                    (origMsg != nullptr && origMsg->getPeerInfo().isServer())); // internal server-to-server RPCs get priority access.
                if (rc == ERR_OK)
                    instanceId = sliverAccess.getSliverInstanceId();
            }
        }
        else
        {
            instanceId = resolvedRoute.getAsInstanceId();
        }

        if (rc == ERR_OK)
        {
            if (isLocal() && (instanceId == INVALID_INSTANCE_ID || instanceId == getLocalInstanceId()))
            {
                // The component's notification and replication subscribe RPCs are exempt from the hold because:
                // 1. They are required for slaves to be able to sync replicated state from their master. 
                // Since slaves publish themselves to the redirector before they are active, the master learns of them as it's coming up,
                // which would create a circular dependency if the master were to avoid processing subscribe RPCs for slaves it is trying to complete connection to.
                // 2. They are read-only RPCs and thus cannot change the state of the master, which makes it safe to call them before the master is 'ready for service' 
                // (e.g: connected to the entire cluster and thus able to propagate all state change events reliably)
                // We should consider removing this RPC hold functionality if ever we re-architect the Blaze framework to correctly treat
                // all notifications as 'stateless' or 'stateful' whereby notifications marked 'stateful' are always guaranteed to be written to persistent storage while
                // 'stateless' notifications sent to an instance or a client may be successfully dropped at any time without loss of functionality.
                if (cmdInfo.commandId != Component::NOTIFICATION_SUBSCRIBE_CMD && 
                    cmdInfo.commandId != Component::REPLICATION_SUBSCRIBE_CMD &&
                    cmdInfo.commandId != Component::REPLICATION_UNSUBSCRIBE_CMD &&
                    asStub()->getHoldIncomingRpcsUntilReadyForService())
                {
                    rc = gController->waitUntilReadyForService();
                }

                if (rc == ERR_OK)
                {
                    rc = asStub()->executeLocalCommand(cmdInfo, request, responseTdf, errorTdf, options, origMsg, obfuscateResponse);
                }
            }
            else if (!isLocal() || options.followMovedTo)
            {
                //Remote execution somewhere else
                RpcProxySender* sender = nullptr;

                if (EA_UNLIKELY(mExternalAddress.isValid()))
                {
                    sender = mResolver.getProxySender(mExternalAddress); 
                }
                else
                {
                    //Still no instance id?  Select one for us
                    if (instanceId == INVALID_INSTANCE_ID)
                    {
                        instanceId = selectInstanceId();
                    }

                    sender = mResolver.getProxySender(instanceId);
                }

                if (sender != nullptr)
                {
                    RpcCallOptions remoteCallOptions = options;
                    if (sliverAccess.hasSliver())
                        remoteCallOptions.routeTo.setSliverIdentity(sliverAccess.getSliverIdentity());
                    rc = executeRemoteCommand(remoteCallOptions, sender, cmdInfo, request, responseTdf, responseRaw, errorTdf, instanceId);
                }
                else if (!sliverAccess.hasSliver() && !options.ignoreReply && checkInstanceIsValid(instanceId)) // Currently, ignoreReply=true means don't block so we fast fail if instance is not available.
                {
                    rc = waitForInstanceRestart(instanceId);
                    if (rc == ERR_OK)
                    {
                        sender = mResolver.getProxySender(instanceId);
                        if (sender != nullptr)
                        {
                            rc = executeRemoteCommand(options, sender, cmdInfo, request, responseTdf, responseRaw, errorTdf, instanceId);
                        }
                        else
                        {
                            BLAZE_ERR_LOG(Log::CONTROLLER, "[Component].sendRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                                << ": " << cmdInfo.loggingName << " to instance (" << instanceId << "). The proxy sender is nullptr");
                            rc = ERR_SYSTEM; // this should never happen because waitForInstanceRestart() should only return ERR_OK when instance is back!
                        }
                    }
                    else
                    {
                        BLAZE_ERR_LOG(Log::CONTROLLER, "[Component].sendRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                            << ": " << cmdInfo.loggingName << " to instance (" << instanceId << "). Got error " << ErrorHelp::getErrorName(rc));
                    }
                }
                else
                {
                    rc = ERR_DISCONNECTED;
                }
            }
            else
            {
                rc = ERR_MOVED;
            }
        }
    }
    while (rc == ERR_MOVED && options.followMovedTo);

    if ((rc == ERR_MOVED) && (movedTo != nullptr))
        *movedTo = instanceId;

    return rc;
}

BlazeRpcError Component::sendRawRequest(const CommandInfo& cmdInfo, RawBufferPtr& requestBuf, RawBufferPtr& responseBuf, const RpcCallOptions& options, InstanceId* movedTo)
{
    EA_ASSERT_MSG(!cmdInfo.useShardKey || !options.routeTo.isValid() || !options.followMovedTo, "RpcCallOptions::routeTo and RpcCallOptions::followMovedTo are mutually exclusive when the command is sharded");
    EA_ASSERT_MSG(options.routeTo.getType() != RoutingOptions::SLIVER_IDENTITY, "Component::sendRawRequest does not currently support sliver sharded requests.");

    RpcProxySender* sender = nullptr;
    InstanceId instanceId = options.routeTo.getAsInstanceId();

    BlazeRpcError rc;
    do
    {
        rc = ERR_DISCONNECTED;
        if (EA_UNLIKELY((mExternalAddress.getIp() != 0) || (mExternalAddress.getHostname()[0] != '\0')))
        {
            sender = mResolver.getProxySender(mExternalAddress); 
        }
        else
        {
            if (instanceId == INVALID_INSTANCE_ID)
            {
                instanceId = selectInstanceId();
            }

            sender = mResolver.getProxySender(instanceId);
        }

        if (sender != nullptr)
        {
            rc = sender->sendRawRequest(getId(), cmdInfo.commandId, requestBuf, responseBuf, instanceId, getLogCategory(), options);
        }
        else if (!options.ignoreReply && checkInstanceIsValid(instanceId)) // Currently, ignoreReply=true means don't block so we fast fail if instance is not available.
        {
            rc = waitForInstanceRestart(instanceId);
            if (rc == ERR_OK)
            {
                sender = mResolver.getProxySender(instanceId);
                if (sender != nullptr)
                {
                    rc = sender->sendRawRequest(getId(), cmdInfo.commandId, requestBuf, responseBuf, instanceId, getLogCategory(), options);
                }
                else
                {
                    BLAZE_ERR_LOG(Log::CONTROLLER, "[Component].sendRawRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                        << ": " << cmdInfo.loggingName << " to instance (" << instanceId << "). The proxy sender is nullptr");
                    rc = ERR_SYSTEM; // this should never happen because waitForInstanceRestart() should only return ERR_OK when instance is back!
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::CONTROLLER, "[Component].sendRawRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                    << ": " << cmdInfo.loggingName << " to instance (" << instanceId << "). Got error " << ErrorHelp::getErrorName(rc));
            }
        }
    }
    while ((rc == ERR_MOVED) && options.followMovedTo);

    return rc;
}

BlazeRpcError Component::sendMultiRequest(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, MultiRequestResponseList& responses, const RpcCallOptions& options)
{
    if (responses.empty())
    {
        BLAZE_TRACE_LOG(getLogCategory(), "[Component].sendMultiRequest: no-op call, MultiRequestResponseList specified in request is empty for RPC: " << cmdInfo.componentInfo->loggingName);
        return ERR_OK;
    }

    BlazeRpcError result = ERR_OK;
    bool hadSuccess = false;
    // first loop and cover the sends
    for (MultiRequestResponseList::iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
    {
        if (*itr != nullptr)
        {
            MultiRequestResponse& response = **itr;
            InstanceId id = (*itr)->instanceId;
            if (id == INVALID_INSTANCE_ID)
            {
                BLAZE_TRACE_LOG(getLogCategory(), "[Component].sendMultiRequest: INVALID_INSTANCE_ID provided at index(" << eastl::distance(responses.begin(), itr) << ") in the list of instance ids for RPC: " << cmdInfo.componentInfo->loggingName);
                continue;
            }

            RpcProxySender* sender = mResolver.getProxySender(id);
            if (sender != nullptr)
            {
                response.error = sender->sendRequestNonblocking(getId(), cmdInfo.commandId, request, response.outboundMessageNum, getLogCategory(), options);
            }
            else if (!options.ignoreReply && checkInstanceIsValid(id)) // Currently, ignoreReply=true means send is optional if the instance is not available.
            {
                response.error = waitForInstanceRestart(id);
                if (response.error == ERR_OK)
                {
                    sender = mResolver.getProxySender(id);
                    if (sender != nullptr)
                    {
                        response.error = sender->sendRequestNonblocking(getId(), cmdInfo.commandId, request, response.outboundMessageNum, getLogCategory(), options);
                    }
                    else
                    {
                        BLAZE_ERR_LOG(getLogCategory(), "[Component].sendMultiRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                            << ": " << cmdInfo.loggingName << " to instance (" << id << "). The proxy sender is nullptr");
                        response.error = ERR_SYSTEM; // this should never happen because waitForInstanceRestart() should only return ERR_OK when instance is back!
                    }
                }
                else
                {
                    BLAZE_ERR_LOG(getLogCategory(), "[Component].sendMultiRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                        << ": " << cmdInfo.loggingName << " to instance (" << id << "). Got error " << ErrorHelp::getErrorName(response.error));
                }
            }
            else
            {
                response.error = ERR_SYSTEM;
                BLAZE_WARN_LOG(getLogCategory(), "[Component].sendMultiRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
                    << ": " << cmdInfo.loggingName << " to instance (" << id << "). Got error " << ErrorHelp::getErrorName(response.error));
            }

            // save off the first error if any - this is just a signal that there are _some_ errors to the caller,
            // they should still check everything afterwards
            if (result == ERR_OK && response.error != ERR_OK)
                result = response.error;
            if (response.error == ERR_OK)
                hadSuccess = true;
        }
    }

    if (!hadSuccess)
    {
        BLAZE_ERR_LOG(getLogCategory(), "[Component].sendMultiRequest: Failed to send held RPC: " << cmdInfo.componentInfo->loggingName
            << ": " << cmdInfo.loggingName << " - no send attempts succeeded.");
        return ERR_SYSTEM;
    }

    if (options.ignoreReply)
        return result;

    // loop again and collect all the results
    for (MultiRequestResponseList::iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
    {
        if (*itr != nullptr)
        {
            MultiRequestResponse& response = **itr;
            if (response.error == ERR_OK)
            {
                RpcProxySender* sender = mResolver.getProxySender((*itr)->instanceId);
                if (sender != nullptr)
                {
                    // TODO: sendMultiRequest() does not currently handle the ERR_MOVED redirection.
                    //       To handle properly, we would need to track which of the requests came back as ERR_MOVED, then
                    //       loop back and retry only those redirected requests.
                    //       At the time being, there is no need for sendMultiRequest() to handle this case, but that could change in the future.
                    InstanceId movedTo_ignored = INVALID_INSTANCE_ID;
                    response.error = sender->waitForRequest(response.outboundMessageNum, response.response, response.errorResponse, movedTo_ignored, getLogCategory(), options);
                }
                else
                {
                    BLAZE_ERR_LOG(getLogCategory(), "[Component].sendMultiRequest: Failed to wait for request for held RPC: " << cmdInfo.componentInfo->loggingName
                        << ": " << cmdInfo.loggingName << ". The proxy sender is nullptr for instance (" << (*itr)->instanceId << ")");
                    response.error = ERR_SYSTEM;
                }
            }

            // save off the first error if any - this is just a signal that there are _some_ errors to the caller,
            // they should still check everything afterwards
            if (result == ERR_OK && response.error != ERR_OK)
                result = response.error;
        }
    }

    return result;
}

void Component::processNotification(Notification& notification)
{
    if (!notification.isNotification())
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "Component.processNotification: Attempting to process an invalid notification.");
        return;
    }

    // Disallow blocking when processing notifications on instances that don't override blocking suppression
    Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

    if (!notification.isForComponentReplication())
    {
        const NotificationInfo& info = *ComponentData::getNotificationInfo(notification.getComponentId(), notification.getNotificationId());

        EA::TDF::TdfPtr payload;
        BlazeRpcError rc = notification.extractPayloadTdf(payload);

        // even if there is no payload we need to trigger the notification listener if there is one
        if (rc == ERR_OK)
        {
            BLAZE_TRACE_RPC_LOG(getLogCategory(), "<- notif recv "
                "[component=" << mComponentInfo.loggingName << " type=" << info.loggingName << " seqno=" << notification.getOpts().msgNum << "]\n" << payload.get());

            if (hasNotificationListener() && info.dispatchFunc != nullptr)
            {
                BLAZE_TRACE_RPC_LOG(getLogCategory(), "<- notif is being delivered "
                    "[component=" << mComponentInfo.loggingName << " type=" << info.loggingName << " seqno=" << notification.getOpts().msgNum << "]");

                // TODO_MC: passing the associatedUserSession seems to not be used anywhere.  I think we should get rid of this legacy parameter.
                //          Use the following Regular Expression to search the solution for all code implementing called listeners: void on.*\(const .*\&.*,.*UserSession.*\*\.*\)
                //          Note also, that, there is the dispatchNotificationNoPayload dispatch func type that won't be matched by the regEx.  But still no users.
                UserSessionPtr associatedUserSession = gUserSessionManager->getSession(notification.getUserSessionId());
                (this->*info.dispatchFunc)(payload.get(), associatedUserSession.get());
            }
        }
    }
    else if (!hasComponentReplication() || !gReplicator->handleReplicationNotification(notification))
    {
        BLAZE_WARN_LOG(getLogCategory(), "[" << mComponentInfo.loggingName << "].processQueuedNotifications: Ignoring unhandled notification type (" << notification.getNotificationId() << ").");
    }
}

BlazeRpcError Component::startTransaction(TransactionContextHandlePtr &ptr, uint64_t customData, TimeValue timeout)
{
    VERIFY_WORKER_FIBER();

    if (!isLocal())
    {
        //We are not a local component, so this needs to go remote
        return forwardStartTransaction(customData, timeout, ptr);
    }

    TransactionContextPtr transactionContextPtr;
    TransactionContext *context = nullptr;
    BlazeRpcError result = createTransactionContext(customData, context);
    if (result == Blaze::ERR_OK)
    {
        if (context != nullptr)
        {
            // setup context
            TransactionContextId id = BuildInstanceKey64(gController->getInstanceId(), ++mLastContextId);

            if (EA_UNLIKELY(mLastContextId == INSTANCE_KEY_MAX_KEY_BASE_64))
            {
                BLAZE_INFO_LOG(Log::CONTROLLER, "[Component].startTransaction: transaction id rollover at this point for "
                    << getName() << " component.");
                mLastContextId = 0;
            }

            // in case when timeout is not specified
            // check if fiber's job has time out and if so 
            // use remaining job's time as timeout 
            if (timeout == 0 && Fiber::getCurrentTimeout() > 0)
                context->setTimeout(Fiber::getCurrentTimeout() - TimeValue::getTimeOfDay());
            else
                context->setTimeout(timeout);

            context->setTransactionContextId(id);
            transactionContextPtr.attach(context);
            mTransactionIdToContextMap[id] = transactionContextPtr;
            // setup handle ptr
            ptr.attach(BLAZE_NEW_MGID(MEM_GROUP_FRAMEWORK_DEFAULT, "Blaze::TransactionContextHandle") TransactionContextHandle(*this, id));
            ptr->assign();
        }
        else
        {
            // something is wrong in implementation
            result = Blaze::ERR_SYSTEM;
        }
    }

    return result;
}

BlazeRpcError Component::commitTransaction(TransactionContextId id)
{
    if (!isLocal())
    {
        //We are not a local component, so this needs to go remote
        return forwardCompleteTransaction(id, true);
    }
    else
    {
        return completeLocalTransaction(id, true);
    }
}

BlazeRpcError Component::rollbackTransaction(TransactionContextId id)
{
    if (!isLocal())
    {
        //We are not a local component, so this needs to go remote
        return forwardCompleteTransaction(id, false);
    }
    else
    {
        return completeLocalTransaction(id, false);
    }
}

BlazeRpcError Component::completeLocalTransaction(TransactionContextId id, bool commit)
{
    TransactionIdToContextMap::iterator it = mTransactionIdToContextMap.find(id);

    if (it == mTransactionIdToContextMap.end())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].completeLocalTransaction: failed to find transaction "
            << getName() << "/" << id);
        return ERR_SYSTEM;
    }

    if (it->second->getRefCount() > 1)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].completeLocalTransaction: attempt to get in-use transaction "
            << getName() << "/" << id);
        return ERR_SYSTEM;
    }

    TransactionContextPtr ptr = it->second;
    mTransactionIdToContextMap.erase(it);

    BlazeRpcError result = Blaze::ERR_SYSTEM;
    if (commit)
        result = ptr->commitInternal();
    else
        result = ptr->rollbackInternal();

    return result;
}

BlazeRpcError Component::forwardStartTransaction(uint64_t customData, TimeValue timeout, TransactionContextHandlePtr &transactionPtr)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    RpcCallOptions opts;
    opts.routeTo.setInstanceId(selectInstanceId()); //select an instance id for this component, and use it for the remote controller

    // forward this call to the remote controller
    StartRemoteTransactionRequest request;
    request.setComponentId(getId());
    request.setCustomData(customData);
    request.setTimeout(timeout);

    StartRemoteTransactionResponse response;
    result = gController->startRemoteTransaction(request, response, opts);
    if (result == ERR_OK)
    {
        transactionPtr.attach(
            BLAZE_NEW_MGID(MEM_GROUP_FRAMEWORK_DEFAULT, "Blaze::TransactionContextHandle") TransactionContextHandle(*this, response.getTransactionId()));
    }


    return result;
}

BlazeRpcError Component::forwardCompleteTransaction(TransactionContextId id, bool commit)
{
    // forward this call to the remote controller
    RpcCallOptions opts;
    opts.routeTo.setInstanceIdFromKey(id);

    CompleteRemoteTransactionRequest request;
    request.setTransactionId(id);
    request.setComponentId(getId());
    request.setCommitTransaction(commit);
    return gController->completeRemoteTransaction(request, opts);
}

TransactionContextPtr Component::getTransactionContext(TransactionContextId id)
{
    TransactionContextPtr result;
    TransactionIdToContextMap::iterator it = mTransactionIdToContextMap.find(id);

    if (it != mTransactionIdToContextMap.end())
    {
        if (it->second->getRefCount() > 1)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].getTransactionContext: attempt to get in-use transaction "
                << getName() << "/" << id);
        }
        else
        {
            result = it->second;
        }
    }

    return result;
}

TransactionContextHandlePtr Component::getTransactionContextHandle(TransactionContextId id)
{
    TransactionContextHandlePtr result;
    TransactionIdToContextMap::iterator it = mTransactionIdToContextMap.find(id);

    if (it != mTransactionIdToContextMap.end())
    {
        if (it->second->getRefCount() > 1)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].getTransactionContextHandle: attempt to get in-use transaction "
                << getName() << "/" << id);
        }
        else
        {
            result.attach(BLAZE_NEW_MGID(MEM_GROUP_FRAMEWORK_DEFAULT, "Blaze::TransactionContextHandle") TransactionContextHandle(*this, id));
        }
    }

    return result;
}

BlazeRpcError Component::expireTransactionContexts(TimeValue defaultTimeout)
{
    TimeValue now = TimeValue::getTimeOfDay();
    typedef eastl::vector<TransactionContextId> TransactionContextIdVector;
    TransactionContextIdVector idsToExpire;
    // first pass, collect ids to expire (destroying context is potentially blocking operation)
    for (TransactionIdToContextMap::const_iterator it = mTransactionIdToContextMap.begin(),
        itEnd = mTransactionIdToContextMap.end();
        it != itEnd;
    ++it)
    {
        TransactionContextPtr ptr = it->second;
        TimeValue timeout = ptr->getTimeout() != 0 ? ptr->getTimeout() : defaultTimeout;
        TimeValue delta = now - ptr->getCreationTimestamp();
        if (delta >= timeout) 
        {
            idsToExpire.push_back(it->second->getTransactionContextId());
        }
    }

    // expire transaction by removing it from map
    for (TransactionContextIdVector::const_iterator itId = idsToExpire.begin(),
        itIdEnd = idsToExpire.end();
        itId != itIdEnd;
    ++itId)
    {
        TransactionIdToContextMap::iterator trIt =  mTransactionIdToContextMap.find(*itId);
        if (trIt != mTransactionIdToContextMap.end())
        {
            mTransactionIdToContextMap.erase(trIt);
        }
    }

    // return code is required as this function is DEFINE_ASYNC
    // because it can block if context object is destroyed as result of
    // reference counter falling to zero when the object is removed from the map
    return ERR_OK;

}

} //namespace
