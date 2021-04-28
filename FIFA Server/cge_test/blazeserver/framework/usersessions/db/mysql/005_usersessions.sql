#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      DROP INDEX `EXTERNAL`,
      ADD UNIQUE KEY `EXTERNAL` (`externalid`, `personanamespace`),
      DROP INDEX `externalblobid_idx`,
      ADD UNIQUE KEY `externalblobid_idx` (`externalblob`, `personanamespace`) USING BTREE;