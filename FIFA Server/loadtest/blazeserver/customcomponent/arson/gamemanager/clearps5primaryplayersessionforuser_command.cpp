/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/clearps5primaryplayersessionforuser_stub.h"
#include "arsonslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5.h" // for ExternalSessionUtilPs5 in execute
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class ClearPs5PrimaryPlayerSessionForUserCommand : public ClearPs5PrimaryPlayerSessionForUserCommandStub
{
public:
    ClearPs5PrimaryPlayerSessionForUserCommand(Message* message, ClearPs5PrimaryPlayerSessionForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearPs5PrimaryPlayerSessionForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~ClearPs5PrimaryPlayerSessionForUserCommand() override {}

private:

    ClearPs5PrimaryPlayerSessionForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();

        auto* extSessUtil = mComponent->getExternalSessionServicePs5(ident.getPlatformInfo().getClientPlatform());
        if (extSessUtil == nullptr)
        {
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_FAILED);//logged
        }
        
        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[ClearPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        ClearPrimaryExternalSessionParameters params;
        ident.copyInto(params.getUserIdentification());

        // get auth token
        eastl::string authToken;
        Blaze::BlazeRpcError error = extSessUtil->getAuthToken(ident, gController->getDefaultServiceName(), authToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[ClearPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) Could not retrieve psn auth token. error(" << ErrorHelp::getErrorName(error) << ")");
            return arsonCommandErrorFromBlazeError(error);
        }
        params.getAuthInfo().setCachedExternalSessionToken(authToken.c_str());
        params.getAuthInfo().setServiceName(gController->getDefaultServiceName());

        BlazeRpcError checkCurrErr = ERR_OK;
        if (mRequest.getExpectWasAlreadyCleared())
        {
            // Boiler plate check: user doesn't have a primary (according to PSN).
            Blaze::ExternalSessionIdentification currentPrimary;
            checkCurrErr = mComponent->retrieveCurrentPrimarySessionPs5(ident, params.getAuthInfo(), currentPrimary, &mResponse.getPsnPlayerSession(),
                mResponse.getDecodedCustomData1(), mResponse.getDecodedCustomData2(), mErrorResponse);
            //mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());//Rate limits API currently n/a from PSN
            if (checkCurrErr == ERR_OK)
            {
                mResponse.setSessionId(currentPrimary.getPs5().getPlayerSession().getPlayerSessionId());
                if (mResponse.getSessionId()[0] != '\0')
                {
                    WARN_LOG("[ClearPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) unexpectedly found a pre-existing primary session(" << mResponse.getSessionId() << ") for user(" << GameManager::toExtUserLogStr(ident) << "). Did a prior test did not cleanup properly?");
                    checkCurrErr = ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
                    // check failed but don't early out yet, complete cleanup of the session below first.
                }
                else
                {
                    // nothing to clear!
                    return arsonCommandErrorFromBlazeError(ERR_OK);
                }
            }
        }

        TRACE_LOG("[ClearPs5PrimaryPlayerSessionForUserCommand].execute: (Arson)clearing any current primary session found for the user(" << GameManager::toExtUserLogStr(ident) << ").");

        BlazeRpcError clearErr = extSessUtil->clearPrimary(params, &mErrorResponse);
        //Rate limits API currently n/a from PSN
        //// For ARSON, tests may need rate limit info to properly back off. Get the lowest/worst case rate limit remaining. (Rate limit info may be set by clearPrimary() regardless of whether there was error).
        //if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
        //{
        //    mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        //}
        return arsonCommandErrorFromBlazeError((checkCurrErr != ERR_OK)? checkCurrErr : clearErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    ClearPs5PrimaryPlayerSessionForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
    {
        blazeError = Blaze::GameManager::convertExternalServiceErrorToGameManagerError(blazeError);
        //switch (blazeError)//Rate limits API currently n/a from PSN
        //{
        //case GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
        //    if (mRequest.getFailOnRateLimitError())
        //    {
        //        return commandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SERVICE_BUSY);
        //    }
        //    else
        //    {
        //        WARN_LOG("[ClearPs5PrimaryPlayerSessionForUserCommand].execute (Arson): User was rate limited, error(" << ErrorHelp::getErrorName(blazeError) << "). Ignoring, to allow caller to back off and retry. Caller (" << GameManager::toExtUserLogStr(mRequest.getExternalIdent()) << ")");
        //        return commandErrorFromBlazeError(ERR_OK);
        //    }
        //default:
        //};
        return commandErrorFromBlazeError(blazeError);
    }
};

DEFINE_CLEARPS5PRIMARYPLAYERSESSIONFORUSER_CREATE()

} // Arson
} // Blaze
