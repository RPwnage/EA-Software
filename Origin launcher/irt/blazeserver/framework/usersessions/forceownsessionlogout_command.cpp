/*************************************************************************************************/
/*!
    \file   forceownsessionlogout_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ForceOwnSessionLogoutCommand

    Logs out the user's other unresponsive session
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/forceownsessionlogout_stub.h"

namespace Blaze
{
    class ForceOwnSessionLogoutCommand : public ForceOwnSessionLogoutCommandStub
    {
    public:

       ForceOwnSessionLogoutCommand(Message* message, Blaze::ForceOwnSessionLogoutRequest* request, UserSessionsSlave* componentImpl)
            : ForceOwnSessionLogoutCommandStub(message, request),
              mComponent(componentImpl)
        {
        }
        ~ForceOwnSessionLogoutCommand() override
        {
        }

    private:

       ForceOwnSessionLogoutCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute()");

            // check if user has the permission to kick itself
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_FORCE_OWN_SESSION_LOGOUT))
            {
                BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: User " << gCurrentUserSession->getBlazeId() << " does not have permission! " <<
                    "No session has been logged out " << requestedUserToLogStr() << ".");
                return ERR_AUTHORIZATION_REQUIRED;
            }

            // for extra security disallow kicking another's sessions (no current use cases need it)
            if (gCurrentUserSession->getBlazeId() != mRequest.getBlazeId())
            {
                BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: callers actual blaze id mismatches that in request. " <<
                    "No session has been logged out " << requestedUserToLogStr() << ".");
                return commandErrorFromBlazeError(Blaze::USER_ERR_INVALID_PARAM);
            }

            // validate request sessionKey's session id
            UserSessionId targetUserSessionId = gUserSessionManager->getUserSessionIdFromSessionKey(mRequest.getSessionKey());
            if (targetUserSessionId == INVALID_USER_SESSION_ID)
            {
                BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: target session key's user session " <<
                    " does not exist. No session has been logged out " << requestedUserToLogStr() << ".");
                return commandErrorFromBlazeError(Blaze::USER_ERR_SESSION_NOT_FOUND);
            }

            if (!gUserSessionManager->getSessionExists(targetUserSessionId))
            {
                BLAZE_TRACE_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: usersession " << targetUserSessionId <<
                    " does not exist. No session has been logged out " << requestedUserToLogStr() << ".");
                return commandErrorFromBlazeError(Blaze::USER_ERR_SESSION_NOT_FOUND);
            }

            BlazeRpcError err;

            // check if user session is local. Its unresponsive-state etc checks must be done on the owning slave.
            if (!UserSession::isOwnedByThisInstance(targetUserSessionId))
            {
                BLAZE_TRACE_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: usersession " << targetUserSessionId <<
                    " exists but is not local. Completing request on the remote owning slave.");
                RpcCallOptions opts;
                opts.routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, targetUserSessionId);
                err = mComponent->sendRequest(UserSessionsSlave::CMD_INFO_FORCEOWNSESSIONLOGOUT, &mRequest, nullptr, nullptr, opts);
            }
            else
            {
                // validate request sessionKey, blaze id
                UserSessionMasterPtr matchedSession = gUserSessionMaster->getLocalSessionBySessionKey(mRequest.getSessionKey());
                if (matchedSession == nullptr)
                {
                    BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: request key validation failed. " <<
                        "No session has been logged out " << requestedUserToLogStr() << ".");
                    return commandErrorFromBlazeError(Blaze::USER_ERR_INVALID_PARAM);
                }
                if (matchedSession->getBlazeId() != mRequest.getBlazeId())
                {
                    BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: actual blaze id(" << matchedSession->getBlazeId() << ") mismatches that in request. " <<
                        "No session has been logged out " << requestedUserToLogStr() << ".");
                    return commandErrorFromBlazeError(Blaze::USER_ERR_INVALID_PARAM);
                }

                // validate unresponsive
                if (!gUserSessionMaster->isLocalUserSessionUnresponsive(targetUserSessionId))
                {
                    BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: sessions state is not unresponsive. " <<
                        "No session has been logged out " << requestedUserToLogStr() << ".");
                    return commandErrorFromBlazeError(Blaze::USER_ERR_INVALID_PARAM);
                }

                err = gUserSessionMaster->destroyUserSession(targetUserSessionId, DISCONNECTTYPE_FORCED_LOGOUT, 0, FORCED_LOGOUTTYPE_RELOGIN_REQUIRED);
                if (err != ERR_OK)
                {
                    BLAZE_WARN_LOG(Log::USER, "[ForceOwnSessionLogoutCommand].execute: failed to logout UserSessionId(" << targetUserSessionId << ") with err(" << ErrorHelp::getErrorName(err) << ")");
                }
            }

            return commandErrorFromBlazeError(err);
        }

    private:

        const char8_t* requestedUserToLogStr() const
        {
            if (mRequestedUserLogStr.empty())
            {
                mRequestedUserLogStr.append_sprintf("(requested: BlazeId=%" PRIi64 ", UserSessionId=%" PRIu64 ", SessionKey=%s)", mRequest.getBlazeId(), gUserSessionManager->getUserSessionIdFromSessionKey(mRequest.getSessionKey()), mRequest.getSessionKey());
            }
            return mRequestedUserLogStr.c_str();
        }
        mutable eastl::string mRequestedUserLogStr;

        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    DEFINE_FORCEOWNSESSIONLOGOUT_CREATE_COMPNAME(UserSessionManager)

} // Blaze
