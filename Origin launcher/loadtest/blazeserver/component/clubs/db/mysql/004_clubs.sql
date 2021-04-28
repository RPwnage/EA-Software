#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_clubs`
      ADD `lastUpdatedBy` bigint(20) unsigned NOT NULL;

    ALTER TABLE `clubs_clubs`
      MODIFY `metaData2` varbinary(4096) NOT NULL;
  

