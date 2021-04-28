#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	ALTER TABLE `vpro_loadouts` ADD  `unlocks3` bigint(20) unsigned NOT NULL DEFAULT 0;
	
