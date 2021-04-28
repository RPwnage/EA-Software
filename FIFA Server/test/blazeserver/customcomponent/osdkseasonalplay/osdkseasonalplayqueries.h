/*************************************************************************************************/
/*!
    \file   osdkseasonalplayqueries.h

    $Header: //gosdev/games/FIFA/2022/GenX/test/blazeserver/customcomponent/osdkseasonalplay/osdkseasonalplayqueries.h#1 $
    $Change: 1636281 $
    $DateTime: 2020/12/03 11:59:08 $

    \attention
        (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKSEASONALPLAY_QUERIES_H
#define BLAZE_OSDKSEASONALPLAY_QUERIES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace OSDKSeasonalPlay
{
    ///////////////////////////////////////////////////////////
    // FETCH
    ///////////////////////////////////////////////////////////
    
    // get the season registration info for the season id that the entity is registered in
    static const char8_t OSDKSEASONALPLAY_FETCH_REGISTRATION[] =
        "SELECT * "
        "FROM `osdkseasonalplay_registration` "
        "WHERE `memberId` = ? AND `memberType` = ?";

    // get all the members registered in a season
    static const char8_t OSDKSEASONALPLAY_FETCH_SEASON_MEMBERS[] =
        "SELECT * "
        "FROM `osdkseasonalplay_registration` "
        "WHERE `seasonId` = ?";

    // get the season number for the season
    static const char8_t OSDKSEASONALPLAY_FETCH_SEASON_NUMBER[] =
        "SELECT `seasonNumber`, `lastRollOverStatPeriodId` "
        "FROM `osdkseasonalplay_seasons` "
        "WHERE `seasonId` = ?";

    // get the awards assigned to a member in a season (all season numbers)
    static const char8_t OSDKSEASONALPLAY_FETCH_AWARDS_MEMBER_ALLSEASONS[] =
        "SELECT  "
        "`id`, `memberId`, `memberType`, `awardId`, `seasonId`, `leagueId`, `seasonId`, "
        "`seasonNumber`, UNIX_TIMESTAMP(`timestamp`), `historical`, `metadata` "
        "FROM `osdkseasonalplay_awards` "
        "WHERE `memberId` = ? AND `memberType` = ? AND `seasonId` = ?";

    // get the awards assigned to a member in a season for a specific season number
    static const char8_t OSDKSEASONALPLAY_FETCH_AWARDS_MEMBER_SEASONNUMBER[] =
        "SELECT  "
        "`id`, `memberId`, `memberType`, `awardId`, `seasonId`, `leagueId`, `seasonId`, "
        "`seasonNumber`, UNIX_TIMESTAMP(`timestamp`), `historical`, `metadata` "
        "FROM `osdkseasonalplay_awards` "
        "WHERE `memberId` = ? AND `memberType` = ? AND `seasonId` = ? AND `seasonNumber` = ?";

    // get the awards assigned in a season (all season numbers)
    static const char8_t OSDKSEASONALPLAY_FETCH_AWARDS_SEASON_ALLSEASONS[] =
        "SELECT  "
        "`id`, `memberId`, `memberType`, `awardId`, `seasonId`, `leagueId`, `seasonId`, "
        "`seasonNumber`, UNIX_TIMESTAMP(`timestamp`), `historical`, `metadata` "
        "FROM `osdkseasonalplay_awards` "
        "WHERE `seasonId` = ?";

    // get the awards assigned in a season for a specific season number
    static const char8_t OSDKSEASONALPLAY_FETCH_AWARDS_SEASON_SEASONNUMBER[] =
        "SELECT  "
        "`id`, `memberId`, `memberType`, `awardId`, `seasonId`, `leagueId`, `seasonId`, "
        "`seasonNumber`, UNIX_TIMESTAMP(`timestamp`), `historical`, `metadata` "
        "FROM `osdkseasonalplay_awards` "
        "WHERE `seasonId` = ? AND `seasonNumber` = ?";


    ///////////////////////////////////////////////////////////
    // INSERT
    ///////////////////////////////////////////////////////////

    // insert a new award assignment.
    static const char8_t OSDKSEASONALPLAY_INSERT_AWARD[] =
        "INSERT INTO `osdkseasonalplay_awards` ("
        "`awardId`, `memberId`, `memberType`, `seasonId`, `leagueId`, `seasonNumber`, `historical`, `metadata`"
        ") VALUES(?,?,?,?,?,?,?,?)";

    // insert a new season. The seasonNumber defaults to 1
    static const char8_t OSDKSEASONALPLAY_INSERT_NEW_SEASON[] =
        "INSERT IGNORE INTO `osdkseasonalplay_seasons` ("
        "`seasonId`"
        ") VALUES(?)";

    // insert a new entity season registration
    static const char8_t OSDKSEASONALPLAY_INSERT_REGISTRATION[] =
        "INSERT INTO `osdkseasonalplay_registration` ("
        "`memberId`, `memberType`, `seasonId`, `leagueId`"
        ") VALUES(?,?,?,?)";

    ///////////////////////////////////////////////////////////
    // UPDATE
    ///////////////////////////////////////////////////////////

    // update which season a member is registered in
    static const char8_t OSDSEASONALPLAY_UPDATE_REGISTRATION[] =
        "UPDATE `osdkseasonalplay_registration` "
        "SET `seasonId` = ?, `leagueId` = ?, `tournamentStatus` = 0 "
        "WHERE `memberId` = ? and `memberType` = ?";

    // update the season number for a season
    static const char8_t OSDKSEASONALPLAY_UPDATE_SEASON_NUMBER[] =
        "UPDATE `osdkseasonalplay_seasons` "
        "SET `seasonNumber` = ?, `lastRollOverStatPeriodId` = ? "
        "WHERE `seasonId` = ?";

    // update the tournament status for a member
    static const char8_t OSDKSEASONALPLAY_UPDATE_MEMBER_TOURNAMENT_STATUS[] =
        "UPDATE `osdkseasonalplay_registration` "
        "SET `tournamentStatus` = ? "
        "WHERE `memberId` = ? and `memberType` = ?";

    // update the tournament status for all members in a specified season
    static const char8_t OSDKSEASONALPLAY_UPDATE_SEASON_TOURNAMENT_STATUS[] =
        "UPDATE `osdkseasonalplay_registration` "
        "SET `tournamentStatus` = ? "
        "WHERE `seasonId` = ?";

    // update who an award is assigned to
    static const char8_t OSDKSEASONALPLAY_UPDATE_AWARD_MEMBER[] =
        "UPDATE `osdkseasonalplay_awards` "
        "SET `memberId` = ? "
        "WHERE `id` = ?";

    ///////////////////////////////////////////////////////////
    // DELETE
    ///////////////////////////////////////////////////////////

    // delete an award by id (this is not the awardid!)
    static const char8_t OSDKSEASONALPLAY_DELETE_AWARD[] =
        "DELETE FROM `osdkseasonalplay_awards` "
        " WHERE `id` = ?";

    // delete a member who is registered
    static const char8_t OSDKSEASONALPLAY_DELETE_REGISTRATION[] =
        "DELETE FROM `osdkseasonalplay_registration` "
        "WHERE `memberId` = ? AND `memberType` = ?";

    ///////////////////////////////////////////////////////////
    // LOCK
    ///////////////////////////////////////////////////////////

    // lock a season
    static const char8_t OSDKSEASONALPLAY_LOCK_SEASON[] =
        "SELECT * "
        "FROM `osdkseasonalplay_seasons` "
        "WHERE `seasonId` = ? FOR UPDATE";

} // namespace OSDKSeasonalPlay
} // namespace Blaze

#endif // BLAZE_OSDKSEASONALPLAY_QUERIES_H
