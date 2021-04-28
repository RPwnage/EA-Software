/*************************************************************************************************/
/*!
    \file   getuserids_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/fetchusersessiondata_stub.h"

namespace Blaze
{
namespace Arson
{
class FetchUserSessionDataCommand : public FetchUserSessionDataCommandStub
{
public:
    FetchUserSessionDataCommand(
        Message* message, FetchUserSessionDataRequest *mRequest, ArsonSlaveImpl* componentImpl)
        : FetchUserSessionDataCommandStub(message, mRequest),
        mComponent(componentImpl)
    {
    }

    ~FetchUserSessionDataCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    FetchUserSessionDataCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ERR_OK;
        FetchUserSessionsRequest request;
        FetchUserSessionsResponse response;

        request.getUserSessionIds().push_back(getUserSessionId());

        Blaze::RpcCallOptions opts;
        opts.routeTo.setSliverIdentityFromKey(Blaze::UserSessionsMaster::COMPONENT_ID, getUserSessionId());
        err = gUserSessionManager->getMaster()->fetchUserSessions(request, response, opts);

        UserSessionPtr userSessionPtr;
        if (err == Blaze::ERR_OK)
        {
            UserSessionToArsonResponse(response.getData().front().get());
        }

        if (err == Blaze::ERR_OK)
        {
            return ERR_OK;
        }
        else
        {
            return ERR_SYSTEM;
        }
    }

    void UserSessionToArsonResponse(Blaze::UserSessionData* userSession)
    {
        mResponse.setUserSessionId(userSession->getUserSessionId());
        mResponse.setChildSessionId(userSession->getChildSessionId());
        mResponse.setParentSessionId(userSession->getParentSessionId());

        mResponse.setCreationTime(userSession->getCreationTime());
        
        userSession->getExtendedData().copyInto(mResponse.getExtendedData());
        mResponse.setLatitude(userSession->getLatitude());
        mResponse.setLongitude(userSession->getLongitude());
        userSession->getClientMetrics().copyInto(mResponse.getClientMetrics());
        userSession->getClientUserMetrics().copyInto(mResponse.getClientUserMetrics());
        userSession->getServerAttributes().copyInto(mResponse.getServerAttributes());
        mResponse.setConnectionGroupId(userSession->getConnectionGroupId());
        mResponse.setConnectionAddr(userSession->getConnectionAddr());
        mResponse.setDirtySockUserIndex(userSession->getDirtySockUserIndex());
        mResponse.setConnectionUserIndex(userSession->getConnectionUserIndex());
        mResponse.getClientState().setStatus(userSession->getClientState().getStatus());
        mResponse.getClientState().setStatus(userSession->getClientState().getStatus());
        mResponse.getRandom().setPart1(userSession->getRandom().getPart1());
        mResponse.getRandom().setPart2(userSession->getRandom().getPart2());
        mResponse.getRandom().setPart3(userSession->getRandom().getPart3());
        mResponse.getRandom().setPart4(userSession->getRandom().getPart4());
        userSession->getClientInfo().copyInto(mResponse.getClientInfo());

        // subscribe Sessions
        for(UserSessionData::Int32ByUserSessionId::const_iterator i = userSession->getSubscribedSessions().begin(); i != userSession->getSubscribedSessions().end(); i++)
        {
            mResponse.getSubscribedSessions().insert(eastl::make_pair(i->first, i->second));
        }
        // subscribe users
        for(UserSessionData::Int32ByBlazeId::const_iterator i = userSession->getSubscribedUsers().begin(); i != userSession->getSubscribedUsers().end(); i++)
        {
            mResponse.getSubscribedUsers().insert(eastl::make_pair(i->first, i->second));
        }

        // pendingNotificationList
        for (Blaze::NotificationDataList::const_iterator i = userSession->getPendingNotificationList().begin(); i != userSession->getPendingNotificationList().end(); i++)
        {
            Arson::NotificationData* pendingNotif = mResponse.getPendingNotificationList().pull_back();
            pendingNotif->setComponentId((*i)->getComponentId());
            pendingNotif->setNotificationId((*i)->getNotificationId());
            pendingNotif->setLogCategory((*i)->getLogCategory());
            pendingNotif->setEncoderType(static_cast<Arson::NotificationData::EncoderType>((*i)->getEncoderType()));
            pendingNotif->setEncoderSubType(static_cast<Arson::NotificationData::EncoderSubType>((*i)->getEncoderSubType()));
            pendingNotif->setEncoderResponseFormat(static_cast<Arson::NotificationData::EncoderResponseFormat>((*i)->getEncoderResponseFormat()));
            (*i)->getData().copyInto(pendingNotif->getData());
        }

        mResponse.setUniqueDeviceId(gCurrentUserSession->getUniqueDeviceId());
        mResponse.setDeviceLocality(gCurrentUserSession->getDeviceLocality());
    }

};

DEFINE_FETCHUSERSESSIONDATA_CREATE()

} //Arson
} //Blaze
