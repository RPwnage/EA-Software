/*************************************************************************************************/
/*!
    \file   resettournament_command.cpp


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
#include "osdktournaments/rpc/osdktournamentsslave/resettournament_stub.h"
#include "osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

class ResetTournamentCommand : public ResetTournamentCommandStub
{
public:
    ResetTournamentCommand(Message* message, ResetTournamentRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   ResetTournamentCommandStub(message, request),
        mComponent(compImpl)
    {}

    virtual ~ResetTournamentCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    ResetTournamentCommandStub::Errors execute();
};

// static factory method impl
DEFINE_RESETTOURNAMENT_CREATE()

ResetTournamentCommandStub::Errors ResetTournamentCommand::execute()
{
    TRACE_LOG("[ResetMyTournamentCommand:" << this << "] start");

    TournamentMemberId memberId = mRequest.getMemberIdOverride();
    if (INVALID_TOURNAMENT_MEMBER_ID == memberId)
    {
        // if the request's member id is "invalid", calculate which member id should be used
        memberId = mComponent->getCurrentUserMemberIdForMode(mRequest.getMode());
    }

    const TournamentId tournId = mRequest.getTournamentId();

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[GetTrophiesCommandStub:" << this << "].execute() - Failed to obtain connection.");
        return ResetTournamentCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    DbError txnError = conn->startTxn();
    if (DB_ERR_OK != txnError)
    {
        // early return here when starting the transaction returns any error
        ERR_LOG("[GetTrophiesCommandStub:" << this << "].execute() - Failed to start transaction.");
        return ResetTournamentCommandStub::ERR_SYSTEM;
    }

    TournamentMemberData memberData;
    if (DB_ERR_OK != tournDb.getMemberStatus(memberId, mRequest.getMode(), tournId, memberData))
    {
        // early return to handle any error determining the member status
        conn->rollback();
        return ResetTournamentCommandStub::OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
    }

    bool setActive = mRequest.getSetActive();

    DbError result = tournDb.deleteTree(memberId, tournId);
    if (DB_ERR_OK == result)
    {
        result = tournDb.updateMemberStatus(memberId, tournId, setActive, 1, 0, INVALID_TEAM_ID);
        DbError commitError = conn->commit();
        if (DB_ERR_OK != commitError)
        {
            conn->rollback();
            ERR_LOG("[GetTrophiesCommandStub:" << this << "].execute() - Failed to commit transaction.");
            return ResetTournamentCommandStub::ERR_SYSTEM;
        }
    }
    else
    {
        conn->rollback();
    }

    if (DB_ERR_OK != result)
    {
        return ResetTournamentCommandStub::ERR_SYSTEM;
    }
    else
    {
        return ResetTournamentCommandStub::ERR_OK;
    }
}

} // OSDKTournaments
} // Blaze
