/*************************************************************************************************/
/*!
	\file   ProClubsCustomqueries.h


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ProClubsCustom_QUERIES_H
#define BLAZE_ProClubsCustom_QUERIES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace proclubscustom
{
	// get team customization setting for a club
	static const char8_t PROCLUBSCUSTOM_FETCH_TEAM_SETTINGS[] =
		"SELECT * FROM `clubscustom_TeamSettings` WHERE `clubId` = ? ";

	// update team customization settings
	static const char8_t PROCLUBSCUSTOM_UPDATE_TEAM_SETTINGS[] =
		"INSERT INTO `clubscustom_TeamSettings` ";
		//this statement tries to insert in to the table where clubId is set to UNIQUE, and if it tries to insert 
		//when row with the same clubId already exists, it will just do an update instead. statement example:
		//"INSERT INTO `clubscustom_TeamSettings` (`clubId`, `customkit1`, `customkit2`) VALUES (100, 200, 300) ON duplicate key update `customkit1` = 200, `customkit2` = 300";


	// get ai player customization setting for a club
	static const char8_t PROCLUBSCUSTOM_FETCH_AI_PLAYER_SETTINGS[] =
		"SELECT * FROM `clubscustom_AIPlayerCustomization` WHERE `clubId` = ? ";

	// update team customization settings
	static const char8_t PROCLUBSCUSTOM_UPDATE_AI_PLAYER_SETTINGS[] =
		"UPDATE `clubscustom_AIPlayerCustomization` SET ";

	// insert in to team customization settings 
	static const char8_t PROCLUBSCUSTOM_INSERT_AI_PLAYER_SETTINGS[] =
		"INSERT INTO `clubscustom_AIPlayerCustomization` ";
		
	// get team customization setting for a club
	static const char8_t PROCLUBSCUSTOM_FETCH_CUSTOM_TACTICS[] =
		"SELECT * FROM `clubscustom_Tactics` WHERE `clubId` = ? ";

	// insert team custom tactics data 
	static const char8_t PROCLUBSCUSTOM_INSERT_CUSTOM_TACTICS[] =
		"INSERT INTO `clubscustom_Tactics` (`clubId`, `slotId`, `tacticsSlotData`) VALUES(?, ?, ?) "
		"ON DUPLICATE KEY UPDATE `tacticsSlotData` = VALUES(`tacticsSlotData`)";

	// reset the Ai Player names to a default 
	static const char8_t PROCLUBSCUSTOM_RESET_AI_PLAYER_NAMES[] =
		"UPDATE `clubscustom_AIPlayerCustomization` SET `firstname` = \"AI\", `lastname` = CONCAT(\"Player\", clubscustom_AIPlayerCustomization.avatarId) WHERE `clubId` = ?";

	// get the Ai Player name for a club 
	static const char8_t PROCLUBSCUSTOM_FETCH_AI_PLAYER_NAMES[] =
		"SELECT `firstname`, `lastname` FROM `clubscustom_AIPlayerCustomization` WHERE `clubId` = ?";

	// get avatar name for a specific user
	static const char8_t PROCLUBSCUSTOM_FETCH_AVATAR_NAME[] =
		"SELECT `firstname`, `lastname` FROM `proclubs_Avatar` WHERE `userId` = ?";

	// get avatar name for a specific user
	static const char8_t PROCLUBSCUSTOM_FETCH_AVATAR_DATA[] =
		"SELECT `isNameProfane`, `avatarNameResetCount` FROM `proclubs_Avatar` WHERE `userId` = ?";

	// insert avatar name
	static const char8_t PROCLUBSCUSTOM_INSERT_AVATAR_NAME[] =
		"INSERT INTO `proclubs_Avatar` (`userId`, `firstname`, `lastname`) VALUES(?, ?, ?) "
		"ON DUPLICATE KEY UPDATE `firstname` = VALUES(`firstname`), `lastname` = VALUES(`lastname`), `isNameProfane` = FALSE";

	// flag users Avatar name 
	static const char8_t PROCLUBSCUSTOM_FLAG_PROFANE_AVATAR_NAME[] =
		"INSERT INTO `proclubs_Avatar` (`userId`, `firstname`, `lastname`, `isNameProfane`, `avatarNameResetCount`) VALUES(?, \"BAD\" , \"Player\", TRUE, 0) "
		"ON DUPLICATE KEY UPDATE `firstname` = \"BAD\", `lastname` = \"Player\", `isNameProfane` = TRUE, `avatarNameResetCount` = `avatarNameResetCount` + 1";

} // namespace proclubsCustom
} // namespace Blaze

#endif // BLAZE_ProClubsCustom_QUERIES_H
