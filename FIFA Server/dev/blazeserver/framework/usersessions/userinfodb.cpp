/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/usersessions/userinfodb.h"
#include "framework/usersessions/namehandler.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/accountinfodb.h"

namespace Blaze
{
namespace UserInfoDbCalls
{

bool updatePlatformInfoFromMap(PlatformInfo& platformInfo, const PlatformInfoByAccountIdMap& platformInfoMap)
{
    ClientPlatformType platform = platformInfo.getClientPlatform();
    PlatformInfoByAccountIdMap::const_iterator itr = platformInfoMap.find(platformInfo.getEaIds().getNucleusAccountId());
    if (itr == platformInfoMap.end())
        return false;
    itr->second.copyInto(platformInfo);
    platformInfo.setClientPlatform(platform);
    return true;
}

UserInfoPtr userInfoFromDbRow(const DbRow& row, ClientPlatformType platform, AccountIdSet& accountIdsToLookUp, const PlatformInfoByAccountIdMap* platformInfoMap = nullptr)
{
    UserInfo* userInfo = BLAZE_NEW UserInfo();

    userInfo->setId(row.getInt64("blazeid"));
    AccountId accountId = (AccountId) row.getInt64("accountid");
    ExternalId extId = row.getUInt64("externalid");
    OriginPersonaId originPersonaId = row.getUInt64("originpersonaid");
    userInfo->setPersonaName(row.getString("persona"));
    userInfo->setPersonaNamespace(row.getString("personanamespace"));
    userInfo->setAccountLocale(row.getUInt("accountlocale"));
    userInfo->setAccountCountry(row.getUInt("accountcountry"));
    userInfo->setUserInfoAttribute(row.getUInt64("customattribute"));
    userInfo->setStatus(row.getInt("status"));
    userInfo->setFirstLoginDateTime(row.getInt64("firstlogindatetime") / 1000); //convert to seconds
    userInfo->setLastLoginDateTime(row.getInt64("lastlogindatetime") / 1000); //convert to seconds
    userInfo->setPreviousLoginDateTime(row.getInt64("previouslogindatetime") / 1000); //convert to seconds
    userInfo->setLastLogoutDateTime(row.getInt64("lastlogoutdatetime") / 1000); //convert to seconds
    userInfo->setLastLocaleUsed(row.getUInt("lastlocaleused"));
    userInfo->setLastCountryUsed(row.getUInt("lastcountryused"));
    userInfo->setLastAuthErrorCode(row.getUInt("lastautherror"));
    userInfo->setIsChildAccount((row.getInt("childaccount") > 0) ? true : false);
    userInfo->setOriginPersonaId(originPersonaId);
    userInfo->setPidId(row.getInt64("pidid"));    
    userInfo->setCrossPlatformOptIn((row.getInt("crossplatformopt") != 0) ? true : false);
    userInfo->setIsPrimaryPersona(row.getInt("isprimarypersona") != 0);
    if (!userInfo->getIsPrimaryPersona() && gUserSessionManager->getOnlyShowPrimaryPersonaAccounts())
    {
        accountId = INVALID_ACCOUNT_ID;
        originPersonaId = INVALID_ORIGIN_PERSONA_ID;
        userInfo->setOriginPersonaId(INVALID_ORIGIN_PERSONA_ID);
    }
    
    ClientPlatformType dbLastPlatform = INVALID;
    const char8_t* lastPlatform = row.getString("lastplatformused");
    if (ParseClientPlatformType(lastPlatform, dbLastPlatform))
    {
        if (platform != dbLastPlatform)
        {
            // This indicates that we asked for a info on one platform, and got data on the other one. 
            // We can output a log, but since most of the current logic looks for Gen4 then Gen5, this will be typical
        }

        // Use the platform from the DB, rather than the one we were searching for:
        platform = dbLastPlatform;
    }
    

    userInfo->getPlatformInfo().getEaIds().setNucleusAccountId(accountId);
    userInfo->getPlatformInfo().setClientPlatform(platform);

    if (platformInfoMap == nullptr || !updatePlatformInfoFromMap(userInfo->getPlatformInfo(), *platformInfoMap))
    {
        convertToPlatformInfo(userInfo->getPlatformInfo(), extId, row.getString("externalstring"), accountId, originPersonaId, nullptr, platform);
        if (accountId != INVALID_ACCOUNT_ID)
        {
            accountIdsToLookUp.insert(accountId);
        }
    }

    return userInfo;
}

// See ./framework/usersessions/db/mysql/usersessions.sql for details on the upsertUserInfo_v8() stored procedure.
#define CALL_UPSERT_USER_INFO \
    "CALL upsertUserInfo_v8(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"

#define UPDATE_PERSONA_NAME \
    "UPDATE `userinfo` SET `persona`=?, `canonicalpersona`=?, `status`=? WHERE `accountid`=?"

#define GET_USER_INFO_COMMON \
    "select blazeid,accountid,persona,personanamespace,externalid,externalstring,accountlocale,accountcountry,customattribute," \
    "status,firstlogindatetime,lastlogindatetime,previouslogindatetime,lastlogoutdatetime,lastautherror,lastlocaleused,lastcountryused," \
    "childaccount,originpersonaid,pidid,crossplatformopt,isprimarypersona,lastplatformused  from userinfo "

void initUserInfoDbCalls(uint32_t dbId, UserInfoPreparedStatements& statements)
{
    statements[CALL_UPSERTUSERINFO] = gDbScheduler->registerPreparedStatement(dbId,
        "userinfo_call_upsert_user_info", CALL_UPSERT_USER_INFO);
    statements[UPDATE_LAST_USED_LOCALE_AND_ERROR] = gDbScheduler->registerPreparedStatement(dbId,
        "userinfo_update_auth_locale_and_err", "UPDATE `userinfo` SET `lastautherror` = ?, `lastlocaleused` = ?, `lastcountryused` = ? WHERE `blazeid` = ?");
    statements[UPDATE_ORIGIN_PERSONA] = gDbScheduler->registerPreparedStatement(dbId,
        "userinfo_update_origin_persona_name", UPDATE_PERSONA_NAME);
}

void shutdownUserInfoDbCalls()
{
}

/*! ***************************************************************************/
/*!    \brief "Upserts" a user record into the DB.
 *!    IMPORTANT: 
 *!     Fields lastautherror && lastlocaleused && lastcountryused are inserted but *not* updated, use updateUserInfoLastUsedLocaleAndError() to update them.
       
    \note This method does NOT upsert the user's UserInfoAttribute to db. Call updateUserInfoAttribute() for that.

    \param data[in] The user data to insert/update. 
    \param nullExt[in] If true the externalid and externalstring fields are nulled out regardless of the PlatformInfo in the provided UserInfoData
    \param updateTimeFields[in] If true the previouslogindatetime and lastlogindatetime fields are updated
    \param updateCrossPlatformOpt[in] If true the crossplatformopt field is updated
    \param upsertUserInfoResults.newRowInserted [out] True if a new row was inserted into the table, false if an existing row was update (or on a no-op)
    \param upsertUserInfoResults.firstSetExternalData [out] True if first console login
    \param upsertUserInfoResults.previousCountry [out] Account country before a user record is "upserted" into the DB (same country if new row)
    \param upsertUserInfoResults.changedPlatformInfo [out] True if the user's originpersonaid, externalid, or externalstring was changed (always true when a new row was inserted)
    \return Any error that occurs, or ERR_OK if none.
*******************************************************************************/
BlazeRpcError upsertUserInfo(const UserInfoData &data, bool nullExt, bool updateTimeFields, bool updateCrossPlatformOpt, UpsertUserInfoResults& upsertUserInfoResults)
{
    BlazeRpcError result = ERR_SYSTEM;

    if (data.getId() == INVALID_BLAZE_ID)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].upsertUserInfo: (" << Fiber::getCurrentContext()
            << ") Rejecting attempt to insert user with invalid Blaze id into database.  UserInfoData is " << data);
        return result;
    }

    upsertUserInfoResults.newRowInserted = false;
    upsertUserInfoResults.firstSetExternalData = false;
    upsertUserInfoResults.changedPlatformInfo = false;

    uint32_t dbId = gUserSessionManager->getDbId(data.getPlatformInfo().getClientPlatform());
    DbConnPtr conn = gDbScheduler->getConnPtr(dbId, UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        uint32_t col = 0;

        PreparedStatement* stmt = conn->getPreparedStatement(gUserSessionManager->getUserInfoPreparedStatement(dbId, CALL_UPSERTUSERINFO));
        stmt->setInt64(col++, data.getId());
        stmt->setInt64(col++, data.getPlatformInfo().getEaIds().getNucleusAccountId());
        if (nullExt)
        {
            // Passing null for both the `externalid` and `externalstring` fields causes the upsertUserInfo stored procedure to
            // nullify the two fields.
            stmt->setBinary(col++, nullptr, 0);
            stmt->setBinary(col++, nullptr, 0);
        }
        else
        {
            // Passing 0 for the `externalid` field causes the upsertUserInfo() stored procedure to leave the current value as is.
            stmt->setUInt64(col++, getExternalIdFromPlatformInfo(data.getPlatformInfo()));
            // Passing a nullptr or zero-length string for the `externalstring` field causes the upsertUserInfo() stored procedure to leave the current value as is.
            stmt->setString(col++, data.getPlatformInfo().getExternalIds().getSwitchId());
        }

        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        bool nullPersona = data.getPersonaName()[0] == '\0';  // ConsoleRenameUtil::doRename may set persona names to nullptr when resolving loops/deadlocks in name updates or deactivating invalid users
        if (!nullPersona)
            gUserSessionManager->getNameHandler().generateCanonical(data.getPersonaName(), canonicalPersona, sizeof(canonicalPersona));
        stmt->setString(col++, data.getPersonaName());
        stmt->setString(col++, nullPersona ? nullptr : canonicalPersona);
        stmt->setString(col++, data.getPersonaNamespace());
        stmt->setUInt32(col++, data.getAccountLocale());
        stmt->setUInt32(col++, data.getAccountCountry());
        stmt->setInt32(col++, data.getStatus());
        stmt->setUInt32(col++, data.getLastAuthErrorCode());
        stmt->setUInt32(col++, data.getLastLocaleUsed());
        stmt->setUInt32(col++, data.getLastCountryUsed());
        stmt->setInt32(col++, (data.getIsChildAccount()) ? 1 : 0);
        stmt->setUInt64(col++, data.getPlatformInfo().getEaIds().getOriginPersonaId());
        stmt->setInt64(col++, data.getPidId());

        // The stored procedure will not modify any date/time fields if 0 is passed as the current time.
        stmt->setInt64(col++, (updateTimeFields ? TimeValue::getTimeOfDay().getMillis() : 0));

        // The stored procedure will not modify the crossplatformopt field if nullptr is passed as the crossplatformopt
        if (updateCrossPlatformOpt)
            stmt->setInt8(col++, data.getCrossPlatformOptIn() ? 1 : 0);
        else
            stmt->setBinary(col++, nullptr, 0);

        stmt->setInt8(col++, data.getIsPrimaryPersona() ? 1 : 0);
        stmt->setString(col++, ClientPlatformTypeToString(data.getPlatformInfo().getClientPlatform()));

        DbResultPtr dbresult;
        BlazeRpcError error = conn->executePreparedStatement(stmt, dbresult);
        if (error == DB_ERR_OK)
        {
            const DbRow* row = *dbresult->begin();
            upsertUserInfoResults.newRowInserted = (row->getInt64("newRowInserted") != 0);
            upsertUserInfoResults.firstSetExternalData = (row->getInt64("firstSetExternalData") != 0);
            upsertUserInfoResults.changedPlatformInfo = (row->getInt64("changedPlatformInfo") != 0);

            if (!upsertUserInfoResults.newRowInserted)
            {
                upsertUserInfoResults.previousCountry = row->getUInt("previousCountry");
                if (upsertUserInfoResults.previousCountry == 0)
                {
                    // for backward compatibility (where we don't have an account country saved *yet*), assume the locale's language variant is the country
                    upsertUserInfoResults.previousCountry = LocaleTokenGetCountry(row->getUInt("previousLocale"));
                }
            }
            result = ERR_OK;
        }
        else if (error == DB_ERR_DUP_ENTRY)
        {
            result = USER_ERR_EXISTS;
        }
        else
        {   
            BLAZE_ERR_LOG(Log::USER,"[UserInfoDbCalls].upsertUserInfo: failed to execute prepared statement '" << stmt->getQuery() 
                          << "'. Error(" << error << "):" << getDbErrorString(error) << " blazeId(" << data.getId() << ")");
        }
    }

    return result;
}

/*! ***************************************************************************/
/*! \brief "Updates" a user's last used locale and error in the DB (used for debugging conn problems).
    \param id [in] BlazeId of the user
    \param lastAuthError [in] Last auth error result (normally ERR_OK)
    \param lastUsedLocale [in] Last locale used by the user
    \param lastUsedCountry [in] Last country used by the user
    \return Any error that occurs, or ERR_OK if none.
*******************************************************************************/
BlazeRpcError updateUserInfoLastUsedLocaleAndError(BlazeId blazeId, ClientPlatformType platform, BlazeRpcError lastAuthError, Locale lastUsedLocale, uint32_t lastUsedCountry)
{
    BlazeRpcError result = ERR_SYSTEM;

    uint32_t dbId = gUserSessionManager->getDbId(platform);
    DbConnPtr conn = gDbScheduler->getConnPtr(dbId, UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        PreparedStatement* stmt = conn->getPreparedStatement(gUserSessionManager->getUserInfoPreparedStatement(dbId, UPDATE_LAST_USED_LOCALE_AND_ERROR));
        stmt->setUInt32(0, lastAuthError);
        stmt->setUInt32(1, lastUsedLocale);
        stmt->setUInt32(2, lastUsedCountry);
        stmt->setInt64(3, blazeId);

        DbResultPtr dbresult;
        result = conn->executePreparedStatement(stmt, dbresult);
        if (result != DB_ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER,"[UserInfoDbCalls].updateUserInfoLastUsedLocaleAndError: failed to execute prepared statement '" << stmt->getQuery() 
                          << "'.Error(" << result << "):" << getDbErrorString(result) << " blazeId(" << blazeId << ")");
            return result;
        }
    }

    return result;
}


BlazeRpcError lookupUsersByIds(const BlazeIdVector& blazeIds, UserInfoPtrList& userInfoList, CountByPlatformMap& metricsCountMap, bool checkStatus, bool isRepLagAcceptable)
{
    BlazeIdSet blazeIdSet;
    for (BlazeIdVector::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
    {
        if (*it == INVALID_BLAZE_ID)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByIds: (" << Fiber::getCurrentContext() << ") Cannot lookup user with invalid blaze id");
            continue;
        }
        blazeIdSet.insert(*it);
    }
    if (blazeIdSet.empty())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByIds: (" << Fiber::getCurrentContext() << ") Cannot query database, no valid ids provided");

        // We return ERR_OK here because we may have found cached users and
        // need to return those to the caller. Having only invalid BlazeIds
        // to lookup here indicates a problem with the request, but we're still
        // returning complete data to the caller.
        return ERR_OK;
    }

    BlazeRpcError retError = ERR_OK;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        // We shouldn't need to do additional platform restrictions here because we know that the PersonaId will be mapped to a single DB entry.  (Since XBL/PSN are their own separate DBs now)

        metricsCountMap[hostedPlatform] += blazeIdSet.size();

        BlazeRpcError err = lookupUsersByIds(blazeIdSet, userInfoList, hostedPlatform, checkStatus, isRepLagAcceptable, &cachedPlatformInfo);
        if (blazeIdSet.empty())
            return ERR_OK; // An empty set means we found entries for all (valid) requested ids

        if (err != ERR_OK)
            retError = err;
    }

    return retError;
}

BlazeRpcError lookupUsersByIds(BlazeIdSet& blazeIds, UserInfoPtrList& userInfoList, ClientPlatformType platform, bool checkStatus, bool isRepLagAcceptable, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/)
{
    if (blazeIds.empty())
        return ERR_OK;

    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        const uint32_t userSessionDbId = gUserSessionManager->getDbId(platform);

        // Use DB master to lookup to avoid replication lag if DB tech does not have synchronous write replication. 
        DbConnPtr conn = isRepLagAcceptable ?
            gDbScheduler->getReadConnPtr(userSessionDbId, UserSessionManager::LOGGING_CATEGORY) :
            gDbScheduler->getLagFreeReadConnPtr(userSessionDbId, UserSessionManager::LOGGING_CATEGORY);

        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

            queryBuf->append(GET_USER_INFO_COMMON "where blazeid in (");

            bool isFirstId = true;
            for (BlazeIdSet::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
            {
                queryBuf->append(isFirstId ? "$D" : ",$D", *it);
                isFirstId = false;
            }

            queryBuf->append(")");
            if (checkStatus)
                queryBuf->append(" and status=1");

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
                {
                    UserInfoPtr userInfo = userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo);
                    userInfoList.push_back(userInfo);
                    blazeIds.erase(userInfo->getId());
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByIds: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupLatLonById(BlazeId blazeId, GeoLocationData& geoLocationData)
{
    UserInfoPtr userInfo = gUserSessionManager->getUser(blazeId);
    if (userInfo != nullptr)
        return lookupLatLonById(blazeId, userInfo->getPlatformInfo().getClientPlatform(), geoLocationData);

    // This blazeId wasn't in our local cache, so we need to search for him in all platform-specific dbs
    BlazeRpcError retError = USER_ERR_USER_NOT_FOUND;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        BlazeRpcError err = lookupLatLonById(blazeId, hostedPlatform, geoLocationData);
        if (err == ERR_OK)
            return err;
        if (err != USER_ERR_USER_NOT_FOUND)
            retError = err;
    }
    return retError;
}

BlazeRpcError lookupLatLonById(BlazeId blazeId, ClientPlatformType platform, GeoLocationData& geoLocationData)
{
    BlazeRpcError err = ERR_SYSTEM;
    
    DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("SELECT latitude, longitude, geoopt FROM userinfo where blazeid=$D and status=1", blazeId);

        DbResultPtr result;
        BlazeRpcError error = conn->executeQuery(queryBuf, result);

        if (error == DB_ERR_OK)
        {
            if (!result->empty())
            {
                //Get the first row out
                const DbRow* row = *result->begin();

                bool overriddenGeoIp = false;
                if (fabs(row->getFloat("latitude")) < 360.0f) // latitude values can be between +/- 90.0f
                {
                    overriddenGeoIp = true;
                    geoLocationData.setLatitude(row->getFloat("latitude"));
                }

                if (fabs(row->getFloat("longitude")) < 360.0f) // longitude values can be between +/- 180.0f
                {
                    overriddenGeoIp = true;
                    geoLocationData.setLongitude(row->getFloat("longitude"));
                }

                if (row->getShort("geoopt") == 1)
                {
                    geoLocationData.setOptIn(true);
                }

                geoLocationData.setIsOverridden(overriddenGeoIp);

                err = ERR_OK;
            }
            else
            {
                err = USER_ERR_USER_NOT_FOUND;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupLatLonById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << error << "):" << getDbErrorString(error) << " blazeId(" << blazeId << ")");
        }
    }

    return err;
}

BlazeRpcError resetGeoIPById(BlazeId blazeId, ClientPlatformType platform)
{
    BlazeRpcError err = ERR_SYSTEM;

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

        queryBuf->append("UPDATE userinfo set latitude=$f, longitude=$f, geoopt=0 where blazeid=$D", 360.0f, 360.0f, blazeId);

        DbResultPtr result;
        err = conn->executeQuery(queryBuf, result);

        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].resetGeoIPById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " blazeId(" << blazeId << ")");
        }
    }

    return err;
}


BlazeRpcError overrideGeoIPById(const GeoLocationData& overrideData, ClientPlatformType platform, bool updateOptIn)
{
    BlazeRpcError err = ERR_SYSTEM;

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("UPDATE userinfo SET ");
        
        if (fabs(overrideData.getLatitude()) < 360.0f)
            queryBuf->append("latitude=$f,", overrideData.getLatitude());

        if (fabs(overrideData.getLongitude()) < 360.0f)
            queryBuf->append("longitude=$f,", overrideData.getLongitude());

        if (updateOptIn)
        {
            queryBuf->append("geoopt=$d", overrideData.getOptIn() == true ? 1 : 0);
        }
        else
        {
            queryBuf->trim(1);
        }
        queryBuf->append(" WHERE blazeid=$D", overrideData.getBlazeId());

        DbResultPtr result;
        err = conn->executeQuery(queryBuf, result);

        if (err != ERR_OK)
        {   
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].overrideGeoIPById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " blazeId(" << overrideData.getBlazeId() << ")");
        }
    }

    return err;
}

BlazeRpcError lookupCrossPlatformOptInById(BlazeId blazeId, ClientPlatformType platform, bool &crossPlatformOptIn)
{
    BlazeRpcError err = ERR_SYSTEM;
    crossPlatformOptIn = false;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("SELECT crossplatformopt FROM userinfo where blazeid=$D and status=1", blazeId);

        DbResultPtr result;
        BlazeRpcError error = conn->executeQuery(queryBuf, result);

        if (error == DB_ERR_OK)
        {
            if (!result->empty())
            {
                //Get the first row out
                const DbRow* row = *result->begin();

                if (row->getShort("crossplatformopt") != 0)
                {
                    crossPlatformOptIn = true;
                }

                err = ERR_OK;
            }
            else
            {
                err = USER_ERR_USER_NOT_FOUND;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupCrossPlatformOptInById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << error << "):" << getDbErrorString(error) << " blazeId(" << blazeId << ")");
        }
    }

    return err;
}

BlazeRpcError setCrossPlatformOptInById(const OptInRequest& optInRequest, ClientPlatformType platform)
{
    BlazeRpcError err = ERR_SYSTEM;

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("UPDATE userinfo SET crossplatformopt=$d WHERE blazeid = $D", (optInRequest.getOptIn() == true ? 1 : 0), optInRequest.getBlazeId());

        DbResultPtr result;
        err = conn->executeQuery(queryBuf, result);

        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].setCrossPlatformOptInById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " blazeId(" << optInRequest.getBlazeId() << ")");
        }
    }

    return err;
}


BlazeRpcError lookupUsersByPersonaNamePrefix(const char8_t* personaNamespace, const char8_t* personaName, uint32_t maxResultCount, UserInfoPtrList& userInfoList)
{
    BlazeRpcError retError = ERR_OK;
    uint32_t totalPlatformCount = 0;
    uint32_t searchedPlatformCount = 0;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        if (blaze_strcmp(personaNamespace, gController->getNamespaceFromPlatform(hostedPlatform)) != 0)
            continue;

        ++totalPlatformCount;
        if ((uint32_t)userInfoList.size() >= maxResultCount)
            continue;

        BlazeRpcError err = lookupUsersByPersonaNamePrefix(personaNamespace, personaName, hostedPlatform, maxResultCount, userInfoList, &cachedPlatformInfo);
        if (err != ERR_OK)
            retError = err;
        else
            ++searchedPlatformCount;
    }

    if ((uint32_t)userInfoList.size() >= maxResultCount && searchedPlatformCount < totalPlatformCount)
    {
        BLAZE_TRACE_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPersonaNamePrefix: returned " << userInfoList.size() << " results from " << searchedPlatformCount
            << " out of " << totalPlatformCount << " hosted platforms that use namespace " << personaNamespace);
        return ERR_OK;
    }
    return retError;
}

BlazeRpcError lookupUsersByPersonaNamePrefix(const char8_t* personaNamespace, const char8_t* personaName, ClientPlatformType platform, uint32_t maxResultCount, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
            char8_t canonicalName[MAX_PERSONA_LENGTH];
            gUserSessionManager->getNameHandler().generateCanonical(personaName, canonicalName, sizeof(canonicalName));

            queryBuf->append(GET_USER_INFO_COMMON "force index(namespace_persona) where canonicalpersona LIKE '$L%' and personanamespace='$s'", canonicalName, personaNamespace);

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); (it != end) && ((uint32_t)userInfoList.size() < maxResultCount); ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPersonaNamePrefix: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupUsersByPersonaNamePrefixMultiNamespace(const char8_t* personaName, uint32_t maxResultCount, UserInfoPtrList& userInfoList)
{
    BlazeRpcError retError = ERR_OK;
    uint32_t platformCount = 0;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        BlazeRpcError err = lookupUsersByPersonaNamePrefixMultiNamespace(personaName, hostedPlatform, maxResultCount, userInfoList, &cachedPlatformInfo);
        if ((uint32_t)userInfoList.size() >= maxResultCount)
            break;

        if (err != ERR_OK)
            retError = err;
        else
            ++platformCount;
    }

    if ((uint32_t)userInfoList.size() >= maxResultCount && platformCount < gUserSessionManager->getUserInfoDbPlatforms().size())
    {
        BLAZE_TRACE_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPersonaNamePrefixMultiNamespace: returned " << userInfoList.size() << " results from " << platformCount
            << " out of " << gUserSessionManager->getUserInfoDbPlatforms().size() << " hosted platforms.");
        return ERR_OK;
    }
    return retError;
}

BlazeRpcError lookupUsersByPersonaNamePrefixMultiNamespace(const char8_t* personaName, ClientPlatformType platform, uint32_t maxResultCount, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
            char8_t canonicalName[MAX_PERSONA_LENGTH];
            gUserSessionManager->getNameHandler().generateCanonical(personaName, canonicalName, sizeof(canonicalName));

            queryBuf->append(GET_USER_INFO_COMMON "force index(namespace_persona) where canonicalpersona LIKE '$L%' ", canonicalName);

            queryBuf->append("limit");
            queryBuf->append(" $D", maxResultCount);

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); (it != end) && ((uint32_t)userInfoList.size() < maxResultCount); ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPersonaNamePrefixMultiNamespace: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupUsersByPersonaNames(const char8_t* personaNamespace, const PersonaNameVector& names, UserInfoPtrList& userInfoList, bool checkStatus)
{
    BlazeRpcError retError = ERR_OK;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        if (blaze_strcmp(personaNamespace, gController->getNamespaceFromPlatform(hostedPlatform)) != 0)
            continue;

        BlazeRpcError err = lookupUsersByPersonaNames(personaNamespace, names, hostedPlatform, userInfoList, checkStatus, &cachedPlatformInfo);
        if (err != ERR_OK)
            retError = err;
    }
    return retError;
}

BlazeRpcError lookupUsersByPersonaNames(const char8_t* personaNamespace, const PersonaNameVector& names, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

            queryBuf->append(GET_USER_INFO_COMMON "where canonicalpersona in (");

            //Go through the list of names and see if we already have them
            if (!names.empty())
            {
                char8_t canonicalPersona[MAX_PERSONA_LENGTH];
                for (PersonaNameVector::const_iterator it = names.begin(), end = names.end(); it != end; ++it)
                {
                    gUserSessionManager->getNameHandler().generateCanonical(*it, canonicalPersona, sizeof(canonicalPersona));
                    queryBuf->append("'$s',", canonicalPersona);
                }

                queryBuf->trim(1); // Remove the trailing comma.
            }

            //Complete the query
            queryBuf->append(")");
            if (checkStatus)
                queryBuf->append(" and status=1");
            if ((personaNamespace != nullptr) && (*personaNamespace != '\0'))
                queryBuf->append(" and personanamespace='$s'", personaNamespace);

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPersonaNames: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupUsersByPlatformInfo(const PlatformInfoVector& platformInfoVector, UserInfoPtrList& userInfoList, CountByPlatformMap& metricsCountMap, bool checkStatus, bool refreshPlatformInfo /*= true*/)
{
    PlatformInfoMap platformInfoMap;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (PlatformInfoVector::const_iterator itr = platformInfoVector.begin(); itr != platformInfoVector.end(); ++itr)
    {
        if (!refreshPlatformInfo)
            cachedPlatformInfo[itr->getEaIds().getNucleusAccountId()] = *itr;
        platformInfoMap[itr->getClientPlatform()].push_back(*itr);
    }

    BlazeRpcError retError = ERR_OK;
    for (PlatformInfoMap::iterator itr = platformInfoMap.begin(); itr != platformInfoMap.end(); ++itr)
    {
        ++metricsCountMap[itr->first];

        BlazeRpcError err = lookupUsersByPlatformInfo(itr->second, itr->first, userInfoList, checkStatus, refreshPlatformInfo ? nullptr : &cachedPlatformInfo);
        if (err != ERR_OK)
            retError = err;
    }
    return retError;
}

BlazeRpcError lookupUsersByPlatformInfo(const PlatformInfoVector& platformInfoVector, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/, bool skipAccountInfoLookup /*= false*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

            queryBuf->append(GET_USER_INFO_COMMON "where ");
            bool usesExtString = gController->usesExternalString(platform);
            bool addedSomething = false;

            if (!platformInfoVector.empty())
            {
                for (const PlatformInfo& platformInfo : platformInfoVector)
                {
                    if (usesExtString)
                    {
                        if (!platformInfo.getExternalIds().getSwitchIdAsTdfString().empty())
                        {
                            if (addedSomething)
                            {
                                queryBuf->append(",'$s'", platformInfo.getExternalIds().getSwitchId());
                            }
                            else
                            {
                                queryBuf->append("externalstring in('$s'", platformInfo.getExternalIds().getSwitchId());
                                addedSomething = true;
                            }
                        }
                    }
                    else
                    {

                        ExternalId extId = getExternalIdFromPlatformInfo(platformInfo);
                        if (extId != INVALID_EXTERNAL_ID)
                        {
                            if (addedSomething)
                            {
                                queryBuf->append(",$U", extId);
                            }
                            else
                            {
                                queryBuf->append("externalid in($U", extId);
                                addedSomething = true;
                            }
                        }
                    }
                }
            }

            if (!addedSomething)
            {
                // This is expected functionality.  If we did not provide any valid strings, we return ERR_OK, so that a later function can return ERR_USER_NOT_FOUND.
                BLAZE_TRACE_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPlatformInfo: No valid external ids or external strings in provided PlatformInfoVector");
                return ERR_OK;
            }

            // Complete the query
            queryBuf->append(")");
            if (checkStatus)
                queryBuf->append(" and status=1");

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByPlatformInfo: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!skipAccountInfoLookup && !accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupUsersByAccountIds(const AccountIdVector& accountIds, UserInfoPtrList& userInfoList)
{
    BlazeRpcError retError = ERR_OK;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        BlazeRpcError err = lookupUsersByAccountIds(accountIds, hostedPlatform, userInfoList, &cachedPlatformInfo);
        if (err != ERR_OK)
            retError = err;
    }
    return retError;
}

BlazeRpcError lookupUsersByAccountIds(const AccountIdVector& accountIds, ClientPlatformType platform, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/, bool skipAccountInfoLookup /*= false*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

            queryBuf->append(GET_USER_INFO_COMMON "where accountid in (");

            if (!accountIds.empty())
            {
                for (AccountIdVector::const_iterator it = accountIds.begin(), end = accountIds.end(); it != end; ++it)
                {
                    queryBuf->append("$D,", *it);
                }

                queryBuf->trim(1);
            }

            // Complete the query
            queryBuf->append(") and status=1");

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByAccountIds: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!skipAccountInfoLookup && !accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError lookupUsersByOriginPersonaIds(const OriginPersonaIdVector &originPersonaIds, UserInfoPtrList& userInfoList, bool checkStatus)
{
    BlazeRpcError retError = ERR_OK;
    PlatformInfoByAccountIdMap cachedPlatformInfo;
    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        BlazeRpcError err = lookupUsersByOriginPersonaIds(originPersonaIds, hostedPlatform, userInfoList, checkStatus, &cachedPlatformInfo);
        if (err != ERR_OK)
            retError = err;
    }
    return retError;
}

BlazeRpcError lookupUsersByOriginPersonaIds(const OriginPersonaIdVector &originPersonaIds, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo /*= nullptr*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // Complete the userinfo DB operations and release the local DbConnPtr
    // before the getPlatformInfoForAccounts function grabs a second DbConnPtr.
    AccountIdSet accountIdsToLookup;
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

            queryBuf->append(GET_USER_INFO_COMMON "where originpersonaid in (");

            if (!originPersonaIds.empty())
            {
                for(OriginPersonaIdVector::const_iterator it = originPersonaIds.begin(), end = originPersonaIds.end(); it != end; ++it)
                {
                    queryBuf->append("$D,", *it);
                }

                queryBuf->trim(1);
            }

            // Complete the query
            queryBuf->append(")");
            if (checkStatus)
                queryBuf->append(" and status=1");

            DbResultPtr result;
            err = conn->executeQuery(queryBuf, result);
            if (err == ERR_OK)
            {
                for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
                {
                    userInfoList.push_back(userInfoFromDbRow(**it, platform, accountIdsToLookup, cachedPlatformInfo));
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupUsersByOriginPersonaIds: failed to execute query '" << queryBuf->get()
                    << "'. Error(" << err << "):" << getDbErrorString(err));
            }
        }
    }

    if (!accountIdsToLookup.empty())
        err = AccountInfoDbCalls::getPlatformInfoForAccounts(accountIdsToLookup, userInfoList, cachedPlatformInfo);

    return err;
}

BlazeRpcError updateUserInfoAttribute(const BlazeIdVector& blazeIds, UserInfoAttribute attribute, UserInfoAttributeMask mask)
{
    // To avoid excess db calls, update any platform-specific schema(s) used by locally-cached users,
    // then update the schemas for other platforms only if necessary.
    ClientPlatformSet platformSet;
    bool foundUnknownUser = false;
    for (BlazeIdVector::const_iterator itr = blazeIds.begin(); itr != blazeIds.end(); ++itr)
    {
        UserInfoPtr userInfo = gUserSessionManager->getUser(*itr);
        if (userInfo != nullptr)
            platformSet.insert(userInfo->getPlatformInfo().getClientPlatform());
        else
            foundUnknownUser = true;
    }

    uint32_t totalRowsUpdated = 0;
    BlazeRpcError retError = ERR_OK;
    for (ClientPlatformSet::const_iterator itr = platformSet.begin(); itr != platformSet.end(); ++itr)
    {
        uint32_t rowsUpdated = 0;
        BlazeRpcError err = updateUserInfoAttribute(blazeIds, *itr, attribute, mask, rowsUpdated);
        if (err == ERR_OK)
            totalRowsUpdated += rowsUpdated;
        else
            retError = err;
    }

    if (foundUnknownUser && totalRowsUpdated < blazeIds.size())
    {
        for (ClientPlatformType platform : platformSet)
        {
            if (gController->isPlatformHosted(platform))  // All of the users' platforms should be hosted, but we check here anyways.
            {
                uint32_t rowsUpdated = 0;
                BlazeRpcError err = updateUserInfoAttribute(blazeIds, platform, attribute, mask, rowsUpdated);
                if (err == ERR_OK)
                {
                    totalRowsUpdated += rowsUpdated;
                    if (totalRowsUpdated == blazeIds.size())
                        return ERR_OK;
                }
                else
                {
                    retError = err;
                }
            }
        }
    }

    if (retError == ERR_OK && totalRowsUpdated != blazeIds.size())
    {
        BLAZE_TRACE_LOG(Log::USER, "[UserInfoDbCalls].updateUserInfoAttribute: failed to update some users; " << totalRowsUpdated << "/" << blazeIds.size() << " users were updated.");
    }
    return retError;
}

BlazeRpcError updateUserInfoAttribute(const BlazeIdVector& blazeIds, ClientPlatformType platform, UserInfoAttribute attribute, UserInfoAttributeMask mask, uint32_t& rowsAffected)
{
    BlazeRpcError err = ERR_SYSTEM;
    rowsAffected = 0;

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

        queryBuf->append("UPDATE userinfo SET customattribute=(customattribute&$U)|$U WHERE blazeid IN (", mask, attribute);

        //Go through the list of ids and see if we already have them
        if (!blazeIds.empty())
        {
            for (BlazeIdVector::const_iterator it = blazeIds.begin(), end = blazeIds.end(); it != end; ++it)
            {
                queryBuf->append("$D,", *it);
            }

            queryBuf->trim(1);
        }

        // Complete the query
        queryBuf->append(")");

        DbResultPtr result;
        err = conn->executeQuery(queryBuf, result);
        if (err == ERR_OK)
            rowsAffected = result->getAffectedRowCount();
        else
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].updateUserInfoAttribute: failed to execute query '" << queryBuf->get()
                << "'. Error(" << err << "):" << getDbErrorString(err));
    }

    return err;
}

BlazeRpcError updateLastLogoutDateTime(BlazeId blazeId, ClientPlatformType platform)
{
    BlazeRpcError err = ERR_SYSTEM;

    if (!gController->isPlatformHosted(platform))
    {
        BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].updateLastLogoutDateTime: (" << Fiber::getCurrentContext()
            << ") Rejecting attempt to update 'lastlogoutdatetime' for user on platform '" << ClientPlatformTypeToString(platform) << "' (not hosted).");
        return err;
    }

    if (blazeId == INVALID_BLAZE_ID)
    {
        BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].updateLastLogoutDateTime: (" << Fiber::getCurrentContext()
            << ") Rejecting attempt to update 'lastlogoutdatetime' for user with invalid Blaze id.");
        return err;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);

        queryBuf->append("UPDATE userinfo SET lastlogoutdatetime=$D WHERE blazeid=$D", TimeValue::getTimeOfDay().getMillis(), blazeId);
        
        DbResultPtr result;
        err = conn->executeQuery(queryBuf, result);

        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].updateLastLogoutDateTime: failed to execute query '" << queryBuf->get()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " blazeId(" << blazeId << ")");
        }
    }

    return err;
}

/*! ***************************************************************************/
/*!    \brief Deactivates any user whose external identifier (externalid or externalstring)
              conflicts with the provided UserInfo.
              Returns USER_ERR_USER_NOT_FOUND if no conflicts are found.

       Since the user with the conflicting external identifier must not be online,
       it's safe to deactivate him. His status, persona name, and external identifier
       will be updated if and when he logs in again.
*******************************************************************************/
BlazeRpcError resolve1stPartyIdConflicts(const UserInfoData &data)
{
    if (!has1stPartyPlatformInfo(data.getPlatformInfo()))
        return USER_ERR_USER_NOT_FOUND;

    PlatformInfoVector platformInfoVector;
    platformInfoVector.push_back(data.getPlatformInfo());
    UserInfoPtrList results;
    // Skip the accountinfo lookup -- we're only looking for userinfo entries with conflicting 1st party identifiers;
    // we don't need UserInfo objects with complete PlatformInfo. (Also, if this blazeserver was upgraded from a pre-Trama
    // Blaze version, there may not be any accountinfo entries associated with the conflicting userinfo entries.)
    BlazeRpcError retError = lookupUsersByPlatformInfo(platformInfoVector, data.getPlatformInfo().getClientPlatform(), results, false /*checkStatus*/, nullptr /*cachedPlatformInfo*/, true /*skipAccountInfoLookup*/);
    int32_t conflictCount = 0;
    if (retError == ERR_OK)
    {
        for (UserInfoPtrList::iterator itr = results.begin(); itr != results.end(); ++itr)
        {
            UserInfoPtr userInfo = *itr;
            if (userInfo->getId() != data.getId())
            {
                ++conflictCount;
                bool unused1 = false, unused2 = false;
                uint32_t unused3 = 0;
                userInfo->setPersonaName(nullptr);
                userInfo->setStatus(0);
                userInfo->setIsPrimaryPersona(false);
                BlazeRpcError err = gUserSessionManager->upsertUserInfo(*userInfo, true /*nullExtId*/, false /*updateTimeFields*/, false /*updateCrossPlatformOpt*/, unused1, unused2, unused3, true /*broadcastUpdateNotification*/);
                if (err != ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].resolve1stPartyIdConflicts: Error[" << ErrorHelp::getErrorName(err) << "] deactivating user with blazeId(" << userInfo->getId() << ")");
                    retError = err;
                }
                else
                {
                    eastl::string platformInfoStr;
                    BLAZE_WARN_LOG(Log::USER, "[UserInfoDbCalls].resolve1stPartyIdConflicts: Deactivating user with blazeId(" << userInfo->getId() << ") due to conflict with user with blazeId("
                        << data.getId() << ") and platformInfo(" << platformInfoToString(data.getPlatformInfo(), platformInfoStr) << ")");
                }
            }
        }
    }

    if (retError == ERR_OK && conflictCount == 0)
        return USER_ERR_USER_NOT_FOUND;

    return retError;
}

/*! ***************************************************************************/
/*!    \brief Deactivates any user whose Nucleus account id
              conflicts with the provided UserInfo.
              Returns USER_ERR_USER_NOT_FOUND if no conflicts are found.

       Since the user with the conflicting Nucleus account identifier must not be online,
       it's safe to deactivate him. His status, persona name, and external identifier
       will be updated if and when he logs in again.
*******************************************************************************/
BlazeRpcError resolveMultiLinkedAccountConflicts(const UserInfoData &data, BlazeIdList& conflictingBlazeIds)
{
    conflictingBlazeIds.clear();

    if ((data.getPlatformInfo().getEaIds().getNucleusAccountId() == INVALID_ACCOUNT_ID) || !has1stPartyPlatformInfo(data.getPlatformInfo()))
        return USER_ERR_USER_NOT_FOUND;

    AccountIdVector accountIdVector;
    accountIdVector.push_back(data.getPlatformInfo().getEaIds().getNucleusAccountId());
    UserInfoPtrList results;
    // Skip the accountinfo lookup -- we're only looking for userinfo entries with conflicting Nucleus persona identifiers;
    // we don't need UserInfo objects with complete PlatformInfo. (Also, if this blazeserver was upgraded from a pre-Trama
    // Blaze version, there may not be any accountinfo entries associated with the conflicting userinfo entries.)
    BlazeRpcError retError = lookupUsersByAccountIds(accountIdVector, data.getPlatformInfo().getClientPlatform(), results, nullptr /*cachedPlatformInfo*/, true /*skipAccountInfoLookup*/);
    if (retError == ERR_OK)
    {
        for (UserInfoPtrList::iterator itr = results.begin(); itr != results.end(); ++itr)
        {
            UserInfoPtr userInfo = *itr;
            if (userInfo->getId() != data.getId())
            {
                conflictingBlazeIds.push_back(userInfo->getId());

                bool unused1 = false, unused2 = false;
                uint32_t unused3 = 0;
                userInfo->setIsPrimaryPersona(false);
                BlazeRpcError err = gUserSessionManager->upsertUserInfo(*userInfo, false /*nullExtId*/, false /*updateTimeFields*/, false /*updateCrossPlatformOpt*/, unused1, unused2, unused3, true /*broadcastUpdateNotification*/);
                if (err != ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].resolveMultiLinkedAccountConflicts: Error[" << ErrorHelp::getErrorName(err) << "] deactivating user with blazeId(" << userInfo->getId() << ")");
                    retError = err;
                }
                else
                {
                    eastl::string platformInfoStr;
                    BLAZE_WARN_LOG(Log::USER, "[UserInfoDbCalls].resolveMultiLinkedAccountConflicts: Deactivating user with blazeId(" << userInfo->getId() << ") due to conflict with user with blazeId("
                        << data.getId() << ") and platformInfo(" << platformInfoToString(data.getPlatformInfo(), platformInfoStr) << ")");
                }
            }
        }
    }

    if (retError == ERR_OK && conflictingBlazeIds.empty())
        return USER_ERR_USER_NOT_FOUND;

    return retError;
}

BlazeRpcError lookupIsPrimaryPersonaById(BlazeId blazeId, ClientPlatformType platform, bool& isPrimaryPersona)
{
    BlazeRpcError err = ERR_SYSTEM;
    isPrimaryPersona = false;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("SELECT isprimarypersona FROM userinfo where blazeid=$D and status=1", blazeId);

        DbResultPtr result;
        BlazeRpcError error = conn->executeQuery(queryBuf, result);

        if (error == DB_ERR_OK)
        {
            if (!result->empty())
            {
                //Get the first row out
                const DbRow* row = *result->begin();

                isPrimaryPersona = row->getBool("isprimarypersona");

                err = ERR_OK;
            }
            else
            {
                err = USER_ERR_USER_NOT_FOUND;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupIsPrimaryPersonaById: failed to execute query '" << queryBuf->get()
                << "'. Error(" << error << "):" << getDbErrorString(error) << " blazeId(" << blazeId << ")");
        }
    }

    return err;
}


BlazeRpcError lookupPrimaryPersonaByAccountId(AccountId nucleusAccountId, ClientPlatformType platform, BlazeId& primaryPersonaBlazeId)
{
    BlazeRpcError err = ERR_SYSTEM;
    primaryPersonaBlazeId = INVALID_BLAZE_ID;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(platform), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("SELECT blazeid FROM userinfo where accountid=$D and status=1 and isprimarypersona=1", nucleusAccountId);

        DbResultPtr result;
        BlazeRpcError error = conn->executeQuery(queryBuf, result);

        if (error == DB_ERR_OK)
        {
            if (!result->empty())
            {
                //Get the first row out
                const DbRow* row = *result->begin();

                primaryPersonaBlazeId = row->getInt64("blazeid");

                err = ERR_OK;
            }
            else
            {
                err = USER_ERR_USER_NOT_FOUND;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].lookupPrimaryPersonaByAccountId: failed to execute query '" << queryBuf->get()
                << "'. Error(" << error << "):" << getDbErrorString(error) << " accountId(" << nucleusAccountId << ")");
        }
    }

    return err;
}


BlazeRpcError updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, const char8_t* canonicalPersonaName)
{
    BlazeRpcError retErr = ERR_OK;
    bool nullPersona = (originPersonaName == nullptr || originPersonaName[0] == '\0');

    for (ClientPlatformType hostedPlatform : gUserSessionManager->getUserInfoDbPlatforms())
    {
        ClientPlatformType platform = hostedPlatform;
        if (blaze_strcmp(gController->getNamespaceFromPlatform(platform), NAMESPACE_ORIGIN) != 0)
            continue;

        BlazeRpcError err = ERR_SYSTEM;
        uint32_t dbId = gUserSessionManager->getDbId(platform);
        DbConnPtr conn = gDbScheduler->getConnPtr(dbId, UserSessionManager::LOGGING_CATEGORY);
        if (conn != nullptr)
        {
            uint32_t col = 0;

            PreparedStatement* stmt = conn->getPreparedStatement(gUserSessionManager->getUserInfoPreparedStatement(dbId, UPDATE_ORIGIN_PERSONA));
            if (nullPersona)
            {
                stmt->setBinary(col++, nullptr, 0);
                stmt->setBinary(col++, nullptr, 0);
                stmt->setInt32(col++, 0);
            }
            else
            {
                stmt->setString(col++, originPersonaName);
                stmt->setString(col++, canonicalPersonaName);
                stmt->setInt32(col++, 1);
            }
            stmt->setInt64(col++, nucleusAccountId);

            DbResultPtr dbresult;
            err = conn->executePreparedStatement(stmt, dbresult);
            if (err == ERR_OK)
            {
                if (dbresult->getAffectedRowCount() != 0)
                    return ERR_OK;
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "[UserInfoDbCalls].updateOriginPersonaName: Error[" << ErrorHelp::getErrorName(err) << "] executing prepared statement " << stmt->getQuery());
            }
        }
        if (err != ERR_OK)
            retErr = err;
    }
    return retErr;
}

} // namespace UserInfoDbCalls
} // namespace Blaze
