
#ifndef CLUBS_IGNITION_H
#define CLUBS_IGNITION_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/component/clubscomponent.h"
#include "BlazeSDK/component/clubs/tdf/clubs.h"
#include "BlazeSDK/component/clubs/tdf/clubs_base.h"

namespace Ignition
{

class Clubs
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        Clubs();
        virtual ~Clubs();

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    //protected:
        //virtual void onStarting();

    private:
        uint64_t currentClubId;
        
        //Pyro Actions
        PYRO_ACTION(Clubs, CreateClub);
        PYRO_ACTION(Clubs, JoinClub);
        PYRO_ACTION(Clubs, ViewClubRecordbook);
        PYRO_ACTION(Clubs, ViewClubs);
        PYRO_ACTION(Clubs, ViewMembers);
        PYRO_ACTION(Clubs, BanMember);
        PYRO_ACTION(Clubs, PromoteToGM);
        PYRO_ACTION(Clubs, DemoteToMember);
        PYRO_ACTION(Clubs, RemoveUser);
        PYRO_ACTION(Clubs, LeaveClub);
        PYRO_ACTION(Clubs, InviteUserById);
        PYRO_ACTION(Clubs, InviteUserByName);
        PYRO_ACTION(Clubs, ViewInvitations);
        PYRO_ACTION(Clubs, AcceptInvitation);
        PYRO_ACTION(Clubs, DeclineInvitation);
        PYRO_ACTION(Clubs, ChangeSettings);
        PYRO_ACTION(Clubs, ViewPetitions);
        PYRO_ACTION(Clubs, AcceptPetition);
        PYRO_ACTION(Clubs, DeclinePetition);
        PYRO_ACTION(Clubs, DisbandClub);
        PYRO_ACTION(Clubs, GetClubBans);
        PYRO_ACTION(Clubs, UnbanMember);
        PYRO_ACTION(Clubs, TransferOwnership);
        PYRO_ACTION(Clubs, ChangeClubStrings);
        PYRO_ACTION(Clubs, RefreshMemberList);
        PYRO_ACTION(Clubs, GetNews);
        PYRO_ACTION(Clubs, PostNews);
        PYRO_ACTION(Clubs, FindClubs);

        //Callbacks, used to display pyro windows and tables
        void CreateClubCb (const Blaze::Clubs::CreateClubResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void JoinOrPetitionClubCb (const Blaze::Clubs::JoinOrPetitionClubResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ErrorWithIdCb (Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void RemoveMemberCb (Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewClubRecordbookCb (const Blaze::Clubs::GetClubRecordbookResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewClubsCb (const Blaze::Clubs::GetClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void GetClubMembershipForUsersCb (const Blaze::Clubs::GetClubMembershipForUsersResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewMembersCb (const Blaze::Clubs::GetMembersResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewInvitationsCb (const Blaze::Clubs::GetInvitationsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewPetitionsCb (const Blaze::Clubs::GetPetitionsForClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void ViewClubBansCb (const Blaze::Clubs::GetClubBansResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void LookupUsersCb (Blaze::BlazeError blazeError, Blaze::JobId jobid, const Blaze::UserManager::User* user);
        void InviteUserByNameCb (Blaze::BlazeError blazeError, Blaze::JobId jobid, const Blaze::UserManager::User* user);
        void DisbandClubCb (Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void GetNewsForClubsCb (const Blaze::Clubs::GetNewsForClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);
        void FindClubsCb (const Blaze::Clubs::FindClubsResponse *res, Blaze::BlazeError blazeError, Blaze::JobId jobid);

};

}//Ignition

#endif
