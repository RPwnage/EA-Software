/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/clearps4primarynpsessionforuser_stub.h"
#include "arsonslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h" // for ExternalSessionUtilPs4 in execute
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class ClearPs4PrimaryNpSessionForUserCommand : public ClearPs4PrimaryNpSessionForUserCommandStub
{
public:
    ClearPs4PrimaryNpSessionForUserCommand(Message* message, ClearPs4PrimaryNpSessionForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearPs4PrimaryNpSessionForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~ClearPs4PrimaryNpSessionForUserCommand() override {}

private:

    ClearPs4PrimaryNpSessionForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();

        Blaze::GameManager::ExternalSessionUtilPs4* extSessUtil = mComponent->getExternalSessionServicePs4();
        if (extSessUtil == nullptr)
        {
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_FAILED);//logged
        }
        
        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[ClearPs4PrimaryNpSessionForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        ClearPrimaryExternalSessionParameters params;
        ident.copyInto(params.getUserIdentification());

        // get auth token
        eastl::string authToken;
        Blaze::BlazeRpcError error = extSessUtil->getAuthToken(ident, gController->getDefaultServiceName(), authToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[ClearPs4PrimaryNpSessionForUserCommand].execute: (Arson) Could not retrieve psn auth token. error(" << ErrorHelp::getErrorName(error) << ")");
            return arsonCommandErrorFromBlazeError(error);
        }
        params.getAuthInfo().setCachedExternalSessionToken(authToken.c_str());
        params.getAuthInfo().setServiceName(gController->getDefaultServiceName());

        BlazeRpcError checkCurrErr = ERR_OK;
        if (mRequest.getExpectWasAlreadyCleared())
        {
            // Boiler plate check: user doesn't have a primary (according to PSN).
            Blaze::ExternalSessionIdentification currentPrimary;
            checkCurrErr = mComponent->retrieveCurrentPrimarySessionPs4(ident, params.getAuthInfo(), currentPrimary, mErrorResponse);
            mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());// This rpc is high use in test, record rate limit info, regardless of pass/fail, to help scripts determine next console to use
            if (checkCurrErr == ERR_OK)
            {
                mResponse.setSessionId(currentPrimary.getPs4().getNpSessionId());
                if (mResponse.getSessionId()[0] != '\0')
                {
                    WARN_LOG("[ClearPs4PrimaryNpSessionForUserCommand].execute: (Arson) unexpectedly found a pre-existing primary session(" << mResponse.getSessionId() << ") for user(" << GameManager::toExtUserLogStr(ident) << "). Did a prior test did not cleanup properly?");
                    checkCurrErr = ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
                    // check failed but don't early out yet, complete cleanup of the NP session below first.
                }
                else
                {
                    // nothing to clear!
                    return arsonCommandErrorFromBlazeError(ERR_OK);
                }
            }
        }

        TRACE_LOG("[ClearPs4PrimaryNpSessionForUserCommand].execute: (Arson)clearing any current primary session found for the user(" << GameManager::toExtUserLogStr(ident) << ").");

        BlazeRpcError clearErr = extSessUtil->clearPrimary(params, &mErrorResponse);
        // For ARSON, tests may need rate limit info to properly back off. Get the lowest/worst case rate limit remaining. (Rate limit info may be set by clearPrimary() regardless of whether there was error).
        if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
        {
            mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        return arsonCommandErrorFromBlazeError((checkCurrErr != ERR_OK)? checkCurrErr : clearErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    ClearPs4PrimaryNpSessionForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
    {
        eastl::string identLogBuf;
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
                WARN_LOG("[ClearPs4PrimaryNpSessionForUserCommand].execute (Arson): User was rate limited, error(" << ErrorHelp::getErrorName(blazeError) << "). Ignoring, to allow caller to back off and retry. Caller (" << GameManager::toExtUserLogStr(mRequest.getExternalIdent()) << ")");
                return commandErrorFromBlazeError(ERR_OK);
            }
        default:
            return commandErrorFromBlazeError(blazeError);
        };
    }
};

DEFINE_CLEARPS4PRIMARYNPSESSIONFORUSER_CREATE()

} // Arson
} // Blaze
