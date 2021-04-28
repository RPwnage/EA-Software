#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:

	CREATE TABLE `proclubs_Avatar` (
		`userId` bigint(20) NOT NULL,
		`firstname` varchar(100) NOT NULL,
		`lastname` varchar(100) NOT NULL,
		`isNameProfane` tinyint(1) DEFAULT '0',
		`avatarNameResetCount` int(10) NOT NULL DEFAULT '0',
		PRIMARY KEY (`userId`), 
		UNIQUE (`userId`)
	) ENGINE=InnoDB COMMENT='ProClubsAvatar';
