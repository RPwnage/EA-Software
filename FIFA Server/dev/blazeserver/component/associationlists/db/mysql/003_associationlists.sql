#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
# THIS IS DESTRUCTIVE.  All tables will be dropped to ensure integrity constraints are met.
COMMON: 
    DROP TABLE `user_association_list_info`;
    DROP TABLE `user_externalid_to_persona`;
    DROP TABLE `user_association_list_size`;
    CREATE TABLE `user_association_list_size` (
      `blazeid` BIGINT(20) unsigned NOT NULL,
      `assoclisttype` SMALLINT(5) unsigned NOT NULL,
      `listsize` INT(10) unsigned NOT NULL DEFAULT '0',
      `maxsize` INT(10) unsigned NOT NULL DEFAULT '100',
      `memberhash` INT(10) unsigned NOT NULL DEFAULT '0',  
      PRIMARY KEY (`blazeid`,`assoclisttype`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
    