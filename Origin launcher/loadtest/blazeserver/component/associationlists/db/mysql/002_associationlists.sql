#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON: 
    ALTER TABLE `user_association_list_info`
      ADD KEY `IDX_LIST_DATEADDED` (`blazeid`,`assoclisttype`,`dateadded`) USING BTREE,
      DROP KEY `IDX_EXTERNAL_LIST`;
