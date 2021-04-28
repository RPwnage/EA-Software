/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getps5match_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for convertExternalServiceErrorToGameManagerError in arsonCommandErrorFromBlazeError
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because Blaze doesn't expose any GET PSN Match rpc to client).
// Fetch PSN Match info for the game if available. If game doesn't have any PSN Match, will return empty PSN Match info.
class GetPs5MatchCommand : public GetPs5MatchCommandStub
{
public:
    GetPs5MatchCommand(Message* message, GetPs5MatchRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPs5MatchCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetPs5MatchCommand() override {}

private:

    GetPs5MatchCommandStub::Errors execute() override
    {
        UserIdentification& ident = mRequest.getExternalIdent();
        eastl::string identLogBuf;
        eastl::string callerLogBuf;

        const char8_t* matchId = mRequest.getSessionIdentification().getPs5().getMatch().getMatchId();
        if (!GameManager::hasFirstPartyId(ident) || ((mRequest.getGameId() == GameManager::INVALID_GAME_ID) && (matchId[0] == '\0')))
        {
            mErrorResponse.setMessage((StringBuilder() << "[GetPs5MatchCommand].execute: (Arson) internal test error, no valid user identification for the user(" << GameManager::toExtUserLogStr(ident) << "), or neither a valid GameId(" << mRequest.getGameId() << ") nor MatchId(" << matchId << "), for fetching the Match, was specified.").c_str());
            WARN_LOG(mErrorResponse.getMessage());
            return arsonCommandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }

        ExternalSessionIdentification blazeGameExtSessionId = mRequest.getSessionIdentification();
        eastl::string gameLogBuf(eastl::string::CtorSprintf(), "%" PRIu64 "", mRequest.getGameId());

        Blaze::GameManager::GetExternalSessionInfoMasterResponse blazeGameInfo;
        if (mRequest.getGameId() != GameManager::INVALID_GAME_ID)
        {
            // get Blaze's tracked external session info from GM
            Blaze::ExternalUserAuthInfo authInfo;
            BlazeRpcError parErr = mComponent->getPsnExternalSessionParams(authInfo, blazeGameInfo, mRequest.getGameId(), ident, EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH);
            if (ERR_OK != parErr)
            {
                mErrorResponse.setMessage((StringBuilder() << "[GetPs5MatchCommand].execute: (Arson) internal test error, failed to load external session url parameters for the user. Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
                ERR_LOG(mErrorResponse.getMessage());
                return arsonCommandErrorFromBlazeError(parErr);
            }
            blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().copyInto(blazeGameExtSessionId);
            ArsonSlaveImpl::toExtGameInfoLogStr(gameLogBuf, blazeGameInfo);

            Blaze::GameManager::ReplicatedGameData gameData;
            BlazeRpcError getErr = mComponent->getGameDataFromSearchSlaves(mRequest.getGameId(), gameData);// for logging
            if (getErr != ERR_OK)
            {
                ERR_LOG("[GetPs5MatchCommand].execute: (Arson) Fetch of GM game data got error code " << ErrorHelp::getErrorName(getErr));
                return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
            }
            else
            {
                // boiler plate validate game vs ext session game info cache?
            }
        }
        const char8_t* sessionId = blazeGameExtSessionId.getPs5().getMatch().getMatchId();
        if (sessionId[0] == '\0')
        {
            // Blaze game says it has no PSN Match.
            return arsonCommandErrorFromBlazeError(ERR_OK);
        }

        // retrieve the external session from PSN
        BlazeRpcError sesErr = mComponent->retrieveExternalPs5Match(ident, blazeGameExtSessionId, nullptr, mResponse.getPsnMatch(), mErrorResponse);
        if (sesErr == ERR_OK)
        {
            // These aren't directly available from PSN, so just set em from Blaze (for test scripts use):
            mResponse.setGameId(mRequest.getGameId());
            blazeGameExtSessionId.copyInto(mResponse.getSessionIdentification());

#if 0 //  The below is currently disabled due to dependencies on timing.  In particular, due to GOSREQ-3819 there can now be up to a second of delay for queued PS5 Match membership updates to be in sync with the Blaze Game.

            // The below unit like checks maybe candidates for removal, but slotted here since closer to the server and easy to debug:

            // Boiler plate checks. Only possible if mRequest.getGameId() != INVALID_GAME_ID, above.
            auto* forType = mComponent->getExternalSessionActivityTypeInfoPs5Match();
            auto blazeGameTrackedExtSessMembers = (forType ? GameManager::getListFromMap(blazeGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), *forType) : nullptr);
            if (blazeGameTrackedExtSessMembers != nullptr)
            {
                // Ensure Blaze and PSN are in sync. pre: arson tests have significant enough time delays between steps.
               // First check Blaze's tracked members, are in the Sony session.
                for (auto itr : *blazeGameTrackedExtSessMembers)
                {
                    if (!isBlazeMemberActivelyInPsnMatch(itr->getUserIdentification(), mResponse.getPsnMatch()))
                    {
                        mErrorResponse.setMessage((StringBuilder() << "[GetPs5MatchCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: Blaze game(" << gameLogBuf << ") is tracking member(" << GameManager::toExtUserLogStr(itr->getUserIdentification()) << ") as being active (i.e. with join flag true) in its PSN Match(" << sessionId
                            << "), but it unexpectedly was NOT found/active in its PSN Match(" << sessionId << ") fetched from Sony (s2s). Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
                        ERR_LOG(mErrorResponse.getMessage());
                        return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                    }
                }
                // Second, check the Sony session's members, are in the Blaze tracked members.
                for (auto itr : mResponse.getPsnMatch().getInGameRoster().getPlayers())
                {
                    if (itr->getJoinFlag() && !isPsnMatchPlayerInBlazeTrackingList(*itr, *blazeGameTrackedExtSessMembers))
                    {
                        mErrorResponse.setMessage((StringBuilder() << "[GetPs5MatchCommand].execute: (Arson) Boiler plate check failed: Blaze unexpectedly out-of-sync with PSN: the Sony side PSN Match(" << sessionId << ") has member(" << itr->getOnlineId() <<
                            "), but the member was unexpectedly was NOT tracked by its Blaze game(" << gameLogBuf << ") as being in its PSN Match(" << sessionId << "). Caller(" << GameManager::toExtUserLogStr(ident) << ").").c_str());
                        ERR_LOG(mErrorResponse.getMessage());
                        return arsonCommandErrorFromBlazeError(ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC);
                    }
                }
            }
#endif
        }

        return arsonCommandErrorFromBlazeError(sesErr);
    }

private:
    ArsonSlaveImpl* mComponent;

    GetPs5MatchCommandStub::Errors arsonCommandErrorFromBlazeError(BlazeRpcError blazeError) const
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
    /*! \brief helper to check whether the Blaze member was joined in the PSN Match fetched from PSN (joinFlag true)
    ***************************************************************************************************/
    bool isBlazeMemberActivelyInPsnMatch(const Blaze::UserIdentification& blazeMember, const Blaze::Arson::ArsonPsnMatch& psnMatch)
    {
        for (auto itr : psnMatch.getInGameRoster().getPlayers())
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64(itr->getAccountId());
            if (id == blazeMember.getPlatformInfo().getExternalIds().getPsnAccountId())
                return itr->getJoinFlag();
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief helper to check whether the PSN Match's member fetched from PSN, is in the Blaze game/external session tracking list
    ***************************************************************************************************/
    bool isPsnMatchPlayerInBlazeTrackingList(const Blaze::Arson::ArsonPsnMatchPlayer& psnMatchPlayer, const Blaze::ExternalMemberInfoList& blazeList)
    {
        for (auto itr : blazeList)
        {
            ExternalPsnAccountId id = EA::StdC::AtoU64(psnMatchPlayer.getAccountId());
            if (id == itr->getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId())
                return true;
        }
        return false;
    }
};

DEFINE_GETPS5MATCH_CREATE()

} // Arson
} // Blaze
