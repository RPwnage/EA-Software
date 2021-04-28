/*! ************************************************************************************************/
/*!
    \file canceltournamentgame_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/canceltournamentgame_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "framework/util/networktopologycommoninfo.h" // for isDedicatedHostedTopology

namespace Blaze
{
namespace GameManager
{
    class CancelTournamentGameCommand : public CancelTournamentGameCommandStub
    {
    public:
        CancelTournamentGameCommand(Message* message, CancelTournamentGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : CancelTournamentGameCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~CancelTournamentGameCommand() override
        {
        }

    private:
        CancelTournamentGameCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            Search::GetGamesResponse ggRsp;
            BlazeRpcError err = fetchGameData(ggRsp);
            if (err != ERR_OK)
            {
                return commandErrorFromBlazeError(err);
            }
            GameManager::ReplicatedGameData& gameData = ggRsp.getFullGameData().front()->getGame();

            // super user privilege required for TO to be able to remove game on master
            UserSession::SuperUserPermissionAutoPtr ptr(true);
            if (isDedicatedHostedTopology(gameData.getNetworkTopology()) && !gameData.getGameSettings().getVirtualized())
            {
                ReturnDedicatedServerToPoolRequest retReq;
                retReq.setGameId(mRequest.getGameId());
                return commandErrorFromBlazeError(mComponent->returnDedicatedServerToPool(retReq));
            }
            else
            {
                // Note: game server implementation should handle recreating the dedicated server resource as needed,
                // e.g. by checking if its a virtualized game with SYS_GAME_ENDING in the destruction notification.
                DestroyGameRequest destroyReq;
                destroyReq.setGameId(mRequest.getGameId());
                destroyReq.setDestructionReason(SYS_GAME_ENDING);
                return commandErrorFromBlazeError(mComponent->destroyGame(destroyReq, mResponse));
            }
        }

        BlazeRpcError fetchGameData(Search::GetGamesResponse& ggRsp) const
        {
            Search::GetGamesRequest ggReq;
            ggReq.setFetchOnlyOne(true);
            ggReq.getGameIds().push_back(mRequest.getGameId());
            ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);

            BlazeRpcError fetchErr = mComponent->fetchGamesFromSearchSlaves(ggReq, ggRsp);
            if (fetchErr != ERR_OK)
            {
                return fetchErr;
            }
            if (ggRsp.getFullGameData().empty())
            {
                return GAMEMANAGER_ERR_INVALID_GAME_ID;
            }
            return ERR_OK;
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CANCELTOURNAMENTGAME_CREATE()

} // namespace GameManager
} // namespace Blaze
