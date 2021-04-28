#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `crossplatformopt` tinyint(1) DEFAULT '0' AFTER `pidid`;

COMMON:
    CREATE TABLE `accountinfo` (
      `accountid` bigint(20) NOT NULL,
      `originpersonaid` bigint(20) unsigned DEFAULT '0',
      `canonicalpersona` varchar(32) DEFAULT NULL COMMENT 'canonical version of origin id',
      `originpersonaname` varchar(32) DEFAULT NULL,
      `primarypersonaidxbl` bigint(20) unsigned DEFAULT NULL COMMENT 'xbox live persona id',
      `primarypersonaidpsn` bigint(20) unsigned DEFAULT NULL COMMENT 'psn persona id',
      `primaryexternalidxbl` bigint(20) unsigned DEFAULT NULL COMMENT 'xbox live externalid',
      `primaryexternalidpsn` bigint(20) unsigned DEFAULT NULL COMMENT 'psn externalid',
      `firstlogindatetime` bigint(20) DEFAULT '0',
      `lastlogindatetime` bigint(20) DEFAULT '0',
      `previouslogindatetime` bigint(20) DEFAULT '0',
      `lastautherror` int(10) unsigned DEFAULT '0',
      `lastpersonaidused` bigint(20) unsigned DEFAULT NULL COMMENT 'last persona logged in',
      `lastplatformused` varchar(32) DEFAULT NULL COMMENT 'platform of last persona logged in',
      PRIMARY KEY (`accountid`),
      UNIQUE KEY (`originpersonaid`),
      UNIQUE KEY (`canonicalpersona`) USING BTREE,
      UNIQUE KEY (`primarypersonaidxbl`),
      UNIQUE KEY (`primaryexternalidxbl`),
      UNIQUE KEY (`primarypersonaidpsn`),
      UNIQUE KEY (`primaryexternalidpsn`),
      UNIQUE KEY `originpersonaid_idx` (`originpersonaid`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;


    DROP PROCEDURE IF EXISTS `upsertAccountInfo`;
    DELIMITER $$
    CREATE PROCEDURE `upsertAccountInfo`(
      _accountid bigint(20),
      _originpersonaid bigint(20) unsigned,
      _canonicalpersona varchar(32),
      _originpersonaname varchar(32),
      _primarypersonaidxbl bigint(20) unsigned,
      _primarypersonaidpsn bigint(20) unsigned,
      _primaryexternalidxbl bigint(20) unsigned,
      _primaryexternalidpsn bigint(20) unsigned,
      _lastautherror int(10) unsigned,
      _lastpersonaidused bigint(20) unsigned,
      _lastplatformused varchar(32))
    BEGIN
      DECLARE currentExternalIdXbl bigint(20) unsigned;
      DECLARE currentExternalIdPsn bigint(20) unsigned;
      DECLARE currentPersonaIdXbl bigint(20) unsigned;
      DECLARE currentPersonaIdPsn bigint(20) unsigned;
      DECLARE currentdatetime bigint(20);
      DECLARE userNotFound BOOL;
      DECLARE initXblIdData BOOL;
      DECLARE initPsnIdData BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      START TRANSACTION;
      BEGIN

        SELECT `primarypersonaidxbl`,`primarypersonaidpsn`, `primaryexternalidxbl`, `primaryexternalidpsn`
          INTO currentPersonaIdXbl, currentPersonaIdPsn, currentExternalIdXbl, currentExternalIdPsn
          FROM `accountinfo`
          WHERE `originpersonaid` = _originpersonaid
          FOR UPDATE;

        SET userNotFound = (ROW_COUNT() = 0);
        SET currentdatetime = CURRENT_TIMESTAMP();

        SET _primarypersonaidxbl = IF(_primarypersonaidxbl <=> 0, NULL, _primarypersonaidxbl);
        SET _primaryexternalidxbl = IF(_primaryexternalidxbl <=> 0, NULL, _primaryexternalidxbl);
        SET _primarypersonaidpsn = IF(_primarypersonaidpsn <=> 0, NULL, _primarypersonaidpsn);
        SET _primaryexternalidpsn = IF(_primaryexternalidpsn <=> 0, NULL, _primaryexternalidpsn);

        SET initXblIdData = ((currentPersonaIdXbl IS NULL) AND (_primarypersonaidxbl IS NOT NULL));
        SET initPsnIdData = ((currentPersonaIdPsn IS NULL) AND (_primarypersonaidpsn IS NOT NULL));

        IF userNotFound THEN
          INSERT INTO `accountinfo`
            (`accountid`, `originpersonaid`, `canonicalpersona`, `originpersonaname`, `primarypersonaidxbl`, `primarypersonaidpsn`, `primaryexternalidxbl`, `primaryexternalidpsn`, 
              `firstlogindatetime`,`lastlogindatetime`, `previouslogindatetime`, `lastautherror`, `lastpersonaidused`, `lastplatformused`)
            VALUES (_accountid, _originpersonaid, _canonicalpersona, _originpersonaname, _primarypersonaidxbl, _primarypersonaidpsn, _primaryexternalidxbl, _primaryexternalidpsn, 
              currentdatetime, currentdatetime, currentdatetime, _lastautherror, _lastpersonaidused, _lastplatformused);
        ELSE
          UPDATE `accountinfo` SET
            `accountid` = _accountid,
            `originpersonaid` = _originpersonaid,
            `canonicalpersona` = IF(LENGTH(_canonicalpersona) <=> 0, NULL, _canonicalpersona),
            `originpersonaname` = IF(LENGTH(_originpersonaname) <=> 0, NULL, _originpersonaname),
            `primarypersonaidxbl` = IF(_primarypersonaidxbl IS NOT NULL, _primarypersonaidxbl, `primarypersonaidxbl`),
            `primarypersonaidpsn` = IF(_primarypersonaidpsn IS NOT NULL, _primarypersonaidpsn, `primarypersonaidpsn`),
            `primaryexternalidxbl` = IF(_primaryexternalidxbl IS NOT NULL, _primaryexternalidxbl, `primaryexternalidxbl`),
            `primaryexternalidpsn` = IF(_primaryexternalidpsn IS NOT NULL, _primaryexternalidpsn, `primaryexternalidpsn`),
            `previouslogindatetime` = `lastlogindatetime`,
            `lastlogindatetime` = currentdatetime,
            `lastpersonaidused` = _lastpersonaidused,
            `lastplatformused` = _lastplatformused
          WHERE originpersonaid = _originpersonaid;
        END IF;

        COMMIT;
      END;

      SELECT
        IF(userNotFound, 1, 0) as newRowInserted,
        IF(initXblIdData, 1, 0) as initXblData,
        IF(initPsnIdData, 1, 0) as initPsnData;

    END $$
    DELIMITER ;