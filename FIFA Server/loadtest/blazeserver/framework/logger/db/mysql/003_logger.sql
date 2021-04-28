#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `log_audits`
      ADD COLUMN `accountid` bigint(20) NOT NULL DEFAULT '0' AFTER `persona`,
      ADD COLUMN `platform` varchar(32) NOT NULL DEFAULT '' AFTER `accountid`,
      DROP INDEX `idx_all`,
      ADD UNIQUE KEY `idx_all` (`blazeid`, `deviceid`, `ip`, `persona`, `accountid`, `platform`) USING BTREE;
