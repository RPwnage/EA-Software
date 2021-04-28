/*************************************************************************************************/
/*!
    \file   outboundgrpcjobhandlers.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"
#include "framework/grpc/grpcutils.h"
#include "framework/util/shared/blazestring.h"
#include "google/rpc/code.pb.h"
#include <EASTL/string.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileStream.h>

#include <chrono>

namespace Blaze
{
namespace Grpc
{

OutboundRpcBase::OutboundRpcBase(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const TimeValue& requestTimeout, grpc::CompletionQueue* cq)
    : mOwner(owner),
      mCQ(cq),
      mModuleName(moduleName),
      mRpcName(rpcName),
      mRpcId(mOwner->getNextRpcId()),
      mPendingAsyncOpCounter(0),
      mInitialized(false),
      mPendingFinish(false),
      mPendingShutdown(false),
      mShutdown(false),
      mFinishOnRead(false),
      mRequestTimeout(requestTimeout),
      mRequestIdOverrideInUse(false),
      mRefCount(0),
      mAsyncReadInProgress(false),
      mAsyncWriteInProgress(false),
      mAsyncInitInProgress(false)
{
    mOwner->incRpcCount();
    mOwner->attachRpc(this);

    // Save the log context for any gRPC async ops
    if (gLogContext != nullptr)
        mLogContext = *gLogContext;
}

OutboundRpcBase::~OutboundRpcBase()
{
    if (mOwner != nullptr)
        mOwner->decRpcCount();
}

const char8_t* OutboundRpcBase::logPrefix() 
{
    if (mLogPrefixString.empty())
        mLogPrefixString.sprintf("[%s:(%s:%s)]", rpcType(), (mOwner != nullptr ? mOwner->getServiceName() : ""), mRpcName.c_str());

    return mLogPrefixString.c_str();
}

BlazeRpcError OutboundRpcBase::sendRequest(const google::protobuf::Message* request, ResponsePtr& response)
{
    BlazeRpcError err = sendRequest(request);
    if (err == ERR_OK)
    {
        err = waitForResponse(response);
        return err;
    }

    return err;
}

BlazeRpcError OutboundRpcBase::sendRequest(const google::protobuf::Message* request)
{
    if (mPendingShutdown)
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".sendRequest: Instance is shutting down. Request won't be sent.");
        return ERR_SYSTEM;
    }

    if (request != nullptr)
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".sendRequest: Sending request:\n" << request->DebugString().c_str());
    }

    if (!mRequestIdOverrideInUse)
    {
        UUIDUtil::generateUUID(mRequestId);
    }

    // Add a UUID for all outgoing gRPC requests to support request tracing in Envoy.
    getClientContext().AddMetadata("x-request-id", mRequestId.c_str());

    return sendRequestImpl(request);
}
const UUID& OutboundRpcBase::overrideRequestId(const UUID& requestId)
{
    mRequestIdOverrideInUse = true;
    if (requestId.empty())
    {
        UUIDUtil::generateUUID(mRequestId);
    }
    else
    {
        mRequestId = requestId;
    }

    return mRequestId;
}

void OutboundRpcBase::clearRequestIdOverride()
{
    mRequestIdOverrideInUse = false;
    mRequestId = "";
}




BlazeRpcError OutboundRpcBase::waitForResponse(ResponsePtr& response)
{
    if (mPendingShutdown)
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponse: Instance is shutting down. No new responses can be received.");
        return ERR_SYSTEM;
    }

    BlazeRpcError rc = waitForResponseImpl(response);
    if (response != nullptr)
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponse: Response read:\n" << response->DebugString().c_str());
    }
    return rc;
}

bool OutboundRpcBase::prepareForShutdown()
{
    BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".prepareForShutdown: Preparing to shutdown. RPC reference count is " << mRefCount << ", hasPendingOperations: " << hasPendingOperations());

    mPendingShutdown = true;

    mClientContext.TryCancel(); // Cancel any in-progress operations

    // A ref count of more than 1 implies that something other than the connection
    // is holding a reference to us, so we need to wait until that reference goes away
    // before shutting down
    return (!hasPendingOperations() && (mRefCount == 1));
}

void OutboundRpcBase::shutdown()
{
    if (!mShutdown)
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".shutdown: Shutting down.");

        mShutdown = true;
        mOwner->detachRpc(getRpcId()); // Notify our owner to release us
        mOwner = nullptr; // Invalidate the reference to our owner as it could be deleted after detaching
    }
}

void OutboundRpcBase::asyncOpStarted(AsyncOpType opType)
{
    ++mPendingAsyncOpCounter;

    switch (opType)
    {
    case ASYNC_OP_TYPE_READ:
        mAsyncReadInProgress = true;
        break;
    case ASYNC_OP_TYPE_WRITE:
        mAsyncWriteInProgress = true;
        break;
    case ASYNC_OP_TYPE_INIT:
        mAsyncInitInProgress = true;
        break;
    case ASYNC_OP_TYPE_FINISH:
        mPendingFinish = true;
        break;
    default: //Don't care about other ops
        break;
    }
}

// returns true if the rpc processing should keep going. false otherwise.
bool OutboundRpcBase::asyncOpEnded(AsyncOpType opType)
{
    --mPendingAsyncOpCounter;
    BLAZE_ASSERT_COND_LOG(Log::GRPC, (mPendingAsyncOpCounter >= 0), logPrefix() << ".asyncOpEnded: attempting to decrement mPendingAsyncOpCounter below 0");

    switch (opType)
    {
    case ASYNC_OP_TYPE_READ:
        mAsyncReadInProgress = false;
        break;
    case ASYNC_OP_TYPE_WRITE:
        mAsyncWriteInProgress = false;
        break;
    case ASYNC_OP_TYPE_INIT:
        mAsyncInitInProgress = false;
        break;
    case ASYNC_OP_TYPE_FINISH:
    {
        BLAZE_ASSERT_COND_LOG(Log::GRPC, (!hasPendingOperations()), logPrefix() << ".asyncOpEnded - FINISH called with pending operations!");
        break;
    }
    default: //Don't care about other ops
        break;
    }

    if ((mPendingShutdown || (opType == ASYNC_OP_TYPE_FINISH)) && !hasPendingOperations())
    {
        if (opType == ASYNC_OP_TYPE_FINISH)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".asyncOpEnded: RPC finished. Shutting down.");
        }
        else
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".asyncOpEnded: No remaining pending operations. Shutting down.");
        }

        shutdown();
        return false;
    }

    return true;
}

void OutboundRpcBase::incRequestsSent()
{
    if (mOwner != nullptr)
        mOwner->incRequestsSent();
}

void OutboundRpcBase::incFailedRequests()
{
    if (mOwner != nullptr)
        mOwner->incFailedRequests();
}

void OutboundRpcBase::incResponsesReceived()
{
    if (mOwner != nullptr)
        mOwner->incResponsesReceived();
}

void OutboundRpcBase::incFailedResponses()
{
    if (mOwner != nullptr)
        mOwner->incFailedResponses();
}

TimeValue OutboundRpcBase::getGrpcTimeout() const
{
    return TimeValue::getTimeOfDay() + mRequestTimeout;
}

eastl::string OutboundRpcBase::getStatusString(const BlazeRpcError& rc)
{
    if (rc != ERR_OK)
    {
        eastl::string blazeStatus(eastl::string::CtorSprintf(), "BLAZE_%s", LOG_ENAME(rc));
        return blazeStatus;
    }
    else 
    {
        const google::protobuf::EnumDescriptor* ed = google::rpc::Code_descriptor();
        eastl::string grpcStatus(eastl::string::CtorSprintf(), "GRPC_%s", ed->FindValueByNumber(getStatus().error_code())->name().c_str());
        return grpcStatus;
    }
}

void OutboundRpcBase::incCallsStartedCount(const char* serviceName, const char* commandUri) 
{
    gController->getExternalServiceMetrics().incCallsStarted(serviceName, commandUri);
}

void OutboundRpcBase::incCallsFinishedCount(const char* serviceName, const char* commandUri)
{
    gController->getExternalServiceMetrics().incCallsFinished(serviceName, commandUri);
}

void OutboundRpcBase::incRequestsSentCount(const char* serviceName, const char* commandUri)
{
    gController->getExternalServiceMetrics().incRequestsSent(serviceName, commandUri);
}

void OutboundRpcBase::incRequestsFailedCount(const char* serviceName, const char* commandUri)
{
    gController->getExternalServiceMetrics().incRequestsFailed(serviceName, commandUri);
}


void OutboundRpcBase::recordResponseTime(const char* serviceName, const char* commandUri, const char* responseCode, const TimeValue& duration)
{
    gController->getExternalServiceMetrics().incResponseCount(serviceName, commandUri, responseCode);
    gController->getExternalServiceMetrics().recordResponseTime(serviceName, commandUri, responseCode, duration.getMicroSeconds());
}

} // namespace Grpc
} // namespace Blaze