/*************************************************************************************************/
/*!
    \file   outboundgrpcmanager.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDGRPCMANAGER_H
#define BLAZE_OUTBOUNDGRPCMANAGER_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"
#include "framework/logger.h"
#include "framework/system/blazethread.h"
#include "framework/system/fiberjobqueue.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"
#include "framework/grpc/outboundgrpcconnection.h"
#include "framework/metrics/metrics.h"
#include "framework/grpc/grpcutils.h"

#include "framework/tdf/frameworkconfigdefinitions_server.h"
#include "framework/tdf/controllertypes_server.h"

#include <EASTL/hash_map.h>

namespace Blaze
{

class Selector;
class ConfigureValidationErrors;

namespace Grpc
{

// These helper functions can be used outside to generate gRPC Rpc connections:
// Connection Name (string), Module, (async) RPC
#define CREATE_GRPC_UNARY_RPC(connectionName, module, rpc) gController->getOutboundGrpcManager()->createUnaryRpc<module>(connectionName, #module, #rpc, &module::Stub::rpc)
#define CREATE_GRPC_CLIENT_RPC(connectionName, module, rpc) gController->getOutboundGrpcManager()->createClientStreamingRpc<module>(connectionName, #module, #rpc, &module::Stub::rpc)
#define CREATE_GRPC_SERVER_RPC(connectionName, module, rpc) gController->getOutboundGrpcManager()->createServerStreamingRpc<module>(connectionName, #module, #rpc, &module::Stub::rpc)
#define CREATE_GRPC_BIDIRECTIONAL_RPC(connectionName, module, rpc) gController->getOutboundGrpcManager()->createBidirectionalStreamingRpc<module>(connectionName, #module, #rpc, &module::Stub::rpc)

class OutboundGrpcManager : public BlazeThread
{
    NON_COPYABLE(OutboundGrpcManager);

public:
    OutboundGrpcManager(Selector& selector, const char8_t* instanceName);
    ~OutboundGrpcManager();

    bool initialize(const OutboundGrpcServiceConnectionMap& grpcServices);
    void prepareForReconfigure(const OutboundGrpcServiceConnectionMap& grpcServices, ConfigureValidationErrors& validationErrors) const;
    void reconfigure(const OutboundGrpcServiceConnectionMap& grpcServices);
    void refreshCerts();
    
    void cleanupServiceConn(const eastl::string& serviceName);
    void cleanupAllServiceConn();
    void shutdown();

    // INTERNAL FUNCTIONS: 
    // Use the CREATE_GRPC_*_RPC macros above for a simpler syntax
    template <typename Module, typename StubType, typename RequestType, typename ResponseType>
    OutboundRpcBasePtr createUnaryRpc(const char8_t* serviceName, const char* moduleName, const char* rpcName, 
        std::unique_ptr< ::grpc::ClientAsyncResponseReader< ResponseType>> (StubType::* rawFunc)(::grpc::ClientContext*, const RequestType&, ::grpc::CompletionQueue*))
    {
        if (mShuttingDown)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].createUnaryRpc: Server is shutting down. RPC will not be created.");
            return nullptr;
        }

        OutboundUnaryRequestRpc<StubType, RequestType, ResponseType> createRpc(rawFunc);
        auto serviceInfo = mServiceConnectionInfo.find(serviceName);
        if (serviceInfo == mServiceConnectionInfo.end())
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].createUnaryRpc: Service name " << serviceName << " has not been configured.");
            return nullptr;
        }

        OutboundGrpcConnection<Module>* conn = getConn<Module>(serviceName, *serviceInfo->second.get());

        OutboundRpcBasePtr rpc = BLAZE_NEW OutboundUnaryRpc<Module, RequestType, ResponseType>(
            conn, moduleName, rpcName, serviceInfo->second->getRequestTimeout(), createRpc, &mCQ);
        return rpc;
    };

    template <typename Module, typename StubType, typename RequestType, typename ResponseType>
    OutboundRpcBasePtr createClientStreamingRpc(const char8_t* serviceName, const char* moduleName, const char* rpcName, 
        std::unique_ptr< ::grpc::ClientAsyncWriter< RequestType>>(StubType::* rawFunc)(::grpc::ClientContext*, ResponseType*, ::grpc::CompletionQueue*, void*))
    {
        if (mShuttingDown)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].createClientStreamingRpc: Server is shutting down. RPC will not be created.");
            return nullptr;
        }

        OutboundClientStreamingRequestRpc<StubType, RequestType, ResponseType> createStream(rawFunc);
        auto serviceInfo = mServiceConnectionInfo.find(serviceName);
        if (serviceInfo == mServiceConnectionInfo.end())
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].createClientStreamingRpc: Service name " << serviceName << " has not been configured.");
            return nullptr;
        }

        OutboundGrpcConnection<Module>* conn = getConn<Module>(serviceName, *serviceInfo->second.get());

        OutboundRpcBasePtr rpc = BLAZE_NEW OutboundClientStreamingRpc<Module, RequestType, ResponseType>(
            conn, moduleName, rpcName, serviceInfo->second->getRequestTimeout(), createStream, &mCQ);
        return rpc;
    };

    template <typename Module, typename StubType, typename RequestType, typename ResponseType>
    OutboundRpcBasePtr createServerStreamingRpc(const char8_t* serviceName, const char* moduleName, const char* rpcName, 
        std::unique_ptr< ::grpc::ClientAsyncReader< ResponseType>>(StubType::* rawFunc)(::grpc::ClientContext*, const RequestType&, ::grpc::CompletionQueue*, void*))
    {
        if (mShuttingDown)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].createServerStreamingRpc: Server is shutting down. RPC will not be created.");
            return nullptr;
        }

        OutboundServerStreamingRequestRpc<StubType, RequestType, ResponseType> createStream(rawFunc);
        auto serviceInfo = mServiceConnectionInfo.find(serviceName);
        if (serviceInfo == mServiceConnectionInfo.end())
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].createServerStreamingRpc: Service name " << serviceName << " has not been configured.");
            return nullptr;
        }

        OutboundGrpcConnection<Module>* conn = getConn<Module>(serviceName, *serviceInfo->second.get());

        OutboundRpcBasePtr rpc = BLAZE_NEW OutboundServerStreamingRpc<Module, RequestType, ResponseType>(
            conn, moduleName, rpcName, serviceInfo->second->getRequestTimeout(), createStream, &mCQ);
        return rpc;
    };

    template <typename Module, typename StubType, typename RequestType, typename ResponseType>
    OutboundRpcBasePtr createBidirectionalStreamingRpc(const char8_t* serviceName, const char* moduleName, const char* rpcName, 
        std::unique_ptr< ::grpc::ClientAsyncReaderWriter< RequestType, ResponseType>>(StubType::* rawFunc)(::grpc::ClientContext*, ::grpc::CompletionQueue*, void*))
    {
        if (mShuttingDown)
        {
            BLAZE_TRACE_LOG(Log::GRPC, "[OutboundGrpcManager].createBidirectionalStreamingRpc: Server is shutting down. RPC will not be created.");
            return nullptr;
        }

        OutboundBidirectionalStreamingRequestRpc<StubType, RequestType, ResponseType> createStream(rawFunc);
        auto serviceInfo = mServiceConnectionInfo.find(serviceName);
        if (serviceInfo == mServiceConnectionInfo.end())
        {
            BLAZE_WARN_LOG(Log::GRPC, "[OutboundGrpcManager].createBidirectionalStreamingRpc: Service name " << serviceName << " has not been configured.");
            return nullptr;
        }

        OutboundGrpcConnection<Module>* conn = getConn<Module>(serviceName, *serviceInfo->second.get());

        OutboundRpcBasePtr rpc = BLAZE_NEW OutboundBidirectionalStreamingRpc<Module, RequestType, ResponseType>(
            conn, moduleName, rpcName, serviceInfo->second->getRequestTimeout(), createStream, &mCQ);
        return rpc;
    };

    // BlazeThread implementation
    void stop() override;
    void run() override;

    void getStatusInfo(OutboundGrpcServiceStatusMap& infoMap) const;
    bool hasService(const char8_t* serviceName) const { return mServiceConnectionInfo.find(serviceName) != mServiceConnectionInfo.end();  }
    const GrpcServiceConnectionInfo* getServiceInfo(const char8_t* serviceName) const
    {
        if (!hasService(serviceName))
            return nullptr;
        return mServiceConnectionInfo.find(serviceName)->second;
    }

private:
    void processTag(TagInfo info);

    template <typename Module>
    OutboundGrpcConnection<Module>* getConn(const char8_t* serviceName, const GrpcServiceConnectionInfo& connInfo)
    {
        OutboundGrpcChannel* channel = nullptr;

        eastl::hash_map<eastl::string, OutboundGrpcChannelPtr>::iterator itrChannel = mChannelByService.find_as(serviceName);
        if (itrChannel == mChannelByService.end())
        {
            channel = BLAZE_NEW OutboundGrpcChannel(serviceName, connInfo);
            mChannelByService[serviceName] = OutboundGrpcChannelPtr(channel);
        }
        else
        {
            channel = itrChannel->second.get();
        }

        OutboundGrpcConnection<Module>* conn = nullptr;

        eastl::string serviceAndModuleName;
        serviceAndModuleName.sprintf("%s%s%s", serviceName, SERVICE_MODULE_DELIMITER, Module::service_full_name());

        ConnectionByModule::iterator itrConnection = mConnectionByService[serviceName].find(serviceAndModuleName);
        if (itrConnection == mConnectionByService[serviceName].end())
        {
            conn = BLAZE_NEW OutboundGrpcConnection<Module>(serviceAndModuleName.c_str(), *channel);
            (mConnectionByService[serviceName])[serviceAndModuleName] = OutboundGrpcConnectionBasePtr(conn);
        }
        else
        {
            conn = (OutboundGrpcConnection<Module>*)(itrConnection->second.get());
        }

        return conn;
    }

private:

    Selector& mSelector;
    OutboundGrpcServiceConnectionMap mServiceConnectionInfo;

    typedef eastl::unique_ptr<OutboundGrpcChannel> OutboundGrpcChannelPtr;
    eastl::hash_map<eastl::string, OutboundGrpcChannelPtr> mChannelByService;
    
    typedef eastl::unique_ptr<OutboundGrpcConnectionBase> OutboundGrpcConnectionBasePtr;
    typedef eastl::hash_map<eastl::string, OutboundGrpcConnectionBasePtr> ConnectionByModule;
    eastl::hash_map<eastl::string, ConnectionByModule> mConnectionByService;
    
    struct ServiceCleanup // contains raw pointers to allow for copying into fiber calls
    {
        eastl::list<OutboundGrpcConnectionBase*> mModuleConnections;
        OutboundGrpcChannel* mChannel;

        ServiceCleanup()
            : mChannel(nullptr)
        {

        }
    };

    FiberJobQueue mServiceConnCleanupQueue;
    grpc::CompletionQueue mCQ;

    // An ever increasing number to make sure our events are executed in-order. The fiber system priority is indeterministic if the events came within a micro-second.
    int64_t mFiberPriority;

    bool mShuttingDown;

    void cleanupServiceConnImpl(ServiceCleanup serviceCleanup);

};

} // namespace Grpc
} // namespace Blaze

#endif // BLAZE_OUTBOUNDGRPCMANAGER_H
