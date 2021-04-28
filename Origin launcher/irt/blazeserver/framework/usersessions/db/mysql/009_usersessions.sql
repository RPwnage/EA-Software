#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      MODIFY COLUMN `latitude` float(53) DEFAULT 360.0,
      MODIFY COLUMN `longitude` float(53) DEFAULT 360.0;