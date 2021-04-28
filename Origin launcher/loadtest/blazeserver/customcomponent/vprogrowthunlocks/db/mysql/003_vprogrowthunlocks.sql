#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	ALTER TABLE `vpro_loadouts` ADD  `unlocks1` bigint(20) unsigned NOT NULL DEFAULT 0;
	ALTER TABLE `vpro_loadouts` ADD  `unlocks2` bigint(20) unsigned NOT NULL DEFAULT 0;
	
	ALTER TABLE `vpro_loadouts` DROP  `trait_unlocks`;
	ALTER TABLE `vpro_loadouts` DROP  `special_unlocks`;
	ALTER TABLE `vpro_loadouts` DROP  `boost_unlocks`;
		    	
