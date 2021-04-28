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
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"

// osdktournaments includes
#include "customcomponent/osdktournaments/rpc/osdktournamentsslave.h"


namespace Blaze
{
namespace OSDKSeasonalPlay
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
void OSDKSeasonalPlaySlaveImpl::onResetTournament(SeasonId seasonId, TournamentId tournamentId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onResetTournament. seasonId = " << seasonId <<
              ", tournamentId = " << tournamentId << "");

    BlazeRpcError error = ERR_OK;

    // reset the tournament
    OSDKTournaments::OSDKTournamentsSlave* tournamentsComponent
        = static_cast<OSDKTournaments::OSDKTournamentsSlave*>(
        gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID,false));
    if (NULL != tournamentsComponent)
    {
		TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onResetTournament: requesting osdktournaments to reset all members in tournament [id] "
		    << tournamentId << "");
        OSDKTournaments::ResetAllTournamentMembersRequest req;
        req.setTournamentId(tournamentId);
        req.setSetActive(false);

        // to invoke this RPC will require authentication
        UserSession::pushSuperUserPrivilege();
        error = tournamentsComponent->resetAllTournamentMembers(req);
        UserSession::popSuperUserPrivilege();

        if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onResetTournament: error " << Blaze::ErrorHelp::getErrorName(error) <<
                    " resetting all members in tournament [id] " << tournamentId);
        }
    }
    else
    {
		ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onResetTournament: unable to retrieve the osdktournaments component. Tournament [id] "
			<< tournamentId << " NOT reset");
    }

    return;
}

} // namespace OSDKSeasonalPlay
} // namespace Blaze

