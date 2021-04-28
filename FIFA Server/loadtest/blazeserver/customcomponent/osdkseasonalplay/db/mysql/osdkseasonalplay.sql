#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

    CREATE TABLE `osdkseasonalplay_seasons` (
      `seasonId` int(11) unsigned NOT NULL,
      `seasonNumber` int(11) unsigned NOT NULL default '1',
      `lastRollOverStatPeriodId` int(11) unsigned NOT NULL default '0',
      PRIMARY KEY USING BTREE (`seasonId`)
    ) ENGINE=InnoDB COMMENT='SeasonalPlay Table for tracking current season number';

    CREATE TABLE `osdkseasonalplay_registration` (
      `memberId` bigint(20) unsigned NOT NULL,
      `memberType` int(11) unsigned NOT NULL,
      `seasonId` int(11) unsigned NOT NULL,
      `leagueId` int(11) unsigned NOT NULL default '0',
      `tournamentStatus` tinyint(1) unsigned NOT NULL default '0',
      PRIMARY KEY USING BTREE (`memberId`, `memberType`),
      KEY `FK_osdkseasonalplay_registration_seasons` (`seasonId`),
      CONSTRAINT `FK_osdkseasonalplay_registration_seasons` FOREIGN KEY (`seasonId`) REFERENCES `osdkseasonalplay_seasons` (`seasonId`) ON DELETE CASCADE ON UPDATE CASCADE
    ) ENGINE=InnoDB COMMENT='Seasonal Play Registration Table';

    CREATE TABLE `osdkseasonalplay_awards` (
      `id` int(11) unsigned NOT NULL auto_increment,
      `memberId` bigint(20) unsigned NOT NULL,
      `memberType` int(11) unsigned NOT NULL,
      `awardId` int(11) unsigned NOT NULL,
      `seasonId` int(11) unsigned NOT NULL default '0',
      `leagueId` int(11) unsigned NOT NULL default '0',
      `seasonNumber` int(11) unsigned NOT NULL default '1',
      `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
      PRIMARY KEY (`id`),
      KEY `index_osdkseasonalplay_awards_memberId` (`memberId`),
      KEY `index_osdkseasonalplay_awards_memberType` (`memberType`),
      KEY `index_osdkseasonalplay_awards_awardId` (`awardId`),
      KEY `index_osdkseasonalplay_awards_seasonId` (`seasonId`),
      KEY `index_osdkseasonalplay_awards_leagueId` (`seasonNumber`)
    ) ENGINE=InnoDB COMMENT='Season Play Awards Table';
	
# Added from version 002	
	ALTER TABLE `osdkseasonalplay_awards`
    ADD COLUMN `historical` int(1) unsigned NOT NULL default '0' AFTER `timestamp`,
    ADD COLUMN `metadata` VARCHAR(256) default NULL after `historical`;

    ALTER TABLE `osdkseasonalplay_registration`
    CHANGE COLUMN `tournamentStatus` `tournamentStatus` int(1) unsigned NOT NULL default '0';
      
