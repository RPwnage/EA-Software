/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps5playersessioninvitations_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionutilps5.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h" //for toExtUserLogStr in execute

namespace Blaze
{
namespace Arson
{

/*! ************************************************************************************************/
/*! \brief Get PSN PlayerSession invites for the caller. This cmd calls PSN directly using Sony's WebApi (wraps an internal proxy cmd that calls PSN S2S).
***************************************************************************************************/
class GetPs5PlayerSessionInvitationsCommand : public GetPs5PlayerSessionInvitationsCommandStub
{
public:
    GetPs5PlayerSessionInvitationsCommand(Message* message, GetPs5PlayerSessionInvitationsRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs5PlayerSessionInvitationsCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs5PlayerSessionInvitationsCommand() override {}

private:

    GetPs5PlayerSessionInvitationsCommandStub::Errors execute() override
    {
        if (!gCurrentLocalUserSession)
        {
            ERR_LOG("[GetPs5PlayerSessionInvitationsCommand].execute: Internal test err: gCurrentLocalUserSession null.");
            return ERR_SYSTEM;
        }

        // get the auth token for S2S
        Blaze::ExternalUserAuthInfo authInfo;
        UserIdentification ident;
        UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), ident);
        Blaze::GameManager::GetExternalSessionInfoMasterResponse blazeGameInfo;
        BlazeRpcError parErr = mComponent->getPsnExternalSessionParams(authInfo, blazeGameInfo, Blaze::GameManager::INVALID_GAME_ID, ident, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);
        if (ERR_OK != parErr)
        {
            ERR_LOG("[GetPs5PlayerSessionInvitationsCommand].execute: (Arson) failed to get PSN S2S auth token for Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return commandErrorFromBlazeError(parErr);
        }
        
        // Make the http request to PSN
        PSNServices::PsnErrorResponse psnErr;
        GetPlayerSessionInvitationsResponse psnRsp;
        GetPlayerSessionInvitationsRequest psnReq;
        psnReq.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        psnReq.setAccountId(eastl::string(eastl::string::CtorSprintf(), "%" PRIi64 "", ident.getPlatformInfo().getExternalIds().getPsnAccountId()).c_str());

        TRACE_LOG("[GetPs5PlayerSessionInvitationsCommand].execute: (Arson) getting invites for user(" << psnReq.getAccountId() << "), calling PSN S2S.");

        BlazeRpcError err = mComponent->getPlayerSessionInvitations(psnReq, psnRsp, psnErr);
        if (err != ERR_OK)
        {
            mErrorResponse.setCode(psnErr.getError().getCode());
            mErrorResponse.setMessage(psnErr.getError().getMessage());
            ERR_LOG("[GetPs5PlayerSessionInvitationsCommand].execute: (Arson) error(" << GameManager::ExternalSessionUtilPs5::toLogStr(err, &psnErr)  << "), getting invites for user(" << psnReq.getAccountId() << "), calling PSN S2S.");
        }
        else
        {
            blazePs5PlayerSessionInvitesResponseToArsonResponse(mResponse, psnRsp);
        }
        return commandErrorFromBlazeError(err);
    }

private:
    ArsonSlaveImpl* mComponent;

    static void blazePs5PlayerSessionInvitesResponseToArsonResponse(Blaze::Arson::GetPs5PlayerSessionInvitationsResponse &arsonResponse, const GetPlayerSessionInvitationsResponse& psnRestReponse)
    {
        for (auto it : psnRestReponse.getInvitations())
        {
            auto rspItem = arsonResponse.getInvites().pull_back();
            rspItem->setInvitationId(it->getInvitationId());
            rspItem->setReceivedTimestamp(it->getReceivedTimestamp());
            rspItem->setInvitationInvalid(it->getInvitationInvalid());
            rspItem->setSessionId(it->getSessionId());
            ExternalPsnAccountId psnAcctId = EA::StdC::AtoI64(it->getFrom().getAccountId());
            rspItem->setFrom(psnAcctId);
            for (auto platIt : it->getSupportedPlatforms())
                rspItem->getSupportedPlatforms().push_back(platIt.c_str());
        }
    }
};

DEFINE_GETPS5PLAYERSESSIONINVITATIONS_CREATE()

} // Arson
} // Blaze
