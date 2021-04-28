#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.   - Also, update getDbSchemaVersion() in UserSessionManager.

# Upgrade your schema
COMMON:
    CREATE TABLE `log_audits` (
      `auditid` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
      `blazeid` bigint(20) NOT NULL DEFAULT '0',
      `deviceid` varchar(128) NOT NULL DEFAULT '',
      `ip` varchar(32) NOT NULL DEFAULT '',
      `persona` varchar(255) NOT NULL DEFAULT '',
      `filename` varchar(255) NOT NULL DEFAULT '',
      PRIMARY KEY (`auditid`),
      UNIQUE KEY `idx_all` (`blazeid`, `deviceid`, `ip`, `persona`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
