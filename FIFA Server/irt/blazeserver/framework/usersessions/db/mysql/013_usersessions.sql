#
# Database migration script.  End each sql statement with a ;
#
[MIGRATION]

# Upgrade your schema
PLATFORM:
    DROP PROCEDURE IF EXISTS `upsertUserInfo`;
    DELIMITER $$
    CREATE PROCEDURE `upsertUserInfo`(
      _blazeid bigint(20),
      _accountid bigint(20),
      _externalid bigint(20) unsigned,
      _externalblob varbinary(64),
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
      DECLARE currentAccountLocale int(10) unsigned;
      DECLARE currentAccountCountry int(10) unsigned;
      DECLARE currentExternalId bigint(20) unsigned;
      DECLARE currentExternalBlob varbinary(64);
      DECLARE userNotFound BOOL;
      DECLARE initExternalData BOOL;
      DECLARE nullifyExternalIds BOOL;

      DECLARE EXIT HANDLER FOR SQLEXCEPTION
      BEGIN
        ROLLBACK;
        RESIGNAL;
      END;

      START TRANSACTION;
      BEGIN

        SELECT `accountlocale`,`accountcountry`,`externalid`,`externalblob`
          INTO currentAccountLocale, currentAccountCountry, currentExternalId, currentExternalBlob
          FROM `userinfo`
          WHERE `blazeid` = _blazeid
          FOR UPDATE;

        SET userNotFound = (ROW_COUNT() = 0);
        SET initExternalData =
          ((currentExternalId IS NULL OR currentExternalId = 0) AND (_externalid IS NOT NULL AND _externalid != 0)) OR
          ((currentExternalBlob IS NULL OR (LENGTH(currentExternalBlob) = 0)) AND (_externalblob IS NOT NULL AND (LENGTH(_externalblob) != 0)));
        SET nullifyExternalIds = (_externalid IS NULL AND _externalblob IS NULL);
        IF NOT nullifyExternalIds THEN
          SET _externalid = IF(_externalid <=> 0, NULL, _externalid);
          SET _externalblob = IF(LENGTH(_externalblob) <=> 0, NULL, _externalblob);
        END IF;
    
        IF userNotFound THEN
          INSERT INTO `userinfo`
            (`blazeid`, `accountid`, `externalid`, `externalblob`, `persona`, `canonicalpersona`, `personanamespace`, `accountlocale`, `accountcountry`, `status`,
              `firstlogindatetime`,`lastlogindatetime`, `lastautherror`, `lastlocaleused`, `lastcountryused`, `childaccount`, `originpersonaid`, `pidid`)
            VALUES (_blazeid, _accountid, _externalid, _externalblob, _persona, _canonicalpersona, _personanamespace, _accountlocale, _accountcountry, _status,
              _currentdatetime, _currentdatetime, _lastautherror, _lastlocaleused, _lastcountryused, _childaccount, _originpersonaid, _pidid);
        ELSE
          UPDATE `userinfo` SET
            `accountid` = _accountid,
            `externalid` = IF(nullifyExternalIds, NULL, IF(_externalid IS NOT NULL, _externalid, `externalid`)),
            `externalblob` = IF(nullifyExternalIds, NULL, IF(_externalblob IS NOT NULL, _externalblob, `externalblob`)),
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
        currentAccountCountry as previousCountry;

    END $$
    DELIMITER ;
