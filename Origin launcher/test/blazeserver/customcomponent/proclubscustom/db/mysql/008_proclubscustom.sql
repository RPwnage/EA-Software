#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `clubscustom_TeamSettings` CHANGE COLUMN `teamBannerId` `themeId` INT(10) NOT NULL DEFAULT -1;
	ALTER TABLE `clubscustom_TeamSettings` CHANGE COLUMN `extra1` `ballId` INT(10) NOT NULL DEFAULT -1;
	ALTER TABLE `clubscustom_TeamSettings` CHANGE COLUMN `extra2` `stadiumColorId` INT(10) NOT NULL DEFAULT -1;
	ALTER TABLE `clubscustom_TeamSettings` CHANGE COLUMN `extra3` `seatColorId` INT(10) NOT NULL DEFAULT -1;
	ALTER TABLE `clubscustom_TeamSettings` CHANGE COLUMN `extra4` `tifoId` INT(10) NOT NULL DEFAULT -1;
