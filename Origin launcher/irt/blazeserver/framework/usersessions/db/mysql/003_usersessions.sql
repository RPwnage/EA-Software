#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
        ADD UNIQUE KEY `externalblobid_idx` (`externalblob`) USING BTREE;
