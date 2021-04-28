/*************************************************************************************************/
/*!
    \file outboundhttpservice.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/blazerpc.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/database/dbobfuscator.h"
#include "framework/controller/controller.h"
#include "framework/vault/vaultmanager.h"
#include <regex>

namespace Blaze
{

class HttpRpcProxyResolver : public RpcProxyResolver
{
public:
    HttpRpcProxyResolver(const char8_t* serviceName)
        : mServiceName(serviceName)
    {
    }

    virtual RpcProxySender* getProxySender(const InetAddress& addr)
    {
        return gOutboundHttpService->getConnection(mServiceName.c_str()).get();
    }

    virtual RpcProxySender* getProxySender(InstanceId instanceId) { return nullptr; } // Force send by address

private:
    eastl::string mServiceName;
};

OutboundHttpService::OutboundHttpService()
{

}


OutboundHttpService::~OutboundHttpService()
{
    mReconfigureMap.clear();
    mOutboundHttpServiceByNameMap.clear();

    ServiceComponentMap::iterator sIter = mServiceComponentMap.begin();
    ServiceComponentMap::iterator sEnd = mServiceComponentMap.end();
    for (; sIter != sEnd; ++sIter)
    {
        delete sIter->second;
    }

    for(auto& resolver : mProxyServiceResolverMap)
        delete resolver.second;
}

bool OutboundHttpService::validateHttpServiceConnConfig(const char8_t* serviceName, const HttpServiceConnection& connConfig, eastl::string& errorString) const
{
    return validateHttpServiceConnConfigInternal(serviceName, connConfig, errorString, false);
}

bool OutboundHttpService::validateHttpServiceConnConfig(const char8_t* serviceName, const HttpServiceConnection& connConfig) const
{
    eastl::string errMsg;
    if (!validateHttpServiceConnConfigInternal(serviceName, connConfig, errMsg, false))
    {
        BLAZE_FATAL_LOG(Log::HTTP, "[OutboundHttpService].validateHttpServiceConnConfig: Unable to register URL " << connConfig.getUrl() <<
            " for service " << serviceName << "; " << errMsg.c_str());
        return false;
    }
    return true;
}

bool OutboundHttpService::validateHttpServiceConnConfigInternal(const char8_t* serviceName, const HttpServiceConnection& connConfig, eastl::string& errorString, bool validateOwnership) const
{
    if (connConfig.getHasProxyComponent())
    {
        if (ComponentData::getComponentDataByName(serviceName) != nullptr)
        {
            if (connConfig.getEncodingType() == nullptr || connConfig.getEncodingType()[0] == '\0')
            {
                errorString.assign("no encoding type specified");
                return false;
            }

            if (validateOwnership && !validateHttpServiceOwningComponentId(serviceName, connConfig, Component::INVALID_COMPONENT_ID))
            {
                errorString.assign("service is already configured under another component");
                return false;
            }

            if (connConfig.getNumPooledConnections() == 0)
            {
                errorString.sprintf("need at least 1 connection for component (%s).", serviceName);
                return false;
            }
        }
        else
        {
            BLAZE_INFO_LOG(Log::HTTP, "[OutboundHttpService].validateHttpServiceConnConfigInternal: service requires proxy component, but no '" << serviceName <<
                "' proxy component is loaded at runtime. Make sure there is an appropriate .rpc component definition in the proxycomponent directory, and that the proxy component slave is referenced somewhere in code (this prevents the linker from optimizing out the component data).");
        }
    }

    eastl::string buf;
    if (!decodeSslKeyPassword(connConfig, buf))
    {
        errorString.assign("failed to decode ssl key password");
        return false;
    }

    HttpStatusCodeSet codeSet;
    if (!buildHttpStatusCodeSet(connConfig.getLogErrForHttpCodes(), codeSet, errorString))
    {
        errorString.append("; failed to validate logErrForHttpCodes config");
        return false;
    }

    return true;
}

bool OutboundHttpService::buildHttpStatusCodeSet(const HttpStatusCodeRangeList& codeRangeList, HttpStatusCodeSet& codeSet, eastl::string& errorString)
{
    for (const auto& range : codeRangeList)
    {
        if (range.empty())
        {
            errorString.assign("logErrForHttpCodes config cannot be empty string");
            return false;
        }

        //logErrForHttpCodes config can be single http status code like "400" or code range like "400-405"
        if (!std::regex_match(range.c_str(), std::regex("[1-5][0-9]{2}|[1-5][0-9]{2}-[1-5][0-9]{2}")))
        {
            errorString.sprintf("logErrForHttpCodes config (%s) must be valid http code from 100 to 599 (e.g. 400), or valid code range specified with one dash - (e.g. 400-405)", range.c_str());
            return false;
        }

        HttpStatusCode begin = 0;
        HttpStatusCode end = 0;
        if (eastl::string(range).find("-") == eastl::string::npos)
        {
            sscanf(range.c_str(), "%d", &begin);
            end = begin;
        }
        else
        {
            sscanf(range.c_str(), "%d-%d", &begin, &end);
        }

        if (begin > end)
        {
            errorString.sprintf("logErrForHttpCodes config (%s) cannot have starting value larger than ending value", range.c_str());
            return false;
        }

        for (HttpStatusCode code = begin; code <= end; ++code)
        {
            if (codeSet.find(code) != codeSet.end())
            {
                errorString.sprintf("logErrForHttpCodes config (%s) cannot overlap with other codes/ranges", range.c_str());
                return false;
            }
            codeSet.insert(code);
        }
    }

    return true;
}

bool OutboundHttpService::validateComponentConfigs(const HttpServiceConfig& config) const
{
    HttpServiceConnectionMap::const_iterator iter = config.getHttpServices().begin();
    HttpServiceConnectionMap::const_iterator end = config.getHttpServices().end();

    for (; iter != end; ++iter)
    {
        const char8_t* componentName = iter->first;
        const HttpServiceConnection& connConfig = *(iter->second);

        if(!validateHttpServiceConnConfig(componentName, connConfig))
            return false;
    }

    return true;
}

bool OutboundHttpService::validateHttpServiceOwningComponentId(const char8_t* serviceName, const HttpServiceConnection& connConfig, ComponentId componentId) const
{
    OutboundHttpServiceMap::const_iterator itr = mOutboundHttpServiceByNameMap.find(serviceName);
    if (itr != mOutboundHttpServiceByNameMap.end())
    {
        if (itr->second->getOwningComponentId() != componentId)
        {
            return false;
        }
    }
    
    return true;
}

/*! ************************************************************************************************/
/*! \brief configure the http service

    \param[in] config = the config
    \return true if server can startup, false if error is fatal.
***************************************************************************************************/
bool OutboundHttpService::configure(const HttpServiceConfig& config)
{
    if (!validateComponentConfigs(config))
    {
        return false;
    }

    HttpServiceConnectionMap::const_iterator iter = config.getHttpServices().begin();
    HttpServiceConnectionMap::const_iterator end = config.getHttpServices().end();

    for (; iter != end; ++iter)
    {
        const HttpServiceConnection& connConfig = *(iter->second);
        registerService(iter->first, connConfig, Component::INVALID_COMPONENT_ID);
    }


    return true;
}

void OutboundHttpService::getHostnameAndPortFromConfig(const char8_t* serverAddress, eastl::string& hostnameAndPort, bool& isSecure) const
{
    const char8_t* serviceHostname = nullptr;
    HttpProtocolUtil::getHostnameFromConfig(serverAddress, serviceHostname, isSecure);

    //make sure serviceHostname does contains the port
    //fill in the port if needed
    hostnameAndPort = serviceHostname;
    char8_t* portStr = blaze_strstr(serviceHostname, ":");

    const uint16_t PORT_DEFAULT_INSECURE = 80;
    const uint16_t PORT_DEFAULT_SECURE = 443;
    if (portStr == nullptr)
    {
        if (isSecure)
        {
            hostnameAndPort.append_sprintf(":%u", PORT_DEFAULT_SECURE);
        }
        else
        {
            hostnameAndPort.append_sprintf(":%u", PORT_DEFAULT_INSECURE);
        }
    }
}

void OutboundHttpService::registerService(const char8_t* serviceName, const HttpServiceConnection& connConfig, const ComponentId owningComponentId)
{
    // Create the host name and address.  (trims https or http, and tells us which one was used)
    eastl::string hostandport;
    bool serviceSecure = false;
    getHostnameAndPortFromConfig(connConfig.getUrl(), hostandport, serviceSecure);
    const char8_t* serviceHostnamePort = hostandport.c_str();

    InetAddress addy(serviceHostnamePort);

    bool localCompFound = true;
    for (auto& compName : connConfig.getRegisterOnlyForLocalComponents())
    {
        EA::TDF::ComponentId slaveCompId, masterCompId;
        BlazeRpcComponentDb::getComponentIdsByBaseName(compName.c_str(), slaveCompId, masterCompId);
        if ((gController->getComponentManager().getLocalComponent(slaveCompId) != nullptr) ||
            (gController->getComponentManager().getLocalComponent(masterCompId) != nullptr))
        {
            localCompFound = true; // Component is local
            break;
        }
        localCompFound = false;
    }

    if (!localCompFound)
    {
        BLAZE_INFO_LOG(Log::HTTP, "[OutboundHttpService].registerService: Service " << serviceName << " was not registered as no required local components were found.");
        return;
    }

    if (connConfig.getHasProxyComponent())
    {
        // validate the component.
        const ComponentData* info = ComponentData::getComponentDataByName(serviceName);
        if (info == nullptr)
        {
            BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpService].registerService: unknown component in configuration (" << serviceName << ").");
            return;
        }

        // Bail if encoder type is not defined.
        if (connConfig.getEncodingType() == nullptr || connConfig.getEncodingType()[0] == '\0')
        {
            BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpService].registerService: Unable to register URL " << connConfig.getUrl() << 
                " against component " << serviceName << ", No encoding type specified.");
            return;
        }

        // Check for duplicate components
        ServiceComponentMap::const_iterator iter = mServiceComponentMap.find(serviceName);
        if (iter != mServiceComponentMap.end())
        {
            BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpService].registerService: Unable to register URL " << connConfig.getUrl() << 
                " against component " << serviceName << ", duplicate component.");
            return;
        }

        HttpRpcProxyResolver* resolver = BLAZE_NEW HttpRpcProxyResolver(serviceName);
        mProxyServiceResolverMap.insert(eastl::make_pair(serviceName, resolver));
        Component* component = info->createRemoteComponent(*resolver, addy);
        mServiceComponentMap.insert(eastl::make_pair(serviceName, component));
    }

    OutboundHttpConnectionManager* connMgr = BLAZE_NEW OutboundHttpConnectionManager(serviceName);
    
    bool secureVerifyPeer = false;
    connMgr->setSslPathType(connConfig.getPathType());
    if (connConfig.getVaultPath().getPath() != nullptr && connConfig.getVaultPath().getPath()[0] != '\0')
    {
        connMgr->setVaultPath(connConfig.getVaultPath().getPath());
    }
    if (connConfig.getSslCert() != nullptr && connConfig.getSslCert()[0] != '\0')
    {
        connMgr->setPrivateCert(connConfig.getSslCert());
    }
    if (connConfig.getSslKey() != nullptr && connConfig.getSslKey()[0] != '\0')
    {
        connMgr->setPrivateKey(connConfig.getSslKey());
    }
    if (connConfig.getSslKeyPasswd() != nullptr && connConfig.getSslKeyPasswd()[0] != '\0')
    {
        // replace encoded ssl key password with its decoded value. Pre: config validation checked decode-ability.
        eastl::string buf;
        decodeSslKeyPassword(connConfig, buf);
        connMgr->setKeyPassphrase(buf.c_str());
    }

    connMgr->initialize(addy, connConfig.getNumPooledConnections(), serviceSecure, secureVerifyPeer, 
        OutboundHttpConnectionManager::SSLVERSION_DEFAULT, connConfig.getEncodingType(), 
        nullptr, //rateLimitCb 
        TimeValue(), connConfig.getRequestTimeout(), TimeValue(), nullptr, owningComponentId,
        connConfig.getLogErrForHttpCodes());

    connMgr->setOutboundMetricsThreshold(connConfig.getOutboundMetricsThreshold());
    connMgr->setMaxHttpCacheSize(connConfig.getMaxHttpCacheMemorySizeinBytes());
    connMgr->setMaxHttpResponseSize(connConfig.getMaxHttpResponseSizeInBytes());

    HttpConnectionManagerPtr sharedConnMgr = HttpConnectionManagerPtr(connMgr);
    mOutboundHttpServiceByNameMap.insert(eastl::make_pair(serviceName, sharedConnMgr));

    BLAZE_INFO_LOG(Log::HTTP, "[OutboundHttpService].registerService: HTTP connection pool to " << serviceHostnamePort << " using service " << 
        serviceName << " has been established with a pool of " << connConfig.getNumPooledConnections() << ". Max http cache memory limit is set to " << connConfig.getMaxHttpCacheMemorySizeinBytes() << " bytes.");
}


void OutboundHttpService::prepareForReconfigure(const HttpServiceConfig& config, ConfigureValidationErrors& validationErrors) const
{
    HttpServiceConnectionMap::const_iterator iter = config.getHttpServices().begin();
    HttpServiceConnectionMap::const_iterator end = config.getHttpServices().end();

    for (; iter != end; ++iter)
    {
        const HttpServiceConnection& connConfig = *(iter->second);
        const char8_t* serviceName = iter->first;

        eastl::string errMsg;
        if (!validateHttpServiceConnConfigInternal(serviceName, connConfig, errMsg, true))
        {
            eastl::string msg;
            msg.sprintf("[OutboundHttpService].prepareForReconfigure:  unable to configure service %s; %s.", serviceName, errMsg.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }
    }
}

bool OutboundHttpService::validateConfigEqualsExistingManager(const HttpServiceConnection& connConfig, HttpConnectionManagerPtr& connMgr) const
{
    eastl::string hostandport;
    bool serviceSecure = false;
    getHostnameAndPortFromConfig(connConfig.getUrl(), hostandport, serviceSecure);
    InetAddress newAddress(hostandport.c_str());

    eastl::string buf;
    decodeSslKeyPassword(connConfig, buf);

    // See if our addresses match, if they both are secure/insecure, and if they both have the same 
    // content type and pool size. Also check that if they're both secure, then they have the same ssl key
    // and ssl cert, and connMgr's SslContext is up-to-date.
    if ((connMgr->getAddress() == newAddress) &&
        (connMgr->isSecure() == serviceSecure) &&
        (!connConfig.getHasProxyComponent() || blaze_strcmp(connMgr->getDefaultContentType(), connConfig.getEncodingType()) == 0) &&
        (connMgr->maxConnectionsCount() == connConfig.getNumPooledConnections()) &&
        (!serviceSecure ||
        ((blaze_strcmp(connMgr->getPrivateKey(), connConfig.getSslKey()) == 0) &&
         (blaze_strcmp(connMgr->getPrivateCert(), connConfig.getSslCert()) == 0) &&
         (blaze_strcmp(connMgr->getKeyPassphrase(), buf.c_str()) == 0) &&
         (connMgr->hasLatestSslContext())))
       )
    {
        HttpStatusCodeSet codeSet;
        eastl::string errorString;
        if (!buildHttpStatusCodeSet(connConfig.getLogErrForHttpCodes(), codeSet, errorString))
            return false;

        if (codeSet.size() != connMgr->getHttpStatusCodeErrSet().size())
            return false;

        // Check if the list of HTTP status codes has changed
        for (const auto& code : codeSet)
            if (connMgr->getHttpStatusCodeErrSet().find(code) == connMgr->getHttpStatusCodeErrSet().end())
                return false;

        return true;
    }
    return false;
}


void OutboundHttpService::reconfigure(const HttpServiceConfig& config)
{
    // Loop through our existing connections to clean out what no longer exists
    // or has been modified in the new config, and leave what has not changed.
    removeHttpServices(config.getHttpServices(), Component::INVALID_COMPONENT_ID);
    // Now loop through our config to add the new/changed items
    HttpServiceConnectionMap::const_iterator cIter = config.getHttpServices().begin();
    HttpServiceConnectionMap::const_iterator cEnd = config.getHttpServices().end();

    for (; cIter != cEnd; ++cIter)
    {
        const char8_t* serviceName = cIter->first;
        const HttpServiceConnection& connConfig = *(cIter->second);
        
        // Check existing connections so we don't generate warnings for things that haven't changed.
        OutboundHttpServiceMap::iterator fIter = mOutboundHttpServiceByNameMap.find(serviceName);
        if (fIter == mOutboundHttpServiceByNameMap.end())
        {
            registerService(serviceName, connConfig, Component::INVALID_COMPONENT_ID);
        }
    }

}

void OutboundHttpService::refreshCerts()
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Starting refresh of Http service certificates from Vault");

    // Take a copy of the map in case it mutates while we are making blocking calls to Vault
    OutboundHttpServiceMap serviceMap(mOutboundHttpServiceByNameMap);

    OutboundHttpServiceMap::const_iterator iter = serviceMap.begin();
    OutboundHttpServiceMap::const_iterator end = serviceMap.end();
    for ( ; iter != end; ++iter )
    {
        HttpConnectionManagerPtr connMgr = iter->second;
        if (connMgr->getSslPathType() == HttpServiceConnection::VAULT)
        {
            SecretVault::SecretVaultDataMap dataMap;
            if (gVaultManager->readSecret(connMgr->getVaultPath(), dataMap) != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Failed to refresh cert for service " << iter->first
                    << " due to error fetching data from Vault path " << connMgr->getVaultPath());
                continue;
            }

            eastl::string cert;
            SecretVault::SecretVaultDataMap::const_iterator dataIter = dataMap.find("sslCert");
            if (dataIter != dataMap.end())
            {
                cert = dataIter->second;
            }
            else
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Failed to locate certificate for service " << iter->first
                    << " in data at Vault path " << connMgr->getVaultPath());
                continue;
            }

            eastl::string key;
            dataIter = dataMap.find("sslKey");
            if (dataIter != dataMap.end())
            {
                key = dataIter->second;
            }
            else
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Failed to locate private key for service " << iter->first
                    << " in data at Vault path " << connMgr->getVaultPath());
                continue;
            }

            if (blaze_strcmp(connMgr->getPrivateCert(), cert.c_str()) == 0 && blaze_strcmp(connMgr->getPrivateKey(), key.c_str()) == 0)
            {
                BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Cert and key for service " << iter->first << " have not changed");
                continue;
            }

            BLAZE_INFO_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Updating cert and key for service " << iter->first);

            connMgr->setPrivateCert(cert.c_str());
            connMgr->setPrivateKey(key.c_str());
        }
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, "[OutboundHttpService].refreshCerts: Done refresh of Http service certificates");
}

void OutboundHttpService::updateHttpService(const char8_t* serviceName, const HttpServiceConnection& connConfig, const ComponentId owningComponentId)
{
    OutboundHttpServiceMap::iterator iter = mOutboundHttpServiceByNameMap.find(serviceName);
    if (iter != mOutboundHttpServiceByNameMap.end())
    {
        if (validateConfigEqualsExistingManager(connConfig, iter->second))
            return;
        else
            removeHttpService(serviceName);
    }
    registerService(serviceName, connConfig, owningComponentId);
}

void OutboundHttpService::removeHttpService(const char8_t* serviceName)
{
    OutboundHttpServiceMap::iterator iter = mOutboundHttpServiceByNameMap.find(serviceName);
    if (iter != mOutboundHttpServiceByNameMap.end())
    {
        HttpConnectionManagerPtr& connMgr = iter->second;

        eastl::string hostandport = connMgr->getAddress().getHostname();
        hostandport.append_sprintf(":%u", connMgr->getAddress().getPort(InetAddress::HOST));

        // If we get here, we have either modified or removed this existing connection.
        // See if we can safely kill it right now.
        if (connMgr->connectionsInUseCount() != 0)
        {
            // We have active connections on this service

            // Add in order to listen for OutboundHttpConnectionListener::onNoActiveConnections()
            // removing first prevents a possible double add assert (does not trigger an assert when removing a listener that's not already added)
            connMgr->removeConnectionListener(*this);
            connMgr->addConnectionListener(*this);

            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpService] HTTP service '" << serviceName << "' at " << hostandport <<
                " has active connections remaining, waiting for drain.");
            // store off connMgr to the side to be drained, now connMgr has at least one ref with mReconfigureMap above, we can safely remove it from our usage maps
            mReconfigureMap.insert(eastl::make_pair(hostandport, connMgr));
        }
        else
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpService] Removing HTTP service '" << serviceName << "' at " << hostandport << ".");
        }

        mOutboundHttpServiceByNameMap.erase(iter);
        ServiceComponentMap::iterator sIter = mServiceComponentMap.find(serviceName);
        if (sIter != mServiceComponentMap.end())
        {
            delete sIter->second;
            mServiceComponentMap.erase(sIter);
        }
        ProxyResolverMap::iterator pIter = mProxyServiceResolverMap.find(serviceName);
        if (pIter != mProxyServiceResolverMap.end())
        {
            delete pIter->second;
            mProxyServiceResolverMap.erase(pIter);
        }
    }
}

void OutboundHttpService::removeHttpServices(const HttpServiceConnectionMap& config, const ComponentId owningComponentId)
{
    OutboundHttpServiceMap::iterator iter = mOutboundHttpServiceByNameMap.begin();

    while (iter != mOutboundHttpServiceByNameMap.end())
    {
        // Copy serviceName; we may want to use it again (e.g. for logging)
        // after removing the associated service
        eastl::string serviceName = iter->first;
        HttpConnectionManagerPtr& connMgr = iter->second;

        if (connMgr->getOwningComponentId() != owningComponentId)
        {
            // item registered by another component, leave it alone.
            ++iter;
            continue;
        }

        // See if one by the same name exists in the configuration.
        HttpServiceConnectionMap::const_iterator cIter = config.find(serviceName.c_str());
        if (cIter != config.end())
        {
            const HttpServiceConnection& connConfig = *(cIter->second);

            if (validateConfigEqualsExistingManager(connConfig, connMgr))
            {
                // items are the same, we can leave it alone.
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpService] HTTP service for " << serviceName << " already exists and has not changed.");
                ++iter;
                continue;
            }
        }

        ++iter;

        removeHttpService(serviceName.c_str());
    }
}

bool OutboundHttpService::decodeSslKeyPassword(const HttpServiceConnection& connConfig, eastl::string& buf) const
{
    // check if password was obfuscated
    if (*connConfig.getSslKeyPasswdObfuscator() != '\0')
    {
        const size_t DECODED_BUFFER_SIZE = 256;
        char8_t buffer[DECODED_BUFFER_SIZE];

        size_t len = DbObfuscator::decode(connConfig.getSslKeyPasswdObfuscator(), connConfig.getSslKeyPasswd(), buffer, DECODED_BUFFER_SIZE);
        if (len < DECODED_BUFFER_SIZE)
        {
            buf.sprintf("%s", buffer);
            return true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpService].decodeSslKeyPassword Could not decode ssl key password: insufficient buffer size.");
            return false;
        }
    }
    buf.sprintf("%s", connConfig.getSslKeyPasswd());
    return true;
}

/*! ************************************************************************************************/
/*! \brief A conn mgr has reached 0 active connections.  Check to see if we are expecting it to drain
        so we can remove it from our reconfigure map.

    \param[in] connMgr - the Outbound http conn mgr that we listened to.
***************************************************************************************************/
void OutboundHttpService::onNoActiveConnections(OutboundHttpConnectionManager& connMgr)
{
    // Remove the listener now that it's job is done
    connMgr.removeConnectionListener(*this);

    eastl::string hostandport = connMgr.getAddress().getHostname();
    hostandport.append_sprintf(":%u", connMgr.getAddress().getPort(InetAddress::HOST));

    OutboundHttpServiceMap::iterator iter = mReconfigureMap.find(hostandport);
    if (iter != mReconfigureMap.end())
    {
        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpService] Removing HTTP service at " << hostandport << ".");
        mReconfigureMap.erase(iter);
    }
}


const Component* OutboundHttpService::getService(const char8_t* componentName) const
{
    ServiceComponentMap::const_iterator iter = mServiceComponentMap.find(componentName);
    if (iter != mServiceComponentMap.end())
    {
        return iter->second;
    }

    return nullptr;
}

HttpConnectionManagerPtr OutboundHttpService::getConnection(const char8_t* serviceName)
{
    OutboundHttpServiceMap::const_iterator iter = mOutboundHttpServiceByNameMap.find(serviceName);
    if (iter != mOutboundHttpServiceByNameMap.cend())
    {
        return iter->second;
    }

    BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpService] Unable to find http connection manager '" << serviceName << "'.");
    return HttpConnectionManagerPtr();
}


void OutboundHttpService::getStatusInfo(HttpServiceStatusMap& infoMap) const
{
    OutboundHttpServiceMap::const_iterator iter = mOutboundHttpServiceByNameMap.begin();
    OutboundHttpServiceMap::const_iterator end = mOutboundHttpServiceByNameMap.end();

    for (; iter != end; ++iter)
    {
        const HttpConnectionManagerPtr connPool = iter->second;

        HttpServiceStatus* serviceStatus = infoMap.allocate_element();    // infoMap will delete it.
        connPool->getHttpServiceStatusInfo(*serviceStatus);
        
        const char8_t* serviceName = iter->first.c_str();
        infoMap[serviceName] = serviceStatus;
    }
}

} // end namespace Blaze
