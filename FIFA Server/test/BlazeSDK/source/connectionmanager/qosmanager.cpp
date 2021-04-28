/**************************************************************************************************/
/*! 
    \file qosmanager.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/serviceresolver.h"
#include "BlazeSDK/component/framework/tdf/networkaddress.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/loginmanager/loginmanager.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/misc/qosclient.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#define DEFAULT_PING_PERIOD_IN_MS (15000)
#define MIN_PING_PERIOD_IN_MS (1000)

namespace Blaze
{
namespace ConnectionManager
{


QosManager::QosManager(BlazeHub &hub, MemoryGroupId memGroupId)
:   mBlazeHub(hub),
    mInitialized(false),
    mQosClient(nullptr),
    mNetworkInfo(memGroupId),
    mReadyToUpdateQosInfoOnServer(false),
    mFinishedQosProcess(false),
    mInitialQosInfoRetrieved(false),
    mNeedReinitialization(false),
    mAllowReinitialization(true),
    mQosConfigInfo(memGroupId),
    mUserManager(nullptr),
    mQosRetrievedCbJobId(INVALID_JOB_ID)
#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    ,mLocalXUIDVector(memGroupId, mBlazeHub.getNumUsers(), MEM_NAME(memGroupId, "QosManager::mLocalXUIDVector"))
#endif
{
    mNetworkInfo.getQosData().setNatType(Util::NAT_TYPE_NONE);

#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
    IpPairAddress ippair;
    ippair.getInternalAddress().setIp(0);
    ippair.getInternalAddress().setPort(0);
    ippair.getExternalAddress().setIp(0);
    ippair.getExternalAddress().setPort(0);
    mNetworkInfo.getAddress().setIpPairAddress(&ippair);
#endif

    attemptUserManagerRegistration();
}

QosManager::~QosManager()
{
    //if we registered and if the user manager is still there.
    if (mUserManager && mBlazeHub.getUserManager())
    {
        mUserManager->removeListener(this);
    }

    //Cancel our ping scheduled function
    destroyQosClient();
    mBlazeHub.getScheduler()->removeByAssociatedObject(this);
}

void QosManager::attemptUserManagerRegistration()
{
    if (!mUserManager)
    {
        mUserManager = mBlazeHub.getUserManager();
        if (mUserManager)
        {
            mUserManager->addListener(this);
        }
    }
}

void QosManager::createQosClient()
{
    // create qos api if it's not there
    if (mQosClient == nullptr)
    {
        DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_FRAMEWORK));
        
        //determine what local port we are going to bind to
        #if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
            // If mInitialQosInfoRetrieved we won't listen for qos using the original game port as it may now be in use by game sessions. 0 for random port
            uint16_t port = (mInitialQosInfoRetrieved ? 0 : mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().getPort());
        #else
            //let qosclient choose the default
            uint16_t port = 0;  
        #endif

        // create qosclient
        mQosClient = QosClientCreate(&QosManager::qosClientCb, this, port);

        //configure qosclient
        if (mQosClient != nullptr)
        {
            //use config overrides found in util.cfg
            Blaze::Util::UtilAPI::createAPI(mBlazeHub);
            //override configs here after we did initial configuration so that the util.cfg overrides have the final say
            mBlazeHub.getUtilAPI()->OverrideConfigs(mQosClient);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[qosmanager] Error failed to allocate QosClient.\n");
        }
        
        DirtyMemGroupLeave();
    }
}

void QosManager::destroyQosClient()
{
    if (mQosClient != nullptr)
    {
        QosClientDestroy(mQosClient);
        mQosClient = nullptr;
    }
}

void QosManager::enableReinitialization(const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb)
{
    mAllowReinitialization = true;
    if (mNeedReinitialization)
        initialize(nullptr, reqCb, cb, true);
}

void QosManager::initialize(const QosConfigInfo *qosConfigInfo, const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb, bool forceReinitialization /*= false*/)
{
    // save the qos settings, we can query them regardless of if qos is enabled.
    if (qosConfigInfo)
        qosConfigInfo->copyInto(mQosConfigInfo);

    if (forceReinitialization && !mAllowReinitialization)
    {
        mNeedReinitialization = true;
        if (mBlazeHub.getInitParams().EnableQos)
        {
            for (uint32_t userIndex = 0; userIndex < mBlazeHub.getNumUsers(); ++userIndex)
                updateServerNetworkInfo(userIndex, false);
        }
        return;
    }

    mInitialQosInfoRetrieved = false;

    initLocalAddress();

    if (mBlazeHub.getInitParams().EnableQos)
    {
        // The internal call is basically the same as the init cal, only without the InitParam checks...
        if (!refreshQosPingSiteLatency(reqCb, cb))
        {
            // qos api service latency request failed, destroy api and send default latency info to server
            finishQosProcess(false);
        }

        mInitialized = true;
    }
}

// MS has moved to IP Pair addresses for xbox gdk
#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
void QosManager::initLocalAddress()
{
    mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().setIp((uint32_t) NetConnStatus('addr', 0, nullptr, 0));
    #if !defined(EA_PLATFORM_XBOX_GDK)
    mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().setPort(mBlazeHub.getInitParams().GamePort);
    #else // default to microsoft preferred udp multiplayer port
    mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().setPort((uint16_t)NetConnStatus('pump', 0, NULL, 0));
    #endif
    mNetworkInfo.getAddress().getIpPairAddress()->setMachineId(NetConnMachineId());

    BLAZE_SDK_DEBUGF("[qosmanager] Set local internal address to %x:%d\n",
        mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().getIp(), mNetworkInfo.getAddress().getIpPairAddress()->getInternalAddress().getPort());
}
#endif // !EA_PLATFORM_XBOXONE


void QosManager::finishQosProcess(bool success)
{
    attemptUserManagerRegistration();

    // qos process is completed
    mFinishedQosProcess = true;
    // latency is retrieved for the first time. Note that on failure state, then defaults are used (but were still "initialized")
    mInitialQosInfoRetrieved = true;

    // latency qos pings are done, let's update data cached on server
    // AUDIT:MLU - this network information is the same for all local users. (void where xbox)
    // we should consider only sending this for the primary user index, and letting everyone
    // else copy it on the server. Note the same is true in updateNatInfoFromUpnp
    for (uint32_t userIndex = 0; userIndex < mBlazeHub.getNumUsers(); ++userIndex)
        updateServerNetworkInfo(userIndex, false);

    //inform SDK that qos is done
    QosRetrievedCbJob* qosRetrievedCbJob = (QosRetrievedCbJob*)mBlazeHub.getScheduler()->getJob(mQosRetrievedCbJobId);
    if (qosRetrievedCbJob != nullptr && qosRetrievedCbJob->mQosRetrievedCb.isValid())
    {
        qosRetrievedCbJob->mQosRetrievedCb(success);
    }
}

void QosManager::updateServerNetworkInfo(uint32_t userIndex, bool natInfoOnly)
{
    attemptUserManagerRegistration();

    // only ready to update latency and qos data on server when
    // 1) ping site latency is ready 
    // 2) user extended data on the server for logged in user is ready
    // Otherwise, only send network address info to the server
    bool networkAddressOnly = (!mReadyToUpdateQosInfoOnServer || !mFinishedQosProcess);

    Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager(userIndex)->getUserSessionsComponent();
    if (userSessions && isAuthenticated(userIndex))
    {
        // If we didn't find any ping sites at all, (and we're not just updating one of the other pieces of info), that's bad
        if (mNetworkInfo.getPingSiteLatencyByAliasMap().empty() && (!networkAddressOnly && !natInfoOnly))
        {
            // We lie and say we're only setting the network info, so we don't end up in a loop:
            BLAZE_SDK_DEBUGF("[qosmanager] updateServerNetworkInfo - Only setting network address because we finished qos but didn't find any ping sites. (Avoids client/server loop.)");
            networkAddressOnly = true;
        }

        UpdateNetworkInfoRequest request;
        request.setNetworkInfo(mNetworkInfo);
        request.getOpts().setBits(0);
        if (natInfoOnly)
            request.getOpts().setNatInfoOnly();
        if (networkAddressOnly)
        {
            request.getOpts().setNetworkAddressOnly();
        }
        else if (!mUpdatedQosMetricsOnServer[userIndex])
        {
            request.getOpts().setUpdateMetrics();
            mUpdatedQosMetricsOnServer[userIndex] = true;
        }

        userSessions->updateNetworkInfo(request);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[qosmanager] User is unauthenticated: skip sending ping site info to blazeserver.\n");
    }
}

bool QosManager::wasQosSuccessful() const
{
    if (mFinishedQosProcess == false || mNetworkInfo.getPingSiteLatencyByAliasMap().empty())
        return false;

    for (const auto& pingLatency : mNetworkInfo.getPingSiteLatencyByAliasMap())
    {
        // Success is only considered if we got a non-error latency back from the pingsite
        if (pingLatency.second < (int32_t)MAX_QOS_LATENCY && pingLatency.second >= 0)
            return true;
    }

    return false;
}

bool QosManager::isQosRefreshActive() const
{
    return (mFinishedQosProcess == false);
}

void QosManager::triggerLatencyServerUpdate(uint32_t userIndex)
{
    mReadyToUpdateQosInfoOnServer = true;
    updateServerNetworkInfo(userIndex, false);
}

void QosManager::updateNatInfoFromUpnp(Blaze::Util::NatType eNatType, uint16_t uExternalPort)
{
#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
    bool bUpdate = false;

    // only update network info if QoS is enabled
    if (!mBlazeHub.getInitParams().EnableQos)
    {
        return;
    }

    // override the nat type
    if (mNetworkInfo.getQosData().getNatType() != eNatType)
    {
        mNetworkInfo.getQosData().setNatType(eNatType);
        bUpdate = true;
    }

    // override external port derived from QoS with UPnP external port
    if (mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().getPort() != uExternalPort)
    {
        mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().setPort(uExternalPort);
        bUpdate = true;
    }

    // force an update if something changed
    if (bUpdate)
    {
        mReadyToUpdateQosInfoOnServer = true;
        for (uint32_t userIndex = 0; userIndex < mBlazeHub.getNumUsers(); ++userIndex)
            updateServerNetworkInfo(userIndex, true);
    }
#endif
}

void QosManager::idle()
{
    if (mQosClient)
    {
        BLAZE_SDK_SCOPE_TIMER("QosManager_idle");
        QosClientUpdate(mQosClient);
    }
}

bool QosManager::initialPingSiteLatencyRetrieved() const
{
    return (mInitialQosInfoRetrieved || !(mBlazeHub.getInitParams().EnableQos));
}

void QosManager::qosClientCb(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo, void *pUserData)
{
    QosManager *qosManager = (QosManager*) pUserData;
    BLAZE_SDK_DEBUGF("[qosmanager] qos completed.\n");

    if (pQosClient == nullptr || pCBInfo == nullptr || pUserData == nullptr)
    {
        BLAZE_SDK_DEBUGF("[qosmanager] QOS failed because not all required comments are available at time of callback: pQosClient(%p), pCBInfo(%p), pUserData(%p).\n", pQosClient, pCBInfo, pUserData);
        return;
    }

    //There are a few things the user can do when QOS completes:
    //-the most obvious is to use the QosCommonProcessedResultsT to inspect the results
    //-you could get the raw results with the QosClientStatus('rraw') selector if you wanted to see the data the coordinator used to produce the QosCommonProcessedResultsT
    //-you could use either 'rraw' or 'rpro' to get the raw or processed results any time after the callback completes
    //-during the callback you could 'rrpc' to get the rpc data that the QosCommonProcessedResultsT is generated from, this has an advantage in that
    // there may be some data there that this version of client doesn't understand (wont parse into QosCommonProcessedResultsT) but forwarding it to 
    // another service may understand it 

    if (pCBInfo->uNumResults > 0)
    {
        uint32_t uBwUp = 0;
        uint32_t uBwDown = 0;
        //set latency values for each site, and gather best bw metrics
        qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap().clear();  // Clear out any data from prior runs
        for (uint32_t i = 0; i < pCBInfo->uNumResults; i++)
        {
            //see if we already have a latency value for this site
            uint32_t existingValue = UINT32_MAX;
            PingSiteLatencyByAliasMap::const_iterator existingEntry = qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap().find(pCBInfo->aTestResults[i].strSiteName);
            if (existingEntry != qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap().end())
            {
                existingValue = existingEntry->second;
            }

            //we will only have one value per site, so this should really only happen once per site
            //if the new value is better use it
            if (pCBInfo->aTestResults[i].uMinRTT <= existingValue)
            {
                qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap()[pCBInfo->aTestResults[i].strSiteName] = pCBInfo->aTestResults[i].uMinRTT;
            }

            //if we haven't got a valid value substitute in the hResult
            if (qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap()[pCBInfo->aTestResults[i].strSiteName] == -1)
            {
                qosManager->mNetworkInfo.getPingSiteLatencyByAliasMap()[pCBInfo->aTestResults[i].strSiteName] = pCBInfo->aTestResults[i].hResult;
            }

            if (pCBInfo->aTestResults[i].uUpbps > uBwUp)
            {
                uBwUp = pCBInfo->aTestResults[i].uUpbps;
            }

            if (pCBInfo->aTestResults[i].uDownbps > uBwDown)
            {
                uBwDown = pCBInfo->aTestResults[i].uDownbps;
            }
        }

        //set bandwidth values
        qosManager->mNetworkInfo.getQosData().setDownstreamBitsPerSecond(uBwDown);
        qosManager->mNetworkInfo.getQosData().setUpstreamBitsPerSecond(uBwUp);

        //set ip info
#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
        qosManager->mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().setIp(pCBInfo->clientExternalAddress.addr.v4);  //todo, this is will only work of ipv4, blaze should have a setter tha works for ipv6
        qosManager->mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().setPort(pCBInfo->clientExternalAddress.uPort);
#else
        qosManager->mExternalAddr.setIP(pCBInfo->clientExternalAddress.addr.v4);
#endif  

        //set the NAT type
        if (pCBInfo->eFirewallType == QOS_COMMON_FIREWALL_OPEN)
        {
            qosManager->mNetworkInfo.getQosData().setNatType(Util::NAT_TYPE_OPEN);
        }
        else if (pCBInfo->eFirewallType == QOS_COMMON_FIREWALL_MODERATE)
        {
            qosManager->mNetworkInfo.getQosData().setNatType(Util::NAT_TYPE_MODERATE);
        }
        else if (pCBInfo->eFirewallType == QOS_COMMON_FIREWALL_STRICT)
        {
            qosManager->mNetworkInfo.getQosData().setNatType(Util::NAT_TYPE_STRICT);
        }
        else
        {
            qosManager->mNetworkInfo.getQosData().setNatType(Util::NAT_TYPE_UNKNOWN);
        }
        
        //qos is done and successful
        qosManager->finishQosProcess(true);
    }
    else
    {
        //qos is done with a failure
        qosManager->finishQosProcess(false);
    }
}


/*! ************************************************************************************************/
/*! \brief ping all qos latency hosts and cache latency for each host
    
    \param [in] bLatencyOnly - refresh only latency ping

    \return true - if at least one of the service request succeeded (reqid != 0 and a callback will be issued)
                    otherwise false
***************************************************************************************************/
bool QosManager::refreshQosPingSiteLatency(const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb, bool bLatencyOnly)
{
    // Call the callback, now that the client is being created:
    reqCb();

    createQosClient();

    QosRetrievedCbJob *qosRetrievedCbJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "QosRetrievedCbJob") QosRetrievedCbJob(cb);
    mQosRetrievedCbJobId = mBlazeHub.getScheduler()->scheduleJobNoTimeout("QosRetrievedCbJob", qosRetrievedCbJob, this);

    QosClientStart(mQosClient, mQosConfigInfo.getQosCoordinatorInfo().getAddress(), mQosConfigInfo.getQosCoordinatorInfo().getPort(), mQosConfigInfo.getQosCoordinatorInfo().getProfile());

    // reset the flags
    mFinishedQosProcess = false;
    mUpdatedQosMetricsOnServer.reset();

    return true;
}

JobId QosManager::setQosPingSiteLatencyDebug(const PingSiteLatencyByAliasMap& latencyMap, const Blaze::UserSessionsComponent::UpdateNetworkInfoCb& cb, bool updateMetrics)
{
    Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager()->getUserSessionsComponent();
    if (userSessions && isAuthenticated())
    {
        latencyMap.copyInto(mNetworkInfo.getPingSiteLatencyByAliasMap());
        UpdateNetworkInfoRequest request;
        request.setNetworkInfo(mNetworkInfo);
        request.getOpts().clearNetworkAddressOnly();
        request.getOpts().clearNatInfoOnly();
        if (updateMetrics)
        {
            request.getOpts().setUpdateMetrics();
        }
        return userSessions->updateNetworkInfo(request, cb);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[qosmanager] User is unauthenticated: skip sending ping site latency data to blazeserver.\n");
    }

    return INVALID_JOB_ID;
}

/*! *******************************************************************************************/
/*! \brief  Sets the Quality of service network data for debugging purposes.
***********************************************************************************************/
void QosManager::setNetworkQosDataDebug(Util::NetworkQosData& networkData, bool updateMetrics /*= false*/)
{
    Blaze::UserSessionsComponent* userSessions = mBlazeHub.getComponentManager()->getUserSessionsComponent();
    if (userSessions && isAuthenticated())
    {
        UpdateNetworkInfoRequest request;
        request.getOpts().clearNetworkAddressOnly();
        if (updateMetrics)
        {
            request.getOpts().clearNatInfoOnly();
            request.getOpts().setUpdateMetrics();
            networkData.copyInto(mNetworkInfo.getQosData());
        }
        else
        {
            // NAT type is the only qos data field used outside of metrics-gathering
            request.getOpts().setNatInfoOnly();
            request.getOpts().clearUpdateMetrics();
            mNetworkInfo.getQosData().setNatType(networkData.getNatType());
        }
        request.setNetworkInfo(mNetworkInfo);
        userSessions->updateNetworkInfo(request);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[qosmanager] User is unauthenticated: skip sending network update info to blazeserver.\n");
    }
}

const InternetAddress *QosManager::getExternalAddress() const
{
    #if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
    if (mNetworkInfo.getAddress().getIpPairAddress() == nullptr)
    {
        return nullptr;
    }
    mExternalAddr.setIP(mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().getIp());
    mExternalAddr.setPort(mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().getPort());
    #endif
    return &mExternalAddr;
}

const NetworkAddress* QosManager::getNetworkAddress() 
{
    return getNetworkAddress(mBlazeHub.getPrimaryLocalUserIndex());
}

const NetworkAddress* QosManager::getNetworkAddress(uint32_t userIndex)  
{
    return &(getNetworkInfo(userIndex).getAddress());
}

const NetworkInfo& QosManager::getNetworkInfo(uint32_t userIndex)
{
    return mNetworkInfo;
}

void QosManager::onExtendedUserDataInfoChanged(Blaze::BlazeId blazeId)
{    
    UserManager::UserManager* userManager = mBlazeHub.getUserManager();
    if (!userManager)
    {
        return;
    }

    UserManager::LocalUser* localuser = userManager->getLocalUser(0);
    if (!localuser) 
    {
        //early out if localuser is not yet authenticated
        return;
    }

    if (blazeId != localuser->getId())
    {
        //early out if the concerned user is not the local user 
        return;
    }

    const UserManager::User* user = userManager->getUserById(blazeId);
    if (user == nullptr)
    {
        BLAZE_SDK_DEBUGF("[qosmanager].onExtendedUserDataInfoChanged: User(%" PRId64 ") not found\n", blazeId);
        return;
    }

    uint32_t uExternalAddrSeenByServ;
    uint16_t uExternalPortSeenByServ;
    const NetworkAddress& address = user->getExtendedData()->getAddress();
    
    if (address.getActiveMember() == NetworkAddress::MEMBER_IPPAIRADDRESS)
    {
        uExternalAddrSeenByServ = address.getIpPairAddress()->getExternalAddress().getIp();
        uExternalPortSeenByServ = address.getIpPairAddress()->getExternalAddress().getPort();

#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
        if ((mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().getIp() == 0) &&
            (uExternalAddrSeenByServ != 0))
        {
            #if ENABLE_BLAZE_SDK_LOGGING
            char strAddrText[20];
            #endif

            BLAZE_SDK_DEBUGF("[qosmanager] Replacing our 0.0.0.0 external address with BlazeServer-provided %s:%d\n",
                SocketInAddrGetText(uExternalAddrSeenByServ, strAddrText, sizeof(strAddrText)), uExternalPortSeenByServ);
            mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().setIp(uExternalAddrSeenByServ);
            mNetworkInfo.getAddress().getIpPairAddress()->getExternalAddress().setPort(uExternalPortSeenByServ);
        }
#endif
    }

}

// helper function to check if the user is authenticated. We can't report any qos data until we are AUTHENTICATED
bool QosManager::isAuthenticated()
{
    UserManager::UserManager* userManager = mBlazeHub.getUserManager();
    return ((userManager != nullptr) && (userManager->getPrimaryLocalUser() != nullptr));
}

bool QosManager::isAuthenticated(uint32_t userIndex)
{
    UserManager::UserManager* userManager = mBlazeHub.getUserManager();
    return ((userManager != nullptr) && (userManager->getLocalUser(userIndex) != nullptr));
}

} // Connection
} // Blaze
