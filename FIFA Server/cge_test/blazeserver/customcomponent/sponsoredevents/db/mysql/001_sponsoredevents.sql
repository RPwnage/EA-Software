#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

    DROP TABLE IF EXISTS `sponsoredevents_registration`;
    CREATE TABLE `sponsoredevents_registration` (
      `userId` bigint(20) unsigned NOT NULL,
      `eventId` int(11) unsigned NOT NULL,
	  `eventData` varchar(64) NOT NULL,
	  PRIMARY KEY USING BTREE (`userId`, `eventId`)
    ) ENGINE=InnoDB COMMENT='Event registration table';
      
