/*************************************************************************************************/
/*!
\file consolerenameutil.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_CONSOLERENAMEUTIL_H
#define BLAZE_AUTHENTICATION_CONSOLERENAMEUTIL_H

#include <EASTL/string.h>

#include "authentication/util/getpersonautil.h"
#include "framework/usersessions/userinfodb.h"

namespace Blaze
{
    namespace Authentication
    {

        class AuthenticationSlaveImpl;

        class ConsoleRenameUtil
        {
        public:
            ConsoleRenameUtil(const PeerInfo* peerInfo);

            BlazeRpcError doRename(const UserInfoData& userInfoData, UserInfoDbCalls::UpsertUserInfoResults& upsertUserInfoResults, bool updateCrossPlatformOpt, bool broadcastUpdateNotification);

        private:

            typedef enum RenameType
            {
                 ACCOUNT,
                 USER
            } RenameType;
            typedef eastl::deque<UserInfoPtr> UserInfoDeque;

            const char8_t* getPersonaName(RenameType renameType, const UserInfoData& userInfo);
            BlazeRpcError doRenameInternal(RenameType renameType, const UserInfoData& userInfoData);
            BlazeRpcError updateUserByPersonaName(RenameType renameType, const char8_t* oldPersonaName, const char8_t* namespaceName, ClientPlatformType platform, UserInfoDeque& userInfoDeque, eastl::string& nextConflictingPersona);

            // Owned memory
            GetPersonaUtil mGetPersonaUtil;
        };
    }
}
#endif //BLAZE_AUTHENTICATION_CONSOLERENAMEUTIL_H

