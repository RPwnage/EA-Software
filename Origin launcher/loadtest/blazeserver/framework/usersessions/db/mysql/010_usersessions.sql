#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `lastlogoutdatetime` bigint(20) DEFAULT '0' AFTER `previouslogindatetime`;