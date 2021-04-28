/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps5playersession_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because Blaze doesn't expose any GET PSN PlayerSession rpc to client).
// Fetch PSN PlayerSession info for the game if available. If game doesn't have any PSN PlayerSession, will return empty PSN PlayerSession info.
class GetPs5PlayerSessionCommand : public GetPs5PlayerSessionCommandStub
{
public:
    GetPs5PlayerSessionCommand(Message* message, GetPs5PlayerSessionRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs5PlayerSessionCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs5PlayerSessionCommand() override {}

private:

    GetPs5PlayerSessionCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();
        eastl::string identLogBuf;
        eastl::string callerLogBuf;

        if (!GameManager::hasFirstPartyId(ident) || mRequest.getGameId() == GameManager::INVALID_GAME_ID)
        {
            mErrorResponse.setMessage((StringBuilder() << "[GetPs5PlayerSessionCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << "), or no valid game id(" << mRequest.getGameId() << ") specified.").c_str());
            ERR_LOG(mErrorResponse.getMessage());
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        // get Blaze's tracked external session info from GM, and auth token from Blaze/Nucleus
        Blaze::ExternalUserAuthInfo authInfo;
        Blaze::GameManager::GetExternalSessionInfoMasterResponse blazeGameInfo;
        BlazeRpcError parErr = mComponent->getPsnExternalSessionParams(authInfo, blazeGameInfo, mRequest.getGameId(), ident, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);//Rate limit APIs N/A currently from Sony //mErrorResponse.getRateLimitInfo().copyInto(mResponse.getRateLimitInfo());
        if (ERR_OK != parErr)
        {
            mErrorResponse.setMessage((StringBuilder() << "[GetPs5PlayerSessionCommand].execute: (Arson) internal test error, failed to load external session url parameters for the user. Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
            ERR_LOG(mErrorResponse.getMessage());
            return arsonCommandErrorFromBlazeError(parErr);
        }
        eastl::string gameLogBuf;
        ArsonSlaveImpl::toExtGameInfoLogStr(gameLogBuf, blazeGameInfo);

        auto& blazeGameExtSessionId = blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification();
        const char8_t* sessionId = blazeGameExtSessionId.getPs5().getPlayerSession().getPlayerSessionId();
        if (sessionId[0] == '\0')
        {
            // Blaze game says it has no PSN PlayerSession.
            mResponse.getPsnPlayerSession().setSessionId("");
            return arsonCommandErrorFromBlazeError(ERR_OK);
        }

        auto* forType = mComponent->getExternalSessionActivityTypeInfoPs5PlayerSession(ident.getPlatformInfo().getClientPlatform());
        auto blazeGameTrackedExtSessMembers = (forType ? GameManager::getListFromMap(blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), *forType) : nullptr);
        if (!blazeGameTrackedExtSessMembers || blazeGameTrackedExtSessMembers->empty())
        {
            mErrorResponse.setMessage((StringBuilder() << "[GetPs5PlayerSessionCommand].execute: (Arson) internal test error, failed to get a non-empty tracking list for the user, even though Blaze master game has session(" << sessionId << "). game(" << gameLogBuf << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
            ERR_LOG(mErrorResponse.getMessage());
            return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
        }


        // retrieve the external session from PSN
        BlazeRpcError sesErr = mComponent->retrieveExternalPs5PlayerSession(ident, authInfo, blazeGameExtSessionId, mRequest.getLanguage(),
            mResponse.getPsnPlayerSession(), mResponse.getDecodedCustomData1(), mResponse.getDecodedCustomData2(), mErrorResponse);
        if (sesErr == ERR_OK)
        {
            // These aren't directly available from PSN, so just set em from Blaze (for test scripts use):
            mResponse.setGameId(mRequest.getGameId());
            blazeGameExtSessionId.copyInto(mResponse.getSessionIdentification());

            const char8_t* rspSessionId = mResponse.getPsnPlayerSession().getSessionId();

            // Boiler plate checks:

            // Ensure Blaze and PSN are in sync. pre: arson tests have significant enough time delays between steps.
            // First check Blaze's tracked members, are in the Sony session.
            for (const auto & blazeGameTrackedExtSessMember : *blazeGameTrackedExtSessMembers)
            {
                if (!isBlazePlayerInPsnPlayerSessionResult(blazeGameTrackedExtSessMember->getUserIdentification(), mResponse.getPsnPlayerSession()))
                {
                    mErrorResponse.setMessage((StringBuilder() << "[GetPs5PlayerSessionCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: Blaze game(" << gameLogBuf << ")" <<
                        " is tracking member(" << GameManager::toExtUserLogStr(blazeGameTrackedExtSessMember->getUserIdentification()) << ") as being in its PSN PlayerSession(" << sessionId << "), but it unexpectedly was NOT found in its external session(" << rspSessionId << ") fetched from Sony (s2s). Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
                    ERR_LOG(mErrorResponse.getMessage());
                    return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                }
            }
            // Second, IF Blaze's game has  check the Sony session's members, are in the Blaze tracked members.
            for (auto & itr : mResponse.getPsnPlayerSession().getPlayers())
            {
                if (!isPsnPlayerSessionPlayerInBlazeResult(*itr, *blazeGameTrackedExtSessMembers))
                {
                    mErrorResponse.setMessage((StringBuilder() << "[GetPs5PlayerSessionCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: the Sony side PSN PlayerSession(" << rspSessionId << ") has member(" << itr->getOnlineId() <<
                        "), but the member was unexpectedly was NOT tracked by its Blaze game game(" << gameLogBuf << ") as being in its PSN PlayerSession(" << sessionId << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
                    ERR_LOG(mErrorResponse.getMessage());
                    return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                }
            }            
        }

        return arsonCommandErrorFromBlazeError(sesErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs5PlayerSessionCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
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

    /*! ************************************************************************************************/
    /*! \brief helper to check whether the Blaze member was in the PSN PlayerSession fetched from PSN
    ***************************************************************************************************/
    bool isBlazePlayerInPsnPlayerSessionResult(const Blaze::UserIdentification& blazeMember, const Blaze::Arson::ArsonPsnPlayerSession& session)
    {
        for (auto itr = session.getPlayers().begin(), end = session.getPlayers().end();
            itr != end; ++itr)
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64((*itr)->getAccountId());
            if (id == blazeMember.getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        for (auto itr = session.getSpectators().begin(), end = session.getSpectators().end();
            itr != end; ++itr)
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64((*itr)->getAccountId());
            if (id == blazeMember.getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief helper to check whether the PSN PlayerSession's member fetched from PSN, is in the Blaze game/external session tracking list
    ***************************************************************************************************/
    bool isPsnPlayerSessionPlayerInBlazeResult(const Blaze::Arson::ArsonPsnPlayerSessionMember& sessionMember,
        const Blaze::ExternalMemberInfoList& blazeList)
    {
        for (auto itr = blazeList.begin(), end = blazeList.end();
            itr != end; ++itr)
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64(sessionMember.getAccountId());
            if (id == (*itr)->getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        return false;
    }
};

DEFINE_GETPS5PLAYERSESSION_CREATE()

} // Arson
} // Blaze
