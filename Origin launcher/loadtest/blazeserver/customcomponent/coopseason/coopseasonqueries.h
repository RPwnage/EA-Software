/*************************************************************************************************/
/*!
    \file   coopseasonqueries.h

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/coopseasonqueries.h#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
        (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

#ifndef BLAZE_COOPSEASON_QUERIES_H
#define BLAZE_COOPSEASON_QUERIES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace CoopSeason
{
    ///////////////////////////////////////////////////////////
    // FETCH
    ///////////////////////////////////////////////////////////
    
    // get all coop season ids for a target user
    static const char8_t COOPSEASON_FETCH_ALL_COOP_IDS[] =
        "SELECT `coopId`, `memberOneBlazeId`, `memberTwoBlazeId` "
        "FROM `coopseason_metadata` "
        "WHERE `memberOneBlazeId` = ? OR `memberTwoBlazeId` = ?";

    // get coop season pair meta data by coop id
    static const char8_t COOPSEASON_FETCH_COOP_DATA_BY_COOPID[] =
        "SELECT * "
        "FROM `coopseason_metadata` "
        "WHERE `coopId` = ?";

	// get coop season pair meta data by two blaze ids
	static const char8_t COOPSEASON_FETCH_COOP_DATA_BY_BLAZEIDS[] =
		"SELECT * "
		"FROM `coopseason_metadata` "
        "WHERE `memberOneBlazeId` = ? AND `memberTwoBlazeId` = ?";

	// insert coopseason meta data	
	static const char8_t COOPSEASON_INSERT_NEW_COOP_METADATA[] =
		"INSERT INTO `coopseason_metadata` ("
		"`memberOneBlazeId`, `memberTwoBlazeId`, `metaData`"
		") VALUES(?,?,?)";
		
	// update coopseason meta data		
	static const char8_t COOPSEASON_UPDATE_COOP_METADATA[] =
		"UPDATE `coopseason_metadata` "
		"SET `metaData` = ? "
		"WHERE `memberOneBlazeId` = ? AND `memberTwoBlazeId` = ?";

	// remove coop season pair record by coop id
	static const char8_t COOPSEASON_REMOVE_COOP_DATA_BY_COOPID[] =
		"DELETE FROM `coopseason_metadata` "
		"WHERE `coopId` = ?";

	// remove coop season pair record by two blaze ids
	static const char8_t COOPSEASON_REMOVE_COOP_DATA_BY_BLAZEIDS[] =
		"DELETE FROM `coopseason_metadata` "
		"WHERE `memberOneBlazeId` = ? AND `memberTwoBlazeId` = ?";

	// select a group of coop season pairs
	#define COOPSEASON_GET_IDENTITY \
	"SELECT `coopId`, `memberOneBlazeId`, `memberTwoBlazeId` FROM `coopseason_metadata` " \
	" WHERE `coopId` IN ( "


} // namespace CoopSeason
} // namespace Blaze

#endif // BLAZE_COOPSEASON_QUERIES_H
