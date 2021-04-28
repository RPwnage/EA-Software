#include "framework/blaze.h"
#include "framework/grpc/outboundgrpcconnection.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"
#include "framework/grpc/outboundgrpcmanager.h"

namespace Blaze
{

namespace Grpc
{

const char8_t* SERVICE_MODULE_DELIMITER = ":";

void OutboundGrpcConnectionBase::shutdown()
{
    BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcConnectionBase].shutdown: Begin shutdown");

    for (auto& it : mOutboundRpcs)
    {
        OutboundRpcBase* rpc = it.second.get();
        
        if (rpc->prepareForShutdown())
            rpc->shutdown(); 
    }

    if (!mOutboundRpcs.empty())
    {
        Fiber::getAndWait(mShutdownEventHandle, "OutboundGrpcConnectionBase::shutdown");
    }

    BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcConnectionBase].shutdown: All outstanding RPCs have shutdown.");
}

void OutboundGrpcConnectionBase::requestCleanup()
{
    gController->getOutboundGrpcManager()->cleanupServiceConn(mServiceNameShortVersion);
}

void OutboundGrpcConnectionBase::attachRpc(OutboundRpcBasePtr rpc)
{
    mOutboundRpcs[rpc->getRpcId()] = rpc;
}

void OutboundGrpcConnectionBase::detachRpc(uint64_t rpcId)
{
    mPendingRemovalQueue.push(rpcId);

    if (!mRpcShutdownPending)
    {
        mRpcShutdownPending = true;
        Fiber::CreateParams params;
        params.stackSize = Fiber::STACK_SMALL;
        gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &OutboundGrpcConnectionBase::detachRpcInternal, "OutboundGrpcConnectionBase::detachRpc", params);
    }
}

void OutboundGrpcConnectionBase::detachRpcInternal()
{
    while (!mPendingRemovalQueue.empty())
    {
        uint64_t rpcId = mPendingRemovalQueue.front();
        mPendingRemovalQueue.pop();

        auto it = mOutboundRpcs.find(rpcId);
        if (it != mOutboundRpcs.end())
        {
            mOutboundRpcs.erase(it);
        }
    }

    mRpcShutdownPending = false;

    if (mOutboundRpcs.empty() && mShutdownEventHandle.isValid())
    {
        // Signal waiting fiber that we've released our last RPC
        Fiber::signal(mShutdownEventHandle, ERR_OK);
    }
}

void OutboundGrpcConnectionBase::getStatusInfo(OutboundGrpcServiceStatusMap& infoMap) const
{
    OutboundGrpcServiceStatus* statusMap = infoMap.allocate_element();
    statusMap->setCurrentRpcCount(mOutboundGrpcMetrics.mNumRpcs.get());
    statusMap->setRequestsSent(mOutboundGrpcMetrics.mNumRequestsSent.getTotal());
    statusMap->setRequestsFailed(mOutboundGrpcMetrics.mNumRequestsFailed.getTotal());
    statusMap->setResponsesReceived(mOutboundGrpcMetrics.mNumResponsesReceived.getTotal());
    statusMap->setResponsesFailed(mOutboundGrpcMetrics.mNumResponsesFailed.getTotal());

    infoMap[mServiceName.c_str()] = statusMap;
}

void OutboundGrpcConnectionBase::incRpcCount()
{
    mOutboundGrpcMetrics.mNumRpcs.increment();
}

void OutboundGrpcConnectionBase::decRpcCount()
{
    mOutboundGrpcMetrics.mNumRpcs.decrement();
}

void OutboundGrpcConnectionBase::incRequestsSent()
{
    mOutboundGrpcMetrics.mNumRequestsSent.increment();
}

void OutboundGrpcConnectionBase::incFailedRequests()
{
    mOutboundGrpcMetrics.mNumRequestsFailed.increment();
}

void OutboundGrpcConnectionBase::incResponsesReceived()
{
    mOutboundGrpcMetrics.mNumResponsesReceived.increment();
    mOutboundGrpcMetrics.mFailureStreak = 0;
}

void OutboundGrpcConnectionBase::incFailedResponses()
{
    mOutboundGrpcMetrics.mNumResponsesFailed.increment();
    ++(mOutboundGrpcMetrics.mFailureStreak);

    const int CONNECTION_CLEANUP_THRESHOLD = 3; // 3 consecutive failed responses. Clean up our connection and try again.
    if (mOutboundGrpcMetrics.mFailureStreak == CONNECTION_CLEANUP_THRESHOLD)
    {
        if (!mCleanupRequested)
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcConnectionBase].incFailedResponses: Consecutive error streak exceeds the threshold. Cleaning up the service("
                << mServiceNameShortVersion << ") connection. New connection will be created for future requests.");

            requestCleanup();
            mCleanupRequested = true;
        }
    }

}

} // namespace Grpc
} // namespace Blaze
