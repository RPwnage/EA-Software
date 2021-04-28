#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    ALTER TABLE `gm_user_connection_metrics`
      ADD COLUMN `qos_rule_enabled` TINYINT(1) unsigned NOT NULL DEFAULT 0 AFTER `client_init_conn`;

