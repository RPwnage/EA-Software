/*************************************************************************************************/
/*!
    \file   lookupusersbypersonamultinamespaceinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersByPersonaMultiNamespaceCommand

    resolve persona name lookup requests from client across all supported namespaces.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersbypersonanamemultinamespace_stub.h"
namespace Blaze
{
    class LookupUsersByPersonaNameMultiNamespaceCommand : public LookupUsersByPersonaNameMultiNamespaceCommandStub
    {
    public:

        LookupUsersByPersonaNameMultiNamespaceCommand(Message* message, Blaze::LookupUsersByPersonaNameMultiNamespaceRequest * request, UserSessionsSlave* componentImpl)
            : LookupUsersByPersonaNameMultiNamespaceCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUsersByPersonaNameMultiNamespaceCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUsersByPersonaNameMultiNamespaceCommandStub::Errors execute() override
        {
            //look up users by persona name
            UserInfoListByPersonaNameByNamespaceMap infoMap;
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaNameMultiNamespace(mRequest.getPersonaName(), infoMap, mRequest.getPrimaryNamespaceOnly(), true /*restrictCrossPlatformResults*/);
            if (err == Blaze::ERR_OK)
            {
                // now complete the lookups
                BlazeIdSet blazeIds; // Make sure we don't return duplicate users (it's possible that a user has the same persona name in multiple namespaces)
                UserDataList& resultList = mResponse.getUserDataList();
                for (const auto& namespaceItr : infoMap)
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
            
            if ((err == Blaze::ERR_OK) && mResponse.getUserDataList().empty())
            {
                err = Blaze::USER_ERR_USER_NOT_FOUND;
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERSBYPERSONANAMEMULTINAMESPACE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
