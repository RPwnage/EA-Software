/*************************************************************************************************/
/*!
    \file   notificationcacheslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FILTER_BY_TYPE_H
#define BLAZE_FILTER_BY_TYPE_H

/*** Include files *******************************************************************************/

#include "framework/notificationcache/notificationcacheslaveimpl.h"

namespace Blaze
{

class FilterByType
{
    uint16_t mComponentId, mNotificationId;

public:
    FilterByType(uint16_t componentId, uint16_t notificationId) : 
        mComponentId(componentId)
        , mNotificationId(notificationId)
        {}
    bool operator()(const NotificationCacheSlaveImpl::CachedNotificationBase& notify) const
    {
        if (mComponentId == Component::INVALID_COMPONENT_ID && mNotificationId == Component::INVALID_NOTIFICATION_ID)
        {
            return true;
        }
        else if (notify.mType.getComponentId() == mComponentId && (notify.mType.getNotificationId() == mNotificationId || mNotificationId == Component::INVALID_NOTIFICATION_ID))
        {
            return true;
        }
        return false;
    }
};

}// Blaze

#endif //BLAZE_FILTER_BY_TYPE_H

