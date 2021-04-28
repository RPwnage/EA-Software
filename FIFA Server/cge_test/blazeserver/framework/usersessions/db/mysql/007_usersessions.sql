#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `geotimezone` varchar(128) DEFAULT NULL AFTER `geostate`;