#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      MODIFY COLUMN `latitude` float DEFAULT NULL,
      MODIFY COLUMN `longitude` float DEFAULT NULL;