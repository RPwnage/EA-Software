/*************************************************************************************************/
/*!
    \file   fetchlastlocaesettingandautherror_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchLastLocaleUsedAndAuthErrorCommand 

    process user last locale setting used and last auth error retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/fetchlastlocaleusedandautherror_stub.h"
#include "framework/util/locales.h"

namespace Blaze
{

    class FetchLastLocaleUsedAndAuthErrorCommand : public FetchLastLocaleUsedAndAuthErrorCommandStub
    {
    public:

        FetchLastLocaleUsedAndAuthErrorCommand(Message* message, Blaze::FetchLastLocaleUsedAndAuthErrorRequest* request, UserSessionsSlave* componentImpl)
            : FetchLastLocaleUsedAndAuthErrorCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~FetchLastLocaleUsedAndAuthErrorCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchLastLocaleUsedAndAuthErrorCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[FetchLastLocaleUsedAndAuthErrorCommand].start()");

            Blaze::BlazeRpcError err = Blaze::ERR_OK;

            if (EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }
            
            UserInfoPtr userInfo;
            err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo, true);
            if (err == Blaze::ERR_OK)
            {
                Locale locale = userInfo->getLastLocaleUsed();

                if (locale != LOCALE_BLANK)
                {
                    char8_t buf[32];
                    LocaleTokenCreateLocalityString(buf, locale);
                    mResponse.setLastLocaleUsed(buf);
                }

                mResponse.setLastAuthError(ErrorHelp::getErrorName(static_cast<BlazeRpcError>(userInfo->getLastAuthErrorCode())));
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_FETCHLASTLOCALEUSEDANDAUTHERROR_CREATE_COMPNAME(UserSessionManager)

} // Blaze
