#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `accountcountry` int(10) unsigned NOT NULL DEFAULT '0' AFTER `accountlocale`,
      ADD COLUMN `lastcountryused` int(10) unsigned DEFAULT '0' AFTER `lastlocaleused`;

