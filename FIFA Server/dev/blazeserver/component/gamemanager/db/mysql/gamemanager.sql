#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    CREATE TABLE `gm_user_connection_metrics` (
      `blazeid` BIGINT(20) unsigned NOT NULL,
      `gameid` BIGINT(20) unsigned NOT NULL,
      `local_ip` BIGINT(20) unsigned NOT NULL,
      `local_port` INT(10) unsigned NOT NULL,
      `local_client_type` SMALLINT(5) unsigned NOT NULL,
      `local_locale` VARCHAR(8) NOT NULL,
      `remote_ip` BIGINT(20) unsigned NOT NULL,
      `remote_port` INT(10) unsigned NOT NULL,
      `remote_client_type` SMALLINT(5) unsigned NOT NULL,
      `remote_locale` VARCHAR(8) NOT NULL,
      `game_mode` VARCHAR(64) NOT NULL,
      `topology` SMALLINT(5) unsigned NOT NULL,
      `max_latency` INT(10) NOT NULL,
      `avg_latency` INT(10) NOT NULL,
      `min_latency` INT(10) NOT NULL,
      `packet_loss` INT(10) NOT NULL,
      `conn_term_reason` INT(10) unsigned NOT NULL,
      `conn_init_time` BIGINT(20) unsigned NOT NULL,
      `conn_term_time` BIGINT(20) unsigned NOT NULL,
      `client_init_conn` TINYINT(1) unsigned NOT NULL,
      `qos_rule_enabled` TINYINT(1) unsigned NOT NULL DEFAULT 0,
      `local_nat_type` INT(10) unsigned NOT NULL,
      `remote_nat_type` INT(10) unsigned NOT NULL,
      `local_router_info` VARCHAR(128) NOT NULL,
      `remote_router_info` VARCHAR(128) NOT NULL,
      `dummy_id` BIGINT(20) unsigned NOT NULL AUTO_INCREMENT,
      PRIMARY KEY (`dummy_id`) USING BTREE,
      KEY (`blazeid`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    
    CREATE TABLE `gm_user_connection_audit` (
      `blazeid` BIGINT(20) unsigned NOT NULL,
      `expiration` BIGINT(20) unsigned NOT NULL,
      PRIMARY KEY (`blazeid`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

    CREATE TABLE `gm_scenario_configuration` (
      `scenario_hash` BIGINT(20) unsigned NOT NULL,
      `scenario_name` VARCHAR(32) NOT NULL,
      `scenario_variant` VARCHAR(32) NOT NULL,
      `scenario_version` BIGINT(20) unsigned NOT NULL,
      `configuration_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY (`scenario_name`, `scenario_variant`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;