#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    CREATE TABLE `arson_test_data` (
      `id` bigint(20) unsigned NOT NULL,
      `data1` varchar(2000) NOT NULL DEFAULT '',
      `data2` varchar(2000) NOT NULL DEFAULT '',
      `data3` varchar(2000) NOT NULL DEFAULT '',
      PRIMARY KEY (`id`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

    INSERT INTO `arson_test_data` VALUES (0,'退出遊戲','','');
