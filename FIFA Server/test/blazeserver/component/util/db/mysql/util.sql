#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    CREATE TABLE `util_user_options` (
      `blazeid` bigint(20) unsigned NOT NULL,
      `telemetryopt` int(10) unsigned NOT NULL,
      PRIMARY KEY (`blazeid`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

PLATFORM:
    CREATE TABLE `util_user_small_storage` (
      `id` bigint(20) unsigned NOT NULL,
      `key` varchar(32) NOT NULL DEFAULT '',
      `data` varbinary(20000) DEFAULT NULL,
      `last_modified_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      `created_date` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
      PRIMARY KEY (`id`,`key`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
