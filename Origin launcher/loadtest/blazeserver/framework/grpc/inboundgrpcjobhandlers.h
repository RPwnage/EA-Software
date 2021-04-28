/*************************************************************************************************/
/*!
\file   inboundgrpcjobhandlers.h

\attention
    (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!

*/
/*************************************************************************************************/
#ifndef BLAZE_INBOUND_GRPC_JOBHANDLERS_H
#define BLAZE_INBOUND_GRPC_JOBHANDLERS_H

#include "framework/blazedefines.h"
#include "EATDF/tdf.h"

#include "framework/logger.h"
#include "framework/util/locales.h"
#include "framework/component/blazerpc.h"
#include "framework/tdf/oauth_server.h"
#include "framework/grpc/grpcutils.h"
#include "framework/grpc/grpcendpoint.h"
#include "framework/grpc/inboundgrpcrequesthandler.h"
#include "framework/component/inboundrpccomponentglue.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/oauth/oauthutil.h"

#include <eathread/eathread_atomic.h>

// Note: All grpc lib calls below are thread-safe.
namespace Blaze
{
namespace Grpc
{

extern EA::Thread::AtomicInt64 gGrpcRpcCounter;

class GrpcEndpoint;

// A base class for various rpc types. We use grpc async threading model which necessitates bookkeeping around pending async operations. 
// You can have both read and write pending at the same time but you can't have more than 1 read and write pending. 

class InboundRpcBase
{
public:
    struct Frame
    {
        Frame()
            : msgNum(0)
            , locale(0)
            , userSessionId(0)
            , accessTokenType(OAuth::TOKEN_TYPE_NONE)
        {
        }

        uint32_t msgNum;
        uint32_t locale;
        InetAddress clientAddr;
        UserSessionId userSessionId;
        eastl::fixed_string<char8_t, MAX_SESSION_KEY_LENGTH> sessionKey;
        eastl::fixed_string<char8_t, MAX_SERVICENAME_LENGTH> serviceName;
        Blaze::OAuth::TokenType accessTokenType;
        eastl::string accessToken;
    };
public:
    InboundRpcBase(const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : mServerContext(nullptr)
        , mCommandInfo(cmdInfo)
        , mComponent(nullptr)
        , mIsTrustedEnvoyClient(false)
        , mIdentityContext(nullptr)
        , mEndpoint(endpoint)
        , mHandler(nullptr)
        , mGrpcRpcId(++gGrpcRpcCounter)
        , mRpcName(Blaze::FixedString64::CtorSprintf(), " [%s:%" PRId64 "] ", mCommandInfo.context, mGrpcRpcId)
        , mRefCount(0)
        , mRequestProcessingFiberId(Fiber::INVALID_EVENT_ID)
        , mDoneTagReceived(false)
        , mDoneTagResult(false)
        , mPendingAsyncOpCounter(0)
        , mAsyncReadInProgress(false)
        , mAsyncWriteInProgress(false)
        , mAsyncInitInProgress(false)
    {
        mEndpoint->updateRpcCountMetric(getRpcType(), true);

        mOnFinish = std::bind(&InboundRpcBase::onFinish, this, std::placeholders::_1);
        mOnDone = std::bind(&InboundRpcBase::onDone, this, std::placeholders::_1);

        // Because instances of this class are refCounted, its lifetime is outside of the control of the framework.  For example, some
        // component might hang onto an instance of this because it wasn't ready to perform the necessary cleanup in a non-blocking manner.
        // On the other hand, the grpc server doesn't like to have ServerContexts lying around after it is done with the request.
        // 
        // So, the situation is that we want to control the lifetime of our data, and grpc wants to control the lifetime of data relevant to it.
        // 
        // The bridge between these two worlds is this 'grpc::ServerContext mServerContext' member.  To satisfy ourselves and grpc, we
        // create a new() instance of grpc::ServerContext just before we tell grpc about ourselves, and we delete() it upon grpc
        // informing us that it is done with this request.
        // 
        // This also gives us a strong internal indication whether or not this request is still being processed by grpc, or if it's left
        // over and referenced by something else.
        mServerContext = BLAZE_NEW grpc::ServerContext();

        // The call to mServerContext->AsyncNotifyWhenDone() below gives grpc a reference
        // to 'this' via the mOnDone function(), so we need to manually bump the refCount.
        AddRef();
        // Set up the completion queue to inform us when grpc is done with this rpc.
        mServerContext->AsyncNotifyWhenDone(&mOnDone);
    }
    
    void setIdentityContext(const OAuth::IdentityContext& identityContext)
    {
        if (mIdentityContext == nullptr)
        {
            mIdentityContext.reset(BLAZE_NEW OAuth::IdentityContext());
            identityContext.copyInto(*(mIdentityContext.get()));
        }
        else
        {
            BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].setIdentityContext:" << mRpcName.c_str() << " changing identityContext when one already exists is not supported.");
        }
    }

    inline const OAuth::IdentityContext* getIdentityContext() const { return mIdentityContext.get(); }
    inline OAuth::IdentityContext* getIdentityContext() { return mIdentityContext.get(); }

    void initialize();

    void processRequestOnFiber(const google::protobuf::Message* protobufMessage);
    void cancelRequestProcessingFiber(BlazeRpcError reason);

    inline int64_t getGrpcRpcId() const { return mGrpcRpcId; }
    inline uint16_t getComponentId() const { return mCommandInfo.componentInfo->baseInfo.id; }
    inline uint16_t getCommandId() const { return mCommandInfo.commandId; }
    inline RpcType getRpcType() const { return mCommandInfo.rpcType; }
    inline const char8_t* getName() const { return mRpcName.c_str(); }

    inline const CommandInfo& getCommandInfo() const { return mCommandInfo; }
    inline Component* getComponent() const { return mComponent; }

    inline const GrpcEndpoint& getEndpoint() const { return *mEndpoint; }
    inline GrpcEndpoint& getEndpoint() { return *mEndpoint; }
    inline const grpc::ServerContext* getServerContext() const { return mServerContext; }
    inline grpc::ServerContext* getServerContext() { return mServerContext; }
    inline int32_t getRefCount() const { return mRefCount; }

    inline const Frame& getFrame() const { return mFrame; }
    inline bool getIsTrustedEnvoyClient() const { return mIsTrustedEnvoyClient; }

    RpcProtocol::Frame makeLegacyRequestFrame();
    void processLegacyRequest(const google::protobuf::Message* protobufMessage);

    Blaze::InetAddress getInetAddressFromServerContext();

    inline InboundGrpcRequestHandler* getGrpcRequestHandler() const { return mHandler; }
    inline void setGrpcRequestHandler(InboundGrpcRequestHandler* handler) { mHandler = handler; }

    bool sendResponse(const google::protobuf::Message* response)
    {
        return sendResponseImpl(response);
    }

    // This should be called for system level errors when no response is available
    bool finishWithError(const grpc::Status& error)
    {
        return finishWithErrorImpl(error);
    }

    bool shouldAbort(bool ok)
    {
        if (mEndpoint->isShuttingDown())
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].isShuttingDown: Abort " << mRpcName.c_str() << " rpc as we are shutting down.");
            if (asyncOpEnded(ASYNC_OP_TYPE_INIT))
            {
                //+ Workaround for the bug - https://github.com/grpc/grpc/issues/10136 . Should be removed when the bug is fixed.
                // See https://eadpjira.ea.com/browse/GOSOPS-175633 for additional info.
                // Turns out grpc has a bug due to which it does not call onDone callback for rpcs that have not started yet. So we workaround for it by detecting
                // shutdown (done above) and the fact that shouldAbort is called with ok being false (meaning all read/init tags have been coming down with their ok 
                // flag being false.  
                if (!ok)
                    onDone(true); // We pass true because grpc api spec is to always pass 'true' to this onDone call.
                //-
            }
            return true;
        }

        return false;
    }

    bool obfuscatePlatformInfo(EA::TDF::Tdf& tdf);
    virtual void processRequest(const google::protobuf::Message* request) = 0;
    virtual void processDone(bool cancelled) = 0;

protected:
    virtual bool sendResponseImpl(const google::protobuf::Message* response) = 0;
    virtual bool finishWithErrorImpl(const grpc::Status& error) = 0;

    inline void AddRef()
    {
        ++mRefCount;
    }

    inline void Release()
    {
        EA_ASSERT(mRefCount > 0);
        if (--mRefCount == 0)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].Release: " << mRpcName.c_str() << " is deleted.");
            mEndpoint->updateRpcCountMetric(getRpcType(), false);
            delete this;
        }
    }

    void asyncOpStarted(AsyncOpType opType)
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
        default: //Don't care about other ops
            break;
        }
    }

    // IMPORTANT: This call can result in 'this' being deleted (due to mRefCount reaching 0).
    // Returns true if the rpc processing should keep going. false otherwise.
    bool asyncOpEnded(AsyncOpType opType)
    {
        --mPendingAsyncOpCounter;

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
        default: //Don't care about other ops
            break;
        }

        return !releaseIfAllDone();
    }

    bool asyncReadInProgress() const
    {
        return mAsyncReadInProgress;
    }

    bool asyncWriteInProgress() const
    {
        return mAsyncWriteInProgress;
    }

    bool asyncInitInProgress() const
    {
        return mAsyncInitInProgress;
    }


protected:
    friend void intrusive_ptr_add_ref(InboundRpcBase*);
    friend void intrusive_ptr_release(InboundRpcBase*);

    virtual ~InboundRpcBase() {}; // Make destructor non-public to ensure that only mechanism to delete the instance is via ref-counting.

    // The application can use the ServerContext for taking into account the current 'situation' of the rpc.
    grpc::ServerContext* mServerContext;

    const CommandInfo& mCommandInfo;
    Component* mComponent;

    Frame mFrame;
    bool mIsTrustedEnvoyClient;
    eastl::unique_ptr<OAuth::IdentityContext> mIdentityContext;

    GrpcEndpoint* mEndpoint;
    InboundGrpcRequestHandler* mHandler;
    int64_t mGrpcRpcId;
    Blaze::FixedString64 mRpcName;
    int32_t mRefCount;

    Fiber::FiberId mRequestProcessingFiberId;

    // In case of an abrupt rpc ending (for example, client process exit), grpc calls OnDone prematurely even while an async operation is in progress
    // and would be notified later. An example sequence would be
    // 1. The client issues an rpc request. 
    // 2. The server handles the rpc and calls Finish with response. At this point, ServerContext::IsCancelled is NOT true. 
    //    Note that, from documentation, When using async API, it is only safe to call IsCancelled after the AsyncNotifyWhenDone tag has been delivered.
    //
    // 3. The client process abruptly exits. 
    // 4. The completion queue dispatches an OnDone tag followed by the OnFinish tag. If the application cleans up the state in OnDone, OnFinish invocation
    //    would result in undefined behavior (e.g. an access violation / SIGSEGV)
    // This actually feels like a pretty odd behavior of the grpc library (it is most likely a result of our multi-threaded usage) so we account for
    // that by keeping track of whether the OnDone was called earlier. As far as the application is concerned, the rpc is only 'done' when no async Ops are pending. 
    bool mDoneTagReceived;
    bool mDoneTagResult;

    TagProcessor mOnFinish;
    TagProcessor mOnDone;

private:
    void processRequestInternal(const google::protobuf::Message* protobufMessage);
    static void _processRequestInternal(InboundRpcBasePtr rpcPtr, const google::protobuf::Message* protobufMessage)
    {
        rpcPtr->processRequestInternal(protobufMessage);
    }

    void onFinish(bool ok)
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].onFinish:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_FINISH] << " Op result(" << ok << ").");
        asyncOpEnded(ASYNC_OP_TYPE_FINISH);
    }

    void onDone(bool ok)
    {
        if (!mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].onDone:" << mRpcName.c_str() << " async done tag received." << " Op result(" << ok << "). Pending async operations(" << mPendingAsyncOpCounter << "). IsCancelled(" << mServerContext->IsCancelled() << ").");

            mDoneTagReceived = true;
            mDoneTagResult = ok;

            if (mHandler)
                mHandler->onInboundRpcDone(*this, mServerContext->IsCancelled());

            releaseIfAllDone();
        }
        else
        {
            // If onDone is called more than once, most likely https://github.com/grpc/grpc/issues/10136 bug is now fixed on the version of grpc we have.
            // Or we could be running into another edge case bug noticed in the above bug description.
            // If the bug is fixed, the workaround in shouldAbort call should be undone.
            BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].onDone:" << mRpcName.c_str() << " onDone called more than once. See comments in the code." );
        }
    }

    // IMPORTANT: This call can result in 'this' being deleted (due to mRefCount reaching 0).
    // Returns true if the rpc is done all async ops *and* the grpc has sent us the done tag, false otherwise.
    bool releaseIfAllDone()
    {
        if (mDoneTagReceived && (mPendingAsyncOpCounter == 0))
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].releaseIfAllDone:" << mRpcName.c_str() << " pending async op finished and grpc lib is done.");

            // See comments in InboundRpcBase::ctor() for details on the ServerContext lifetime management.
            delete mServerContext;
            mServerContext = nullptr;

            Release();

            return true;
        }

        return false;
    }

private:
    int32_t mPendingAsyncOpCounter;
    bool mAsyncReadInProgress;
    bool mAsyncWriteInProgress;
    bool mAsyncInitInProgress;

};

using InboundRpcBasePtr = eastl::intrusive_ptr<InboundRpcBase>;
inline void intrusive_ptr_add_ref(InboundRpcBase* ptr) { ptr->AddRef(); }
inline void intrusive_ptr_release(InboundRpcBase* ptr) { ptr->Release(); }

// The application code communicates with our utility classes using following handlers. 

// typedefs.
// In grpc async model, an application has to explicitly ask the grpc server to start handling an incoming rpc on a particular service.
// createRpc is called when an outstanding RpcJob starts serving an incoming rpc and we need to create the next rpc of this type to service 
// further incoming rpcs.
using CreateRpc = std::function<void(grpc::Service*, grpc::ServerCompletionQueue*, GrpcEndpoint*)>;

// A new request has come in for this rpc. processIncomingRequest is called to handle it. Note that with streaming rpcs, a request can 
// come in multiple times. 
using ProcessIncomingRequest = std::function<void(InboundRpcBase&, const google::protobuf::Message*)>;

// The actual queuing function on the generated service. This is called when an instance of rpc job is created. This is a different specialization
// for each rpc type.
template<typename ServiceType, typename RequestType, typename ResponseType>
using UnaryRequestRpc = std::function<void(ServiceType*, grpc::ServerContext*, RequestType*, grpc::ServerAsyncResponseWriter<ResponseType>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

template<typename ServiceType, typename RequestType, typename ResponseType>
using ServerStreamingRequestRpc = std::function<void(ServiceType*, grpc::ServerContext*, RequestType*, grpc::ServerAsyncWriter<ResponseType>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

template<typename ServiceType, typename RequestType, typename ResponseType>
using ClientStreamingRequestRpc = std::function<void(ServiceType*, grpc::ServerContext*, grpc::ServerAsyncReader<ResponseType, RequestType>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

template<typename ServiceType, typename RequestType, typename ResponseType>
using BidirectionalStreamingRequestRpc = std::function<void(ServiceType*, grpc::ServerContext*, grpc::ServerAsyncReaderWriter<ResponseType, RequestType>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void *)>;

// Each Rpc class wraps the template based access to grpc core lib. For Unary rpcs, however, a good portion of the code does not need to be templated so we isolate
// that in per rpc base class. While code becomes slightly harder to read, the savings are substantial (in 10s of MBs/about 15% of binary size at the time of writing).

// Note that for logging, we continue to use "UnaryRpc" rather than "UnaryRpcBase" as existence of UnaryRpcBase is just an optimization and the class is not expected to be
// instantiated by any other means.

// For rpc types other than unary, there isn't much that can be isolated for the non-templated access. So we leave them as is. Fortunately, the use cases for these rpcs are
// very limited overall.
class UnaryRpcBase : public InboundRpcBase
{
protected:
    bool setupSendResponse(const google::protobuf::Message* response)
    {
        if (response == nullptr)
        {
            BLAZE_ASSERT_LOG(Log::GRPC, "If no response is available, use InboundRpcBase::finishWithError.");
            return false;
        }
        
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].setupSendResponse:" << mRpcName.c_str() << " ignored send request as rpc was already done.");
            return false;
        }

        BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].setupSendResponse:" << mRpcName.c_str() << "response sent. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);

        return true;
    }

    bool setupFinishWithError(const grpc::Status& error)
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].setupFinishWithError:" << mRpcName.c_str() << " ignored finishWithError request as rpc was already done.");
            return false;
        }

        BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].setupFinishWithError:" << mRpcName.c_str() << "had system level error(" << error.error_message().c_str() << "|" << error.error_code() << "). " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);

        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        return true;
    }
    
    void onRead(bool ok)
    {
        if (shouldAbort(ok))
            return;

        if (ok)
        {
            mCreateRpc(mServicePtr, mCQ, mEndpoint);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[UnaryRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op failed. No rpc of this type will be queued again.");
        }

        if (asyncOpEnded(ASYNC_OP_TYPE_INIT))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op successful. Process incoming request.");
                mProcessIncomingRequest(*this, mRequestPtr);
            }
        }
    }

protected:
    UnaryRpcBase(grpc::Service* service, grpc::ServerCompletionQueue* cq, 
        CreateRpc createRpc, ProcessIncomingRequest processIncomingRequest,
        google::protobuf::Message* requestPtr,
        const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : InboundRpcBase(cmdInfo, endpoint)
        , mServicePtr(service)
        , mCQ(cq)
        , mCreateRpc(createRpc)
        , mProcessIncomingRequest(processIncomingRequest)
        , mRequestPtr(requestPtr)
    {
        // create TagProcessors that we'll use to interact with grpc CompletionQueue
        mOnRead = std::bind(&UnaryRpcBase::onRead, this, std::placeholders::_1);
    }
    
    grpc::Service* mServicePtr;
    grpc::ServerCompletionQueue* mCQ;

    TagProcessor mOnRead;

    CreateRpc mCreateRpc;
    ProcessIncomingRequest mProcessIncomingRequest;
    
    google::protobuf::Message* mRequestPtr;
};

/*
We implement UnaryRpc, ServerStreamingRpc, ClientStreamingRpc and BidirectionalStreamingRpc below.
*/

template<typename ServiceType, typename RequestType, typename ResponseType>
class UnaryRpc : public UnaryRpcBase
{
public:
    UnaryRpc(ServiceType* service, grpc::ServerCompletionQueue* cq, 
        CreateRpc createRpc, ProcessIncomingRequest processIncomingRequest, UnaryRequestRpc<ServiceType, RequestType, ResponseType> requestRpc,
        const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : UnaryRpcBase(service, cq, createRpc, processIncomingRequest, &mRequest, cmdInfo, endpoint)
        , mService(service)
        , mResponder(mServerContext)
        , mRequestRpc(requestRpc)
    {
        // issue the async request needed by grpc to start handling this rpc.
        BLAZE_TRACE_LOG(Log::GRPC, "[UnaryRpc].Ctor:" << mRpcName.c_str() << "created. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_INIT]);
        
        asyncOpStarted(ASYNC_OP_TYPE_INIT); 
        mRequestRpc(mService, mServerContext, &mRequest, &mResponder, mCQ, mCQ, &mOnRead);
    }

private:

    bool sendResponseImpl(const google::protobuf::Message* response) override
    {
        if (setupSendResponse(response))
        {
            // We need to make a copy as grpc expects the object to be around until it comes back with the finish event.
            mResponse = *(static_cast<const ResponseType*>(response)); 
            mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);
            return true;
        }
        
        return false;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        if (setupFinishWithError(error))
        {
            mResponder.FinishWithError(error, &mOnFinish);
            return true;
        }

        return false;
    }
   
private:

    ServiceType* mService;
    grpc::ServerAsyncResponseWriter<ResponseType> mResponder;

    RequestType mRequest;
    ResponseType mResponse;

    UnaryRequestRpc<ServiceType, RequestType, ResponseType> mRequestRpc;
};


template<typename ServiceType, typename RequestType, typename ResponseType>
class LegacyBlazeRpc : public UnaryRpc<ServiceType, RequestType, ResponseType>
{
public:
    using UnaryRpc<ServiceType, RequestType, ResponseType>::UnaryRpc;

private:
    void processRequest(const google::protobuf::Message* request) override
    {
        InboundRpcBase::processLegacyRequest(request);
    }
    void processDone(bool cancelled) override
    {
        if (cancelled)
            UnaryRpcBase::cancelRequestProcessingFiber(ERR_CANCELED);
    }
};


template<typename ServiceType, typename RequestType, typename ResponseType>
class ServerStreamingRpc : public InboundRpcBase
{
public:
    ServerStreamingRpc(ServiceType* service, grpc::ServerCompletionQueue* cq,
        CreateRpc createRpc, ProcessIncomingRequest processIncomingRequest, ServerStreamingRequestRpc<ServiceType, RequestType, ResponseType> requestRpc,
        const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : InboundRpcBase(cmdInfo, endpoint)
        , mService(service)
        , mCQ(cq)
        , mResponder(mServerContext)
        , mCreateRpc(createRpc)
        , mProcessIncomingRequest(processIncomingRequest)
        , mRequestRpc(requestRpc)
        , mServerStreamingDone(false)
    {
        // create TagProcessors specific to ServerStreaming RPCs that we'll use to interact with grpc CompletionQueue
        mOnRead = std::bind(&ServerStreamingRpc::onRead, this, std::placeholders::_1);
        mOnWrite = std::bind(&ServerStreamingRpc::onWrite, this, std::placeholders::_1);

        // finally, issue the async request needed by grpc to start handling this rpc.
        BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].Ctor:" << mRpcName.c_str() << "created. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_INIT]);
        
        asyncOpStarted(ASYNC_OP_TYPE_INIT);
        mRequestRpc(mService, mServerContext, &mRequest, &mResponder, mCQ, mCQ, &mOnRead);
    }

protected:
    RequestType mRequest;

private:

    // grpc can only do one async write at a time but that is very inconvenient from the application point of view.
    // So we buffer the response below in a queue if we have not received the completion tag for previous write op. 
    // The application can send a null response in order to indicate the completion of server side streaming. 
    bool sendResponseImpl(const google::protobuf::Message* responseMsg) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].sendResponseImpl:" << mRpcName.c_str() << " ignored send request as rpc was already done.");
            return false;
        }
        
        auto response = static_cast<const ResponseType*>(responseMsg);
        
        if (response != nullptr)
        {
            mResponseQueue.push_back(*response);

            if (!asyncWriteInProgress())
            {
                doSendResponse();
            }
        }
        else
        {
            mServerStreamingDone = true;

            if (!asyncWriteInProgress())
            {
                doFinish();
            }
        }

        return true;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << " ignored finishWithError request as rpc was already done.");
            return false;
        }
        
        BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << "had system level error(" << error.error_message().c_str() << "|" << error.error_code() << "). " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);

        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(error, &mOnFinish);

        return true;
    }

    void doSendResponse()
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].doSendResponse:" << mRpcName.c_str() << "response sent. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_WRITE]);
        
        asyncOpStarted(ASYNC_OP_TYPE_WRITE);
        mResponder.Write(mResponseQueue.front(), &mOnWrite);
    }

    void doFinish()
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].doFinish:" << mRpcName.c_str() << "finished streaming. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(grpc::Status::OK, &mOnFinish);
    }

    void onRead(bool ok)
    {
        if (shouldAbort(ok))
            return;

        if (ok)
        {
            mCreateRpc(mService, mCQ, mEndpoint);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[ServerStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op failed. No rpc of this type will be queued again.");
        }
        
        if (asyncOpEnded(ASYNC_OP_TYPE_INIT))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op successful. Process incoming request.");
                mProcessIncomingRequest(*this, &mRequest);
            }
        }
    }

    void onWrite(bool ok)
    {
        if (asyncOpEnded(ASYNC_OP_TYPE_WRITE))
        {
            // Get rid of the message that just finished.
            mResponseQueue.pop_front();

            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].onWrite:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_WRITE] << " Op successful.");
                
                if (!mResponseQueue.empty()) // If we have more messages waiting to be sent, send them.
                {
                    doSendResponse();
                }
                else if (mServerStreamingDone) // Previous write completed and we did not have any pending write. If the application has finished streaming responses, finish the rpc processing.
                {
                    doFinish();
                }
            }
            else 
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ServerStreamingRpc].onWrite:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_WRITE] << " Op failed. Rpc will finish when all pending async tags are received.");
            }
        }
    }

private:
    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    grpc::ServerAsyncWriter<ResponseType> mResponder;

    CreateRpc mCreateRpc;
    ProcessIncomingRequest mProcessIncomingRequest;
    ServerStreamingRequestRpc<ServiceType, RequestType, ResponseType> mRequestRpc;

    TagProcessor mOnRead;
    TagProcessor mOnWrite;

    std::list<ResponseType> mResponseQueue;
    bool mServerStreamingDone;
};


template<typename ServiceType, typename RequestType, typename ResponseType>
class NotificationSubscriptionRpc : public ServerStreamingRpc<ServiceType, RequestType, ResponseType>
{
public:
    using ServerStreamingRpc<ServiceType, RequestType, ResponseType>::ServerStreamingRpc;

    void processRequest(const google::protobuf::Message* request) override
    {
        InboundRpcBase::processLegacyRequest(request);
    }
    void processDone(bool cancelled) override
    {
        //nothing to do here.
    }
};


template<typename ServiceType, typename RequestType, typename ResponseType>
class ClientStreamingRpc : public InboundRpcBase
{
public:
    ClientStreamingRpc(ServiceType* service, grpc::ServerCompletionQueue* cq, 
        CreateRpc createRpc, ProcessIncomingRequest processIncomingRequest, ClientStreamingRequestRpc<ServiceType, RequestType, ResponseType> requestRpc,
        const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : InboundRpcBase(cmdInfo, endpoint)
        , mService(service)
        , mCQ(cq)
        , mResponder(mServerContext)
        , mCreateRpc(createRpc)
        , mProcessIncomingRequest(processIncomingRequest)
        , mRequestRpc(requestRpc)
        , mClientStreamingDone(false)
        , mReadNextRequestCallPending(false)
    {
        // create TagProcessors that we'll use to interact with grpc CompletionQueue
        mOnInit = std::bind(&ClientStreamingRpc::onInit, this, std::placeholders::_1);
        mOnRead = std::bind(&ClientStreamingRpc::onRead, this, std::placeholders::_1);

        // finally, issue the async request needed by grpc to start handling this rpc.
        BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].Ctor:" << mRpcName.c_str() << "created. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_INIT]);

        asyncOpStarted(ASYNC_OP_TYPE_INIT);
        mRequestRpc(mService, mServerContext, &mResponder, mCQ, mCQ, &mOnInit);
    }

protected:
    void readNextRequest()
    {
        if (!mReadNextRequestCallPending)
            return;
        mReadNextRequestCallPending = false;

        if (mServerContext == nullptr)
            return; // Grpc has told us the re  quest is done with, we can no longer make any calls in to grpc.

        BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].readNextRequest:" << mRpcName.c_str() << sAsyncOpStartedMsg[ASYNC_OP_TYPE_READ] << " Command handler is ready for next client streamed request.");
        asyncOpStarted(ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
    }

private:

    bool sendResponseImpl(const google::protobuf::Message* responseMsg) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].sendResponseImpl:" << mRpcName.c_str() << " ignored send request as rpc was already done.");
            return false;
        }
        
        auto response = static_cast<const ResponseType*>(responseMsg);
        
        EA_ASSERT_MSG(response, "If no response is available, use InboundRpcBase::finishWithError.");
        
        if (response == nullptr)
        {
            return false;
        }

        if (!mClientStreamingDone)
        {
            // It does not make sense for server to finish the rpc before client has streamed all the requests. Supporting this behavior could lead to writing error-prone
            // code so it is specifically disallowed. 
            EA_ASSERT_MSG(false, "If you want to cancel, use InboundRpcBase::finishWithError with grpc::Cancelled status.");
            return false;
        }

        mResponse = *response;

        BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].sendResponseImpl:" << mRpcName.c_str() << "response sent. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

        return true;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << " ignored finishWithError request as rpc was already done.");
            return false;
        }
        
        BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << "had system level error(" << error.error_message().c_str() << "|" << error.error_code() << "). " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.FinishWithError(error, &mOnFinish);

        return true;
    }

    void onInit(bool ok)
    {
        if (shouldAbort(ok))
            return;

        if (ok)
        {
            mCreateRpc(mService, mCQ, mEndpoint);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[ClientStreamingRpc].onInit:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op failed. No rpc of this type will be queued again.");
        }
        
        if (asyncOpEnded(ASYNC_OP_TYPE_INIT))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].onInit:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op successful. "<< sAsyncOpStartedMsg[ASYNC_OP_TYPE_READ]);
                asyncOpStarted(ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
        }
    }

    void onRead(bool ok)
    {
        if (asyncOpEnded(ASYNC_OP_TYPE_READ))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_READ] << " New client request. Process incoming request.");

                // we don't start the next async read yet because we only have one mRequest, and we can run into concurrency issues if the previous request
                // is still being processed on a fiber that made a blocking call (e.g. a db query, etc.). The code implementing the handler for this
                // request needs to call readNextRequest() when it's done with the previous request.
                mReadNextRequestCallPending = true;

                if (InboundRpcBase::mHandler != nullptr)
                {
                    // We have already been attached to an InboundGrpcRequestHandler which means we have already processed the
                    // first request received from the client. The current request must be a streamed request sent from the client.
                    // Streamed requests are handled internally by the InboundRpcBase derived classes rather than the InboundGrpcRequestHandler.
                    InboundRpcBase::processRequestOnFiber(&mRequest);
                }
                else
                {
                    // inform application that a new request has come in
                    mProcessIncomingRequest(*this, &mRequest);
                }
            }
            else
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_READ] << " Client request streaming done.");
                
                mClientStreamingDone = true;

                InboundRpcBase::processRequestOnFiber(nullptr);
            }
        }
    }

private:

    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    grpc::ServerAsyncReader<ResponseType, RequestType> mResponder;

    RequestType mRequest;
    ResponseType mResponse;

    CreateRpc mCreateRpc;
    ProcessIncomingRequest mProcessIncomingRequest;
    ClientStreamingRequestRpc<ServiceType, RequestType, ResponseType> mRequestRpc;

    TagProcessor mOnInit;
    TagProcessor mOnRead;

    bool mClientStreamingDone;
    bool mReadNextRequestCallPending;
};



template<typename ServiceType, typename RequestType, typename ResponseType>
class BidirectionalStreamingRpc : public InboundRpcBase
{
public:
    BidirectionalStreamingRpc(ServiceType* service, grpc::ServerCompletionQueue* cq,
        CreateRpc createRpc, ProcessIncomingRequest processIncomingRequest, BidirectionalStreamingRequestRpc<ServiceType, RequestType, ResponseType> requestRpc,
        const CommandInfo& cmdInfo, GrpcEndpoint* endpoint)
        : InboundRpcBase(cmdInfo, endpoint)
        , mService(service)
        , mCQ(cq)
        , mResponder(mServerContext)
        , mCreateRpc(createRpc)
        , mProcessIncomingRequest(processIncomingRequest)
        , mRequestRpc(requestRpc)
        , mServerStreamingDone(false)
        , mClientStreamingDone(false)
        , mReadNextRequestCallPending(false)
    {
        // create TagProcessors that we'll use to interact with grpc CompletionQueue
        mOnInit = std::bind(&BidirectionalStreamingRpc::onInit, this, std::placeholders::_1);
        mOnRead = std::bind(&BidirectionalStreamingRpc::onRead, this, std::placeholders::_1);
        mOnWrite = std::bind(&BidirectionalStreamingRpc::onWrite, this, std::placeholders::_1);

        // finally, issue the async request needed by grpc to start handling this rpc.
        BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].Ctor:" << mRpcName.c_str() << "created. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_INIT]);

        asyncOpStarted(ASYNC_OP_TYPE_INIT);
        mRequestRpc(mService, mServerContext, &mResponder, mCQ, mCQ, &mOnInit);
    }

protected:
    void readNextRequest()
    {
        if (!mReadNextRequestCallPending)
            return;
        mReadNextRequestCallPending = false;

        BLAZE_TRACE_LOG(Log::GRPC, "[ClientStreamingRpc].readNextRequest:" << mRpcName.c_str() << sAsyncOpStartedMsg[ASYNC_OP_TYPE_READ] << " Command handler is ready for next client streamed request.");

        asyncOpStarted(ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
    }

private:

    bool sendResponseImpl(const google::protobuf::Message* responseMsg) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].sendResponseImpl:" << mRpcName.c_str() << " ignored send request as rpc was already done.");
            return false;
        }
        
        auto response = static_cast<const ResponseType*>(responseMsg);
        
        if (response == nullptr && !mClientStreamingDone)
        {
            // It does not make sense for server to finish the rpc before client has streamed all the requests. Supporting this behavior could lead to writing error-prone
            // code so it is specifically disallowed.   
            EA_ASSERT_MSG(false, "If you want to cancel, use InboundRpcBase::finishWithError with grpc::Cancelled status.");
            return false;
        }

        if (response != nullptr)
        {
            mResponseQueue.push_back(*response); // We need to make a copy of the response because we need to maintain it until we get a completion notification. 

            if (!asyncWriteInProgress())
            {
                doSendResponse();
            }
        }
        else
        {
            mServerStreamingDone = true;

            if (!asyncWriteInProgress()) // Kick the async op if our state machine is not going to be kicked from the completion queue
            {
                doFinish();
            }
        }

        return true;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        if (mDoneTagReceived)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << " ignored finishWithError request as rpc was already done.");
            return false;
        }
        
        BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].finishWithErrorImpl:" << mRpcName.c_str() << "had system level error(" << error.error_message().c_str() << "|" << error.error_code() << "). " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(error, &mOnFinish);

        return true;
    }

    void onInit(bool ok)
    {
        if (shouldAbort(ok))
            return;

        if (ok)
        {
            mCreateRpc(mService, mCQ, mEndpoint);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onInit:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op failed. No rpc of this type will be queued again.");
        }

        if (asyncOpEnded(ASYNC_OP_TYPE_INIT))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onInit:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_INIT] << " Op successful. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_READ]);
                asyncOpStarted(ASYNC_OP_TYPE_READ);
                mResponder.Read(&mRequest, &mOnRead);
            }
        }
    }

    void onRead(bool ok)
    {
        if (asyncOpEnded(ASYNC_OP_TYPE_READ))
        {
            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_READ] << " New client request. Process incoming request.");

                // we don't start the next async read yet because we only have one mRequest, and we can run into concurrency issues if the previous request
                // is still being processed on a fiber that made a blocking call (e.g. a db query, etc.). The code implementing the handler for this
                // request needs to call readNextRequest() when it's done with the previous request.
                mReadNextRequestCallPending = true;

                if (InboundRpcBase::mHandler != nullptr)
                {
                    // We have already been attached to an InboundGrpcRequestHandler which means we have already processed the
                    // first request received from the client. The current request must be a streamed request sent from the client.
                    // Streamed requests are handled internally by the InboundRpcBase derived classes rather than the InboundGrpcRequestHandler.
                    InboundRpcBase::processRequestOnFiber(&mRequest);
                }
                else
                {
                    // inform application that a new request has come in
                    mProcessIncomingRequest(*this, &mRequest);
                }
            }
            else
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onRead:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_READ] << " Client request streaming done.");
               
                mClientStreamingDone = true;

                InboundRpcBase::processRequestOnFiber(nullptr);
            }
        }
    }

    void onWrite(bool ok)
    {
        if (asyncOpEnded(ASYNC_OP_TYPE_WRITE))
        {
            // Get rid of the message that just finished. 
            mResponseQueue.pop_front();

            if (ok)
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onWrite:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_WRITE] << " Op successful.");
                
                if (!mResponseQueue.empty()) // If we have more messages waiting to be sent, send them.
                {
                    doSendResponse();
                }
                else if (mServerStreamingDone) // Previous write completed and we did not have any pending write. If the application indicated a done operation, finish the rpc processing.
                {
                    doFinish();
                }
            }
            else
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].onWrite:" << mRpcName.c_str() << sAsyncOpEndedMsg[ASYNC_OP_TYPE_WRITE] << " Op failed. Rpc will finish when all pending async tags are received.");
            }
        }
    }

    void doSendResponse()
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].doSendResponse:" << mRpcName.c_str() << "response sent. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_WRITE]);
        
        asyncOpStarted(ASYNC_OP_TYPE_WRITE);
        mResponder.Write(mResponseQueue.front(), &mOnWrite);
    }

    void doFinish()
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[BidirectionalStreamingRpc].doFinish:" << mRpcName.c_str() << "finished streaming. " << sAsyncOpStartedMsg[ASYNC_OP_TYPE_FINISH]);
        
        asyncOpStarted(ASYNC_OP_TYPE_FINISH);
        mResponder.Finish(grpc::Status::OK, &mOnFinish);
    }

private:
    ServiceType* mService;
    grpc::ServerCompletionQueue* mCQ;
    grpc::ServerAsyncReaderWriter<ResponseType, RequestType> mResponder;

    RequestType mRequest;

    CreateRpc mCreateRpc;
    ProcessIncomingRequest mProcessIncomingRequest;
    BidirectionalStreamingRequestRpc<ServiceType, RequestType, ResponseType> mRequestRpc;

    TagProcessor mOnInit;
    TagProcessor mOnRead;
    TagProcessor mOnWrite;

    std::list<ResponseType> mResponseQueue;
    bool mServerStreamingDone;
    bool mClientStreamingDone;
    bool mReadNextRequestCallPending;
};

} //namespace Grpc
} //namespace Blaze


#endif // BLAZE_INBOUND_GRPC_JOBHANDLERS_H
