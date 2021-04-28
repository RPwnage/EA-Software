#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `osdktournaments_trophies` CHANGE COLUMN `memberId` `memberId` BIGINT(20) UNSIGNED NOT NULL,
        DROP PRIMARY KEY,
        ADD PRIMARY KEY USING BTREE(`memberId`, `tournamentId`);

