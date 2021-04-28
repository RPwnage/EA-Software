#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_seasons`
      ADD PRIMARY KEY (`state`);
