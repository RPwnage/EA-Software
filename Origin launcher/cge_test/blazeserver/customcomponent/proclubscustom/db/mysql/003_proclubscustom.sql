#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `clubscustom_AIPlayerCustomization`
      ADD COLUMN `shortstyle` INT(10) NOT NULL DEFAULT 0 AFTER `socklengthcode`;