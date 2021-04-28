#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    DROP TABLE IF EXISTS messaging_inbox;
    CREATE TABLE `messaging_inbox` (
      `message_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
      `src_component` smallint(5) unsigned NOT NULL DEFAULT '0',
      `src_type` smallint(5) unsigned NOT NULL DEFAULT '0',
      `src_id` bigint(20) signed NOT NULL DEFAULT '0',
      `tgt_component` smallint(5) unsigned NOT NULL DEFAULT '0',
      `tgt_type` smallint(5) unsigned NOT NULL DEFAULT '0',
      `tgt_id` bigint(20) signed NOT NULL DEFAULT '0',
      `flags` int(10) unsigned NOT NULL DEFAULT '0',
      `type` int(10) unsigned NOT NULL DEFAULT '0',
      `tag` int(10) unsigned NOT NULL DEFAULT '0',
      `status` int(10) unsigned NOT NULL DEFAULT '0',
      `timestamp` int(10) unsigned NOT NULL DEFAULT '0',
      `attr_key0` int(10) unsigned NOT NULL DEFAULT '0',
      `attr_str0` varchar(1024) DEFAULT NULL,
      `attr_key1` int(10) unsigned NOT NULL DEFAULT '0',
      `attr_str1` varchar(1024) DEFAULT NULL,
      `attr_key2` int(10) unsigned NOT NULL DEFAULT '0',
      `attr_str2` varchar(1024) DEFAULT NULL,
      `attr_key3` int(10) unsigned NOT NULL DEFAULT '0',
      `attr_str3` varchar(1024) DEFAULT NULL,
      PRIMARY KEY (`message_id`) USING BTREE,
      KEY `messaging_inbox_tgt` (`tgt_id`,`tgt_type`,`tgt_component`,`timestamp`,`type`),
      KEY `messaging_inbox_timestamp` (`timestamp`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

