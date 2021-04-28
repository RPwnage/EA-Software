#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    ALTER TABLE `gm_user_connection_metrics` 
      MODIFY COLUMN `max_latency` INT(10) NOT NULL,
      MODIFY COLUMN `avg_latency` INT(10) NOT NULL,
      MODIFY COLUMN `min_latency` INT(10) NOT NULL,
      MODIFY COLUMN `packet_loss` INT(10) NOT NULL;

