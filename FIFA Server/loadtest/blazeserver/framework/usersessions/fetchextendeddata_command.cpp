/*************************************************************************************************/
/*!
    \file   fetchextendeddata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchExtendedDataCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/fetchextendeddata_stub.h"

namespace Blaze
{

    class FetchExtendedDataCommand : public FetchExtendedDataCommandStub
    {
    public:

        FetchExtendedDataCommand(Message* message, Blaze::FetchExtendedDataRequest* request, UserSessionsSlave* componentImpl)
            : FetchExtendedDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~FetchExtendedDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchExtendedDataCommandStub::Errors execute() override
        {
            UserInfoPtr userInfo;
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            if (err != Blaze::ERR_OK)
            {
                return USER_ERR_USER_NOT_FOUND;
            }

            // Check if user is online, and if so grab the UED from the remote session
            UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(mRequest.getBlazeId());
            if (primarySessionId != INVALID_USER_SESSION_ID)
            {
                // user is online, but on a remote instance, fetch UED from it and refresh it.
                err = gUserSessionManager->getRemoteUserExtendedData(primarySessionId, mResponse);
                if (err == Blaze::ERR_OK)
                {
                    err = gUserSessionManager->refreshUserExtendedData(*userInfo, mResponse);
                    if (err != Blaze::ERR_OK)
                    {
                        BLAZE_FAIL_LOG(Log::USER, "[FetchExtendedDataCommand].execute: "
                            "Failed to refresh UED for remote UserSession(" << primarySessionId << ")");
                    }
                }
                else
                {
                    BLAZE_FAIL_LOG(Log::USER, "[FetchExtendedDataCommand].execute: "
                        "Failed to get UED for UserSession(" << primarySessionId << ") from remote instance.");
                }
                return commandErrorFromBlazeError(err);
            }

            // Users not online, or not local pull from DB &&
            // request extended data from listeners
            err = gUserSessionManager->requestUserExtendedData(*userInfo, mResponse);

            if (err != Blaze::ERR_OK)
            {
                BLAZE_FAIL_LOG(Log::USER, "[FetchExtendedDataCommand].execute: "
                    "Failed to request UED for UserSession(" << primarySessionId << ")");
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_FETCHEXTENDEDDATA_CREATE_COMPNAME(UserSessionManager)

} // Blaze
