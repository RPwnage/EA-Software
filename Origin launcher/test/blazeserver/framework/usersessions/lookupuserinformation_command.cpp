/*************************************************************************************************/
/*!
    \file   lookupuserinformation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUserInformationCommand

    resolve externalId lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupuser_stub.h"
namespace Blaze
{
    class LookupUserCommand : public LookupUserCommandStub
    {
    public:

        LookupUserCommand(Message* message, Blaze::UserIdentification* request, UserSessionsSlave* componentImpl)
            : LookupUserCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUserCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUserCommandStub::Errors execute() override
        {
            ClientPlatformType requestedPlatform = mRequest.getPlatformInfo().getClientPlatform();
            // Backwards compatibility: set PlatformInfo from deprecated UserIdentification fields
            // (note that this will not overwrite the PlatformInfo with unset/invalid entries)
            convertToPlatformInfo(mRequest.getPlatformInfo(), mRequest.getExternalId(), nullptr, mRequest.getAccountId(), mRequest.getOriginPersonaId(), nullptr, gCurrentUserSession->getClientPlatform());

            BlazeRpcError err;
            UserInfoPtr userInfo = nullptr;
            if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
            {
                eastl::string callerNamespace;
                bool crossPlatformOptIn;
                const ClientPlatformSet* platformSet = nullptr;
                err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
                if (err != ERR_OK)
                    return commandErrorFromBlazeError(err);

                //lookup user by blaze id
                err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
                // Make sure the caller is permitted to look up users on this platform
                ClientPlatformType callerPlatform = gCurrentUserSession->getClientPlatform();
                if (err == ERR_OK && callerPlatform != common)
                {
                    if (userInfo->getPlatformInfo().getClientPlatform() != callerPlatform)
                    {
                        const ClientPlatformSet& unrestrictedPlatforms = gUserSessionManager->getUnrestrictedPlatforms(callerPlatform);
                        if (unrestrictedPlatforms.find(userInfo->getPlatformInfo().getClientPlatform()) == unrestrictedPlatforms.end())
                            return LookupUserCommandStub::USER_ERR_DISALLOWED_PLATFORM;
                        else if (!crossPlatformOptIn)
                            return LookupUserCommandStub::USER_ERR_CROSS_PLATFORM_OPTOUT;
                    }
                }
            }
            else if (gCurrentUserSession->getClientPlatform() == common)
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUser: For callers on platform 'common', lookupUser is only supported for lookups by BlazeId. To look up users by NucleusAccountId, "
                    "OriginPersonaId, Origin persona name, or 1st party id, use lookupUsersCrossPlatform. To look up users by persona name, use lookupUsersByPersonaNames.");
                err = ERR_SYSTEM;
            }
            else if(blaze_strcmp(mRequest.getName(), "") != 0)
            {
                eastl::string targetNamespace(mRequest.getPersonaNamespace());
                bool crossPlatformOptIn;
                const ClientPlatformSet* platformSet = nullptr;
                err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, targetNamespace, crossPlatformOptIn, true /*matchPrimaryNamespace*/);
                if (err != ERR_OK)
                    return commandErrorFromBlazeError(err);

                //look up user by persona name
                err = gUserSessionManager->lookupUserInfoByPersonaName(mRequest.getName(), mRequest.getPlatformInfo().getClientPlatform(), userInfo, targetNamespace.c_str());
            }
            else if (requestedPlatform != INVALID && requestedPlatform != NATIVE && requestedPlatform != mRequest.getPlatformInfo().getClientPlatform())
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUser: Searching by ExternalId or OriginPersonaId, but requested platform (" << ClientPlatformTypeToString(requestedPlatform) << ") does not match caller's platform (" << ClientPlatformTypeToString(mRequest.getPlatformInfo().getClientPlatform()) <<
                    "). To look up a user by PlatformInfo or OriginPersonaId on a multi-platform server, use the lookupUsersCrossPlatform RPC instead.");
                err = ERR_SYSTEM;
            }
            else if (has1stPartyPlatformInfo(mRequest.getPlatformInfo()))
            {
                //look up user by external id / platform info
                err = gUserSessionManager->lookupUserInfoByPlatformInfo(mRequest.getPlatformInfo(), userInfo);
            }
            else if (mRequest.getPlatformInfo().getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID)
            {
                //look up user by origin persona id
                if (gController->isPlatformHosted(mRequest.getPlatformInfo().getClientPlatform()))
                {
                    err = gUserSessionManager->lookupUserInfoByOriginPersonaId(mRequest.getPlatformInfo().getEaIds().getOriginPersonaId(), mRequest.getPlatformInfo().getClientPlatform(), userInfo);
                }
                else
                {
                    BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUser: Cannot look up user by OriginPersonaId(" << mRequest.getPlatformInfo().getEaIds().getOriginPersonaId() << "): user's platform (" << ClientPlatformTypeToString(mRequest.getPlatformInfo().getClientPlatform()) << ") is not hosted.");
                    return LookupUserCommandStub::ERR_SYSTEM;
                }
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::processLookupUser: Unsupported lookup type, BlazeId|PersonaName|ExternalId|OriginPersonaId required.");
                return LookupUserCommandStub::ERR_SYSTEM;
            }
            if( (err == Blaze::ERR_OK) && (userInfo == nullptr) )
            {
                return LookupUserCommandStub::USER_ERR_USER_NOT_FOUND;
            }
            else if (err != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(err);
            }
            // fill out the user data
            return commandErrorFromBlazeError(gUserSessionManager->filloutUserData(*userInfo, mResponse, 
                (userInfo->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), gCurrentUserSession->getClientPlatform()));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSER_CREATE_COMPNAME(UserSessionManager)

} // Blaze
