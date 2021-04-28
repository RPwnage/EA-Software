/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/channel.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/connection/connection.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/selector.h"
#include "framework/connection/endpoint.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

namespace Blaze
{

ConnectionManager::ConnectionManager(Selector &selector)
    : mSelector(selector),
    mShutdown(false),
    mIdentFactory(1, Connection::CONNECTION_IDENT_MAX),
    mConnections(BlazeStlAllocator("ConnectionManager::mConnections")),
    mServerConnections(BlazeStlAllocator("ConnectionManager::mServerConnections")),
    mSlaveSessions(BlazeStlAllocator("ConnectionManager::mSlaveSessions")),
    mInactivityTimerId(INVALID_TIMER_ID),
    mNextConnectionBookmark(1),
    mShutdownTimeout(0),
    mShutdownStartTime(0)
{
    mEndpointGroup[0] = '\0';
}

ConnectionManager::~ConnectionManager()
{
    EndpointList::iterator ei = mEndpoints.begin();
    EndpointList::iterator ee = mEndpoints.end();
    for (; ei != ee; ++ei)
        delete *ei;

    // mConnections should be empty because Controller::shutdownControllerInternal() invokes ConnectionManager::shutdown() which has ensured
    // all existing connections are forced closed via forceShutdownAllConnections()
    EA_ASSERT(mConnections.empty());
}

bool ConnectionManager::initialize(
    uint16_t basePort, const RateConfig& rateConfig, const EndpointConfig& endpointConfig, const char8_t* endpointGroup)
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].initialize");

    blaze_strnzcpy(mEndpointGroup, endpointGroup, sizeof(mEndpointGroup));

    EndpointConfig::EndpointGroupsMap::const_iterator group = endpointConfig.getEndpointGroups().find(mEndpointGroup);
    const EndpointConfig::EndpointGroup::StringEndpointsList& names = group->second->getEndpoints();

    if (gProcessController->isUsingAssignedPorts())
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].initialize: Using " << (uint32_t)names.size() << " assigned ports");
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].initialize: Using port range: " << (uint32_t)basePort << "-"
            << ((uint32_t)basePort + names.size() - 1) << " (" << (uint32_t)names.size() << " ports)");
    }

    for (EndpointConfig::EndpointGroup::StringEndpointsList::const_iterator
        name = names.begin(), ename = names.end(); name != ename; ++name)
    {
        EndpointConfig::EndpointTypesMap::const_iterator itr = endpointConfig.getEndpointTypes().find(*name);
        
        if (itr == endpointConfig.getEndpointTypes().end())
            continue;

        uint16_t port = 0;
        if (gProcessController->isUsingAssignedPorts())
        {
            BindType bindType = itr->second->getBind();
            if (!gProcessController->getAssignedPort(bindType, port))
            {
                BLAZE_FATAL_LOG(Log::CONNECTION, "[ConnectionManager].initialize: No assigned ports available for endpoint "
                    << itr->first << " with bind type " << BindTypeToString(bindType));
                return false;
            }
        }
        else
        {
            port = basePort++;
        }

        Endpoint* endpoint = Endpoint::createFromConfig(itr->first, *itr->second, rateConfig, endpointConfig.getRpcControlLists(), endpointConfig.getPlatformSpecificRpcControlLists());
        if (endpoint == nullptr)
            return false;

        if (!endpoint->assignPort(port))
        {
            BLAZE_FATAL_LOG(Log::CONNECTION, "[ConnectionManager].initialize: Unable to "
                "assign port " << port << " to endpoint " << endpoint->getName());
            delete endpoint;
            return false;
        }

        ServerSocketChannel* serverChannel
            = static_cast<ServerSocketChannel*>(endpoint->getChannel());
        if (!serverChannel->initialize(this))
        {
            BLAZE_FATAL_LOG(Log::CONNECTION, "[ConnectionManager].initialize: Unable to "
                "initialize server socket.");
            delete endpoint;
            return false;
        }

        mEndpoints.push_back(endpoint);

        serverChannel->start();
    }

    mShutdownTimeout = endpointConfig.getShutdownTimeout();

    // Schedule a timer to expire inactive connections
    mInactivityTimerId = mSelector.scheduleTimerCall(
            TimeValue::getTimeOfDay() + (CONNECTION_INACTIVITY_PERIOD_MS * 1000),
            this, &ConnectionManager::expireInactiveConnections,
            "ConnectionManager::expireInactiveConnections");

    return true;
}

void ConnectionManager::reconfigure(const EndpointConfig& config, const RateConfig& rateConfig)
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].reconfigure");

    // NOTE: Currently no handling for new endpoints

    EndpointList::const_iterator i = mEndpoints.begin();
    EndpointList::const_iterator e = mEndpoints.end();
    for (; i != e; ++i)
    {
        EndpointConfig::EndpointTypesMap::const_iterator itr = config.getEndpointTypes().find((*i)->getName());
        if (itr == config.getEndpointTypes().end())
            continue;

        (*i)->reconfigure(*itr->second, rateConfig, config.getRpcControlLists(), config.getPlatformSpecificRpcControlLists());
    }

    // after re-configuring, go through each connection and set a new rate-limit counter based
    // on the changes that just occurred
    ConnectionsByIdent::iterator it = mConnections.begin();
    ConnectionsByIdent::iterator itend = mConnections.end();
    for ( ; it != itend; ++it)
    {
        InboundRpcConnection *connection = (*it).second;

        RatesPerIp *rateLimitCounter = connection->getEndpoint().getRateLimiter().getCurrentRatesForIp(connection->getPeerAddress().getIp());
        connection->setRateLimitCounter(rateLimitCounter);
    }

    mShutdownTimeout = config.getShutdownTimeout();
}

void ConnectionManager::onNewChannel(SocketChannel& channel, Endpoint& endpoint, RatesPerIp *rateLimitCounter)
{
    if (mShutdown)
    {
        BLAZE_SPAM_LOG(Log::CONNECTION, "[ConnectionManager].onNewChannel: rejecting a new connection due to the ConnectionManager is already shutdown / being shutting down.");

        // we own the channel, so delete it
        delete &channel;
        return;
    }
    
    // Create a new connection to handle this connection
    ConnectionId ident = mIdentFactory.allocate();

    InboundRpcConnection* connection = nullptr; 
    if (endpoint.getConnectionType() == EndpointConfig::CONNECTION_TYPE_CLIENT)
    {
        connection = BLAZE_NEW ClientInboundRpcConnection(*this, ident, channel, endpoint, rateLimitCounter);
    }
    else
    {
        connection = BLAZE_NEW ServerInboundRpcConnection(*this, ident, channel, endpoint, rateLimitCounter);        
    }

    char addrStr[64];
    channel.getPeerAddress().toString(addrStr, sizeof(addrStr));

    BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].onNewChannel: conn=" << SbFormats::Raw("0x%08x", connection->getIdent()) << " ident=" 
        << SbFormats::Raw("0x%08x", ident) << " channel=" << &channel << " addr=" << addrStr);
    
    // Keep track of the new connection
    mConnections[ident] = connection;
    if (connection->isServerConnection())
        mServerConnections[ident] = connection;
    
    // copy the connecting address to the presumed real address.
    channel.setRealPeerAddress(channel.getPeerAddress());
    BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].onNewChannel: Real IP: real address set to incoming one; addr=" << addrStr);
    // Connect the channel and start reading/writing
    connection->accept();
}

const Endpoint* ConnectionManager::findEndpoint(const char8_t* name) const
{
    EndpointList::const_iterator i = mEndpoints.begin();
    EndpointList::const_iterator e = mEndpoints.end();
    for (; i != e; ++i)
    {
        if (blaze_strcmp((*i)->getName(), name) == 0)
            return *i;
    }
    return nullptr;
}

void ConnectionManager::resumeEndpoints()
{
    EndpointList::const_iterator i = mEndpoints.begin();
    EndpointList::const_iterator e = mEndpoints.end();
    for (; i != e; ++i)
    {
        Endpoint* endpoint = *i;
        ServerSocketChannel* serverChannel
            = static_cast<ServerSocketChannel*>(endpoint->getChannel());
        serverChannel->resume();
    }
}

void ConnectionManager::pauseEndpoints()
{
    EndpointList::const_iterator i = mEndpoints.begin();
    EndpointList::const_iterator e = mEndpoints.end();
    for (; i != e; ++i)
    {
        Endpoint* endpoint = *i;
        ServerSocketChannel* serverChannel
            = static_cast<ServerSocketChannel*>(endpoint->getChannel());
        serverChannel->pause();
    }
}

InboundRpcConnection* ConnectionManager::getConnection(ConnectionId ident) const
{
    ConnectionsByIdent::const_iterator find = mConnections.find(ident);
    if (find == mConnections.end())
        return nullptr;

    if (find->second->isQueuedForDeletion())
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].getConnection: Connection ident=" << SbFormats::Raw("0x%08x", ident) << " is queued for deletion.");
        return nullptr;
    }
    return find->second;
}

void ConnectionManager::removeConnection(Connection& connection)
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].removeConnection: ident=" << SbFormats::Raw("0x%08x", connection.getIdent()));

    ConnectionId ident = connection.getIdent();
    ConnectionsByIdent::iterator find = mConnections.find(ident);
    if (find != mConnections.end())
        mConnections.erase(find);
    else
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[ConnectionManager].removeConnection: Attempted to remove unknown connection ident=" << SbFormats::Raw("0x%08x", ident));
        return;
    }

    if (connection.isServerConnection())
    {
        ConnectionsByIdent::iterator serverFind = mServerConnections.find(ident);
        if (serverFind != mServerConnections.end())
            mServerConnections.erase(serverFind);
    }    

    mIdentFactory.release(ident);
    delete &connection;

    // signal back that there's no more inbound connections left
    if (mShutdown && mConnections.empty())
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].removeConnection: Last inbound connection is removed. Ident=" << SbFormats::Raw("0x%08x", ident) << ". Signaling to proceed for the rest of graceful closure.");
        Fiber::signal(mAllConnsRemovedEvent, ERR_OK);
    }     
}

SlaveSessionPtr ConnectionManager::getSlaveSessionByInstanceId(InstanceId instanceId) const
{
    SlaveSessionsById::const_iterator iter = mSlaveSessions.find(instanceId);
    if (iter != mSlaveSessions.end())
        return iter->second;
    return nullptr;
}

void ConnectionManager::getStatusInfo(ConnectionManagerStatus& status) const
{
    uint64_t totalSendBytes = 0;
    uint64_t totalSendCount = 0;
    uint64_t totalRecvBytes = 0;
    uint64_t totalRecvCount = 0;
    uint64_t totalRpcFiberLimitExceededDropped = 0; 
    uint64_t totalRpcFiberLimitExceededPermitted = 0; 
    uint64_t numConnections = 0;

    for(auto& endpoint : mEndpoints)
    {
        totalSendBytes += endpoint->getTotalSendBytes();
        totalSendCount += endpoint->getTotalSendCount();
        totalRecvBytes += endpoint->getTotalRecvBytes();
        totalRecvCount += endpoint->getTotalRecvCount();
        totalRpcFiberLimitExceededDropped += endpoint->getTotalExceededRpcFiberLimitDropped();
        totalRpcFiberLimitExceededPermitted += endpoint->getTotalExceededRpcFiberLimitPermitted();
        numConnections += endpoint->getCurrentConnections();
    }

    status.setConnectionCount((uint32_t)numConnections);
    status.setTotalSendBytes(totalSendBytes);
    status.setTotalSendCount(totalSendCount);
    status.setTotalRecvBytes(totalRecvBytes);
    status.setTotalRecvCount(totalRecvCount);
    status.setInboundTotalRpcFiberLimitExceededDropped(totalRpcFiberLimitExceededDropped);
    status.setInboundTotalRpcFiberLimitExceededPermitted(totalRpcFiberLimitExceededPermitted);

    uint64_t totRej = 0, totRejMaxRate = 0;

    status.getEndpointsStatus().reserve(mEndpoints.size());

    for (EndpointList::const_iterator i = mEndpoints.begin(), e = mEndpoints.end(); i != e; ++i)
    {
        Endpoint* temp = *i;
        totRej += temp->getCountTotalRejects();
        totRejMaxRate += temp->getCountTotalRejectsMaxRateLimit();

        Blaze::EndpointStatus* epStatus = status.getEndpointsStatus().pull_back();
        epStatus->setName(temp->getName());
        if (temp->getConnectionType() == EndpointConfig::CONNECTION_TYPE_CLIENT)
        {
            epStatus->setType("client connection");
        }
        else
        {
            epStatus->setType("server connection");
        }
        epStatus->setPort(temp->getPort(InetAddress::HOST));
        epStatus->setConnectionNum((uint32_t)temp->getCurrentConnections());
        epStatus->setMaxConnectionNum(temp->getMaxConnections());
        epStatus->setHighWatermarks((uint32_t)temp->getHighWatermarks());
        epStatus->setTotalHighWatermarks(temp->getTotalHighWatermarks());
        epStatus->setTotalHighWatermarkDisconnects(temp->getTotalExceededDataCloses());
        epStatus->setTotalResumedConnections(temp->getTotalResumedConnections());
        epStatus->setTotalResumedConnectionsFailed(temp->getTotalResumedConnectionsFailed());
        epStatus->setTotalRpcAuthorizationFailureCount(temp->getTotalRpcAuthorizationFailureCount());
        epStatus->setTotalSendBytes(temp->getTotalSendBytes());
        epStatus->setTotalRecvBytes(temp->getTotalRecvBytes());

        status.getEndpointDrainStatus()[temp->getName()] = (gController->isDraining()) ? 0 : temp->getMaxConnections(); 
    
    }
    status.setTotalRejectMaxInboundConnections(totRej);
    status.setTotalRejectMaxRateLimitInboundConnections(totRejMaxRate);
}

void ConnectionManager::getMetrics(const GetConnMetricsRequest &request, GetConnMetricsResponse &response)
{
    uint32_t maxConns = (uint32_t)mConnections.size();
    if (maxConns > request.CONN_RESULT_LIMIT)
    {
        maxConns = request.CONN_RESULT_LIMIT;
    }
    response.getConnections().reserve(maxConns);
    
    // Check if the match value is IP:Port type
    InetAddress target(request.getMatchValue());
    bool isIp = target.hostnameIsIp();
    uint32_t reqPort = target.getPort(InetAddress::NET);
    uint32_t reqIp = isIp ? target.getIp(InetAddress::NET) : 0;

    ConnectionsByIdent::const_iterator itr = mConnections.begin();
    ConnectionsByIdent::const_iterator end = mConnections.end();
    for (; response.getConnections().size() < maxConns && itr != end; ++itr)
    {
        InboundRpcConnection* conn = itr->second;        
        // Perform the logic for checking whether each connection matching the value
        bool match = false;
        if (isIp)
        {
            match = (conn->getPeerAddress().getIp(InetAddress::NET) == reqIp);
            if (match && reqPort != 0)
                match = (conn->getPeerAddress().getPort(InetAddress::NET) == reqPort);
        }
        else if (blaze_stricmp(request.getMatchValue(), conn->getPeerName()) == 0)
        {
            match = true;
        }
        if (match)
        {
            ConnectionStatus* conStatus = response.getConnections().pull_back();
            conn->getStatusInfo(*conStatus);
        }
    }
}

void ConnectionManager::expireInactiveConnections()
{
    TimeValue currentTime = TimeValue::getTimeOfDay();

    ConnectionsByIdent::iterator i = mConnections.begin();
    ConnectionsByIdent::iterator e = mConnections.end();
    for (; i != e; ++i)
    {
        InboundRpcConnection* conn = i->second;
        TimeValue inactivityTimeout = conn->getEndpoint().getInactivityTimeout();

        if (conn->isClosed())
        {
            continue;
        }

        if (!conn->isInactivityTimeoutSuspended() && (inactivityTimeout > 0) && !conn->getIgnoreInactivityTimeout() &&
            ( (currentTime - conn->getLastActivityTime()) > inactivityTimeout ) && !conn->hasOutstandingCommands())
        {
            char namebuf[Connection::PEER_NAME_LEN];
            BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].expireInactiveConnections: "
                "Expiring inbound connection(" << conn->toString(namebuf, sizeof(namebuf)) << ") "
                "due to inactivity timeout");
            conn->setCloseReason(DISCONNECTTYPE_INACTIVITY_TIMEOUT, 0);
            conn->close();
        }
    }

    // Re-schedule this method to run again
    mInactivityTimerId = mSelector.scheduleTimerCall(
            TimeValue::getTimeOfDay() + (CONNECTION_INACTIVITY_PERIOD_MS* 1000),
            this, &ConnectionManager::expireInactiveConnections,
            "ConnectionManager::expireInactiveConnections");
}

void ConnectionManager::shutdown()
{
    if (!Fiber::isCurrentlyBlockable())
    {
        gSelector->scheduleFiberCall(this, &ConnectionManager::shutdown, "ConnectionManager::shutdown");
        return;
    }
    
    if (mShutdown)
    {
        return;
    }
    
    // IMPORTANT! We must set this value before closing out connections
    // because ConnectionManager::isActive() will be used to determine
    // whether closed connections are 'resumable'. Since we are shutting
    // down, none of the connections are allowed to be resumable!
    mShutdown = true;

    BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].shutdown: attempting to gracefully close " << mConnections.size() << " inbound connections, of which " << mServerConnections.size() << " are server connections.");

    ConnectionsByIdent::iterator i = mConnections.begin();
    ConnectionsByIdent::iterator e = mConnections.end();
    for (; i != e; ++i)
    {
        // initiate the graceful closure process on the connection
        i->second->gracefulClose();
    }

    // if all conns are gone already and that there aren't any connections removal queued, we are done!
    if (mConnections.empty() && !queuedRemovingConnections())
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[ConnectionManager].shutdown: all inbound connections are removed.");
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].shutdown: waiting for inbound connections to close. Inbound connections left: " << mConnections.size() << ", of which " << mServerConnections.size() << " are still in the server connections pool. Shutdown timeout: " << mShutdownTimeout.getSec() << "s.");
    
        mShutdownStartTime = TimeValue::getTimeOfDay();
    
        // wait for all conns to be removed (and destroyed), or timeout
        BlazeRpcError err = Fiber::getAndWait(mAllConnsRemovedEvent, "ConnectionManager::shutdown wait for all inbounds to be closed", mShutdownStartTime + mShutdownTimeout);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[ConnectionManager].shutdown: in the process of forced closed due to timeout. Inbound connections left:" << mConnections.size() << " of which " << mServerConnections.size() << " are server connections. Killing all inbound Rpc connections.");
            forceShutdownAllConnections();

            // connection removal are done on a separate fiber, so wait until they're all closed before we return from shutdown()
            if (queuedRemovingConnections())
            {
                Fiber::getAndWait(mAllConnsRemovedEvent, "ConnectionManager: wait for all inbound connections to be forced closed during shutdown.");
            }
        }
    }

    mSelector.cancelTimer(mInactivityTimerId);
    mInactivityTimerId = INVALID_TIMER_ID;
}

void ConnectionManager::forceShutdownAllConnections()
{
    ConnectionsByIdent::iterator i = mConnections.begin();
    ConnectionsByIdent::iterator e = mConnections.end();    
    for (; i != e; ++i)
    {
        const int32_t lastSockErr = i->second->getLastError();                
        BLAZE_INFO_LOG(Log::CONNECTION, "[ConnectionManager].forceShutdownAllConnections: closing connection: " << ConnectionSbFormat(i->second) << ". Last sockErr: " << lastSockErr << ". Socket shutting down: " << i->second->getBlockingSocket().isShuttingDown());        
        i->second->close(); 
    }
}

void ConnectionManager::addSlaveSession(SlaveSession& session)
{
    mSlaveSessions[GetInstanceIdFromInstanceKey64(session.getId())] = &session;
}

void ConnectionManager::removeSlaveSession(SlaveSession& session)
{
    SlaveSessionPtr sessionPtr = &session;

    mSlaveSessions.erase(GetInstanceIdFromInstanceKey64(session.getId()));

    // Calling blocking onSlaveSessionRemoved callbacks from a blocking fiber.
    // session will be destroyed when removeSlaveSession method is out of scope, but will not trigger callbacks
    gSelector->scheduleFiberCall(this, &ConnectionManager::destroySlaveSession, sessionPtr, "ConnectionManager::destroySlaveSession");
}

void ConnectionManager::destroySlaveSession(SlaveSessionPtr sessionPtr)
{
    sessionPtr->triggerOnSlaveSessionRemoved();
}

TimeValue ConnectionManager::getMaxEndpointCommandTimeout() const
{
    EndpointList::const_iterator i = mEndpoints.begin();
    EndpointList::const_iterator e = mEndpoints.end();

    TimeValue maxTimeout, currTimeout = 0;
    for (; i != e; ++i)
    {
        currTimeout = (*i)->getCommandTimeout();
        if (currTimeout > maxTimeout)
            maxTimeout = currTimeout;
    }
    
    return maxTimeout;
}

} // Blaze
