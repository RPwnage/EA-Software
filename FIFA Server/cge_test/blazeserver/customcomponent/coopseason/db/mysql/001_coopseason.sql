#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

    CREATE TABLE `coopseason_metadata` (
      `coopId` bigint(20) NOT NULL AUTO_INCREMENT,
      `memberOneBlazeId` bigint(20) unsigned NOT NULL,
      `memberTwoBlazeId` bigint(20) unsigned NOT NULL,
      `metaData` varbinary(1024) NOT NULL,
      PRIMARY KEY (`coopId`)
    ) ENGINE=InnoDB COMMENT='Coop Season Table for tracking settings for the pair';
    
    ALTER TABLE `coopseason_metadata` AUTO_INCREMENT = 2;
	
    
