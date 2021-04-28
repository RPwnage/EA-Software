/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps5matchesforuser_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

class GetPs5MatchesForUserCommand : public GetPs5MatchesForUserCommandStub
{
public:
    GetPs5MatchesForUserCommand(Message* message, GetPs5MatchesForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs5MatchesForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs5MatchesForUserCommand() override {}

private:

    GetPs5MatchesForUserCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();
        if (mRequest.getUseCurrentUserForExternalSessionCall() && (gCurrentUserSession == nullptr))
        {
            ERR_LOG("[GetPs5MatchesForUserCommand].execute: (Arson) internal test error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
        if (mRequest.getUseCurrentUserForExternalSessionCall())
        {
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), mRequest.getExternalIdent());
        }
        if (!Blaze::GameManager::hasFirstPartyId(ident))
        {
            ERR_LOG("[GetPs5MatchesForUserCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
        TRACE_LOG("[GetPs5MatchesForUserCommand].execute: (Arson) retrieving current Matches for the user(" << GameManager::toExtUserLogStr(ident) << ").");

        BlazeRpcError err = mComponent->retrieveCurrentPs5Matches(ident, mResponse.getMatches(), mErrorResponse, true);
        if (err != ERR_OK)
        {
            return arsonCommandErrorFromBlazeError(err);
        }
        for (auto itr : mResponse.getMatches())
        {
            auto matchId = itr->getSessionIdentification().getPs5().getMatch().getMatchId();
            auto isInMatch = ArsonSlaveImpl::isBlazeUserInPsnMatch(ident, itr->getPsnMatch());
            if (ArsonSlaveImpl::IS_IN_PSNMATCH_YES_WITH_JOINFLAG_TRUE == isInMatch)
            {
                mResponse.getMatchesWithJoinFlagTrue().push_back(matchId);
            }
            else
            {
                mResponse.getMatchesWithJoinFlagFalse().push_back(matchId);
                ASSERT_COND_LOG((ArsonSlaveImpl::IS_IN_PSNMATCH_YES_WITH_JOINFLAG_FALSE == isInMatch), "[GetPs5MatchesForUserCommand].execute: (Arson) internal test issue, got a match(" << matchId << ") but helper couldn't find user in rsp Match, for the user(" << GameManager::toExtUserLogStr(ident) << ") for game(" << itr->getGameId() << ") specified.");
            }
        }
        TRACE_LOG("[GetPs5MatchesForUserCommand].execute: (Arson) found current Matches: with joinFlag true(" << mResponse.getMatchesWithJoinFlagTrue().size() <<
            "), with joinFlag false(" << mResponse.getMatchesWithJoinFlagFalse().size() << "), for the user(" << GameManager::toExtUserLogStr(ident) << ") specified.");
        return arsonCommandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs5MatchesForUserCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
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

DEFINE_GETPS5MATCHESFORUSER_CREATE()

} // Arson
} // Blaze
