/*! ************************************************************************************************/
/*!
    \file leavegamebygroup_slavecommands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/leavegamebygroup_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/commoninfo.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief LeaveGameByGroup and all players in the group from the game; 
    ***************************************************************************************************/
    class LeaveGameByGroupCommand : public LeaveGameByGroupCommandStub
    {
    public:
        LeaveGameByGroupCommand(Message* message, RemovePlayerRequest* request, GameManagerSlaveImpl* componentImpl)
            : LeaveGameByGroupCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~LeaveGameByGroupCommand() override {}

    private:
        
        LeaveGameByGroupCommandStub::Errors execute() override
        {            
            if (mRequest.getPlayerRemovedReason() == SYS_PLAYER_REMOVE_REASON_INVALID)
            {
                return LeaveGameByGroupError::Error::ERR_SYSTEM;
            }
            
            LeaveGameByGroupMasterRequest masterRequest;
            masterRequest.setGameId(mRequest.getGameId());
            masterRequest.setPlayerId(mRequest.getPlayerId());
            masterRequest.setPlayerRemovedTitleContext(mRequest.getPlayerRemovedTitleContext());
            masterRequest.setTitleContextString(mRequest.getTitleContextString());
            masterRequest.setPlayerRemovedReason(mRequest.getPlayerRemovedReason());
            UserGroupId userGroupId = mRequest.getGroupId(); 

            // Disallow an invalid group Id (For example, an empty local user group) from a slave command.   
            if (userGroupId == EA::TDF::OBJECT_ID_INVALID)
            {
                return GAMEMANAGER_ERR_INVALID_GROUP_ID;
            }

            // Somebody else is kicking this group
            if (removalKicksConngroup(mRequest.getPlayerRemovedReason()))
            {
                // Fill up the session ids so that all players from userGroupId are kicked.
                if (gUserSetManager->getSessionIds(userGroupId, masterRequest.getSessionIdList()) != Blaze::ERR_OK)
                {                        
                    return GAMEMANAGER_ERR_INVALID_GROUP_ID;
                }

                // If the removal reason is a ban, set up the account id of the player being banned. 
                if (isRemovalReasonABan(mRequest.getPlayerRemovedReason()))
                {
                    //lookup the account id of the banned user (user can be offline), and this operation can hit the db (and block)
                    UserInfoPtr bannedUserInfo;
                    if (gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getPlayerId(), bannedUserInfo) != Blaze::ERR_OK)
                    {
                        WARN_LOG("[LeaveGameByGroup] User info not found for banned PlayerId(" << mRequest.getPlayerId() << "), GameId(" << mRequest.getGameId() << ")");
                        return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
                    }
                    masterRequest.setBanAccountId(bannedUserInfo->getPlatformInfo().getEaIds().getNucleusAccountId());
                }
            }
            else
            {
                // The group is leaving voluntarily. Validate group existence & membership of leader
                BlazeRpcError blazeErr = ValidationUtils::validateUserGroup(UserSession::getCurrentUserSessionId(), userGroupId, masterRequest.getSessionIdList());
                if (blazeErr != Blaze::ERR_OK)
                {
                    return commandErrorFromBlazeError( blazeErr );
                }
                //validateUserGroup does not put the caller in the list. We explicitly push him here.
                masterRequest.getSessionIdList().push_back(UserSession::getCurrentUserSessionId());
            }
            
            // pass the session list to master
            LeaveGameByGroupMasterResponse masterResponse;
            BlazeRpcError error = mComponent.getMaster()->leaveGameByGroupMaster(masterRequest, masterResponse);

            // We create a copy of external session params for the queued job, which later will be owned by GameManagerSlaveImpl::leaveExternalSession()
            LeaveGroupExternalSessionParametersPtr leaveGroupExternalSessonParams = BLAZE_NEW LeaveGroupExternalSessionParameters(masterResponse.getExternalSessionParams());                     

            if ((ERR_OK == error) && !leaveGroupExternalSessonParams->getExternalUserLeaveInfos().empty())
            {
                mComponent.getExternalSessionJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::leaveExternalSession, leaveGroupExternalSessonParams, "GameManagerSlaveImpl::leaveExternalSession");
            }

            return commandErrorFromBlazeError(error);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    //static creation factory method of command's stub class
    DEFINE_LEAVEGAMEBYGROUP_CREATE()

} // namespace GameManager
} // namespace Blaze
