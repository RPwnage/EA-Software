#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `log_audits`
      MODIFY COLUMN `persona` varbinary(255) NOT NULL DEFAULT '',
      MODIFY COLUMN `filename` varbinary(255) NOT NULL DEFAULT '';