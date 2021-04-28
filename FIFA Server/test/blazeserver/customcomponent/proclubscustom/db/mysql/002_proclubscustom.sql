#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `clubscustom_AIPlayerCustomization`
      ADD COLUMN `kitname` varchar(100) DEFAULT NULL AFTER `commonname`;