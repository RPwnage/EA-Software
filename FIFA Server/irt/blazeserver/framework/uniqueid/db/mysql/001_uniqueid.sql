#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON: 
    DROP TABLE IF EXISTS unique_id;
    CREATE TABLE `unique_id` (
      `component_id` int(11) NOT NULL,
      `component` varchar(40) DEFAULT NULL,
      `id_type` int(11) NOT NULL,
      `last_id` bigint(20) unsigned NOT NULL,
      `last_modified_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`component_id`, `id_type`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

