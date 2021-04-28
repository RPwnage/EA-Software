#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
# -- ---------------------------------------------
# -- Creating user small storage table.
# -- ---------------------------------------------

    CREATE TABLE `dynamicinetfilter` (
        `row_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
        `group` VARCHAR(64),
        `owner` VARCHAR(64),
        `ip` VARCHAR(15),
        `prefix_length` INT UNSIGNED,
        `comment` VARCHAR(160),
        `last_modified_date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        `created_date` TIMESTAMP NOT NULL DEFAULT '1970-01-01 00:00:01',
        PRIMARY KEY (`row_id`)
    ) 
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

