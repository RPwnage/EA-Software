#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON:
    ALTER TABLE `gm_user_connection_metrics`
      ADD COLUMN `dummy_id` BIGINT(20) unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY AFTER `remote_router_info`;
