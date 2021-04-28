/*************************************************************************************************/
/*!
    \file   leavetournament.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/custom/leavetournament.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

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
//#include "component/tournaments/tournamentsslaveimpl.h"



namespace Blaze
{
namespace FifaCups
{

/*************************************************************************************************/
/*!
    \brief onLeaveTournament

    Custom processing to have a member leave a tournament

    Note - when a club switch seasons, they must leave the current tournament that they are in.

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onLeaveTournament(MemberId memberId, MemberType memberType, TournamentId tournamentId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].onLeaveTournament. memberId = "<< memberId<<", memberType = "<< MemberTypeToString(memberType) <<", tournamentId = "<< tournamentId<<" ");
/*
    BlazeRpcError error = ERR_OK;

    // leave the tournament
    Tournaments::TournamentsSlaveImpl* tournamentsComponent
        = static_cast<Tournaments::TournamentsSlaveImpl*>(
        gController->getComponent(Tournaments::TournamentsSlaveImpl::COMPONENT_ID));
    if (NULL != tournamentsComponent)
    {
        //Tournaments::TournamentMemberId tournMemberId = static_cast<Tournaments::TournamentMemberId>(memberId);
        //error = tournamentsComponent->leaveTournament(tournMemberId, tournamentId);
        //if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[FifaCupsSlaveImpl].onLeaveTournament: tournaments component leaveTournament() returned error "<< Blaze::ErrorHelp::getErrorName(error) <<" ");
        }
    }
    else
    {
        ERR_LOG("[FifaCupsSlaveImpl].onLeaveTournament: unable to retrieve the tournaments component. leaveTournament() not called");
    }*/
    return;
}

} // namespace FifaCups
} // namespace Blaze

