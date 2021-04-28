/*************************************************************************************************/
/*!
\file   outboundgrpcjobhandlers.h

\attention
(c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!

*/
/*************************************************************************************************/
#ifndef BLAZE_OUTBOUND_GRPC_JOBHANDLERS_H
#define BLAZE_OUTBOUND_GRPC_JOBHANDLERS_H

#include "framework/blazedefines.h"

#include "framework/logger.h"
#include "framework/grpc/grpcutils.h"
#include "framework/grpc/outboundgrpcconnection.h"
#include "framework/connection/sslcontext.h"
#include "framework/util/uuid.h"

#include <EATDF/tdf.h>
#include <EASTL/queue.h>

#include <eathread/eathread_atomic.h>

// Note: All grpc lib calls below are thread-safe.
namespace Blaze
{
namespace Grpc
{

typedef eastl::unique_ptr<google::protobuf::Message> ResponsePtr;

class OutboundRpcBase
{
public:
    OutboundRpcBase(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const EA::TDF::TimeValue& requestTimeout, grpc::CompletionQueue* cq);
    virtual ~OutboundRpcBase();


    //*! ************************************************************************************************/
    /*! OutboundRpcBase::sendRequest

        \brief Sends a request to the remote service and waits for a response.

        \param[in] request - The request to send
        \param[out] response - The resultant response. nullptr if an error occurred retrieving the response.

        \return - false if the request was unsuccessfully sent (e.g., an error was received from a previous send) or if a nullptr response was generated, true otherwise.
            If false is returned, the caller should immediately discard (i.e., set to nullptr) the OutboundGrpcBase object
            to clean up the RPCs allocated memory
    ***************************************************************************************************/
    virtual BlazeRpcError sendRequest(const google::protobuf::Message* request, ResponsePtr& response);

    //*! ************************************************************************************************/
    /*! OutboundRpcBase::sendRequest

        \brief Sends a request to the remote service.

        \param[in] request - The request to send

        \return - false if the request was unsuccessfully sent (e.g., an error was received from a previous send), true otherwise.
            If false is returned, the caller should immediately discard (i.e., set to nullptr) the OutboundGrpcBase object
    ***************************************************************************************************/
    virtual BlazeRpcError sendRequest(const google::protobuf::Message* request);

    //*! ************************************************************************************************/
    /*! OutboundRpcBase::waitForResponse

        \brief Waits for a response from a previous call to sendRequest.

        \return - The resultant response. nullptr if an error occurred retrieving the response.
    ***************************************************************************************************/
    virtual BlazeRpcError waitForResponse(ResponsePtr& response);

    inline bool hasPendingOperations() const { return mPendingAsyncOpCounter != 0; }
    bool prepareForShutdown();
    void shutdown();

    // intrusive_ptr implementation
    inline void AddRef()
    {
        ++mRefCount;
    }

    inline void Release()
    {
        if (--mRefCount == 0)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".release");
            delete this;
        }
    }

    grpc::Status getStatus() { return mStatus; }
    
    eastl::string getStatusString(const BlazeRpcError& rc);

    grpc::ClientContext& getClientContext() { return mClientContext; }
    uint64_t getRpcId() const { return mRpcId; }
    const UUID& getRequestId() const { return mRequestId; }

    // Overrides the request id ("x-request-id") for all subsequent request. 
    // Pass in an empty string and a random UUID will be generated and returned.
    const UUID& overrideRequestId(const UUID& requestId);
    void clearRequestIdOverride();

    //returns if blaze should retry sending the gRPC request
    bool shouldRetry() const { return mStatus.error_code() == grpc::UNKNOWN || mStatus.error_code() == grpc::ABORTED || mStatus.error_code() == grpc::UNAVAILABLE; }

protected:
    virtual BlazeRpcError sendRequestImpl(const google::protobuf::Message *request) = 0;
    virtual BlazeRpcError waitForResponseImpl(ResponsePtr& response) = 0;

    void asyncOpStarted(AsyncOpType opType);
    bool asyncOpEnded(AsyncOpType opType); // returns true if the rpc processing should keep going. false otherwise.

    inline bool asyncReadInProgress() const { return mAsyncReadInProgress; }
    inline bool asyncWriteInProgress() const { return mAsyncWriteInProgress; }
    inline bool asyncInitInProgress() const { return mAsyncInitInProgress; }

    void incRequestsSent();
    void incFailedRequests();
    void incResponsesReceived();
    void incFailedResponses();

    void incCallsStartedCount(const char* serviceName, const char* commandUri);
    void incCallsFinishedCount(const char* serviceName, const char* commandUri); 
    void incRequestsSentCount(const char* serviceName, const char* commandUri);
    void incRequestsFailedCount(const char* serviceName, const char* commandUri);
    void recordResponseTime(const char* serviceName, const char* commandUri, const char* responseCode, const TimeValue& duration);

    EA::TDF::TimeValue getGrpcTimeout() const;
    
    virtual const char8_t* rpcType() const = 0;
    const char8_t* logPrefix();

protected:
    OutboundGrpcConnectionBase* mOwner;

    grpc::CompletionQueue* mCQ;
    grpc::ClientContext mClientContext;
    grpc::Status mStatus;

    Fiber::WaitList mInitHandles;         // Multiple request may be waiting on the init to complete
    Fiber::EventHandle mResponseWaitHandle;
    LogContextWrapper mLogContext;

    eastl::string mLogPrefixString;
    eastl::string mModuleName;      // Full GRPC module name
    eastl::string mRpcName;         // RPC name being used

    uint64_t mRpcId; // Used as key in owner's map to simplify cleanup

    // Tracks pending async operations to ensure proper object destruction
    int32_t mPendingAsyncOpCounter;

    bool mInitialized; // Indicates that the RPC is initialized
    bool mPendingFinish; // Indicates that this RPC has invoked Finish (no more reads or writes)
    bool mPendingShutdown; // Indicates that we're shutting down (cancels any in-progress ops and prevents further RPC processing)
    bool mShutdown; // Indicates that we're done (all async operations are complete and we've told our owner to release us)

    bool mFinishOnRead;

private:
    EA::TDF::TimeValue mRequestTimeout;
    UUID mRequestId;
    bool mRequestIdOverrideInUse;

    mutable uint32_t mRefCount;

    bool mAsyncReadInProgress;
    bool mAsyncWriteInProgress;
    bool mAsyncInitInProgress;
};
typedef eastl::intrusive_ptr<OutboundRpcBase> OutboundRpcBasePtr;

template<typename StubType, typename RequestType, typename ResponseType>
using OutboundUnaryRequestRpc = std::function<std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>(StubType*, grpc::ClientContext*, const RequestType&, grpc::CompletionQueue*)>;

template<typename StubType, typename RequestType, typename ResponseType>
using OutboundClientStreamingRequestRpc = std::function<std::unique_ptr<grpc::ClientAsyncWriter<RequestType>>(StubType*, grpc::ClientContext*, ResponseType*, grpc::CompletionQueue*, void*)>;

template<typename StubType, typename RequestType, typename ResponseType>
using OutboundServerStreamingRequestRpc = std::function<std::unique_ptr<grpc::ClientAsyncReader<ResponseType>>(StubType*, grpc::ClientContext*, const RequestType&, grpc::CompletionQueue*, void* tag)>;

template<typename StubType, typename RequestType, typename ResponseType>
using OutboundBidirectionalStreamingRequestRpc = std::function<std::unique_ptr<grpc::ClientAsyncReaderWriter<RequestType, ResponseType>>(StubType*, grpc::ClientContext*, grpc::CompletionQueue*, void*)>;

template<typename Module, typename RequestType, typename ResponseType>
class OutboundUnaryRpc : public OutboundRpcBase
{
    typedef typename Module::Stub StubType;

public:
    OutboundUnaryRpc(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const EA::TDF::TimeValue& requestTimeout,
        OutboundUnaryRequestRpc<StubType, RequestType, ResponseType> callRpc, grpc::CompletionQueue* cq)
        : OutboundRpcBase(owner, moduleName, rpcName, requestTimeout, cq),
          mCallRpc(callRpc)
    {
        mOnFinish = std::bind(&OutboundUnaryRpc::onFinish, this, std::placeholders::_1);
    }

    const char8_t* rpcType() const override { return "OutboundUnaryRpc"; }

private:
    BlazeRpcError sendRequestImpl(const google::protobuf::Message* request) override
    {
        incCallsStartedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());

        if (mRpc != nullptr)
        {
            BLAZE_ASSERT_LOG(Log::GRPC, logPrefix() << ".sendRequestImpl: Attempting to send a request on an unary RPC that has already sent a request is not allowed.");
            return ERR_OK;
        }

        if (request != nullptr)
        {
            mRpc = mCallRpc(mOwner->getAsService<Module>()->getStub(), &mClientContext, static_cast<const RequestType&>(*request), mCQ);
            incRequestsSent();
            incRequestsSentCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
        }

        return ERR_OK;
    };

    BlazeRpcError waitForResponseImpl(ResponsePtr& response) override
    {
        response = nullptr;
        if (mRpc != nullptr && mResponsePtr == nullptr)
        {
            TimeValue beforeGetAndWait = TimeValue::getTimeOfDay();
            mResponsePtr = ResponsePtr(BLAZE_NEW ResponseType);

            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: Reading response.");
            asyncOpStarted(ASYNC_OP_TYPE_FINISH);
            mRpc->Finish(static_cast<ResponseType*>(mResponsePtr.get()), &mStatus, &mOnFinish);
            BlazeRpcError rc = Fiber::getAndWait(mResponseWaitHandle, "OutboundUnaryRpc::waitForResponseImpl", getGrpcTimeout());
            TimeValue afterGetAndWait = TimeValue::getTimeOfDay();
            recordResponseTime(mOwner->getServiceNameShortVersion(), mRpcName.c_str(), getStatusString(rc).c_str(), afterGetAndWait - beforeGetAndWait);
            
            if (rc != ERR_OK)
            {
                if (rc == ERR_TIMEOUT)
                    mClientContext.TryCancel();     // Even if we're TryCancelling, we may still need to keep the response around.

                BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: Error occurred while waiting for response: " << ErrorHelp::getErrorName(rc) << " after waiting " << TimeValue(afterGetAndWait - beforeGetAndWait).getMillis() << " ms.");
                return rc;
            }
            else
            {
                // Only return the response pointer in the case of success
                response = eastl::move(mResponsePtr);
                return ERR_OK;
            }
        }

        BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: No write has occurred yet. Ignoring request.");
        return ERR_SYSTEM;
    }

    void onFinish(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        if (ok && mStatus.error_code() == grpc::OK)
        {
            incResponsesReceived();
            Fiber::signal(mResponseWaitHandle, ERR_OK);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onFinish: An unexpected error occurred. Status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << ").");

            incFailedResponses();
            Fiber::signal(mResponseWaitHandle, ERR_SYSTEM);
        }

        incCallsFinishedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());

        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
            << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        asyncOpEnded(ASYNC_OP_TYPE_FINISH);
    }

private:
    OutboundUnaryRequestRpc<StubType, RequestType, ResponseType> mCallRpc;
    std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> mRpc;

    ResponsePtr mResponsePtr;
    TagProcessor mOnFinish;
};

template<typename Module, typename RequestType, typename ResponseType>
class OutboundClientStreamingRpc : public OutboundRpcBase
{
    typedef typename Module::Stub StubType;

public:
    OutboundClientStreamingRpc(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const EA::TDF::TimeValue& requestTimeout,
        OutboundClientStreamingRequestRpc<StubType, RequestType, ResponseType> createStream,
        grpc::CompletionQueue* cq)
        : OutboundRpcBase(owner, moduleName, rpcName, requestTimeout, cq),
          mCreateStream(createStream),
          mPendingWritesDone(false),
          mWritesDone(false)
    {
        mOnInit = std::bind(&OutboundClientStreamingRpc::onInit, this, std::placeholders::_1);
        mOnWrite = std::bind(&OutboundClientStreamingRpc::onWrite, this, std::placeholders::_1);
        mOnWritesDone = std::bind(&OutboundClientStreamingRpc::onWritesDone, this, std::placeholders::_1);
        mOnFinish = std::bind(&OutboundClientStreamingRpc::onFinish, this, std::placeholders::_1);
    }

    const char8_t* rpcType() const override { return "OutboundClientStreamingRpc"; }

private:
    BlazeRpcError init()
    {
        if (!mInitialized)
        {
            if (!asyncInitInProgress())
            {
                incCallsStartedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".init: Initializing client streaming RPC. ");
                asyncOpStarted(ASYNC_OP_TYPE_INIT);
                mStream = mCreateStream(mOwner->getAsService<Module>()->getStub(), &mClientContext, &mResponse, mCQ, &mOnInit);
            }

            BlazeRpcError rc = mInitHandles.wait("OutboundClientStreamingRpc::init", getGrpcTimeout());
            if (rc != ERR_OK)
            {
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".init: Error occurred while waiting for initialization to complete: " << ErrorHelp::getErrorName(rc)); 
                mClientContext.TryCancel();
                return rc;
            }
        }

        return ERR_OK;
    }

    BlazeRpcError sendRequestImpl(const google::protobuf::Message *request) override
    {
        if (mPendingWritesDone || mPendingFinish)
        {
            BLAZE_INFO_LOG(Log::GRPC, logPrefix() << ".sendRequestImpl: Connection is no longer accepting requests.");
            return ERR_OK;
        }

        BlazeRpcError err = init();
        if (err != ERR_OK)
            return err;

        if (request != nullptr)
        {
            mPendingRequests.push(*static_cast<const RequestType*>(request)); // Need to make a copy here
        }
        else
        {
            mPendingWritesDone = true; // Delay calling WritesDone until the queue is empty
        }

        doSendRequest();

        return ERR_OK;
    }

    void doWritesDone()
    {
        if (!asyncWriteInProgress() && !mWritesDone)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doWritesDone: finished sending.");
            asyncOpStarted(ASYNC_OP_TYPE_WRITESDONE);
            mStream->WritesDone(&mOnWritesDone); // Signal that we aren't writing any more, but will still accept responses
        }
    }

    void doSendRequest()
    {
        if (!asyncWriteInProgress())
        {
            if (!mPendingRequests.empty())
            {
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doSendRequest: Sending request. " << mPendingRequests.size() << " requests remaining. ");
                asyncOpStarted(ASYNC_OP_TYPE_WRITE);
                mStream->Write(mPendingRequests.front(), &mOnWrite);
            }
            else if (mPendingWritesDone)
            {
                doWritesDone();
            }
        }
    }

    BlazeRpcError waitForResponseImpl(ResponsePtr& response) override
    {
        // Calling this method will prevent sending any more requests
        BlazeRpcError rc;
        response = nullptr;

        // Wait for any pending requests to finish
        mPendingWritesDone = true;
        if (!mPendingRequests.empty() || !mWritesDone)
        {
            if (!mWritesDone)
                doWritesDone();

            rc = Fiber::getAndWait(mWritesDoneHandle, "OutboundClientStreamingRpc::waitForResponseImpl", getGrpcTimeout());
            if (rc != ERR_OK)
            {
                if (rc == ERR_TIMEOUT)
                    mClientContext.TryCancel();

                return rc;
            }
        }

        if (!asyncReadInProgress() && mInitialized)
        {
            TimeValue beforeGetAndWait = TimeValue::getTimeOfDay();

            mFinishOnRead = true;
            finish();
            rc = Fiber::getAndWait(mResponseWaitHandle, "OutboundClientStreamingRpc::waitForResponseImpl", getGrpcTimeout());
            
            TimeValue afterGetAndWait = TimeValue::getTimeOfDay();
            recordResponseTime(mOwner->getServiceNameShortVersion(), mRpcName.c_str(), getStatusString(rc).c_str(), afterGetAndWait - beforeGetAndWait);

            if (rc != ERR_OK)
            {
                if (rc == ERR_TIMEOUT)
                    mClientContext.TryCancel();

                return rc;
            }
        }

        response = BLAZE_NEW ResponseType;
        response->CopyFrom(mResponse);
        return ERR_OK;
    }

    void finish()
    {
        if (!mPendingFinish)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".finish: Finish streaming. ");

            mPendingRequests = eastl::queue<RequestType>(); // Ignore any outstanding requests, they'll just fail

            asyncOpStarted(ASYNC_OP_TYPE_FINISH);
            mPendingFinish = true; // Prevent subsequent requests from being processed
            mStream->Finish(&mStatus, &mOnFinish);
        }
    }

    void onInit(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        mInitHandles.signal(ok ? ERR_OK : ERR_SYSTEM);
        if (!asyncOpEnded(ASYNC_OP_TYPE_INIT))
            return;

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onInit: Connected to service.");
            mInitialized = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onInit: Failed to connect to service - finishing stream. (Check onFinished for more details.)");
            incRequestsFailedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            incFailedRequests();
            finish();
        }
    }
    
    void onWrite(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        if (!asyncOpEnded(ASYNC_OP_TYPE_WRITE))
            return;

        if (!mPendingRequests.empty())
            mPendingRequests.pop();

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onWrite: OK.");
            incRequestsSent();
            incRequestsSentCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            doSendRequest();
        }
        else
        {
            // Server reported error, kill the stream
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onWrite: Server indicated error - finishing stream. (Check onFinished for more details.)");
            incFailedRequests();
            incRequestsFailedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            finish();
        }
    }

    void onWritesDone(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        mWritesDone = true;
        if (mWritesDoneHandle.isValid())
        {
            Fiber::signal(mWritesDoneHandle, ok ? ERR_OK : ERR_SYSTEM);
            mWritesDoneHandle.reset();
        }

        if (!asyncOpEnded(ASYNC_OP_TYPE_WRITESDONE))
            return;

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onWritesDone: OK. ");
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onWritesDone: Error - finishing stream. (Check onFinished for more details.)");
            finish();
        }
    }


    void onFinish(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        bool errorOccurred = (mStatus.error_code() > grpc::UNKNOWN || ok == false);

        // grpc will return UNKNOWN when closing the stream, which doesn't constitute a failure,
        // There may be other cases where UNKNOWN would be a failure, but that would require parsing the
        // error_message or error_details strings, which can break should the error strings change,
        // Thus, a compromise here to ignore UNKNOWN as a response error
        if (mFinishOnRead && mStatus.error_code() > grpc::UNKNOWN)
        {
            incFailedResponses();
            
        } 
        
        incCallsFinishedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
        
        mFinishOnRead = false;

        if (mWritesDoneHandle.isValid())
        {
            Fiber::signal(mWritesDoneHandle, errorOccurred ? ERR_OK : ERR_SYSTEM);
            mWritesDoneHandle.reset();
        }

        if (mResponseWaitHandle.isValid())
        {
            Fiber::signal(mResponseWaitHandle, errorOccurred ? ERR_OK : ERR_SYSTEM);
            mResponseWaitHandle.reset();
        }

        if (errorOccurred)
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }
        else
        {
            incResponsesReceived();
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }

        asyncOpEnded(ASYNC_OP_TYPE_FINISH);
    }

private:
    OutboundClientStreamingRequestRpc<StubType, RequestType, ResponseType> mCreateStream;
    std::unique_ptr<grpc::ClientAsyncWriter<RequestType>> mStream;

    ResponseType mResponse;

    eastl::queue<RequestType> mPendingRequests;

    TagProcessor mOnInit;
    TagProcessor mOnWrite;
    TagProcessor mOnWritesDone;
    TagProcessor mOnFinish;

    bool mPendingWritesDone; // Indicates that we're waiting for the writesDone async op to complete
    bool mWritesDone; // Indicates that the RPC is done writing, but reads are still allowed

    Fiber::EventHandle mWritesDoneHandle;
};

template<typename Module, typename RequestType, typename ResponseType>
class OutboundServerStreamingRpc : public OutboundRpcBase
{
    typedef typename Module::Stub StubType;

public:
    OutboundServerStreamingRpc(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const EA::TDF::TimeValue& requestTimeout,
        OutboundServerStreamingRequestRpc<StubType, RequestType, ResponseType> createStream,
        grpc::CompletionQueue* cq)
        : OutboundRpcBase(owner, moduleName, rpcName, requestTimeout, cq),
          mCreateStream(createStream)
    {
        mOnInit = std::bind(&OutboundServerStreamingRpc::onInit, this, std::placeholders::_1);
        mOnRead = std::bind(&OutboundServerStreamingRpc::onRead, this, std::placeholders::_1);
        mOnFinish = std::bind(&OutboundServerStreamingRpc::onFinish, this, std::placeholders::_1);
    }

    virtual ~OutboundServerStreamingRpc()
    {
        while (!mResponseWaitHandles.empty())
        {
            Fiber::signal(mResponseWaitHandles.front(), ERR_CANCELED);
            mResponseWaitHandles.pop();
        }
    }
    const char8_t* rpcType() const override { return "OutboundServerStreamingRpc"; }

private:
    BlazeRpcError sendRequestImpl(const google::protobuf::Message *request) override
    {
        if (mStream != nullptr)
        {
            BLAZE_ASSERT_LOG(Log::GRPC, logPrefix() << ".sendRequestImpl: Attempting to send a request on a server streaming RPC that has already sent a request is not allowed.");
            return ERR_OK;
        }

        if (request != nullptr)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".sendRequestImpl: Initializing server streaming RPC. ");
            incCallsStartedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            asyncOpStarted(ASYNC_OP_TYPE_INIT);
            mStream = mCreateStream(mOwner->getAsService<Module>()->getStub(), &mClientContext, static_cast<const RequestType&>(*request), mCQ, &mOnInit);
        }

        return ERR_OK;
    }

    void doReadResponse()
    {
        if (!asyncReadInProgress())
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doReadResponse: Reading response from stream. ");
            mResponsePtrs.push(ResponsePtr(BLAZE_NEW ResponseType));   // The response object will only be filled out when Read completes
            asyncOpStarted(ASYNC_OP_TYPE_READ);
            mStream->Read(static_cast<ResponseType*>(mResponsePtrs.back().get()), &mOnRead);
        }
    }

    BlazeRpcError waitForResponseImpl(ResponsePtr& response) override
    {
        if (!mInitialized)
        {
            BlazeRpcError rc = mInitHandles.wait("OutboundServerStreamingRpc::waitForResponseImpl", getGrpcTimeout());
            if (rc != ERR_OK)
            {
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: Error waiting for initialization to complete.");
                mClientContext.TryCancel();
                return rc;
            }
        }

        // Start a read if nothing is waiting:
        doReadResponse();

        // Get in line for our response
        TimeValue beforeWait = TimeValue::getTimeOfDay();
        mResponseWaitHandles.emplace(Fiber::getNextEventHandle());
        const Fiber::EventHandle& handle = mResponseWaitHandles.back();
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: Waiting on fiber id: " << handle.eventId);
        BlazeRpcError rc = Fiber::wait(handle, "OutboundServerStreamingRpc::waitForResponseImpl", getGrpcTimeout());
        
        TimeValue afterWait = TimeValue::getTimeOfDay();
        recordResponseTime(mOwner->getServiceNameShortVersion(), mRpcName.c_str(), getStatusString(rc).c_str(), afterWait - beforeWait);

        if (rc != ERR_OK)
        {
            if (rc == ERR_TIMEOUT)
                mClientContext.TryCancel();     // Even if we're TryCancelling, we may still need to keep the response around.

            return rc;
        }
        else
        {
            // Only return the response point in the case of success.
            response = eastl::move(mResponsePtrs.front());
            mResponsePtrs.pop();
            return ERR_OK;
        }
    }

    void onInit(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        mInitHandles.signal( ok ? ERR_OK : ERR_SYSTEM);

        if (!asyncOpEnded(ASYNC_OP_TYPE_INIT))
            return;

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onInit: Connected to service." );
            mInitialized = true;
            incRequestsSent();
            incRequestsSentCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onInit: Failed to connect to service - finishing stream. (Check onFinished for more details.)");
            incFailedRequests();
            incRequestsFailedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            finish();
        }
    }

    void finish()
    {
        if (!mPendingFinish)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".finish: Finish streaming. ");
            
            asyncOpStarted(ASYNC_OP_TYPE_FINISH);
            mPendingFinish = true;
            mStream->Finish(&mStatus, &mOnFinish);
        }
    }

    void onRead(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        // Signal all waiting response handles because we're shutting down
        if (mPendingShutdown)
        {
            while (!mResponseWaitHandles.empty())
            {
                const Fiber::EventHandle& handle = mResponseWaitHandles.front();
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Signaling fiber id: " << handle.eventId);
                Fiber::signal(handle, ERR_SYSTEM);
                mResponseWaitHandles.pop();
            }
        }

        if (!asyncOpEnded(ASYNC_OP_TYPE_READ))
            return;

        // Wake up the (oldest) fiber that was waiting for a response:
        if (!mResponseWaitHandles.empty())
        {
            const Fiber::EventHandle& handle = mResponseWaitHandles.front();
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Signaling fiber id: " << handle.eventId);
            Fiber::signal(handle, ok ? ERR_OK : ERR_SYSTEM);
            mResponseWaitHandles.pop();
        }

        // Once we start reading, we don't stop until there's an error:
        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: OK. ");
            incResponsesReceived();
            doReadResponse();
        }
        else
        {
            // Server is done, finish to get final status
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Server indicated it was done (error or nothing left to do) - finishing stream. (Check onFinished for more details.)");
            mFinishOnRead = true;
            finish();
        }
    }

    void onFinish(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        bool errorOccurred = (mStatus.error_code() > grpc::UNKNOWN || ok == false);

        // grpc will return UNKNOWN when closing the stream, which doesn't constitute a failure.
        // There may be other cases where UNKNOWN would be a failure, but that would require parsing the
        // error_message or error_details strings, which can break should the error strings change.
        // Thus, a compromise here to ignore UNKNOWN as an response error.
        if (mFinishOnRead && mStatus.error_code() > grpc::UNKNOWN)
        {
            incFailedResponses();
        }
        
        incCallsFinishedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
        
        mFinishOnRead = false;

        if (errorOccurred)
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }
        else
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }

        asyncOpEnded(ASYNC_OP_TYPE_FINISH);
    }

private:
    OutboundServerStreamingRequestRpc<StubType, RequestType, ResponseType> mCreateStream;
    std::unique_ptr<grpc::ClientAsyncReader<ResponseType>> mStream;

    // Pointers to the most recent responses:
    eastl::queue<ResponsePtr> mResponsePtrs;
    eastl::queue<Fiber::EventHandle> mResponseWaitHandles;

    TagProcessor mOnInit;
    TagProcessor mOnRead;
    TagProcessor mOnFinish;
};

template<typename Module, typename RequestType, typename ResponseType>
class OutboundBidirectionalStreamingRpc : public OutboundRpcBase
{
    typedef typename Module::Stub StubType;

public:
    OutboundBidirectionalStreamingRpc(OutboundGrpcConnectionBase* owner, const char8_t* moduleName, const char8_t* rpcName, const EA::TDF::TimeValue& requestTimeout,
        OutboundBidirectionalStreamingRequestRpc<StubType, RequestType, ResponseType> createStream,
        grpc::CompletionQueue* cq)
        : OutboundRpcBase(owner, moduleName, rpcName, requestTimeout, cq),
          mCreateStream(createStream),
          mPendingWritesDone(false)
    {
        mOnInit = std::bind(&OutboundBidirectionalStreamingRpc::onInit, this, std::placeholders::_1);
        mOnWrite = std::bind(&OutboundBidirectionalStreamingRpc::onWrite, this, std::placeholders::_1);
        mOnRead = std::bind(&OutboundBidirectionalStreamingRpc::onRead, this, std::placeholders::_1);
        mOnWritesDone = std::bind(&OutboundBidirectionalStreamingRpc::onWritesDone, this, std::placeholders::_1);
        mOnFinish = std::bind(&OutboundBidirectionalStreamingRpc::onFinish, this, std::placeholders::_1);
    }

    virtual ~OutboundBidirectionalStreamingRpc()
    {
        while (!mResponseWaitHandles.empty())
        {
            Fiber::signal(mResponseWaitHandles.front(), ERR_CANCELED);
            mResponseWaitHandles.pop();
        }
    }
    const char8_t* rpcType() const override { return "OutboundBidirectionalStreamingRpc"; }

private:
    BlazeRpcError init()
    {
        if (!mInitialized)
        {
            if (!asyncInitInProgress())
            {
                incCallsStartedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".init: Initializing bidirectional streaming RPC. ");
                asyncOpStarted(ASYNC_OP_TYPE_INIT);
                mStream = mCreateStream(mOwner->getAsService<Module>()->getStub(), &mClientContext, mCQ, &mOnInit);
            }

            BlazeRpcError rc = mInitHandles.wait("OutboundBidirectionalStreamingRpc::init", getGrpcTimeout());
            if (rc != ERR_OK)
            {
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".init: Error occurred while waiting for initialization to complete: " << ErrorHelp::getErrorName(rc));
                mClientContext.TryCancel();
                return rc;
            }
        }

        return ERR_OK;
    }

    BlazeRpcError sendRequestImpl(const google::protobuf::Message *request) override
    {
        if (mPendingWritesDone || mPendingFinish)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".sendRequestImpl: Connection is no longer accepting requests.");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = init();
        if (err != ERR_OK)
            return err;

        if (request != nullptr)
        {
            mPendingRequests.push(*static_cast<const RequestType*>(request)); // Need to make a copy here
        }
        else
        {
            mPendingWritesDone = true; // Delay calling WritesDone until the queue is empty
        }

        doSendRequest();

        return ERR_OK;
    };

    void doWritesDone()
    {
        if (!asyncWriteInProgress())
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doWritesDone: finished sending. ");
            asyncOpStarted(ASYNC_OP_TYPE_WRITESDONE);
            mStream->WritesDone(&mOnWritesDone); // Signal that we aren't writing any more, but will still accept responses
        }
    }

    void doSendRequest()
    {
        if (!asyncWriteInProgress())
        {
            if (!mPendingRequests.empty())
            {
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doSendRequest: Sending request. " << mPendingRequests.size() << " requests remaining. ");
                asyncOpStarted(ASYNC_OP_TYPE_WRITE);
                mStream->Write(mPendingRequests.front(), &mOnWrite);
            }
            else if (mPendingWritesDone)
            {
                doWritesDone();
            }
        }
    }

    void doReadResponse()
    {
        if (!asyncReadInProgress())
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".doReadResponse: Reading response from stream. ");
            mResponsePtrs.push(ResponsePtr(BLAZE_NEW ResponseType));   // The response object will only be filled out when Read completes
            asyncOpStarted(ASYNC_OP_TYPE_READ);
            mStream->Read(static_cast<ResponseType*>(mResponsePtrs.back().get()), &mOnRead);
        }
    }

    BlazeRpcError waitForResponseImpl(ResponsePtr& response) override
    {
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: start");
        BlazeRpcError err = init();
        if (err != ERR_OK)
            return err;

        // Start a read if nothing is waiting:
        doReadResponse();

        // Get in line for our response
        TimeValue beforeWait = TimeValue::getTimeOfDay();
        mResponseWaitHandles.emplace(Fiber::getNextEventHandle());
        const Fiber::EventHandle& handle = mResponseWaitHandles.back();
        BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".waitForResponseImpl: Waiting on fiber id: " << handle.eventId);
        BlazeRpcError rc = Fiber::wait(handle, "OutboundBidirectionalStreamingRpc::waitForResponseImpl", getGrpcTimeout());
        
        TimeValue afterWait = TimeValue::getTimeOfDay();
        recordResponseTime(mOwner->getServiceNameShortVersion(), mRpcName.c_str(), getStatusString(rc).c_str(), afterWait - beforeWait);

        if (rc != ERR_OK)
        {
            if (rc == ERR_TIMEOUT)
                mClientContext.TryCancel();     // Even if we're TryCancelling, we may still need to keep the response around.

            return rc;
        }
        else
        {
            // Only return the response point in the case of success.
            response = eastl::move(mResponsePtrs.front());
            mResponsePtrs.pop();
            return ERR_OK;
        }
    }

    void finish()
    {
        if (!mPendingFinish)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".finish: Finish streaming. ");

            mPendingRequests = eastl::queue<RequestType>(); // Ignore any outstanding requests, they'll just fail

            asyncOpStarted(ASYNC_OP_TYPE_FINISH);
            mPendingFinish = true;
            mStream->Finish(&mStatus, &mOnFinish);
        }
    }

    void onInit(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        mInitHandles.signal(ok ? ERR_OK : ERR_SYSTEM);

        if (!asyncOpEnded(ASYNC_OP_TYPE_INIT))
            return;

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onInit: Connected to service. ");
            mInitialized = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onInit: Failed to connect to service - finishing stream. (Check onFinished for more details.)");
            incRequestsFailedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            incFailedRequests();
            finish();
        }
    }

    void onWrite(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        if (!asyncOpEnded(ASYNC_OP_TYPE_WRITE))
            return;

        if (!mPendingRequests.empty())
            mPendingRequests.pop();

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onWrite: Wrote to service. " << mPendingRequests.size() << " requests remaining. ");
            incRequestsSent();
            incRequestsSentCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            doSendRequest();
        }
        else
        {
            // Server reported error, kill the stream
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onWrite: Server indicated error - finishing stream. (Check onFinished for more details.) ");
            incFailedRequests();
            incRequestsFailedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
            finish();
        }
    }

    void onRead(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        if (mPendingShutdown)
        {
            // Signal all waiting response handles because we're shutting down
            while (!mResponseWaitHandles.empty())
            {
                const Fiber::EventHandle& handle = mResponseWaitHandles.front();
                BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Signaling fiber id: " << handle.eventId);
                Fiber::signal(handle, ERR_SYSTEM);
                mResponseWaitHandles.pop();
            }
        }

        if (!asyncOpEnded(ASYNC_OP_TYPE_READ))
            return;

        // Wake up the (oldest) fiber that was waiting for a response:
        if (!mResponseWaitHandles.empty())
        {
            const Fiber::EventHandle& handle = mResponseWaitHandles.front();
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Signaling fiber id: " << handle.eventId);
            Fiber::signal(handle, ok ? ERR_OK : ERR_SYSTEM);
            mResponseWaitHandles.pop();
        }

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: OK. ");
            incResponsesReceived();
            doReadResponse();
        }
        else
        {
            // Server is done, finish to get final status
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onRead: Server indicated it was done (error or nothing left to do) - finishing stream. (Check onFinished for more details.)");
            mFinishOnRead = true;
            finish();
        }
    }

    void onWritesDone(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        if (!asyncOpEnded(ASYNC_OP_TYPE_WRITESDONE))
            return;

        if (ok)
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onWritesDone: OK. ");
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onWritesDone: Error - finishing stream. (Check onFinished for more details.)");
            finish();
        }
    }

    void onFinish(bool ok)
    {
        // Use the waiting fiber's log context for this asyncOp
        LogContextOverride logContextOverride(mLogContext);

        bool errorOccurred = (mStatus.error_code() > grpc::UNKNOWN || ok == false);

        // grpc will return UNKNOWN when closing the stream, which doesn't constitute a failure,
        // There may be other cases where UNKNOWN would be a failure, but that would require parsing the
        // error_message or error_details strings, which can break should the error strings change,
        // Thus, a compromise here to ignore UNKNOWN as a response error
        if (mFinishOnRead && mStatus.error_code() > grpc::UNKNOWN)
        {
            incFailedResponses();
        }
            
        incCallsFinishedCount(mOwner->getServiceNameShortVersion(), mRpcName.c_str());
        
        mFinishOnRead = false;
        if (errorOccurred)
        {
            BLAZE_ERR_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }
        else
        {
            BLAZE_TRACE_LOG(Log::GRPC, logPrefix() << ".onFinish: finished with status code(" << mStatus.error_code()
                << "), message(" << mStatus.error_message().c_str() << "), details(" << mStatus.error_details().c_str() << "). ");
        }

        asyncOpEnded(ASYNC_OP_TYPE_FINISH);
    }

private:
    OutboundBidirectionalStreamingRequestRpc<StubType, RequestType, ResponseType> mCreateStream;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<RequestType, ResponseType>> mStream;

    eastl::queue<RequestType> mPendingRequests;
    // Pointers to the most recent responses:
    eastl::queue<ResponsePtr> mResponsePtrs;
    eastl::queue<Fiber::EventHandle> mResponseWaitHandles;

    TagProcessor mOnInit;
    TagProcessor mOnWrite;
    TagProcessor mOnRead;
    TagProcessor mOnWritesDone;
    TagProcessor mOnFinish;

    bool mPendingWritesDone; // Indicates that we're waiting for the writesDone async op to complete
};

} // namespace Grpc
} // namespace Blaze

#endif // BLAZE_OUTBOUND_GRPC_JOBHANDLERS_H