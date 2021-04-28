#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `pidid` bigint(20) DEFAULT '0',
      ADD KEY `pidid_idx` (`pidid`) USING BTREE;
