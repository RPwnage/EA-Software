/*************************************************************************************************/
/*!
    \file   notificationcacheslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FILTER_BY_SEQ_NO_H
#define BLAZE_FILTER_BY_SEQ_NO_H

/*** Include files *******************************************************************************/
#include "framework/notificationcache/notificationcacheslaveimpl.h"

namespace Blaze
{

class FilterBySeqNo
{
    uint32_t mSeqNo;

public:
    FilterBySeqNo(uint32_t seqNo) : mSeqNo(seqNo) {}
    bool operator()(const NotificationCacheSlaveImpl::CachedNotificationBase& notify) const
    {
        return (notify.mSeqNo == mSeqNo);
    }
};

}// Blaze

#endif //BLAZE_FILTER_BY_SEQ_NO_H

