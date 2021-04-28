/*! ************************************************************************************************/
/*!
    \file utilapi.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/internal/util/utiljobs.h"
#include "BlazeSDK/usermanager/usermanager.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h" // for ConnectionManager in OverrideConfigs()
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"

#include "EAStdC/EAString.h"


#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_NX) || defined(EA_PLATFORM_OSX)
    #define strtok_s strtok_r
#endif

namespace Blaze
{
namespace Util
{

/*! public methods *******************************************************************************/
UtilAPI::UtilAPI(BlazeHub &blazeHub)
    : SingletonAPI(blazeHub),
    mClientUserMetricsJobId(INVALID_JOB_ID)
{
    Util::UtilComponent::createComponent(&blazeHub);
    mComponent = getBlazeHub()->getComponentManager()->getUtilComponent();
    if (EA_UNLIKELY(!mComponent))
    {
        BlazeAssert(false && "UtilAPI - the util component is invalid.");
        return;
    }
    ds_memclr(&mUsersAccessabilityState, sizeof(UserAccessabilityStateT) * VOIP_MAXLOCALUSERS);
    getBlazeHub()->getUserManager()->addPrimaryUserListener(this);
}

UtilAPI::~UtilAPI()
{
    getBlazeHub()->getUserManager()->removePrimaryUserListener(this);

    // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
    // to call the callback.  Since the object is being deleted, we go with the remove.
    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
    mClientUserMetricsJobId = INVALID_JOB_ID;
}

void UtilAPI::logStartupParameters() const
{
}

void UtilAPI::createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator /*= nullptr*/)
{
    if(hub.getUtilAPI() != nullptr)
    {
        BLAZE_SDK_DEBUGF("[UtilAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }

    if (Blaze::Allocator::getAllocator(MEM_GROUP_UTIL) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_UTIL, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
    Util::UtilComponent::createComponent(&hub);

    hub.createAPI(UTIL_API, BLAZE_NEW(MEM_GROUP_UTIL, "UtilAPI") UtilAPI(hub));
}

VoipSpeechToTextMetricsT UtilAPI::getVoipSpeechToTextMetrics(uint32_t iDsUserIndex)
{
    VoipSpeechToTextMetricsT VoipSpeechToTextMetrics;
    ds_memclr(&VoipSpeechToTextMetrics, sizeof(VoipSpeechToTextMetrics));

    //transcribe metrics
    VoipRefT *pVoipRefT = VoipGetRef();
    if (pVoipRefT)
    {
        VoipStatus(pVoipRefT, 'sttm', iDsUserIndex, &VoipSpeechToTextMetrics, sizeof(VoipSpeechToTextMetrics));
    }

    return VoipSpeechToTextMetrics;
}

VoipTextToSpeechMetricsT UtilAPI::getVoipTextToSpeechMetrics(uint32_t iDsUserIndex)
{
    VoipTextToSpeechMetricsT VoipTextToSpeechMetrics;
    ds_memclr(&VoipTextToSpeechMetrics, sizeof(VoipTextToSpeechMetrics));

    //narate metrics
    VoipRefT *pVoipRefT = VoipGetRef();
    if (pVoipRefT)
    {
        VoipStatus(pVoipRefT, 'ttsm', iDsUserIndex, &VoipTextToSpeechMetrics, sizeof(VoipTextToSpeechMetrics));
    }

    return VoipTextToSpeechMetrics;
}

void UtilAPI::clientUserMetricsJob()
{
    int32_t iClientUserMetricsRate = DEFAULT_CLIENT_USER_METRICS_RATE;
    mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_CLIENT_METRICS2_UPDATE_RATE, &iClientUserMetricsRate);

    for (uint32_t iBlazeUserIndex = 0; iBlazeUserIndex < VOIP_MAXLOCALUSERS; ++iBlazeUserIndex)
    {
        Blaze::UserManager::LocalUser* localUser = mBlazeHub->getUserManager()->getLocalUser(iBlazeUserIndex);
        if (localUser != nullptr)
        {
            ClientUserMetrics request;
            int32_t iDirtySockIndex = mBlazeHub->getLoginManager(iBlazeUserIndex)->getDirtySockUserIndex();
            VoipSpeechToTextMetricsT STTMetrics = getVoipSpeechToTextMetrics(iDirtySockIndex);
            VoipTextToSpeechMetricsT TTSMetrics = getVoipTextToSpeechMetrics(iDirtySockIndex);

            // if the metrics are blank just carry on, its possible the voip user became inactive and that shouldn't be updated on the server 
            if ((STTMetrics.uEventCount == 0) && 
                (STTMetrics.uErrorCount == 0) &&
                (TTSMetrics.uEventCount == 0) && 
                (TTSMetrics.uErrorCount == 0))
            {
                continue;
            }

            // check to see that something changed, no use sending an update otherwise
            if ((STTMetrics.uEventCount != mUsersAccessabilityState[iDirtySockIndex].PreviousSentSTTMetrics.uEventCount) ||
                (STTMetrics.uErrorCount != mUsersAccessabilityState[iDirtySockIndex].PreviousSentSTTMetrics.uErrorCount) || 
                (TTSMetrics.uEventCount != mUsersAccessabilityState[iDirtySockIndex].PreviousSentTTSMetrics.uEventCount) ||
                (TTSMetrics.uErrorCount != mUsersAccessabilityState[iDirtySockIndex].PreviousSentTTSMetrics.uErrorCount))
            {
                // prepare to send STT the metrics to the blaze server
                request.setSttEventCount(STTMetrics.uEventCount);
                request.setSttDurationMsSent(STTMetrics.uDurationMsSent);
                request.setSttCharCountRecv(STTMetrics.uCharCountRecv);
                request.setSttEmptyResultCount(STTMetrics.uEmptyResultCount);
                request.setSttErrorCount(STTMetrics.uErrorCount);
                request.setSttDelay(STTMetrics.uDelay);

                // prepare to send TTS the metrics to the blaze server
                request.setTtsEventCount(TTSMetrics.uEventCount);
                request.setTtsCharCountSent(TTSMetrics.uCharCountSent);
                request.setTtsDurationMsRecv(TTSMetrics.uDurationMsRecv);
                request.setTtsEmptyResultCount(TTSMetrics.uEmptyResultCount);
                request.setTtsErrorCount(TTSMetrics.uErrorCount);
                request.setTtsDelay(TTSMetrics.uDelay);

                //send the metrics
                getBlazeHub()->getComponentManager(iBlazeUserIndex)->getUtilComponent()->setClientUserMetrics(request, Util::UtilComponent::SetClientUserMetricsCb());

                mUsersAccessabilityState[iDirtySockIndex].PreviousSentSTTMetrics = STTMetrics;
                mUsersAccessabilityState[iDirtySockIndex].PreviousSentTTSMetrics = TTSMetrics;
            }
        }
    }

    // setup the next metric push
    if (iClientUserMetricsRate != 0)
    {
        mClientUserMetricsJobId = getBlazeHub()->getScheduler()->scheduleMethod(this, &UtilAPI::clientUserMetricsJob, this, iClientUserMetricsRate);
    }
}

void UtilAPI::onAuthenticated(uint32_t userIndex)
{
    //now that we have re-authenticated, there will be a new session on the blaze server, attribute only the new metrics to the new session
    VoipRefT *pVoipRefT = VoipGetRef();
    if (pVoipRefT)
    {
        int32_t iDirtySockIndex = mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex();
        VoipControl(pVoipRefT, 'cstm', iDirtySockIndex, NULL);
        ds_memclr(&mUsersAccessabilityState[iDirtySockIndex].PreviousSentSTTMetrics, sizeof(mUsersAccessabilityState[iDirtySockIndex].PreviousSentSTTMetrics));
        VoipControl(pVoipRefT, 'ctsm', iDirtySockIndex, NULL);
        ds_memclr(&mUsersAccessabilityState[iDirtySockIndex].PreviousSentTTSMetrics, sizeof(mUsersAccessabilityState[iDirtySockIndex].PreviousSentTTSMetrics));
    }

    //kick off the job to periodically process the metrics
    if (mClientUserMetricsJobId == INVALID_JOB_ID)
    {
        int32_t clientUserMetricsRate;
        mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_CLIENT_METRICS2_UPDATE_RATE, &clientUserMetricsRate);
        if (clientUserMetricsRate != 0)
        {
            mClientUserMetricsJobId = getBlazeHub()->getScheduler()->scheduleMethod(this, &UtilAPI::clientUserMetricsJob, this, clientUserMetricsRate);
        }
    }
}

void UtilAPI::onDisconnected(BlazeError errorCode)
{
    BLAZE_SDK_DEBUGF("[UtilAPI:%p].onDisconnected\n", this);

    //  cancel all pending jobs (as there may be jobs not linked to any blaze server connection still pending, but we still want to cancel
    //  and jobs related to the util.
    getBlazeHub()->getScheduler()->cancelByAssociatedObject(this, Blaze::SDK_ERR_SERVER_DISCONNECT);
    mClientUserMetricsJobId = INVALID_JOB_ID;
}

JobId UtilAPI::filterUserText(const UserStringList &userStringList, const FilterUserTextCb &titleCb, ProfanityFilterType filterType, uint32_t timeoutMS)
{
    if (userStringList.getTextList().empty())
    {
        BLAZE_SDK_DEBUGF("[UTIL] UtilAPI::filterUserText request is empty.\n");
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("filterUserTextCb", titleCb, ERR_OK, jobId, (const FilterUserTextResponse*)nullptr,
                nullptr, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleCb);
        return jobId;
    }

    bool clientOnly = (filterType == FILTER_CLIENT_ONLY);
    JobScheduler *jobScheduler = getBlazeHub()->getScheduler();

#if defined(EA_PLATFORM_XBOXONE)
    // on Xbox One, schedule the larger task and also kick off an XboxOneValidateString call to run the XBLS string verification
    JobId jobId = jobScheduler->reserveJobId();
    XboxOneValidateString *xboxOneValidateString = BLAZE_NEW(MEM_GROUP_UTIL_TEMP, "XboxOneValidateString") 
            XboxOneValidateString(jobId, mComponent, this, clientOnly, MEM_GROUP_UTIL_TEMP);
    if (!xboxOneValidateString->start(userStringList))
    {
        BLAZE_SDK_DEBUGF("Error: XboxOneValidateString::start failed!\n");
        BLAZE_DELETE(MEM_GROUP_UTIL_TEMP, xboxOneValidateString);

        jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("filterUserTextCb", titleCb, ERR_SYSTEM, jobId, (const FilterUserTextResponse*)nullptr,
                nullptr, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mBlazeHub->getScheduler() , jobId, titleCb);
        return jobId;
    }

    FilterUserTextJobXboxOne *job = BLAZE_NEW(MEM_GROUP_UTIL_TEMP, "FilterUserTextJob") FilterUserTextJobXboxOne(titleCb, this, *xboxOneValidateString);
    return jobScheduler->scheduleJob("FilterUserTextJob", job, this, timeoutMS, jobId);

#else
    // non-XboxOne platforms
    FilterUserTextJob *job = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FilterUserTextJob") FilterUserTextJob(userStringList, titleCb, this, clientOnly);

    // schedule the job
    if (clientOnly)
    {
        // for a non-XOne client-only job, we schedule to dispatch success on the next tick
        //  (return the unfiltered text)
        return jobScheduler->scheduleJob("FilterUserTextJob", job, this);
    }
    else
    {
        // otherwise, we schedule the job to timeout and kick off the filter profanity RPC
        JobId jobId = jobScheduler->scheduleJob("FilterUserTextJob", job, this, timeoutMS);

        // note: rpc reused the response obj for the request (no typedef)
        FilterUserTextResponse request;
        UtilAPI::initFilterUserTextRequest(userStringList, request);
        mComponent->filterForProfanity(request, MakeFunctor(this, &UtilAPI::internalFilterUserTextCb), jobId);        
        return jobId;
    }
#endif
}

void UtilAPI::internalFilterUserTextCb(const FilterUserTextResponse *res, BlazeError err, JobId rpcJobId, JobId jobId)
{
    FilterUserTextJobBase *filterUserTextJob = static_cast<FilterUserTextJobBase*>(getBlazeHub()->getScheduler()->getJob(jobId));
    if (filterUserTextJob != nullptr)
    {
        filterUserTextJob->setExecuting(true);
        filterUserTextJob->dispatchTitleCallback(err, err==ERR_OK?res:nullptr);
        filterUserTextJob->setExecuting(false);
        getBlazeHub()->getScheduler()->removeJob(filterUserTextJob);
    }
    else
    {
        // job was canceled or removed already (title callback already dispatched or swallowed)
    }
}  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

void UtilAPI::initFilterUserTextRequest(const UserStringList &userStringList, FilterUserTextResponse &request)
{
    request.getFilteredTextList().reserve(userStringList.getTextList().size());
    UserStringList::UserTextList::const_iterator i = userStringList.getTextList().begin();
    UserStringList::UserTextList::const_iterator e = userStringList.getTextList().end();
    for (; i!=e; ++i) 
    {
        FilteredUserText *filteredUserText = request.getFilteredTextList().pull_back();
        filteredUserText->setFilteredText((*i)->getText());
        filteredUserText->setResult(FILTER_RESULT_UNPROCESSED);
    }
}

void UtilAPI::onPrimaryLocalUserChanged(uint32_t userIndex)
{
    BLAZE_SDK_DEBUGF("[UtilAPI:%p].onPrimaryLocalUserChanged : User index %u is now primary user\n", this, userIndex);
    mComponent = getBlazeHub()->getComponentManager(userIndex)->getUtilComponent();
    if (EA_UNLIKELY(!mComponent))
    {
        BlazeAssert(false && "UtilAPI - the util component is invalid.");
        return;
    }
}


void UtilAPI::OverrideConfigs(Blaze::BlazeNetworkAdapter::ConnApiAdapterConfig * dest)
{
    bool bAltered = false;
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_ConnApiAdapter_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 1)
            {
                bAltered = true;
                //we use dirtysdk style selectors to identify the settings they're going to modify
                // ConnApi
                if (selector == 'mngl')
                {
                    dest->mEnableDemangler = (blaze_strncmp(token0, "true", sizeof("true")) == 0);
                }
                else if (selector == 'time')
                {
                    dest->mTimeout = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'ctim')
                {
                    dest->mConnectionTimeout = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'ulmt')
                {
                    dest->mUnackLimit = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'meta')
                {
                    dest->mMultiHubTestMode = (blaze_strncmp(token0, "true", sizeof("true")) == 0);
                }
                else if (selector == 'mwid')
                {
                    dest->mPacketSize = EA::StdC::AtoI32(token0);
                }
                //ProtoTunnel
                else if (selector == 'rbuf')
                {
                    dest->mTunnelSocketRecvBuffSize = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'sbuf')
                {
                    dest->mTunnelSocketSendBuffSize = EA::StdC::AtoI32(token0);
                }
                //these settings don't match up directly to a DS Control('') call, but we format it that way anyways
                else if (selector == 'mnvp')
                {
                    dest->mMaxNumVoipPeers = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'mntn')
                {
                    dest->mMaxNumTunnels = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'mvgp')
                {
                    dest->mVirtualGamePort = (uint16_t)EA::StdC::AtoU32(token0);
                }  
                else if (selector == 'mvvp')
                {
                    dest->mVirtualVoipPort = (uint16_t)EA::StdC::AtoU32(token0);
                }
                else if (selector == 'evoi')
                {
                    dest->mEnableVoip = (blaze_strncmp(token0, "true", sizeof("true")) == 0);
                }                 
                else if (selector == 'vpsp')
                {
                    dest->mVoipPlatformSpecificParam = EA::StdC::AtoI32(token0);
                }
                else if (selector == 'lbnd')
                {
                    dest->mLocalBind = (blaze_strncmp(token0, "true", sizeof("true")) == 0);
                }
                else
                {
                    //using NetPrintf for %C handling
                    NetPrintf(("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_CONNAPI_ADAPTER did not recognize selector %C\n", selector));
                }
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_CONNAPI_ADAPTER expected only one token.\n");
            }
        }
    }
    if (bAltered)
    {
        BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs: CONFIGURATION_MODULE_CONNAPI_ADAPTER changed settings, new settings are:\n");
        dest->printSettings();
    }
}

void UtilAPI::OverrideConfigs(ConnApiRefT * dest)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_ConnApi_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }
                ConnApiControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_CONNAPI expected three tokens.\n");
            }
        }
    }
}

void UtilAPI::OverrideConfigs(ProtoTunnelRefT * dest)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_ProtoTunnel_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }
                ProtoTunnelControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_PROTO_TUNNEL expected three tokens.\n");
            }
        }
    }
}

void UtilAPI::OverrideConfigs(VoipTunnelRefT * dest)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_VoipTunnel_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }
                VoipTunnelControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_VOIP_TUNNEL expected three tokens.\n");
            }
        }
    }
}

void UtilAPI::OverrideConfigs(DirtySessionManagerRefT * dest)
{
    #if defined EA_PLATFORM_PS4
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_DirtySessionManager_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }

                // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
                #if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
                DirtySessionManagerControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
                #endif
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_DIRTYSESSION_MANAGER expected three tokens.\n");
            }
        }
    }    
    #endif
}

void UtilAPI::OverrideConfigs(ProtoHttpRefT * dest, const char8_t * instanceName)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_ProtoHttp_", instanceName, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }
                ProtoHttpControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_PROTO_HTTP expected three tokens.\n");
            }
        }
    }
}

void UtilAPI::OverrideConfigs(ProtoSSLRefT * dest)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_ProtoSSL_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 3)
            {
                if ((blaze_strncmp(token2, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token2, "nullptr", sizeof("nullptr")) == 0))
                {
                    token2 = nullptr;
                }
                ProtoSSLControl(dest, selector, EA::StdC::AtoI32(token0), EA::StdC::AtoI32(token1), token2);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_PROTO_SSL expected three tokens.\n");
            }
        }
    }
}

void UtilAPI::OverrideConfigs(QosClientRefT * dest)
{
    const Util::FetchConfigResponse::ConfigMap& configs = getBlazeHub()->getConnectionManager()->getServerConfigs();
    Util::FetchConfigResponse::ConfigMap::const_iterator itr = configs.begin();
    Util::FetchConfigResponse::ConfigMap::const_iterator end = configs.end();
    for ( ; itr != end; itr++)
    {
        int32_t selector;
        uint32_t tokenCount;
        char8_t *token0, *token1, *token2;
        
        //make a copy of the value, since parsing it into tokens will alter it
        char8_t mTempValue[2048];
        blaze_strnzcpy(mTempValue, itr->second.c_str(), sizeof(mTempValue));

        if (parseConfigOverride("Override_QosClient_", nullptr, itr->first.c_str(), mTempValue, &selector, &tokenCount, &token0, &token1, &token2))
        {
            if (tokenCount == 2)
            {
                if ((blaze_strncmp(token1, "NULL", sizeof("NULL")) == 0) || (blaze_strncmp(token1, "nullptr", sizeof("nullptr")) == 0))
                {
                    token1 = nullptr;
                }
                QosClientControl(dest, selector, EA::StdC::AtoI32(token0), token1);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[UtilAPI].OverrideConfigs error: CONFIGURATION_MODULE_QOS_API expected three tokens.\n");
            }
        }
    }
}

bool UtilAPI::parseConfigOverride(const char8_t * moduleTag, const char8_t * instanceName, const char8_t *key, char8_t *value, int32_t *selector, uint32_t *tokenCount, char8_t **token0, char8_t **token1, char8_t **token2)
{
    bool ret = false;
    bool validInstance = false;
    bool vaildClientType = true;
    *selector = 0;
    *tokenCount = 0;
    *token0 = nullptr;
    *token1 = nullptr;
    *token2 = nullptr;
    const char8_t * parseKey = key;

    //an example of a key value pair is:
    //Override_ProtoTunnel_<optional instance name>_rbuf = "64000,0,nullptr"

    //figgure out if this setting belongs to the section of configs we're interested in "Override_ProtoTunnel_"
    uint32_t uModuleTagLength = (uint32_t)strlen(moduleTag);
    if (blaze_strnicmp(parseKey, moduleTag, uModuleTagLength) == 0)
    {
        parseKey = parseKey + uModuleTagLength;

        //the next part of the key is a string to identify differnt instances of a particular module, such as ProtoHttp which are used several times in Blaze.
        if (instanceName != nullptr)
        {
            uint32_t instanceNameLength = (uint32_t)strlen(instanceName);
            if (blaze_strnicmp(parseKey, instanceName, instanceNameLength) == 0)
            {
                parseKey = parseKey + instanceNameLength;

                // if formated properly the remainder of the string should be _wxyz
                if (parseKey[0] == '_')
                {
                    parseKey++; //advance past the unserscore
                    validInstance = true;
                }
                else
                {
                    BLAZE_SDK_DEBUGF("[UtilAPI].parseConfigOverride: expected to find an _ after %s in %s\n", instanceName, key); 
                }
            }        
        }
        else
        {
            validInstance = true;
        }

        //instance is valid we check to see if client type is valid wxyz_clientType
        if (validInstance == true)
        {
            char8_t *selectorKey = strtok(const_cast<char8_t *>(parseKey), "_");
            char8_t *clientTypeKey = nullptr;

            //if the selectorKey has length greater than 4 assume we have a client type and try to read next arugment as the selector key
            if (strlen(selectorKey) > 4)
            {
                clientTypeKey = selectorKey;
                selectorKey = strtok(nullptr, "_");
            }

            if (selectorKey != nullptr)
            {
                //check if client type is specified
                if (clientTypeKey != nullptr)
                {
                    if (blaze_strnicmp(clientTypeKey, "GameplayUser", sizeof("GameplayUser") - 1) == 0)
                    {
                        if (mBlazeHub->getInitParams().Client != CLIENT_TYPE_GAMEPLAY_USER)
                        {
                            vaildClientType = false;
                        }
                    }
                    else if (blaze_strnicmp(clientTypeKey, "HttpUser", sizeof("HttpUser") - 1) == 0)
                    {
                        if (mBlazeHub->getInitParams().Client != CLIENT_TYPE_HTTP_USER)
                        {
                            vaildClientType = false;
                        }
                    }
                    else if (blaze_strnicmp(clientTypeKey, "DedicatedServer", sizeof("DedicatedServer") - 1) == 0)
                    {
                        if (mBlazeHub->getInitParams().Client != CLIENT_TYPE_DEDICATED_SERVER)
                        {
                            vaildClientType = false;
                        }
                    }
                    else if (blaze_strnicmp(clientTypeKey, "Tools", sizeof("Tools") - 1) == 0)
                    {
                        if (mBlazeHub->getInitParams().Client != CLIENT_TYPE_TOOLS)
                        {
                            vaildClientType = false;
                        }
                    }
                    else if (blaze_strnicmp(clientTypeKey, "LimitedGameplayUser", sizeof("LimitedGameplayUser") - 1) == 0)
                    {
                        if (mBlazeHub->getInitParams().Client != CLIENT_TYPE_LIMITED_GAMEPLAY_USER)
                        {
                            vaildClientType = false;
                        }
                    }
                    else
                    {
                        vaildClientType = false;
                        BLAZE_SDK_DEBUGF("[UtilAPI].parseConfigOverride error, override config client type does not match any known types.\n");
                    }
                }

                if (vaildClientType == true)
                {
                    //the control selector will be the next 4 bytes if its done correctly "rbuf'
                    if (strlen(selectorKey) == 4)
                    {
                        //we know we have good input now, so lets log this
                        BLAZE_SDK_DEBUGF("[UtilAPI].parseConfigOverride: %s: %s\n", key, value); 

                        *selector = selectorKey[0] << 24;
                        *selector += selectorKey[1] << 16;
                        *selector += selectorKey[2] << 8;
                        *selector += selectorKey[3];

                        char8_t *saveptr;
                        char8_t *token;
                        int32_t currentToken = 0;
        
                        token = strtok_s(value, ",", &saveptr);
                        while (token != nullptr)
                        {
                            //process the token into a variable based off the token count
                            if (currentToken == 0)
                            {
                                *token0 = token;
                            }
                            else if (currentToken == 1)
                            {
                                *token1 = token;
                            }
                            else if (currentToken == 2)
                            {
                                *token2 = token;
                            }
                            token = strtok_s(nullptr, ",", &saveptr);
                            currentToken++;
                            ret = true;
                        }
                        *tokenCount = currentToken;
                    }
                    else
                    {
                        BLAZE_SDK_DEBUGF("[UtilAPI].parseConfigOverride error, override config selector is not 4 characters long in key.\n"); 
                    }
                }
            }
        }//this setting belongs to a differnt instance of this module
    }//this setting doesn't belong to this module   
    return(ret);
}


} // Util
} // Blaze
