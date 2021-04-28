#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `personanamespace` varchar(32) DEFAULT NULL AFTER `persona`,
      DROP INDEX `canonicalpersona_idx`,
      ADD UNIQUE KEY `namespace_persona` (`canonicalpersona`, `personanamespace`) USING BTREE;

