/*************************************************************************************************/
/*!
\file FinishLoginUtil.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"

#include "authentication/util/updateuserinfoutil.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError UpdateUserInfoUtil::updateLocaleAndError(const UserInfo& userInfo, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError) const
{
    BlazeRpcError err = Blaze::ERR_OK;

    Locale lastLocale = userInfo.getLastLocaleUsed();
    BlazeRpcError lastAuthError = userInfo.getLastAuthErrorCode();

    if (sessionLocale != lastLocale || authError != lastAuthError)
    {
        Blaze::UserInfoData userInfoData;
        userInfo.copyInto(userInfoData);
        userInfoData.setLastAuthErrorCode(authError);
        userInfoData.setLastLocaleUsed(sessionLocale);
        userInfoData.setLastCountryUsed(sessionCountry);

        err = gUserSessionManager->updateLastUsedLocaleAndError(userInfoData);
        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateUserInfoUtil].updateLocaleAndError(): failed to update user(" << userInfoData.getId() << ").");
        }
    }

    return err;
}

BlazeRpcError UpdateUserInfoUtil::updateLocaleAndErrorByBlazeId(BlazeId personaId, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError)
{
    if (personaId == INVALID_BLAZE_ID)
        return Blaze::ERR_SYSTEM;

    UserInfoPtr userInfo;
    BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(personaId, userInfo);

    if ((err == Blaze::ERR_OK) && (userInfo != nullptr)) 
    {
        err = updateLocaleAndError(*userInfo, sessionLocale, sessionCountry, authError);
    }

    return err;
}

BlazeRpcError UpdateUserInfoUtil::updateLocaleAndErrorByPersonaName(ClientPlatformType platform, const char8_t* personaNamespace, const char8_t* persona, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError)
{
    if (*persona == '\0')
        return Blaze::ERR_SYSTEM;

    UserInfoPtr userInfo;

    BlazeRpcError err;
    err = gUserSessionManager->lookupUserInfoByPersonaName(persona, platform, userInfo, personaNamespace, true);

    if ((err == Blaze::ERR_OK) && (userInfo != nullptr))
    {
        err = updateLocaleAndError(*userInfo, sessionLocale, sessionCountry, authError);
    }

    return err;
}
}
}

