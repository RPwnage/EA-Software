#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    -- since this might be on _xbl/_psn now, just create the table directly if it's not already there.
    CREATE TABLE IF NOT EXISTS `userinfo` (
      `blazeid` bigint(20) NOT NULL,
      `accountid` bigint(20) NOT NULL,
      `persona` varbinary(255) DEFAULT NULL,
      `personanamespace` varchar(32) DEFAULT NULL,
      `accountlocale` int(10) unsigned NOT NULL DEFAULT '0',
      `accountcountry` int(10) unsigned NOT NULL DEFAULT '0',
      `canonicalpersona` varchar(255) DEFAULT NULL,
      `externalid` bigint(20) unsigned DEFAULT NULL,
      `externalstring` varchar(256) DEFAULT NULL,
      `status` int(10) DEFAULT '1',
      `latitude` float(53) DEFAULT 360.0,
      `longitude` float(53) DEFAULT 360.0,
      `geoopt` tinyint(1) DEFAULT '0',
      `customattribute` bigint(20) unsigned DEFAULT '0',
      `firstlogindatetime` bigint(20) DEFAULT '0',
      `lastlogindatetime` bigint(20) DEFAULT '0',
      `previouslogindatetime` bigint(20) DEFAULT '0',
      `lastlogoutdatetime` bigint(20) DEFAULT '0',
      `lastautherror` int(10) unsigned DEFAULT '0',
      `lastlocaleused` int(10) unsigned DEFAULT '0',
      `lastcountryused` int(10) unsigned DEFAULT '0',
      `childaccount` int(10) DEFAULT '0',
      `originpersonaid` bigint(20) unsigned DEFAULT NULL,
      `pidid` bigint(20) DEFAULT '0',
      `crossplatformopt` tinyint(1) DEFAULT '1',
      `isprimarypersona` tinyint(1) DEFAULT '1',  -- not including lastplatformused because we're altering the table immediately
      PRIMARY KEY (`blazeid`),
      UNIQUE KEY `namespace_persona`(`canonicalpersona`, `personanamespace`) USING BTREE,
      UNIQUE KEY `EXTERNAL` (`externalid`, `personanamespace`),
      KEY `userinfo_accountid_idx` (`accountid`) USING BTREE,
      KEY `originpersonaid_idx` (`originpersonaid`) USING BTREE,
      KEY `pidid_idx` (`pidid`) USING BTREE,
      UNIQUE KEY `externalstring_idx` (`externalstring`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8 ;

    ALTER TABLE `userinfo`
     ADD COLUMN `lastplatformused` varchar(32) DEFAULT NULL AFTER `isprimarypersona`;

    DROP PROCEDURE IF EXISTS `upsertUserInfo_v8`;
    DELIMITER $$
    CREATE PROCEDURE `upsertUserInfo_v8`(
      _blazeid bigint(20),
      _accountid bigint(20),
      _externalid bigint(20) unsigned,
      _externalstring varchar(256),
      _persona varbinary(255),
      _canonicalpersona varchar(255),
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
      _isprimarypersona tinyint(1),
      _lastplatformused varchar(32))
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
          SET _lastplatformused = IF(_lastplatformused IS NULL, '0', _lastplatformused);
          INSERT INTO `userinfo`
            (`blazeid`, `accountid`, `externalid`, `externalstring`, `persona`, `canonicalpersona`, `personanamespace`, `accountlocale`, `accountcountry`, `status`,
              `firstlogindatetime`,`lastlogindatetime`, `lastautherror`, `lastlocaleused`, `lastcountryused`, `childaccount`, `originpersonaid`, `pidid`, `crossplatformopt`, `isprimarypersona`, `lastplatformused`)
            VALUES (_blazeid, _accountid, _externalid, _externalstring, _persona, _canonicalpersona, _personanamespace, _accountlocale, _accountcountry, _status,
              _currentdatetime, _currentdatetime, _lastautherror, _lastlocaleused, _lastcountryused, _childaccount, _originpersonaid, _pidid, _crossplatformopt, _isprimarypersona, _lastplatformused);
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
            `isprimarypersona` = IF(_isprimarypersona IS NULL, `isprimarypersona`, _isprimarypersona), 
            `lastplatformused` = IF(_lastplatformused IS NULL, `lastplatformused`, _lastplatformused)
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

