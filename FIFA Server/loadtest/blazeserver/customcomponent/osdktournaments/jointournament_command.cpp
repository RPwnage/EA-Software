/*************************************************************************************************/
/*!
    \file   jointournament_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/usersessions/usersession.h"

// osdktournaments includes
#include "osdktournaments/osdktournamentsdatabase.h"
#include "osdktournaments/osdktournamentsslaveimpl.h"
#include "osdktournaments/rpc/osdktournamentsslave/jointournament_stub.h"
#include "osdktournaments/rpc/osdktournamentsslave_stub.h"

namespace Blaze
{
namespace OSDKTournaments
{

class JoinTournamentCommand : public JoinTournamentCommandStub
{
public:
    JoinTournamentCommand(Message* message, JoinTournamentRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    : JoinTournamentCommandStub(message, request),
    mComponent(compImpl)
    {}

    virtual ~JoinTournamentCommand() {}

private:
    JoinTournamentCommandStub::Errors execute();

    OSDKTournamentsSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_JOINTOURNAMENT_CREATE()

JoinTournamentCommandStub::Errors JoinTournamentCommand::execute()
{
    TRACE_LOG("[JoinTournamentCommand:" << this << "] start");
    
    // determine what member id to use depending on the tournament mode
	TournamentData* data = mComponent->getTournamentDataById(mRequest.getTournamentId());
    if (NULL == data)
    {
        return JoinTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }
    
    TournamentMemberId requestorId = mRequest.getMemberIdOverride();
    
    // if the request's member id is "invalid", calculate which member id should be used
    if (INVALID_TOURNAMENT_MEMBER_ID == requestorId)
    {
        requestorId = mComponent->getCurrentUserMemberIdForMode(data->getMode());
    }

    if (INVALID_TOURNAMENT_MEMBER_ID == requestorId)
    {
        return JoinTournamentCommandStub::OSDKTOURN_ERR_MEMBER_NOT_FOUND;
    }

    DbConnPtr dbConnection = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (dbConnection == NULL)
    {
        ERR_LOG("[JoinTournamentCommand:" << this << "].execute() - Failed to obtain connection.");
        return JoinTournamentCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(dbConnection);
    
    DbError txnError = dbConnection->startTxn();
    if (DB_ERR_OK != txnError)
    {
        ERR_LOG("[JoinTournamentCommand:" << this << "].execute() - Failed to start transaction.");
        return JoinTournamentCommandStub::ERR_SYSTEM;
    }

    // use the tournament id from the request as the member may be in multiple tournaments
    TournamentId tournamentId = mRequest.getTournamentId();

    TournamentMemberData memberData;
    if (DB_ERR_OK != tournDb.getMemberStatus(requestorId, data->getMode(), tournamentId, memberData))
    {
        dbConnection->rollback();
        return JoinTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    BlazeRpcError removeResult = DB_ERR_OK;
    bool8_t wasInactive = false;

    // only bother checking the old tournament if it was valid
    if (INVALID_TOURNAMENT_ID != tournamentId)
    {
        wasInactive = !memberData.getIsActive();

		if (NULL == mComponent->getTournamentDataById(tournamentId) || 
            true == wasInactive)
        {
            //member is in a disabled tournament or was inactive so clear their state before proceeding
            tournDb.deleteTree(requestorId, tournamentId);
            removeResult = tournDb.removeMember(tournamentId, requestorId);
        }
        else if (false == wasInactive)
        {
            dbConnection->rollback();

            // Race condition - 2 users trying to join a club into tournament.
            // The club appears to be already in tournament for one of the users.
            // Return OK, so client doesn't show error popup to user.
            TRACE_LOG("[JoinTournamentCommand:" << this << "].execute() - The member is already in tournament [" << tournamentId << "]");
            return JoinTournamentCommand::ERR_OK;
        }
    }

    DbError insertResult = tournDb.insertMember(tournamentId, requestorId, mRequest.getTournAttribute(), mRequest.getTeam());
    if ((DB_ERR_OK != insertResult) || (DB_ERR_OK != removeResult))
    {
        dbConnection->rollback();
        return JoinTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    JoinTournamentNotification notification;
	notification.setTournId(mRequest.getTournamentId());
    notification.setMemberId(requestorId);
    notification.setWasInactive(wasInactive);
    mComponent->sendJoinTournamentNotificationToAllSlaves(&notification);

    DbError commitError = dbConnection->commit();
    if (DB_ERR_OK != commitError)
    {
        dbConnection->rollback();
        ERR_LOG("[JoinTournamentCommand:" << this << "].execute() - Failed to commit connection.");
        return JoinTournamentCommandStub::ERR_SYSTEM;
    }

	Blaze::BlazeRpcError joinError = mComponent->addUserExtendedData(requestorId, tournamentId);
    if (Blaze::ERR_OK == joinError)
    {
        if (Blaze::ERR_OK == joinError)
        {
			TournamentData* updatedTournament = mComponent->getTournamentDataById(tournamentId);
            if (NULL != updatedTournament)
            {
                updatedTournament->copyInto(mResponse.getTournament());
            }
        }
    }

    return JoinTournamentCommandStub::commandErrorFromBlazeError(joinError);
}

} // OSDKTournaments
} // Blaze
