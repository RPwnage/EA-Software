/*************************************************************************************************/
/*!
    \file   tournamentstatusupdate.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/custom/tournamentstatusupdate.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "EASTL/string.h"

// seasonal play includes
#include "customcomponent/fifacups/fifacupsslaveimpl.h"
#include "logclubnewsutil.h"


namespace Blaze
{
namespace FifaCups
{
    // Ping specific message for a club winning or being eliminated from a tournament
    static const char8_t PING_CLUB_TOURNAMENT_WON[] =           "SDB_SEASONAL_CLUB_PLAYOFF_WON";
    static const char8_t PING_CLUB_TOURNAMENT_ELIMINATED[] =    "SDB_SEASONAL_CLUB_PLAYOFF_LOST";


/*************************************************************************************************/
/*!
    \brief onTournamentStatusUpdate

    Custom processing for the tournament status of a user is updated

    Note: this method is only called when a member's tournament status is updated to "won" or
    "eliminated"
*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onTournamentStatusUpdate(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus)
{
	TRACE_LOG("[FifaCupsSlaveImpl:"<< this<<"].onTournamentStatusUpdate. memberId = "<< memberId<<", memberType = "<<MemberTypeToString(memberType) <<", status = "<< TournamentStatusToString(tournamentStatus)<<" ");

    // 
    // Even though seasonal play supports seasons that have users as members, we will not do any processing
    // of tournament status updates as only Club awards are being surfaced on the consoles. However the
    // concepts below do apply to users as well as clubs.
    //
    if (FIFACUPS_MEMBERTYPE_USER == memberType)
    {
        TRACE_LOG("[FifaCupsSlaveImpl:"<< this<<"].onTournamentStatusUpdate. User MemberType - exiting.");
        return;
    }

	//TODO fill in any logic here for clubs
    return;
}

} // namespace FifaCups
} // namespace Blaze

