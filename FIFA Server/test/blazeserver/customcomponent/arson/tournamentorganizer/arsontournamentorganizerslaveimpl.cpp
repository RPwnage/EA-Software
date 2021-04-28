/*************************************************************************************************/
/*!
    \file   arsontournamentorganizerslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arsontournamentorganizerslaveimpl.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for getNextGameExternalSessionUniqueId in getNextTeamExternalSessionName
#include "framework/controller/controller.h" // for gController in getNextTeamExternalSessionName
#include "gamemanager/rpc/gamemanagerslave.h" // for GameManagerSlave in getMatchData
#include "gamemanager/tdf/gamebrowser.h" //for GetFullGameDataRequest in getMatchData()
#include "gamemanager/gamemanagermasterimpl.h" // for doSendExternalTournamentGameEvent in testSendGameEvent commands

namespace Blaze
{
namespace Arson
{
    ArsonTournamentOrganizerSlave* ArsonTournamentOrganizerSlave::createImpl()
    {
        return BLAZE_NEW_NAMED("ArsonTournamentOrganizerSlaveImpl") ArsonTournamentOrganizerSlaveImpl();
    }

    ArsonTournamentOrganizerSlaveImpl::ArsonTournamentOrganizerSlaveImpl() :
        mExternalTournamentUtil(nullptr)
    {
    }

    ArsonTournamentOrganizerSlaveImpl::~ArsonTournamentOrganizerSlaveImpl()
    {
        if (mExternalTournamentUtil != nullptr)
        {
            delete mExternalTournamentUtil;
        }
    }

    bool ArsonTournamentOrganizerSlaveImpl::onConfigure()
    {
        const ArsonTournamentOrganizerConfig &config = getConfig();
        INFO_LOG("[ArsonTournamentOrganizerSlaveImpl].onconfigure: \n" << config);

        // possible future enhancement: implement db, setup prepared statements here

        mExternalTournamentUtil = ArsonExternalTournamentUtil::createExternalTournamentService(getConfig());
        if (mExternalTournamentUtil == nullptr)
        {
            return false;  // (err already logged)
        }

        return true;
    }

    /*! \brief return a unique name for a team */
    BlazeRpcError ArsonTournamentOrganizerSlaveImpl::getNextTeamExternalSessionName(eastl::string& result, const ArsonTournamentOrganizerConfig& config) const
    {
        uint64_t uid = Blaze::GameManager::getNextGameExternalSessionUniqueId();
        if (uid == 0)
            return ERR_SYSTEM;

        result.append_sprintf("%s%s_team_%" PRIu64, config.getExternalSessions().getExternalSessionNamePrefix(), gController->getDefaultServiceName(), uid);

        // MS Tournament Hubs known issue: team ids currently must be lower case to avoid http 403. MS is looking into fixing this
        result.make_lower();

        return ERR_OK;
    }

    BlazeRpcError ArsonTournamentOrganizerSlaveImpl::getMatchData(Blaze::GameManager::GameId matchId, Blaze::GameManager::ReplicatedGameData& matchData,
        Blaze::GameManager::ReplicatedGamePlayerList* matchPlayerData /*= nullptr*/)
    {
        BlazeRpcError error;
        Blaze::GameManager::GameManagerSlave* gmComp = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true, &error));
        if (gmComp == nullptr)
        {
            ERR_LOG("[ArsonTournamentOrganizerSlaveImpl].getMatchData: internal unexpected error " << Blaze::ErrorHelp::getErrorName(error) << ", GameManager component n/a. Unable to get game data.");
            return ERR_COMPONENT_NOT_FOUND;
        }

        Blaze::GameManager::GetFullGameDataRequest ggReq;
        Blaze::GameManager::GetFullGameDataResponse ggResp;
        ggReq.getGameIdList().push_back(matchId);
        
        BlazeRpcError lookupUserGameError = gmComp->getFullGameData(ggReq, ggResp);
        if (lookupUserGameError != ERR_OK)
            return lookupUserGameError;
        if (ggResp.getGames().empty())
        {
            ERR_LOG("[ArsonTournamentOrganizerSlaveImpl].getMatchData: could not find the tournament match(" << matchId << "). Unable to retrieve its data.");
            return ARSON_TO_ERR_INVALID_MATCH_ID;
        }
        ggResp.getGames().front()->getGame().copyInto(matchData);
        if (matchPlayerData != nullptr)
        {
            ggResp.getGames().front()->getGameRoster().copyInto(*matchPlayerData);
        }
        return ERR_OK;
    }

    BlazeRpcError ArsonTournamentOrganizerSlaveImpl::testSendTournamentGameEvent(const Blaze::Arson::TestSendTournamentGameEventRequest& request,
        Blaze::Arson::TestSendTournamentGameEventResponse& response, bool testStartEvent)
    {
        // get the game
        Blaze::GameManager::ReplicatedGameData gameData;
        Blaze::GameManager::ReplicatedGamePlayerList gamePlayerData;
        BlazeRpcError err = getMatchData(request.getMatchId(), gameData, &gamePlayerData);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonTournamentOrganizerSlaveImpl].execute: failed to get matchId/gameId(" << request.getMatchId() << "), Error(" << ErrorHelp::getErrorName(err) << ").");
            response.setErrorDetails(ErrorHelp::getErrorDescription(err));
            return err;
        }

        const char8_t* nonBaseUri = (testStartEvent ? gameData.getGameStartEventUri() : gameData.getGameEndEventUri());

        if ((gameData.getGameEventAddress()[0] == '\0') || (nonBaseUri[0] == '\0'))
        {
            ERR_LOG("[ArsonTournamentOrganizerSlaveImpl].execute: failed to send event for matchId/gameId(" << request.getMatchId() << "), missing/invalid game event address base(" <<
                gameData.getGameEventAddress() << "), or non-base(" << nonBaseUri << ") URI. For event(" << (testStartEvent ? "start" : "end") << ")");
            err = ARSON_TO_ERR_INVALID_GAME_EVENT_ADDRESS;
        }
        else
        {
            TRACE_LOG("[ArsonTournamentOrganizerSlaveImpl].execute: ARSON testing sending event for match(" << request.getMatchId() << "), gameId(" << gameData.getGameId() <<
                "), tournament(" << gameData.getTournamentIdentification().getTournamentId() << "), to URL(" << gameData.getGameEventAddress() << nonBaseUri << ").");

            // send
            Blaze::GameManager::ExternalHttpGameEventData& dataSent = response.getAttemptedEventData();
            dataSent.setGameId(gameData.getGameId());
            gameData.getTournamentIdentification().copyInto(dataSent.getTournamentIdentification());
            if (testStartEvent)
            {
                for (auto itr : gamePlayerData)
                {
                    // check if there's an override:
                    const Blaze::GameManager::ExternalHttpGamePlayerEventData* fakeData = findPlayerEventData(request.getFakeGameRoster(), *itr);
                    Blaze::GameManager::SlotType slotType = (fakeData == nullptr ? itr->getSlotType() : Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
                    Blaze::GameManager::PlayerState state = (fakeData == nullptr ? itr->getPlayerState() : fakeData->getPlayerState());
                    const char8_t* encryptedMemberBlazeId = (fakeData == nullptr ? itr->getEncryptedBlazeId() : fakeData->getEncryptedBlazeId());

                    Blaze::GameManager::GameManagerMasterImpl::populateTournamentGameEventPlayer(dataSent.getGameRoster(), slotType, encryptedMemberBlazeId, itr->getPlayerId(), state);
                }
            }

            err = Blaze::GameManager::GameManagerMasterImpl::doSendTournamentGameEvent(&dataSent, eastl::string(gameData.getGameEventAddress()),
                eastl::string(nonBaseUri), eastl::string(JSON_CONTENTTYPE), getConfig().getSendTournamentEventRetryLimit());
        }
        response.setAttemptedUrl(eastl::string(eastl::string::CtorSprintf(), "%s%s", gameData.getGameEventAddress(), nonBaseUri).c_str());
        response.setAttemptedHttpMethod(HttpProtocolUtil::getStringFromHttpMethod(Blaze::GameManager::GameManagerMasterImpl::getTournamentGameEventHttpMethod()));
        response.setErrorDetails(ErrorHelp::getErrorDescription(err));
        return err;
    }

    const Blaze::GameManager::ExternalHttpGamePlayerEventData* ArsonTournamentOrganizerSlaveImpl::findPlayerEventData(
        const Blaze::Arson::TestSendTournamentGameEventRequest::ExternalHttpGamePlayerEventDataList& list, const Blaze::GameManager::ReplicatedGamePlayer& player) const
    {
        for (auto reqItr : list)
        {
            if ((reqItr->getBlazeId() == player.getPlayerId()) || (blaze_strcmp(reqItr->getEncryptedBlazeId(), player.getEncryptedBlazeId()) == 0))
            {
                return &(*reqItr);
            }
        }
        return nullptr;
    }

}
} // Blaze
