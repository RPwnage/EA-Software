#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.

# Upgrade your schema
COMMON: 
    CREATE TABLE `user_association_list_size` (
      `blazeid` BIGINT(20) unsigned NOT NULL,
      `assoclisttype` SMALLINT(5) unsigned NOT NULL,
      `listsize` INT(10) unsigned NOT NULL DEFAULT '0',
      `maxsize` INT(10) unsigned NOT NULL DEFAULT '100',
      `memberhash` INT(10) unsigned NOT NULL DEFAULT '0',
      PRIMARY KEY (`blazeid`,`assoclisttype`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    
