/*************************************************************************************************/
/*!
    \file

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONNECTIONMANAGER_H
#define BLAZE_CONNECTIONMANAGER_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_mutex.h"
#include "framework/util/pmap.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/connection.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/util/intervalmap.h"
#include "framework/util/pasutil.h"
#include "framework/util/xmlbuffer.h"
#include "framework/system/job.h"
#include "framework/system/fiber.h"
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "framework/tdf/controllertypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

class SocketChannel;
class ConnectionManagerStatus;
class Selector;
class FrameworkConfig;
class SlaveSession;

class ConnectionManager :
    public ConnectionOwner,
    private ServerSocketChannel::Handler
{
    NON_COPYABLE(ConnectionManager);

public:
    typedef eastl::vector<Endpoint*> EndpointList;

    ConnectionManager(Selector &selector);
    ~ConnectionManager() override;

    SlaveSessionPtr getSlaveSessionByInstanceId(InstanceId instanceId) const;
    size_t getNumConnections() const { return mConnections.size(); }

    bool initialize(uint16_t basePort, const RateConfig& rateConfig, const EndpointConfig& endpointConfig, const char8_t* endpointGroup);

    void reconfigure(const EndpointConfig& config, const RateConfig& rateConfig);
    bool isShutdown() { return mShutdown; }

    const EndpointList& getEndpoints() const { return mEndpoints; }
    const Endpoint* findEndpoint(const char8_t* name) const;
    size_t getEndpointCount() const { return mEndpoints.size(); }

    void resumeEndpoints();
    void pauseEndpoints();
    
    void shutdown();

    void getStatusInfo(ConnectionManagerStatus& status) const;
    void getMetrics(const GetConnMetricsRequest& request, GetConnMetricsResponse &response);

    void addSlaveSession(SlaveSession& session);
    void removeSlaveSession(SlaveSession& session);

    void forceShutdownAllConnections();
    EA::TDF::TimeValue getMaxEndpointCommandTimeout() const;

private:
    void destroySlaveSession(SlaveSessionPtr sessionPtr);

    // Sentinel value to indicate we are not draining (our default state) 
    static const uint32_t DRAIN_CONNECTIONS_NOT_SET = UINT32_MAX;

    // How frequently we check for inactive connections
    static const int32_t CONNECTION_INACTIVITY_PERIOD_MS = 1000;
   
    typedef eastl::hash_map<ConnectionId, InboundRpcConnection*> ConnectionsByIdent;
    typedef eastl::hash_map<InstanceId, SlaveSessionPtr> SlaveSessionsById;

    Selector &mSelector;
    char8_t mEndpointGroup[128];
    EndpointList mEndpoints;
    bool mShutdown;
    IntervalMap mIdentFactory;
    ConnectionsByIdent mConnections;
    ConnectionsByIdent mServerConnections;

    SlaveSessionsById mSlaveSessions;
    TimerId mInactivityTimerId;
    PasUtil mPasUtil;
    
    uint32_t mNextConnectionBookmark;

    EA::TDF::TimeValue mShutdownTimeout;
    EA::TDF::TimeValue mShutdownStartTime;
    Fiber::EventHandle mAllConnsRemovedEvent;
    
    InboundRpcConnection* getConnection(ConnectionId ident) const;

    void removeConnection(Connection& connection) override;
    bool isActive() const override { return !mShutdown; }
    
    void expireInactiveConnections();

    // ServerSocketChannel:: interface
    void onNewChannel(SocketChannel& channel, Endpoint& endpoint, RatesPerIp *rateLimitCounter) override;
};

} // Blaze
#endif // BLAZE_CONNECTIONMANAGER_H

