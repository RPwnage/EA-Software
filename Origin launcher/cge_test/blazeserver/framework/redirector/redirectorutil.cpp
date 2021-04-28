/*************************************************************************************************/
/*!
    \file   utilslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/nameresolver.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "redirector/tdf/redirectortypes_server.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/redirector/redirectorutil.h"

namespace Blaze
{
namespace Redirector
{

static void addAddress(ServerAddressType type, uint16_t port, const char8_t* hostname, ServerEndpointInfo& info)
{
    ServerAddressInfo* addrInfo = info.getAddresses().pull_back();
    addrInfo->setType(type);

    IpAddress ipAddr;
    const InetAddress* inetAddr = nullptr;
    if (type == INTERNAL_IPPORT)
    {
        inetAddr = &gNameResolver->getInternalAddress();
        if (gNameResolver->isInternalHostnameOverride())
            hostname = nullptr;
    }
    else if (type == EXTERNAL_IPPORT)
    {
        inetAddr = &gNameResolver->getExternalAddress();
        if (gNameResolver->isExternalHostnameOverride())
            hostname = nullptr;
    }
    else
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[RedirectorUtil].addAddress: ServerAddressType is neither internal or external.");
    }

    if (inetAddr != nullptr)
    {
        ipAddr.setIp(inetAddr->getIp(InetAddress::HOST));
        ipAddr.setPort(port);

        if (((hostname == nullptr) || (hostname[0] == '\0')) && !inetAddr->hostnameIsIp())
            hostname = inetAddr->getHostname();
        if ((hostname != nullptr) && (hostname[0] != '\0'))
            ipAddr.setHostname(hostname);
    }

    addrInfo->getAddress().setIpAddress(&ipAddr);
}

template<typename EndpointType>
static void setupServerEndpointInfo(ServerEndpointInfo* info, EndpointType* endpoint)
{
    info->setChannel(endpoint->getChannelType());
    info->setProtocol(endpoint->getProtocolName());
    info->setEncoder(endpoint->getEncoderName());
    info->setDecoder(endpoint->getDecoderName());
    info->setCurrentConnections((uint32_t)endpoint->getCurrentConnections());
    info->setBindType(endpoint->getBindType());

    info->setMaxConnections(RedirectorUtil::getMaxConnectionsForEndpoint(*endpoint));

    switch (endpoint->getBindType())
    {
    case Blaze::BIND_INTERNAL:
        addAddress(INTERNAL_IPPORT, endpoint->getPort(InetAddress::HOST), endpoint->getCommonName(), *info);
        break;
    case Blaze::BIND_EXTERNAL:
        addAddress(EXTERNAL_IPPORT, endpoint->getPort(InetAddress::HOST), endpoint->getCommonName(), *info);
        break;
    case Blaze::BIND_ALL:
        addAddress(INTERNAL_IPPORT, endpoint->getPort(InetAddress::HOST), endpoint->getCommonName(), *info);
        addAddress(EXTERNAL_IPPORT, endpoint->getPort(InetAddress::HOST), endpoint->getCommonName(), *info);
        break;
    }
    info->setConnectionTypeString(EndpointConfig::ConnectionTypeToString(endpoint->getConnectionType()));
}

static void parseTrialServiceNameRemaps(const Blaze::RedirectorSettingsConfig &config, TrialServiceNameRemapMap& remaps)
{
    TrialServiceNameRemapMap::const_iterator begin = config.getTrialServiceNameRemap().begin();
    TrialServiceNameRemapMap::const_iterator end = config.getTrialServiceNameRemap().end();
    for (; begin != end; ++begin)
    {
        remaps[begin->first] = begin->second;
    }
}

// static
void RedirectorUtil::populateServerInstanceStaticData(ServerInstanceStaticData& staticData)
{
    const ConnectionManager& connMgr = gController->getConnectionManager();
    auto grpcManager = gController->getInboundGrpcManager();
    StaticServerInstance& instance = staticData.getInstance();

    switch(gController->getInstanceType())
    {
        case ServerConfig::SLAVE:
            instance.setInstanceType(SLAVE);
            break;
        case ServerConfig::CONFIG_MASTER:
            instance.setInstanceType(CONFIG_MASTER);
            break;
        case ServerConfig::AUX_MASTER:
            instance.setInstanceType(AUX_MASTER);
            break;
        case ServerConfig::AUX_SLAVE:
            instance.setInstanceType(AUX_SLAVE);
            break;
        default:
            // todo: log error here
            return;
    }

    instance.setInstanceId(gController->getInstanceId());
    instance.setInstanceName(gController->getInstanceName());

    // Clear previous client types list and set the ClientTypes the redirector should direct to this server
    instance.getClientTypes().reserve(gController->getClientTypes().size());
    gController->getClientTypes().copyInto(instance.getClientTypes());

    // Clear the previous endpoint list since the instance object may be recycled
    instance.getEndpoints().clear();
    instance.getEndpoints().reserve(connMgr.getEndpoints().size() + (grpcManager ? grpcManager->getGrpcEndpoints().size() : 0));
    
    for(const auto& endpoint : connMgr.getEndpoints())
    {
        ServerEndpointInfo* info = instance.getEndpoints().pull_back();
        setupServerEndpointInfo(info, endpoint);
    }

    if (grpcManager != nullptr)
    {
        for (const auto& endpoint : grpcManager->getGrpcEndpoints())
        {
            ServerEndpointInfo* info = instance.getEndpoints().pull_back();
            setupServerEndpointInfo(info, endpoint.get());
        }
    }

    instance.setCurrentWorkingDirectory(gController->getHostCurrentWorkingDirectory());

    // clear and set the serviceNames that are associated with this server
    staticData.getServiceNames().reserve(gController->getHostedServicesInfo().size());

    gController->getHostedServicesInfo().copyInto(staticData.getServiceNames());

    //set the attributeMap of attributes to send to the redirector
    Redirector::ServerInstanceStaticData::AttributeMapMap& attrMap = staticData.getAttributeMap();
    const ServerVersion& version = gProcessController->getVersion();
    attrMap[Redirector::ATTRIBUTE_BUILD_LOCATION] = version.getBuildLocation();
    attrMap[Redirector::ATTRIBUTE_BUILD_TARGET] = version.getBuildTarget();
    attrMap[Redirector::ATTRIBUTE_BUILD_TIME] = version.getBuildTime();
    if(blaze_strcmp(gController->getFrameworkConfigTdf().getConfigVersion(), "") != 0)
    {
        attrMap[Redirector::ATTRIBUTE_CONFIG_VERSION] = gController->getFrameworkConfigTdf().getConfigVersion();
    }
    else
    {
        attrMap[Redirector::ATTRIBUTE_CONFIG_VERSION] = version.getVersion();
    }
    attrMap[Redirector::ATTRIBUTE_DEPOT_LOCATION] = version.getP4DepotLocation();
    attrMap[Redirector::ATTRIBUTE_VERSION] = version.getVersion();

    const ClientVersionInfo &versions = gController->getFrameworkConfigTdf().getRedirectorSettingsConfig().getClientVersions();
    VersionList::const_iterator it = versions.getCompatible().begin();
    VersionList::const_iterator endit = versions.getCompatible().end();
    for (; it != endit; ++it)
    {
        staticData.getCompatibleClientVersions().push_back((*it));
    }
    it = versions.getIncompatible().begin();
    endit = versions.getIncompatible().end();
    for (; it != endit; ++it)
    {
        staticData.getIncompatibleClientVersions().push_back((*it));
    }
    staticData.setDefaultServiceName(gController->getDefaultServiceName());
    
    // Add in info for primary sandbox id: (Xbox One only)
    if (gController->isPlatformHosted(xone))
    {
        attrMap[Redirector::ATTRIBUTE_SANDBOX_ID] = gController->getFrameworkConfigTdf().getSandboxId();
    }

    // Add in info for trial servicename remap
    parseTrialServiceNameRemaps(gController->getFrameworkConfigTdf().getRedirectorSettingsConfig(), staticData.getTrialServiceNameRemaps());
}

void RedirectorUtil::copyServerInstanceToStaticServerInstance(const ServerInstance& instance, StaticServerInstance& staticInstance)
{
    staticInstance.setInstanceId(instance.getInstanceId());
    staticInstance.setInstanceType(instance.getInstanceType());
    staticInstance.setInstanceName(instance.getInstanceName());
    instance.getEndpoints().copyInto(staticInstance.getEndpoints());
    staticInstance.setCurrentWorkingDirectory(instance.getCurrentWorkingDirectory());
    instance.getClientTypes().copyInto(staticInstance.getClientTypes());
}

void RedirectorUtil::copyStaticServerInstanceToServerInstance(const StaticServerInstance& staticInstance, ServerInstance& instance)
{
    instance.setInstanceId(staticInstance.getInstanceId());
    instance.setInstanceType(staticInstance.getInstanceType());
    instance.setInstanceName(staticInstance.getInstanceName());
    staticInstance.getEndpoints().copyInto(instance.getEndpoints());
    instance.setCurrentWorkingDirectory(staticInstance.getCurrentWorkingDirectory());
    staticInstance.getClientTypes().copyInto(instance.getClientTypes());
}

void RedirectorUtil::populateServerInstanceDynamicData(ServerInstanceDynamicData& dynamicData)
{
    // Clear the previous endpoint list since the instance object may be recycled
    const ConnectionManager& connMgr = gController->getConnectionManager();
    auto grpcManager = gController->getInboundGrpcManager();

    dynamicData.getEndpoints().clear();
    dynamicData.getEndpoints().reserve(connMgr.getEndpoints().size() + (grpcManager ? grpcManager->getGrpcEndpoints().size() : 0));
    
    for(const auto& endpoint : connMgr.getEndpoints())
    {
        DynamicServerEndpointInfo* info = dynamicData.getEndpoints().pull_back();
        info->setCurrentConnections((uint32_t)endpoint->getCurrentConnections());
        info->setMaxConnections(getMaxConnectionsForEndpoint(*endpoint));
    }
   
    if (grpcManager != nullptr)
    {
        for (const auto& endpoint : grpcManager->getGrpcEndpoints())
        {
            DynamicServerEndpointInfo* info = dynamicData.getEndpoints().pull_back();
            info->setCurrentConnections((uint32_t)endpoint->getCurrentConnections());
            info->setMaxConnections(getMaxConnectionsForEndpoint(*endpoint));
        }
    }
}

uint32_t RedirectorUtil::getMaxConnectionsForEndpoint(const Endpoint& endpoint)
{
    return (!gController->isDraining()) ? endpoint.getMaxConnections() : 0;
}

uint32_t RedirectorUtil::getMaxConnectionsForEndpoint(const Grpc::GrpcEndpoint& endpoint)
{
    return (!gController->isDraining()) ? endpoint.getMaxConnections() : 0;
}

} // namespace Redirector
} // namespace Blaze

