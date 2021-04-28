#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `fifacups_registration` 
      ADD COLUMN `activeCupId` int(11) unsigned NOT NULL default '0' AFTER `tournamentStatus`;
 
