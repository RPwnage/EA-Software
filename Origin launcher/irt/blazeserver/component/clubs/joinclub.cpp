/*************************************************************************************************/
/*!
    \file   joinclub.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \brief joinClub

    A user joins to a public club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/usersessions/userinfo.h"
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth.h"

// clubs includes
#include "clubsslaveimpl.h"
#include "clubsdbconnector.h"

#include "associationlists/rpc/associationlistsslave.h"
#include "associationlists/tdf/associationlists.h"

#include "xblsocial/tdf/xblsocial.h"
#include "xblsocial/rpc/xblsocialslave.h"

#include "gamemanager/tdf/externalsessiontypes_server.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"// for isMockPlatformEnabled in getXblSocialPeopleRelationshipHelper

namespace Blaze
{
namespace Clubs
{


bool ClubsSlaveImpl::isMock() const
{
    return (getConfig().getUseMock() || gController->getUseMockConsoleServices());
}

// utility impl
bool ClubsSlaveImpl::isClubGMFriend(const ClubId clubId, const BlazeId blazeId, ClubsDbConnector& dbc) const
{
    if (gCurrentUserSession == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].isClubGMFriend: current user session is nullptr.");
        return false;
    }
    
    ClientPlatformType playerPlatform = gCurrentUserSession->getClientPlatform();
    if (playerPlatform == INVALID)
    {
        ERR_LOG("[ClubsSlaveImpl].isClubGMFriend: player platform is invalid.");
        return false;   
    }

    //grab the list of GMs first
    ClubMemberList memberList;
    BlazeRpcError error = dbc.getDb().getClubMembers(
        clubId,
        UINT32_MAX,
        0,
        memberList,
        MEMBER_NO_ORDER,
        ASC_ORDER,
        GM_MEMBERS); // only get GMs    

    if (error == Blaze::ERR_OK && !memberList.empty())
    {
        //For Xbox One, People's list isn't mutual, therefore we have to query People's relationship based on GM's point of view, and check if the user is in it
        if ((playerPlatform == xone) || (playerPlatform == xbsx) || isMock())
        {   
            //for every GM
            for (ClubMemberList::const_iterator gmIt = memberList.begin(); gmIt != memberList.end(); ++gmIt)
            {
                BlazeRpcError blazeRpcError = ERR_SYSTEM;

                //retrieve gm blaze id
                BlazeId gmBlazeId = (*gmIt)->getUser().getBlazeId();

                //lookup GM's userInfo by blaze id, in order to retrieve the GM's ext id
                UserInfoPtr gmUserInfo;
                blazeRpcError = gUserSessionManager->lookupUserInfoByBlazeId(gmBlazeId, gmUserInfo);
                if (blazeRpcError != ERR_OK)
                {
                    ERR_LOG("[ClubsSlaveImpl].isClubGMFriend: Error occured looking up GM with blazeId " << gmBlazeId << ", with error '" << ErrorHelp::getErrorName(blazeRpcError) <<"'.");
                    return false;
                }
                ExternalId gmExtId = gmUserInfo->getPlatformInfo().getExternalIds().getXblAccountId();   //retrieve the current GM's ext id

                //lookup GM's People's list
                if (getXblSocialPeopleRelationshipHelper(gmBlazeId, gmExtId, gCurrentUserSession->getPlatformInfo().getExternalIds().getXblAccountId(), false))
                {
                    //This GM's People list does contain self
                    return true;
                }
                else
                {
                    //retry with a refreshed XBL token as the token from nucleus could have been expired
                    TRACE_LOG("[ClubsSlaveImpl].isClubGMFriend: Retrying with a refreshed XBL Token as it could have been expired.");

                    if (getXblSocialPeopleRelationshipHelper(gmBlazeId, gmExtId, gCurrentUserSession->getPlatformInfo().getExternalIds().getXblAccountId(), true))
                    {
                        //This GM's People list does contain self
                        return true;
                    }
                }
            }
        } 
        else  //for Gen3 + PS4
        {
            Blaze::Association::AssociationListsSlave *associationListsComponent = static_cast<Blaze::Association::AssociationListsSlave*>(gController->getComponent(Blaze::Association::AssociationListsSlave::COMPONENT_ID));
            if (associationListsComponent != nullptr)
            {
                BlazeId joinerBlazeId = gCurrentUserSession->getBlazeId();

                Blaze::Association::CheckListContainsMembersRequest findReq;
                Blaze::Association::ListBlazeIds findRsp;

                //Blaze AssociationList's friendList is non-mutual (we rely on first party's friendlist)
                findReq.getListIdentification().setListType(Blaze::Association::LIST_TYPE_FRIEND);
                findReq.setOwnerBlazeId(joinerBlazeId); 

                //add all the GMs into the find membersList
                for (ClubMemberList::const_iterator gmIt = memberList.begin(); gmIt != memberList.end(); ++gmIt)
                {
                    BlazeId gmBlazeId = (*gmIt)->getUser().getBlazeId();  //retrieve gm blaze id
                    findReq.getMembersBlazeIds().getBlazeIds().push_back(gmBlazeId);
                }

                if (associationListsComponent->checkListContainsMembers(findReq, findRsp) == Blaze::ERR_OK)
                {
                    TRACE_LOG("[ClubsSlaveImpl].isClubGMFriend: At least one GM is a friend of User BlazeId: " << joinerBlazeId << ".");
                    return true;
                }
            }
        }
    }

    TRACE_LOG("[ClubsSlaveImpl].isClubGMFriend: the user is not a friend of any GM(s).");
    return false;
}

//helper to retrieve XBL auth token and query the MS server for the People list's relationship
bool ClubsSlaveImpl::getXblSocialPeopleRelationshipHelper(const BlazeId ownerBlazeId, const ExternalId ownerExtId, const ExternalId memberExtId, bool forceRefresh) const
{
    Blaze::OAuth::GetUserXblTokenRequest req;
    Blaze::OAuth::GetUserXblTokenResponse rsp;
    BlazeRpcError blazeRpcError = ERR_SYSTEM;

    req.getRetrieveUsing().setPersonaId(ownerBlazeId);
    req.setForceRefresh(forceRefresh);

    Blaze::OAuth::OAuthSlave *oAuthComponent = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
        Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
    if (oAuthComponent == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].getPeopleHelper: AuthenticationSlave was unavailable, People lookup failed.");
        return false;
    }
    else
    {
        //need superuser's privilege to retrieve list owner's XBLToken because it's possible that list owner != gCurrentUser
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);

        //get the list owner's XBL token
        //Mock testing of XBL failure can occur if the test account wasn't set up w/nucleus. For convenience, don't fail, if mock testing (already logged)
        blazeRpcError = oAuthComponent->getUserXblToken(req, rsp);
        if ((blazeRpcError != ERR_OK) && !isMock())
        {
            ERR_LOG("[ClubsSlaveImpl].getPeopleHelper: Failed to retrieve xbl auth token from service, for BlazeId id: " << ownerBlazeId << ", with error '" << ErrorHelp::getErrorName(blazeRpcError) <<"'.");
            return false;
        }
    }

    //Is the member in the owner's People list?  Fire REST call to MS server to query.
    Blaze::XBLSocial::GetPeopleRequest getPeopleRequest;
    Blaze::XBLSocial::Person response;
    getPeopleRequest.setXuidOwner(ownerExtId); //People list owner's XUID
    getPeopleRequest.setXuidTarget(memberExtId); //The potential member's XUID
    getPeopleRequest.getPeopleRequestsHeader().setAuthToken(rsp.getXblToken());
    getPeopleRequest.getPeopleRequestsHeader().setContractVersion("1");     //contract verison is 1 Social now
    Blaze::XBLSocial::XBLSocialSlave * xblSocialSlave = (Blaze::XBLSocial::XBLSocialSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLSocial::XBLSocialSlave::COMPONENT_INFO.name);
    if (xblSocialSlave == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].getPeopleHelper: XblSocialSlave proxy component was unavailable, People lookup failed.");
        return false;
    }
    blazeRpcError = xblSocialSlave->getPeople(getPeopleRequest, response);
    if (blazeRpcError == ERR_OK)   //After GOS-25225 is fixed, we'll also catch for XBLSERVICECONFIGS_AUTHENTICATION_REQUIRED
    {
        //People list returned 200, meaning a Person is returned (The target user is in the owner's People list)                                
        TRACE_LOG("[ClubsSlaveImpl].getPeopleHelper: Owner BlazeId: " << ownerBlazeId << " with ExternalId: " << ownerExtId << " is a friend of Member ExternalId: " << memberExtId);
        return true;
    }

    //the current owner's People list doesn't contain the member in it
    TRACE_LOG("[ClubsSlaveImpl].isClubGMFriend: Member ExternalId: " << memberExtId << " is not found in owners People list. Owner BlazeId: " << ownerBlazeId << " with ExternalId: " << ownerExtId << ".");
    return false;
}

bool ClubsSlaveImpl::isClubJoinAccepted(const ClubSettings& clubSettings, const bool isGMFriend) const
{
    return (clubSettings.getJoinAcceptance() == CLUB_ACCEPT_ALL
        || (clubSettings.getJoinAcceptance() == CLUB_ACCEPT_GM_FRIENDS && isGMFriend));
}

bool ClubsSlaveImpl::isClubPasswordCheckSkipped(const ClubSettings& clubSettings, const bool isGMFriend) const
{
    return (clubSettings.getSkipPasswordCheckAcceptance() == CLUB_ACCEPT_ALL
        || (clubSettings.getSkipPasswordCheckAcceptance() == CLUB_ACCEPT_GM_FRIENDS && isGMFriend));
}

bool ClubsSlaveImpl::isClubPetitionAcctepted(const ClubSettings& clubSettings, const bool isGMFriend) const
{
    return (clubSettings.getPetitionAcceptance() == CLUB_ACCEPT_ALL
        || (clubSettings.getPetitionAcceptance() == CLUB_ACCEPT_GM_FRIENDS && isGMFriend));
}

BlazeRpcError ClubsSlaveImpl::joinClub(const ClubId joinClubId, const bool skipJoinAcceptanceCheck, const bool petitionIfJoinFails, const bool skipPasswordCheck, const ClubPassword& password,
                                       const BlazeId rtorBlazeId, const bool isGMFriend, const uint32_t invitationId, ClubsDbConnector& dbc, uint32_t& messageId)
{
    BlazeRpcError error = Blaze::ERR_OK;
    messageId = 0;

    if (!dbc.isTransactionStart())
    {
        if (!dbc.startTransaction())
            return ERR_SYSTEM;
    }

    Club joinClub;
    uint64_t version = 0;

    // lock club so that member count can not be updated
    error = dbc.getDb().lockClub(joinClubId, version, &joinClub);
    if (error != Blaze::ERR_OK)
    {
        return error;
    }
    
    // Check that this club is accepting joins
    if (!skipJoinAcceptanceCheck && !isClubJoinAccepted(joinClub.getClubSettings(), isGMFriend))
    {
        return Blaze::CLUBS_ERR_JOIN_DISABLED;
    }

    const ClubDomain *domain = nullptr;
    error = getClubDomainForClub(joinClubId, dbc.getDb(), domain);
    if (error != Blaze::ERR_OK)
    {
        return error;
    }

    // Check if this clubs is full
    if(joinClub.getClubInfo().getMemberCount() == domain->getMaxMembersPerClub())
    {
        return CLUBS_ERR_CLUB_FULL;
    }

    // Check that the requestor's password is match club password
    if (!skipPasswordCheck && joinClub.getClubSettings().getPassword()[0] != '\0')
    {
        if (!isClubPasswordCheckSkipped(joinClub.getClubSettings(), isGMFriend))
        {
            if (password.c_str()[0] == '\0')
            {
                if (!isClubPetitionAcctepted(joinClub.getClubSettings(), isGMFriend) || !petitionIfJoinFails)
                {
                    TRACE_LOG("[ClubsSlaveImpl::joinClub] user " << rtorBlazeId << " failed to join club " << joinClubId << ", password is wrong!");
                    return Blaze::CLUBS_ERR_PASSWORD_REQUIRED;
                }
                else
                {
                    // Send petiton to this club.
                    dbc.completeTransaction(false);
                    return petitionClub(joinClubId, domain, rtorBlazeId, true/*skipMembershipCheck*/, dbc, messageId);
                }
            }
            if (blaze_stricmp(joinClub.getClubSettings().getPassword(), password.c_str()) != 0)
            {
                TRACE_LOG("[ClubsSlaveImpl::joinClub] user " << rtorBlazeId << " failed to join club " << joinClubId << ", password is wrong!");
                return Blaze::CLUBS_ERR_WRONG_PASSWORD;
            }
        }
    }

    // Check that the requester is not trying to join a club when he already belongs to one
    ClubMembership membership;
    error = dbc.getDb().getMembershipInClub(
        joinClubId,
        rtorBlazeId,
        membership);
    if (error == Blaze::ERR_OK)
    {
        return Blaze::CLUBS_ERR_ALREADY_MEMBER;
    }
    else if (error != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
    {
        return error;
    }

    error = dbc.getDb().lockMemberDomain(rtorBlazeId, domain->getClubDomainId());
    if (error != Blaze::ERR_OK)
    {
        return error;
    }

    // Check the user is not already member of maximum number of clubs in this domain
    ClubMemberStatusList statusList;
    error = getMemberStatusInDomain(
        domain->getClubDomainId(), 
        rtorBlazeId, 
        dbc.getDb(), 
        statusList,
        true);
    if (static_cast<uint16_t>(statusList.size()) >= domain->getMaxMembershipsPerUser())
    {
        return Blaze::CLUBS_ERR_MAX_CLUBS;
    }

    // Check that the requestor is not already banned from this club
    BanStatus banStatus = CLUBS_NO_BAN;
    error = dbc.getDb().getBan(joinClubId, rtorBlazeId, &banStatus);

    if (error != Blaze::ERR_OK)
    {
        return error;
    }
    else if (banStatus != CLUBS_NO_BAN)
    {
        return Blaze::CLUBS_ERR_USER_BANNED;
    }

    ClubMember joinUser;
    joinUser.getUser().setBlazeId(rtorBlazeId);
    joinUser.setMembershipStatus(CLUBS_MEMBER);
    error = dbc.getDb().insertMember(joinClubId, joinUser, domain->getMaxMembersPerClub());

    if (error != Blaze::ERR_OK)
    {
        if (error != Blaze::CLUBS_ERR_CLUB_FULL)
        {
            // log unexpected error
            ERR_LOG("[ClubsSlaveImpl::joinClub] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId << " failed to join club " << joinClubId);
        }
        return error;
    }

    joinClub.getClubInfo().setMemberCount(joinClub.getClubInfo().getMemberCount() + 1);

    if (invitationId > 0)
    {
        error = dbc.getDb().deleteClubMessage(invitationId);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ClubsSlaveImpl::joinClub] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId
                << " failed to delete invitation " << invitationId);
            return error;
        }
    }

    ClubMessageList petitionMsgs;
    error = dbc.getDb().getClubMessagesByUserSent(rtorBlazeId, CLUBS_PETITION, CLUBS_SORT_TIME_DESC, petitionMsgs);    
    if (error != Blaze::ERR_OK)
    {
        return error;
    }
    for (ClubMessageList::const_iterator it = petitionMsgs.begin(); it != petitionMsgs.end(); ++it)
    {
        if ((*it)->getClubId() == joinClubId)
        {
            uint32_t petitionId = (*it)->getMessageId();
            error = dbc.getDb().deleteClubMessage(petitionId);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[ClubsSlaveImpl::joinClub] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId
                    << " failed to delete petition " << petitionId);
                return error;
            }
        }
    }

    error = onMemberAdded(dbc.getDb(), joinClub, joinUser);
    if (error != Blaze::ERR_OK)
    {
        return error;
    }

    // Signal master as if the session is created for the new user
    error = updateCachedDataOnNewUserAdded(
        joinClubId,
        joinClub.getName(),
        joinClub.getClubDomainId(),
        joinClub.getClubSettings(),
        joinUser.getUser().getBlazeId(),
        joinUser.getMembershipStatus(),
        joinUser.getMembershipSinceTime(),
        UPDATE_REASON_USER_JOINED_CLUB);
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl::joinClub] updateOnlineStatusUserAdded failed");
        return error;
    }

    error = updateMemberUserExtendedData(joinUser.getUser().getBlazeId(), joinClubId, joinClub.getClubDomainId(), UPDATE_REASON_USER_JOINED_CLUB, getOnlineStatus(joinUser.getUser().getBlazeId()));
    if (error != Blaze::ERR_OK)
    {
        return error;
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::petitionClub(const ClubId petitionClubId, const ClubDomain *domain, const BlazeId rtorBlazeId, bool skipMembershipCheck,
                                           ClubsDbConnector& dbc, uint32_t& messageId)
{
    BlazeRpcError error = Blaze::ERR_OK;
    messageId = 0;

    if (!dbc.isTransactionStart())
    {
        if (!dbc.startTransaction())
            return ERR_SYSTEM;
    }

    // petition can be sent only once
    error = checkPetitionAlreadySent(dbc.getDb(), rtorBlazeId, petitionClubId);
    if (error != Blaze::ERR_OK)
    {
        return error;
    }
    
    if (!skipMembershipCheck)
    {
        MembershipStatus rtorMshipStatus;
        MemberOnlineStatus rtorOnlSatus;
        TimeValue memberSince;

        // Check that the requester is not petitioning to a club that he already belongs to
        error = getMemberStatusInClub(
            petitionClubId, 
            rtorBlazeId, dbc.getDb(), 
            rtorMshipStatus, 
            rtorOnlSatus,
            memberSince);
        if (error == (BlazeRpcError)ERR_OK)
        {
            return CLUBS_ERR_ALREADY_MEMBER;
        }
        if (error != static_cast<BlazeRpcError>(CLUBS_ERR_USER_NOT_MEMBER))
        {
            return error;
        }
    }

    ClubMessage message;
    message.setClubId(petitionClubId);
    message.setMessageType(CLUBS_PETITION);
    message.getSender().setBlazeId(rtorBlazeId);

    // insert the petition to club_messages table and also updates the last active time for the club
    error = dbc.getDb().insertClubMessage(message, domain->getClubDomainId(), domain->getMaxInvitationsPerUserOrClub());
    if (error != Blaze::ERR_OK)
    {
        if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_SENT)
        {
            return CLUBS_ERR_MAX_PETITIONS_SENT;
        }
        else if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_RECEIVED)
        {
            return CLUBS_ERR_MAX_PETITIONS_RECEIVED;
        }
        else
        {
            ERR_LOG("[ClubsSlaveImpl::petitionClub] failed to send petition from user [blazeid]: " 
                    << rtorBlazeId << " error was " << ErrorHelp::getErrorName(error));
            return error;
        }
    }
    messageId = message.getMessageId();
    
    return Blaze::ERR_OK;
}

} // clubs
} // Blaze
