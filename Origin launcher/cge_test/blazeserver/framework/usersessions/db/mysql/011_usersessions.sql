#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      DROP COLUMN `cityoverride`,
      DROP COLUMN `geocity`,
      DROP COLUMN `geocountry`,
      DROP COLUMN `geostate`,
      DROP COLUMN `geotimezone`;