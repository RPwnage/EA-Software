/*************************************************************************************************/
/*!
    \file 
  

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/blazerpc.h"
#include "framework/component/censusdata.h"
#include "blazerpcerrors.h"
#include "EASTL/set.h"
#include "EASTL/string.h"
#include "framework/component/componentmanager.h"
#include "framework/controller/controller.h"
#include "framework/controller/instanceinfocontroller.h"
#include "framework/controller/processcontroller.h"
#include "framework/controller/mastercontroller.h"
#include "framework/controller/metricsexporter.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/config/config_file.h"
#include "framework/config/configmerger.h"
#include "framework/config/configdecoder.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/session.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/database/dbscheduler.h"
#include "framework/metrics/metrics.h"
#include "framework/metrics/outboundmetricsmanager.h"
#include "framework/replication/replicator.h"
#include "framework/system/fibermanager.h"
#include "framework/system/fiber.h"
#include "framework/util/profanityfilter.h"
#include "framework/util/localization.h"
#include "framework/event/eventmanager.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/eventxmlprotocol.h"
#include "framework/protocol/restprotocol.h"
#include "framework/system/debugsys.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/storage/storagemanager.h"
#include "framework/vault/vaultmanager.h"
#include "framework/controller/drainlistener.h"
#include "framework/controller/remoteinstancelistener.h"
#include "framework/redis/redismanager.h"
#include "EAIO/EAFileUtil.h"

#include <regex>

#ifdef EA_PLATFORM_LINUX
#include <pwd.h>
#endif


namespace Blaze
{

static const char8_t sComponentChangeNamedContext[] = "ControllerComponentChangeOrReconfigure";

BlazeControllerSlave* BlazeControllerSlave::createImpl()
{
    EA_FAIL_MESSAGE("Controller createImpl should never be called.");
    return nullptr;
}

/*! ***************************************************************************/
/*!    \brief Constructor
*******************************************************************************/
Controller::Controller(Selector& selector, const char8_t* baseName, const char8_t* instanceName, uint16_t basePort)
    : mComponentManager(*(BLAZE_NEW ComponentManager)),
      mBootConfigTdf(gProcessController->getBootConfigTdf()),
      mPendingFrameworkConfigTdf(nullptr),
      mInstanceInfoController(*(BLAZE_NEW InstanceInfoController(selector))),
      mConnectionManager(*(BLAZE_NEW ConnectionManager(selector))),
      mOutboundConnectionManager((OutboundConnectionManager&) mResolver),
      mBasePort(basePort),
      mInstanceId(INVALID_INSTANCE_ID),
      mInstanceSessionId(SlaveSession::INVALID_SESSION_ID),
      mInstanceTypeIndex(0),
      mSlaveTypeCount(1), // There is always at least one type of slave (core)
      mTaskGroupId(Fiber::INVALID_FIBER_GROUP_ID),
      mChangeGroupId(Fiber::INVALID_FIBER_GROUP_ID),
      mBootstrapComplete(false),
      mIsDraining(false),
      mShutdownPending(false),
      mShuttingDown(false),
      mInService(false),
      mIsReadyForService(false),
      mIsStartInService(gProcessController->getStartInService()),
      mInServiceCheckCounter(0),
      mSelector(selector),
      mStartupTime(TimeValue::getTimeOfDay()),
      mConfigReloadTime(mStartupTime),
      mDrainStartTime(0),
      mUptimeMetric(*Metrics::gFrameworkCollection, "uptime", [this]() { return (TimeValue::getTimeOfDay() - mStartupTime).getMillis(); }),
      mConfigUptimeMetric(*Metrics::gFrameworkCollection, "config_uptime", [this]() { return (TimeValue::getTimeOfDay()-mConfigReloadTime).getMillis(); }),
      mInServiceMetric(*Metrics::gFrameworkCollection, "inservice", [this]() { return (uint64_t)isInService(); }),
      mIsDrainingMetric(*Metrics::gFrameworkCollection, "draining", [this]() { return (uint64_t)isDraining(); }),
      mDeprecatedRPCs(*Metrics::gFrameworkCollection, "deprecatedRPCs", Metrics::Tag::component, Metrics::Tag::command, Metrics::Tag::client_type, Metrics::Tag::endpoint, Metrics::Tag::ip_addr),
      mProcessResidentMemory(*Metrics::gFrameworkCollection, "memory.processResidentMemory", []() { uint64_t size, resident; Allocator::getProcessMemUsage(size, resident); return resident; }),
      mExecutingStateTransition(true), //Initialized to true because we start out in "activation"
      mAuditEntryExpiryTimerId(INVALID_TIMER_ID),
      mMetricsTimerId(INVALID_TIMER_ID),
      mOIMetricsTimerId(INVALID_TIMER_ID),
      mNextMetricsLogTime(0),
      mNextMetricsExportTime(0),
      mCertRefreshTimerId(INVALID_TIMER_ID),
      mDrainStatusCheckHandlers(BlazeStlAllocator("Controller::mDrainStatusCheckHandlers", COMPONENT_MEMORY_GROUP)),
      mDrainTimerId(INVALID_TIMER_ID),
      mDrainComplete(false),
      mLastBroadcastLoad(0),
      mLastBroadcastSystemLoad(0),
      mInboundGracefulClosureAckTimeout(0),
      mUpdateAuditStateQueue("Controller::fetchAndUpdateAuditState"),
      mExternalServiceMetrics(getMetricsCollection())
{
    mComponentManager.initialize(*this);

    blaze_strnzcpy(mBaseName, baseName, sizeof(mBaseName));
    blaze_strnzcpy(mInstanceName, instanceName, sizeof(mInstanceName));

    // For non configMaster instances we will need to do this again after subscribing to the configMaster
    // This is done to ensure that all server configuration comes from a single source.
    BootConfig::ServerConfigsMap::const_iterator i = mBootConfigTdf.getServerConfigs().find(mBaseName);
    if (i != mBootConfigTdf.getServerConfigs().end())
    {
        i->second->copyInto(mServerConfigTdf);
    }

    mDebugSys = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "DebugSystem") DebugSystem();

    // Zero this buffer out because there is currently a bug in EAStdC::Strlen() (called indirectly
    // from GetCurrentWorkingDirectory) which may trigger a memory checking tool warning on opt builds.
    memset(mCwd, 0, sizeof(mCwd));
    EA::IO::Directory::GetCurrentWorkingDirectory(mCwd, sizeof(mCwd));

#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_LINUX)
    // Linux blazeservers may be running in a container.
    // If that's the case, we'll want to fetch the current working directory on the host running the container
    // (which the deploy scripts will set in the BLAZE_CWD environment variable)
    const char8_t* blazeCwd = getenv("BLAZE_CWD");
    if (blazeCwd != nullptr)
        mHostCwd.assign(blazeCwd);
#endif

    mProcessOwner[0] = '\0';
#ifdef WIN32
    DWORD ownerLen = sizeof(mProcessOwner);
    GetUserName(mProcessOwner, &ownerLen);
#else
    // Seems like a complicated way to get the user name but cuserid() appears to be on the way
    // out and getlogin() returns nullptr.
    struct passwd pwEntry;
    struct passwd* pwEntryResult = nullptr;
    size_t pwBufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char8_t pwBuf[pwBufSize];
    getpwuid_r(getuid(), &pwEntry, pwBuf, pwBufSize, &pwEntryResult);
    if (pwEntryResult != nullptr)
        blaze_strnzcpy(mProcessOwner, pwEntryResult->pw_name, sizeof(mProcessOwner));
#endif

    mInboundGrpcManager.reset(BLAZE_NEW Grpc::InboundGrpcManager(mSelector));
    mOutboundGrpcManager.reset(BLAZE_NEW Grpc::OutboundGrpcManager(mSelector, mInstanceName)); // pass in mInstanceName because gController does not exist yet (we are in it's constructor)
}

/*! ***************************************************************************/
/*!    \brief Destructor
*******************************************************************************/
Controller::~Controller()
{
    SAFE_DELETE_REF(mComponentManager);
    SAFE_DELETE_REF(mInstanceInfoController);
    SAFE_DELETE_REF(mConnectionManager);
    SAFE_DELETE_REF(mOutboundConnectionManager);

    //Its possible there's some state transition jobs in our queue still 
    while (!mStateTransitionJobQueue.empty())
    {
        Job *orphan = mStateTransitionJobQueue.front();
        mStateTransitionJobQueue.pop_front();
        delete orphan;
    }

    if (mDebugSys)
    {
        delete mDebugSys;
        mDebugSys = nullptr;
    }
}

bool Controller::reserveEndpointPorts(uint16_t& basePort)
{
    // If basePort is not zero, the configuration files are supposed to make sure that enough ports starting at base port are available for assignment. Nothing happens in this function.
    // If basePort is zero, the instance needs to reserver ports using Port Assignment Service (PAS) which is what we do below. Note that an instance can call PAS only once so we need to
    // take into account ports for both ConnectionManager and InboundGrpcManager. 

    basePort = mBasePort;

    // the PAS util only needs to know about one of the service names, since uniques of all of them will be
    // checked by the redirector.  We can arbitrarily choose the first one here.
    if (!mPasUtil.initialize(getDefaultServiceName(), getInstanceName(), mBootConfigTdf.getPortAssignmentServiceAddr()) && mBasePort == 0)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].reserveEndpointPorts: No basePort and no valid PAS configuration specified for instance.");
        return false;
    }

    if (mBasePort == 0)
    {
        EndpointConfig::EndpointGroupsMap::const_iterator group = mFrameworkConfigTdf.getEndpoints().getEndpointGroups().find(mServerConfigTdf.getEndpoints());
        const auto& connMgrEndpointNames = group->second->getEndpoints();
        const auto& grpcEndpointNames = group->second->getGrpcEndpoints();

        return gController->getPasUtil().getBasePort(connMgrEndpointNames.size() + grpcEndpointNames.size(), basePort);
    }

    return true;
}

/*! ***************************************************************************/
/*! \brief  This method can only be called after the endpoint information has
            been received from the configMaster. Obviously configMaster itself
            does not need to wait for anything.
*******************************************************************************/
bool Controller::initializeConnectionManager(uint16_t basePort)
{
    if (!mConnectionManager.initialize(basePort, mFrameworkConfigTdf.getRates(), mFrameworkConfigTdf.getEndpoints(), 
        mServerConfigTdf.getEndpoints()))
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].initializeConnectionManager: Unable to initialize connection manager.");
        return false;
    }

    // Determine our internal endpoint that other instances will use to connect to us
    const Endpoint* ep = getInternalEndpoint();
    if ((mConnectionManager.getEndpointCount() != 0) && (ep == nullptr))
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].initializeConnectionManager: internal endpoint '" << mServerConfigTdf.getInternalEndpoint() << "' not configured.");
        return false;
    }

    if ((ep != nullptr) && (ep->getBindType() != BIND_INTERNAL))
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].initializeConnectionManager: internal endpoint '" << mServerConfigTdf.getInternalEndpoint() << "' must be set to BIND_INTERNAL.");
        return false;
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].initializeConnectionManager: using '" << mServerConfigTdf.getInternalEndpoint() << "' as the internal endpoint.");

    return true;
}

bool Controller::initializeInboundGrpcManager(uint16_t basePort)
{
    if (mInboundGrpcManager->initialize(basePort, mServerConfigTdf.getEndpoints(), mFrameworkConfigTdf.getEndpoints(), mFrameworkConfigTdf.getRates()))
    {
        return mInboundGrpcManager->registerServices(mComponentManager); 
    }
    return false;
}

bool Controller::initializeOutboundGrpcManager()
{
    return mOutboundGrpcManager->initialize(mFrameworkConfigTdf.getOutboundGrpcServiceConfig().getOutboundGrpcServices());
}

/*! ***************************************************************************/
/*! \brief  getComponent 
    
    Gets the Component that matches the required parameters.
     
    \param  componentId - Id of the component to fetch (can be master or slave)
    \param  reqLocal - Component must be locally registered (castable to ComponentTypeImpl*)
    \param  reqActive - Component must be in state == ComponentStub::ACTIVE (remote components are always active)
    \param  waitResult - If specified, call may block the current fiber and wait for the required component
    \return Component* - Component matching the required parameters, or nullptr if not available
*******************************************************************************/
Component* Controller::getComponent(ComponentId componentId, bool reqLocal, bool reqActive, BlazeRpcError* waitResult)
{
    // if the caller specified that we can block, check to make sure we are on a worker fiber
    EA_ASSERT(waitResult == nullptr || Fiber::isCurrentlyBlockable());
    
    EA::TDF::TimeValue beginTime = TimeValue::getTimeOfDay();
    uint32_t logCounter = 0;
    do
    {
        Component* component = mComponentManager.getComponent(componentId, reqLocal);
        
        if (component != nullptr)
        {
            if (component->isLocal() && component->asStub()->getState() == ComponentStub::ERROR_STATE)
            {
                // If the component managed to get into an error state, bail out.
                if (waitResult)
                    *waitResult = ERR_SYSTEM;
                break;
            }
            
            // We verify that a remote component is active by checking the instance count
            // as we won't be aware of any instances of the component unless it has gone active.
            if ((!component->isLocal() && (component->getComponentInstanceCount() > 0)) 
                || !reqActive 
                || ((component->asStub() != nullptr) && (component->asStub()->getState() == ComponentStub::ACTIVE)))
            {
                if (waitResult)
                    *waitResult = ERR_OK;
                return component;
            }
        }
        else if (reqLocal)
        {
            //We bail immediately if a local component is requested and does not exist.  Components are created
            //at once, so if the dependency hasn't been resolved by the time another component is asking for it,
            //its never going to happen.
            if (waitResult)
                *waitResult = ERR_COMPONENT_NOT_FOUND;
            return nullptr;
        }
        
        if (waitResult == nullptr)
        {
            // If we cannot block, bail out
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].getComponent: Required " << (reqLocal ? "local " : "") << (reqActive ? "active " : "") 
                            << "component " << (BlazeRpcComponentDb::getComponentNameById(componentId)) << "(" << SbFormats::Raw("0x%04x", componentId) << ") not available.");
            break;
        }
        
        if (isShuttingDown())
        {
            // If the controller is shutting down, bail out.
            *waitResult = ERR_CANCELED;
            break;
        }
        
        const uint32_t LOG_FREQUENCY = 15; // 15s
        EA::TDF::TimeValue loopWaitTime = TimeValue::getTimeOfDay();
        if (loopWaitTime - beginTime > TimeValue(LOG_FREQUENCY * 1000 * 1000)) //if more than LOG_FREQUENCY seconds have passed and we are still waiting, log a warning.
        {
            beginTime = TimeValue::getTimeOfDay();
            ++logCounter; 
            
            eastl::string callstack;
            CallStack::getCurrentStackSymbols(callstack, 3, 2);//3 frames to ignore, 2 frames to keep
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getComponent: Waited " << (LOG_FREQUENCY * logCounter) << "s for " << (reqLocal ? "local " : "") << (reqActive ? "active " : "")
                << (BlazeRpcComponentDb::getComponentNameById(componentId)) << "(" << SbFormats::Raw("0x%04x", componentId) << ") component. Caller:\n" << SbFormats::PrefixAppender("  ", callstack.c_str()));
        }
    }
    while ((*waitResult = mSelector.sleep(1*1000*1000)) == ERR_OK); //sleep 1s
    
    return nullptr;
}

/*! ***************************************************************************/
/*!    \brief Starts the controller bootstrap process.

    This is the main function called into by the thread.  It creates all the local
    components of the server, sets up the metrics code, and schedules the fiber
    bootstrap function to run.

    \return True if the controller initialized properly, false otherwise.
*******************************************************************************/
bool Controller::startController()
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].startController: starting controller with "<<Allocator::getTypeName()<<" allocator.");

    //Bootstrap this component - we don't go through the normal component activation because we need to 
    //do a few extra things along the way.

    //First assign the group ids here before we make any fibers.
    // IMPORTANT: controller member fiber group ids cannot be assigned in the constructor because the Controller's constructor 
    // is called before the ServerThread::run() thus causing the WRONG thread locals to be used for allocating the group ids!
    mTaskGroupId = Fiber::allocateFiberGroupId();

    Fiber::CreateParams params;
    params.blocking = true;
    params.stackSize = Fiber::STACK_HUGE;
    params.namedContext = "Controller::controllerBootstrap";
    params.groupId = mTaskGroupId;
    gFiberManager->scheduleCall(this, &Controller::controllerBootstrap, params);

    return true;
}

bool Controller::isOurInternalEndpointAddress(const InetAddress& addr)
{
    const Endpoint* ep = getInternalEndpoint();
    if (ep != nullptr)
    {
        // if we already know our internal endpoint we can use it
        // to check if the address is for ourselves
        InetAddress internalAddr(NameResolver::getInternalAddress().getIp(InetAddress::NET), 
            ep->getPort(InetAddress::NET), InetAddress::NET);

        if (internalAddr == addr)
            return true;
    }

    return false;
}

void Controller::connectAndActivateRemoteInstances(const eastl::vector<InetAddress>& remoteAddrList)
{
    for (const InetAddress& remoteAddr : remoteAddrList)
    {
        connectAndActivateInstance(remoteAddr);
    }
}

void Controller::connectAndActivateInstance(const InetAddress& addr)
{
    if (isShuttingDown())
        return;
        
    // if we already know our internal endpoint we can use it
    // to check if we are trying to connect to ourselves.
    if (isOurInternalEndpointAddress(addr))
        // if we are trying to connect to ourselves, early out
        return;
    
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstance: Connecting to new remote server instance @ " << addr);
    
    Fiber::CreateParams params;
    params.blocking = true;
    params.stackSize = Fiber::STACK_LARGE;
    params.namedContext = "Controller::connectAndActivateInstanceInternal";
    params.groupId = mTaskGroupId;
    gFiberManager->scheduleCall(this, &Controller::connectAndActivateInstanceInternal, addr, params);
}

void Controller::connectAndActivateInstanceInternal(InetAddress addr)
{
    BlazeRpcError rc = ERR_SYSTEM;
    char8_t buf[256];
    if (!addr.isResolved())
    {
        NameResolver::LookupJobId jobId;
        rc = gNameResolver->resolve(addr, jobId);
        if (rc != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
                "Failed to resolve remote instance address '"
                << addr.toString(buf, sizeof(buf))
                << "', rc: " << ErrorHelp::getErrorName(rc)
                << " Resyncing cluster info.");

            mInstanceInfoController.onConnectAndActivateInstanceFailure();
            return;
        }
    }

    RemoteInstanceByAddrMap::const_iterator riit = mRemoteInstancesByAddr.find(addr);
    if (riit != mRemoteInstancesByAddr.end())
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
            "Remote instance @ " 
            << addr.toString(buf, sizeof(buf)) 
            << " already exists");
        return;
    }

    RemoteServerInstance& remoteInstance = *(BLAZE_NEW RemoteServerInstance(addr));
    // write partial instance info into buffer, since
    // that's all we have prior to call to initialize()
    remoteInstance.toString(buf, sizeof(buf));
    mRemoteInstancesByAddr[addr] = &remoteInstance;
    rc = remoteInstance.connect(); // This actually retries internally indefinitely (until we start shutting down ourselves)
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
            "Failed to connect to remote instance " << buf 
            << ", rc: " << ErrorHelp::getErrorName(rc)
            << " Resyncing cluster info.");

        shutdownAndRemoveInstance(addr, true);
        mInstanceInfoController.onConnectAndActivateInstanceFailure();

        return;
    }
    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
        "Connected to remote instance " << buf);
    // remoteInstance.initialize calls the getInstanceInfo rpc on remote instance. The rpc will only succeed once the remote instance has gone ACTIVE else it will block indefinitely (unless
    // the socket connection is broken)
    rc = remoteInstance.initialize(); 
    if (rc != ERR_OK)
    {
        if (rc == CTRL_ERROR_SERVER_INSTANCE_ALREADY_HOSTS_COMPONENT)
        {
            if (!mBootstrapComplete)
            {
                // NOTE: Upon detecting a local conflict we can only shut down safely if we are still booting.
                // Once bootstrap is complete, the onus is on the remote instance to detect the conflict and shut itself down.
                BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
                    "Component conflict with instance " << remoteInstance.toString(buf, sizeof(buf))
                    << " detected during boot, shutting down because bootstrap was not done...");
                gProcessController->shutdown(ProcessController::EXIT_MASTER_TERM);
            }
            else if (remoteInstance.getStartupTime() != 0 && remoteInstance.getStartupTime() < mStartupTime)
            {
                BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
                    "Component conflict with instance " << remoteInstance.toString(buf, sizeof(buf))
                    << " detected during boot, shutting down because we're the younger instance...");
                gProcessController->shutdown(ProcessController::EXIT_MASTER_TERM);
            }
        }
        if (rc != CTRL_ERROR_SERVER_INSTANCE_ALREADY_REGISTERED)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
                "Failed to initialize remote instance " << buf 
                << ", rc: " << ErrorHelp::getErrorName(rc)
                << " Resyncing cluster info.");
        }
        shutdownAndRemoveInstance(addr, true);
        if (rc != CTRL_ERROR_SERVER_INSTANCE_ALREADY_REGISTERED)
        {
            mInstanceInfoController.onConnectAndActivateInstanceFailure();
        }

        return;
    }
    // write full instance info into the buffer, since we
    // have it only after successful call to initialize()
    remoteInstance.toString(buf, sizeof(buf));
    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
        "Initialized proxy components for remote instance " << buf);
    RemoteInstanceByIdMap::const_iterator rit = mRemoteInstancesById.find(remoteInstance.getInstanceId());
    if (rit != mRemoteInstancesById.end())
    {
        char8_t buf2[256];
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
            "New remote instance " << buf << " conflicts with existing remote instance " 
            << rit->toString(buf2, sizeof(buf2))
            << " Resyncing cluster info.");
        shutdownAndRemoveInstance(addr, true);

        mInstanceInfoController.onConnectAndActivateInstanceFailure();
        return;
    }
    mRemoteInstancesById.insert(remoteInstance);
    rc = remoteInstance.activate(); // Create remote components and notify waiters of the state change.
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
            "Failed to activate remote instance " << buf << ", rc: " << ErrorHelp::getErrorName(rc)
            << " Resyncing cluster info.");
        shutdownAndRemoveInstance(addr, true);

        mInstanceInfoController.onConnectAndActivateInstanceFailure();

        return;
    }
    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].connectAndActivateInstanceInternal: "
        "Activated proxy components for remote instance " << buf);
    remoteInstance.setListener(*this);

    Fiber::CreateParams params;
    params.blocking = true;
    params.stackSize = Fiber::STACK_LARGE;
    params.namedContext = "Controller::onRemoteInstanceStateChanged";
    params.groupId = remoteInstance.getFiberGroupId();
    // NOTE: Schedule this on a separate fiber tagged with the fiber group id of the remote server instance because RemoteServerInstance::shutdown() needs to abort the state changes.
    gFiberManager->scheduleCall<Controller, RemoteServerInstance&>(this, &Controller::onRemoteInstanceStateChanged, remoteInstance, params);
}

void Controller::shutdownAndRemoveInstance(InetAddress addr, bool removeConnected)
{
    RemoteInstanceByAddrMap::iterator it = mRemoteInstancesByAddr.find(addr);
    if (it != mRemoteInstancesByAddr.end())
    {
        RemoteServerInstance* instance = it->second;
        if (removeConnected || !instance->isConnected())
        {
            if (instance->shutdown())
            {
                mRemoteInstancesByAddr.erase(addr);
                mRemoteInstancesById.erase(instance->getInstanceId());
                delete instance;
            }
        }
    }
}

uint16_t Controller::getConfiguredInstanceTypeCount(const char8_t* typeName) const
{
    uint16_t count = 0;

    UInt16ByString::const_iterator it = mConfiguredCountByInstanceType.find(typeName);
    if (it != mConfiguredCountByInstanceType.end())
        count = it->second;

    return count;
}

const ClientPlatformSet& Controller::getHostedPlatforms() const
{
    return mHostedPlatforms;
}

const ClientPlatformSet& Controller::getUsedPlatforms() const
{
    if (!isSharedCluster())
        return mHostedPlatforms;

    static ClientPlatformSet sharedClusterPlatformSet;
    sharedClusterPlatformSet.insert(mHostedPlatforms.begin(), mHostedPlatforms.end());
    sharedClusterPlatformSet.insert(common);
    return sharedClusterPlatformSet;
}

bool Controller::isPlatformHosted(ClientPlatformType platform) const
{
    return mHostedPlatforms.find(platform) != mHostedPlatforms.end();
}

void Controller::validateHostedServicesConfig(ConfigureValidationErrors& validationErrors) const 
{
    validateServiceNameStandard(validationErrors);

    // validate our platform configuration
    ClientPlatformSet hostedPlatformSet;
    {
        for (auto platform : mBootConfigTdf.getHostedPlatforms())
        {
            // Make sure there are no incorrect platforms in the list
            if (platform == INVALID || platform == NATIVE || platform == common)
            {
                FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: invalid platform (%s) specified in the hosted platform list.", ClientPlatformTypeToString(platform));
                validationErrors.getErrorMessages().push_back().set(msg.c_str());
            }

            // Make sure there are no duplicate platforms in the list
            if (hostedPlatformSet.insert(platform).second == false)
            {
                FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: duplicate platform (%s) specified in the hosted platform list.", ClientPlatformTypeToString(platform));
                validationErrors.getErrorMessages().push_back().set(msg.c_str());
            }

            // Make sure that each hosted platform has a default service configured
            if (mBootConfigTdf.getPlatformToDefaultServiceNameMap().find(platform) == mBootConfigTdf.getPlatformToDefaultServiceNameMap().end())
            {
                FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: platform (%s) does not have a default service.", ClientPlatformTypeToString(platform));
                validationErrors.getErrorMessages().push_back().set(msg.c_str());
            }

        }

        // Make sure we have at least 1 platform that we are hosting
        if (hostedPlatformSet.size() == 0)
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: no platform configured for hosting (no valid platforms in the hostedPlatforms list).");
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
        else if (hostedPlatformSet.size() == 1)
        {
            if (mBootConfigTdf.getConfiguredPlatform() != common && mBootConfigTdf.getConfiguredPlatform() != *hostedPlatformSet.begin())
            {
                FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: Invalid 'configuredPlatform' setting (%s). Configured platform must either be 'common' (for shared cluster deployments), or the platform in the hostedPlatforms list (for single-platform deployments).", ClientPlatformTypeToString(mBootConfigTdf.getConfiguredPlatform()));
                validationErrors.getErrorMessages().push_back().set(msg.c_str());
            }
        }
        else if (mBootConfigTdf.getConfiguredPlatform() != common)
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: Invalid 'configuredPlatform' setting (%s). Configured platform must be 'common' when there is more than one platform in the hostedPlatforms list.", ClientPlatformTypeToString(mBootConfigTdf.getConfiguredPlatform()));
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
    }

    // validate our services configuration
    ServiceNameInfoMap hostedServicesInfo;
    {
        // Make sure we have services that can be hosted on this instance
        if (mBootConfigTdf.getServiceNames().empty())
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: no serviceName specified in boot config.");
            validationErrors.getErrorMessages().push_back().set(msg.c_str()); 
        }

        // Build the map of the platform specific hosted services by this instance
        for (const auto& serviceNamePair : mBootConfigTdf.getServiceNames())
        {
            if (hostedPlatformSet.find(serviceNamePair.second->getPlatform()) == hostedPlatformSet.end())
                continue;
            
            hostedServicesInfo[serviceNamePair.first] = hostedServicesInfo.allocate_element();
            serviceNamePair.second->copyInto(*hostedServicesInfo[serviceNamePair.first]);
        }

        // Make sure that the platform specific hosted services has the default service for each platform
        for (auto defaultPlatformServicePair : mBootConfigTdf.getPlatformToDefaultServiceNameMap())
        {
            if (hostedPlatformSet.find(defaultPlatformServicePair.first) == hostedPlatformSet.end())
                continue;

            bool foundDefaultServiceForHostedPlatform = false;
            for (auto& serviceInfoPair : hostedServicesInfo)
            {
                if (defaultPlatformServicePair.first == serviceInfoPair.second->getPlatform()
                    && blaze_strcmp(defaultPlatformServicePair.second, serviceInfoPair.first) == 0)
                {
                    foundDefaultServiceForHostedPlatform = true;
                    break;
                }
            }

            if (!foundDefaultServiceForHostedPlatform)
            {
                FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: default service (%s) for platform (%s) not listed in service info of boot config.", defaultPlatformServicePair.second.c_str(), ClientPlatformTypeToString(defaultPlatformServicePair.first));
                validationErrors.getErrorMessages().push_back().set(msg.c_str());
            }
        }
    }

    // validate our configuration for common platform and service
    if (isSharedCluster())
    {
        // Make sure that we have a default service for common platform
        eastl::string commonServiceName;
        auto iter = mBootConfigTdf.getPlatformToDefaultServiceNameMap().find(common);
        if (iter == mBootConfigTdf.getPlatformToDefaultServiceNameMap().end())
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: no default service configured for platform (common)");
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
        else
        {
            commonServiceName.assign(iter->second);
        }
        
        // Make sure we have a common service
        bool foundCommonService = false;
        for (const auto& serviceNamePair : mBootConfigTdf.getServiceNames())
        {
            if (serviceNamePair.second->getPlatform() == ClientPlatformType::common)
            {
                if (foundCommonService)
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: more than 1 service specified their platform as common. Not allowed.");
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                    // In order to collect other errors, we are not breaking out of loop intentionally
                }

                foundCommonService = true;
                
                hostedServicesInfo[serviceNamePair.first] = hostedServicesInfo.allocate_element();
                serviceNamePair.second->copyInto(*hostedServicesInfo[serviceNamePair.first]);

                if (commonServiceName.compare(serviceNamePair.first) != 0)
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: common service name(%s) in defaultPlatformToServiceMap does not match service info(%s).", commonServiceName.c_str(), serviceNamePair.first.c_str());
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                }
            }
        }

        if (!foundCommonService)
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: common service for shared instance not listed in boot config.");
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
    }

    for (const auto& serviceNamePair : hostedServicesInfo)
    {
        const char8_t* productName = serviceNamePair.second->getProductName();
        if (productName[0] == '\0' && serviceNamePair.second->getPlatform() != common)
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Controller].validateHostedServicesConfig: productName is not specified for service name %s.", serviceNamePair.first.c_str());
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
    }

}

void Controller::configureHostedServices()
{
    mHostedPlatforms.clear();
    mSupportedNamespaceList.clear();
    mHostedServicesInfo.clear();

    // First establish the platforms we are hosting
    for (auto platform : mBootConfigTdf.getHostedPlatforms())
    {
        mHostedPlatforms.insert(platform);
        mSupportedNamespaceList.insert(getNamespaceFromPlatform(platform));

        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].configureHostedServices: hosting platform " << ClientPlatformTypeToString(platform));
    }

    if (isSharedCluster())
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].configureHostedServices: hosting platform " << ClientPlatformTypeToString(common));
    }

    // set up the hosted service names based on the platforms we are hosting.
    for (const auto& serviceNamePair : mBootConfigTdf.getServiceNames())
    {
        if ((mHostedPlatforms.find(serviceNamePair.second->getPlatform()) != mHostedPlatforms.end())
            || (isSharedCluster() && (serviceNamePair.second->getPlatform() == ClientPlatformType::common))
        )
        {
            mHostedServicesInfo[serviceNamePair.first] = mHostedServicesInfo.allocate_element();
            serviceNamePair.second->copyInto(*mHostedServicesInfo[serviceNamePair.first]);

            BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].configureHostedServices: hosting service (" << serviceNamePair.first.c_str() <<
                ") for platform (" << ClientPlatformTypeToString(serviceNamePair.second->getPlatform()) << ")");
        }
    }
}


/*! ***************************************************************************/
/*!    \brief Bootstrap function that connects to the master controller and starts 
           off our controller activation.

           This function must run from within a fiber - it handles connecting
           to the master controller, getting the initial component list,
           and starts off the component activation process.
*******************************************************************************/
void Controller::controllerBootstrap()
{
    BlazeRpcError rc;

    // Verify service configuration right away 
    ConfigureValidationErrors validationErrors;
    validateHostedServicesConfig(validationErrors);
    if (validationErrors.getErrorMessages().size() > 0)
    {
        for (auto& error : validationErrors.getErrorMessages())
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, error.c_str());
        }
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure validating hosted services config");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    configureHostedServices();
    
    mGrpcUserAgent.append_sprintf("%s:%s", getDefaultServiceName(), getInstanceName());

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: getting instance id from redis...");
    if (gRedisManager->initFromConfig(mBootConfigTdf.getRedisConfig()) != REDIS_ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize redis manager.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    mInstanceId = mInstanceInfoController.allocateInstanceIdFromRedis();
    if (mInstanceId == INVALID_INSTANCE_ID)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to allocate instance id from redis.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    mInstanceSessionId = SlaveSession::makeSlaveSessionId(mInstanceId, 0);
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: assigned instance id " << mInstanceId << "(" << SbFormats::HexLower(mInstanceId) << ")");

    mInstanceInfoController.onInstanceIdReady();

    if (isInstanceTypeConfigMaster())
    {
        // configMaster's Controller is special, no need to resolve anything, link up the instance manually
        mMaster = (MasterController*) mComponentManager.addLocalComponent(MasterController::COMPONENT_ID);
        if (mMaster == nullptr)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: "
                "Failed to create master controller component");
            shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
            popControllerStateTransition();
            return;
        }
    }

    // controller needs to add itself locally to get the correct count back as it does not call addLocalComponent() nor addRemoteComponent() on itself
    addInstance(mInstanceId);

    if (BlazeRpcComponentDb::hasDuplicateResourcePath())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: has duplicate http resource path");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    ComponentIdList componentIds;
    const ServerConfig::StringComponentsList& gnames = mServerConfigTdf.getComponents();
    for (ServerConfig::StringComponentsList::const_iterator
        gname = gnames.begin(), egname = gnames.end(); gname != egname; ++gname)
    {
        const BootConfig::ComponentGroupsMap& groups = mBootConfigTdf.getComponentGroups();
        BootConfig::ComponentGroupsMap::const_iterator g = groups.find(*gname);
        if (g == groups.end())
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Undefined component group: " << gname->c_str());
            shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
            popControllerStateTransition();
            return;
        }

        bool isVerbatimComponentGroup = g->second->getVerbatim();
        const BootConfig::ComponentGroup::StringComponentsList& cnames = g->second->getComponents();
        for (BootConfig::ComponentGroup::StringComponentsList::const_iterator
            cname = cnames.begin(), ecname = cnames.end(); cname != ecname; ++cname)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(*cname);
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Unknown component: " << cname->c_str());
                shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
                popControllerStateTransition();
                return;
            }

            if (!isVerbatimComponentGroup)
            {
                //if we haven't been told to specifically make a master id, decide based on whether or not this is a master instance whether to do a slave or master.
                if (!RpcIsMasterComponent(id))
                {
                    if (isInstanceTypeMaster())
                    {
                        id = RpcMakeMasterId(id);
                        if (!BlazeRpcComponentDb::isValidComponentId(id))
                        {
                            BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: skipping 'master-less' component (" << cname->c_str() << ")");
                            continue;
                        }
                    }
                    else
                    {
                        id = RpcMakeSlaveId(id);
                        if (!BlazeRpcComponentDb::isValidComponentId(id))
                        {
                            BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: skipping 'slave-less' component (" << cname->c_str() << ")");
                            continue;
                        }
                    }
                }
            }

            ComponentStub* component = mComponentManager.addLocalComponent(id); 
            if (component != nullptr)
            {
                componentIds.push_back(id);
            }
            else
            {
                BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to create component: " << (BlazeRpcComponentDb::getComponentNameById(id)));
                shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
                popControllerStateTransition();
                return;
            }
        }
    }

    if (!mInstanceInfoController.initializeRedirectorConnection())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize redirector connection");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!mInstanceInfoController.startRemoteInstanceSynchronization())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to resolve remote server instances");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!isInstanceTypeConfigMaster())
    {
        transitionControllerState(ComponentStub::RESOLVING);
        // wait indefinitely until a connection to the ConfigMaster is established
        mMaster = (MasterController*) getComponent(MasterController::COMPONENT_ID, false, true, &rc);
        if (rc != ERR_OK)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: "
                "Failed to get master controller component, rc: " << ErrorHelp::getErrorName(rc));
            shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
            popControllerStateTransition();
            return;
        }

        transitionControllerState(ComponentStub::RESOLVED);
    }

    rc = getMaster()->notificationSubscribe();
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Master controller notification subscribe failed " << (ErrorHelp::getErrorName(rc)) << "(" << rc << ")");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }
    
    //Now that we have the MasterController component, subscribe for notifications
    getMaster()->addNotificationListener(*this);

    transitionControllerState(ComponentStub::CONFIGURING);

    GetServerConfigRequest getConfigReq;
    GetServerConfigResponse getConfigResp;
    getConfigReq.setBaseName(mBaseName); // base name is only used for checking if ConfigMaster knows about our instance type. The return value is always just the entire boot config tdf.
    rc = getMaster()->getServerConfigMaster(getConfigReq, getConfigResp);
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Master controller subscription failed " << (ErrorHelp::getErrorName(rc)) << "(" << rc << ")");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    const BootConfig::ServerConfigsMap& serverConfigs = getConfigResp.getMasterBootConfig().getServerConfigs();
    BootConfig::ServerConfigsMap::const_iterator configIt = serverConfigs.find(mBaseName);
    if (configIt == getConfigResp.getMasterBootConfig().getServerConfigs().end())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Server instance=" << mBaseName << " config not found.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: got instance config from ConfigMaster; starting configuration.");

    // aux slave types are indexed(starting @ 1) consistently throughout the system by using the order they are stored in the config map
    for (BootConfig::ServerConfigsMap::const_iterator i = serverConfigs.begin(), e = serverConfigs.end(); i != e; ++i)
    {
        if (i->second->getInstanceType() == ServerConfig::AUX_SLAVE)
        {
            if (blaze_strcmp(i->first.c_str(), mBaseName) == 0)
                mInstanceTypeIndex = mSlaveTypeCount;
            ++mSlaveTypeCount;
        }
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: instance base name (" << mBaseName << ") instance type index(" << mInstanceTypeIndex << ").");
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: total(" << mSlaveTypeCount << ") core and aux slave types known.");

    if (!validateConfiguredMockServices(getUseMockConsoleServices()))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Mock platform was enabled on a non-dev/test server. This is not supported.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    // register subscribe to ourself.
    rc = notificationSubscribe();
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: controller notification subscribe failed " << (ErrorHelp::getErrorName(rc)) << "(" << rc << ")");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!isInstanceTypeConfigMaster())
    {
        // update our instance config from the BootConfig delivered from the configMaster
        configIt->second->copyInto(mServerConfigTdf);
    }

    // get rest of the configuration from the ConfigMaster
    rc = getConfigTdf("logging", false, mLoggingConfigTdf);
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure fetching logging config.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    Logger::configure(mLoggingConfigTdf, false);

    SSLConfig sslConfigTdf;
    rc = getConfigTdf("ssl", false, sslConfigTdf);
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure fetching ssl config.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!gSslContextManager->initializeContextsUseContextState(sslConfigTdf))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize ssl contexts. ");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    VaultConfig vaultConfigTdf;
    rc = getConfigTdf("vault", false, vaultConfigTdf);
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure fetching vault config.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    ConfigureValidationErrors vaultErrors;
    gVaultManager->validateConfig(vaultConfigTdf, vaultErrors);
    if (!vaultErrors.getErrorMessages().empty())
    {
        const ConfigureValidationErrors::StringValidationErrorMessageList &errorList = vaultErrors.getErrorMessages();
        for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
        {
            BLAZE_ERR(Log::CONTROLLER, cit->c_str());
        }
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }
    if (!gVaultManager->configure(vaultConfigTdf))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to configure the vault manager");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    // While the SSL context manager was initialized earlier (in order to allow us to make an outbound
    // secure conn to Vault), we load the server certs after configuring vault because we may need
    // to fetch them from vault
    if (!gSslContextManager->loadServerCerts(false))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to load SSL server certs");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    rc = getConfigTdf("framework", false, mFrameworkConfigTdf);
    if (rc != ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure fetching framework config.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    // Validate configuration and print out errors if validation fails.
    // First dynamically update any SecretVaultLookup entries in the configuration. This will make a blocking call to the gVaultSecretManager.
    bool vaultSuccess = gVaultManager->updateTdfWithSecrets(mFrameworkConfigTdf, validationErrors);
    if (!vaultSuccess ||
        !onValidateConfig(mFrameworkConfigTdf, nullptr, validationErrors))
    {
        const ConfigureValidationErrors::StringValidationErrorMessageList &errorList =  validationErrors.getErrorMessages();
        for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
        {
            BLAZE_ERR(Log::CONTROLLER, cit->c_str());
        }
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure " << (vaultSuccess ? "validating config TDF." : "updating config TDF with Vault secrets."));
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    Metrics::gMetricsCollection->configure(mFrameworkConfigTdf.getMetricsLoggingConfig().getOperationalInsightsExport());

    gFiberManager->configure(mFrameworkConfigTdf.getFibers());

    HttpXmlProtocol::setDefaultEnumEncodingIsValue(mFrameworkConfigTdf.getHttpXmlConfig().getEnumOption() == HttpXmlProtocolConfig::VALUE);
    HttpXmlProtocol::setDefaultXmlOutputIsElements(mFrameworkConfigTdf.getHttpXmlConfig().getXmlOutputOption() == HttpXmlProtocolConfig::ELEMENTS);
    
    HttpXmlProtocol::setUseCompression(mFrameworkConfigTdf.getHttpXmlConfig().getUseCompression());
    EventXmlProtocol::setCompressNotifications(mFrameworkConfigTdf.getHttpXmlConfig().getCompressEventNotifications());
    HttpXmlProtocol::setCompressionLevel(mFrameworkConfigTdf.getHttpXmlConfig().getCompressionLevel());

    RestProtocol::setUseCompression(mFrameworkConfigTdf.getRestConfig().getUseCompression());
    RestProtocol::setCompressionLevel(mFrameworkConfigTdf.getRestConfig().getCompressionLevel());

    gProfanityFilter->onConfigure(mFrameworkConfigTdf.getProfanityConfig());
    gLocalization->configure(mFrameworkConfigTdf.getLocalization(), mFrameworkConfigTdf.getDefaultLocale());

    // gOutboundHttpService needs to be configured before gOutboundMetricsManager because gOutboundMetricsManager will
    // automatically include metrics tracking for all services registered with gOutboundHttpService
    if (!gOutboundHttpService->configure(mFrameworkConfigTdf.getHttpServiceConfig()))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to configure the gOutboundHttpService");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!gOutboundMetricsManager->configure(mFrameworkConfigTdf.getOutboundMetricsConfig()))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to configure the gOutboundMetricsManager");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    //We can now initialize the DB
    if (gDbScheduler->initFromConfig(mFrameworkConfigTdf.getDatabaseConfig()) != DB_ERR_OK 
        || gDbScheduler->initSchemaVersions() != DB_ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize database scheduler because of database errors");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    bool loggerDbNameNotFound = mLoggingConfigTdf.getDbNamesByPlatform().find(gController->getDefaultClientPlatform()) == mLoggingConfigTdf.getDbNamesByPlatform().end();
    bool loggerDbIdNotFound = !gDbScheduler->parseDbNameByPlatformTypeMap("logger", mLoggingConfigTdf.getDbNamesByPlatform(), Logger::getDbIdByPlatformTypeMap());
    if (loggerDbNameNotFound || loggerDbIdNotFound)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure configuring logging database. Logger db "
            << (loggerDbNameNotFound ? "name" : "")
            << (loggerDbIdNotFound && loggerDbNameNotFound ? " and db " : "")
            << (loggerDbIdNotFound ? "id" : "") 
            << " not found.");
        
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!upgradeSchemaWithDbMig(Logger::getDbSchemaVersion(), 0, "logger", Logger::getDbIdByPlatformTypeMap()))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failure updating logging DB schema.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    Logger::configure(mLoggingConfigTdf, false);
    fetchAndUpdateAuditState();

    uint16_t basePort = 0;
    if (!gProcessController->isUsingAssignedPorts() && !reserveEndpointPorts(basePort))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to get base port.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition(); 
        return;
    }

    // Enable processing of new connections at tcp level (otherwise mutually dependent servers would not be able to start their components)
    if (!initializeConnectionManager(basePort))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize connection manager");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!initializeInboundGrpcManager(basePort + mConnectionManager.getEndpointCount()))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize Grpc server");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    if (!initializeOutboundGrpcManager())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to initialize Outbound Grpc manager");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    // shutdown settings reconfiguration has to go after connectionmanager reconfiguration because it may depend on its max endpoint timeout
    configureShutdownSettings();

    mInstanceInfoController.onControllerConfigured();
   
    transitionControllerState(ComponentStub::CONFIGURED);

    checkClientTypeBindings();

    transitionControllerState(ComponentStub::ACTIVATING);

    // perform staged bring up of all local components
    if (!executeComponentStateChange(componentIds, ComponentStub::ACTIVE, &ComponentStub::activateComponent, "ComponentStub::activateComponent"))
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Controller failed to bring up components");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    //Swap out the db scheduler timeout (which is set to a long startup timeout) to a more reasonable runtime timeout
    mFrameworkConfigTdf.getDatabaseConfig().getDbScheduler().setDbStartupQueryTimeout(mFrameworkConfigTdf.getDatabaseConfig().getDbScheduler().getDbRuntimeQueryTimeout());

    //We're done with activation!
    popControllerStateTransition();

    // signal final controller transition
    transitionControllerState(ComponentStub::ACTIVE);

    // allow other server instances to connect to this server and have their rpc execute
    mConnectionManager.resumeEndpoints();

    // allow clients to connect to grpc server
    if (!mInboundGrpcManager->startGrpcServers())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: Failed to start grpc server.");
        shutdownController(SHUTDOWN_BOOTSTRAP_ERROR);
        popControllerStateTransition();
        return;
    }

    gProcessController->getVersion().copyInto(mServerEventData.getVersion());
    mServerEventData.setInstanceId(mInstanceId);
    mServerEventData.setInstanceName(mInstanceName);
    mServerEventData.setInstanceSessionId(mInstanceSessionId);
    const ServiceNameInfoMap& serviceNameInfos = getHostedServicesInfo();
    for (ServiceNameInfoMap::const_iterator itr = serviceNameInfos.begin(), itrend = serviceNameInfos.end(); itr != itrend; ++itr)
    {
        mServerEventData.getServiceNames().push_back(itr->first);
    }

    gEventManager->submitBinaryEvent(EVENT_STARTUPEVENT, mServerEventData);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].controllerBootstrap: " 
        << mInstanceName << " instance ACTIVE");

    // Start checking the CPU budget 
    mSelector.scheduleCall(this, &Controller::checkCpuBudget);         
    
    // start transaction clean sweeps
    mSelector.scheduleFiberCall(this, &Controller::transactionContextCleanSweep, 
        "Controller::transactionContextCleanSweep");
        
    scheduleMetricsTimer();
    scheduleOIMetricsTimer();

    scheduleCertRefreshTimer();

    if (isInstanceTypeConfigMaster())
    {
        scheduleExpireAuditEntries(TimeValue::getTimeOfDay());
    }

    updateInServiceStatus();

    //Get a clean slate for timing data, if desired.
    if (mFrameworkConfigTdf.getFibers().getTimeOnlyActiveServerFibers())
    {
        //NOTE: We need to schedule this for later, this *must* be the last function call in 
        //controllerBootstrap(), or else the current bootstrap function will show up in the data
        mSelector.scheduleCall(this, &Controller::clearFiberTimingData);
    }

    mBootstrapComplete = true;

    // Bootstrap is now complete...note that this instance may still not be IN-SERVICE. That's an async process started by updateInServiceStatus call above and will continue
    // to reschedule itself as needed.
}

void Controller::handleRemoteInstanceInfo(const RemoteInstanceInfo& remoteInstanceInfo, bool isRedirectorUpdate)
{
    const bool isRedirectorAuthoritative = mBootConfigTdf.getUseRedirectorForRemoteServerInstances();

    // Equivalent logic: (isRedirectorUpdate && isRedirectorAuthoritative) || (!isRedirectorUpdate && !isRedirectorAuthoritative);
    const bool isAuthoritativeSource = (isRedirectorUpdate == isRedirectorAuthoritative);
    const char8_t* remoteInstanceDataSource = isRedirectorUpdate ? "redirector update" : "remote instance info";

    if (!remoteInstanceInfo.mAddrList.empty())
    {
        char8_t addrBuf[256];
        uint32_t numOfExistingInstances = 0;
        for (const RemoteInstanceAddress& serverAddress : remoteInstanceInfo.mAddrList)
        {
            InetAddress inetAddr(serverAddress.mIp, serverAddress.mPort, InetAddress::HOST);
            if (mRemoteInstancesByAddr.find(inetAddr) != mRemoteInstancesByAddr.end())
            {
                // skip known instances
                ++numOfExistingInstances;
                continue;
            }

            if (isAuthoritativeSource)
            {
                // schedule a connect to the new instance
                connectAndActivateInstance(inetAddr);
            }
            else if (isInService() && !isOurInternalEndpointAddress(inetAddr))
            {
                BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].handleRemoteInstanceInfo: "
                    "New remote server instance @ " << inetAddr.toString(addrBuf, sizeof(addrBuf))
                    << " was found in the latest " << remoteInstanceDataSource << " but not yet connected.");
            }
        }

        if (numOfExistingInstances < mRemoteInstancesByAddr.size())
        {
            eastl::vector<InetAddress> remoteInstanceAddrToRemove;
            for (const auto& knownRemoteInstancePair : mRemoteInstancesByAddr)
            {
                const InetAddress& knownRemoteInstanceAddr = knownRemoteInstancePair.first;
                const RemoteServerInstance* knownRemoteInstanceInfo = knownRemoteInstancePair.second;

                bool foundInRemoteInstanceInfo = false;
                for (const RemoteInstanceAddress& serverAddress : remoteInstanceInfo.mAddrList)
                {
                    if (serverAddress.mIp == knownRemoteInstanceAddr.getIp(InetAddress::HOST) &&
                        serverAddress.mPort == knownRemoteInstanceAddr.getPort(InetAddress::HOST))
                    {
                        // found the known remote server instance in the latest remote instance info
                        foundInRemoteInstanceInfo = true;
                        break;
                    }
                }

                if (!foundInRemoteInstanceInfo)
                {
                    if (knownRemoteInstanceInfo->isConnected())
                    {
                        // Is the remote instance connected but not yet initialized?
                        if (knownRemoteInstanceInfo->getInstanceId() == INVALID_INSTANCE_ID)
                        {
                            BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].handleRemoteInstanceInfo: "
                                "Connected but uninitialized remote server instance "
                                << " @ " << knownRemoteInstanceAddr.toString(addrBuf, sizeof(addrBuf))
                                << " is missing in the latest " << remoteInstanceDataSource << ".");
                        }
                        else
                        {
                            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].handleRemoteInstanceInfo: "
                                "Connected remote server instance " << knownRemoteInstanceInfo->getInstanceId()
                                << " @ " << knownRemoteInstanceAddr.toString(addrBuf, sizeof(addrBuf))
                                << " is missing in the latest " << remoteInstanceDataSource << "."
                                << (isAuthoritativeSource ? " Will keep connection intact for now." : ""));
                        }
                    }
                    else if (!knownRemoteInstanceInfo->isShutdown())
                    {
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].handleRemoteInstanceInfo: "
                            "Pending/disconnected remote server instance @ " << knownRemoteInstanceAddr.toString(addrBuf, sizeof(addrBuf))
                            << " is missing in the latest " << remoteInstanceDataSource << "."
                            << (isAuthoritativeSource ? " Removing it locally." : ""));

                        if (isAuthoritativeSource)
                        {
                            // Don't shutdownAndRemoveInstance immediately because we'll end up deleting the instance which will alter our iterator
                            remoteInstanceAddrToRemove.push_back(knownRemoteInstanceInfo->getInternalAddress());
                        }
                    }
                }
            }

            for (const InetAddress& disconnectedAddr : remoteInstanceAddrToRemove)
            {
                Fiber::CreateParams params;
                params.namedContext = "Controller::shutdownAndRemoveInstance";
                gFiberManager->scheduleCall(this, &Controller::shutdownAndRemoveInstance, disconnectedAddr, false, params);
            }
        }
    }
}

void Controller::updateInServiceStatus()
{
    if (mInService)
        return;
    
    // In order to go *in service*, 
        // 1. A blaze server instance must be *ready* for service which means
            // 1.a It's connected to all the remote instances it knows of
            // 1.b All such instances are active.
        //
        // AND
        //
        // 2. Must own at least 1 sliver if the instance hosts namespaces(components) that support slivers. 
        //

    // NOte: We don't lose our IN-SERVICE state if an instance springs up later and we can't connect to it.
    char8_t buf[256];
    BlazeRpcError rc = ERR_OK;    
    bool reschedule = false;
    RemoteServerInstance* inst = nullptr;
    RemoteInstanceByAddrMap::const_iterator it = mRemoteInstancesByAddr.begin();
    while (it != mRemoteInstancesByAddr.end())
    {
        inst = it->second;
        if (inst->getMutuallyConnected())
        {
            ++it;
        }
        else if (!inst->isActive())
        {
            // remote instance isn't active yet, reschedule
            reschedule = true;
            break;
        }
        else
        {
            // We already have a connection to this remote instance.
            // Ask the remote instance if it has a connection to us and thinks we are active (this instance is definitely active locally).
            rc = inst->checkMutualConnectivity();
            if (rc != ERR_OK)
            {
                if (rc == CTRL_ERROR_SERVER_INSTANCE_ALREADY_REGISTERED)
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: "
                        "Remote instance " << inst->toString(buf, sizeof(buf)) 
                        << " reports another instance already using our instance id: " << getInstanceId());
                }
                else if (rc != CTRL_ERROR_SERVER_INSTANCE_NOT_REGISTERED)
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: "
                        "Cluster membership check failed for instance " << inst->toString(buf, sizeof(buf)) 
                        << ", rc: " << ErrorHelp::getErrorName(rc));
                }
                reschedule = true;
                break;
            }
            // restart iteration for safety, as the collection may have mutated
            it = mRemoteInstancesByAddr.begin();
        }
    }

    if (reschedule)
    {
        // At least one of the remote instances does not have an outbound connection to us yet or is not active.
        if (mInServiceCheckCounter == 0)
        {
            const char* reason = "";
            if (inst->isConnected() && inst->isActive())
                reason = " to confirm it's outbound connection to us.";
            else if (!inst->isActive())
                reason = " to become active.";

            // only print this log every 15 in-service checks to avoid spamming the logs
            BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: "
                << "Waiting for (" 
                << (inst->isConnected() ? "connected" : "pending") 
                << (inst->isActive() ? ", active" : ", not active") 
                << ") instance "
                << inst->toString(buf, sizeof(buf)) << reason);
        }
    }
    else
    {
        if (!mIsReadyForService)
        {
            mReadyForServiceDispatcher.dispatch(&ReadyForServiceListener::onControllerReadyForService);
            mIsReadyForService = true;
            mReadyForServiceWaiters.signal(ERR_OK);
        }

        if (!gSliverManager->canTransitionToInService())
        {
            if (mInServiceCheckCounter == 0)
            {
                // only print this log every 15 in-service checks to avoid spamming the logs
                BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: Waiting for SliverManager confirm sliver ownership.");
            }
            reschedule = true;
        }
    }

    if (reschedule)
    {
        mInServiceCheckCounter = (mInServiceCheckCounter + 1) % 15;
        TimeValue updateTime = TimeValue::getTimeOfDay() + UPDATE_SERVICE_STATUS_INTERVAL*1000;
        mSelector.scheduleFiberTimerCall(updateTime, this, &Controller::updateInServiceStatus, 
            "Controller::updateInServiceStatus");
        return;
    }

    if (mIsStartInService)
    {
        mInService = true;
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: " 
            << mInstanceName << " instance IN-SERVICE");
    }
    else
    {
        mInService = false;
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].updateInServiceStatus: " 
            << mInstanceName << " instance OUT-OF-SERVICE");
    }

    mInstanceInfoController.onControllerServiceStateChange();

    // Only configure exception handling once we've gone in service.  Until then any exceptions
    // should be considered fatal, and the instance should go down to both draw attention
    // to the severity of the issue, as well as ensure we at least are in a good state to start
    gProcessController->configureExceptionHandler(mFrameworkConfigTdf.getExceptionConfig());
}

uint32_t Controller::getSchemaVersion(uint32_t dbId, const char8_t* componentName) const
{
    return gDbScheduler->getComponentVersion(dbId, componentName);
}

const ServiceNameInfo* Controller::getServiceNameInfo(const char8_t* serviceName) const
{
    ServiceNameInfoMap::const_iterator itr = mHostedServicesInfo.find(serviceName);
    if (itr != mHostedServicesInfo.end())
        return itr->second;

    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getServiceNameInfo: The service ("<< serviceName <<") is not hosted by this instance.");
    return nullptr;
}

const char8_t* Controller::getServiceProductName(const char8_t* serviceName) const
{
    const ServiceNameInfo* info = getServiceNameInfo(serviceName);
    if (info != nullptr)
        return info->getProductName();

    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getServiceProductName: The service (" << serviceName << ") is not hosted by this instance. No product info available.");
    return nullptr;
}

ClientPlatformType Controller::getServicePlatform(const char8_t* serviceName) const
{
    const ServiceNameInfo* info = getServiceNameInfo(serviceName);
    if (info != nullptr)
        return info->getPlatform();

    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getServicePlatform: The service (" << serviceName << ") is not hosted by this instance. No platform info available.");
    return INVALID;
}

const char8_t* Controller::getServicePersonaNamespace(const char8_t* serviceName) const
{
    ClientPlatformType servicePlatform = INVALID;
    const ServiceNameInfo* info = getServiceNameInfo(serviceName);
    if (info != nullptr)
        servicePlatform = info->getPlatform();

    if ((servicePlatform == INVALID) || (servicePlatform == common)) 
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getServicePersonaNamespace: The service platform(" << ClientPlatformTypeToString(servicePlatform) << ") does not have a persona namespace.");
        return nullptr;
    }
    // service name unrecognized, return INVALID
    return getNamespaceFromPlatform(servicePlatform);
}

const char8_t* Controller::getReleaseType(const char8_t* serviceName) const
{
    const ServiceNameInfo* serviceNameInfo = getServiceNameInfo(serviceName);
    if (serviceNameInfo != nullptr)
    {
        return gUserSessionManager->getProductReleaseType(serviceNameInfo->getProductName());
    }

    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].getReleaseType: The service (" << serviceName << ") is not hosted by this instance. No release type info available.");
    return nullptr;
}

bool Controller::isNucleusClientIdInUse(const char8_t* clientId) const
{
    return (blaze_stricmp(clientId, getBlazeServerNucleusClientId()) == 0);
}

/*! ***************************************************************************/
/*!    \brief calls the master to run the external python dbmig script to upgrade the database.
*******************************************************************************/
bool Controller::upgradeSchemaWithDbMig(uint32_t componentVersion,
                                        uint32_t contentVersion,
                                        const char8_t* componentName,
                                        const DbIdByPlatformTypeMap& dbIdByPlatformTypeMap) const
{
    RunDbMigInfo request;
    request.setVersion(componentVersion);
    request.setContentVersion(contentVersion);
    request.setComponentName(componentName);
    dbIdByPlatformTypeMap.copyInto(request.getDbIdByPlatformType());
    request.setDbDestructive(isDBDestructiveAllowed());

    RpcCallOptions opts;
    opts.timeoutOverride = EA::TDF::TimeValue(mFrameworkConfigTdf.getDatabaseConfig().getDbmig().getDbmigDurationTimeout());

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].upgradeSchemaWithDbMig: Calling runDbMig for Component " << componentName << "'.");

    BlazeRpcError err = getMaster()->runDbMig(request, opts);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].upgradeSchemaWithDbMig: Failed to upgrade Component " << componentName << "Schema, with error " << ErrorHelp::getErrorName(err) << ".");
        if (err == ERR_TIMEOUT)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].upgradeSchemaWithDbMig: Consider increasing DbmigDurationTimeout to avoid the timeout.  Current value = " << mFrameworkConfigTdf.getDatabaseConfig().getDbmig().getDbmigDurationTimeout().getSec() << "s.");
        }
        return false;
    }

    return true;
}

void Controller::addPendingConfigFeatures(const ConfigFeatureList& list)
{
    mPendingFeatures.clear();
    // Setup the features to validate.  Note this reuses the member used 
    for (ConfigFeatureList::const_iterator i = list.begin(), e = list.end(); i != e; ++i)
    {
        const char8_t* featureName = *i;
        ComponentId masterId, slaveId;
        if (BlazeRpcComponentDb::isNonProxyComponent(featureName) && BlazeRpcComponentDb::getComponentIdsByBaseName(featureName, slaveId, masterId))
        {
            //get the component and its name and put it in the list
            Component* slaveComponent = nullptr;
            if (slaveId != 0 && (slaveComponent = getComponent(slaveId, true, false)) != nullptr)
            {
                //add the slave name
                mPendingFeatures.insert(slaveComponent->getName());
            }

            Component* masterComponent = nullptr;
            if (masterId != 0 && (masterComponent = getComponent(masterId, true, false)) != nullptr)
            {
                //add the master name
                mPendingFeatures.insert(masterComponent->getName());
            }
        }
        else
        {
            // this is a non-component feature
            mPendingFeatures.insert(featureName);
        }
    }
}


void Controller::onNotifyPrepareForReconfigure(const Blaze::PrepareForReconfigureNotification& data, UserSession*)
{
    if (mReconfigureState.mReconfigureInProgress)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].onNotifyPrepareForReconfigure: Reconfigure already in progress.");
        if (!data.getFeatures().empty())
        {
            // Report this as a validation error for the first feature in the list of features to be reconfigured.
            // This will cause the configMaster to abort the reconfigure.
            // We need to use a feature from the list because the configMaster would otherwise ignore the validation error.
            ConfigureValidationErrors validationErrors;
            eastl::string msg;
            msg.sprintf("[Controller].onNotifyPrepareForReconfigure: Received prepareForReconfigure notification from configMaster while a reconfigure was already in progress.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            setConfigureValidationErrorsForFeature(data.getFeatures().begin()->c_str(), validationErrors);
        }
        else
        {
            // This should never happen, since the configMaster rejects reconfigure requests containing an empty list of features
            BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].onNotifyPrepareForReconfigure: Received prepareForReconfigure notification with empty feature list.");
        }
    }
    else
    {
        mReconfigureState.mReconfigureInProgress = true;
        mReconfigureState.mValidationErrors.release();          //  clear validation errors for new validation task.

        //Store off the features from the notification.
        addPendingConfigFeatures(data.getFeatures());
    }
    //Note: The context here is important because we cancel by it during shutdown.
    Fiber::CreateParams params;
    params.blocking = true;
    params.stackSize = Fiber::STACK_MEDIUM;
    params.groupId = mTaskGroupId;
    params.namedContext = sComponentChangeNamedContext;
    gFiberManager->scheduleCall(this, &Controller::internalPrepareForReconfigure, params);
}

void Controller::internalPrepareForReconfigure()
{
    if (!mPendingFeatures.empty() && mReconfigureState.mValidationErrors.empty())
    {
        if (mPendingFeatures.find("logging") != mPendingFeatures.end())
        {
            LoggingConfig newConfigTdf;
            BlazeRpcError rc = getConfigTdf("logging", true, newConfigTdf);
            if (rc == ERR_OK)
            {
                newConfigTdf.copyInto(mPendingLoggingConfigTdf);
            }
            else 
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Failure fetching config for logging, error=" << ErrorHelp::getErrorName(rc));
                mLoggingConfigTdf.copyInto(mPendingLoggingConfigTdf);
            }
        }

        bool reconfigureFramework = mPendingFeatures.find("framework") != mPendingFeatures.end();

        if (mPendingFeatures.find("ssl") != mPendingFeatures.end())
        {
            SSLConfig newConfigTdf;
            BlazeRpcError rc = getConfigTdf("ssl", true, newConfigTdf);
            if (rc == ERR_OK)
            {
                if (!gSslContextManager->startReloadingContexts(newConfigTdf))
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Component (ssl) failed validation.");
                    eastl::string msg;
                    msg.sprintf("[Controller].internalPrepareForReconfigure: Failed to load one or more SSL context(s).");
                    ConfigureValidationErrors validationErrors;
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    setConfigureValidationErrorsForFeature("ssl", validationErrors);
                }
                else if (!reconfigureFramework)
                {
                    // Verify that all ssl contexts used by secure endpoints exist in the new config.
                    // This check is performed when we validate the framework config, so we skip it if
                    // framework is one of the features being reconfigured.
                    ConfigureValidationErrors validationErrors;
                    for (EndpointConfig::EndpointTypesMap::const_iterator itEndpoint = mFrameworkConfigTdf.getEndpoints().getEndpointTypes().begin();
                        itEndpoint != mFrameworkConfigTdf.getEndpoints().getEndpointTypes().end(); ++itEndpoint)
                    {
                        Endpoint::validateSslContext(*(itEndpoint->second), itEndpoint->first.c_str(), validationErrors);
                    }
                    if (!validationErrors.getErrorMessages().empty())
                        setConfigureValidationErrorsForFeature("ssl", validationErrors);
                }
            }
            else 
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Failure fetching config for ssl.");
            }
        }

        if (mPendingFeatures.find("vault") != mPendingFeatures.end())
        {
            VaultConfig vaultConfigTdf;
            BlazeRpcError rc = getConfigTdf("vault", true, vaultConfigTdf);
            if (rc == ERR_OK)
            {
                ConfigureValidationErrors vaultErrors;
                gVaultManager->validateConfig(vaultConfigTdf, vaultErrors);
                if (!vaultErrors.getErrorMessages().empty() || !gVaultManager->configure(vaultConfigTdf))
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Component (vault) failed validation and/or encountered an error during configuration.");
                }
                if (!vaultErrors.getErrorMessages().empty())
                    setConfigureValidationErrorsForFeature("vault", vaultErrors);
            }
            else
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Failure fetching config for vault.");
            }
        }

        // Similar to the bootstrap flow, we split out the loading of ssl server certs to happen
        // after any potential reconfiguration of vault, as we may need to fetch the certs from vault
        if (mPendingFeatures.find("ssl") != mPendingFeatures.end())
        {
            ConfigureValidationFailureMap::iterator it = mReconfigureState.mValidationErrors.find("ssl");
            if (it == mReconfigureState.mValidationErrors.end())
            {
                if (!gSslContextManager->loadServerCerts(true))
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Component (ssl) failed validation.");
                    eastl::string msg;
                    msg.sprintf("[Controller].internalPrepareForReconfigure: Failed to load one or more SSL server cert(s).");
                    ConfigureValidationErrors validationErrors;
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    setConfigureValidationErrorsForFeature("ssl", validationErrors);
                }
            }
        }

        if (reconfigureFramework)
        {
            mPendingFrameworkConfigTdf = nullptr;

            FrameworkConfig newConfigTdf;
            BlazeRpcError rc = getConfigTdf("framework", true, newConfigTdf);
            if (rc == ERR_OK)
            {
                mPendingFrameworkConfigTdf = mFrameworkConfigTdf.clone();

                ConfigMerger merger;
                ConfigureValidationErrors validationErrors;

                if (merger.merge(*mPendingFrameworkConfigTdf, newConfigTdf))
                {
                    // Dynamically update any VaultLookup entries in the configuration. This will make a blocking call to the gVaultManager.
                    if (!gVaultManager->updateTdfWithSecrets(*mPendingFrameworkConfigTdf, validationErrors) ||
                        !onValidateConfig(*mPendingFrameworkConfigTdf, &mFrameworkConfigTdf, validationErrors))
                    {
                        const ConfigureValidationErrors::StringValidationErrorMessageList &errorList = validationErrors.getErrorMessages();
                        for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
                        {
                            BLAZE_ERR(Log::CONTROLLER, cit->c_str());
                        }
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Component (framework) failed validation.");
                        setConfigureValidationErrorsForFeature("framework", validationErrors);
                        mPendingFrameworkConfigTdf = nullptr;
                    }
                    else
                    {
                        gDbScheduler->prepareForReconfigure(mPendingFrameworkConfigTdf->getDatabaseConfig());
                    }
                }
                else
                {
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: failed to merge existing configuration for framework.");

                    validationErrors.getErrorMessages().push_back("[Controller].internalPrepareForReconfigure: Failed to merge existing configuration.");
                    setConfigureValidationErrorsForFeature("framework", validationErrors);
                    mPendingFrameworkConfigTdf = nullptr;
                }
            }
            else 
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Failure fetching config for framework.");
            }
        }

        ComponentIdList componentsToValidate;
        for(ConfigFeatureSet::const_iterator i = mPendingFeatures.begin(), e = mPendingFeatures.end(); i != e; ++i)
        {
            ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(i->c_str());
            if (componentId != 0)
            {
                componentsToValidate.push_back(componentId);
            }
        }

        if (!componentsToValidate.empty())
        {
            // Validate components.
            executeBlockingComponentMethod(componentsToValidate, &ComponentStub::prepareForReconfigure, "ComponentStub::prepareForReconfigure");
        }
    }

    // the master expects a response even if we had no features to reconfigure
    PreparedForReconfigureRequest req;
    req.setValidationErrorMap(mReconfigureState.mValidationErrors);
    if (!mReconfigureState.mValidationErrors.empty())
    {
        // StringBuilder doesn't accept TdfStructMap, so using the request TDF (which is just a wrapper for the TdfStructMap), and it shouldn't add that much extra cruft to the log output
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: failed validation with errors:\n" << req);
    }
    BlazeRpcError rc = getMaster()->preparedForReconfigureMaster(req);
    if (rc != ERR_OK && rc != ERR_CANCELED)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalPrepareForReconfigure: Failed to send prepare configure ACK to the master - returning slave to ACTIVE state as master will not signal.");
    }
}

//  referenceConfig is nullptr on initial configuration.
bool Controller::onValidateConfig(FrameworkConfig& config, const FrameworkConfig* referenceConfig, ConfigureValidationErrors& validationErrors) const
{
    ExceptionConfig& exceptionConfig = config.getExceptionConfig();

    // Correct new configuration if invalid values are specified in the new configuration.
    if (exceptionConfig.getExceptionHandlingEnabled())
    {
        if (exceptionConfig.getDelayPerFiberException() == 0)
        {
            eastl::string msg;
            msg.sprintf("[Controller].onValidateConfig: exception delay must be greater than zero seconds.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());            
        }
        eastl::string corePrefixName = exceptionConfig.getCoreFilePrefix();
        if (corePrefixName.find_first_of("<>:\"\\|?*") != eastl::string::npos)
        {
            eastl::string msg;
            msg.sprintf("[Controller].onValidateConfig: core filename prefix (%s) contains invalid characters.", corePrefixName.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());            
        }
    }

    // These configuration values must be valid or else fail validation.
    const EndpointConfig* oldEndpointConfig = nullptr;
    if (referenceConfig != nullptr)
        oldEndpointConfig = &(referenceConfig->getEndpoints());
    Endpoint::validateConfig(config.getEndpoints(), oldEndpointConfig, mServerConfigTdf.getEndpoints(), config.getRates(), validationErrors);

    gDbScheduler->validateConfig(config.getDatabaseConfig(), (referenceConfig != nullptr ? &referenceConfig->getDatabaseConfig() : nullptr), validationErrors);

    // Warning if localization is not available
    Localization::validateConfig(config.getLocalization(), config.getLocStringMaxLength(), validationErrors);

    gOutboundHttpService->prepareForReconfigure(config.getHttpServiceConfig(), validationErrors);
    mOutboundGrpcManager->prepareForReconfigure(config.getOutboundGrpcServiceConfig().getOutboundGrpcServices(), validationErrors);
    gOutboundMetricsManager->validateConfig(config.getOutboundMetricsConfig(), validationErrors);
    
    validateTrialNameRemaps(config.getRedirectorSettingsConfig(), validationErrors);
    validateHostedServicesConfig(validationErrors);

    if (getBlazeServerNucleusClientId()[0] == '\0')
    {
        eastl::string msg(eastl::string::CtorSprintf(), "[Controller].onValidateConfig: No Nucleus client id specified.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (getServiceEnvironment()[0] == '\0')
    {
        eastl::string msg(eastl::string::CtorSprintf(), "[Controller].onValidateConfig: No service environment specified.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].onValidateConfig: Blaze Service Environment: " << getServiceEnvironment());

    MetricsExporter::validateConfig(config.getMetricsLoggingConfig().getMetricsExport(), validationErrors);

    int32_t entryCount = gProfanityFilter->onValidateConfig(config.getProfanityConfig(), validationErrors);
    if (entryCount < 0)
    {
        eastl::string msg;
        msg.sprintf("[Controller].onValidateConfig: Invalid profanity filter list. List may contain unsupported characters or have too many unique characters or too many equivalent character definitions.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].onValidateConfig: ProfanityFilter validated with "<<entryCount<<" entries.");
    }

    //  if there are validation errors, consider this validation check a failure.
    return validationErrors.getErrorMessages().empty();
}

void Controller::onNotifyPrepareForReconfigureComplete(const Blaze::PrepareForReconfigureCompleteNotification& data, UserSession*)
{
    if (data.getRestoreToActive())
    {
        // the entire system has been reconfigured with validate only or errors
        transitionControllerState(ComponentStub::ACTIVE);
        gProcessController->clearReconfigurePending();
        mReconfigureState.mReconfigureInProgress = false;
        gSslContextManager->finishReloadingContexts(false);
    }
    if (mReconfigureState.mSemaphoreId != 0)
    {
        BlazeRpcError err = ERR_OK;
        if (!data.getValidationErrorMap().empty())
        {
            gProcessController->clearReconfigurePending();
            mReconfigureState.mReconfigureInProgress = false;
            gSslContextManager->finishReloadingContexts(false);
            err = CTRL_ERROR_RECONFIGURE_VALIDATION_FAILED;
        }
        data.getValidationErrorMap().copyInto(mReconfigureState.mValidationErrors);
        gFiberManager->signalSemaphore(mReconfigureState.mSemaphoreId, err);
    }
}

void Controller::onNotifyReconfigure(const ReconfigureNotification& data, UserSession*)
{
    if (data.getFinal())
    {
        // the entire system has been reconfigured
        transitionControllerState(ComponentStub::ACTIVE);
        gProcessController->clearReconfigurePending();
        mReconfigureState.mReconfigureInProgress = false;
        return;
    }

    //Store off the features from the notification.
    addPendingConfigFeatures(data.getFeatures());

    Fiber::CreateParams params;
    params.blocking = mBootConfigTdf.getAllowBlockingReconfigure(); //kevlar remove
    params.stackSize = Fiber::STACK_HUGE;
    params.namedContext = sComponentChangeNamedContext;
    params.groupId = mTaskGroupId;
    gFiberManager->scheduleCall(this, &Controller::internalReconfigure, mBootConfigTdf.getAllowBlockingReconfigure(), params);
}

/*! ***************************************************************************/
/*!    \brief Notification received when the master controller has instructed us 
           to disable a component.

    \param data with component information.
*******************************************************************************/
void Controller::onSetComponentState(const  SetComponentStateRequest& data, UserSession*)
{
    uint16_t componentId = BlazeRpcComponentDb::getComponentIdByName(data.getComponentName());

    ComponentStub* stub  = mComponentManager.getLocalComponentStub(componentId);

    if (stub != nullptr && !stub->asComponent().isMaster())
    {       
        switch (data.getAction())
        {
            case SetComponentStateRequest::ENABLE :
                stub->setEnabled(true);
                stub->setDisableErrorReturnCode(Blaze::ERR_OK);
                break;
            case SetComponentStateRequest::DISABLE :
                if (data.getErrorCode() > 0)
                {
                    stub->setDisableErrorReturnCode((BlazeRpcError)data.getErrorCode());
                }
                stub->setEnabled(false);
                break;
            default : break;
        }
    }
}

void Controller::configureShutdownSettings()
{
    mInboundGracefulClosureAckTimeout = getFrameworkConfigTdf().getConnectionManagerConfig().getGracefulClosureAckTimeout();
}

/*! ***************************************************************************/
/*!    \brief This function reconfigures all the components.
*******************************************************************************/
void Controller::internalReconfigure(bool allowBlocking)
{
    mConfigReloadTime = TimeValue::getTimeOfDay();

    if (mPendingFeatures.find("logging") != mPendingFeatures.end())
    {
        mPendingLoggingConfigTdf.copyInto(mLoggingConfigTdf);
        Logger::configure(mLoggingConfigTdf, true);
    }

    bool reconfigureSsl = mPendingFeatures.find("ssl") != mPendingFeatures.end();
    if (reconfigureSsl)
    {
        gSslContextManager->finishReloadingContexts(true);
        // If ssl is reconfigured, gOutboundHttpService needs to be reconfigured as well. Secure connections managed by 
        // gOutboundHttpService's OutboundHttpConnectionManagers do not refresh their ssl contexts each time their channels 
        // are reopened, but by reconfiguring gOutboundHttpService, we ensure these connections will be drained, closed,
        // and replaced by secure connections that use the updated ssl contexts.
        gOutboundHttpService->reconfigure(mFrameworkConfigTdf.getHttpServiceConfig());
    }
    
    if (mPendingFeatures.find("framework") != mPendingFeatures.end() && mPendingFrameworkConfigTdf != nullptr)
    {
        mFrameworkConfigTdf = *mPendingFrameworkConfigTdf;
        // Reconfigure core framework pieces
        
        gProcessController->configureExceptionHandler(mFrameworkConfigTdf.getExceptionConfig());
        mConnectionManager.reconfigure(mFrameworkConfigTdf.getEndpoints(), mFrameworkConfigTdf.getRates());
        mInboundGrpcManager->reconfigure(mFrameworkConfigTdf.getEndpoints(), mFrameworkConfigTdf.getRates());
        mOutboundGrpcManager->reconfigure(mFrameworkConfigTdf.getOutboundGrpcServiceConfig().getOutboundGrpcServices());

        // shutdown settings reconfiguration has to go after connectionmanager reconfiguration because it may depend on its max endpoint timeout
        configureShutdownSettings();

        gFiberManager->configure(mFrameworkConfigTdf.getFibers());

        gDbScheduler->reconfigure(mFrameworkConfigTdf.getDatabaseConfig());

        gProfanityFilter->onReconfigure(mFrameworkConfigTdf.getProfanityConfig());

        if (!gLocalization->configure(mFrameworkConfigTdf.getLocalization(), mFrameworkConfigTdf.getDefaultLocale()))
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalReconfigure: Failure configuring localization");
        }

        // gOutboundHttpService needs to be reconfigured before gOutboundMetricsManager (if it hasn't already been reconfigured)
        // because gOutboundMetricsManager will automatically include metrics tracking for all services registered with gOutboundHttpService
        if (!reconfigureSsl)
            gOutboundHttpService->reconfigure(mFrameworkConfigTdf.getHttpServiceConfig());
        gOutboundMetricsManager->reconfigure(mFrameworkConfigTdf.getOutboundMetricsConfig());

        HttpXmlProtocol::setDefaultEnumEncodingIsValue(mFrameworkConfigTdf.getHttpXmlConfig().getEnumOption() == HttpXmlProtocolConfig::VALUE);
        HttpXmlProtocol::setDefaultXmlOutputIsElements(mFrameworkConfigTdf.getHttpXmlConfig().getXmlOutputOption() == HttpXmlProtocolConfig::ELEMENTS);

        HttpXmlProtocol::setUseCompression(mFrameworkConfigTdf.getHttpXmlConfig().getUseCompression());
        EventXmlProtocol::setCompressNotifications(mFrameworkConfigTdf.getHttpXmlConfig().getCompressEventNotifications());
        HttpXmlProtocol::setCompressionLevel(mFrameworkConfigTdf.getHttpXmlConfig().getCompressionLevel());

        RestProtocol::setUseCompression(mFrameworkConfigTdf.getRestConfig().getUseCompression());
        RestProtocol::setCompressionLevel(mFrameworkConfigTdf.getRestConfig().getCompressionLevel());

        mInstanceInfoController.onControllerReconfigured();

        Metrics::gMetricsCollection->configure(mFrameworkConfigTdf.getMetricsLoggingConfig().getOperationalInsightsExport());

        scheduleMetricsTimer();
        scheduleOIMetricsTimer();

        scheduleCertRefreshTimer();

        if (isInstanceTypeConfigMaster())
        {
            scheduleExpireAuditEntries(TimeValue::getTimeOfDay());
        }

        mPendingFrameworkConfigTdf = nullptr;
    }

    if (allowBlocking)
    {
        ComponentIdList componentsToReconfigure;
        for(ConfigFeatureSet::const_iterator i = mPendingFeatures.begin(), e = mPendingFeatures.end(); i != e; ++i)
        {
            ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(i->c_str());
            if (componentId != 0)
            {
                componentsToReconfigure.push_back(componentId);
            }
        }
        
        mPendingFeatures.clear();

        if (!componentsToReconfigure.empty())
        {
            // Reconfigure specified components
            executeBlockingComponentMethod(componentsToReconfigure, &ComponentStub::reconfigureComponent, 
                "ComponentStub::reconfigureComponent", COMPONENT_STAGE_MAX);
        }

        reportReconfigureFinished();
    }
    else
    {
        for(ConfigFeatureSet::const_iterator i = mPendingFeatures.begin(), e = mPendingFeatures.end(); i != e; ++i)
        {
            ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(i->c_str());
            if (componentId != 0)
            {
                ComponentStub* stub = mComponentManager.getLocalComponentStub(componentId);
                if (stub != nullptr)
                {
                    stub->reconfigureComponent();
                }
            }
        }

        //its ok to run the report on a fiber.

        Fiber::CreateParams params;
        params.blocking = true;
        params.stackSize = Fiber::STACK_MEDIUM;
        params.groupId = mTaskGroupId;
        params.namedContext = sComponentChangeNamedContext;
        gFiberManager->scheduleCall(this, &Controller::reportReconfigureFinished, params);
    }
}

void Controller::reportReconfigureFinished()
{
    // master expects a response even if we had no features to reconfigure
    BlazeRpcError rc = getMaster()->reconfiguredMaster();
    if (rc != ERR_OK && rc != ERR_CANCELED)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].internalReconfigure: Failed to send reconfigure ACK to the master.");
    }

    gProcessController->clearReconfigurePending();
    mReconfigureState.mReconfigureInProgress = false;
}

/*! ***************************************************************************/
/*!    \brief Called from the processor thread or externally to reconfigure 
           the controller.
*******************************************************************************/
void Controller::reconfigureController()
{
    mSelector.scheduleFiberCall(this, &Controller::internalReconfigureController, "Controller::internalReconfigureController");
}

void Controller::internalReconfigureController()
{
    if (getMaster() == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].reconfigureController: Master does not exist when reconfigure was called. This will not succeed.");
        gProcessController->clearReconfigurePending();
        mReconfigureState.mReconfigureInProgress = false;
        return;
    }

    ReconfigureRequest req;
    if (mBootConfigTdf.getHupReconfigureFeatures().empty())
        req.getFeatures().push_back("logging");
    else
    {
        // take overriding list of features to reconfigure via SIGHUP/CTRL+Break
        for (auto&& feature : mBootConfigTdf.getHupReconfigureFeatures())
        {
            req.getFeatures().push_back(feature.c_str());
        }
    }

    UserSession::SuperUserPermissionAutoPtr permissionPtr(true);
    BlazeRpcError rc = getMaster()->reconfigureMaster(req);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].reconfigureController: Failed to reconfigure master, rc: " << ErrorHelp::getErrorName(rc));
        gProcessController->clearReconfigurePending();
        mReconfigureState.mReconfigureInProgress = false;
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////Controller Shutdown code.
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void Controller::onShutdownBroadcasted(const ShutdownRequest& data, UserSession*)
{
    gSelector->scheduleFiberCall<Controller, ShutdownRequestPtr>(this, &Controller::doOnShutdownBroadcasted, &const_cast<ShutdownRequest&>(data), "Controller::doOnShutdownBroadcasted");
}

void Controller::doOnShutdownBroadcasted(ShutdownRequestPtr data)
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].doOnShutdownBroadcasted: this instance [" << getInstanceId() << "] received shutdown broadcast:\n" << data);

    if (data->getTarget() == SHUTDOWN_TARGET_ALL)
    {
        executeShutdown(data->getRestart(), data->getReason());
    }
    else if (!data->getInstanceIds().empty())
    {
        ShutdownRequest::InstanceIdList::const_iterator i =  data->getInstanceIds().begin();
        ShutdownRequest::InstanceIdList::const_iterator e =  data->getInstanceIds().end();
        for (; i != e; ++i)
        {
            if (getInstanceId() == *i)
            {
                executeShutdown(data->getRestart(), data->getReason());
                break;
            }
        }
    }
}

void Controller::shutdownController(ShutdownReason shutdownReason)
{
    if (!gProcessController->isShuttingDown())
    {
        // make the process controller aware that it needs to shutdown all threads it owns
        ProcessController::ExitCode exitCode = ProcessController::EXIT_NORMAL;
        switch (shutdownReason)
        {
            case SHUTDOWN_BOOTSTRAP_ERROR:
                exitCode = ProcessController::EXIT_FAIL;
                break;
            case SHUTDOWN_REBOOT:
                exitCode = ProcessController::EXIT_RESTART;
                break;
            default:
                break;
        }
        gProcessController->shutdown(exitCode);
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownController: controller already shutting down, ignoring duplicate shutdown attempt.");
    }
}

void Controller::shutdownControllerInternal(ShutdownReason shutdownReason)
{
    if (mShuttingDown || mShutdownPending)
    {
        return;
    }

    mShutdownPending = true;

    if (gFiberManager->getCommandFiberCount() > 0)
    {
        TimeValue commandTimeout;
        for (const auto& endpoint : mConnectionManager.getEndpoints())
        {
            if (commandTimeout < endpoint->getCommandTimeout())
                commandTimeout = endpoint->getCommandTimeout();
        }
        for (const auto& endpoint : mInboundGrpcManager->getGrpcEndpoints())
        {
            if (commandTimeout < endpoint->getCommandTimeout())
                commandTimeout = endpoint->getCommandTimeout();
        }

        BlazeError waitErr = gFiberManager->waitForCommandFibers(TimeValue::getTimeOfDay() + commandTimeout, "Controller::shutdownControllerInternal");
        if (waitErr != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: Not all command fibers completed after waiting " << commandTimeout.getMillis() << "ms"
                "(wait error = " << waitErr << ").  Remaining commands will be cancelled.");
        }
    }

    mShutdownPending = false;
    mShuttingDown = true;

    if (shutdownReason == SHUTDOWN_NORMAL || shutdownReason == SHUTDOWN_REBOOT)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: Remapping [FATAL] log messages to [WARN] due to user initiated SHUTDOWN.");
        Logger::remapLogLevels();
    }

    if (mAuditEntryExpiryTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mAuditEntryExpiryTimerId);
        mAuditEntryExpiryTimerId = INVALID_TIMER_ID;
    }

    mInstanceInfoController.shutdownController();

    gProcessController->setRestartAfterShutdown(shutdownReason == SHUTDOWN_REBOOT);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: halting server activity.");

    uint32_t waiters = mReadyForServiceWaiters.getCount();
    mReadyForServiceWaiters.signal(ERR_CANCELED);
    if (waiters > 0)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: canceled " << waiters << " ready-for-service waiters.");
    }

    transitionControllerState(ComponentStub::SHUTTING_DOWN);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: prepared controller for shutdown.");

    // Stop accepting new connections off the endpoints
    mConnectionManager.pauseEndpoints();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: paused inbound endpoints.");

    // Gracefully close inbound connections
    mConnectionManager.shutdown();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: shutdown inbound endpoints.");

    mInboundGrpcManager->shutdown();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: shutdown inbound grpc manager.");

    mOutboundGrpcManager->shutdown();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: shutdown outbound grpc manager.");

    // Force shutdown outbound connections
    mOutboundConnectionManager.shutdown();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: shutdown outbound endpoints.");

    // Tell the replicator to stop handling any running sync processes
    gReplicator->shutdown();
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: shutdown replicator.");

    // Nuke any running command fibers
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: cancel pending tasks.");
    Fiber::cancelGroup(mTaskGroupId, ERR_CANCELED);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: begin remote instance teardown (" << mRemoteInstancesByAddr.size() << " remaining).");

    // delete all existing remote instances
    while (!mRemoteInstancesByAddr.empty())
    {
        bool waitPendingShutdown = true;
        for (RemoteInstanceByAddrMap::iterator i = mRemoteInstancesByAddr.begin(), e = mRemoteInstancesByAddr.end(); i != e; ++i)
        {
            RemoteServerInstance& instance = *i->second;
            if (instance.shutdown())
            {
                // as long as we are making progress shutting down
                // remote instances we don't need to wait.
                waitPendingShutdown = false;
                mRemoteInstancesByAddr.erase(instance.getInternalAddress());
                mRemoteInstancesById.erase(instance.getInstanceId());
                delete &instance;
                break;
            }
        }
        if (waitPendingShutdown)
        {
            BlazeRpcError rc = mSelector.sleep(REMOTE_INSTANCE_SHUTDOWN_TIMEOUT*1000);
            if (rc != ERR_OK)
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: " << (ErrorHelp::getErrorName(rc)) << " error calling sleep pending shutdown");
            }
        }
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownControllerInternal: finish remote instance teardown.");

    // Kick off component shutdown
    shutdownComponents();
    
    // Destroy components
    destroyComponents();

    // Shutdown instance
    gCurrentThread->stop();
}

/*! ***************************************************************************/
/*!    \brief This function shuts down all our components.
*******************************************************************************/
void Controller::shutdownComponents()
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownComponents: begin component shutdown.");

    ComponentIdList componentIds;
    componentIds.reserve(mComponentManager.getLocalComponents().size());
    for (ComponentManager::ComponentStubMap::const_iterator i = mComponentManager.getLocalComponents().begin(), e = mComponentManager.getLocalComponents().end(); i != e; ++i)
        componentIds.push_back(i->first);

    TimeValue timeoutTime(TimeValue::getTimeOfDay().getMicroSeconds() + (SHUTDOWN_STATE_CHANGE_TIMEOUT * 1000));

    // tell all components to shutdown.  If one or more components did not successfully transition to the SHUTDOWN state
    // before timeoutTime expires, the component state change will be canceled to allow server shutdown execution to continue
    executeComponentStateChange(componentIds, ComponentStub::SHUTDOWN, &ComponentStub::shutdownComponent, "ComponentStub::shutdownComponent", timeoutTime);   

    // cancel all fibers (except for this one)
    Fiber::cancelAll(ERR_CANCELED);

    // stop processing Db requests now that component shutdown has finished executing
    gDbScheduler->stopAllowingRequests();

    // stop processing Redis requests now that component shutdown has finished executing
    gRedisManager->deactivate();

    // perform final controller state transition
    transitionControllerState(ComponentStub::SHUTDOWN);
    
    // db scheduler is no longer usable, we do this here because this includes closing all connections cleanly which may involve performing blocking operations on the DB
    gDbScheduler->stop();

    // make sure no outstanding timers or jobs fire
    // (component shutdown may have accidentally scheduled some)
    mSelector.removeAllEvents();

    gEventManager->submitBinaryEvent(EVENT_SHUTDOWNEVENT, mServerEventData);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].shutdownComponents: finish component shutdown.");
}

void Controller::destroyComponents()
{
    // destroy all replicated maps before deleting local components because
    // some replicated items may depend to them (ie: UserSessionManager)
    gReplicator->destroy();
    
    // stop selecting anything
    mSelector.close();

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].destroyComponents: begin component destruction.");
    
    mComponentManager.shutdown();
    mMaster = nullptr;

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].destroyComponents: finish component destruction.");
}

bool Controller::executeComponentStateChange(const ComponentIdList& components, ComponentStub::State desiredState, void (ComponentStub::*func)(), const char8_t* methodName, const TimeValue& timeoutTime)
{
    bool result = true;
    for (int32_t stageIndex = COMPONENT_STAGE_FRAMEWORK; stageIndex < COMPONENT_STAGE_MAX; ++stageIndex)
    {
        ComponentStage stage;
        // start from framework down for activate, go the opposite way for shutdown
        if (desiredState == ComponentStub::ACTIVE)
            stage = (ComponentStage) stageIndex;
        else
            stage = (ComponentStage) (COMPONENT_STAGE_MAX - 1 - stageIndex);
      

        //Before we block on changing all the components, set a timer that will go off and fire
        //info out to the log about what's holding up the transition
        TimerId stateChangeInfoTimer;
        scheduleComponentStateChangeTimeout(stage, desiredState, stateChangeInfoTimer);

        result = executeBlockingComponentMethod(components, func, methodName, stage, timeoutTime);

        gSelector->cancelTimer(stateChangeInfoTimer);
        stateChangeInfoTimer = INVALID_TIMER_ID;
        

        //Check all components to make sure they transitioned properly
        for (ComponentManager::ComponentStubMap::const_iterator i = mComponentManager.getLocalComponents().begin(), e = mComponentManager.getLocalComponents().end(); i != e; ++i)
        {
            if (i->second->getState() == ComponentStub::ERROR_STATE)
            {
                //We've reached an error - mark us for shutdown.  We don't exit here because we don't want to 
                //cut off any running fibers or DB threads by accident.                    
                BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].executeComponentStateChange: " << i->second->getComponentName()
                                << "(" << SbFormats::Raw("0x%04x", i->second->getComponentId()) << ") error changing to state " << (ComponentStub::getStateName(desiredState)));
                result = false;
            }
        }

        if (!result)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[Controller].executeComponentStateChange: controller could not transition to " 
                << (ComponentStub::getStateName(desiredState)) << " state.");
        }        
    }

    return result;
}

bool Controller::executeBlockingComponentMethod(const ComponentIdList& components, void (ComponentStub::*func)(), 
                                                const char8_t* methodName, ComponentStage stageFilter, const TimeValue& timeoutTime)
{
    typedef eastl::list<ComponentStub*> CompList;
    CompList changingComponents;
    for (ComponentIdList::const_iterator i = components.begin(), e = components.end(); i != e; ++i)
    {
        if (stageFilter == COMPONENT_STAGE_MAX || stageFilter == getStage(*i))
        {
            ComponentStub* component = mComponentManager.getLocalComponentStub(*i);
            if (component != nullptr)
            {
                //add to the list of changing components
                changingComponents.push_back(component);
            }
        }
    }

    if (!changingComponents.empty())
    {
        //Get a new component group id for this change.  
        mChangeGroupId = Fiber::allocateFiberGroupId();
        
        Fiber::CreateParams fiberParams;
        fiberParams.blocking = true;
        fiberParams.groupId = mChangeGroupId;
        fiberParams.namedContext = sComponentChangeNamedContext;
        fiberParams.stackSize = Fiber::STACK_MEDIUM;

        for (CompList::iterator i = changingComponents.begin(), e = changingComponents.end(); i != e; ++i)
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].executeBlockingComponentMethod: " << ((*i)->getComponentName()) 
                << " now executing " << methodName);

            //Call the function on the component using a fiber
            gFiberManager->scheduleCall(*i, func, fiberParams);
        }

        BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].executeBlockingComponentMethod: joining group id " << mChangeGroupId);

        BlazeRpcError rc = Fiber::join(mChangeGroupId, timeoutTime);

        if (rc != ERR_OK)
        {
            //If we were canceled, cancel our fibers as well.
            Fiber::cancelGroup(mChangeGroupId, rc);

            BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].executeBlockingComponentMethod: controller failed executing " << methodName);
            return false;
        }
    }

    return true;
}

void Controller::scheduleComponentStateChangeTimeout(ComponentStage stage, ComponentStub::State desiredState, TimerId& outTimerId)
{
    TimeValue timeoutTime(TimeValue::getTimeOfDay().getMicroSeconds() + (STATE_CHANGE_TIMEOUT * 1000));
    outTimerId = mSelector.scheduleTimerCall<Controller, ComponentStage, ComponentStub::State, TimerId&>(timeoutTime, this, 
        &Controller::executeComponentStateChangeTimeout, stage, desiredState, outTimerId,
        "Controller::executeComponentStateChangeTimeout");
}

void Controller::executeComponentStateChangeTimeout(ComponentStage stage, ComponentStub::State desiredState, TimerId& outTimerId)
{
    ComponentManager::ComponentStubMap::iterator i = mComponentManager.getLocalComponents().begin();
    ComponentManager::ComponentStubMap::iterator e = mComponentManager.getLocalComponents().end();
    
    StringBuilder sb;
    for(; i != e; ++i)
    {
        ComponentStub* stub = i->second;
        if (stage == getStage(stub->getComponentId()))
        {
            if (stub->getState() != desiredState)
            {
                sb.append("Component:(%s) Current State:(%s) Desired State:(%s)\n", stub->getComponentName(), (ComponentStub::getStateName(stub->getState())), (ComponentStub::getStateName(desiredState)));
            }
        }
    }

    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].executeComponentStateChangeTimeout: problematic components holding state transition \n" << sb.c_str());

    Fiber::dumpFiberGroupStatus(mTaskGroupId);
    Fiber::dumpFiberGroupStatus(mChangeGroupId);

    scheduleComponentStateChangeTimeout(stage, desiredState, outTimerId);
}


void Controller::transitionControllerState(ComponentStub::State desiredState)
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].transitionControllerState: (" << (getComponentName())
        << ") started transition from " << (ComponentStub::getStateName(getState())) << " to " << (ComponentStub::getStateName(desiredState)));
    
    if (!Fiber::isCurrentlyBlockable())
    {
        // if called from the main fiber, schedule to run on a worker fiber
        mSelector.scheduleFiberCall(this, &Controller::transitionControllerState, desiredState, "Controller::transitionControllerState");
        return;
    }
    
    if (mState != desiredState)
    {
        const ComponentStub::State state = mState;
        mState = desiredState;
        mStateChangeDispatcher.dispatch(&StateChangeListener::onControllerStateChange, state, mState);
    }
}

/*! ***************************************************************************/
/*! \param[out] config If successful this will be set to the (new) config. nullptr on error.
    \return ERR_OK if successful, otherwise an appropriate error code.
*******************************************************************************/
BlazeRpcError Controller::getConfig(const char8_t* featureName, bool loadFromStaging, const ConfigFile*& config, EA::TDF::Tdf* configTdf, bool strictParsing, bool preconfig) const
{
    ConfigRequest request;
    ConfigResponse response;
    request.setFeature(featureName);
    request.setLoadFromStaging(loadFromStaging);
    request.setLoadPreconfig(preconfig);
    BlazeRpcError rc = getMaster()->getConfigMaster(request, response);
    if (rc != ERR_OK)
    {
        config = nullptr;
        return rc;
    }
    bool allowUnquotedStrings = false;
  #ifdef BLAZE_ENABLE_DEPRECATED_UNQUOTED_STRING_IN_CONFIG
    allowUnquotedStrings = true;
  #endif

    config = ConfigFile::createFromString(response.getConfigData(), response.getOverrideConfigData(), allowUnquotedStrings, nullptr, gProcessController->getConfigStringEscapes());
    rc = ERR_SYSTEM;
    if (config != nullptr)
    {
        if (configTdf != nullptr)
        {
            ConfigDecoder decoder(config->getFeatureConfigMap(featureName), strictParsing, featureName);
            if (decoder.decode(configTdf))
            {
                rc = ERR_OK;
            }
            else
            {
                delete config;
                config = nullptr;
            }
        }
        else
            rc = ERR_OK;
    }
    return rc;
}

BlazeRpcError Controller::getConfigTdf(const char8_t* featureName, bool loadFromStaging, EA::TDF::Tdf& configTdf, bool strictParsing, bool preconfig) const
{
    const ConfigFile* file = nullptr;
    BlazeRpcError rc = getConfig(featureName, loadFromStaging, file, &configTdf, strictParsing, preconfig);
    
    if (file != nullptr)
        delete file;

    return rc;
}

void Controller::onRemoteInstanceStateChanged(RemoteServerInstance& instance)
{
    if (isShuttingDown())
    {
        // if we are in the process of shutting down, handling remote instances just adds to the overhead, bail.
        return;
    }
    
    mRemoteInstanceDispatcher.dispatch<const RemoteServerInstance&>(&RemoteServerInstanceListener::onRemoteInstanceChanged, instance);

    char8_t nameBuf[256];
    if (!instance.isConnected())
    {
        bool signalShutdown = false;
        if(instance.getServiceImpact() == ServerConfig::CRITICAL)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].onRemoteInstanceStateChanged: "
                "lost connection to critical instance " << instance.toString(nameBuf, sizeof(nameBuf)) << ", shutting down...");
            signalShutdown = true;
        }
        else if (instance.getServiceImpact() == ServerConfig::CLUSTER)
        {
            // scan local components impacted by remote instance going away
            for (ComponentManager::ComponentStubMap::const_iterator i = mComponentManager.getLocalComponents().begin(), e = mComponentManager.getLocalComponents().end(); i != e; ++i)
            {
                ComponentId id = i->first;
                if (RpcIsMasterComponent(id))
                    continue;

                // check if local slave is impacted by a master going away with this remote server instance
                Component* master = getComponentManager().getComponent(RpcMakeMasterId(id));
                if (master != nullptr && !master->hasMultipleInstances() && instance.hasComponent(RpcMakeMasterId(id)))
                {
                    // master got disconnected, shutdown
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].onRemoteInstanceStateChanged: "
                        "lost connection to cluster-affecting instance " << instance.toString(nameBuf, sizeof(nameBuf))
                        << " hosting required component(" << master->getName() << "), shutting down...");
                    signalShutdown = true;
                    break;
                }
            }
        }
        if (signalShutdown)
        {
            gProcessController->shutdown(ProcessController::EXIT_MASTER_TERM);
            return;
        }
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].onRemoteInstanceStateChanged: "
            "lost connection to instance " << instance.toString(nameBuf, sizeof(nameBuf)));
        mSelector.scheduleFiberCall(this, &Controller::shutdownAndRemoveInstance,
            instance.getInternalAddress(), true, "Controller::shutdownAndRemoveInstance");
    }
}

void Controller::pushControllerStateTransition(Job& job)
{
    mStateTransitionMutex.Lock();

    //If the queue is empty, schedule the job immediately
    if (!mExecutingStateTransition)
    {
        mSelector.queueJob(&job);
        mExecutingStateTransition = true;
    }
    else
    {
        mStateTransitionJobQueue.push_back(&job);
    }    

    mStateTransitionMutex.Unlock();
}

void Controller::popControllerStateTransition()
{    
    mStateTransitionMutex.Lock();

    //If something was waiting to run, queue it up.
    if (!mStateTransitionJobQueue.empty())
    {
        mSelector.queueJob(mStateTransitionJobQueue.front());
        mStateTransitionJobQueue.pop_front();
    }    
    else
    {
        mExecutingStateTransition = false;
    }

    mStateTransitionMutex.Unlock();
}

GetInstanceInfoError::Error Controller::processGetInstanceInfo(GetInstanceInfoResponse &response, const Message *message)
{
    ServerInstanceInfo& info = response.getInstanceInfo();
    info.setInstanceId(getInstanceId());
    info.setInstanceName(mInstanceName);
    info.setInstanceType(mBaseName);
    info.setInstanceTypeIndex(mInstanceTypeIndex);
    info.setRegistrationId(mInstanceInfoController.getRegistrationId());
    info.setServiceImpact(isInstanceTypeMaster() ? mServerConfigTdf.getServiceImpact() : ServerConfig::TRANSIENT);
    // NOTE: Immediate partial local user replication setting only takes effect if partial user replication setting is enabled.
    info.setImmediatePartialLocalUserReplication(getPartialUserReplicationEnabled() && getImmediatePartialLocalUserReplicationEnabled());
    info.setIsDraining(isDraining());
    info.setStartupTime(mStartupTime);

    ServerInstanceInfo::ComponentMap& map = info.getComponents();
    map.reserve(mComponentManager.getLocalComponents().size());
    for (ComponentManager::ComponentStubMap::const_iterator 
        i = mComponentManager.getLocalComponents().begin(), 
        e = mComponentManager.getLocalComponents().end(); i != e; ++i)
    {
        map[i->second->getComponentName()] = i->first; 
    }
    
    mInstanceInfoController.getEndpointInfoList().copyInto(info.getEndpointInfoList());

    const Endpoint* ep = getInternalEndpoint();
    if (ep != nullptr)
    {
        // fill in the internal endpoint information for this server instance
        ServerInstanceInfo::EndpointInfo& iep = info.getInternalEndpoint();
        iep.setPort(ep->getPort());
        iep.setIntAddress(NameResolver::getInternalAddress().getIp());
        iep.setExtAddress(NameResolver::getExternalAddress().getIp());
        iep.setProtocol(ep->getProtocolName());
        iep.setEncoder(ep->getEncoderName());
        iep.setDecoder(ep->getDecoderName());
        return GetInstanceInfoError::ERR_OK;
    }
    
    // this should never happen, every server must have an 
    // internal endpoint used for server/server communication
    BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].processGetInstanceInfo: "
        "Failed to obtain internal endpoint.");
    
    return GetInstanceInfoError::ERR_SYSTEM;
}

CheckClusterMembershipError::Error Controller::processCheckClusterMembership(const CheckClusterMembershipRequest &req, const Message *message)
{
    char8_t buf[256];
    CheckClusterMembershipError::Error result = CheckClusterMembershipError::CTRL_ERROR_SERVER_INSTANCE_NOT_REGISTERED;
    RemoteInstanceByIdMap::const_iterator it = mRemoteInstancesById.find(req.getInstanceId());
    InetAddress remoteAddr(req.getInternalAddress(), req.getInternalPort(), InetAddress::NET);
    if (it != mRemoteInstancesById.end())
    {
        const RemoteServerInstance& inst = *it;
        if (remoteAddr == inst.getInternalAddress())
        {
            if (inst.isActive())
            {
                // This server is connected to the remote instance(which is active) sending the request; therefore, confirm the cluster membership.
                result = CheckClusterMembershipError::ERR_OK;
            }
            else
            {
                BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].processCheckClusterMembership: "
                    "Remote instance '" << inst.toString(buf, sizeof(buf)) << "' is not active.");
            }
        }
        else
        {
            char8_t addrBuf[256];
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].processCheckClusterMembership: "
                "Remote instance " << inst.toString(buf, sizeof(buf)) << " is already connected, can't register "
                "another instance with same id @ " << remoteAddr.toString(addrBuf, sizeof(addrBuf)));
            result = CheckClusterMembershipError::CTRL_ERROR_SERVER_INSTANCE_ALREADY_REGISTERED;
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].processCheckClusterMembership: "
            "Remote instance " << req.getInstanceName() << ":" << req.getInstanceId() 
            << " @ " << remoteAddr.toString(buf, sizeof(buf)) << " not found.");
    }
    return result;
}

BlazeRpcError Controller::checkClusterMembershipOnRemoteInstance(InstanceId remoteInstanceId)
{
    BlazeRpcError rc = ERR_SYSTEM;

    RpcCallOptions opts;
    opts.routeTo.setInstanceId(remoteInstanceId);

    CheckClusterMembershipRequest req;
    req.setInstanceId(getInstanceId());
    req.setRegistrationId(mInstanceInfoController.getRegistrationId());
    req.setInstanceName(getInstanceName());

    const Endpoint* ep = getInternalEndpoint();
    if (ep != nullptr)
    {
        req.setInternalAddress(NameResolver::getInternalAddress().getIp(InetAddress::NET));
        req.setInternalPort(ep->getPort(InetAddress::NET));
    }
    
    rc = checkClusterMembership(req, opts); // Ask remote instance if it knows us and think we are ACTIVE.

    return rc;
}

void Controller::getStatusInfo(ComponentStatus& status) const
{
    // retrieve metrics for generic information
    mExternalServiceMetrics.getStatusInfo(status);
    gProfanityFilter->getStatusInfo(status);

} // getStatusInfo


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////Server Instance Status
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/*! ***************************************************************************/
/*!    \brief Handles a request for server status.

    \param response [out] The server status 
    \return Any errors.
*******************************************************************************/
GetStatusError::Error Controller::processGetStatus(ServerStatus &response, const Message *message)
{    
    tickDeprecatedRPCMetric("blazecontroller", "getStatus", message);

    gProcessController->getVersion().copyInto(response.getVersion());
    response.setInstanceId(mInstanceId);
    response.setInstanceName(mInstanceName);
    response.setInstanceSessionId(mInstanceSessionId);

    response.setDefaultNamespace(getNamespaceFromPlatform(getDefaultClientPlatform()));

    for (auto curPlatform : getHostedPlatforms())
    {
        response.getAdditionalNamespaces().push_back(getNamespaceFromPlatform(curPlatform));
    }

    response.setProcessId(gProcessController->getProcessId());
    response.setCurrentWorkingDirectory(getHostCurrentWorkingDirectory());
    gProcessController->getCommandLineArgs().copyInto(response.getCommandLineArgs());
    response.setProcessOwner(mProcessOwner);

    char8_t buf[64];
    response.setStartupTime(mStartupTime.toString(buf, sizeof(buf)));
    response.setConfigReloadTime(mConfigReloadTime.toString(buf, sizeof(buf)));
    response.setUptime((TimeValue::getTimeOfDay()-mStartupTime).getMillis());
    response.setConfigUptime((TimeValue::getTimeOfDay()-mConfigReloadTime).getMillis());
    gDbScheduler->getStatusInfo(response.getDatabase());
    gRedisManager->getStatusInfo(response.getRedisDatabase());

    mConnectionManager.getStatusInfo(response.getConnectionManager());
    mInboundGrpcManager->getStatusInfo(response.getGrpcManagerStatus());
    mOutboundGrpcManager->getStatusInfo(response.getOutboundGrpcServiceStatus());
    mSelector.getStatusInfo(response.getSelector());
    gOutboundHttpService->getStatusInfo(response.getHttpServiceStatus());
    gEventManager->getStatus(response.getEventManagerStatus());

    Allocator::getStatus(response.getMemory());

    // Gather status for each of the configured components
    ServerStatus::ComponentStatusList& list = response.getComponents();
    list.reserve(mComponentManager.getLocalComponents().size());
    ComponentManager::ComponentStubMap::const_iterator itr = mComponentManager.getLocalComponents().begin();
    ComponentManager::ComponentStubMap::const_iterator end = mComponentManager.getLocalComponents().end();
    for(; itr != end; ++itr)
    {
        ComponentStub* stub = itr->second;

        ComponentStatus& status = *list.pull_back();
        status.setComponentId(stub->getComponentId());
        status.setComponentName(stub->getComponentName());
        status.setState(ComponentStub::getStateName(stub->getState()));

        if (stub->getState() == ComponentStub::ACTIVE)
        {
            stub->getStatusInfo(status);
            status.setIsReloadableComponent(stub->isReconfigurable());
        }
    }
    gFiberManager->getStatus(response.getFiberManagerStatus());    

    response.setInService(isInService());
    if (isDraining())
    {
        ServerStatus::DrainInfoMap& map = response.getDrainInfoMap();
        char8_t value[64];

        //Drain Start Time
        blaze_snzprintf(value, sizeof(value), "%" PRIi64, mDrainStartTime.getMicroSeconds());
        map["DrainStartTime"] = value;
    }

    response.setSslVersion(SSLeay_version(SSLEAY_VERSION));

    Logger::getStatus(response.getLoggerMetrics());

    return GetStatusError::ERR_OK;
}

GetComponentStatusError::Error Controller::processGetComponentStatus(const ComponentStatusRequest &request, ComponentStatus &response, const Message *message)
{
    ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(request.getComponentName());
    if (componentId == Component::INVALID_COMPONENT_ID)
    {
        return GetComponentStatusError::CTRL_ERROR_UNKNOWN_COMPONENT;
    }
    ComponentManager::ComponentStubMap::const_iterator itr = mComponentManager.getLocalComponents().find(componentId);
    if (itr == mComponentManager.getLocalComponents().end())
    {
        return GetComponentStatusError::CTRL_ERROR_COMPONENT_NOT_HOSTED;
    }

    ComponentStub* stub = itr->second;
    response.setComponentId(stub->getComponentId());
    response.setComponentName(stub->getComponentName());
    response.setState(ComponentStub::getStateName(stub->getState()));

    if (stub->getState() == ComponentStub::ACTIVE)
    {
        stub->getStatusInfo(response);
        response.setIsReloadableComponent(stub->isReconfigurable());
    }

    return GetComponentStatusError::ERR_OK;
}

GetConnMetricsError::Error Controller::processGetConnMetrics(const GetConnMetricsRequest &request, GetConnMetricsResponse &response, const Message* message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getConnMetrics", message);

    if(request.getIsOutbound())
    {
        mOutboundConnectionManager.getOutConnMetricsInfo(request, response);
    }
    else
    {
        mConnectionManager.getMetrics(request, response);
    }
    return GetConnMetricsError::ERR_OK;
}

GetComponentMetricsError::Error Controller::processGetComponentMetrics(
        const ComponentMetricsRequest& request, ComponentMetricsResponse &response, const Message *message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getComponentMetrics", message);

    ComponentManager::ComponentStubMap::const_iterator i = mComponentManager.getLocalComponents().begin();
    ComponentManager::ComponentStubMap::const_iterator e = mComponentManager.getLocalComponents().end();
    for(; i != e; ++i)
    {
        ComponentStub* stub = i->second;
        if ((request.getComponentId() == RPC_COMPONENT_UNKNOWN)
                || (request.getComponentId() == stub->getComponentId()))
        {
            stub->gatherMetrics(response, request.getCommandId());
        }
    }
    return GetComponentMetricsError::ERR_OK;
}

ConfigureMemoryMetricsError::Error Controller::processConfigureMemoryMetrics(
    const Blaze::ConfigureMemoryMetricsRequest &request, Blaze::ConfigureMemoryMetricsResponse &response, const Message* message)
{
    Allocator::processConfigureMemoryMetrics(request, response);
    
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processConfigureMemoryMetrics: " << UserSessionManager::formatAdminActionContext(message) 
        << ". Configured memory metrics, req = \n" << request);
    
    return ConfigureMemoryMetricsError::ERR_OK;
}

GetMemoryMetricsError::Error Controller::processGetMemoryMetrics(
    const Blaze::GetMemoryMetricsRequest &request, Blaze::GetMemoryMetricsResponse &response, const Message* message)
{
    Allocator::processGetMemoryMetrics(request, response);
    return GetMemoryMetricsError::ERR_OK;
}

GetPSUByClientTypeError::Error Controller::processGetPSUByClientType(const Blaze::GetMetricsByGeoIPDataRequest &request, Blaze::GetPSUResponse &response, const Message* message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getPSUByClientType", message);

    if (request.getIncludeISP())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].processGetPSUByClientType: The (deprecated) BlazeController::getPSUByClientType() RPC "
            "is being called from " << (message ? message->getPeerInfo().getPeerAddress() : InetAddress()) << " with the IncludeISP option set to true.  However, ISP information is "
            "no longer returned by this RPC.");
    }

    return GetPSUByClientTypeError::commandErrorFromBlazeError(gUserSessionManager->getLocalUserSessionCounts(response));
}

/*! ************************************************************************************************/
void Controller::executeDrainShutdown(bool restartAfterShutdown)
{
    // Take the server process out of service as we're draining
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].executeDrainShutdown: set service state to out-of-service");
    gProcessController->setServiceState(false);

    // no need to send duplicate notifications if they were already sent when we first went into draining mode
    if (!mIsDraining)
    {
        mIsDraining = true;
        mDrainStartTime = TimeValue::getTimeOfDay();

        ComponentManager::ComponentStubMap::const_iterator i = mComponentManager.getLocalComponents().begin();
        ComponentManager::ComponentStubMap::const_iterator e = mComponentManager.getLocalComponents().end();
        for (; i != e; ++i)
        {
            Component* comp = mComponentManager.getLocalComponent(i->first);
            if (comp != nullptr)
            {
                comp->setInstanceDraining(mInstanceId, true);
            }
        }

        //Let everyone else know we're draining
        ControllerStateChangeNotification notif;
        notif.setInstanceId(mInstanceId);
        notif.setIsDraining(true);
        sendNotifyControllerStateChangeToRemoteSlaves(&notif);

        mInstanceInfoController.onControllerDrain();

        //let anyone who takes immediate action on draining know we're draining
        mDrainDispatcher.dispatch(&DrainListener::onControllerDrain);
    }

    //kick off timer to check drain status and shutdown when drain is complete
    gSelector->scheduleTimerCall(TimeValue::getTimeOfDay(), this, &Controller::checkDrainStatus, restartAfterShutdown, "Controller::checkDrainStatus");
}

void Controller::onSlaveSessionAdded(SlaveSession& session)
{
    if (mLastBroadcastLoad == 0)
        return;

    // Immediately notify connecting instance of our load so it can make good decisions as soon as possible
    InstanceLoadChangeNotification notif;
    notif.setInstanceId(mInstanceId);
    notif.setTotalLoad(mLastBroadcastLoad);
    notif.setSystemLoad(mLastBroadcastSystemLoad);
    sendNotifyLoadChangeToSlaveSession(&session, &notif);
}

void Controller::onSlaveSessionRemoved(SlaveSession& session)
{
    // If this is the last session to be removed
    if ((mSlaveList.size() == 1) && mDelayedDrainEventHandle.isValid())
    {
        Fiber::signal(mDelayedDrainEventHandle, ERR_OK);
    }
}

/*! ************************************************************************************************/
/*! \brief Shutdown this particular server instance

    \param[in] request
    \return void
***************************************************************************************************/
void Controller::executeShutdown(bool restartAfterShutdown, ShutdownReason reason)
{
    if (!mDrainComplete)
    {
        if ((gSliverManager != nullptr) && gSliverManager->getCoordinator().isDesignatedCoordinator())
        {
            // HACK: In case of a full cluster shutdown, this delay is to give a few of the instances a chance to request
            //       sliver ejection from the SliverCoordinator, which will get things going.  Then the DrainStatusCheckHandler will
            //       prevent this instance from shutting down until all sliver migration related work has finished.
            if (!mSlaveList.empty())
            {
                mDelayedDrainEventHandle = Fiber::getNextEventHandle();
                Fiber::sleep(10 * 1000 * 1000, "Controller::executeShutdown", &mDelayedDrainEventHandle);
                mDelayedDrainEventHandle = Fiber::INVALID_EVENT_ID;
                Fiber::yield();
            }
        }

        if (!restartAfterShutdown)
        {
            gProcessController->signalMonitorNoRestart();
        }
    
        // kick off graceful drain, then then shutdown
        executeDrainShutdown(restartAfterShutdown);
    }
    else
    {
        shutdownControllerInternal(reason);
    }
}

void Controller::registerDrainStatusCheckHandler(const char8_t* name, DrainStatusCheckHandler& handler)
{
    if(mDrainStatusCheckHandlers.find(name) != mDrainStatusCheckHandlers.end())
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].registerDrainStatusCheckHandler: Handler(" << name << ") already registered.");
        return;
    }
    mDrainStatusCheckHandlers[name] = &handler;

    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].registerDrainStatusCheckHandler: Handler(" << name << ") registered.");
}

void Controller::deregisterDrainStatusCheckHandler(const char8_t* name)
{
    DrainStatusCheckHandlersByName::iterator it = mDrainStatusCheckHandlers.find(name);
    if(it == mDrainStatusCheckHandlers.end())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].deregisterDrainStatusCheckHandler: Could not find handler(" << name << ") in registration list.");
        return;
    }
    mDrainStatusCheckHandlers.erase(it);

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].deregisterDrainStatusCheckHandler: Handler(" << name << ") de-registered.");
}

void Controller::checkDrainStatus(bool restartAfterShutdown)
{
    if (mDrainTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDrainTimerId);
        mDrainTimerId = INVALID_TIMER_ID;
    }

    mDrainComplete = true;

    // mDrainComplete will stay at true when all the DrainStatusCheckHandlers-Subscribed instances are fully drained
    StringBuilder sb;
    DrainStatusCheckHandlersByName::const_iterator itr = mDrainStatusCheckHandlers.begin();
    DrainStatusCheckHandlersByName::const_iterator end = mDrainStatusCheckHandlers.end();
    for (; itr != end; ++itr)
    {
        // create a list of all the instances that drain-incomplete, and also set drainComplete=false if exists
        if (!itr->second->isDrainComplete())
        {
            sb << ((sb.length() == 0) ? "" : ", ") << itr->first.c_str();

            mDrainComplete = false;
        }
    }
    
    // Determine whether we should go ahead and call shutdownController or schedule the next checkDrainStatus()
    if (!mDrainComplete)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].checkDrainStatus: Handlers(" << sb.get() << ") are not yet drain complete.");

        // TODO_MC: This condition makes no sense, we should *always* schedule the next check.
        //          The todo here is: find out why this was ever needed.
        TimeValue checkInterval = mFrameworkConfigTdf.getDrainStatusCheckInterval();
        if (checkInterval > 0)
        {
            // Align the interval such that they fire across all blaze instances at the same time to ensure
            // a consistent view of the system.
            TimeValue now = TimeValue::getTimeOfDay();
            TimeValue nextTime = (((now + checkInterval) / checkInterval) * checkInterval);

            mDrainTimerId = mSelector.scheduleTimerCall(nextTime, this, &Controller::checkDrainStatus, restartAfterShutdown, "Controller::checkDrainStatus");
        }
    }
    else
    {
        mDrainTimerId = INVALID_TIMER_ID;
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].checkDrainStatus: Triggering shutdown on the controller.");

        mSelector.scheduleFiberCall(this, &Controller::shutdownControllerInternal, (restartAfterShutdown ? SHUTDOWN_REBOOT : SHUTDOWN_NORMAL), "Controller::shutdownControllerInternal");
    }
}

void Controller::onNotifyControllerStateChange(const ControllerStateChangeNotification& data, UserSession*)
{
    //Get the remote instance and set it to draining
    RemoteInstanceByIdMap::iterator itr = mRemoteInstancesById.find(data.getInstanceId());
    if (itr != mRemoteInstancesById.end())
    {
        itr->setDraining(data.getIsDraining());
        mRemoteInstanceDispatcher.dispatch<const RemoteServerInstance&>(&RemoteServerInstanceListener::onRemoteInstanceDrainingStateChanged, *itr);
    }
}

void Controller::onNotifyLoadChange(const InstanceLoadChangeNotification& data, UserSession*)
{
    auto itr = mRemoteInstancesById.find(data.getInstanceId());
    if (itr != mRemoteInstancesById.end())
    {
        itr->setLoad(data.getTotalLoad(), data.getSystemLoad());
    }
}

BlazeRpcError Controller::enableAuditLogging(const UpdateAuditLoggingRequest &request, bool unrestricted)
{
    return Logger::addAudit(request, unrestricted);
}

BlazeRpcError Controller::disableAuditLogging(const UpdateAuditLoggingRequest &request)
{
    return Logger::removeAudit(request);
}

void Controller::onNotifyUpdateAuditState(UserSession*)
{
    mUpdateAuditStateQueue.queueFiberJob(this, &Controller::fetchAndUpdateAuditState, "Controller::onNotifyUpdateAuditState");
}

void Controller::fetchAndUpdateAuditState()
{
    Logger::updateAudits();
}

void Controller::deleteExpiredAuditEntries()
{
    mAuditEntryExpiryTimerId = INVALID_TIMER_ID;

    Logger::expireAudits(mLoggingConfigTdf.getAuditEntryExpiryDuration());

    scheduleExpireAuditEntries(TimeValue::getTimeOfDay() + mLoggingConfigTdf.getAuditEntryExpiryCheckInterval());
}

void Controller::scheduleExpireAuditEntries(const TimeValue& deadline)
{
    if (mAuditEntryExpiryTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mAuditEntryExpiryTimerId);
        mAuditEntryExpiryTimerId = INVALID_TIMER_ID;
    }

    if (!mShuttingDown)
    {
        mAuditEntryExpiryTimerId = mSelector.scheduleFiberTimerCall(deadline, this, &Controller::deleteExpiredAuditEntries, "Controller::deleteExpiredAuditEntries");
    }
}

SetServiceStateError::Error Controller::processSetServiceState(const SetServiceStateRequest& request, const Message* message)
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processSetServiceState: Attempted-" 
        << UserSessionManager::formatAdminActionContext(message) << ", req = \n" << request);

    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].processSetServiceState: " << UserSessionManager::formatAdminActionContext(message) 
            << ". Attempted to set service state, no permission!");
        return SetServiceStateError::ERR_AUTHORIZATION_REQUIRED;
    }

    // Adjust the service state for all instance running in this process.  This is done because
    // taking an instance out of service is used to drain users from it so it can be shut down.
    // There isn't any point in taking only a single slave out of service if there are other
    // slaves running in the same process.  By forcing all instances in the process out of
    // service it should eliminate user error cases where a slave is taken down with one or
    // more instances in the process still servicing clients.
    gProcessController->setServiceState(request.getInService());
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processSetServiceState: Succeeded-" << UserSessionManager::formatAdminActionContext(message) << ". Set service state, had permission.");
    return SetServiceStateError::ERR_OK;
}


DumpCoreFileError::Error Controller::processDumpCoreFile(const Blaze::DumpCoreFileRequest& request, const Message* message)
{
    gProcessController->coreDump("rpc", request.getMaxFileSize());

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processDumpCoreFile: " << UserSessionManager::formatAdminActionContext(message) 
        << ". Dumped core file,  req = \n" << request);    
    return DumpCoreFileError::ERR_OK;
}

GetFiberTimingsError::Error Controller::processGetFiberTimings(Blaze::FiberTimings &response, const Message* message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getFiberTimings", message);

    if (gFiberManager == nullptr)
    {
        return GetFiberTimingsError::ERR_SYSTEM;
    }
    SelectorStatus selectorStatus;
    mSelector.getStatusInfo(selectorStatus);
    response.setTotalCpuTime(selectorStatus.getJobQueueTime() + selectorStatus.getTimerQueueTime() + selectorStatus.getNetworkTime() + selectorStatus.getNetworkPriorityTime());
    gFiberManager->filloutFiberTimings(response);

    return GetFiberTimingsError::ERR_OK;
}

GetDbQueryMetricsError::Error Controller::processGetDbQueryMetrics(Blaze::DbQueryMetrics &response, const Message* message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getDbQueryMetrics", message);

    if ((gFiberManager == nullptr) || (gDbScheduler == nullptr))
    {
        return GetDbQueryMetricsError::ERR_SYSTEM;
    }

    gDbScheduler->buildDbQueryAccountingMap(response.getDbQueryAccountingInfos());

    return GetDbQueryMetricsError::ERR_OK;
}

ShutdownError::Error Controller::processShutdown(const Blaze::ShutdownRequest &request, const Message* message)
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processShutdown: " << UserSessionManager::formatAdminActionContext(message) << ". Initiated shutdown:\n" << request);

    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].processShutdown: " << UserSessionManager::formatAdminActionContext(message) << ". Attempted to shutdown, no permission!");
        return ShutdownError::ERR_AUTHORIZATION_REQUIRED;
    }

    Blaze::ShutdownRequestPtr adjustedShutdownReq = BLAZE_NEW ShutdownRequest();
    request.copyInto(*adjustedShutdownReq);

    if (request.getReason() == SHUTDOWN_REBOOT)   // ShutdownReason is being deprecated: currently using it to override mRestart if SHUTDOWN_REBOOT
    {
        adjustedShutdownReq->setRestart(true);
    }
    
    // Now after shutdown request is adjusted, we can go ahead

    switch (request.getTarget())
    {
        case SHUTDOWN_TARGET_LOCAL:
            executeShutdown(adjustedShutdownReq->getRestart(), adjustedShutdownReq->getReason());
            break;
        case SHUTDOWN_TARGET_ALL:
        case SHUTDOWN_TARGET_LIST:
            // everyone else first ...
            sendShutdownBroadcastedToRemoteSlaves(adjustedShutdownReq.get());
            // then myself ...
            onShutdownBroadcasted(*adjustedShutdownReq, nullptr);
            break;
        default:
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].processShutdown: unknown target set (" << adjustedShutdownReq->getTarget() << ")");
            break;
        }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].processShutdown: " << UserSessionManager::formatAdminActionContext(message) 
        << ". Shutdown the server, had permission.");
    return ShutdownError::ERR_OK;
}

GetOutboundMetricsError::Error Controller::processGetOutboundMetrics(Blaze::OutboundMetrics &response, const Message* message)
{
    tickDeprecatedRPCMetric("blazecontroller", "getOutboundMetrics", message);

    if (gOutboundMetricsManager == nullptr)
    {
        return GetOutboundMetricsError::ERR_SYSTEM;
    }

    gOutboundMetricsManager->filloutOutboundMetrics(response);
    
    return GetOutboundMetricsError::ERR_OK;
}

GetDrainStatusError::Error Controller::processGetDrainStatus(Blaze::DrainStatus &response, const Message* message)
{
    response.setIsDraining(isDraining());
    response.setInService(isInService());
    return GetDrainStatusError::ERR_OK;
}

void Controller::clearFiberTimingData()
{
    gFiberManager->clearFiberAccounting();
}

void Controller::checkCpuBudget()
{
    if (isShuttingDown())
    {
        // if we are in the process of shutting down, bail
        return;
    }

    TimeValue checkInterval = gFiberManager->checkCpuBudget();
    if (checkInterval <= 0)
    {
        EA_FAIL_MSG("Cpu budget interval must be greater than 0!");
        return;
    }

    auto newCpuUsage = gFiberManager->getCpuUsageForProcessPercent();
    auto cpuAbsoluteDelta = (newCpuUsage > mLastBroadcastLoad) ? (newCpuUsage - mLastBroadcastLoad) : (mLastBroadcastLoad - newCpuUsage);
    if (cpuAbsoluteDelta >= 1)
    {
        mLastBroadcastLoad = newCpuUsage;
        mLastBroadcastSystemLoad = gFiberManager->getSystemCpuUsageForProcessPercent();
        InstanceLoadChangeNotification notif;
        notif.setInstanceId(mInstanceId);
        notif.setTotalLoad(mLastBroadcastLoad);
        notif.setSystemLoad(mLastBroadcastSystemLoad);
        sendNotifyLoadChangeToAllSlaves(&notif);
    }

    // IMPORTANT: Align the interval such that they fire across all blaze instances at the same time to ensure a consistent view of the system.
    TimeValue now = TimeValue::getTimeOfDay();
    TimeValue nextBudgetCheckTime = ((now + checkInterval) / checkInterval) * checkInterval;
    mSelector.scheduleTimerCall(nextBudgetCheckTime, this, &Controller::checkCpuBudget, "Controller::checkCpuBudget");
}

void Controller::setServiceState(bool inService)
{
    if (mInService == inService)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].setServiceState: "
            "Instance is already " << (inService ? "in-service" : "out-of-service"));
        return;
    }

    if (mIsReadyForService)
    {
        mInService = inService;
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].setServiceState: "
            "Instance changed to " << (inService ? "in-service" : "out-of-service"));
    }
    else
    {
        mIsStartInService = inService;
        BLAZE_INFO_LOG(Log::CONTROLLER, "[Controller].setServiceState: "
            "Instance is connecting to cluster, will start " << (inService ? "in-service" : "out-of-service"));
    }

    mInstanceInfoController.onControllerServiceStateChange();
}

bool Controller::isDBDestructiveAllowed() const
{
    return gProcessController->allowDestructiveDbActions(); 
}

const Endpoint* Controller::getInternalEndpoint() const
{
    return mConnectionManager.findEndpoint(mServerConfigTdf.getInternalEndpoint()); 
}

Controller::ComponentStage Controller::getStage(ComponentId componentId)
{
    ComponentStage stage;
    if (RpcIsFrameworkComponent(componentId))
    {
        stage = COMPONENT_STAGE_FRAMEWORK;
    }
    else if (RpcIsMasterComponent(componentId))
        stage = COMPONENT_STAGE_MASTER;
    else
        stage = COMPONENT_STAGE_SLAVE;
    return stage;
}

const char8_t *Controller::getStageName(ComponentStage stage)
{
    static const char *sComponentStageNames[] = 
    {
        "FRAMEWORK",
        "MASTER",
        "SLAVE"
    };
    static_assert(COMPONENT_STAGE_MAX == EAArrayCount(sComponentStageNames), "If you see this compile error, make sure ComponentStage and sComponentStageNames are in sync.");
    return sComponentStageNames[stage];
}

const char8_t* Controller::getNamespaceFromPlatform(ClientPlatformType platform) const
{
    switch (platform)
    {
    case xone:
        return NAMESPACE_XBOXONE;
    case xbsx:
        return NAMESPACE_XBSX;
    case ps4:
        return NAMESPACE_PS4;
    case ps5:
        return NAMESPACE_PS5;
    case nx: // Switch persona names are not unique, so we use the Origin namespace instead
    case pc:        
        return NAMESPACE_ORIGIN;
    case steam:
        return NAMESPACE_STEAM; 
    case stadia:
        return NAMESPACE_STADIA; 
    
    default:
        return "";
    }
}

const Blaze::Controller::NamespacesList& Controller::getAllSupportedNamespaces() const
{
    return mSupportedNamespaceList;
}

const char8_t* Controller::validateServiceName(const char8_t* requestedServiceName) const
{
    if (requestedServiceName != nullptr && requestedServiceName[0] != '\0')
    {
        if (isServiceHosted(requestedServiceName))
        {
            return requestedServiceName;
        }
    }

    return getDefaultServiceName();
}


const char8_t* Controller::verifyServiceName(const char8_t* serviceName) const
{
    if (isSharedCluster())
    {
        if (serviceName == nullptr || serviceName[0] == '\0')
        {
            return nullptr;
        }

        if (!gController->isServiceHosted(serviceName))
        {
            return nullptr;
        }
    }
    else
    {
        serviceName = gController->validateServiceName(serviceName);
    }

    return serviceName;
}

bool Controller::isServiceHosted(const char8_t* serviceName) const
{
    return (getHostedServicesInfo().find(serviceName) != getHostedServicesInfo().end());
}

bool Controller::isInstanceTypeConfigMaster() const
{
    return mServerConfigTdf.getInstanceType() == ServerConfig::CONFIG_MASTER;
}

bool Controller::isInstanceTypeMaster() const
{
    return ((mServerConfigTdf.getInstanceType() == ServerConfig::CONFIG_MASTER) ||
            (mServerConfigTdf.getInstanceType() == ServerConfig::AUX_MASTER));
}

void Controller::syncRemoteInstanceListener(RemoteServerInstanceListener& listener) const
{
    eastl::hash_set<InstanceId> ids;
    for (RemoteInstanceByIdMap::const_iterator i = mRemoteInstancesById.begin(), e = mRemoteInstancesById.end(); i != e; ++i)
    {
        const RemoteServerInstance& remoteInstance = *i;
        if ((ids.find(remoteInstance.getInstanceId()) == ids.end()) && remoteInstance.isActive())
        {
            ids.insert(remoteInstance.getInstanceId());
            listener.onRemoteInstanceChanged(remoteInstance);

            // Map might mutate during call to onRemoteInstanceChanged, restart iteration for safety
            i = mRemoteInstancesById.begin();
        }
    }
}

const RemoteServerInstance* Controller::getRemoteServerInstance(InstanceId instanceId) const
{
    RemoteInstanceByIdMap::const_iterator itr = mRemoteInstancesById.find(instanceId);
    return (itr != mRemoteInstancesById.end()) ? &*itr : nullptr;
}

Blaze::BlazeRpcError Controller::waitUntilReadyForService()
{
    if (mIsReadyForService)
        return ERR_OK;
    if (mShuttingDown)
        return ERR_CANCELED;
    return mReadyForServiceWaiters.wait("Controller::waitUntilReadyForService");
}

BlazeRpcError Controller::waitForReconfigureValidation(ConfigureValidationFailureMap& validationErrorMap)
{
    if (mReconfigureState.mSemaphoreId != 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].waitForReconfigureValidation: There is an active reconfiguration semaphore for this controller.  This should not happen.");
        return ERR_SYSTEM;
    }

    mReconfigureState.mSemaphoreId = gFiberManager->getSemaphore("Controller::waitForReconfigureValidation");
    BlazeRpcError err = gFiberManager->waitSemaphore(mReconfigureState.mSemaphoreId);
    mReconfigureState.mSemaphoreId = 0;
    if (err == CTRL_ERROR_RECONFIGURE_VALIDATION_FAILED)
    {
        mReconfigureState.mValidationErrors.copyInto(validationErrorMap);
        mReconfigureState.mValidationErrors.release();
    }
    return err;
}

void Controller::setConfigureValidationErrorsForFeature(const char8_t *featureName, ConfigureValidationErrors& validationErrorMap)
{
    ConfigureValidationFailureMap::iterator it = mReconfigureState.mValidationErrors.find(featureName);
    if (it == mReconfigureState.mValidationErrors.end())
    {
        ConfigureValidationErrors* errors = mReconfigureState.mValidationErrors.allocate_element();
        validationErrorMap.copyInto(*errors);
        mReconfigureState.mValidationErrors[featureName] = errors;
    }
    else
    {
        validationErrorMap.copyInto(*it->second);
    }
}

void Controller::checkClientTypeBindings()
{
    eastl::set<ClientType> partitionedClientTypes;
    partitionedClientTypes.insert(CLIENT_TYPE_GAMEPLAY_USER);
    partitionedClientTypes.insert(CLIENT_TYPE_LIMITED_GAMEPLAY_USER);
    partitionedClientTypes.insert(CLIENT_TYPE_DEDICATED_SERVER);
    partitionedClientTypes.insert(CLIENT_TYPE_HTTP_USER);
    partitionedClientTypes.insert(CLIENT_TYPE_TOOLS);
    BootConfig::ServerConfigsMap::const_iterator itr = mBootConfigTdf.getServerConfigs().begin();
    BootConfig::ServerConfigsMap::const_iterator end = mBootConfigTdf.getServerConfigs().end();
    for ( ; itr != end; ++itr)
    {
        if (((*itr).second->getInstanceType() == ServerConfig::SLAVE) || 
            ((*itr).second->getInstanceType() == ServerConfig::AUX_SLAVE))
        {
            if (!itr->second->getClientTypes().empty())
            {
                ClientTypeList::const_iterator ctlItr = itr->second->getClientTypes().begin();
                ClientTypeList::const_iterator ctlEnd = itr->second->getClientTypes().end();
                for ( ; ctlItr != ctlEnd; ++ctlItr)
                    partitionedClientTypes.erase(*ctlItr);

                if (partitionedClientTypes.empty())
                    break;
            }
            else
            {
                partitionedClientTypes.clear();
                break;
            }
        }
    }

    if (!partitionedClientTypes.empty())
    {
        eastl::set<ClientType>::const_iterator ctlItr = partitionedClientTypes.begin();
        eastl::set<ClientType>::const_iterator ctlEnd = partitionedClientTypes.end();
        for ( ; ctlItr != ctlEnd; ++ctlItr)
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].checkClientTypeBindings: The ClientType '" << ClientTypeToString(*ctlItr) << "' is not being handled by any slave.");
        }
    }

}

void Controller::transactionContextCleanSweep()
{
    TimeValue defaultTimeout = mFrameworkConfigTdf.getTransactionConfig().getDefaultTransactionTimeout();
    TimeValue cleanSweepInterval = mFrameworkConfigTdf.getTransactionConfig().getCleanSweepInterval();

    if (EA_LIKELY(!isShuttingDown()))
    {
        for (ComponentManager::ComponentStubMap::iterator it = mComponentManager.getLocalComponents().begin(),
             endIt = mComponentManager.getLocalComponents().end();
             it != endIt;
             it++)
        {
            BlazeRpcError err = it->second->asComponent().expireTransactionContexts(defaultTimeout);

            if (EA_UNLIKELY(err == Blaze::ERR_CANCELED || isShuttingDown()))
                return;
        }

        mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + cleanSweepInterval, this, 
            &Controller::transactionContextCleanSweep, "Controller::transactionContextCleanSweep");
    }
}

void Controller::logAndExportMetrics()
{
    mMetricsTimerId = INVALID_TIMER_ID;

    MetricsLoggingConfig& config = mFrameworkConfigTdf.getMetricsLoggingConfig();

    bool doLog = false;
    bool doExport = false;

    TimeValue now = TimeValue::getTimeOfDay();
    if ((mNextMetricsLogTime > 0) && (mNextMetricsLogTime <= now))
        doLog = true;
    if ((mNextMetricsExportTime > 0) && (mNextMetricsExportTime <= now))
        doExport = true;

    if (doLog || doExport)
    {
        MetricsExporter exporter(getDefaultServiceName(), getInstanceName(), gProcessController->getVersion().getBuildLocation(), mFrameworkConfigTdf.getMetricsLoggingConfig().getMetricsExport());

        ComponentMetricsResponse componentMetrics;
        if (config.getComponentMetrics() || exporter.isComponentMetricsEnabled())
        {
            ComponentMetricsRequest metricsRequest;
            processGetComponentMetrics(metricsRequest, componentMetrics, nullptr);
        }

        FiberTimings fiberTimings;
        if (config.getFiberTimings() || exporter.isFiberTimingsEnabled())
        {
            processGetFiberTimings(fiberTimings, nullptr);
        }

        ServerStatus status;
        if (config.getStatus() || exporter.isStatusEnabled())
        {
            Controller::processGetStatus(status, nullptr);
        }

        DbQueryMetrics dbMetrics;
        if (config.getDbMetrics() || exporter.isDbMetricsEnabled())
        {
            processGetDbQueryMetrics(dbMetrics, nullptr);
        }

        OutboundMetrics outboundMetrics;
        if (config.getOutboundMetrics() || exporter.isOutboundMetricsEnabled())
        {
            processGetOutboundMetrics(outboundMetrics, nullptr);
        }

        GetStorageMetricsResponse storageManagerMetrics;
        if (config.getStorageManagerMetrics() || exporter.isStorageManagerMetricsEnabled())
        {
            gStorageManager->processGetStorageMetrics(storageManagerMetrics, nullptr);
        }

        if (doExport)
            exporter.beginExport();

        if (doLog && config.getComponentMetrics())
            gEventManager->submitEvent(EVENT_COMPONENTMETRICSEVENT, componentMetrics, true);
        if (doExport && exporter.isComponentMetricsEnabled())
            exporter.exportComponentMetrics(now, componentMetrics);

        if (doLog && config.getFiberTimings())
            gEventManager->submitEvent(EVENT_FIBERTIMINGSEVENT, fiberTimings, true);
        if (doExport && exporter.isFiberTimingsEnabled())
            exporter.exportFiberTimings(now, fiberTimings);

        if (doLog && config.getStatus())
            gEventManager->submitEvent(EVENT_STATUSEVENT, status, true);
        if (doExport && exporter.isStatusEnabled())
            exporter.exportStatus(now, status);

        if (doLog && config.getDbMetrics())
            gEventManager->submitEvent(EVENT_DBQUERYMETRICSEVENT, dbMetrics, true);
        if (doExport && exporter.isDbMetricsEnabled())
            exporter.exportDbMetrics(now, dbMetrics);

        if (doLog && config.getOutboundMetrics())
            gEventManager->submitEvent(EVENT_OUTBOUNDMETRICSEVENT, outboundMetrics, true);
        if (doExport && exporter.isOutboundMetricsEnabled())
            exporter.exportOutboundMetrics(now, outboundMetrics);

        if (doLog && config.getStorageManagerMetrics())
            gEventManager->submitEvent(EVENT_STORAGEMANAGERMETRICSEVENT, storageManagerMetrics, false);
        if (doExport && exporter.isStorageManagerMetricsEnabled())
            exporter.exportStorageManagerMetrics(now, storageManagerMetrics);

        if (doExport)
            exporter.endExport();
    }

    scheduleMetricsTimer();
}

void Controller::scheduleMetricsTimer()
{
    if (mMetricsTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mMetricsTimerId);
        mMetricsTimerId = INVALID_TIMER_ID;
    }

    TimeValue now = TimeValue::getTimeOfDay();

    TimeValue logInterval = mFrameworkConfigTdf.getMetricsLoggingConfig().getLoggingInterval();
    if (logInterval <= 0)
    {
        mNextMetricsLogTime = 0;
    }
    else
    {
        // Align the interval such that they fire across all blaze instances at the same time to ensure
        // a consistent view of the system.
        mNextMetricsLogTime = ((now + logInterval) / logInterval) * logInterval;
    }

    TimeValue exportInterval = mFrameworkConfigTdf.getMetricsLoggingConfig().getMetricsExport().getExportInterval();
    if (exportInterval <= 0)
    {
        mNextMetricsExportTime = 0;
    }
    else
    {
        // Align the interval such that they fire across all blaze instances at the same time to ensure
        // a consistent view of the system.
        mNextMetricsExportTime = ((now + exportInterval) / exportInterval) * exportInterval;
    }

    TimeValue nextTime = mNextMetricsLogTime;
    if (nextTime == 0 || ((mNextMetricsExportTime > 0) && (mNextMetricsExportTime < nextTime)))
        nextTime = mNextMetricsExportTime;
    if (nextTime > 0)
    {
        mMetricsTimerId = mSelector.scheduleFiberTimerCall(nextTime, this, &Controller::logAndExportMetrics, "Controller::logAndExportMetrics");
    }
}

void Controller::scheduleOIMetricsTimer()
{
    if (mOIMetricsTimerId != INVALID_TIMER_ID)
        mSelector.cancelTimer(mOIMetricsTimerId);

    mOIMetricsTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (EXPORT_OI_METRICS_INTERVAL * 1000),
        this, &Controller::exportOIMetrics, "Controller::exportOIMetrics");
}

void Controller::exportOIMetrics()
{
    mMetricsTimerId = INVALID_TIMER_ID;

    Metrics::gMetricsCollection->exportMetrics();

    scheduleOIMetricsTimer();
}

void Controller::scheduleCertRefreshTimer()
{
    if (mCertRefreshTimerId != INVALID_TIMER_ID)
        mSelector.cancelTimer(mCertRefreshTimerId);

    TimeValue delay = mFrameworkConfigTdf.getCertRefreshInterval();

    // Ensure we don't hammer ESS at an excessive rate
    if (delay.getSec() < 60)
        delay.setSeconds(60);

    // Randomize the time we hit ESS so that all Blaze instances are not polling it on the same cadence
    delay.setSeconds(static_cast<int64_t>(gRandom->getRandomFloat(0.5f, 1.5f) * delay.getSec()));

    mCertRefreshTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + delay, this, &Controller::certRefresh, "Controller::certRefresh");
}

void Controller::certRefresh()
{
    mCertRefreshTimerId = INVALID_TIMER_ID;

    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].certRefresh: Starting refresh of certificates from ESS");

    gSslContextManager->refreshCerts();
    gOutboundHttpService->refreshCerts();
    mOutboundGrpcManager->refreshCerts();

    BLAZE_TRACE_LOG(Log::CONTROLLER, "[Controller].certRefresh: Done refresh of certificates from ESS");

    scheduleCertRefreshTimer();
}

bool Controller::validateTrialNameRemaps(const RedirectorSettingsConfig &config, ConfigureValidationErrors &validationErrors) const
{
    for (const auto& mapEntry : config.getTrialServiceNameRemap())
    {
        if (isServiceHosted(mapEntry.first) && !isServiceHosted(mapEntry.second))
        {
            eastl::string msg;
            msg.sprintf("[Controller].validateTrialNameRemaps: Specified trial name remap (%s -> %s) references non-existent trial servicename.", mapEntry.first.c_str(), mapEntry.second.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    return true;
} 

GetCensusDataProvidersError::Error Controller::processGetCensusDataProviders(GetCensusDataProvidersResponse &response, const Message *message)
{
    gCensusDataManager->getCensusDataProviderComponentIds(response.getComponentIds());
    return GetCensusDataProvidersError::ERR_OK;
}

uint64_t Controller::getMachineInstanceCount() const
{
    int64_t count = 1; // This instance is included in the count.
    uint32_t localIp = NameResolver::getInternalAddress().getIp(InetAddress::NET);

    RemoteInstanceByAddrMap::const_iterator itr = mRemoteInstancesByAddr.begin();
    RemoteInstanceByAddrMap::const_iterator end = mRemoteInstancesByAddr.end();
    for(; itr != end; ++itr)
    {
        if (itr->first.getIp() == localIp)
            ++count;
    }
    return count;
}

bool Controller::isSharedCluster() const
{
    return gProcessController->isSharedCluster();
}

const char8_t* Controller::getDefaultServiceName() const
{
    if (isSharedCluster())
    {
        auto iter = mBootConfigTdf.getPlatformToDefaultServiceNameMap().find(common);
        if (iter != mBootConfigTdf.getPlatformToDefaultServiceNameMap().end())
            return iter->second.c_str();
    }
    else
    {
        auto iter = mBootConfigTdf.getPlatformToDefaultServiceNameMap().find(*(mHostedPlatforms.begin()));
        if (iter != mBootConfigTdf.getPlatformToDefaultServiceNameMap().end())
            return iter->second.c_str();
    }

    BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].getDefaultServiceName: No default service name set. This should be a boot failure.");
    return nullptr;
}

const char8_t* Controller::getDefaultServiceName(ClientPlatformType platform) const
{
    if (mHostedPlatforms.find(platform) == mHostedPlatforms.end()
        && platform != common)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].getDefaultServiceName: No default ServiceName info exists for platform (" << ClientPlatformTypeToString(platform) << ").");
        return nullptr;
    }

    auto iter = mBootConfigTdf.getPlatformToDefaultServiceNameMap().find(platform);
    if (iter != mBootConfigTdf.getPlatformToDefaultServiceNameMap().end())
        return iter->second.c_str();

    BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].getDefaultServiceName: No default ServiceName info exists for platform (" << ClientPlatformTypeToString(platform) << ").");
    return nullptr;
}

ClientPlatformType Controller::getDefaultClientPlatform() const
{
    const ServiceNameInfo* info = getServiceNameInfo(getDefaultServiceName());
    if (info != nullptr)
        return info->getPlatform();

    return INVALID;
}

const char8_t* Controller::getDefaultProductName() const
{
    const ServiceNameInfo* info = getServiceNameInfo(getDefaultServiceName());
    if (info != nullptr)
        return info->getProductName();

    return "";
}


const ServiceNameInfo* Controller::getDefaultServiceNameInfo(ClientPlatformType platform) const
{
    const ServiceNameInfo* info = getServiceNameInfo(getDefaultServiceName(platform));
    if (info != nullptr)
        return info;

    return nullptr;
}

const char8_t* Controller::getDefaultProductNameFromPlatform(ClientPlatformType platform) const
{
    const ServiceNameInfo* serviceNameInfo = getDefaultServiceNameInfo(platform);
    if (serviceNameInfo)
        return serviceNameInfo->getProductName();

    return "";
}

GetMetricsError::Error Controller::processGetMetrics(const GetMetricsRequest &request, GetMetricsResponse &response, const Message* message)
{
    if (!Metrics::gMetricsCollection->getMetrics(request, response))
        return GetMetricsError::CTRL_ERROR_INVALID_FILTER;
    return GetMetricsError::ERR_OK;
}

GetMetricsSchemaError::Error Controller::processGetMetricsSchema(Blaze::GetMetricsSchemaResponse &response, const Message* message)
{
    Metrics::gMetricsCollection->getMetricsSchema(response);
    return GetMetricsSchemaError::ERR_OK;
}

void Controller::tickDeprecatedRPCMetric(const char8_t* component, const char8_t* command, const Message* message)
{
    if (message == nullptr)
        return;
    const PeerInfo& peerInfo = message->getPeerInfo();
    mDeprecatedRPCs.increment(1, component, command, peerInfo.getClientType(), peerInfo.getEndpointName(), peerInfo.getPeerAddress());
}

bool Controller::validateConfiguredMockServices(bool mockServiceEnabled)
{
    if (mockServiceEnabled == false)
        return true;

    if (((blaze_stricmp(getServiceEnvironment(), "dev") != 0) && (blaze_stricmp(getServiceEnvironment(), "test") != 0)))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Controller].validateConfiguredMockServices: Mock platform testing is not supported for current service environment(" << getServiceEnvironment() << "). Mock services cannot be enabled.");
        return false;
    }
    return true;
}

void Controller::validateServiceNameStandard(ConfigureValidationErrors& validationErrors) const
{
    ClientPlatformSet inUsePlatformSet;
    for (const auto& hostedPlatform : mBootConfigTdf.getHostedPlatforms())
        inUsePlatformSet.insert(hostedPlatform);
    if (isSharedCluster())
        inUsePlatformSet.insert(common);

    for (const auto& serviceNamePair : mBootConfigTdf.getServiceNames())
    {
        const ClientPlatformType platformType = serviceNamePair.second->getPlatform();
        if (inUsePlatformSet.find(platformType) == inUsePlatformSet.end())
            continue;

        const eastl::string serviceName(serviceNamePair.first);
        eastl::vector<eastl::string> serviceNameParts;
        blaze_stltokenize(serviceName, "-", serviceNameParts, true);  //Split service name by '-' with empty substrings allowed

        eastl::vector<eastl::string>::const_iterator iter = serviceNameParts.begin();
        eastl::vector<eastl::string>::const_iterator iterEnd = serviceNameParts.end();
        const eastl::string& title = (iter == iterEnd ? "" : *iter++);
        const eastl::string& version = (iter == iterEnd ? "" : *iter++);
        const eastl::string& platform = (iter == iterEnd ? "" : *iter++);
        eastl::string message = "";

        if (serviceName.length() > MAX_SERVICENAME_LENGTH - 1)
        {
            eastl::string max_length = std::to_string(MAX_SERVICENAME_LENGTH - 1).c_str();
            eastl::string length = std::to_string(serviceName.length()).c_str();
            message += "Invalid length: " + length + " for Service Name, must not exceed " + max_length + " characters. ";
        }

        if (title.empty())
            message += "<title> is missing. ";
        else if (!std::regex_match(title.c_str(), std::regex("[a-z0-9]+")))
            message += "<title> \"" + title + "\" can contain only lower case letters a-z and digits 0-9. ";

        if (version.empty())
            message += "<version> is missing. ";
        else if (!std::regex_match(version.c_str(), std::regex("[a-z0-9_]+")))
            message += "<version> \"" + version + "\" can contain only lower case letters a-z, digits 0-9, and underscore _. ";

        if (platform.empty())
            message += "<platform> is missing. ";
        else if (blaze_strcmp(platform.c_str(), ClientPlatformTypeToString(platformType)) != 0)
            message += "<platform> \"" + platform + "\" does not match configured type \"" + ClientPlatformTypeToString(platformType) + "\" (case sensitive). ";

        eastl::string invalidSuffix = "";
        bool hasEmptySuffix = false;
        while (iter != iterEnd)
        {
            const eastl::string& suffix = *iter++;
            if (suffix.empty())
                hasEmptySuffix = true;
            else if (!std::regex_match(suffix.c_str(), std::regex("[a-z0-9_]+")))
                invalidSuffix += ", \"" + suffix + "\"";
        }

        if (!invalidSuffix.empty())
            message += "<suffix> " + invalidSuffix.substr(2) + " can contain only lower case letters a-z, digits 0-9, and underscore _. ";
        if (hasEmptySuffix)
            message += "Service Name cannot end with dash - or contain two consecutive dash --. ";

        if (!message.empty())
        {
            message = "[Controller].validateServiceNameStandard: Invalid Service Name \"" + serviceName + "\". " + message;
            message += "See Service Name Standard page for details, https://developer.ea.com/display/blaze/Service+Name+Standard";
            validationErrors.getErrorMessages().push_back().set(message.c_str());
        }
    }
}

} // Blaze

