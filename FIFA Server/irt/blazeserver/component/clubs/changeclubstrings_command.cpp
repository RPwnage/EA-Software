/*************************************************************************************************/
/*!
    \file   changeclubstrings_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ChangeClubStringsCommand

    Change club name, non-unique name and/or description after creation

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/profanityfilter.h"

// clubs includes
#include "clubs/rpc/clubsslave/changeclubstrings_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class ChangeClubStringsCommand : public ChangeClubStringsCommandStub, private ClubsCommandBase
{
public:
    ChangeClubStringsCommand(Message* message, ChangeClubStringsRequest* request, ClubsSlaveImpl* componentImpl)
        : ChangeClubStringsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~ChangeClubStringsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    // storage space for profanity
    // declared here to be allocated on heap space (rather than on stack space)
    char8_t mProfanityStorage[OC_MAX_NAME_LENGTH + OC_MAX_NON_UNIQUE_NAME_LENGTH + OC_MAX_DESCR_LENGTH + 1];

    ChangeClubStringsCommandStub::Errors execute() override
    {
        TRACE_LOG("[ChangeClubStringsCommand] start");

        BlazeRpcError error;

        const char8_t *clubName = mRequest.getName();
        if (*clubName == '\0')
            clubName = nullptr;

        const char8_t *clubNonUniqueName = mRequest.getNonUniqueName();
        if (*clubNonUniqueName == '\0')
            clubNonUniqueName = nullptr;

        const char8_t *clubDescription = mRequest.getDescription();
        if (*clubDescription == '\0')
            clubDescription = nullptr;

        if (clubName == nullptr && clubNonUniqueName == nullptr && clubDescription == nullptr)
        {
            // nothing to do
            return ERR_OK;
        }

        error = mComponent->checkClubStrings(
            clubName, 
            clubNonUniqueName,
            clubDescription);

        if (error != Blaze::ERR_OK)
            return commandErrorFromBlazeError(error);

        BlazeId requestorId = gCurrentUserSession->getBlazeId();
        ClubId clubId = mRequest.getClubId();

        ClubsDbConnector dbc(mComponent);
        dbc.startTransaction();

        bool isAdmin = false;
        bool rtorIsThisClubGM = false;

        MembershipStatus mshipStatus;
        MemberOnlineStatus onlStatus;
        TimeValue memberSince;
        error = mComponent->getMemberStatusInClub(
            clubId, 
            requestorId, 
            dbc.getDb(), 
            mshipStatus, 
            onlStatus,
            memberSince);

        // only GMs or users authorized as Clubs Administrator can access this RPC
        if (error == Blaze::ERR_OK)
        {
            rtorIsThisClubGM = (mshipStatus >= CLUBS_GM);
        }

        if (!rtorIsThisClubGM)
        {
            isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

            if (!isAdmin)
            {
                if(error == Blaze::CLUBS_ERR_USER_NOT_MEMBER || error == Blaze::ERR_OK)
                {
                    TRACE_LOG("[ChangeClubStringsCommand] user [" << requestorId
                        << "] is not admin for club [" << clubId << "]");
                    if (error == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                }
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
        }

        ClubSettings clubSettings;
        error = mComponent->getClubSettings(dbc.getDbConn(), clubId, clubSettings);

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ChangeClubStringsCommand] could not get club settings for club " << clubId);
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        uint64_t version = 0;
        error = dbc.getDb().changeClubStrings(
            clubId, 
            clubName, 
            clubNonUniqueName,
            clubDescription,
            version);

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        ClubSettings originalSettings;
        clubSettings.copyInto(originalSettings);

        clubSettings.setNonUniqueName(mRequest.getNonUniqueName());

        error = mComponent->onSettingsUpdated(&dbc.getDb(), clubId, &originalSettings, &clubSettings);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ChangeClubStringsCommand] custom code returned error code when processing club " << clubId);
            return commandErrorFromBlazeError(error);
        }

        const ClubDomain *clubDomain;
        error = mComponent->getClubDomainForClub(clubId, dbc.getDb(), clubDomain);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ChangeClubStringsCommand] could not find domain for club " << clubId);
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        if (clubName != nullptr)
        {
            Club club;
            club.setClubDomainId(clubDomain->getClubDomainId());
            club.setName(clubName);
            club.setClubId(clubId);
            clubSettings.copyInto(club.getClubSettings());
            mComponent->updateCachedClubInfo(version, clubId, club);
            mComponent->removeIdentityCacheEntry(clubId);
        }

        dbc.completeTransaction(true); 

        mComponent->mPerfAdminActions.increment();

        return ERR_OK;
    }
};

// static factory method impl
DEFINE_CHANGECLUBSTRINGS_CREATE()

} // Clubs
} // Blaze
