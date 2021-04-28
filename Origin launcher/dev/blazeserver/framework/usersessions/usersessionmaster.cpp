/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/event/eventmanager.h"
#include "framework/util/hashutil.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"



namespace Blaze
{


static const char* sConnectivityStateStr[] =
{
    "CONNECTIVITY_STATE_CHECK_DISABLED",
    "CONNECTIVITY_STATE_PENDING_DETERMINATION", 
    "CONNECTIVITY_STATE_RESPONSIVE", 
    "CONNECTIVITY_STATE_UNRESPONSIVE",
};


FiberLocalWrappedPtr<UserSessionMasterPtr, UserSessionMaster> gCurrentLocalUserSession;

void intrusive_ptr_add_ref(UserSessionMaster* ptr)
{
    ++ptr->mRefCount;
}

void intrusive_ptr_release(UserSessionMaster* ptr)
{
    if (ptr->mRefCount > 0)
    {
        if (--ptr->mRefCount == 0)
        {
            delete ptr;
        }
    }
    else
    {
        EA_FAIL_MSG("Invalid UserSessionMaster::mRefCount");
    }
}

void NotificationJobQueue::onSubscriptionJobQueueOpened()
{
    SliverPtr sliver = gSliverManager->getSliver(MakeSliverIdentity(UserSessionsMaster::COMPONENT_ID, mSliverId));
    EA_ASSERT((sliver != nullptr) && sliver->isOwnedByCurrentInstance());

    BlazeRpcError err = sliver->waitForAccess(mSliverAccess, "NotificationJobQueue::onSubscriptionJobQueueOpened");
    if (err != ERR_OK)
    {
        BLAZE_ASSERT_LOG(Log::USER, "NotificationJobQueue.onSubscriptionJobQueueOpened: Failed to get access for SliverId(" << sliver->getSliverId() << ") in "
            "SliverNamespace(" << gSliverManager->getSliverNamespaceStr(sliver->getSliverNamespace()) << ") with err(" << ErrorHelp::getErrorName(err) << ")");
    }
}

void NotificationJobQueue::onSubscriptionJobQueueDrained()
{
    mSliverAccess.release();
}

UserSessionMaster::UserSessionMaster(const UserSessionMaster& initializeFrom) :
    UserSession(initializeFrom.mExistenceData->clone()),
    mUserInfo(initializeFrom.mUserInfo),
    mUserSessionData(initializeFrom.mUserSessionData->clone()),
    mPeerInfo(nullptr),
    mCleanupTimerId(INVALID_TIMER_ID),
    mCoalescedUedUpdateTimerId(INVALID_TIMER_ID),
    mCurrentConnectivityState(CONNECTIVITY_STATE_CHECK_DISABLED),
    mConnectivityStateCheckTimerId(INVALID_TIMER_ID)
{
    static_assert(CONNECTIVITY_STATE_UNRESPONSIVE == (EAArrayCount(sConnectivityStateStr) - 1), "If you see this compile error, make sure UserSessionMaster::ConnectivityState and sConnectivityStateStr are in sync.");
    
    EA_ASSERT(mUserInfo != nullptr);

    setUserSessionId(INVALID_USER_SESSION_ID);
    setParentSessionId(INVALID_USER_SESSION_ID);
    setChildSessionId(INVALID_USER_SESSION_ID);
    getSubscribedUsers().clear();
    getSubscribedSessions().clear();

    ConnectionGroupIdListNode::mpNext = ConnectionGroupIdListNode::mpPrev = nullptr;
}

UserSessionMaster::UserSessionMaster(UserSessionId userSessionId, UserInfoPtr userInfo) :
    UserSession(BLAZE_NEW UserSessionExistenceData),
    mUserInfo(userInfo),
    mUserSessionData(BLAZE_NEW UserSessionData),
    mPeerInfo(nullptr),
    mCleanupTimerId(INVALID_TIMER_ID),
    mCoalescedUedUpdateTimerId(INVALID_TIMER_ID),
    mCurrentConnectivityState(CONNECTIVITY_STATE_CHECK_DISABLED),
    mConnectivityStateCheckTimerId(INVALID_TIMER_ID)
{
    EA_ASSERT(mUserInfo != nullptr);
    setUserSessionId(userSessionId);

    ConnectionGroupIdListNode::mpNext = ConnectionGroupIdListNode::mpPrev = nullptr;
}

UserSessionMaster::~UserSessionMaster()
{
    cancelCoalescedExtendedDataUpdate();

    if (mConnectivityStateCheckTimerId != INVALID_TIMER_ID || mCleanupTimerId != INVALID_TIMER_ID)
    {
        if (gSelector->cancelTimer(mConnectivityStateCheckTimerId))
        {
            // how did we leave this timer active???
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.dtor: mConnectivityStateCheckTimerId=" << mConnectivityStateCheckTimerId << " was not canceled");
        }
        else if (mConnectivityStateCheckTimerId != INVALID_TIMER_ID)
        {
            // why is this ID not reset if there is no associated timer?
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.dtor: mConnectivityStateCheckTimerId=" << mConnectivityStateCheckTimerId << " was not reset");
        }
        if (gSelector->cancelTimer(mCleanupTimerId))
        {
            // how did we leave this timer active???
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.dtor: mCleanupTimerId=" << mCleanupTimerId << " was not canceled");
        }
        else if (mCleanupTimerId != INVALID_TIMER_ID)
        {
            // why is this ID not reset if there is no associated timer?
            BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.dtor: mCleanupTimerId=" << mCleanupTimerId << " was not reset");
        }
        eastl::string callstack;
        CallStack::getCurrentStackSymbols(callstack);
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.dtor: UserSessionId=" << getUserSessionId() << ", BlazeId=" << getBlazeId() << ", ConnectionGroupId=" << getConnectionGroupId()
            << "\n  callstack:\n" << SbFormats::PrefixAppender("  ", callstack.c_str()));
    }
    mConnectivityStateCheckTimerId = INVALID_TIMER_ID;
    mCleanupTimerId = INVALID_TIMER_ID;
    removeFromConnectionGroupList();
}

void UserSessionMaster::changeUserInfo(UserInfoPtr userInfo)
{
    EA_ASSERT(userInfo != nullptr);
    mUserInfo = userInfo;
}

void UserSessionMaster::filloutUserIdentification(UserIdentification& userIdentification) const
{
    mUserInfo->filloutUserIdentification(userIdentification);

    // Override the BlazeId member of the UserIdentification with this UserSessions's blazeId value.
    // The reason we do this is, if this is a USER_SESSION_GUEST type, then the blazeId will be negative, and this
    // needs to be propagated to callers interested in this UserSession's info.  The *real* UserInfo is always available
    // via getUserInfo().  Furthermore, callers can use getUserInfo().filloutUserIdentification() to get the "real" user's ident.
    userIdentification.setBlazeId(getBlazeId());
}

void UserSessionMaster::filloutUserCoreIdentification(CoreIdentification& coreIdentification) const
{
    mUserInfo->filloutUserCoreIdentification(coreIdentification);
    coreIdentification.setBlazeId(getBlazeId());
}

Blaze::ConnectionGroupObjectId UserSessionMaster::getConnectionGroupObjectId() const
{
    if (getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
        return EA::TDF::ObjectId(ENTITY_TYPE_CONN_GROUP, getConnectionGroupId());
    return EA::TDF::OBJECT_ID_INVALID;
}

void UserSessionMaster::removeFromConnectionGroupList()
{
    if (ConnectionGroupIdListNode::mpNext != nullptr && ConnectionGroupIdListNode::mpPrev != nullptr)
    {
        ConnectionGroupList::remove(*this);
        ConnectionGroupIdListNode::mpNext = ConnectionGroupIdListNode::mpPrev = nullptr;
    }
}

BlazeRpcError UserSessionMaster::updateDataValue(UserExtendedDataKey& key, UserExtendedDataValue value)
{
    return updateDataValue(COMPONENT_ID_FROM_UED_KEY(key), DATA_ID_FROM_UED_KEY(key), value);
}

BlazeRpcError UserSessionMaster::updateDataValue(uint16_t componentId, uint16_t key, UserExtendedDataValue value)
{
    return gUserSessionManager->updateExtendedData(getUserSessionId(), componentId, key, value);
}

bool UserSessionMaster::getDataValue(UserExtendedDataKey& key, UserExtendedDataValue &value) const
{
    return getDataValue(COMPONENT_ID_FROM_UED_KEY(key), DATA_ID_FROM_UED_KEY(key), value);
}

bool UserSessionMaster::getDataValue(uint16_t componentId, uint16_t key, UserExtendedDataValue &value) const
{
    return UserSession::getDataValue(getExtendedData().getDataMap(), componentId, key, value);
}


bool UserSessionMaster::isAllowedToInteractWithPlatform(ClientPlatformType targetPlatform) const
{
    if (targetPlatform == getClientPlatform())
    {
        return true;
    }
    else if (isCrossplayEnabled())
    {
        const ClientPlatformSet& allowedPlatformSet = gUserSessionManager->getUnrestrictedPlatforms(getClientPlatform());
        return (allowedPlatformSet.find(targetPlatform) != allowedPlatformSet.end());
    }

    return false;
}

void UserSessionMaster::attachPeerInfo(PeerInfo& peerInfo)
{
    // A UserSessionMaster can only be attached to a connection whose protocol is based on persistence, or a gRPC connection. (e.g. supports notifications)
    if (!peerInfo.canAttachToUserSession())
        return;

    bool shouldReEnableConnectivityChecking = isConnectivityCheckingEnabled();
    disableConnectivityChecking();

    EA_ASSERT(mPeerInfo == nullptr && peerInfo.getUserIndexFromUserSessionId(getUserSessionId()) == -1);
    mPeerInfo = &peerInfo;

    // Update the peer info for any updated information that may have come in as part of the login request (such as a tools client type
    // coming in over http end point)
    mPeerInfo->setClientInfo(&getClientInfo());
    mPeerInfo->setBaseLocale(getSessionLocale());
    mPeerInfo->setBaseCountry(getSessionCountry());
    mPeerInfo->setClientType(getClientType());
    mPeerInfo->setServiceName(getServiceName());
    mPeerInfo->setIgnoreInactivityTimeout(mExistenceData->getSessionFlags().getIgnoreInactivityTimeout()); 
    
    if (mPeerInfo->isPersistent())
    {
        // If this UserSessionMaster is being attached to a peer that has a persistent connection, then the lifetime of this
        // UserSessionMaster is governed by the peer, and it's associated InactivityTimeout
        // rules, and *not* by a common lifetime timer.
        cancelCleanupTimeout();

        if (mPeerInfo->getAttachedUserSessionsCount() == 0)
        {
            if (mPeerInfo->getConnectionGroupId() == INVALID_CONNECTION_GROUP_ID)
                mPeerInfo->setConnectionGroupId(getConnectionGroupId());
            else if (mPeerInfo->getConnectionGroupId() != getConnectionGroupId())
            {
                BLAZE_ASSERT_LOG(Log::USER, "UserSessionMaster.attachToPeerInfo: UserSessionId " << getUserSessionId() << " with ConnectionGroupId " << getConnectionGroupId() << " "
                    "is attaching to a PeerInfo with ConnectionGroupId " << mPeerInfo->getConnectionGroupId() << ".  This should never happen.");
            }
        }

        mPeerInfo->setUserSessionIdAtUserIndex(getConnectionUserIndex(), getUserSessionId());

        if (!getPendingNotificationList().empty())
        {
            for (NotificationDataList::iterator it = getPendingNotificationList().begin(), end = getPendingNotificationList().end(); it != end; ++it)
            {
                Notification pendingNotification(*it);
                if (pendingNotification.isNotification())
                {
                    pendingNotification.setUserSessionId(getUserSessionId());
                    sendNotificationInternal(pendingNotification);
                }
                else
                {
                    BLAZE_ASSERT_LOG(Log::USER, "UserSessionMaster.attachToClientConnection: pendingNotification is not valid.");
                }
            }

            getPendingNotificationList().clear();
            gUserSessionMaster->upsertUserSessionData(*mUserSessionData);
        }
    }
    else
    {
        mPeerInfo->setUserSessionId(getUserSessionId());
    }

    if (shouldReEnableConnectivityChecking)
    {
        EA_ASSERT(mPeerInfo->isPersistent());
        enableConnectivityChecking();
    }
}

void UserSessionMaster::detachPeerInfo()
{
    if (mPeerInfo != nullptr)
    {
        if (mPeerInfo->isPersistent())
        {
            EA_ASSERT(mPeerInfo->getUserIndexFromUserSessionId(getUserSessionId()) == (int32_t)getConnectionUserIndex());

            mPeerInfo->setUserSessionIdAtUserIndex(getConnectionUserIndex(), INVALID_USER_SESSION_ID);

            bool shouldReEnableConnectivityChecking = isConnectivityCheckingEnabled();
            disableConnectivityChecking();

            // Now that this UserSessionMaster has no associated peer to govern its lifetime, we need
            // to set the cleanup timer so that it will expire after the configured timeout period with no activity.
            setOrUpdateCleanupTimeout();

            mPeerInfo = nullptr;

            if (shouldReEnableConnectivityChecking)
                enableConnectivityChecking();
        }
        else
        {
            mPeerInfo->setUserSessionId(INVALID_USER_SESSION_ID);
            mPeerInfo = nullptr;
        }
    }
}

/*! \brief forces a disconnect for this user session's connection. For testing purposes only. */
void UserSessionMaster::forceDisconnect()
{
    BLAZE_WARN_LOG(Log::USER,"UserSession(" << getUserSessionId() << ").forceDisconnect() called, this method should only be called for testing purposes.");
    if (mPeerInfo != nullptr)
    {
        mPeerInfo->closePeerConnection(DISCONNECTTYPE_FORCED, true);
    }
}

void UserSessionMaster::getKey(char8_t (&keyString)[MAX_SESSION_KEY_LENGTH]) const
{
    const int32_t count = blaze_snzprintf(keyString, MAX_SESSION_KEY_LENGTH, "%.16" PRIx64 "_", getUserSessionId());
    UserSessionManager::createSHA256Sum(keyString + count, MAX_SESSION_KEY_LENGTH - count, getUserSessionId(), getRandom(), getCreationTime().getMicroSeconds());
}

bool UserSessionMaster::validateKeyHashSum(const char8_t* keyHashSum) const
{
    if (EA_UNLIKELY(keyHashSum == nullptr))
        return false;

    // Compare the hash sum passed in against one generated against this session
    char8_t sessionSum[HashUtil::SHA256_STRING_OUT];
    UserSessionManager::createSHA256Sum(sessionSum, sizeof(sessionSum), getUserSessionId(), getRandom(), getCreationTime().getMicroSeconds());

    return (blaze_strcmp(keyHashSum, sessionSum) == 0);
}

BlazeRpcError UserSessionMaster::waitForJobQueueToFinish()
{
    if (EA_UNLIKELY(mNotificationJobQueue == nullptr))
        return ERR_OK;

    return mNotificationJobQueue->join();
}

void UserSessionMaster::sendNotification(Notification& notification)
{
    if (mNotificationJobQueue == nullptr)
    {
        // There is no notification queue. This can happen if this UserSession has just been logged out (or otherwise destroyed), while something
        // is still (for example) iterating a loop, and trying to send this guy some notifications.
        return;
    }

    if (notification.isNotification())
    {
        if (!mNotificationJobQueue->hasPendingWork() ||
            notification.is(UserSessionsSlaveStub::NOTIFICATION_INFO_USERADDED) ||
            notification.is(UserSessionsSlaveStub::NOTIFICATION_INFO_USERREMOVED) ||
            notification.is(UserSessionsSlaveStub::NOTIFICATION_INFO_USERSESSIONEXTENDEDDATAUPDATE))
        {
            sendNotificationInternal(notification);
        }
        else
        {
            NotificationPtr copy = notification.createSnapshot();

            mNotificationJobQueue->queueFiberJob<UserSessionId, NotificationPtr>
                (&UserSessionMaster::sendNotificationInternalAsync, getUserSessionId(), copy, "UserSessionMaster::sendNotificationInternalAsync");
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionMaster.sendNotification: notification is not valid.");
    }
}

void UserSessionMaster::sendNotificationInternalAsync(UserSessionId userSessionId, NotificationPtr notification)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
        return;

    userSession->sendNotificationInternal(*notification);
}

void UserSessionMaster::sendNotificationInternal(Notification& notification)
{
    if (notification.isNotification())
    {
        notification.setUserSessionId(getUserSessionId());
        notification.obfuscatePlatformInfo(getClientPlatform());

        if (mExistenceData->getSessionFlags().getUseNotificationCache())
        {
            NotificationCacheSlaveImpl *notifyCache = static_cast<NotificationCacheSlaveImpl*>(gController->getComponent(NotificationCacheSlave::COMPONENT_ID));
            if (notifyCache != nullptr)
                notifyCache->cacheNotification(notification);
        }
        else if (mPeerInfo != nullptr && mPeerInfo->hasNotificationHandler(notification.getComponentId()))
        {
            mPeerInfo->sendNotification(notification);
        }
        else
        {
            NotificationData* notificationData = getPendingNotificationList().allocate_element();
            notification.filloutNotificationData(*notificationData);
            getPendingNotificationList().push_back(notificationData);
        }

        // Event manager will handle the determination re whether or not to generate an event from this notification.
        gEventManager->submitSessionNotification(notification, *this);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::USER, "UserSessionMaster.sendNotification: notification is not valid.");
    }
}

void UserSessionMaster::sendPendingNotificationsByComponent(ComponentId componentId)
{
    if (!getPendingNotificationList().empty())
    {
        for (NotificationDataList::iterator iter = getPendingNotificationList().begin(); iter != getPendingNotificationList().end();)
        {
            if ((*iter)->getComponentId() == componentId)
            {
                Notification pendingNotification(*iter);
                if (pendingNotification.isNotification())
                {
                    pendingNotification.setUserSessionId(getUserSessionId());
                    sendNotificationInternal(pendingNotification);
                }
                else
                {
                    BLAZE_ASSERT_LOG(Log::USER, "UserSessionMaster.sendPendingNotificationByComponent: pendingNotification is not valid.");
                }

                iter = getPendingNotificationList().erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        gUserSessionMaster->upsertUserSessionData(*mUserSessionData);
    }
}

bool UserSessionMaster::isBlockingNotifications() const
{
    if (mPeerInfo != nullptr && mPeerInfo->supportsNotifications())
        return mPeerInfo->isBlockingNotifications(false);
    
    return false;
}

bool UserSessionMaster::isBlockingNonPriorityNotifications() const
{
    if (mPeerInfo != nullptr && mPeerInfo->supportsNotifications())
        return mPeerInfo->isBlockingNotifications(true);
    
    return false;
}

void UserSessionMaster::setOrUpdateCleanupTimeout()
{
    TimeValue absoluteTimeout = TimeValue::getTimeOfDay() + gUserSessionManager->getSessionCleanupTimeout(getClientType());
    if (mCleanupTimerId != INVALID_TIMER_ID)
    {
        gSelector->updateTimer(mCleanupTimerId, absoluteTimeout);
    }
    else
    {
        mCleanupTimerId = gSelector->scheduleFiberTimerCall(absoluteTimeout, &UserSessionMaster::handleCleanupTimeoutAsync, getUserSessionId(), "UserSessionMaster::handleCleanupTimeoutAsync");
        if (EA_UNLIKELY(mCleanupTimerId == INVALID_TIMER_ID))
        {
            // The timer wasn't valid?  We've got a problem.
            BLAZE_ERR_LOG(Log::USER, "UserSessionMaster.setOrUpdateCleanupTimeout: Unable to add a UserSessionMaster cleanup timer for session " << getUserSessionId() << ".");
        }
    }
}

void UserSessionMaster::handleCleanupTimeoutAsync(UserSessionId userSessionId)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
    {
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.handleCleanupTimeoutAsync: Could not get local session for id " << userSessionId);
        return;
    }

    gUserSessionMaster->destroyUserSession(userSessionId, DISCONNECTTYPE_INACTIVITY_TIMEOUT, 0, FORCED_LOGOUTTYPE_INVALID);
}

void UserSessionMaster::cancelCleanupTimeout()
{
    if (mCleanupTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mCleanupTimerId);
        mCleanupTimerId = INVALID_TIMER_ID;
    }
}

BlazeRpcError UserSessionMaster::addUser(BlazeId blazeId, bool sendImmediately, bool includeSessionSubscription)
{
    BlazeRpcError err = ERR_OK;

    if (blazeId == INVALID_BLAZE_ID)
        return USER_ERR_INVALID_PARAM;

    // Find or insert the blazeId into this UserSession's SubscribedUsers map.
    UserSessionData::Int32ByBlazeId::insert_return_type insertRet = getSubscribedUsers().insert(eastl::make_pair(blazeId, 0));
    EA_ASSERT(insertRet.second || (insertRet.first->second > 0));

    // Inc the ref-count for this user.
    ++insertRet.first->second;

    // Note, insertRet.second==true means a new node was inserted.
    if (insertRet.second)
    {
        UserInfoPtr userInfo;
        bool trusted = false;

        if (blazeId == getBlazeId())
        {
            // First, if the blazeId is actually our user id, then we already have the UserInfo.
            userInfo = mUserInfo;
        }
        else
        {
            if (UserSessionManager::isStatelessUser(blazeId))
            {
                // Or maybe, if the blazeId is "stateless", meaning, a BlazeId with a negative value, then we need to find the real user id.
                UserSessionId statelessUserSessionId = gUserSessionManager->getUserSessionIdByBlazeId(blazeId);
                UserSessionPtr statelessUserSession = gUserSessionManager->getSession(statelessUserSessionId);
                if (statelessUserSession != nullptr)
                {
                    if (statelessUserSession->getUserSessionType() == USER_SESSION_TRUSTED)
                    {
                        trusted = true;
                    }

                    err = gUserSessionManager->lookupUserInfoByPlatformInfo(statelessUserSession->getPlatformInfo(), userInfo);
                }
            }
            else
            {
                err = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
            }
        }

        if (err == ERR_OK)
        {
            NotifyUserAdded notifyUserAdded;
            if (userInfo != nullptr)
                userInfo->filloutUserIdentification(notifyUserAdded.getUserInfo());

            // Because the blazeId that ended up being looked up in the system could have been resolved to a different (real)
            // user id, we always need to set the blazeId in the notification back to the blazeId that was passed in.
            notifyUserAdded.getUserInfo().setBlazeId(blazeId);

            UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId);
            if (UserSession::isValidUserSessionId(primarySessionId))
            {
                BlazeRpcError extDataErr = gUserSessionManager->getRemoteUserExtendedData(primarySessionId, notifyUserAdded.getExtendedData(),
                    (includeSessionSubscription ? getUserSessionId() : INVALID_USER_SESSION_ID));
                if (extDataErr != ERR_OK)
                {
                    // This error is ok to ignore
                    BLAZE_TRACE_LOG(Log::USER, "UserSession.addUser: Failed to getRemoteUserExtendedData(" << SbFormats::HexLower(primarySessionId) << ") for BlazeId(" << blazeId << ") "
                        "requested by UserSession(" << SbFormats::HexLower(getUserSessionId()) << ") with BlazeId(" << getBlazeId() << ")");
                }

                if ((blazeId != getBlazeId()) && !isAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true))
                {
                    notifyUserAdded.getExtendedData().getAddress().setDummyStruct(nullptr);
                }
            }

            gUserSessionManager->sendUserAddedToUserSession(this, &notifyUserAdded, sendImmediately);
        }
        else if (!trusted)
        {
            BLAZE_ERR_LOG(Log::USER, "UserSessionMaster.addUser: Error (" << ErrorHelp::getErrorName(err) << ") occurred when looking up user info for BlazeId (" << blazeId << ").  The user was subscribed, but no notification will be sent.");
        }
    }

    gUserSessionMaster->upsertUserSessionData(*mUserSessionData);

    return err;
}

BlazeRpcError UserSessionMaster::addUsers(const BlazeIdList& blazeIds, bool sendImmediately, bool includeSessionSubscription)
{
    BlazeRpcError err = ERR_OK;
    for (BlazeIdList::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
    {
        err = addUser(*it, sendImmediately, includeSessionSubscription);
    }
    return err;
}

void UserSessionMaster::addUsersAsync(const BlazeIdList& blazeIds, bool includeSessionSubscription)
{
    if (EA_UNLIKELY(mNotificationJobQueue == nullptr))
        return;

    EA::TDF::tdf_ptr<BlazeIdList> _blazeIds = BLAZE_NEW BlazeIdList(blazeIds);
    mNotificationJobQueue->queueFiberJob<UserSessionId, EA::TDF::tdf_ptr<BlazeIdList>, bool >(&UserSessionMaster::finish_addUsersAsync, getUserSessionId(), _blazeIds, includeSessionSubscription, "UserSessionMaster::finish_addUsersAsync");
}

void UserSessionMaster::finish_addUsersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<BlazeIdList> _blazeIds, bool includeSessionSubscription)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
        return;

    userSession->addUsers(*_blazeIds, true, includeSessionSubscription);
}

BlazeRpcError UserSessionMaster::removeUser(BlazeId blazeId, bool sendImmediately, bool includeSessionSubscription)
{
    BlazeRpcError err = ERR_OK;

    if (blazeId == INVALID_BLAZE_ID)
        return USER_ERR_INVALID_PARAM;

    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId);
    if (UserSession::isValidUserSessionId(primarySessionId))
        removeNotifier(primarySessionId, sendImmediately);

    UserSessionData::Int32ByBlazeId::iterator it = getSubscribedUsers().find(blazeId);
    if (it != getSubscribedUsers().end())
    {
        EA_ASSERT(it->second > 0);
        if (--it->second == 0)
        {
            BLAZE_TRACE_LOG(Log::USER, "UserSessionMaster.removeUser: BlazeId(" << blazeId << ") was removed from the subscribed users list." ); 

            getSubscribedUsers().erase(blazeId);

            NotifyUserRemoved notifyUserRemoved;
            notifyUserRemoved.setBlazeId(blazeId);
            gUserSessionManager->sendUserRemovedToUserSession(this, &notifyUserRemoved, sendImmediately);
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::USER, "UserSessionMaster.removeUser: BlazeId(" << blazeId << ") is already removed from UserSession(" << getUserSessionId() << ")");
    }

    gUserSessionMaster->upsertUserSessionData(*mUserSessionData);

    return err;
}

BlazeRpcError UserSessionMaster::removeUsers(const BlazeIdList& blazeIds, bool sendImmediately, bool includeSessionSubscription)
{
    BlazeRpcError err = ERR_OK;
    for (BlazeIdList::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
    {
        err = removeUser(*it, sendImmediately, includeSessionSubscription);
    }
    return err;
}

void UserSessionMaster::removeUsersAsync(const BlazeIdList& blazeIds, bool includeSessionSubscription)
{
    if (EA_UNLIKELY(mNotificationJobQueue == nullptr))
        return;

    EA::TDF::tdf_ptr<BlazeIdList> _blazeIds = BLAZE_NEW BlazeIdList(blazeIds);
    mNotificationJobQueue->queueFiberJob<UserSessionId, EA::TDF::tdf_ptr<BlazeIdList>, bool >(&UserSessionMaster::finish_removeUsersAsync, getUserSessionId(), _blazeIds, includeSessionSubscription, "UserSessionMaster::finish_removeUsersAsync");
}

void UserSessionMaster::finish_removeUsersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<BlazeIdList> _blazeIds, bool includeSessionSubscription)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
        return;

    userSession->removeUsers(*_blazeIds, true, includeSessionSubscription);
}

BlazeRpcError UserSessionMaster::addNotifier(UserSessionId notifierId, bool sendImmediately)
{
    BlazeRpcError err = ERR_OK;

    if (notifierId == INVALID_USER_SESSION_ID)
        return err;

    UserSessionSubscriberRequest request;
    request.setNotifierSessionId(notifierId);
    request.setSubscriberSessionId(getUserSessionId());
    request.setSendImmediately(sendImmediately);

    if (getSliverId() == GetSliverIdFromSliverKey(notifierId))
        err = gUserSessionMaster->processAddUserSessionSubscriber(request, nullptr);
    else
        err = gUserSessionManager->getMaster()->addUserSessionSubscriber(request);

    if (err != ERR_OK && err != USER_ERR_SESSION_NOT_FOUND)
    {
        // If err==USER_ERR_SESSION_NOT_FOUND, then no big deal. Otherwise, something interesting/bad may be happenning.
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.addNotifier: Failed to add notifier(" << notifierId << ") "
            "requested by UserSession(" << getUserSessionId() << ") with BlazeId(" << getBlazeId() << "), err: " << ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError UserSessionMaster::addNotifiers(const UserSessionIdList& notifierIds, bool sendImmediately)
{
    BlazeRpcError err = ERR_OK;
    for (UserSessionIdList::const_iterator it = notifierIds.begin(), end = notifierIds.end(); it != end; ++it)
    {
        err = addNotifier(*it, sendImmediately);
    }
    return err;
}

void UserSessionMaster::addNotifiersAsync(const UserSessionIdList& notifierIds)
{
    if (EA_UNLIKELY(mNotificationJobQueue == nullptr))
        return;

    EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds = BLAZE_NEW UserSessionIdList(notifierIds);
    mNotificationJobQueue->queueFiberJob<UserSessionId, EA::TDF::tdf_ptr<UserSessionIdList> >(&UserSessionMaster::finish_addNotifiersAsync, getUserSessionId(), _notifierIds, "UserSessionMaster::finish_addNotifiersAsync");
}

void UserSessionMaster::finish_addNotifiersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
        return;

    userSession->addNotifiers(*_notifierIds, true);
}

BlazeRpcError UserSessionMaster::removeNotifier(UserSessionId notifierId, bool sendImmediately)
{
    BlazeRpcError err = ERR_OK;

    if (notifierId == INVALID_USER_SESSION_ID)
        return err;

    UserSessionSubscriberRequest request;
    request.setNotifierSessionId(notifierId);
    request.setSubscriberSessionId(getUserSessionId());
    request.setSendImmediately(sendImmediately);

    if (getSliverId() == GetSliverIdFromSliverKey(notifierId))
        err = gUserSessionMaster->processRemoveUserSessionSubscriber(request, nullptr);
    else
        err = gUserSessionManager->getMaster()->removeUserSessionSubscriber(request);

    if (err != ERR_OK && err != USER_ERR_SESSION_NOT_FOUND)
    {
        // If err==USER_ERR_SESSION_NOT_FOUND, then no big deal. Otherwise, something interesting/bad may be happenning.
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.removeNotifier: Failed to remove notifier(" << notifierId << ") "
            "requested by UserSession(" << getUserSessionId() << ") with BlazeId(" << getBlazeId() << "), err: " << ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError UserSessionMaster::removeNotifiers(const UserSessionIdList& notifierIds, bool sendImmediately)
{
    BlazeRpcError err = ERR_OK;
    for (UserSessionIdList::const_iterator it = notifierIds.begin(), end = notifierIds.end(); it != end; ++it)
    {
        err = removeNotifier(*it, sendImmediately);
    }
    return err;
}

void UserSessionMaster::removeNotifiersAsync(const UserSessionIdList& notifierIds)
{
    if (EA_UNLIKELY(mNotificationJobQueue == nullptr))
        return;

    EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds = BLAZE_NEW UserSessionIdList(notifierIds);
    mNotificationJobQueue->queueFiberJob<UserSessionId, EA::TDF::tdf_ptr<UserSessionIdList> >(&UserSessionMaster::finish_removeNotifiersAsync, getUserSessionId(), _notifierIds, "UserSessionMaster::finish_removeNotifiersAsync");
}

void UserSessionMaster::finish_removeNotifiersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
        return;

    userSession->removeNotifiers(*_notifierIds, true);
}

EA::TDF::ObjectId UserSessionMaster::getLocalUserGroupObjectId() const
{
    if (getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID)
        return EA::TDF::ObjectId(ENTITY_TYPE_LOCAL_USER_GROUP, getConnectionGroupId());

    return EA::TDF::OBJECT_ID_INVALID;
}

void UserSessionMaster::sendCoalescedExtendedDataUpdateAsync(UserSessionId userSessionId)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
    {
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.sendCoalescedExtendedDataUpdateAsync: Could not get local session for id " << userSessionId);
        return;
    }

    // isCoalescedUpdateSentinel is always true here to avoid calling scheduleCoalescedExtendedDataUpdate again
    // and to reset mCoalescedUedUpdateTimerId (via cancelCoalescedExtendedDataUpdate)
    userSession->sendCoalescedExtendedDataUpdate(true);
}

void UserSessionMaster::sendCoalescedExtendedDataUpdate(bool isCoalescedUpdateSentinel)
{
    if (isCoalescedUpdateSentinel)
    {
        typedef eastl::vector<UserSessionId> UserSessionIdVector;
        UserSessionIdVector targetIdsOmitNetworkAddress;
        UserSessionIdVector targetIdsIncludeNetworkAddress;
        bool sendUpdateWithNetworkAddress = false;
        bool sendUpdateWithoutNetworkAddress = false;

        targetIdsOmitNetworkAddress.reserve(getSubscribedSessions().size());
        targetIdsIncludeNetworkAddress.reserve(getSubscribedSessions().size());
        for (UserSessionData::Int32ByUserSessionId::iterator it = getSubscribedSessions().begin(), end = getSubscribedSessions().end(); it != end; ++it)
        {
            if ((it->first == getUserSessionId()) || UserSession::isAuthorized(it->first, Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true)) // determine if current user is entitled to *any* network addresses 
            {
                
                targetIdsIncludeNetworkAddress.push_back(it->first);
                // we can send a usersession its own network address
                // This little thing ensures that 'this' UserSession receives his own UED update first, which is needed to address some SDK expectations.
                if (targetIdsIncludeNetworkAddress.back() == getUserSessionId())
                {
                    UserSessionId frontId = targetIdsIncludeNetworkAddress.front();
                    targetIdsIncludeNetworkAddress.front() = getUserSessionId();
                    targetIdsIncludeNetworkAddress.back() = frontId;
                }

                sendUpdateWithNetworkAddress = true;
            }
            else
            {
                // this user doesn't get network addresses
                targetIdsOmitNetworkAddress.push_back(it->first);
                sendUpdateWithoutNetworkAddress = true;
            }
        }

        if (sendUpdateWithNetworkAddress)
        {
            UserSessionExtendedDataUpdate updateWithNetworkAddress;
            updateWithNetworkAddress.setBlazeId(getBlazeId());
            updateWithNetworkAddress.setSubscribed(true);
            updateWithNetworkAddress.setExtendedData(getExtendedData());

            gUserSessionManager->sendUserSessionExtendedDataUpdateToUserSessionsById(
                targetIdsIncludeNetworkAddress.begin(), targetIdsIncludeNetworkAddress.end(), UserSessionIdIdentityFunc(), &updateWithNetworkAddress);
        }

        if (sendUpdateWithoutNetworkAddress)
        {
            // clear out the network address, then send to remaining users
            UserSessionExtendedDataUpdate updateWithoutNetworkAddress;
            updateWithoutNetworkAddress.setBlazeId(getBlazeId());
            updateWithoutNetworkAddress.setSubscribed(true);
            getExtendedData().copyInto(updateWithoutNetworkAddress.getExtendedData());
            
            NetworkAddress unsetNetworkAddress;
            unsetNetworkAddress.copyInto(updateWithoutNetworkAddress.getExtendedData().getAddress());

            gUserSessionManager->sendUserSessionExtendedDataUpdateToUserSessionsById(
                targetIdsOmitNetworkAddress.begin(), targetIdsOmitNetworkAddress.end(), UserSessionIdIdentityFunc(), &updateWithoutNetworkAddress);
        }


        cancelCoalescedExtendedDataUpdate();
    }
    else
    {
        gUserSessionManager->getMetricsObj().mUEDMessagesCoallesced.increment(getSubscribedSessions().size());
        scheduleCoalescedExtendedDataUpdate();
    }
}

void UserSessionMaster::scheduleCoalescedExtendedDataUpdate()
{
    if (mCoalescedUedUpdateTimerId == INVALID_TIMER_ID)
    {
        mCoalescedUedUpdateTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + gUserSessionManager->getConfig().getUEDUpdateRateTimeDelay(),
            &UserSessionMaster::sendCoalescedExtendedDataUpdateAsync, getUserSessionId(), "UserSessionMaster::sendCoalescedExtendedDataUpdateAsync");
    }
}

void UserSessionMaster::cancelCoalescedExtendedDataUpdate()
{
    gSelector->cancelTimer(mCoalescedUedUpdateTimerId);
    mCoalescedUedUpdateTimerId = INVALID_TIMER_ID;
}

void UserSessionMaster::insertComponentStateTdf(ComponentId componentId, const EA::TDF::TdfPtr& tdfPtr)
{
    mCustomStateTdfByComponentId.insert(eastl::make_pair(componentId, tdfPtr));
}

void UserSessionMaster::eraseComponentStateTdf(ComponentId componentId)
{
    mCustomStateTdfByComponentId.erase(componentId);
}

void UserSessionMaster::upsertCustomStateTdf(ComponentId componentId, EA::TDF::Tdf& tdf)
{
    EA_ASSERT(!tdf.isNotRefCounted());
    TdfPtrByComponentId::insert_return_type insertRet = mCustomStateTdfByComponentId.insert(eastl::make_pair(componentId, &tdf));
    if (!insertRet.second)
    {
        if (tdf.getTdfId() != insertRet.first->second->getTdfId())
        {
            BLAZE_WARN_LOG(UserSessionsMaster::LOGGING_CATEGORY, "UserSessionMaster.upsertComponentStateTdf: Existing TdfId(" << insertRet.first->second->getTdfId() << ") is being overwritten by TdfId(" << tdf.getTdfId() << ") "
                "for UserSessionId(" << getUserSessionId() << "), ComponentId(" << componentId << "). This can affect backwards compatability.");
        }
    }

    insertRet.first->second = &tdf;

    StorageFieldName customStateFieldName(StorageFieldName::CtorSprintf(), UserSessionsMasterImpl::PRIVATE_CUSTOM_STATE_FIELD_FMT, componentId);
    gUserSessionMaster->getUserSessionStorageTable().upsertField(getUserSessionId(), customStateFieldName.c_str(), tdf);
}

void UserSessionMaster::enableConnectivityChecking()
{
    if (gUserSessionManager->getClientTypeDescription(getClientType()).getEnableConnectivityChecks())
        BLAZE_INFO_LOG(Log::USER, "UserSessionMaster.enableConnectivityChecking: current connectivity state(" << sConnectivityStateStr[mCurrentConnectivityState] << "), PeerInfo(" << mPeerInfo << "), UserSessionId(" << getUserSessionId() << ").");
   
    if (isConnectivityCheckingEnabled())
        return;

    const CheckConnectivityConfig& checkConnCfg = gUserSessionMaster->getConfig().getCheckConnectivitySettings();
    if (checkConnCfg.getInterval() > 0)
    {
        mCurrentConnectivityState = CONNECTIVITY_STATE_PENDING_DETERMINATION;
        if (mPeerInfo == nullptr)
        {
            // The session is being migrated. Spawn a fiber to check the connectivity later.  
            mConnectivityStateCheckTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + gUserSessionMaster->getConfig().getCheckConnectivitySettings().getInterval(),
                &UserSessionMaster::handleConnectivityChangeAsync, getUserSessionId(), "UserSessionMaster::handleConnectivityChangeAsync");
        }
        else
        {
            EA_ASSERT(mPeerInfo->isPersistent());
            mPeerInfo->enableConnectivityChecks(checkConnCfg.getInterval(), checkConnCfg.getTimeToWait());
        }
    }
}

void UserSessionMaster::disableConnectivityChecking()
{
    if (gUserSessionManager->getClientTypeDescription(getClientType()).getEnableConnectivityChecks())
        BLAZE_INFO_LOG(Log::USER, "UserSessionMaster.disableConnectivityChecking: current connectivity state(" << sConnectivityStateStr[mCurrentConnectivityState] << "), PeerInfo(" << mPeerInfo << "), UserSessionId(" << getUserSessionId() << ").");
    
    if (!isConnectivityCheckingEnabled())
        return;

    mCurrentConnectivityState = CONNECTIVITY_STATE_CHECK_DISABLED;

    if (mPeerInfo == nullptr)
    {
        gSelector->cancelTimer(mConnectivityStateCheckTimerId);
        mConnectivityStateCheckTimerId = INVALID_TIMER_ID;
    }
    else
    {
        EA_ASSERT(mPeerInfo->isPersistent());
        
        bool shouldDisableConnectivityChecks = true;
        UserSessionIdVectorSet siblingUserSessionIds = mPeerInfo->getUserSessionIds();
        for (UserSessionIdVectorSet::iterator it = siblingUserSessionIds.begin(), end = siblingUserSessionIds.end(); it != end; ++it)
        {
            if (*it == getUserSessionId())
                continue;

            //do not disable connectivity check on the peer if at least one other sibling is still interested in connectivity checking
            UserSessionMasterPtr localUserSession = gUserSessionMaster->getLocalSession(*it);
            if ((localUserSession != nullptr) && localUserSession->isConnectivityCheckingEnabled())
            {
                shouldDisableConnectivityChecks = false;
                break;
            }
        }

        if (shouldDisableConnectivityChecks)
            mPeerInfo->disableConnectivityChecks();
    }
}

void UserSessionMaster::handleConnectivityChangeAsync(UserSessionId userSessionId)
{
    UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
    if (userSession == nullptr)
    {
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.handleConnectivityChangeAsync: Could not get local session for id " << userSessionId);
        return;
    }

    userSession->handleConnectivityChange();
}

void UserSessionMaster::handleConnectivityChange()
{
    if (gUserSessionManager->getClientTypeDescription(getClientType()).getEnableConnectivityChecks())
        BLAZE_INFO_LOG(Log::USER, "UserSessionMaster.handleConnectivityChange: (Entry) current connectivity state(" << sConnectivityStateStr[mCurrentConnectivityState] << "), PeerInfo(" << mPeerInfo << "), UserSessionId(" << getUserSessionId() << ")");
    
    if (!isConnectivityCheckingEnabled())
        return; // Nothing to do if conn checks aren't enabled.

    // This method should never be called on a UserSessionMaster that doesn't already have an owning sliver. 
    EA_ASSERT(mOwnedSliverRef != nullptr);


    mConnectivityStateCheckTimerId = INVALID_TIMER_ID;

    // The callbacks dispatched from the method are allowed to block, so we need to get sliver access.
    Sliver::AccessRef sliverAccess;
    if (!mOwnedSliverRef->getPrioritySliverAccess(sliverAccess))
    {
        // OwnedSliver::getPrioritySliverAccess() does not log failure in all cases so we add a log here.
        BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.handleConnectivityChange: Could not get sliver access for id " << getUserSessionId());
        return;
    }

    UserSessionMasterPtr temp(this); // keep a temp ref so that "this" doesn't become invalid if/while a dispatched callback is blocked

    if (mPeerInfo == nullptr)
    {
        // The session is being migrated and we are watching for connectivity. As it has not attached to a new peerInfo yet, mark it unresponsive.
        if (mCurrentConnectivityState != CONNECTIVITY_STATE_UNRESPONSIVE)
        {
            if (mCurrentConnectivityState == CONNECTIVITY_STATE_RESPONSIVE)
            {
                BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.handleConnectivityChange: No connection for responsive id " << getUserSessionId());
            }
            else
            {
                BLAZE_WARN_LOG(Log::USER, "UserSessionMaster.handleConnectivityChange: No connection for newly checking id " << getUserSessionId());
            }
            mCurrentConnectivityState = CONNECTIVITY_STATE_UNRESPONSIVE;
            gUserSessionMaster->mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionUnresponsive, *this);
        }
        // else no status change
    }
    else
    {
        EA_ASSERT(mPeerInfo->isPersistent());

        if (mPeerInfo->isResponsive())
        {
            if (mCurrentConnectivityState != CONNECTIVITY_STATE_RESPONSIVE)
            {
                mCurrentConnectivityState = CONNECTIVITY_STATE_RESPONSIVE;
                gUserSessionMaster->mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionResponsive, *this);
            }
            // else no status change
        }
        else
        {
            if (mCurrentConnectivityState != CONNECTIVITY_STATE_UNRESPONSIVE)
            {
                mCurrentConnectivityState = CONNECTIVITY_STATE_UNRESPONSIVE;
                gUserSessionMaster->mDispatcher.dispatch<const UserSessionMaster&>(&LocalUserSessionSubscriber::onLocalUserSessionUnresponsive, *this);
            }
            // else no status change
        }
    }

    if (gUserSessionManager->getClientTypeDescription(getClientType()).getEnableConnectivityChecks())
        BLAZE_INFO_LOG(Log::USER, "UserSessionMaster.handleConnectivityChange: (Exit) transitioned connectivity state(" << sConnectivityStateStr[mCurrentConnectivityState] << "), PeerInfo(" << mPeerInfo << "), UserSessionId(" << getUserSessionId() << ")");

}

} //namespace Blaze
