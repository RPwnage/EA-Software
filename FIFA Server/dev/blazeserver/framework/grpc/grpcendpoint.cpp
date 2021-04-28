/*************************************************************************************************/
/*!
    \file   grpcendpoint.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/grpc/grpcendpoint.h"
#include "framework/grpc/inboundgrpcrequesthandler.h"
#include "framework/grpc/grpcutils.h"
#include "framework/connection/selector.h"
#include "framework/connection/sslcontext.h"
#include "framework/connection/endpointhelper.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsmasterimpl.h"

#include "framework/tdf/frameworkconfigtypes_server.h"

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>

// Note: Completion Queue is thread-safe.  

namespace Blaze
{
namespace Grpc
{

static const int64_t REQUESTHANDLER_INACTIVITY_CHECK_PERIOD_US = 60 * SECONDS_TO_MICROSECONDS; // 60s
static const int64_t SHUTDOWN_WAIT_PERIOD_US = 45 * SECONDS_TO_MICROSECONDS; //45s - Max limit, not necessarily 45s always.

GrpcEndpoint::GrpcEndpoint(const char8_t* name, uint16_t port, Selector& selector)
    : BlazeThread(eastl::string(eastl::string::CtorSprintf(), "%s:%s", gController->getInstanceName(), name).c_str(), BlazeThread::GRPC_CQ, 1024 * 1024)
    , mName(name)
    , mPort(port)
    , mSelector(selector)
    , mEndpointMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::endpoint, name))
    , mGrpcMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::inboundgrpc, name))
    , mRpcAuthorizer(mEndpointMetricsCollection, "endpoint.rpcAuthorizationFailureCount")
    , mShuttingDown(false)
    , mTempSessionKeyCounter(0)
    , mRequestHandlersPendingRemoval(false)
    , mExpireRequestHandlersTimerId(INVALID_TIMER_ID)
    , mAllowedIpAddressAlertInfo(1000)
    , mFiberPriority(0)
    , mTotalExceededRpcFiberLimitDropped(mEndpointMetricsCollection, "endpoint.exceededRpcFiberLimitDropped")
    , mTotalExceededRpcFiberLimitPermitted(mEndpointMetricsCollection, "endpoint.exceededRpcFiberLimitPermitted")
    , mCountTotalRejects(mEndpointMetricsCollection, "endpoint.rejects")
    , mNumRequestHandlers(mEndpointMetricsCollection, "endpoint.connections")
    , mNumUnaryRpcs(mGrpcMetricsCollection, "inboundgrpc.unaryRpcs")
    , mNumServerStreamingRpcs(mGrpcMetricsCollection, "inboundgrpc.serverStreamingRpcs")
    , mNumClientStreamingRpcs(mGrpcMetricsCollection, "inboundgrpc.clientStreamingRpcs")
    , mNumBidiStreamingRpcs(mGrpcMetricsCollection, "inboundgrpc.bidiStreamingRpcs")
    , mEndpointDeletedSentinel(false) // Only used for debugging.
    , mShutdownThread()
    , mPendingTagCount(0)
{

}

GrpcEndpoint::~GrpcEndpoint()
{
    mEndpointDeletedSentinel = true;

    BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].~GrpcEndpoint: endpoint is deleted.");
    
    BLAZE_ASSERT_COND_LOG(Log::GRPC, mSessionKeyToRequestHandlerMap.size() == 0, "No request handler should be alive at endpoint deletion.");
    BLAZE_ASSERT_COND_LOG(Log::GRPC, mPendingRemovalQueue.size() == 0, "No request handler should be queued for deletion at endpoint deletion.");
}

void GrpcEndpoint::beginShutdown()
{
    EA_ASSERT_MSG(!mShuttingDown, "Already shutting down!");

    BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].beginShutdown: Pending tags in queue (" << mPendingTagCount <<"). Outstanding grpc request handlers ("<<  mSessionKeyToRequestHandlerMap.size() << ").");

    mShuttingDown = true;

    mSelector.cancelTimer(mExpireRequestHandlersTimerId);
    mExpireRequestHandlersTimerId = INVALID_TIMER_ID;

    // Make grpc server stop sending us new work and wait until deadline for all the current pending work to finish.
    // Launch a thread as the mServer->Shutdown call is potentially blocking. Putting this on a different thread gives us a 
    // chance to finish the pending rpcs in our selector loop. 
    mShutdownThread = std::thread(&GrpcEndpoint::startGrpcServerShutdown, this, SHUTDOWN_WAIT_PERIOD_US);
}

void GrpcEndpoint::shutdown()
{
    EA_ASSERT_MSG(mShuttingDown, "should have already called beginShutdown");

    if (mSessionKeyToRequestHandlerMap.empty() && !mRequestHandlersPendingRemoval)
    {
        BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].shutdown: No request handlers on the endpoint. Proceed with shutdown.");
    }
    else
    {
        BLAZE_WARN_LOG(Log::GRPC, "[GrpcEndpoint].shutdown: Pending Request handlers ("<< mSessionKeyToRequestHandlerMap.size()<<"). Pending removal of request handlers ("<<mRequestHandlersPendingRemoval<<"). Force shutdown all the outstanding rpcs.");

        for (auto& entry : mSessionKeyToRequestHandlerMap)
            entry.second->shutdown();

        if (!mSessionKeyToRequestHandlerMap.empty())
        {
            BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].shutdown: Wait for the shutdown to finish...");
            Fiber::getAndWait(mAllRequestHandlersDoneEvent, "GrpcEndpoint::shutdown wait for all request handlers to be finished after shutdown.");
            BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].shutdown: shutdown of request handlers finished. Proceeed with shutdown. ");
        }
    }

    BLAZE_ASSERT_COND_LOG(Log::GRPC, mSessionKeyToRequestHandlerMap.empty(), "Expecting empty request handler map");
    BLAZE_ASSERT_COND_LOG(Log::GRPC, mPendingTagCount == 0, "Expecting no pending tags");

    // join the grpc server begin shutdown thread
    mShutdownThread.join();

    BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].shutdown: Grpc Server shutdown complete.");

    // From official docs, always shutdown the completion queue after the server.
    // If any more new work/tag is added to completion queue after this call, it will assert.
    if (mCQ != nullptr)
        mCQ->Shutdown();

    // Join the completion queue thread
    stop();

   
}

bool GrpcEndpoint::initialize(const EndpointConfig::GrpcEndpointConfig& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    config.copyInto(mConfig);

    mAllowedIpNetworks.reset(BLAZE_NEW Blaze::InetFilter());
    if (!mAllowedIpNetworks->initialize(true /*allowIfEmpty*/, mConfig.getTrust()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: invalid allowedIpList configuration for endpoint '" << getName() << "'");
        return false;
    }

    mTrustedClientIpNetworks.reset(BLAZE_NEW Blaze::InetFilter());
    if (!mTrustedClientIpNetworks->initialize(false /*allowIfEmpty*/, mConfig.getTrustedClientIpList()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: invalid trustedClientIpList configuration for endpoint '" << getName() << "'");
        return false;
    }

    if (mConfig.getHasInternalNicAccess().empty() && mConfig.getBind() != BIND_INTERNAL)
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: hasInternalNicAccess cannot be empty for non-internal endpoint '" << getName() << "'");
        return false;
    }
    else
    {
        mInternalNicAccessNetworks.reset(BLAZE_NEW Blaze::InetFilter());
        if (!mInternalNicAccessNetworks->initialize(false /*allowIfEmpty*/, mConfig.getHasInternalNicAccess()))
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: invalid hasInternalNicAccess configuration for endpoint '" << getName() << "'");
            return false;
        }
    }

    if (!config.getEnvoyXfccTrustedHashes().empty())
    {
        if (config.getEnvoyResourceName()[0] == '\0')
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: envoyXfccTrustedHashes requires an Envoy-enabled endpoint '" << getName() << "'");
            return false;
        }
        else if (config.getBind() != BIND_INTERNAL)
        {
            // Requires internal endpoint or mTLS trustedHashes to ensure validity of x-forwarded-client-cert header. 
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: envoyXfccTrustedHashes requires an internal or mTLS endpoint '" << getName() << "'");
            return false;
        }
        else
        {
            for (const auto& trustedHash : config.getEnvoyXfccTrustedHashes())
            {
                mEnvoyXfccTrustedHashes.insert(trustedHash.c_str());
            }
        }
    }

    if (mConfig.getRateLimiter()[0] != '\0')
    {
        RateConfig::RateLimitersMap::const_iterator rateLimiterIt = rateConfig.getRateLimiters().find(mConfig.getRateLimiter());
        if (!mRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second))
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: Failed to initialize RateLimiter '" << mConfig.getRateLimiter() << "' "
                "for endpoint '" << getName() << "'");
            return false;
        }
    }

    if (gSslContextManager->get(mConfig.getSslContext()) == nullptr)
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: SSL context '" << mConfig.getSslContext() << "' not found for endpoint '" << getName() << "'");
        return false;
    }
    
    mRpcAuthorizer.initializeRpcAuthorizationList(config.getRpcWhiteList(), config.getRpcBlackList(), rpcControlLists, platformSpecificControlLists);

    mExpireRequestHandlersTimerId = mSelector.scheduleTimerCall(TimeValue::getTimeOfDay() + REQUESTHANDLER_INACTIVITY_CHECK_PERIOD_US, this, &GrpcEndpoint::expireRequestHandlers, "GrpcEndpoint::expireRequestHandlers");
    
    BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].initialize: initialized endpoint '" << getName() << "'");

    return true;
}

void GrpcEndpoint::startGrpcServerShutdown(int64_t timespanUS)
{
    // When we call shutdown on the server, all tags in the completion queue start showing up with their 'ok' bool being false.
    if (mServer != nullptr)
        mServer->Shutdown(gpr_time_add(gpr_now(GPR_CLOCK_MONOTONIC), gpr_time_from_micros(timespanUS, GPR_TIMESPAN)));
}

void GrpcEndpoint::expireRequestHandlers()
{
    // Periodically, clean up the request handlers whose user sessions no longer exist. The user session may not exist because either the user logged out or their session was cleaned up
    // due to inactivity (driven by sessionCleanupTimeout in usersessions.cfg).
    // Note that the request handlers not attached to a user session are deleted immediately after executing their rpc and don't stuck around to get cleaned up here later. 
    BLAZE_TRACE_LOG(Log::GRPC, "[GrpcEndpoint].expireRequestHandlers: Timer fired, clean up the request handlers whose user sessions no longer exists.");
    
    if (mShuttingDown)
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[GrpcEndpoint].expireRequestHandlers: ignore timer as already shutting down.");
        return;
    }

    // It's okay to iterate through the container as the destroy call below queues up the destruction by putting items in a list (so you 
    // don't run the risk of invalidating iterator). 
    for (auto& entry : mSessionKeyToRequestHandlerMap) 
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSessionBySessionKey(entry.first.c_str());
        if (userSession == nullptr && entry.second->isAttachedToUserSession())
        {
            // If a user session is not found and the request handler to that user session exists, it is time to get rid of it.
            // It is important to check if the request handler is already attached to the user session or not otherwise we run the risk
            // of destroying a request handler in the middle of a blocking operation (if this request handler is currently unauthenticated and
            // currently executing an rpc that is blocked).
            BLAZE_TRACE_LOG(Log::GRPC, "[GrpcEndpoint].expireRequestHandlers: Queueing up shutdown of the request handler associated with session key (" << entry.first.c_str() << ") as session no longer present.");
            entry.second->detachFromUserSession();
            entry.second->shutdown();
        }
    }

    mExpireRequestHandlersTimerId = mSelector.scheduleTimerCall(TimeValue::getTimeOfDay() + REQUESTHANDLER_INACTIVITY_CHECK_PERIOD_US, this, &GrpcEndpoint::expireRequestHandlers, "GrpcEndpoint::expireRequestHandlers");
}

void GrpcEndpoint::reconfigure(const EndpointConfig::GrpcEndpointConfig& newConfig, const RateConfig& newRateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    mConfig.setCommandTimeout(newConfig.getCommandTimeout());
    mConfig.setSslContext(newConfig.getSslContext());

    Blaze::InetFilter* newFilter = BLAZE_NEW Blaze::InetFilter();
    if (!newFilter->initialize(true /*allowIfEmpty*/, newConfig.getTrust()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: invalid allowedIpList configuration for endpoint '" << getName() << "'");
        delete newFilter;
    }
    else
    {
        mAllowedIpNetworks.reset(newFilter); // destroy old one and take ownership of new one
    }

    newFilter = BLAZE_NEW Blaze::InetFilter();
    if (!newFilter->initialize(false /*allowIfEmpty*/, newConfig.getTrustedClientIpList()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: invalid trustedClientIpList configuration for endpoint '" << getName() << "'");
        delete newFilter;
    }
    else
    {
        mTrustedClientIpNetworks.reset(newFilter); // destroy old one and take ownership of new one
    }

    if (newConfig.getHasInternalNicAccess().empty() && newConfig.getBind() != BIND_INTERNAL)
    {
        BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: hasInternalNicAccess cannot be empty for non-internal endpoint '" << getName() << "'");
    }
    else
    {
        newFilter = BLAZE_NEW Blaze::InetFilter();
        if (!newFilter->initialize(false /*allowIfEmpty*/, newConfig.getHasInternalNicAccess()))
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: invalid hasInternalNicAccess configuration for endpoint '" << getName() << "'");
            delete newFilter;
        }
        else
        {
            mInternalNicAccessNetworks.reset(newFilter); // destroy old one and take ownership of new one
        }
    }

    if (!newConfig.getEnvoyXfccTrustedHashes().empty())
    {
        if (newConfig.getEnvoyResourceName()[0] == '\0')
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: envoyXfccTrustedHashes requires an Envoy-enabled endpoint '" << getName() << "'");
        }
        else if (newConfig.getBind() != BIND_INTERNAL)
        {
            // Requires internal endpoint or mTLS trustedHashes to ensure validity of x-forwarded-client-cert header. 
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].reconfigure: envoyXfccTrustedHashes requires an internal or mTLS endpoint '" << getName() << "'");
        }
        else
        {
            mEnvoyXfccTrustedHashes.clear();
            for (const auto& trustedHash : newConfig.getEnvoyXfccTrustedHashes())
            {
                mEnvoyXfccTrustedHashes.insert(trustedHash.c_str());
            }
        }
    }
    else
    {
        mEnvoyXfccTrustedHashes.clear();
    }

    if (newConfig.getRateLimiter()[0] != '\0')
    {
        // RateLimiter configuration
        RateLimiter newRateLimiter;

        RateConfig::RateLimitersMap::const_iterator rateLimiterIt = newRateConfig.getRateLimiters().find(newConfig.getRateLimiter());
        if (newRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second))
        {
            mRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].initialize: Failed to initialize RateLimiter '" << newConfig.getRateLimiter() << "' "
                "for endpoint '" << getName() << "'");
        }
    }
    else
    {
        mRateLimiter.finalize();
    }
    
    for (auto& entry : mSessionKeyToRequestHandlerMap)
    {
        auto* rateLimitCounter = mRateLimiter.getCurrentRatesForIp(entry.second->getPeerInfo().getPeerAddress().getIp());
        entry.second->setRateLimitCounter(rateLimitCounter);
    }

    mRpcAuthorizer.initializeRpcAuthorizationList(newConfig.getRpcWhiteList(), newConfig.getRpcBlackList(), rpcControlLists, platformSpecificControlLists);
}

bool GrpcEndpoint::isShuttingDown() const
{
    return mShuttingDown;
}

void GrpcEndpoint::getStatusInfo(Blaze::GrpcEndpointStatus& status) const
{
    status.setName(mName.c_str());
    status.setPort(mPort);
    
    status.setConnectionNum(getCurrentConnections());
    status.setMaxConnectionNum(getMaxConnections());
    
    status.setTotalRpcAuthorizationFailureCount(mRpcAuthorizer.getTotalRpcAuthorizationFailureCount());
   
    status.setNumUnaryRpcs(mNumUnaryRpcs.get());
    status.setNumServerStreamingRpcs(mNumServerStreamingRpcs.get());
    status.setNumClientStreamingRpcs(mNumClientStreamingRpcs.get());
    status.setNumBidiStreamingRpcs(mNumBidiStreamingRpcs.get());
}

const char8_t* GrpcEndpoint::getCommonName() const
{
    if (blaze_stricmp(getChannelType(), "ssl") == 0)
    {
        auto sslContext = gSslContextManager->get(mConfig.getSslContext());
        if (sslContext != nullptr)
            return sslContext->getCommonName();
    }

    return nullptr;
}

uint32_t GrpcEndpoint::getCurrentConnections() const
{
    // As an approximation to the connections (until https://eadpjira.ea.com/browse/GOS-31011 is addressed), return the number of usersessions as the load. This is better indicator of 
    // load anyway. 
    return mSessionKeyToRequestHandlerMap.size(); 
}

bool GrpcEndpoint::registerServices(const Blaze::ComponentManager& compMgr)
{
    char8_t addrBuf[256];
    if (mConfig.getBind() == BIND_INTERNAL)
    {
        Blaze::getInetAddress(BIND_INTERNAL, mPort).toString(addrBuf, sizeof(addrBuf), InetAddress::IP_PORT);
    }
    else
    {
        // BIND_ALL is used for both BIND_EXTERNAL/BIND_ALL as we are okay hitting external endpoint from anywhere. 
        Blaze::getInetAddress(BIND_ALL, mPort).toString(addrBuf, sizeof(addrBuf), InetAddress::IP_PORT);
    }
    
    BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].registerServices: Opening port (" << mPort << ") with bind type (" << BindTypeToString(getBindType()) << ") for endpoint (" << getName() << "). Interface(" << addrBuf << ")");

    if (blaze_stricmp(getChannelType(), "ssl") == 0)
    {
        auto sslContext = gSslContextManager->get(mConfig.getSslContext());
        
        grpc::string instanceCert, instanceKey, rootCerts;

        instanceCert = sslContext->getCertData(); // The SSL cert of this instance that is presented to the client
        instanceKey = sslContext->getKeyData(); // The SSL key of this cert.
        Blaze::Grpc::readFile(sslContext->getCaFile(), rootCerts); // The set of root certs for any outbound access. It should not be required here as we are not doing any client cert validation.

        grpc::SslServerCredentialsOptions::PemKeyCertPair keycertPair = { instanceKey, instanceCert };

        grpc::SslServerCredentialsOptions sslOps;
        sslOps.pem_root_certs = rootCerts;
        sslOps.pem_key_cert_pairs.push_back(keycertPair);

        mServerBuilder.AddListeningPort(addrBuf, grpc::SslServerCredentials(sslOps));
    }
    else
    {
        mServerBuilder.AddListeningPort(addrBuf, grpc::InsecureServerCredentials());
    }
    
    {
        // Connection Management
        // See https://developer.ea.com/download/attachments/163610330/RE%20Grpc%20%28and%20Blaze%20in%20general%29%20Dead%20Client%20design.msg?version=1&modificationDate=1505429497330&api=v2 
        // As grpc disassociates connection from the rpc handling, the network resources are target for 'eventual' clean up and not immediate clean up (the way we have dealt with persistent
        // channels earlier). 
        // It should however be noted that grpc does conflate streaming rpc handling with connection. So streaming rpcs are cancelled if the underlying connection is broken.
        
        // The network resources on grpc side are cleaned up after GRPC_ARG_MAX_CONNECTION_IDLE_MS duration since the number of outstanding rpcs became zero. 

        // 1. If a client makes a request and has not subscribed to any notifications(notifications are long lived server streaming rpcs), the connection resource would be cleaned up after configured value. 
        // If the client makes another request within expiration window, the clean up timer resets.
        // 2. For an authenticated client that has not subscribed to any notifications, if he makes another call before the user session expiration but after the network resource is cleaned up, it
        // does not matter! The server and client transparently use a new connection underneath. 
        // 3. For an authenticated client that has subscribed to the notifications, the idle timer does not kick in. If a client does not send any rpc within the user session expiration limit,
        // the user session is cleaned up. After some time, the InboundGrpcRequestHandler would expire which will clean up the streaming rpcs. This, in turn, would clean up the network resources in near 
        // future.
        
        mServerBuilder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS, (int)mConfig.getMaxConnIdleTime().getMillis()); 

        mServerBuilder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, (int)mConfig.getKeepAlive());
        // We can tune GRPC_ARG_KEEPALIVE_TIME_MS/GRPC_ARG_KEEPALIVE_TIMEOUT_MS in future as needed for SDK clients.
        mServerBuilder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, (int)mConfig.getKeepAliveSendInterval().getMillis());
        mServerBuilder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, (int)mConfig.getKeepAliveTimeout().getMillis());

    }

    // set up Max incoming message size
    {
        int32_t maxIncomingMessageSize = static_cast<int32_t>(mConfig.getMaxFrameSize());
        mServerBuilder.AddChannelArgument(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, maxIncomingMessageSize);
    }

    // Disable grpc's handling of the deadline. Instead we rely on our command timeout at blaze server
    // level.
    {
        mServerBuilder.AddChannelArgument(GRPC_ARG_ENABLE_DEADLINE_CHECKS, 0);
    }

    for (size_t i = 0; i < Blaze::ComponentData::getComponentCount(); ++i)
    {
        auto componentData = Blaze::ComponentData::getComponentDataAtIndex(i);
        if (componentData != nullptr)
        {
            eastl::unique_ptr<grpc::Service> service(componentData->grpcServiceCreateFunc());
            if (service.get() != nullptr)
            {
                mServiceComponentDataMap[service.get()] = componentData;
                mServerBuilder.RegisterService(service.get());
                mServices.push_back(eastl::move(service));
            }
        }
    }
    
    return true;
}

bool GrpcEndpoint::startGrpcServer()
{
    {
        // As per docs, Completion Queue need to be started before server creation.
        mCQ.reset(mServerBuilder.AddCompletionQueue().release());

        // kick off the completion queue processing thread
        if (start() == EA::Thread::kThreadIdInvalid)
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].startGrpcServer: Failed to launch grpc completion queue thread: '" << mName << "'");
            return false;
        }

        auto grpcServer = mServerBuilder.BuildAndStart();
        if (grpcServer)
        {
            BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].startGrpcServer: Start accepting incoming connections for endpoint '" << mName << "'");
            mServer.reset(grpcServer.release());
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].startGrpcServer: Failed to start grpc server: '" << mName << "'");
            return false;
        }
    }

    // Now that the server is up and running, request all the rpcs in various components.
    for (auto& serviceComponentDataMapEntry : mServiceComponentDataMap) // For all components
    {
        for (auto& commandInfoMapEntry : serviceComponentDataMapEntry.second->commands) // For each command in the component
        {
            if (commandInfoMapEntry.second->grpcRpcCreateFunc)
            {
                commandInfoMapEntry.second->grpcRpcCreateFunc(serviceComponentDataMapEntry.first, mCQ.get(), this);
            }
        }
    }

    return true;
}

void GrpcEndpoint::stop()
{
    waitForEnd();
}

void GrpcEndpoint::run()
{
    while (true)
    {
        TagInfo tagInfo;
        tagInfo.fiberPriority = ++mFiberPriority; 
        
        if (!mCQ->Next((void**)&tagInfo.tagProcessor, &tagInfo.ok))
        {
            BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].run: Shutting down grpc completion queue processing thread as the queue indicating a shutdown..");
            break;
        }
        // Put the incoming request on our selector queue. If we call mSelector.scheduleFiberCall, it'd end up creating a delayed fiber call (See FiberManager::executeJob).
        ++mPendingTagCount;
        mSelector.scheduleCall(this, &GrpcEndpoint::processTag, tagInfo, tagInfo.fiberPriority);
    }
}

void GrpcEndpoint::processTag(TagInfo tag)
{
    --mPendingTagCount;
    (*(tag.tagProcessor))(tag.ok);
}

InboundGrpcRequestHandler* GrpcEndpoint::createRequestHandler()
{
    if (getCurrentConnections() < getMaxConnections() && !isShuttingDown())
    {
        char8_t tempSessionKeyBuf[EA::StdC::kUint64MinCapacity];
        tempSessionKeyBuf[0] = '\0';
        EA::StdC::U64toa(getNextTempSessionKey(), tempSessionKeyBuf, 10);
        FixedString64 tempSessionKey(tempSessionKeyBuf);

        // The endpoint needs to take ownership of the request handler at it's creation in order to allow for a graceful finish in 
        // case of a shutdown. 
        auto requestHandler = BLAZE_NEW InboundGrpcRequestHandler(*this, tempSessionKey);
        mSessionKeyToRequestHandlerMap.insert(eastl::make_pair(tempSessionKey, eastl::unique_ptr<InboundGrpcRequestHandler>(requestHandler)));
        mNumRequestHandlers.increment(1);
        return requestHandler;
    }
    else
    {
        incrementCountTotalRejects();
    }

    return nullptr;
}

void GrpcEndpoint::destroyRequestHandler(InboundGrpcRequestHandler* handler)
{
    mPendingRemovalQueue.push_back(handler);

    if (!mRequestHandlersPendingRemoval)
    {
        Fiber::CreateParams params;
        params.stackSize = Fiber::STACK_SMALL;
        gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &GrpcEndpoint::destroyRequestHandlersInternal, "GrpcEndpoint::destroyRequestHandlersInternal", params);
        mRequestHandlersPendingRemoval = true;
    }
}

void GrpcEndpoint::destroyRequestHandlersInternal()
{
    while (!mPendingRemovalQueue.empty())
    {
        auto requestHandler = mPendingRemovalQueue.front();
        mPendingRemovalQueue.pop_front();
        
        auto iter = mSessionKeyToRequestHandlerMap.find_as(requestHandler->getSessionKey());
        if (iter != mSessionKeyToRequestHandlerMap.end())
        {
            mSessionKeyToRequestHandlerMap.erase(iter);
            mNumRequestHandlers.decrement(1);
        }
    }

    mRequestHandlersPendingRemoval = false;

    if (isShuttingDown() && mSessionKeyToRequestHandlerMap.empty())
    {
        BLAZE_INFO_LOG(Log::GRPC, "[GrpcEndpoint].destroyRequestHandlersInternal: signal shutdown fiber event as all the request handlers have finished processing.");
        Fiber::signal(mAllRequestHandlersDoneEvent, ERR_OK);
    }
}

InboundGrpcRequestHandler* GrpcEndpoint::getRequestHandler(const char8_t* sessionKey)
{
    if (isShuttingDown()) // Don't let the new rpc attach to us if we are already shutting down.
        return nullptr;

    if (sessionKey != nullptr && sessionKey[0] != '\0')
    {
        auto it = mSessionKeyToRequestHandlerMap.find_as(sessionKey);
        if (it != mSessionKeyToRequestHandlerMap.end())
        {
            return (*it).second.get();
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::GRPC, "[GrpcEndpoint].getRequestHandler: No request handler found for sessionKey - " << sessionKey);
    }

    return nullptr;
}

bool GrpcEndpoint::changeRequestHandlerSessionKey(const char8_t* newSessionKey, const char8_t* tempSessionKey)
{
    bool isAttachedToUserSession = false;

    auto iter = mSessionKeyToRequestHandlerMap.find_as(tempSessionKey);
    if (iter != mSessionKeyToRequestHandlerMap.end())
    {
        // Prevent the insert below from failing due to a duplicate key
        auto dupeIter = mSessionKeyToRequestHandlerMap.find_as(newSessionKey);
        if (dupeIter == mSessionKeyToRequestHandlerMap.end())
        {
            auto requestHandler = iter->second.release();
            requestHandler->setSessionKey(newSessionKey);

            // Remove old entry and insert new one
            mSessionKeyToRequestHandlerMap.erase(iter);
            mSessionKeyToRequestHandlerMap.insert(eastl::make_pair(newSessionKey, eastl::unique_ptr<InboundGrpcRequestHandler>(requestHandler)));

            isAttachedToUserSession = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[GrpcEndpoint].changeRequestHandlerSessionKey: mSessionKeyToRequestHandlerMap already has a handler for SessionKey [" << newSessionKey << "].");
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::GRPC, "[GrpcEndpoint].changeRequestHandlerSessionKey: Request Handler does not have a valid temp session key.");
    }

    return isAttachedToUserSession;
}

void GrpcEndpoint::updateRpcCountMetric(RpcType rpcType, bool increment)
{
    switch (rpcType)
    {
    case RPC_TYPE_UNARY:
        increment ? mNumUnaryRpcs.increment(1) : mNumUnaryRpcs.decrement(1);
        break;

    case RPC_TYPE_SERVER_STREAMING:
        increment ? mNumServerStreamingRpcs.increment(1) : mNumServerStreamingRpcs.decrement(1);
        break; 
    
    case RPC_TYPE_CLIENT_STREAMING:
        increment ? mNumClientStreamingRpcs.increment(1) : mNumClientStreamingRpcs.decrement(1);
        break;
    
    case RPC_TYPE_BIDI_STREAMING:
        increment ? mNumBidiStreamingRpcs.increment(1) : mNumBidiStreamingRpcs.decrement(1);
        break;

    default:
        EA_ASSERT_MSG(false, "This should not happen.");
        break;
    }
}

bool GrpcEndpoint::getInternalCommandsAllowed() const
{
    return (getBindType() == BIND_INTERNAL);
}

bool GrpcEndpoint::getEnvoyAccessEnabled() const
{
    if (mConfig.getEnvoyResourceName()[0] != '\0')
        return true;

    return false;
}

bool GrpcEndpoint::isEnvoyXfccTrustedHash(const eastl::string& hash) const
{
    return (mEnvoyXfccTrustedHashes.find(hash) != mEnvoyXfccTrustedHashes.end());
}

} //namespace Grpc
} //namespace Blaze


