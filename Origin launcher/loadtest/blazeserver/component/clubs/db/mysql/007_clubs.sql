#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_clubs`
      ADD `isNameProfane` tinyint(1) DEFAULT '0';