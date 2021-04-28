/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/blaze.h"
#include "framework/controller/processcontroller.h"
#include "framework/usersessions/accountinfodb.h"
#include "framework/usersessions/namehandler.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

namespace Blaze
{
namespace AccountInfoDbCalls
{

//+ Prepared statements
#define CALL_UPSERT_ACCOUNT_INFO \
    "CALL upsertAccountInfo_v6(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)" 

#define SELECT_EA_IDS_BY_ORIGIN_NAME \
    "SELECT `accountid`,`originpersonaid` from `accountinfo` WHERE `canonicalpersona`=?"

#define UPDATE_ORIGIN_PERSONA_NAME \
    "UPDATE `accountinfo` SET `originpersonaname`=?, `canonicalpersona`=? WHERE `accountid`=?"
//-

//+ Common queries
#define GET_BLAZE_IDS_COMMON \
    "SELECT `originpersonaid`, `primarypersonaidpsn`, `primarypersonaidxbl`, `primarypersonaidswitch`, `primarypersonaidsteam`, `primarypersonaidstadia` FROM `accountinfo`"

#define GET_ACCOUNT_INFO \
    "SELECT `accountid`, `originpersonaid`, `primaryexternalidpsn`,`primaryexternalidxbl`, `primaryexternalidswitch`, `primaryexternalidsteam`, `primaryexternalidstadia`, `originpersonaname`, `lastplatformused` FROM `accountinfo`"

//-

static EA_THREAD_LOCAL PreparedStatementId mCallUpsertAccountInfo;
static EA_THREAD_LOCAL PreparedStatementId mSelectEaIdsByOriginPersonaName;
static EA_THREAD_LOCAL PreparedStatementId mUpdateOriginPersonaName;

void initAccountInfoDbCalls()
{
    mCallUpsertAccountInfo = gDbScheduler->registerPreparedStatement(gUserSessionManager->getDbId(), "accountinfo_call_upsert_account_info", CALL_UPSERT_ACCOUNT_INFO);
    mSelectEaIdsByOriginPersonaName = gDbScheduler->registerPreparedStatement(gUserSessionManager->getDbId(), "accountinfo_select_ea_ids_by_origin_persona_name", SELECT_EA_IDS_BY_ORIGIN_NAME);
    mUpdateOriginPersonaName = gDbScheduler->registerPreparedStatement(gUserSessionManager->getDbId(), "accountinfo_update_origin_persona_name", UPDATE_ORIGIN_PERSONA_NAME);
}

/*! ***************************************************************************/
/*!    \brief "Upserts" a user account info record into the DB

        This includes updating the user's last and previous login times, and the 
        last used personaid and platform.

*******************************************************************************/
BlazeRpcError upsertAccountInfo(const UserInfoData &data)
{
    BlazeRpcError result = ERR_SYSTEM;

    if (data.getPlatformInfo().getEaIds().getNucleusAccountId() == INVALID_ACCOUNT_ID)
    {
        BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].upsertAccountInfo: (" << Fiber::getCurrentContext()
            << ") Rejecting attempt to insert user with invalid Nucleus account id into database.  UserInfoData is " << data);
        return result;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        uint32_t col = 0;

        PreparedStatement* stmt = conn->getPreparedStatement(mCallUpsertAccountInfo);
        stmt->setInt64(col++, data.getPlatformInfo().getEaIds().getNucleusAccountId());
        stmt->setUInt64(col++, data.getPlatformInfo().getEaIds().getOriginPersonaId());

        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        if (data.getPlatformInfo().getEaIds().getOriginPersonaNameAsTdfString().empty())
        {
            stmt->setString(col++, nullptr);
            stmt->setString(col++, nullptr);
        }
        else
        {
            gUserSessionManager->getNameHandler().generateCanonical(data.getPlatformInfo().getEaIds().getOriginPersonaName(), canonicalPersona, sizeof(canonicalPersona));
            stmt->setString(col++, canonicalPersona);
            stmt->setString(col++, data.getPlatformInfo().getEaIds().getOriginPersonaName());
        }

        switch (data.getPlatformInfo().getClientPlatform())
        {
        case xone:
        case xbsx:
            stmt->setUInt64(col++, data.getId());//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setUInt64(col++, 0);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            stmt->setUInt64(col++, data.getPlatformInfo().getExternalIds().getXblAccountId());//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setString(col++, nullptr);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            break;
        case ps4:
        case ps5:
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, data.getId());//psn
            stmt->setUInt64(col++, 0);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, data.getPlatformInfo().getExternalIds().getPsnAccountId());//psn
            stmt->setString(col++, nullptr);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            break;
        case nx:
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setUInt64(col++, data.getId());//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setString(col++, data.getPlatformInfo().getExternalIds().getSwitchId());//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            break;
        case steam:
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setUInt64(col++, 0);//switch
            stmt->setUInt64(col++, data.getId());//steam
            stmt->setUInt64(col++, 0);//stadia
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setString(col++, nullptr);//switch
            stmt->setUInt64(col++, data.getPlatformInfo().getExternalIds().getSteamAccountId());//steam
            stmt->setUInt64(col++, 0);//stadia
            break;
        case stadia:
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setUInt64(col++, 0);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, data.getId());//stadia
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setString(col++, nullptr);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, data.getPlatformInfo().getExternalIds().getStadiaAccountId());//stadia
            break; 
        default:
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setUInt64(col++, 0);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            stmt->setUInt64(col++, 0);//xbl
            stmt->setUInt64(col++, 0);//psn
            stmt->setString(col++, nullptr);//switch
            stmt->setUInt64(col++, 0);//steam
            stmt->setUInt64(col++, 0);//stadia
            break;
        }

        stmt->setUInt32(col++, data.getLastAuthErrorCode());
        stmt->setUInt64(col++, data.getId());
        stmt->setString(col++, ClientPlatformTypeToString(data.getPlatformInfo().getClientPlatform()));

        DbResultPtr dbresult;
        BlazeRpcError error = conn->executePreparedStatement(stmt, dbresult);
        if (error == DB_ERR_OK)
        {
            result = ERR_OK;
        }
        else if (error == DB_ERR_DUP_ENTRY)
        {
            result = USER_ERR_EXISTS;
        }
        else if (error == ERR_DB_USER_DEFINED_EXCEPTION)
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].upsertAccountInfo: Tried to set OriginPersonaId to " << data.getPlatformInfo().getEaIds().getOriginPersonaId() << ", but Nucleus account " <<
                data.getPlatformInfo().getEaIds().getNucleusAccountId() << " is already associated with a different OriginPersonaId. This should never happen!");
            result = ERR_SYSTEM;
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].upsertAccountInfo: failed to execute prepared statement '" << stmt->getQuery()
                << "'. Error(" << error << "):" << getDbErrorString(error) << " blazeId(" << data.getId() << ")");
        }
    }
    return result;
}

void filloutPlatformInfoFromDbRow(const DbRow* dbRow, PlatformInfo& platformInfo)
{
    platformInfo.getExternalIds().setXblAccountId(dbRow->getUInt64("primaryexternalidxbl"));
    platformInfo.getExternalIds().setPsnAccountId(dbRow->getUInt64("primaryexternalidpsn"));
    platformInfo.getExternalIds().setSwitchId(dbRow->getString("primaryexternalidswitch"));
    platformInfo.getExternalIds().setSteamAccountId(dbRow->getUInt64("primaryexternalidsteam"));
    platformInfo.getExternalIds().setStadiaAccountId(dbRow->getUInt64("primaryexternalidstadia"));
    // Only set the personas if this is the primary login
    // We should not get here for any non-primary personas 
    platformInfo.getEaIds().setOriginPersonaName(dbRow->getString("originpersonaname"));
    platformInfo.getEaIds().setOriginPersonaId(dbRow->getUInt64("originpersonaid"));
}

BlazeRpcError getPlatformInfoForAccounts(const AccountIdSet& accountIdsToLookUp, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo)
{
    if (accountIdsToLookUp.empty())
        return USER_ERR_USER_NOT_FOUND;

    BlazeRpcError error = ERR_SYSTEM;
    
    int dbOpCount = 0;
    do
    {
        DbConnPtr conn = (dbOpCount == 0) ?
            gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY) :
            gDbScheduler->getConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);

        if (conn != nullptr)
        {
            QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
            queryBuf->append(GET_ACCOUNT_INFO " WHERE accountid IN(");
            for (const auto& accountId : accountIdsToLookUp)
                queryBuf->append("$D,", accountId);

            queryBuf->trim(1);
            queryBuf->append(")");

            DbResultPtr result;
            error = conn->executeQuery(queryBuf, result);
            if (error == ERR_OK && result->empty())
                error = USER_ERR_USER_NOT_FOUND;

            if (error == ERR_OK)
            {
                typedef eastl::hash_map<AccountId, UserInfoPtrList> UserInfoByAccountId;
                UserInfoByAccountId userInfoByAccountId;
                for (auto& userIt : userInfoList)
                    userInfoByAccountId[userIt->getPlatformInfo().getEaIds().getNucleusAccountId()].push_back(userIt);

                PlatformInfoVector platformList;
                for (DbResult::const_iterator rit = result->begin(); rit != result->end(); ++rit)
                {
                    const DbRow* dbRow = *rit;
                    AccountId accountId = dbRow->getInt64("accountid");
                    UserInfoByAccountId::iterator userIt = userInfoByAccountId.find(accountId);
                    if (userIt != userInfoByAccountId.end() && !userIt->second.empty())
                    {
                        PlatformInfo platformInfo;
                        platformInfo.getEaIds().setNucleusAccountId(accountId);
                        filloutPlatformInfoFromDbRow(dbRow, platformInfo);
                        if (cachedPlatformInfo != nullptr)
                            (*cachedPlatformInfo)[platformInfo.getEaIds().getNucleusAccountId()] = platformInfo;

                        for (auto& it : userIt->second)
                        {
                            ClientPlatformType platform = it->getPlatformInfo().getClientPlatform();
                            platformInfo.copyInto(it->getPlatformInfo());
                            it->getPlatformInfo().setClientPlatform(platform);
                        }
                    }
                }
            }
            else if (error != USER_ERR_USER_NOT_FOUND)
                BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].getPlatformInfoForAccounts: Error(" << ErrorHelp::getErrorName(error) << ") executing query: " << queryBuf->c_str());
        }

        ++dbOpCount;

    } while (dbOpCount < 2 && error == USER_ERR_USER_NOT_FOUND); // < 2 -> first attempt with lagFreeRead and 2nd attempt with Master node

    return error;
}

const char8_t* getColumnSuffixForPlatform(ClientPlatformType platform)
{
    switch (platform)
    {
    case xone:
    case xbsx:
        return "xbl";
    case ps4:
    case ps5:
        return "psn";
    case nx:
        return "switch";
    case steam:
        return "steam";
    case stadia:
        return "stadia"; 
    default:
        return nullptr;
    }
}

const char8_t* getBlazeIdColumnForPlatform(ClientPlatformType platform)
{
    switch (platform)
    {
    case pc:
        return "originpersonaid";
    case xone:
    case xbsx:
        return "primarypersonaidxbl";
    case ps4:
    case ps5:
        return "primarypersonaidpsn";
    case nx:
        return "primarypersonaidswitch";
    case steam:
        return "primarypersonaidsteam";
    case stadia:
        return "primarypersonaidstadia"; 
    default:
        return nullptr;
    }
}

void parseBlazeIdQuery(const DbResultPtr result, BlazeIdsByPlatformMap& blazeIdMap)
{
    for (DbResult::const_iterator it = result->begin(); it != result->end(); ++it)
    {
        for (const auto& platform : gController->getHostedPlatforms())
        {
            const char8_t* blazeIdColumn = getBlazeIdColumnForPlatform(platform);
            if (blazeIdColumn != nullptr)
            {
                BlazeId blazeId = (*it)->getUInt64(blazeIdColumn);
                if (blazeId != INVALID_BLAZE_ID)
                    blazeIdMap[platform].push_back(blazeId);
            }
        }
    }
}

BlazeRpcError getBlazeIdsByOriginPersonaNames(const PersonaNameVector& names, BlazeIdsByPlatformMap& blazeIdMap)
{
    BlazeRpcError retError = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append(GET_BLAZE_IDS_COMMON " WHERE `canonicalpersona` in (");
        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        for (const auto& name : names)
        {
            gUserSessionManager->getNameHandler().generateCanonical(name, canonicalPersona, sizeof(canonicalPersona));
            queryBuf->append("'$s',", canonicalPersona);
        }

        queryBuf->trim(1); // Remove the trailing comma.
        queryBuf->append(")");  // Complete the query

        DbResultPtr result;
        retError = conn->executeQuery(queryBuf, result);
        if (retError == ERR_OK)
            parseBlazeIdQuery(result, blazeIdMap);
        else
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].getBlazeIdsByOriginPersonaNames: failed to execute query '" << queryBuf->get()
                << "'. Error(" << retError << "):" << getDbErrorString(retError));
    }
    return retError;
}

BlazeRpcError getBlazeIdsByOriginPersonaNamePrefix(const char8_t* prefix, BlazeIdsByPlatformMap& blazeIdMap)
{
    BlazeRpcError retError = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        gUserSessionManager->getNameHandler().generateCanonical(prefix, canonicalPersona, sizeof(canonicalPersona));

        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append(GET_BLAZE_IDS_COMMON " force index(canonicalpersona) WHERE `canonicalpersona` LIKE '$L%'", canonicalPersona);

        DbResultPtr result;
        retError = conn->executeQuery(queryBuf, result);
        if (retError == ERR_OK)
            parseBlazeIdQuery(result, blazeIdMap);
        else
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].getBlazeIdsByOriginPersonaNamePrefix: failed to execute query '" << queryBuf->get()
                << "'. Error(" << retError << "):" << getDbErrorString(retError));
    }
    return retError;
}

void platformInfoFromDbResult(const DbResultPtr& result, PlatformInfoList& platformList)
{
    platformList.reserve(result->size());
    for (DbResult::const_iterator it = result->begin(); it != result->end(); ++it)
    {
        const DbRow* dbRow = *it;
        ClientPlatformType platform = INVALID;
        const char8_t* lastPlatform = dbRow->getString("lastplatformused");
        AccountId accountId = dbRow->getInt64("accountid");
        if (!ParseClientPlatformType(lastPlatform, platform))
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].platformInfoFromDbResult: Failed to parse last platform used (" << lastPlatform << ") for entry with accountid (" << accountId << ")");
            continue;
        }
        PlatformInfoPtr platformInfo = platformList.pull_back();
        platformInfo->setClientPlatform(platform);
        platformInfo->getEaIds().setNucleusAccountId(accountId);
        filloutPlatformInfoFromDbRow(dbRow, *platformInfo);
    }
}

BlazeRpcError executeQueryForLookupType(DbConnPtr& dbConn, CrossPlatformLookupType lookupType, ClientPlatformType platform, const PlatformInfoList& inputList, PlatformInfoList& outputList)
{
    if (inputList.empty())
        return USER_ERR_USER_NOT_FOUND;

    QueryPtr queryBuf = DB_NEW_QUERY_PTR(dbConn);
    if (platform != INVALID)
        queryBuf->append(GET_ACCOUNT_INFO " WHERE lastplatformused = '$s' AND", ClientPlatformTypeToString(platform));
    else
        queryBuf->append(GET_ACCOUNT_INFO " WHERE");

    // PC uses the NucleusAccountId instead of a 1st-party id
    if (lookupType == FIRST_PARTY_ID && platform == pc)
        lookupType = NUCLEUS_ACCOUNT_ID;

    switch (lookupType)
    {
    case NUCLEUS_ACCOUNT_ID:
    {
        queryBuf->append(" accountid IN (");
        for (const auto& platformInfo : inputList)
            queryBuf->append("$D,", platformInfo->getEaIds().getNucleusAccountId());
    }
    break;
    case ORIGIN_PERSONA_ID:
    {
        queryBuf->append(" originpersonaid IN (");
        for (const auto& platformInfo : inputList)
            queryBuf->append("$U,", platformInfo->getEaIds().getOriginPersonaId());
    }
    break;
    case ORIGIN_PERSONA_NAME:
    {
        queryBuf->append(" canonicalpersona IN (");
        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        for (const auto& platformInfo : inputList)
        {
            gUserSessionManager->getNameHandler().generateCanonical(platformInfo->getEaIds().getOriginPersonaName(), canonicalPersona, sizeof(canonicalPersona));
            queryBuf->append("'$s',", canonicalPersona);
        }
    }
    break;
    case FIRST_PARTY_ID:
    {
        const char8_t* columnSuffix = getColumnSuffixForPlatform(platform);
        if (columnSuffix == nullptr)
            return USER_ERR_USER_NOT_FOUND;
        queryBuf->append(" primaryexternalid$s IN (", columnSuffix);
        bool addedSomething = false;
        for (const auto& platformInfo : inputList)
        {
            if (platform == nx)
            {
                if (!platformInfo->getExternalIds().getSwitchIdAsTdfString().empty())
                {
                    addedSomething = true;
                    queryBuf->append("'$s',", platformInfo->getExternalIds().getSwitchId());
                }
            }
            else
            {
                ExternalId externalId = getExternalIdFromPlatformInfo(*platformInfo);
                if (externalId != INVALID_EXTERNAL_ID)
                {
                    addedSomething = true;
                    queryBuf->append("$U,", externalId);
                }
            }
        }
        if (!addedSomething)
            return USER_ERR_USER_NOT_FOUND;
    }
    break;
    default:
        BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].executeQueryForLookupType: Unsupported lookup type: " << CrossPlatformLookupTypeToString(lookupType));
        return ERR_SYSTEM;
    }

    queryBuf->trim(1);
    queryBuf->append(")");

    DbResultPtr result;
    BlazeRpcError err = dbConn->executeQuery(queryBuf, result);
    if (err == ERR_OK)
        platformInfoFromDbResult(result, outputList);
    else if (err != USER_ERR_USER_NOT_FOUND)
        BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].executeQueryForLookupType: Error(" << ErrorHelp::getErrorName(err) << ") executing query: " << queryBuf->c_str());

    return err;
}

BlazeRpcError getPlatformInfoForMostRecentLogin(CrossPlatformLookupType lookupType, const PlatformInfoList& inputList, PlatformInfoList& outputList, bool requestedPlatformOnly)
{
    BlazeRpcError retError = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        if (requestedPlatformOnly || lookupType == CrossPlatformLookupType::FIRST_PARTY_ID)
        {
            retError = ERR_OK;
            typedef eastl::map<ClientPlatformType, PlatformInfoList> PlatformInfoListByPlatformMap;
            PlatformInfoListByPlatformMap platformInfoMap;
            for (const auto& platformInfo : inputList)
                platformInfoMap[platformInfo->getClientPlatform()].push_back(platformInfo.get());

            for (const auto& platItr : platformInfoMap)
            {
                BlazeRpcError err = executeQueryForLookupType(conn, lookupType, platItr.first, platItr.second, outputList);
                if (err != ERR_OK && err != USER_ERR_USER_NOT_FOUND)
                    retError = err;
            }
        }
        else
        {
            retError = executeQueryForLookupType(conn, lookupType, INVALID, inputList, outputList);
        }
    }
    return retError;
}

/*! ***************************************************************************/
/*!    \brief Finds any user(s) whose platform externalid/personaid conflict with the
              provided UserInfo, and sets both the externalid and personaid to nullptr.
              Returns USER_ERR_USER_NOT_FOUND if no conflicts are found.

       Since the user with the conflicting externalid/personaid must not be online
       on the affected platform, it's safe to postpone updating his externalid and
       personaid until the next time he logs in on that platform.
*******************************************************************************/
BlazeRpcError resolve1stPartyIdConflicts(const UserInfoData &data)
{
    const char8_t* columnSuffix = getColumnSuffixForPlatform(data.getPlatformInfo().getClientPlatform());
    if (columnSuffix == nullptr)
        return USER_ERR_USER_NOT_FOUND;

    eastl::vector<AccountId> accountIdList;
    eastl::string multipleResultsSelectQueryStr;
    {
        DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
        if (readConn != nullptr)
        {
            QueryPtr selectQuery = DB_NEW_QUERY_PTR(readConn);
            selectQuery->append("SELECT accountid FROM accountinfo WHERE primarypersonaid$s=$U OR primaryexternalid$s=$U", columnSuffix, data.getId(), columnSuffix, getExternalIdFromPlatformInfo(data.getPlatformInfo()));
            DbResultPtr result;
            BlazeRpcError error = readConn->executeQuery(selectQuery, result);
            if (error != DB_ERR_OK)
            {
                BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].resolve1stPartyIdConflicts: failed to execute query '" << selectQuery->get()
                    << "'. Error(" << error << "):" << getDbErrorString(error));
                return error;
            }

            for (DbResult::const_iterator it = result->begin(); it != result->end(); ++it)
            {
                const DbRow* dbRow = *it;
                AccountId accountId = dbRow->getInt64("accountid");
                if (accountId == data.getPlatformInfo().getEaIds().getNucleusAccountId())
                    continue;

                accountIdList.push_back(accountId);
            }

            // Multiple rows will be affected. Save the selectQuery string for a WARN later.
            if (accountIdList.size() > 1)
            {
                multipleResultsSelectQueryStr = selectQuery->c_str();
            }
        }
    }

    if (accountIdList.empty())
        return USER_ERR_USER_NOT_FOUND;

    BlazeRpcError error = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        QueryPtr updateQuery = DB_NEW_QUERY_PTR(conn);
        uint32_t resultCount = 0;
        for (AccountId accountId : accountIdList)
        {
            if (resultCount == 0)
            {
                updateQuery->append("UPDATE accountinfo SET primarypersonaid$s=NULL, primaryexternalid$s=NULL WHERE accountid in ($D", columnSuffix, columnSuffix, accountId);
            }
            else // resultCount > 0
            {
                updateQuery->append(",$D", accountId);
            }
            ++resultCount;
        }

        if (resultCount > 1)
        {
            BLAZE_WARN_LOG(Log::USER, "[AccountInfoDbCalls].resolve1stPartyIdConflicts: Multiple rows will be affected. Search query: '" << multipleResultsSelectQueryStr << "'; Update query: '" << updateQuery->c_str() << "'");
        }
        updateQuery->append(")");
        error = conn->executeQuery(updateQuery);

        if (error != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].resolve1stPartyIdConflicts: failed to execute query '" << updateQuery->get()
                << "'. Error(" << error << "):" << getDbErrorString(error));
        }
    }
    return error;
}

BlazeRpcError updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, const char8_t* canonicalPersona)
{
    BlazeRpcError err = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        uint32_t col = 0;
        PreparedStatement* stmt = conn->getPreparedStatement(mUpdateOriginPersonaName);
        if (originPersonaName == nullptr)
        {
            stmt->setBinary(col++, nullptr, 0);
            stmt->setBinary(col++, nullptr, 0);
        }
        else
        {
            stmt->setString(col++, originPersonaName);
            stmt->setString(col++, canonicalPersona);
        }
        stmt->setInt64(col++, nucleusAccountId);

        DbResultPtr dbresult;
        err = conn->executePreparedStatement(stmt, dbresult);
        if (err != DB_ERR_OK)
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].updateOriginPersonaName: failed to execute prepared statement '" << stmt->getQuery()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " nucleusAccountId(" << nucleusAccountId << ")");

            if (err == ERR_DB_DUP_ENTRY)
                err = USER_ERR_EXISTS;
        }
    }

    return err;
}

BlazeRpcError filloutEaIdsByOriginPersonaName(EaIdentifiers& eaIds)
{
    eaIds.setOriginPersonaId(INVALID_ORIGIN_PERSONA_ID);
    eaIds.setNucleusAccountId(INVALID_ACCOUNT_ID);

    if (eaIds.getOriginPersonaNameAsTdfString().empty())
        return USER_ERR_USER_NOT_FOUND;

    BlazeRpcError err = ERR_SYSTEM;
    DbConnPtr conn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(), UserSessionManager::LOGGING_CATEGORY);
    if (conn != nullptr)
    {
        PreparedStatement* stmt = conn->getPreparedStatement(mSelectEaIdsByOriginPersonaName);

        char8_t canonicalPersona[MAX_PERSONA_LENGTH];
        gUserSessionManager->getNameHandler().generateCanonical(eaIds.getOriginPersonaName(), canonicalPersona, sizeof(canonicalPersona));

        stmt->setString(0, canonicalPersona);
        DbResultPtr dbresult;
        err = conn->executePreparedStatement(stmt, dbresult);
        if (err == DB_ERR_OK)
        {
            if (dbresult->empty())
                return USER_ERR_USER_NOT_FOUND;

            const DbRow* row = *dbresult->begin();
            eaIds.setOriginPersonaId(row->getUInt64("originpersonaid"));
            eaIds.setNucleusAccountId(row->getInt64("accountid"));
        }
        else
        {
            BLAZE_ERR_LOG(Log::USER, "[AccountInfoDbCalls].filloutEaIdsByOriginPersonaName: failed to execute prepared statement '" << stmt->getQuery()
                << "'. Error(" << err << "):" << getDbErrorString(err) << " originPersonaName(" << eaIds.getOriginPersonaName() << ")");
        }
    }
    return err;
}

} // namespace AccountInfoDbCalls
} // namespace Blaze
