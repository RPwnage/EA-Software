#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

    ALTER TABLE `coopseason_metadata` ADD INDEX `index_memberOneBlazeId`(`memberOneBlazeId`);
    ALTER TABLE `coopseason_metadata` ADD INDEX `index_memberTwoBlazeId`(`memberTwoBlazeId`);
