#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    DROP TABLE IF EXISTS task_scheduler;
    CREATE TABLE `task_scheduler` (
      `task_id` int(20) unsigned NOT NULL auto_increment,
      `task_name` varchar(128) NOT NULL,
      `component_id` int(11) NOT NULL,
      `tdf_type` int(10) unsigned NOT NULL default '0',
      `tdf_raw` varbinary(4000) default NULL,
      `start` int(10) unsigned NOT NULL default '0',
      `duration` int(10) unsigned NOT NULL default '0',
      `recurrence` int(10) unsigned NOT NULL default '0',
      `status` int(10) unsigned NOT NULL DEFAULT '0',
      `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      PRIMARY KEY  (`task_id`),
      UNIQUE KEY `index_task_scheduler_task_name` (`task_name`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

