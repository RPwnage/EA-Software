#
# Database migration script.  End each sql statement with a ;
[MIGRATION]

# Upgrade your schema
COMMON:
	DELETE FROM `vpro_loadouts`;
	DELETE FROM `userinfo`;
	
