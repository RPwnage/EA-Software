/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SESSION_MASTER_H
#define BLAZE_USER_SESSION_MASTER_H

/*** Include files *******************************************************************************/
#include "framework/rpc/usersessionsslave_stub.h"
#include "framework/usersessions/usersession.h"
#include "framework/slivers/ownedsliver.h"

#include "EASTL/intrusive_hash_map.h"
#include "EASTL/intrusive_list.h"

namespace Blaze
{

class NotificationJobQueue;
struct ConnectionGroupIdListNode : public eastl::intrusive_list_node {};

class UserSessionMaster :
    public UserSession,
    public ConnectionGroupIdListNode
{
    NON_COPYABLE(UserSessionMaster);
public:
    ~UserSessionMaster() override;

    const UserInfo& getUserInfo() const { return *mUserInfo; }

    void filloutUserIdentification(UserIdentification& userIdentification) const;
    void filloutUserCoreIdentification(CoreIdentification& coreIdentification) const;

    ConnectionGroupObjectId getConnectionGroupObjectId() const;
    EA::TDF::ObjectId getLocalUserGroupObjectId() const;

    /*! \brief Gets the key of the session. */
    void getKey(char8_t (&keyString)[MAX_SESSION_KEY_LENGTH]) const;

    /*! \brief Returns whether or not a given key matches this session. */  
    bool validateKeyHashSum(const char8_t* keyHashSum) const;
    
    BlazeRpcError DEFINE_ASYNC_RET(updateDataValue(UserExtendedDataKey& key, UserExtendedDataValue value));
    BlazeRpcError DEFINE_ASYNC_RET(updateDataValue(ComponentId componentId, uint16_t key, UserExtendedDataValue value));
    bool getDataValue(UserExtendedDataKey& key, UserExtendedDataValue &value) const;
    bool getDataValue(ComponentId componentId, uint16_t key, UserExtendedDataValue &value) const;

    // TODO: PeerInfo::isPersistent() related path should be unified when we get around to EADP SDK integration and
    // redesign the server to work with 'persistent but transient' bi-directional connection channels.
    
    // If peerInfo is nullptr, user session is currently migrating.
    PeerInfo* getPeerInfo() const { return mPeerInfo; }
    void attachPeerInfo(PeerInfo& peerInfo);
    void detachPeerInfo();

    /*! \brief forces a disconnect for this user session's connection. For testing purposes only. */
    void forceDisconnect();

    void sendNotification(Notification& notification);
    void sendNotificationInternal(Notification& notification);
    static void sendNotificationInternalAsync(UserSessionId userSessionId, NotificationPtr notification);
    void sendPendingNotificationsByComponent(ComponentId componentId);

    /*! \brief Returns true if the underlying connection implementation is currently blocking outgoing notifications. Can be used as a hint to callers of sendNotification() */
    bool isBlockingNotifications() const;

    /*! \brief Returns true if the underlying connection implementation is currently blocking lower priority notifications.
               Can be used as a hint to callers of sendNotification() that while the connection not squelched, it has a lot of queued data. */
    bool isBlockingNonPriorityNotifications() const;

    void setOrUpdateCleanupTimeout();
    static void handleCleanupTimeoutAsync(UserSessionId userSessionId);
    void cancelCleanupTimeout();

    void setOwnedSliverRef(OwnedSliver* ownedSliver) { mOwnedSliverRef = ownedSliver; }

    bool isCoalescedUedUpdatePending() const { return (mCoalescedUedUpdateTimerId != INVALID_TIMER_ID); }

    bool hasOutstandingSubscriptionJobs() const;
    void onSubscriptionJobQueueOpened();
    void onSubscriptionJobQueueDrained();

    BlazeRpcError addUser(BlazeId blazeId, bool sendImmediately = true, bool includeSessionSubscription = false);
    BlazeRpcError addUsers(const BlazeIdList& blazeIds, bool sendImmediately = true, bool includeSessionSubscription = false);
    void addUsersAsync(const BlazeIdList& blazeIds, bool includeSessionSubscription);
    static void finish_addUsersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<BlazeIdList> _blazeIds, bool includeSessionSubscription);

    BlazeRpcError removeUser(BlazeId blazeId, bool sendImmediately = true, bool includeSessionSubscription = false);
    BlazeRpcError removeUsers(const BlazeIdList& blazeIds, bool sendImmediately = true, bool includeSessionSubscription = false);
    void removeUsersAsync(const BlazeIdList& blazeIds, bool includeSessionSubscription);
    static void finish_removeUsersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<BlazeIdList> _blazeIds, bool includeSessionSubscription);

    BlazeRpcError addNotifier(UserSessionId notifierId, bool sendImmediately = true);
    BlazeRpcError addNotifiers(const UserSessionIdList& notifierIds, bool sendImmediately = true);
    void addNotifiersAsync(const UserSessionIdList& notifierIds);
    static void finish_addNotifiersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds);

    BlazeRpcError removeNotifier(UserSessionId notifierId, bool sendImmediately = true);
    BlazeRpcError removeNotifiers(const UserSessionIdList& notifierIds, bool sendImmediately = true);
    void removeNotifiersAsync(const UserSessionIdList& notifierIds);
    static void finish_removeNotifiersAsync(UserSessionId userSessionId, EA::TDF::tdf_ptr<UserSessionIdList> _notifierIds);

    inline UserSessionDataPtr getUserSessionData() { return mUserSessionData; }
    inline const UserSessionDataPtr getUserSessionData() const { return mUserSessionData; }

    inline UserSessionId getChildSessionId() const { return mUserSessionData->getChildSessionId(); }
    inline void setChildSessionId(UserSessionId val) {  mUserSessionData->setChildSessionId(val); }

    inline UserSessionId getParentSessionId() const { return mUserSessionData->getParentSessionId(); }
    inline void setParentSessionId(UserSessionId val) {  mUserSessionData->setParentSessionId(val); }

    inline EA::TDF::TimeValue& getCreationTime() {  return mUserSessionData->getCreationTime(); }
    inline const EA::TDF::TimeValue& getCreationTime() const { return mUserSessionData->getCreationTime(); }
    inline void setCreationTime(const EA::TDF::TimeValue& val) {  mUserSessionData->setCreationTime(val); }

    inline UserSessionExtendedData& getExtendedData() { return mUserSessionData->getExtendedData(); }
    inline const UserSessionExtendedData& getExtendedData() const { return mUserSessionData->getExtendedData(); }

    inline float getLatitude() const { return mUserSessionData->getLatitude(); }
    inline void setLatitude(float val) {  mUserSessionData->setLatitude(val); }

    inline float getLongitude() const { return mUserSessionData->getLongitude(); }
    inline void setLongitude(float val) {  mUserSessionData->setLongitude(val); }

    inline ClientMetrics& getClientMetrics() { return mUserSessionData->getClientMetrics(); }
    inline const ClientMetrics& getClientMetrics() const { return mUserSessionData->getClientMetrics(); }

    inline ClientUserMetrics& getClientUserMetrics() { return mUserSessionData->getClientUserMetrics(); }
    inline const ClientUserMetrics& getClientUserMetrics() const { return mUserSessionData->getClientUserMetrics(); }

    inline UserSessionAttributeMap& getServerAttributes() { return mUserSessionData->getServerAttributes(); }
    inline const UserSessionAttributeMap& getServerAttributes() const { return mUserSessionData->getServerAttributes(); }

    inline ConnectionGroupId getConnectionGroupId() const { return mUserSessionData->getConnectionGroupId(); }
    inline void setConnectionGroupId(ConnectionGroupId val) {  mUserSessionData->setConnectionGroupId(val); }

    inline const char8_t* getConnectionAddr() const { return mUserSessionData->getConnectionAddr(); }
    inline void setConnectionAddr(const char8_t* val) {  mUserSessionData->setConnectionAddr(val); }

    inline int32_t getDirtySockUserIndex() const { return mUserSessionData->getDirtySockUserIndex(); }
    inline void setDirtySockUserIndex(int32_t val) {  mUserSessionData->setDirtySockUserIndex(val); }

    inline uint32_t getConnectionUserIndex() const { return mUserSessionData->getConnectionUserIndex(); }
    inline void setConnectionUserIndex(uint32_t val) {  mUserSessionData->setConnectionUserIndex(val); }

    inline ClientState& getClientState() { return mUserSessionData->getClientState(); }
    inline const ClientState& getClientState() const { return mUserSessionData->getClientState(); }

    inline Int256Buffer& getRandom() { return mUserSessionData->getRandom(); }
    inline const Int256Buffer& getRandom() const { return mUserSessionData->getRandom(); }

    inline UserSessionData::Int32ByUserSessionId& getSubscribedSessions() { return mUserSessionData->getSubscribedSessions(); }
    inline const UserSessionData::Int32ByUserSessionId& getSubscribedSessions() const { return mUserSessionData->getSubscribedSessions(); }

    inline UserSessionData::Int32ByBlazeId& getSubscribedUsers() { return mUserSessionData->getSubscribedUsers(); }
    inline const UserSessionData::Int32ByBlazeId& getSubscribedUsers() const { return mUserSessionData->getSubscribedUsers(); }

    inline NotificationDataList& getPendingNotificationList() { return mUserSessionData->getPendingNotificationList(); }
    inline const NotificationDataList& getPendingNotificationList() const { return mUserSessionData->getPendingNotificationList(); }

    inline ClientInfo& getClientInfo() { return mUserSessionData->getClientInfo(); }
    inline const ClientInfo& getClientInfo() const { return mUserSessionData->getClientInfo(); }

    inline const char8_t* getUUID() const { return mUserSessionData->getUUID(); }
    inline void setUUID(const char8_t* val) { mUserSessionData->setUUID(val); }

    template <class T>
    EA::TDF::tdf_ptr<T> getCustomStateTdf(ComponentId componentId) const
    {
        TdfPtrByComponentId::const_iterator it = mCustomStateTdfByComponentId.find(componentId);
        if (it == mCustomStateTdfByComponentId.end())
            return nullptr;

        EA::TDF::TdfId tdfId = it->second->getTdfId();
        if (T::TDF_ID != tdfId)
        {
            BLAZE_WARN_LOG(UserSessionsMaster::LOGGING_CATEGORY, "UserSessionMaster.getCustomStateTdf: Expected TdfId(" << T::TDF_ID << ") mismatches stored TdfId(" << tdfId << ") "
                "for UserSessionId(" << getUserSessionId() << "), ComponentId(" << componentId << ").");
            return nullptr;
        }
        return static_cast<T*>(it->second.get());
    }

    void upsertCustomStateTdf(ComponentId componentId, EA::TDF::Tdf& tdf);

    bool isConnectionUnresponsive() const { return (mCurrentConnectivityState == CONNECTIVITY_STATE_UNRESPONSIVE); }
    
    // Clients can be configured to check for the changes in their connectivity periodically (See ClientTypeFlags::setEnableConnectivityChecks). As of writing, dedicated server(CLIENT_TYPE_DEDICATED_SERVER)
    // are set up to check their connectivity periodically. 
    // 1. Connectivity check layer calls handleConnectivityChange if there is a change in info.
    // 2. During session migration, if the connectivity check is enabled, the following is also called internally from a timer. This allows for marking the connection unresponsive until the client reconnects. 
    static void handleConnectivityChangeAsync(UserSessionId userSessionId);
    void handleConnectivityChange();

    // crossplay accessors

    /*! \brief returns true if the user is opted-in to crossplay */
    bool isCrossplayEnabled() const { return mUserInfo->getCrossPlatformOptIn(); }
    /*! ***************************************************************************/
    /*! \brief returns true if the user is allowed to interact with the passed in client platform type

        Always returns true if the targetPlatform is the UserSession's current platform.
        Otherwise, returns true if the UserSession is opted into crossplay, and the target platform is in this User's platformRestrictions list

        \param targetPlatform - the platform we're querying
        \return - true if the interaction is allowed.
    *******************************************************************************/
    bool isAllowedToInteractWithPlatform(ClientPlatformType targetPlatform) const;

    /*! ***************************************************************************/
    /*! \brief returns a list of platforms this usersession can interact with

        \return - the configured platformRestrictions list for this user if opted into crossplay, the user's current platform if the user is opted-out
    *******************************************************************************/
   // const ClientPlatformSet& getAllowedPlatforms() const;


private:
    friend class UserSessionsMasterImpl;

    UserSessionMaster(UserSessionId userSessionId, UserInfoPtr userInfo);
    // UserSessionMaster(const UserSessionMaster& initializeFrom); Note, this constructor is already defined by the NON_COPYABLE macro.

    inline void setUserSessionId(UserSessionId userSessionId) { mExistenceData->setUserSessionId(userSessionId); mUserSessionData->setUserSessionId(userSessionId); }
    inline void setUserSessionType(UserSessionType val) { mExistenceData->setUserSessionType(val); }
    inline void setBlazeId(BlazeId val) { mExistenceData->setBlazeId(val); }
    inline void setUserId(BlazeId val) { setBlazeId(val); } // DEPRECATED
    inline void setAccountId(AccountId val) { mExistenceData->getPlatformInfo().getEaIds().setNucleusAccountId(val); }
    inline void setPersonaName(const char8_t* val) { mExistenceData->setPersonaName(val); }
    inline void setPersonaNamespace(const char8_t* val) { mExistenceData->setPersonaNamespace(val); }
    inline void setClientPlatform(ClientPlatformType val) { mExistenceData->getPlatformInfo().setClientPlatform(val); }
    inline void setUniqueDeviceId(const char8_t* val) { mExistenceData->setUniqueDeviceId(val); }
    inline void setDeviceLocality(DeviceLocality val) { mExistenceData->setDeviceLocality(val); }
    inline void setClientType(ClientType val) { mExistenceData->setClientType(val); }
    inline void setClientVersion(const char8_t* val) { mExistenceData->setClientVersion(val); }
    inline void setBlazeSDKVersion(const char8_t* val) { mExistenceData->setBlazeSDKVersion(val); }
    inline void setProtoTunnelVer(const char8_t* val) { mExistenceData->setProtoTunnelVer(val); }
    inline void setSessionLocale(uint32_t val) { mExistenceData->setSessionLocale(val); }
    inline void setSessionCountry(uint32_t val) { mExistenceData->setSessionCountry(val); }
    inline void setAccountLocale(uint32_t val) { mExistenceData->setAccountLocale(val); }
    inline void setAccountCountry(uint32_t val) { mExistenceData->setAccountCountry(val); }
    inline void setPreviousAccountCountry(uint32_t val) { mExistenceData->setPreviousAccountCountry(val); }
    inline void setServiceName(const char8_t* val) { mExistenceData->setServiceName(val); }
    inline void setProductName(const char8_t* val) { mExistenceData->setProductName(val); }
    inline void setAuthGroupName(const char8_t* val) { mExistenceData->setAuthGroupName(val); }

    void changeUserInfo(UserInfoPtr userInfo);

    void removeFromConnectionGroupList();

    BlazeRpcError waitForJobQueueToFinish();

    static void sendCoalescedExtendedDataUpdateAsync(UserSessionId userSessionId);
    void sendCoalescedExtendedDataUpdate(bool isCoalescedUpdateSentinel = false);
    void scheduleCoalescedExtendedDataUpdate();
    void cancelCoalescedExtendedDataUpdate();

    void insertComponentStateTdf(ComponentId componentId, const EA::TDF::TdfPtr& tdfPtr);
    void eraseComponentStateTdf(ComponentId componentId);

    void enableConnectivityChecking();
    void disableConnectivityChecking();

private:
    UserInfoPtr mUserInfo;

    UserSessionDataPtr mUserSessionData;

    PeerInfo* mPeerInfo;
    // When adding jobs to this FiberJobQueue, pass this UserSessionId as an argument to a static method, rather than
    // a pointer-to-member of 'this'.  See finish_addUsersAsync() for an example.
    NotificationJobQueue* mNotificationJobQueue;

    OwnedSliverRef mOwnedSliverRef;

    TimerId mCleanupTimerId;

    TimerId mCoalescedUedUpdateTimerId;

    typedef eastl::vector_map<ComponentId, EA::TDF::TdfPtr> TdfPtrByComponentId;
    TdfPtrByComponentId mCustomStateTdfByComponentId;

    // Connectivity state of this user session
    enum ConnectivityState
    {
        CONNECTIVITY_STATE_CHECK_DISABLED, // Connectivity state checking is disabled
        CONNECTIVITY_STATE_PENDING_DETERMINATION, // Connectivity state is pending determination
        CONNECTIVITY_STATE_RESPONSIVE, // Connectivity state is responsive
        CONNECTIVITY_STATE_UNRESPONSIVE // Connectivity state is unresponsive
    };

    ConnectivityState mCurrentConnectivityState;
    TimerId mConnectivityStateCheckTimerId; // The timer id for the connectivity state check. Used during session migration if the user session has connectivity checking enabled.

    bool isConnectivityCheckingEnabled() const { return (mCurrentConnectivityState != CONNECTIVITY_STATE_CHECK_DISABLED); }

    friend void intrusive_ptr_add_ref(UserSessionMaster*);
    friend void intrusive_ptr_release(UserSessionMaster*);
};

typedef eastl::intrusive_list<UserSessionMaster> ConnectionGroupList;

typedef eastl::intrusive_ptr<UserSessionMaster> UserSessionMasterPtr;
extern FiberLocalWrappedPtr<UserSessionMasterPtr, UserSessionMaster> gCurrentLocalUserSession;

void intrusive_ptr_add_ref(UserSessionMaster* ptr);
void intrusive_ptr_release(UserSessionMaster* ptr);

class NotificationJobQueue :
    public FiberJobQueue
{
public:
    NotificationJobQueue(const char8_t* namedContext) :
        FiberJobQueue(namedContext),
        mSliverId(INVALID_SLIVER_ID)
    {
        setJobFiberStackSize(Fiber::STACK_SMALL);
        setDefaultJobTimeout(10 * 1000 * 1000);
        setQueueEventCallbacks(
            QueueEventCb(this, &NotificationJobQueue::onSubscriptionJobQueueOpened),
            QueueEventCb(this, &NotificationJobQueue::onSubscriptionJobQueueDrained));
    }

    ~NotificationJobQueue() override
    {
    }

private:
    void onSubscriptionJobQueueOpened();
    void onSubscriptionJobQueueDrained();

private:
    friend class UserSessionsMasterImpl;
    friend class UserSessionMaster;

    SliverId mSliverId;
    Sliver::AccessRef mSliverAccess;
};

} //namespace Blaze

#endif  // BLAZE_USER_SESSION_MASTER_H

