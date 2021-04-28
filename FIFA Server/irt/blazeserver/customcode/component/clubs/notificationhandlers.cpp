/*************************************************************************************************/
/*!
    \file   notificationhandlers.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"

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

/*************************************************************************************************/
/*!
    \brief notifyInvitationAccepted

    Custom processing upon invitation to join a club is accepted.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyInvitationAccepted(
        const ClubId clubId, 
        const BlazeId accepterId)
{
    UserInfoPtr newMember = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(accepterId, newMember);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyInvitationAccepted] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }
    
    if (newMember != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_INVITATION_ACCEPTED",
            newMember->getPersonaName());
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyInvitationDeclined

    Custom processing upon invitation to join a club is declined.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyInvitationDeclined(
        const ClubId clubId, const BlazeId declinerId)
{
    UserInfoPtr decliner = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(declinerId, decliner);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyInvitationDeclined] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (decliner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_INVITATION_DECLINED",
            decliner->getPersonaName());
    }
            
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyInvitationRevoked

    Custom processing upon invitation to join a club has been revoked
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyInvitationRevoked(
        const ClubId clubId, const BlazeId revokerId, const BlazeId inviteeId)
{
    UserInfoPtr revoker = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(revokerId, revoker);
    if (err != Blaze::ERR_OK)
        return err;
    
    UserInfoPtr invitee = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(inviteeId, invitee);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyInvitationRevoked] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (revoker != nullptr && invitee != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_INVITATION_REVOKED",
            revoker->getPersonaName(),
            invitee->getPersonaName());
    }

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyInvitationSent

    Custom processing upon invitation to join a club is declined.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyInvitationSent(
        const ClubId clubId, 
        const BlazeId inviterId,
        const BlazeId inviteeId)
{
    // send persistant message to the user
    // sending notification message is not crucial so just disregard return code
    BlazeRpcError res = sendMessagingMsgToNotifyUser(inviteeId); 
    if (res != Blaze::ERR_OK)
    {
        WARN_LOG("[notifyInvitationSent] could not send message to user " << inviteeId 
                 << " because messaging component returned error: " << SbFormats::HexLower(res) << ".");
    }

    UserInfoPtr inviter = nullptr;
    UserInfoPtr invitee = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(inviterId, inviter);
    if (err != Blaze::ERR_OK)
        return err;

    err = gUserSessionManager->lookupUserInfoByBlazeId(inviteeId, invitee);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyInvitationSent] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (inviter != nullptr && invitee != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_INVITATION_SENT",
            inviter->getPersonaName(),
            invitee->getPersonaName());
    }
            
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyPetitionSent

    Custom processing upon petition to join a club is sent.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyPetitionSent( 
        const ClubId clubId, 
        const char8_t *clubName,
        const uint32_t petitionId, 
        const BlazeId petitionerId)
{
    // send notification to all GMs so that they can review petition
    BlazeRpcError error = sendMessagingMsgToGMs(
        clubId, 
        clubName,
        CLUBS_PI_PETITION_SENT,
        petitionId,
        true);
        
    if (error != Blaze::ERR_OK)
    {
        WARN_LOG("[notifyPetitionSent] Failed to send petition messages for club " << clubId << ".");
    }

    UserInfoPtr petitioner = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(petitionerId, petitioner);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyPetitionDeclined] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (petitioner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_PETITION_SENT",
            petitioner->getPersonaName());
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyPetitionAccepted

    Custom processing upon petition to join a club is accepted.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyPetitionAccepted(
        const ClubId clubId, 
        const char8_t *clubName, 
        const uint32_t petitionId, 
        const BlazeId accepterId, 
        const BlazeId petitionerId)
{
    // send notification to all GMs so that they know petition is accepted
    BlazeRpcError error = sendMessagingMsgToGMs(
        clubId, 
        clubName,
        CLUBS_PI_PETITION_ACCEPTED,
        petitionId,
        false);
        
    if (error != Blaze::ERR_OK)
    {
        TRACE_LOG("[notifyPetitionAccepted] Failed to send petition messages for club " << clubId << ".");
    }

    // send persistant message to the user
    // sending notification message is not crucial so just disregard return code
    // get messaging component
    Messaging::MessagingSlaveImpl *msging 
        = static_cast<Messaging::MessagingSlaveImpl*>
            (gController->getComponent(Messaging::MessagingSlave::COMPONENT_ID));
            
    if (msging != nullptr)
    {            
        Messaging::ClientMessage message;
        message.setType(static_cast<Messaging::MessageType>(ClubsSlave::COMPONENT_ID));
        message.setTargetType(ENTITY_TYPE_USER);
        message.getTargetIds().push_back(petitionerId);
        message.getFlags().setPersistent();
        message.getFlags().setLocalize();

        message.getAttrMap().insert(eastl::make_pair(Messaging::MSG_ATTR_BODY, Messaging::AttrValue("OSDK_CLUBS_DLG_PLAYER_JOINED_CLUB")));
        Messaging::MessagingSlaveImpl::setAttrParam(message, Messaging::MSG_ATTR_BODY, 0, clubName);
            
        msging->sendMessage(EA::TDF::ObjectId(ENTITY_TYPE_USER, accepterId), message);
    }

    // send non-persistent message to user
    if (sendMessagingMsgPetition(
            petitionerId, 
            CLUBS_PI_PETITION_ACCEPTED, 
            petitionId,
            clubName) != Blaze::ERR_OK)
    {
            WARN_LOG("[notifyPetitionAccepted] Failed to send message to user " << petitionerId);
    }

    UserInfoPtr accepter = nullptr;
    UserInfoPtr petitioner = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(petitionerId, petitioner);
    if (err != Blaze::ERR_OK)
        return err;

    err = gUserSessionManager->lookupUserInfoByBlazeId(accepterId, accepter);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyPetitionAccepted] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }
    
    if (accepter != nullptr && petitioner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_PETITION_ACCEPTED",
            accepter->getPersonaName(),
            petitioner->getPersonaName());
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyPetitionRevoked

    Custom processing upon petition to join a club is revoked.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyPetitionRevoked(
        const ClubId clubId, 
        const uint32_t petitionId,
        const BlazeId petitionerId)
{
    // send notification to all GMs so that they know petition is revoked
    BlazeRpcError error = sendMessagingMsgToGMs(
        clubId, 
        nullptr,
        CLUBS_PI_PETITION_REVOKED,
        petitionId,
        false);
        
    if (error != Blaze::ERR_OK)
    {
        WARN_LOG("[notifyPetitionRevoked] Failed to send petition messages for club " << clubId << ".");
    }

    UserInfoPtr petitioner = nullptr;
    error = gUserSessionManager->lookupUserInfoByBlazeId(petitionerId, petitioner);
    if (error != Blaze::ERR_OK)
        return error;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyPetitionRevoked] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (petitioner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_PETITION_REVOKED",
            petitioner->getPersonaName());
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyPetitionDeclined

    Custom processing upon petition to join a club is declined
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyPetitionDeclined(
        const ClubId clubId, 
        const char8_t *clubName,
        const uint32_t petitionId,
        const BlazeId declinerId, 
        const BlazeId petitionerId)
{
    if (sendMessagingMsgPetition(
            petitionerId, 
            CLUBS_PI_PETITION_DECLINED, 
            petitionId,
            clubName
            ) != Blaze::ERR_OK)
    {
        WARN_LOG("[notifyPetitionDeclined] Failed to send message to user " << petitionerId);
    }
    
    if (sendMessagingMsgToGMs(
        clubId, 
        clubName,
        CLUBS_PI_PETITION_DECLINED,
        petitionId,
        false) != Blaze::ERR_OK)
    {
        WARN_LOG("[notifyPetitionDeclined] Failed to send petition messages for club " << clubId << ".");
    }

    UserInfoPtr decliner = nullptr;
    UserInfoPtr petitioner = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(petitionerId, petitioner);
    if (err != Blaze::ERR_OK)
        return err;
    
    err = gUserSessionManager->lookupUserInfoByBlazeId(declinerId, decliner);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyPetitionDeclined] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (petitioner != nullptr && decliner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_PETITION_DECLINED",
            decliner->getPersonaName(),
            petitioner->getPersonaName());
    }
    
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyUserJoined

    Custom processing upon user joined the club.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyUserJoined(
        const ClubId clubId, 
        const BlazeId joinerId)
{
    UserInfoPtr joiner = nullptr;
    
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(joinerId, joiner);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyUserJoined] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (joiner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_USER_JOINED_CLUB",
            joiner->getPersonaName());
    }
            
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyMemberRemoved

    Custom processing upon member is removed from club.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyMemberRemoved(
        const ClubId clubId, 
        const BlazeId removerId,
        const BlazeId removedMemberBlazeId)
{
    UserInfoPtr user = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(removedMemberBlazeId, user);
    if (err != Blaze::ERR_OK)
        return err;

    UserInfoPtr remover = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(removerId, remover);
    if (err != Blaze::ERR_OK)
        return err;
    
    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyMemberRemoved] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (removerId == removedMemberBlazeId)
    {
        if (user != nullptr)
        {
            publishClubTickerMessage(
                &dbc.getDb(), 
                true,    
                clubId,
                0,
                0,
                nullptr,
                "OSDK_CLUBS_SERVEREVENT_2",
                user->getPersonaName());
        }
    }
    else
    {
        if (user != nullptr && remover != nullptr)
        {
            publishClubTickerMessage(
                &dbc.getDb(), 
                true,    
                clubId,
                0,
                0,
                nullptr,
                "CLB_TICKER_USER_REMOVED",
                remover->getPersonaName(),
                user->getPersonaName()
                );
        }
    }

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyMemberPromoted

    Custom processing upon club member is promoted to GM
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyMemberPromoted(
        const ClubId clubId, 
        const BlazeId promoterId,
        const BlazeId promotedBlazeId)
{
    UserInfoPtr gm = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(promoterId, gm);
    if (err != Blaze::ERR_OK)
        return err;

    UserInfoPtr user = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(promotedBlazeId, user);
    if (err != Blaze::ERR_OK)
        return err;
    
    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyMemberPromoted] could not send message"
            " because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (user != nullptr)
    {
        // log promote event
        char blazeId[32];
        blaze_snzprintf(blazeId, sizeof(blazeId), "%" PRId64, user->getId());
        err = logEvent(&dbc.getDb(), clubId, getEventString(LOG_EVENT_GM_PROMOTED),
                NEWS_PARAM_BLAZE_ID, blazeId);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[notifyMemberPromoted] unable to log event for user " << promotedBlazeId << " in club " << clubId);
        }
    }

    if (user != nullptr && gm != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_USER_PROMOTED",
            gm->getPersonaName(),
            user->getPersonaName()
            );
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyGmDemoted

    Custom processing upon club GM is demoted to member
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyGmDemoted(
        const ClubId clubId,
        const BlazeId demoterId,
        const BlazeId demotedUserId)
{
    UserInfoPtr gm = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(demoterId, gm);
    if (err != Blaze::ERR_OK)
        return err;

    UserInfoPtr user = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(demotedUserId, user);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyGmDemoted] could not send message because I could not acquire database connection");

        return Blaze::ERR_SYSTEM;
    }

    if (user != nullptr)
    {
        // log demote event
        char blazeId[32];
        blaze_snzprintf(blazeId, sizeof(blazeId), "%" PRId64, user->getId());
        err = logEvent(&dbc.getDb(), clubId, getEventString(LOG_EVENT_GM_DEMOTED),
            NEWS_PARAM_BLAZE_ID, blazeId);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[notifyGmDemoted] unable to log event for user " << demotedUserId << " in club " << clubId);
        }
    }

    if (user != nullptr && gm != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_USER_DEMOTED",
            gm->getPersonaName(),
            user->getPersonaName()
            );
    }

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyOwnershipTransferred

    Custom processing upon ownership is transferred
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyOwnershipTransferred(
        const ClubId clubId, 
        const BlazeId transferrerId,
        const BlazeId oldOwnerId,
        const BlazeId newOwnerId)
{
    UserInfoPtr tfer = nullptr;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(transferrerId, tfer);
    if (err != Blaze::ERR_OK)
        return err;

    UserInfoPtr oldOwner = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(oldOwnerId, oldOwner);
    if (err != Blaze::ERR_OK)
        return err;
 
    UserInfoPtr newOwner = nullptr;
    err = gUserSessionManager->lookupUserInfoByBlazeId(newOwnerId, newOwner);
    if (err != Blaze::ERR_OK)
        return err;

    ClubsDbConnector dbc(this);
    if (dbc.acquireDbConnection(false) == nullptr)
    {
        WARN_LOG("[notifyOwnershipTransferred:] could not send message because I could not acquire database connection");
            
        return Blaze::ERR_SYSTEM;
    }

    if (oldOwner != nullptr && newOwner != nullptr)
    {
        // log transfer ownership event
        char oldId[32];
        char newId[32];
        blaze_snzprintf(oldId, sizeof(oldId), "%" PRId64, oldOwner->getId());
        blaze_snzprintf(newId, sizeof(newId), "%" PRId64, newOwner->getId());
        err = logEvent(&dbc.getDb(), clubId, getEventString(LOG_EVENT_OWNERSHIP_TRANSFERRED),
                NEWS_PARAM_BLAZE_ID, oldId, NEWS_PARAM_BLAZE_ID, newId);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[notifyOwnershipTransferred] unable to log event for old owner " 
                << oldId << "and new owner" << newId <<" in club" << clubId);
        }
    }

    if (tfer != nullptr && oldOwner != nullptr && newOwner != nullptr)
    {
        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            "CLB_TICKER_OWNERSHIP_TRANSFERRED",
            tfer->getPersonaName(),
            oldOwner->getPersonaName(),
            newOwner->getPersonaName()
            );
    }
    
    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief notifyNewsPublished

    Custom processing upon news item is published.
    Use this code to send messages only, no changes on club objects are allowed here.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::notifyNewsPublished(
        const ClubId clubId, 
        const BlazeId publisherId,
        const ClubNews &news)
{
    // publish only member generated news
    if (news.getType() == CLUBS_MEMBER_POSTED_NEWS)
    {
        UserInfoPtr member = nullptr;
        BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(news.getUser().getBlazeId(), member);
        if (err != Blaze::ERR_OK)
            return err;
        
        if (member == nullptr)
            return Blaze::ERR_SYSTEM;
            
        ClubsDbConnector dbc(this);
        if (dbc.acquireDbConnection(false) == nullptr)
        {
            WARN_LOG("[notifyMemberPromoted] could not send message"
                " because I could not acquire database connection");
                
            return Blaze::ERR_SYSTEM;
        }
        
        eastl::string newsString;
        newsString.sprintf("[%s] %s", member->getPersonaName(), news.getText());

        publishClubTickerMessage(
            &dbc.getDb(), 
            true,    
            clubId,
            0,
            0,
            nullptr,
            newsString.c_str()
            );
    }
    
    return Blaze::ERR_OK;
}

} // Clubs
} // Blaze
