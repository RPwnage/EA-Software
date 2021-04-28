/*************************************************************************************************/
/*!
    \file   getmytournamentdetails_command.cpp


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
#include "osdktournaments/osdktournamentsslaveimpl.h"
#include "osdktournaments/rpc/osdktournamentsslave/getmytournamentid_stub.h"
#include "osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

class GetMyTournamentIdCommand : public GetMyTournamentIdCommandStub
{
public:
    GetMyTournamentIdCommand(Message* message, GetMyTournamentIdRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   GetMyTournamentIdCommandStub(message, request),
    mComponent(compImpl)
    { }

    virtual ~GetMyTournamentIdCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    GetMyTournamentIdCommandStub::Errors execute();

};
// static factory method impl
DEFINE_GETMYTOURNAMENTID_CREATE()

GetMyTournamentIdCommandStub::Errors GetMyTournamentIdCommand::execute()
{
    TRACE_LOG("[GetMyTournamentIdCommand:" << this << "] start");
    GetMyTournamentIdCommandStub::Errors result = GetMyTournamentIdCommandStub::ERR_OK;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[GetMyTournamentIdCommandStub:" << this << "].execute() - Failed to obtain connection.");
        return GetMyTournamentIdCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    TournamentMemberId memberId = mRequest.getMemberIdOverride();
    if (INVALID_TOURNAMENT_MEMBER_ID == memberId)
    {
        // if the request's member id is "invalid", calculate which member id should be used
        memberId = mComponent->getCurrentUserMemberIdForMode(mRequest.getMode());
    }

    TournamentMemberData memberData;
    if (DB_ERR_OK == tournDb.getMemberStatus(memberId, mRequest.getMode(), memberData))
    {
        TournamentId tournId = memberData.getTournamentId();
		if ((INVALID_TOURNAMENT_ID == tournId) || (NULL == mComponent->getTournamentDataById(tournId)))
        {
            result = GetMyTournamentIdCommandStub::OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
        }
        mResponse.setTournamentId(tournId);
        mResponse.setIsActive(memberData.getIsActive());
    }
    else
    {
        result = GetMyTournamentIdCommandStub::ERR_SYSTEM;
    }

    return result;
}

} // OSDKTournaments
} // Blaze
