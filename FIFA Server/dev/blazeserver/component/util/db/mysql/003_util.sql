#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM: 
    ALTER TABLE `util_user_small_storage`
      MODIFY `created_date` timestamp NOT NULL DEFAULT '1970-01-01 00:00:01';

