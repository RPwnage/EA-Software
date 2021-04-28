/*************************************************************************************************/
/*!
    \file   reportMatchResult_command.cpp


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
#include "osdktournaments/rpc/osdktournamentsslave/reportmatchresult_stub.h"

namespace Blaze
{
namespace OSDKTournaments
{

class ReportMatchResultCommand : public ReportMatchResultCommandStub
{
public:
    ReportMatchResultCommand(Message* message, ReportMatchResultRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   ReportMatchResultCommandStub(message, request),
    mComponent(compImpl)
    { }

    virtual ~ReportMatchResultCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    ReportMatchResultCommandStub::Errors execute();

};

// static factory method impl
DEFINE_REPORTMATCHRESULT_CREATE()

ReportMatchResultCommandStub::Errors ReportMatchResultCommand::execute()
{
    TRACE_LOG("[ReportMatchResultCommand:" << this << "] start");
	Blaze::BlazeRpcError error = Blaze::ERR_OK;
	
	error = mComponent->reportMatchResult(
        mRequest.getTournId(),
        mRequest.getUserOneId(),
        mRequest.getUserTwoId(),
        mRequest.getUserOneTeam(),
        mRequest.getUserTwoTeam(),
        mRequest.getUserOneScore(),
        mRequest.getUserTwoScore(),
		mRequest.getUserOneMetaData(),
        mRequest.getUserTwoMetaData());

	return ReportMatchResultCommandStub::commandErrorFromBlazeError(error);
}

} // OSDKTournaments
} // Blaze
