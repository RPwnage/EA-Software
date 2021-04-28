/*! ************************************************************************************************/
/*!
    \file joingamebyuserlist_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "framework/usersessions/usersessionmanager.h"
#include "gamemanager/rpc/gamemanagerslave/joingamebyuserlist_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{

    class JoinGameByUserListCommand : public JoinGameByUserListCommandStub
    {
    public:

        JoinGameByUserListCommand(Message* message, JoinGameByUserListRequest *request, GameManagerSlaveImpl* componentImpl)
            : JoinGameByUserListCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~JoinGameByUserListCommand() override
        {
        }

        /************************************** Private methods *******************************************/
    private:

        JoinGameByUserListCommandStub::Errors execute() override
        {
            if (mRequest.getGameId() == INVALID_GAME_ID || gSliverManager->getSliver(mComponent.getMaster()->COMPONENT_ID, GetSliverIdFromSliverKey(mRequest.getGameId())) == nullptr)
            {
                return commandErrorFromBlazeError(GAMEMANAGER_ERR_INVALID_GAME_ID);
            }

            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            // Copy everything but the group join info:
            JoinGameByGroupMasterRequest mMasterRequest;
            mMasterRequest.setJoinLeader(false);
            mMasterRequest.getJoinRequest().setGameId(mRequest.getGameId());
            mMasterRequest.getJoinRequest().setJoinMethod(mRequest.getJoinMethod());
            mRequest.getPlayerJoinData().copyInto(mMasterRequest.getJoinRequest().getPlayerJoinData());
            mRequest.getCommonGameData().copyInto(mMasterRequest.getJoinRequest().getCommonGameData());

            // find and add each user to the reserved players list.
            eastl::string callerExtAuthToken;
            const char8_t* callerAuthToken = nullptr;
            if (ERR_OK == getCallerExternalAuthToken(callerExtAuthToken))
            {
                callerAuthToken = callerExtAuthToken.c_str();
            }
            
            UserIdentificationList groupUserIds; // Empty list == Noone was in a group.
            mComponent.populateExternalPlayersInfo(mMasterRequest.getJoinRequest().getPlayerJoinData(), 
                                                   groupUserIds,
                                                   mMasterRequest.getUsersInfo(), 
                                                   callerAuthToken);

            // find and add each user session id to session id list
            BlazeRpcError err = ValidationUtils::userIdListToJoiningInfoList(mRequest.getPlayerJoinData(), groupUserIds, mMasterRequest.getUsersInfo(), &mComponent.getExternalSessionUtilManager());
            if ((err != ERR_OK) || mMasterRequest.getUsersInfo().empty())
            {
                return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
            }


            //update reputation ued. side:
            GameManagerSlaveImpl::updateAndCheckUserReputations(mMasterRequest.getUsersInfo());

            //check join restrictions
            ExternalSessionIdentification extSessIdent;
            BlazeRpcError resErr = mComponent.checkExternalSessionJoinRestrictions(extSessIdent, mMasterRequest.getJoinRequest(), mMasterRequest.getUsersInfo());
            if (isExternalSessionJoinRestrictionError(resErr) || (resErr == GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR) || (resErr == GAMEMANAGER_ERR_INVALID_GAME_ID))
            {
                return commandErrorFromBlazeError(resErr);//other errs logged.
            }

            GetExternalDataSourceApiListRequest getExternalDataSourceRequest;
            getExternalDataSourceRequest.setGameId(mRequest.getGameId());
            GetExternalDataSourceApiListResponse getExternalDataSourceResponse;

            err = fixupPlayerJoinDataRoles(mMasterRequest.getJoinRequest().getPlayerJoinData(), true, mComponent.getLogCategory(), &mErrorResponse);
            if (err != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(err);
            }

            err = mComponent.getMaster()->getExternalDataSourceApiListByGameId(getExternalDataSourceRequest, getExternalDataSourceResponse);
            if ( err == ERR_OK )
            {
                if (!mComponent.getExternalDataManager().fetchAndPopulateExternalData(getExternalDataSourceResponse.getDataSourceNameList(), mMasterRequest.getJoinRequest()))
                {
                    WARN_LOG("[GameManagerJoinGameByUserListCommand].execute: Failed to fetch and populate external data");
                }
            }

            // pass both lists to master
            JoinGameMasterResponse masterResponse;
            masterResponse.setJoinResponse(mResponse);
            BlazeRpcError error = mComponent.getMaster()->joinGameByGroupMaster(mMasterRequest, masterResponse, mErrorResponse);
            if ((error == GAMEMANAGER_ERR_GAME_BUSY) && (mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() != 0))
            {
                TRACE_LOG("[GameManagerJoinGameByUserListCommand].execute: unable to join game " << mRequest.getGameId() << ", due to game being busy. Suspending join for up to " << mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() << "ms.");
                if (ERR_OK != Fiber::sleep(mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis(), "Sleeping GameManagerJoinGameByUserListCommand::execute"))
                    return ERR_SYSTEM;
                error = mComponent.getMaster()->joinGameByGroupMaster(mMasterRequest, masterResponse, mErrorResponse);
            }

            if (ERR_OK == error)
            {
                // Update to external session, the players which were updated by the blaze master call
                masterResponse.getExternalSessionParams().setGroupExternalSessionToken(getExternalAuthToken(mMasterRequest, masterResponse, callerAuthToken));

                ExternalSessionJoinInitError joinInitErr;
                error = mComponent.joinExternalSession(masterResponse.getExternalSessionParams(), mMasterRequest.getJoinLeader(), joinInitErr, mResponse.getExternalSessionIdentification());
            }
            return commandErrorFromBlazeError(error);
        }

    private:
        GameManagerSlaveImpl &mComponent;

        // For the group external session auth token, use the caller's if avail.
        const char8_t* getExternalAuthToken(const JoinGameByGroupMasterRequest& masterRequest, const JoinGameMasterResponse& masterResponse, const char8_t* callerAuthToken) const
        {
            if (callerAuthToken != nullptr)
                return callerAuthToken;

            if (masterResponse.getExternalSessionParams().getExternalUserJoinInfos().empty())
                return nullptr;//unneeded

            return masterResponse.getExternalSessionParams().getGroupExternalSessionToken();
        }

        BlazeRpcError getCallerExternalAuthToken(eastl::string& extToken) const
        {
            if (gCurrentLocalUserSession != nullptr)
            {
                UserIdentification userIdent;
                gCurrentLocalUserSession->getUserInfo().filloutUserIdentification(userIdent);
                return mComponent.getExternalSessionUtilManager().getAuthToken(userIdent, gCurrentLocalUserSession->getServiceName(), extToken);
            }
            return ERR_SYSTEM;
        }
    };


    //static creation factory method of command's stub class
    DEFINE_JOINGAMEBYUSERLIST_CREATE()

} //GameManager namespace
} // Blaze namespace
