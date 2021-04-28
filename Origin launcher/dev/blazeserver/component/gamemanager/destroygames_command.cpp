/*! ************************************************************************************************/
/*!
    \file destroygames_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/destroygames_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"


namespace Blaze
{
namespace GameManager
{

    class DestroyGamesCommand : public DestroyGamesCommandStub
    {
    public:
        DestroyGamesCommand(Message* message, DestroyGamesRequest* request, GameManagerSlaveImpl* componentImpl)
            :   DestroyGamesCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~DestroyGamesCommand() override {}

    private:
        
        DestroyGamesCommandStub::Errors execute() override
        {
            // check caller's admin rights
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_BATCH_DESTROY_GAMES, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            BlazeRpcError error = ERR_OK;

            if (!mRequest.getGameIdList().empty())
            {
                DestroyGameRequest masterReq;
                DestroyGameResponse masterResp;
                masterReq.setDestructionReason(mRequest.getDestructionReason());
                GameIdList& gameIdList = mResponse.getGameIdList();
                gameIdList.reserve(mRequest.getGameIdList().size());
                GameIdList::const_iterator idIter = mRequest.getGameIdList().begin();
                GameIdList::const_iterator idEnd = mRequest.getGameIdList().end();
                for (; idIter != idEnd; ++idIter)
                {
                    masterReq.setGameId(*idIter);
                    BlazeRpcError rc = mComponent.getMaster()->destroyGameMaster(masterReq, masterResp);
                    // if game is already gone, then it's counted as destroyed
                    if (rc == ERR_OK || rc == GAMEMANAGER_ERR_INVALID_GAME_ID)
                    {
                        gameIdList.push_back(*idIter);
                    }
                    else
                    {
                        WARN_LOG("[DestroyGamesCommand].execute: Failed to destroy game " << *idIter << " in batch, error(" << ErrorHelp::getErrorName(rc) << ")");
                    }
                }

                // only return non-ERR_OK error all destroys fail
                if (gameIdList.empty())
                {
                    error = ERR_SYSTEM;
                }
            }

            return commandErrorFromBlazeError(error);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_DESTROYGAMES_CREATE()

} // namespace GameManager
} // namespace Blaze
