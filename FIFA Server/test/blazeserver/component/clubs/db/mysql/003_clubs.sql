#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    CREATE TABLE `clubs_memberdomains` (
      `blazeId` bigint(20) unsigned NOT NULL,
      `domainId` int(10) unsigned NOT NULL,
      PRIMARY KEY (`blazeId`, `domainId`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
