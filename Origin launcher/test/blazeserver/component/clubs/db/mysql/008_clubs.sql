#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_clubs`
      ADD `clubNameResetCount` int(10) NOT NULL DEFAULT '0';