#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON: 
# -- ---------------------------------------------
# -- Creating user small storage table.
# -- ---------------------------------------------

    DROP TABLE IF EXISTS arson_test_data;
    CREATE TABLE `arson_test_data` (
      `id` bigint(20) unsigned NOT NULL,
      `data1` varchar(2000) NOT NULL DEFAULT '',
      `data2` varchar(2000) NOT NULL DEFAULT '',
      `data3` varchar(2000) NOT NULL DEFAULT '',
      PRIMARY KEY (`id`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

    INSERT INTO `arson_test_data` VALUES (0,'退出遊戲','','');
