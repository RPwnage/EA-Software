#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `clubs_clubs`
      MODIFY `lastOppo` BIGINT(20) UNSIGNED DEFAULT NULL;
    ALTER TABLE `clubs_members`
      MODIFY `blazeId` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_messages`
      MODIFY `sender` BIGINT(20) NOT NULL,
      MODIFY `receiver` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_news`
      MODIFY `contentCreator` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_recordbooks`
      MODIFY `blazeId` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_tickermsgs`
      MODIFY `includeUserId` BIGINT(20) NOT NULL,
      MODIFY `excludeUserId` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_lastleavetime`
      MODIFY `blazeId` BIGINT(20) NOT NULL;
    ALTER TABLE `clubs_memberdomains`
      MODIFY `blazeId` BIGINT(20) NOT NULL;