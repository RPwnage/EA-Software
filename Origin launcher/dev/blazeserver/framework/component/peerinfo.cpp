
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/component/peerinfo.h"
#include "framework/connection/connection.h"
#include "framework/util/locales.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/grpc/inboundgrpcjobhandlers.h" 
#include "framework/grpc/inboundgrpcrequesthandler.h"

namespace Blaze
{

PeerInfo::UserIndexEntry::UserIndexEntry() 
    : userSessionId(INVALID_USER_SESSION_ID)
    , includeSessionKey(false) 
{
}

PeerInfo::PeerInfo()
    : mLocale(LOCALE_DEFAULT)
    , mBaseLocale(LOCALE_DEFAULT)
    , mCountry((uint32_t)COUNTRY_DEFAULT)
    , mBaseCountry((uint32_t)COUNTRY_DEFAULT)
    , mClientType(CLIENT_TYPE_INVALID)
    , mIgnoreInactivityTimeoutEnabled(false)
    , mIgnoreInactivityTimeout(false)
    , mCommandTimeout(0)
    , mAttachedUserSessionsCount(0)
    , mConnectionGroupId(INVALID_CONNECTION_GROUP_ID)
    , mListener(nullptr)
    , mSupportsNotifications(true)
    , mTrustedClient(false)
    , mIsServer(false)
    , mUserSessionId(INVALID_USER_SESSION_ID)
{
    blaze_strnzcpy(mServiceName, gController->getDefaultServiceName(), sizeof(mServiceName));
}

PeerInfo::~PeerInfo()
{
}

void PeerInfo::setServiceName(const char8_t* serviceName)
{
    if (serviceName[0] != '\0')
        blaze_strnzcpy(mServiceName, serviceName, sizeof(mServiceName));
    else
    {
        EA_FAIL_MSG("Invalid service name!");
    }
}

void PeerInfo::setEndpointName(const char8_t* endpointName)
{
    mEndpointName = endpointName;
}

void PeerInfo::setClientInfo(const ClientInfo *clientInfo)
{
    if (!mClientInfo)
        mClientInfo = clientInfo->clone();
    else
        clientInfo->copyInto(*mClientInfo);
}

void PeerInfo::getSessionKey(char8_t(&keyBuffer)[MAX_SESSION_KEY_LENGTH], uint32_t userIndex, UserSessionId userSessionId)
{
    keyBuffer[0] = '\0';
    if ((userIndex < mUserIndexEntryList.size()) && mUserIndexEntryList[userIndex].includeSessionKey)
    {
        if (userSessionId == 0)
            userSessionId = mUserIndexEntryList[userIndex].userSessionId;

        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            userSession->getKey(keyBuffer);
        }

        // This signals that the session key should be included in the response.  Fire2 uses this
        // to decide if it needs to include the session key in the Fire2 metadata.  Note, the session key
        // can be an empty string, indicating that the user was logged out, and the client should reset
        // it's copy of the session key.
        mUserIndexEntryList[userIndex].includeSessionKey = false;
    }
}

UserSessionId PeerInfo::getUserSessionId(const char8_t* sessionKey, uint32_t userIndex)
{
    // If a sessionKey was provided, then it (rather than the userIndex) should be used to find the session.
    // The sessionKey is provided by the client in case it is connecting to a new core slave or it is using non-persistent connection. 
    if ((sessionKey != nullptr) && (*sessionKey != '\0'))
    {
        return gUserSessionManager->getUserSessionIdFromSessionKey(sessionKey);
    }
    
    return getUserSessionIdAtUserIndex(userIndex);
}

void PeerInfo::setUserSessionIdAtUserIndex(int32_t userIndex, UserSessionId userSessionId)
{
    if ((userIndex < 0) || (userIndex >= (int32_t)MAX_USER_INDICES))
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "PeerInfo.setUserSessionIdAtUserIndex: The provided userIndex(" << userIndex << ") is not >=0 and <=" << MAX_USER_INDICES << ". This should never happen.");
        return;
    }

    if (isPersistent())
    {
        if (userIndex >= (int32_t)mUserIndexEntryList.size())
            mUserIndexEntryList.resize(userIndex + 1);

        EA_ASSERT(userSessionId == INVALID_USER_SESSION_ID || mUserIndexEntryList[userIndex].userSessionId == INVALID_USER_SESSION_ID);

        mUserIndexEntryList[userIndex].userSessionId = userSessionId;
        mUserIndexEntryList[userIndex].includeSessionKey = true;

        mAttachedUserSessionsCount += (userSessionId != INVALID_USER_SESSION_ID ? 1 : -1);

        EA_ASSERT(mAttachedUserSessionsCount <= MAX_USER_INDICES);

        if (mAttachedUserSessionsCount == 0)
        {
            setConnectionGroupId(INVALID_CONNECTION_GROUP_ID);
            if (mListener != nullptr)
            {
                mListener->onAllUsersRemoved();
            }
        }
    }
}

UserSessionId PeerInfo::getUserSessionIdAtUserIndex(int32_t userIndex) const
{
    if (isPersistent())
    {
        if ((userIndex >= 0) && (userIndex < (int32_t)mUserIndexEntryList.size()))
            return mUserIndexEntryList[userIndex].userSessionId;
    }

    return INVALID_USER_SESSION_ID;
}

int32_t PeerInfo::getUserIndexFromUserSessionId(UserSessionId sessionId) const
{
    if (isPersistent())
    {
        for (size_t index = 0; index < mUserIndexEntryList.size(); ++index)
        {
            if (mUserIndexEntryList[index].userSessionId == sessionId)
                return (int32_t)index;
        }
    }

    return -1;
}

UserSessionIdVectorSet PeerInfo::getUserSessionIds() const
{
    UserSessionIdVectorSet userSessionIds;
    
    for (UserIndexEntryList::const_iterator it = mUserIndexEntryList.begin(), end = mUserIndexEntryList.end(); it != end; ++it)
    {
        if (UserSession::isValidUserSessionId(it->userSessionId))
            userSessionIds.insert(it->userSessionId);
    }

    return userSessionIds;
}

void PeerInfo::setUserSessionId(UserSessionId userSessionId)
{
    EA_ASSERT(!isPersistent());
    mUserSessionId = userSessionId;
}

UserSessionId PeerInfo::getUserSessionId() const
{
    EA_ASSERT(!isPersistent());
    return mUserSessionId;
}

void PeerInfo::sendNotification(Notification& /*notification*/)
{
    EA_ASSERT_MSG(false, "sendNotification: This needs a concrete override.");
}

bool PeerInfo::isBlockingNotifications(bool /*notificationPriorityLow*/) const
{
    return false;
}

void PeerInfo::enableConnectivityChecks(TimeValue /*interval*/, TimeValue /*timeout*/)
{
    EA_ASSERT_MSG(false, "enableConnectivityChecks: This needs a concrete override.");
}

void PeerInfo::disableConnectivityChecks()
{
    EA_ASSERT_MSG(false, "disableConnectivityChecks: This needs a concrete override.");
}

bool PeerInfo::isResponsive() const
{
    EA_ASSERT_MSG(false, "isResponsive: This needs a concrete override.");
    return true;
}

BlazeRpcError PeerInfo::checkConnectivity(const TimeValue& absoluteTimeout)
{
    EA_ASSERT_MSG(false, "checkConnectivity: This needs a concrete override.");
    return Blaze::ERR_OK;
}


void PeerInfo::closePeerConnection(UserSessionDisconnectType /*closeReason*/, bool /*immediate*/)
{
    EA_ASSERT_MSG(false, "closePeerConnection: This needs a concrete override.");
}

const char8_t* PeerInfo::toString(char8_t* buf, uint32_t len) const
{
    getPeerAddress().toString(buf, len, InetAddress::IP_PORT);
    return buf;
}

const char8_t* PeerInfo::toString() const
{
    mToString.sprintf("[IP(%s), ClientType(%s)]", getPeerAddress().getIpAsString(), Blaze::ClientTypeToString(getClientType()));
    return mToString.c_str();
}

GrpcPeerInfo::GrpcPeerInfo(Grpc::InboundGrpcRequestHandler& requestHandler)
    : mRequestHandler(requestHandler)
{

}


GrpcPeerInfo::~GrpcPeerInfo()
{
   
}

void GrpcPeerInfo::setClientType(ClientType clientType)
{
   PeerInfo::setClientType(clientType);
}

bool GrpcPeerInfo::isPersistent() const
{
    return false;
}

bool GrpcPeerInfo::canAttachToUserSession() const
{ 
    return true; 
}

void GrpcPeerInfo::enableConnectivityChecks(TimeValue interval, TimeValue timeout)
{
    BLAZE_WARN_LOG(Log::GRPC, "GrpcPeerInfo::enableConnectivityChecks - Unfinished business until Blaze module for the EADP SDK is ready");
}

void GrpcPeerInfo::disableConnectivityChecks()
{
    BLAZE_WARN_LOG(Log::GRPC, "GrpcPeerInfo::disableConnectivityChecks - Unfinished business until Blaze module for the EADP SDK is ready");
}

bool GrpcPeerInfo::isResponsive() const
{
    BLAZE_WARN_LOG(Log::GRPC, "GrpcPeerInfo::isResponsive - Unfinished business until Blaze module for the EADP SDK is ready");
    return true;
}

BlazeRpcError GrpcPeerInfo::checkConnectivity(const TimeValue& absoluteTimeout)
{
    BLAZE_WARN_LOG(Log::GRPC, "GrpcPeerInfo::checkConnectivity - Unfinished business until Blaze module for the EADP SDK is ready");
    return Blaze::ERR_OK;
}

void GrpcPeerInfo::closePeerConnection(UserSessionDisconnectType closeReason, bool immediate)
{

}

void GrpcPeerInfo::sendNotification(Notification& notification)
{
    if (isClient())
    {
        mRequestHandler.sendNotification(notification, false);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::GRPC, "GrpcPeerInfo::sendNotification called for a peer which is not a client.");
    }
}

bool GrpcPeerInfo::hasNotificationHandler(ComponentId componentId) const
{
    if (isClient())
    {
        return mRequestHandler.hasNotificationHandler(componentId);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::GRPC, "GrpcPeerInfo::hasNotificationHandler called for a peer which is not a client.");
    }

    return false;
}


LegacyPeerInfo::LegacyPeerInfo(InboundRpcConnection& connection)
    : PeerInfo()
    , mConnection(connection)
{

}

LegacyPeerInfo::~LegacyPeerInfo()
{

}

const InetAddress& LegacyPeerInfo::getPeerAddress() const
{
    return mConnection.getPeerAddress();
}

const InetAddress& LegacyPeerInfo::getRealPeerAddress() const
{
    return mConnection.getRealPeerAddress();
}
bool LegacyPeerInfo::isRealPeerAddressResolved() const
{
    return mConnection.isRealPeerAddressResolved();
}

bool LegacyPeerInfo::isPersistent() const 
{ 
    return mConnection.isPersistent(); 
}

void LegacyPeerInfo::sendNotification(Notification& notification)
{
    if (isClient())
    {
        static_cast<ClientInboundRpcConnection&>(mConnection).sendNotification(notification);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::sendNotification called for a peer which is not a client.");
    }
}

bool LegacyPeerInfo::isBlockingNotifications(bool notificationPriorityLow) const
{
    if (isClient())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        if (notificationPriorityLow)
        {
            if (!connection.isClosed())
                return connection.isAboveLowWatermark();
        }
        else
        {
            if (!connection.isClosed())
                return connection.isSquelching();
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::isBlockingNotifications called for a peer which is not a client.");
    }

    return false;
}

void LegacyPeerInfo::enableConnectivityChecks(TimeValue interval, TimeValue timeout)
{
    if (isClient())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        connection.setConnectivityCheckSettings(interval, timeout);
        connection.enableConnectivityChecking();
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::enableConnectivityChecks called for a peer which is not a client.");
    }
}

void LegacyPeerInfo::disableConnectivityChecks()
{
    if (isClient())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        connection.disableConnectivityChecking();
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::disableConnectivityChecks called for a peer which is not a client.");
    }
}

bool LegacyPeerInfo::isResponsive() const
{
    if (isClient())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        return connection.isResponsive();
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::isResponsive called for a peer which is not a client.");
    }

    return true;
}

BlazeRpcError LegacyPeerInfo::checkConnectivity(const TimeValue& absoluteTimeout)
{
    if (isClient() && isPersistent())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        return connection.waitForConnectivityCheck(absoluteTimeout);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::checkConnectivity called for a peer which is either not a client or does not have persistent connection.");
    }

    return Blaze::ERR_SYSTEM;
}

void LegacyPeerInfo::closePeerConnection(UserSessionDisconnectType closeReason, bool immediate)
{
    if (isClient())
    {
        ClientInboundRpcConnection& connection = static_cast<ClientInboundRpcConnection&>(mConnection);

        if (connection.isPersistent())
        {
            connection.setCloseReason(closeReason, 0);

            if (immediate)
                connection.close();
            else
                connection.scheduleGracefulClose();
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::CONNECTION, "LegacyPeerInfo::closePeerConnection called for a peer which is not a client.");
    }
}


}

