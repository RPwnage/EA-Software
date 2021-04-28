#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    CREATE TABLE `gm_scenario_configuration` (
      `scenario_hash` BIGINT(20) unsigned NOT NULL,
      `scenario_name` VARCHAR(32) NOT NULL,
      `scenario_variant` VARCHAR(32) NOT NULL,
      `scenario_version` BIGINT(20) unsigned NOT NULL,
      `configuration_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`scenario_name`, `scenario_variant`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
