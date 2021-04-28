/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PEERINFO_H
#define BLAZE_PEERINFO_H

/*** Include files *******************************************************************************/
#include <EATDF/time.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector_set.h>

#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/tdf/notifications_server.h"
#include "framework/connection/inetaddress.h"
#include "framework/grpc/grpcutils.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
typedef eastl::vector_set<UserSessionId> UserSessionIdVectorSet;

class ClientInfo;
class PeerInfoListener;
struct Notification;
class InboundRpcConnection;

// This class represents information about the peer that is making an RPC request. A peer may be another server instance in the Blaze cluster or
// one of the various clients of Blaze server. If a peer is a Blaze client, as it stands now, has information about all the users on that peer (think MLU). 
// This class separates connection layer from rest of the blaze. This class should get iteratively slimmer as we move
// towards gRPC conversion from our legacy tech. But as it stands now, it has some back door entries in order to not break our existing tech.
class PeerInfo
{
public:
    static const uint32_t MAX_USER_INDICES = 16;
    
    PeerInfo();
    virtual ~PeerInfo();

    uint32_t getLocale() const { return mLocale; }
    void setLocale(uint32_t locale) { mLocale = locale; }

    uint32_t getBaseLocale() const { return mBaseLocale; }
    void setBaseLocale(uint32_t locale) { mBaseLocale = locale; }

    uint32_t getCountry() const { return mCountry; }
    void setCountry(uint32_t country) { mCountry = country; }

    uint32_t getBaseCountry() const { return mBaseCountry; }
    void setBaseCountry(uint32_t country) { mBaseCountry = country; }

    ClientType getClientType() const { return mClientType; }
    virtual void setClientType(ClientType clientType) { mClientType = clientType; }

    const char8_t* getServiceName() const { return mServiceName; }
    void setServiceName(const char8_t* serviceName);

    const char8_t* getEndpointName() const { return mEndpointName.c_str(); }
    void setEndpointName(const char8_t* endpointName);

    ClientInfo* getClientInfo() { return mClientInfo; }
    const ClientInfo* getClientInfo() const { return mClientInfo; }
    void setClientInfo(const ClientInfo *clientInfo);
    
    // Set whether this peer is allowed to disable inactivity period if it so desires. 
    void setIgnoreInactivityTimeoutEnabled(bool enabled) { mIgnoreInactivityTimeoutEnabled = enabled; }
    void setIgnoreInactivityTimeout(bool ignoreInactivityTimeout)
    {
        if (mIgnoreInactivityTimeoutEnabled)
            mIgnoreInactivityTimeout = ignoreInactivityTimeout;
    }
    bool getIgnoreInactivityTimeout() const { return mIgnoreInactivityTimeout; }

    void setCommandTimeout(EA::TDF::TimeValue timout) { mCommandTimeout = timout; }
    EA::TDF::TimeValue getCommandTimeout() const { return mCommandTimeout; }

    // A server is an entity that accesses the endpoint configured for CONNECTION_TYPE_SERVER (currently, this is limited to another member of the cluster).
    void setPeerAsServer() { mIsServer = true; }
    bool isServer() const { return mIsServer; }
    bool isClient() const { return !isServer(); }

    UserSessionId getUserSessionIdAtUserIndex(int32_t userIndex) const;
    void setUserSessionIdAtUserIndex(int32_t userIndex, UserSessionId userSessionId);
    
    UserSessionId getUserSessionId() const;
    void setUserSessionId(UserSessionId userSessionId);

    int32_t getUserIndexFromUserSessionId(UserSessionId sessionId) const;

    // Returning UserSessionIdVectorSet by value is neither inefficient nor unsafe. The compiler would be able to avoid all the copy ctors (named return value optimization).
    // If the caller (by mistake) tries to keep a reference, the compiler will fail in case of a non-const reference. In case of a const reference, the lifetime of the temporary
    // will be extended - https://herbsutter.com/2008/01/01/gotw-88-a-candidate-for-the-most-important-const/ . 
    UserSessionIdVectorSet getUserSessionIds() const;
    uint32_t getAttachedUserSessionsCount() const { return mAttachedUserSessionsCount; }

    ConnectionGroupId getConnectionGroupId() const { return mConnectionGroupId; }
    void setConnectionGroupId(ConnectionGroupId connectionGroupId) { mConnectionGroupId = connectionGroupId; }

    void getSessionKey(char8_t(&keyBuffer)[MAX_SESSION_KEY_LENGTH], uint32_t userIndex, UserSessionId userSessionId);
    UserSessionId getUserSessionId(const char8_t* sessionKey, uint32_t userIndex);

    void setListener(PeerInfoListener& listener) { mListener = &listener; }
    void removeListener() { mListener = nullptr; }
    
    const char8_t* toString(char8_t* buf, uint32_t len) const;
    const char8_t* toString() const;

    void setPeerAddress(const InetAddress& peerAddr) { mPeerAddress = mRealPeerAddress = peerAddr; }
    void setRealPeerAddress(const InetAddress& peerAddr) { mRealPeerAddress = peerAddr; }

    //+
    // Following is a collection of methods that should become simplified or removed as we rework our current tech
    // and move closer to gRPC.
    virtual const InetAddress& getPeerAddress() const { return mPeerAddress; }
    virtual const InetAddress& getRealPeerAddress() const { return mRealPeerAddress; }
    virtual bool isRealPeerAddressResolved() const { return mRealPeerAddress.isResolved(); }

    void setTrustedClient(bool trustedClient) { mTrustedClient = trustedClient; }
    bool getTrustedClient() const { return mTrustedClient; }

    virtual bool isPersistent() const { return true; }
    virtual bool canAttachToUserSession() const { return isPersistent(); }
    
    // Handle connectivity checking on the peer. 
    virtual void enableConnectivityChecks(EA::TDF::TimeValue interval, EA::TDF::TimeValue timeout);
    virtual void disableConnectivityChecks();
    virtual bool isResponsive() const;
    virtual BlazeRpcError checkConnectivity(const EA::TDF::TimeValue& absoluteTimeout);
    
    virtual void closePeerConnection(UserSessionDisconnectType closeReason, bool immediate);
    
    // Notifications. In gRPC, these would be implemented via streaming rpcs. Currently, only FIRE/EventXML connections support notifications.
    // The Http users get their notifications via NotificationCache.
    bool supportsNotifications() const { return mSupportsNotifications; }
    void setSupportsNotifications(bool supportsNotifications) { mSupportsNotifications = supportsNotifications; }
    virtual bool isBlockingNotifications(bool notificationPriorityLow) const;
    virtual bool hasNotificationHandler(EA::TDF::ComponentId componentId) const
    {
        if (supportsNotifications())
            return true;
        
        return false;
    }
    virtual void sendNotification(Notification& notification);
    //-
            
private:
    uint32_t mLocale;
    uint32_t mBaseLocale;
    uint32_t mCountry;
    uint32_t mBaseCountry;
    ClientType mClientType;
    char8_t mServiceName[64];
    eastl::string mEndpointName;
    
    bool mIgnoreInactivityTimeoutEnabled;
    bool mIgnoreInactivityTimeout;
    
    EA::TDF::TimeValue mCommandTimeout;
    ClientInfoPtr mClientInfo;

    struct UserIndexEntry
    {
        UserIndexEntry();
        UserSessionId userSessionId;
        bool includeSessionKey;
    };
    typedef eastl::vector<UserIndexEntry> UserIndexEntryList;
    UserIndexEntryList mUserIndexEntryList;
    uint32_t mAttachedUserSessionsCount;
    ConnectionGroupId mConnectionGroupId;

    PeerInfoListener* mListener;
    
    bool mSupportsNotifications;
    
    InetAddress mPeerAddress;
    InetAddress mRealPeerAddress;

    bool mTrustedClient;

    bool mIsServer;

    UserSessionId mUserSessionId; //ugly but to be used with non-persistent connections
    mutable FixedString64 mToString; // Useful string for logging info about this peer 
};

namespace Grpc
{
    class InboundGrpcRequestHandler;
}

class GrpcPeerInfo : public PeerInfo
{
public:
    GrpcPeerInfo(Grpc::InboundGrpcRequestHandler& requestHandler);
    ~GrpcPeerInfo();
    
    bool isPersistent() const override;
    bool canAttachToUserSession() const override;
    void setClientType(ClientType clientType) override;

    // Client Connectivity detection support
    void enableConnectivityChecks(EA::TDF::TimeValue interval, EA::TDF::TimeValue timeout) override;
    void disableConnectivityChecks() override;
    bool isResponsive() const override;
    BlazeRpcError checkConnectivity(const EA::TDF::TimeValue& absoluteTimeout) override;
    void closePeerConnection(UserSessionDisconnectType closeReason, bool immediate) override;

    bool hasNotificationHandler(EA::TDF::ComponentId componentId) const override;
    void sendNotification(Notification& notification) override;

private:
    Grpc::InboundGrpcRequestHandler& mRequestHandler;
};

class Connection;
class LegacyPeerInfo : public PeerInfo
{
public:
    LegacyPeerInfo(InboundRpcConnection& connection);
    ~LegacyPeerInfo() override;

    const InetAddress& getPeerAddress() const override;
    const InetAddress& getRealPeerAddress() const override;
    bool isRealPeerAddressResolved() const override;
    bool isPersistent() const override;
    void sendNotification(Notification& notification) override;
    bool isBlockingNotifications(bool notificationPriorityLow) const override;
    void enableConnectivityChecks(EA::TDF::TimeValue interval, EA::TDF::TimeValue timeout) override;
    void disableConnectivityChecks() override;
    bool isResponsive() const override;
    BlazeRpcError checkConnectivity(const EA::TDF::TimeValue& absoluteTimeout) override;
    void closePeerConnection(UserSessionDisconnectType closeReason, bool immediate) override;

    // Nobody should be calling the deprecated method below. 
    InboundRpcConnection& getDeprecatedLegacyConnection() const { return mConnection; }
private:
    InboundRpcConnection& mConnection;
};

class PeerInfoListener
{
public:
    virtual void onAllUsersRemoved() = 0;
    virtual ~PeerInfoListener() { }
};


} // Blaze

#endif // BLAZE_PEERINFO_H

