/*************************************************************************************************/
/*!
\file telemetryapi.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/shared/framework/locales.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/util/telemetryapi.h"
#include "BlazeSDK/component/utilcomponent.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

// DirtySDK includes
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#define BLAZE_TELEMETRY_MAX_PING_SITES (25)

namespace Blaze
{
namespace Telemetry
{

PinHostConfigT TelemetryAPI::mPinConfig;

void TelemetryAPI::createAPI(BlazeHub& hub, const TelemetryApiParams& params, EA::Allocator::ICoreAllocator* allocator/* = nullptr*/)
{
    if (hub.getTelemetryAPI(0) != nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }

    if (Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_TELEMETRY, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());

    APIPtrVector *apiVector = BLAZE_NEW(MEM_GROUP_TELEMETRY, "TelemetryAPI")
        APIPtrVector(MEM_GROUP_TELEMETRY, hub.getNumUsers(), MEM_NAME(MEM_GROUP_TELEMETRY, "TelemetryAPIArray")); 
    // create a separate instance for each userIndex
    for(uint32_t i=0; i < hub.getNumUsers(); i++)
    {
        TelemetryAPI *api = BLAZE_NEW(MEM_GROUP_TELEMETRY, "TelemetryAPI") 
            TelemetryAPI(hub, params, i);
        (*apiVector)[i] = api; // Note: the Blaze framework will delete the API(s)

        // block telemetry until user options can be verified
        TelemetryApiControl(api->getTelemetryApiRefT(), 'unbl', 0, nullptr);
    }

    hub.createAPI(TELEMETRY_API, apiVector);

}

TelemetryAPI::TelemetryAPI(BlazeHub& blazeHub, const TelemetryApiParams& params, uint32_t userIndex)
    : MultiAPI(blazeHub, userIndex),
    mBlazeHub(blazeHub),
    mCreateRefParams(params),
    mTelemetryRef(nullptr),
#if TELEMETRY_VERSION_MAJOR >= 15
    mPinEntity(nullptr),
#endif    
    mEnableDisconnectTelemetry(false),
    mCancelIdleJobId(INVALID_JOB_ID)
{
    mUserManager = blazeHub.getUserManager();
    BlazeAssert(mUserManager != nullptr);
    mUtilComponent = blazeHub.getComponentManager()->getUtilComponent();
    BlazeAssert(mUtilComponent != nullptr);

    create();
}

TelemetryAPI::~TelemetryAPI()
{
    destroy();
}

void TelemetryAPI::logStartupParameters() const
{
}

void TelemetryAPI::create()
{
    if (mTelemetryRef != nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].create() - ref already exists\n", this);
        return;
    }

    DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY));
#ifdef USE_BLAZE_TELEMERY_SERVER
    if(mCreateRefParams.mIsVersion3) {
        if(mCreateRefParams.mBuffer3Size > 0) {
            mTelemetryRef = TelemetryApiCreateEx(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType), mCreateRefParams.mBuffer3Size, Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY));
        } else{
            mTelemetryRef = TelemetryApiCreate3(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType), Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY));
        }
    } else{
        mTelemetryRef = TelemetryApiCreate(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType), Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY));
    }
#else
    if(mCreateRefParams.mIsVersion3) {
        if(mCreateRefParams.mBuffer3Size > 0) {
            mTelemetryRef = TelemetryApiCreateEx(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType), mCreateRefParams.mBuffer3Size);
        } else{
            mTelemetryRef = TelemetryApiCreate3(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType));
        }
    } else{
        mTelemetryRef = TelemetryApiCreate(mCreateRefParams.mNumEvents, static_cast<TelemetryApiBufferTypeE>(mCreateRefParams.mBufferType));
    }
#endif

    if (mTelemetryRef == nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].create() - error creating ref\n", this);
    }
#if TELEMETRY_VERSION_MAJOR >= 15
    else
    {
        mPinEntity = BLAZE_NEW(MEM_GROUP_TELEMETRY, "Telemetry::PinPlayerEntity") PinPlayerEntity(Blaze::Allocator::getAllocator(MEM_GROUP_TELEMETRY));
        if (mPinEntity == nullptr)
        {
            BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].create() - error creating PIN ref\n", this);
        }
    
        TelemetryApiSetPinInterface(mTelemetryRef, mPinEntity);
    }
#endif    

    DirtyMemGroupLeave();
}

void TelemetryAPI::configure()
{
    if (mTelemetryRef == nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].configure() - ref does not exist\n", this);
        return;
    }

    const Util::GetTelemetryServerResponse *response = getBlazeHub()->getLoginManager(getUserIndex())->getTelemetryServer();

    if (response->getKey() != nullptr && strlen(response->getKey()) > 0)
    {
        onGetTelemetryServer(response, Blaze::ERR_OK, Blaze::INVALID_JOB_ID);
        return;
    }

    Util::GetTelemetryServerRequest request;
    mUtilComponent->getTelemetryServer(
        request,
        Util::UtilComponent::GetTelemetryServerCb(this, &TelemetryAPI::onGetTelemetryServer));
}

void TelemetryAPI::destroy()
{
    cancelIdle(true);

    if (mTelemetryRef != nullptr)
    {
        TelemetryApiDestroy(mTelemetryRef); 
        mTelemetryRef = nullptr;
    }
#if TELEMETRY_VERSION_MAJOR >= 15
    if (mPinEntity != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_TELEMETRY, mPinEntity);
        mPinEntity = nullptr;
    }
#endif
}

bool TelemetryAPI::connect(TelemetryApiRefT *pRef)
{
    if (pRef == nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI].connect() - not initialized\n");
        return false;
    }

    if(TelemetryApiConnect(pRef) == 0)
        return false;

    return true;
}

void TelemetryAPI::disconnect(TelemetryApiRefT *pRef)
{
    if (pRef == nullptr)
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI].disconnect() - not initialized, nothing disconnected\n");
        return;
    }

    TelemetryApiDisconnect(pRef);
}


void TelemetryAPI::onAuthenticated(uint32_t userIndex)
{
    if (userIndex == getUserIndex())
    {
        // we just authenticated
        configure();

        // if the user is opted in unblock telemetry
        for(uint32_t i=0; i < getBlazeHub()->getNumUsers(); i++)
        {
            UserManager::LocalUser *pUser = getBlazeHub()->getUserManager()->getLocalUser(i);
            if (pUser && pUser->getUserOptions()->getTelemetryOpt() == Blaze::Util::TELEMETRY_OPT_IN)
            {
                TelemetryApiControl(mTelemetryRef, 'unbl', 1, nullptr);
            }
            else if (pUser)
            {
                BLAZE_SDK_DEBUGF("[TelemetryAPI] Info: Local User %u has opted out, no telemetry will be sent\n", i);
            }
        }
    }
}

void TelemetryAPI::onDeAuthenticated(uint32_t userIndex)
{
    if (userIndex == getUserIndex())
    {
        // Wait until telemetry has finished sending everything (up to 15 seconds)
        mBlazeHub.addIdler(this, "TelemetryApi");
        mCancelIdleJobId = mBlazeHub.getScheduler()->scheduleMethod("cancelIdle", this, &TelemetryAPI::cancelIdle, true, nullptr, 15 * 1000);
    }
}

void TelemetryAPI::onGetTelemetryServer(const Util::GetTelemetryServerResponse* response, BlazeError error, JobId id)
{
    if (response && error == Blaze::ERR_OK)
    {
        mEnableDisconnectTelemetry = response->getEnableDisconnectTelemetry();
    }
    
    //Call helper function
    if(!TelemetryAPI::initAPI(mTelemetryRef,response,error,mUserManager->getLocalUser(getUserIndex())))
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].onGetTelemetryServer() - error initializing Telemetry API\n", this);
    }
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void TelemetryAPI::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    // Flush the send buffer
    TelemetryApiControl(mTelemetryRef, 'flsh', 0, nullptr);

    // Check if Telemetry has finished
    if ((TelemetryApiStatus(mTelemetryRef, 'done', nullptr, 0) == 1))
    {
        BLAZE_SDK_SCOPE_TIMER("TelemetryAPI_cancleIdle");

        cancelIdle(false);
    }
}

void TelemetryAPI::cancelIdle(bool timedout)
{
    if (timedout) {
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].cancelIdle() - Timed out waiting for Telemetry to finish sending data. Disconnecting.\n", this);
    }else{
        BLAZE_SDK_DEBUGF("[TelemetryAPI:%p].idle() - Telemetry done sending data. Disconnecting.\n", this);
    }

    if (mCancelIdleJobId != INVALID_JOB_ID)
    {
        mBlazeHub.getScheduler()->cancelJob(mCancelIdleJobId);
        mCancelIdleJobId = INVALID_JOB_ID;
    }

    doFinishIdle();
}

void TelemetryAPI::doFinishIdle()
{
    mBlazeHub.removeIdler(this);

    // block telemetry
    TelemetryApiControl(mTelemetryRef, 'unbl', 0, nullptr);

    // we just lost authentication
    disconnect(mTelemetryRef);
}

bool TelemetryAPI::initAPI(TelemetryApiRefT *pRef, 
                           const Util::GetTelemetryServerResponse* response, 
                           BlazeError error, 
                           Blaze::UserManager::LocalUser *pUser)
{
    char strKind[8];
    char strHostName[256];
    int32_t bSecured;
    uint8_t bPortSpecified;

    if (error != Blaze::ERR_OK)
    {
        // TODO: Print error name as well.
        BLAZE_SDK_DEBUGF("[TelemetryAPI].initAPI() - no telemetry server available (error code 0x%x)\n", error);
        return false;
    }

    // now that we've got the config from the server, setup the DirtySDK's TelemetryAPI

    // set the ip/port/auth string
    if (response->getKey() != nullptr && strlen(response->getKey()) > 0)
    {
        uint32_t locale = response->getLocale();

        if (locale == Blaze::LOCALE_BLANK)
        {
            BLAZE_SDK_DEBUGF("[TelemetryAPI].initAPI() - locale was not provided in response. Using %c%c%c%c", LocaleTokenPrintCharArray(Blaze::LOCALE_UNKNOWN));
            locale = Blaze::LOCALE_UNKNOWN;
        }

        char8_t auth[4096];
        blaze_snzprintf(auth, sizeof(auth), "%s,%d,%c%c%c%c,%s",
            response->getAddress(),
            response->getPort(),
            LocaleTokenPrintCharArray(locale),
            response->getKey());
        
        ProtoHttpUrlParse2(response->getPinUrl(), strKind, sizeof(strKind), strHostName, sizeof(strHostName), &mPinConfig.iPinServerPort, &bSecured, &bPortSpecified);
        ds_snzprintf(mPinConfig.strPinServerAddress, sizeof(mPinConfig.strPinServerAddress), "%s://%s", strKind, strHostName);
        blaze_strnzcpy(mPinConfig.strEnv, response->getPinEnvironment(), sizeof(mPinConfig.strEnv));

        // set pin telemetry config
        IPin *pPin = (IPin*)TelemetryApiGetPinInterface(pRef);
        pPin->ConfigurePinEndpoint(&mPinConfig);

        TelemetryApiControl(pRef, 'uage', response->getUnderage() ? 1 : 0, nullptr);
        TelemetryApiControl(pRef, 'tcsn', 0, (void*)response->getTelemetryServiceName());
        TelemetryApiSetSessionID(pRef, response->getSessionID());
        TelemetryApiAuthent(pRef, auth);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI].initAPI() - auth key missing\n");
        return false;
    }

    // set the timeout
    TelemetryApiControl(pRef, 'time', static_cast<int32_t>(response->getSendDelay()), nullptr);

    // set the threshold percentage
    TelemetryApiControl(pRef, 'thrs', static_cast<int32_t>(response->getSendPercentage()), nullptr);

    // set the list of disabled countries
    char8_t temp[4096];
    blaze_strnzcpy(temp, response->getDisable(), sizeof(temp));
    TelemetryApiControl(pRef, 'cdbl', 0, temp);

    // set any event filter rules
    TelemetryApiFilter(pRef, response->getFilter());

    TelemetryApiControl(pRef, 'stio', 0, (void*)response->getUseServerTime());

    if (TelemetryAPI::connect(pRef))
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI].initAPI() - connected\n");
    }
    else
    {
        BLAZE_SDK_DEBUGF("[TelemetryAPI].initAPI() - unable to connect\n");
        return false;
    }

    return true;
}

void TelemetryAPI::queueBlazeSDKTelemetry(TelemetryApiEvent3T& event)
{
    if (mEnableDisconnectTelemetry && (mTelemetryRef != nullptr))
    {
        //add general sdk information to the event
        
        //time since the last idle
        BlazeHub* blazeHub = getBlazeHub();
        uint32_t idleInterval = blazeHub->getCurrentTime() - blazeHub->getLastIdleTime();
        int32_t retVal = TelemetryApiEncAttributeInt(&event, 'idle', (int32_t)idleInterval);

        //time since the last ping response was received on the client
        uint32_t pingInterval = blazeHub->getCurrentTime() - blazeHub->getConnectionManager()->getLastClientPingTime();
        retVal += TelemetryApiEncAttributeInt(&event, 'ping', (int32_t)pingInterval);

        // the last time we received something from the blaze server
        uint32_t recvInterval = blazeHub->getCurrentTime() - blazeHub->getComponentManager()->getDefaultSender().getLastReceiveTime();
        retVal += TelemetryApiEncAttributeInt(&event, 'recv', (int32_t)recvInterval);

        //qos data - this will be null if qos has not completed yet.
        const Blaze::Util::NetworkQosData* qosData = blazeHub->getConnectionManager()->getNetworkQosData();
        if (qosData != nullptr)
        {
            retVal += TelemetryApiEncAttributeString(&event, 'natt', NatTypeToString(qosData->getNatType()));
            retVal += TelemetryApiEncAttributeInt(&event, 'ubps', (int32_t)qosData->getUpstreamBitsPerSecond());
            retVal += TelemetryApiEncAttributeInt(&event, 'dbps', (int32_t)qosData->getDownstreamBitsPerSecond());
        }

        const PingSiteLatencyByAliasMap* pingLatencyMap = blazeHub->getConnectionManager()->getQosPingSitesLatency();
        int32_t aPingLatencyArray[BLAZE_TELEMETRY_MAX_PING_SITES]; // max 25 ping sites
        uint32_t uPingArraySize = (uint32_t)pingLatencyMap->size();
        memset(aPingLatencyArray, 0, sizeof(aPingLatencyArray));
        if (pingLatencyMap != nullptr)
        {
            PingSiteLatencyByAliasMap::const_iterator pingItr = pingLatencyMap->begin();
            PingSiteLatencyByAliasMap::const_iterator pingEnd = pingLatencyMap->end();

            for (int i = 0; (pingItr != pingEnd) && (i < BLAZE_TELEMETRY_MAX_PING_SITES); ++pingItr, ++i)
            {
                aPingLatencyArray[i] = pingItr->second;
            }
        }

        if (retVal == TC_OKAY)
        {
            retVal = TelemetryApiSubmitEvent3Ex(mTelemetryRef, &event, uPingArraySize, 
                'png1', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[0],
                'png2', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[1],
                'png3', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[2],
                'png4', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[3],
                'png5', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[4],
                'png6', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[5],
                'png7', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[6],
                'png8', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[7],
                'png9', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[8],
                'pngA', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[9],
                'pngB', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[10],
                'pngC', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[11],
                'pngD', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[12],
                'pngE', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[13],
                'pngF', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[14],
                'pngG', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[15],
                'pngH', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[16],
                'pngI', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[17],
                'pngJ', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[18],
                'pngK', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[19],
                'pngL', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[20],
                'pngM', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[21],
                'pngN', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[22],
                'pngO', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[23], 
                'pngP', TELEMETRY3_ATTRIBUTE_TYPE_INT, aPingLatencyArray[24]);
            if (retVal < TC_OKAY)
            {
                BLAZE_SDK_DEBUGF("[TelemetryAPI].queueBlazeSDKTelemetry() - unable to submit BlazeSDK telemetry. Error: %i.\n", retVal);
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[TelemetryAPI].queueBlazeSDKTelemetry() - unable to generate BlazeSDK telemetry. Error: %i.\n", retVal);
        }
    }
}


}// Telemetry

}// Blaze
