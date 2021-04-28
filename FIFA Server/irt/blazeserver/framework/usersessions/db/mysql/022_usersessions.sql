#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
COMMON:
    ALTER TABLE `accountinfo`
      ADD COLUMN `primarypersonaidsteam` bigint(20) unsigned DEFAULT NULL COMMENT 'steam persona id' AFTER `primarypersonaidswitch`,
      ADD COLUMN `primaryexternalidsteam` bigint(20) unsigned DEFAULT NULL COMMENT 'steam externalid' AFTER `primaryexternalidswitch`,
      ADD UNIQUE KEY (`primarypersonaidsteam`),
      ADD UNIQUE KEY (`primaryexternalidsteam`);

    DROP PROCEDURE IF EXISTS `upsertAccountInfo_v5`;
    DELIMITER $$
    CREATE PROCEDURE `upsertAccountInfo_v5`(
      _accountid bigint(20),
      _originpersonaid bigint(20) unsigned,
      _canonicalpersona varchar(32),
      _originpersonaname varchar(32),
      _primarypersonaidxbl bigint(20) unsigned,
      _primarypersonaidpsn bigint(20) unsigned,
      _primarypersonaidswitch bigint(20) unsigned,
      _primarypersonaidsteam bigint(20) unsigned,
      _primaryexternalidxbl bigint(20) unsigned,
      _primaryexternalidpsn bigint(20) unsigned,
      _primaryexternalidswitch varchar(256),
      _primaryexternalidsteam bigint(20) unsigned,
      _lastautherror int(10) unsigned,
      _lastpersonaidused bigint(20) unsigned,
      _lastplatformused varchar(32))
    BEGIN
      DECLARE currentOriginPersonaId bigint(20) unsigned; 
      DECLARE currentExternalIdXbl bigint(20) unsigned;
      DECLARE currentExternalIdPsn bigint(20) unsigned;
      DECLARE currentExternalIdSwitch varchar(256);
      DECLARE currentExternalIdSteam bigint(20) unsigned;
      DECLARE currentPersonaIdXbl bigint(20) unsigned;
      DECLARE currentPersonaIdPsn bigint(20) unsigned;
      DECLARE currentPersonaIdSwitch bigint(20) unsigned;
      DECLARE currentPersonaIdSteam bigint(20) unsigned;
      DECLARE currentdatetime bigint(20);
      DECLARE userNotFound BOOL;
      DECLARE initOriginPersonaId BOOL;
      DECLARE initXblIdData BOOL;
      DECLARE initPsnIdData BOOL;
      DECLARE initSwitchIdData BOOL;
      DECLARE initSteamIdData BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      BEGIN
        SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
        START TRANSACTION;

        SELECT `originpersonaid`,`primarypersonaidxbl`,`primarypersonaidpsn`, `primarypersonaidswitch`, `primarypersonaidsteam`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`, `primaryexternalidsteam`
          INTO currentOriginPersonaId, currentPersonaIdXbl, currentPersonaIdPsn, currentPersonaIdSwitch, currentPersonaIdSteam, currentExternalIdXbl, currentExternalIdPsn, currentExternalIdSwitch, currentExternalIdSteam
          FROM `accountinfo`
          WHERE `accountid` = _accountid
          FOR UPDATE;

        SET userNotFound = (ROW_COUNT() = 0);
        SET currentdatetime = CURRENT_TIMESTAMP();

        SET _originpersonaid  = IF(_originpersonaid <=> 0, NULL, _originpersonaid);
        SET _primarypersonaidxbl = IF(_primarypersonaidxbl <=> 0, NULL, _primarypersonaidxbl);
        SET _primaryexternalidxbl = IF(_primaryexternalidxbl <=> 0, NULL, _primaryexternalidxbl);
        SET _primarypersonaidpsn = IF(_primarypersonaidpsn <=> 0, NULL, _primarypersonaidpsn);
        SET _primaryexternalidpsn = IF(_primaryexternalidpsn <=> 0, NULL, _primaryexternalidpsn);
        SET _primarypersonaidswitch = IF(_primarypersonaidswitch <=> 0, NULL, _primarypersonaidswitch);
        SET _primaryexternalidswitch = IF(LENGTH(_primaryexternalidswitch) <=> 0, NULL, _primaryexternalidswitch);
        SET _primarypersonaidsteam = IF(_primarypersonaidsteam <=> 0, NULL, _primarypersonaidsteam);
        SET _primaryexternalidsteam = IF(_primaryexternalidsteam <=> 0, NULL, _primaryexternalidsteam);

        SET initOriginPersonaId = ((currentOriginPersonaId IS NULL) AND (_originpersonaid IS NOT NULL));
        SET initXblIdData = ((currentPersonaIdXbl IS NULL) AND (_primarypersonaidxbl IS NOT NULL));
        SET initPsnIdData = ((currentPersonaIdPsn IS NULL) AND (_primarypersonaidpsn IS NOT NULL));
        SET initSwitchIdData = ((currentPersonaIdSwitch IS NULL) AND (_primarypersonaidswitch IS NOT NULL));
        SET initSteamIdData = ((currentPersonaIdSteam IS NULL) AND (_primarypersonaidsteam IS NOT NULL));

        IF userNotFound THEN
          INSERT INTO `accountinfo`
            (`accountid`, `originpersonaid`, `canonicalpersona`, `originpersonaname`, `primarypersonaidxbl`, `primarypersonaidpsn`, `primarypersonaidswitch`, `primarypersonaidsteam`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`, `primaryexternalidsteam`,
              `firstlogindatetime`,`lastlogindatetime`, `previouslogindatetime`, `lastautherror`, `lastpersonaidused`, `lastplatformused`)
            VALUES (_accountid, _originpersonaid, _canonicalpersona, _originpersonaname, _primarypersonaidxbl, _primarypersonaidpsn, _primarypersonaidswitch, _primarypersonaidsteam, _primaryexternalidxbl, _primaryexternalidpsn, _primaryexternalidswitch, _primaryexternalidsteam, 
              currentdatetime, currentdatetime, currentdatetime, _lastautherror, _lastpersonaidused, _lastplatformused);
        ELSE
          IF (_originpersonaid IS NOT NULL) AND (currentOriginPersonaId IS NOT NULL) AND (_originpersonaid != currentOriginPersonaId) THEN
              SET @message_text = CONCAT('Attempt to change existing originpersonaid (', currentOriginPersonaId, ')');
              SIGNAL SQLSTATE '45000'
              SET MESSAGE_TEXT = @message_text;
          END IF;

          UPDATE `accountinfo` SET
            `accountid` = _accountid,
            `originpersonaid` = IF(_originpersonaid IS NOT NULL, _originpersonaid, `originpersonaid`),
            `canonicalpersona` = IF(_canonicalpersona IS NOT NULL, _canonicalpersona, `canonicalpersona`),
            `originpersonaname` = IF(_originpersonaname IS NOT NULL, _originpersonaname, `originpersonaname`),
            `primarypersonaidxbl` = IF(_primarypersonaidxbl IS NOT NULL, _primarypersonaidxbl, `primarypersonaidxbl`),
            `primarypersonaidpsn` = IF(_primarypersonaidpsn IS NOT NULL, _primarypersonaidpsn, `primarypersonaidpsn`),
            `primarypersonaidswitch` = IF(_primarypersonaidswitch IS NOT NULL, _primarypersonaidswitch, `primarypersonaidswitch`),
            `primarypersonaidsteam` = IF(_primarypersonaidsteam IS NOT NULL, _primarypersonaidsteam, `primarypersonaidsteam`),
            `primaryexternalidxbl` = IF(_primaryexternalidxbl IS NOT NULL, _primaryexternalidxbl, `primaryexternalidxbl`),
            `primaryexternalidpsn` = IF(_primaryexternalidpsn IS NOT NULL, _primaryexternalidpsn, `primaryexternalidpsn`),
            `primaryexternalidswitch` = IF(_primaryexternalidswitch IS NOT NULL, _primaryexternalidswitch, `primaryexternalidswitch`),
            `primaryexternalidsteam` = IF(_primaryexternalidsteam IS NOT NULL, _primaryexternalidsteam, `primaryexternalidsteam`),
            `previouslogindatetime` = `lastlogindatetime`,
            `lastlogindatetime` = currentdatetime,
            `lastpersonaidused` = _lastpersonaidused,
            `lastplatformused` = _lastplatformused
          WHERE accountid = _accountid;
        END IF;

        COMMIT;
      END;

      SELECT
        IF(userNotFound, 1, 0) as newRowInserted,
        IF(initOriginPersonaId, 1, 0) as initOriginPersonaId,
        IF(initXblIdData, 1, 0) as initXblData,
        IF(initPsnIdData, 1, 0) as initPsnData,
        IF(initSwitchIdData, 1, 0) as initSwitchData,
        IF(initSteamIdData, 1, 0) as initSteamData;

    END $$
    DELIMITER ;