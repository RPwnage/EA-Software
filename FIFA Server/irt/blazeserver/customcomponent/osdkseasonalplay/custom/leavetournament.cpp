/*************************************************************************************************/
/*!
    \file   leavetournament.cpp

	$Header: //gosdev/games/Ping/V8_Gen4/ML/blazeserver/15.x/customcomponent/osdkseasonalplay/custom/leavetournament.cpp#1 $
    $Change: 1033186 $
    $DateTime: 2014/11/07 14:23:28 $
	
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
    \brief onLeaveTournament

    Custom processing to have a member leave a tournament

    Note - when a club switch seasons, they must leave the current tournament that they are in.

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::onLeaveTournament(MemberId memberId, MemberType memberType, TournamentId tournamentId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onLeaveTournament. memberId = " << memberId << ", memberType = "
              << MemberTypeToString(memberType) << ", tournamentId = " << tournamentId << "");

    BlazeRpcError error = ERR_OK;

    // leave the tournament
    OSDKTournaments::OSDKTournamentsSlave* tournamentsComponent
        = static_cast<OSDKTournaments::OSDKTournamentsSlave*>(
        gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID,false));
    if (NULL != tournamentsComponent)
    {
        OSDKTournaments::TournamentMemberId tournMemberId = static_cast<OSDKTournaments::TournamentMemberId>(memberId);
        OSDKTournaments::LeaveTournamentRequest req;
        req.setTournamentId(tournamentId);
        req.setMemberIdOverride(tournMemberId);
        error = tournamentsComponent->leaveTournament(req);
        if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onLeaveTournament: osdktournaments component leaveTournament() returned error "
                << Blaze::ErrorHelp::getErrorName(error));
        }
    }
    else
    {
		ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onLeaveTournament: unable to retrieve the osdktournaments component. leaveTournament() not called");
    }
    return;
}

} // namespace OSDKSeasonalPlay
} // namespace Blaze

