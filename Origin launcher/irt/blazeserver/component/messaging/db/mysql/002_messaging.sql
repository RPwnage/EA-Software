#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `messaging_inbox`
      DROP KEY `messaging_inbox_tgt`;
    ALTER TABLE `messaging_inbox`
      ADD KEY `messaging_inbox_tgt` (`tgt_id`,`tgt_type`,`tgt_component`,`type`,`timestamp`);
 
