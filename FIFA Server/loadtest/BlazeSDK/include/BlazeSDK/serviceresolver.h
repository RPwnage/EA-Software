/*************************************************************************************************/
/*!
    \file serviceresolver.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_SERVICE_RESOLVER_H
#define BLAZE_SERVICE_RESOLVER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/jobid.h"
#include "EASTL/intrusive_list.h"

#include "BlazeSDK/component/framework/tdf/networkaddress.h"
#include "BlazeSDK/internetaddress.h"

namespace Blaze
{

class BlazeHub;
class Heat2Encoder;
class Heat2Decoder;

namespace Redirector
{
    class ServerInstanceInfo;
    class ServerInstanceError;
    class ServerInstanceErrorResponse;
    class ServerInstanceHttpRequest;
}

class BLAZESDK_API ServiceResolver
{
    NON_COPYABLE(ServiceResolver);

public:
    typedef Functor6<BlazeError, const char8_t*, const Redirector::ServerInstanceInfo*, const Redirector::ServerInstanceError*, int32_t, int32_t> ResolveCb;

public:   
    ServiceResolver(BlazeHub &hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~ServiceResolver();

    JobId resolveService(const char8_t* serviceName, const ResolveCb& cb);

private:
    friend class BlazeHub;

    class RequestInfo
    {
    public:
        RequestInfo(const char8_t* serviceName, const ResolveCb& cb) : mCb(cb), mJobId(INVALID_JOB_ID)
        {
            if (serviceName != nullptr)
            {
                blaze_strnzcpy(mServiceName, serviceName, sizeof(mServiceName));
            }
            else
            {
                mServiceName[0] = '\0';
            }
        }

        char8_t mServiceName[128];
        ResolveCb mCb;
        JobId mJobId;
    };

    Redirector::RedirectorComponent* getRedirectorProxy();
    void destroyRedirectorProxy();

    void startServiceRequest(RequestInfo* request);
    void onGetServerInstance(const Redirector::ServerInstanceInfo* response, 
        const Redirector::ServerInstanceError* errorResponse, BlazeError error, JobId jobId);

    void prepareServerInstanceRequest(const RequestInfo& requestInfo, Redirector::ServerInstanceRequest& serverInstanceRequest) const;

    EnvironmentType getPlatformEnv(EnvironmentType curEnv) const;
    const char8_t* getRedirectorByEnvironment(EnvironmentType env) const;
    void getRedirectorConnInfo(BlazeSender::ServerConnectionInfo &connInfo) const;

private:
    BlazeHub &mHub;
    MemoryGroupId mMemGroup;

    Redirector::RedirectorComponent *mRdirProxy;

    typedef eastl::map<JobId, RequestInfo*> RequestInfoMap;
    RequestInfoMap mRequestInfoMap;
};

}   // namespace Blaze

#endif
