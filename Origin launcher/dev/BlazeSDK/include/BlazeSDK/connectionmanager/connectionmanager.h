/**************************************************************************************************/
/*! 
    \file connectionmanager.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazestateprovider.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/fire2connection.h"
#include "BlazeSDK/internetaddress.h"

#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#include "BlazeSDK/component/util/tdf/utiltypes.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "BlazeSDK/component/framework/tdf/userextendeddatatypes.h"
#include "BlazeSDK/connectionmanager/qosmanager.h"

namespace Blaze
{

// Forward declarations
class BlazeHub;

namespace Util
{
    class UtilComponent;
}

namespace ConnectionManager
{

class ConnectionManagerListener;

// Constants
// TODO: get locale constants from a separate header
const size_t LOCALE_MAX_LENGTH = 16; //!< All locale strings must fit into a 16 char buffer.


/*! ****************************************************************************/
/*! \struct ConnectionStatus

    \brief Contains attributes detailing the last connection state before the last
        state change.    
    
    On error this structure contains information that can better describe conditions that 
        might have lead to the connection failure.

    Extended Errors (in addition to the standard connection errors documented.)
        SDK_ERR_NETWORK_CONN_TIMEOUT
        SDK_ERR_NETWORK_CONN_FAILED
        SDK_ERR_NETWORK_DISCONNECTED
        SDK_ERR_LSP_LOOKUP,
        SDK_ERR_RESOLVER_TIMEOUT,
        SDK_ERR_BLAZE_CONN_TIMEOUT,
        SDK_ERR_BLAZE_CONN_FAILED
********************************************************************************/
struct BLAZESDK_API ConnectionStatus
{
    BlazeError mError;                      //!< Extended error code for the connection error.
    TimeValue mConnectTime;                 //!< Time relative to start of connection process through connection.
    int32_t mNetConnStatus;                 //!< Reported NetConnStatus code on state change (or last error)
    int32_t mProtoSslError;                 //!< Reported ProtoSSLStatus - see Dirtysock documentation and error codes.
    int32_t mSocketError;                   //!< Reported socket error - see Dirtysock SOCKERR error codes. 
    int32_t mNumReconnections;              //!< Number of reconnections attempted.

    ConnectionStatus();
};


/*! ***********************************************************************************************/
/*! \class ConnectionManager
    \ingroup _mod_initialization

    \brief Class used to manage connections to Blaze and console networks.
***************************************************************************************************/
class BLAZESDK_API ConnectionManager : public Idler
{
public:
    typedef Functor2<BlazeError, const Redirector::DisplayMessageList*> ConnectionMessagesCb;

    // Helper class to hold the callback:
    class ConnectionCbJob : public Job
    {
    public:
        ConnectionCbJob(const ConnectionManager::ConnectionMessagesCb& cb) 
        { 
            mConnectCb = cb;
            setAssociatedTitleCb(mConnectCb);
        }
        virtual void execute() {}
        ConnectionMessagesCb mConnectCb;
    };

    /*! *******************************************************************************************/
    /*! \brief Address type for connection.
    ***********************************************************************************************/
    enum AddressType
    {
        ADDRESS_TYPE_INTERNET ///< Standard IP address or host name.
    };   

    static ConnectionManager* create(BlazeHub &hub);
    
    /*! *******************************************************************************************/
    /*! \name Connection methods
    */
    /*! *******************************************************************************************/
    /*! \brief Initiates connection to the Blaze server, and to any necessary console network.
               Listen to the BlazeHub state-change notifications for the result. If
               the connection is successful, the hub will transition to the CONNECTED state.
               Otherwise, it will drop to a lower state and provide an error code.
               By default the parameter isSilent is false, in this configuration 1st party HUDs will
               be displayed to aid the user in signing in an account with an appropriate network 
               configuration.  To disable the 1st party huds pass in true.  The connection
               messages callback will be called in both success and failure conditions if there are
               any critical display messages that need displayed to the user.  For example, if the
               given server is down, one or more messages will be returned from the redirector
               indicating the reason and they should be displayed to the end user.  Execution of
               this callback does not indicate that the connection process is complete yet.

               \return returns true, if connection attempt was made - false if not (possibly due to a 
                       duplicate call to connect).
    ***********************************************************************************************/
    bool connect(const ConnectionMessagesCb& connectCb, uint8_t isSilent=false);
    
    /*! *******************************************************************************************/
    /*! \brief Closes connection to Blaze server, and to any active console network. Listen to the
               BlazeHub state-change notifications for the result. If the disconnection is
               successful, the hub will transition to the NETWORK_INITIALIZED state. Otherwise, it will drop
               to a lower state and provide an error code.

        \return bool - true if the disconnect was handled (and a state will be dispatched), 
                       false if the connection was not connecting or connected. 
    ***********************************************************************************************/
    virtual bool disconnect(BlazeError error = ERR_OK);
    virtual void dirtyDisconnect() { mBlazeConnection.dirtyDisconnect(); }

    /*! *******************************************************************************************/
    /*! \brief Returns the component manager.
    
        \param userIndex Local user index to fetch the component manager for.
        \return The component manager or nullptr if not found..
    ***********************************************************************************************/
    virtual ComponentManager* getComponentManager(uint32_t userIndex) const  { return mBlazeConnection.getComponentManager(userIndex); }

    /*! *******************************************************************************************/
    /*! \brief Returns the server address of the Blaze server

        \return a string representation of the server address, it could be in the form 
                of a host name or an IP address. The address is only valid while connected to the
                blazeserver. Returns an empty string if not connected. 
    ***********************************************************************************************/
    virtual const char8_t* getServerAddress() const; 

    /*! *******************************************************************************************/
    /*! \name Time methods
     */
    /*! *******************************************************************************************/
    /*! \brief Returns the current time from the Blaze server, in UTC time zone.
        
        \return The current UTC time.
    ***********************************************************************************************/
    virtual uint32_t getServerTimeUtc() const;
    
    /*! *******************************************************************************************/
    /*! \name Config methods
     */

    /*! *******************************************************************************************/
    /*! \brief return the whole collection of server configs
    
        \return collection of configs
    ***********************************************************************************************/
    virtual const Util::FetchConfigResponse::ConfigMap& getServerConfigs() const;

    /*! *******************************************************************************************/
    /*! \brief Given a key, return the associated server config string.
    
        \return true if the item was found; false otherwise
    ***********************************************************************************************/
    virtual bool getServerConfigString(const char8_t* key, const char8_t** value) const;

    /*! \brief Given a key, return the associated server config integer.
    
        \return true if the item was found; false otherwise
    ***********************************************************************************************/
    virtual bool getServerConfigInt(const char8_t* key, int32_t* value) const;
    

    /*! \brief Given a key, return the associated server config time interval.
    
        \return true if the item was found; false otherwise
    ***********************************************************************************************/
    virtual bool getServerConfigTimeValue(const char8_t* key, TimeValue& value) const;


    // Idler interface
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);


    /*! *******************************************************************************************/
    /*! \brief Returns a pointer to this connection manager's NetworkQosData object. This data will
        be valid after the listener onQosPingSiteLatencyRetrieved() is dispatched.
        \return a pointer to this connections NetworkQosData object.
    ***********************************************************************************************/
    virtual const Util::NetworkQosData *getNetworkQosData() const { return mQosManager.getNetworkQosData(); }

    /*! *******************************************************************************************/
    /*! \brief  Sets the Quality of service network data for debugging purposes. This overrides the QOS info blaze
            automatically sends to the blazeServer as part of the login process, and optionally triggers an update
            of the QOS metrics tracked by the blazeserver.

            Typically used to test matchmaking or gameBrowsing rules using 'fake' firewall, bandwidth, and network location
            data.  (When testing from within EA, everyone's listed as behind a strict firewall and typically
            has similar bandwidth & pings to the various EA data centers).

            See ConnectionManager::getNetworkQosData().

            This triggers a "fire & forget" transaction, refreshing this player's QOS info on the blazeServer.
    ***********************************************************************************************/
    virtual void setNetworkQosDataDebug(Util::NetworkQosData& networkData, bool updateMetrics = false) { mQosManager.setNetworkQosDataDebug(networkData, updateMetrics); }

    /*! *******************************************************************************************/
    /*! \brief Returns the external address of this connection or nullptr if no external address could
            be determined
        \return the external address of this connection or nullptr if no external address could
            be determined
    ***********************************************************************************************/
    virtual const InternetAddress *getExternalAddress() const { return mQosManager.getExternalAddress(); } 

    /*! *******************************************************************************************/
    /*! \brief Returns the full network address of this connection for the primary user or nullptr if no address could
    be determined
    \return the address of this connection or nullptr if no address could
    be determined
    ***********************************************************************************************/
    virtual const NetworkAddress *getNetworkAddress() { return mQosManager.getNetworkAddress(); }

    /*! *******************************************************************************************/
    /*! \brief Returns the full network address of this connection or nullptr if no address could
    be determined
    \param userIndex the specific user index to get network address for.
    \return the address of this connection or nullptr if no address could
    be determined
    ***********************************************************************************************/
    virtual const NetworkAddress *getNetworkAddress(uint32_t userIndex) { return mQosManager.getNetworkAddress(userIndex); }

    /*! ************************************************************************************************/
    /*! \brief ping all qos latency hosts and cache latency for each host

        Note, the first refresh (during logging) should not be triggered until bandwidth qos request id is
        completed, onQosPingSiteLatencyRetrieved() is dispatched when the refresh is completed.

        \param [in] bLatencyOnly - refresh only latency ping

        \return true - if at least one of the service request succeeded (reqid != 0 and a callback will be issued)
            otherwise false
    ***************************************************************************************************/
    virtual bool refreshQosPingSiteLatency(bool bLatencyOnly = false);

    /*! ************************************************************************************************/
    /*! \brief get qos ping site latency retrieved via qos latency request through qos api

        \return reference of local cached PingSiteLatencyByAliasMap
    ***************************************************************************************************/
    virtual const Blaze::PingSiteLatencyByAliasMap* getQosPingSitesLatency() { return mQosManager.getQosPingSitesLatency(); }

    /*! ************************************************************************************************/
    /*! \brief check if the qos update was successful.

    \return true if the qos information was retrieved with at least 1 valid ping
    ***************************************************************************************************/
    virtual bool wasQosSuccessful() const { return mQosManager.wasQosSuccessful(); }

    /*! ************************************************************************************************/
    /*! \brief check if the qos update is active.
    
    \return true if the qos update is active
    ***************************************************************************************************/
    virtual bool isQosRefreshActive() const { return mQosManager.isQosRefreshActive(); }
    
    /*! ************************************************************************************************/
    /*! \brief [DEPRECATED] get qos site list - Use getQosPingSitesLatency instead.

    \return reference of local ping site info map
    ***************************************************************************************************/
    virtual const Blaze::PingSiteInfoByAliasMap* getQosPingSitesInfo() { return mQosManager.getQosPingSitesInfo(); }

    /*! ************************************************************************************************/
    /*! \brief set a user's qos ping site latency info for debugging purposes

        \param [in] latencyMap - latency map sent to server to refresh ping site latency
        \param [in] cb - callback supply a functor callback to dispatch upon RPC completion (or cancellation/timeout).  
                    Note: The default callback functor makes this a fire and forget RPC.
    ***************************************************************************************************/
    virtual JobId setQosPingSiteLatencyDebug(const PingSiteLatencyByAliasMap& latencyMap, 
        const Blaze::UserSessionsComponent::UpdateNetworkInfoCb& cb = Blaze::UserSessionsComponent::UpdateNetworkInfoCb(), bool updateMetrics = true) { return mQosManager.setQosPingSiteLatencyDebug(latencyMap, cb, updateMetrics); }

    /*! ************************************************************************************************/
    /*! \brief set the server qos latency update to true, which means user session is AUTHENTICATED
        we can update the UserExtendedData on the server 
    ***************************************************************************************************/
    virtual void triggerLatencyServerUpdate(uint32_t userIndex) { mQosManager.triggerLatencyServerUpdate(userIndex); }

    /*! ************************************************************************************************/
    /*! \brief reset the upnp discover flag when user logs out in case the connection still exists
    ***************************************************************************************************/
    virtual void resetRetriveUpnpFlag();

    /*! ************************************************************************************************/
    /*! \brief return the server version
    ***************************************************************************************************/
    virtual const char8_t* getServerVersion() const { return mServerVersion; }

    /*! ************************************************************************************************/
    /*! \brief return the server platform
    ***************************************************************************************************/
    virtual const char8_t* getPlatform() const { return mPlatform; }

    /*! ************************************************************************************************/
    /*! \brief return the server name
    ***************************************************************************************************/
    virtual const char8_t* getServiceName() const { return mServiceName; }

    /*! ************************************************************************************************/
    /*! \brief return the server clientId (only available after connecting to Blazeserver)
    ***************************************************************************************************/
    virtual const char8_t* getClientId() const { return mClientId; }

    /*! ************************************************************************************************/
    /*! \brief return the server release type (only available after connecting to Blazeserver) Used by PIN login events. 
    ***************************************************************************************************/
    virtual const char8_t* getReleaseType() const { return mReleaseType; }

    /*! ************************************************************************************************/
    /*! \brief return the blaze server namespace
    ***************************************************************************************************/
    virtual const char8_t* getPersonaNamespace() const { return mPersonaNamespace; }
    /*! ************************************************************************************************/
    /*! \brief Indicates that the initial PingSite latency retrieval has run.
        If QOS is not enabled we return true since default values will prevail
    ***************************************************************************************************/
    virtual bool initialQosPingSiteLatencyRetrieved() const;

    /*! ************************************************************************************************/
    /*! \brief return the connection status since the last state change (error status on failure.)
    ***************************************************************************************************/
    const ConnectionStatus& getConnectionStatus() const { return mConnectionStatus; }

    /*! ************************************************************************************************/
    /*! \brief return the list of configured server components since the last preAuth
    ***************************************************************************************************/
    const Util::ComponentIdList& getComponentIds() const { return mComponentIds; }

    /*! ************************************************************************************************/
    /*! \brief return the default or override value of client platform type
    ***************************************************************************************************/
    ClientPlatformType getClientPlatformType() const;

    /*! ************************************************************************************************/
    /*! \brief set the client platform type
    ***************************************************************************************************/
    void setClientPlatformTypeOverride(ClientPlatformType clientPlatformType) { mClientPlatformTypeOverride = clientPlatformType; }

    /*! ****************************************************************************/
    /*! \name ConnectionManagerListener methods
    ********************************************************************************/

    /*! ****************************************************************************/
    /*! \brief Adds a listener to receive async notifications from the ConnectionManager.

    Once an object adds itself as a listener, ConnectionManager will dispatch
    async events to the object via the methods in ConnectionManagerListener.

    \param listener The listener to add.
    ********************************************************************************/
    void addListener(ConnectionManagerListener *listener) { mDispatcher.addDispatchee( listener ); }

    /*! ****************************************************************************/
    /*! \brief Removes a listener from the ConnectionManager, preventing any further async
    notifications from being dispatched to the listener.

    \param listener The listener to remove.
    ********************************************************************************/
    void removeListener(ConnectionManagerListener *listener) { mDispatcher.removeDispatchee( listener); }

    /*! ************************************************************************************************/
    /*! \brief Adds a connection manager state listener for receiving notifications about
        connects/disconnects
    
        \param[in] listener - The listener to add
    ***************************************************************************************************/
    void addStateListener(ConnectionManagerStateListener *listener) { mStateDispatcher.addDispatchee( listener ); }

    /*! ************************************************************************************************/
    /*! \brief Removes a connection manager state listener from receiving notifications about
        connects/disconnects
    
        \param[in] listener - The listener to remove.
    ***************************************************************************************************/
    void removeStateListener(ConnectionManagerStateListener *listener) { mStateDispatcher.removeDispatchee( listener); }

    /*! *******************************************************************************************/
    /*! \name Destructor
    ***********************************************************************************************/
    virtual ~ConnectionManager();

    /*! ************************************************************************************************/
    /*! \brief returns true if the BlazeHub is currently connected to the Blaze server.
    ***************************************************************************************************/
    bool isConnected() const { return mConnected; }

    /*! ************************************************************************************************/
    /*! \brief returns true if the BlazeHub is currently attempting to re-establish a lost connection
               to the Blaze server.
    ***************************************************************************************************/
    bool isReconnecting() const { return mIsReconnectingInProgress; }

    /*! ************************************************************************************************/
    /*! \brief generates the connection parameters that are passed to NetConnConnect

    This was added at the request of OSDK due to how they initialize their network.

    \param [out] buff output buffer containing the connection params
    \param [in] size size of the output buffer
    \param [in] isSilentConnect flag indicating if the connection is silent
    ***************************************************************************************************/
    void generateConnectionParams(char8_t* buff, uint32_t size, uint8_t isSilentConnect = true) const;

    /*! ************************************************************************************************/
    /*! \brief returns the ios Unique Device Id as reported by NetConnStatus()
    ***************************************************************************************************/
    const char8_t* getUniqueDeviceId() const;

    /*! ************************************************************************************************/
    /*! \brief returns the last time that a ping response was received from the server in milliseconds
    ***************************************************************************************************/
    uint32_t getLastClientPingTime() const { return mLastPingClientTimeMS; };

    /*! ************************************************************************************************/
    /*! \brief Get or set whether the auto reconnect feature is enabled or not.
    ***************************************************************************************************/
    bool getAutoReconnectEnabled() const { return mAutoReconnectEnabled; };
    void setAutoReconnectEnabled(bool autoReconnectEnabled) { mAutoReconnectEnabled = autoReconnectEnabled; }

    /*! ************************************************************************************************/
    /*! \brief Get or set the interval between each reconnection attempt.
    ***************************************************************************************************/
    uint32_t getAutoReconnectIntervalMs() const { return mBlazeConnection.getReconnectTimeout(); };
    void setAutoReconnectIntervalMs(uint32_t autoReconnectIntervalMs) { mBlazeConnection.setReconnectTimeout(autoReconnectIntervalMs); }

    /*! ************************************************************************************************/
    /*! \brief Internal debugging and unit testing only! Can't be used by Integrators! 
    ***************************************************************************************************/
    bool setServerConnInfo(const char8_t *serviceName, const char8_t *address, uint16_t port, bool secure) { return mBlazeConnection.setServerConnInfo(serviceName, address, port, secure); };

    /*! ************************************************************************************************/
    /*! \brief Attempt to refresh the QoS manager.  This involves talking to the Coordinator and performing QoS again.
               Checks the current state of QoS and gameplay (Games/Matchmaking) before attempting the QoS refresh.
        returns false if the title is not in a state where it can reinitialize (either QoS is still active, or a Game/Matchmaking session is in progress).
    ***************************************************************************************************/
    bool refreshQosManager();

    /*! ************************************************************************************************/
    /*! \brief Internal use only - methods to handle qos settings updates from the server.
               resetQosManager will reinitialize the qos manager if qos reinitialization is enabled, forcing BlazeSDK to
               refresh all ping site latencies. If reinitialization is disabled, resetQosManager will trigger the qos manager 
               to erase any removed ping sites and insert default latencies (interpreted by the blazeserver as max possible latencies)
               for any added ping sites; but ping site latencies will not be refreshed until reinitialization is re-enabled.
    ***************************************************************************************************/
    void resetQosManager(const QosConfigInfo& qosConfigInfo);
    void disableQosReinitialization() { mQosManager.disableReinitialization(); }
    void enableQosReinitialization();
    
protected:

    /*! *******************************************************************************************/
    /*! \name Constructor
    ***********************************************************************************************/
    ConnectionManager(BlazeHub &hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);


protected:
    // BlazeConnection callbacks
    void onBlazeConnect(BlazeError result, int32_t sslError, int32_t sockError);
    void onBlazeDisconnect(BlazeError result, int32_t sslError, int32_t sockError);
    void onBlazeConnectionTimeout(BlazeError result);

    void onBlazeReconnectBegin();
    void onBlazeReconnectEnd();

    void sendPreAuth();
    void onPreAuth(const Util::PreAuthResponse* response, BlazeError error, JobId id);

    void setConnectionStatus(BlazeError error = ERR_OK, BlazeError extendedError = ERR_OK);

    void sendUtilPing();
    void onUtilPingResponse(const Util::PingResponse* response, BlazeError error, JobId id);

    void doDisconnect(BlazeError error, BlazeError extendedError);

    void dispatchDisconnect(BlazeError error);

    void retrieveUpnpStatus();

    void internalQosPingSiteLatencyRequestedCb();
    void internalQosPingSiteLatencyRetrievedCb(bool success);

    void getValidVersionList(const char8_t* versionStr, uint32_t* version, uint32_t length) const;
    bool isOlderBlazeServerVersion(const char8_t* blazeSdkVersion, const char8_t* serverVersion) const;

    void netConnectionStatusToBlazeError(int32_t status, bool connected, BlazeError& error, BlazeError& extendedError) const;

    // Return true when connecting, connected, or reconnecting:
    bool canDisconnect() const;

    void connectComplete();

protected:
    BlazeHub &mBlazeHub;
    Fire2Connection mBlazeConnection;

    ProtoUpnpRefT*  mProtoUpnp;

    uint32_t mTimeSpentWaitingForConnection;

    static const uint32_t MAX_TIME_TO_WAIT_FOR_CONNECTION = Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS;
    uint8_t mSilentConnect;
    JobId mConnectionCbJobId;
    JobId mUpnpJobId;

    uint32_t mLastPingServerTimeS;
    uint32_t mLastPingClientTimeMS;

    Blaze::UserManager::UserManager* mUserManager;

    bool mUpnpInfoDiscovered;
    bool mConnectionAttempted;
    bool mIsWaitingForNetConn;      // Tracks NetConn '+onl' status.  Set when connect is first called, and cleared when '+onl' is returned (or onDisconnect is called)
    bool mConnecting;               // Set when connect is first called, persists until connected = true (or onDisconnect is called)
    bool mConnected;                // Set true when the first ping is received from Blaze. 
    bool mIsReconnectingInProgress;
    bool mAutoReconnectEnabled;

    // cached config information from util.cfg
    Util::FetchConfigResponse mClientConfig;
    ConnectionStatus mConnectionStatus;
    Util::ComponentIdList mComponentIds;

    char8_t  mServerVersion[512];
    char8_t  mPlatform[512];
    char8_t  mServiceName[512];
    char8_t  mClientId[512];
    char8_t  mReleaseType[512];
    char8_t  mPersonaNamespace[32];

    ClientPlatformType mClientPlatformTypeOverride;

    typedef Dispatcher<ConnectionManagerListener> ConnectionManagerDispatcher;
    ConnectionManagerDispatcher mDispatcher;

    typedef Dispatcher<ConnectionManagerStateListener> ConnectionmanagerStateDispatcher;
    ConnectionmanagerStateDispatcher mStateDispatcher;

    QosManager mQosManager;
    EA::TDF::TimeValue mTimeOfLastQosRefresh;
    EA::TDF::TimeValue mTimeOfLastQosRefreshFailure;
};

/*! ****************************************************************************/
/*! \class ConnectionManagerListener

\brief classes that wish to receive async notifications about ConnectionManager events implement
this interface and add them self as a listener.

Listeners must add themselves to each ConnectionManager that they wish to get notifications about.
See ConnectionManager::addListener.
********************************************************************************/
class ConnectionManagerListener
{
public:

    /*! ************************************************************************************************/
    /*! \brief Async notification that the Blaze has started a refresh of  QOS ping site latency information.

    This is triggered automatically as part of the initial login flow, and after a configurable amount of time (3h default)
    when the player is not in a Game, Group, or Matchmaking session. 

    ***************************************************************************************************/
    virtual void onQosPingSiteLatencyRequested() {}

    /*! ************************************************************************************************/
    /*! \brief Async notification that the initial retrieval of QOS ping site latency information has been
    retrieved.

    This is only triggered when the ping site info is retrieved successfully.

    \param [in] success Indicates if all QoS requests were successful
    ***************************************************************************************************/    
    virtual void onQosPingSiteLatencyRetrieved(bool success) = 0;

    /*! ************************************************************************************************/
    /*! \brief Async notification when a client loses connection with Blaze, triggering a reconnection.

    Clients can bring up UI or otherwise handle special cases when temporarily losing connection with the
    blaze server.
    ***************************************************************************************************/    
    virtual void onReconnectionStart() {}

    /*! ************************************************************************************************/
    /*! \brief Async notification when a reconnection finishes, allowing callers to release any UI or
            logic initiated with an onReconnectionStart().

    Note that the time interval between onReconnectionStart() and onReconnectionEnd() may be less than a second
    depending on latency and other factors.  If reconnection fails, then Blaze will trigger both this notification
    and the usual disconnect state handler.
    ***************************************************************************************************/    
    virtual void onReconnectionFinish(bool success) {}

    /*! ****************************************************************************/
    /*! \name Destructor
    ********************************************************************************/
    virtual ~ConnectionManagerListener() {};

}; // class ConnectionManagerListener



} // Connection
} // Blaze

#endif // CONNECTIONMANAGER_H
