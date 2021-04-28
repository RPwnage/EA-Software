/*************************************************************************************************/
/*!
    \file notificationcacheslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"

#include "framework/notificationcache/filterbyseqno.h"
#include "framework/notificationcache/filterbytype.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

// static
NotificationCacheSlave* NotificationCacheSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Component") NotificationCacheSlaveImpl();
}

NotificationCacheSlaveImpl::NotificationCacheSlaveImpl() : mNotificationMap(BlazeStlAllocator("NotificationCacheSlaveImpl::mNotificationMap"))
{
    mNumNotificationsExpired = 0;
}

NotificationCacheSlaveImpl::~NotificationCacheSlaveImpl()
{
    
}


void* NotificationCacheSlaveImpl::allocCore(size_t nSize, void *pContext)
{
    return BLAZE_ALLOC_MGID(nSize, MEM_GROUP_FRAMEWORK_DEFAULT, "NotifyCacheNode" );
}


void NotificationCacheSlaveImpl::freeCore(void *pCore, void *pContext)
{
    BLAZE_FREE(pCore);
}


//  cache a notification EA::TDF::Tdf
//      allocate a notification list for the session
//      allocate a raw buffer for the encode the notification 
void NotificationCacheSlaveImpl::cacheNotification(Notification& notification)
{
    if (getState() != ACTIVE)
        return;

    if (mEnabledComponents.find(notification.getComponentId()) == mEnabledComponents.end())
        return;

    UserSessionCachedNotificationMap::insert_return_type inserter = mNotificationMap.insert(notification.getUserSessionId());
    UserSessionCachedNotificationList& notifyList = inserter.first->second;
    if (inserter.second)
    {
        BLAZE_TRACE_LOG(LOGGING_CATEGORY, "[NotificationCacheSlaveImpl].cacheNotification: "
            "Tracking notifications for session.");
    }

    CachedNotification* notify = new(mNotificationHeap.Malloc()) CachedNotification(notification.createSnapshot());

    notifyList.push_back(notify);

    mCachedNotificationList.push_back(*notify);
}


//  generate a list of notifications sent back to clients.
//      True, then notification is added to the passed in FetchAsyncNotificationList and removed from the session's notification cache.
//      False, then skipped.
void NotificationCacheSlaveImpl::generateNotificationList(UserSessionId sessionId, FetchAsyncNotificationPtrList& fetchList, FilterBySeqNo& filter, uint32_t limit)
{
    UserSessionCachedNotificationMap::iterator itMap = mNotificationMap.find(sessionId);
    if (itMap == mNotificationMap.end())
        return;

    UserSessionCachedNotificationList &list = itMap->second;
    uint32_t count = 0;

    for (UserSessionCachedNotificationList::iterator itList = list.begin(); itList != list.end() && count < limit; )
    {
        CachedNotification* notification = *itList;
        if (filter(*notification))
        {
            FetchAsyncNotificationPtrDesc& fetchNotificationPtrDesc = fetchList.push_back();
            fetchNotificationPtrDesc.notificationPtr = notification->getNotificationPtr();
            fetchNotificationPtrDesc.componentName = BlazeRpcComponentDb::getComponentNameById(notification->mType.getComponentId());
            fetchNotificationPtrDesc.notificationName = BlazeRpcComponentDb::getNotificationNameById(notification->mType.getComponentId(), notification->mType.getNotificationId());
            fetchNotificationPtrDesc.timeAdded = notification->mTimeAdded;
            fetchNotificationPtrDesc.sessionId = sessionId;
            fetchNotificationPtrDesc.blazeId = gUserSessionManager->getBlazeId(sessionId);

            //  detach notification from the global list and the session's cache.
            itList = list.erase(itList);                        // removing from the session's cache.
            mCachedNotificationList.remove(*notification);             // removing from the global instrusive_list

            //  free notification object
            notification->~CachedNotification();
            mNotificationHeap.Free((void*)notification);
            ++count;
        }
        else
        {
            ++itList;
        }
    }    
}

//  generates a list of notifications based on filter criteria including sessions to filter through
//  if the session list is empty, then ignore the session filter.
//  use the global notification list since we're filtering based on any arbitrary session.
//  this takes the submission order of notifications into account first, then filters notifications into the target list depending on session.
void NotificationCacheSlaveImpl::generateNotificationList(const FetchByTypeAndSessionRequest::UserSessionIdList& sessionIdFilterList, FetchAsyncNotificationPtrList& fetchList, FilterByType& filter, uint32_t limit)
{
    uint32_t count = 0;
    for (CachedNotificationList::iterator it = mCachedNotificationList.begin(); it != mCachedNotificationList.end() && count < limit;)
    {
        CachedNotification& notification = *it;
        UserSessionId userSessionId = notification.getNotificationPtr()->getUserSessionId();
        if ((sessionIdFilterList.empty() || eastl::find(sessionIdFilterList.begin(), sessionIdFilterList.end(), userSessionId) != sessionIdFilterList.end()) && filter(notification))
        {
            FetchAsyncNotificationPtrDesc& fetchNotificationPtrDesc = fetchList.push_back();
            fetchNotificationPtrDesc.notificationPtr = notification.getNotificationPtr();
            fetchNotificationPtrDesc.componentName = BlazeRpcComponentDb::getComponentNameById(notification.mType.getComponentId());
            fetchNotificationPtrDesc.notificationName = BlazeRpcComponentDb::getNotificationNameById(notification.mType.getComponentId(), notification.mType.getNotificationId());
            fetchNotificationPtrDesc.timeAdded = notification.mTimeAdded;
            fetchNotificationPtrDesc.sessionId = notification.getNotificationPtr()->getUserSessionId();
            fetchNotificationPtrDesc.blazeId = gUserSessionManager->getBlazeId(userSessionId);
            
            //  detach notification from the session cache first before erasing it from the global list (consistent with other removals.)
            UserSessionCachedNotificationMap::iterator sessionNotMapIt = mNotificationMap.find(userSessionId);
            if (EA_UNLIKELY(sessionNotMapIt == mNotificationMap.end()))
            {
                BLAZE_ERR_LOG(LOGGING_CATEGORY, "[NotificationCacheSlaveImpl].generateNotificationList: notification(" << fetchNotificationPtrDesc.componentName.c_str() << "," << fetchNotificationPtrDesc.notificationName.c_str() << 
                    " not found in session(" << userSessionId << ") notification map!");
            }
            else
            {
                UserSessionCachedNotificationList& sessionNotList = sessionNotMapIt->second;
                sessionNotList.remove(&notification);
            }

            it = mCachedNotificationList.erase(it);

            //  free notification object
            notification.~CachedNotification();
            mNotificationHeap.Free((void*)&notification);
            ++count;
        }
        else
        {
            ++it;
        }
    }
}


/*** Private Methods ******************************************************************************/

bool NotificationCacheSlaveImpl::onValidateConfig(NotificationCacheConfig& config, const NotificationCacheConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    for (NotificationCacheConfig::StringComponentNameList::const_iterator citComponent = config.getComponents().begin();
        citComponent != config.getComponents().end(); ++citComponent)
    {
        ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(citComponent->c_str());
        if (componentId == Component::INVALID_COMPONENT_ID)
        {
            BLAZE_INFO_LOG(LOGGING_CATEGORY, "[NotificationCacheSlaveImpl:onValidateConfig: component(" << citComponent->c_str() << ") not found. Notifications won't be cached.");
        }
    }
    
    return true;
}


bool NotificationCacheSlaveImpl::onConfigure()
{
    const NotificationCacheConfig& config = getConfig();

    //  read enabled components.
    mEnabledComponents.clear();
    for (NotificationCacheConfig::StringComponentNameList::const_iterator citComponent = config.getComponents().begin();
        citComponent != config.getComponents().end(); ++citComponent)
    {
        ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(citComponent->c_str());
        mEnabledComponents.insert(componentId);
    }

    //  all necessary information stored in config tdf - no additional config needed.   
    return true;
}


//  Reconfiguration
bool NotificationCacheSlaveImpl::onReconfigure()
{
    return onConfigure();
}


//  component startup
bool NotificationCacheSlaveImpl::onActivate()
{
    //  fixed heap for notifications.
    mNotificationHeap.Init(1024, nullptr, 0, &NotificationCacheSlaveImpl::allocCore, &NotificationCacheSlaveImpl::freeCore, (void *)this); 

    gUserSessionManager->addSubscriber(*this);

    TimeValue now = TimeValue::getTimeOfDay();
    Fiber::CreateParams callParams(Fiber::STACK_SMALL);
    gSelector->scheduleFiberTimerCall(now +  getConfig().getExpiryTime(), this, &NotificationCacheSlaveImpl::reapNotifications, "NotificationCacheSlaveImpl::reapNotifications",
                    callParams);
    return true;
}


void NotificationCacheSlaveImpl::onShutdown()
{
    if (gUserSessionManager != nullptr)
    {
        gUserSessionManager->removeSubscriber(*this);
    }

    //  free notifications in cache.
    for (CachedNotificationList::iterator notificationListIt = mCachedNotificationList.begin(), notificationListItEnd = mCachedNotificationList.end();
            notificationListIt != notificationListItEnd;
            ++notificationListIt)
    {
        CachedNotification& notify = *notificationListIt;
        notify.~CachedNotification();
    }

    mCachedNotificationList.clear();
    mNotificationMap.clear();
    mNotificationHeap.Shutdown();
}


void NotificationCacheSlaveImpl::getStatusInfo(ComponentStatus& status) const
{   
    ComponentStatus::InfoMap& map = status.getInfo();
    char8_t value[64];

    blaze_snzprintf(value, sizeof(value), "%" PRIu64, mNumNotificationsExpired);
    map["TotalNotificationsExpired"] = value;

    blaze_snzprintf(value, sizeof(value), "%" PRIsize, (size_t)mNotificationMap.size());
    map["GaugeTrackedUsers"] = value;

    typedef eastl::vector_map<CachedNotificationType, uint32_t> NotificationCounters;
    NotificationCounters gaugeCachedNotifications;
    size_t gaugeTotalCachedNotifications = 0;

    for (UserSessionCachedNotificationMap::const_iterator citMap = mNotificationMap.begin(); citMap != mNotificationMap.end(); ++citMap)
    {
        const UserSessionCachedNotificationList& list = citMap->second;

        for (UserSessionCachedNotificationList::const_iterator citNot = list.begin(); citNot != list.end(); ++citNot)
        {
            const CachedNotification* notify = *citNot;
            eastl::pair<NotificationCounters::iterator, bool> inserter = gaugeCachedNotifications.insert(NotificationCounters::value_type(notify->mType, 0));
            ++inserter.first->second;
        }

        gaugeTotalCachedNotifications += list.size();
    }

    blaze_snzprintf(value, sizeof(value), "%" PRIsize, gaugeTotalCachedNotifications);
    map["GaugeCachedNotifications"] = value;

    for (NotificationCounters::const_iterator citCounter = gaugeCachedNotifications.begin(); citCounter < gaugeCachedNotifications.end(); ++citCounter)
    {
        char8_t notnamevalue[128];
        blaze_snzprintf(notnamevalue, sizeof(notnamevalue), "Gauge_%s_%s",
            BlazeRpcComponentDb::getComponentNameById(citCounter->first.getComponentId()),
            BlazeRpcComponentDb::getNotificationNameById(citCounter->first.getComponentId(), citCounter->first.getNotificationId())
            );
        blaze_snzprintf(value, sizeof(value), "%u", citCounter->second);
        map[notnamevalue] = value;
    }
}



void NotificationCacheSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
{
    //  clear session data.
    clearSessionData(userSession.getUserSessionId());
}


//  Internal Housekeeping methods.
void NotificationCacheSlaveImpl::reapNotifications()
{
    if (getState() != ACTIVE)
        return;

    const TimeValue NOTIFICATION_TIME_LIMIT = getConfig().getExpiryTime();
    TimeValue now = TimeValue::getTimeOfDay();
    uint64_t numNotificationsExpired = mNumNotificationsExpired;

    //  reap old, unfetched notifications from each session's tracking list.
    for (UserSessionCachedNotificationMap::iterator itMap = mNotificationMap.begin(); itMap != mNotificationMap.end(); ++itMap)
    {
        UserSessionCachedNotificationList &list = itMap->second;

        //  entries at the list head are the going to be the oldest.  Iterate through the list while there are still old notifications
        //  remaining
        while (!list.empty())
        {
            CachedNotification* notify = list.front();
            if ((notify->mTimeAdded + NOTIFICATION_TIME_LIMIT) > now)
            {
                //  notification has not expired.  no need to check later notifications
                break;
            }

            //  expired notification - remove from the session's list and the global list.
            list.pop_front();
            mCachedNotificationList.remove(*notify);
            //  free notification memory
            notify->~CachedNotification();
            mNotificationHeap.Free((void *)notify);

            ++numNotificationsExpired;
        }
    }

    if (numNotificationsExpired != mNumNotificationsExpired)
    {
        BLAZE_TRACE_LOG(LOGGING_CATEGORY, "[NotificationCacheSlaveImpl].reapNotifications: "
            << (numNotificationsExpired - mNumNotificationsExpired) << " expired this iteration (total=" << numNotificationsExpired << ")."
            );
        mNumNotificationsExpired = numNotificationsExpired;
    }

    //  re-schedule notification list reaping.
    Fiber::CreateParams callParams(Fiber::STACK_SMALL);
    gSelector->scheduleFiberTimerCall(now + NOTIFICATION_TIME_LIMIT, this, &NotificationCacheSlaveImpl::reapNotifications, "NotificationCacheSlaveImpl::reapNotifications",
                    callParams);
}


void NotificationCacheSlaveImpl::clearSessionData(UserSessionId sessionId)
{
    //  kill notifications for the session.
    UserSessionCachedNotificationMap::iterator notificationListIt = mNotificationMap.find(sessionId);
    if (notificationListIt != mNotificationMap.end())
    {
        UserSessionCachedNotificationList &list = (notificationListIt->second);
        for (UserSessionCachedNotificationList::iterator it = list.begin(), itEnd = list.end(); it != itEnd; ++it)
        {
            CachedNotification* notify = *it;
            mCachedNotificationList.remove(*notify);           // remove from the global list.
            notify->~CachedNotification();
            mNotificationHeap.Free((void*)(notify));
        }
        //  list no longer needed since session has been cleared.
        mNotificationMap.erase(notificationListIt);
    }
}

} //namespace Blaze
