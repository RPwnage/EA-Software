#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `log_audits`
      ADD COLUMN `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      ADD KEY `timestamp_idx` (`timestamp`) USING BTREE;

    UPDATE `log_audits` SET `timestamp`=CURRENT_TIMESTAMP;

