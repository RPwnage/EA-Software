#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# IMPORTANT! This master script *must* be updated whenever a new 00N_<component>.sql is added.   - Also, update getDbSchemaVersion() in UserSessionManager.

# Upgrade your schema  - NOTE: PLATFORM here refers to XBL/PSN for consoles (not xone/xbsx, ps4/ps5), since they do not have separate accounts for Gen4/Gen5. 
PLATFORM:
    CREATE TABLE `userinfo` (
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
      `isprimarypersona` tinyint(1) DEFAULT '1',
      `lastplatformused` varchar(32) DEFAULT NULL COMMENT 'platform name used since XBL (and PSN) use the same DB for Gen4 and Gen5',
      PRIMARY KEY (`blazeid`),
      UNIQUE KEY `namespace_persona`(`canonicalpersona`, `personanamespace`) USING BTREE,
      UNIQUE KEY `EXTERNAL` (`externalid`, `personanamespace`),
      KEY `userinfo_accountid_idx` (`accountid`) USING BTREE,
      KEY `originpersonaid_idx` (`originpersonaid`) USING BTREE,
      KEY `pidid_idx` (`pidid`) USING BTREE,
      UNIQUE KEY `externalstring_idx` (`externalstring`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

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

COMMON:
    CREATE TABLE `accountinfo` (
      `accountid` bigint(20) NOT NULL,
      `originpersonaid` bigint(20) unsigned DEFAULT NULL,
      `canonicalpersona` varchar(255) DEFAULT NULL COMMENT 'canonical version of origin id',
      `originpersonaname` varchar(255) DEFAULT NULL,
      `primarypersonaidxbl` bigint(20) unsigned DEFAULT NULL COMMENT 'xbox live persona id',
      `primarypersonaidpsn` bigint(20) unsigned DEFAULT NULL COMMENT 'psn persona id',
      `primarypersonaidswitch` bigint(20) unsigned DEFAULT NULL COMMENT 'nintendo switch persona id',
      `primarypersonaidsteam` bigint(20) unsigned DEFAULT NULL COMMENT 'steam persona id',
      `primarypersonaidstadia` bigint(20) unsigned DEFAULT NULL COMMENT 'stadia persona id',
      `primaryexternalidxbl` bigint(20) unsigned DEFAULT NULL COMMENT 'xbox live externalid',
      `primaryexternalidpsn` bigint(20) unsigned DEFAULT NULL COMMENT 'psn externalid',
      `primaryexternalidswitch` varchar(256) DEFAULT NULL COMMENT 'nintendo service account id',
      `primaryexternalidsteam` bigint(20) unsigned DEFAULT NULL COMMENT 'steam externalid',
      `primaryexternalidstadia` bigint(20) unsigned DEFAULT NULL COMMENT 'stadia externalid',
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
      UNIQUE KEY (`primarypersonaidsteam`),
      UNIQUE KEY (`primaryexternalidsteam`),
      UNIQUE KEY (`primarypersonaidstadia`),
      UNIQUE KEY (`primaryexternalidstadia`),
      UNIQUE KEY `originpersonaid_idx` (`originpersonaid`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

    DROP PROCEDURE IF EXISTS `upsertAccountInfo_v6`;
    DELIMITER $$
    CREATE PROCEDURE `upsertAccountInfo_v6`(
      _accountid bigint(20),
      _originpersonaid bigint(20) unsigned,
      _canonicalpersona varchar(255),
      _originpersonaname varchar(255),
      _primarypersonaidxbl bigint(20) unsigned,
      _primarypersonaidpsn bigint(20) unsigned,
      _primarypersonaidswitch bigint(20) unsigned,
      _primarypersonaidsteam bigint(20) unsigned,
      _primarypersonaidstadia bigint(20) unsigned,
      _primaryexternalidxbl bigint(20) unsigned,
      _primaryexternalidpsn bigint(20) unsigned,
      _primaryexternalidswitch varchar(256),
      _primaryexternalidsteam bigint(20) unsigned,
      _primaryexternalidstadia bigint(20) unsigned,
      _lastautherror int(10) unsigned,
      _lastpersonaidused bigint(20) unsigned,
      _lastplatformused varchar(32))
    BEGIN
      DECLARE currentOriginPersonaId bigint(20) unsigned; 
      DECLARE currentExternalIdXbl bigint(20) unsigned;
      DECLARE currentExternalIdPsn bigint(20) unsigned;
      DECLARE currentExternalIdSwitch varchar(256);
      DECLARE currentExternalIdSteam bigint(20) unsigned;
      DECLARE currentExternalIdStadia bigint(20) unsigned;
      DECLARE currentPersonaIdXbl bigint(20) unsigned;
      DECLARE currentPersonaIdPsn bigint(20) unsigned;
      DECLARE currentPersonaIdSwitch bigint(20) unsigned;
      DECLARE currentPersonaIdSteam bigint(20) unsigned;
      DECLARE currentPersonaIdStadia bigint(20) unsigned;
      DECLARE currentdatetime bigint(20);
      DECLARE userNotFound BOOL;
      DECLARE initOriginPersonaId BOOL;
      DECLARE initXblIdData BOOL;
      DECLARE initPsnIdData BOOL;
      DECLARE initSwitchIdData BOOL;
      DECLARE initSteamIdData BOOL;
      DECLARE initStadiaIdData BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      BEGIN
        SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
        START TRANSACTION;

        SELECT `originpersonaid`,`primarypersonaidxbl`,`primarypersonaidpsn`, `primarypersonaidswitch`, `primarypersonaidsteam`, `primarypersonaidstadia`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`, `primaryexternalidsteam`, `primaryexternalidstadia`
          INTO currentOriginPersonaId, currentPersonaIdXbl, currentPersonaIdPsn, currentPersonaIdSwitch, currentPersonaIdSteam, currentPersonaIdStadia, currentExternalIdXbl, currentExternalIdPsn, currentExternalIdSwitch, currentExternalIdSteam, currentExternalIdStadia
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
        SET _primarypersonaidstadia = IF(_primarypersonaidstadia <=> 0, NULL, _primarypersonaidstadia);
        SET _primaryexternalidstadia = IF(_primaryexternalidstadia <=> 0, NULL, _primaryexternalidstadia);

        SET initOriginPersonaId = ((currentOriginPersonaId IS NULL) AND (_originpersonaid IS NOT NULL));
        SET initXblIdData = ((currentPersonaIdXbl IS NULL) AND (_primarypersonaidxbl IS NOT NULL));
        SET initPsnIdData = ((currentPersonaIdPsn IS NULL) AND (_primarypersonaidpsn IS NOT NULL));
        SET initSwitchIdData = ((currentPersonaIdSwitch IS NULL) AND (_primarypersonaidswitch IS NOT NULL));
        SET initSteamIdData = ((currentPersonaIdSteam IS NULL) AND (_primarypersonaidsteam IS NOT NULL));
        SET initStadiaIdData = ((currentPersonaIdStadia IS NULL) AND (_primarypersonaidstadia IS NOT NULL));

        IF userNotFound THEN
          INSERT INTO `accountinfo`
            (`accountid`, `originpersonaid`, `canonicalpersona`, `originpersonaname`, `primarypersonaidxbl`, `primarypersonaidpsn`, `primarypersonaidswitch`, `primarypersonaidsteam`, `primarypersonaidstadia`, `primaryexternalidxbl`, `primaryexternalidpsn`, `primaryexternalidswitch`, `primaryexternalidsteam`,`primaryexternalidstadia`,
              `firstlogindatetime`,`lastlogindatetime`, `previouslogindatetime`, `lastautherror`, `lastpersonaidused`, `lastplatformused`)
            VALUES (_accountid, _originpersonaid, _canonicalpersona, _originpersonaname, _primarypersonaidxbl, _primarypersonaidpsn, _primarypersonaidswitch, _primarypersonaidsteam, _primarypersonaidstadia, _primaryexternalidxbl, _primaryexternalidpsn, _primaryexternalidswitch, _primaryexternalidsteam,_primaryexternalidstadia, 
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
            `primarypersonaidstadia` = IF(_primarypersonaidstadia IS NOT NULL, _primarypersonaidstadia, `primarypersonaidstadia`),
            `primaryexternalidxbl` = IF(_primaryexternalidxbl IS NOT NULL, _primaryexternalidxbl, `primaryexternalidxbl`),
            `primaryexternalidpsn` = IF(_primaryexternalidpsn IS NOT NULL, _primaryexternalidpsn, `primaryexternalidpsn`),
            `primaryexternalidswitch` = IF(_primaryexternalidswitch IS NOT NULL, _primaryexternalidswitch, `primaryexternalidswitch`),
            `primaryexternalidsteam` = IF(_primaryexternalidsteam IS NOT NULL, _primaryexternalidsteam, `primaryexternalidsteam`),
            `primaryexternalidstadia` = IF(_primaryexternalidstadia IS NOT NULL, _primaryexternalidstadia, `primaryexternalidstadia`),
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
        IF(initSteamIdData, 1, 0) as initSteamData,
        IF(initStadiaIdData, 1, 0) as initStadiaData;

    END $$
    DELIMITER ;