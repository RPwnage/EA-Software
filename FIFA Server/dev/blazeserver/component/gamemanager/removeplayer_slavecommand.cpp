/*! ************************************************************************************************/
/*!
    \file removeplayer_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/removeplayer_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/commoninfo.h"

namespace Blaze
{
namespace GameManager
{

    class RemovePlayerCommand : public RemovePlayerCommandStub
    {
    public:

        RemovePlayerCommand(Message* message, RemovePlayerRequest *request, GameManagerSlaveImpl* componentImpl)
            : RemovePlayerCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~RemovePlayerCommand() override
        {
        }

    private:

        RemovePlayerCommandStub::Errors execute() override
        {
            if (mRequest.getPlayerRemovedReason() == SYS_PLAYER_REMOVE_REASON_INVALID)
            {
                return RemovePlayerError::Error::ERR_SYSTEM;
            }
            
            RemovePlayerMasterRequest masterRequest;
            masterRequest.setPlayerId(mRequest.getPlayerId());
            masterRequest.setGameId(mRequest.getGameId());
            masterRequest.setPlayerRemovedReason(mRequest.getPlayerRemovedReason());
            masterRequest.setPlayerRemovedTitleContext(mRequest.getPlayerRemovedTitleContext());
            masterRequest.setTitleContextString(mRequest.getTitleContextString());

            // if we are kick & banning, lookup the account id of the account we're kicking
            if (isRemovalReasonABan(masterRequest.getPlayerRemovedReason()))
            {
                // get account id of banned player
                UserInfoPtr bannedUserInfo;
                if (gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getPlayerId(), bannedUserInfo) != Blaze::ERR_OK)
                {
                    return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
                }

                masterRequest.setAccountId( bannedUserInfo->getPlatformInfo().getEaIds().getNucleusAccountId() );
            }

            RemovePlayerMasterResponse masterResponse;

            BlazeRpcError error = mComponent.getMaster()->removePlayerMaster(masterRequest, masterResponse);

            // We create a copy of external session params for the queued job, which later will be owned by GameManagerSlaveImpl::leaveExternalSession()
            LeaveGroupExternalSessionParametersPtr leaveGroupExternalSessonParams = BLAZE_NEW LeaveGroupExternalSessionParameters(masterResponse.getExternalSessionParams());      

            // if master filled in a user to leave from external session, do the external session leave
            if ((ERR_OK == error) && !leaveGroupExternalSessonParams->getExternalUserLeaveInfos().empty())
            {
                // Update to external session, using the removed player's external id
                mComponent.getExternalSessionJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::leaveExternalSession, leaveGroupExternalSessonParams, "GameManagerSlaveImpl::leaveExternalSession");
            }

            return commandErrorFromBlazeError(error);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_REMOVEPLAYER_CREATE()

} // namespace GameManager
} // namespace Blaze
