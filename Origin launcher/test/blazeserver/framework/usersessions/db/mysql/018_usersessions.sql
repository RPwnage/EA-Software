#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ALTER `originpersonaid` SET DEFAULT NULL;

    DROP PROCEDURE IF EXISTS `upsertUserInfo_v4`;
    DELIMITER $$
    CREATE PROCEDURE `upsertUserInfo_v4`(
      _blazeid bigint(20),
      _accountid bigint(20),
      _externalid bigint(20) unsigned,
      _externalstring varchar(256),
      _persona varchar(32),
      _canonicalpersona varchar(32),
      _personanamespace varchar(32),
      _accountlocale int(10) unsigned,
      _accountcountry int(10) unsigned,
      _status int(10),
      _lastautherror int(10) unsigned,
      _lastlocaleused int(10) unsigned,
      _lastcountryused int(10) unsigned,
      _childaccount int(10),
      _originpersonaid bigint(20) unsigned,
      _pidid bigint(20),
      _currentdatetime bigint(20))
    BEGIN
      DECLARE currentOriginPersonaId bigint(20) unsigned;
      DECLARE currentAccountLocale int(10) unsigned;
      DECLARE currentAccountCountry int(10) unsigned;
      DECLARE currentExternalId bigint(20) unsigned;
      DECLARE currentExternalString varchar(256);
      DECLARE userNotFound BOOL;
      DECLARE initExternalData BOOL;
      DECLARE nullifyExternalIds BOOL;
      DECLARE changedPlatformInfo BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      START TRANSACTION;
      BEGIN

        SELECT `accountlocale`,`accountcountry`,`externalid`,`externalstring`,`originpersonaid`
          INTO currentAccountLocale, currentAccountCountry, currentExternalId, currentExternalString, currentOriginPersonaId  
          FROM `userinfo`
          WHERE `blazeid` = _blazeid
          FOR UPDATE;

        SET userNotFound = (ROW_COUNT() = 0);
        SET _originpersonaid = IF(_originpersonaid <=> 0, NULL, _originpersonaid);
        SET changedPlatformInfo = (userNotFound) OR (currentExternalId != _externalid) OR (currentExternalString != _externalstring) OR (currentOriginPersonaId IS NULL AND _originpersonaid IS NOT NULL);
        SET initExternalData =
          ((currentExternalId IS NULL OR currentExternalId = 0) AND (_externalid IS NOT NULL AND _externalid != 0)) OR
          ((currentExternalString IS NULL OR (LENGTH(currentExternalString) = 0)) AND (_externalstring IS NOT NULL AND (LENGTH(_externalstring) != 0)));
        SET nullifyExternalIds = (_externalid IS NULL AND _externalstring IS NULL);
        IF NOT nullifyExternalIds THEN
          SET _externalid = IF(_externalid <=> 0, NULL, _externalid);
          SET _externalstring = IF(LENGTH(_externalstring) <=> 0, NULL, _externalstring);
        END IF;

        IF userNotFound THEN
          INSERT INTO `userinfo`
            (`blazeid`, `accountid`, `externalid`, `externalstring`, `persona`, `canonicalpersona`, `personanamespace`, `accountlocale`, `accountcountry`, `status`,
              `firstlogindatetime`,`lastlogindatetime`, `lastautherror`, `lastlocaleused`, `lastcountryused`, `childaccount`, `originpersonaid`, `pidid`)
            VALUES (_blazeid, _accountid, _externalid, _externalstring, _persona, _canonicalpersona, _personanamespace, _accountlocale, _accountcountry, _status,
              _currentdatetime, _currentdatetime, _lastautherror, _lastlocaleused, _lastcountryused, _childaccount, _originpersonaid, _pidid);
        ELSE
          UPDATE `userinfo` SET
            `accountid` = _accountid,
            `externalid` = IF(nullifyExternalIds, NULL, IF(_externalid IS NOT NULL, _externalid, `externalid`)),
            `externalstring` = IF(nullifyExternalIds, NULL, IF(_externalstring IS NOT NULL, _externalstring, `externalstring`)),
            `persona` = IF(LENGTH(_persona) <=> 0, NULL, _persona),
            `canonicalpersona` = IF(LENGTH(_persona) <=> 0, NULL, _canonicalpersona),
            `accountlocale` = _accountlocale,
            `accountcountry` = _accountcountry,
            `status` = _status,
            `previouslogindatetime` = IF(_currentdatetime = 0, `previouslogindatetime`, `lastlogindatetime`),
            `lastlogindatetime` = IF(_currentdatetime = 0, `lastlogindatetime`, _currentdatetime),
            `childaccount` = _childaccount,
            `originpersonaid` = _originpersonaid,
            `pidid` = _pidid
          WHERE blazeid = _blazeid;
        END IF;

        COMMIT;
      END;

      SELECT
        IF(userNotFound, 1, 0) as newRowInserted,
        IF(initExternalData, 1, 0) as firstSetExternalData,
        currentAccountLocale as previousLocale,
        currentAccountCountry as previousCountry,
        IF(changedPlatformInfo, 1, 0) as changedPlatformInfo;

    END $$
    DELIMITER ;

COMMON:
    ALTER TABLE `accountinfo`
      ALTER `originpersonaid` SET DEFAULT NULL;

    DROP PROCEDURE IF EXISTS `upsertAccountInfo_v2`;
    DELIMITER $$
    CREATE PROCEDURE `upsertAccountInfo_v2`(
      _accountid bigint(20),
      _originpersonaid bigint(20) unsigned,
      _canonicalpersona varchar(32),
      _originpersonaname varchar(32),
      _primarypersonaidxbl bigint(20) unsigned,
      _primarypersonaidpsn bigint(20) unsigned,
      _primarypersonaidswitch bigint(20) unsigned,
      _primaryexternalidxbl bigint(20) unsigned,
      _primaryexternalidpsn bigint(20) unsigned,
      _primaryexternalidswitch varchar(256),
      _lastautherror int(10) unsigned,
      _lastpersonaidused bigint(20) unsigned,
      _lastplatformused varchar(32))
    BEGIN
      DECLARE currentOriginPersonaId bigint(20) unsigned; 
      DECLARE currentExternalIdXbl bigint(20) unsigned;
      DECLARE currentExternalIdPsn bigint(20) unsigned;
      DECLARE currentExternalIdSwitch varchar(256);
      DECLARE currentPersonaIdXbl bigint(20) unsigned;
      DECLARE currentPersonaIdPsn bigint(20) unsigned;
      DECLARE currentPersonaIdSwitch bigint(20) unsigned;
      DECLARE currentdatetime bigint(20);
      DECLARE userNotFound BOOL;
      DECLARE initOriginPersonaId BOOL;
      DECLARE initXblIdData BOOL;
      DECLARE initPsnIdData BOOL;
      DECLARE initSwitchIdData BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      START TRANSACTION;
      BEGIN

        SELECT `originpersonaid`,`primarypersonaidxbl`,`primarypersonaidpsn`, `primarypersonaidswitch`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`
          INTO currentOriginPersonaId, currentPersonaIdXbl, currentPersonaIdPsn, currentPersonaIdSwitch, currentExternalIdXbl, currentExternalIdPsn, currentExternalIdSwitch
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

        SET initOriginPersonaId = ((currentOriginPersonaId IS NULL) AND (_originpersonaid IS NOT NULL));
        SET initXblIdData = ((currentPersonaIdXbl IS NULL) AND (_primarypersonaidxbl IS NOT NULL));
        SET initPsnIdData = ((currentPersonaIdPsn IS NULL) AND (_primarypersonaidpsn IS NOT NULL));
        SET initSwitchIdData = ((currentPersonaIdSwitch IS NULL) AND (_primarypersonaidswitch IS NOT NULL));

        IF userNotFound THEN
          INSERT INTO `accountinfo`
            (`accountid`, `originpersonaid`, `canonicalpersona`, `originpersonaname`, `primarypersonaidxbl`, `primarypersonaidpsn`, `primarypersonaidswitch`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`, 
              `firstlogindatetime`,`lastlogindatetime`, `previouslogindatetime`, `lastautherror`, `lastpersonaidused`, `lastplatformused`)
            VALUES (_accountid, _originpersonaid, _canonicalpersona, _originpersonaname, _primarypersonaidxbl, _primarypersonaidpsn, _primarypersonaidswitch, _primaryexternalidxbl, _primaryexternalidpsn, _primaryexternalidswitch,
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
            `canonicalpersona` = IF(LENGTH(_canonicalpersona) <=> 0, NULL, _canonicalpersona),
            `originpersonaname` = IF(LENGTH(_originpersonaname) <=> 0, NULL, _originpersonaname),
            `primarypersonaidxbl` = IF(_primarypersonaidxbl IS NOT NULL, _primarypersonaidxbl, `primarypersonaidxbl`),
            `primarypersonaidpsn` = IF(_primarypersonaidpsn IS NOT NULL, _primarypersonaidpsn, `primarypersonaidpsn`),
            `primarypersonaidswitch` = IF(_primarypersonaidswitch IS NOT NULL, _primarypersonaidswitch, `primarypersonaidswitch`),
            `primaryexternalidxbl` = IF(_primaryexternalidxbl IS NOT NULL, _primaryexternalidxbl, `primaryexternalidxbl`),
            `primaryexternalidpsn` = IF(_primaryexternalidpsn IS NOT NULL, _primaryexternalidpsn, `primaryexternalidpsn`),
            `primaryexternalidswitch` = IF(_primaryexternalidswitch IS NOT NULL, _primaryexternalidswitch, `primaryexternalidswitch`),
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
        IF(initSwitchIdData, 1, 0) as initSwitchData;

    END $$
    DELIMITER ;