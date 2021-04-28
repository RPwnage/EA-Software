#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE sponsoredevents_registration
    ADD COLUMN `title` varchar(32) NOT NULL,			
    ADD COLUMN `platform` varchar(10) NOT NULL,			
    ADD COLUMN `flags` int(32) unsigned default '0',
    ADD COLUMN `ban` int(8) default '0',				
    ADD COLUMN `persona` varchar(256) NOT NULL,			
    ADD COLUMN `email` varchar(256) NOT NULL,			
    ADD COLUMN `country` varchar(16) NOT NULL,			
    ADD COLUMN `dateofbirth` varchar(32) NOT NULL,		
    ADD COLUMN `whobanned` varchar(32) NOT NULL,		
    ADD COLUMN `whybanned` varchar(256) NOT NULL,		
    ADD COLUMN `creationdate` date NOT NULL,			
	ADD COLUMN `sendinformation` int(8) default '0';    
    
