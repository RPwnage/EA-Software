/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_USER_INFO_DB_H
#define BLAZE_USER_INFO_DB_H

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "blazerpcerrors.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/userinfo.h"

#include "framework/database/preparedstatement.h"

namespace Blaze
{

namespace UserInfoDbCalls
{
    typedef enum UserInfoCallType
    {
        CALL_UPSERTUSERINFO,
        UPDATE_LAST_USED_LOCALE_AND_ERROR,
        UPDATE_ORIGIN_PERSONA,
        CALLTYPE_MAXSIZE
    } UserInfoCallType;

    typedef struct UserInfoPreparedStatements
    {
        PreparedStatementId statements[CALLTYPE_MAXSIZE];
        PreparedStatementId& operator[](UserInfoCallType callType)
        {
            return statements[callType];
        }
        PreparedStatementId operator[](UserInfoCallType callType) const 
        {
            if (callType >= CALLTYPE_MAXSIZE)
                return INVALID_PREPARED_STATEMENT_ID;
            return statements[callType];
        }
    } UserInfoPreparedStatements;

    typedef struct UpsertUserInfoResults
    {
        bool newRowInserted;
        bool firstSetExternalData;
        uint32_t previousCountry;
        bool changedPlatformInfo;
        UpsertUserInfoResults(uint32_t country) :
            newRowInserted(false), firstSetExternalData(false), previousCountry(country), changedPlatformInfo(false) {}
    } UpsertUserInfoResults;

    typedef eastl::hash_map<uint32_t, UserInfoPreparedStatements> PreparedStatementsByDbIdMap;

    void initUserInfoDbCalls(uint32_t dbId, UserInfoPreparedStatements& statements);
    void shutdownUserInfoDbCalls();

    BlazeRpcError upsertUserInfo(const UserInfoData &data, bool nullExt, bool updateTimeFields, bool updateCrossPlatformOpt, UpsertUserInfoResults& upsertUserInfoResults);

    BlazeRpcError lookupUsersByIds(const BlazeIdVector& blazeIds, UserInfoPtrList& userInfoList, CountByPlatformMap& metricsCountMap, bool checkStatus, bool isRepLagAcceptable);
    BlazeRpcError lookupUsersByIds(BlazeIdSet& blazeIds, UserInfoPtrList& userInfoList, ClientPlatformType platform, bool checkStatus, bool isRepLagAcceptable = true, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr);

    BlazeRpcError lookupUsersByPersonaNamePrefix(const char8_t* personaNamespace, const char8_t* personaName, uint32_t maxResultCount, UserInfoPtrList& userInfoList);
    BlazeRpcError lookupUsersByPersonaNamePrefix(const char8_t* personaNamespace, const char8_t* personaName, ClientPlatformType platform, uint32_t maxResultCount, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr);
    BlazeRpcError lookupUsersByPersonaNamePrefixMultiNamespace(const char8_t* personaName, uint32_t maxResultCount, UserInfoPtrList& userInfoList);
    BlazeRpcError lookupUsersByPersonaNamePrefixMultiNamespace(const char8_t* personaName, ClientPlatformType platform, uint32_t maxResultCount, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr);
    BlazeRpcError lookupUsersByPersonaNames(const char8_t* personaNamespace, const PersonaNameVector& names, UserInfoPtrList& userInfoList, bool checkStatus);
    BlazeRpcError lookupUsersByPersonaNames(const char8_t* personaNamespace, const PersonaNameVector& names, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr);

    BlazeRpcError lookupUsersByPlatformInfo(const PlatformInfoVector& platformInfoVector, UserInfoPtrList& userInfoList, CountByPlatformMap& metricsCountMap, bool checkStatus, bool refreshPlatformInfo = true);
    BlazeRpcError lookupUsersByPlatformInfo(const PlatformInfoVector& platformInfoVector, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr, bool skipAccountInfoLookup = false);

    BlazeRpcError lookupUsersByAccountIds(const AccountIdVector& accountIds, UserInfoPtrList& userInfoList);
    BlazeRpcError lookupUsersByAccountIds(const AccountIdVector& accountIds, ClientPlatformType platform, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr, bool skipAccountInfoLookup = false);

    BlazeRpcError lookupUsersByOriginPersonaIds(const OriginPersonaIdVector &originPersonaIds, UserInfoPtrList& userInfoList, bool checkStatus);
    BlazeRpcError lookupUsersByOriginPersonaIds(const OriginPersonaIdVector &originPersonaIds, ClientPlatformType platform, UserInfoPtrList& userInfoList, bool checkStatus, PlatformInfoByAccountIdMap* cachedPlatformInfo = nullptr);

    BlazeRpcError updateUserInfoAttribute(const BlazeIdVector& blazeIds, UserInfoAttribute attribute, UserInfoAttributeMask mask);
    BlazeRpcError updateUserInfoAttribute(const BlazeIdVector& blazeIds, ClientPlatformType platform, UserInfoAttribute attribute, UserInfoAttributeMask mask, uint32_t& rowsAffected);
    BlazeRpcError updateUserInfoLastUsedLocaleAndError(BlazeId blazeId, ClientPlatformType platform, BlazeRpcError lastAuthError, Locale lastUsedLocale, uint32_t lastUsedCountry);
    BlazeRpcError updateLastLogoutDateTime(BlazeId blazeId, ClientPlatformType platform);

    BlazeRpcError lookupLatLonById(BlazeId blazeId, GeoLocationData& geoLocationData);
    BlazeRpcError lookupLatLonById(BlazeId blazeId, ClientPlatformType platform, GeoLocationData& geoLocationData);
    BlazeRpcError resetGeoIPById(BlazeId blazeId, ClientPlatformType platform);
    BlazeRpcError overrideGeoIPById(const GeoLocationData& overrideData, ClientPlatformType platform, bool updateOptIn);

    BlazeRpcError lookupCrossPlatformOptInById(BlazeId blazeId, ClientPlatformType platform, bool &crossPlatformOptIn);
    BlazeRpcError setCrossPlatformOptInById(const OptInRequest& optInRequest, ClientPlatformType platform);

    BlazeRpcError lookupIsPrimaryPersonaById(BlazeId blazeId, ClientPlatformType platform, bool& isPrimaryPersona);
    BlazeRpcError lookupPrimaryPersonaByAccountId(AccountId nucleusAccountId, ClientPlatformType platform, BlazeId& primaryPersonaBlazeId);
    BlazeRpcError resolveMultiLinkedAccountConflicts(const UserInfoData& data, BlazeIdList& conflictingBlazeIds);

    // Helper methods used by Authentication::ConsoleRenameUtil
    BlazeRpcError resolve1stPartyIdConflicts(const UserInfoData &data);
    BlazeRpcError updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, const char8_t* canonicalPersonaName);
}

}

#endif //BLAZE_USER_INFO_DB_H
