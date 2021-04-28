/*! ************************************************************************************************/
/*!
\file inboundrpcomponentglue.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/ratelimiter.h"
#include "framework/component/inboundrpccomponentglue.h"

#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/slivers/slivermanager.h"

namespace Blaze
{

FiberLocalPtr<RpcContext> gRpcContext;

RpcContext::RpcContext(const CommandInfo& cmdInfo, Message& message) :
    mCmdInfo(cmdInfo),
    mMessage(message),
    mMovedTo(INVALID_INSTANCE_ID)
{
    EA_ASSERT(gRpcContext == nullptr);
    gRpcContext = this;
}

RpcContext::~RpcContext()
{
    SAFE_DELETE_REF(mMessage);

    EA_ASSERT(gRpcContext == this);
    gRpcContext = nullptr;
}

InboundRpcConnectionRpcContext::InboundRpcConnectionRpcContext(const CommandInfo& cmdInfo, Message& message)
    : RpcContext(cmdInfo, message)
    , mRequest(nullptr)
    , mResponse(nullptr)
    , mErrorResponse(nullptr)
{

}

InboundRpcConnectionRpcContext::~InboundRpcConnectionRpcContext()
{

}


GrpcRpcContext::GrpcRpcContext(const CommandInfo& cmdInfo, Message& message)
    : RpcContext(cmdInfo, message)
    , mRequest(nullptr)
    , mResponse(nullptr)
    , mErrorResponse(nullptr)
{

}

GrpcRpcContext::~GrpcRpcContext()
{

}

BlazeRpcError InboundRpcComponentGlue::initRpcContext(RpcContext& rpcContext, PeerInfo& peerInfo)
{
    BlazeRpcError err = ERR_OK;

    UserSessionMasterPtr foundUserSession = nullptr;

    UserSessionId incomingUserSessionId = rpcContext.mMessage.getUserSessionId();
    if (UserSession::isValidUserSessionId(incomingUserSessionId))
    {
        // A valid UserSessionId has been provided on a Client connection.  We can't allow any further processing of this request
        // until we determine that the request is coming into the right server; that is, the Blaze instance that owns the UserSession.

        err = gSliverManager->waitForSliverAccess(UserSession::makeSliverIdentity(incomingUserSessionId), rpcContext.mSliverAccess, Fiber::getCurrentContext());
        if (err == ERR_OK)
        {
            if (rpcContext.mSliverAccess.getSliverInstanceId() == gController->getInstanceId())
            {
                foundUserSession = gUserSessionMaster->getLocalSession(incomingUserSessionId);
                if (foundUserSession != nullptr)
                {
                    // We need to verify a couple things based on whether or not the foundUserSession is bound to a connection.
                    if (foundUserSession->getPeerInfo() == nullptr)
                    {
                        // If the foundUserSession does not know of peer, then attach this peer.
                        foundUserSession->attachPeerInfo(peerInfo);
                        if (foundUserSession->getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
                        {
                            // The foundUserSession is part of a ConnectionGroup (e.g. multiple UserSessions associated with one peer), therefore
                            // we need to find the UserSessions in the group and attach them to this peer as well.
                            UserSessionIdList userSessionIds;
                            gUserSessionMaster->getConnectionGroupSessionIds(foundUserSession->getConnectionGroupObjectId(), userSessionIds);
                            for (UserSessionIdList::const_iterator it = userSessionIds.begin(), end = userSessionIds.end(); it != end; ++it)
                            {
                                if (foundUserSession->getUserSessionId() == *it)
                                    continue;

                                UserSessionMasterPtr localUserSession = gUserSessionMaster->getLocalSession(*it);
                                if ((localUserSession != nullptr) && (localUserSession->getPeerInfo() == nullptr))
                                    localUserSession->attachPeerInfo(peerInfo);
                            }
                        }

                        // We still need to check for nullptr here because the call to attachPeerInfo() above doesn't guarrantee this peer
                        // will be attached. And, if the peer is still nullptr, then we need to set/update the cleanup timer.
                        if (foundUserSession->getPeerInfo() == nullptr)
                            foundUserSession->setOrUpdateCleanupTimeout();
                    }
                    else if (foundUserSession->getPeerInfo() != &peerInfo && peerInfo.isPersistent())
                    {
                        // If the foundUserSession is already attached to a peer, but that peer is *not* the same as incoming peer,
                        // then that means this RPC request specified a SessionKey that resolved to a UserSession that is still attached
                        // to a different peer.  This should normally not happen, but it can happen if the user is doing intentional
                        // cable pulls, or some unintentional network interruption occurred.
                        char8_t peerInfoBufIncoming[256], peerInfoBufUserSession[256];
                        const RpcProtocol::Frame& frame = rpcContext.mMessage.getFrame();
                        BLAZE_INFO_LOG(Log::CONNECTION, "[PeerInfo:" << peerInfo.toString(peerInfoBufIncoming, sizeof(peerInfoBufIncoming)) << "].initRpcContext: Attempt to execute "
                            "command(" << BlazeRpcComponentDb::getComponentNameById(frame.componentId) << "::" << BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId) << ") "
                            "on this connection by UserSession(" << incomingUserSessionId << "/" << foundUserSession->getUserInfo().getPersonaName() << "), "
                            "but the UserSession is already bound to [PeerInfo:" << foundUserSession->getPeerInfo()->toString(peerInfoBufUserSession, sizeof(peerInfoBufUserSession)) << "]. "
                            "This usually indicates a network interruption between the client and server, and/or a possible fraud attempt.");

                        // Return a generic error because it is possible this is a fraud attempt.
                        err = ERR_SYSTEM;
                    }
                }
                else
                {
                    // We get here because the client sent us a session key that resolves to a SliverId owned by this
                    // instance, but his user session is not here. Perhaps it timed out, or his UserSession was destroyed
                    // for whatever reason. Tell the client he's no longer authenticated, and is required to login again.
                    err = ERR_AUTHENTICATION_REQUIRED;
                }
            }
            else
            {
                // The user session does not belong to the this coreSlave but instead has moved to another coreSlave. 
                rpcContext.mMovedTo = rpcContext.mSliverAccess.getSliverInstanceId();
                err = ERR_MOVED;
            }
        }
        else
        {
            // This shouldn't normally occur, but it is certainly inevitable *if* the entire Blaze cluster is shutting down.
            BLAZE_WARN_LOG(Log::SYSTEM, "RpcGlue::initRpcContext: waitForSliverAccess() failed with err(" << ErrorHelp::getErrorName(err) << "), "
                "when trying to sliver(" << GetSliverIdFromSliverKey(incomingUserSessionId) << ") in sliver namespace(" << BlazeRpcComponentDb::getComponentNameById(UserSessionsMaster::COMPONENT_ID) << ")");
        }
    }

    if (foundUserSession != nullptr)
    {
        if ((foundUserSession->getPeerInfo() != nullptr) && !foundUserSession->getPeerInfo()->isPersistent())
        {
            // Update the clean up timer as a new request has showed up for user session.
            foundUserSession->setOrUpdateCleanupTimeout();
        }

        UserSession::setCurrentUserSessionId(foundUserSession->getUserSessionId());
    }
    else
    {
        UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
    }

    return err;
}

BlazeRpcError InboundRpcComponentGlue::processRequest(Component& component, RpcContext& rpcContext, const CommandInfo& cmdInfo, Message& message, const EA::TDF::Tdf* request, EA::TDF::Tdf* response, EA::TDF::Tdf* errorResponse)
{
    RpcCallOptions opts;
    
    // We explicitly set followMovedTo to false here as this code flow is exercised directly from the client. In this case, client will fulfill it's ZDT contract and
    // establish a new session on another coreSlave.
    opts.followMovedTo = false; 
    opts.routeTo.setSliverIdentity(message.getFrame().sliverIdentity);
    return component.sendRequest(cmdInfo, request, response, errorResponse, opts, &message, &rpcContext.mMovedTo, nullptr, message.getPeerInfo().isClient() /*obfuscateResponse*/);
}

void InboundRpcComponentGlue::setLoggingContext(SlaveSessionId slaveSessionId, UserSessionId userSessionId, const char8_t* deviceId, const char8_t* personaName, const InetAddress& ipAddress, AccountId nucleusAccountId, ClientPlatformType platform)
{
    BlazeId blazeId = gUserSessionManager->getBlazeId(userSessionId);
    const char8_t* useDeviceId = deviceId;
    const char8_t* usePersonaName = personaName;
    AccountId useNucleusAccountId = nucleusAccountId;
    ClientPlatformType usePlatform = platform;
    if (gUserSessionMaster != nullptr)
    {
        UserSessionMasterPtr session = gUserSessionMaster->getLocalSession(userSessionId);
        if (session != nullptr)
        {
            useDeviceId = session->getUniqueDeviceId();
            usePersonaName = session->getUserInfo().getPersonaName();
            useNucleusAccountId = session->getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId();
            usePlatform = session->getUserInfo().getPlatformInfo().getClientPlatform();
        }
    }

    gLogContext->set(slaveSessionId, userSessionId, blazeId, useDeviceId, usePersonaName, ipAddress.getIp(InetAddress::HOST), useNucleusAccountId, usePlatform);
}

void InboundRpcComponentGlue::updatePeerServiceName(PeerInfo& peerInfo, const char8_t* requestedServiceName)
{
    if (requestedServiceName[0] != '\0')
    {
        const char8_t* serviceName = gController->resolveServiceName(requestedServiceName);
        peerInfo.setServiceName(serviceName);
    }
}

Component* InboundRpcComponentGlue::getComponent(ComponentId componentId, Blaze::BindType endpointBindType)
{
    // If you are a fire2 client (game or dedicated server client), you are always talking to coreSlave regardless of where the component handling the rpc is located. So for example, even if
    // you are calling an rpc on custom component, you simply make the request to coreSlave. BlazeSDK wraps all that. Once on the coreSlave, framework ensures that the request is handled on 
    // an instance where the component is located.
    // If you are an http client, you have the flexibility to call 'any instance' but you need to make sure that instance can handle the rpc you wish to execute. If unsure, call coreSlave. 

    // A request to handle an rpc has come to this instance. If the rpc is meant for a master component(which can be on a coreSlave) or anything other than coreSlave instance(so like a master or 
    // custom slave instance), the component must be loaded locally.  
    const bool requireLocal = RpcIsMasterComponent(componentId) || gController->getInstanceType() != ServerConfig::SLAVE;
    Component* component = gController->getComponent(componentId, requireLocal, true);

    // 1. (component != nullptr) - Does the cluster has this component somewhere in the instance we want? If no, complete the request with an error. If yes,
    // 2. !component->isLocal() - If the component is not local, let's go ahead and make the request. The instance handling it will decide what to do with it. If local, 
    // this instance should handle the request so let's evaluate other conditions.
    // 3. || !component->isMaster() - If it is slave instance, we can handle rpc request coming on any endpoint. Let's go ahead and handle it. If not slave (meaning if master)
    // 4. (endpointBindType == Blaze::BIND_INTERNAL) - On master, make sure that request is coming at an internal endpoint. If yes, go ahead and process the request. 

    if (component != nullptr &&
        (!component->isLocal() || !component->isMaster() || (endpointBindType == Blaze::BIND_INTERNAL)))
    {
        return component;
    }
    
    return nullptr;
}

FixedString32 InboundRpcComponentGlue::getCurrentUserSessionDeviceId(UserSessionId userSessionId)
{
    const char8_t* deviceId = "(unknown)";
    if (gUserSessionMaster != nullptr)
    {
        UserSessionMasterPtr currentLocalUserSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (currentLocalUserSession != nullptr && currentLocalUserSession->getUniqueDeviceId()[0] != '\0')
            deviceId = currentLocalUserSession->getUniqueDeviceId();
    }

    return FixedString32(deviceId);
}

bool InboundRpcComponentGlue::hasReachedFiberLimit(ComponentId componentId, bool& isFrameworkCommand, FixedString32& droppedOrPermitted)
{
    if (gFiberManager->getCommandFiberCount() >= gFiberManager->getMaxCommandFiberCount())
    {
        if (RpcIsFrameworkComponent(componentId))
        {
            droppedOrPermitted.assign("permitted framework");
            isFrameworkCommand = true;
        }
        else
        {
            droppedOrPermitted.assign("dropped");
        }

        return true;
    }

    return false;
}

Fiber::CreateParams InboundRpcComponentGlue::getCommandFiberParams(bool isLocal, const CommandInfo& cmdInfo, TimeValue commandTimeout, bool blocksCaller, Fiber::FiberGroupId groupId)
{
    Fiber::CreateParams fiberParams;
    
    fiberParams.relativeTimeout = (cmdInfo.fiberTimeoutOverride != 0) ? cmdInfo.fiberTimeoutOverride : commandTimeout;
    fiberParams.namedContext = cmdInfo.context;
    fiberParams.blocksCaller = blocksCaller; //If we're doing one at a time, wait for the first request to finish before taking another
    fiberParams.groupId = groupId;
    fiberParams.blocking = true; // fiber can always block, we disable blocking later
    if (isLocal)
    {
        fiberParams.stackSize = cmdInfo.fiberStackSize;
    }
    else
    {
        //Non local commands are always just simple passthroughs
        fiberParams.stackSize = Fiber::STACK_SMALL;
    }

    return fiberParams;
}

bool InboundRpcComponentGlue::hasExceededCommandRate(RateLimiter& rateLimiter, RatesPerIp* rateCounter, const CommandInfo& cmdInfo, bool checkOverall)
{
    if (rateCounter != nullptr && rateLimiter.isEnabled())
    {
        if (checkOverall)
        {
            return !rateLimiter.tickAndCheckOverallCommandRate(*rateCounter);
        }
        else
        {
            return !rateLimiter.tickAndCheckCommandRate(*rateCounter, cmdInfo.componentInfo->id, cmdInfo.commandId);
        }
    }

    return false;
}
}