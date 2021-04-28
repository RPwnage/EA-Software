/*************************************************************************************************/
/*!
    \file   resetalltournamentmembers_command.cpp


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
#include "osdktournaments/rpc/osdktournamentsslave/resetalltournamentmembers_stub.h"

namespace Blaze
{
namespace OSDKTournaments
{

class ResetAllTournamentMembersCommand : public ResetAllTournamentMembersCommandStub
{
public:
    ResetAllTournamentMembersCommand(Message* message, ResetAllTournamentMembersRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   ResetAllTournamentMembersCommandStub(message, request),
        mComponent(compImpl)
    {}

    virtual ~ResetAllTournamentMembersCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    ResetAllTournamentMembersCommandStub::Errors execute();
};

// static factory method impl
DEFINE_RESETALLTOURNAMENTMEMBERS_CREATE()

ResetAllTournamentMembersCommandStub::Errors ResetAllTournamentMembersCommand::execute()
{
    const TournamentId tournId = mRequest.getTournamentId();
	TournamentData* data = mComponent->getTournamentDataById(tournId);
    if (NULL == data)
    {
        return ResetAllTournamentMembersCommandStub::OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }

    DbConnPtr dbConnection = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (dbConnection == NULL)
    {
        ERR_LOG("[ResetAllTournamentMembersCommand:" << this << "].execute() - Failed to obtain connection.");
        return ResetAllTournamentMembersCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournamentsDb(dbConnection);

    if (INVALID_TOURNAMENT_ID == tournId)
    {
        return ResetAllTournamentMembersCommandStub::ERR_SYSTEM;
    }

    bool setActive = mRequest.getSetActive();

    BlazeRpcError error = mComponent->resetAllTournamentMembers(tournId, setActive);

    return commandErrorFromBlazeError(error);
}

} // OSDKTournaments
} // Blaze
