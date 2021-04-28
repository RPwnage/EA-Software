#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

	CREATE TABLE `vpro_unlocks_sets` (
	  `entityId` BIGINT(20) unsigned NOT NULL,
	  `setId` INT(11) unsigned NOT NULL,
	  `setName` VARCHAR(32) NOT NULL DEFAULT '',
	  `active` INT(11) unsigned NOT NULL,	  	  
	  `spgranted` INT(11) unsigned NOT NULL,
      `spconsumed` INT(11) unsigned NOT NULL,
      `boost_unlocks` INT(11) unsigned NOT NULL,
	  `special_unlocks` INT(11) unsigned NOT NULL,
	  `trait1_unlocks` INT(11) unsigned NOT NULL,
	  `trait2_unlocks` INT(11) unsigned NOT NULL,	  
      PRIMARY KEY (`entityId`, `setId`)
    ) ENGINE=InnoDB COMMENT='VPro growth unlocks sets';	
	    	
