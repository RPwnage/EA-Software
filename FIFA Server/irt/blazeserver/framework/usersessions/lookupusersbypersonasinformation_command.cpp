/*************************************************************************************************/
/*!
    \file   lookupusersbypersonasinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersByPersonasCommand

    resolve persona name lookup requests from client across a specified namespace.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersbypersonanames_stub.h"
namespace Blaze
{


    class LookupUsersByPersonaNamesCommand : public LookupUsersByPersonaNamesCommandStub
    {
    public:

        LookupUsersByPersonaNamesCommand(Message* message, Blaze::LookupUsersByPersonaNamesRequest * request, UserSessionsSlave* componentImpl)
            : LookupUsersByPersonaNamesCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUsersByPersonaNamesCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUsersByPersonaNamesCommandStub::Errors execute() override
        {
            BlazeRpcError err;
            PersonaNameVector personaNameLookups;

            const PersonaNameList& personaNameList = mRequest.getPersonaNameList();
            personaNameLookups.reserve(personaNameList.size());
            for (PersonaNameList::const_iterator i = personaNameList.begin(), e = personaNameList.end(); i != e; ++i)
            {
                personaNameLookups.push_back(*i);
            }

            eastl::string targetNamespace(mRequest.getPersonaNamespace());
            bool crossPlatformOptIn;
            const ClientPlatformSet* platformSet = nullptr;
            err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, targetNamespace, crossPlatformOptIn, true /*matchPrimaryNamespace*/);
            if (err != ERR_OK)
            {
                return commandErrorFromBlazeError(err);
            }
            mRequest.setPersonaNamespace(targetNamespace.c_str());

            //look up users by persona name
            UserDataList& resultList = mResponse.getUserDataList();
            for (const auto& platform : *platformSet)
            {
                PersonaNameToUserInfoMap resultMap;
                BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByPersonaNames(personaNameLookups, platform, resultMap, mRequest.getPersonaNamespace());
                if (lookupErr == ERR_OK)
                {
                    for (PersonaNameToUserInfoMap::const_iterator i = resultMap.begin(), end = resultMap.end(); i != end; ++i)
                    {
                        // fill out the user data
                        BlazeRpcError filloutErr = gUserSessionManager->filloutUserData(*i->second.get(), *resultList.pull_back(),
                            (i->second.get()->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), gCurrentUserSession->getClientPlatform());
                        if (filloutErr != Blaze::ERR_OK)
                        {
                            err = filloutErr;
                            break;
                        }
                    }
                }
                else if (lookupErr != USER_ERR_USER_NOT_FOUND)
                {
                    err = lookupErr;
                }
            }

            if (err == ERR_OK && resultList.empty())
                return LookupUsersByPersonaNamesCommandStub::USER_ERR_USER_NOT_FOUND;

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERSBYPERSONANAMES_CREATE_COMPNAME(UserSessionManager)

} // Blaze
