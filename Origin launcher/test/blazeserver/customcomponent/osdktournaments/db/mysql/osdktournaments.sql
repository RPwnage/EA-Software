#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    DROP TABLE IF EXISTS osdktournaments_matches;
    CREATE TABLE `osdktournaments_matches` (
      `matchId` int(10) unsigned NOT NULL auto_increment,
      `userOneBlazeId` bigint(20) unsigned NOT NULL,
      `userTwoBlazeId` bigint(20) unsigned NOT NULL,
      `userOneTeam` int(10) NOT NULL,
      `userTwoTeam` int(10) NOT NULL,
      `userOneScore` int(10) unsigned NOT NULL,
      `userTwoScore` int(10) unsigned NOT NULL,
      `userOneLastMatchId` int(10) unsigned default '0',
      `userTwoLastMatchId` int(10) unsigned default '0',
      `userOneMetaData` varchar(256) default NULL,
      `userTwoMetaData` varchar(256) default NULL,
      `userOneAttribute` varchar(32) default NULL,
      `userTwoAttribute` varchar(32) default NULL,
      `treeOwnerBlazeId` bigint(20) unsigned NOT NULL,
      PRIMARY KEY  (`matchId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS osdktournaments_members; 
    CREATE TABLE `osdktournaments_members` (
      `blazeId` bigint(20) unsigned NOT NULL,
      `tournamentId` int(10) unsigned NOT NULL,
      `isActive` int(11) NOT NULL default '1',
      `lastMatchId` int(10) unsigned default '0',
      `level` int(10) unsigned NOT NULL default '1',
      `attribute` varchar(32) NOT NULL,
      `team` int(10) NOT NULL default '-1',
      PRIMARY KEY  (`blazeId`),
      KEY `FK_osdktourn_members_userInfo_blazeId` (`tournamentId`),
      CONSTRAINT `FK_osdktourn_members_tourn_tournaments_tournamentId` FOREIGN KEY (`tournamentId`) REFERENCES `osdktournaments_tournaments` (`tournamentId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS osdktournaments_tournaments;
    CREATE TABLE `osdktournaments_tournaments` (
      `tournamentId` int(10) unsigned NOT NULL auto_increment,
      `name` varchar(64) NOT NULL,
      `description` varchar(128) default NULL,
      `trophyName` varchar(32) NOT NULL,
      `trophyMetaData` varchar(64) default NULL,
      `numRounds` int(10) unsigned NOT NULL,
      PRIMARY KEY  (`tournamentId`),
      UNIQUE KEY `index_osdktourn_tournaments_name` USING BTREE (`name`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS osdktournaments_trophies;
    CREATE TABLE `osdktournaments_trophies` (
      `blazeId` bigint(20) unsigned NOT NULL,
      `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
      `tournamentId` int(10) unsigned NOT NULL,
      `awardCount` int(10) unsigned NOT NULL default '1',
      PRIMARY KEY  (`blazeId`,`tournamentId`),
      KEY `FK_osdktourn_trophies_tourn_tournaments_tournamentId` (`tournamentId`),
      CONSTRAINT `FK_osdktourn_trophies_tourn_tournaments_tournamentId` FOREIGN KEY (`tournamentId`) REFERENCES `osdktournaments_tournaments` (`tournamentId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

    ALTER TABLE `osdktournaments_matches` ADD INDEX `index_osdktourn_matches_treeOwnerBlazeId`(`treeOwnerBlazeId`);
    ALTER TABLE `osdktournaments_trophies` ADD INDEX `index_osdktourn_trophies_tournamentId`(`tournamentId`);
    ALTER TABLE `osdktournaments_trophies` ADD INDEX `index_osdktourn_trophies_timestamp`(`timestamp` DESC);
    
    ALTER TABLE `osdktournaments_matches` CHANGE COLUMN `userOneBlazeId` `memberOneId` BIGINT(20) UNSIGNED NOT NULL,
        CHANGE COLUMN `userTwoBlazeId` `memberTwoId` BIGINT(20) UNSIGNED NOT NULL,
        CHANGE COLUMN `userOneTeam` `memberOneTeam` INT(10) NOT NULL,
        CHANGE COLUMN `userTwoTeam` `memberTwoTeam` INT(10) NOT NULL,
        CHANGE COLUMN `userOneScore` `memberOneScore` INTEGER UNSIGNED NOT NULL,
        CHANGE COLUMN `userTwoScore` `memberTwoScore` INTEGER UNSIGNED NOT NULL,
        CHANGE COLUMN `userOneLastMatchId` `memberOneLastMatchId` INTEGER UNSIGNED DEFAULT 0,
        CHANGE COLUMN `userTwoLastMatchId` `memberTwoLastMatchId` INTEGER UNSIGNED DEFAULT 0,
        CHANGE COLUMN `userOneMetaData` `memberOneMetaData` VARCHAR(256) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL,
        CHANGE COLUMN `userTwoMetaData` `memberTwoMetaData` VARCHAR(256) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL,
        CHANGE COLUMN `userOneAttribute` `memberOneAttribute` VARCHAR(32) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL,
        CHANGE COLUMN `userTwoAttribute` `memberTwoAttribute` VARCHAR(32) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL,
        CHANGE COLUMN `treeOwnerBlazeId` `treeOwnerMemberId` BIGINT(20) UNSIGNED NOT NULL, 
        DROP INDEX `index_osdktourn_matches_treeOwnerBlazeId`,
        ADD INDEX `index_osdktourn_matches_treeOwnerMemberId` USING BTREE(`treeOwnerMemberId`);
    
    ALTER TABLE `osdktournaments_members` CHANGE COLUMN `blazeId` `memberId` BIGINT(20) UNSIGNED NOT NULL,
        DROP PRIMARY KEY,
        ADD PRIMARY KEY USING BTREE(`memberId`);

    ALTER TABLE `osdktournaments_tournaments` ADD COLUMN `mode` INTEGER UNSIGNED NOT NULL DEFAULT 1 AFTER `name`;

    ALTER TABLE `osdktournaments_trophies` CHANGE COLUMN `blazeId` `memberId` BIGINT(20) UNSIGNED NOT NULL,
        DROP PRIMARY KEY,
        ADD PRIMARY KEY USING BTREE(`memberId`, `tournamentId`);

    ALTER TABLE `osdktournaments_members` DROP PRIMARY KEY,
 	ADD PRIMARY KEY USING BTREE(`memberId`, `tournamentId`);

    ALTER TABLE `osdktournaments_matches` ADD COLUMN `tournamentId` int(10) unsigned NOT NULL AFTER `matchId`;    

    
