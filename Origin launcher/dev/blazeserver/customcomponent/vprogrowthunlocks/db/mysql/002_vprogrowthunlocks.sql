#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	RENAME TABLE `vpro_unlocks_sets` TO `vpro_loadouts`;
	
	ALTER TABLE `vpro_loadouts` CHANGE `setId` `loadoutId` INT(11) unsigned NOT NULL;
	ALTER TABLE `vpro_loadouts` CHANGE `setName` `loadoutName` VARCHAR(32) NOT NULL DEFAULT '';
	ALTER TABLE `vpro_loadouts` CHANGE `trait1_unlocks` `trait_unlocks` INT(11) unsigned NOT NULL;
	ALTER TABLE `vpro_loadouts` DROP  `active`;
	ALTER TABLE `vpro_loadouts` DROP  `trait2_unlocks`;
	ALTER TABLE `vpro_loadouts` ADD  `vpro_height` INT(11) unsigned NOT NULL DEFAULT 0;
	ALTER TABLE `vpro_loadouts` ADD  `vpro_weight` INT(11) unsigned NOT NULL DEFAULT 0;
	ALTER TABLE `vpro_loadouts` ADD  `vpro_position` INT(11) unsigned NOT NULL DEFAULT 0;
	ALTER TABLE `vpro_loadouts` ADD  `vpro_foot` INT(11) unsigned NOT NULL DEFAULT 0;
		    	
