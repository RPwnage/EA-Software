/*************************************************************************************************/
/*!
	\file   vprogrowthunlocksqueries.h


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_VPROGROWTHUNLOCKS_QUERIES_H
#define BLAZE_VPROGROWTHUNLOCKS_QUERIES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace VProGrowthUnlocks
{
	// get vpro xpEarned value for a single user
	static const char8_t VPROGROWTHUNLOCKS_FETCH_XPEARNED[] =
		"SELECT `xpEarned` FROM `vpro_loadouts` WHERE `entityId` = ? AND `loadoutId` = ? ";

	// get vpro unlocks loadouts for a single user
	static const char8_t VPROGROWTHUNLOCKS_FETCH_LOADOUTS[] =
		"SELECT * FROM `vpro_loadouts` WHERE `entityId` = ? ";

	// get vpro unlocks loadouts for a multiple users
	static const char8_t VPROGROWTHUNLOCKS_FETCH_LOADOUTS_MULTI_USERS[] =
		"SELECT * FROM `vpro_loadouts` WHERE `entityId` IN ( ";

	// reset vpro loadouts		
	static const char8_t VPROGROWTHUNLOCKS_RESET_LOADOUTS[] =
		"UPDATE `vpro_loadouts` "
		"SET `spconsumed` = 0, `unlocks1` = 0, `unlocks2` = 0, `unlocks3` = 0, `assignedPerks` = '\0\0\0', `vpro_height` = 0, `vpro_weight` = 0, `vpro_position` = 0, `vpro_foot` = 0 "
		"WHERE `entityId` = ? ";

	// reset single vpro loadouts		
	static const char8_t VPROGROWTHUNLOCKS_RESET_SINGLE_LOADOUT[] =
		"UPDATE `vpro_loadouts` "
		"SET `spconsumed` = 0, `unlocks1` = 0, `unlocks2` = 0, `unlocks3` = 0, `assignedPerks` = '\0\0\0' WHERE `entityId` = ? AND `loadoutId` = ? ";

	// insert vpro loadout 
	static const char8_t VPROGROWTHUNLOCKS_INSERT_LOADOUT[] =
		"INSERT INTO `vpro_loadouts` ("
		"`entityId`, `loadoutId`, `loadoutName`, `xpEarned`, `matchspgranted`, `objectivespgranted`, `spconsumed`, `vpro_height`, `vpro_weight`, `vpro_position`, `vpro_foot`"
		") VALUES(?,?,?,?,?,?,?,?,?,?,?)";

	// update vpro loadout peripheral
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_LOADOUT_PERIPHERALS[] =
		"UPDATE `vpro_loadouts` "
		"SET `loadoutName` = ?, `vpro_height` = ?, `vpro_weight` = ?, `vpro_position` = ?, `vpro_foot` = ? "
		"WHERE `entityId` = ? AND `loadoutId` = ? ";

		// update vpro loadout peripheral
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_LOADOUT_PERIPHERALS_WITHOUT_NAME[] =
		"UPDATE `vpro_loadouts` "
		"SET `vpro_height` = ?, `vpro_weight` = ?, `vpro_position` = ?, `vpro_foot` = ? "
		"WHERE `entityId` = ? AND `loadoutId` = ? ";

	// update vpro loadout unlocks
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_LOADOUT_UNLOCKS[] =
		"UPDATE `vpro_loadouts` "
		"SET `spconsumed` = ?, `unlocks1` = ?, `unlocks2` = ?, `unlocks3` = ?, `assignedPerks` = ? WHERE `entityId` = ? AND `loadoutId` = ? ";

	// update xpEarned 		
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_PLAYER_GROWTH[] =
		"UPDATE `vpro_loadouts` "
		"SET `xpEarned` = ?, `matchspgranted` = ? "
		"WHERE `entityId` = ? ";

	// update matchspgranted 		
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_MATCH_SPGRANTED[] =
		"UPDATE `vpro_loadouts` "
		"SET `matchspgranted` = ? "
		"WHERE `entityId` = ? ";

	// update objectivespgranted 		
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_OBJECTIVE_SPGRANTED[] =
		"UPDATE `vpro_loadouts` "
		"SET `objectivespgranted` = ? "
		"WHERE `entityId` = ? ";

	// update matchspgranted & objectivespgranted 		
	static const char8_t VPROGROWTHUNLOCKS_UPDATE_ALL_SPGRANTED[] =
		"UPDATE `vpro_loadouts` "
		"SET  `matchspgranted` = ?, `objectivespgranted` = ? "
		"WHERE `entityId` = ? ";

} // namespace VProGrowthUnlocks
} // namespace Blaze

#endif // BLAZE_VPROGROWTHUNLOCKS_QUERIES_H
