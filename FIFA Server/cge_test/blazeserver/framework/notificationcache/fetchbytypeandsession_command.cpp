/*************************************************************************************************/
/*!
    \file   fetchbytype_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchByTypeCommandp

    Returns notifications tagged by component id/notification id pair.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/rpc/notificationcacheslave/fetchbytypeandsession_stub.h"
#include "framework/notificationcache/filterbytype.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"

namespace Blaze
{

    class FetchByTypeAndSessionCommand : public FetchByTypeAndSessionCommandStub
    {
    public:

        FetchByTypeAndSessionCommand(Message* message, Blaze::FetchByTypeAndSessionRequest* request, NotificationCacheSlaveImpl* componentImpl)
            :FetchByTypeAndSessionCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~FetchByTypeAndSessionCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchByTypeAndSessionCommand::Errors execute() override
        {            
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_FETCH_NOTIFICATIONS_BY_SESSION))
            {
                BLAZE_WARN_LOG(Log::SYSTEM, "FetchByTypeAndSessionCommand].fetchByTypeAndSession: User [" << gCurrentUserSession->getBlazeId()
                    << "] attempted to fetch by type and session, no permission!");
                return ERR_AUTHORIZATION_REQUIRED;
            }

            uint16_t componentId = BlazeRpcComponentDb::getComponentIdByName(mRequest.getType().getComponentName());
            uint16_t notificationId = BlazeRpcComponentDb::getNotificationIdByName(componentId, mRequest.getType().getNotificationName());
            if (mRequest.getType().getCount() == 0)
            {
                return FetchByTypeAndSessionCommandStub::NOTIFICATIONCACHE_ERR_INVALID_FETCH_CRITERIA;
            }

            FilterByType filter(componentId, notificationId);
            mComponent->generateNotificationList(mRequest.getSessionIds(), mAsyncNotPtrList, filter, mRequest.getType().getCount());
            FetchAsyncNotificationList& list = mResponse.getNotificationDescs();            
            NotificationCacheSlaveImpl::createFetchAsyncNotificationListFromPtrList(list, mAsyncNotPtrList);

            return FetchByTypeAndSessionCommandStub::ERR_OK;
        }

    private:
        NotificationCacheSlaveImpl* mComponent;  // memory owned by creator, don't free
        NotificationCacheSlaveImpl::FetchAsyncNotificationPtrList mAsyncNotPtrList;
    };

    // static factory method impl
    DEFINE_FETCHBYTYPEANDSESSION_CREATE_COMPNAME(NotificationCacheSlaveImpl)

} // Blaze
