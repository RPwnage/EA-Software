/*************************************************************************************************/
/*!
    \file   inboundgrpcmanager.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class InboundGrpcManager

*/
/*************************************************************************************************/
#ifndef BLAZE_INBOUND_GRPC_MANAGER_H
#define BLAZE_INBOUND_GRPC_MANAGER_H

#include "framework/blazedefines.h"
#include "framework/system/blazethread.h"

#include "EATDF/tdf.h"

#include "framework/tdf/frameworkconfigtypes_server.h"

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

#include "framework/grpc/inboundgrpcjobhandlers.h"
#include "framework/grpc/grpcendpoint.h"

namespace grpc
{
class Service;
class ServerCompletionQueue;
}

namespace Blaze
{
class ComponentManager;
class Selector;

namespace Grpc
{

typedef eastl::vector<eastl::unique_ptr<GrpcEndpoint> > GrpcEndpoints;

class InboundGrpcManager 
{
    NON_COPYABLE(InboundGrpcManager);
public:
    InboundGrpcManager(Blaze::Selector& selector);
    ~InboundGrpcManager();

    bool initialize(uint16_t basePort, const char8_t* endpointGroup, const EndpointConfig& endpointConfig, const RateConfig& rateConfig);
    bool startGrpcServers(); 
    bool registerServices(const Blaze::ComponentManager& compMgr);

    void reconfigure(const EndpointConfig& config, const RateConfig& rateConfig);

    void shutdown();
    bool isShuttingDown() { return mShuttingDown; }

    int32_t getNumConnections() const;
    void getStatusInfo(Blaze::GrpcManagerStatus& status) const;
    const GrpcEndpoints& getGrpcEndpoints() const { return mGrpcEndpoints; }


private:
    void shutdownEndpoint(GrpcEndpoint* endpoint);

    GrpcEndpoints mGrpcEndpoints;
    Blaze::Selector& mSelector;

    EA::TDF::TimeValue mShutdownTimeout;
    bool mShuttingDown;
    bool mInitialized;

    FiberJobQueue mEndpointShutdownQueue;
};

} //namespace Grpc
} // namespace Blaze



#endif // BLAZE_INBOUND_GRPC_MANAGER_H
