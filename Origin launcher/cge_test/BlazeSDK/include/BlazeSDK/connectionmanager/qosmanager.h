/**************************************************************************************************/
/*! 
    \file qosmanager.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef QOSMANAGERIMPL_H
#define QOSMANAGERIMPL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "BlazeSDK/blaze_eastl/map.h"

#include "BlazeSDK/component/util/tdf/utiltypes.h"
#include "BlazeSDK/component/framework/tdf/userextendeddatatypes.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usermanager.h"


#include "DirtySDK/dirtysock.h"
#include "DirtySDK/misc/qosclient.h"
#include "DirtySDK/dirtysock/netconn.h"

// QoS Flags
#define BLAZESDK_QOSMANAGER_FIREWALL 1
#define BLAZESDK_QOSMANAGER_BANDWIDTH 2

// forward declaration for DS structure
struct ProtoUpnpRefT;

namespace Blaze
{

class BlazeHub;
class IpAddress;

namespace ConnectionManager
{
class BLAZESDK_API QosManager: public UserManager::ExtendedUserDataInfoChangeListener
{
public:
    QosManager(BlazeHub &hub, MemoryGroupId memGroupId);
    virtual ~QosManager();

    typedef Functor QosPingSiteLatencyRequestedCb;
    typedef Functor1<bool> QosPingSiteLatencyRetrievedCb;

    // Helper class to hold the callback:
    class QosRetrievedCbJob : public Job
    {
    public:
        QosRetrievedCbJob(const QosManager::QosPingSiteLatencyRetrievedCb& cb) 
        { 
            mQosRetrievedCb = cb;
            setAssociatedTitleCb(mQosRetrievedCb);
        }
        virtual void execute() {}
        QosPingSiteLatencyRetrievedCb mQosRetrievedCb;
    };

    // Initialize or reinitialize the QosManager.  If nullptr is passed in as the qosConfigInfo, then the existing info will be used.  
    void initialize(const QosConfigInfo *qosConfigInfo, const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb,  bool forceReinitialization = false);

    // QoS methods
    virtual const Util::NetworkQosData* getNetworkQosData() const
    {
        return &mNetworkInfo.getQosData();
    }

    /*! *******************************************************************************************/
    /*! \brief  Sets the Quality of service network data for debugging purposes.
    ***********************************************************************************************/
    void setNetworkQosDataDebug(Util::NetworkQosData& networkData, bool updateMetrics = false);

    virtual const InternetAddress *getExternalAddress() const;
    virtual const NetworkAddress *getNetworkAddress() ;
    virtual const NetworkAddress* getNetworkAddress(uint32_t userIndex);

    /*! ************************************************************************************************/
    /*! \brief ping all qos latency hosts and cache latency for each host

        Note, the first refresh (during logging) should not be triggered until bandwidth qos request id completed

        \param [in] bLatencyOnly - refresh only latency ping

        \return true - if at least one of the service request succeeded (reqid != 0 and a callback will be issued)
                        otherwise false
    ***************************************************************************************************/
    bool refreshQosPingSiteLatency(const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb, bool bLatencyOnly = false);

    /*! ************************************************************************************************/
    /*! \brief get qos ping site latency retrieved via qos latency request

        \return reference of local cached QosPingSiteLatencyVector
    ***************************************************************************************************/
    const Blaze::PingSiteLatencyByAliasMap* getQosPingSitesLatency() { return &mNetworkInfo.getPingSiteLatencyByAliasMap(); };
    

    /*! ************************************************************************************************/
    /*! \brief check if the qos update was successful.

    \return true if the qos information was retrieved with at least 1 valid ping
    ***************************************************************************************************/
    bool wasQosSuccessful() const;

    /*! ************************************************************************************************/
    /*! \brief check if the qos update is active.
    
    \return true if the qos update is active
    ***************************************************************************************************/
    bool isQosRefreshActive() const;

    /*! ************************************************************************************************/
    /*! \brief [DEPRECATED] get qos site list - Only functional with QoS 1.0, use getQosPingSitesLatency to get ping site names with QoS 2.0. 

        \return reference of local ping site info map
    ***************************************************************************************************/
    const Blaze::PingSiteInfoByAliasMap* getQosPingSitesInfo() { return &mQosConfigInfo.getPingSiteInfoByAliasMap(); };
    
    /*! ************************************************************************************************/
    /*! \brief set a user's qos ping site latency info for debugging purposes

        \param [in] PingSiteLatencyByAliasMap - latency map sent to server to refresh ping site latency
        \param [in] cb - callback supply a functor callback to dispatch upon RPC completion (or cancellation/timeout).  
                    Note: The default callback functor makes this a fire and forget RPC.
        \param [in] updateMetrics - controls if the metrics tracked by the Blaze server are updated for the new ping site. 
                    
    ***************************************************************************************************/
    JobId setQosPingSiteLatencyDebug(const PingSiteLatencyByAliasMap& latencyMap, const Blaze::UserSessionsComponent::UpdateNetworkInfoCb& cb = Blaze::UserSessionsComponent::UpdateNetworkInfoCb(), bool updateMetrics = true);

    /*! ************************************************************************************************/
    /*! \brief set the server qos latency update to true, which means user session is AUTHENTICATED
        we can update the UserExtendedData on the server 
    ***************************************************************************************************/
    void triggerLatencyServerUpdate(uint32_t userIndex);

    /*! ************************************************************************************************/
    /*! \brief Idles the QOS process
    ***************************************************************************************************/
    void idle();

    /*! ************************************************************************************************/
    /*! \brief Indicates that the initial PingSite latency retrieval has run.
    ***************************************************************************************************/
    bool initialPingSiteLatencyRetrieved() const;

    /*! ************************************************************************************************/
    /*! \brief Prevent QoS re-initialization. Used internally after a user logs in to prevent 
               QoS initialization from fighting with games for port access. 
    ***************************************************************************************************/
    void disableReinitialization() { mAllowReinitialization = false; }

    /*! ************************************************************************************************/
    /*! \brief Enable QoS re-initialization. Used internally after a call to disableReinitialization() to
               allow QoS re-initialization to occur once all local users log out.
               If re-initialization was required after disableReinitialization() was called, then calling
               enableReinitialization() will cause QoS to be re-initialized immediately with the supplied callback.
    ***************************************************************************************************/
    void enableReinitialization(const QosPingSiteLatencyRequestedCb& reqCb, const QosPingSiteLatencyRetrievedCb& cb);

private:

    virtual const NetworkInfo& getNetworkInfo(uint32_t userIndex);
    virtual void onExtendedUserDataInfoChanged(Blaze::BlazeId blazeId);


    void createQosClient();
    void destroyQosClient();

    void initLocalAddress();
    void attemptUserManagerRegistration();

    bool isAuthenticated();
    bool isAuthenticated(uint32_t userIndex);
    
    static void qosClientCb(QosClientRefT *pQosClient, QosCommonProcessedResultsT *pCBInfo, void *pUserData);

    // qos ping site related helpers
    void finishQosProcess(bool success);
    void updateServerNetworkInfo(uint32_t userIndex, bool natInfoOnly);
    
    // update network info based on upnp result
    void updateNatInfoFromUpnp(Blaze::Util::NatType natType, uint16_t externalPort);

protected:
    BlazeHub &mBlazeHub;
    bool mInitialized;
    QosClientRefT* mQosClient;

    // qos bandwidth service request related
    NetworkInfo mNetworkInfo;
    mutable InternetAddress mExternalAddr; //mutable as this value is only computed as needed upon query

    bool mReadyToUpdateQosInfoOnServer;
    bool mFinishedQosProcess;
    eastl::bitset<NETCONN_MAXLOCALUSERS> mUpdatedQosMetricsOnServer;
    bool mInitialQosInfoRetrieved;
    bool mNeedReinitialization;
    bool mAllowReinitialization;

    // cached config information from util.cfg
    QosConfigInfo mQosConfigInfo;

    // xbox address by ping site alias map
#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    typedef vector<uint64_t> XUIDVector;
    XUIDVector mLocalXUIDVector;
#endif

    UserManager::UserManager* mUserManager;
    JobId mQosRetrievedCbJobId;

    friend class ConnectionManager;
};

} // Connection
} // Blaze

#endif // QOSMANAGERIMPL_H
