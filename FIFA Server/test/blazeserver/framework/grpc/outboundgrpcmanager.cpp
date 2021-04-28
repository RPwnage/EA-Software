    /*************************************************************************************************/
/*!
    \file   outboundgrpcmanager.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/grpc/grpcutils.h"

#include "framework/connection/selector.h"
#include "framework/database/dbobfuscator.h"
#include "framework/vault/vaultmanager.h"

namespace Blaze
{

namespace Grpc
{

OutboundGrpcManager::OutboundGrpcManager(Selector& selector, const char8_t* instanceName)
    : BlazeThread(eastl::string(eastl::string::CtorSprintf(), "%s:OutboundGrpcManager", instanceName).c_str(), BlazeThread::GRPC_CQ, 1024 * 1024),
      mSelector(selector),
      mServiceConnCleanupQueue("OutboundGrpcManager::ServiceConnCleanupQueue"),
      mShuttingDown(false)
{
}

OutboundGrpcManager::~OutboundGrpcManager()
{
}

bool OutboundGrpcManager::initialize(const OutboundGrpcServiceConnectionMap& grpcServices)
{
    grpcServices.copyInto(mServiceConnectionInfo);
    for (auto& connInfo : mServiceConnectionInfo)
    {
        eastl::string serviceAddress(connInfo.second->getServiceAddress());
        serviceAddress.make_lower();
        if (serviceAddress.find("https://") != eastl::string::npos)
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].initialize: The configured OutboundGrpcService has a serviceAddress '"<< serviceAddress << "' which contains a protocol specifier."
                "Please remove any protocol specifier such as 'https://' from serviceAddress.");
            return false;
        }
        // Validate that the SSL context is available
        if (gSslContextManager->get(connInfo.second->getSslContext()) == nullptr)
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].initialize: Failed to find SSL context " << connInfo.second->getSslContext() << " for service " << connInfo.first.c_str());
            return false;
        }
        // Validate the Loadbalancer Policy Name.  Valid values are 'round_robin', 'pick_first', or 'grpclb'.
        if ((blaze_strcmp(connInfo.second->getLbPolicyName(), "round_robin") != 0) &&
            (blaze_strcmp(connInfo.second->getLbPolicyName(), "pick_first") != 0) &&
            (blaze_strcmp(connInfo.second->getLbPolicyName(), "grpclb") != 0))
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].initialize: The lbPolicyName '" << connInfo.second->getLbPolicyName() << "' configured for service " << connInfo.first.c_str() << " is invalid. "
                "Valid values are 'round_robin', 'pick_first', or 'grpclb'.");
            return false;
        }
    }

    return (start() != EA::Thread::kThreadIdInvalid);
}

void OutboundGrpcManager::prepareForReconfigure(const OutboundGrpcServiceConnectionMap& grpcServices, ConfigureValidationErrors& validationErrors) const
{
    for (auto& connInfo : mServiceConnectionInfo)
    {
        eastl::string serviceAddress(connInfo.second->getServiceAddress());
        serviceAddress.make_lower();
        if (serviceAddress.find("https://") != eastl::string::npos)
        {
            eastl::string msg;
            msg.append_sprintf("The configured OutboundGrpcService has a serviceAddress '%s' which contains a protocol specifier. Please remove any protocol specifier such as 'https://' from serviceAddress.", serviceAddress.c_str());
            validationErrors.getErrorMessages().push_back(msg.c_str());
        }
        
        if (gSslContextManager->get(connInfo.second->getSslContext()) == nullptr)
        {
            eastl::string msg;
            msg.append_sprintf("Failed to find SSL context %s for service %s", connInfo.second->getSslContext(), connInfo.first.c_str());
            validationErrors.getErrorMessages().push_back(msg.c_str());
        }

        if ((blaze_strcmp(connInfo.second->getLbPolicyName(), "round_robin") != 0) &&
            (blaze_strcmp(connInfo.second->getLbPolicyName(), "pick_first") != 0) &&
            (blaze_strcmp(connInfo.second->getLbPolicyName(), "grpclb") != 0))
        {
            eastl::string msg;
            msg.append_sprintf("The lbPolicyName '%s' configured for service %s is invalid. Valid values are 'round_robin', 'pick_first', or 'grpclb'.",
                connInfo.second->getLbPolicyName(), connInfo.first.c_str());
            validationErrors.getErrorMessages().push_back(msg.c_str());
        }
    }
}

void OutboundGrpcManager::cleanupServiceConn(const eastl::string& serviceName)
{
    if (mChannelByService.find(serviceName) == mChannelByService.end() ||
        mConnectionByService.find(serviceName) == mConnectionByService.end())
    {
        BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].cleanupServiceConn: Failed to find any connection/channel to ("<< serviceName <<") service. Ignore clean up request.");
        return;
    }

    ServiceCleanup serviceCleanup;
    
    for (auto& modConn : mConnectionByService.find(serviceName)->second)
        serviceCleanup.mModuleConnections.push_back(eastl::move(modConn.second).release());
            
    mConnectionByService.erase(serviceName);
    
    serviceCleanup.mChannel = eastl::move((mChannelByService.find(serviceName))->second).release();
    mChannelByService.erase(serviceName);

    mServiceConnCleanupQueue.queueFiberJob(this, &OutboundGrpcManager::cleanupServiceConnImpl, serviceCleanup, "OutboundGrpcManager::cleanupServiceConnImpl");
}

void OutboundGrpcManager::cleanupAllServiceConn()
{
    while (!mChannelByService.empty())
        cleanupServiceConn(mChannelByService.begin()->first); 

    mConnectionByService.clear();
    mChannelByService.clear();
}

void OutboundGrpcManager::reconfigure(const OutboundGrpcServiceConnectionMap& grpcServices)
{
    mServiceConnectionInfo.clear();
    grpcServices.copyInto(mServiceConnectionInfo);

    cleanupAllServiceConn();
}

void OutboundGrpcManager::refreshCerts()
{
    BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Starting refresh of Grpc outbound certificates from Vault");

    // Take a copy of the service connection map in case it mutates (e.g. due to reconfigure) while we are blocked talking to Vault
    OutboundGrpcServiceConnectionMap serviceConnMap;
    mServiceConnectionInfo.copyInto(serviceConnMap);

    for (auto& conn : serviceConnMap)
    {
        if (conn.second->getPathType() == GrpcServiceConnectionInfo::VAULT)
        {
            SecretVault::SecretVaultDataMap dataMap;
            if (gVaultManager->readSecret(conn.second->getVaultPath().getPath(), dataMap) != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Failed to refresh cert for service " << conn.first
                    << " due to error fetching data from Vault path " << conn.second->getVaultPath().getPath());
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
                BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Failed to locate certificate for service " << conn.first
                    << " in data at Vault path " << conn.second->getVaultPath().getPath());
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
                BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Failed to locate private key for service " << conn.first
                    << " in data at Vault path " << conn.second->getVaultPath().getPath());
                continue;
            }

            auto serviceInfo = mServiceConnectionInfo.find(conn.first);
            if (serviceInfo == mServiceConnectionInfo.end())
            {
                BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: No entry for service " << conn.first << " skipping cert refresh");
                continue;
            }

            if ((blaze_strcmp(cert.c_str(), serviceInfo->second->getSslCert()) == 0) && (blaze_strcmp(key.c_str(), serviceInfo->second->getSslKey()) == 0))
            {
                BLAZE_INFO_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Cert and key for service " << conn.first << " have not changed");
                continue;
            }

            BLAZE_INFO_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Updating cert and key for service " << conn.first);
            serviceInfo->second->setSslCert(cert.c_str());
            serviceInfo->second->setSslKey(key.c_str());
        }
    }

    BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].refreshCerts: Done refresh of Grpc outbound certificates");
}

// Helper fiber to shutdown the services that are no longer in use. 
void OutboundGrpcManager::cleanupServiceConnImpl(ServiceCleanup serviceCleanup)
{
    for (auto& modConn : serviceCleanup.mModuleConnections)
    {
        modConn->shutdown();
        delete modConn;
    }

    delete serviceCleanup.mChannel;
}

void OutboundGrpcManager::stop()
{
    waitForEnd();
}

void OutboundGrpcManager::shutdown()
{
    if (mShuttingDown)
        return;

    mShuttingDown = true;

    cleanupAllServiceConn();
    mServiceConnCleanupQueue.join();

    mCQ.Shutdown();
    stop();
}

void OutboundGrpcManager::run()
{
    while (true)
    {
        TagInfo tagInfo;
        tagInfo.fiberPriority = ++mFiberPriority;

        if (!mCQ.Next((void**)&tagInfo.tagProcessor, &tagInfo.ok))
        {
            BLAZE_INFO_LOG(Log::GRPC, "[OutboundGrpcManager].run: Shutting down grpc completion queue processing thread as the queue indicated a shutdown.");
            break;
        }

        // Put the incoming request on our selector queue. If we call mSelector.scheduleFiberCall, it'd end up creating a delayed fiber call (See FiberManager::executeJob).
        mSelector.scheduleCall(this, &OutboundGrpcManager::processTag, tagInfo, tagInfo.fiberPriority);
    }
}

void OutboundGrpcManager::getStatusInfo(OutboundGrpcServiceStatusMap& infoMap) const
{
    for (const auto& conn : mConnectionByService)
    {
        for (const auto& modConn : conn.second)
            modConn.second->getStatusInfo(infoMap);
    }
}

void OutboundGrpcManager::processTag(TagInfo tag)
{
    (*(tag.tagProcessor))(tag.ok);
}

} // namespace Grpc
} // namespace Blaze
