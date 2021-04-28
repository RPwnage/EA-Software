/*************************************************************************************************/
/*!
    \file   lookupusersbypersonasmultinamespaceinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersByPersonasMultiNamespaceCommand

    resolve persona name lookup requests from client across all supported namespaces.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersbypersonanamesmultinamespace_stub.h"
namespace Blaze
{
    class LookupUsersByPersonaNamesMultiNamespaceCommand : public LookupUsersByPersonaNamesMultiNamespaceCommandStub
    {
    public:

        LookupUsersByPersonaNamesMultiNamespaceCommand(Message* message, Blaze::LookupUsersByPersonaNamesMultiNamespaceRequest * request, UserSessionsSlave* componentImpl)
            : LookupUsersByPersonaNamesMultiNamespaceCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUsersByPersonaNamesMultiNamespaceCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUsersByPersonaNamesMultiNamespaceCommandStub::Errors execute() override
        {
            BlazeRpcError err;
            PersonaNameVector personaNameLookups;
            UserInfoListByPersonaNameByNamespaceMap resultMap(BlazeStlAllocator("LookupUsersByPersona::resultMap"));

            const PersonaNameList& personaNameList = mRequest.getPersonaNameList();
            personaNameLookups.reserve(personaNameList.size());
            for (PersonaNameList::const_iterator i = personaNameList.begin(), e = personaNameList.end(); i != e; ++i)
            {
                personaNameLookups.push_back(*i);
            }

            //look up users by persona name
            err = gUserSessionManager->lookupUserInfoByPersonaNamesMultiNamespace(personaNameLookups, resultMap, mRequest.getPrimaryNamespaceOnly(), true /*restrictCrossPlatformResults*/);
            if (err == Blaze::ERR_OK)
            {
                // now complete the lookups
                BlazeIdSet blazeIds; // Make sure we don't return duplicate users (it's possible that a user has the same persona name in multiple namespaces)
                UserDataList &resultList = mResponse.getUserDataList();
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
                if (resultList.empty())
                    return LookupUsersByPersonaNamesMultiNamespaceCommandStub::USER_ERR_USER_NOT_FOUND;
            }
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERSBYPERSONANAMESMULTINAMESPACE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
