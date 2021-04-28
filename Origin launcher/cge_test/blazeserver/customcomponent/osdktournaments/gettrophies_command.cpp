/*************************************************************************************************/
/*!
    \file   gettrophies_command.cpp


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
#include "osdktournaments/rpc/osdktournamentsslave/gettrophies_stub.h"
#include "osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

class GetTrophiesCommand : public GetTrophiesCommandStub
{
public:
    GetTrophiesCommand(Message* message, GetTrophiesRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :GetTrophiesCommandStub(message, request),
    mComponent(compImpl)
    { }

    virtual ~GetTrophiesCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    GetTrophiesCommandStub::Errors execute();

};
// static factory method impl
DEFINE_GETTROPHIES_CREATE()

GetTrophiesCommandStub::Errors GetTrophiesCommand::execute()
{
    TRACE_LOG("[GetTrophiesCommand:" << this << "] start");
    GetTrophiesCommandStub::Errors result = GetTrophiesCommandStub::ERR_OK;

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[GetTrophiesCommandStub:" << this << "].execute() - Failed to obtain connection.");
        return GetTrophiesCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    DbError dbError = tournDb.fetchTrophies(static_cast<BlazeId>( mRequest.getMemberId() ), mRequest.getNumTrophies(), getPeerInfo()->getLocale(), mResponse.getTrophies());
    if (DB_ERR_OK != dbError)
    {
        result = GetTrophiesCommandStub::ERR_SYSTEM;
    }

    return result;
}

} // OSDKTournaments
} // Blaze
