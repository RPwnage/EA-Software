#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.   - Also, update getDbSchemaVersion() in Logger.

# Upgrade your schema
COMMON:
    CREATE TABLE `log_audits` (
      `auditid` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
      `blazeid` bigint(20) NOT NULL DEFAULT '0',
      `deviceid` varchar(128) NOT NULL DEFAULT '',
      `ip` varchar(32) NOT NULL DEFAULT '',
      `persona` varbinary(255) NOT NULL DEFAULT '',
      `accountid` bigint(20) NOT NULL DEFAULT '0',
      `platform` varchar(32) NOT NULL DEFAULT '',
      `filename` varbinary(255) NOT NULL DEFAULT '',
      `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`auditid`),
      UNIQUE KEY `idx_all` (`blazeid`, `deviceid`, `ip`, `persona`, `accountid`, `platform`) USING BTREE,
      KEY `timestamp_idx` (`timestamp`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

