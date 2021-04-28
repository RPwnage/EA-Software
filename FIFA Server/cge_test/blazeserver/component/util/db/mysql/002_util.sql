#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM: 
    ALTER TABLE `util_user_small_storage`
      MODIFY `data` varbinary(20000) DEFAULT NULL;

