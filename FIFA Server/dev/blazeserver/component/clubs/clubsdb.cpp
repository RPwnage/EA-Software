/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/blaze.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersession.h"

#include "clubs/clubsdb.h"

namespace Blaze
{

namespace Clubs
{

#define CLUBSDB_INSERT_CLUB \
    "INSERT INTO `clubs_clubs` (" \
        " `domainId` , `name` , `teamId`, `language`, `nonuniquename`" \
        "  , `description`, `region`, `logoId`, `bannerId`" \
        "  , `acceptanceFlags`, `artPackType`, `password`, `seasonLevel`" \
        "  , `previousSeasonLevel`, `lastSeasonLevelUpdate`, `metaData`" \
        "  , `metaDataType`, `metaData2`, `metaDataType2`" \
        "  , `joinAcceptance`, `skipPasswordCheckAcceptance`, `petitionAcceptance`" \
        "  , `custOpt1`, `custOpt2`, `custOpt3`, `custOpt4`, `custOpt5`, `lastUpdatedBy`" \
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define CLUBSDB_UPDATE_CLUB_SETTINGS \
    "UPDATE `clubs_clubs` SET" \
    "    `version` = `version` + 1" \
    "  , `teamId` = ?, `language` = ?, `nonuniquename` = ?" \
    "  , `description` = ?, `region` = ?, `logoId`= ?" \
    "  , `bannerId` = ?, `acceptanceFlags` = ?, `artPackType` = ?, `password` = ?" \
    "  , `seasonLevel` = ?, `previousSeasonLevel` = ?" \
    "  , `lastSeasonLevelUpdate` = ?, `metaData` = ?, `metaDataType` = ?" \
    "  , `metaData2` = ?, `metaDataType2` = ?" \
    "  , `joinAcceptance` = ?, `skipPasswordCheckAcceptance` = ?, `petitionAcceptance` = ?" \
    "  , `custOpt1` = ?, `custOpt2` =?, `custOpt3` = ?, `custOpt4` = ?, `custOpt5` = ?" \
    " WHERE `clubId` = ?"

#define CLUBSDB_UPDATE_CLUB_LAST_GAME \
    " UPDATE clubs_clubs SET " \
    "   `lastOppo` = ?, `lastResult` = ?, `lastGameTime`=NOW(), `lastActiveTime`=NOW() " \
    " WHERE `clubId` = ?"

#define CLUBSDB_SELECT_CLUB \
    " SELECT " \
    "    `clubId`, `domainId`, `name`" \
    "  , UNIX_TIMESTAMP(`creationTime`), UNIX_TIMESTAMP(`lastActiveTime`)" \
    "  , `memberCount`, `gmCount`, `awardCount`" \
    "  , `lastOppo`, `lastResult`, UNIX_TIMESTAMP(`lastGameTime`) " \
    "  , `rivalCount` " \
    "  , `teamId`, `language`, `nonuniquename`" \
    "  , `description`, `region`, `logoId`" \
    "  , `bannerId`, `acceptanceFlags`" \
    "  , `artPackType`, `password`, `seasonLevel`, `previousSeasonLevel`" \
    "  , `lastSeasonLevelUpdate`, `metaData`" \
    "  , `metaDataType`, `metaData2`, `metaDataType2`" \
    "  , `joinAcceptance`, `skipPasswordCheckAcceptance`, `petitionAcceptance`" \
    "  , `custOpt1`, `custOpt2` " \
    "  , `custOpt3`, `custOpt4`, `custOpt5`, `lastUpdatedBy`" \
    "  , `version`, `isNameProfane`" \
    " FROM `clubs_clubs`" \
    " WHERE `clubId` = $U"

#define CLUBSDB_SELECT_CLUBS \
    " SELECT $s" \
    "    `clubId`, `domainId`, `name`" \
    "  , UNIX_TIMESTAMP(`creationTime`), UNIX_TIMESTAMP(`lastActiveTime`)" \
    "  , `memberCount`, `gmCount`, `awardCount`" \
    "  , `lastOppo`, `lastResult`, UNIX_TIMESTAMP(`lastGameTime`)" \
    "  , `rivalCount` " \
    "  , `teamId`, `language`, `nonuniquename`" \
    "  , `description`, `region`, `logoId`" \
    "  , `bannerId`, `acceptanceFlags`" \
    "  , `artPackType`, `password`, `seasonLevel`, `previousSeasonLevel`" \
    "  , `lastSeasonLevelUpdate`, `metaData`, `metaDataType`, `metaData2`, `metaDataType2`" \
    "  , `joinAcceptance`, `skipPasswordCheckAcceptance`, `petitionAcceptance`" \
    "  , `custOpt1`, `custOpt2`" \
    "  , `custOpt3`, `custOpt4`, `custOpt5`, `lastUpdatedBy`" \
    "  , `version`, `isNameProfane`" \
    " FROM `clubs_clubs` WHERE `clubId` IN ("

#define CLUBSDB_REMOVE_CLUB \
    "DELETE FROM `clubs_clubs` WHERE `clubId`=?"

#define CLUBSDB_SELECT_CLUB_MEMBERS \
    "SELECT $s " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`membershipStatus`" \
    "  , UNIX_TIMESTAMP(`cm`.`memberSince`) `memberSince`" \
    "  , `cm`.`metadata`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    " WHERE " \
    "    `cm`.`clubId` = $U "

#define CLUBSDB_SELECT_BLAZEIDS_FOR_PERSONANAME \
    "SELECT " \
    " `userinfo`.`blazeId`" \
    " FROM " \
    "   `userinfo` " \
    " WHERE " \
    " `userinfo`.`persona` " \
    " LIKE '%$s%' "

#define CLUBSDB_SELECT_CLUB_MEMBERS_BY_BLAZEID \
    "SELECT " \
    "   `clubs_members`.`blazeId`" \
    " , `clubs_members`.`membershipStatus`"\
    " , UNIX_TIMESTAMP(`clubs_members`.`memberSince`) `memberSince`" \
    " , `clubs_members`.`metadata`" \
    " FROM " \
    "   `clubs_members`" \
    " WHERE " \
    "   `clubs_members`.`clubId` = $U "\
    " AND `clubs_members`.`blazeId` IN ("

#define CLUBSDB_INSERT_MEMBER \
    "INSERT INTO `clubs_members` (" \
    "    `blazeId`" \
    "  , `clubId`" \
    "  , `membershipStatus`" \
    "  ) VALUES (?, ?, ?)"

#define CLUBSDB_REMOVE_MEMBER \
    "DELETE FROM `clubs_members`" \
    "  WHERE `clubId` = ? AND `blazeId` = ? "

#define CLUBSDB_UPSERT_USER_LAST_LEAVE_TIME \
    " INSERT INTO `clubs_lastleavetime` ( " \
    "   `blazeId` " \
    " , `domainId` " \
    " , `lastLeaveTime`) " \
    " VALUES (?, ?, NOW()) " \
    " ON DUPLICATE KEY UPDATE " \
    "   `lastLeaveTime` = VALUES(`lastLeaveTime`) "
    
#define CLUBSDB_GET_USER_LAST_LEAVE_TIME \
    " SELECT " \
    "    UNIX_TIMESTAMP(`lastLeaveTime`) " \
    " FROM  `clubs_lastleavetime` " \
    " WHERE " \
    "    `blazeId` = $D " \
    "    AND `domainId` = $u "

#define CLUBSDB_GET_CLUB_MEMBERSHIPS_IN_DOMAIN \
    "SELECT " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`membershipStatus`" \
    "  , UNIX_TIMESTAMP(`cm`.`memberSince`) `memberSince`" \
    "  , `cm`.`clubId` " \
    "  , `cm`.`metadata`" \
    "  , `cc`.`name`" \
    "  , `cc`.`domainId`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    "  , `clubs_clubs` `cc` " \
    " WHERE " \
    "    `cm`.`blazeId` = $D " \
    "    AND `cc`.`domainId` = $u " \
    "    AND `cc`.`clubId` = `cm`.`clubId` "

#define CLUBSDB_GET_CLUB_MEMBERSHIP_IN_CLUB \
    "SELECT " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`membershipStatus`" \
    "  , UNIX_TIMESTAMP(`cm`.`memberSince`) `memberSince`" \
    "  , `cm`.`clubId` " \
    "  , `cm`.`metadata`" \
    "  , `cc`.`name`" \
    "  , `cc`.`domainId`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    "  , `clubs_clubs` `cc` " \
    " WHERE " \
    "    `cm`.`blazeId` = $D " \
    "    AND `cc`.`clubId` = `cm`.`clubId` " \
    "    AND `cc`.`clubId` = $U "

#define CLUBSDB_GET_CLUB_MEMBERSHIP_IN_CLUB_FOR_USERS \
    "SELECT " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`membershipStatus`" \
    "  , UNIX_TIMESTAMP(`cm`.`memberSince`) `memberSince`" \
    "  , `cm`.`clubId` " \
    "  , `cm`.`metadata`" \
    "  , `cc`.`name`" \
    "  , `cc`.`domainId`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    "  , `clubs_clubs` `cc` " \
    " WHERE " \
    "    `cc`.`clubId` = `cm`.`clubId` " \
    "    AND `cc`.`clubId` = $U " \
    "    AND `cm`.`blazeId` IN ( "

#define CLUBSDB_INSERT_NEWS \
    "INSERT INTO `clubs_news` ( " \
    "    `clubId` " \
    "  , `newsType` " \
    "  , `contentCreator` " \
    "  , `text` " \
    "  , `stringId` " \
    "  , `paramList` " \
    "  , `associatedClubId`)" \
    " VALUES (?, ?, ?, ?, ?, ?, ?)"

#define CLUBSDB_DELETE_OLDEST_NEWS \
    "DELETE FROM `clubs_news` " \
    " WHERE `clubid` = $U " \
    " ORDER BY `timestamp` ASC LIMIT 1"

#define CLUBSDB_COUNT_NEWS \
    "SELECT COUNT(1) " \
    " FROM `clubs_news` " \
    " WHERE clubId=$U"

#define CLUBSDB_COUNT_NEWS_FOR_MULTIPLE_CLUBS \
    "SELECT COUNT(1) " \
    " FROM `clubs_news` "

#define CLUBSDB_SELECT_NEWS \
    "SELECT " \
    "    `clubid` " \
    "  , `contentCreator` " \
    "  , `newsType` " \
    "  , `text` " \
    "  , `stringId` " \
    "  , `paramList` " \
    "  , `associatedClubId` " \
    "  , UNIX_TIMESTAMP(`cn`.`timestamp`) AS `timestamp` " \
    "  , `flags` " \
    "  , `newsId` " \
    " FROM `clubs_news` `cn` " \
    " WHERE `clubId` = $U "

#define CLUBSDB_SELECT_NEWS_ASSOICATE \
    "SELECT " \
    "    `clubid` " \
    "  , `contentCreator` " \
    "  , `newsType` " \
    "  , `text` " \
    "  , `stringId` " \
    "  , `paramList` " \
    "  , `associatedClubId` " \
    "  , UNIX_TIMESTAMP(`cn`.`timestamp`) AS `timestamp` " \
    "  , `flags` " \
    "  , `newsId` " \
    " FROM `clubs_news` `cn` " \
    " WHERE `associatedClubId` = $U AND clubId <> associatedClubId AND `newsType` IN (0, 2) "

#define CLUBSDB_SELECT_SERVER_NEWS \
    "SELECT " \
    "    `clubid` " \
    "  , `contentCreator` " \
    "  , `newsType` " \
    "  , `text` " \
    "  , `stringId` " \
    "  , `paramList` " \
    "  , `associatedClubId` " \
    "  , UNIX_TIMESTAMP(`cn`.`timestamp`) AS `timestamp` " \
    "  , `flags` " \
    "  , `newsId` " \
    " FROM `clubs_news` `cn` " \
    " WHERE `newsId` = $u "

#define CLUBSDB_UPDATE_NEWS_ITEM_FLAGS \
    "UPDATE " \
    "    `clubs_news` " \
    " SET " \
    "    `flags` = (`flags` & ?) | ? " \
    " ,  `timestamp` = `timestamp` " \
    " WHERE " \
    "  `newsId` = ? "

#define CLUBSDB_PROMOTE_CLUB_GM \
    "UPDATE " \
    "    `clubs_members` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "SET " \
    "  `cm`.`membershipStatus` = ?, " \
    "  `cc`.`gmCount` = `cc`.`gmCount` + 1, " \
    "  `cc`.`lastActiveTime` = NOW() " \
    " WHERE " \
    "    `cm`.`blazeId` = ? " \
    "    AND `cm`.`clubId` = `cc`.`clubId` " \
    "    AND `cc`.`clubId` = ? " \
    "    AND `cc`.`gmCount` < ? "

#define CLUBSDB_DEMOTE_CLUB_MEMBER \
    "UPDATE " \
    "    `clubs_members` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "SET " \
    "  `cm`.`membershipStatus` = ?, " \
    "  `cc`.`gmCount` = `cc`.`gmCount` - 1, " \
    "  `cc`.`lastActiveTime` = NOW() " \
    " WHERE " \
    "    `cm`.`blazeId` = ? " \
    "    AND `cm`.`clubId` = `cc`.`clubId` " \
    "    AND `cc`.`clubId` = ? "

#define CLUBSDB_INSERT_MESSAGE \
    "INSERT INTO `clubs_messages` ( " \
    "    `clubId` " \
    "  , `messageType` " \
    "  , `sender` " \
    "  , `receiver` " \
    "  , `timestamp`) " \
    " VALUES (?, ?, ?, ?, NOW())"

#define CLUBSDB_UPDATE_LAST_ACTIVE \
    "UPDATE " \
    "    `clubs_clubs`  " \
    "SET " \
    "  `lastActiveTime` = NOW() " \
    " WHERE " \
    "    `clubId` = ? "

#define CLUBSDB_COUNT_MESSAGES_RECEIVED \
    "SELECT COUNT(1) FROM " \
    "    `clubs_messages`  " \
    "WHERE " \
    "    `messageType` = $u"

#define CLUBSDB_COUNT_MESSAGES_SENT \
    "SELECT COUNT(1) FROM " \
    "    `clubs_messages`  " \
    "WHERE " \
    "    `messageType` = $u"
    
#define CLUBSDB_COUNT_CLUB_MESSAGES \
    "SELECT COUNT(1) FROM " \
    "    `clubs_messages`  " \
    "WHERE " \
    "    `messageType` = $u " \
    "    AND `clubid` = $U "

#define CLUBSDB_COUNT_USER_MESSAGES \
    "SELECT COUNT(1) FROM " \
    "    `clubs_messages` `cm` " \
    "  , `clubs_clubs` `cc` " \
    "WHERE " \
    "    `cm`.`messageType` = $u " \
    "    AND `cc`.`clubId` = `cm`.`clubId` " \
    "    AND `cc`.`domainId` = $u "

#define CLUBSDB_LOCK_USER_MESSAGES \
    "SELECT * FROM `clubs_messages` WHERE `sender` = $D " \
    "                               OR `receiver` = $D " \
    " FOR UPDATE"

#define CLUBSDB_LOCK_MEMBER_DOMAIN \
    "INSERT IGNORE INTO `clubs_memberdomains` VALUES ($D, $u) "

#define CLUBSDB_UPDATE_METADATA \
    "UPDATE `clubs_clubs` " \
    " SET " \
    "   `metaDataType` = ? " \
    " , `metaData` = ? " \
    " WHERE " \
    "   `clubId` = ? "
 
#define CLUBSDB_UPDATE_METADATA_2 \
    "UPDATE `clubs_clubs` " \
    " SET " \
    "   `metaDataType2` = ? " \
    " , `metaData2` = ? " \
    " WHERE " \
    "   `clubId` = ? "
    
#define CLUBSDB_UPDATE_METADATA_LAST_ACTIVE \
    "UPDATE `clubs_clubs` " \
    " SET " \
    "   `metaDataType` = ? " \
    " , `metaData` = ? " \
    " , `lastActiveTime` = NOW() " \
    " WHERE " \
    "   `clubId` = ? "

#define CLUBSDB_UPDATE_METADATA_2_LAST_ACTIVE \
    "UPDATE `clubs_clubs` " \
    " SET " \
    "   `metaDataType2` = ? " \
    " , `metaData2` = ? " \
    " , `lastActiveTime` = NOW() " \
    " WHERE " \
    "   `clubId` = ? "

#define CLUBSDB_FIND_CLUBS \
    " SELECT $s $s " \
    "    `cc`.`clubId`, `cc`.`domainId`, `cc`.`name`" \
    "  , UNIX_TIMESTAMP(`cc`.`creationTime`), UNIX_TIMESTAMP(`cc`.`lastActiveTime`)" \
    "  , `cc`.`memberCount`, `cc`.`gmCount`, `cc`.`awardCount`" \
    "  , `cc`.`lastOppo`, `cc`.`lastResult`, UNIX_TIMESTAMP(`cc`.`lastGameTime`) " \
    "  , `cc`.`rivalCount` " \
    "  , `cc`.`teamId`, `cc`.`language`, `cc`.`nonuniquename`" \
    "  , `cc`.`description`, `cc`.`region`, `cc`.`logoId`" \
    "  , `cc`.`bannerId`, `cc`.`acceptanceFlags`" \
    "  , `cc`.`artPackType`, `cc`.`password`, `cc`.`seasonLevel`, `cc`.`previousSeasonLevel`" \
    "  , `cc`.`lastSeasonLevelUpdate`, `cc`.`metaData`, `cc`.`metaDataType`" \
    "  , `cc`.`metaData2`, `cc`.`metaDataType2`, `cc`.`joinAcceptance`, `cc`.`skipPasswordCheckAcceptance`" \
    "  , `cc`.`petitionAcceptance`, `cc`.`custOpt1`, `cc`.`custOpt2`" \
    "  , `cc`.`custOpt3`, `cc`.`custOpt4`, `cc`.`custOpt5`" \
    " FROM `clubs_clubs` `cc` "

#define CLUBSDB_GET_MESSAGES_BY_USER_RECEIVED \
    "SELECT `cm`.`messageId`, " \
    "    `cm`.`clubId`, " \
    "    `cm`.`messageType`, " \
    "    `sender`, " \
    "    `cm`.`receiver`, " \
    "    UNIX_TIMESTAMP(`cm`.`timestamp`) as `timestamp`, " \
    "    `cc`.`name` " \
    "FROM " \
    "    `clubs_messages` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "WHERE " \
    "  `receiver` = $D" \
    "  AND `messageType` = $u" \
    "  AND `cm`.clubId = `cc`.clubId" \
    "  ORDER BY `timestamp` $s "

#define CLUBSDB_GET_MESSAGES_BY_USER_SENT \
    "SELECT `cm`.`messageId`, " \
    "    `cm`.`clubId`, " \
    "    `cm`.`messageType`, " \
    "    `sender`, " \
    "    `cm`.`receiver`, " \
    "    UNIX_TIMESTAMP(`cm`.`timestamp`) as `timestamp`, " \
    "    `cc`.`name` " \
    "FROM " \
    "    `clubs_messages` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "WHERE " \
    "  `sender` = $D" \
    "  AND `messageType` = $u" \
    "  AND `cm`.clubId = `cc`.clubId" \
    "  ORDER BY `timestamp` $s "

#define CLUBSDB_GET_MESSAGES_BY_CLUB \
    "SELECT `cm`.`messageId`, " \
    "    `cm`.`clubId`, " \
    "    `cm`.`messageType`, " \
    "    `sender`, " \
    "    `cm`.`receiver`, " \
    "    UNIX_TIMESTAMP(`cm`.`timestamp`) as `timestamp`, " \
    "    `cc`.`name` " \
    "FROM " \
    "    `clubs_messages` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "WHERE " \
    "  `cm`.`clubId` IN ($s)" \
    "  AND `messageType` = $u" \
    "  AND `cm`.clubId = `cc`.clubId" \
    "  ORDER BY `timestamp` $s "

// a dummy `name` column is required for ClubsDatabase::clubDbRowToClubMessage()
#define CLUBSDB_GET_MESSAGE \
    "SELECT `messageId`, " \
    "    `clubId`, " \
    "    `messageType`, " \
    "    `sender`, " \
    "    `receiver`, " \
    "    UNIX_TIMESTAMP(`timestamp`) as `timestamp`, " \
    "    '' as `name` " \
    "FROM " \
    "    `clubs_messages`  " \
    "WHERE " \
    "  `messageId` = $u"

#define CLUBSDB_GET_NAME_RESET_COUNT \
    "SELECT `clubNameResetCount` " \
    "FROM " \
    "    `clubs_clubs`  " \
    "WHERE " \
    "  `clubId` = $u"

#define CLUBSDB_SORT_ASC "ASC"
#define CLUBSDB_SORT_DESC "DESC"

#define CLUBSDB_DELETE_MESSAGE \
    "DELETE FROM `clubs_messages` " \
    " WHERE `messageid` = ? "

#define CLUBSDB_GET_IDENTITY \
    "SELECT `clubid`, `name` FROM `clubs_clubs` " \
    " WHERE `clubid` IN ( "

#define CLUBSDB_GET_IDENTITY_BY_NAME \
    "SELECT `clubid` FROM `clubs_clubs` " \
    " WHERE `name` = '$s';"

#define CLUBSDB_LOCK_CLUB \
    "SELECT * FROM `clubs_clubs` WHERE `clubId` = $U FOR UPDATE"

#define CLUBSDB_LOCK_CLUBS \
    "SELECT * FROM `clubs_clubs` WHERE `clubId` IN ("

#define CLUBSDB_GET_CLUB_MEMBERSHIP_FOR_USERS \
    "SELECT " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`membershipStatus`" \
    "  , UNIX_TIMESTAMP(`cm`.`memberSince`) `memberSince`" \
    "  , `cm`.`clubId` " \
    "  , `cm`.`metadata`" \
    "  , `cc`.`name`" \
    "  , `cc`.`domainId`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    "  , `clubs_clubs` `cc` " \
    " WHERE " \
    "    `cc`.`clubId` = `cm`.`clubId` " \
    "    AND `cm`.`blazeId` IN ("

#define CLUBSDB_GET_CLUB_METADATA_FOR_USERS \
    "SELECT " \
    "    `cm`.`blazeId` " \
    "  , `cm`.`clubId` " \
    "  , `cc`.`metaData`" \
    "  , `cc`.`metaDataType`" \
    " FROM " \
    "    `clubs_members` `cm` " \
    "  , `clubs_clubs` `cc` " \
    " WHERE " \
    "    `cc`.`clubId` = `cm`.`clubId` " \
    "    AND `cm`.`blazeId` IN ("

#define CLUBSDB_GET_CLUB_METADATA \
    "SELECT " \
    " `clubId` " \
    "  ,`metaData`" \
    "  , `metaDataType`" \
    " FROM " \
    " `clubs_clubs` " \
    " WHERE " \
    "  `clubId` = ? "    

#define CLUBSDB_GET_CLUB_RECORDS \
    "SELECT " \
    "    `cr`.`recordId` " \
    "  , `cr`.`blazeId`" \
    "  , `cr`.`recordStatInt`" \
    "  , `cr`.`recordStatFloat`" \
    "  , `cr`.`recordStatType`" \
    "  , UNIX_TIMESTAMP(`cr`.`timestamp`) `lastupdate`" \
    " FROM " \
    "    `clubs_recordbooks` `cr` " \
    " WHERE " \
    "    `cr`.`clubId` = $U "

#define CLUBSDB_RESET_CLUB_RECORDS \
    "DELETE " \
    " FROM `clubs_recordbooks` " \
    " WHERE " \
    "    `clubid` = $U "

#define CLUBSDB_RESET_CLUB_RECORDS_LIST \
    "DELETE " \
    " FROM `clubs_recordbooks` " \
    " WHERE " \
    "     `clubId` = $U " \
    " AND `recordId` IN ( $s ) "

#define CLUBSDB_GET_CLUB_RECORD \
    "SELECT " \
    "    `cr`.`recordId` " \
    "  , `cr`.`blazeId`" \
    "  , `cr`.`recordStatInt`" \
    "  , `cr`.`recordStatFloat`" \
    "  , `cr`.`recordStatType`" \
    "  , UNIX_TIMESTAMP(`cr`.`timestamp`) `lastupdate`" \
    " FROM " \
    "    `clubs_recordbooks` `cr` " \
    " WHERE " \
    "    `cr`.`clubId` = ? " \
    "    AND `cr`.`recordId` = ? "

#define CLUBSDB_UPSERT_CLUB_RECORD \
    "REPLACE INTO `clubs_recordbooks` " \
    "  (`clubId` " \
    "  ,`recordId` " \
    "  ,`blazeId`" \
    "  ,`recordStatInt`" \
    "  ,`recordStatFloat`" \
    "  ,`recordStatType`" \
    "  ,`timestamp`) " \
    " VALUES (?,?,?,?,?,?,NOW()) "

#define CLUBSDB_UPSERT_CLUB_AWARD \
    "INSERT INTO `clubs_awards` SET" \
    "   `clubid`=? " \
    " , `awardid`=? " \
    " , `timestamp`=NOW() " \
    " ON DUPLICATE KEY UPDATE " \
    "   `awardCount` = `awardCount` + 1 " \
    " , `timestamp`=NOW() "

#define CLUBSDB_GET_CLUB_AWARDS \
    "SELECT " \
    "   `awardid` " \
    " , `awardCount` " \
    " , UNIX_TIMESTAMP(`timestamp`) " \
    " FROM " \
    "   `clubs_awards` " \
    " WHERE " \
    "    `clubid` = $U "
    
#define CLUBSDB_UPDATE_CLUB_AWARD_COUNT \
    "UPDATE  " \
    "   `clubs_clubs` " \
    "SET " \
    "   `awardCount` = `awardCount` + 1 " \
    " , `lastActiveTime` = NOW() " \
    " WHERE " \
    "    `clubid` = ? "

#define CLUBSDB_UPDATE_MEMBER_METADATA \
    "UPDATE  " \
    "   `clubs_members` " \
    "SET " \
    "   `metadata` = ? " \
    " WHERE " \
    "    `clubId` = ? " \
    "     AND `blazeid` = ? "

#define CLUBSDB_INSERT_SEASON_STATE \
    "REPLACE INTO  " \
    "   `clubs_seasons` " \
    "  (`state` " \
    "  ,`lastUpdated`) " \
    " VALUES ($u,FROM_UNIXTIME($D)) "

#define CLUBSDB_UPDATE_SEASON_STATE \
    "UPDATE  " \
    "   `clubs_seasons` " \
    "  SET `state`=?, `lastUpdated`=FROM_UNIXTIME(?) "

#define CLUBSDB_LOCK_SEASON \
    "SELECT * FROM `clubs_seasons` FOR UPDATE"

#define CLUBSDB_GET_SEASON_STATE \
    "SELECT state, UNIX_TIMESTAMP(lastUpdated) FROM `clubs_seasons`"

#define CLUBSDB_DELETE_AWARDS \
    "DELETE FROM `clubs_awards` WHERE `awardId` IN ("

#define CLUBSDB_LIST_CLUB_RIVALS \
    "SELECT " \
    "   `rivalClubId` " \
    " , `custOpt1` " \
    " , `custOpt2` " \
    " , `custOpt3` " \
    " , `metadata` " \
    " , UNIX_TIMESTAMP(`creationTime`) " \
    " , UNIX_TIMESTAMP(`lastUpdateTime`) " \
    " FROM `clubs_rivals` " \
    " WHERE `clubId` = $U " \
    " ORDER BY `creationTime` DESC"

#define CLUBSDB_DELETE_RIVALS \
    "DELETE " \
    "  FROM `clubs_rivals` " \
    "  WHERE `clubId` = $U "

#define CLUBSDB_INSERT_RIVALS \
    "INSERT " \
    "  INTO `clubs_rivals` " \
    "  (  `clubId` " \
    "   , `rivalClubId` " \
    "   , `custOpt1` " \
    "   , `custOpt2` " \
    "   , `custOpt3` " \
    "   , `metadata` " \
    "   , `creationTime` " \
    "   , `lastUpdateTime` " \
    "  ) " \
    "  VALUES (?, ?, ?, ?, ?, ?, NOW(), NOW()) "

#define CLUBSDB_UPDATE_RIVAL \
    "UPDATE " \
    "  `clubs_rivals` " \
    "SET " \
    "     `custOpt1` = ? " \
    "   , `custOpt2` = ? " \
    "   , `custOpt3` = ? " \
    "   , `metadata` = ?" \
    "   , `lastUpdateTime` = NOW() " \
    "   , `creationTime` = `creationTime` " \
    "WHERE " \
    "   `clubId`= ? " \
    "   AND `rivalClubId` = ? " \
    "   AND UNIX_TIMESTAMP(`creationTime`) = ? "


#define CLUBSDB_DELETE_OLDEST_RIVAL \
    "DELETE FROM `clubs_rivals` " \
    " WHERE `clubid` = ? " \
    " ORDER BY `creationTime` ASC LIMIT 1"

#define CLUBSDB_INCREMENT_RIVAL_COUNT \
    "UPDATE `clubs_clubs` " \
    "  SET `rivalCount` = `rivalCount` + 1, `lastActiveTime` = NOW() " \
    "  WHERE `clubId` = ? "

// The following #defines use 'UserId', to represent the 'BlazeId' of the users. (Not changed to avoid changing DB fields.)
#define CLUBSDB_INSERT_TICKERMSG \
    "INSERT INTO `clubs_tickermsgs` ( " \
    "    `clubId` " \
    "  , `includeUserId` " \
    "  , `excludeUserId` " \
    "  , `text` " \
    "  , `params` " \
    "  , `metadata`) " \
    " VALUES (?, ?, ?, ?, ?, ?) "

#define CLUBSDB_DELETE_OLDEST_TICKERMSG \
    "DELETE FROM `clubs_tickermsgs` " \
    " WHERE `timestamp` < FROM_UNIXTIME(?) "

#define CLUBSDB_GET_TICKERMSGS \
    "SELECT `clubId`, `text`, `params`, `metadata`, UNIX_TIMESTAMP(`timestamp`) " \
    " FROM `clubs_tickermsgs` " \
    " WHERE  (`includeUserId` = $D OR `includeUserId` = 0) " \
    " AND `excludeUserId` != $D " \
    " AND `clubid` IN (0"

#define CLUBSDB_COUNT_MESSAGES \
    "SELECT COUNT(1) FROM `clubs_messages` " \
    "  WHERE `clubId` = $U " \
    "  AND   `messageType` = $u "

#define CLUBSDB_COUNT_MESSAGES_FOR_CLUBS \
    "SELECT clubId, COUNT(1) FROM `clubs_messages` " \
    "  WHERE `messageType` = $u " \
    "  AND  clubId IN ("

#define CLUBSDB_GET_CLUB_BANS \
    "SELECT " \
    "   `userId`, " \
    "   `banStatus` " \
    " FROM " \
    "   `clubs_bans` " \
    " WHERE " \
    "   `clubId` = $U "

#define CLUBSDB_GET_USER_BANS \
    "SELECT " \
    "   `clubId`, " \
    "   `banStatus` " \
    " FROM " \
    "   `clubs_bans` " \
    " WHERE " \
    "   `userId` = $D "

#define CLUBSDB_GET_BAN \
    "SELECT " \
    "   `banStatus` " \
    " FROM " \
    "   `clubs_bans` " \
    " WHERE " \
    "   `clubId` = $U " \
    "   AND `userId` = $D "

#define CLUBSDB_ADD_BAN \
    "INSERT " \
    " INTO `clubs_bans` " \
    " SET " \
    "   `clubId` = $U, " \
    "   `userId` = $D, " \
    "   `banStatus` = $u "

#define CLUBSDB_REMOVE_BAN \
    "DELETE " \
    " FROM `clubs_bans` " \
    " WHERE " \
    "   `clubId` = $U AND " \
    "   `userId` = $D "
    
#define CLUBSDB_CLUBS_IN_WHICH_REQUESTOR_IS_GM_OF_USER \
    "SELECT " \
    "   `clubId` " \
    " FROM `clubs_members` requestor" \
    " WHERE " \
    " requestor.blazeId = $D AND " \
    " requestor.membershipStatus >= $u AND " \
    " requestor.clubId IN (SELECT `clubId` FROM `clubs_members` user WHERE user.blazeId = $D) "

#define CLUBSDB_SELECT_TAG \
    " SELECT " \
    "    `tagId`, `tagText`" \
    " FROM  `clubs_tags` " \
    " WHERE " \
    "    `tagText` = '$s' "

#define CLUBSDB_INSERT_TAG \
    "INSERT INTO `clubs_tags` (" \
    "    `tagText` " \
    ") VALUES ('$s') "

#define CLUBSDB_INSERT_CLUB_TAGS \
    " INSERT INTO `clubs_club2tag` (" \
    "    `clubId`, `tagId` " \
    ") VALUES "

#define CLUBSDB_REMOVE_CLUB_TAGS \
    "DELETE FROM `clubs_club2tag` WHERE `clubId`= $U"

#define CLUBSDB_GET_CLUBS_TAGS \
    "SELECT " \
    "   `c2t`.`clubId`, `t`.`tagText` " \
    "FROM `clubs_club2tag` `c2t` " \
    "INNER JOIN `clubs_tags` `t` " \
    "ON `c2t`.`tagId`=`t`.`tagId` " \
    "WHERE `c2t`.`clubId` IN ("

#define  CLUBDB_FILTER_TAGS \
    "INNER JOIN (SELECT $s" \
    "   `c2t`.`clubId` " \
    "FROM `clubs_club2tag` `c2t` " \
    "INNER JOIN `clubs_tags` `t` " \
    "ON `c2t`.`tagId` = `t`.`tagId` " \
    "WHERE `t`.`tagText` IN ("

#define CLUBSDB_SELECT_FOUND_ROWS "SELECT FOUND_ROWS()"
#define CLUBSDB_SQL_CALC_FOUND_ROWS "SQL_CALC_FOUND_ROWS"

#define CLUBSDB_COUNT_CLUBS \
    "SELECT COUNT(clubid) AS clubCount,"\
    "    SUM(memberCount) AS memberCount,"\
    "    domainid"\
    " FROM `clubs_clubs` GROUP BY domainid"

#define CLUBSDB_UPDATE_CLUB_MEMBERSHIP \
    "UPDATE " \
    "    `clubs_members` `cm`, " \
    "    `clubs_clubs` `cc` " \
    "SET " \
    "  `cm`.`membershipStatus` = ?, " \
    "  `cc`.`gmCount` = `cc`.`gmCount` + (?), " \
    "  `cc`.`lastActiveTime` = NOW() " \
    " WHERE " \
    "    `cm`.`blazeId` = ? " \
    "    AND `cm`.`clubId` = `cc`.`clubId` " \
    "    AND `cc`.`clubId` = ? " \
    "    AND `cc`.`gmCount` < ? "

#define CLUBDB_GET_CLUB_OWNER \
    "SELECT " \
    "   `blazeId` " \
    " FROM `clubs_members` `requestor`" \
    " WHERE " \
    "   `requestor`.`clubId` = $U AND " \
    "   `requestor`.`membershipStatus` = 2 "

#define CLUBDB_GET_CLUB_IDS_BY_NAME \
    "SELECT `cc`.`clubId` " \
    " FROM `clubs_clubs` `cc` " \
    " WHERE "

EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdSettingsUpdateId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdSelectClubsId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdRemoveClubId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertMemberId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdRemoveMemberId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdLogUserLastLeaveClubTimeId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertNewsId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdDeleteOldestNewsId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdPromoteClubGMId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdDemoteClubMemberId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertMessageId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateLastActiveId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMetadataId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMetadata_2_Id = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMetadataAndLastActiveId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMetadata_2_AndLastActiveId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdDeleteMessageId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateLastGameId = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdGetClubRecord = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateClubRecord = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateClubAward = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateClubAwardCount = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMemberMetaData = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateSeasonState = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdGetSeasonState = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertRival = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateRival = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdDeleteOldestRival = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdIncrementRivalCount = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdInsertTickerMsg = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdDeleteOldestTickerMsg = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateNewsItemFlags = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdUpdateMembership = 0;
EA_THREAD_LOCAL PreparedStatementId ClubsDatabase::mCmdGetClubMetadataId = 0;

ClubsDatabase::ClubsDatabase()
{
}

void ClubsDatabase::initialize(uint32_t dbId)
{
    mCmdInsertId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_insert", CLUBSDB_INSERT_CLUB);
    mCmdSettingsUpdateId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateSettings", CLUBSDB_UPDATE_CLUB_SETTINGS);
    mCmdSelectClubsId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_selectClubs", CLUBSDB_SELECT_CLUBS);
    mCmdRemoveClubId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_removeClub", CLUBSDB_REMOVE_CLUB);
    mCmdInsertMemberId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_members_insertMember", CLUBSDB_INSERT_MEMBER);
    mCmdRemoveMemberId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_members_removeMember", CLUBSDB_REMOVE_MEMBER);
    mCmdLogUserLastLeaveClubTimeId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_logUserLastLeaveClubTime", CLUBSDB_UPSERT_USER_LAST_LEAVE_TIME);
    mCmdInsertNewsId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_news_insertNews", CLUBSDB_INSERT_NEWS);
    mCmdDeleteOldestNewsId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_news_deleteOldestNews", CLUBSDB_DELETE_OLDEST_NEWS);
    mCmdPromoteClubGMId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_members_promoteClubGM", CLUBSDB_PROMOTE_CLUB_GM);
    mCmdDemoteClubMemberId = gDbScheduler->registerPreparedStatement(dbId,
        "clubs_members_demoteClubMember", CLUBSDB_DEMOTE_CLUB_MEMBER);
    mCmdInsertMessageId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_messages_insertMessage", CLUBSDB_INSERT_MESSAGE);
    mCmdUpdateLastActiveId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateLastActive", CLUBSDB_UPDATE_LAST_ACTIVE);
    mCmdUpdateMetadataId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateMetadata", CLUBSDB_UPDATE_METADATA);
    mCmdUpdateMetadata_2_Id = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateMetadata_2", CLUBSDB_UPDATE_METADATA_2);
    mCmdUpdateMetadataAndLastActiveId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateMetadata_updateLastActive", CLUBSDB_UPDATE_METADATA_LAST_ACTIVE);
    mCmdUpdateMetadata_2_AndLastActiveId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_updateMetadata_2_updateLastActive", CLUBSDB_UPDATE_METADATA_2_LAST_ACTIVE);
    mCmdDeleteMessageId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_messages_deleteMessage", CLUBSDB_DELETE_MESSAGE);
    mCmdUpdateLastGameId = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubs_update_last_game", CLUBSDB_UPDATE_CLUB_LAST_GAME);
    mCmdGetClubRecord = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubrecordbooks_get_club_record", CLUBSDB_GET_CLUB_RECORD);
    mCmdUpdateClubRecord = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubrecordbooks_upsert_club_record", CLUBSDB_UPSERT_CLUB_RECORD);
    mCmdUpdateClubAward = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubawards_upsert_club_award", CLUBSDB_UPSERT_CLUB_AWARD);
    mCmdUpdateClubAwardCount = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_clubawards_upsert_club_award_count", CLUBSDB_UPDATE_CLUB_AWARD_COUNT);
    mCmdUpdateMemberMetaData = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_members_update_metadata", CLUBSDB_UPDATE_MEMBER_METADATA);
    mCmdUpdateSeasonState = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_seasons_update_state", CLUBSDB_UPDATE_SEASON_STATE);
    mCmdGetSeasonState = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_seasons_get_state", CLUBSDB_GET_SEASON_STATE);
    mCmdInsertRival = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_rivals_insert_rival", CLUBSDB_INSERT_RIVALS);
    mCmdUpdateRival = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_rivals_insert_rival", CLUBSDB_UPDATE_RIVAL);
    mCmdDeleteOldestRival = gDbScheduler->registerPreparedStatement(dbId,
            "clubs_rivals_delete_oldset", CLUBSDB_DELETE_OLDEST_RIVAL);
    mCmdIncrementRivalCount = gDbScheduler->registerPreparedStatement(dbId,
            "club_rivals_increment_rival_count", CLUBSDB_INCREMENT_RIVAL_COUNT);
    mCmdInsertTickerMsg =  gDbScheduler->registerPreparedStatement(dbId,
            "club_tickermsgs_insert", CLUBSDB_INSERT_TICKERMSG);
    mCmdDeleteOldestTickerMsg =  gDbScheduler->registerPreparedStatement(dbId,
            "club_tickermsgs_delete_oldest", CLUBSDB_DELETE_OLDEST_TICKERMSG);
    mCmdUpdateNewsItemFlags = gDbScheduler->registerPreparedStatement(dbId,
            "club_update_news_item_flags", CLUBSDB_UPDATE_NEWS_ITEM_FLAGS);
    mCmdUpdateMembership = gDbScheduler->registerPreparedStatement(dbId,
            "club_update_club_membership", CLUBSDB_UPDATE_CLUB_MEMBERSHIP);
    mCmdGetClubMetadataId = gDbScheduler->registerPreparedStatement(dbId,
        "clubs_get_club_metadata", CLUBSDB_GET_CLUB_METADATA);

}


ClubsDatabase::~ClubsDatabase()
{
}

void ClubsDatabase::clubSettingsToStmt(uint32_t &col, PreparedStatement* stmt, const ClubSettings *clubSettings) const
{
    // teamId
    stmt->setUInt32(col++, clubSettings->getTeamId());
    // language
    stmt->setString(col++, clubSettings->getLanguage());
    // nonuniquename
    stmt->setString(col++, clubSettings->getNonUniqueName());
    // description
    stmt->setString(col++, clubSettings->getDescription());
    // region
    stmt->setUInt32(col++, clubSettings->getRegion());
    // logoId
    stmt->setUInt32(col++, clubSettings->getLogoId());
    // bannerId
    stmt->setUInt32(col++, clubSettings->getBannerId());
    // acceptanceFlag
    stmt->setUInt32(col++, clubSettings->getAcceptanceFlags().getBits());
    // artPackType
    stmt->setUInt32(col++, clubSettings->getArtPackageType());
    // password
    stmt->setString(col++, clubSettings->getPassword());
    // seasonLevel
    stmt->setUInt32(col++, clubSettings->getSeasonLevel());
    // previousSeasonLevel
    stmt->setUInt32(col++, clubSettings->getPreviousSeasonLevel());
    // lastSeasonLevelUpdate
    stmt->setUInt32(col++, static_cast<uint32_t>(clubSettings->getLastSeasonLevelUpdate()));
    // metadata
    if (clubSettings->getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
    {
        stmt->setBinary(col++, reinterpret_cast<const uint8_t*>(clubSettings->getMetaDataUnion().getMetadataString()), strlen(clubSettings->getMetaDataUnion().getMetadataString()) + 1);
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_STRING);
    }
    else if (clubSettings->getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_METADATABLOB)
    {
        stmt->setBinary(col++, clubSettings->getMetaDataUnion().getMetadataBlob()->getData(), clubSettings->getMetaDataUnion().getMetadataBlob()->getSize());
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_BINARY);
    }
    else
    {
        // Backwards compatibility: use deprecated MetaData string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion)
        stmt->setBinary(col++, reinterpret_cast<const uint8_t*>(clubSettings->getMetaData()), strlen(clubSettings->getMetaData()) + 1);
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_STRING);
    }
    // metadata2
    if (clubSettings->getMetaData2Union().getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
    {
        stmt->setBinary(col++, reinterpret_cast<const uint8_t*>(clubSettings->getMetaData2Union().getMetadataString()), strlen(clubSettings->getMetaData2Union().getMetadataString()) + 1);
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_STRING);
    }
    else if (clubSettings->getMetaData2Union().getActiveMember() == MetadataUnion::MEMBER_METADATABLOB)
    {
        stmt->setBinary(col++, clubSettings->getMetaData2Union().getMetadataBlob()->getData(), clubSettings->getMetaData2Union().getMetadataBlob()->getSize());
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_BINARY);
    }
    else
    {
        // Backwards compatibility: use deprecated MetaData2 string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion)
        stmt->setBinary(col++, reinterpret_cast<const uint8_t*>(clubSettings->getMetaData2()), strlen(clubSettings->getMetaData2()) + 1);
        stmt->setUInt32(col++, CLUBS_METADATA_TYPE_STRING);
    }
    // joinAcceptance
    stmt->setUInt32(col++, clubSettings->getJoinAcceptance());
    // skipPasswordCheckAcceptance
    stmt->setUInt32(col++, clubSettings->getSkipPasswordCheckAcceptance());
    // petitionAcceptance
    stmt->setUInt32(col++, clubSettings->getPetitionAcceptance());
    // custOpt1
    stmt->setUInt32(col++, clubSettings->getCustClubSettings().getCustOpt1());
    // custOpt2
    stmt->setUInt32(col++, clubSettings->getCustClubSettings().getCustOpt2());
    // custOpt3
    stmt->setUInt32(col++, clubSettings->getCustClubSettings().getCustOpt3());
    // custOpt4
    stmt->setUInt32(col++, clubSettings->getCustClubSettings().getCustOpt4());
    // custOpt5
    stmt->setUInt32(col++, clubSettings->getCustClubSettings().getCustOpt5());
	// lastUpdatedBy
	stmt->setInt64(col++, clubSettings->getLastUpdatedBy());
    // isNameProfane
    stmt->setUInt32(col++, clubSettings->getIsNameProfane());
}


void ClubsDatabase::clubDBRowToClubSettings(uint32_t &col, const DbRow* dbrow, ClubSettings &clubSettings, bool skipMetadata) const
{
    // teamId
    clubSettings.setTeamId(static_cast<TeamId>(dbrow->getUInt(col++)));
    // language
    clubSettings.setLanguage(dbrow->getString(col++));
	// name
	clubSettings.setClubName(dbrow->getString("Name"));
    // nonuniquename
    clubSettings.setNonUniqueName(dbrow->getString(col++));
    // description
    clubSettings.setDescription(dbrow->getString(col++));
    // region
    clubSettings.setRegion(static_cast<ClubRegion>(dbrow->getUInt(col++)));
    // logoId
    clubSettings.setLogoId(static_cast<LogoId>(dbrow->getUInt(col++)));
    // bannerId
    clubSettings.setBannerId(static_cast<BannerId>(dbrow->getUInt(col++)));
    // acceptanceFlag
    clubSettings.getAcceptanceFlags().setBits(dbrow->getUInt(col++));
    // artPackType
    clubSettings.setArtPackageType(static_cast<ArtPackageType>(dbrow->getUInt(col++)));
    // password
    clubSettings.setPassword(dbrow->getString(col++));
    // seasonLevel
    clubSettings.setSeasonLevel(static_cast<SeasonLevel>(dbrow->getUInt(col++)));
    // previousSeasonLevel
    clubSettings.setPreviousSeasonLevel(static_cast<SeasonLevel>(dbrow->getUInt(col++)));
    // lastSeasonLevelUpdate
    clubSettings.setLastSeasonLevelUpdate(dbrow->getInt(col++));
    // metadata
    if (skipMetadata)
    {
        col++;
        col++;
    }
    else
    {
        size_t len;
        const uint8_t* data = dbrow->getBinary(col++, &len);
        MetaDataType type = static_cast<MetaDataType>(dbrow->getUInt(col++));
        if (type == CLUBS_METADATA_TYPE_STRING)
        {
            clubSettings.getMetaDataUnion().setMetadataString(reinterpret_cast<const char8_t*>(data));
            // Backwards compatibility: set deprecated MetaData string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion).
            clubSettings.setMetaData(reinterpret_cast<const char8_t*>(data));
        }
        else if (type == CLUBS_METADATA_TYPE_BINARY)
        {
            MetadataBlob metadata;
            metadata.setData(data, len);
            clubSettings.getMetaDataUnion().setMetadataBlob(&metadata);
        }
    }
    // metadata
    if (skipMetadata)
    {
        col++;
        col++;
    }
    else
    {
        size_t len;
        const uint8_t* data = dbrow->getBinary(col++, &len);
        MetaDataType type = static_cast<MetaDataType>(dbrow->getUInt(col++));
        if (type == CLUBS_METADATA_TYPE_STRING)
        {
            clubSettings.getMetaData2Union().setMetadataString(reinterpret_cast<const char8_t*>(data));
            // Backwards compatibility: set deprecated MetaData2 string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion).
            clubSettings.setMetaData2(reinterpret_cast<const char8_t*>(data));
        }
        else if (type == CLUBS_METADATA_TYPE_BINARY)
        {
            MetadataBlob metadata;
            metadata.setData(data, len);
            clubSettings.getMetaData2Union().setMetadataBlob(&metadata);
        }
    }
    // joinAcceptance
    clubSettings.setJoinAcceptance(static_cast<RequestorAcceptance>(dbrow->getUInt(col++)));
    // skipPasswordCheckAcceptance
    clubSettings.setSkipPasswordCheckAcceptance(static_cast<RequestorAcceptance>(dbrow->getUInt(col++)));
    // petitionAcceptance
    clubSettings.setPetitionAcceptance(static_cast<RequestorAcceptance>(dbrow->getUInt(col++)));
    // custOpt1
    clubSettings.getCustClubSettings().setCustOpt1(dbrow->getUInt(col++));
    // custOpt2
    clubSettings.getCustClubSettings().setCustOpt2(dbrow->getUInt(col++));
    // custOpt3
    clubSettings.getCustClubSettings().setCustOpt3(dbrow->getUInt(col++));
    // custOpt4
    clubSettings.getCustClubSettings().setCustOpt4(dbrow->getUInt(col++));
    // custOpt5
    clubSettings.getCustClubSettings().setCustOpt5(dbrow->getUInt(col++));
	// lastUpdatedBy
	clubSettings.setLastUpdatedBy(dbrow->getInt64(col++));
    // isNameProfane
	clubSettings.setIsNameProfane((dbrow->getInt("IsNameProfane") > 0) ? true : false);
    //clubSettings.setIsNameProfane(dbrow->getUInt(col++));
}

void ClubsDatabase::clubDBRowToClub(uint32_t &col, const DbRow* dbrow, Club *club) const
{
    // We only take out simple type vars that belong to
    // club here. ClubSettings and ClubInfo
    // have their own db-row-to-correspondig-struct method.

    // clubId
    club->setClubId(dbrow->getUInt64(col++));
    // domainId
    club->setClubDomainId(dbrow->getUInt(col++));
    // name
    club->setName(dbrow->getString(col++));
    
}

void ClubsDatabase::clubDBRowToClubInfo(uint32_t &col, const DbRow* dbrow, ClubInfo &clubInfo) const
{
    // creationTime
    clubInfo.setCreationTime(dbrow->getUInt(col++));
    // lastActiveTime
    clubInfo.setLastActiveTime(dbrow->getUInt(col++));
    // memberCount
    clubInfo.setMemberCount(dbrow->getUInt(col++));
    // gmCount
    clubInfo.setGmCount(dbrow->getUInt(col++));
    // awardCount
    clubInfo.setAwardCount(dbrow->getUInt(col++));
    // lastOppo
    clubInfo.setLastOppo(dbrow->getUInt64(col++));
    // lastResult
    clubInfo.setLastGameResult(dbrow->getString(col++));
    // lastGameTime
    clubInfo.setLastGameTime(dbrow->getUInt(col++));
    // rivalCount
    clubInfo.setRivalCount(dbrow->getUInt(col++));
}

void ClubsDatabase::clubDbRowToClubMessage(const DbRow* dbrow, ClubMessage *clubMessage) const
{
    clubMessage->setMessageId(dbrow->getInt("messageId"));
    clubMessage->setClubId(dbrow->getUInt64("clubId"));
    clubMessage->setMessageType(static_cast<MessageType>(dbrow->getInt("messageType")));
    clubMessage->getSender().setBlazeId(dbrow->getInt64("sender"));
    clubMessage->getReceiver().setBlazeId(dbrow->getInt64("receiver"));
    clubMessage->setTimeSent(dbrow->getInt("timestamp"));
    clubMessage->setClubName(dbrow->getString("name"));
}

void ClubsDatabase::clubDbRowToClubMembership(const DbRow* dbrow, ClubMembership *clubMembership) const
{
    clubMembership->setClubId(dbrow->getUInt64("clubId"));
    clubMembership->setClubDomainId(dbrow->getUInt("domainId"));
    clubMembership->setClubName(dbrow->getString("name"));
    clubMembership->getClubMember().getUser().setBlazeId(dbrow->getInt64("blazeId"));
    clubMembership->getClubMember().setMembershipSinceTime(static_cast<uint32_t>(dbrow->getInt("memberSince")));
    clubMembership->getClubMember().setMembershipStatus(static_cast<MembershipStatus>(dbrow->getInt("membershipStatus")));
    clubMembership->getClubMember().setOnlineStatus(CLUBS_MEMBER_OFFLINE);

    char8_t* pMetadata = BLAZE_NEW_ARRAY(char8_t, OC_MAX_MEMBER_METADATA_SIZE);
    blaze_strnzcpy(pMetadata, dbrow->getString("metadata"), OC_MAX_MEMBER_METADATA_SIZE);
    encodeMemberMetaData(clubMembership->getClubMember().getMetaData(), pMetadata);
    delete[] pMetadata;
}

void ClubsDatabase::clubDbRowToClubMetadata(const DbRow* dbrow, ClubMetadata *clubmetadata) const
{
	clubmetadata->setBlazeId(dbrow->getInt64("blazeId"));
	clubmetadata->setClubId(dbrow->getUInt64("clubId"));

	size_t len;
	const uint8_t* data = dbrow->getBinary("metaData", &len);
	MetaDataType type = static_cast<MetaDataType>(dbrow->getUInt("metaDataType"));
	if (type == CLUBS_METADATA_TYPE_STRING)
	{
		clubmetadata->getMetaDataUnion().setMetadataString(reinterpret_cast<const char8_t*>(data));
	}
	else if (type == CLUBS_METADATA_TYPE_BINARY)
	{
		MetadataBlob metadata;
		metadata.setData(data, len);
		clubmetadata->getMetaDataUnion().setMetadataBlob(&metadata);
	}
}

void ClubsDatabase::clubDbRowToClubMetadataWithouBlazeId(const DbRow* dbrow, ClubMetadata *clubmetadata) const
{    
    clubmetadata->setClubId(dbrow->getUInt64("clubId"));

    size_t len;
    const uint8_t* data = dbrow->getBinary("metaData", &len);
    MetaDataType type = static_cast<MetaDataType>(dbrow->getUInt("metaDataType"));
    if (type == CLUBS_METADATA_TYPE_STRING)
    {
        clubmetadata->getMetaDataUnion().setMetadataString(reinterpret_cast<const char8_t*>(data));
    }
    else if (type == CLUBS_METADATA_TYPE_BINARY)
    {
        MetadataBlob metadata;
        metadata.setData(data, len);
        clubmetadata->getMetaDataUnion().setMetadataBlob(&metadata);
    }
}

void ClubsDatabase::encodeMemberMetaData(MemberMetaData &metadata, char* strMetadata) const
{
    char8_t* saveptr;
    char8_t* nextValue;
    char8_t* nextKey = blaze_strtok(strMetadata, "=", &saveptr);
    while (nextKey != nullptr)
    {
        nextValue = blaze_strtok(nullptr, ",", &saveptr);
        metadata.insert(eastl::make_pair(nextKey, nextValue));
        nextKey = blaze_strtok(nullptr, "=", &saveptr);
    }
}

void ClubsDatabase::decodeMemberMetaData(const MemberMetaData &metadata, 
                                         char* strMetadata, size_t strMetadataLen) const
{
    strMetadata[0]='\0';

    Collections::AttributeMap::const_iterator i = metadata.begin();
    Collections::AttributeMap::const_iterator end = metadata.end();
    size_t pos = 0;
    for(; i != end && pos < strMetadataLen; i++)
    {
        if (pos == 0)
        {
            pos += blaze_snzprintf(strMetadata, strMetadataLen, "%s=%s", 
                i->first.c_str(), i->second.c_str());
        }
        else
        {
            strMetadata[pos++] = ',';
            strMetadata[pos] = 0;
            pos = blaze_strnzcat(strMetadata, i->first.c_str(), strMetadataLen);
            strMetadata[pos++] = '=';
            strMetadata[pos] = 0;
            pos = blaze_strnzcat(strMetadata, i->second.c_str(), strMetadataLen);
        }
    }
}



BlazeRpcError ClubsDatabase::insertClub(ClubDomainId domainId, const char8_t* name, const ClubSettings &clubSettings, ClubId &clubId)
{
    TRACE_LOG("[ClubsDatabase::insertClub] Inserting club: " << name);
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertId);
        
        if (stmt != nullptr)
        {
            uint32_t col = 0;

            // domain id
            stmt->setUInt32(col++, domainId);

            // name
            stmt->setString(col++, name);
            
            clubSettingsToStmt(col, stmt, &clubSettings);
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->getLastInsertId() > 0)
                {
                    clubId = dbresult->getLastInsertId();
                    result = ERR_OK;
                }
            }
            else
            {
                // let's see if the club name is already in use
                if (error == DB_ERR_DUP_ENTRY)
                    return static_cast<BlazeRpcError>(CLUBS_ERR_CLUB_NAME_IN_USE);
            }

            
        }
    }

    return result;
}


BlazeRpcError ClubsDatabase::updateClubSettings(ClubId clubId, const ClubSettings &clubSettings, uint64_t& version)
{
    TRACE_LOG("[ClubsDatabase::updateClubSettings] Updating club settings for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        DbResultPtrs dbResults;
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        query->append("UPDATE `clubs_clubs` SET" \
            "    `version` = `version` + 1" \
            "  , `teamId` = $u, `name` = '$s', `language` = '$s', `nonuniquename` = '$s'" \
            "  , `description` = '$s', `region` = $u, `logoId`= $u" \
            "  , `bannerId` = $u, `acceptanceFlags` = $u, `artPackType` = $u, `password` = '$s'" \
            "  , `seasonLevel` = $u, `previousSeasonLevel` = $u" \
            "  , `lastSeasonLevelUpdate` = $u, ",
                clubSettings.getTeamId(), clubSettings.getClubName(), clubSettings.getLanguage(), clubSettings.getNonUniqueName(),
                clubSettings.getDescription(), clubSettings.getRegion(), clubSettings.getLogoId(), clubSettings.getBannerId(),
                clubSettings.getAcceptanceFlags().getBits(), clubSettings.getArtPackageType(), clubSettings.getPassword(),
                clubSettings.getSeasonLevel(), clubSettings.getPreviousSeasonLevel(), clubSettings.getLastSeasonLevelUpdate());
        
        if (clubSettings.getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
        {
            query->append("`metaData` = '$s', `metaDataType` = $u, ", clubSettings.getMetaDataUnion().getMetadataString(), CLUBS_METADATA_TYPE_STRING);
        }
        else if (clubSettings.getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_METADATABLOB)
        {
            query->append("`metaData` = '$b', `metaDataType` = $u, ", clubSettings.getMetaDataUnion().getMetadataBlob(), CLUBS_METADATA_TYPE_BINARY);
        }
        else
        {
            // Backwards compatibility: use deprecated MetaData string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion)
            query->append("`metaData` = '$s', `metaDataType` = $u, ", clubSettings.getMetaData(), CLUBS_METADATA_TYPE_STRING);
        }
        if (clubSettings.getMetaData2Union().getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
        {
            query->append("`metaData2` = '$s', `metaDataType2` = $u, ", clubSettings.getMetaData2Union().getMetadataString(), CLUBS_METADATA_TYPE_STRING);
        }
        else if (clubSettings.getMetaData2Union().getActiveMember() == MetadataUnion::MEMBER_METADATABLOB)
        {
            query->append("`metaData2` = '$b', `metaDataType2` = $u, ", clubSettings.getMetaData2Union().getMetadataBlob(), CLUBS_METADATA_TYPE_BINARY);
        }
        else
        {
            // Backwards compatibility: use deprecated MetaData2 string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion)
            query->append("`metaData2` = '$s', `metaDataType2` = $u, ", clubSettings.getMetaData2(), CLUBS_METADATA_TYPE_STRING);
        }
        
        query->append("`joinAcceptance` = $u, `skipPasswordCheckAcceptance` = $u, `petitionAcceptance` = $u" \
            "  , `custOpt1` = $u, `custOpt2` = $u, `custOpt3` = $u, `custOpt4` = $u, `custOpt5` = $u, `lastUpdatedBy` = $U" \
            " WHERE `clubId` = $U;",
            clubSettings.getJoinAcceptance(), clubSettings.getSkipPasswordCheckAcceptance(), clubSettings.getPetitionAcceptance(),
            clubSettings.getCustClubSettings().getCustOpt1(), clubSettings.getCustClubSettings().getCustOpt2(),
            clubSettings.getCustClubSettings().getCustOpt3(), clubSettings.getCustClubSettings().getCustOpt4(),
            clubSettings.getCustClubSettings().getCustOpt5(), clubSettings.getLastUpdatedBy(), clubId);
        query->append("SELECT `version` FROM `clubs_clubs` WHERE `clubId` = $U;", clubId);

        BlazeRpcError error = mDbConn->executeMultiQuery(query, dbResults);
        if (error == DB_ERR_OK)
        {
            if (dbResults.size() != 2)
            {
                ERR_LOG("[ClubsDatabase].updateClubSettings: Incorrect number of results returned from multi query");
                return Blaze::ERR_SYSTEM;
            }

            DbResultPtr dbResult = dbResults[1];
            if (dbResult->size() != 1)
            {
                ERR_LOG("[ClubsDatabase].updateClubSettings: Incorrect number of rows returned for result");
                return Blaze::ERR_SYSTEM;
            }

            DbRow *row = *dbResult->begin();
            version = row->getUInt64(uint32_t(0));
            result = ERR_OK;
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClub(ClubId clubId, Club *club, uint64_t& version)
{
    TRACE_LOG("[ClubsDatabase::getClub] Fetching club data for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    if (clubId == INVALID_CLUB_ID)
    {
        result = static_cast<BlazeRpcError>(CLUBS_ERR_INVALID_CLUB_ID);
        return result;
    }

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append(CLUBSDB_SELECT_CLUB, clubId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->size() == 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_INVALID_CLUB_ID);
                }
                else
                {
                    uint32_t col = 0;
                    const DbRow *dbrow = *dbRes->begin();
                    clubDBRowToClub(col, dbrow, club);
                    clubDBRowToClubInfo(col, dbrow, club->getClubInfo());
                    clubDBRowToClubSettings(col, dbrow, club->getClubSettings());
                    version = dbrow->getUInt64(col++);
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMembers(ClubId clubId, uint32_t maxResultCount, uint32_t offset, ClubMemberList &clubMemberList, MemberOrder orderType, OrderMode orderMode, MemberTypeFilter memberType, const char8_t* personNamePattern, uint32_t *totalCount)
{
    TRACE_LOG("[ClubsDatabase].getClubMembers: Fetching club members for club [id]: " << clubId);

    // updated below on success:
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    if (totalCount != nullptr)
    {
        *totalCount = 0;
    }
    
    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        bool isCalcFoundRows = (maxResultCount < UINT32_MAX || offset > 0) && (totalCount != nullptr);
        
        if (query != nullptr)
        {
            if (personNamePattern == nullptr)
            {
                query->append(CLUBSDB_SELECT_CLUB_MEMBERS, isCalcFoundRows ? CLUBSDB_SQL_CALC_FOUND_ROWS : "", clubId);

                if (memberType != ALL_MEMBERS)
                {
                    query->append("$s $u", (memberType != GM_MEMBERS ? " AND `cm`.`membershipStatus` < " : " AND `cm`.`membershipStatus` >= "), CLUBS_GM);
                }

                if (orderType != MEMBER_NO_ORDER)
                {
                    query->append(" ORDER BY `cm`.`memberSince` $s ", (orderMode == DESC_ORDER) ? CLUBSDB_SORT_DESC : CLUBSDB_SORT_ASC);
                }

                if (maxResultCount > 0 && (maxResultCount < UINT32_MAX || offset > 0))
                {
                    query->append(" LIMIT $u ", maxResultCount);
                    if (offset > 0)
                        query->append(" OFFSET $u ", offset);
                }
            }
            else
            {
                uint32_t dbId = DbScheduler::INVALID_DB_ID;
                if (!gController->isSharedCluster())
                {
                    dbId = gUserSessionManager->getDbId();
                }
                else
                {
                    if (gCurrentUserSession != nullptr)
                    {
                        ClientPlatformType platform = gCurrentUserSession->getPlatformInfo().getClientPlatform();
                        dbId = gUserSessionManager->getDbId(platform);
                    }
                    else
                    {
                        ERR_LOG("[ClubsDatabase].getClubMembers: Searching for club members by persona name requires a current user session on shared cluster servers");
                        return DB_ERR_SYSTEM;
                    }
                }

                DbConnPtr userInfoConn = gDbScheduler->getReadConnPtr(dbId);
                if (userInfoConn != nullptr)
                {
                    QueryPtr userInfoQuery = DB_NEW_QUERY_PTR(userInfoConn);
                    userInfoQuery->append(CLUBSDB_SELECT_BLAZEIDS_FOR_PERSONANAME, personNamePattern);

                    if (orderType != MEMBER_NO_ORDER)
                    {
                        userInfoQuery->append(" ORDER BY `userinfo`.`persona` $s ", (orderMode == DESC_ORDER) ? CLUBSDB_SORT_DESC : CLUBSDB_SORT_ASC);
                    }

                    DbResultPtr userInfoRes;
                    BlazeRpcError error = userInfoConn->executeQuery(userInfoQuery, userInfoRes);
                    if (error == DB_ERR_OK)
                    {
                        if (!userInfoRes->empty())
                        {
                            query->append(CLUBSDB_SELECT_CLUB_MEMBERS_BY_BLAZEID, clubId);
                            for (DbResult::const_iterator it = userInfoRes->begin(), end = userInfoRes->end(); it != end; it++)
                            {
                                DbRow *dbRow = *it;

                                query->append("$U,", dbRow->getUInt64("blazeId"));
                            }
                            query->trim(1);
                            query->append(")");
                            if (maxResultCount > 0 && (maxResultCount < UINT32_MAX || offset > 0))
                            {
                                query->append(" LIMIT $u ", maxResultCount);
                                if (offset > 0)
                                    query->append(" OFFSET $u ", offset);
                            }
                        }
                    }
                    else
                    {
                        ERR_LOG("[ClubsDatabase].getClubMembers: Failed to execute query with error " << getDbErrorString(error));
                        return error;
                    }
                }
                else
                {
                    ERR_LOG("[ClubsDatabase].getClubMembers: Failed to obtain connection to userinfo table");
                    return DB_ERR_SYSTEM;
                }
            }

            if (!query->isEmpty())
            {
                DbResultPtr dbRes;
                BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

                if (error == DB_ERR_OK)
                {
                    const uint32_t resSize = dbRes->size();

                    if (dbRes->empty())
                    {
                        result = maxResultCount > 0 && offset == 0 && memberType == ALL_MEMBERS && personNamePattern == nullptr ? CLUBS_ERR_INVALID_CLUB_ID : ERR_OK;
                    }
                    else
                    {
                        // reserve the size with 1 allocation, note, it destroys all data already in EA::TDF::TdfStructVector...
                        clubMemberList.reserve(dbRes->size());
                        for (DbResult::const_iterator it = dbRes->begin(), end = dbRes->end(); it != end; it++)
                        {
                            DbRow *dbRow = *it;

                            // fill club member struct
                            ClubMember *cm = clubMemberList.pull_back();
                            cm->getUser().setBlazeId(dbRow->getInt64("blazeId"));
                            cm->setMembershipSinceTime(static_cast<uint32_t>(dbRow->getInt("memberSince")));
                            cm->setMembershipStatus(static_cast<MembershipStatus>(dbRow->getInt("membershipStatus")));
                            // set default online status to offline
                            cm->setOnlineStatus(CLUBS_MEMBER_OFFLINE);
                            char8_t* pMetadata = BLAZE_NEW_ARRAY(char8_t, OC_MAX_MEMBER_METADATA_SIZE);
                            blaze_strnzcpy(pMetadata, dbRow->getString("metadata"), OC_MAX_MEMBER_METADATA_SIZE);
                            encodeMemberMetaData(cm->getMetaData(), pMetadata);
                            delete[] pMetadata;

                        }
                        result = ERR_OK;
                    }

                    if (result == ERR_OK)
                    {
                        if (isCalcFoundRows)
                        {
                            DB_QUERY_RESET(query);
                            query->append(CLUBSDB_SELECT_FOUND_ROWS);
                            result = Blaze::ERR_SYSTEM;

                            error = mDbConn->executeQuery(query, dbRes);

                            if (error == DB_ERR_OK)
                            {
                                if (dbRes->size() > 0)
                                {
                                    DbResult::const_iterator it = dbRes->begin();
                                    DbRow *dbRow = *it;
                                    *totalCount = dbRow->getUInt((uint32_t)0);
                                    result = ERR_OK;
                                }
                            }
                        }
                        else if (totalCount != nullptr)
                        {
                            *totalCount = resSize;
                        }
                    }
                }
            }
            else
            {
                result = maxResultCount > 0 && offset == 0 && memberType == ALL_MEMBERS && personNamePattern == nullptr ? CLUBS_ERR_INVALID_CLUB_ID : ERR_OK;
            }
        }
    }    
    
    return result;
}

BlazeRpcError ClubsDatabase::getClubNameResetCount(ClubId clubId, int32_t& resetCount)
{
	TRACE_LOG("[ClubsDatabase::getClubNameResetCount] for [clubId=" << clubId << "]:");

	BlazeRpcError result = Blaze::ERR_SYSTEM;

	if (mDbConn != nullptr)
	{
		QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

		if (query != nullptr)
		{
			query->append(CLUBSDB_GET_NAME_RESET_COUNT, clubId);

			DbResultPtr dbresult;
			BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
			if (error == DB_ERR_OK)
			{
				if (!dbresult->empty())
				{
					DbResult::const_iterator it = dbresult->begin();
					const DbRow *dbrow = *it;

					resetCount = dbrow->getInt("clubNameResetCount");
					result = ERR_OK;
				}
			}
		}
	}

	return result;
}

BlazeRpcError ClubsDatabase::insertMember(ClubId clubId, const ClubMember &clubMember, const int32_t maxMemberCount)
{
    TRACE_LOG("[ClubsDatabase::insertMember] Inserting club member [blazeid]: " << clubMember.getUser().getBlazeId() << " into club [id]: " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        DbResultPtr dbRes;
        BlazeRpcError error;            
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append("UPDATE clubs_clubs SET memberCount = memberCount + 1, lastActiveTime = NOW() ");
            if (clubMember.getMembershipStatus() >= CLUBS_GM)
            {
                query->append(", gmCount = gmCount + 1 ");
            }            
            query->append("WHERE clubId = $U ", clubId);
            query->append("AND memberCount < $u ", maxMemberCount);
                        
            error = mDbConn->executeQuery(query, dbRes);
        
            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() <= 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_CLUB_FULL);
                }
                else
                {
                    result = Blaze::ERR_OK;
                }
            }
            else
            {
                result = Blaze::ERR_SYSTEM;
            }
        }
        else
        {
            result = Blaze::ERR_SYSTEM;
        }
        
        if (result == Blaze::ERR_OK)
        {

            PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertMemberId);
            
            if (stmt != nullptr)
            {
                uint32_t col = 0;
            
                // clubId
                stmt->setInt64(col++, clubMember.getUser().getBlazeId());
                stmt->setUInt64(col++, clubId);
                stmt->setInt32(col++, clubMember.getMembershipStatus());
                
                error = mDbConn->executePreparedStatement(stmt, dbRes);
                if (error == DB_ERR_OK)
                {
                    result = ERR_OK;
                }
                else if (error == DB_ERR_DUP_ENTRY)
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_ALREADY_MEMBER);
            }
        }            
    }    
    
    return result;
}

BlazeRpcError ClubsDatabase::removeMember(ClubId clubId, BlazeId blazeId, MembershipStatus clubMembershipStatus)
{
    TRACE_LOG("[ClubsDatabase::removeMember] Remove member [blazeid]: " << blazeId << " from club [id]: " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        DbResultPtr dbRes;
        BlazeRpcError error;
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdRemoveMemberId);
        
        if (stmt != nullptr)
        {
            uint32_t col = 0;
        
            // clubId
            stmt->setUInt64(col++, clubId);
            // blazeId
            stmt->setInt64(col++, blazeId);
            
            error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                uint32_t affectedRows = dbRes->getAffectedRowCount();
                
                if ( affectedRows == 0)
                    return Blaze::CLUBS_ERR_USER_NOT_MEMBER;
            }
            else
            {
                return Blaze::ERR_SYSTEM;
            }
            
        }

        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append("UPDATE clubs_clubs SET memberCount = memberCount - 1, lastActiveTime = NOW() ");
            if (clubMembershipStatus >= CLUBS_GM)
            {
                query->append(", gmCount = gmCount - 1 ");
            }
            query->append("WHERE clubId = $U ", clubId);
                        
            error = mDbConn->executeQuery(query, dbRes);
        
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }

    }    
    return result;
}

BlazeRpcError ClubsDatabase::logUserLastLeaveClubTime(BlazeId blazeId, ClubDomainId domainId)
{
    TRACE_LOG("[ClubsDatabase::logUserLastLeaveClubTime] Log user [blazeid]: " << blazeId << " leave time in domain [domainid]: " << domainId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {            
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdLogUserLastLeaveClubTimeId);
        if (stmt != nullptr)
        {
            uint32_t col = 0;
            
            //blazeId
            stmt->setInt64(col++, blazeId);
            //domainId
            stmt->setInt32(col++, domainId);
            
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }
    
    return result;
}

BlazeRpcError ClubsDatabase::getUserLastLeaveClubTime(BlazeId blazeId, ClubDomainId domainId, int64_t& lastLeaveTime)
{
    TRACE_LOG("[ClubsDatabase::getUserLastLeaveClubTime] Get user [blazeid]: " << blazeId << " last leave club time in domain [domainId]: " <<  domainId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;
    lastLeaveTime = 0;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_USER_LAST_LEAVE_TIME, blazeId, domainId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
                if (!dbRes->empty())
                {
                    DbRow *row = *dbRes->begin();
                    uint32_t col = 0;
                    lastLeaveTime = row->getInt64(col++);
                }
            }
        }     
    }
    return result;
}

BlazeRpcError ClubsDatabase::getClubs(const ClubIdList &clubIdList, ClubList &clubList, uint64_t*& versions, const uint32_t maxResultCount,
                                      const uint32_t offset, uint32_t* totalCount, const ClubsOrder clubsOrder, const OrderMode orderMode)
{
    TRACE_LOG("[ClubsDatabase::getClubs]ClubId list size: " << clubIdList.size());

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            // need to calc found rows?
            bool isCalcFoundRows = (maxResultCount < UINT32_MAX || offset > 0) && (totalCount != nullptr);

            query->append(CLUBSDB_SELECT_CLUBS,
                (isCalcFoundRows) ? CLUBSDB_SQL_CALC_FOUND_ROWS : "");
            
            ClubIdList::const_iterator it = clubIdList.begin();
            while (it != clubIdList.end())
            {
                query->append("$U", *it++);
                if (it == clubIdList.end())
                    query->append(")");
                else
                    query->append(",");
            }
            const char* orderModeString = (orderMode == DESC_ORDER) ? CLUBSDB_SORT_DESC : CLUBSDB_SORT_ASC ;
            switch (clubsOrder)
            {
            case CLUBS_NO_ORDER:
                break;
            case CLUBS_ORDER_BY_MEMBERCOUNT:
                query->append(" ORDER BY `memberCount` $s ", orderModeString); break;
            default:
                break;
            }
            // limit query results
            if (maxResultCount > 0 && (maxResultCount < UINT32_MAX || offset > 0))
            {
                query->append(" LIMIT $u ", maxResultCount);
                if (offset > 0)
                    query->append(" OFFSET $u ", offset);
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                uint32_t resultSize= dbresult->size();

                if (dbresult->empty())
                {
                    result = maxResultCount > 0 && offset == 0 ? CLUBS_ERR_INVALID_CLUB_ID : ERR_OK;
                }
                else
                {
                    DbResult::const_iterator itr = dbresult->begin();
                    versions = BLAZE_NEW_ARRAY(uint64_t, dbresult->size());
                    uint32_t versionCount = 0;
                    for (itr = dbresult->begin(); itr != dbresult->end(); itr++)
                    {
                        const DbRow *dbrow = *itr;
                        
                        uint32_t col = 0;
                        Club *club = clubList.pull_back();
                        
                        clubDBRowToClub(col, dbrow, club);
                        clubDBRowToClubInfo(col, dbrow, club->getClubInfo());
                        clubDBRowToClubSettings(col, dbrow, club->getClubSettings());
                        versions[versionCount++] = dbrow->getUInt64(col++);
                    }

                    result = ERR_OK;
                 }

                if (result == ERR_OK)
                {
                    if (isCalcFoundRows)
                    {
                        DB_QUERY_RESET(query);
                        query->append(CLUBSDB_SELECT_FOUND_ROWS);
                        result = Blaze::ERR_SYSTEM;
                        error = mDbConn->executeQuery(query, dbresult);
                        if (error == DB_ERR_OK)
                        {
                            if (dbresult->size() > 0)
                            {
                                DbRow *row = *dbresult->begin();
                                uint32_t col = 0;
                                *totalCount = (uint32_t)row->getUInt(col);
                                result = ERR_OK;
                            }
                        }
                    }
                    else if (totalCount != nullptr)
                    {
                        *totalCount = resultSize;
                    }
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubIdsByName(const char8_t* clubName, const char8_t* clubNonUniqueName, ClubIdList& clubIds, uint32_t& rowsFound)
{
    TRACE_LOG("[ClubsDatabase::getMembershipsInDomain] Fetching club Ids for club name: " << clubName << ", non-unique name: " << clubNonUniqueName);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append(CLUBDB_GET_CLUB_IDS_BY_NAME);

            if (clubName[0] != '\0')
                query->append(" `cc`.`name` LIKE $s ", clubName);

            if (clubNonUniqueName[0] != '\0')
            {
                if (clubName[0] != '\0')
                    query->append("AND");

                query->append(" `cc`.`nonUniqueName` = $s ", clubNonUniqueName);
            }

            DbResultPtr dbRes;
            result = mDbConn->executeQuery(query, dbRes);
            if (result == DB_ERR_OK)
            {
                rowsFound = dbRes->size();
                DbResult::const_iterator itr = dbRes->begin();
                DbResult::const_iterator end = dbRes->end();
                for (; itr != end; ++itr)
                {
                    DbRow *dbRow = *itr;
                    uint32_t col = 0;
                    clubIds.push_back(dbRow->getUInt64(col));
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMembershipsInDomain(ClubDomainId domainId, BlazeId blazeId, ClubMembershipList &clubMembershipList)
{
    TRACE_LOG("[ClubsDatabase::getMembershipsInDomain] Fetching club membership for user [blazeId]: " << blazeId << " in domain " << domainId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_MEMBERSHIPS_IN_DOMAIN, blazeId, domainId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->empty())
                {
                    result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
                }
                else
                {
                    DbResult::const_iterator itr = dbRes->begin();
                    DbResult::const_iterator end = dbRes->end();
                    for (; itr != end; ++itr)
                    {
                        DbRow *dbRow = *itr;
                        ClubMembership *clubMembership = clubMembershipList.pull_back();

                        // fill club membership struct
                        clubDbRowToClubMembership(dbRow, clubMembership);

                    }
                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::removeClub(ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::removeClub] Remove club [clubId]: " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdRemoveClubId);
        
        if (stmt != nullptr)
        {
            // clubId
            stmt->setUInt64(0, clubId);
            
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }        
    }    
    return result;
}

BlazeRpcError ClubsDatabase::promoteClubGM(BlazeId blazeId, ClubId clubId, const int32_t maxGmCount)
{
    TRACE_LOG("[clubs][ClubsDatabase::promoteClubGM] Promote blazeId [blazeId]: " << blazeId << " to GM for club [clubId]: " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdPromoteClubGMId);
        
        if (stmt != nullptr)
        {
            // clubId
            stmt->setInt32(0, CLUBS_GM);
            stmt->setInt64(1, blazeId);
            stmt->setUInt64(2, clubId);
            stmt->setInt32(3, maxGmCount);
            
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() <= 0)
                {
                    result = Blaze::CLUBS_ERR_TOO_MANY_GMS;
                }
                else
                {
                    result = ERR_OK;
                }
            }
        }
    }    
    return result;
}

BlazeRpcError ClubsDatabase::demoteClubMember(BlazeId gmId, ClubId clubId)
{
    TRACE_LOG("[clubs][ClubsDatabase::demoteClubMember] Demote GM blazeId [blazeId]: " << gmId << " to member for club [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdDemoteClubMemberId);

        if (stmt != nullptr)
        {
            // clubId
            stmt->setInt32(0, CLUBS_MEMBER);
            stmt->setInt64(1, gmId);
            stmt->setUInt64(2, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() <= 0)
                {
                    result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
                }
                else
                {
                    result = ERR_OK;
                }
            }
        }
    }    
    return result;
}

BlazeRpcError ClubsDatabase::getClubOwner(BlazeId& owner, ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::getClubOwner]Get club owner for club [clubId]: " <<  clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {

        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBDB_GET_CLUB_OWNER, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->empty())
                {
                    result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
                }
                else
                {                 
                    DbRow *row = *dbRes->begin();
                    uint32_t col = 0;
                    owner = row->getInt64(col);

                    result = ERR_OK;
                }

            }
        }     
    }
    return result;
}

BlazeRpcError ClubsDatabase::transferClubOwnership(BlazeId oldOwnerId, BlazeId newOwnerId, ClubId clubId, MembershipStatus newOwnerStatusBefore, MembershipStatus exOwnersNewStatus, const int32_t maxGmCount)
{
    TRACE_LOG("[clubs][ClubsDatabase::transferClubOwnership] Transfer Club Ownership to blazeId [blazeId]: " << newOwnerId << " for club [clubId]: " << clubId);

    if (exOwnersNewStatus == CLUBS_MEMBER)
    {
        TRACE_LOG("[clubs][ClubsDatabase::transferClubOwnership] The old owner blazeId [blazeId]:  "<< oldOwnerId << " for club [clubId]: " << clubId << " is non-GM");
    }
    else
    {
        TRACE_LOG("[clubs][ClubsDatabase::transferClubOwnership] The old owner blazeId [blazeId]: "<< oldOwnerId << " for club [clubId]: " << clubId << " is GM");
    }        
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateMembership);
        
        if (stmt != nullptr)
        {            
            stmt->setInt32(0, exOwnersNewStatus);
            stmt->setInt32(1, (exOwnersNewStatus == CLUBS_GM) ? 0 :(-1));
            stmt->setInt64(2, oldOwnerId);
            stmt->setUInt64(3, clubId);
            stmt->setInt32(4, maxGmCount + 1);
            
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() <= 0)
                {
                    return Blaze::CLUBS_ERR_TOO_MANY_GMS;
                }
            }
            else
            {
                return ERR_SYSTEM;
            }

            stmt->setInt32(0, CLUBS_OWNER);
            stmt->setInt32(1, (newOwnerStatusBefore == CLUBS_GM) ? 0 : 1);
            stmt->setInt64(2, newOwnerId);
            stmt->setUInt64(3, clubId);
            stmt->setInt32(4, maxGmCount);

            DbResultPtr dbResult;
            error = mDbConn->executePreparedStatement(stmt, dbResult);
            if (error == DB_ERR_OK)
            {
                if (dbResult->getAffectedRowCount() <= 0)
                {
                    return Blaze::CLUBS_ERR_TOO_MANY_GMS;
                }
                else
                {
                    return ERR_OK;
                }
            }
        }
    } 

    return ERR_SYSTEM;
}

BlazeRpcError ClubsDatabase::insertNews(ClubId clubId, BlazeId contentCreator, const ClubNews &news )
{
    TRACE_LOG("[ClubsDatabase::insertNews] [clubId]: " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertNewsId);
        
        if (stmt != nullptr)
        {
            uint32_t col = 0;
            
            //clubId
            stmt->setUInt64(col++, clubId);
            //newsType
            stmt->setInt32(col++, news.getType());
            //contentCreator
            stmt->setInt64(col++, contentCreator);
            //text
            stmt->setString(col++, news.getText());
            //stringId
            stmt->setString(col++, news.getStringId());
            //paramList
            stmt->setString(col++, news.getParamList());
            //associatedClubId
            stmt->setUInt64(col++, news.getAssociateClubId());
            
            
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }    

    return result;

}

BlazeRpcError ClubsDatabase::countNews(ClubId clubId, uint16_t &newsCount )
{
    NewsTypeList typeFilters;
    typeFilters.push_back(CLUBS_ALL_NEWS);

    return countNewsByType(clubId, newsCount, typeFilters);
}

BlazeRpcError ClubsDatabase::countNewsByType(ClubId clubId, uint16_t &newsCount, const NewsTypeList &typeFilters)
{
    TRACE_LOG("[ClubsDatabase::countNewsByType] [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_COUNT_NEWS, clubId);

            NewsTypeList::const_iterator it;

            bool bAllNews = false;

            // search for CLUBS_ALL_NEWS
            for (it = typeFilters.begin(); it != typeFilters.end(); it++)
            {
                if (*it == CLUBS_ALL_NEWS)
                    bAllNews = true;
            }

            if (!bAllNews)
            {
                query->append(" AND `newsType` IN (");

                NewsTypeList::const_iterator itr = typeFilters.begin();
                while(itr != typeFilters.end())
                {
                    if (itr != typeFilters.begin())
                        query->append(",");

                    query->append("$u", static_cast<uint32_t>(*itr++));

                    if (itr == typeFilters.end())
                        query->append(")");
                }
            }
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->size() == 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_INVALID_CLUB_ID);
                }
                else
                {
                    DbRow *row = *dbresult->begin();
                    uint16_t col = 0;
                    newsCount = (uint16_t)row->getUInt(col);
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::countNewsByTypeForMultipleClubs(const ClubIdList& clubIdList, uint16_t &newsCount, const NewsTypeList &typeFilters)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            ClubIdList::const_iterator iter = clubIdList.begin();
            query->append(CLUBSDB_COUNT_NEWS_FOR_MULTIPLE_CLUBS);

            query->append(" WHERE `clubId` IN (");

            while(iter != clubIdList.end())
            {
                if (iter != clubIdList.begin())
                    query->append(",");

                query->append("$U", *iter++);

                if (iter == clubIdList.end())
                    query->append(")");
            }

            // append the newsType filter
            NewsTypeList::const_iterator it;
            bool bAllNews = false;

            // search for CLUBS_ALL_NEWS
            for (it = typeFilters.begin(); it != typeFilters.end(); it++)
            {
                if (*it == CLUBS_ALL_NEWS)
                    bAllNews = true;
            }

            if (!bAllNews)
            {
                query->append(" AND `newsType` IN (");

                NewsTypeList::const_iterator itr = typeFilters.begin();
                while(itr != typeFilters.end())
                {
                    if (itr != typeFilters.begin())
                        query->append(",");

                    query->append("$u", static_cast<uint32_t>(*itr++));

                    if (itr == typeFilters.end())
                        query->append(")");
                }
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->size() == 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_INVALID_CLUB_ID);
                }
                else
                {
                    DbResult::const_iterator itr = dbresult->begin();

                    DbRow *row = *itr;
                    uint32_t col = 0;
                    newsCount = (uint16_t)row->getUInt(col);
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::deleteOldestNews(ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::deleteOldestNews] [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_DELETE_OLDEST_NEWS, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getNews(
    ClubId clubId, 
    const NewsTypeList &typeFilters, 
    TimeSortType sortType,
    uint32_t maxResultCount,
    uint32_t offset,
    eastl::list<ClubServerNewsPtr> &clubNewsList,
    bool& getAssociateNews)
{
    ClubIdList clubIdList;
    clubIdList.push_back(clubId);

    return getNews(clubIdList, typeFilters, sortType, maxResultCount, offset, clubNewsList, getAssociateNews);
}

BlazeRpcError ClubsDatabase::getNews(
        const ClubIdList& clubIdList, 
        const NewsTypeList &typeFilters, 
        TimeSortType sortType,
        uint32_t maxResultCount,
        uint32_t offset,
        eastl::list<ClubServerNewsPtr> &clubNewsList,
        bool& getAssociateNews)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {

            NewsTypeList::const_iterator typeItr;

            bool bAllNews = false;
            bool bAssociateNews = false;
            
            // search for CLUBS_ALL_NEWS, ASSOCIATE_NEWS
            for (typeItr = typeFilters.begin(); typeItr != typeFilters.end(); typeItr++)
            {
                if (*typeItr == CLUBS_ALL_NEWS)
                    bAllNews = true;
                    
                if (*typeItr == CLUBS_ASSOCIATE_NEWS)
                    bAssociateNews = true;
            }

            ClubIdList::const_iterator iter = clubIdList.begin();
            ClubIdList::const_iterator iterEnd = clubIdList.end();
            StringBuilder sb(nullptr);
            ClubId clubId;
            for (; iter != iterEnd; ++iter)
            {
                DB_QUERY_RESET(query);
                uint32_t totalCount = 0;
                clubId = *iter;
                query->append(CLUBSDB_SELECT_NEWS, clubId);
                if (!bAllNews)
                {
                    query->append(" AND `newsType` IN (");

                    typeItr = typeFilters.begin();
                    while(typeItr != typeFilters.end())
                    {
                        if (typeItr != typeFilters.begin())
                            query->append(",");
                            
                        query->append("$u", static_cast<uint32_t>(*typeItr++));
                            
                        if (typeItr == typeFilters.end())
                            query->append(")");
                    }
                }

                query->append(" ORDER BY `timestamp` ");
                if (sortType == CLUBS_SORT_TIME_DESC)
                    query->append(" DESC ");
                else
                    query->append(" ASC ");
                query->append("LIMIT $u, $u", offset, maxResultCount);

                DbResultPtr dbresult;
                BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
                if (error != DB_ERR_OK)
                    return result;

                DbResultPtr dbAssociateNewsresult;
                if (bAssociateNews || bAllNews)
                {
                    DB_QUERY_RESET(query);
                    query->append(CLUBSDB_SELECT_NEWS_ASSOICATE, clubId);
                    query->append(" ORDER BY `timestamp` ");
                    if (sortType == CLUBS_SORT_TIME_DESC)
                        query->append(" DESC ");
                    else
                        query->append(" ASC ");
                    if (maxResultCount > 0)
                        query->append("LIMIT $u, $u", offset, maxResultCount);

                    error = mDbConn->executeQuery(query, dbAssociateNewsresult);
                }

                if (error != DB_ERR_OK)
                    return result;

                result = ERR_OK;
                DbResult::const_iterator it = dbresult->begin();
                DbResult::const_iterator end = dbresult->end();
                DbResult::const_iterator assocIt = (bAssociateNews || bAllNews) ? dbAssociateNewsresult->begin() : nullptr;
                DbResult::const_iterator assocEnd = (bAssociateNews || bAllNews) ? dbAssociateNewsresult->end() : nullptr;
                const DbRow *newsDbRow = nullptr;
                const DbRow *assocDbRow = nullptr;
                while(totalCount < maxResultCount)
                {
                    const DbRow *rowToInsert = nullptr;
                    uint32_t newsTime = 0;
                    uint32_t assocNewsTime = 0;

                    if (it != end &&
                        newsDbRow == nullptr)
                    {
                        newsDbRow = *it;
                        ++it;
                    }
                    if (assocIt != assocEnd &&
                        assocDbRow == nullptr)
                    {
                        assocDbRow = *assocIt;
                        ++assocIt;
                    }

                    if (newsDbRow != nullptr)
                        newsTime = newsDbRow->getUInt("timestamp");
                    if (assocDbRow != nullptr)
                        assocNewsTime = assocDbRow->getUInt("timestamp");

                    if (sortType == CLUBS_SORT_TIME_DESC)
                    {
                        if (newsTime > 0 &&
                            newsTime >= assocNewsTime)
                        {
                            rowToInsert = newsDbRow;
                            newsDbRow = nullptr;
                        }
                        else
                        {
                            rowToInsert = assocDbRow;
                            assocDbRow = nullptr;
                        }
                    }
                    else
                    {
                        if (newsTime > 0 &&
                            assocNewsTime <= 0)
                        {
                            rowToInsert = newsDbRow;
                            newsDbRow = nullptr;
                        }
                        else if (assocNewsTime > 0 &&
                                 newsTime <= 0)
                        {
                            rowToInsert = assocDbRow;
                            assocDbRow = nullptr;
                        }
                        else if (newsTime <= assocNewsTime)
                        {
                            rowToInsert = newsDbRow;
                            newsDbRow = nullptr;
                        }
                        else
                        {
                            rowToInsert = assocDbRow;
                            assocDbRow = nullptr;
                        }
                    }

                    if (rowToInsert == nullptr)
                        break; //we're done

                    if (totalCount >= offset)
                    {                
                        uint32_t col = 0;
                        ClubServerNews *clubNews = BLAZE_NEW ClubServerNews();

                        // rowclub it
                        ClubId rowClubId;
                        rowClubId = rowToInsert->getUInt64(col++);
                        // contentCreator(BlazeId)
                        clubNews->setContentCreator(rowToInsert->getInt64(col++));
                        // newsType
                        clubNews->setType(static_cast<NewsType>(rowToInsert->getUInt(col++)));
                        // text
                        clubNews->setText(rowToInsert->getString(col++));
                        // stringId 
                        clubNews->setStringId(rowToInsert->getString(col++));
                        // paramList
                        clubNews->setParamList(rowToInsert->getString(col++));
                        // associatedClubId
                        clubNews->setAssociateClubId(rowToInsert->getUInt64(col++));

                        if (clubNews->getType() != CLUBS_ASSOCIATE_NEWS)
                        {
                            // this is club news that belongs to this club so
                            // replace associated id with 0
                            clubNews->setAssociateClubId(0);
                        }// else keep the accociateclubId as the same in DB, cause when calling postNews to post CLUBS_ASSOCIATE_NEWS,
                         // it must proide the accociateclubId and it should be not the same as clubId.

                        // timestamp
                        clubNews->setTimestamp(rowToInsert->getUInt(col++));
                        // flags
                        clubNews->getFlags().setBits(rowToInsert->getUInt(col++));
                        // newsId
                        clubNews->setNewsId(rowToInsert->getUInt(col++));
                        // clubid
                        clubNews->setClubId(rowClubId);

                        clubNewsList.push_back(clubNews);
                    }
                    ++totalCount;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getServerNews(
    const NewsId newsId,
    ClubId &clubId,
    ClubServerNews &serverNews)
{
    TRACE_LOG("[ClubsDatabase::getServerNews] [newsId]: " << newsId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_SELECT_SERVER_NEWS, newsId);
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                DbResult::const_iterator it = dbresult->begin();
                if (it == dbresult->end())
                    result = Blaze::CLUBS_ERR_NEWS_ITEM_NOT_FOUND;
                else
                {
                    const DbRow *dbrow = *it;
                    uint32_t col = 0;
                    // clubId
                    clubId = dbrow->getUInt64(col++);
                    // contentCreator(BlazeId)
                    serverNews.setContentCreator(dbrow->getInt64(col++));
                    // newsType
                    serverNews.setType(static_cast<NewsType>(dbrow->getUInt(col++)));
                    // text
                    serverNews.setText(dbrow->getString(col++));
                    // stringId 
                    serverNews.setStringId(dbrow->getString(col++));
                    // paramList
                    serverNews.setParamList(dbrow->getString(col++));
                    // associatedClubId
                    serverNews.setAssociateClubId(dbrow->getUInt64(col++));
                    if (clubId == serverNews.getAssociateClubId())
                    {
                        // this is club news that belongs to this club so
                        // replace associated id with INVALID_CLUB_ID
                        serverNews.setAssociateClubId(INVALID_CLUB_ID);
                    }

                    // timestamp
                    serverNews.setTimestamp(dbrow->getUInt(col++));
                    // flags
                    serverNews.getFlags().setBits(dbrow->getUInt(col++));
                    // newsId
                    serverNews.setNewsId(dbrow->getUInt(col++));

                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}


BlazeRpcError ClubsDatabase::updateNewsItemFlags(
        NewsId newsId,
        ClubNewsFlags mask, 
        ClubNewsFlags value)
{
    TRACE_LOG("[ClubsDatabase::updateNewsItemFlags] [newsId]: " << newsId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateNewsItemFlags);

        if (stmt != nullptr)
        {
            uint32_t col = 0;

            //inv mask
            stmt->setUInt32(col++, ~mask.getBits());
            //value
            stmt->setUInt32(col++, mask.getBits() & value.getBits());
            //newsId
            stmt->setUInt32(col++, newsId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() == 0)
                {
                    result = Blaze::CLUBS_ERR_NEWS_ITEM_NOT_FOUND;
                }
                else
                    result = Blaze::ERR_OK;
            }
        }
    }

    return result;

}

BlazeRpcError ClubsDatabase::insertClubMessage(ClubMessage &message, ClubDomainId domainId, uint32_t maxNumOfMessages)
{
    TRACE_LOG("[ClubsDatabase::insertClubMessage] insert message for club [clubId]: " << message.getClubId());

    BlazeRpcError result = Blaze::ERR_SYSTEM;
    DbResultPtr dbRes;
    BlazeRpcError error;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            switch (message.getMessageType())
            {
            case CLUBS_INVITATION:
                {
                    query->append(CLUBSDB_COUNT_CLUB_MESSAGES, CLUBS_INVITATION, message.getClubId());
                    break;
                }
            case CLUBS_PETITION:
                {
                    query->append(CLUBSDB_COUNT_USER_MESSAGES, CLUBS_PETITION, domainId);
                    query->append(" AND `sender` = $D ", message.getSender().getBlazeId());
                    break;
                }
            default:
                {
                    return CLUBS_ERR_INVALID_ARGUMENT;
                }
            }

            error = mDbConn->executeQuery(query, dbRes);
            if(error == DB_ERR_OK)
            {
                DbResult::const_iterator it = dbRes->begin();
                DbRow *dbRow = *it;
                if(dbRow->getInt((uint32_t)0) == static_cast<int32_t>(maxNumOfMessages))
                {
                    return CLUBS_ERR_MAX_MESSAGES_SENT;
                }
            }

            DB_QUERY_RESET(query);
            switch (message.getMessageType())
            {
            case CLUBS_INVITATION:
                {
                    query->append(CLUBSDB_COUNT_USER_MESSAGES, CLUBS_INVITATION, domainId);
                    query->append(" AND `receiver` = $D ", message.getReceiver().getBlazeId());
                    break;
                }
            case CLUBS_PETITION:
                {
                    query->append(CLUBSDB_COUNT_CLUB_MESSAGES, CLUBS_PETITION, message.getClubId());
                    break;
                }
            }

            error = mDbConn->executeQuery(query, dbRes);    
            if(error == DB_ERR_OK)
            {
                DbResult::const_iterator it = dbRes->begin();
                DbRow *dbRow = *it;

                if(dbRow->getInt((uint32_t)0) == static_cast<int32_t>(maxNumOfMessages))
                {
                    return CLUBS_ERR_MAX_MESSAGES_RECEIVED;
                }
            }
        }

        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertMessageId);
        
        if (stmt != nullptr)
        {
            // clubId
            int32_t col = 0;
            stmt->setUInt64(col++, message.getClubId());
            stmt->setInt32(col++, message.getMessageType());
            stmt->setInt64(col++, message.getSender().getBlazeId());
            stmt->setInt64(col++, message.getReceiver().getBlazeId());
            
            error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                message.setMessageId(static_cast<uint32_t>(dbRes->getLastInsertId()));
                result = ERR_OK;
            }
        }  
    }    
    return result;
}

BlazeRpcError ClubsDatabase::updateMetaData(
    ClubId clubId, 
    ClubMetadataUpdateType metadateUpdateType, 
    const MetadataUnion& metaData,
    bool updateLastActive)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        const char* metadataName = "metadata";
        PreparedStatement* stmt = nullptr;

        // Case where we combine the update last active and update the metadata
        if (updateLastActive)
        {
            switch (metadateUpdateType)
            {
            case CLUBS_METADATA_UPDATE:
                stmt = mDbConn->getPreparedStatement(mCmdUpdateMetadataAndLastActiveId);
                break;
            case CLUBS_METADATA_UPDATE_2:
                metadataName = "metadata2";
                stmt = mDbConn->getPreparedStatement(mCmdUpdateMetadata_2_AndLastActiveId);
                break;
            default:
                break;
            }
        }
        else // Only updating metadata
        {
            switch (metadateUpdateType)
            {
            case CLUBS_METADATA_UPDATE:
                stmt = mDbConn->getPreparedStatement(mCmdUpdateMetadataId);
                break;
            case CLUBS_METADATA_UPDATE_2:
                metadataName = "metadata2";
                stmt = mDbConn->getPreparedStatement(mCmdUpdateMetadata_2_Id);
                break;
            default:
                break;
            }
        }

        TRACE_LOG("[ClubsDatabase::updateMetaData] Updating club " << metadataName << " for club [id]: " << clubId);

        if (stmt != nullptr)
        {
            uint32_t col = 0;

            if (metaData.getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
            {
                stmt->setInt32(col++, CLUBS_METADATA_TYPE_STRING);
                stmt->setBinary(col++, reinterpret_cast<const uint8_t*>(metaData.getMetadataString()), strlen(metaData.getMetadataString()) + 1);
            }
            else if (metaData.getActiveMember() == MetadataUnion::MEMBER_METADATABLOB)
            {
                stmt->setInt32(col++, CLUBS_METADATA_TYPE_BINARY);
                stmt->setBinary(col++, metaData.getMetadataBlob()->getData(), metaData.getMetadataBlob()->getSize());
            }
            // clubId
            stmt->setUInt64(col++, clubId);
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::findClubs(
    const bool anyDomain,
    const ClubDomainId domainId,
    const char8_t* name,
    const char8_t* language,
    const bool anyTeamId,
    const TeamId teamId,
    const char8_t* nonUniqueName,
    const ClubAcceptanceFlags& acceptanceMask,
    const ClubAcceptanceFlags& acceptanceFlags,
    const SeasonLevel seasonLevel,
    const ClubRegion region,
    const uint32_t minMemberCount,
    const uint32_t maxMemberCount,
    const ClubIdList &clubIdFilterList,
    const BlazeIdList &blazeIdFilterList,
    const uint16_t customSettingsMask,
    const CustClubSettings* customSettings,
    const uint32_t lastGameTime,
    const uint32_t maxResultCount,
    const uint32_t offset,
    const bool skipMetadata,
    const TagSearchOperation tagSearchOperation,
    const ClubTagList &tagList,
    const PasswordOption passwordOpt,
    const SearchRequestorAcceptance& petitionAcceptance,
    const SearchRequestorAcceptance& joinAcceptance,
    ClubList &clubList,
    uint32_t* totalCount,
    const ClubsOrder clubsOrder,
    const OrderMode orderMode)
{
    TRACE_LOG("[ClubsDatabase::findClubs] Finding clubs...");

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        { 
            // need to calc found rows?
            bool isCalcFoundRows = (maxResultCount < UINT32_MAX || offset > 0) && (totalCount != nullptr);

            // filter by club members, and calculate total count.
            bool isListEmpty = blazeIdFilterList.empty();
            query->append(CLUBSDB_FIND_CLUBS,
                (!isListEmpty) ? "DISTINCT" : "",
                (isCalcFoundRows) ? CLUBSDB_SQL_CALC_FOUND_ROWS : "");
            if (!isListEmpty)
                query->append(" INNER JOIN `clubs_members` `cm` ON cc.clubId = cm.clubId ");


            //filter by club tags list
            //If the request lists no tags, the search will not consider tags, and any clubs could be returned (based on other search parameters).
            bool searchTags = (!tagList.empty() && tagSearchOperation != CLUB_TAG_SEARCH_IGNORE);
            if (searchTags)
            {
                matchTagsFilter(query, tagList, (tagSearchOperation == CLUB_TAG_SEARCH_MATCH_ALL), maxResultCount);
            }
            else
                query->append(" WHERE TRUE ");

            uint16_t numCriteria = 0;

            // filter by domain id
            if (!anyDomain)
            {
                query->append(" AND `cc`.`domainId` = $u ", domainId);
                ++numCriteria;
            }

            //filter by club name
            if (name != nullptr && *name != '\0')
            {
                query->append(" AND `cc`.`name` LIKE '$s%' ", name);
                ++numCriteria;
            }

            //filter by club language
            if (language != nullptr && *language != '\0')
            {
                query->append(" AND `cc`.`language`='$s' ", language);
                ++numCriteria;
            }

            // filter by club team id
            if (!anyTeamId)
            {
                query->append(" AND `cc`.`teamId` = $u ", teamId);
                ++numCriteria;
            }

            //filter by club nonuniquename
            if (nonUniqueName != nullptr && *nonUniqueName != '\0')
            {
                query->append(" AND `cc`.`nonuniquename`='$s' ", nonUniqueName);
                ++numCriteria;
            }

            if (acceptanceMask.getBits() != 0)
            {
                //filter by acceptance flags
                bool bFirstFindVal = true;
                query->append(" AND `cc`.`acceptanceFlags` IN (");
                for (uint32_t val = 0; val < 16; val++)
                {
                    if (! ((val ^ acceptanceFlags.getBits()) & acceptanceMask.getBits()))
                    {
                        if (bFirstFindVal)
                            bFirstFindVal = false;
                        else
                        {
                            query->append(",");
                        }
                        query->append("$u", val);
                    }
                }
                query->append(") ");
                ++numCriteria;
             }

            //filter by season level
            if (seasonLevel != 0 )
            {
                query->append(" AND (`cc`.`seasonLevel` = $u) ", seasonLevel);
                ++numCriteria;
            }

            //filter by club region
            if (region != 0 )
            {
                query->append(" AND (`cc`.`region` = $u) ", region);
                ++numCriteria;
            }

            // filter by min member count
            if (minMemberCount != 0)
            {
                query->append(" AND `cc`.`memberCount` >= $u ", minMemberCount);
                ++numCriteria;
            }

            // filter by  max member count
            if (maxMemberCount != 0)
            {
                query->append(" AND `cc`.`memberCount` <= $u ", maxMemberCount);
                ++numCriteria;
            }

            // filter by clubs
            if (!clubIdFilterList.empty())
            {
                query->append(" AND `cc`.`clubId` IN (");
                ClubIdList::const_iterator it = clubIdFilterList.begin();
                while (it != clubIdFilterList.end())
                {
                    query->append("$U", *it++);
                    if (it == clubIdFilterList.end())
                        query->append(")");
                    else
                        query->append(",");
                }
                ++numCriteria;
            }

            //filter by club members
            if (!isListEmpty)
            {
                query->append(" AND `cm`.`blazeId` IN (");
                BlazeIdList::const_iterator it = blazeIdFilterList.begin();
                while (it != blazeIdFilterList.end())
                {
                    query->append("$D", *it++);
                    if (it == blazeIdFilterList.end())
                        query->append(")");
                    else
                        query->append(",");
                }
                ++numCriteria;
            }

            if (customSettingsMask != 0 && customSettings != nullptr)
            {
                //filter by custom options
                if ((1 << 1) & customSettingsMask)
                {
                    query->append(" AND `cc`.`custOpt1` = $u ", customSettings->getCustOpt1());
                    ++numCriteria;
                }
                if ((1 << 2) & customSettingsMask)
                {
                    query->append(" AND `cc`.`custOpt2` = $u ", customSettings->getCustOpt2());
                    ++numCriteria;
                }
                if ((1 << 3) & customSettingsMask)
                {
                    query->append(" AND `cc`.`custOpt3` = $u ", customSettings->getCustOpt3());
                    ++numCriteria;
                }
                if ((1 << 4) & customSettingsMask)
                {
                    query->append(" AND `cc`.`custOpt4` = $u ", customSettings->getCustOpt4());
                    ++numCriteria;
                }
                if ((1 << 5) & customSettingsMask)
                {
                    query->append(" AND `cc`.`custOpt5` = $u ", customSettings->getCustOpt5());
                    ++numCriteria;
                }
             }

            //filter by last game time
            if (lastGameTime != 0 )
            {
                query->append(" AND UNIX_TIMESTAMP(`cc`.`lastGameTime`) >= $u ", lastGameTime);
                ++numCriteria;
            }

            //filter by club password
            switch (passwordOpt)
            {
            case CLUB_PASSWORD_IGNORE:
                break;
            case CLUB_PASSWORD_NOT_PROTECTED:
                query->append(" AND `cc`.`password` = '' ");
                ++numCriteria;
                break;
            case CLUB_PASSWORD_PROTECTED:
                query->append(" AND `cc`.`password` != '' ");
                ++numCriteria;
                break;
            default:
                break;
            }

            // filter by club petition acceptance
            if (petitionAcceptance == CLUB_ACCEPTS_ALL)
            {
                query->append(" AND `cc`.`petitionAcceptance` = $u ",  CLUB_ACCEPT_ALL);
                ++numCriteria;
            } 
            else if (petitionAcceptance == CLUB_ACCEPTS_NONE)
            {
                query->append(" AND `cc`.`petitionAcceptance` = $u ",  CLUB_ACCEPT_NONE);
                ++numCriteria;
            } 

            // filter by club join acceptance
            if (joinAcceptance == CLUB_ACCEPTS_ALL)
            {
                query->append(" AND `cc`.`joinAcceptance` = $u ",  CLUB_ACCEPT_ALL);
                ++numCriteria;
            }
            else if (joinAcceptance == CLUB_ACCEPTS_NONE)
            {
                query->append(" AND `cc`.`joinAcceptance` = $u ",  CLUB_ACCEPT_NONE);
                ++numCriteria;
            }

            // potential db issues can arise from find clubs creating queries based off too few criteria.  This can particularly be a problem
            // when sorting ordered results that match a large subset of rows.
            if (numCriteria == 0 || (!anyDomain && numCriteria == 1))
            {
                WARN_LOG("[ClubsDatabase::findClubs] Too few find club criteria, please consult OPS to add an index on clubs_clubs table for specific filtering that you application requires");
            }

            // order the search result
            const char* orderModeString = (orderMode == DESC_ORDER) ? CLUBSDB_SORT_DESC : CLUBSDB_SORT_ASC ;
            switch (clubsOrder)
            {
            case CLUBS_NO_ORDER:
                break;
            case CLUBS_ORDER_BY_NAME:
                query->append(" ORDER BY `cc`.`name` $s ", orderModeString);
                break;
            case CLUBS_ORDER_BY_CREATIONTIME:
                // Use clubId instead of creationTime because IDs are unique and assigned in sequence when a club is created.
                // Also, this clause should be faster because the clubId is the primary key, and thus entries within some other
                // index that we may use for searching just so happen to be ordered inside that index by primary key.  However,
                // just having an index on a field doesn't mean sorting will be any faster if the query engine needs to use some
                // other index to find matching rows for the query (as likely for name or memberCount).
                query->append(" ORDER BY `cc`.`clubId` $s ", orderModeString);
                break;
            case CLUBS_ORDER_BY_MEMBERCOUNT:
                query->append(" ORDER BY `cc`.`memberCount` $s ", orderModeString);
                break;
            default:
                break;
            }

            // limit query results
            if (maxResultCount > 0 && (maxResultCount < UINT32_MAX || offset > 0))
            {
                query->append(" LIMIT $u ", maxResultCount);
                if (offset > 0)
                {
                    query->append(" OFFSET $u ", offset);
                }
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                uint32_t resultSize = dbresult->size();

                if (resultSize > 0)
                {
                    DbResult::const_iterator it = dbresult->begin();
                    for (it = dbresult->begin(); it != dbresult->end(); it++)
                    {
                        const DbRow *dbrow = *it;

                        uint32_t col = 0;
                        Club *club = clubList.pull_back();

                        clubDBRowToClub(col, dbrow, club);

                        clubDBRowToClubInfo(
                            col, 
                            dbrow, 
                            club->getClubInfo());

                        clubDBRowToClubSettings(
                            col, 
                            dbrow, 
                            club->getClubSettings(), 
                            skipMetadata);

                        if (clubList.size() >= maxResultCount)
                            break;
                    }
                }

                result = ERR_OK;

                // query again to get total count of previous query result without limit or offset.
                if (isCalcFoundRows)
                {
                    DB_QUERY_RESET(query);
                    query->append(CLUBSDB_SELECT_FOUND_ROWS);
                    result = Blaze::ERR_SYSTEM;
                    error = mDbConn->executeQuery(query, dbresult);
                    if (error == DB_ERR_OK)
                    {
                        if (dbresult->size() > 0)
                        {
                            DbRow *row = *dbresult->begin();
                            uint32_t col = 0;
                            *totalCount = row->getUInt(col);
                            result = ERR_OK;
                        }
                    }
                }
                else if (totalCount != nullptr)
                {
                    *totalCount = resultSize;
                }
            }
        }
    }

    return result;
}

void ClubsDatabase::matchTagsFilter(QueryPtr& query, const ClubTagList &tagList, bool matchAllTags, uint32_t maxResultCount)
{
    const size_t tagListSize = tagList.size();
    query->append(CLUBDB_FILTER_TAGS, (!matchAllTags && tagListSize > 1) ? "DISTINCT" : "");
    ClubTagList::const_iterator tagItr = tagList.begin();
    ClubTagList::const_iterator tagEnd = tagList.end();
    for (; tagItr != tagEnd; ++tagItr)
    {
        query->append("'$s',", (*tagItr).c_str());
    }
    query->trim(1);
    query->append(")");

    // Only need to GROUP BY if we're searching for more than one tag
    if (matchAllTags && (tagListSize > 1))
    {
        // make sure the number of tags each clubId contains equals the taglist.size()
        // if not, the result will match any tag
        query->append(" GROUP BY `c2t`.`clubId` HAVING COUNT(*) = $u", tagList.size());
    }

    // For performance reasons, we need to limit the number of rows we examine here based on the requested maxResultCount
    // in the main findClubs query, up to a maximum value of CLUBS_TAG_MAX_RESULT_COUNT
    // The multiplier should be large enough that we'll return enough clubIds for the main findClubs query
    // to return the requested number of clubs
    uint32_t maxClubTagResultCount = (maxResultCount * CLUBS_TAGS_RESULT_COUNT_MULTIPLIER);
    if (maxClubTagResultCount == 0 || maxClubTagResultCount > CLUBS_TAG_MAX_RESULT_COUNT)
        maxClubTagResultCount = CLUBS_TAG_MAX_RESULT_COUNT;

    query->append(" LIMIT $u) `ct` WHERE cc.clubId = ct.clubId", maxClubTagResultCount);
}

BlazeRpcError ClubsDatabase::getClubMessagesByUserReceived(BlazeId blazeId, MessageType messageType, 
                                                   TimeSortType sortType, ClubMessageList &messageList)
{
    TRACE_LOG("[ClubsDatabase::getClubMessagesByUserReceived]user [blazeid]: " << blazeId << " message type [messagetype]: " << messageType);

    BlazeRpcError result = Blaze::ERR_SYSTEM;
        
    if (mDbConn != nullptr)
    {
        
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        const char8_t *sortString = nullptr;
        switch(sortType)
        {
        case CLUBS_SORT_TIME_DESC:
            sortString = CLUBSDB_SORT_DESC; break;
        case CLUBS_SORT_TIME_ASC:
            sortString = CLUBSDB_SORT_ASC; break;
        }
        
        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_MESSAGES_BY_USER_RECEIVED, blazeId, messageType, sortString);

            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->size() == 0)
                {
                    result = ERR_OK;
                }
                else
                {
                    DbResult::const_iterator it = dbresult->begin();
                    for (it = dbresult->begin(); it != dbresult->end(); it++)
                    {
                        const DbRow *dbrow = *it;
                        ClubMessage *msg = messageList.pull_back();
                        clubDbRowToClubMessage(dbrow, msg);
                    }
                    
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMessagesByUserSentDoDBWork(BlazeId blazeId, MessageType messageType, 
                                                             TimeSortType sortType, DbResultPtr &dbresult)
{
    TRACE_LOG("[ClubsDatabase::getClubMessagesByUserSent]user [blazeid]: " << blazeId << " message type [messagetype]: " << messageType);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {

        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        const char8_t *sortString = nullptr;
        switch(sortType)
        {
        case CLUBS_SORT_TIME_DESC:
            sortString = CLUBSDB_SORT_DESC; break;
        case CLUBS_SORT_TIME_ASC:
            sortString = CLUBSDB_SORT_ASC; break;
        }

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_MESSAGES_BY_USER_SENT, blazeId, messageType, sortString);

            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMessagesByUserSent(BlazeId blazeId, MessageType messageType, 
                                                   TimeSortType sortType, ClubMessageList &messageList)
{
    TRACE_LOG("[ClubsDatabase::getClubMessagesByUserSent]user [blazeid]: " << blazeId << " message type [messagetype]: " << messageType);

    DbResultPtr dbresult;
    BlazeRpcError result = getClubMessagesByUserSentDoDBWork(blazeId, messageType, sortType, dbresult);

    if( result == ERR_OK && dbresult->size() > 0)
    {
        DbResult::const_iterator it = dbresult->begin();
        for (it = dbresult->begin(); it != dbresult->end(); it++)
        {
            const DbRow *dbrow = *it;
            ClubMessage *msg = messageList.pull_back();
            clubDbRowToClubMessage(dbrow, msg);
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMessagesByClubDoDBWork(const ClubIdList& clubIdList, MessageType messageType, 
                                                        TimeSortType sortType, DbResultPtr &dbresult)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        const char8_t *sortString = nullptr;
        switch(sortType)
        {
        case CLUBS_SORT_TIME_DESC:
            sortString = CLUBSDB_SORT_DESC; break;
        case CLUBS_SORT_TIME_ASC:
            sortString = CLUBSDB_SORT_ASC; break;
        }

        StringBuilder sb(nullptr);
        bool firstClubId = true;
        if (query != nullptr)
        {
            ClubIdList::const_iterator iter = clubIdList.begin();
            ClubIdList::const_iterator iterEnd = clubIdList.end();
            for (; iter != iterEnd; ++iter)
            {
                TRACE_LOG("[ClubsDatabase::getClubMessagesByClub]club [clubid]: " << *iter << " messaget type [messagetype]: " << messageType);
                sb << (firstClubId ? "" : ",") << *iter;
                firstClubId = false;
            }
            query->append(CLUBSDB_GET_MESSAGES_BY_CLUB, sb.get(), messageType, sortString);

            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMessagesByClub(ClubId clubId, MessageType messageType, 
            TimeSortType sortType, ClubMessageList &messageList)
{
    ClubIdList clubIdList;
    clubIdList.push_back(clubId);

    return getClubMessagesByClubs(clubIdList, messageType, sortType, messageList);
}


BlazeRpcError ClubsDatabase::getClubMessagesByClubs(const ClubIdList& clubIdList, MessageType messageType, 
        TimeSortType sortType, ClubMessageList &messageList)
{
    DbResultPtr dbresult;
    BlazeRpcError result = getClubMessagesByClubDoDBWork(clubIdList, messageType, sortType, dbresult);

    if (result == ERR_OK && dbresult->size() > 0)
    {
        DbResult::const_iterator it = dbresult->begin();
        for (it = dbresult->begin(); it != dbresult->end(); it++)
        {
            const DbRow *dbrow = *it;
            ClubMessage *msg = messageList.pull_back();
            clubDbRowToClubMessage(dbrow, msg);
        }

        result = ERR_OK;
    }

    return result;
}


BlazeRpcError ClubsDatabase::getClubMessage(uint32_t messageId, ClubMessage &message)
{
    TRACE_LOG("[ClubsDatabase::getClubMessage]message [messageid=" << messageId << "]:");

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_MESSAGE, messageId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (!dbresult->empty())
                {
                    DbResult::const_iterator it = dbresult->begin();
                    const DbRow *dbrow = *it;
                    clubDbRowToClubMessage(dbrow, &message);
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::deleteClubMessage(uint32_t messageTid)
{
    TRACE_LOG("[ClubsDatabase::deleteClubMessage] deleting club message [id]: " << messageTid);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdDeleteMessageId);
        
        if (stmt != nullptr)
        {
            //metaDataType
            stmt->setUInt32(0, messageTid);
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateClubLastActive(ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::updatClubLastActive] club [clubid]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateLastActiveId);

        if (stmt != nullptr)
        {
            //metaDataType
            stmt->setUInt64(0, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubIdentities(const ClubIdList *clubIdList, ClubList *clubList)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_IDENTITY);

            ClubIdList::const_iterator it = clubIdList->begin();
            while (it != clubIdList->end())
            {
                query->append("$U", *it++);
                if (it == clubIdList->end())
                    query->append(")");
                else
                    query->append(",");
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->empty())
                {
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                }
                else
                {
                    DbResult::const_iterator itr = dbresult->begin();
                    for (itr = dbresult->begin(); itr != dbresult->end(); itr++)
                    {
                        const DbRow *dbrow = *itr;

                        uint32_t col = 0;
                        Club *club = clubList->pull_back();

                        club->setClubId(dbrow->getUInt64(col++));
                        club->setName(dbrow->getString(col++));
                    }

                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubIdentityByName(const char8_t *name, Club *club)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_IDENTITY_BY_NAME, name);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->empty())
                {
                    result = CLUBS_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    DbResult::const_iterator it = dbresult->begin();
                    const DbRow *dbrow = *it;

                    club->setClubId(dbrow->getUInt64((uint32_t)0));
                    club->setName(name);

                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::lockClub(ClubId clubId, uint64_t& version, Club* club/*= nullptr*/)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    TimeValue start = TimeValue::getTimeOfDay();

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_LOCK_CLUB, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->empty())
                {
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                }
                else
                {
                    if (club != nullptr)
                    {
                        const DbRow *dbrow = *dbresult->begin();
                        
                        uint32_t col = 0;
                        clubDBRowToClub(col, dbrow, club);
                        clubDBRowToClubInfo(col, dbrow, club->getClubInfo());
                        clubDBRowToClubSettings(col, dbrow, club->getClubSettings());
                        version = dbrow->getUInt64(col++);
                    }
                    result = ERR_OK;
                }
            }
        }
    }

    TimeValue end = TimeValue::getTimeOfDay();
    TRACE_LOG("[lockClub] Execution time was " << (end - start).getMillis() << ", club was " << clubId << ", result was " << result);

    return result;
}

BlazeRpcError ClubsDatabase::lockClubs(ClubIdList *clubIdList, ClubList *clubList)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    TimeValue start = TimeValue::getTimeOfDay();

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_LOCK_CLUBS);

            ClubIdList::const_iterator it = clubIdList->begin();
            while (it != clubIdList->end())
            {
                query->append("$U", *it++);
                if (it == clubIdList->end())
                    query->append(")");
                else
                    query->append(",");
            }

            query->append(" ORDER BY clubId FOR UPDATE ");
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->empty())
                {
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                }
                else
                {
                    if (clubList != nullptr)
                    {
                        DbResult::const_iterator itr = dbresult->begin();
                        for (itr = dbresult->begin(); itr != dbresult->end(); itr++)
                        {
                            const DbRow *dbrow = *itr;

                            uint32_t col = 0;
                            Club *club = clubList->pull_back();

                            clubDBRowToClub(col, dbrow, club);
                            clubDBRowToClubInfo(col, dbrow, club->getClubInfo());
                            clubDBRowToClubSettings(col, dbrow, club->getClubSettings());
                        }
                    }
                    result = ERR_OK;
                }
            }
        }
    }

    TimeValue end = TimeValue::getTimeOfDay();
    TRACE_LOG("[lockClubs] Execution time was " << (end - start).getMillis() << ", result was " << result);

    return result;
}

BlazeRpcError ClubsDatabase::lockUserMessages(BlazeId blazeId)
{       
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append(CLUBSDB_LOCK_USER_MESSAGES, blazeId, blazeId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::lockMemberDomain(BlazeId blazeId, ClubDomainId domainId)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_LOCK_MEMBER_DOMAIN, blazeId, domainId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMembershipForUsers(const BlazeIdList& blazeIdList, ClubMembershipMap &clubMembershipMap)
{
    TRACE_LOG("[ClubsDatabase::getClubMembershipForUsers] Fetching club memberships for users");
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_MEMBERSHIP_FOR_USERS);
            BlazeIdList::const_iterator it = blazeIdList.begin();
            while (it != blazeIdList.end())
            {
                query->append("$D", *it++);
                if (it == blazeIdList.end())
                    query->append(")");
                else
                    query->append(",");
            }

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    DbResult::const_iterator dbit = dbRes->begin();
                    while (dbit != dbRes->end())
                    {
                        DbRow *dbRow = *dbit;

                        ClubMembership *clubMembership = BLAZE_NEW ClubMembership();
                        // fill club membership struct
                        clubDbRowToClubMembership(dbRow, clubMembership);
                        ClubMembershipMap::iterator mapItr = clubMembershipMap.find(clubMembership->getClubMember().getUser().getBlazeId());
                        if (mapItr == clubMembershipMap.end())
                        {
                            clubMembershipMap.insert(eastl::make_pair(clubMembership->getClubMember().getUser().getBlazeId(), BLAZE_NEW ClubMemberships));
                            mapItr = clubMembershipMap.find(clubMembership->getClubMember().getUser().getBlazeId());
                        }
                        ClubMemberships *clubMemberships = mapItr->second;
                        clubMemberships->getClubMembershipList().push_back(clubMembership);

                        dbit++;
                    }
                }
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubMetadataForUsers(const BlazeIdList& blazeIdList, ClubMetadataMap &clubMetadataMap)
{
	TRACE_LOG("[ClubsDatabase::getClubMetadataForUsers] Fetching club metadata for users");
	BlazeRpcError result = Blaze::ERR_SYSTEM;

	if (mDbConn != nullptr)
	{
		QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

		if (query != nullptr)
		{
			query->append(CLUBSDB_GET_CLUB_METADATA_FOR_USERS);
			BlazeIdList::const_iterator it = blazeIdList.begin();
			while (it != blazeIdList.end())
			{
				query->append("$D", *it++);
				if (it == blazeIdList.end())
					query->append(")");
				else
					query->append(",");
			}

			DbResultPtr dbRes;
			BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

			if (error == DB_ERR_OK)
			{
				if (!dbRes->empty())
				{
					DbResult::const_iterator dbit = dbRes->begin();
					while (dbit != dbRes->end())
					{
						DbRow *dbRow = *dbit;

						ClubMetadata *clubMetadata = BLAZE_NEW ClubMetadata();
						clubDbRowToClubMetadata(dbRow, clubMetadata);

						ClubMetadataMap::iterator mapItr = clubMetadataMap.find(clubMetadata->getBlazeId());
						if (mapItr == clubMetadataMap.end())
						{
							clubMetadataMap.insert(eastl::make_pair(clubMetadata->getBlazeId(), clubMetadata));
						}

						dbit++;
					}
				}
				result = ERR_OK;
			}
		}
	}

	return result;
}

BlazeRpcError ClubsDatabase::getClubMetadata(const ClubId &clubId, ClubMetadata &clubMetadata)
{
    TRACE_LOG("[ClubsDatabase::getClubMetadata] Fetching club metadata for a given clubId");
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdGetClubMetadataId);

        if (stmt != nullptr)
        {
            stmt->setUInt64(0, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);
            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    DbResult::const_iterator dbit = dbRes->begin();
                    DbRow *dbRow = *dbit;                        
                    clubDbRowToClubMetadataWithouBlazeId(dbRow, &clubMetadata);                   
                    result = ERR_OK;
                }
                else
                {
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                }                
            }
        }
    }

    return result;
}


BlazeRpcError ClubsDatabase::updateLastGame(ClubId clubId, ClubId opponentId, const char8_t* gameResult)
{
    TRACE_LOG("[ClubsDatabase::updateLastGame] club [clubid]: " << clubId << ", opponent [clubid] " 
              << opponentId << ", result: " << gameResult);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateLastGameId);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // lastOppo
            stmt->setUInt64(prm++, opponentId);
            // lastResult
            stmt->setString(prm++, gameResult);
            // clubId
            stmt->setUInt64(prm++, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubRecords(ClubId clubId, ClubRecordbookRecordMap &clubRecordMap)
{
    TRACE_LOG("[ClubsDatabase::getClubRecords] Fetching club records for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    // clear club list
    clubRecordMap.clear();

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_RECORDS, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    for ( DbResult::const_iterator it = dbRes->begin();
                            it != dbRes->end(); it++)
                    {
                        DbRow *dbRow = *it;

                        // fill club member struct
                        ClubRecordbookRecord cr;
                        cr.clubId = clubId;
                        cr.recordId = (dbRow->getUInt("recordId"));
                        cr.blazeId = (dbRow->getInt64("blazeId"));
                        cr.statType = (static_cast<RecordStatType>(dbRow->getInt("recordStatType")));
                        cr.statValueInt = (dbRow->getInt64("recordStatInt"));
                        cr.statValueFloat = (dbRow->getFloat("recordStatFloat"));
                        cr.lastUpdate = (dbRow->getUInt("lastupdate"));
                        clubRecordMap.insert(eastl::make_pair(cr.recordId, cr));
                        result = ERR_OK;
                    }
                }
                else
                {
                    // check that the club id is valid
                    Club club;
                    uint64_t version = 0;
                    result = getClub(clubId, &club, version);
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubRecord(ClubRecordbookRecord &clubRecord)
{
    TRACE_LOG("[ClubsDatabase::getClubRecords] Fetching club records for club [id]: " << clubRecord.clubId 
              << ", clubRecord [id] " << clubRecord.recordId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdGetClubRecord);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubRecord.clubId);
            // recordId
            stmt->setUInt32(prm++, clubRecord.recordId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                if (!dbresult->empty())
                {
                    DbRow *dbRow = *(dbresult->begin());
                    clubRecord.blazeId = (dbRow->getInt64("blazeId"));
                    clubRecord.statValueInt = (dbRow->getInt64("recordStatInt"));
                    clubRecord.statValueFloat = (dbRow->getFloat("recordStatFloat"));
                    clubRecord.statType = static_cast<RecordStatType>(dbRow->getInt("recordStatType"));
                    clubRecord.lastUpdate = (dbRow->getUInt("lastupdate"));
                }
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateClubRecord(ClubRecordbookRecord &clubRecord)
{
    TRACE_LOG("[ClubsDatabase::updatClubRecord] Updating club records for club [id]: " << clubRecord.clubId 
              << ", clubRecord [id] " << clubRecord.recordId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateClubRecord);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubRecord.clubId);
            // recordId
            stmt->setInt32(prm++, clubRecord.recordId);
            // blazeId
            stmt->setInt64(prm++, clubRecord.blazeId);

            switch (clubRecord.statType)
            {
            case CLUBS_RECORD_STAT_INT:
                {
                    // recordStatInt
                    stmt->setInt64(prm++, clubRecord.statValueInt);
                    // recordStatFloat
                    stmt->setFloat(prm++, static_cast<const float_t>(clubRecord.statValueInt));
                    // recordStatType
                    stmt->setInt32(prm++, static_cast<const int32_t>(clubRecord.statType));
                    break;
                }
            case CLUBS_RECORD_STAT_FLOAT:
                {
                    // recordStatInt
                    stmt->setInt32(prm++, static_cast<const int32_t>(clubRecord.statValueFloat));
                    // recordStatFloat
                    stmt->setFloat(prm++, clubRecord.statValueFloat);
                    // recordStatType
                    stmt->setInt32(prm++, static_cast<const int32_t>(clubRecord.statType));
                    break;
                }
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::resetClubRecords(ClubId clubId, const ClubRecordIdList &recordIdList)
{
    TRACE_LOG("[ClubsDatabase::resetClubRecords] Resetting club records for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            if (recordIdList.empty())
            {
                query->append(CLUBSDB_RESET_CLUB_RECORDS, clubId);
            }
            else
            {
                eastl::string idList, idStr;
                for (ClubRecordIdList::const_iterator it = recordIdList.begin();
                     it != recordIdList.end();
                     it++)
                {
                    if (it != recordIdList.begin())
                        idList.append(",");
                    idStr.sprintf("%" PRId64, *it);
                    idList.append(idStr);
                }
                query->append(CLUBSDB_RESET_CLUB_RECORDS_LIST, clubId, idList.c_str());
            }

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() == 0)
                {
                    // check that the club id is valid
                    Club club;
                    uint64_t version = 0;
                    result = getClub(clubId, &club, version);
                }
                else
                    result = Blaze::ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubAwards(ClubId clubId, ClubAwardList &awardList)
{
    TRACE_LOG("[ClubsDatabase::getClubAwards] Fetching club awards for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_AWARDS, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    // reserve the size with 1 allocation, note, it destroys all data already in EA::TDF::TdfStructVector...
                    awardList.reserve(dbRes->size());
                    for ( DbResult::const_iterator it = dbRes->begin();
                            it != dbRes->end(); it++)
                    {
                        DbRow *dbRow = *it;
                        uint32_t col = 0;
                        
                        // fill club member struct
                        ClubAward *ca = awardList.pull_back();
                        // awardid
                        ca->setAwardId(dbRow->getUInt(col++));
                        // awardCount
                        ca->setCount(dbRow->getUInt(col++));
                        // timestamp
                        ca->setLastUpdateTime(dbRow->getUInt(col++));
                    }
                    result = ERR_OK;
                }
                else
                {
                    // check that the club id is valid
                    Club club;
                    uint64_t version = 0;
                    result = getClub(clubId, &club, version);
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateClubAward(ClubId clubId, ClubAwardId awardId)
{
    TRACE_LOG("[ClubsDatabase::updateClubAward] Updating club award [id]: " << awardId << " for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateClubAward);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);
            // award
            stmt->setUInt32(prm++, awardId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() == 0)
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                else
                    result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateClubAwardCount(ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::updateClubAward] Updating club award count for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateClubAwardCount);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->getAffectedRowCount() == 0)
                    result = CLUBS_ERR_INVALID_CLUB_ID;
                else
                    result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateMemberMetaData(ClubId clubId, const ClubMember &member)
{
    TRACE_LOG("[ClubsDatabase::updateMemberMetaData] Updating metadata for member [id]: " << member.getUser().getBlazeId()
        << " in club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateMemberMetaData);
        
        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // metadata
            char8_t* pMetadata = BLAZE_NEW_ARRAY(char8_t, OC_MAX_MEMBER_METADATA_SIZE);
            decodeMemberMetaData(member.getMetaData(), pMetadata, OC_MAX_MEMBER_METADATA_SIZE);
            stmt->setString(prm++, pMetadata);
            stmt->setUInt64(prm++, clubId);
            stmt->setInt64(prm++, member.getUser().getBlazeId());

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);

            if (error == DB_ERR_OK)
            {
                result = Blaze::ERR_OK;
            }
            delete[] pMetadata;
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateSeasonState(SeasonState state, int64_t seasonStartTime)
{
    TRACE_LOG("[ClubsDatabase::updateSeasonState] Updating season state to [enum state]: " << state);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateSeasonState);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            stmt->setUInt32(prm++, static_cast<const uint32_t>(state));
            stmt->setInt64(prm++, seasonStartTime);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);

            if (error == DB_ERR_OK)
            {
                const uint32_t rowCount = dbRes->getAffectedRowCount();
                if (rowCount == 0)
                {
                    QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
                    query->append(CLUBSDB_INSERT_SEASON_STATE, state, seasonStartTime);
                    error = mDbConn->executeQuery(query);
                    if (error == DB_ERR_OK)
                    {
                        result = Blaze::ERR_OK;
                    }
                }
                else
                {
                    result = Blaze::ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::lockSeason()
{
    TRACE_LOG("[ClubsDatabase::lockSeason] Locking clubs_seasons table");
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        
        if (query != nullptr)
        {
            query->append(CLUBSDB_LOCK_SEASON);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                result = Blaze::ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getSeasonState(SeasonState &state, int64_t &lastUpdate)
{
    TRACE_LOG("[ClubsDatabase::getSeasonState] Fetching season state");
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdGetSeasonState);

        if (stmt != nullptr)
        {
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbRes);

            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    DbResult::const_iterator it = dbRes->begin();
                    DbRow *dbRow = *it;
                    uint32_t col = 0;
                    state = static_cast<SeasonState>(dbRow->getUInt(col++));
                    lastUpdate = dbRow->getInt64(col++);
                }
                else
                {
                    state = CLUBS_IN_SEASON;
                    lastUpdate = 0;
                }
                result = Blaze::ERR_OK;
            }
        }
    }

    return result;
}

int32_t ClubsDatabase::deleteAwards(ClubAwardIdList* awardIdList)
{
    int32_t deletedRows = -1;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_DELETE_AWARDS);

            ClubAwardIdList::const_iterator it = awardIdList->begin();
            while (it != awardIdList->end())
            {
                query->append("$u", *it++);
                if (it == awardIdList->end())
                    query->append(")");
                else
                    query->append(",");
            }
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                deletedRows = dbresult->getAffectedRowCount();
            }
        }
    }

    return deletedRows;
}


BlazeRpcError ClubsDatabase::listClubRivals(const ClubId clubId, ClubRivalList *clubRivalList)
{
    TRACE_LOG("[ClubsDatabase::listClubRivals] ClubId: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_LIST_CLUB_RIVALS, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
                if (dbresult->size() > 0)
                {
                    DbResult::const_iterator it = dbresult->begin();
                    for (it = dbresult->begin(); it != dbresult->end(); it++)
                    {
                        const DbRow *dbRow = *it;
    
                        uint32_t col = 0;
                        ClubRival *rival = clubRivalList->pull_back();

                        // rivalClubId
                        rival->setRivalClubId(dbRow->getUInt64(col++));
                        // custOpt1
                        rival->setCustOpt1(dbRow->getUInt64(col++));
                        // custOpt2
                        rival->setCustOpt2(dbRow->getUInt64(col++));
                        // custOpt3
                        rival->setCustOpt3(dbRow->getUInt64(col++));
                        // metadata
                        size_t len;
                        rival->setMetaData(reinterpret_cast<const char8_t*>(dbRow->getBinary(col++, &len)));
                        // creationTime
                        rival->setCreationTime(dbRow->getUInt(col++));
                        // lastUpdateTime
                        rival->setLastUpdateTime(dbRow->getUInt(col++));
                    }
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::insertRival(const ClubId clubId, const ClubRival &clubRival)
{
    TRACE_LOG("[ClubsDatabase::insertRival] Inserting rival [id] " << clubRival.getRivalClubId() << " for club [id] " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertRival);
        
        if (stmt != nullptr)
        {            
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);
            // rivalClubId
            stmt->setUInt64(prm++, clubRival.getRivalClubId());
            // custOpt1
            stmt->setUInt64(prm++, clubRival.getCustOpt1());
            // custOpt2
            stmt->setUInt64(prm++, clubRival.getCustOpt2());
            // custOpt3
            stmt->setUInt64(prm++, clubRival.getCustOpt3());
            // metadata
            stmt->setBinary(prm++, reinterpret_cast<const uint8_t*>(
                clubRival.getMetaData()), strlen(clubRival.getMetaData()) + 1);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
            else if (error == DB_ERR_DUP_ENTRY)
            {
                result = Blaze::CLUBS_ERR_DUPLICATE_RIVALS;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateRival(const ClubId clubId, const ClubRival &clubRival)
{
    TRACE_LOG("[ClubsDatabase::insertRival] Update rival [id] " << clubRival.getRivalClubId() << " for club [id] " << clubId);
        
    BlazeRpcError result = Blaze::ERR_SYSTEM;
    
    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdUpdateRival);
        
        if (stmt != nullptr)
        {            
            uint32_t prm = 0;
            // custOpt1
            stmt->setUInt64(prm++, clubRival.getCustOpt1());
            // custOpt2
            stmt->setUInt64(prm++, clubRival.getCustOpt2());
            // custOpt3
            stmt->setUInt64(prm++, clubRival.getCustOpt3());
            // metadata
            stmt->setBinary(prm++, reinterpret_cast<const uint8_t*>(
                clubRival.getMetaData()), strlen(clubRival.getMetaData()) + 1);
            // clubId
            stmt->setUInt64(prm++, clubId);
            // rivalClubId
            stmt->setUInt64(prm++, clubRival.getRivalClubId());
            // creationTime
            stmt->setInt32(prm++, static_cast<const int32_t>(clubRival.getCreationTime()));
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->getAffectedRowCount() == 0)
                    result = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
                else
                    result = ERR_OK;
            }
        }
    }    
    return result;
}

BlazeRpcError ClubsDatabase::deleteRivals(const ClubId clubId, const ClubIdList &rivalIds)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_DELETE_RIVALS, clubId);

            ClubIdList::const_iterator it = rivalIds.begin();
            if (it != rivalIds.end())
            {
                query->append(" AND rivalClubId IN ( ");
                while (it != rivalIds.end())
                {
                    query->append("$U", *it++);
                    if (it == rivalIds.end())
                        query->append(")");
                    else
                        query->append(",");
                }
            }

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                result = Blaze::ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::deleteOldestRival(const ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::deleteOldestRival] Delete oldest rival for club [id] " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdDeleteOldestRival);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);
            
            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::incrementRivalCount(const ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::incrementRivalCount] Incrementing rival count for club [id] " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdIncrementRivalCount);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::deleteOldestTickerMsg(const uint32_t timestamp)
{
    TRACE_LOG("[ClubsDatabase::deleteOldestRival] Delete oldest ticker msg after timestamp " << timestamp);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdDeleteOldestTickerMsg);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // timestamp
            stmt->setInt32(prm++, static_cast<const int32_t>(timestamp));

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::insertTickerMsg(
    const ClubId clubId, 
    const BlazeId includeBlazeId, 
    const BlazeId excludeBlazeId, 
    const char8_t *text,
    const char8_t *params,
    const char8_t *metadata)
{
    TRACE_LOG("[ClubsDatabase::insertRival] Inserting message ticker item forclub [id] " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mCmdInsertTickerMsg);

        if (stmt != nullptr)
        {
            uint32_t prm = 0;
            // clubId
            stmt->setUInt64(prm++, clubId);
            // includeBlazeId
            stmt->setInt64(prm++, includeBlazeId);
            // excludeBlazeId
            stmt->setInt64(prm++, excludeBlazeId);
            // text
            stmt->setString(prm++, text);
            // params
            stmt->setString(prm++, params);
            // metadata
            stmt->setString(prm++, metadata);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbresult);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getTickerMsgs(
    const ClubId clubId, 
    const BlazeId blazeId,
    const uint32_t timestamp,
    TickerMessageList &resultList)
{
    ClubIdList clubIdList;
    clubIdList.push_back(clubId);

    return getTickerMsgs(clubIdList, blazeId, timestamp, resultList);
}

BlazeRpcError ClubsDatabase::getTickerMsgs(
    const ClubIdList& requestorClubIdList, 
    const BlazeId blazeId,
    const uint32_t timestamp,
    TickerMessageList &resultList)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_TICKERMSGS, blazeId, blazeId);

            ClubIdList::const_iterator iter = requestorClubIdList.begin();
            ClubIdList::const_iterator iterEnd = requestorClubIdList.end();

            if (iter == iterEnd)
            {
                query->append(")");
            }
            else
            {
                // Since there is already one value in the query IN(0
                // Need to append "," at the beginning.
                query->append(",");

                while (iter != iterEnd)
                {
                    ClubId clubId = *iter++;
                    TRACE_LOG("[ClubsDatabase::getTickerMsgs] Fetching club ticker messages  for club [id]: " << clubId);
                    query->append("$U", clubId);
                    if (iter == requestorClubIdList.end())
                        query->append(")");
                    else
                        query->append(",");
                }
            }

            if (timestamp > 0)
                query->append(" AND `timestamp` >= FROM_UNIXTIME($u) ", timestamp); 
            query->append(" ORDER BY `timestamp` DESC");

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    // reserve the size with 1 allocation, note, it destroys all data already in EA::TDF::TdfStructVector...
                    for ( DbResult::const_iterator it = dbRes->begin();
                            it != dbRes->end(); it++)
                    {
                        DbRow *dbRow = *it;
                        uint32_t col = 0;

                        TickerMessage item;
                        // club id
                        item.mClubId = dbRow->getUInt64(col++);
                        // text
                        item.mText = dbRow->getString(col++);
                        // params
                        item.mParams = dbRow->getString(col++);
                        // metadata
                        item.mMetadata = dbRow->getString(col++);
                        // timestamp
                        item.mTimestamp = dbRow->getUInt(col++);
                        resultList.push_back(item);
                    }
                    result = ERR_OK;
                }
                else
                {
                    // check that the club id is valid
                    ClubList clubList;
                    uint64_t* versions = nullptr;
                    result = getClubs(requestorClubIdList, clubList, versions);

                    if (versions != nullptr)
                        delete [] versions;
                };
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::changeClubStrings(
    const ClubId clubId, 
    const char8_t* name,
    const char8_t* nonUniqueName,
    const char8_t* description,
    uint64_t& version)
{
    TRACE_LOG("[ClubsDatabase].changeClubStrings: Changing strings for club [id]: " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (name == nullptr && nonUniqueName == nullptr && description == nullptr)
        return Blaze::ERR_OK; // nothing to do...

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append( "UPDATE `clubs_clubs` SET `version` = `version` + 1 ");

            if (name != nullptr )
                query->append(", `name` = '$s' ", name);
            else    
                query->append(", `name` = `name` ");

            if (nonUniqueName != nullptr )
                query->append(", `nonuniquename` = '$s' ", nonUniqueName);
            else
                query->append(", `nonuniquename` = `nonuniquename` ");

            if (description != nullptr )
                query->append(", `description` = '$s' ", description);
            else
                query->append(", `description` = `description` ");

			query->append(", `clubNameResetCount` = `clubNameResetCount` + 1 ");

            query->append("  WHERE `clubId` = $U;", clubId);
            query->append("SELECT `version` FROM `clubs_clubs` WHERE `clubId` = $U;", clubId);

            DbResultPtrs dbResults;
            BlazeRpcError error = mDbConn->executeMultiQuery(query, dbResults);

            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
                if (dbResults.size() != 2)
                {
                    ERR_LOG("[ClubsDatabase].changeClubStrings: Incorrect number of results returned from multi query");
                    return Blaze::ERR_SYSTEM;
                }

                DbResultPtr dbResult = dbResults[1];
                if (dbResult->size() != 1)
                {
                    ERR_LOG("[ClubsDatabase].changeClubStrings: Incorrect number of rows returned for result");
                    return Blaze::ERR_SYSTEM;
                }

                DbRow *row = *dbResult->begin();
                version = row->getUInt64(uint32_t(0));
            }
            else
            {
                // is name duplicated?
                if (error == DB_ERR_DUP_ENTRY)
                    result = Blaze::CLUBS_ERR_CLUB_NAME_IN_USE;
                else
                {
                    // check that the club id is valid
                    Club club;
                    result = getClub(clubId, &club, version);
                    if (result == Blaze::ERR_OK)
                        result = ERR_SYSTEM;    // we don't know what's wrong
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::changeIsNameProfaneFlag(
	const ClubId clubId,
	uint32_t isNameProfane)
{
	TRACE_LOG("[ClubsDatabase].changeIsNameProfaneFlag: for club [id]: " << clubId);
	BlazeRpcError result = Blaze::ERR_SYSTEM;

	if (mDbConn != nullptr)
	{
		QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

		if (query != nullptr)
		{
			query->append("UPDATE `clubs_clubs` SET `isNameProfane` = '$U' ", isNameProfane);
		}
		query->append("  WHERE `clubId` = $U;", clubId);

		DbResultPtrs dbResults;
		BlazeRpcError error = mDbConn->executeMultiQuery(query, dbResults);

		if (error == DB_ERR_OK)
		{
			result = ERR_OK;
		}
	}

	TRACE_LOG("[ClubsDatabase].changeIsNameProfaneFlag: result = " << result);

	return result;
}

BlazeRpcError ClubsDatabase::countMessages(
    const ClubId clubId, 
    const MessageType messageType, 
    uint32_t &messageCount)
{
    TRACE_LOG("[ClubsDatabase::countMessages] [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_COUNT_MESSAGES, clubId, static_cast<uint32_t>(messageType));

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->size() == 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_INVALID_CLUB_ID);
                }
                else
                {
                    DbRow *row = *dbresult->begin();
                    uint16_t col = 0;
                    messageCount = (uint16_t)row->getUInt(col);
                    result = ERR_OK;
                 }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::countMessages(
    const ClubIdList& clubIdList, 
    const MessageType messageType, 
    ClubMessageCountMap &messageCountMap)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_COUNT_MESSAGES_FOR_CLUBS, static_cast<uint32_t>(messageType));

            ClubIdList::const_iterator it = clubIdList.begin();
            while(it != clubIdList.end())
            {
                TRACE_LOG("[ClubsDatabase::countMessages] [clubId]: " << *it);

                if (it != clubIdList.begin())
                    query->append(",");

                query->append("$U", *it++);

                if (it == clubIdList.end())
                    query->append(")");
            }

            query->append( " GROUP BY clubId ");

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                if (dbresult->size() != 0)
                {
                    for ( DbResult::const_iterator itr = dbresult->begin(), end = dbresult->end(); itr != end ; itr++)
                    {
                        DbRow *dbRow = *itr;
                        uint16_t col = 0;
                        ClubId clubId = dbRow->getUInt64(col++);
                        uint32_t messageCount = (uint16_t)dbRow->getUInt(col);

                        messageCountMap[clubId] = messageCount;
                    }
                }

                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getMembershipInClub(const ClubId clubId, const BlazeId blazeId, ClubMembership &clubMembership)
{
    TRACE_LOG("[ClubsDatabase::getMembershipInClub] Fetching club membership for user [blazeId]: " << blazeId << " in club " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_MEMBERSHIP_IN_CLUB, blazeId, clubId);

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
        
            if (error == DB_ERR_OK)
            {
                if (dbRes->empty())
                {
                    result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
                }
                else if (dbRes->size() > 1)
                {
                    result = Blaze::ERR_SYSTEM;
                }
                else
                {
                    DbResult::const_iterator it = dbRes->begin();
                    DbRow *dbRow = *it;

                    // fill club membership struct
                    clubDbRowToClubMembership(dbRow, &clubMembership);

                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}


BlazeRpcError ClubsDatabase::getMembershipInClub(const ClubId clubId, const BlazeIdList& blazeIds, MembershipStatusMap &clubMembershipStatusMap)
{
    TRACE_LOG("[ClubsDatabase::getMembershipInClub] Fetching club membership for users in club " << clubId);
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_MEMBERSHIP_IN_CLUB_FOR_USERS, clubId);

            BlazeIdList::const_iterator it = blazeIds.begin();
            while (it != blazeIds.end())
            {
                query->append("$D", *it++);
                if (it == blazeIds.end())
                    query->append(")");
                else
                    query->append(",");
            }

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);

            if (error == DB_ERR_OK)
            {
                if (dbRes->empty())
                {
                    result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
                }
                else
                {
                    for (DbResult::const_iterator i = dbRes->begin(), e = dbRes->end(); i != e; ++i)
                    {
                        DbRow *dbRow = *i;
                        ClubMembership clubMembership;
                        // fill club membership struct
                        clubDbRowToClubMembership(dbRow, &clubMembership);
                        MembershipStatusValue statusValue = clubMembership.getClubMember().getMembershipStatus();

                        clubMembershipStatusMap[clubMembership.getClubMember().getUser().getBlazeId()] = statusValue;
                    }

                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubBans(
    const ClubId clubId, 
    BlazeIdToBanStatusMap* blazeIdToBanStatusMap)
{
    TRACE_LOG("[ClubsDatabase::getClubBans] [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;
       
    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUB_BANS, clubId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    for (DbResult::const_iterator dbit = dbRes->begin(); dbit != dbRes->end(); ++dbit)
                    {
                        const DbRow* dbRow = *dbit;
                        const uint32_t blazeIdCol = 0;
                        const BlazeId blazeId = dbRow->getInt64(blazeIdCol);
                        const uint32_t banStatusCol = 1;
                        const BanStatus banStatus = dbRow->getUInt(banStatusCol);
                        (*blazeIdToBanStatusMap)[blazeId] = banStatus;
                    }

                    result = Blaze::ERR_OK;
                }
                else
                {
                    result = checkClubExists(clubId);
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getUserBans(
    const BlazeId blazeId, 
    ClubIdToBanStatusMap* clubIdToBanStatusMap)
{
    TRACE_LOG("[ClubsDatabase::getUserBans] [blazeId]: " << blazeId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_USER_BANS, blazeId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    for (DbResult::const_iterator dbit = dbRes->begin(); dbit != dbRes->end(); ++dbit)
                    {
                        const DbRow* dbRow = *dbit;
                        const uint32_t clubIdCol = 0;
                        const ClubId clubId = dbRow->getUInt64(clubIdCol);
                        const uint32_t banStatusCol = 1;
                        const BanStatus banStatus = dbRow->getUInt(banStatusCol);
                        (*clubIdToBanStatusMap)[clubId] = banStatus;
                    }

                    result = Blaze::ERR_OK;
                }
                else
                {
                    result = checkUserExists(blazeId);
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getBan(
     const ClubId clubId, 
     const BlazeId blazeId,
     BanStatus* banStatus)
{
    TRACE_LOG("[ClubsDatabase::getBan] [clubId]: " << clubId << ", [blazeId]: " << blazeId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_BAN, clubId, blazeId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->size() == 1)
                {
                    result = Blaze::ERR_OK;

                    const DbRow* dbRow = *(dbRes->begin());
                    const uint32_t banStatusCol = 0;
                    *banStatus = dbRow->getUInt(banStatusCol);
                }
                else
                {
                    result = checkClubAndUserExist(clubId, blazeId);

                    if (result == Blaze::ERR_OK)
                    {
                        *banStatus = CLUBS_NO_BAN;
                    }
                }
            }
        }
    }    

    return result;
}

BlazeRpcError ClubsDatabase::addBan(
    const ClubId clubId, 
    const BlazeId blazeId,
    const BanStatus banStatus)
{
    TRACE_LOG("[ClubsDatabase::addBan] [clubId]: " << clubId << ", [blazeId]: " << blazeId << ", [banStatus]: " << banStatus);

    BlazeRpcError result = checkClubAndUserExist(clubId, blazeId);

    if (result == Blaze::ERR_OK)
    {
        if (mDbConn != nullptr)
        {
            QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
            if (query != nullptr)
            {
                query->append(CLUBSDB_ADD_BAN, clubId, blazeId, banStatus);
                DbResultPtr dbRes;
                BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
                if (error == DB_ERR_OK)
                {
                    result = Blaze::ERR_OK;
                }
                else if (error == DB_ERR_DUP_ENTRY)
                {
                    result = CLUBS_ERR_USER_BANNED;
                }

                // Club and user existence is checked upfront, so they shouldn't 
                // be the problem here - no need to recheck.
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::removeBan(
    const ClubId clubId, 
    const BlazeId blazeId)
{
    TRACE_LOG("[ClubsDatabase::removeBan] [clubId]: " << clubId << ", [blazeId]: " << blazeId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_REMOVE_BAN, clubId, blazeId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                result = Blaze::ERR_OK;
            }
            else
            {
                result = checkClubAndUserExist(clubId, blazeId);
            }
        }
    }

    return result;
}

inline BlazeRpcError ClubsDatabase::checkClubExists(ClubId clubId)
{
    Club club;
    uint64_t version = 0;
    return getClub(clubId, &club, version);
}

inline BlazeRpcError ClubsDatabase::checkUserExists(BlazeId blazeId) const
{
    UserInfoPtr userInfo = nullptr;
    BlazeRpcError existError = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
    return (existError == USER_ERR_USER_NOT_FOUND) ? CLUBS_ERR_INVALID_USER_ID : existError;
}

BlazeRpcError ClubsDatabase::checkClubAndUserExist(ClubId clubId, BlazeId blazeId)
{
    // Check if the club and user IDs are valid.
    BlazeRpcError existError = checkClubExists(clubId);
    if (existError == Blaze::ERR_OK)
    {
        existError = checkUserExists(blazeId);
    }
    return existError;
}

BlazeRpcError ClubsDatabase::requestorIsGmOfUser(BlazeId requestorId, BlazeId blazeId, ClubIdList *clubsIdsInWhichGm)
{
    TRACE_LOG("[ClubsDatabase::requestorIsGmOfUser] [requestorId]: " << requestorId << ", [blazeId] " << blazeId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_CLUBS_IN_WHICH_REQUESTOR_IS_GM_OF_USER, requestorId, static_cast<uint32_t>(CLUBS_GM), blazeId);
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (!dbRes->empty())
                {
                    for (DbResult::const_iterator dbit = dbRes->begin(); dbit != dbRes->end(); ++dbit)
                    {
                        const DbRow* dbRow = *dbit;
                        const uint32_t clubIdCol = 0;
                        const ClubId clubId = dbRow->getUInt64(clubIdCol);
                        clubsIdsInWhichGm->push_back(clubId);
                    }

                    result = Blaze::ERR_OK;
                }
                else
                {
                    result = checkUserExists(requestorId);
                    if (result == Blaze::ERR_OK)
                    {
                        result = checkUserExists(blazeId);
                    }
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::getClubsComponentInfo(
    ClubsComponentInfo &info)
{
    TRACE_LOG("[ClubsDatabase::getClubsComponentInfo]");

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_COUNT_CLUBS);

            DbResultPtr dbresult;
            BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
            if (error == DB_ERR_OK)
            {
                info.setClubsCount(0);
                info.setMembersCount(0);
                if (dbresult->size() != 0)
                {
                    DbResult::const_iterator itr = dbresult->begin();
                    DbResult::const_iterator end = dbresult->end();
                    for (; itr != end; ++itr)
                    {
                        DbRow *dbrow = *itr;
                        ClubDomainId clubDomainId = dbrow->getUInt("domainId");
                        uint32_t clubCount = dbrow->getUInt("clubCount");
                        uint32_t memberCount = dbrow->getUInt("memberCount");
                        (info.getClubsByDomain())[clubDomainId] = clubCount;
                        (info.getMembersByDomain())[clubDomainId] = memberCount;
                        info.setClubsCount(info.getClubsCount() + clubCount);
                        info.setMembersCount(info.getMembersCount() + memberCount);
                    }
                }
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::findTag(const ClubTag& tagText, ClubTagId& tagId)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        if (query != nullptr)
        {
            query->append(CLUBSDB_SELECT_TAG, tagText.c_str());
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->size() == 0)
                {
                    result = static_cast<BlazeRpcError>(CLUBS_ERR_TAG_TEXT_NOT_FOUND);
                }
                else
                {
                    const DbRow *dbrow = *dbRes->begin();
                    tagId = dbrow->getUInt(0U);
                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::insertTag(const ClubTag& tagText, ClubTagId& tagId)
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        if (query != nullptr)
        {
            query->append(CLUBSDB_INSERT_TAG, tagText.c_str());
            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->getLastInsertId() > 0)
                {
                    tagId = static_cast<ClubTagId>(dbRes->getLastInsertId());
                    result = ERR_OK;
                }
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::insertClubTags(const ClubId clubId, const ClubTagList& clubTagList)
{
    if (clubTagList.empty())
        return ERR_OK;

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
        if (query != nullptr)
        {
            ClubTagIdList tagIdList;
            for (ClubTagList::const_iterator it = clubTagList.begin(); it != clubTagList.end(); it++)
            {
                ClubTagId tagId = 0;
                BlazeRpcError findTagError = findTag(*it, tagId);
                if (findTagError == ERR_OK)
                {
                    // maybe the tag id has existed, in case of repeated tags in tagList
                    ClubTagIdList::iterator itr = tagIdList.begin();
                    for (; itr != tagIdList.end(); itr++)
                    {
                        if (tagId == *itr)
                            break;
                    }
                    if (itr == tagIdList.end())
                        tagIdList.push_back(tagId);
                }
                else if (findTagError == CLUBS_ERR_TAG_TEXT_NOT_FOUND)
                {
                    BlazeRpcError insertTagError = insertTag(*it, tagId);
                    if (insertTagError == ERR_OK)
                        tagIdList.push_back(tagId);
                    else
                    {
                        return insertTagError;
                    }
                }
                else
                {
                    return findTagError;
                }
            }
            
            if (tagIdList.size() > 0)
            {
                query->append(CLUBSDB_INSERT_CLUB_TAGS);
                for (ClubTagIdList::iterator it = tagIdList.begin(); it != tagIdList.end(); it++)
                {
                    if (it + 1 != tagIdList.end())
                        query->append("($U,$u),", clubId, *it);
                    else
                        query->append("($U,$u)", clubId, *it);
                }
            }

            BlazeRpcError error = mDbConn->executeQuery(query);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::removeClubTags(const ClubId clubId)
{
    TRACE_LOG("[ClubsDatabase::updateClubTags] [clubId]: " << clubId);

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_REMOVE_CLUB_TAGS, clubId);

            BlazeRpcError error = mDbConn->executeQuery(query);
            if (error == DB_ERR_OK)
            {
                result = ERR_OK;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsDatabase::updateClubTags(const ClubId clubId, const ClubTagList& clubTagList)
{
    BlazeRpcError result = removeClubTags(clubId);
    if (result == ERR_OK)
        return insertClubTags(clubId, clubTagList);
    else
        return result;
}
BlazeRpcError ClubsDatabase::getTagsForClubs(ClubList& clubList)
{
    if (clubList.empty())
        return ERR_OK;

    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mDbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != nullptr)
        {
            query->append(CLUBSDB_GET_CLUBS_TAGS);
       
            for (ClubList::const_iterator it = clubList.begin(); it != clubList.end(); it++)
            {
                if (it + 1 != clubList.end())
                    query->append("$U,", (*it)->getClubId());
                else
                    query->append("$U)", (*it)->getClubId());
            }

            DbResultPtr dbRes;
            BlazeRpcError error = mDbConn->executeQuery(query, dbRes);
            if (error == DB_ERR_OK)
            {
                if (dbRes->size() > 0)
                {
                    for (ClubList::iterator clubIt = clubList.begin(); clubIt != clubList.end(); ++clubIt)
                    {
                        for (DbResult::const_iterator dbIt = dbRes->begin(); dbIt != dbRes->end(); ++dbIt)
                        {
                            const DbRow *dbrow = *dbIt;
                            uint32_t col = 0;
                            if (dbrow->getUInt64(col) == (*clubIt)->getClubId())
                                (*clubIt)->getTagList().push_back(dbrow->getString(col + 1));
                        }
                    }
                }

                result = ERR_OK;
            }
        }
    }

    return result;
}

} // Clubs

} // Blaze

