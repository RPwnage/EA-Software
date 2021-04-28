/*************************************************************************************************/
/*!
    \file outboundhttpservice.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDHTTPSERVICE_H
#define BLAZE_OUTBOUNDHTTPSERVICE_H

#include "framework/component/component.h"
#include "framework/component/rpcproxyresolver.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/connection/outboundhttpconnectionmanager.h"


namespace Blaze
{

    class OutboundConnMgrStatus;
    class HttpRpcProxyResolver;

    class OutboundHttpService : public OutboundHttpConnectionListener
    {

    public:
        typedef eastl::map<eastl::string, HttpConnectionManagerPtr> OutboundHttpServiceMap;

        OutboundHttpService();
        ~OutboundHttpService() override;

        bool configure(const HttpServiceConfig& config);

        void prepareForReconfigure(const HttpServiceConfig& config, ConfigureValidationErrors& validationErrors) const;
        void reconfigure(const HttpServiceConfig& config);

        void refreshCerts();

        const Component* getService(const char8_t* componentName) const;

        HttpConnectionManagerPtr getConnection(const char8_t* serviceName);

        void getStatusInfo(HttpServiceStatusMap& infoMap) const;

        // from OutboundHttpConnectionListener
        void onNoActiveConnections(OutboundHttpConnectionManager& connMgr) override;

        // for OutboundMetricsManager to auto track metrics for all services registered in outboundHttpService.
        const OutboundHttpServiceMap& getOutboundHttpServiceByNameMap() const { return mOutboundHttpServiceByNameMap; }

        bool validateHttpServiceConnConfig(const char8_t* serviceName, const HttpServiceConnection& connConfig) const;
        bool validateHttpServiceConnConfig(const char8_t* serviceName, const HttpServiceConnection& connConfig, eastl::string& errorString) const;
        bool validateComponentConfigs(const HttpServiceConfig& config) const;
        void registerService(const char8_t* serviceName, const HttpServiceConnection& connConfig, const ComponentId owningComponentId);

        // returns false, if either the service name or host/port for are already configured under another component id
        bool validateHttpServiceOwningComponentId(const char8_t* serviceName, const HttpServiceConnection& connConfig, ComponentId componentId) const;
        
        void removeHttpServices(const HttpServiceConnectionMap& config, const ComponentId owningComponentId);

        static bool buildHttpStatusCodeSet(const HttpStatusCodeRangeList& codeRangeList, HttpStatusCodeSet& codeSet, eastl::string& errorString);

    protected:
        friend class VaultManager;

        void removeHttpService(const char8_t* serviceName);

        // Registers the service if necessary (removes existing service if its configuration differs from connConfig).
        // Assumes connConfig has already been validated.
        void updateHttpService(const char8_t* serviceName, const HttpServiceConnection& connConfig, const ComponentId owningComponentId);

    private:
        bool validateConfigEqualsExistingManager(const HttpServiceConnection& connConfig, HttpConnectionManagerPtr& connMgr) const;
        bool validateHttpServiceConnConfigInternal(const char8_t* serviceName, const HttpServiceConnection& connConfig, eastl::string& errorString, bool validateOwnership) const;
        void getHostnameAndPortFromConfig(const char8_t* serverAddress, eastl::string& hostnameAndPort, bool& isSecure) const;
        bool decodeSslKeyPassword(const HttpServiceConnection& connConfig, eastl::string& buf) const;

        typedef eastl::map<eastl::string, Component*> ServiceComponentMap;
        ServiceComponentMap mServiceComponentMap;

        typedef eastl::map<eastl::string, HttpRpcProxyResolver*> ProxyResolverMap;
        ProxyResolverMap mProxyServiceResolverMap;

        OutboundHttpServiceMap mOutboundHttpServiceByNameMap; //map by service name
        OutboundHttpServiceMap mReconfigureMap;
    };

extern EA_THREAD_LOCAL OutboundHttpService* gOutboundHttpService;
}

#endif
