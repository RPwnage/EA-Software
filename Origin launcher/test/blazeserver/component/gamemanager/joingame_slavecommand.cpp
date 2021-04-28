/*! ************************************************************************************************/
/*!
    \file joingame_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "gamemanager/rpc/gamemanagerslave/joingame_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/rolescommoninfo.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{

    class JoinGameCommand : public JoinGameCommandStub
    {
    public:

        JoinGameCommand(Message* message, JoinGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : JoinGameCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~JoinGameCommand() override
        {
        }

    private:

        JoinGameCommandStub::Errors execute() override
        {
            convertToPlatformInfo(mRequest.getUser());

            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());


            // check whether this client has permission to join game session or game group.
            // Specific join permission per game type is checked by the master.
            // We do a soft check to avoid extra warnings in the (unlikely) case of a game group join-only permission. 
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_SESSION, true) && 
                !UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_GROUP, true))
            {
                // Just do the same checks in non-silent mode, so that both permissions are noted in the logs:
                UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_SESSION);
                UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_GROUP);
                return JoinGameCommand::ERR_AUTHORIZATION_REQUIRED;
            }

            ExternalSessionIdentification externalSessionIdentification;//for join restrictions check
            bool requireSessionTemplateFetch = false;

            const GameManager::ExternalSessionUtil* util = mComponent.getExternalSessionUtilManager().getUtil(gCurrentLocalUserSession->getClientPlatform());
            if (util && !util->getConfig().getSessionTemplateNames().empty())
            {
                // initialize with the default template name for joining the game
                externalSessionIdentification.getXone().setTemplateName(util->getConfig().getSessionTemplateNames().front());
                requireSessionTemplateFetch = (util->getConfig().getSessionTemplateNames().size() > 1);
            }             

            // The master only knows how to join by game id, so if we're actually joining from a player,
            //  we lookup that player's game now and send a joinById to the master.            
            if (mRequest.getGameId() == INVALID_GAME_ID)
            {
                UserInfoPtr userInfo;
                BlazeRpcError userError = gUserSessionManager->lookupUserInfoByIdentification(mRequest.getUser(), userInfo);
                if (userError != ERR_OK)
                {
                     return commandErrorFromBlazeError(GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
                }

                //Cache off as part of join request if request specified a the target player in a different way.
                if (mRequest.getUser().getBlazeId() == INVALID_BLAZE_ID)
                {
                    mRequest.getUser().setBlazeId(userInfo->getId());
                }

                //Find out from the search slaves where the player has a game
                Blaze::Search::GetGamesRequest ggReq;
                Blaze::Search::GetGamesResponse ggResp;
                ggReq.getBlazeIds().push_back(userInfo->getId());
                ggReq.setGameProtocolVersionString(mRequest.getCommonGameData().getGameProtocolVersionString());
                ggReq.setErrorIfGameProtocolVersionStringsMismatch(true);
                ggReq.setFetchOnlyOne(true);
                ggReq.setResponseType(requireSessionTemplateFetch ? Blaze::Search::GET_GAMES_RESPONSE_TYPE_FULL : Blaze::Search::GET_GAMES_RESPONSE_TYPE_ID);
                ggReq.getGameTypeList().push_back(mRequest.getCommonGameData().getGameType());

                BlazeRpcError lookupUserGameError = mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp);
                if (lookupUserGameError == ERR_OK)
                {
                    if (!ggResp.getGameIdList().empty())
                    {
                        mRequest.setGameId(ggResp.getGameIdList().front());
                    }
                    else if (requireSessionTemplateFetch && !ggResp.getFullGameData().empty())
                    {
                        mRequest.setGameId(ggResp.getFullGameData().front()->getGame().getGameId());
                        ggResp.getFullGameData().front()->getGame().getExternalSessionIdentification().copyInto(externalSessionIdentification);
                    }
                }
                else if (lookupUserGameError == GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH)
                {
                    return JoinGameCommand::GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH;
                }

                // If we weren't able to find a game (for any other reason):
                if (mRequest.getGameId() == INVALID_GAME_ID)
                {
                    return commandErrorFromBlazeError(GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
                }
            }
            else if (gSliverManager->getSliver(mComponent.getMaster()->COMPONENT_ID, GetSliverIdFromSliverKey(mRequest.getGameId())) == nullptr)
            {
                return commandErrorFromBlazeError(GAMEMANAGER_ERR_INVALID_GAME_ID);
            }
            else if (requireSessionTemplateFetch)
            {
                //Find out from the search slaves where the player has a game
                if (mRequest.getPlayerJoinData().getDefaultTeamIndex() == TARGET_USER_TEAM_INDEX)
                    return commandErrorFromBlazeError(GAMEMANAGER_ERR_TEAM_NOT_ALLOWED);

                BlazeRpcError lookupUserGameError = mComponent.fetchExternalSessionDataFromSearchSlaves(mRequest.getGameId(), externalSessionIdentification);
                if (lookupUserGameError != ERR_OK)
                {
                    return commandErrorFromBlazeError(lookupUserGameError);
                }
            }
            else if (mRequest.getPlayerJoinData().getDefaultTeamIndex() == TARGET_USER_TEAM_INDEX)
            {
                return commandErrorFromBlazeError(GAMEMANAGER_ERR_TEAM_NOT_ALLOWED);
            }

            if (mRequest.getPlayerJoinData().getPlayerDataList().size() > 1 || (mRequest.getPlayerJoinData().getGroupId() != EA::TDF::OBJECT_ID_INVALID))
            {
                JoinGameByGroupMasterRequest masterRequest;
                mRequest.copyInto(masterRequest.getJoinRequest());

                ValidationUtils::insertHostSessionToPlayerJoinData(masterRequest.getJoinRequest().getPlayerJoinData(), gCurrentLocalUserSession);

                UserIdentificationList groupUserIds;
                // retrieve user session list/player id list from the usersSetManager  
                BlazeRpcError err = ValidationUtils::loadGroupUsersIntoPlayerJoinData(masterRequest.getJoinRequest().getPlayerJoinData(), groupUserIds, mComponent);
                if (err != Blaze::ERR_OK)
                {
                    TRACE_LOG("[GameManagerJoinGameCommand].execute: getSessionIds failed with error " 
                        << (ErrorHelp::getErrorName(err)));
                    return commandErrorFromBlazeError(err);
                }

                err = fixupPlayerJoinDataRoles(masterRequest.getJoinRequest().getPlayerJoinData(), true, mComponent.getLogCategory(), &mErrorResponse);
                if (err != Blaze::ERR_OK)
                {
                    return commandErrorFromBlazeError(err);
                }

                GetExternalDataSourceApiListRequest getExternalDataSourceRequest;
                getExternalDataSourceRequest.setGameId(mRequest.getGameId());
                GetExternalDataSourceApiListResponse getExternalDataSourceResponse;

                err = mComponent.getMaster()->getExternalDataSourceApiListByGameId(getExternalDataSourceRequest, getExternalDataSourceResponse);
                if ( err == ERR_OK )
                {
                    if (!mComponent.getExternalDataManager().fetchAndPopulateExternalData(getExternalDataSourceResponse.getDataSourceNameList(), masterRequest.getJoinRequest()))
                    {
                        WARN_LOG("[GameManagerJoinGameCommand].execute: Failed to fetch and populate external data");
                    }
                }

                // If there are more people in the PJD than the group, it indicates that some were loose players (as from a user list)
                if (masterRequest.getJoinRequest().getPlayerJoinData().getPlayerDataList().size() > groupUserIds.size())
                {
                    //check whether this client has the permission to join game
                    if (!gUserSessionManager->isSessionAuthorized(UserSession::getCurrentUserSessionId(), Authorization::PERMISSION_JOIN_GAME_BY_USERLIST))
                    {
                        return JoinGameCommand::ERR_AUTHORIZATION_REQUIRED;
                    }
                }

                // find and add each user session id to session id list (Pass in the master join data so that it can be updated if duplicates exist)
                err = ValidationUtils::userIdListToJoiningInfoList(masterRequest.getJoinRequest().getPlayerJoinData(), groupUserIds,
                                                                                    masterRequest.getUsersInfo(),
                                                                                    &mComponent.getExternalSessionUtilManager());
                if (err != Blaze::ERR_OK)
                {
                    return commandErrorFromBlazeError(err);
                }

                UserSessionInfo* leaderInfo = getHostSessionInfo(masterRequest.getUsersInfo());
                //add players from the reservedExternalPlayers list (Pass in the master join data so that it can be updated if duplicates exist)
                mComponent.populateExternalPlayersInfo( masterRequest.getJoinRequest().getPlayerJoinData(), 
                                                        groupUserIds,
                                                        masterRequest.getUsersInfo(), 
                                                        leaderInfo ? leaderInfo->getExternalAuthInfo().getCachedExternalSessionToken() : nullptr);

                //check join restrictions
                BlazeRpcError resErr = mComponent.checkExternalSessionJoinRestrictions(externalSessionIdentification, masterRequest.getJoinRequest(), masterRequest.getUsersInfo());
                if (isExternalSessionJoinRestrictionError(resErr) || (resErr == GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR) || (resErr == GAMEMANAGER_ERR_INVALID_GAME_ID)) //Also bail on GAMEMANAGER_ERR_INVALID_GAME_ID just in case game n/a on slaves but actually avail on master, to avoid potential bypassing join restrictions
                {
                    return commandErrorFromBlazeError(resErr);//other errs logged.
                }

                //update reputation ued. side:
                GameManagerSlaveImpl::updateAndCheckUserReputations(masterRequest.getUsersInfo());
                
                // The rest of the group will join later, but for the leader we need to get the latest UED value:
                bool hasBadRep = ReputationService::ReputationServiceUtil::userHasPoorReputation(gCurrentLocalUserSession->getExtendedData());
                if (leaderInfo)
                    leaderInfo->setHasBadReputation(hasBadRep);
                TRACE_LOG("[GameManagerJoinGameCommand].execute() : Setting leader HasBadReputation- " << hasBadRep);

                // if joining the game's first user with external sessions access (e.g. pre-host-inject games), it'll init game's correlation id on master.
                BlazeRpcError sessErr = initRequestExternalSessionData(masterRequest.getExternalSessionData(), externalSessionIdentification, mRequest.getCommonGameData().getGameType(),
                    masterRequest.getUsersInfo(), mComponent.getConfig().getGameSession(), mComponent.getExternalSessionUtilManager(), mComponent);
                if (sessErr != ERR_OK && masterRequest.getExternalSessionData().getJoinInitErr().getFailedOnAllPlatforms())
                {
                    return commandErrorFromBlazeError(sessErr);
                }

                JoinGameMasterResponse masterResponse;
                masterResponse.setJoinResponse(mResponse);

                JoinGameCommandStub::Errors masterErr = commandErrorFromBlazeError(mComponent.getMaster()->joinGameByGroupMaster(masterRequest, masterResponse, mErrorResponse));
                if ((masterErr == GAMEMANAGER_ERR_GAME_BUSY) && (mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() != 0))
                {
                    TRACE_LOG("[GameManagerJoinGameCommand].execute() : player " << mRequest.getUser().getBlazeId() << "unable to join game " << mRequest.getGameId() << ", due to game being busy. Suspending join for up to " << mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() << "ms.");
                    if (ERR_OK != Fiber::sleep(mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy(), "Sleeping GameManagerJoinGameCommand::execute"))
                        return ERR_SYSTEM;
                    masterErr = commandErrorFromBlazeError(mComponent.getMaster()->joinGameByGroupMaster(masterRequest, masterResponse, mErrorResponse));
                }

                if (masterErr != JoinGameCommandStub::ERR_OK)
                {
                    SPAM_LOG("[GameManagerJoinGameCommand].execute() : player " << mRequest.getUser().getBlazeId() 
                            << "unable to join game " << mRequest.getGameId() << ", error " << (ErrorHelp::getErrorName(masterErr)) << " (" << masterErr << ").");
                    return masterErr;
                }

                // Update to external session, the players which were updated by the blaze master call
                return commandErrorFromBlazeError(mComponent.joinExternalSession(masterResponse.getExternalSessionParams(), true, masterRequest.getExternalSessionData().getJoinInitErr(), mResponse.getExternalSessionIdentification()));
            }

            // Join game - no lists.
            JoinGameMasterRequest masterRequest;
            masterRequest.setJoinRequest(mRequest);
            BlazeRpcError setErr = ValidationUtils::setJoinUserInfoFromSession(masterRequest.getUserJoinInfo().getUser(), *gCurrentUserSession, &mComponent.getExternalSessionUtilManager());
            if (setErr != ERR_OK)
            {
                return commandErrorFromBlazeError(setErr);
            }

            //check join restrictions (we use a temp list here because the data is const)
            GameManager::UserJoinInfoList tempUserInfos;
            masterRequest.getUserJoinInfo().getUser().copyInto(tempUserInfos.pull_back()->getUser());
            BlazeRpcError resErr = mComponent.checkExternalSessionJoinRestrictions(externalSessionIdentification, masterRequest.getJoinRequest(), tempUserInfos);
            if (isExternalSessionJoinRestrictionError(resErr) || (resErr == GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR) || (resErr == GAMEMANAGER_ERR_INVALID_GAME_ID))
            {
                return commandErrorFromBlazeError(resErr);//other errs logged.
            }

            GetExternalDataSourceApiListRequest getExternalDataSourceRequest;
            getExternalDataSourceRequest.setGameId(mRequest.getGameId());
            GetExternalDataSourceApiListResponse getExternalDataSourceResponse;

            resErr = mComponent.getMaster()->getExternalDataSourceApiListByGameId(getExternalDataSourceRequest, getExternalDataSourceResponse);
            if ( resErr == ERR_OK )
            {
                if (!mComponent.getExternalDataManager().fetchAndPopulateExternalData(getExternalDataSourceResponse.getDataSourceNameList(), masterRequest.getJoinRequest()))
                {
                    WARN_LOG("[GameManagerJoinGameCommand].execute: Failed to fetch and populate external data");
                }
            }  

            GameManagerSlaveImpl::updateAndCheckUserReputations(masterRequest.getUserJoinInfo().getUser());
            bool hasBadRep = ReputationService::ReputationServiceUtil::userHasPoorReputation(gCurrentLocalUserSession->getExtendedData());
            masterRequest.getUserJoinInfo().getUser().setHasBadReputation(hasBadRep);
            TRACE_LOG("[GameManagerJoinGameCommand].execute() : Setting user HasBadReputation- " << hasBadRep);

            // if joining the game's first user with external sessions access (e.g. pre-host-inject games), it'll init game's correlation id on master.
            UserJoinInfoList joinList;
            masterRequest.getUserJoinInfo().copyInto(*(joinList.pull_back()));
            BlazeRpcError sessErr = initRequestExternalSessionData(masterRequest.getExternalSessionData(), externalSessionIdentification, mRequest.getCommonGameData().getGameType(),
                joinList, mComponent.getConfig().getGameSession(), mComponent.getExternalSessionUtilManager(), mComponent);
            if (sessErr != ERR_OK && masterRequest.getExternalSessionData().getJoinInitErr().getFailedOnAllPlatforms())
            {
                return commandErrorFromBlazeError(sessErr);
            }

            JoinGameMasterResponse masterResponse;
            masterResponse.setJoinResponse(mResponse);

            Blaze::BlazeRpcError masterErr = mComponent.getMaster()->joinGameMaster(masterRequest, masterResponse, mErrorResponse);
            if ((masterErr == GAMEMANAGER_ERR_GAME_BUSY) && (mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() != 0))
            {
                TRACE_LOG("[GameManagerJoinGameCommand].execute() : player " << mRequest.getUser().getBlazeId() << " unable to join game " << mRequest.getGameId() << ", due to game being busy. Suspending join for up to " << mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy().getMillis() << "ms.");
                if (ERR_OK != Fiber::sleep(mComponent.getConfig().getGameSession().getJoinWaitTimeOnBusy(), "Sleeping GameManagerJoinGameCommand::execute"))
                    return ERR_SYSTEM;
                masterErr = commandErrorFromBlazeError(mComponent.getMaster()->joinGameMaster(masterRequest, masterResponse, mErrorResponse));
            }

            if (masterErr != Blaze::ERR_OK)
            {

                SPAM_LOG("[GameManagerJoinGameCommand].execute() : player " << mRequest.getUser() 
                    << " unable to join game " << mRequest.getGameId() << ", error " << (ErrorHelp::getErrorName(masterErr)) 
                    << " (" << masterErr << ").");

                return (commandErrorFromBlazeError(masterErr));
            }
            
            // Update to external session, the players which were updated by the blaze master call
            return commandErrorFromBlazeError(mComponent.joinExternalSession(masterResponse.getExternalSessionParams(), true, masterRequest.getExternalSessionData().getJoinInitErr(), mResponse.getExternalSessionIdentification()));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_JOINGAME_CREATE()

} // namespace GameManager
} // namespace Blaze
