/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/clearps5matchesforuser_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class ClearPs5MatchesForUserCommand : public ClearPs5MatchesForUserCommandStub
{
public:
    ClearPs5MatchesForUserCommand(Message* message, ClearPs5MatchesForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearPs5MatchesForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~ClearPs5MatchesForUserCommand() override {}

private:

    ClearPs5MatchesForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();

        auto* extSessUtil = mComponent->getExternalSessionServicePs5(ident.getPlatformInfo().getClientPlatform());
        if (extSessUtil == nullptr)
        {
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_FAILED);//logged
        }        
        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[ClearPs5MatchesForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        BlazeRpcError clearErr = mComponent->clearCurrentPs5Matches(ident, mResponse.getMatches(), mErrorResponse);

        if (mRequest.getExpectWasAlreadyCleared())
        {
            // Boiler plate check: user doesn't have a primary (according to PSN).
            if (clearErr == ERR_OK)
            {
                if (!mResponse.getMatches().empty())
                {
                    WARN_LOG("[ClearPs5MatchesForUserCommand].execute: (Arson) unexpectedly found (" << mResponse.getMatches().size() << ") matches, including(" << *mResponse.getMatches().front() << ") for user(" << GameManager::toExtUserLogStr(ident) << "). Did a prior test did not cleanup properly?");
                    return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
                }
            }
        }

        return arsonCommandErrorFromBlazeError(clearErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    ClearPs5MatchesForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
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
        //        WARN_LOG("[ClearPs5MatchesForUserCommand].execute (Arson): User was rate limited, error(" << ErrorHelp::getErrorName(blazeError) << "). Ignoring, to allow caller to back off and retry. Caller (" << GameManager::toExtUserLogStr(mRequest.getExternalIdent()) << ")");
        //        return commandErrorFromBlazeError(ERR_OK);
        //    }
        //default:  
        //};
        return commandErrorFromBlazeError(blazeError);
    }
};

DEFINE_CLEARPS5MATCHESFORUSER_CREATE()

} // Arson
} // Blaze
