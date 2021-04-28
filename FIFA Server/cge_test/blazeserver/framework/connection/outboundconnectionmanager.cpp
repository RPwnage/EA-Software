/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OutboundConnectionManager

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/selector.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/connection/outboundrpcconnection.h"
#include "framework/util/random.h"
#include "redirector/rpc/redirectorslave.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/

OutboundConnectionManager::OutboundConnectionManager()
    : mShutdown(false),
      mEndpoint("outboundRpcFire2", Fire2Protocol::getClassName(), Heat2Encoder::getClassName(),
      Heat2Decoder::getClassName(), 16 * 1024 * 1024),
      mIdentFactory(1),
      mConnectionsByInstanceId(BlazeStlAllocator("mConnectionsByInstanceId")),
      mProxy(nullptr),
      mRedirEndpoint("outboundRpcFire", FireProtocol::getClassName(), Heat2Encoder::getClassName(), Heat2Decoder::getClassName(), 16 * 1024 * 1024),
      mRedirectorRpcProxyResolver(nullptr)
{
}

OutboundConnectionManager::~OutboundConnectionManager()
{
    if (mProxy != nullptr)
        delete mProxy;

    if (mRedirectorRpcProxyResolver != nullptr)
        delete mRedirectorRpcProxyResolver;

    // mConnectionsByAddress should be empty because Controller::shutdownControllerInternal() invokes OutboundConnectionManager::shutdown() which has ensured
    // all existing connections are forced closed via shutdown()
    EA_ASSERT(mConnectionsByAddress.empty());
}

void OutboundConnectionManager::setRedirectorAddr(const InetAddress& redirectorAddr)
{
    mRedirAddress = redirectorAddr;
}

OutboundRpcConnection* OutboundConnectionManager::getConnection(const InetAddress& target) const
{
    if (mShutdown)
        return nullptr;

    ConnectionsByAddress::const_iterator find = mConnectionsByAddress.find(&target);
    if (find == mConnectionsByAddress.end())
        return nullptr;
    return find->second;
}

void OutboundConnectionManager::bindConnectionToInstanceId(OutboundRpcConnection& conn, InstanceId instanceId)
{
    mConnectionsByInstanceId[instanceId] = &conn;
    conn.setInstanceId(instanceId);
}

OutboundRpcConnection* OutboundConnectionManager::getConnectionByInstanceId(InstanceId id) const
{
    if (mShutdown)
        return nullptr;

    ConnectionsById::const_iterator find = mConnectionsByInstanceId.find(id);
    if (find == mConnectionsByInstanceId.end())
        return nullptr;
    return find->second;
}

OutboundRpcConnection* OutboundConnectionManager::createConnection(
        const InetAddress& target, Endpoint* endpoint /* = nullptr */)
{
    if (mShutdown)
        return nullptr;
    
    ConnectionsByAddress::iterator find = mConnectionsByAddress.find(&target);
    if (find != mConnectionsByAddress.end())
        return nullptr;

    // No connection exists to this endpoint yet so create one
    // The SocketChannel becomes owned by Connection::mBlockingSocket, and is deleted in the BlockingSocket destructor
    SocketChannel* sok = BLAZE_NEW SocketChannel(target);
    // the target address is already resolved (would have asserted in the find() above if not
    // we don't want to resolve again at connection time because that could change sok->getPeerAddress()
    // which is the key to our mConnectionsByAddress map
    sok->setResolveAlways(false);

    ConnectionId connId = mIdentFactory.allocate();

    OutboundRpcConnection* connection = BLAZE_NEW OutboundRpcConnection(
            (*this), connId, *sok, endpoint != nullptr ? *endpoint : mEndpoint);

    char addr[64];
    BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundConnectionManager].createConnection: creating connection; sess=" << connection << " ident=" 
                    << SbFormats::Raw("0x%08x", connId) << " channel=" << sok << " addr=" << sok->getPeerAddress().toString(addr, sizeof(addr)));

    mConnectionsByAddress[&sok->getPeerAddress()] = connection;

    // Try and connect the socket
    if (!connection->connect())
    {
        // We failed.  Set conection to nullptr.  It will be cleaned up in onConnectionDisconnected() which is called
        // during the handling of the connection attempt failure by Connection::close().
        connection = nullptr;
    }
#ifdef EA_PLATFORM_LINUX
    else
    {
        connection->getChannel().setCloseOnFork();
    }
#endif
   
    return connection;
}

bool OutboundConnectionManager::isFatalRedirectorError(BlazeRpcError rc)
{
    return (rc == ERR_DISCONNECTED) || (rc == ERR_SYSTEM);
}

void OutboundConnectionManager::disconnectRedirectorSlaveProxy()
{
    OutboundRpcConnection* conn = getConnection(mRedirAddress);
    if (conn != nullptr)
    {
        char8_t addr[64];
        BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundConnectionManager].disconnectRedirectorSlaveProxy: close rdir connection; sess=" << conn << " ident=" 
            << SbFormats::Raw("0x%08x", conn->getIdent()) << " addr=" << mRedirAddress.toString(addr, sizeof(addr)));
        conn->close();
    }
}

Redirector::RedirectorSlave* OutboundConnectionManager::getRedirectorSlaveProxy()
{
    OutboundRpcConnection* conn = getConnection(mRedirAddress);

    // We can only safely delete the redirector if the connection is closed (indicating that Connection::readLoop is no longer being run).
    if (conn == nullptr || conn->isClosed())
    {
        char8_t addr[64];

        // delete the existing proxy so that we can recreate it with the latest mRedirAddress
        if (mProxy != nullptr)
        {
            delete mProxy;
            mProxy = nullptr;
        }

        if (mRedirectorRpcProxyResolver != nullptr)
        {
            delete mRedirectorRpcProxyResolver;
            mRedirectorRpcProxyResolver = nullptr;
        }

        // re-resolve the address before we try to create a new connection in case there's been a DNS change.
        // At the socketchannel level, we will always resolve addr before doing connect(). And after connection is created
        // we add it to the map mConnectionsByAddress[] keyed by &sok->getPeerAddress().  Therefore, if we don't resolve mRedirAddress
        // here, we could have created a connection and added it to mConnectionsByAddress[] but won't be able to find that address
        // using mRedirAddress due to the possible difference in resolved IP address.
        NameResolver::LookupJobId jobId;
        BlazeRpcError err =  gNameResolver->resolve(mRedirAddress, jobId);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[OutboundConnectionManager].getRedirectorSlaveProxy: could not resolve "
                            << "redirector slave server address: "<< mRedirAddress.toString(addr, sizeof(addr)));
            return nullptr;
        }
    }

    if (mProxy == nullptr)
    {
        if (mRedirectorRpcProxyResolver == nullptr)
        {
            mRedirectorRpcProxyResolver = BLAZE_NEW RedirectorRpcProxyResolver(*this, mRedirAddress, mRedirEndpoint);
        }
        mProxy = BLAZE_NEW Redirector::RedirectorSlave(*mRedirectorRpcProxyResolver, mRedirAddress);
    }

    return mProxy;
}

BlazeRpcError OutboundConnectionManager::getInetAddressByServiceName(
    const Redirector::ServerAddressMapRequest& request, InetAddress& address, int32_t instanceIndex /* = -1*/)
{
    Redirector::RedirectorSlave* proxy = getRedirectorSlaveProxy();
    if (proxy == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundConnectionManager::getInetAddressByServiceName]: Attempt to connect to rdir without initializing manager with rdir slave address.");
        return ERR_SYSTEM;
    }

    Redirector::ServerAddressMapResponse response;
    BlazeRpcError error = proxy->getServerAddressMap(request, response);
    if ((error == ERR_OK) &&
        (!response.getAddressMap().empty()))
    {
        Redirector::ServerAddressMapData* mapData = nullptr; 
        if (instanceIndex < 0 || instanceIndex >= (int32_t) response.getAddressMap().size())
        {
            Redirector::InstanceNameAddressMap::iterator itr = response.getAddressMap().begin();
            Redirector::InstanceNameAddressMap::iterator end = response.getAddressMap().end();  
            mapData = itr->second;
            for (; itr != end; ++itr)
            {
                if(itr->second->getLoad() < mapData->getLoad())
                {
                    mapData = itr->second;
                }
            }
        }
        else
        {
            mapData = response.getAddressMap().at(static_cast<uint32_t>(instanceIndex)).second;
        }
        
        const Redirector::ServerAddressInfo& addrInfo = mapData->getServerAddressInfo();
        address.setHostname(addrInfo.getAddress().getIpAddress()->getHostname());
        address.setPort(addrInfo.getAddress().getIpAddress()->getPort(), InetAddress::HOST);
        address.setIp(addrInfo.getAddress().getIpAddress()->getIp(), InetAddress::HOST);
    }
    else if (isFatalRedirectorError(error))
    {
        disconnectRedirectorSlaveProxy();
    }
    return error;
}

OutboundRpcConnection* OutboundConnectionManager::getConnectionByServiceName(const Redirector::ServerAddressMapRequest& request, int32_t instanceIndex /*= -1*/)
{
    InetAddress address;
    BlazeRpcError error = getInetAddressByServiceName(request, address, instanceIndex);
    if (error != ERR_OK)
        return nullptr;
    return getConnection(address);
}

OutboundRpcConnection* OutboundConnectionManager::createConnectionByServiceName(const char8_t* serviceName, Redirector::ServerAddressType addrType,
                                                                                int32_t instanceIndex /*= -1*/, Endpoint* endpoint /*= nullptr*/)
{
    InetAddress address;
    Redirector::ServerAddressMapRequest request;
    if (endpoint == nullptr)
        endpoint = &mEndpoint;
    request.setChannel(endpoint->getChannelType());
    request.setProtocol(endpoint->getProtocolName());
    request.setAddressType(Redirector::INTERNAL_IPPORT);
    request.setName(serviceName);

    BlazeRpcError error = getInetAddressByServiceName(request, address, instanceIndex);
    if (error != ERR_OK)
        return nullptr;
    return createConnection(address, endpoint);
}

void OutboundConnectionManager::removeConnection(Connection& connection)
{
    ConnectionsByAddress::iterator i = mConnectionsByAddress.begin();
    ConnectionsByAddress::iterator e = mConnectionsByAddress.end();
    bool found = false;
    for (; i != e; ++i)
    {
        if (i->second == &(OutboundRpcConnection&)connection)
        {
            mConnectionsByAddress.erase(i);
            mConnectionsByInstanceId.erase(((OutboundRpcConnection&) connection).getInstanceId());
            found = true;
            break;
        }
    }

    const ConnectionId connId = connection.getIdent();

    if (!found)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundConnectionManager].removeConnection: Attempted to remove unknown connection ident=" << SbFormats::Raw("0x%08x", connId));
        return;
    }
    
    delete &connection;

    // signal that there's no more outbound connections left
    if (mShutdown && mConnectionsByAddress.empty())
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundConnectionManager].removeConnection: Last outbound connection is removed. Ident=" << SbFormats::Raw("0x%08x", connId) << ". Signaling to proceed for the rest of server shutdown.");
        Fiber::signal(mAllConnsRemovedEvent, ERR_OK);
    }
}

RpcProxySender* OutboundConnectionManager::getProxySender(const InetAddress& addr)
{
    OutboundRpcConnection *conn = getConnection(addr);

    if (conn == nullptr)
    {
        conn = createConnection(addr, &mEndpoint);
    }

    if (conn != nullptr && conn->isConnected())
        return conn;

    return nullptr;
}

void OutboundConnectionManager::getOutConnMetricsInfo(const GetConnMetricsRequest &request, GetConnMetricsResponse &response)
{
    uint32_t maxConns = (uint32_t)mConnectionsByAddress.size();
    if (maxConns > request.CONN_RESULT_LIMIT)
    {
        maxConns = request.CONN_RESULT_LIMIT;
    }
    response.getConnections().reserve(maxConns);    
    
    InetAddress target(request.getMatchValue());

    // Check if the match value is IP:Port type
    if (target.hostnameIsIp()) // Make sure the hostname is IP, since find operation requires Ip
    {
        if (target.getPort(InetAddress::NET) != 0)
        {
            ConnectionsByAddress::iterator find = mConnectionsByAddress.find(&target);
            if (find != mConnectionsByAddress.end())
            {
                ConnectionStatus* conStatus = response.getConnections().pull_back();
                find->second->getStatusInfo(*conStatus);
            }
        }
        else
        {
            // Check if the match value is IP
            const uint32_t targetIp = target.getIp(InetAddress::NET);
            ConnectionsByAddress::const_iterator itr = mConnectionsByAddress.lower_bound(&target);
            ConnectionsByAddress::const_iterator end = mConnectionsByAddress.end();
            for (; response.getConnections().size() < maxConns && itr != end; ++itr)
            {
                uint32_t ip = itr->first->getIp(InetAddress::NET);
                if (ip == targetIp)
                {
                    ConnectionStatus* conStatus = response.getConnections().pull_back();
                    itr->second->getStatusInfo(*conStatus);
                }
                else
                    break;
            }
        }

    }
    else
    {
        // The match value is PeerName
        ConnectionsByAddress::const_iterator itr = mConnectionsByAddress.begin();
        ConnectionsByAddress::const_iterator end = mConnectionsByAddress.end();
        for (; response.getConnections().size() < maxConns && itr != end; ++itr)
        {
            if (blaze_stricmp(request.getMatchValue(), itr->second->getPeerName()) == 0)
            {
                ConnectionStatus* conStatus = response.getConnections().pull_back();
                itr->second->getStatusInfo(*conStatus);
            }
        }
    }
}

void OutboundConnectionManager::shutdown()
{
    if (!Fiber::isCurrentlyBlockable())
    {
        gSelector->scheduleFiberCall(this, &OutboundConnectionManager::shutdown, "OutboundConnectionManager::shutdown");
    }
    else
    {
        mShutdown = true;
        
        BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundConnectionManager].shutdown: force closing " << mConnectionsByAddress.size() << " outbound connections.");
        
        // NOTE: mConnectionsByAddress is guaranteed not mutated while calling Connection::close(); therfore, we can cache the iterators
        ConnectionsByAddress::iterator i = mConnectionsByAddress.begin();
        ConnectionsByAddress::iterator e = mConnectionsByAddress.end();
        for (; i != e; ++i)
        {
            const int32_t lastSockErr = i->second->getLastError();
            BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundConnectionManager].shutdown: closing connection: " << ConnectionSbFormat(i->second) << ". Last sockErr: " << lastSockErr << ". Socket shutting down: " << i->second->getBlockingSocket().isShuttingDown());        
            i->second->close();
        }

        // if all conns are gone already and that there aren't any connections removal queued, we are done!
        if (mConnectionsByAddress.empty() && !queuedRemovingConnections())
        {
            BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundConnectionManager].shutdown: all outbound connections are removed.");
        }
        else
        {
            BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundConnectionManager].shutdown: waiting for outbound connections to be deleted. Outbound connections left: " << mConnectionsByAddress.size());
            // connections removal are done on a separate fiber, so wait until they're all closed before we return from shutdown()
            if (queuedRemovingConnections())
            {  
                Fiber::getAndWait(mAllConnsRemovedEvent, "OutboundConnectionManager: wait for all outbound connections to be forced closed during shutdown");
            }            
        }
    }
}

RedirectorRpcProxyResolver::RedirectorRpcProxyResolver(OutboundConnectionManager& outboundConnectionManager, const InetAddress& address, Endpoint& endpoint)
    : mOwner(outboundConnectionManager),
      mAddress(address),
      mEndpoint(endpoint)
{
}

RpcProxySender* RedirectorRpcProxyResolver::getProxySender(const InetAddress& addr)
{
    char8_t address[64];

    if (addr != mAddress)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "RedirectorRpcProxyResolver].getProxySender: trying to get redirector proxy sender with the wrong address! "
            << "addr=" << addr.toString(address, sizeof(address)) << "does not match rdirAddr=" << mAddress.toString(address, sizeof(address)));
        return nullptr;
    }

    OutboundRpcConnection* conn = mOwner.getConnection(mAddress);
    if (conn == nullptr)
    {
        conn = mOwner.createConnection(mAddress, &mEndpoint);
        if (conn != nullptr)
        {
            BLAZE_INFO_LOG(Log::CONNECTION, "[RedirectorRpcProxyResolver].getProxySender: established rdir connection; sess=" << conn << " ident=" 
                << ConnectionSbFormat(conn).getFormattedString() << " addr=" << mAddress.toString(address, sizeof(address)));
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[RedirectorRpcProxyResolver].getProxySender: failed to establish rdir connection; addr=" << mAddress.toString(address, sizeof(address)));
        }
    }
    return conn;
}

RpcProxySender* RedirectorRpcProxyResolver::getProxySender(InstanceId instanceId)
{
    BLAZE_ERR_LOG(Log::CONNECTION, "RedirectorRpcProxyResolver].getProxySender: trying to get redirector proxy sender with an instanceId, which is not supported");

    return nullptr;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

