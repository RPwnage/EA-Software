/*************************************************************************************************/
/*!
    \file   tournamentstatusupdate.cpp

    $Header: //gosdev/games/Ping/V8_Gen4/ML/blazeserver/15.x/customcomponent/osdkseasonalplay/custom/tournamentstatusupdate.cpp#1 $
    $Change: 1033186 $
    $DateTime: 2014/11/07 14:23:28 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "EASTL/string.h"

// seasonal play includes
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"
#include "logclubnewsutil.h"


namespace Blaze
{
namespace OSDKSeasonalPlay
{
    // Ping specific message for a club winning or being eliminated from a tournament
    static const char8_t PING_CLUB_TOURNAMENT_WON[] =           "PSDB_SEASONAL_CLUB_PLAYOFF_WON";
    static const char8_t PING_CLUB_TOURNAMENT_ELIMINATED[] =    "PSDB_SEASONAL_CLUB_PLAYOFF_LOST";


/*************************************************************************************************/
/*!
    \brief onTournamentStatusUpdate

    Custom processing for the tournament status of a user is updated

    Note: this method is only called when a member's tournament status is updated to "won" or
    "eliminated"
*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::onTournamentStatusUpdate(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onTournamentStatusUpdate. memberId = " << memberId <<
              ", memberType = " << MemberTypeToString(memberType) << ", status = " << TournamentStatusToString(tournamentStatus) << "");

    // 
    // Even though seasonal play supports seasons that have users as members, we will not do any processing
    // of tournament status updates as only Club awards are being surfaced on the consoles. However the
    // concepts below do apply to users as well as clubs.
    //
    if (SEASONALPLAY_MEMBERTYPE_USER == memberType)
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this  << "].onTournamentStatusUpdate. User MemberType - exiting.");
        return Blaze::ERR_OK;
    }

    // log club news to indicate that the club has won or been eliminated from a tournament.

    // need to get the season id
    BlazeRpcError error = Blaze::ERR_OK;
    SeasonId seasonId = SEASON_ID_INVALID;
    error = dbHelper.fetchSeasonId(memberId, memberType, seasonId);
    if (Blaze::ERR_OK != error)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onTournamentStatusUpdate. Unable to get season id for memberId = "
            << memberId << " - exiting.");
        return error;
    }
    
    // need to get the club's division
    uint32_t divisionRank = 0, divisionStartingRank = 0, overallRank =0;
    uint8_t division = 0;
    error = calculateRankAndDivision(seasonId, memberId, divisionRank, division, divisionStartingRank, overallRank);
    if (Blaze::ERR_OK != error)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onTournamentStatusUpdate. Unable to calculate rank and division for memberId = "
                  << memberId << " - exiting.");
        return error;
    }
    eastl::string strDivision;
    strDivision.sprintf("%d", division);

    LogClubNewsUtil logClubNewsUtil;
    
    switch (tournamentStatus)
    {
    case SEASONALPLAY_TOURNAMENT_STATUS_ELIMINATED:
        logClubNewsUtil.logNews(memberId, PING_CLUB_TOURNAMENT_ELIMINATED, strDivision.c_str());
        break;
    case SEASONALPLAY_TOURNAMENT_STATUS_WON:
        logClubNewsUtil.logNews(memberId, PING_CLUB_TOURNAMENT_WON, strDivision.c_str());
        break;
    default:
        break; // unsupported tournament status - do nothing
    }

    return error;
}

} // namespace OSDKSeasonalPlay
} // namespace Blaze

