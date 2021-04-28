#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	ALTER TABLE `vpro_loadouts` ADD  `objectivespgranted` INT(11) unsigned NOT NULL DEFAULT 0 AFTER `spgranted`;
	ALTER TABLE `vpro_loadouts` ADD  `matchspgranted` INT(11) unsigned NOT NULL DEFAULT 0 AFTER `spgranted`;
	ALTER TABLE `vpro_loadouts` DROP  `spgranted`;
