/*! ************************************************************************************************/
/*!
    \file banplayer_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/banplayer_stub.h"
#include "gamemanagerslaveimpl.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Look up a user (possibly from the db, if they aren't in game or logged in), then
            forward ban cmd to master with the banned user's blazeId.
    ***************************************************************************************************/
    class BanPlayerCommand : public BanPlayerCommandStub
    {
    public:

        BanPlayerCommand(Message* message, BanPlayerRequest *request, GameManagerSlaveImpl* componentImpl)
            : BanPlayerCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~BanPlayerCommand() override
        {
        }

    private:

        BanPlayerCommandStub::Errors execute() override
        {
            // Set info for our request to the master
            BanPlayerMasterRequest masterRequest;
            masterRequest.setGameId(mRequest.getGameId());
            masterRequest.setPlayerRemovedTitleContext(mRequest.getPlayerRemovedTitleContext());

            masterRequest.getPlayerIds().reserve(mRequest.getPlayerIds().size());
            masterRequest.getAccountIds().reserve(mRequest.getPlayerIds().size());

            PlayerIdList::const_iterator it = mRequest.getPlayerIds().begin();
            PlayerIdList::const_iterator end = mRequest.getPlayerIds().end();
            for (; it != end; ++it)
            {
                // lookup the account id of the banned user (they don't have to be a game member -- or even logged in)
                // NOTE: this can hit the db (and block)
                UserInfoPtr bannedUserInfo;
                if (gUserSessionManager->lookupUserInfoByBlazeId(*it, bannedUserInfo) != Blaze::ERR_OK)
                {
                    return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
                }

                masterRequest.getPlayerIds().push_back(*it);
                masterRequest.getAccountIds().push_back( bannedUserInfo->getPlatformInfo().getEaIds().getNucleusAccountId() );
            }            

            BanPlayerMasterResponse masterResponse;
            BlazeRpcError banErr = mComponent.getMaster()->banPlayerMaster(masterRequest, masterResponse);

            // We create a copy of external session params for the fiber pool schedule call, which later will be owned by GameManagerSlaveImpl::leaveExternalSession()
            LeaveGroupExternalSessionParametersPtr leaveGroupExternalSessonParams = BLAZE_NEW LeaveGroupExternalSessionParameters(masterResponse.getExternalSessionParams());      

            // Update to external session, the players which were updated by the blaze master call
            if ((ERR_OK == banErr) && !leaveGroupExternalSessonParams->getExternalUserLeaveInfos().empty())
            {
                mComponent.getExternalSessionJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::leaveExternalSession, leaveGroupExternalSessonParams, "GameManagerSlaveImpl::leaveExternalSession");
            }

            return commandErrorFromBlazeError( banErr );
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_BANPLAYER_CREATE()

} // namespace GameManager
} // namespace Blaze
