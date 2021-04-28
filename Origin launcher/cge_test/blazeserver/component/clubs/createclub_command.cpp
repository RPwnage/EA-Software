/*************************************************************************************************/
/*!
    \file   createclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CreateClubCommand

    Create a club in the Database.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/profanityfilter.h"

// clubs includes
#include "clubs/rpc/clubsslave/createclub_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsdb.h"

namespace Blaze
{
namespace Clubs
{

class CreateClubCommand : public CreateClubCommandStub, private ClubsCommandBase
{
public:
    CreateClubCommand(Message* message, CreateClubRequest* request, ClubsSlaveImpl* componentImpl)
        : CreateClubCommandStub(message, request),
        ClubsCommandBase(componentImpl),
        mComponentImp(componentImpl)
    {
    }

    ~CreateClubCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    BlazeRpcError checkRequestParameters()
    {
        BlazeRpcError result = Blaze::ERR_OK;

        // check tags
        result = mComponent->checkClubTags(mRequest.getTagList());
        if (result != Blaze::ERR_OK)
            return result;

        // check club strings
        result = mComponent->checkClubStrings(
            mRequest.getName(), 
            mRequest.getClubSettings().getNonUniqueName(),
            mRequest.getClubSettings().getDescription(),
            mRequest.getClubSettings().getPassword());
        if (result != Blaze::ERR_OK)
            return result;

        return Blaze::ERR_OK;
    }

    CreateClubCommandStub::Errors execute() override
    {
        TRACE_LOG("[CreateClubCommand] start");

        BlazeRpcError result = Blaze::ERR_SYSTEM;

        // check parameters of request (without database connection)
        result = checkRequestParameters();
        if (result != Blaze::ERR_OK)
            return commandErrorFromBlazeError(result);

        const ClubDomain* domain = mComponent->getClubDomain(mRequest.getClubDomainId());
        if (domain == nullptr)
        {
            return CLUBS_ERR_INVALID_DOMAIN_ID;
        }

        ClubMemberStatusList statusList;
        ClubId clubId = INVALID_CLUB_ID;
        BlazeId blazeId = gCurrentUserSession->getBlazeId();
		mRequest.getClubSettings().setLastUpdatedBy(blazeId);

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        BlazeRpcError error = dbc.getDb().lockMemberDomain(blazeId, domain->getClubDomainId());
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        result = mComponent->getMemberStatusInDomain(
            mRequest.getClubDomainId(),
            blazeId, 
            dbc.getDb(), 
            statusList,
            true);

        if (static_cast<uint16_t>(statusList.size()) >= domain->getMaxMembershipsPerUser())
        {
            // user is already member of maximum number of clubs in this domain
            dbc.completeTransaction(false);
            return CLUBS_ERR_MAX_CLUBS;
        }

        result = 
            dbc.getDb().insertClub(
            mRequest.getClubDomainId(),
            mRequest.getName(), 
            mRequest.getClubSettings(), 
            clubId);

        if (result != Blaze::ERR_OK)
        {
            if (result != Blaze::CLUBS_ERR_CLUB_NAME_IN_USE)
            {
                // if this is not duplicate name, report it as error
                ERR_LOG("[CreateClubCommand] error (" << result << ") failed to insert club");
            }
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        ClubMember cm;
        cm.getUser().setBlazeId(blazeId);
        cm.setMembershipStatus(CLUBS_OWNER);

        // add current user as club member and owner
        result = dbc.getDb().insertMember(clubId, cm, domain->getMaxMembersPerClub());

        if (result != Blaze::ERR_OK)
        {
            // if we missed this user in cache, we'll hit him in database 
            if (result != Blaze::CLUBS_ERR_ALREADY_MEMBER)
            {
                // if it's not cache miss then it is database error
                ERR_LOG("[CreateClubCommand] error (" << result << ") failed to insert member(" << cm.getUser().getBlazeId() 
                    << ") for club(" << clubId << ")");
            }
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        result = mComponent->onClubCreated(&dbc.getDb(), mRequest.getName(), mRequest.getClubSettings(), clubId, cm);
        if (result != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        // signal master as if the session is created for the new club owner
        result = mComponent->updateCachedDataOnNewUserAdded(
            clubId,
            mRequest.getName(),
            mRequest.getClubDomainId(),
            mRequest.getClubSettings(),
            cm.getUser().getBlazeId(), 
            cm.getMembershipStatus(),
            cm.getMembershipSinceTime(),
            UPDATE_REASON_CLUB_CREATED);

        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[CreateClubCommand] updateOnlineStatusUserAdded failed");
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        // add new club id to extended data 
        result = mComponent->updateMemberUserExtendedData(cm.getUser().getBlazeId(), clubId, 
            mRequest.getClubDomainId(), UPDATE_REASON_CLUB_CREATED,
            mComponent->getOnlineStatus(cm.getUser().getBlazeId()));
        if (result != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        // add tags
        result = dbc.getDb().insertClubTags(clubId, mRequest.getTagList());
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[CreateClubCommand] add tags failed");
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(result);
        }

        mComponent->mPerfClubsCreated.increment(1, *domain);
        mComponent->mPerfJoinsLeaves.increment();

        mResponse.setClubId(clubId);

        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[CreateClubCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        NewClubCreated newClubCreatedEvent;
        newClubCreatedEvent.setCreatorPersonaId(blazeId);
        newClubCreatedEvent.setClubId(clubId);
        gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_NEWCLUBCREATEDEVENT), newClubCreatedEvent, true /*logBinaryEvent*/);

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }

    ClubsSlaveImpl *mComponentImp;

};
// static factory method impl
DEFINE_CREATECLUB_CREATE()


} // Clubs
} // Blaze
