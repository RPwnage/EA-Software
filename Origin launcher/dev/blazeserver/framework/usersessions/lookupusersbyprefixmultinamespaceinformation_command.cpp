/*************************************************************************************************/
/*!
    \file   lookupusersbyprefixmultinamespaceinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersByPrefixMultiNamespaceCommand

    resolve persona name Prefix lookup requests from client across all supported namespaces.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersbyprefixmultinamespace_stub.h"
namespace Blaze
{

    class LookupUsersByPrefixMultiNamespaceCommand : public LookupUsersByPrefixMultiNamespaceCommandStub
    {
    public:

        LookupUsersByPrefixMultiNamespaceCommand(Message* message, Blaze::LookupUsersByPrefixMultiNamespaceRequest * request, UserSessionsSlave* componentImpl)
            : LookupUsersByPrefixMultiNamespaceCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUsersByPrefixMultiNamespaceCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUsersByPrefixMultiNamespaceCommandStub::Errors execute() override
        {
            UserInfoListByPersonaNameByNamespaceMap resultMap(BlazeStlAllocator("LookupUsersByPrefix::resultMap"));
            const size_t MINIMUM_PREFIX_CHARACTERS = 3; // There is a minimum # of characters that the prefix must include.
            
            if (strlen(mRequest.getPrefixName()) < MINIMUM_PREFIX_CHARACTERS)
            {
                BLAZE_ERR_LOG(
                    Log::USER, 
                    "UserManager::processLookupUsersByPrefixMultiNamespace: Incorrect persona name input, "
                    "The size of persona must be at least " << (static_cast<int> (MINIMUM_PREFIX_CHARACTERS)));
                return LookupUsersByPrefixMultiNamespaceCommandStub::USER_ERR_MINIMUM_CHARACTERS;
            }
            else if (mRequest.getMaxResultCount() <= 0)
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUsersByPrefixMultiNamespace: MaxResultCount required.");
                return LookupUsersByPrefixMultiNamespaceCommandStub::ERR_SYSTEM;
            }
            
            //look up users by prefix persona name
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaNamePrefixMultiNamespace(mRequest.getPrefixName(), mRequest.getMaxResultCount(), resultMap, mRequest.getPrimaryNamespaceOnly(), true /*restrictCrossPlatformResults*/);
            if (err == Blaze::ERR_OK)
            {
                // then complete the lookups
                BlazeIdSet blazeIds; // Make sure we don't return duplicate users (it's possible that a user has the same persona name in multiple namespaces)
                UserDataList& resultList = mResponse.getUserDataList();
                resultList.reserve(resultMap.size());
                for (const auto& namespaceItr : resultMap)
                {
                    for (const auto& personaItr : namespaceItr.second)
                    {
                        for (const auto& userItr : personaItr.second)
                        {
                            if (!blazeIds.insert(userItr->getId()).second)
                                continue;

                            // fill out the user data
                            err = gUserSessionManager->filloutUserData(*userItr, *resultList.pull_back(),
                                (userItr->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), gCurrentUserSession->getClientPlatform());
                            if (err != Blaze::ERR_OK)
                                break;
                        }
                    }
                }
            }
            
            if ((err == Blaze::ERR_OK) && resultMap.empty())
            {
                err = USER_ERR_USER_NOT_FOUND;
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERSBYPREFIXMULTINAMESPACE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
