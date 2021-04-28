/*! ************************************************************************************************/
/*!
    \file gettournamentgamestatus_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/gettournamentgamestatus_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameManager
{
    class GetTournamentGameStatusCommand : public GetTournamentGameStatusCommandStub
    {
    public:
        GetTournamentGameStatusCommand(Message* message, GetTournamentGameStatusRequest *request, GameManagerSlaveImpl* componentImpl)
            : GetTournamentGameStatusCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~GetTournamentGameStatusCommand() override
        {
        }

    private:
        GetTournamentGameStatusCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            GetGameDataFromIdRequest getDataReq;
            getDataReq.setListConfigName(mComponent->getConfig().getGameBrowser().getTournamentGameStatusListConfigName());
            getDataReq.getGameIds().push_back(mRequest.getGameId());

            GameBrowserDataList getDataResp;

            GetTournamentGameStatusCommandStub::Errors err = commandErrorFromBlazeError(mComponent->getGameDataFromId(getDataReq, getDataResp));
            if (err == ERR_OK && getDataResp.getGameData().size() > 0)
            {
                GameBrowserGameData& gameData = *getDataResp.getGameData().front();
                mResponse.setGameId(gameData.getGameId());
                mResponse.setGameName(gameData.getGameName());
                mResponse.setGameState(gameData.getGameState());

                // for security, only expose encrypted blaze ids
                for (GameBrowserGameData::GameBrowserPlayerDataList::const_iterator itr = gameData.getGameRoster().begin(), end = gameData.getGameRoster().end(); itr != end; ++itr)
                {
                    GameBrowserPlayerData* playerData = mResponse.getGameRoster().pull_back();
                    playerData->setEncryptedBlazeId((*itr)->getEncryptedBlazeId());
                    playerData->setExternalId((*itr)->getExternalId());
                    convertToPlatformInfo((*itr)->getPlatformInfo(), (*itr)->getExternalId(), nullptr, INVALID_ACCOUNT_ID, (*itr)->getPlatformInfo().getClientPlatform());
                    playerData->setPlayerName((*itr)->getPlayerName());
                    playerData->setPlayerState((*itr)->getPlayerState());
                    playerData->setJoinedGameTimestamp((*itr)->getJoinedGameTimestamp());
                    playerData->setReservationCreationTimestamp((*itr)->getReservationCreationTimestamp());
                    playerData->setRoleName((*itr)->getRoleName());
                    playerData->setSlotType((*itr)->getSlotType());
                    playerData->setTeamIndex((*itr)->getTeamIndex());
                    (*itr)->getPlayerAttribs().copyInto(playerData->getPlayerAttribs());

                    if ((playerData->getEncryptedBlazeId()[0] == '\0') && EA_LIKELY(gController && ((blaze_stricmp(gController->getServiceEnvironment(), "dev") == 0) || (blaze_stricmp(gController->getServiceEnvironment(), "test") == 0))))//just in case guard env
                    {
                        playerData->setPlayerId((*itr)->getPlayerId());
                    }
                }
            }
            return err;
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_GETTOURNAMENTGAMESTATUS_CREATE()

} // namespace GameManager
} // namespace Blaze
