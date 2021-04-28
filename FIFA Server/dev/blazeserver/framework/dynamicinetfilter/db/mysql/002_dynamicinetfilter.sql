#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `dynamicinetfilter`
      MODIFY `created_date` TIMESTAMP NOT NULL DEFAULT '1970-01-01 00:00:01';