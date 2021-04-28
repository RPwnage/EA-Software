#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON: 
    DROP TABLE IF EXISTS user_association_list_info;
    CREATE TABLE `user_association_list_info` (
      `blazeid` bigint(20) unsigned NOT NULL,
      `assoclisttype` int(10) unsigned NOT NULL,
      `memberblazeid` bigint(20) unsigned NOT NULL DEFAULT '0',
      `memberexternalidblob` varbinary(64) DEFAULT NULL COMMENT 'sizeof(SceNpId)==36, sizeof(xuid)==8',
      `dateadded` bigint(20) unsigned NOT NULL DEFAULT '0',
      `externalsystemid` int(11) NOT NULL DEFAULT '0',
      `personaname` VARCHAR(32) DEFAULT NULL,
      PRIMARY KEY (`blazeid`,`assoclisttype`,`externalsystemid`,`memberexternalidblob`) USING BTREE,
      KEY `IDX_EXTERNAL_LIST` (`blazeid`,`assoclisttype`,`externalsystemid`,`memberexternalidblob`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS user_association_list_size;
    CREATE TABLE `user_association_list_size` (
      `listid` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
      `ownerblazeid` bigint(20) unsigned NOT NULL,
      `assoclistid` int(10) unsigned NOT NULL,
      PRIMARY KEY (`ownerblazeid`,`assoclistid`) USING BTREE,
      UNIQUE KEY `index_listid`(`listid`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS user_externalid_to_persona;
    CREATE TABLE `user_externalid_to_persona` (
      `externalid` varchar(255) NOT NULL,
      `personaname` varchar(32) NOT NULL,
      PRIMARY KEY (`externalid`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
