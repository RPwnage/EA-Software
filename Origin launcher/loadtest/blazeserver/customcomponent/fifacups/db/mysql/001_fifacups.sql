#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

    CREATE TABLE `fifacups_seasons` (
      `seasonId` int(11) unsigned NOT NULL,
      `seasonNumber` int(11) unsigned NOT NULL default '1',
      `lastRollOverStatPeriodId` int(11) unsigned NOT NULL default '0',
      PRIMARY KEY USING BTREE (`seasonId`)
    ) ENGINE=InnoDB COMMENT='Seasonal Play Table for tracking current season number';

    CREATE TABLE `fifacups_registration` (
      `memberId` bigint(20) unsigned NOT NULL,
      `memberType` int(11) unsigned NOT NULL,
      `seasonId` int(11) unsigned NOT NULL,
      `leagueId` int(11) unsigned NOT NULL default '0',
      `qualifyingDiv` int(11) unsigned NOT NULL default '0',
      `pendingDiv` int(11) unsigned NOT NULL default '0',
	  `tournamentStatus` int(11) unsigned NOT NULL default '0',
      PRIMARY KEY USING BTREE (`memberId`, `memberType`),
      KEY `FK_fifacups_registration_seasons` (`seasonId`),
      CONSTRAINT `FK_fifacups_registration_seasons` FOREIGN KEY (`seasonId`) REFERENCES `fifacups_seasons` (`seasonId`) ON DELETE CASCADE ON UPDATE CASCADE
    ) ENGINE=InnoDB COMMENT='Seasonal Play Registration Table';
      
