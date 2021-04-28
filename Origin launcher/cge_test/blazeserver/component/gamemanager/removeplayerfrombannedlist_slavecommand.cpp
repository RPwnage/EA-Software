/*! ************************************************************************************************/
/*!
    \file removeplayerfrombannedlist_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/removeplayerfrombannedlist_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Look up a user (possibly from the db, if they aren't in game or logged in), then
            forward ban cmd to master with the banned user's blazeId.
    ***************************************************************************************************/
    class RemovePlayerFromBannedListCommand : public RemovePlayerFromBannedListCommandStub
    {
    public:

        RemovePlayerFromBannedListCommand(Message* message, RemovePlayerFromBannedListRequest *request, GameManagerSlaveImpl* componentImpl)
            : RemovePlayerFromBannedListCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~RemovePlayerFromBannedListCommand() override
        {
        }

    private:

        RemovePlayerFromBannedListCommandStub::Errors execute() override
        {
            // lookup the account id of the removed player
            UserInfoPtr userInfo;
            if (gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo) != Blaze::ERR_OK)
            {
                return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
            }

            // Set info for our request to the master
            RemovePlayerFromBannedListMasterRequest masterRequest;
            masterRequest.setGameId(mRequest.getGameId());
            masterRequest.setPlayerId(mRequest.getBlazeId());
            masterRequest.setAccountId(userInfo->getPlatformInfo().getEaIds().getNucleusAccountId());

            BlazeRpcError remErr = mComponent.getMaster()->removePlayerFromBannedListMaster(masterRequest);
            return commandErrorFromBlazeError( remErr );
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_REMOVEPLAYERFROMBANNEDLIST_CREATE()

} // namespace GameManager
} // namespace Blaze
