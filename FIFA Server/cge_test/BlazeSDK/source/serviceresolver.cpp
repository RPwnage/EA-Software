/*************************************************************************************************/
/*!
    \file serviceresolver.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/serviceresolver.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazesender.h"
#include "BlazeSDK/jobscheduler.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/component/redirectorcomponent.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/version.h"
#include "shared/framework/protocol/shared/heat2encoder.h"
#include "shared/framework/protocol/shared/heat2decoder.h"
#include "shared/framework/protocol/shared/httpprotocolutil.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtycert.h"

namespace Blaze
{

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_GDK)
static uint16_t const REDIRECTOR_HTTPS_PORT = 42130;
#else
static uint16_t const REDIRECTOR_HTTPS_PORT = 42230;
#endif

typedef struct RedirectorEnvironment
{
    EnvironmentType env;
    const char8_t* hostname;
} RedirectorEnvironment;

static const RedirectorEnvironment sRedirectorsByEnvironment[] =
{
    { ENVIRONMENT_SDEV, "spring18.gosredirector.sdev.ea.com" },
    { ENVIRONMENT_STEST, "spring18.gosredirector.stest.ea.com" },
    { ENVIRONMENT_SCERT, "spring18.gosredirector.scert.ea.com" },
    { ENVIRONMENT_PROD, "spring18.gosredirector.ea.com" },
    { ENVIRONMENT_SDEV, nullptr }
};


ServiceResolver::ServiceResolver(BlazeHub &hub, MemoryGroupId memGroupId)
    : mHub(hub),
      mMemGroup(memGroupId),
      mRdirProxy(nullptr)
{
    // Still need to instantiate base component even when using HTTP to pull in error code
    // parsing as the server returns as the redirector component.
    Redirector::RedirectorComponent::createComponent(&mHub);
}

ServiceResolver::~ServiceResolver()
{
    for (RequestInfoMap::iterator itr = mRequestInfoMap.begin(); itr != mRequestInfoMap.end(); ++itr)
    {
        RequestInfo* requestInfo = itr->second;
        mHub.getScheduler()->removeJob(requestInfo->mJobId);

        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, requestInfo);   
    }
}

Redirector::RedirectorComponent* ServiceResolver::getRedirectorProxy()
{
    if (mRdirProxy == nullptr)
    {
        BlazeSender::ServerConnectionInfo rdirConnInfo;
        getRedirectorConnInfo(rdirConnInfo);

        // The RedirectorProxy component might actually already be created, but we need
        // it do be associated with an HttpConnection.  Using this createComponent() overload
        // make sure it's created, and has an HttpConnection associated with it.
        Redirector::RedirectorComponent::createComponent(&mHub, rdirConnInfo, Encoder::XML2);

        mRdirProxy = mHub.getComponentManager()->getRedirectorComponent();
    }
    else
    {
        // We need to make sure that the Redirector Component is using the correct settings.
        // On the XboxOne, these settings may change depending on the user's connection to XBL, so to be safe we
        // update the connection information each time, to keep it in sync with the latest info.
        BlazeSender::ServerConnectionInfo rdirConnInfo;
        getRedirectorConnInfo(rdirConnInfo);

        BlazeSender* blazeSender = mHub.getComponentManager()->getBlazeSender(mRdirProxy->getComponentId());
        if (blazeSender)
        {
            if (!blazeSender->isActive())
            {
                blazeSender->setServerConnInfo(rdirConnInfo);
            }
            else
            {
                BLAZE_SDK_DEBUGF("Unexpected: Resolve Service being called while redirector component is still active. "
                                 "Redirector will continue trying to connect to (Address: %s, Service: %s), not (Address: %s, Service: %s).", 
                                 blazeSender->getServerConnInfo().getAddress(), blazeSender->getServerConnInfo().getServiceName(), 
                                 rdirConnInfo.getAddress(), rdirConnInfo.getServiceName());
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("Unexpected: BlazeSender not valid in current redirector component.");
        }
        
    }

    BlazeAssertMsg(mRdirProxy != nullptr, "[ServiceResolver] ERR: unable to find redirector component");

    return mRdirProxy;
}

void ServiceResolver::destroyRedirectorProxy()
{
    if (mRdirProxy != nullptr)
    {
        mHub.getComponentManager()->destroyComponent(Blaze::REDIRECTORCOMPONENT_COMPONENT_ID);
        mRdirProxy = nullptr;
    }
}

JobId ServiceResolver::resolveService(const char8_t* serviceName, const ServiceResolver::ResolveCb& cb)
{
    // early out if we already have an ongoing request
    if (mRdirProxy != nullptr)
    {
        BlazeSender* blazeSender = mHub.getComponentManager()->getBlazeSender(mRdirProxy->getComponentId());
        if (blazeSender)
        {
            if (blazeSender->isActive())
            {
                BLAZE_SDK_DEBUGF("ServiceResolver.resolveService: Unexpected. Resolve Service being called while redirector component is still active. "
                    "Redirector will continue trying to connect to (Address: %s, Service: %s).",
                    blazeSender->getServerConnInfo().getAddress(), blazeSender->getServerConnInfo().getServiceName());
                return INVALID_JOB_ID;
            }
        }
    }

    RequestInfo* requestInfo = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "ServiceResolver.resolveService") RequestInfo(serviceName, cb);

    requestInfo->mJobId = mHub.getScheduler()->scheduleMethod(this, &ServiceResolver::startServiceRequest, requestInfo, this);
    Job::addTitleCbAssociatedObject(mHub.getScheduler(), requestInfo->mJobId, cb); 

    mRequestInfoMap[requestInfo->mJobId] = requestInfo;

    return requestInfo->mJobId;
}


void ServiceResolver::startServiceRequest(RequestInfo* requestInfo)
{
    if (requestInfo == nullptr)
    {
        BLAZE_SDK_DEBUGF("ServiceResolver.startServiceRequest: Provided requestInfo is nullptr. Unable to start service request. \n" );
        return;
    }

    if (requestInfo->mServiceName[0] == '\0')
    {
        requestInfo->mCb(SDK_ERR_NO_SERVICE_NAME_PROVIDED, nullptr, nullptr, nullptr, PROTOSSL_ERROR_NONE, SOCKERR_NONE);

        RequestInfoMap::iterator it = mRequestInfoMap.find(requestInfo->mJobId);
        if (it != mRequestInfoMap.end())
        {
            RequestInfo* info = it->second;
            mRequestInfoMap.erase(it);
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, info);
        }
        return;
    }

    int32_t status = NetConnStatus('conn', 0, nullptr, 0);
    if (status == 0 || (status >> 24) == '-')
    {
        // if explicitly disconnected or any error condition about being connected, don't reschedule online check
        requestInfo->mCb(ERR_DISCONNECTED, requestInfo->mServiceName, nullptr, nullptr, PROTOSSL_ERROR_NONE, SOCKERR_NONE);

        RequestInfoMap::iterator it = mRequestInfoMap.find(requestInfo->mJobId);
        if (it != mRequestInfoMap.end())
        {
            RequestInfo* info = it->second;
            mRequestInfoMap.erase(it);
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, info);
        }
        return;
    }
    if (status != '+onl')
    {
        // re-schedule ourself to check if status is online
        mRequestInfoMap.erase(requestInfo->mJobId);

        requestInfo->mJobId = mHub.getScheduler()->scheduleMethod(this, &ServiceResolver::startServiceRequest, requestInfo, this);
        Job::addTitleCbAssociatedObject(mHub.getScheduler(), requestInfo->mJobId, requestInfo->mCb);

        mRequestInfoMap[requestInfo->mJobId] = requestInfo;
        return;
    }

    Redirector::ServerInstanceRequest serverInstanceRequest;
    prepareServerInstanceRequest(*requestInfo, serverInstanceRequest);

    Redirector::RedirectorComponent *rdirProxy = getRedirectorProxy();
    JobId jobId = rdirProxy->getServerInstanceHttp(serverInstanceRequest,
        Redirector::RedirectorComponent::GetServerInstanceCb(
        (ServiceResolver*)this, &ServiceResolver::onGetServerInstance), requestInfo->mJobId);

    Job::addTitleCbAssociatedObject(mHub.getScheduler(), jobId, requestInfo->mCb);
}

void ServiceResolver::onGetServerInstance(const Redirector::ServerInstanceInfo* response,
    const Redirector::ServerInstanceError* errorResponse, BlazeError error, JobId jobId)
{
    // find the requestInfo based on jobId
    RequestInfoMap::iterator requestInfoIt = mRequestInfoMap.find(jobId);
    if (requestInfoIt == mRequestInfoMap.end())
    {
        return;
    }

    RequestInfo* requestInfo = requestInfoIt->second;
    mRequestInfoMap.erase(requestInfoIt);  // remove from mRequestInfoMap first

    if ((error == ERR_OK) && (response != nullptr) && (response->getAddress().getActiveMember() != Redirector::ServerAddress::MEMBER_UNSET))
    {
        Redirector::CACertificateList::const_iterator it = response->getCertificateList().begin();
        Redirector::CACertificateList::const_iterator end = response->getCertificateList().end();
        for ( ; it != end; ++it)
        {
            ProtoSSLSetCACert((*it)->getData(), (int32_t)(*it)->getSize());
        }

        // disable pre-loading, since we just did that
        DirtyCertControl('prld', 0, 0, nullptr);
    }
    else
    {
#if ENABLE_BLAZE_SDK_LOGGING
        const char8_t* envStr = EnvironmentTypeToString(getPlatformEnv(mHub.getInitParams().Environment));
        const char8_t* messageHeader = "ServiceResolver.onGetServerInstance";
        const size_t maxMessageLength = 128;
        char8_t messageTail[maxMessageLength];
        blaze_snzprintf(messageTail, maxMessageLength, "error code %s (0x%x), disconnecting\n", mHub.getComponentManager()->getErrorName(error), error);

        switch (error)
        {
        case REDIRECTOR_NO_MATCHING_INSTANCE:
            BLAZE_SDK_DEBUGF("%s : %s redirector could NOT assign an instance of the requested service %s, %s", messageHeader, envStr, requestInfo->mServiceName, messageTail);
            break;
        case REDIRECTOR_SERVER_NOT_FOUND:
            BLAZE_SDK_DEBUGF("%s : %s redirector could NOT find the requested service %s, %s", messageHeader, envStr, requestInfo->mServiceName, messageTail);
            break;
        case REDIRECTOR_NO_SERVER_CAPACITY:
            BLAZE_SDK_DEBUGF("%s : The requested service %s has reached maximum connections, %s", messageHeader, requestInfo->mServiceName, messageTail);
            break;
        case REDIRECTOR_CLIENT_NOT_COMPATIBLE:
            BLAZE_SDK_DEBUGF("%s : Current client version %s is NOT compatible with %s redirector, %s", messageHeader, mHub.getInitParams().ClientVersion, envStr, messageTail);
            break;
        case REDIRECTOR_CLIENT_UNKNOWN:
            BLAZE_SDK_DEBUGF("%s : Current client version %s could NOT be recognized by %s redirector, %s", messageHeader, mHub.getInitParams().ClientVersion, envStr, messageTail);
            break;
        case REDIRECTOR_UNKNOWN_CONNECTION_PROFILE:
            BLAZE_SDK_DEBUGF("%s : %s redirector could NOT locate the requested connection profile, %s", messageHeader, envStr, messageTail);
            break;
        case REDIRECTOR_SERVER_SUNSET:
            BLAZE_SDK_DEBUGF("%s : The requested service %s has been sunset, %s", messageHeader, requestInfo->mServiceName, messageTail);
            break;
        case REDIRECTOR_SERVER_DOWN:
            BLAZE_SDK_DEBUGF("%s : The requested service %s is down, %s", messageHeader, requestInfo->mServiceName, messageTail);
            break;
        default:
            BLAZE_SDK_DEBUGF("%s : Unexpected error code for service %s received from %s redirector, %s", messageHeader, requestInfo->mServiceName, envStr, messageTail);
        }
#endif
    }

    requestInfo->mCb(error, requestInfo->mServiceName, response, errorResponse, PROTOSSL_ERROR_NONE, SOCKERR_NONE);

    // finally delete the requestInfo obj
    BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, requestInfo);
}

void ServiceResolver::prepareServerInstanceRequest(const RequestInfo& requestInfo, Redirector::ServerInstanceRequest& serverInstanceRequest) const
{
    const BlazeHub::InitParameters& initParams = mHub.getInitParams();

    serverInstanceRequest.setName(requestInfo.mServiceName);
    serverInstanceRequest.setPlatform(EA_PLATFORM_NAME);
    serverInstanceRequest.setClientName(initParams.ClientName);
    serverInstanceRequest.setClientVersion(initParams.ClientVersion);
    serverInstanceRequest.setClientType(initParams.Client);
    serverInstanceRequest.setClientSkuId(initParams.ClientSkuId);
    serverInstanceRequest.setClientLocale(initParams.Locale);
    serverInstanceRequest.setTimeZoneOffset(ds_timezone());
    serverInstanceRequest.setClientPlatform(mHub.getConnectionManager()->getClientPlatformType());
    serverInstanceRequest.setBlazeSDKVersion(getBlazeSdkVersionString());

    char8_t buildDate[64];
    blaze_snzprintf(buildDate, sizeof(buildDate), "%s %s", __DATE__, __TIME__);
    serverInstanceRequest.setBlazeSDKBuildDate(buildDate);

    char8_t dsVers[32];
    blaze_snzprintf(dsVers, sizeof(dsVers), "%d.%d.%d.%d.%d",
        DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    serverInstanceRequest.setDirtySDKVersion(dsVers);

    switch (getPlatformEnv(initParams.Environment))
    {
    case ENVIRONMENT_SDEV:
        serverInstanceRequest.setEnvironment(Redirector::ENVIRONMENT_SDEV);
        break;
    case ENVIRONMENT_STEST:
        serverInstanceRequest.setEnvironment(Redirector::ENVIRONMENT_STEST);
        break;
    case ENVIRONMENT_SCERT:
        serverInstanceRequest.setEnvironment(Redirector::ENVIRONMENT_SCERT);
        break;
    case ENVIRONMENT_PROD:
        serverInstanceRequest.setEnvironment(Redirector::ENVIRONMENT_PROD);
        break;
    }

    if (initParams.Secure)
        serverInstanceRequest.setConnectionProfile(Redirector::CONNECTIONPROFILE_STANDARD_CLIENT_SECURE);
    else
        serverInstanceRequest.setConnectionProfile(Redirector::CONNECTIONPROFILE_STANDARD_CLIENT);

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    // Include 1st party information
    // First, lets convert the local index to the user index used by the console
    uint32_t primeLocalUserIndex = mHub.getPrimaryLocalUserIndex();
    uint32_t dirtySockUserIndex = mHub.getLoginManager(primeLocalUserIndex)->getDirtySockUserIndex();

    char8_t gamertag[32] = "";
    NetConnStatus('gtag', dirtySockUserIndex, gamertag, sizeof(gamertag));
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    uint64_t xboxUid = 0;
    NetConnStatus('xuid', dirtySockUserIndex, &xboxUid, sizeof(xboxUid));

    Redirector::XboxId xboxId;
    xboxId.setGamertag(gamertag);
    xboxId.setXuid(xboxUid);

    // Add in the caps sandbox info:
    char8_t sandboxId[32] = "";
    NetConnStatus('envi', 0, (void*)sandboxId, sizeof(sandboxId));
    xboxId.setSandboxId(sandboxId);    

    serverInstanceRequest.getFirstPartyId().setXboxId(&xboxId);

#elif defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    serverInstanceRequest.getFirstPartyId().setPersona(gamertag);
#endif

// Check if the running title is a trial
#if defined(BLAZESDK_TRIAL_SUPPORT)
    int32_t isTrial = NetConnStatus('tria', 0, nullptr, 0);
    serverInstanceRequest.setIsTrial(isTrial == 1 ? true : false);
#else
    serverInstanceRequest.setIsTrial(initParams.isTrial);
#endif

}

EnvironmentType ServiceResolver::getPlatformEnv(EnvironmentType curEnv) const
{
    EnvironmentType newEnv = curEnv;

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
    int32_t backEndEnvironment = NetConnStatus('envi', 0, nullptr, 0);
    if (curEnv != ENVIRONMENT_SDEV)
    {
        switch (backEndEnvironment)
        {
        case NETCONN_PLATENV_DEV:
            newEnv = ENVIRONMENT_SDEV;
            break;
        case NETCONN_PLATENV_TEST:
            newEnv = ENVIRONMENT_STEST;
            break;
        case NETCONN_PLATENV_CERT:
            newEnv = ENVIRONMENT_SCERT;
            break;
        case NETCONN_PLATENV_PROD:
            newEnv = ENVIRONMENT_PROD;
            break;
        default:
            newEnv = ENVIRONMENT_PROD;
            BLAZE_SDK_DEBUGF("ServiceResolver.getPlatformEnv: Unknown environment %d from NetConnStatus, defaulting to environment \"%s\" \n", 
                backEndEnvironment, EnvironmentTypeToString(newEnv));
        }
    }
    else if (backEndEnvironment != NETCONN_PLATENV_DEV)
    {
        BLAZE_SDK_DEBUGF("ServiceResolver.getPlatformEnv: Requested environment \"%s\" is not using DEV platform environment \n", EnvironmentTypeToString(curEnv));
    }
#endif

    BLAZE_SDK_DEBUGF("ServiceResolver.getPlatformEnv: Passed in environment is: \"%s\", resolved environment is: \"%s\"\n", 
        EnvironmentTypeToString(curEnv), EnvironmentTypeToString(newEnv));

    return newEnv;
}

const char8_t* ServiceResolver::getRedirectorByEnvironment(EnvironmentType env) const
{
    for (int32_t idx = 0; ; ++idx)
    {
        // Expects final entry in map to have nullptr hostname.
        if (sRedirectorsByEnvironment[idx].hostname == nullptr)
        {
            BlazeAssertMsg(false,  "No redirector entry found for environment.");
            return nullptr;
        }
        if (sRedirectorsByEnvironment[idx].env == env)
        {
            return sRedirectorsByEnvironment[idx].hostname;
        }
    }
}

void ServiceResolver::getRedirectorConnInfo(BlazeSender::ServerConnectionInfo &connInfo) const
{
    const BlazeHub::InitParameters& params = mHub.getInitParams();

    const char8_t *rdirAddress = nullptr;
    uint16_t rdirPort = REDIRECTOR_HTTPS_PORT;
    bool rdirSecure = true;

    if (params.Override.RedirectorAddress[0] != '\0')
    {
        // The user wants to override the default redirector address
        rdirAddress = params.Override.RedirectorAddress;
        rdirSecure = params.Override.RedirectorSecure;
        if (params.Override.RedirectorPort != 0)
            rdirPort = params.Override.RedirectorPort;
    }

    if (rdirAddress == nullptr)
    {
        EnvironmentType env = getPlatformEnv(params.Environment);
        rdirAddress = getRedirectorByEnvironment(env);
    }

    connInfo = BlazeSender::ServerConnectionInfo(rdirAddress, rdirPort, rdirSecure);
}
} // namespace Blaze

