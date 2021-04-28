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
#include "framework/rpc/notificationcacheslave/fetchbytype_stub.h"
#include "framework/notificationcache/filterbytype.h"
#include "framework/notificationcache/notificationcacheslaveimpl.h"

namespace Blaze
{

    class FetchByTypeCommand : public FetchByTypeCommandStub
    {
    public:

        FetchByTypeCommand(Message* message, Blaze::FetchByTypeRequest* request, NotificationCacheSlaveImpl* componentImpl)
            :FetchByTypeCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~FetchByTypeCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchByTypeCommandStub::Errors execute() override
        {            
            uint16_t componentId = BlazeRpcComponentDb::getComponentIdByName(mRequest.getComponentName());
            uint16_t notificationId = BlazeRpcComponentDb::getNotificationIdByName(componentId, mRequest.getNotificationName());
            if (mRequest.getCount() == 0)
            {
                return FetchByTypeCommandStub::NOTIFICATIONCACHE_ERR_INVALID_FETCH_CRITERIA;
            }

            FilterByType filter(componentId, notificationId);
            FetchByTypeAndSessionRequest::UserSessionIdList sessionIdFilterList;
            sessionIdFilterList.push_back(gCurrentUserSession->getSessionId());
            mComponent->generateNotificationList(sessionIdFilterList, mAsyncNotPtrList, filter, mRequest.getCount());
            FetchAsyncNotificationList& list = mResponse.getNotificationDescs();            
            
            NotificationCacheSlaveImpl::createFetchAsyncNotificationListFromPtrList(list, mAsyncNotPtrList);

            return FetchByTypeCommandStub::ERR_OK;
        }

    private:
        NotificationCacheSlaveImpl* mComponent;  // memory owned by creator, don't free
        NotificationCacheSlaveImpl::FetchAsyncNotificationPtrList mAsyncNotPtrList;
    };

    // static factory method impl
    DEFINE_FETCHBYTYPE_CREATE_COMPNAME(NotificationCacheSlaveImpl)

} // Blaze
