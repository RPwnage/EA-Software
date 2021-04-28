/*************************************************************************************************/
/*!
    \file   fifacupsqueries.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsqueries.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
        (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFACUPS_QUERIES_H
#define BLAZE_FIFACUPS_QUERIES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace FifaCups
{
    ///////////////////////////////////////////////////////////
    // FETCH
    ///////////////////////////////////////////////////////////
    
    // get the season registration info for the season id that the entity is registered in
    static const char8_t FIFACUPS_FETCH_REGISTRATION[] =
        "SELECT * "
        "FROM `fifacups_registration` "
        "WHERE `memberId` = ? AND `memberType` = ?";

    // get all the members registered in a season
    static const char8_t FIFACUPS_FETCH_SEASON_MEMBERS[] =
        "SELECT * "
        "FROM `fifacups_registration` "
        "WHERE `seasonId` = ?";

	// get all the members registered in a season
	static const char8_t FIFACUPS_FETCH_TOURNAMENT_MEMBERS[] =
		"SELECT * "
		"FROM `fifacups_registration` "
		"WHERE `seasonId` = ? AND `tournamentStatus` = ?";

    // get the season number for the season
    static const char8_t FIFACUPS_FETCH_SEASON_NUMBER[] =
        "SELECT `seasonNumber`, `lastRollOverStatPeriodId` "
        "FROM `fifacups_seasons` "
        "WHERE `seasonId` = ?";

    ///////////////////////////////////////////////////////////
    // INSERT
    ///////////////////////////////////////////////////////////

    // insert a new season. The seasonNumber defaults to 1
    static const char8_t FIFACUPS_INSERT_NEW_SEASON[] =
        "INSERT IGNORE INTO `fifacups_seasons` ("
        "`seasonId`"
        ") VALUES(?)";

    // insert a new entity season registration
    static const char8_t FIFACUPS_INSERT_REGISTRATION[] =
        "INSERT INTO `fifacups_registration` ("
        "`memberId`, `memberType`, `seasonId`, `leagueId`, `qualifyingDiv`"
        ") VALUES(?,?,?,?,?)";

    ///////////////////////////////////////////////////////////
    // UPDATE
    ///////////////////////////////////////////////////////////

    // update which season a member is registered in
    static const char8_t OSDSEASONALPLAY_UPDATE_REGISTRATION[] =
        "UPDATE `fifacups_registration` "
        "SET `seasonId` = ?, `leagueId` = ?, `tournamentStatus` = 0 "
        "WHERE `memberId` = ? and `memberType` = ?";

    // update the season number for a season
    static const char8_t FIFACUPS_UPDATE_SEASON_NUMBER[] =
        "UPDATE `fifacups_seasons` "
        "SET `seasonNumber` = ?, `lastRollOverStatPeriodId` = ? "
        "WHERE `seasonId` = ?";

    // update the tournament status for a member
    static const char8_t FIFACUPS_UPDATE_MEMBER_TOURNAMENT_STATUS[] =
        "UPDATE `fifacups_registration` "
        "SET `tournamentStatus` = ? "
        "WHERE `memberId` = ? and `memberType` = ?";

	// update the qualifying div for the cups member
	static const char8_t FIFACUPS_UPDATE_MEMBER_QUALIFYING_DIV[] =
		"UPDATE `fifacups_registration` "
		"SET `qualifyingDiv` = ? "
		"WHERE `memberId` = ? and `memberType` = ?";

	// update the pending div for the cups member
	static const char8_t FIFACUPS_UPDATE_MEMBER_PENDING_DIV[] =
		"UPDATE `fifacups_registration` "
		"SET `pendingDiv` = ? "
		"WHERE `memberId` = ? and `memberType` = ?";

	// update the active cup id for the cups member
	static const char8_t FIFACUPS_UPDATE_MEMBER_ACTIVE_CUPID[] =
		"UPDATE `fifacups_registration` "
		"SET `activeCupId` = ? "
		"WHERE `memberId` = ? and `memberType` = ?";

	// update the tournament status for all members in a specified season
    static const char8_t FIFACUPS_UPDATE_SEASON_TOURNAMENT_STATUS[] =
        "UPDATE `fifacups_registration` "
        "SET `tournamentStatus` = ? "
        "WHERE `seasonId` = ?";

	// update the active cup id for all members in a specified season
	static const char8_t FIFACUPS_UPDATE_SEASON_ACTIVE_CUPID[] =
		"UPDATE `fifacups_registration` "
		"SET `activeCupId` = ? "
		"WHERE `seasonId` = ?";

	// update the qualifying div with the pending division for all members
	static const char8_t FIFACUPS_UPDATE_COPY_PENDING_DIVISION[] =
		"UPDATE `fifacups_registration` "
		"SET `qualifyingDiv` = `pendingDiv` "
		"WHERE `seasonId` = ? and `pendingDiv` > 0";

	// clear the pending division for all members
	static const char8_t FIFACUPS_CLEAR_PENDING_DIVISION[] =
		"UPDATE `fifacups_registration` "
		"SET `pendingDiv` = ? "
		"WHERE `seasonId` = ?";

    ///////////////////////////////////////////////////////////
    // DELETE
    ///////////////////////////////////////////////////////////


} // namespace FifaCups
} // namespace Blaze

#endif // BLAZE_FIFACUPS_QUERIES_H
