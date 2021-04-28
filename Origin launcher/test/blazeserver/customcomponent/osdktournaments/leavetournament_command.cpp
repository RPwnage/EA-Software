/*************************************************************************************************/
/*!
    \file   leavetournament_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

// osdktournaments includes
#include "osdktournaments/osdktournamentsdatabase.h"
#include "osdktournaments/osdktournamentsslaveimpl.h"
#include "osdktournaments/rpc/osdktournamentsslave/leavetournament_stub.h"

namespace Blaze
{
namespace OSDKTournaments
{

class LeaveTournamentCommand : public LeaveTournamentCommandStub
{
public:
    LeaveTournamentCommand(Message* message, LeaveTournamentRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    : LeaveTournamentCommandStub(message, request),
    mComponent(compImpl)
    {}

    virtual ~LeaveTournamentCommand() {}

private:
    LeaveTournamentCommandStub::Errors execute();

    OSDKTournamentsSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_LEAVETOURNAMENT_CREATE()

LeaveTournamentCommandStub::Errors LeaveTournamentCommand::execute()
{
    // determine what member id to use depending on the tournament mode
    const TournamentId tournId = mRequest.getTournamentId();
	TournamentData* data = mComponent->getTournamentDataById(tournId);
    if (NULL == data)
    {
        return LeaveTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }

    DbConnPtr dbConnection = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (dbConnection == NULL)
    {
        ERR_LOG("[LeaveTournamentCommand:" << this << "].execute() - Failed to obtain connection.");
        return LeaveTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }
    OSDKTournamentsDatabase tournamentsDb(dbConnection);

    TournamentMemberId memberId = mRequest.getMemberIdOverride();
    if (INVALID_TOURNAMENT_MEMBER_ID == memberId)
    {
        // if the request's member id is "invalid", calculate which member id should be used
        memberId = mComponent->getCurrentUserMemberIdForMode(data->getMode());
    }

    TournamentMemberData memberData;
    if (DB_ERR_OK != tournamentsDb.getMemberStatus(memberId, data->getMode(), tournId, memberData))
    {
        return LeaveTournamentCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    if (INVALID_TOURNAMENT_ID == memberData.getTournamentId())
    {
        return LeaveTournamentCommandStub::OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
    }

    BlazeRpcError error = mComponent->leaveTournament(memberId, mRequest.getTournamentId());

    return commandErrorFromBlazeError(error);
}

} // OSDKTournaments
} // Blaze
