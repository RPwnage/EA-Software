/*************************************************************************************************/
/*!
    \file   fetchbyseqno_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class fetchbyseqno_command.cpp

    Returns notifications tagged by sequence number for the current user session.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/rpc/notificationcacheslave/fetchbyseqno_stub.h"
#include "framework/notificationcache/filterbyseqno.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"

namespace Blaze
{

    class FetchBySeqNoCommand : public FetchBySeqNoCommandStub
    {
    public:

       FetchBySeqNoCommand(Message* message, Blaze::FetchBySeqNoRequest* request, NotificationCacheSlaveImpl* componentImpl)
            :FetchBySeqNoCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~FetchBySeqNoCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchBySeqNoCommandStub::Errors execute() override
        {            
            if (mRequest.getCount() == 0)
            {
                return FetchBySeqNoCommandStub::NOTIFICATIONCACHE_ERR_INVALID_FETCH_CRITERIA;
            }

            FilterBySeqNo filter(mRequest.getSeqNo());
            mComponent->generateNotificationList(gCurrentUserSession->getSessionId(), mAsyncNotPtrList, filter, mRequest.getCount());
            
            FetchAsyncNotificationList& list = mResponse.getNotificationDescs();
            NotificationCacheSlaveImpl::createFetchAsyncNotificationListFromPtrList(list, mAsyncNotPtrList);

            return FetchBySeqNoCommandStub::ERR_OK;
        }

    private:
        NotificationCacheSlaveImpl* mComponent;  // memory owned by creator, don't free
        NotificationCacheSlaveImpl::FetchAsyncNotificationPtrList mAsyncNotPtrList;
    };

    // static factory method impl
    DEFINE_FETCHBYSEQNO_CREATE_COMPNAME(NotificationCacheSlaveImpl)

} // Blaze
