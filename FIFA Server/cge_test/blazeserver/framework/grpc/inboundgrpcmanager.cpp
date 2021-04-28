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

InboundGrpcManager::~InboundGrpcManager()
{
    EA_ASSERT_MSG(mGrpcEndpoints.size() == 0, "GrpcEndpoints still around, did you forget to call shutdown?");
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
                    BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcManager].initialize: Failed to initialize endpoint (" << grpcEndpointEntry->first <<"). Aborting self.");
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

    // Shutting down grpc endpoints cleanly is a tricky problem because of unwieldy apis provided by grpc core and our threading model for the async apis. What we do is

    // 1. Let all the endpoints know that they should begin the shutdown process. This way, they can tell the core grpc server to stop accepting new work immediately. 
    for (auto& endpoint : mGrpcEndpoints)
        endpoint->beginShutdown();

    
    // 2. Sleep for a grace time to allow for the currently pending work to be processed. Due to the multi-threading issues involved, there isn't a safe way to avoid this sleep
    // if no work is happening as the grpc work might be "in transit across thread boundaries".  
    Fiber::sleep(TimeValue(PENDING_SHUTDOWN_GRACE_PERIOD_US), "wait for pending tags in selector queue and grpc internal thread. Also, a grace period for rpcs in progress.");

    // 3. Force the final shutdown. Cancel any work still pending and join all the threads.  
    for (auto& endpoint : mGrpcEndpoints)
        endpoint->shutdown();

    mGrpcEndpoints.clear();
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

// https://groups.google.com/forum/#!msg/grpc-io/X_bUx3T8S7s/x38FU429CgAJ
// Map the Blaze system errors to standard grpc errors. Obviously, they don't have a one-to-one mapping so the custom message string
// can be used to identify the details. If there turns out to be a real need for all the Blaze system errors, we'll need to send
// them as metadata. Until then, we send unmapped errors as UNKNOWN errors.
grpc::Status InboundGrpcManager::getGrpcStatusFromSystemError(BlazeRpcError err)
{
    switch (err)
    {
    case ERR_SYSTEM:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_SYSTEM));

    case ERR_COMPONENT_NOT_FOUND:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_COMPONENT_NOT_FOUND));

    case ERR_COMMAND_NOT_FOUND:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_COMMAND_NOT_FOUND));

    case ERR_AUTHENTICATION_REQUIRED:
        return grpc::Status(grpc::UNAUTHENTICATED, ErrorHelp::getErrorDescription(ERR_AUTHENTICATION_REQUIRED));

    case ERR_TIMEOUT:
        return grpc::Status(grpc::DEADLINE_EXCEEDED, ErrorHelp::getErrorDescription(ERR_TIMEOUT));

    case ERR_DISCONNECTED:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DISCONNECTED));

    case ERR_DUPLICATE_LOGIN:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DUPLICATE_LOGIN));

    case ERR_AUTHORIZATION_REQUIRED:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_AUTHORIZATION_REQUIRED));

    case ERR_CANCELED:
        return grpc::Status(grpc::CANCELLED, ErrorHelp::getErrorDescription(ERR_CANCELED));

    case ERR_CUSTOM_REQUEST_HOOK_FAILED:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_CUSTOM_REQUEST_HOOK_FAILED));

    case ERR_CUSTOM_RESPONSE_HOOK_FAILED:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_CUSTOM_RESPONSE_HOOK_FAILED));

    case ERR_TDF_STRING_TOO_LONG:
        return grpc::Status(grpc::OUT_OF_RANGE, ErrorHelp::getErrorDescription(ERR_TDF_STRING_TOO_LONG));

    case ERR_INVALID_TDF_ENUM_VALUE:
        return grpc::Status(grpc::OUT_OF_RANGE, ErrorHelp::getErrorDescription(ERR_INVALID_TDF_ENUM_VALUE));

    case ERR_MOVED:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_MOVED));

    case ERR_COMMAND_FIBERS_MAXED_OUT:
        return grpc::Status(grpc::RESOURCE_EXHAUSTED, ErrorHelp::getErrorDescription(ERR_COMMAND_FIBERS_MAXED_OUT));

    case ERR_INVALID_ENDPOINT:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_INVALID_ENDPOINT));

    case ERR_DB_SYSTEM:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_SYSTEM));

    case ERR_DB_NOT_CONNECTED:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_DB_NOT_CONNECTED));

    case ERR_DB_NOT_SUPPORTED:
        return grpc::Status(grpc::UNIMPLEMENTED, ErrorHelp::getErrorDescription(ERR_DB_NOT_SUPPORTED));

    case ERR_DB_NO_CONNECTION_AVAILABLE:
        return grpc::Status(grpc::RESOURCE_EXHAUSTED, ErrorHelp::getErrorDescription(ERR_DB_NO_CONNECTION_AVAILABLE));

    case ERR_DB_DUP_ENTRY:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DB_DUP_ENTRY));

    case ERR_DB_NO_SUCH_TABLE:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_DB_NO_SUCH_TABLE));

    case ERR_DB_TIMEOUT:
        return grpc::Status(grpc::DEADLINE_EXCEEDED, ErrorHelp::getErrorDescription(ERR_DB_TIMEOUT));

    case ERR_DB_DISCONNECTED:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_DISCONNECTED));

    case ERR_DB_INIT_FAILED:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_DB_INIT_FAILED));

    case ERR_DB_TRANSACTION_NOT_COMPLETE:
        return grpc::Status(grpc::ABORTED, ErrorHelp::getErrorDescription(ERR_DB_TRANSACTION_NOT_COMPLETE));

    case ERR_DB_LOCK_DEADLOCK:
        return grpc::Status(grpc::INTERNAL, ErrorHelp::getErrorDescription(ERR_DB_LOCK_DEADLOCK));

    case ERR_DB_DROP_PARTITION_NON_EXISTENT:
        return grpc::Status(grpc::NOT_FOUND, ErrorHelp::getErrorDescription(ERR_DB_DROP_PARTITION_NON_EXISTENT));

    case ERR_DB_SAME_NAME_PARTITION:
        return grpc::Status(grpc::ALREADY_EXISTS, ErrorHelp::getErrorDescription(ERR_DB_SAME_NAME_PARTITION));

    case ERR_SERVER_BUSY:
        return grpc::Status(grpc::UNAVAILABLE, ErrorHelp::getErrorDescription(ERR_SERVER_BUSY));

    case ERR_GUEST_SESSION_NOT_ALLOWED:
        return grpc::Status(grpc::PERMISSION_DENIED, ErrorHelp::getErrorDescription(ERR_GUEST_SESSION_NOT_ALLOWED));

    case ERR_DB_USER_DEFINED_EXCEPTION:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_DB_USER_DEFINED_EXCEPTION));

    case ERR_COULDNT_CONNECT:
        return grpc::Status(grpc::FAILED_PRECONDITION, ErrorHelp::getErrorDescription(ERR_COULDNT_CONNECT));

    case ERR_DB_PREVIOUS_TRANSACTION_IN_PROGRESS:
        return grpc::Status(grpc::ABORTED, ErrorHelp::getErrorDescription(ERR_DB_PREVIOUS_TRANSACTION_IN_PROGRESS));

    case ERR_UNAVAILABLE:
        return grpc::Status(grpc::UNAVAILABLE, ErrorHelp::getErrorDescription(ERR_UNAVAILABLE));

    default:
        return grpc::Status(grpc::UNKNOWN, ErrorHelp::getErrorDescription(ERR_SYSTEM));
    }
}

} //namespace Grpc
} //namespace Blaze


