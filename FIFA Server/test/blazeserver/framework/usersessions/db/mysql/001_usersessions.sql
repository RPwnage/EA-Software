#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    DROP TABLE IF EXISTS user_accessgroup_info;
    CREATE TABLE `user_accessgroup_info` (
      `externalid` varchar(255) NOT NULL,
      `clienttype` int(10) unsigned NOT NULL,
      `groupname` varchar(256) NOT NULL,
      PRIMARY KEY (`externalid`,`clienttype`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    DROP TABLE IF EXISTS userinfo;
    CREATE TABLE `userinfo` (
      `blazeid` bigint(20) NOT NULL,
      `accountid` bigint(20) NOT NULL,
      `persona` varchar(32) DEFAULT NULL,
      `accountlocale` int(10) unsigned NOT NULL DEFAULT '0',
      `canonicalpersona` varchar(32) DEFAULT NULL,
      `externalid` bigint(20) unsigned DEFAULT NULL,
      `externalblob` varbinary(64) DEFAULT NULL COMMENT 'sizeof(SceNpId)==36',
      `status` int(10) DEFAULT '1',
      `latitude` int(10) DEFAULT NULL,
      `longitude` int(10) DEFAULT NULL,
      `cityoverride` varchar(64) DEFAULT NULL,
      `geocity` varchar(256) DEFAULT NULL,
      `geocountry` varchar(3) DEFAULT NULL,
      `geostate` varchar(3) DEFAULT NULL,
      `geoopt` tinyint(1) DEFAULT '0',
      `customattribute` bigint(20) unsigned DEFAULT '0',
      `firstlogindatetime` bigint(20) DEFAULT '0',
      `lastlogindatetime` bigint(20) DEFAULT '0',
      `previouslogindatetime` bigint(20) DEFAULT '0',
      `lastautherror` int(10) unsigned DEFAULT '0',
      `lastlocaleused` int(10) unsigned DEFAULT '0',
      `childaccount` int(10) DEFAULT '0',
      `originpersonaid` bigint(20) unsigned DEFAULT '0',
      PRIMARY KEY (`blazeid`),
      UNIQUE KEY `canonicalpersona_idx` (`canonicalpersona`) USING BTREE,
      UNIQUE KEY `EXTERNAL` (`externalid`),
      KEY `userinfo_accountid_idx` (`accountid`) USING BTREE,
      KEY `originpersonaid_idx` (`originpersonaid`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
