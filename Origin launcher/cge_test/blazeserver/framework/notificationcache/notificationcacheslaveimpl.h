/*************************************************************************************************/
/*!
    \file   notificationcacheslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_NOTIFICATION_CACHE_IMPL_H
#define BLAZE_NOTIFICATION_CACHE_IMPL_H

/*** Include files *******************************************************************************/

#include "framework/rpc/notificationcacheslave_stub.h"
#include "framework/tdf/notificationcache_server.h"
#include "framework/tdf/entity.h"                                 // for BlazeId
#include "framework/tdf/notifications_server.h"                   // for NotificationId

#include "framework/usersessions/usersessionsubscriber.h"

#include "PPMalloc/EAFixedAllocator.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector_set.h"


namespace Blaze
{

class FilterBySeqNo;
class FilterByType;
class UserSession;

//  NotificationCache Slave Implementation
//      Manages the notification cache for each user session residing on the slave.


class NotificationCacheSlaveImpl :
    public NotificationCacheSlaveStub,
    private UserSessionSubscriber
{
public:
    struct CachedNotificationType : eastl::pair<uint16_t, uint16_t> 
    {
        CachedNotificationType() {}
        CachedNotificationType(ComponentId componentId, NotificationId notificationId) { set(componentId, notificationId); }
        void set(ComponentId componentId, NotificationId notificationId) { first = componentId; second = notificationId; }
        ComponentId getComponentId() const { return first; }
        NotificationId getNotificationId() const { return second; }

        //  if needed by eastl, requires a custom implementation
        bool operator<(const CachedNotificationType& nt) const {
            if (getComponentId() != nt.getComponentId())
                return (getComponentId() < nt.getComponentId());

            if (getNotificationId() != nt.getNotificationId())
                return (getNotificationId() < nt.getNotificationId());

            return false;
        }
    };

public:
    NotificationCacheSlaveImpl();
    ~NotificationCacheSlaveImpl() override;

    //  cache a notification EA::TDF::Tdf
    void cacheNotification(Notification& notification);

    //  generate a list of notifications sent back to clients.
    //      True, then notification is added to the passed in FetchAsyncNotificationList and removed from the session's notification cache.
    //      False, then skipped.
    struct CachedNotificationBase
    {
        CachedNotificationBase(ComponentId componentId, NotificationId notificationId, uint32_t seqNo) :
            mType(componentId, notificationId),
            mTimeAdded(EA::TDF::TimeValue::getTimeOfDay()),
            mSeqNo(seqNo)
        {}

        CachedNotificationType mType;
        EA::TDF::TimeValue mTimeAdded;
        uint32_t mSeqNo;
    };
    struct FetchAsyncNotificationPtrDesc
    {
        NotificationPtr notificationPtr;
        ComponentName componentName;
        NotificationName notificationName;
        EA::TDF::TimeValue timeAdded;
        UserSessionId sessionId;
        BlazeId blazeId;
    };
    typedef eastl::vector < FetchAsyncNotificationPtrDesc > FetchAsyncNotificationPtrList;

    void generateNotificationList(UserSessionId sessionId, FetchAsyncNotificationPtrList& list, FilterBySeqNo& filter, uint32_t limit);
    void generateNotificationList(const FetchByTypeAndSessionRequest::UserSessionIdList& sessionIdFilterList, FetchAsyncNotificationPtrList& list, FilterByType& filter, uint32_t limit);

    static void createFetchAsyncNotificationListFromPtrList(FetchAsyncNotificationList& list, const FetchAsyncNotificationPtrList& asyncNotPtrList)
    {
        if (!asyncNotPtrList.empty())
        {
            list.reserve(asyncNotPtrList.size());
            for (NotificationCacheSlaveImpl::FetchAsyncNotificationPtrList::const_iterator it = asyncNotPtrList.begin(), itEnd = asyncNotPtrList.end();
                it != itEnd; ++it)
            {
                FetchAsyncNotificationDesc *desc = list.pull_back();
                desc->setComponentName(it->componentName.c_str());
                desc->setNotificationName(it->notificationName.c_str());
                desc->setTimeSent(it->timeAdded);
                desc->setSessionId(it->sessionId);
                desc->setBlazeId(it->blazeId);

                EA::TDF::TdfPtr tdf;
                if ((it->notificationPtr->extractPayloadTdf(tdf) == ERR_OK) && (tdf != nullptr))
                    desc->setNotification(*tdf);
            }
        }
    }

private:
    //  Configuration Validation
    bool onValidateConfig(NotificationCacheConfig& config, const NotificationCacheConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    //  Configuration
    bool onConfigure() override;
    //  Reconfiguration
    bool onReconfigure() override;
    //  Activation
    bool onActivate() override;
    //  Cleanup
    void onShutdown() override;
    //  Component Status Event Handler
    void getStatusInfo(ComponentStatus& status) const override;

private:    
    // UserSessionSubscriber interface functions
    void onUserSessionExtinction(const UserSession& userSession) override;

private:
    void clearSessionData(UserSessionId sessionId);
    void reapNotifications();           // timer call to free out-of-date notifications.
    
    //  EAFixedAllocator allocation hooks
    static void* allocCore(size_t nSize, void *pContext);
    static void freeCore(void *pCore, void *pContext);
  
    //  Contains the notification TDF and metadata.
    //  AsyncNotificationPtr contains a copy of the notification payload originating by the publisher's sendNotification calls.
    //  So two or more user sessions will share the same cloned EA::TDF::Tdf, freed once fetched or expired by the last session owning it.
    //  The CachedNotification is a node in the store for all notifications (the store is an intrusive list)
    //  A CachedNotification is also list date
    class CachedNotification : public CachedNotificationBase, public eastl::intrusive_list_node
    {
    public:
        CachedNotification(NotificationPtr notification) :
            CachedNotificationBase(notification->getComponentId(), notification->getNotificationId(), notification->getOpts().msgNum),
            mAsyncNotification(notification)
        {}

        NotificationPtr& getNotificationPtr() { return mAsyncNotification; }

    private:
        NotificationPtr mAsyncNotification;
    };

    EA::Allocator::FixedAllocator<CachedNotification> mNotificationHeap;       

    typedef eastl::list<CachedNotification*> UserSessionCachedNotificationList;
    typedef eastl::hash_map<UserSessionId, UserSessionCachedNotificationList> UserSessionCachedNotificationMap;
    UserSessionCachedNotificationMap mNotificationMap;

    //  ordered by time - most recently submitted is at the tail end of the list
    typedef eastl::intrusive_list<CachedNotification> CachedNotificationList;
    CachedNotificationList mCachedNotificationList;

    typedef eastl::vector_set<ComponentId> ComponentSet;
    ComponentSet mEnabledComponents;

    uint64_t mNumNotificationsExpired;
    bool mIsActive;
};

}

#endif //BLAZE_USER_SESSIONS_MASTER_IMPL_H
