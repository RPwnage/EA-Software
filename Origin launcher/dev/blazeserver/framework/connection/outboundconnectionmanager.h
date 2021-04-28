/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDCONNECTIONMANAGER_H
#define BLAZE_OUTBOUNDCONNECTIONMANAGER_H

/*** Include files *******************************************************************************/

#include "framework/component/rpcproxyresolver.h"
#include "framework/connection/outboundrpcconnection.h"
#include "framework/connection/endpoint.h"
#include "framework/protocol/fireprotocol.h"
#include "framework/protocol/fire2protocol.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/intervalmap.h"
#include "framework/util/pmap.h"
#include "redirector/tdf/redirectortypes_server.h"
#include "redirector/rpc/redirectorslave.h"
#include "framework/tdf/controllertypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Connection;
class ConnectionManagerStatus;
class OutboundRpcConnection;
class RedirectorRpcProxyResolver;

class OutboundConnectionManager : public RpcProxyResolver, public ConnectionOwner
{
public:
    OutboundConnectionManager();
    ~OutboundConnectionManager() override;

    void setRedirectorAddr(const InetAddress& redirectorAddr);

    // Get a connection connected to the given endpoint.
    OutboundRpcConnection* getConnection(const InetAddress& address) const;
    OutboundRpcConnection* createConnection(const InetAddress& address, Endpoint* endpoint = nullptr);
    BlazeRpcError DEFINE_ASYNC_RET(getInetAddressByServiceName(const Redirector::ServerAddressMapRequest& request, InetAddress& address, int32_t instanceIndex = -1));
    OutboundRpcConnection* getConnectionByServiceName(const Redirector::ServerAddressMapRequest& request, int32_t instanceIndex = -1);
    OutboundRpcConnection* createConnectionByServiceName(const char8_t* serviceName, Redirector::ServerAddressType addrType,
        int32_t instanceIndex = -1, Endpoint* endpoint = nullptr);

    void bindConnectionToInstanceId(OutboundRpcConnection& conn, InstanceId instanceId);
    OutboundRpcConnection* getConnectionByInstanceId(InstanceId id) const; 

    // RpcProxyResolver interface
    RpcProxySender* getProxySender(const InetAddress& addr) override;
    RpcProxySender* getProxySender(InstanceId instanceId) override { return getConnectionByInstanceId(instanceId); }

    void removeConnection(Connection& connection) override;
    bool isActive() const override { return !mShutdown; }

    void getOutConnMetricsInfo(const GetConnMetricsRequest &request, GetConnMetricsResponse &response);
    void shutdown();
    void disconnectRedirectorSlaveProxy();
    Redirector::RedirectorSlave* getRedirectorSlaveProxy();
    static bool isFatalRedirectorError(BlazeRpcError rc);

 
private:
    typedef pmap<const InetAddress*, OutboundRpcConnection*>::Type ConnectionsByAddress;
    typedef eastl::hash_map<InstanceId, OutboundRpcConnection*> ConnectionsById;

    bool mShutdown;
    Endpoint mEndpoint;
    IntervalMap mIdentFactory;
    ConnectionsByAddress mConnectionsByAddress;
    ConnectionsById mConnectionsByInstanceId;
    Redirector::RedirectorSlave* mProxy;
    InetAddress mRedirAddress;
    Endpoint mRedirEndpoint;
    RedirectorRpcProxyResolver* mRedirectorRpcProxyResolver;

    Fiber::EventHandle mAllConnsRemovedEvent;

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.
    // Copy constructor 
    OutboundConnectionManager(const OutboundConnectionManager& otherObj);

    // Assignment operator
    OutboundConnectionManager& operator=(const OutboundConnectionManager& otherObj);
};

class RedirectorRpcProxyResolver : public RpcProxyResolver
{
public:
    RedirectorRpcProxyResolver(OutboundConnectionManager& outboundConnectionManager, const InetAddress& address, Endpoint& endpoint);
    ~RedirectorRpcProxyResolver() override {}

    // RpcProxyResolver interface
    RpcProxySender* getProxySender(const InetAddress& addr) override;
    RpcProxySender* getProxySender(InstanceId instanceId) override;

private:
    OutboundConnectionManager& mOwner;
    const InetAddress& mAddress;
    Endpoint& mEndpoint;
};

} // Blaze

#endif // BLAZE_OUTBOUNDCONNECTIONMANAGER_H

