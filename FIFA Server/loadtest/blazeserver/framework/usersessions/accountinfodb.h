/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_ACCOUNT_INFO_DB_H
#define BLAZE_ACCOUNT_INFO_DB_H

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "blazerpcerrors.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/database/preparedstatement.h"

namespace Blaze
{

namespace AccountInfoDbCalls
{
    void initAccountInfoDbCalls();

    const char8_t* getColumnSuffixForPlatform(ClientPlatformType platform);
    const char8_t* getBlazeIdColumnForPlatform(ClientPlatformType platform);

    BlazeRpcError upsertAccountInfo(const UserInfoData &data);
    BlazeRpcError getPlatformInfoForAccounts(const AccountIdSet& accountIdsToLookUp, UserInfoPtrList& userInfoList, PlatformInfoByAccountIdMap* cachedPlatformInfo);
    BlazeRpcError filloutEaIdsByOriginPersonaName(EaIdentifiers& eaIds);
    BlazeRpcError getBlazeIdsByOriginPersonaNames(const PersonaNameVector& names, BlazeIdsByPlatformMap& blazeIdMap);
    BlazeRpcError getBlazeIdsByOriginPersonaNamePrefix(const char8_t* prefix, BlazeIdsByPlatformMap& blazeIdMap);

    // Fetches complete PlatformInfo for each entry in the supplied PlatformInfoList, setting ClientPlatform to the platform of the most recent login.
    // Uses the identifier specified by CrossPlatformLookupType for db lookups.
    // If 'requestedPlatformOnly' is true, results are excluded from the output list if the most recent login was not on the ClientPlatform specified by
    // the corresponding entry in the input list. ('requestedPlatformOnly' is assumed to be true if the CrossPlatformLookupType is FIRST_PARTY_ID)
    BlazeRpcError getPlatformInfoForMostRecentLogin(CrossPlatformLookupType lookupType, const PlatformInfoList& inputList, PlatformInfoList& outputList, bool requestedPlatformOnly);

    // Helper methods used by Authentication::ConsoleRenameUtil
    BlazeRpcError resolve1stPartyIdConflicts(const UserInfoData &data);
    BlazeRpcError updateOriginPersonaName(AccountId nucleusAccountId, const char8_t* originPersonaName, const char8_t* canonicalPersona);
}

}

#endif //BLAZE_ACCOUNT_INFO_DB_H
