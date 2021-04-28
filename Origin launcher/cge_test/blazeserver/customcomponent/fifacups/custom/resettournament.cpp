/*************************************************************************************************/
/*!
    \file   resettournament.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// seasonal play includes
#include "customcomponent/fifacups/fifacupsslaveimpl.h"

// tournaments includes
#include "customcomponent/osdktournaments/rpc/osdktournamentsslave.h"


namespace Blaze
{
namespace FifaCups
{
        
/*************************************************************************************************/
/*!
    \brief onResetTournament

    Custom processing for when the tournament for a season needs to be reset

    Note: this method will be called for each season id. The purpose of this method is to 
    reset the tournament associated with a season so that all seasonal play members start
    fresh the next time the tournament period is entered.
*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onResetTournament(SeasonId seasonId, TournamentId tournamentId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].onResetTournament. seasonId = "<<seasonId <<", tournamentId = "<<tournamentId<<" ");

	// reset the tournament
	OSDKTournaments::OSDKTournamentsSlave* component = static_cast<OSDKTournaments::OSDKTournamentsSlave*>(gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID, false));    
	if (component == NULL)
	{
		ERR_LOG("[FifaCupsSlaveImpl].onResetTournament: unable to retrieve the OSDKTournamentsSlave. Tournament [id] "<<tournamentId <<" NOT reset");
		return;
	}

	OSDKTournaments::ResetAllTournamentMembersRequest req;
	req.setTournamentId(tournamentId);
	req.setSetActive(true);

	UserSession::pushSuperUserPrivilege();
	BlazeRpcError error =  component->resetAllTournamentMembers(req);
	UserSession::popSuperUserPrivilege();

	if (error != ERR_OK)
	{
		ERR_LOG("[FifaCupsSlaveImpl].onResetTournament: unable to reset all tournament members ("<< error <<") ");
	}
}

} // namespace FifaCups
} // namespace Blaze

