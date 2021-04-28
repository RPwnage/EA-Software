/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps5primaryplayersessionforuser_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "component/gamemanager/externalsessions/externalsessionutilps5.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class GetPs5PrimaryPlayerSessionForUserCommand : public GetPs5PrimaryPlayerSessionForUserCommandStub
{
public:
    GetPs5PrimaryPlayerSessionForUserCommand(Message* message, GetPs5PrimaryPlayerSessionForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs5PrimaryPlayerSessionForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs5PrimaryPlayerSessionForUserCommand() override {}

private:

    GetPs5PrimaryPlayerSessionForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();
        
        // validate user and game id args specified
        if (mRequest.getUseCurrentUserForExternalSessionCall() && (gCurrentUserSession == nullptr))
        {
            ERR_LOG("[GetPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) internal test error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
        if (mRequest.getUseCurrentUserForExternalSessionCall())
        {
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), mRequest.getExternalIdent());
        }

        auto* extSessUtil = mComponent->getExternalSessionServicePs5(ident.getPlatformInfo().getClientPlatform());
        if (extSessUtil == nullptr)
        {
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_FAILED);//logged
        }

        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[GetPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        // get auth token
        Blaze::ExternalUserAuthInfo authInfo;
        eastl::string authToken;
        Blaze::BlazeRpcError err = extSessUtil->getAuthToken(ident, gController->getDefaultServiceName(), authToken);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[GetPs5PrimaryPlayerSessionForUserCommand].execute (Arson): Could not retrieve psn auth token. error " << ErrorHelp::getErrorName(err));
            return arsonCommandErrorFromBlazeError(err);
        }
        authInfo.setCachedExternalSessionToken(authToken.c_str());
        authInfo.setServiceName(gController->getDefaultServiceName());
        TRACE_LOG("[GetPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) retrieving current primary session for the user(" << GameManager::toExtUserLogStr(ident) << ").");

        Blaze::ExternalSessionIdentification currentPrimary;
        err = mComponent->retrieveCurrentPrimarySessionPs5(ident, authInfo, currentPrimary, &mResponse.getPsnPlayerSession(), mResponse.getDecodedCustomData1(),
            mResponse.getDecodedCustomData2(), mErrorResponse);
        //mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());//Rate limits API currently n/a from PSN
        if (err != ERR_OK)
        {
            return arsonCommandErrorFromBlazeError(err);
        }

        mResponse.setSessionId(currentPrimary.getPs4().getNpSessionId());
        if (mResponse.getSessionId()[0] == '\0')
        {
            TRACE_LOG("[GetPs5PrimaryPlayerSessionForUserCommand].execute: (Arson) found NO current primary session for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
        }

        return arsonCommandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs5PrimaryPlayerSessionForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
    {
        auto gmErr = Blaze::GameManager::convertExternalServiceErrorToGameManagerError(blazeError);
        //switch (gmErr)//Rate limits API currently n/a from PSN
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
        auto reErr = ((gmErr != ERR_SYSTEM) ? gmErr : blazeError);
        auto cmdErr = commandErrorFromBlazeError(reErr);
        return ((cmdErr != ERR_SYSTEM) ? cmdErr : commandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC));
    }
};

DEFINE_GETPS5PRIMARYPLAYERSESSIONFORUSER_CREATE()

} // Arson
} // Blaze
