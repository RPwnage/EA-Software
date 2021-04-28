#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	ALTER TABLE `vpro_loadouts` ADD `xpEarned` INT(11) unsigned NOT NULL DEFAULT 0 AFTER `loadoutName`;
	ALTER TABLE `vpro_loadouts` ADD `assignedPerks` BINARY(3) NOT NULL DEFAULT '\0\0\0' AFTER `unlocks3`;
