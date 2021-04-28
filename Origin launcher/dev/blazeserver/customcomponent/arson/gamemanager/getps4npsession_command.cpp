/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps4npsession_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionutil.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because Blaze doesn't expose any GET NP session rpc to client).
// Fetch NP session info for the game if available. If game doesn't have any NP session, will return empty NP session info.
class GetPs4NpSessionCommand : public GetPs4NpSessionCommandStub
{
public:
    GetPs4NpSessionCommand(Message* message, GetPs4NpSessionRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs4NpSessionCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs4NpSessionCommand() override {}

private:

    GetPs4NpSessionCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();

        if (!GameManager::hasFirstPartyId(ident) || mRequest.getGameId() == GameManager::INVALID_GAME_ID)
        {
            ERR_LOG("[GetPs4NpSessionCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << "), or no valid game id(" << mRequest.getGameId() << ") specified.");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        // get Blaze's tracked external session info from GM, and auth token from Blaze/Nucleus
        Blaze::ExternalUserAuthInfo authInfo;
        Blaze::GameManager::GetExternalSessionInfoMasterResponse blazeGameInfo;
        BlazeRpcError parErr = mComponent->getPsnExternalSessionParams(authInfo, blazeGameInfo, mRequest.getGameId(), ident, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);
        mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());// This rpc is high use in test, record rate limit info, regardless of pass/fail, to help scripts determine next console to use
        if (ERR_OK != parErr)
        {
            ERR_LOG("[GetPs4NpSessionCommand].execute: (Arson) internal test error, failed to load external session url parameters for the user. Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return arsonCommandErrorFromBlazeError(parErr);
        }
        Blaze::ExternalSessionIdentification& blazeGameExtSessionId = blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification();
        if (blazeGameExtSessionId.getPs4().getNpSessionId()[0] == '\0')
        {
            // Blaze game says it has no NP session.
            mResponse.getNpSession().setSessionId("");
            return arsonCommandErrorFromBlazeError(ERR_OK);
        }
        auto* forType = mComponent->getExternalSessionActivityTypeInfoPs4();
        auto blazeGameTrackedExtSessMembers = (forType ? GameManager::getListFromMap(blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), *forType) : nullptr);
        if (!blazeGameTrackedExtSessMembers || blazeGameTrackedExtSessMembers->empty())
        {
            ERR_LOG("[GetPs4NpSessionCommand].execute: (Arson) internal test error, failed to get a non-empty tracking list for the user, even though Blaze master game has session(" << blazeGameExtSessionId.getPs4().getNpSessionId() << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").");
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
        }

        // retrieve the external session from PSN
        BlazeRpcError sesErr = mComponent->retrieveExternalSessionPs4(ident, authInfo, blazeGameExtSessionId, mRequest.getLanguage(),
            mResponse.getNpSession(), mErrorResponse);
        if (sesErr == ERR_OK)
        {
            // Boiler plate check: ensure Blaze and PSN are in sync. pre: arson tests have significant enough time delays between steps.
            // First check Blaze's tracked members, are in the Sony session.
            for (Blaze::ExternalMemberInfoList::const_iterator itr = blazeGameTrackedExtSessMembers->begin(),
                end = blazeGameTrackedExtSessMembers->end();
                itr != end; ++itr)
            {
                if (!isBlazeMemberInNpSessionResult((*itr)->getUserIdentification(), mResponse.getNpSession()))
                {
                    ERR_LOG("[GetPs4NpSessionCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: Blaze game(" << mRequest.getGameId() << ") is tracking member(" << GameManager::toExtUserLogStr((*itr)->getUserIdentification()) << ") as being in its NP session(" << blazeGameExtSessionId.getPs4().getNpSessionId() << "), but it unexpectedly was NOT found in its external session(" << mResponse.getNpSession().getSessionId() << ") fetch from Sony (s2s). Caller(" << GameManager::toExtUserLogStr(ident) << ").");
                    return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                }
            }
            // Second, IF Blaze's game has  check the Sony session's members, are in the Blaze tracked members.
            for (Blaze::Arson::ArsonNpSession::ArsonNpSessionMemberList::const_iterator itr = mResponse.getNpSession().getMembers().begin(),
                end = mResponse.getNpSession().getMembers().end();
                itr != end; ++itr)
            {
                if (!isNpSessionMemberInBlazeResult(**itr, *blazeGameTrackedExtSessMembers))
                {
                    ERR_LOG("[GetPs4NpSessionCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: the Sony side NP session(" << mResponse.getNpSession().getSessionId() << ") has member(" << (*itr)->getOnlineId() << "), but the member was unexpectedly was NOT tracked by its Blaze game(" << mRequest.getGameId() << ") as being in its NP session(" << blazeGameExtSessionId.getPs4().getNpSessionId() << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").");
                    return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                }
            }
        }
        // get the lowest/worst case rate limit remaining
        if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
            mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());

        // retrieve its session-data as needed also
        if ((sesErr == ERR_OK) && mRequest.getFetchExternalSessionSessionData())
        {
            sesErr = mComponent->retrieveExternalSessionSessionData(ident, authInfo, blazeGameExtSessionId, mResponse.getSessionData(), mErrorResponse);
            if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
                mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        if ((sesErr == ERR_OK) && mRequest.getFetchExternalSessionChangeableSessionData())
        {
            sesErr = mComponent->retrieveExternalSessionChangeableSessionData(ident, authInfo, blazeGameExtSessionId, mResponse.getChangeableSessionData(), mErrorResponse);
            if (mComponent->hasHigherRateLimitRemaining(mResponse.getRateLimitInfo(), mErrorResponse.getRateLimitInfo()))
                mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        }
        return arsonCommandErrorFromBlazeError(sesErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs4NpSessionCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
    {
        blazeError = GameManager::convertExternalServiceErrorToGameManagerError(blazeError);
        switch (blazeError)
        {
        case GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
            if (mRequest.getFailOnRateLimitError())
            {
                return commandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SERVICE_BUSY);
            }
            else
            {
                WARN_LOG("[GetPs4NpSessionCommand].execute (Arson): User was rate limited with error(" << ErrorHelp::getErrorName(blazeError) << "). Ignoring, to allow caller to back off and retry. Caller(" << GameManager::toExtUserLogStr(mRequest.getExternalIdent()) << ")");
                return commandErrorFromBlazeError(ERR_OK);
            }
        default:
            return commandErrorFromBlazeError(blazeError);
        };
    }

    /*! ************************************************************************************************/
    /*! \brief helper to check whether the Blaze member was in the NP session fetched from PSN
    ***************************************************************************************************/
    bool isBlazeMemberInNpSessionResult(const Blaze::UserIdentification& blazeMember, const Blaze::Arson::ArsonNpSession& npSession)
    {
        for (auto itr : npSession.getMembers())
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64(itr->getAccountId());
            if (id == blazeMember.getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief helper to check whether the NP session's member fetched from PSN, is in the Blaze game/external session tracking list
    ***************************************************************************************************/
    bool isNpSessionMemberInBlazeResult(const Blaze::Arson::ArsonNpSessionMember& npSessionMember,
        const Blaze::ExternalMemberInfoList& blazeList)
    {
        for (auto itr : blazeList)
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64(npSessionMember.getAccountId());
            if (id == itr->getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        return false;
    }
};

DEFINE_GETPS4NPSESSION_CREATE()

} // Arson
} // Blaze
