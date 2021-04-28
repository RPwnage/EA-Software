//*************************************************************************************************/
//*!
//    \file   fetchuserfirstlastauthtime_command.cpp
//
//
//    \attention
//        (c) Electronic Arts. All Rights Reserved.
//*/
//*************************************************************************************************/
//
//*************************************************************************************************/
//*!
//    \class FetchUserFirstLastAuthTimeCommand
//
//    Fetch the user's first and last auth time.
//
//    \note
//
//*/
//*************************************************************************************************/
//
//*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/fetchuserfirstlastauthtime_stub.h"

namespace Blaze
{

    class FetchUserFirstLastAuthTimeCommand : public FetchUserFirstLastAuthTimeCommandStub
    {
    public:

       FetchUserFirstLastAuthTimeCommand(Message* message, Blaze::FetchUserFirstLastAuthTimeRequest* request, UserSessionsSlave* componentImpl)
            :FetchUserFirstLastAuthTimeCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~FetchUserFirstLastAuthTimeCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       FetchUserFirstLastAuthTimeCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[FetchUserFirstLastAuthTimeCommandStub].start()");

            BlazeRpcError err;
            UserInfoPtr userInfo;
            if(mRequest.getBlazeId() != 0)
            {
                //lookup user by blaze id
                err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            }
            else
            {
                BLAZE_ERR_LOG(Log::USER, "UserManager::FetchUserFirstLastAuthTimeCommand: Unsupported lookup type, the User BlazeId required.");
                return FetchUserFirstLastAuthTimeCommandStub::ERR_SYSTEM;
            }
            if( (err != Blaze::ERR_OK) || (userInfo == nullptr) )
            {
                return FetchUserFirstLastAuthTimeCommandStub::USER_ERR_USER_NOT_FOUND;
            }
            // fill out the user first and last login time
            mResponse.setUserFirstAuthTime(userInfo->getFirstLoginDateTime());
            mResponse.setUserLastAuthTime(userInfo->getLastLoginDateTime());
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_FETCHUSERFIRSTLASTAUTHTIME_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze
