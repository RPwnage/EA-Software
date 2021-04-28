#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	ALTER TABLE `vpro_loadouts` DROP `assignedPerks`;
	ALTER TABLE `vpro_loadouts` ADD `assignedPerks` VARCHAR(32) NOT NULL DEFAULT '-1,-1,-1' AFTER `unlocks3`;
