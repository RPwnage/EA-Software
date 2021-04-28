/*************************************************************************************************/
/*!
    \file   inboundgrpcmanager.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "framework/grpc/inboundgrpcrequesthandler.h"

#include "framework/connection/selector.h"
#include "framework/connection/endpointhelper.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/component/componentmanager.h"
#include "framework/protocol/rpcprotocol.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <eathread/eathread_atomic.h>

namespace Blaze
{
namespace Grpc
{

EA::Thread::AtomicInt64 gGrpcRpcCounter = 0;

#if ENABLE_CLANG_SANITIZERS
static const int64_t PENDING_SHUTDOWN_GRACE_PERIOD_US = 45 * SECONDS_TO_MICROSECONDS; //45s
#else
static const int64_t PENDING_SHUTDOWN_GRACE_PERIOD_US = 15 * SECONDS_TO_MICROSECONDS; //15s
#endif


InboundGrpcManager::InboundGrpcManager(Blaze::Selector& selector)
    : mSelector(selector)
    , mShutdownTimeout(0)
    , mShuttingDown(false)
    , mInitialized(false)
    , mEndpointShutdownQueue("InboundGrpcManager::mEndpointShutdownQueue")
{
    mEndpointShutdownQueue.setMaximumWorkerFibers(FiberJobQueue::SMALL_WORKER_LIMIT);// This is set to 10 which is plenty for us.
}

InboundGrpcManager::~InboundGrpcManager()
{
    if (mGrpcEndpoints.size() != 0)
    {
        // Following can still some time occur due to oddities in Controller code. Changing to WARN from ASSERT so that we can capture full logs.
        // The simple clear of endpoints below should typically handle the issue occuring during start up. 
        BLAZE_WARN_LOG(Log::GRPC, "GrpcEndpoints still around, did you forget to call shutdown?");
    }
    mGrpcEndpoints.clear();
}

bool InboundGrpcManager::initialize(uint16_t basePort, const char8_t* endpointGroup, const EndpointConfig& endpointConfig, const RateConfig& rateConfig)
{
    if (mShuttingDown)
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcManager].initialize: Already shutdown or in the process. Ignoring initialize call.");
        return false; 
    }
    
    BLAZE_INFO_LOG(Log::GRPC, "[InboundGrpcManager].initialize: initializing grpc endpoints with base port(" << basePort << ") and endpointGroup (" << endpointGroup << ").");
    
    const auto endpointGroupEntry = endpointConfig.getEndpointGroups().find(endpointGroup);
    if (endpointGroupEntry != endpointConfig.getEndpointGroups().end())
    {
        const auto& grpcEndpointNames = endpointGroupEntry->second->getGrpcEndpoints();

        for (const auto& grpcEndpointName : grpcEndpointNames)
        {
            const auto grpcEndpointEntry = endpointConfig.getGrpcEndpointTypes().find(grpcEndpointName);
            if (grpcEndpointEntry != endpointConfig.getGrpcEndpointTypes().end())
            {
                uint16_t port = 0;
                if (gProcessController->isUsingAssignedPorts())
                {
                    BindType bindType = grpcEndpointEntry->second->getBind();
                    if (!gProcessController->getAssignedPort(bindType, port))
                    {
                        mGrpcEndpoints.clear(); //clean up the already created endpoints
                        BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcManager].initialize: No assigned ports available for endpoint ("
                            << grpcEndpointEntry->first << ") with bind type " << BindTypeToString(bindType) << ". Aborting self.");
                        return false;
                    }
                }
                else
                {
                    port = basePort++;
                }
                eastl::unique_ptr<GrpcEndpoint> endpoint(BLAZE_NEW GrpcEndpoint(grpcEndpointEntry->first, port, mSelector));
                
                if (!endpoint->initialize(*(grpcEndpointEntry->second), rateConfig, endpointConfig.getRpcControlLists(), endpointConfig.getPlatformSpecificRpcControlLists()))
                {
                    mGrpcEndpoints.clear(); //clean up the already created endpoints
                    BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcManager].initialize: Failed to initialize endpoint (" << grpcEndpointEntry->first << "). Aborting self.");
                    return false;
                }

                mGrpcEndpoints.push_back(eastl::move(endpoint));
            }
        }
    }
  
    mShutdownTimeout = endpointConfig.getShutdownTimeout();

    mInitialized = true;
    return true;
}

void InboundGrpcManager::shutdown()
{
    if (!mInitialized)
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcManager].shutdown: Never initialized. Ignoring shutdown call.");
        return;
    }

    if (mShuttingDown)
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcManager].shutdown: Already shutdown or in the process. Ignoring shutdown call.");
        return;
    }
    
    BLAZE_INFO_LOG(Log::GRPC, "[InboundGrpcManager].shutdown: attempting to gracefully close " << mGrpcEndpoints.size() << " grpc endpoints. Wait for (" << (PENDING_SHUTDOWN_GRACE_PERIOD_US/SECONDS_TO_MICROSECONDS)<<")s.");

    mShuttingDown = true;

    for (auto& endpoint : mGrpcEndpoints)
        mEndpointShutdownQueue.queueFiberJob<InboundGrpcManager, GrpcEndpoint*>(this, &InboundGrpcManager::shutdownEndpoint, endpoint.get(), "InboundGrpcManager::shutdownEndpoint");

    mEndpointShutdownQueue.join();
    mGrpcEndpoints.clear();
}

void InboundGrpcManager::shutdownEndpoint(GrpcEndpoint* endpoint)
{
   // Shutting down grpc endpoints cleanly is a tricky problem because of unwieldy apis provided by grpc core and our threading model for the async apis. What we do is

   // 1. Let the endpoints know that they should begin the shutdown process. This way, they can tell the core grpc server to stop accepting new work immediately. 
    endpoint->beginShutdown();

    // 2. Sleep for a grace time to allow for the currently pending work to be processed. Due to the multi-threading issues involved, there isn't a safe way to avoid this sleep
    // if no work is happening as the grpc work might be "in transit across thread boundaries".  
    Fiber::sleep(TimeValue(PENDING_SHUTDOWN_GRACE_PERIOD_US), "wait for pending tags in selector queue and grpc internal thread. Also, a grace period for rpcs in progress.");

    // 3. Force the final shutdown. Cancel any work still pending and join all the threads.  
    endpoint->shutdown();
}


void InboundGrpcManager::getStatusInfo(Blaze::GrpcManagerStatus& status) const
{
    uint64_t rpcDropped = 0, rpcPermitted = 0, connRejected = 0;
    for (auto& endpoint : mGrpcEndpoints)
    {
        endpoint->getStatusInfo(*(status.getGrpcEndpointsStatus().pull_back()));
        
        rpcDropped += endpoint->getTotalExceededRpcFiberLimitDropped();
        rpcPermitted += endpoint->getTotalExceededRpcFiberLimitPermitted();
        connRejected += endpoint->getCountTotalRejects();
    }

    status.setInboundTotalRpcFiberLimitExceededDropped(rpcDropped);
    status.setInboundTotalRpcFiberLimitExceededPermitted(rpcPermitted);
    status.setTotalRejectMaxInboundConnections(connRejected);

    status.setConnectionCount(getNumConnections());
}

int32_t InboundGrpcManager::getNumConnections() const
{
    int32_t numConnections = 0;
    
    for (auto& endpoint : mGrpcEndpoints)
    {
        numConnections += endpoint->getCurrentConnections();
    }

    return numConnections;
}

void InboundGrpcManager::reconfigure(const EndpointConfig& config, const RateConfig& rateConfig)
{
    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcManager].reconfigure");

    for (auto& endpoint : mGrpcEndpoints)
    {
        const auto itr = config.getGrpcEndpointTypes().find(endpoint->getName());
        if (itr == config.getGrpcEndpointTypes().end())
            continue;
        
        endpoint->reconfigure(*itr->second, rateConfig, config.getRpcControlLists(), config.getPlatformSpecificRpcControlLists());

    }

    mShutdownTimeout = config.getShutdownTimeout();
}

bool InboundGrpcManager::registerServices(const Blaze::ComponentManager& compMgr)
{
    for (auto& endpoint : mGrpcEndpoints)
    {
       if (!endpoint->registerServices(compMgr))
            return false;
    }

    return true;
}

bool InboundGrpcManager::startGrpcServers()
{
    if (mShuttingDown)
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcManager].startGrpcServers: Already shutdown or in the process. Ignoring start call.");
        return false;
    }
    
    for (auto& endpoint : mGrpcEndpoints)
    {
        if (!endpoint->startGrpcServer())
            return false;
    }

    return true;
}



} //namespace Grpc
} //namespace Blaze


