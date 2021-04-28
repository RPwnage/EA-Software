/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps4primarynpsessionforuser_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class GetPs4PrimaryNpSessionForUserCommand : public GetPs4PrimaryNpSessionForUserCommandStub
{
public:
    GetPs4PrimaryNpSessionForUserCommand(Message* message, GetPs4PrimaryNpSessionForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs4PrimaryNpSessionForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs4PrimaryNpSessionForUserCommand() override {}

private:

    GetPs4PrimaryNpSessionForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();

        // validate user and game id args specified
        if (mRequest.getUseCurrentUserForExternalSessionCall() && (gCurrentUserSession == nullptr))
        {
            ERR_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute: (Arson) internal test error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
        if (mRequest.getUseCurrentUserForExternalSessionCall())
        {
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), mRequest.getExternalIdent());
        }

        Blaze::GameManager::ExternalSessionUtilPs4* extSessUtil = mComponent->getExternalSessionServicePs4();
        if (extSessUtil == nullptr)
        {
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_FAILED);//logged
        }

        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        // get auth token
        Blaze::ExternalUserAuthInfo authInfo;
        eastl::string authToken;
        Blaze::BlazeRpcError error = extSessUtil->getAuthToken(ident, gController->getDefaultServiceName(), authToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute (Arson): Could not retrieve psn auth token. error " << ErrorHelp::getErrorName(error));
            return arsonCommandErrorFromBlazeError(error);
        }
        authInfo.setCachedExternalSessionToken(authToken.c_str());
        authInfo.setServiceName(gController->getDefaultServiceName());
        TRACE_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute: (Arson) retrieving current primary session for the user(" << GameManager::toExtUserLogStr(ident) << ").");

        Blaze::ExternalSessionIdentification currentPrimary;
        BlazeRpcError getCurrErr = mComponent->retrieveCurrentPrimarySessionPs4(ident, authInfo, currentPrimary, mErrorResponse);
        mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());// This rpc is high use in test, record rate limit info, regardless of pass/fail, to help scripts determine next console to use
        if (getCurrErr != ERR_OK)
        {
            return arsonCommandErrorFromBlazeError(getCurrErr);
        }

        mResponse.setSessionId(currentPrimary.getPs4().getNpSessionId());
        if (mResponse.getSessionId()[0] == '\0')
        {
            TRACE_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute: (Arson) found NO current primary session for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return ERR_OK;
        }

        // retrieve the external session from PSN
        BlazeRpcError sesErr = ERR_OK;
        if (mRequest.getFetchExternalSessionSessionProperties())
        {
            sesErr = mComponent->retrieveExternalSessionPs4(ident, authInfo, currentPrimary, "en", mResponse.getNpSession(), mErrorResponse);
            // get the lowest/worst case rate limit remaining
            if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
                mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        // retrieve its session-data as needed also
        if ((sesErr == ERR_OK) && mRequest.getFetchExternalSessionSessionData())
        {
            sesErr = mComponent->retrieveExternalSessionSessionData(ident, authInfo, currentPrimary, mResponse.getSessionData(), mErrorResponse);
            if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
                mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        if ((sesErr == ERR_OK) && mRequest.getFetchExternalSessionChangeableSessionData())
        {
            sesErr = mComponent->retrieveExternalSessionChangeableSessionData(ident, authInfo, currentPrimary, mResponse.getChangeableSessionData(), mErrorResponse);
            if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
                mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        return arsonCommandErrorFromBlazeError(sesErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs4PrimaryNpSessionForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
    {
        blazeError = Blaze::GameManager::convertExternalServiceErrorToGameManagerError(blazeError);
        switch (blazeError)
        {
        case GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
            if (mRequest.getFailOnRateLimitError())
            {
                return commandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SERVICE_BUSY);
            }
            else
            {
                WARN_LOG("[GetPs4PrimaryNpSessionForUserCommand].execute (Arson): User was rate limited, error: " << ErrorHelp::getErrorName(blazeError) << ". Ignoring, to allow caller to back off and retry. Caller (" << GameManager::toExtUserLogStr(mRequest.getExternalIdent()) << ")");
                return commandErrorFromBlazeError(ERR_OK);
            }
        default:
            return commandErrorFromBlazeError(blazeError);
        };
    }
};

DEFINE_GETPS4PRIMARYNPSESSIONFORUSER_CREATE()

} // Arson
} // Blaze
