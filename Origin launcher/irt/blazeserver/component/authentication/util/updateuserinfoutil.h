/*************************************************************************************************/
/*!
\file UpdateUserInfoUtil.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_UPDATEUSERINFOUTIL_H
#define BLAZE_AUTHENTICATION_UPDATEUSERINFOUTIL_H

namespace Blaze
{

namespace Authentication
{

class AuthenticationSlaveImpl;

class UpdateUserInfoUtil
{
public:
    UpdateUserInfoUtil(AuthenticationSlaveImpl &component) {}

    BlazeRpcError updateLocaleAndError(const UserInfo& userInfo, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError) const;
    BlazeRpcError updateLocaleAndErrorByBlazeId(BlazeId personaId, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError);
    BlazeRpcError updateLocaleAndErrorByPersonaName(ClientPlatformType platform, const char8_t* personaNamespace, const char8_t* persona, Locale sessionLocale, uint32_t sessionCountry, BlazeRpcError authError);
};

}
}
#endif //BLAZE_AUTHENTICATION_UPDATEUSERINFOUTIL_H

