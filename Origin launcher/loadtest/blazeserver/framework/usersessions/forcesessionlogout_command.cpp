/*************************************************************************************************/
/*!
    \file   forcesessionlogout_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ForceSessionLogoutCommand

    Lookup the user session id by the user's blaze id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/forcesessionlogout_stub.h"

namespace Blaze
{

    class ForceSessionLogoutCommand : public ForceSessionLogoutCommandStub
    {
    public:

       ForceSessionLogoutCommand(Message* message, Blaze::ForceSessionLogoutRequest* request, UserSessionsSlave* componentImpl) :
           ForceSessionLogoutCommandStub(message, request)
        {
        }

        ~ForceSessionLogoutCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ForceSessionLogoutCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ForceSessionLogoutCommand].execute()");

            // check if the current user has the permission to kick another users session
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_FORCE_SESSION_LOGOUT))
            {
                BLAZE_WARN_LOG(Log::USER, "[ForceSessionLogoutCommand].execute: User " << UserSession::getCurrentUserBlazeId()
                 << " attempted to logout another users session without permission!" );

                return ERR_AUTHORIZATION_REQUIRED;
            }

            BlazeRpcError err = Blaze::ERR_SYSTEM;
            if ((mComponentStub != nullptr) && (mComponentStub->getMaster() != nullptr))
            {
                err = mComponentStub->getMaster()->forceSessionLogoutMaster(mRequest);
                if (err != ERR_OK)
                {
                    BLAZE_INFO_LOG(Log::USER, "[ForceSessionLogoutCommand].execute: UserSessionId [" << mRequest.getSessionId() << "] "
                        "does not exist. No session has been logged out.");

                    err = Blaze::USER_ERR_SESSION_NOT_FOUND;
                }
            }

            return commandErrorFromBlazeError(err);
        }
    };

    
    DEFINE_FORCESESSIONLOGOUT_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze
