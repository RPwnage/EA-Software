/*************************************************************************************************/
/*!
    \file   inboundgrpcjobhandlers.cpp

    \attention
        (c) Electronic Arts Inc. 2018
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"

#include "framework/rpc/oauthslave.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Grpc
{

void InboundRpcBase::cancelRequestProcessingFiber(BlazeRpcError reason)
{
    Fiber::cancel(mRequestProcessingFiberId, reason);
}

void InboundRpcBase::processRequestOnFiber(const google::protobuf::Message* protobufMessage, bool serializeRequests)
{
    if (EA_UNLIKELY(gController->isShuttingDown()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[InboundRpcBase].processRequestOnFiber: Scheduling new command fiber while this instance is shutting down for request: command=" << getCommandInfo().loggingName);
    }

    Fiber::CreateParams fiberParams = InboundRpcComponentGlue::getCommandFiberParams(
        true, getCommandInfo(), getEndpoint().getCommandTimeout(), serializeRequests, mHandler->getProcessMessageFiberGroupId());

    gSelector->scheduleFiberCall<InboundRpcBasePtr, const google::protobuf::Message*>(
        &InboundRpcBase::_processRequestInternal, this, protobufMessage, nullptr, fiberParams);
}

void InboundRpcBase::processRequestInternal(const google::protobuf::Message* protobufMessage)
{
    mRequestProcessingFiberId = Fiber::getCurrentFiberId();

    CommandFiberTracker commandFiberTracker;

    gLogContext.copyFromCallingFiber();

    if (EA_UNLIKELY(gController->isShuttingDown()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[InboundRpcBase].processRequestInternal: Processing command fiber while this instance is shutting down for request: command=" << getCommandInfo().loggingName);
    }

    // For Grpc clients, if exceeding rate limit, we sleep in processRequest instead of processMessage (unlike InboundRpcConnection).
    // The reason being that processMessage executes on main selector loop instead of it's own fiber. If we made our fiber sleep there, we'll 
    // sleep the entire instance. 
    // It's also not wrong to introduce this sleep here because with grpc, a client could potentially be running multiple rpcs and net goal is 
    // to slow down a particular client. We'd be giving that client a slightly lesser command timeout as a result of this sleep but there is nothing
    // wrong with penalizing an abusive client this way.
    if (InboundRpcComponentGlue::hasExceededCommandRate(getEndpoint().getRateLimiter(), mHandler->getRateLimitCounter(), getCommandInfo(), false))
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].processRequestInternal: "
            "Specific RPC rate limit of for command " << getCommandInfo().loggingName << "(" << getCommandId() << ")" <<
            " was exceeded for RateLimiter " << getEndpoint().getRateLimiter().getName());

        Fiber::sleep(RATE_LIMIT_GRACE_PERIOD * 1000 * 1000, "InboundRpcBase command rate check squelch");
    }

    if (InboundRpcComponentGlue::hasExceededCommandRate(getEndpoint().getRateLimiter(), mHandler->getRateLimitCounter(), getCommandInfo(), true))
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].processRequestInternal: "
            "Total RPC rate limit of (" << getEndpoint().getRateLimiter().getCommandRateLimit() << ") was exceeded for RateLimiter "
            << getEndpoint().getRateLimiter().getName());

        Fiber::sleep(RATE_LIMIT_GRACE_PERIOD * 1000 * 1000, "InboundRpcBase overall rate check squelch");
    }

    if (getCommandInfo().commandCreateFunc == nullptr && getIdentityContext() == nullptr && getCommandInfo().requiresAuthentication)
    {
        Blaze::OAuth::OAuthSlave *oAuthSlave = (Blaze::OAuth::OAuthSlave*) gController->getComponent(Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
        if (oAuthSlave == nullptr)
        {
            BLAZE_ERR_LOG(Log::GRPC, "[InboundRpcBase].processRequestInternal: OAuth component is null. Can't crack access token!");

            mHandler->completeRequest(*this, ERR_SYSTEM, INVALID_INSTANCE_ID, nullptr);
            return;
        }
        
        OAuth::IdentityContextRequest identityContextReq;
        identityContextReq.setAccessTokenType(getFrame().accessTokenType);
        identityContextReq.setAccessToken(getFrame().accessToken.c_str());
        identityContextReq.setIpAddr(mHandler->getPeerInfo().getRealPeerAddress().getIpAsString());
        identityContextReq.setPlatform(gController->getServicePlatform(getFrame().serviceName.c_str()));
        identityContextReq.setComponentName(BlazeRpcComponentDb::getComponentNameById(getComponentId()));
        identityContextReq.setRpcName(BlazeRpcComponentDb::getCommandNameById(getComponentId(), getCommandId()));
        identityContextReq.setTrusted(mHandler->getPeerInfo().getTrustedClient());

        OAuth::IdentityContext identityContext;
        BlazeRpcError err = oAuthSlave->getIdentityContext(identityContextReq, identityContext);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::GRPC, "[InboundRpcBase].processRequestInternal: "
                "The command " << getName() <<
                " requires authentication but the provided credentials could not acquire an IdentityContext.");
            BlazeRpcError clientErr = ERR_SYSTEM;
            switch (err)
            {
            case OAUTH_ERR_INVALID_TOKEN:
                clientErr = ERR_AUTHENTICATION_REQUIRED;
                break;
            case OAUTH_ERR_INSUFFICIENT_SCOPES:
                clientErr = ERR_AUTHORIZATION_REQUIRED;
                break;
            case OAUTH_ERR_NOT_TRUSTED_CLIENT:
            case OAUTH_ERR_NO_SCOPES_CONFIGURED:
                clientErr = ERR_SYSTEM;
                break;
            default:
                break;
            }
            mHandler->completeRequest(*this, clientErr, INVALID_INSTANCE_ID, nullptr);
            return;
        }

        setIdentityContext(identityContext);
    }

    // This is the divergence path for the Legacy Grpc vs grpcOnly requests. The former ends up in InboundRpcBase::processLegacyRequest whereas new path will end up in 
    // onProcessRequest of the actual command implementation. 
    processRequest(protobufMessage); 
    mRequestProcessingFiberId = Fiber::INVALID_FIBER_ID;
}

RpcProtocol::Frame InboundRpcBase::makeLegacyRequestFrame()
{
    RpcProtocol::Frame frame(mHandler->getPeerInfo().getBaseLocale(), mHandler->getPeerInfo().getBaseCountry());

    frame.componentId = getComponentId();
    frame.commandId = getCommandId();
    frame.msgNum = getFrame().msgNum;
    frame.locale = getFrame().locale;
    frame.clientAddr = getFrame().clientAddr;
    frame.requestIsSecure = (blaze_stricmp(getEndpoint().getChannelType(), "ssl") == 0);
    frame.setSessionKey(getFrame().sessionKey.c_str());
    blaze_strnzcpy(frame.serviceName, getFrame().serviceName.c_str(), MAX_SERVICENAME_LENGTH);

    // RpcProtocol::Frame already has suitable defaults so we only set the ones we want to change. 
    frame.format = RpcProtocol::FORMAT_INVALID;
    frame.enumFormat = RpcProtocol::ENUM_FORMAT_INVALID;
    frame.xmlResponseFormat = RpcProtocol::XML_RESPONSE_FORMAT_INVALID;

    return frame;
}

void InboundRpcBase::processLegacyRequest(const google::protobuf::Message* protobufMessage)
{
    RpcProtocol::Frame blazeRpcFrame = makeLegacyRequestFrame();
    Message* blazeMessage = BLAZE_NEW Message(SlaveSession::INVALID_SESSION_ID, getFrame().userSessionId, mHandler->getPeerInfo(), blazeRpcFrame, nullptr, protobufMessage);
    mHandler->getPeerInfo().setLocale(getFrame().locale); //reset when the command completes
    //mHandler->getPeerInfo().setCountry(inFrame.country); // TODO_MC: Unsure how the Frame::country gets set in the first place.

    GrpcRpcContext rpcContext(getCommandInfo(), *blazeMessage);
    BlazeRpcError result = InboundRpcComponentGlue::initRpcContext(rpcContext, mHandler->getPeerInfo());

    if (getCommandId() == Blaze::Component::NOTIFICATION_SUBSCRIBE_GRPC_CMD)
    {
        // Special handling for the notificationSubscribe RPC
        mHandler->completeNotificationSubscription(*this, result, rpcContext.mMovedTo);
    }
    else
    {
        // These are created here so that the refCounted TdfPtrs remain in scope until after the call to completeRequest() below.
        EA::TDF::TdfPtr response = getCommandInfo().createResponse("Rpc Response");
        EA::TDF::TdfPtr errorResponse = getCommandInfo().createErrorResponse("Rpc Error Response");

        if (result == ERR_OK)
        {
            rpcContext.mRequest = getCommandInfo().requestProtobufToTdf ? getCommandInfo().requestProtobufToTdf(protobufMessage) : nullptr;
            rpcContext.mResponse = response.get();
            rpcContext.mErrorResponse = errorResponse.get();

            result = InboundRpcComponentGlue::processRequest(*getComponent(), rpcContext, getCommandInfo(), *blazeMessage, rpcContext.mRequest, rpcContext.mResponse, rpcContext.mErrorResponse);
        }

        mHandler->completeRequest(*this, result, rpcContext.mMovedTo, result == ERR_OK ? rpcContext.mResponse : rpcContext.mErrorResponse);
    }
}

Blaze::InetAddress InboundRpcBase::getInetAddressFromServerContext()
{
    // gRPC peer() string format is ipv4:<ipv4 address including port> or ipv6:<ipv6 address including port>.
    // Currently only ipv4 is supported. There is no technical limitation on supporting ipv6 other than making
    // rest of the code base understand ipv6.
    
    InetAddress peerAddr;
    if (mServerContext != nullptr)
    {
        auto peerStr = mServerContext->peer();
    
        // chop off the leading "ipv4:" and set rest as the IP:Port
        if (peerStr.substr(0, peerStr.find_first_of(":")) == grpc::string("ipv4"))
        {
            peerAddr.setHostnameAndPort(peerStr.substr(peerStr.find_first_of(":") + 1).c_str());
        }
    }
    return peerAddr;
}

void InboundRpcBase::initialize()
{
    // all grpc metadata key should be lower case - https://github.com/grpc/grpc/issues/9863 
    // Note that grpc::string_ref in requestMetadata map is NOT null terminated. 
    auto& requestMetadata = getServerContext()->client_metadata();

    BLAZE_SPAM_LOG(Log::GRPC, "[InboundRpcBase].initialize: requestMetadata for " << getName());
    for (auto& mapEntry : requestMetadata)
    {
        BLAZE_SPAM_LOG(Log::GRPC, FixedString32(mapEntry.first.data(), mapEntry.first.length()).c_str() << ":" << FixedString32(mapEntry.second.data(), mapEntry.second.length()).c_str());
    }

    Blaze::InetAddress peerAddr = getInetAddressFromServerContext();
    bool isTrustedClientIp = peerAddr.isValid() && getEndpoint().isTrustedClientIpAddress(peerAddr);

    // mTLS could indicate that the client itself is trusted, or it could only indicate that the request came from the ingress Envoy (from an untrusted Service Mesh client).
    // Either way, an mTLS client would have credible client metadata.
    // TODO: Add gRPC mTLS support.
    const bool isVerifiedMtlsPeer = false;

    // If the endpoint config does not automatically assume that any internal peer address is a trusted client,
    // then we need to at least rely on metadata from internal clients (i.e. ingress Envoy) to check the Envoy XFCC metadata
    // (or at least rely on the internal check until we have gRPC mTLS support).
    // TODO: Reevaluate this internal check after mTLS support is added.
    const bool isRequestMetadataCredible = isTrustedClientIp || getEndpoint().isInternalPeerAddress(peerAddr) || isVerifiedMtlsPeer;

    bool isEnvoyClient = false;
    bool isTrustedEnvoyClientCert = false;
    if (isRequestMetadataCredible)
    {
        BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].initialize: metadata for" << getName() << " is credible because isTrustedClientIp(" << isTrustedClientIp << ")"
            << ", internal peer address(" << getEndpoint().isInternalPeerAddress(peerAddr) << ")"
            << ", verified mTLS peer(" << isVerifiedMtlsPeer << ").");


        // We don't want to use the XFCC metadata unless we know the client is credible.
        if (getEndpoint().getEnvoyAccessEnabled())
        {
            auto it = requestMetadata.find("x-forwarded-client-cert");
            if (it != requestMetadata.end())
            {
                // Since the metadata is from a credible client,
                // the presence of this Envoy-specific metadata confirms the request came from an Envoy proxy.
                isEnvoyClient = true;

                // Peer IP address is the ingress Envoy and does not indicate if the client can be trusted.
                isTrustedClientIp = false;

                // Check the XFCC from Envoy to see if this is a trusted Envoy client.
                // Example x-forwarded-client-cert value from Envoy:
                // Hash=cdb15c74e943d625eb590dcea4ff8fd5350c20004dd736ba76b240af1f3c33a3;Subject="CN=EADP_GS_BLZDEV_COM_BLZ_SERVER.client.int.eadp.ea.com,OU=EA Online/Pogo.com,O=Electronic Arts\, Inc.,ST=California,C=US"
                eastl::string clientCertDetails(it->second.data(), it->second.length());

                // Get the hash value of the client's cert
                eastl::vector<eastl::string> certDetails;
                blaze_stltokenize(clientCertDetails, ";", certDetails);
                for (eastl::string& certDetail : certDetails)
                {
                    eastl::string::size_type kvDelimPos = certDetail.find_first_of("=");
                    if (kvDelimPos != eastl::string::npos)
                    {
                        eastl::string certDetailKey = certDetail.substr(0, kvDelimPos);
                        if (certDetailKey == "Hash")
                        {
                            // The hash value sent from Envoy can be generated by running:
                            // openssl x509 -in <clientCertFileName> -outform DER | openssl dgst -sha256 | cut -d" " -f2
                            eastl::string clientCertHash = certDetail.substr(kvDelimPos + 1);
                            isTrustedEnvoyClientCert = getEndpoint().isEnvoyXfccTrustedHash(clientCertHash);
                            if (isTrustedEnvoyClientCert)
                            {
                                BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].initialize: xfcc hash (" << clientCertHash << ") for " << getName() << " is trusted.");
                            }

                            break;
                        }
                    }
                }
            }
        }

        // We don't want to override the client IP using the XFF metadata from non-credible external clients.
        {
            auto it = isEnvoyClient ? requestMetadata.find("x-envoy-external-address") : requestMetadata.find("x-forwarded-for");
            if (it != requestMetadata.end())
            {
                FixedString256 value(it->second.data(), it->second.length());
                HttpProtocolUtil::getClientIPFromXForwardForHeader(value.c_str(), mFrame.clientAddr);

                // Now that we have the actual client IP, reevaluate if this is a trusted client IP
                if (mFrame.clientAddr.isValid())
                {
                    isTrustedClientIp = getEndpoint().isTrustedClientIpAddress(mFrame.clientAddr);
                    if (isTrustedClientIp)
                    {
                        BLAZE_TRACE_LOG(Log::GRPC, "[InboundRpcBase].initialize: the real peer (" << mFrame.clientAddr << ") for " << getName() << " is a trusted IP.");
                    }
                }
            }
        }
    }

    {
        auto it = requestMetadata.find("x-blaze-seqno");
        if (it != requestMetadata.end())
        {
            uint32_t seqno = 0;
            FixedString16 value((*it).second.data(), (*it).second.length());
            blaze_str2int(value.c_str(), &seqno);
            mFrame.msgNum = seqno;
        }
        else
        {
            mFrame.msgNum = mHandler->getNextMsgNum();
        }
    }

    {
        // Note that WAL uses BLAZE-SESSION but that is a mistake and not what we document. But that code has been there for many years so not changing that.
        // But still using the right header name for the grpc. 
        auto it = requestMetadata.find("x-blaze-session");
        if (it != requestMetadata.end())
        {
            mFrame.sessionKey.assign(it->second.data(), it->second.length());
            mFrame.userSessionId = mHandler->getPeerInfo().getUserSessionId(mFrame.sessionKey.c_str(), 0);
        }
    }

    {
        auto it = requestMetadata.find("x-blaze-id");
        if (it != requestMetadata.end() && it->second.length() <= (MAX_SERVICENAME_LENGTH - 1)) // subtract 1 for the null terminator
        {
            mFrame.serviceName.assign(it->second.data(), it->second.length());
        }
        else
        {
            mFrame.serviceName.assign(gController->getDefaultServiceName());
        }
    }

    {
        auto it = requestMetadata.find("x-blaze-locale");
        if (it != requestMetadata.end() && it->second.length() == 4)
        {
            mFrame.locale = LocaleTokenCreateFromStrings(it->second.data(), it->second.data() + 2);
        }
    }

    {
        auto it = requestMetadata.find("authorization");
        if (it != requestMetadata.end())
        {
            FixedString256 value((*it).second.data(), (*it).second.length());
            value.trim();

            auto tokenTypeEnd = value.find_first_of(" ");
            if (tokenTypeEnd != FixedString256::npos)
            {
                mFrame.accessTokenType = OAuth::OAuthUtil::getTokenType(value.substr(0, tokenTypeEnd).c_str());

                if (mFrame.accessTokenType != OAuth::TOKEN_TYPE_NONE)
                {
                    FixedString256 accessToken(value.substr(tokenTypeEnd + 1, value.size()));
                    accessToken.trim();
                    mFrame.accessToken.assign(accessToken.c_str());
                }
            }

            if (mFrame.accessTokenType == OAuth::TOKEN_TYPE_NONE || mFrame.accessToken.empty())
            {
                BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].initialize: authorization header value (" << value.c_str() << ")could not be understood.");
            }
        }
    }

    bool isTrustedClient = false;
    if (isEnvoyClient)
    {
        // The peer is actually an ingress Envoy proxying a request from a Service Mesh client.
        // Both the client IP and x-forwarded-client-cert header must indicate that the client is trusted.
        isTrustedClient = (isTrustedClientIp && isTrustedEnvoyClientCert);
        if (!isTrustedClient)
        {
            BLAZE_WARN_LOG(Log::GRPC, "[InboundRpcBase].initialize: Request coming from Envoy for the peer (" << peerAddr << ":" << mFrame.clientAddr
                << ") for " << getName() << " is not a trusted client because trustedClientIp(" << isTrustedClientIp << "), trustedXfccHash(" << isTrustedEnvoyClientCert << ").");
        }

    }
    else // Not an Envoy client
    {
        if (isVerifiedMtlsPeer)
        {
            // If the client passed mTLS, then the client is a trusted mTLS peer
            // (since we know the client is not the ingress Envoy proxy).
            isTrustedClient = true;
        }
        else
        {
            // Without mTLS and without Envoy,
            // only use the client IP to determine if the client is trusted.
            isTrustedClient = isTrustedClientIp;
        }
    }
    BLAZE_SPAM_LOG(Log::GRPC, "[InboundRpcBase].initialize: peer (" << peerAddr << ":" << mFrame.clientAddr
        << ") for " << getName() << " status - trustedClientIp(" << isTrustedClientIp << "), trustedXfccHash(" << isTrustedEnvoyClientCert << ")"
        << ", isVerifiedMtlsPeer(" << isVerifiedMtlsPeer << "), trustedClient(" << isTrustedClient << ")");

    mHandler->getPeerInfo().setTrustedClient(isTrustedClient);

    mComponent = InboundRpcComponentGlue::getComponent(getComponentId(), getEndpoint().getBindType());
}

bool InboundRpcBase::obfuscatePlatformInfo(EA::TDF::Tdf& tdf)
{
    if (!getCommandInfo().obfuscatePlatformInfo || getEndpoint().getBindType() == BIND_INTERNAL || mComponent == nullptr || !gController->isSharedCluster())
        return false;

    ClientPlatformType platform = gController->getServicePlatform(mFrame.serviceName.c_str());
    if (platform == common)
        return false;

    if (mComponent->isLocal())
        mComponent->asStub()->obfuscatePlatformInfo(platform, tdf);

    return true;
}

void InboundRpcBase::logRpcPreamble(const EA::TDF::Tdf* request)
{
    if (mCommandInfo.commandCreateFunc == nullptr) //grpcOnly path handled here, legacy path handled via ComponentStub
    {
        StringBuilder sb;
        if (BLAZE_IS_LOGGING_ENABLED(mCommandInfo.componentInfo->baseInfo.index, Logging::T_RPC))
        {
            BLAZE_TRACE_RPC_LOG(mCommandInfo.componentInfo->baseInfo.index, "<- request [component=" << mCommandInfo.componentInfo->loggingName <<
                " command=" << mCommandInfo.loggingName << " seqno=" << getName() << "]" <<
                ((request != nullptr) ? "\n" : "") << request);
        }
    }
    
}

void InboundRpcBase::logRpcPostamble(const EA::TDF::Tdf* response, const grpc::Status& status)
{
    if (mCommandInfo.commandCreateFunc == nullptr) //grpcOnly path handled here, legacy path handled via ComponentStub
    {
        StringBuilder sb;
        if (BLAZE_IS_LOGGING_ENABLED(mCommandInfo.componentInfo->baseInfo.index, Logging::T_RPC))
        {
            BLAZE_TRACE_RPC_LOG(mCommandInfo.componentInfo->baseInfo.index, "-> response [component=" << mCommandInfo.componentInfo->loggingName <<
                " command=" << mCommandInfo.loggingName << " seqno=" << getName() << " ec=" <<
                gRPCStatusCodeToString(status.error_code()) << "(" << status.error_code() << ")]" <<
                ((response != nullptr) ? "\n" : "") << response);
        }
    }

}

} //namespace Grpc
} //namespace Blaze


