/*************************************************************************************************/
/*!
    \file   clubsmessages.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ClubsSlaveImpl

    Implementation of messaging helper functions

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/database/dbutil.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/localization.h"

// clubs includes
#include "clubsslaveimpl.h"
#include "clubsdbconnector.h"

//messaging
#include "messaging/messagingslaveimpl.h"
#include "messaging/tdf/messagingtypes.h"

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/
BlazeRpcError ClubsSlaveImpl::publishClubTickerMessage(
    ClubsDatabase *clubsDb,
    const bool notify,                 // set to true if you want to notify subscribed users
    const ClubId clubId,                  // club id, 0 to broatcast to all clubs 
    const BlazeId includeBlazeId,           // private message for blazeId, 0 to broadcast to all users
    const BlazeId excludeBlazeId,           // exclude user, 0 to broadcast to all users
    const char8_t *metadata,              // metadata
    const char8_t *text,                  // message text or message id for localized messages
    const char8_t *param1,                // optional message parameters for localized messages
    const char8_t *param2,
    const char8_t *param3,
    const char8_t *param4,
    const char8_t *param5)
{
    BlazeRpcError result = Blaze::ERR_OK;
    
    eastl::string paramList;
    const char8_t* params[] = { param1, param2, param3, param4, param5 };
    
    for (int i = 0; i < 5; i++)
    {
        if (params[i] != nullptr)
        {
            if (i > 0)
                paramList.append(",");
            paramList.append(params[i]);
        }
        else
            break;
    }
    
    
    if (notify)
    {
        PublishClubTickerMessageRequest msg;
        msg.setClubId(clubId);
        msg.setIncludeBlazeId(includeBlazeId);
        msg.setExcludeBlazeId(excludeBlazeId);
        msg.setParams(paramList.c_str());
        if (text)
            msg.getMessage().setText(text);
        msg.getMessage().setTimestamp(static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()));
        if (metadata)
            msg.getMessage().setMetadata(metadata);
        sendPublishClubTickerMessageNotificationToAllSlaves(&msg);
        if (result != Blaze::ERR_OK)
        {
            WARN_LOG("[PublishClubTickerMessage] Failed to publish message for club " << clubId);
        }
    }

    if (metadata == nullptr)
        metadata = "";
    
    result = 
        clubsDb->insertTickerMsg(
            clubId,
            includeBlazeId,
            excludeBlazeId,
            text,
            paramList.c_str(),
            metadata);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[PublishClubTickerMessage] Failed to write ticker message to db for club " << clubId);
    }
        
    return result;
}

BlazeRpcError ClubsSlaveImpl::setClubTickerSubscription(const ClubId clubId, const BlazeId blazeId, bool isSubscribed)
 {
    // check if already subscribed               
    ClubsSlaveImpl::TickerSubscriptionList::iterator cit = 
        mTickerSubscriptionList.find(clubId);

    if (cit == mTickerSubscriptionList.end())
    {
        // if unsubscribe is called and the user wasn't subscribed there is nothing to do
        if (isSubscribed == false)
        {
            return ERR_OK;
        }
        
        eastl::list<BlazeId> userList;
        userList.push_back(blazeId);
        mTickerSubscriptionList.insert(eastl::make_pair(clubId, userList));
    }
    else
    {
        eastl::list<BlazeId> &userList = cit->second;   
        eastl::list<BlazeId>::iterator uit = eastl::find(userList.begin(), userList.end(), blazeId);
        if (uit == userList.end())
        {
            if (isSubscribed)
            {
                // subscribe user
                userList.push_back(blazeId);
            }
        }
        else if (isSubscribed == false)
        {
            // user is found so unsubscribe it
            userList.erase(uit);
            if (userList.size() == 0)
                mTickerSubscriptionList.erase(cit);                        
        }
    }

    return ERR_OK;            
}
 
BlazeRpcError ClubsSlaveImpl::sendMessagingMsgToNotifyUser(BlazeId receiver) const
{
    // get messaging component
    Messaging::MessagingSlaveImpl *msging 
        = static_cast<Messaging::MessagingSlaveImpl*>
            (gController->getComponent(Messaging::MessagingSlave::COMPONENT_ID));
            
    if (msging == nullptr)
    {
        ERR_LOG("[sendMessagingMsgToNotifyUser] failed to send message to user " << receiver 
                << " because messaging component is not found.");
        
        return ERR_SYSTEM;
    }
    
    Messaging::ClientMessage message;
    message.setType(static_cast<Messaging::MessageType>(COMPONENT_ID));
    message.setTargetType(ENTITY_TYPE_USER);
    message.getTargetIds().push_back(receiver);
    message.getFlags().setPersistent();
    message.getFlags().setLocalize();

    message.getAttrMap().insert(eastl::make_pair(Messaging::MSG_ATTR_BODY, Messaging::AttrValue("OSDK_EASW_MAIL_NOTIFICATION")));
        
    BlazeRpcError err = ERR_SYSTEM;
    
    if (gCurrentLocalUserSession != nullptr)
        err = msging->sendMessage(gCurrentLocalUserSession->getUserInfo().getBlazeObjectId(), message);

    return err;
}

BlazeRpcError ClubsSlaveImpl::sendMessagingMsgToGMs(
    ClubId receiverClub, 
    const char8_t* clubName,
    PetitionMessageType msgType,
    uint32_t msgId,
    bool sendPersistent)
{
    BlazeRpcError error = Blaze::ERR_OK;

    ClubMemberList clubMemberList;
    ClubsDbConnector dbcon(this);
     if (dbcon.acquireDbConnection(true) == nullptr)
     {
        ERR_LOG("[sendMessagingMsgToGMs] failed to send message to GMs of club " << clubName 
                << " because of failure to obtain database connection.");
        return Blaze::ERR_SYSTEM;
     }
    
    ClubsDatabase &clubsDb = dbcon.getDb();

    Club club;
    if (clubName == nullptr)
    {
        uint64_t version = 0;
        if ((error = clubsDb.getClub(receiverClub, &club, version)) != Blaze::ERR_OK)
        {
            WARN_LOG("[sendMessagingMsgToGMs] Could not get club [id]: " << receiverClub);
            return error;
        }
        
        clubName = club.getName();
    }
           
    if ((error = clubsDb.getClubMembers(receiverClub, OC_UNLIMITED_RESULTS, 0, clubMemberList)) != Blaze::ERR_OK)
    {
        
        WARN_LOG("[sendMessagingMsgToGMs] failed to send message to GMs of club " << clubName 
                 << " because of failure to obtain club member list, error was " << ErrorHelp::getErrorName(error) << ".");
        return error;
    }

    dbcon.releaseConnection();
        
    for (ClubMemberList::const_iterator it = clubMemberList.begin(); it != clubMemberList.end(); it++)
    {
        const ClubMember *member = *it;
        if (member->getMembershipStatus() >= CLUBS_GM)
        {
            if (sendPersistent)
            {
                if (sendMessagingMsgToNotifyUser(member->getUser().getBlazeId()) != Blaze::ERR_OK)
                {
                    WARN_LOG("[sendMessagingMsgToGMs] failed to send persistent messaging message to GM member [blazeid]: " 
                             << member->getUser().getBlazeId());
                }
            }

            if (sendMessagingMsgPetition(
                    member->getUser().getBlazeId(), 
                    msgType, 
                    msgId,
                    clubName) != Blaze::ERR_OK)
            {
                TRACE_LOG("[sendMessagingMsgToGMs] failed to send messaging message to GM member [blazeid]: " 
                          << member->getUser().getBlazeId() << ".");
            }
        }
    }
    return Blaze::ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::sendMessagingMsgPetition(
    BlazeId receiver, 
    PetitionMessageType msgType,
    uint32_t msgId,
    const char8_t *clubName) const
{
    // get messaging component
    Messaging::MessagingSlaveImpl *msging 
        = static_cast<Messaging::MessagingSlaveImpl*>
            (gController->getComponent(Messaging::MessagingSlave::COMPONENT_ID));
            
    if (msging == nullptr)
    {
        ERR_LOG("[sendMessagingMsgPetitionAndInvitation] failed to send message to user " << receiver 
                << " because messaging component is not found.");
        
        return ERR_SYSTEM;
    }
    
    Messaging::ClientMessage message;
    message.setType(static_cast<Messaging::MessageType>(COMPONENT_ID));
    message.setTargetType(ENTITY_TYPE_USER);
    message.getTargetIds().push_back(receiver);
    message.getFlags().setLocalize();
    
    const char8_t *messageText = nullptr;
    switch(msgType)
    {
        case CLUBS_PI_PETITION_SENT:
            messageText = "OSDK_CLUB_PETITION_SENT";
            break;
        case CLUBS_PI_PETITION_ACCEPTED:
            messageText = "OSDK_CLUB_PETITION_ACCEPTED";
            break;
        case CLUBS_PI_PETITION_DECLINED:
            messageText = "OSDK_CLUB_PETITION_DECLINED";
            break;
        case CLUBS_PI_PETITION_REVOKED:
            messageText = "OSDK_CLUB_PETITION_REVOKED";
            break;
        default:
        {
            ERR_LOG("[sendMessagingMsgPetitionAndInvitation] failed to send message to user " << receiver 
                    << " because message type is unknown: " << msgType << ".");
            return Blaze::ERR_SYSTEM;
        }
    }
    
    message.getAttrMap().insert(eastl::make_pair(Messaging::MSG_ATTR_BODY, Messaging::AttrValue(messageText)));
    
    char8_t buffer[32];

    blaze_snzprintf(buffer, 32, "%d", msgType);
    message.getAttrMap().insert(
        eastl::make_pair(static_cast<Messaging::AttrKey>(CLUBS_MSGATR_MESSAGE_TYPE), buffer));

    blaze_snzprintf(buffer, 32, "%d", msgId);
    message.getAttrMap().insert(
        eastl::make_pair(static_cast<Messaging::AttrKey>(CLUBS_MSGATR_CLUB_MSG_ID), buffer));
        
    message.getAttrMap().insert(
        eastl::make_pair(static_cast<Messaging::AttrKey>(CLUBS_MSGATR_CLUB_NAME), clubName));

    BlazeRpcError err = ERR_SYSTEM;
    
    if (gCurrentLocalUserSession != nullptr)
        err = msging->sendMessage(gCurrentLocalUserSession->getUserInfo().getBlazeObjectId(), message);
        
    return err;
}

/*** Private Methods ******************************************************************************/

void ClubsSlaveImpl::onPublishClubTickerMessageNotification(
    const PublishClubTickerMessageRequest& message,
    Blaze::UserSession *)
{
    ClubId clubId = message.getClubId();
    TickerSubscriptionList::const_iterator cit = mTickerSubscriptionList.begin(); 
    
    if (clubId != INVALID_CLUB_ID)
        cit = mTickerSubscriptionList.find(clubId);

    uint32_t prevLocale = 0;
    eastl::string text;
        
    while (cit != mTickerSubscriptionList.end())
    {
        ClubTickerMessage msg;
        msg.setMetadata(message.getMessage().getMetadata());
        msg.setTimestamp(message.getMessage().getTimestamp());
            
        const eastl::list<BlazeId> &list = cit->second;
        for (eastl::list<BlazeId>::const_iterator it = list.begin(); it != list.end(); it++)
        {
            
            BlazeId blazeId = *it;
            if ((message.getIncludeBlazeId() == blazeId || message.getIncludeBlazeId() == 0)
                && message.getExcludeBlazeId() != blazeId)
            {
                UserSessionIdList ids;
                
                UserInfoPtr userInfo = gUserSessionManager->getUser(blazeId);
                if (userInfo != nullptr)
                {
                    gUserSessionManager->getSessionIds(blazeId, ids);
                }
                
                for (UserSessionIdList::const_iterator sit = ids.begin(); sit != ids.end(); ++sit)
                {
                    UserSessionId userSessionId = *sit;

                    if (gUserSessionManager->getSessionExists(userSessionId))
                        continue;

                    uint32_t curLocale = gUserSessionManager->getSessionLocale(userSessionId);
                    if (prevLocale != curLocale)
                    {
                        text.clear();
                        prevLocale = curLocale;
                        localizeTickerMessage(
                            message.getMessage().getText(),
                            message.getParams(),
                            prevLocale,
                            text);
                    }
                    
                    msg.setText(text.c_str());
                    sendNewClubTickerMessageNotificationToUserSessionById(userSessionId, &msg);
                }    
            }
        }
        
        if (clubId != INVALID_CLUB_ID)
            break;  // this message was not broadcast to all clubs kind of message
        
        cit++;
    }
}

BlazeRpcError ClubsSlaveImpl::purgeStaleTickerMessages(DbConnPtr& conn, uint32_t tickerMessageExpiryDay) const //defined in clubsmessages.cpp
{
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(conn);
    BlazeRpcError result = 
        clubsDb.deleteOldestTickerMsg(
            static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec() - tickerMessageExpiryDay * 60 * 60 * 24));
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[purgeStaleTickerMessages] failed to purge old ticker message itmes, the return code was: " << result << ".");
    }
    
    return Blaze::ERR_OK;
}

void ClubsSlaveImpl::localizeTickerMessage(
    const char8_t *origText, 
    const char8_t *msgParams, 
    const uint32_t locale,
    eastl::string &text) const
{
   text = gLocalization->localize(
        origText,
        locale);
    
    eastl::vector<eastl::string> paramList;
    eastl::string params = msgParams;
    
    if (params.length() > 0)
    {
        eastl::string::size_type prevPos = 0;
        eastl::string::size_type pos = params.find(",");
        
        if (pos == params.npos)
        {
            paramList.push_back(params);
        }
        else
        {
            do
            {
                if (pos != params.npos)
                    paramList.push_back(params.substr(prevPos, pos - prevPos));
                else
                    paramList.push_back(params.substr(prevPos));
                prevPos = pos;
                if (prevPos != params.npos)
                {
                    ++prevPos;
                    pos = params.find(",", prevPos);
                }
            } while (prevPos != params.npos);
        }
    }
   
                            
    for (unsigned int i = 0; i < paramList.size(); i++)
    {
        eastl::string num;
        num.sprintf("<%d>", i);
        eastl::string::size_type n;
        do
        {
            n = text.find(num);
            if (n != eastl::string::npos)
                text.replace(n, num.length(), paramList[i]);
        }
        while (n != eastl::string::npos);
    }
}

} // Clubs
} // Blaze
