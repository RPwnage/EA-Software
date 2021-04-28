#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    ALTER TABLE `userinfo`
      ADD COLUMN `isprimarypersona` tinyint(1) DEFAULT '1' AFTER `crossplatformopt`;

    DROP PROCEDURE IF EXISTS `upsertUserInfo_v6`;
    DELIMITER $$
    CREATE PROCEDURE `upsertUserInfo_v6`(
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
      _currentdatetime bigint(20),
      _crossplatformopt tinyint(1),
      _isprimarypersona tinyint(1))
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

      BEGIN
        SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
        START TRANSACTION;

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
          SET _crossplatformopt = IF(_crossplatformopt IS NULL, '1', _crossplatformopt);
          SET _isprimarypersona = IF(_isprimarypersona IS NULL, '1', _isprimarypersona);
          INSERT INTO `userinfo`
            (`blazeid`, `accountid`, `externalid`, `externalstring`, `persona`, `canonicalpersona`, `personanamespace`, `accountlocale`, `accountcountry`, `status`,
              `firstlogindatetime`,`lastlogindatetime`, `lastautherror`, `lastlocaleused`, `lastcountryused`, `childaccount`, `originpersonaid`, `pidid`, `crossplatformopt`, `isprimarypersona`)
            VALUES (_blazeid, _accountid, _externalid, _externalstring, _persona, _canonicalpersona, _personanamespace, _accountlocale, _accountcountry, _status,
              _currentdatetime, _currentdatetime, _lastautherror, _lastlocaleused, _lastcountryused, _childaccount, _originpersonaid, _pidid, _crossplatformopt, _isprimarypersona);
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
            `pidid` = _pidid,
            `crossplatformopt` = IF(_crossplatformopt IS NULL, `crossplatformopt`, _crossplatformopt),
            `isprimarypersona` = IF(_isprimarypersona IS NULL, `isprimarypersona`, _isprimarypersona)
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