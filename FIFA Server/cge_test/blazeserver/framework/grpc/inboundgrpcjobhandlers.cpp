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

void InboundRpcBase::processRequestOnFiber(const google::protobuf::Message* protobufMessage)
{
    if (EA_UNLIKELY(gController->isShuttingDown()))
    {
        BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequestOnFiber: Scheduling new command fiber while this instance is shutting down for request: command=" << getCommandInfo().loggingName);
    }

    Fiber::CreateParams fiberParams = InboundRpcComponentGlue::getCommandFiberParams(
        true, getCommandInfo(), getEndpoint().getCommandTimeout(), false, mHandler->getProcessMessageFiberGroupId());

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
        BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequestInternal: Processing command fiber while this instance is shutting down for request: command=" << getCommandInfo().loggingName);
    }

    // For Grpc clients, if exceeding rate limit, we sleep in processRequest instead of processMessage (unlike InboundRpcConnection).
    // The reason being that processMessage executes on main selector loop instead of it's own fiber. If we made our fiber sleep there, we'll 
    // sleep the entire instance. 
    // It's also not wrong to introduce this sleep here because with grpc, a client could potentially be running multiple rpcs and net goal is 
    // to slow down a particular client. We'd be giving that client a slightly lesser command timeout as a result of this sleep but there is nothing
    // wrong with penalizing an abusive client this way.
    if (InboundRpcComponentGlue::hasExceededCommandRate(getEndpoint().getRateLimiter(), mHandler->getRateLimitCounter(), getCommandInfo(), false))
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequest: "
            "Specific RPC rate limit of for command " << getCommandInfo().loggingName << "(" << getCommandId() << ")" <<
            " was exceeded for RateLimiter " << getEndpoint().getRateLimiter().getName());

        Fiber::sleep(RATE_LIMIT_GRACE_PERIOD * 1000 * 1000, "InboundGrpcRequestHandler command rate check squelch");
    }

    if (InboundRpcComponentGlue::hasExceededCommandRate(getEndpoint().getRateLimiter(), mHandler->getRateLimitCounter(), getCommandInfo(), true))
    {
        BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequest: "
            "Total RPC rate limit of (" << getEndpoint().getRateLimiter().getCommandRateLimit() << ") was exceeded for RateLimiter "
            << getEndpoint().getRateLimiter().getName());

        Fiber::sleep(RATE_LIMIT_GRACE_PERIOD * 1000 * 1000, "InboundGrpcRequestHandler overall rate check squelch");
    }

    if (getCommandInfo().commandCreateFunc == nullptr && getIdentityContext() == nullptr && getCommandInfo().requiresAuthentication)
    {
        Blaze::OAuth::OAuthSlave *oAuthSlave = (Blaze::OAuth::OAuthSlave*) gController->getComponent(Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
        if (oAuthSlave == nullptr)
        {
            BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequest: OAuth component is null. Can't crack access token!");

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

        OAuth::IdentityContext identityContext;
        BlazeRpcError err = oAuthSlave->getIdentityContext(identityContextReq, identityContext);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::GRPC, "[InboundGrpcRequestHandler].processRequest: "
                "The command " << BlazeRpcComponentDb::getCommandNameById(getComponentId(), getCommandId()) << "(" << getCommandId() << ")" <<
                " requires authentication but the provided token is not valid/does not have sufficient privileges.");

            mHandler->completeRequest(*this, (err == OAUTH_ERR_INSUFFICIENT_SCOPES ? ERR_AUTHORIZATION_REQUIRED : ERR_AUTHENTICATION_REQUIRED), INVALID_INSTANCE_ID, nullptr);
            return;
        }

        setIdentityContext(identityContext);
    }

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

    if (mIsTrustedEnvoyClient)
    {
        mHandler->getPeerInfo().setTrustedClient(true);
    }

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
    bool isTrustedPeer = false;
    Blaze::InetAddress peerAddr = getInetAddressFromServerContext();
    if (peerAddr.isValid())
    {
        isTrustedPeer = getEndpoint().isTrustedAddress(peerAddr);
    }

    // Note that this does not indicate that a gRPC request on this endpoint came from Envoy.
    // Instead, it indicates that this is a gRPC endpoint used by Envoy.
    // (It also indicates that the client is a trusted peer on an internal secure endpoint.)
    const bool isTrustedInternalSecureEnvoyEndpoint = getEndpoint().isTrustedInternalSecureEnvoyEndpoint();

    // all grpc metadata key should be lower case - https://github.com/grpc/grpc/issues/9863 
    // Note that grpc::string_ref in requestMetadata map is NOT null terminated. 
    auto& requestMetadata = getServerContext()->client_metadata();

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

    if (isTrustedPeer)
    {
        // Check for the Envoy specific external address before trying to use the XFF.
        auto it = requestMetadata.end();
        if (isTrustedInternalSecureEnvoyEndpoint)
        {
            it = requestMetadata.find("x-envoy-external-address");
        }

        if (it == requestMetadata.end())
        {
            it = requestMetadata.find("x-forwarded-for");
        }
        if (it != requestMetadata.end())
        {
            FixedString256 value(it->second.data(), it->second.length());
            HttpProtocolUtil::getClientIPFromXForwardForHeader(value.c_str(), mFrame.clientAddr);
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
                BLAZE_WARN_LOG(Log::GRPC, "[InboundGrpcRequestHandler].createRequestFrame: authorization header value (" << value.c_str() << ")could not be understood.");
            }
        }
    }

    // Check the XFCC from Envoy to see if this is a trusted Envoy client.
    if (isTrustedPeer && isTrustedInternalSecureEnvoyEndpoint)
    {
        auto it = requestMetadata.find("x-forwarded-client-cert");
        if (it != requestMetadata.end())
        {
            // Example x-forwarded-client-cert value from Envoy:
            // Hash=2f516409649ddb3617d68088f44cc7346f8e6d29cd536381c76b5799d9dc51b6;Subject="CN=gos-blazeserver-dev2nexus.clinet.int.eadp.ea.com,OU=EA Online/Pogo.com,O=Electronic Arts\, Inc.,ST=California,C=US"
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
                        mIsTrustedEnvoyClient = getEndpoint().isEnvoyXfccTrustedHash(clientCertHash);
                        break;
                    }
                }
            }
        }
    }

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
} //namespace Grpc
} //namespace Blaze


