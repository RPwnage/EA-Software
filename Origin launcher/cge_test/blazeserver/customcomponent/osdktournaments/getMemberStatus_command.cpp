/*************************************************************************************************/
/*!
    \file   getMemberStatus_command.cpp


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
#include "osdktournaments/rpc/osdktournamentsslave/getmemberstatus_stub.h"

namespace Blaze
{
namespace OSDKTournaments
{

class GetMemberStatusCommand : public GetMemberStatusCommandStub
{
public:
    GetMemberStatusCommand(Message* message, GetMemberStatusRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   GetMemberStatusCommandStub(message, request),
    mComponent(compImpl)
    { }

    virtual ~GetMemberStatusCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    GetMemberStatusCommandStub::Errors execute();

};

// static factory method impl
DEFINE_GETMEMBERSTATUS_CREATE()

GetMemberStatusCommandStub::Errors GetMemberStatusCommand::execute()
{
    TRACE_LOG("[GetMemberStatusCommand:" << this << "] start");
	Blaze::BlazeRpcError error = Blaze::ERR_OK;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[GetMemberStatusCommand:" << this << "].execute(): unable to find a database connection");
        return OSDKTOURN_ERR_NO_DB_CONNECTION;
    }
    OSDKTournaments::OSDKTournamentsDatabase tournDb(conn);

    error = tournDb.getMemberStatus(mRequest.getMemberId(), mRequest.getMode(), mResponse);
    return GetMemberStatusCommandStub::commandErrorFromBlazeError(error);
}

} // OSDKTournaments
} // Blaze
