#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_clubs`
      MODIFY `lastActiveTime` datetime NOT NULL DEFAULT '1970-01-01 00:00:01',
      MODIFY `lastGameTime` datetime DEFAULT '1970-01-01 00:00:01';
    ALTER TABLE `clubs_rivals`
      MODIFY `lastUpdateTime` timestamp NOT NULL DEFAULT '1970-01-01 00:00:01';
    ALTER TABLE `clubs_seasons`
      MODIFY `lastUpdated` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
    ALTER TABLE `clubs_lastleavetime`
      MODIFY `lastLeaveTime` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';