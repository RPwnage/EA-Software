/*! ************************************************************************************************/
/*!
    \file gamemanagervalidationutils.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "gamemanager/commoninfo.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/userset/userset.h"
#include "gamemanager/gamemanagermasterimpl.h"


namespace Blaze
{
namespace GameManager
{
namespace ValidationUtils
{

    /*! ************************************************************************************************/
    /*! \brief validate a userGroup for a "group RPC".  validates that the userGroup exists and that the calling user
                is a member of the group.  On success, populates the userSessionIdList with the group members (except the caller).

        \param[in] callingUserSessionId - the UserSessionId of the person calling the group RPC
        \param[in] userGroupId
        \param[out] userSessionIdList on success, we populate this with the session ids of the user group members except the caller.
    ***************************************************************************************************/
    BlazeRpcError validateUserGroup(UserSessionId callingUserSessionId, const UserGroupId& userGroupId, UserSessionIdList &userSessionIdList)
    {
        // Session Id list in the create request is not exposed to the client, so should not be populated unless by us.
        EA_ASSERT(userSessionIdList.empty());

        // retrieve user session list/player id list from the usersSetManager
        if ( gUserSetManager->getSessionIds(userGroupId, userSessionIdList) != ERR_OK)
        {
            return GAMEMANAGER_ERR_INVALID_GROUP_ID;
        }

        // validate that the caller is a member of the group they're taking action on.
        for (UserSessionIdList::iterator iter = userSessionIdList.begin(), end = userSessionIdList.end(); iter != end; ++iter )
        {
            if (*iter == callingUserSessionId)
            {
                // We remove the calling user as they are joined immediately to the game and do not need
                // a reservation.
                userSessionIdList.erase(iter);
                return ERR_OK;
            }
        }

        return GAMEMANAGER_ERR_PLAYER_NOT_IN_GROUP;
    }



    BlazeRpcError setJoinUserInfoFromSession(UserSessionInfo& userInfo, const UserSessionExistence& joiningSession, const ExternalSessionUtilManager* extSessionUtilManager)
    {
        UserSessionDataPtr userSessionData;
        BlazeRpcError err = joiningSession.fetchSessionData(userSessionData);
        if ((err != ERR_OK) || (userSessionData == nullptr))
        {
            WARN_LOG("[setJoinUserInfoFromSession]: fetchSessionData for UserSessionId (" << joiningSession.getUserSessionId() << ") failed to return data with err(" << LOG_ENAME(err) << ")");
            return err;
        }

        UserInfoPtr userInfoData;
        err = joiningSession.fetchUserInfo(userInfoData);
        if (err == USER_ERR_USER_NOT_FOUND)
        {
            // If the UserSession exists at all, but it has no associated UserInfo, then it is a UserSession created via trusted login and was assigned
            // a non-Nucleus based BlazeId.  This type of user can still perform GameManager related activity, however, its ability to do certain things may
            // be limited at different levels/locations.
            err = ERR_OK;
        }
        else if ((err != ERR_OK) || (userInfoData == nullptr))
        {
            WARN_LOG("[setJoinUserInfoFromSession]: fetchUserInfo for UserSessionId (" << joiningSession.getUserSessionId() << ") failed to return data with err(" << LOG_ENAME(err) << ").");
            return err;
        }

        UserIdentification ident; //cache off before any blocking calls below
        if (userInfoData != nullptr)
            userInfoData->filloutUserIdentification(ident);

        userInfo.setSessionId(joiningSession.getUserSessionId());

        // We need to override the blaze id with the UserSessions UserId, which may be a psuedo (or stateless) BlazeId.
        // So first, copy the whole thing (if we have any UserInfo), then set the id specifically.
        if (userInfoData != nullptr)
            userInfoData->copyInto(userInfo.getUserInfo());
        else
        {
            // For Trusted Login Blaze Servers, try to fill out the UserInfo from other sources:
            // We list out all of the parameters below, since in theory everything should be set, even though not all of the information
            //  is available for users without a session.   (Members that can't be set accurately are just left commented out, 
            //  in case the data becomes available in the future.)

            userInfo.getUserInfo().setPersonaName(joiningSession.getPersonaName());
            userInfo.getUserInfo().setPersonaNamespace(joiningSession.getPersonaNamespace());
            joiningSession.getPlatformInfo().copyInto(userInfo.getUserInfo().getPlatformInfo());
            userInfo.getUserInfo().setAccountLocale(joiningSession.getAccountLocale());
            userInfo.getUserInfo().setAccountCountry(joiningSession.getAccountCountry());
            userInfo.getUserInfo().setUserInfoAttribute(userSessionData->getExtendedData().getUserInfoAttribute());
            //userInfo.getUserInfo().setStatus();
            //userInfo.getUserInfo().setFirstLoginDateTime();
            //userInfo.getUserInfo().setLastLoginDateTime();
            //userInfo.getUserInfo().setPreviousLoginDateTime();
            //userInfo.getUserInfo().setLastLogoutDateTime();
            //userInfo.getUserInfo().setLastLocaleUsed();
            //userInfo.getUserInfo().setLastCountryUsed();
            //userInfo.getUserInfo().setLastAuthErrorCode();
            //userInfo.getUserInfo().setIsChildAccount();
            userInfo.getUserInfo().setCrossPlatformOptIn(userSessionData->getExtendedData().getCrossPlatformOptIn());
            //userInfo.getUserInfo().setStopProcess();
            //userInfo.getUserInfo().setOriginPersonaId();
            //userInfo.getUserInfo().setPidId();
        }
        userInfo.getUserInfo().setId(joiningSession.getBlazeId());

        userSessionData->getExtendedData().getDataMap().copyInto(userInfo.getDataMap());
        userSessionData->getClientMetrics().copyInto(userInfo.getClientMetrics());
        userSessionData->getClientUserMetrics().copyInto(userInfo.getClientUserMetrics());
        userInfo.setClientType(joiningSession.getClientType());
        userInfo.setServiceName(joiningSession.getServiceName());
        userInfo.setProductName(joiningSession.getProductName());
        userInfo.setHasHostPermission(gUserSessionManager->isSessionAuthorized(joiningSession.getUserSessionId(), Blaze::Authorization::PERMISSION_HOST_GAME_SESSION, true));
        userInfo.setConnectionGroupId(userSessionData->getConnectionGroupId());
        userInfo.setConnectionAddr(userSessionData->getConnectionAddr());
        userSessionData->getExtendedData().getAddress().copyInto(userInfo.getNetworkAddress());
        userSessionData->getExtendedData().getQosData().copyInto(userInfo.getQosData());
        userInfo.setBestPingSiteAlias(userSessionData->getExtendedData().getBestPingSiteAlias());
        userSessionData->getExtendedData().getLatencyMap().copyInto(userInfo.getLatencyMap());
        userInfo.setLongitude(userSessionData->getLongitude());
        userInfo.setLatitude(userSessionData->getLatitude());
        userInfo.setCountry(userSessionData->getExtendedData().getCountry());
        userInfo.setTimeZone(userSessionData->getExtendedData().getTimeZone());
        userInfo.setISP(userSessionData->getExtendedData().getISP());
        // userInfo.setHasBadReputation()  (Set below)
        // userInfo.setExternalAuthInfo() (set below)
        userInfo.setHasExternalSessionJoinPermission(ExternalSessionUtil::hasJoinExternalSessionPermission(joiningSession.getExistenceData()));
        // userInfo.setGroupSize()
        userInfo.setDirtySockUserIndex(userSessionData->getDirtySockUserIndex());
        userInfo.setUUID(userSessionData->getUUID());

        // TODO_MC: Why is this 'if connection exists' needed?  The connection would only be non-nullptr if the UserSession
        //          is *owned* by the current instance AND is a persistent connection.
        //if (joiningSession.getConnection())
        {
            ConnectUtil::setExternalAddr(userInfo.getNetworkAddress(), userSessionData->getConnectionAddr());
        }

        // potential blocking calls below
        userInfo.setHasBadReputation(ReputationService::ReputationServiceUtil::userHasPoorReputation(userSessionData->getExtendedData()));

        if (userInfo.getHasExternalSessionJoinPermission() && extSessionUtilManager != nullptr)
        {
            eastl::string buf;
            extSessionUtilManager->getAuthToken(ident, joiningSession.getServiceName(), buf);
            userInfo.getExternalAuthInfo().setServiceName(joiningSession.getServiceName());
            
            // If we can't get an access token, it is not fatal yet. We may fail later, specifically on the platform for which we failed to get this token and actually tried to use it. 
            if (!buf.empty())
                userInfo.getExternalAuthInfo().setCachedExternalSessionToken(buf.c_str());

        }
        return err;
    }

    void setJoinUserInfoFromSessionExistence(UserSessionInfo& userInfo, UserSessionId userSessionId,
        const BlazeId newBlazeId, const PlatformInfo& platformInfo, const Locale newAccountLocale, const uint32_t newAccountCountry, const char8_t* serviceName, const char8_t* productName)
    {
        userInfo.setSessionId(userSessionId);
        platformInfo.copyInto(userInfo.getUserInfo().getPlatformInfo());
        userInfo.getUserInfo().setAccountLocale(newAccountLocale);
        userInfo.getUserInfo().setAccountCountry(newAccountCountry);
        userInfo.getUserInfo().setId(newBlazeId);
        userInfo.setServiceName(serviceName);
        userInfo.setProductName(productName);
    }

    bool isUserSessionInList(UserSessionId userSessionId, const UserJoinInfoList& joiningUserInfoList, const UserJoinInfo** outSelfInfo)
    {
        for (auto& curIter : joiningUserInfoList)
        {
           if ((curIter)->getUser().getSessionId() == userSessionId)
           {
               if (outSelfInfo)
               {
                   (*outSelfInfo) = (curIter);
               }
               return true;
           }
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief This function adds all the logged in users with BlazeIds to the players info list (or optional list).
               It does this by grabbing all PJD users with BlazeIds, trying to find their sessions,
               and adding them to the appropriate list.

        \param[in] playerJoinData - the joining players (all players should be in here)
        \param[in] groupUserIds - players that were in the joining game group, sets groupId
        \param[out] userInfos - list of joining players (with valid session info)
        \param[in] extSessionUtilManager - used to get external data about the session
    ***************************************************************************************************/
    BlazeRpcError userIdListToJoiningInfoList(PlayerJoinData& playerJoinData,
        const UserIdentificationList& groupUserIds,
        UserJoinInfoList& userInfos,
        const ExternalSessionUtilManager* extSessionUtilManager,
        bool autoHostAssign)
    {
        PerPlayerJoinDataList& userList = playerJoinData.getPlayerDataList();
        UserSessionIdList sessionIds;
        sessionIds.reserve(userList.size());

        // this set is used to enforce uniqueness on our session ids.
        eastl::set<UserSessionId> primarySessionIdSet;

        // retrieve user session list/player id list from the usersSetManager
        for (PerPlayerJoinDataList::iterator it=userList.begin(); it != userList.end(); )  // iterator may be erased in loop. (can't hold 'end' value either)
        {
            if ((*it)->getUser().getBlazeId() == INVALID_BLAZE_ID)  // isExternalUser
            {
                ++it;
                continue;
            }

            UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId((*it)->getUser().getBlazeId());
            if (primarySessionId != INVALID_USER_SESSION_ID)
            {
                if (primarySessionIdSet.insert(primarySessionId).second)
                {
                    // For caller, use its current/calling user session, to ensure that receives notifications for the join/game etc.
                    UserSessionId sessionIdToAdd = (((*it)->getUser().getBlazeId() == UserSession::getCurrentUserBlazeId())?
                        UserSession::getCurrentUserSessionId() : primarySessionId);

                    if (!isUserSessionInList(sessionIdToAdd, userInfos))
                    {
                        // unique session id, put it in the list
                        sessionIds.push_back(sessionIdToAdd);
                    }
                    ++it;
                }
                else
                {
                    // We should only delete entries if the player is in the user set
                    WARN_LOG("[userIdListToJoiningInfoList]: Skipping over duplicated primary UserSessionId (" << primarySessionId << ") belonging to user with BlazeID (" << (*it)->getUser().getBlazeId() << ").");
                    it = userList.erase(it);
                }
            }
            else
            {
                // Special case: Dedicated servers don't have a valid primary session id (they have multiple sessions), but they can still be the host:
                if ((*it)->getUser().getBlazeId() == UserSession::getCurrentUserBlazeId())
                {
                    UserJoinInfo* joinInfo = userInfos.pull_back();
                    joinInfo->setIsOptional(false);
                    joinInfo->setIsHost(true);
                    BlazeRpcError result = setJoinUserInfoFromSession(joinInfo->getUser(), *gCurrentUserSession, extSessionUtilManager);
                    if (result != ERR_OK)
                        return result;
                }

                ++it;
            }
        }

        if (sessionIds.size() != userList.size())
        {
            TRACE_LOG("[userIdListToJoiningInfoList]: Found " << sessionIds.size() << " joinable user sessions, but input playerIdList specified " << userList.size() << " items.");
        }

        bool hasSetHost = false;
        BlazeRpcError result = ERR_OK;
        const BlazeId callerBlazeId = UserSession::getCurrentUserBlazeId();
        for (UserSessionIdList::iterator itr = sessionIds.begin(), end = sessionIds.end(); itr != end; ++itr)
        {
            UserSessionPtr session = gUserSessionManager->getSession(*itr);
            if (session == nullptr)
                continue;

            BlazeId curBlazeId = session->getBlazeId();

            // Check if the player was an optional player or not:
            bool isOptional = false;
            for (PerPlayerJoinDataList::iterator it=userList.begin(), itEnd=userList.end(); it != itEnd; ++it)  // iterator may be erased in loop.
            {
                if ((*it)->getUser().getBlazeId() == curBlazeId)
                {
                    isOptional = ((*it)->getIsOptionalPlayer());
                    if ((*it)->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                        (*it)->setTeamIndex(playerJoinData.getDefaultTeamIndex());
                    break;
                }
            }

            UserJoinInfo* joinInfo = userInfos.pull_back();
            joinInfo->setIsOptional(isOptional);
            if ((callerBlazeId == curBlazeId) ||
                ((isOptional == false) && (hasSetHost == false) && (autoHostAssign == true)))
            {
                // We set the host in two cases, either it's the user that initiated matchmaking (normal use case)
                // or, if we're in an indirect matchmaking request, we just assign the first user to be a placeholder 'host'  (needed for various Matchmaking rules)
                joinInfo->setIsHost(true);
                hasSetHost = true;
            }

            BlazeRpcError err = setJoinUserInfoFromSession(joinInfo->getUser(), *session, extSessionUtilManager);
            result = (result != ERR_OK ? result : err);//use 1st

            // Check if the user was in the original group, set group id if it was:
            for (UserIdentificationList::const_iterator it=groupUserIds.begin(), itEnd=groupUserIds.end(); it != itEnd; ++it)  // iterator may be erased in loop.
            {
                if ((*it)->getBlazeId() == curBlazeId)
                {
                    joinInfo->setUserGroupId(playerJoinData.getGroupId());
                    break;
                }
            }
        }

        return result;
    }

    // Returns bool true if a new player was added (false if already existing in data)
    bool insertGroupUserIdentificationToPlayerJoinData(PlayerJoinData& playerJoinData, const UserIdentification& userId, bool wasReserved)
    {
        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            // If player is found, we don't need to add anything:
            if (areUserIdsEqual((*playerIter)->getUser(), userId))
            {
                // Players in the group are not allowed to be optional.
                (*playerIter)->setIsOptionalPlayer(wasReserved);
                return false;
            }
        }

        PerPlayerJoinDataPtr joinData = playerJoinData.getPlayerDataList().pull_back();
        userId.copyInto(joinData->getUser());
        joinData->setIsOptionalPlayer(wasReserved);
        return true;
    }

    void insertHostSessionToPlayerJoinData(PlayerJoinData& playerJoinData, const UserSessionMaster* session)
    {
        if (session == nullptr)
            return;

        UserIdentification userId;
        session->getUserInfo().filloutUserIdentification(userId);

        PerPlayerJoinDataList::const_iterator playerIter = playerJoinData.getPlayerDataList().begin();
        PerPlayerJoinDataList::const_iterator playerEnd = playerJoinData.getPlayerDataList().end();
        for (; playerIter != playerEnd; ++playerIter)
        {
            // If player is found, we don't need to add anything:
            if (areUserIdsEqual((*playerIter)->getUser(), userId))
            {
                // Players in the group are not allowed to be optional.
                (*playerIter)->setIsOptionalPlayer(false);
                return;
            }
        }

        PerPlayerJoinDataPtr joinData = playerJoinData.getPlayerDataList().pull_back();
        userId.copyInto(joinData->getUser());
    }


    /*! ************************************************************************************************/
    /*! \brief  This function adds all the users in the PJD's GroupId into the PlayerJoinData lists,
                and tracks their user ids.

        \param[in\out] playerJoinData - the joining players, this function adds the guys from the group into the list.
        \param[out] userIds - players that were in the joining game group
    ***************************************************************************************************/
    BlazeRpcError loadGroupUsersIntoPlayerJoinData(PlayerJoinData& playerJoinData, UserIdentificationList& userIds, GameManagerSlaveImpl& component)
    {
        const UserGroupId& userGroupId = playerJoinData.getGroupId();
        if (userGroupId == EA::TDF::OBJECT_ID_INVALID)
        {
            // If no group was provided, just early out (no harm, no foul)
            return ERR_OK;
        }

        // Get the data directly from the game group if possible:
        eastl::list<PlayerState> playerStateList;
        BlazeRpcError result = ERR_OK;
        if ((userGroupId.type == ENTITY_TYPE_GAME) || (userGroupId.type == ENTITY_TYPE_GAME_GROUP))
        {
            Search::GetGamesResponse resp;
            result = component.getFullGameDataForBlazeObjectId(userGroupId, resp);
            if (result != ERR_OK)
            {
                return result;
            }

            if (!resp.getFullGameData().empty())
            {
                ListGameData& gameData = *resp.getFullGameData().front();
                for (ReplicatedGamePlayerList::const_iterator itr = gameData.getGameRoster().begin(), end = gameData.getGameRoster().end(); itr != end; ++itr)
                {
                    UserIdentificationPtr userIdent = userIds.pull_back();
                    (*itr)->getPlatformInfo().copyInto(userIdent->getPlatformInfo());
                    userIdent->setBlazeId((*itr)->getPlayerId());
                    userIdent->setExternalId(getExternalIdFromPlatformInfo(userIdent->getPlatformInfo()));
                    userIdent->setName((*itr)->getPlayerName());
                    userIdent->setAccountLocale((*itr)->getAccountLocale());
                    userIdent->setAccountCountry((*itr)->getAccountCountry());
                    userIdent->setPersonaNamespace((*itr)->getPersonaNamespace());
                    playerStateList.push_back((*itr)->getPlayerState());      // <--- This is the extra data that's not part of the userIds, that indicates if a player is external/reserved.
                }
            }
        }
        else
        {
            // retrieve user session list/player id list from the usersSetManager
            if (gUserSetManager->getUserIds(userGroupId, userIds) != ERR_OK)
            {
                return GAMEMANAGER_ERR_INVALID_GROUP_ID;
            }
        }

        eastl::list<PlayerState>::const_iterator playerStateListIter = playerStateList.begin();
        for (UserIdentificationList::const_iterator itr = userIds.begin(), end = userIds.end(); itr != end; ++itr)
        {
            // Basically, I need to add a special case for the reserved members (so they stay optional)
            bool wasReserved = false;
            if (userGroupId.type == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP ||
                userGroupId.type == Blaze::GameManager::ENTITY_TYPE_GAME)
            {
                wasReserved = ((*playerStateListIter) == Blaze::GameManager::RESERVED);
                ++playerStateListIter;
            }

            insertGroupUserIdentificationToPlayerJoinData(playerJoinData, **itr, wasReserved);
        }

        // validate that the caller is a member of the group they're taking action on.
        bool found = false;
        if (UserSession::getCurrentUserBlazeId() != INVALID_BLAZE_ID)
        {
            for (UserIdentificationList::iterator iter = userIds.begin(), end = userIds.end(); !found && iter != end; ++iter)
            {
                if ((*iter)->getBlazeId() == UserSession::getCurrentUserBlazeId())
                {
                    found = true;
                }
            }
        }

        //bail if we somehow got a group the requesting user doesn't belong to
        if (!found)
        {
            return GAMEMANAGER_ERR_PLAYER_NOT_IN_GROUP;
        }

        return ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief validate the game's capacity vs # of joining players and their joiningSlotType.  Returns ERR_OK on success.
        \param[in] joiningPlayerCount - the number of players joining the game as a result of this action
        \param[in] createGameRequest - the createGameRequest information to validate.
    ***************************************************************************************************/
    BlazeRpcError validateGameCapacity(const SlotTypeSizeMap& joiningPlayerMap, const CreateGameRequest& createGameRequest)
    {
        // early out if no overall participant slot capacity
        if ( (createGameRequest.getSlotCapacities().size() != MAX_SLOT_TYPE) || 
            ((createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) <= 0) && 
            (createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) <= 0)) )
        {
            // we don't allow for spectator-only games, so we're not going to check those sizes here
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO;
        }
            
        // Check each set of joining player slots:
        SlotTypeSizeMap::const_iterator curPlayerSize = joiningPlayerMap.begin();
        SlotTypeSizeMap::const_iterator endPlayerSize = joiningPlayerMap.end();
        for (; curPlayerSize != endPlayerSize; ++curPlayerSize)
        {
            SlotType joiningSlotType = curPlayerSize->first;
            uint16_t joiningPlayerCount = curPlayerSize->second;
    
            // validate joining slot type is valid
            if (!isParticipantSlot(joiningSlotType) && 
                !isSpectatorSlot(joiningSlotType))
            {
                // invalid slot type
                ERR_LOG("[GameManagerValidationUtils].validateGameCapacity unknown value encountered for player joiningSlotType: "
                        << joiningSlotType);
                return ERR_SYSTEM;
            }

            // Note: Dedicated server hosts do not join the created game, and do not create with group members,
            // their session id list should be empty.  But a resetting user's session id list may not be (both use createGameRequest)

            // validate capacity of the number of people in my session id list (usergroup) as
            // well as the number of slots i want reserved.
            uint32_t availableSlots = createGameRequest.getSlotCapacities().at(joiningSlotType);
            if (availableSlots < joiningPlayerCount)
            {
                int32_t playerCountDifference = joiningPlayerCount - availableSlots;

                // If we wanted private, we could still take a public slot, if there is room.
                if ((joiningSlotType == SLOT_PRIVATE_PARTICIPANT && createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) >= playerCountDifference) ||
                    (joiningSlotType == SLOT_PRIVATE_SPECTATOR && createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) >= playerCountDifference))
                {
                    continue;
                }
                else
                {
                    TRACE_LOG("[GameSessionMaster] Unable to create game with " << createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT)
                              << " public participant and " << createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT)
                              << " private participant slots, " << createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR)
                              << " public spectator and " << createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR)
                              << " private spectator slots, not enough room for host of type " << SlotTypeToString(joiningSlotType)
                              << " to join");
                    return GAMEMANAGER_ERR_GAME_CAPACITY_TOO_SMALL;
                }
            }
        }
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief validate the game's capacity vs # of joining players and their joiningSlotType.  Returns ERR_OK on success.
                should only be used to validate the capacity on a create game or reset dedicated server request.

    \param[in] joiningPlayerCount - the number of players joining the game as a result of this action
    \param[in] createGameRequest - the request parameters to validate
    ***************************************************************************************************/
    BlazeRpcError validateTeamCapacity(const SlotTypeSizeMap& joiningPlayerMap, const CreateGameRequest& createGameRequest, bool isVirtualWithReservation)
    {
        // If creator specified no teams, teams will be disabled for the created game.
        if (createGameRequest.getGameCreationData().getTeamIds().empty())
        {
            TRACE_LOG("[GameManagerValidationUtils].validateTeamCapacity: Unable to create game, empty TeamIdVector not allowed.");
            return GAMEMANAGER_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE;
        }

        // Validate the team vector and duplicate team ids.
        eastl::hash_map<TeamId, TeamIndex> duplicateTeamMap(BlazeStlAllocator("GameManagerMasterImpl::duplicateTeamMap", GameManagerSlave::COMPONENT_MEMORY_GROUP));;
        const TeamIdVector &teamIdVector = createGameRequest.getGameCreationData().getTeamIds();
        TeamIdVector::const_iterator iter = teamIdVector.begin();

        for (TeamIndex i = 0; iter != teamIdVector.end(); ++iter, ++i)
        {
            TeamId teamId = *iter;
            // Validate game setting flag vs vector
            if (!createGameRequest.getGameCreationData().getGameSettings().getAllowSameTeamId() && teamId != ANY_TEAM_ID)
            {
                if (duplicateTeamMap.insert(eastl::make_pair(teamId, i)).second == false)
                {
                    TRACE_LOG("[GameManagerValidationUtils].validateTeamCapacity: Unable to create game, duplicate teamId(" << teamId
                              << ") in capacity vector not allowed by game settings.");
                    return GAMEMANAGER_ERR_DUPLICATE_TEAM_CAPACITY;
                }
            }
        }

        // Return early for dedicated servers hosts, who don't join the game.
        // All subsequent checks should only be comparing the joining players.
        if (joiningPlayerMap.empty())
        {
            return ERR_OK;
        }

        // Creator needs to specify which team they want to join, and that team has to exist in the game.
        if (createGameRequest.getPlayerJoinData().getPlayerDataList().empty())
        {
            // Spectators do not specify a valid team index, so there's no need for this check:
            if (!isSpectatorSlot(createGameRequest.getPlayerJoinData().getDefaultSlotType()))
            {
                // Only use the default team index if no players specify one directly: 
                if ((createGameRequest.getPlayerJoinData().getDefaultTeamIndex() == UNSPECIFIED_TEAM_INDEX) || 
                    (createGameRequest.getPlayerJoinData().getDefaultTeamIndex() >= createGameRequest.getGameCreationData().getTeamIds().size()) )
                {
                    // can't join team if there are no teams defined
                    TRACE_LOG("[GameManagerValidationUtils].validateTeamCapacity: user attempting to join team at index UNSPECIFIED_TEAM_INDEX when there are teams defined in teamCapacites.");
                    return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
                }
            }
        }
        else
        {
            // If creating virtual reserved game ensure no caller gets tallied.
            const BlazeId virtualReservationCaller = (isVirtualWithReservation ? UserSession::getCurrentUserBlazeId() : INVALID_BLAZE_ID);

            PerPlayerJoinDataList::const_iterator curIter = createGameRequest.getPlayerJoinData().getPlayerDataList().begin();
            PerPlayerJoinDataList::const_iterator endIter = createGameRequest.getPlayerJoinData().getPlayerDataList().end();
            for (; curIter != endIter; ++curIter)
            {
                // Spectators do not specify a valid team index, so there's no need for this check:
                if (!isSpectatorSlot(getSlotType(createGameRequest.getPlayerJoinData(), (*curIter)->getSlotType())) && (virtualReservationCaller != (*curIter)->getUser().getBlazeId()))
                {
                    TeamIndex teamIndex = getTeamIndex(createGameRequest.getPlayerJoinData(), (*curIter)->getTeamIndex());
                    if ((teamIndex == UNSPECIFIED_TEAM_INDEX) || 
                        (teamIndex >= createGameRequest.getGameCreationData().getTeamIds().size()) )
                    {
                        // can't join team if there are no teams defined
                        TRACE_LOG("[GameManagerValidationUtils].validateTeamCapacity: Individual user attempting to join team at index UNSPECIFIED_TEAM_INDEX when there are teams defined in teamCapacites.");
                        return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
                    }
                }
            }
        }

        uint16_t totalParticipantCapacity = createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) + createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT);
        if ((totalParticipantCapacity % teamIdVector.size()) != 0)
        {
             TRACE_LOG("[GameManagerValidationUtils].validateTeamCapacity: (" << teamIdVector.size() << ") teams do not divide participant capacity of ("
                << totalParticipantCapacity << ") evenly.");
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS;
        }

        // validatePlayerRolesForCreateGame and findOpenTeam will handle the team join capacity logic.
        return ERR_OK;
    }

     /*! ************************************************************************************************/
    /*! \brief validate the game's role information vs # of joining players and their assigned role.  Returns ERR_OK on success.
                should only be used to validate Roles on a create game or reset dedicated server request.

    \param[in] createGameRequest - the request parameters to validate
    \param[in] isReset - if true this is a game reset
    ***************************************************************************************************/
    Blaze::BlazeRpcError validatePlayerRolesForCreateGame( const CreateGameMasterRequest& masterRequest, bool isReset/* = false*/)
    {
        const CreateGameRequest& request = masterRequest.getCreateRequest();
        const UserSessionInfo* hostUserSession = getHostSessionInfo(masterRequest.getUsersInfo());
        BlazeId creatingPlayerId = hostUserSession ? hostUserSession->getUserInfo().getId() : INVALID_BLAZE_ID;

        if (request.getGameCreationData().getRoleInformation().getRoleCriteriaMap().empty())
        {
            // no roles
            TRACE_LOG("[GameSessionValidationUtils].validatePlayerRolesForCreateGame RoleCriteriaMap was empty for game creation attempt by player ("
                << creatingPlayerId << ").");
            return GAMEMANAGER_ERR_EMPTY_ROLE_CAPACITIES;
        }


        // Tally the team/roles:
        eastl::hash_map<TeamIndex, RoleSizeMap> gameTeamRoleSizeMap(BlazeStlAllocator("GameManagerMasterImpl::gameTeamRoleSizeMap", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        UserJoinInfoList::const_iterator iter = masterRequest.getUsersInfo().begin();
        for (; iter != masterRequest.getUsersInfo().end(); ++iter)
        {
            // Optional Users (and the host when it's a dedicated/virtual game and not being reset) get skipped:
            if ((*iter)->getIsOptional() || ((*iter)->getIsHost() &&
                (request.getGameCreationData().getGameSettings().getVirtualized() || isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology())) && !isReset))
            {
                continue;
            }

            const UserSessionInfo& userInfo = (*iter)->getUser();
            if (gUserSessionManager->getSessionExists(userInfo.getSessionId()))
            {
                // Skip spectators when tallying role capacities:
                if (isSpectatorSlot(getSlotType(request.getPlayerJoinData(), userInfo.getUserInfo())))
                    continue;

                TeamIndex teamIndex = getTeamIndex(request.getPlayerJoinData(), userInfo.getUserInfo());
                const char8_t* roleName = lookupPlayerRoleName(request.getPlayerJoinData(), userInfo.getUserInfo().getId());
                if (!roleName)
                {
                    ERR_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame: Missing user in UserInfo (" << userInfo.getUserInfo().getId() << ") from PlayerJoinData (while not using default role).");
                    return ERR_SYSTEM;
                }
                gameTeamRoleSizeMap[teamIndex][roleName]++;
            }
        }



        // get team capacity
        uint16_t teamCount = request.getGameCreationData().getTeamIds().size();
        uint16_t teamCapacity = teamCount != 0 ? (request.getSlotCapacities()[SLOT_PUBLIC_PARTICIPANT] + request.getSlotCapacities()[SLOT_PRIVATE_PARTICIPANT]) / teamCount : 0;
        // per team total role capacity
        uint16_t totalRoleCapacity = 0;
        // now validate that the roles aren't oversubscribed
        RoleCriteriaMap::const_iterator roleCriteriaIter = request.getGameCreationData().getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator roleCriteriaEnd = request.getGameCreationData().getRoleInformation().getRoleCriteriaMap().end();
        for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            uint16_t roleCapacity = roleCriteriaIter->second->getRoleCapacity();
            const RoleName &roleName = roleCriteriaIter->first;

            if (blaze_stricmp(roleName, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
            {
                // 'PLAYER_ROLE_NAME_ANY' is reserved and not allowed as a joinable role in the game session
                TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame RoleName '" << roleName << "' is not allowed, found in CreateGameRequest by player (" 
                    << creatingPlayerId << ").");
                return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;

            }

            if (roleCapacity > teamCapacity)
            {
                TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame RoleName '" << roleName << "' capacity was (" << roleCapacity
                    << ") with team capacity (" << teamCapacity << ") in CreateGameRequest by player (" << creatingPlayerId << ").");
                return GAMEMANAGER_ERR_ROLE_CAPACITY_TOO_LARGE;
            }

            totalRoleCapacity += roleCapacity;
        }

        if (totalRoleCapacity < teamCapacity)
        {
            // didn't make enough role capacity to fill a team
            TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame total capacity of (" << totalRoleCapacity
                << ") was too small with team capacity ("<< teamCapacity << ") in CreateGameRequest by player (" << creatingPlayerId << ").");
            return GAMEMANAGER_ERR_ROLE_CAPACITY_TOO_SMALL;
        }

        // Construct compound role criteria
        MultiRoleEntryCriteriaEvaluator evaluator;
        evaluator.setStaticRoleInfoSource(request.getGameCreationData().getRoleInformation());
        int32_t errors = evaluator.createCriteriaExpressions();
        if (errors != 0)
        {
            TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame there were errors with the multirole criteria in CreateGameRequest by player (" << creatingPlayerId << ").");
            return GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_INVALID;
        }

        // Here we check each team's roles to make sure they're not exceeding capacity or using invalid roles:
        eastl::hash_map<TeamIndex, RoleSizeMap>::const_iterator curTeamIter = gameTeamRoleSizeMap.begin();
        eastl::hash_map<TeamIndex, RoleSizeMap>::const_iterator endTeamIter = gameTeamRoleSizeMap.end();
        for (; curTeamIter != endTeamIter; ++curTeamIter)
        {
            TeamIndex teamIndex = curTeamIter->first;

            // For each role used:
            RoleSizeMap::const_iterator curRoleIter = curTeamIter->second.begin();
            RoleSizeMap::const_iterator endRoleIter = curTeamIter->second.end();
            for (; curRoleIter != endRoleIter; ++curRoleIter)
            {
                RoleName roleName = curRoleIter->first;
                uint16_t roleSize = curRoleIter->second;

                if (blaze_stricmp(roleName, GameManager::PLAYER_ROLE_NAME_ANY) != 0)
                {
                    // We have already verified that each role exists?
                    RoleCriteriaMap::const_iterator rcIter = request.getGameCreationData().getRoleInformation().getRoleCriteriaMap().find(roleName);
                    if (rcIter != request.getGameCreationData().getRoleInformation().getRoleCriteriaMap().end())
                    {
                        uint16_t roleCapacity = rcIter->second->getRoleCapacity();
                        if (roleSize > roleCapacity)
                        {
                            TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame RoleName '" << roleName << "' capacity of (" << roleCapacity
                                << ") was exceeded ("<< roleSize <<") when creating player's team in CreateGameRequest by player (" << creatingPlayerId << ").");
                            return GAMEMANAGER_ERR_ROLE_FULL;
                        }
                    }
                    else
                    {
                        // role not present in game
                        TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame RoleName '" << roleName
                            << "' requested by join not present in CreateGameRequest by player (" << creatingPlayerId << ").");
                        return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                    }
                }
            }

            // Validate compound role criteria
            EntryCriteriaName failedName;
            if (!evaluator.evaluateEntryCriteria(curTeamIter->second, failedName))
            {
                TRACE_LOG("[GameManagerValidationUtils].validatePlayerRolesForCreateGame failed the multirole criteria (" << failedName.c_str() << ") with the given player roles for team index " << teamIndex<< " .");
                return GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED;
            }
        }


        return ERR_OK;
    }

    void validatePlayersInCreateRequest( const CreateGameMasterRequest& createGameRequest, BlazeRpcError& error, SlotTypeSizeMap& sizeMap, bool isVirtualWithReservation)
    {
        error = Blaze::ERR_OK;

        if ( !isDedicatedHostedTopology(createGameRequest.getCreateRequest().getGameCreationData().getNetworkTopology()) || isVirtualWithReservation )
        {
            populateSlotTypeSizes(createGameRequest, sizeMap, isVirtualWithReservation);
        }
        else
        {
            // Virtual game creation doesn't add players, except if its reservations
            if (createGameRequest.getUsersInfo().size() > 1)
            {
                WARN_LOG("[GameManagerValidationUtils].validatePlayersInCreateRequest: Create game failed since a group can't create client server dedicated game, use resetDedecatedServer instead.");
                error = Blaze::GAMEMANAGER_ERR_INVALID_ACTION_FOR_GROUP;
            }
        }
    }

    void populateSlotTypeSizes(const CreateGameMasterRequest& createGameRequest, SlotTypeSizeMap& slotTypeSizeMap, bool isVirtualWithReservation)
    {
        eastl::set<BlazeId> duplicatePlayerSet(BlazeStlAllocator("GameManagerMasterImpl::duplicatePlayerSet", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        // First add in the player creating.
        const UserSessionInfo* hostUserSession = getHostSessionInfo(createGameRequest.getUsersInfo());
        // Only add the host if they really exist. If creating virtual reserved game ensure no caller gets tallied.
        const BlazeId virtualReservationCaller = (isVirtualWithReservation ? UserSession::getCurrentUserBlazeId() : INVALID_BLAZE_ID);
        if (hostUserSession && (virtualReservationCaller != hostUserSession->getUserInfo().getId()))
        {
            duplicatePlayerSet.insert(hostUserSession->getUserInfo().getId());
            SlotType slotType = getSlotType(createGameRequest.getCreateRequest().getPlayerJoinData(), hostUserSession->getUserInfo());
            slotTypeSizeMap[slotType] += 1;
        }

        // If we are specifying a user group, add in all those players.
        if (createGameRequest.getCreateRequest().getPlayerJoinData().getGroupId() != EA::TDF::OBJECT_ID_INVALID)
        {
            UserJoinInfoList::const_iterator iter = createGameRequest.getUsersInfo().begin();
            for (; iter != createGameRequest.getUsersInfo().end(); ++iter)
            {
                if ((*iter)->getIsOptional() || (*iter)->getIsHost())
                    continue;

                const UserSessionInfo& userInfo = (*iter)->getUser();
                if (gUserSessionManager->getSessionExists(userInfo.getSessionId()))
                {
                    bool newPlayer = duplicatePlayerSet.insert(userInfo.getUserInfo().getId()).second;
                    if (newPlayer)
                    {
                        SlotType slotType = getSlotType(createGameRequest.getCreateRequest().getPlayerJoinData(), userInfo.getUserInfo());
                        slotTypeSizeMap[slotType] += 1;
                    }
                }
                else
                {
                    WARN_LOG("[GameManagerValidationUtils] Unknown UserSession(" << userInfo.getSessionId() << ") in group for create request");
                }
            }
        }
        else if (!createGameRequest.getIsPseudoGame())
        {
            TRACE_LOG( "[GameManagerValidationUtils] Missing PlayerJoinData GroupId when validatePlayersInResetRequest. No players were validated." );
        }

        // Any reserved seats, possible already existed in the user group, or the creator.
        PerPlayerJoinDataList::const_iterator playerIter = createGameRequest.getCreateRequest().getPlayerJoinData().getPlayerDataList().begin();
        for (; playerIter != createGameRequest.getCreateRequest().getPlayerJoinData().getPlayerDataList().end(); ++playerIter)
        {
            // Count all the non-optional users:
            BlazeId id = (*playerIter)->getUser().getBlazeId();
            if (id != INVALID_BLAZE_ID && !(*playerIter)->getIsOptionalPlayer() && EA_UNLIKELY(id != createGameRequest.getExternalOwnerInfo().getUserInfo().getId()) && (id != virtualReservationCaller))
            {
                bool newPlayer = duplicatePlayerSet.insert(id).second;
                if (newPlayer)
                {
                    slotTypeSizeMap[getSlotType(createGameRequest.getCreateRequest().getPlayerJoinData(), (*playerIter)->getSlotType())] += 1;
                }
            }
        }

        uint16_t joiningPlayerCount = static_cast<uint16_t>(duplicatePlayerSet.size());
        TRACE_LOG("[GameManagerValidationUtils] " << joiningPlayerCount << " players validated in create request.");
    }

    bool validateAttributeIsPresent(const Collections::AttributeMap& attributeMap, const Collections::AttributeName& attributeName)
    {
        Collections::AttributeMap::const_iterator it = attributeMap.find(attributeName);
        if( it != attributeMap.end())
        {
            return true;
        }

        return false;
    }

    bool hasValidCCSConfig(const GameManagerServerConfig& config)
    {
        const char8_t* env = gController->getServiceEnvironment();
        const char8_t* apiVersion = config.getCcsConfig().getCcsAPIVer();
        const char8_t* pool = config.getCcsConfig().getCcsPool();
        if ((env[0] != '\0') && (apiVersion[0] != '\0') && (pool[0] != '\0'))
        {
            return true; 
        }

        return false;
    }

    bool isSetupToUseCCS(const GameManagerServerConfig& config)
    {
        for (GameModeToCCSModeMap::const_iterator it = config.getGameSession().getGameModeToCCSModeMap().begin(), itEnd = config.getGameSession().getGameModeToCCSModeMap().end(); it != itEnd; ++it)
        {
            if (it->second->getCCSMode() == CCS_MODE_HOSTEDONLY || it->second->getCCSMode() == CCS_MODE_HOSTEDFALLBACK)
            {
                return true;
            } 
        }
        
        return false;
    }
    
    bool validateFindDedicatedServerRules(const char8_t* createGameTemplate, const GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors)
    {
        CreateGameTemplateMap::const_iterator templateIter = config.getCreateGameTemplatesConfig().getCreateGameTemplates().find(createGameTemplate);
        if (templateIter == config.getCreateGameTemplatesConfig().getCreateGameTemplates().end())
        {
            StringBuilder strBuilder;
            strBuilder << "[GameManagerValidationUtils] createGameTemplate(" << createGameTemplate << ") not exist";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
            return false;
        }

        CreateGameTemplate templateConfig = (*templateIter->second);
        if (templateConfig.getCreateRequestType() != CreateGameTemplate::RESET_GAME)
        {
            if (!templateConfig.getFindDedicatedServerRulesMap().empty())
            {
                StringBuilder strBuilder;
                strBuilder << "[GameManagerValidationUtils] createGameTemplate(" << createGameTemplate << ") specifies FindDedicatedServerRulesMap but createRequestType is not RESET_GAME";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                return false;
            }
            else
                return true;
        }

        for (const auto& findRule : templateConfig.getFindDedicatedServerRulesMap())
        {
            bool found = false;
            for (const auto& configRule : config.getMatchmakerSettings().getRules().getDedicatedServerAttributeRules())
            {
                if (blaze_stricmp(configRule.first, findRule.first) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                StringBuilder strBuilder;
                strBuilder << "[GameManagerValidationUtils] createGameTemplate(" << createGameTemplate << ") has non existing dedicatedServerAttribute rule '" << findRule.first << "'";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }

        return (validationErrors.getErrorMessages().size() == 0);
    }

    BlazeRpcError validateRequestCrossplaySettings(bool& crossplayEnabled, const UserJoinInfoList& usersInfo, bool checkTopologies, const GameCreationData& gameCreationData,
                                                   const GameSessionServerConfig& config, ClientPlatformTypeList& clientPlatformListOverride, bool crossplayMustMatch)
    {
        // Cleanup the request and remove any possible duplicates from the Override list (since we care about list size being accurate):
        clientPlatformListOverride.eraseDuplicatesAsSet();

        // next we remove any platforms that aren't hosted by this blaze server
        auto curPlatformItr = clientPlatformListOverride.begin();
        while (curPlatformItr != clientPlatformListOverride.end())
        {
            // unsupported platform, remove from the list
            if (gController->getHostedPlatforms().find(*curPlatformItr) == gController->getHostedPlatforms().end())
            {
                WARN_LOG("[validateRequestCrossplaySettings]: Override list included non-hosted platform, '" << ClientPlatformTypeToString(*curPlatformItr) << "', removing from the override list.");
                curPlatformItr = clientPlatformListOverride.eraseAsSet(*curPlatformItr);
                if (clientPlatformListOverride.empty())
                {
                    // The only platforms in the override list aren't hosted by this Blaze instance.
                    // We should error out, because the caller may not support the platforms actually hosted at all.
                    ERR_LOG("[validateRequestCrossplaySettings]: Override list included only non-hosted platforms in clientPlatformListOverride. Failing validation.");
                    return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                }
            }
            else
            {
                ++curPlatformItr;
            }
        }



        // Check that overrides aren't invalid:
        if (clientPlatformListOverride.empty() && crossplayMustMatch == true)
        {
            ERR_LOG("[validateRequestCrossplaySettings]: Override list was empty, but crossplay was set to must match. (Override list required)");
            return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
        }

        // Get the User Information:
        ClientPlatformSet groupPlatforms;
        bool groupCrossplayEnabled = true;
        bool groupRequiresCrossplay = false;
        bool platformMissingFromOverrideList = false;
        bool usingNonCrossplayPlatformSet = false;
        const ClientPlatformSet* nonCrossplayPlatformSet = nullptr;
        for (auto curUserInfo : usersInfo)
        {


            // don't consider this user if it's a dedicated server host
            // testing both getIsHost, & the topology + client type to ensure that we're only skipping the *host* of a newly created dedicated server game
            if (!curUserInfo->getIsHost() 
                || !isDedicatedHostedTopology(gameCreationData.getNetworkTopology())
                || (curUserInfo->getUser().getClientType() != CLIENT_TYPE_DEDICATED_SERVER))
            {
                ClientPlatformType curPlatform = curUserInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform();
                groupPlatforms.insert(curPlatform);
                groupCrossplayEnabled = groupCrossplayEnabled && UserSessionManager::isCrossplayEnabled(curUserInfo->getUser().getUserInfo());

                // try to pull a non-crossplay platform set if we don't have one already
                if ((nonCrossplayPlatformSet == nullptr) || nonCrossplayPlatformSet->empty())
                {
                    nonCrossplayPlatformSet = &gUserSessionManager->getNonCrossplayPlatformSet(curPlatform);
                }
                // if anyone has a platform that's not in the nonCrossplayPlatformSet
                if (nonCrossplayPlatformSet->find(curPlatform) == nonCrossplayPlatformSet->end())
                {
                    groupRequiresCrossplay = true;
                    
                }

                if (!clientPlatformListOverride.empty() && (clientPlatformListOverride.findAsSet(curPlatform) == clientPlatformListOverride.end()))
                {
                    // we can replace the override list later on so need to mark now if we're missing anything
                    platformMissingFromOverrideList = true;
                }
            }

        }

        // Fail if this group requires crossplay but members don't have it enabled
        if (groupRequiresCrossplay && !groupCrossplayEnabled)
        {
            ERR_LOG("[validateRequestCrossplaySettings]: Group requires crossplay due to platforms used, but someone has crossplay disabled.");
            return GAMEMANAGER_ERR_CROSSPLAY_DISABLED_USER;
        }

        if (!groupRequiresCrossplay && !groupCrossplayEnabled && clientPlatformListOverride.empty())
        {
            if (nonCrossplayPlatformSet != nullptr)
            {
                clientPlatformListOverride.clear();
                for (const auto curPlatform : *nonCrossplayPlatformSet)
                {
                    clientPlatformListOverride.insertAsSet(curPlatform);
                }

                usingNonCrossplayPlatformSet = true;
            }

        }
        else if ((groupPlatforms.size() == 0) 
            && (isDedicatedHostedTopology(gameCreationData.getNetworkTopology()) || gameCreationData.getGameSettings().getVirtualized()))
        {
            // we've got a dedicated server game being created, support all the hosted platforms (actual settings are applied during game resets)
            if (clientPlatformListOverride.empty())
            {
                nonCrossplayPlatformSet = &gUserSessionManager->getNonCrossplayPlatformSet(common);
                for (auto curPlat : *nonCrossplayPlatformSet)
                    clientPlatformListOverride.insertAsSet(curPlat);

                usingNonCrossplayPlatformSet = true;
            }
        }

        // Check for Group Members that are in invalid platforms:
        for (auto curPlatform : groupPlatforms)
        {
            // Check for Group Members that are not in the Override list: 
            bool foundPlatform = false;
            for (ClientPlatformType curOverridePlat : clientPlatformListOverride)
            {
                if (curOverridePlat == curPlatform)
                {
                    foundPlatform = true;
                    break;
                }
            }
            if (!foundPlatform)
            {
                // see if the clientPlatformListOverride is a subset of our non-crossplay platform set. If so, we can use that instead. Skip all this if we already tried the nonCrossplayPlatformSet
                if (!groupRequiresCrossplay && !usingNonCrossplayPlatformSet)
                {
                    if (!nonCrossplayPlatformSet->empty())
                    {
                        // we can only use the non-crossplay platform set in cases where either we don't require that crossplay matches, or the clientPlatformListOverride is a SUBSET of the non-crossplay platform set
                        if (crossplayMustMatch)
                        {
                            for (ClientPlatformType curOverridePlat : clientPlatformListOverride)
                            {
                                if (nonCrossplayPlatformSet->find(curOverridePlat) == nonCrossplayPlatformSet->end())
                                {
                                    ERR_LOG("[validateRequestCrossplaySettings]: Group member was using platform " << ClientPlatformTypeToString(curPlatform) << " which is not in provided override list, " << ClientPlatformTypeToString(curOverridePlat) 
                                        << " in the clientPlatformListOverride was not a member of the nonCrossplayPlatformSet, blocking that usage.");
                                    return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                                }
                            }
                        }
                        else if (platformMissingFromOverrideList)// && !crossplayMustMatch - need to make sure that at least one member of our non-crossplay platform list is in the override list
                        {
                            
                            for (ClientPlatformType nonCrossplayPlatform : *nonCrossplayPlatformSet)
                            {
                                if (clientPlatformListOverride.findAsSet(nonCrossplayPlatform) != clientPlatformListOverride.end())
                                {
                                    foundPlatform = true;
                                    break;
                                    
                                }
                            }

                            if (!foundPlatform)
                            {
                                ERR_LOG("[validateRequestCrossplaySettings]: Group member was using platform " << ClientPlatformTypeToString(curPlatform) << " which is not in provided override list.");
                                return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                            }
                        }

                        // ok to replace client platform list override here, since we'll not have REDUCED the set of allowed platforms.
                        clientPlatformListOverride.clear();
                        for (const auto curNonCrossplayPlatform : *nonCrossplayPlatformSet)
                        {
                            clientPlatformListOverride.insertAsSet(curNonCrossplayPlatform);
                        }

                        usingNonCrossplayPlatformSet = true;
                    }

                }
                else
                {
                    ERR_LOG("[validateRequestCrossplaySettings]: Group member was using platform " << ClientPlatformTypeToString(curPlatform) << " which is not in provided override list.");
                    return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                }
                
            }


            // Clean up the request to only allow platforms that are supported by the players that are joining:
            for (auto curOverridePlatIter = clientPlatformListOverride.begin(); curOverridePlatIter != clientPlatformListOverride.end(); )
            {
                const ClientPlatformSet& platSet = gUserSessionManager->getUnrestrictedPlatforms(curPlatform);
                if (platSet.find(*curOverridePlatIter) == platSet.end())
                {
                    TRACE_LOG("[validateRequestCrossplaySettings]: Removing platform " << ClientPlatformTypeToString(*curOverridePlatIter) << " from override list because platform of user (" << ClientPlatformTypeToString(curPlatform) << ") doesn't support it.");;
                    curOverridePlatIter = clientPlatformListOverride.erase(curOverridePlatIter);
                }
                else
                {
                    ++curOverridePlatIter;
                }
            }

            // Check if someone was trying to disable Crossplay, even though we have must match on a CP enabled override: 
            if (!groupCrossplayEnabled && !usingNonCrossplayPlatformSet)
            {
                // test if this crossplay-disabled situation is trying to include excess platforms
                for (ClientPlatformType curOverridePlat : clientPlatformListOverride)
                {
                    if (nonCrossplayPlatformSet->find(curOverridePlat) == nonCrossplayPlatformSet->end())
                    {
                        if (crossplayMustMatch)
                        {
                            ERR_LOG("[validateRequestCrossplaySettings]: Group crossplay does not match platform list override, which has must match set.  Update player settings to match the Override to avoid this issue.");
                            return GAMEMANAGER_ERR_CROSSPLAY_DISABLED_USER;
                        }
                        else
                        {
                            clientPlatformListOverride.clear();
                            for (const auto curNonCrossplayPlatform : *nonCrossplayPlatformSet)
                            {
                                clientPlatformListOverride.insertAsSet(curNonCrossplayPlatform);
                            }

                            usingNonCrossplayPlatformSet = true;
                            break;
                        }
                    }
                }

            }
        }

        

        crossplayEnabled = groupCrossplayEnabled && (!usingNonCrossplayPlatformSet && (clientPlatformListOverride.size() > 1));

        // If Crossplay is enabled, check the settings that are being used in the request: 
        if (crossplayEnabled && checkTopologies)
        {
            // Check if CCS is enabled:  (first get the game mode)
            bool ccsEnabled = false;
            const char8_t* gameMode = "";
            auto iter = gameCreationData.getGameAttribs().find(config.getGameModeAttributeName());
            if (iter != gameCreationData.getGameAttribs().end()) // Other checks exist for this missing
                gameMode = iter->second.c_str();

            CCSMode ccsMode = CCS_MODE_INVALID;
            getCCSModeToUse(config, gameMode, crossplayEnabled, ccsMode);
            if (ccsMode == CCS_MODE_HOSTEDFALLBACK || ccsMode == CCS_MODE_HOSTEDONLY)
                ccsEnabled = true;

            // We only need to do these topology checks if CCS is disabled:
            if (!ccsEnabled)
            {
                bool voipSupportsCrossplay = (gameCreationData.getVoipNetwork() == VOIP_DEDICATED_SERVER) || (gameCreationData.getVoipNetwork() == VOIP_DISABLED);
                bool networkSupportsCrossplay = (gameCreationData.getNetworkTopology() == CLIENT_SERVER_DEDICATED) || (gameCreationData.getNetworkTopology() == NETWORK_DISABLED);

                if (!voipSupportsCrossplay || !networkSupportsCrossplay)
                {
                    // If Crossplay can be disabled, attempt to disable it, rather than failing to create the Game. 
                    if (!crossplayMustMatch && !groupRequiresCrossplay)
                    {
                        // By using a nonCrossplayPlatformSet, we can prevent the Game from being created incorrectly. 
                        
                        if (nonCrossplayPlatformSet != nullptr && (!nonCrossplayPlatformSet->empty()))
                        {
                            // replace clientPlatformListOverride with the nonCrossplayPlatformSet
                            crossplayEnabled = false;
                            clientPlatformListOverride.clear();
                            if (nonCrossplayPlatformSet->size() > 1)
                            {
                                for (const auto curPlatform : *nonCrossplayPlatformSet)
                                {
                                    clientPlatformListOverride.insertAsSet(curPlatform);
                                }
                                usingNonCrossplayPlatformSet = true;
                            }

                        }
                        else if(groupPlatforms.size() == 1)
                        {
                            WARN_LOG("[validateRequestCrossplaySettings]: User " << UserSession::getCurrentUserBlazeId()
                                << " had group containing only platform " << ClientPlatformTypeToString(*groupPlatforms.begin()) << " which does not have a non-crossplay platform set defined.");
                            crossplayEnabled = false;
                            clientPlatformListOverride.clear();
                            clientPlatformListOverride.insertAsSet(*groupPlatforms.begin());
                            usingNonCrossplayPlatformSet = true;
                        }
                    }

                    // If we weren't able to disable crossplay, then  we have to fail.
                    if (crossplayEnabled)
                    {
                        if (!voipSupportsCrossplay)
                        {
                            ERR_LOG("[validateRequestCrossplaySettings]: User " << UserSession::getCurrentUserBlazeId()
                                << " attempted to reset Crossplay enabled game[" << gameCreationData.getGameName() << "] with voip topology[" <<
                                VoipTopologyToString(gameCreationData.getVoipNetwork()) << "], which is not supported with crossplay (Only VOIP_DISABLED works without CCS).");
                            return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                        }

                        if (!networkSupportsCrossplay)
                        {
                            ERR_LOG("[validateRequestCrossplaySettings]: User " << UserSession::getCurrentUserBlazeId()
                                << " attempted to create Crossplay enabled game[" << gameCreationData.getGameName() << "] with topology[" <<
                                GameNetworkTopologyToString(gameCreationData.getNetworkTopology()) << "], which is not supported with crossplay (Only CLIENT_SERVER_DEDICATED and NETWORK_DISSABLED work without CCS).");
                            return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
                        }
                    }
                }
            }
        }


        return ERR_OK;
    }

    void getCCSModeToUse(const GameSessionServerConfig& config, const char8_t* gameMode, bool crossplayEnabled, CCSMode& ccsMode)
    {
        bool ccsModeSet = false;
        if (gameMode != nullptr && gameMode[0] != '\0') // game mode specific ccs mode
        {
            GameModeToCCSModeMap::const_iterator it = config.getGameModeToCCSModeMap().find(gameMode);
            if (it != config.getGameModeToCCSModeMap().end())
            {
                ccsMode = it->second->getCCSMode();
                ccsModeSet = true;
            }
        }
        if (ccsModeSet == false) // try again by looking up the default setting if available. 
        {
            GameModeToCCSModeMap::const_iterator it = config.getGameModeToCCSModeMap().find("defaultCCSMode");
            if (it != config.getGameModeToCCSModeMap().end())
            {
                ccsMode = it->second->getCCSMode();
            }
        }
    }

    void getCCSModeAndPoolToUse(const GameManagerServerConfig& config, const char8_t* gameMode, bool crossplayEnabled, CCSMode& ccsMode, const char8_t*& ccsPool)
    {
        bool ccsModeSet = false;
        if (gameMode != nullptr && gameMode[0] != '\0') // game mode specific ccs mode
        {
            GameModeToCCSModeMap::const_iterator it = config.getGameSession().getGameModeToCCSModeMap().find(gameMode);
            if (it != config.getGameSession().getGameModeToCCSModeMap().end())
            {
                ccsMode = it->second->getCCSMode();
                ccsPool = it->second->getCCSPool();
                ccsModeSet = true;
            }
        }
        if (ccsModeSet == false) // try again by looking up the default setting if available. 
        {
            GameModeToCCSModeMap::const_iterator it = config.getGameSession().getGameModeToCCSModeMap().find("defaultCCSMode");
            if (it != config.getGameSession().getGameModeToCCSModeMap().end())
            {
                ccsMode = it->second->getCCSMode();
                ccsPool = config.getCcsConfig().getCcsPool();
            }
        }
    }

    BlazeRpcError validateExternalOwnerParamsInReq(const CreateGameRequest& req, const UserSessionInfo& extOwnerInfo)
    {
        if (!req.getGameCreationData().getIsExternalOwner())
        {
            return ERR_OK;
        }
        if ((extOwnerInfo.getUserInfo().getId() == INVALID_BLAZE_ID) || (extOwnerInfo.getSessionId() == INVALID_USER_SESSION_ID) ||
            (!gUserSessionManager || !gUserSessionManager->getSessionExists(extOwnerInfo.getSessionId())))
        {
            ERR_LOG("[GameManagerValidationUtils] no valid creating user session info for external owner: " << extOwnerInfo);
            return GAMEMANAGER_ERR_USER_NOT_FOUND;
        }
        // restrict untested/unsupported features for games with external owners
        if (req.getGameCreationData().getNetworkTopology() != NETWORK_DISABLED)
        {
            ERR_LOG("[GameManagerValidationUtils] unsupported external owner topology(" << GameNetworkTopologyToString(req.getGameCreationData().getNetworkTopology()) << ").");
            return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
        }
        return ERR_OK;
    }

    void fixupPjdByRemovingCaller(PlayerJoinData& playerJoinData)
    {
        auto callerBlazeId = UserSession::getCurrentUserBlazeId();
        if (callerBlazeId != INVALID_BLAZE_ID)
        {
            PerPlayerJoinDataList::iterator extOwnerPjdIt = playerJoinData.getPlayerDataList().end();
            for (auto it = playerJoinData.getPlayerDataList().begin(); it != playerJoinData.getPlayerDataList().end(); ++it)
            {
                if ((*it)->getUser().getBlazeId() == callerBlazeId)
                {
                    extOwnerPjdIt = it;
                    break;
                }
            }
            if (extOwnerPjdIt != playerJoinData.getPlayerDataList().end())
            {
                playerJoinData.getPlayerDataList().erase(extOwnerPjdIt);
            }
        }
    }
    

} // namespace ValidationUtils
} // namespace GameManager
} // namespace Blaze
