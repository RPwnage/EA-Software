#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    DROP TABLE IF EXISTS clubs_awards;
    CREATE TABLE `clubs_awards` (
      `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `awardId` int(10) unsigned NOT NULL,
      `awardCount` int(10) unsigned NOT NULL DEFAULT '1',
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (`id`),
      UNIQUE KEY `index_clubs_awards_clubid_awardId` (`clubId`,`awardId`),
      KEY `index_clubs_awards_awardId` (`awardId`),
      CONSTRAINT `fk_clubs_awards_clubs_clubs` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_bans;
    CREATE TABLE `clubs_bans` (
      `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `userId` bigint(20) NOT NULL,
      `banStatus` int(10) unsigned NOT NULL,
      PRIMARY KEY (`id`),
      UNIQUE KEY `index_clubs_bans_clubId_userId` (`clubId`,`userId`),
      KEY `index_clubs_bans_userId` (`userId`),
      CONSTRAINT `clubs_bans_clubs_clubs_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_club2tag;
    CREATE TABLE `clubs_club2tag` (
      `clubId` bigint(20) unsigned NOT NULL,
      `tagId` int(10) unsigned NOT NULL,
      PRIMARY KEY (`clubId`,`tagId`),
      KEY `tagId` (`tagId`),
      CONSTRAINT `fk_clubs_club2tag_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE,
      CONSTRAINT `fk_clubs_club2tag_tagId` FOREIGN KEY (`tagId`) REFERENCES `clubs_tags` (`tagId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_clubs;
    CREATE TABLE `clubs_clubs` (
      `clubId` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
      `domainId` int(10) unsigned NOT NULL DEFAULT '0',
      `name` varchar(30) NOT NULL,
      `creationTime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
      `lastActiveTime` datetime NOT NULL DEFAULT '1970-01-01 00:00:01',
      `memberCount` int(10) unsigned NOT NULL DEFAULT '0',
      `gmCount` int(10) unsigned NOT NULL DEFAULT '0',
      `awardCount` int(10) unsigned NOT NULL DEFAULT '0',
      `lastOppo` int(10) unsigned DEFAULT NULL,
      `lastResult` varchar(32) NOT NULL DEFAULT '',
      `lastGameTime` datetime DEFAULT '0000-00-00 00:00:00',
      `rivalCount` int(10) unsigned NOT NULL DEFAULT '0',
      `teamId` int(10) unsigned NOT NULL,
      `language` char(3) NOT NULL,
      `nonuniquename` varchar(30) NOT NULL,
      `description` varchar(65) NOT NULL,
      `region` int(10) unsigned NOT NULL,
      `logoId` int(10) unsigned NOT NULL,
      `bannerId` int(10) unsigned NOT NULL,
      `acceptanceFlags` int(10) unsigned NOT NULL,
      `artPackType` int(10) unsigned NOT NULL,
      `password` varchar(32) NOT NULL,
      `seasonLevel` int(10) unsigned NOT NULL DEFAULT '0',
      `previousSeasonLevel` int(10) unsigned NOT NULL DEFAULT '0',
      `lastSeasonLevelUpdate` int(10) unsigned NOT NULL DEFAULT '0',
      `metaData` varbinary(2048) NOT NULL,
      `metaDataType` int(10) unsigned NOT NULL,
      `metaData2` varbinary(2048) NOT NULL,
      `metaDataType2` int(10) unsigned NOT NULL,
      `joinAcceptance` int(10) unsigned NOT NULL,
      `skipPasswordCheckAcceptance` int(10) unsigned NOT NULL,
      `petitionAcceptance` int(10) unsigned NOT NULL,
      `custOpt1` int(10) unsigned NOT NULL,
      `custOpt2` int(10) unsigned NOT NULL,
      `custOpt3` int(10) unsigned NOT NULL,
      `custOpt4` int(10) unsigned NOT NULL,
      `custOpt5` int(10) unsigned NOT NULL,
      `version` bigint(20) unsigned NOT NULL DEFAULT '0',
     PRIMARY KEY (`clubId`),
      UNIQUE KEY `index_clubs_clubs_name` (`name`),
      KEY `index_clubs_clubs_memberCount` (`memberCount`),
      KEY `index_clubs_clubs_teamId` (`teamId`),
      KEY `index_clubs_clubs_language` (`language`),
      KEY `index_clubs_clubs_region` (`region`),
      KEY `index_clubs_clubs_acceptanceFlags` (`acceptanceFlags`),
      KEY `index_clubs_clubs_customOpt1` (`custOpt1`),
      KEY `index_clubs_clubs_custOpt2` (`custOpt2`),
      KEY `index_clubs_clubs_custOpt3` (`custOpt3`),
      KEY `index_clubs_clubs_custOpt4` (`custOpt4`),
      KEY `index_clubs_clubs_custOpt5` (`custOpt5`),
      KEY `index_clubs_clubs_lastActiveTime` (`lastActiveTime`),
      KEY `index_clubs_clubs_seasonLevel` (`seasonLevel`),
      KEY `index_clubs_clubs_nonuniquename` (`nonuniquename`) USING BTREE,
      KEY `index_clubs_clubs_lastGameTime` (`lastGameTime`) USING BTREE,
      KEY `index_clubs_clubs_creationTime` (`creationTime`) USING BTREE,
      KEY `index_clubs_clubs_domainId` (`domainId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_members;
    CREATE TABLE `clubs_members` (
      `blazeId` bigint(20) unsigned NOT NULL,
      `clubId` bigint(20) unsigned NOT NULL,
      `membershipStatus` int(10) unsigned NOT NULL,
      `memberSince` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
      `metadata` varchar(1000) DEFAULT NULL,
      PRIMARY KEY (`blazeId`,`clubId`),
      KEY `index_clubs_members_clubId` (`clubId`),
      KEY `index_clubs_members_memberSince` (`memberSince`) USING BTREE,
      CONSTRAINT `fk_clubs_members_clubs_clubs_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_messages;
    CREATE TABLE `clubs_messages` (
      `messageId` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `messageType` int(10) unsigned NOT NULL,
      `sender` bigint(20) unsigned NOT NULL,
      `receiver` bigint(20) unsigned NOT NULL,
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`messageId`),
      KEY `index_clubs_messages_sender` (`sender`),
      KEY `index_clubs_messages_receiver` (`receiver`),
      KEY `index_clubs_messages_clubId_messageType` (`clubId`,`messageType`),
      CONSTRAINT `fk_clubs_messages_clubs_clubs_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_news;
    CREATE TABLE `clubs_news` (
      `clubId` bigint(20) unsigned NOT NULL,
      `associatedClubId` bigint(20) unsigned NOT NULL,
      `newsType` int(10) unsigned NOT NULL,
      `contentCreator` bigint(20) unsigned NOT NULL,
      `text` varchar(2048) NOT NULL,
      `stringId` varchar(128) NOT NULL,
      `paramList` varchar(256) NOT NULL,
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      `flags` int(10) unsigned NOT NULL DEFAULT '0',
      `newsId` int(10) unsigned NOT NULL AUTO_INCREMENT,
      PRIMARY KEY (`newsId`),
      KEY `index_clubs_news_newsType` (`newsType`),
      KEY `index_clubs_news_timestamp` (`timestamp`),
      KEY `fk_clubs_news_clubs_clubs` (`clubId`),
      KEY `fk_clubs_news_associatedClubId_clubs_clubs_clubsId` (`associatedClubId`),
      CONSTRAINT `fk_clubs_news_associatedClubId_clubs_clubs_clubsId` FOREIGN KEY (`associatedClubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE,
      CONSTRAINT `fk_clubs_news_clubs_clubs` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_recordbooks;
    CREATE TABLE `clubs_recordbooks` (
      `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `recordId` int(10) unsigned NOT NULL,
      `blazeId` bigint(20) unsigned NOT NULL,
      `recordStatInt` bigint(20) NOT NULL DEFAULT '0',
      `recordStatFloat` float NOT NULL DEFAULT '0',
      `recordStatType` int(11) NOT NULL DEFAULT '0',
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (`id`),
      UNIQUE KEY `clubId` (`clubId`,`recordId`),
      KEY `index_clubs_recordbooks_blazeId` (`blazeId`) USING BTREE,
      CONSTRAINT `fk_clubs_recordbooks_clubs_clubs_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_rivals;
    CREATE TABLE `clubs_rivals` (
      `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `rivalClubId` bigint(20) unsigned NOT NULL,
      `custOpt1` bigint(20) unsigned NOT NULL,
      `custOpt2` bigint(20) unsigned NOT NULL,
      `custOpt3` bigint(20) unsigned NOT NULL,
      `metadata` varbinary(1024) NOT NULL,
      `creationTime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      `lastUpdateTime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
      PRIMARY KEY (`id`),
      KEY `index_clubs_rivals_creationTime` (`creationTime`),
      KEY `index_clubs_rivals_lastUpdateTime` (`lastUpdateTime`),
      KEY `index_clubs_rivals_rivalClubId` (`rivalClubId`),
      KEY `fk_clubs_rivals_clubs_clubs_clubId` (`clubId`),
      CONSTRAINT `fk_clubs_rivals_clubs_clubs_clubId` FOREIGN KEY (`clubId`) REFERENCES `clubs_clubs` (`clubId`) ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_seasons;
    CREATE TABLE `clubs_seasons` (
      `state` int(10) unsigned NOT NULL DEFAULT '0',
      `lastUpdated` datetime NOT NULL DEFAULT '0000-00-00 00:00:00'
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_tags;
    CREATE TABLE `clubs_tags` (
      `tagId` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `tagText` varchar(255) NOT NULL,
      PRIMARY KEY (`tagId`),
      UNIQUE KEY `tagText` (`tagText`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_tickermsgs;
    CREATE TABLE `clubs_tickermsgs` (
      `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
      `clubId` bigint(20) unsigned NOT NULL,
      `includeUserId` bigint(20) unsigned NOT NULL,
      `excludeUserId` bigint(20) unsigned NOT NULL,
      `text` varchar(2048) NOT NULL,
      `params` varchar(256) NOT NULL,
      `metadata` varchar(256) NOT NULL,
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`id`),
      KEY `index_clubs_tickermsgs_clubid` (`clubId`),
      KEY `index_clubs_tickermsgs_timestamp` (`timestamp`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS clubs_lastleavetime;
    CREATE TABLE `clubs_lastleavetime` (
      `blazeId` bigint(20) unsigned NOT NULL,
      `domainId` int(10) unsigned NOT NULL, 
      `lastLeaveTime` datetime NOT NULL DEFAULT '1970-01-01 00:00:01',
      PRIMARY KEY (`blazeId`, `domainId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
