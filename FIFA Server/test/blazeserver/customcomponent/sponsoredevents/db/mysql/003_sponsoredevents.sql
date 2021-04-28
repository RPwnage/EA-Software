#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE sponsoredevents_registration
	DROP COLUMN `persona`,
    DROP COLUMN `email`,
	DROP COLUMN `dateofbirth`;
    
    
