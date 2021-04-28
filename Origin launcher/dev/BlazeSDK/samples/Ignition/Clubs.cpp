
#include "Ignition/Clubs.h"

#include "BlazeSDK/component/clubs/tdf/clubs.h"
#include "BlazeSDK/component/clubs/tdf/clubs_base.h"

namespace Ignition
{

Clubs::Clubs()
    : BlazeHubUiBuilder("Clubs")
{
    //Initialize the Clubs component
    gBlazeHub->getComponentManager()->getClubsComponent()->createComponent(gBlazeHub->getComponentManager());
    //getBlazeHub()->getComponentManager()->getClubsComponent()->createComponent(getBlazeHub()->getComponentManager());
    //Add the 5 main actions
    getActions().add(&getActionCreateClub());
    getActions().add(&getActionJoinClub());
    getActions().add(&getActionFindClubs());
    getActions().add(&getActionViewClubs());
    getActions().add(&getActionViewInvitations());
    currentClubId = 0;
}

Clubs::~Clubs()
{
}

void Clubs::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(true);
}

void Clubs::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getWindows().clear();
}

/****************************************************
Initialize the actions
****************************************************/

void Clubs::initActionCreateClub(Pyro::UiNodeAction &action)
{
    action.setText("Create Club");
    action.setDescription("Create a new club");
    
    action.getParameters().addString("name", "", "Club Name", "", "The name unique name of the club");
    action.getParameters().addString("nonuniquename", "", "Non-unique Name", "", "The non-unique name of the club");
    action.getParameters().addUInt64("domain", 10, "Domain", "10,20,30", "The domain of the club, valid values are 10, 20, 30");
    action.getParameters().addString("password", "", "Password", "", "The password for the club, if empty, there won't be a password");
    action.getParameters().addString("description", "", "Description", "", "A description of the club");
    action.getParameters().addEnum("jacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Join Acceptance", "The join permission for the club");
    action.getParameters().addEnum("pacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Petition Acceptance", "The petition permission for the club");
    action.getParameters().addEnum("spcacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Skip Password Check Acceptance", "The skip password-check permission for the club");
    action.getParameters().addString("language", "", "Language", "", "The language of the club");
}

void Clubs::initActionJoinClub(Pyro::UiNodeAction &action)
{
    action.setText("Join Club");
    action.setDescription("Join an existing club");
    
    action.getParameters().addUInt64("clubid", 0, "Club Id", "", "The club id of the club you wish to join");
    action.getParameters().addString("password", "", "Password", "The password (if required) of the club"); 
    action.getParameters().addBool("issetpetitionifjoinfails", true, "Set Petition If Join Fails", "Set petition if join fails");
}


void Clubs::initActionLeaveClub(Pyro::UiNodeAction &action)
{
    action.setText("Leave Club");
}

void Clubs::initActionViewClubRecordbook(Pyro::UiNodeAction &action)
{
    action.setText("View Recordbook");
}

void Clubs::initActionViewClubs(Pyro::UiNodeAction &action)
{
    action.setText("View your Clubs");
    action.setDescription("View clubs that you are a member of");
}

void Clubs::initActionViewMembers(Pyro::UiNodeAction &action)
{
    action.setText("View Club Members");
}

void Clubs::initActionBanMember(Pyro::UiNodeAction &action)
{
    action.setText("Ban Member");
}

void Clubs::initActionPromoteToGM(Pyro::UiNodeAction &action)
{
    action.setText("Promote to GM");
}

void Clubs::initActionDemoteToMember(Pyro::UiNodeAction &action)
{
    action.setText("Demote to Member");
}

void Clubs::initActionRemoveUser(Pyro::UiNodeAction &action)
{
    action.setText("Remove Member");
}

void Clubs::initActionInviteUserById(Pyro::UiNodeAction &action)
{
    action.setText("Invite User by Id");
    action.setDescription("Invite a user to the club based on their blaze id");

    action.getParameters().addUInt64("blazeid", 0, "Blaze Id", "", "The blaze id of the user to invite");
}

void Clubs::initActionInviteUserByName(Pyro::UiNodeAction &action)
{
    action.setText("Invite User by Name");
    action.setDescription("Invite a user to the club based on their persona name");

    action.getParameters().addString("personaname", "", "Persona", "", "The persona name of the user to invite");
}

void Clubs::initActionViewInvitations(Pyro::UiNodeAction &action)
{
    action.setText("View Invitations");
    action.setDescription("View club invitations");
}

void Clubs::initActionAcceptInvitation(Pyro::UiNodeAction &action)
{
    action.setText("Accept Invitation");
}

void Clubs::initActionDeclineInvitation(Pyro::UiNodeAction &action)
{
    action.setText("Decline Invitation");
}

void Clubs::initActionChangeSettings(Pyro::UiNodeAction &action)
{
    action.setText("Change Club Settings");
    action.setDescription("Change the settings for the current club");

    action.getParameters().addString("description", "", "Description", "", "A new description for the club");
    action.getParameters().addString("password", "", "Password", "", "The new password for the club");
    action.getParameters().addEnum("jacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Join acceptance", "The club's new join permissions");
    action.getParameters().addEnum("pacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Petition acceptance", "The club's new petition permissions");
    action.getParameters().addEnum("spcacceptance", Blaze::Clubs::CLUB_ACCEPT_NONE, "Skip Password Check Acceptance", "The skip password-check permission for the club");
    action.getParameters().addString("language", "", "Language", "", "The new language of the club");
}

void Clubs::initActionViewPetitions(Pyro::UiNodeAction &action)
{
    action.setText("View Petitions");
    action.setDescription("View petitions to the club");
}

void Clubs::initActionAcceptPetition(Pyro::UiNodeAction &action)
{
    action.setText("Accept Petition");
}

void Clubs::initActionDeclinePetition(Pyro::UiNodeAction &action)
{
    action.setText("Decline Petition");
}

void Clubs::initActionDisbandClub(Pyro::UiNodeAction &action)
{
    action.setText("Disband Club");
    action.setDescription("All the members in the club will be removed");

    action.getParameters().addBool("yn", false, "Are you sure?", "Confirmation to disband club");
}

void Clubs::initActionGetClubBans(Pyro::UiNodeAction &action)
{
    action.setText("Get Club Bans");
    action.setDescription("View the users that are banned from the club");
}

void Clubs::initActionUnbanMember(Pyro::UiNodeAction &action)
{
    action.setText("Unban Member");
}

void Clubs::initActionTransferOwnership(Pyro::UiNodeAction &action)
{
    action.setText("Transfer Ownership");
}

void Clubs::initActionChangeClubStrings(Pyro::UiNodeAction &action)
{
    action.setText("Change Club Name");
    action.setDescription("Change the name and/or non-unique name of the club");

    action.getParameters().addString("name", "", "Name", "", "The new unique name for the club");
    action.getParameters().addString("clubnonuniquename", "", "Non-unique name", "", "The new non-unique name for the club");
}

void Clubs::initActionRefreshMemberList(Pyro::UiNodeAction &action)
{
    action.setText("Refresh Member List");
    action.setDescription("Refresh the member list");
}

void Clubs::initActionGetNews(Pyro::UiNodeAction &action)
{
    action.setText("Get Club News");
    action.setDescription("Get the news for the club");

    action.getParameters().addEnum("newstype", Blaze::Clubs::CLUBS_ALL_NEWS, "News Type", "The type of news to get");
    action.getParameters().addUInt32("maxresults", 10, "Maximum Number of Results", "", "The maximum amount of results to return");
}

void Clubs::initActionPostNews(Pyro::UiNodeAction &action)
{
    action.setText("Post News");
    action.setDescription("Post news for the club");

    action.getParameters().addEnum("newstype", Blaze::Clubs::CLUBS_ALL_NEWS, "News Type", "The type of news that will be posted");
    action.getParameters().addString("text", "", "News", "", "The message that will be posted");
}

void Clubs::initActionFindClubs(Pyro::UiNodeAction &action)
{
    action.setText("Find Clubs");
    action.setDescription("Find clubs given specific criteria");

    action.getParameters().addBool("anydomain", true, "Any domain?", "Whether or not all domains will be searched");
    action.getParameters().addUInt64("domain", 0, "Domain", "10,20,30", "The domain to search (ignored if Any Domain? is true");
    action.getParameters().addEnum("clubsorder", Blaze::Clubs::CLUBS_NO_ORDER, "Clubs Order", "The order of the clubs");
    action.getParameters().addEnum("joinacceptance", Blaze::Clubs::CLUB_ACCEPTS_UNSPECIFIED, "Join Acceptance", "The join permission of the clubs");
    action.getParameters().addString("language", "", "Language", "", "The language of the club");
    action.getParameters().addUInt32("maxresults", 10, "Max Results", "", "The maximum number of results");
    action.getParameters().addUInt32("maxmembers", 0, "Max Members", "", "The maximum number of members in a club");
    action.getParameters().addUInt32("minmembers", 0, "Min Members", "", "The minimum number of members in a club");
    action.getParameters().addString("clubname", "", "Club Name", "", "The name of the club");
    action.getParameters().addString("nonuniquename", "", "Club Non-unique Name", "", "The non-unique name of the club");
    action.getParameters().addEnum("ordermode", Blaze::Clubs::ASC_ORDER, "Order Mode", "The order to display the clubs in");
    action.getParameters().addEnum("passwordoption", Blaze::Clubs::CLUB_PASSWORD_IGNORE, "Password Option", "Should clubs that have a password be included");
    action.getParameters().addEnum("petitionacceptance", Blaze::Clubs::CLUB_ACCEPTS_UNSPECIFIED, "Petition Acceptance", "The petition permission of the clubs");
}

/***************************************************************
**                                                            **
**                     Action Functionality                   **
**                                                            **
***************************************************************/

void Clubs::actionCreateClub(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::CreateClubRequest req;
    req.setName(parameters["name"]);
    req.setClubDomainId(parameters["domain"]);
    if ((parameters["password"].getAsString() != nullptr) && (parameters["password"].getAsString()[0] != '\0'))
    {
        req.getClubSettings().setHasPassword(true);
        req.getClubSettings().setPassword(parameters["password"]);
    }
    else
    {
        req.getClubSettings().setHasPassword(false);
    }
    req.getClubSettings().setDescription(parameters["description"]);
    req.getClubSettings().setJoinAcceptance(parameters["jacceptance"]);
    req.getClubSettings().setPetitionAcceptance(parameters["pacceptance"]);
    req.getClubSettings().setSkipPasswordCheckAcceptance(parameters["spcacceptance"]);
    req.getClubSettings().setLanguage(parameters["language"]);
    req.getClubSettings().setNonUniqueName(parameters["nonuniquename"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->createClub(req, Blaze::Clubs::ClubsComponent::CreateClubCb(this, &Clubs::CreateClubCb)));
}

void Clubs::actionJoinClub(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::JoinOrPetitionClubRequest req;
    req.setClubId(parameters["clubid"]);
    if ((parameters["password"].getAsString() != nullptr) && (parameters["password"].getAsString()[0] != '\0'))
    {
        req.setPassword(parameters["password"]);
    }
    req.setPetitionIfJoinFails(parameters["issetpetitionifjoinfails"].getAsBool());

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->joinOrPetitionClub(req, Blaze::Clubs::ClubsComponent::JoinOrPetitionClubCb(this, &Clubs::JoinOrPetitionClubCb)));
}

void Clubs::actionLeaveClub(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::RemoveMemberRequest req;
    
    req.setClubId(currentClubId);
    req.setBlazeId(gBlazeHub->getUserManager()->getLocalUser(parameters["userIndex"])->getId());

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->removeMember(req, Blaze::Clubs::ClubsComponent::RemoveMemberCb(this, &Clubs::RemoveMemberCb)));
}

void Clubs::actionViewClubRecordbook(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //set-up the request
    Blaze::Clubs::GetClubRecordbookRequest req;
    req.setClubId(parameters["clubid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getClubRecordbook(req, Blaze::Clubs::ClubsComponent::GetClubRecordbookCb(this, &Clubs::ViewClubRecordbookCb)));
}

void Clubs::actionViewClubs(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Clubs::GetClubMembershipForUsersRequest req;
    req.getBlazeIdList().push_back(gBlazeHub->getUserManager()->getLocalUser(parameters["userIndex"])->getId());

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getClubMembershipForUsers(req, Blaze::Clubs::ClubsComponent::GetClubMembershipForUsersCb(this, &Clubs::GetClubMembershipForUsersCb)));
}

void Clubs::actionViewMembers(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Clubs::GetMembersRequest req;

    if (parameters["clubid"])
    {
        req.setClubId(parameters["clubid"]);
        currentClubId = parameters["clubid"].getAsUInt64();
    }
    else
    {
        req.setClubId(currentClubId);
    }

    //Set-up the value panel; if it hasn't been set-up already
    Pyro::UiNodeValuePanel &valuePanel = getWindow("Clubs").getValuePanel("Settings");
    valuePanel.getValue("description").setText("Description");
    valuePanel.getValue("password").setText("Password");
    valuePanel.getValue("jacceptance").setText("Join Acceptance");
    valuePanel.getValue("pacceptance").setText("Petition Acceptance");
    valuePanel.getValue("language").setText("Language");
    valuePanel.getValue("clubnonuniquename").setText("Club Non-unique Name");
    valuePanel.getValue("clubname").setText("Club Unique Name");
    
    valuePanel.setText("Club Settings");
    valuePanel.getValue("description") = parameters["description"].getAsString();
    valuePanel.getValue("password") = parameters["password"].getAsString();
    int ja = parameters["jacceptance"].getAsInt32();
    int pa = parameters["pacceptance"].getAsInt32();
    if (ja == 0)
        valuePanel.getValue("jacceptance") = "CLUB_ACCEPTS_UNSPECIFIED";
    else if (ja == 1)
        valuePanel.getValue("jacceptance") = "CLUB_ACCEPTS_ALL";
    else
        valuePanel.getValue("jacceptance").setAsInt32(ja);
    if (pa == 0)
        valuePanel.getValue("pacceptance") = "CLUB_ACCEPTS_UNSPECIFIED";
    else if (pa == 1)
        valuePanel.getValue("pacceptance") = "CLUB_ACCEPTS_ALL";
    else
        valuePanel.getValue("pacceptance").setAsInt32(pa);
    //valuePanel.getValue("jacceptance") = 0;
    //valuePanel.getValue("pacceptance") = 0;//gClubsRequestorAcceptanceEnum.getArray()[parameters["pacceptance"].getAsUInt32()].first;
    valuePanel.getValue("language") = parameters["language"].getAsString();
    valuePanel.getValue("clubnonuniquename") = parameters["clubnonuniquename"].getAsString();
    valuePanel.getValue("clubname") = parameters["clubname"].getAsString();
    
    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getMembers(req, Blaze::Clubs::ClubsComponent::GetMembersCb(this, &Clubs::ViewMembersCb)));
}

void Clubs::actionBanMember(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::BanUnbanMemberRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->banMember(req, Blaze::Clubs::ClubsComponent::BanMemberCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionPromoteToGM(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::PromoteToGMRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->promoteToGM(req, Blaze::Clubs::ClubsComponent::PromoteToGMCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionDemoteToMember(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::DemoteToMemberRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->demoteToMember(req, Blaze::Clubs::ClubsComponent::DemoteToMemberCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionRemoveUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::RemoveMemberRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    if (parameters["memberid"].getAsInt64() == gBlazeHub->getUserManager()->getLocalUser(parameters["userIndex"])->getId())
    {
        vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->removeMember(req, Blaze::Clubs::ClubsComponent::RemoveMemberCb(this, &Clubs::RemoveMemberCb)));
    }
    else
    {
        vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->removeMember(req, Blaze::Clubs::ClubsComponent::RemoveMemberCb(this, &Clubs::ErrorWithIdCb)));
    }
}

void Clubs::actionInviteUserById(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::SendInvitationRequest req;
    req.setClubId(currentClubId);
    req.setBlazeId(parameters["blazeid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->sendInvitation(req, Blaze::Clubs::ClubsComponent::SendInvitationCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionInviteUserByName(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getUserManager()->lookupUserByName(parameters["personaname"].getAsString(), Blaze::UserManager::UserManager::LookupUserCb(this, &Clubs::InviteUserByNameCb)));
}

void Clubs::actionViewInvitations(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Declare the request
    Blaze::Clubs::GetInvitationsRequest req;
    
    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getInvitations(req, Blaze::Clubs::ClubsComponent::GetInvitationsCb(this, &Clubs::ViewInvitationsCb)));
}

void Clubs::actionAcceptInvitation(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::ProcessInvitationRequest req;
    req.setInvitationId(parameters["messageid"]);
    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->acceptInvitation(req, Blaze::Clubs::ClubsComponent::AcceptInvitationCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionDeclineInvitation(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::ProcessInvitationRequest req;
    req.setInvitationId(parameters["messageid"]);
    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->declineInvitation(req, Blaze::Clubs::ClubsComponent::DeclineInvitationCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionChangeSettings(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Add clubid to parameters
    //parameters.addUInt64("clubid", currentClubId, "Club Id");

    Blaze::Clubs::UpdateClubSettingsRequest req;
    req.setClubId(currentClubId/*parameters["clubid"]*/);
    req.getClubSettings().setDescription(parameters["description"]);
    req.getClubSettings().setJoinAcceptance(parameters["jacceptance"]);
    req.getClubSettings().setLanguage(parameters["language"]);
    req.getClubSettings().setPetitionAcceptance(parameters["pacceptance"]);
    req.getClubSettings().setSkipPasswordCheckAcceptance(parameters["spcacceptance"]);
    //Set-up password
    if ((parameters["password"].getAsString() != nullptr) && (parameters["password"].getAsString()[0] != '\0'))
    {
        req.getClubSettings().setHasPassword(true);
        req.getClubSettings().setPassword(parameters["password"]);
    }
    else
    {
        req.getClubSettings().setHasPassword(false);
    }
    Pyro::UiNodeValuePanel &valuePanel = getWindow("Clubs").getValuePanel("Settings");
    req.getClubSettings().setNonUniqueName(valuePanel.getValue("clubnonuniquename"));
    
    //Update the value panel
    valuePanel.getValue("description") = parameters["description"].getAsString();
    valuePanel.getValue("password") = parameters["password"].getAsString();
    int ja = parameters["jacceptance"].getAsInt32();
    int pa = parameters["pacceptance"].getAsInt32();
    if (ja == 0)
        valuePanel.getValue("jacceptance") = "CLUB_ACCEPTS_UNSPECIFIED";
    else if (ja == 1)
        valuePanel.getValue("jacceptance") = "CLUB_ACCEPTS_ALL";
    else
        valuePanel.getValue("jacceptance").setAsInt32(ja);
    if (pa == 0)
        valuePanel.getValue("pacceptance") = "CLUB_ACCEPTS_UNSPECIFIED";
    else if (pa == 1)
        valuePanel.getValue("pacceptance") = "CLUB_ACCEPTS_ALL";
    else
        valuePanel.getValue("pacceptance").setAsInt32(pa);
    //valuePanel.getValue("jacceptance") = parameters["jacceptance"].getAsUInt32();
    //valuePanel.getValue("pacceptance") = parameters["pacceptance"].getAsUInt32();
    valuePanel.getValue("language") = parameters["language"].getAsString();

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->updateClubSettings(req, Blaze::Clubs::ClubsComponent::UpdateClubSettingsCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionViewPetitions(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::GetPetitionsForClubsRequest req;
    req.getClubIdList().push_back(currentClubId);
    req.setPetitionsType(Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getPetitionsForClubs(req, Blaze::Clubs::ClubsComponent::GetPetitionsForClubsCb(this, &Clubs::ViewPetitionsCb)));
}

void Clubs::actionAcceptPetition(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::ProcessPetitionRequest req;
    req.setPetitionId(parameters["messageid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->acceptPetition(req, Blaze::Clubs::ClubsComponent::AcceptPetitionCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionDeclinePetition(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::ProcessPetitionRequest req;
    req.setPetitionId(parameters["messageid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->declinePetition(req, Blaze::Clubs::ClubsComponent::DeclinePetitionCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionDisbandClub(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (parameters["yn"].getAsBool() == false)
        return;

    //Set-up the request
    Blaze::Clubs::DisbandClubRequest req;
    req.setClubId(currentClubId);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->disbandClub(req, Blaze::Clubs::ClubsComponent::DisbandClubCb(this, &Clubs::DisbandClubCb)));
}

void Clubs::actionGetClubBans(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::GetClubBansRequest req;
    req.setClubId(currentClubId);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getClubBans(req, Blaze::Clubs::ClubsComponent::GetClubBansCb(this, &Clubs::ViewClubBansCb)));
}

void Clubs::actionUnbanMember(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::BanUnbanMemberRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->unbanMember(req, Blaze::Clubs::ClubsComponent::UnbanMemberCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionTransferOwnership(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Set-up the request
    Blaze::Clubs::TransferOwnershipRequest req;
    req.setClubId(parameters["clubid"]);
    req.setBlazeId(parameters["memberid"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->transferOwnership(req, Blaze::Clubs::ClubsComponent::TransferOwnershipCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionChangeClubStrings(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Add the current clubid to the parameters
    //parameters.addUInt64("clubid", currentClubId, "Club Id");
    
    //Set-up the request
    Blaze::Clubs::ChangeClubStringsRequest req;
    req.setClubId(currentClubId/*parameters["clubid"]*/);
    req.setName(parameters["name"]);
    req.setNonUniqueName(parameters["clubnonuniquename"]);

    Pyro::UiNodeValuePanel &valuePanel = getWindow("Clubs").getValuePanel("Settings");
    req.setDescription(valuePanel.getValue("description"));

    //Make sure the value panel is updated to the new changes
    valuePanel.getValue("description") = parameters["description"].getAsString();
    valuePanel.getValue("clubnonuniquename") = parameters["clubnonuniquename"].getAsString();
    valuePanel.getValue("clubname") = parameters["name"].getAsString();

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->changeClubStrings(req, Blaze::Clubs::ClubsComponent::ChangeClubStringsCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionRefreshMemberList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //Add the current clubid to the parameters
    //parameters.addUInt64("clubid", currentClubId, "Club Id");

    //Set the description of the value panel so that it isn't set-up again
    getWindow("Clubs").getValuePanel("Settings").setDescription("Initialized");

    actionViewMembers(action, parameters);
}

void Clubs::actionGetNews(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Clubs::GetNewsForClubsRequest req;
    req.getClubIdList().push_back(currentClubId);
    req.getTypeFilters().push_back(parameters["newstype"]);
    req.setMaxResultCount(parameters["maxresults"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getNewsForClubs(req, Blaze::Clubs::ClubsComponent::GetNewsForClubsCb(this, &Clubs::GetNewsForClubsCb)));
}

void Clubs::actionPostNews(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Clubs::PostNewsRequest req;
    req.setClubId(currentClubId);
    req.getClubNews().setType(parameters["newstype"]);
    req.getClubNews().getUser().setBlazeId(gBlazeHub->getUserManager()->getLocalUser(parameters["userIndex"])->getId());
    req.getClubNews().setText(parameters["text"].getAsString());
    req.getClubNews().setTimestamp(gBlazeHub->getCurrentTime());

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->postNews(req, Blaze::Clubs::ClubsComponent::PostNewsCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::actionFindClubs(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Clubs::FindClubsRequest req;
    req.setAnyDomain(parameters["anydomain"].getAsBool());
    req.setClubDomainId(parameters["domain"]);
    req.setClubsOrder(parameters["clubsorder"]);
    req.setJoinAcceptance(parameters["joinacceptance"]);
    req.setLanguage(parameters["language"]);
    req.setMaxResultCount(parameters["maxresults"]);
    req.setMaxMemberCount(parameters["maxmembers"]);
    req.setMinMemberCount(parameters["minmembers"]);
    req.setName(parameters["clubname"]);
    req.setNonUniqueName(parameters["nonuniquename"]);
    req.setOrderMode(parameters["ordermode"]);
    req.setPasswordOption(parameters["passwordoption"]);
    req.setPetitionAcceptance(parameters["petitionacceptance"]);

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->findClubs(req, Blaze::Clubs::ClubsComponent::FindClubsCb(this, &Clubs::FindClubsCb)));
}

/**************************************************
**                                               **
**                   Callbacks                   **
**                                               **
**************************************************/

void Clubs::CreateClubCb (const Blaze::Clubs::CreateClubResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError != Blaze::ERR_OK)
        REPORT_BLAZE_ERROR(blazeError);
}

void Clubs::JoinOrPetitionClubCb (const Blaze::Clubs::JoinOrPetitionClubResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError != Blaze::ERR_OK)
        REPORT_BLAZE_ERROR(blazeError);
}

void Clubs::ErrorWithIdCb (Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError != Blaze::ERR_OK)
        REPORT_BLAZE_ERROR(blazeError);
}

void Clubs::RemoveMemberCb (Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        getWindows().removeById("Clubs");
        getWindows().removeById("Petitions");
        getWindows().removeById("Banned");
        getWindows().removeById("News");
        currentClubId = 0;
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewClubRecordbookCb (const Blaze::Clubs::GetClubRecordbookResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubRecordList clubRecordList;
        res->getClubRecordList().copyInto(clubRecordList);

        //set-up the table
        Pyro::UiNodeTable &table = getWindow("Club Recordbook").getTable("Records");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("recordid").setText("Record Id");
        table.getColumn("recordname").setText("Record Name");
        table.getColumn("recorddescription").setText("Record Description");
        table.getColumn("persona").setText("Record Holder");
        table.getColumn("blazeid").setText("Blaze Id");
        table.getColumn("stats").setText("Stats");

        for (unsigned int i = 0; i < clubRecordList.size(); i++)
        {
            //put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubRecordList[i]->getRecordId()).getFields();
            fields["recordid"] = clubRecordList[i]->getRecordId();
            fields["recordname"] = clubRecordList[i]->getRecordName();
            fields["recorddescription"] = clubRecordList[i]->getRecordDescription();
            fields["persona"] = clubRecordList[i]->getUser().getName();
            fields["blazeid"] = clubRecordList[i]->getUser().getBlazeId();
            fields["stats"] = clubRecordList[i]->getRecordStat();
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewClubsCb (const Blaze::Clubs::GetClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubList clubList;
        res->getClubList().copyInto(clubList);

        //Set-up the table
        Pyro::UiNodeTable &table = getWindow("Your Clubs").getTable("Clubs");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("clubid").setText("Club Id");
        table.getColumn("clubdomain").setText("Club Domain");
        table.getColumn("clubname").setText("Club Name");
        table.getColumn("clubnonuniquename").setText("Non-Unique Name");
        table.getColumn("password").setText("Password");
        table.getColumn("nummembers").setText("Members");
        table.getColumn("description").setText("Description");
        table.getColumn("jacceptance").setText("Join Acceptance");
        table.getColumn("pacceptance").setText("Petition Acceptance");
        table.getColumn("spcacceptance").setText("Skip Password Check Acceptance");
        table.getColumn("language").setText("Language");

        table.getRows().clear();

        table.getActions().insert(&getActionViewMembers());
        table.getActions().insert(&getActionViewClubRecordbook());

        for (unsigned int i = 0; i < clubList.size(); i++)
        {
            //put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubList[i]->getClubId()).getFields();
            fields["clubid"] = clubList[i]->getClubId();
            fields["clubdomain"] = clubList[i]->getClubDomainId();
            fields["clubname"] = clubList[i]->getName();
            fields["clubnonuniquename"] = clubList[i]->getClubSettings().getNonUniqueName();
            fields["password"] = clubList[i]->getClubSettings().getPassword();
            fields["nummembers"] = clubList[i]->getClubInfo().getMemberCount();
            fields["description"] = clubList[i]->getClubSettings().getDescription();
            int ja = clubList[i]->getClubSettings().getJoinAcceptance();
            int pa = clubList[i]->getClubSettings().getPetitionAcceptance();
            int spca = clubList[i]->getClubSettings().getSkipPasswordCheckAcceptance();
            if (ja == 0)
                fields["jacceptance"] = "CLUB_ACCEPT_NONE";
            else if (ja == 1)
                fields["jacceptance"] = "CLUB_ACCEPT_GM_FRIENDS";
            else if (ja == 2)
                fields["jacceptance"] = "CLUB_ACCEPT_ALL";
            else
                fields["jacceptance"].setAsInt32(ja);

            if (pa == 0)
                fields["pacceptance"] = "CLUB_ACCEPT_NONE";
            else if (pa == 1)
                fields["pacceptance"] = "CLUB_ACCEPT_GM_FRIENDS";
            else if (pa == 2)
                fields["pacceptance"] = "CLUB_ACCEPT_ALL";
            else
                fields["pacceptance"].setAsInt32(pa);

            if (spca == 0)
                fields["spcacceptance"] = "CLUB_ACCEPT_NONE";
            else if (spca == 1)
                fields["spcacceptance"] = "CLUB_ACCEPT_GM_FRIENDS";
            else if (spca == 2)
                fields["spcacceptance"] = "CLUB_ACCEPT_ALL";
            else
                fields["spcacceptance"].setAsInt32(spca);
            //fields["jacceptance"] = clubList[i]->getClubSettings().getJoinAcceptance();//gClubsRequestorAcceptanceEnum.getArray()[clubList[i]->getClubSettings().getJoinAcceptance()].first;
            //fields["pacceptance"] = clubList[i]->getClubSettings().getPetitionAcceptance();//gClubsRequestorAcceptanceEnum.getArray()[clubList[i]->getClubSettings().getPetitionAcceptance()].first;
            fields["language"] = clubList[i]->getClubSettings().getLanguage();
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::GetClubMembershipForUsersCb (const Blaze::Clubs::GetClubMembershipForUsersResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubMembershipMap clubMembershipMap;
        res->getMembershipMap().copyInto(clubMembershipMap);

        Blaze::Clubs::ClubMembershipMap::iterator listIt = clubMembershipMap.begin();

        Blaze::Clubs::GetClubsRequest req;
        
        for (; listIt < clubMembershipMap.end(); listIt++)
        {
            Blaze::Clubs::ClubMembershipList clubMembershipList;
            listIt->second->getClubMembershipList().copyInto(clubMembershipList);

            for (unsigned int i = 0; i < clubMembershipList.size(); i++)
            {
                req.getClubIdList().push_back(clubMembershipList[i]->getClubId());
            }
        }

        //If its empty just display the table
        if (req.getClubIdList().size() == 0)
        {
            Pyro::UiNodeTable &table = getWindow("Your Clubs").getTable("Clubs");
            table.setPosition(Pyro::ControlPosition::TOP_LEFT);

            table.getColumn("clubid").setText("Club Id");
            table.getColumn("clubdomain").setText("Club Domain");
            table.getColumn("clubname").setText("Club Name");
            table.getColumn("clubnonuniquename").setText("Non-Unique Name");
            table.getColumn("password").setText("Password");
            table.getColumn("nummembers").setText("Members");
            table.getColumn("description").setText("Description");
            table.getColumn("jacceptance").setText("Join Acceptance");
            table.getColumn("pacceptance").setText("Petition Acceptance");
            table.getColumn("language").setText("Language");

            table.getRows().clear();

            return;
        }

        vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->getClubs(req, Blaze::Clubs::ClubsComponent::GetClubsCb(this, &Clubs::ViewClubsCb)));
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewMembersCb (const Blaze::Clubs::GetMembersResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if(res == nullptr)
        return;
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubMemberList clubMemberList;
        res->getClubMemberList().copyInto(clubMemberList); 

        //Set-up the Member List table
        Pyro::UiNodeTable &table = getWindow("Clubs").getTable("Member List");
        table.setPosition(Pyro::ControlPosition::TOP_RIGHT);

        table.getColumn("clubid").setText("Club Id");
        table.getColumn("memberid").setText("Blaze Id");
        table.getColumn("membername").setText("Name");
        table.getColumn("memberstatus").setText("Status");
        table.getColumn("rank").setText("Rank");
    
        //Clear the table so we don't get old data
        table.getRows().clear();

        //Insert the right click actions
        table.getActions().insert(&getActionBanMember());
        table.getActions().insert(&getActionPromoteToGM());
        table.getActions().insert(&getActionDemoteToMember());
        table.getActions().insert(&getActionRemoveUser());
        table.getActions().insert(&getActionTransferOwnership());

        //Set-up the action panel
        Pyro::UiNodeActionGroup &actionGroup = getWindow("Clubs").getActionPanel("Club Actions").getGroup("Club Actions");
        
        actionGroup.getActions().insert(&getActionRefreshMemberList());
        actionGroup.getActions().insert(&getActionInviteUserById());
        actionGroup.getActions().insert(&getActionInviteUserByName());
        actionGroup.getActions().insert(&getActionLeaveClub());
        actionGroup.getActions().insert(&getActionPostNews());
        actionGroup.getActions().insert(&getActionGetNews());
        actionGroup.getActions().insert(&getActionChangeSettings());
        actionGroup.getActions().insert(&getActionChangeClubStrings());
        actionGroup.getActions().insert(&getActionViewPetitions());
        actionGroup.getActions().insert(&getActionGetClubBans());
        actionGroup.getActions().insert(&getActionDisbandClub());       

        for (unsigned int i = 0; i < clubMemberList.size(); i++)
        {
            //Put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubMemberList[i]->getUser().getBlazeId()).getFields();
            fields["clubid"] = currentClubId;
            fields["memberid"] = clubMemberList[i]->getUser().getBlazeId();
            fields["membername"] = clubMemberList[i]->getUser().getName();
            if (clubMemberList[i]->getOnlineStatus() == 0)
            {
                fields["memberstatus"] = "OFFLINE";
            }
            else
            {
                fields["memberstatus"] = "ONLINE";
            }
            if (clubMemberList[i]->getMembershipStatus() == 0)
            {
                fields["rank"] = "Member";  
            }
            else if (clubMemberList[i]->getMembershipStatus() == 1)
            {
                fields["rank"] = "GM";
            }
            else
            {
                fields["rank"] = "Owner";
            }
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewInvitationsCb(const Blaze::Clubs::GetInvitationsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubMessageList clubInvList;
        res->getClubInvList().copyInto(clubInvList);

        //Set-up the table
        Pyro::UiNodeTable &table = getWindow("Invitations").getTable("Invitations");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("messageid").setText("Message Id");
        table.getColumn("clubid").setText("Club Id");
        table.getColumn("clubname").setText("Club Name");
        table.getColumn("messagetype").setText("Message Type");
        table.getColumn("sender").setText("Sender");

        table.getRows().clear();

        table.getActions().insert(&getActionAcceptInvitation());
        table.getActions().insert(&getActionDeclineInvitation());

        for (unsigned int i = 0; i < clubInvList.size(); i++)
        {
            //Put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubInvList[i]->getMessageId()).getFields();
            fields["messageid"] = clubInvList[i]->getMessageId();
            fields["clubid"] = clubInvList[i]->getClubId();
            fields["clubname"] = clubInvList[i]->getClubName();
            fields["messagetype"] = "CLUBS_INVITATION";
            fields["sender"] = clubInvList[i]->getSender().getName();
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewPetitionsCb (const Blaze::Clubs::GetPetitionsForClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubMessageListMap clubPetitionListMap;
        res->getClubPetitionListMap().copyInto(clubPetitionListMap);

        Blaze::Clubs::ClubMessageListMap::iterator listIt = clubPetitionListMap.begin();

        //Set-up the table
        Pyro::UiNodeTable &table = getWindow("Petitions").getTable("Petitions");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("messageid").setText("Message Id");
        table.getColumn("clubid").setText("Club Id");
        table.getColumn("clubname").setText("Club Name");
        table.getColumn("messagetype").setText("Message Type");
        table.getColumn("sender").setText("Sender");

        table.getRows().clear();

        table.getActions().insert(&getActionAcceptPetition());
        table.getActions().insert(&getActionDeclinePetition());

        for (; listIt < clubPetitionListMap.end(); listIt++)
        {
            //copy the messagelist from the map
            Blaze::Clubs::ClubMessageList clubMessageList;
            listIt->second->copyInto(clubMessageList);

            for (unsigned int i = 0; i < clubMessageList.size(); i++)
            {
                //Put the data in the fields
                Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubMessageList[i]->getMessageId()).getFields();
                fields["messageid"] = clubMessageList[i]->getMessageId();
                fields["clubid"] = clubMessageList[i]->getClubId();
                fields["clubname"] = clubMessageList[i]->getClubName();
                fields["messagetype"] = "CLUBS_PETITION";
                fields["sender"] = clubMessageList[i]->getSender().getName();
            }
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::ViewClubBansCb(const Blaze::Clubs::GetClubBansResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::BlazeIdToBanStatusMap idToBan;
        res->getBlazeIdToBanStatusMap().copyInto(idToBan);

        Blaze::Clubs::BlazeIdToBanStatusMap::iterator listIt = idToBan.begin();

        //set-up the table
        Pyro::UiNodeTable &table = getWindow("Banned").getTable("Banned");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("clubid").setText("Banned From");
        table.getColumn("memberid").setText("Blaze Id");
        table.getColumn("membername").setText("Name");

        table.getRows().clear();

        table.getActions().insert(&getActionUnbanMember());

        for (; listIt < idToBan.end(); listIt++)
        {
            //put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", listIt->first).getFields();
            fields["clubid"] = currentClubId;
            fields["memberid"] = listIt->first;

            vLOG(gBlazeHub->getUserManager()->lookupUserByBlazeId(listIt->first, Blaze::UserManager::UserManager::LookupUserCb(this, &Clubs::LookupUsersCb)));
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::LookupUsersCb (Blaze::BlazeError blazeError, Blaze::JobId jobid, const Blaze::UserManager::User* user)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Pyro::UiNodeTable &table = getWindow("Banned").getTable("Banned");
        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", user->getId()).getFields();
        fields["membername"] = user->getName();
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::InviteUserByNameCb (Blaze::BlazeError blazeError, Blaze::JobId jobid, const Blaze::UserManager::User* user)
{
    //Set-up the request
    Blaze::Clubs::SendInvitationRequest req;
    req.setClubId(currentClubId);
    req.setBlazeId(user->getId());

    vLOG(gBlazeHub->getComponentManager()->getClubsComponent()->sendInvitation(req, Blaze::Clubs::ClubsComponent::SendInvitationCb(this, &Clubs::ErrorWithIdCb)));
}

void Clubs::DisbandClubCb (Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError != Blaze::ERR_OK)
        REPORT_BLAZE_ERROR(blazeError);
}

void Clubs::GetNewsForClubsCb (const Blaze::Clubs::GetNewsForClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubLocalizedNewsListMap localizedNewsListMap;
        res->getLocalizedNewsListMap().copyInto(localizedNewsListMap);

        //Set-up the table
        Pyro::UiNodeTable &table = getWindow("News").getTable("Club News");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("newsid").setText("News Id");
        table.getColumn("name").setText("Posted By");
        table.getColumn("timestamp").setText("Timestamp");
        table.getColumn("text").setText("News");

        table.getRows().clear();

        Blaze::Clubs::ClubLocalizedNewsListMap::iterator listIt = localizedNewsListMap.begin();

        for (; listIt < localizedNewsListMap.end(); listIt++)
        {
            Blaze::Clubs::ClubLocalizedNewsList localizedNewsList;
            listIt->second->copyInto(localizedNewsList);

            for (unsigned int i = 0; i < localizedNewsList.size(); i++)
            {
                Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%s", localizedNewsList[i]->getNewsId().toString().c_str()).getFields();
                fields["newsid"] = localizedNewsList[i]->getNewsId().toString().c_str();
                fields["name"] = localizedNewsList[i]->getUser().getName();
                fields["timestamp"] = localizedNewsList[i]->getTimestamp();
                fields["text"] = localizedNewsList[i]->getText();
            }
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void Clubs::FindClubsCb (const Blaze::Clubs::FindClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid)
{
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Clubs::ClubList clubList;
        res->getClubList().copyInto(clubList);

        //Set-up the table
        Pyro::UiNodeTable &table = getWindow("Find Clubs").getTable("Clubs");
        table.setPosition(Pyro::ControlPosition::TOP_LEFT);

        table.getColumn("clubid").setText("Club Id");
        table.getColumn("clubdomain").setText("Club Domain");
        table.getColumn("clubname").setText("Club Name");
        table.getColumn("clubnonuniquename").setText("Non-Unique Name");
        table.getColumn("password").setText("Password");
        table.getColumn("nummembers").setText("Members");
        table.getColumn("description").setText("Description");
        table.getColumn("jacceptance").setText("Join Acceptance");
        table.getColumn("pacceptance").setText("Petition Acceptance");
        table.getColumn("language").setText("Language");

        table.getRows().clear();

        for (unsigned int i = 0; i < res->getTotalCount(); i++)
        {
            //put the data in the fields
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%d", clubList[i]->getClubId()).getFields();
            fields["clubid"] = clubList[i]->getClubId();
            fields["clubdomain"] = clubList[i]->getClubDomainId();
            fields["clubname"] = clubList[i]->getName();
            fields["clubnonuniquename"] = clubList[i]->getClubSettings().getNonUniqueName();
            fields["password"] = clubList[i]->getClubSettings().getPassword();
            fields["nummembers"] = clubList[i]->getClubInfo().getMemberCount();
            fields["description"] = clubList[i]->getClubSettings().getDescription();
            int ja = clubList[i]->getClubSettings().getJoinAcceptance();
            int pa = clubList[i]->getClubSettings().getPetitionAcceptance();
            if (ja == 0)
                fields["jacceptance"] = "CLUB_ACCEPTS_UNSPECIFIED";
            else if (ja == 1)
                fields["jacceptance"] = "CLUB_ACCEPTS_ALL";
            else
                fields["jacceptance"].setAsInt32(ja);
            if (pa == 0)
                fields["pacceptance"] = "CLUB_ACCEPTS_UNSPECIFIED";
            else if (pa == 1)
                fields["pacceptance"] = "CLUB_ACCEPTS_ALL";
            else
                fields["pacceptance"].setAsInt32(pa);
            //fields["jacceptance"] = SearchReqAcc.getValueAt(clubList[i]->getClubSettings().getJoinAcceptance());//gClubsRequestorAcceptanceEnum.getArray()[clubList[i]->getClubSettings().getJoinAcceptance()].first;
            //fields["pacceptance"] = SearchReqAcc.getValueAt(clubList[i]->getClubSettings().getPetitionAcceptance());//gClubsRequestorAcceptanceEnum.getArray()[clubList[i]->getClubSettings().getPetitionAcceptance()].first;
            fields["language"] = clubList[i]->getClubSettings().getLanguage();
        }
    }
    else
    {
        REPORT_BLAZE_ERROR(blazeError);
    }
}

}
