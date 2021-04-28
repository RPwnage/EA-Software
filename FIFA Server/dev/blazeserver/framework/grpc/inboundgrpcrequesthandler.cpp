/*************************************************************************************************/
/*!
    \file   inboundgrpcrequesthandler.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/grpc/inboundgrpcrequesthandler.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"
#include "framework/grpc/grpcendpoint.h"
#include "framework/grpc/inboundgrpcmanager.h"

#include "framework/logger.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/util/locales.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/component/inboundrpccomponentglue.h"
#include "framework/component/component.h"
#include "framework/connection/selector.h"
#include "framework/oauth/oauthutil.h"
#include "framework/rpc/oauthslave.h"
#include "nucleusconnect/tdf/nucleusconnect.h"

#include <EASTL/fixed_string.h>
#include <eathread/eathread_atomic.h>
namespace Blaze
{
namespace Grpc
{

static EA::Thread::AtomicInt64 grpcRequestHandlersCount = 0;

InboundGrpcRequestHandler::InboundGrpcRequestHandler(GrpcEndpoint& endpoint, FixedString64& tempSessionKey)
    : mOwner(endpoint)
    , mPeerInfo(*this)
    , mNextMsgNum(0)
    , mProcessMessageFiberGroupId(Fiber::allocateFiberGroupId())
    , mIsAttachedToUserSession(false)
    , mSessionKey(tempSessionKey)
    , mShutdown(false)
    , mRateLimitCounter(nullptr)
    , mLegacyRpcQueue("InboundGrpcRequestHandler::mLegacyRpcQueue")
    , mOutstandingUnaryRpcs(0)
{
    // Setting the default client as CLIENT_TYPE_GAMEPLAY_USER helps us avoid needing a duplicate CLIENT_TYPE_GAMEPLAY_USER_2. If our default client type is anything other than CLIENT_TYPE_GAMEPLAY_USER, 
    // a grpc based client can't set it to CLIENT_TYPE_GAMEPLAY_USER because that enum value is 0 and won't be serialized over the wire.
    mPeerInfo.setClientType(CLIENT_TYPE_GAMEPLAY_USER); 
    mPeerInfo.setSupportsNotifications(true);
    // Grpc does not have a way to set the inactivity timeout per connection. There is a global timeout per endpoint (GRPC_ARG_MAX_CONNECTION_IDLE_MS)
    mPeerInfo.setIgnoreInactivityTimeoutEnabled(false);   
    mPeerInfo.setCommandTimeout(endpoint.getCommandTimeout());
    mPeerInfo.setEndpointName(endpoint.getName());

    mPeerInfo.setListener(*this);

    mLegacyRpcQueue.setMaximumWorkerFibers(FiberJobQueue::ONE_WORKER_LIMIT);// This should be always 1 to make sure requests are serialized.
    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].ctor: Global Count(" << ++grpcRequestHandlersCount << ")");
}

InboundGrpcRequestHandler::~InboundGrpcRequestHandler()
{
    mLegacyRpcQueue.cancelAllWork();

    EA_ASSERT_MSG(mNotificationHandlersMap.empty(), "All notification handlers on this request handler should have finished by now.");
    EA_ASSERT_MSG(mInboundRpcBasePtrByIdMap.empty(), "All rpcs on this request handler should have finished by now.");

    EA_ASSERT(mPeerInfo.getAttachedUserSessionsCount() == 0);
    mPeerInfo.removeListener();

    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].dtor: Global Count(" << --grpcRequestHandlersCount << ")");
}

void InboundGrpcRequestHandler::processMessage(InboundRpcBase& rpc, const google::protobuf::Message* protobufMessage)
{
    Fiber::BlockingSuppressionAutoPtr suppressBlocking(true);

    attachAndInitializeRpc(rpc);

    // set the real peer address if x-forwarded-for header was present in the incoming request headers
    if (rpc.getFrame().clientAddr.isResolved())
    {
        mPeerInfo.setRealPeerAddress(rpc.getFrame().clientAddr);
    }

    mPeerInfo.setServiceName(gController->resolveServiceName(rpc.getFrame().serviceName.c_str()));
    ClientPlatformType servicePlatform = gController->getServicePlatform(mPeerInfo.getServiceName());

    InboundRpcComponentGlue::setLoggingContext(SlaveSession::INVALID_SESSION_ID, rpc.getFrame().userSessionId, nullptr, nullptr, mPeerInfo.getRealPeerAddress(), INVALID_ACCOUNT_ID, servicePlatform);

    char8_t localeBuf[256];
    blaze_snzprintf(localeBuf, sizeof(localeBuf), "%c%c%c%c", LocaleTokenPrintCharArray(rpc.getFrame().locale));

    // The protobufMessage->ByteSize() is an expensive call so is commented out in the trace log below. gRPC does not expose any other way to get the message wire size as the 
    // size of the transport protocol message is not necessarily the size of the incoming request.  

    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: Received request: "
        << "rpc=" << rpc.getName() << " msgNum=" << rpc.getFrame().msgNum << " type=" << rpc.getRpcType() << " locale=" << localeBuf
        /*<< " size=" << ((int32_t)protobufMessage->ByteSize())*/);

    if (rpc.getComponent() != nullptr)
    {
        // Check if this RPC is authorized (in the RPC whitelist and not blacklisted) before executing
        if (!rpc.getEndpoint().isRpcAuthorized(servicePlatform, rpc.getComponentId(), rpc.getCommandId()))
        {
            rpc.getEndpoint().incrementTotalRpcAuthorizationFailureCount();
            if (rpc.getComponent()->isLocal())
                rpc.getComponent()->asStub()->tickRpcAuthorizationFailureCount(mPeerInfo, rpc.getCommandInfo());

            FixedString32 deviceId = InboundRpcComponentGlue::getCurrentUserSessionDeviceId(rpc.getFrame().userSessionId);

            BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: Attempted to execute unauthorized RPC " <<
                rpc.getComponent()->getComponentInfo().loggingName << "/" << rpc.getCommandInfo().loggingName << ". User's deviceId: " << deviceId.c_str() << ". PeerInfo: " << mPeerInfo.toString());

            completeRequest(rpc, ERR_AUTHORIZATION_REQUIRED, INVALID_INSTANCE_ID, nullptr);
            return;
        }

        // make sure we're not overloading fibers.  However, framework level commands are always allowed.
        bool isFrameworkCommand = false;
        FixedString32 droppedOrPermitted;

        if (InboundRpcComponentGlue::hasReachedFiberLimit(rpc.getComponentId(), isFrameworkCommand, droppedOrPermitted))
        {
            if (isFrameworkCommand)
            {
                rpc.getEndpoint().incrementTotalExceededRpcFiberLimitPermitted();
            }
            else
            {
                // We always want to allow framework level RPCs, even if the command fiber count is > MaxCommandFiberCount (see https://eadpjira.ea.com/browse/GOS-30620)
                rpc.getEndpoint().incrementTotalExceededRpcFiberLimitDropped();

                // The logging logic of InboundRpcConnection is too convoluted and bloated when command fibers are maxed out.
                // Don't log anything as we are using specific error code now instead of ERR_SYSTEM. We can add a log later if it becomes necessary.
                completeRequest(rpc, ERR_COMMAND_FIBERS_MAXED_OUT, INVALID_INSTANCE_ID, nullptr);
                return;
            }
        }

        // Internal commands need to be run on internal bound endpoints
        // (excluding internal endpoints used for external Envoy requests)
        if (rpc.getCommandInfo().isInternalOnly && !rpc.getEndpoint().getInternalCommandsAllowed())
        {
            BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: An attempt to execute internal only RPC (" <<
                rpc.getComponent()->getComponentInfo().loggingName << ":" << rpc.getCommandInfo().loggingName << "). PeerInfo:" << mPeerInfo.toString() << " ) on Externally accessible endpoint. Ignored.");

            completeRequest(rpc, ERR_INVALID_ENDPOINT, INVALID_INSTANCE_ID, nullptr);
            return;
        }

        if (gController->isShutdownPending())
        {
            // The last few RPCs are finishing up, and then this instance will shutdown.
            // Tell the caller ERR_UNAVAILABLE instead of possibly only partially executing their RPC.
            completeRequest(rpc, ERR_UNAVAILABLE, INVALID_INSTANCE_ID, nullptr);
            return;
        }

        //  If the request has no associated CommandCreateFunc, then that means the command is a pure gRPC *only* command.  
        //  This code-path is much leaner than Component::sendRequest().  That is to say, all handling of the command occurs locally within
        //  the context of the InboundRpcBase instance.  There is no automatic RPC routing, gCurrentUserSession, or sliver locking but we need
        //  to ensure that rpc has an IdentityContext that can be used. 
        bool grpcOnly = (rpc.getCommandInfo().commandCreateFunc == nullptr);
        if (grpcOnly)
        {
            // This path expects the requests to come on the instance that has the component loaded locally.
            if (!rpc.getComponent()->isLocal())
            {
                BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: Component "
                    << BlazeRpcComponentDb::getComponentNameById(rpc.getComponentId()) << "(" << rpc.getComponentId() << ") not loaded for locally.");

                completeRequest(rpc, ERR_COMPONENT_NOT_FOUND, INVALID_INSTANCE_ID, nullptr);
                return;
            }

            if (rpc.getCommandInfo().requiresAuthentication)
            {
                if (rpc.getFrame().accessTokenType == OAuth::TOKEN_TYPE_NONE || rpc.getFrame().accessToken.empty())
                {
                    BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: "
                        "The command " << BlazeRpcComponentDb::getCommandNameById(rpc.getComponentId(), rpc.getCommandId()) << "(" << rpc.getCommandId() << ")" <<
                        " requires authentication but no valid access token is available.");

                    completeRequest(rpc, ERR_AUTHENTICATION_REQUIRED, INVALID_INSTANCE_ID, nullptr);
                    return;
                }

                
                // We can't establish an IdentityContext yet because that requires blocking and we are on main fiber.
                // We'll deal with it in code path later.

                
            }
        }

        if (grpcOnly)
            rpc.processRequestOnFiber(protobufMessage, false);
        else
            mLegacyRpcQueue.queueFiberJob<InboundGrpcRequestHandler, InboundRpcBasePtr, const google::protobuf::Message*>(this, &InboundGrpcRequestHandler::processLegacyMessage, &rpc, protobufMessage, "InboundGrpcRequestHandler::processMessage");
    }
    else
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processMessage: Component "
            << BlazeRpcComponentDb::getComponentNameById(rpc.getComponentId()) << "(" << rpc.getComponentId() << ") not loaded for command "
            << BlazeRpcComponentDb::getCommandNameById(rpc.getComponentId(), rpc.getCommandId()) << "(" << rpc.getCommandId() << ")");
        
        //Finish off the message with the correct error
        completeRequest(rpc, ERR_COMPONENT_NOT_FOUND, INVALID_INSTANCE_ID, nullptr);
    }

    gLogContext->clear();
}

void InboundGrpcRequestHandler::processLegacyMessage(InboundRpcBasePtr rpcPtr, const google::protobuf::Message* protobufMessage)
{
    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processLegacyMessage: Processing request: "
        << "rpc=" << rpcPtr->getName() << " msgNum=" << rpcPtr->getFrame().msgNum << " type=" << rpcPtr->getRpcType());
    
    rpcPtr->processRequestOnFiber(protobufMessage, true);
}

void InboundGrpcRequestHandler::handleSystemError(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo)
{
    if (err != ERR_MOVED)
    {
        rpc.finishWithError(Grpc::getGrpcStatusFromBlazeError(err));
    }
    else if (rpc.getServerContext() != nullptr)
    {
        // Use trailing metadata (instead of initial metadata) to support the ERR_MOVED. 
        // After reading https://grpc.io/docs/guides/concepts.html and https://github.com/grpc/grpc-java/issues/2770 , it looks like initial metadata is designed 
        // for the server to immediately respond to the client after it calls an rpc and server has no idea what the processing would look like. Trailing metadata
        // is supposed to be used for when the server knows something specific about the rpc call.

        grpc::string movedToInstanceStr(std::to_string(movedTo));
        rpc.getServerContext()->AddTrailingMetadata("x-blaze-movedto-id", movedToInstanceStr);

        InetAddress movedToAddr;
        const RemoteServerInstance* remoteServerInstance = gController->getRemoteServerInstance(movedTo);
        if (remoteServerInstance != nullptr)
        {
            remoteServerInstance->getAddressForEndpointType(
                mOwner.isInternalPeerAddress(getPeerInfo().getPeerAddress()),
                movedToAddr,
                mOwner.getBindType(),
                mOwner.getChannelType(),
                mOwner.getEncoderName(),
                mOwner.getDecoderName());

            if (movedToAddr.isValid())
            {
                // Only send down the hostname:port and not IP address. IP address only exists in Fire2Metadata to deal with a bug in the older SDKs. Ip address should not be used for connecting 
                // to a new instance. 
                grpc::string movedToAddrStr(movedToAddr.getHostname());
                movedToAddrStr.append(":");
                movedToAddrStr.append(std::to_string(movedToAddr.getPort(InetAddress::HOST)));
                rpc.getServerContext()->AddTrailingMetadata("x-blaze-movedto-host", movedToAddrStr);
            }
        }

        rpc.finishWithError(Grpc::getGrpcStatusFromBlazeError(err));
    }
}

void InboundGrpcRequestHandler::completeRequest(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo, const EA::TDF::Tdf* tdf /* = nullptr */)
{
    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeRequest: Sending response: "
        << "rpc =" << rpc.getName() << "msgnum = " << rpc.getFrame().msgNum << " ec = " << BlazeError(err) << "(" << err << ")");
    
    // Reset back to the base locale/country in case it had been overridden by this command
    mPeerInfo.setLocale(mPeerInfo.getBaseLocale());
    mPeerInfo.setCountry(mPeerInfo.getBaseCountry()); 
    
    if (!rpc.isDoneTagReceived()) // If we have received done tag, grpc library is done and we can't do anything about this rpc
    {
        // In case of a system error, there is no payload(Response or ErrorResponse) to send. So finish with error.
        if (BLAZE_ERROR_IS_SYSTEM_ERROR(err))
        {
            handleSystemError(rpc, err, movedTo);
        }
        else
        {
            rpc.getCommandInfo().sendGrpcResponse(rpc, tdf, err);
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeRequest: Sending response skipped due to already received done tag: "
            << "rpc =" << rpc.getName() << "msgnum = " << rpc.getFrame().msgNum << " ec = " << BlazeError(err) << "(" << err << ")");
    }

    sendQueuedNotifications();

    if (!mIsAttachedToUserSession)
    {
        // If this rpc installed a new user session, store this InboundGrpcRequestHandler to persist across the requests.
        if (UserSession::getCurrentUserSessionId() != INVALID_USER_SESSION_ID)
        {
            UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(UserSession::getCurrentUserSessionId());
            if (userSession != nullptr)
            {
                char8_t keyBuffer[MAX_SESSION_KEY_LENGTH];
                keyBuffer[0] = '\0';
                userSession->getKey(keyBuffer);

                if (keyBuffer[0] != '\0')
                {
                    if (rpc.getEndpoint().changeRequestHandlerSessionKey(keyBuffer, mSessionKey.c_str()))
                    {
                        mIsAttachedToUserSession = true;
                    }
                }
            }
        }
    }
}


void InboundGrpcRequestHandler::completeNotificationSubscription(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo)
{
    mPeerInfo.setLocale(mPeerInfo.getBaseLocale());
    mPeerInfo.setCountry(mPeerInfo.getBaseCountry());

    // In case of a system error, there is no payload(Response or ErrorResponse) to send. So finish with error.
    if (BLAZE_ERROR_IS_SYSTEM_ERROR(err))
    {
        handleSystemError(rpc, err, movedTo);
    }
    else
    {
        if (UserSession::getCurrentUserSessionId() != INVALID_USER_SESSION_ID)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeNotificationSubscription: Subscribed notifications "
                << "rpc =" << rpc.getName()
                << "component = " << BlazeRpcComponentDb::getComponentNameById(rpc.getComponentId()));

            addNotificationHandler(rpc);
        }
        else
        {
            BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeNotificationSubscription: No UserSession found. Noop subscription request."
                << "rpc =" << rpc.getName()
                << "component = " << BlazeRpcComponentDb::getComponentNameById(rpc.getComponentId()));

            rpc.finishWithError(Grpc::getGrpcStatusFromBlazeError(ERR_AUTHENTICATION_REQUIRED));
        }
    }
}

void InboundGrpcRequestHandler::shutdown()
{
    if (mShutdown)
        return;

    mShutdown = true;

    removeNotificationHandlers();

    if (!hasOutstandingRpcs())
    {
        BLAZE_INFO_LOG(Log::GRPC, "[InboundGrpcRequestHandler].shutdown: shutdown is called on request handler and no outstanding rpc. queue up the request handler for destruction.");
        mOwner.destroyRequestHandler(this);
    }
    else
    {
        //+ Unary rpcs
        StringBuilder sb;
        Fiber::dumpFiberGroupStatus(mProcessMessageFiberGroupId, sb);
        BLAZE_INFO_LOG(Log::GRPC, "[InboundGrpcRequestHandler].shutdown: Canceling (and joining) fiber group(" << mProcessMessageFiberGroupId << ") on the following fibers:\n" << SbFormats::PrefixAppender("    ", sb.get()));

        Fiber::cancelGroup(mProcessMessageFiberGroupId, ERR_CANCELED);
        Fiber::join(mProcessMessageFiberGroupId);
        BLAZE_INFO_LOG(Log::GRPC, "[InboundGrpcRequestHandler].shutdown: all outstanding legacy command fibers joined.");

        // The following should essentially be a no-op as we are joining using mProcessMessageFiberGroupId.
        mLegacyRpcQueue.join();
        //-

        //+ streaming rpcs need to be explicitly cancelled
        for (auto& rpcPtrEntry : mInboundRpcBasePtrByIdMap)
        {
            BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].shutdown: cancelling rpc(" << rpcPtrEntry.second->getName() << ") due to shutdown.");
            rpcPtrEntry.second->getServerContext()->TryCancel();
        }
        //-
    }
}

void InboundGrpcRequestHandler::attachAndInitializeRpc(InboundRpcBase& rpc)
{
    // Early out if we're already attached.
    if (rpc.getGrpcRequestHandler() == this)
        return;

    EA_ASSERT_MSG(rpc.getGrpcRequestHandler() == nullptr, "Once an InboundGrpcJobHandler is attached to a request handler, it should never get re-attached to another handler.");

    // Pin a reference to this rpc by putting it in the map.  The reference will be removed once this request handler
    // is done with the request.  But, by then, the application command handler may have taken a reference in order to
    // stream more responses to the client.
    EA_ASSERT_MSG(rpc.getRefCount() > 0, "rpc must be a refCounted heap allocated instance.");
    mInboundRpcBasePtrByIdMap.insert(eastl::make_pair(rpc.getGrpcRpcId(), &rpc));
    if (rpc.getRpcType() == RPC_TYPE_UNARY)
        ++mOutstandingUnaryRpcs;

    // For Envoy, peerAddr will be the address of the local ingress Envoy node (not the actual client).
    // The actual client's IP address will be determined below in the call to the rpc.initialize() function.
    Blaze::InetAddress peerAddr = rpc.getInetAddressFromServerContext();
    if (peerAddr.isValid())
    {
        mPeerInfo.setPeerAddress(peerAddr);
        if (mRateLimitCounter == nullptr)
        {
            mRateLimitCounter = rpc.getEndpoint().getRateLimiter().getCurrentRatesForIp(peerAddr.getIp());
        }
    }

    rpc.setGrpcRequestHandler(this);

    rpc.initialize();
}

void InboundGrpcRequestHandler::detachRpc(InboundRpcBase& rpc)
{
    if (rpc.getRpcType() == RPC_TYPE_UNARY)
        --mOutstandingUnaryRpcs;

    const bool erased = (mInboundRpcBasePtrByIdMap.erase(rpc.getGrpcRpcId()) > 0);
    EA_ASSERT_MSG(erased, "There should not be a code path that result in this method being called for an rpc that is not currently attached to this handler.");

    if (!hasOutstandingRpcs())
    {
        EA_ASSERT(mInboundRpcBasePtrByIdMap.empty());
        if (rpc.getEndpoint().isShuttingDown())
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeRequest: queue up the request handler for destruction as endpoint is shutting down.");
            rpc.getEndpoint().destroyRequestHandler(this);
        }
        else if (!mIsAttachedToUserSession)
        {
            // an unauthenticated user or instance other than coreSlave. Get rid of the request handler. 
            // Unlike InboundRpcConnection, an unauthenticated InboundGrpcRequestHandler can have at most 1 rpc in progress so we don't need to check for any outstanding rpcs.
            // Unauthenticated InboundGrpcRequestHandler has at most 1 rpc in progress because without the user session id, we don't reuse the same request handler and a new one
            // is created as the request comes in.
            // This path also comes into play for the access token based requests.
            BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].completeRequest: Request handler not attached to a user session. queue up the request handler for destruction.");
            rpc.getEndpoint().destroyRequestHandler(this);
        }
    }
}

// Called when we receive onDone tag from the gRPC lib.
void InboundGrpcRequestHandler::onInboundRpcDone(InboundRpcBase& rpc, bool rpcCancelled)
{
    BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].onInboundRpcDone: rpc name(" << rpc.getName() << ") done with status(" << rpcCancelled << ").");
    
    // Give the legacy/grpcOnly rpc command handlers a chance to finish. 
    rpc.processDone(rpcCancelled);

    // If this rpc is a notification handler, remove it from the map where we cached it. 
    auto notifyIt = mNotificationHandlersMap.find(rpc.getComponentId());
    if (notifyIt != mNotificationHandlersMap.end() && (*notifyIt).second == &rpc)
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[InboundGrpcRequestHandler].onInboundRpcDone: Removing notification subscription for component(" << BlazeRpcComponentDb::getComponentNameById(rpc.getComponentId()) << "), rpc name(" << rpc.getName() << ").");
        mNotificationHandlersMap.erase(notifyIt);
    }

    detachRpc(rpc);
}

void InboundGrpcRequestHandler::onAllUsersRemoved()
{
    // This is currently neither called nor handled. Each request handlers caters to it's on user session (unlike InboundRpcConnection where multiple users share the instance).
    // When the SDK support comes along, we'll need to figure out how to support MLU.

    // The InboundRpcConnection schedules a graceful close in such scenario. The InboundGrpcRequestHandler need not worry about it as network layer is completely handled inside
    // grpc lib and the request handler itself always gracefully expires. 
}

void InboundGrpcRequestHandler::addNotificationHandler(Blaze::Grpc::InboundRpcBase& handler)
{
    auto iter = mNotificationHandlersMap.find(handler.getComponentId());
    if (iter != mNotificationHandlersMap.end())
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].addNotificationHandler: Notification was already subscribed for "
            << "component(" << BlazeRpcComponentDb::getComponentNameById(handler.getComponentId()) << "). Ending previous subscription.");

        iter->second->sendResponse(nullptr);
    }

    // We have already setup the listener and addRef at start of processMessage so don't need to do that here.
    mNotificationHandlersMap.insert(eastl::make_pair(handler.getComponentId(), &handler));
    
    // The UserSessionMaster::sendNotificationInternal has notifications cached in absence of the notification handler.
    // Now, that we have the handler, ask it to send those.
    sendPendingNotifications(handler.getComponentId());
}

void InboundGrpcRequestHandler::removeNotificationHandlers()
{
    for (auto& handlerEntry : mNotificationHandlersMap)
    {
        handlerEntry.second->sendResponse(nullptr);
    }
    mNotificationHandlersMap.clear();

    // Do not remove from mInboundRpcBasePtrByIdMap just yet, let it come down via the grpc lib
}

void InboundGrpcRequestHandler::sendQueuedNotifications()
{
    for (auto& notificationPtr : mQueuedNotificationList)
    {
        sendNotification(*notificationPtr, true);
    }
    mQueuedNotificationList.clear();
}

void InboundGrpcRequestHandler::sendNotification(Notification& notification, bool overrideImmediately)
{
    bool sendImmediately = notification.getOpts().sendImmediately
        || overrideImmediately
        || (mOutstandingUnaryRpcs == 0);//ensure no outstanding commands as this may be a notification from in-progress command. All the notifications are sent at the end of the command regardless.
    
    if (!sendImmediately)
    {
        BLAZE_TRACE_RPC_LOG(notification.getLogCategory(), "[InboundGrpcRequestHandler].sendNotification: -> Notif cached" << " "
            "[component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId()) << "), "
            "type=" << BlazeRpcComponentDb::getNotificationNameById(notification.getComponentId(), notification.getNotificationId()) << "(" << notification.getNotificationId() << "), "
            "userIndex=" << 0 << "]");

        NotificationPtr notificationSnapshot = notification.createSnapshot();
        mQueuedNotificationList.push_back(notificationSnapshot);
        return;
    }
                                        
    if (!mShutdown)
    {
        bool hasHandler = hasNotificationHandler(notification.getComponentId());
        if (hasHandler)
        {
            auto iter = mNotificationHandlersMap.find(notification.getComponentId());
            auto notificationRpc = iter->second;
            const NotificationInfo& info = *ComponentData::getNotificationInfo(notification.getComponentId(), notification.getNotificationId());
            if (info.sendNotificationGrpc)
            {
                BLAZE_TRACE_RPC_LOG(notification.getLogCategory(), "[InboundGrpcRequestHandler].sendNotification: -> Notif sent" << " "
                    "[component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId()) << "), "
                    "type=" << BlazeRpcComponentDb::getNotificationNameById(notification.getComponentId(), notification.getNotificationId()) << "(" << notification.getNotificationId() << "), "
                    "userIndex=" << 0 << "]\n" << notification.getPayload().get());

                info.sendNotificationGrpc(*notificationRpc, notification);
            }
            else
            {
                // We are not caching any notifications here because we have delegated that job to UserSessionMaster::sendNotificationInternal.
                BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].sendNotification: Failed to send notification as component has no support for sending notifications."
                    << "component(" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << ")");
            }
        }
    }
}

bool InboundGrpcRequestHandler::hasNotificationHandler(ComponentId componentId) const
{
    return mNotificationHandlersMap.find(componentId) != mNotificationHandlersMap.end();
}

void InboundGrpcRequestHandler::sendPendingNotifications(ComponentId componentId)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(UserSession::getCurrentUserSessionId());
    if (userSession != nullptr)
        userSession->sendPendingNotificationsByComponent(componentId);
}

} //namespace Grpc
} //namespace Blaze


