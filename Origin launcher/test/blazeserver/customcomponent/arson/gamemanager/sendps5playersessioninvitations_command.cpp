/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/sendps5playersessioninvitations_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionutilps5.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h" //for hasFirstPartyId, toExtUserLogStr in execute

namespace Blaze
{
namespace Arson
{
// Custom ARSON test method, to send PSN PlayerSession invites for the game.
// (side: we have this in ARSON, because BlazeSDK doesn't expose any send invites API).
class SendPs5PlayerSessionInvitationsCommand : public SendPs5PlayerSessionInvitationsCommandStub
{
public:
    SendPs5PlayerSessionInvitationsCommand(Message* message, SendPs5PlayerSessionInvitationsRequest* request, ArsonSlaveImpl* componentImpl)
        : SendPs5PlayerSessionInvitationsCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~SendPs5PlayerSessionInvitationsCommand() override {}

private:

    SendPs5PlayerSessionInvitationsCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getSender();
        eastl::string identLogBuf;
        eastl::string callerLogBuf;
        if (!GameManager::hasFirstPartyId(ident) || mRequest.getGameId() == GameManager::INVALID_GAME_ID)
        {
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << "), or no valid game id(" << mRequest.getGameId() << ") specified.");
            return commandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
        // get Blaze's tracked external session info from GM, and auth token from Blaze/Nucleus
        Blaze::ExternalUserAuthInfo authInfo;
        Blaze::GameManager::GetExternalSessionInfoMasterResponse blazeGameInfo;
        BlazeRpcError parErr = mComponent->getPsnExternalSessionParams(authInfo, blazeGameInfo, mRequest.getGameId(), ident, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);
        if (ERR_OK != parErr)
        {
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) internal test error, failed to load external session url parameters for the user. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return commandErrorFromBlazeError(parErr);
        }
        auto& blazeGameExtSessionId = blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification();
        const char8_t* sessionId = blazeGameExtSessionId.getPs5().getPlayerSession().getPlayerSessionId();
        if (sessionId[0] == '\0')
        {
            // Blaze game says it has no PSN PlayerSession.
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) poss test error, Game(" << mRequest.getGameId() << ") has no PlayerSessionId. Noop. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return commandErrorFromBlazeError(ERR_OK);
        }
        // boiler plat check: confirm there is indeed PS users in the game, including the sender
        auto* forType = mComponent->getExternalSessionActivityTypeInfoPs5PlayerSession(ident.getPlatformInfo().getClientPlatform());
        auto blazeGameTrackedExtSessMembers = (forType ? GameManager::getListFromMap(blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), *forType) : nullptr);
        if (!blazeGameTrackedExtSessMembers || blazeGameTrackedExtSessMembers->empty())
        {
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) internal test error, failed to get a non-empty tracking list for the user, even though Blaze master game(" << mRequest.getGameId() << ") has PlayerSession(" << sessionId << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return commandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
        }
        if (nullptr == GameManager::getUserInExternalMemberInfoList(ident.getBlazeId(), *blazeGameTrackedExtSessMembers))
        {
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) poss test or Blaze syncing error, inviter(" << GameManager::toExtUserLogStr(ident) << ") is not in the tracking list for the Blaze master game(" << mRequest.getGameId() << "), PlayerSession(" << sessionId << ").");
            return commandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        // Make the http request
        SendPlayerSessionInvitationsResponse psnRsp;
        PSNServices::PsnErrorResponse psnErr;
        SendPlayerSessionInvitationsRequest psnReq;
        psnReq.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        psnReq.setSessionId(sessionId);
        for (auto itr : mRequest.getInvitees())
        {
            eastl::string accountIdBuf;
            psnReq.getBody().getInvitations().pull_back()->getTo().setAccountId(GameManager::ExternalSessionUtilPs5::getPsnAccountId(accountIdBuf, *itr).c_str());
        }
        TRACE_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) sending invites to game(" << mRequest.getGameId() << "), PlayerSession(" << sessionId << "), from inviter(" << GameManager::toExtUserLogStr(ident) << "), to: " << psnReq.getBody().getInvitations());

        auto err = mComponent->sendPlayerSessionInvitations(psnReq, psnRsp, psnErr);
        if (err != ERR_OK)
        {
            mErrorResponse.setCode(psnErr.getError().getCode());
            mErrorResponse.setMessage(psnErr.getError().getMessage());
            ERR_LOG("[SendPs5PlayerSessionInvitationsCommand].execute: (Arson) error(" << GameManager::ExternalSessionUtilPs5::toLogStr(err, &psnErr)  << "), sending invites to game(" << mRequest.getGameId() << "), PlayerSession(" << sessionId << "), from inviter(" << GameManager::toExtUserLogStr(ident) << "), to: " << psnReq.getBody().getInvitations());
        }
        mResponse.setPlayerSessionIdSentFor(sessionId);
        for (auto it : psnRsp.getInvitations())
            mResponse.getInvitationIds().push_back(it->getInvitationId());
        return commandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_SENDPS5PLAYERSESSIONINVITATIONS_CREATE()

} // Arson
} // Blaze
