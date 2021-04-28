/*************************************************************************************************/
/*!
    \file   lookupusersbyprefixinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUsersByPrefixCommand

    resolve persona name Prefix lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersbyprefix_stub.h"
namespace Blaze
{
    class LookupUsersByPrefixCommand : public LookupUsersByPrefixCommandStub
    {
    public:

        LookupUsersByPrefixCommand(Message* message, Blaze::LookupUsersByPrefixRequest * request, UserSessionsSlave* componentImpl)
            : LookupUsersByPrefixCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUsersByPrefixCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUsersByPrefixCommandStub::Errors execute() override
        {
            LookupUsersByPrefixCommandStub::Errors rc;
            BlazeRpcError err;
            const size_t MINIMUM_PREFIX_CHARACTERS = 3; // There is a minimum # of characters that the prefix must include.
            
            if ( strlen(mRequest.getPrefixName()) >= MINIMUM_PREFIX_CHARACTERS 
                 && mRequest.getMaxResultCount() > 0 )
            {
                eastl::string targetNamespace(mRequest.getPersonaNamespace());
                bool crossPlatformOptIn;
                const ClientPlatformSet* platformSet = nullptr;
                err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, targetNamespace, crossPlatformOptIn, mRequest.getPrimaryNamespaceOnly());
                if (err != ERR_OK)
                {
                    return commandErrorFromBlazeError(err);
                }
                mRequest.setPersonaNamespace(targetNamespace.c_str());

                //look up user by prefix persona name
                UserDataList& resultList = mResponse.getUserDataList();
                if (!mRequest.getPrimaryNamespaceOnly() && blaze_strcmp(mRequest.getPersonaNamespace(), NAMESPACE_ORIGIN) == 0)
                {
                    UserInfoListByPersonaNameMap resultMap;
                    err = gUserSessionManager->lookupUserInfoByOriginPersonaNamePrefix(mRequest.getPrefixName(), *platformSet, mRequest.getMaxResultCount(), resultMap);
                    if (err == ERR_OK)
                    {
                        for (const auto& personaItr : resultMap)
                        {
                            for (const auto& userItr : personaItr.second)
                            {
                                // fill out the user data
                                err = gUserSessionManager->filloutUserData(*userItr, *resultList.pull_back(),
                                    (userItr->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), gCurrentUserSession->getClientPlatform());
                                if (err != Blaze::ERR_OK)
                                    break;
                            }
                        }
                    }
                }
                else
                {
                    for (const auto& platform : *platformSet)
                    {
                        PersonaNameToUserInfoMap resultMap;
                        BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByPersonaNamePrefix(mRequest.getPrefixName(), platform, mRequest.getMaxResultCount(), resultMap, mRequest.getPersonaNamespace());
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
                }

                rc = commandErrorFromBlazeError(err);
            }
            else if (strlen(mRequest.getPrefixName()) < MINIMUM_PREFIX_CHARACTERS)
            {
                BLAZE_ERR_LOG(
                    Log::USER, 
                    "UserManager::processLookupUsersByPrefix: Incorrect persona name input, "
                    "The size of persona must be at least " << (static_cast<int> (MINIMUM_PREFIX_CHARACTERS)));
                return LookupUsersByPrefixCommandStub::USER_ERR_MINIMUM_CHARACTERS;
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUsersByPrefix: Unsupported type, PrefixName|MaxResultCount required.");
                return LookupUsersByPrefixCommandStub::ERR_SYSTEM;
            }
            
            if( (err == Blaze::ERR_OK) && (mResponse.getUserDataList().size() == 0) )
            {
                return LookupUsersByPrefixCommandStub::USER_ERR_USER_NOT_FOUND;
            }

            return rc;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERSBYPREFIX_CREATE_COMPNAME(UserSessionManager)

} // Blaze
