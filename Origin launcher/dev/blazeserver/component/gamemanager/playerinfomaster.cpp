/*! ************************************************************************************************/
/*!
    \file playerinfomaster.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/endpoint.h"
#include "framework/util/uuid.h"
#include "gamemanager/playerinfomaster.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/gamesessionmaster.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief construct a PlayerInfoMaster obj based on passed in parameters
    
        \param[in]gameId    - gameId of the game the player belongs to
        \param[in]playerId  - playerId of the to be created player
        \param[in]externalId- externalId of the game the player belongs to
        \param[in]accountLocale- Account Locale of the to be created player
        \param[in]state     - player state of the to be created player
        \param[in]slotType  - the slotType the player will consume
        \param[in]teamId    - the team the player will join
        \param[in]roleName  - the role the player will take on
        \param[in]teamIndex - the index of the team the player will join
        \param[in]session   - user session of the player to be created, could be nullptr since in some cases
                              (eg matchmaking reserve player playerState == RESERVED), there is no associated user session

        Note, when the user session is nullptr, the player is not really created, it's in RESERVED state,
        so for network info, player attribute etc properties of player, it's not set yet, we will provide
        these info later on when we really join the game in some active state
    ***************************************************************************************************/
    PlayerInfoMaster::PlayerInfoMaster(const GameSessionMaster& gameSessionMaster, PlayerState state, SlotType slotType, 
        TeamIndex teamIndex, const RoleName& roleName, const UserGroupId& groupId,
        const UserSessionInfo& joinInfo, const char8_t* encryptedBlazeId)
        : PlayerInfo(BLAZE_NEW ReplicatedGamePlayerServer), mPlayerDataMaster(BLAZE_NEW PlayerDataMaster)
    {
        mPlayerDataMaster->setOriginalPlayerSessionId(joinInfo.getSessionId());
        mPlayerDataMaster->setOriginalConnectionGroupId(joinInfo.getConnectionGroupId());

        // NOTE: This ensures that both 'new' Player and 'joined' Player constructor paths always set the userGroup to connectionGroup if the former is unspecified (see: setupJoiningPlayer() usage in overloaded ctor below).
        auto userGroupId = (groupId == EA::TDF::OBJECT_ID_INVALID) ? EA::TDF::ObjectId(ENTITY_TYPE_CONN_GROUP, joinInfo.getConnectionGroupId()) : groupId;

        mPlayerDataMaster->setOriginalUserGroupId(userGroupId);

        mPlayerData->setGameId(gameSessionMaster.getGameId());
        mPlayerData->setPlayerState(state);
        mPlayerData->setSlotType(slotType);
        mPlayerData->setTeamIndex(teamIndex);
        mPlayerData->setRoleName(roleName);
        mPlayerData->setUserGroupId(userGroupId);

        joinInfo.copyInto(mPlayerData->getUserInfo());
        // to be deprecated as this flag should be replaced by player settings
        mPlayerData->setHasJoinFirstPartyGameSessionPermission(mPlayerData->getUserInfo().getHasExternalSessionJoinPermission()); 

        if (mPlayerData->getUserInfo().getHasExternalSessionJoinPermission())
            mPlayerData->getPlayerSettings().setHasJoinFirstPartyGameSessionPermission();

        if (mPlayerData->getUserInfo().getUserInfo().getVoipDisabled())
            mPlayerData->getPlayerSettings().setHasVoipDisabled();

        if (encryptedBlazeId != nullptr)
            mPlayerData->setEncryptedBlazeId(encryptedBlazeId);

        eastl::string platformInfoStr;
        TRACE_LOG("[PlayerInfoMaster].ctor (long, info) creating new player for game '" << gameSessionMaster.getGameId() << "', playerId '" 
                  << getPlayerId() << "' platformInfo '" << platformInfoToString(getUserInfo().getUserInfo().getPlatformInfo(), platformInfoStr) << "' playerState '" << PlayerStateToString(state) << "' slotType '" << slotType << "' teamIndex '"
                  << teamIndex << "' roleName '" << roleName << "' groupId '" << userGroupId.toString().c_str() << "'");       

        // start qos validation timeout, to ensure mm finalization ends
        if (gameSessionMaster.isPlayerInMatchmakingQosDataMap(getPlayerId()))
        {
            startQosValidationTimer();
        }

        mPlayerDataMaster->setProtoTunnelVer(gUserSessionManager->getProtoTunnelVer(getPlayerSessionId()));
    }

    /*! ************************************************************************************************/
    /*! \brief construct a PlayerInfoMaster obj based on passed in parameters

        \param[in]request   - join game request
        \param[in]session   - user session of the player to be created, should never be nullptr

        Note, when we would like to create a real player in the game, we should use this constructor so
        all information of the player will be provided and set
    ***************************************************************************************************/
    PlayerInfoMaster::PlayerInfoMaster(const JoinGameMasterRequest& request, const GameSessionMaster& gameSessionMaster)
        : PlayerInfo(BLAZE_NEW ReplicatedGamePlayerServer), mPlayerDataMaster(BLAZE_NEW PlayerDataMaster)
    {
        mPlayerDataMaster->setOriginalPlayerSessionId(request.getUserJoinInfo().getUser().getSessionId());
        mPlayerDataMaster->setOriginalConnectionGroupId(request.getUserJoinInfo().getUser().getConnectionGroupId());
        mPlayerDataMaster->setOriginalUserGroupId(request.getUserJoinInfo().getUserGroupId());
        mPlayerDataMaster->setJoinMethod(request.getJoinRequest().getJoinMethod());
        mPlayerDataMaster->setGameEntryType(request.getJoinRequest().getPlayerJoinData().getGameEntryType());

        mPlayerData->setUserGroupId(request.getUserJoinInfo().getUserGroupId());
        mPlayerData->setTargetPlayerId(request.getJoinRequest().getUser().getBlazeId());
        mPlayerData->setScenarioName(request.getUserJoinInfo().getScenarioInfoPerUser().getScenarioName());

        request.getUserJoinInfo().getScenarioInfoPerUser().copyInto(mPlayerDataMaster->getScenarioInfo());
        mPlayerDataMaster->setReservationTimeoutOverride(request.getUserJoinInfo().getPlayerReservationTimeout());

        // Default the players state
        // Note, if it's a host player, we need to set the state to ACTIVE_CONNECTED      
        bool hostPlayerSession = ((gameSessionMaster.getTopologyHostSessionId() == request.getUserJoinInfo().getUser().getSessionId()) ? true : false);
        if (hostPlayerSession)
        {
            EA_ASSERT(request.getJoinRequest().getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION);

            setPlayerState(ACTIVE_CONNECTED);
        }
        else
        {
            if (gameSessionMaster.getPlayerRoster()->hasConnectedPlayersOnConnection(mPlayerDataMaster->getOriginalConnectionGroupId()))
            {
                setPlayerState(ACTIVE_CONNECTED);
            }
            else
            {
                setPlayerState(ACTIVE_CONNECTING);
            }
        }
       
        if (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {
            setPlayerState(RESERVED);
        }

        // set conn group here for incoming players
        // can't set inside setupJoiningPlayer as that is also called when claiming a reservations
        // and it can't be set there
        setupJoiningPlayer(request);  

        if (gameSessionMaster.isPlayerInMatchmakingQosDataMap(getPlayerId()))
        {
            startQosValidationTimer();
        }

        mPlayerData->setEncryptedBlazeId(GameManager::getEncryptedBlazeId(request.getJoinRequest().getPlayerJoinData(), request.getUserJoinInfo().getUser().getUserInfo()));
    }

    /*! ************************************************************************************************/
    /*! \brief Copy constructor
    
        \param[in] player to copy
        \param[in] newUser if specified, update the new player's user related data from this session.
    ***************************************************************************************************/
    PlayerInfoMaster::PlayerInfoMaster(const PlayerInfoMaster& player)
        : PlayerInfo(BLAZE_NEW ReplicatedGamePlayerServer), mPlayerDataMaster(BLAZE_NEW PlayerDataMaster)
    {
        player.mPlayerDataMaster->copyInto(*mPlayerDataMaster);

        mPlayerData->setGameId(player.getGameId());
        mPlayerData->setSlotId(player.getSlotId());
        mPlayerData->setPlayerState(player.getPlayerState());
        player.getCustomData()->copyInto(mPlayerData->getCustomData());
        mPlayerData->setSlotType(player.getSlotType());
        mPlayerData->setTeamIndex(player.getTeamIndex());
        mPlayerData->setRoleName(player.getRoleName());
        mPlayerData->setUserGroupId(player.getUserGroupId());
        mPlayerData->setJoinedViaMatchmaking(player.getJoinedViaMatchmaking());
        mPlayerData->setJoinedGameTimestamp(player.getJoinedGameTimestamp());
        mPlayerData->setReservationCreationTimestamp(player.getReservationCreationTimestamp());
        mPlayerData->setConnectionSlotId(player.getConnectionSlotId());
        mPlayerData->setTargetPlayerId(player.getTargetPlayerId());
        player.mPlayerData->getUserInfo().copyInto(mPlayerData->getUserInfo());
        // this flag should be deprecated as it is now replaced by player settings
        mPlayerData->setHasJoinFirstPartyGameSessionPermission(mPlayerData->getUserInfo().getHasExternalSessionJoinPermission());
        mPlayerData->getPlayerSettings().setBits(player.mPlayerData->getPlayerSettings().getBits());
        mPlayerData->setEncryptedBlazeId(player.mPlayerData->getEncryptedBlazeId());
    }

    PlayerInfoMaster::PlayerInfoMaster(PlayerDataMaster& playerDataMaster, ReplicatedGamePlayerServer& playerData)
        : PlayerInfo(&playerData), mPlayerDataMaster(&playerDataMaster) // NOTE: PlayerInfoMaster will own playerDataMaster/playerData
    {
    }

    PlayerInfoMaster::~PlayerInfoMaster()
    {
        cancelJoinGameTimer();
        cancelPendingKickTimer();
        cancelReservationTimer();
        cancelQosValidationTimer();

        // Remove entry from external player tracking map if present. Players that were created
        // with a user session, are never inserted to the map so no need to check those.
        if (hasExternalPlayerId() && !getSessionExists())
        {
            gGameManagerMaster->eraseFromExternalPlayerToGameIdsMap(getUserInfo().getUserInfo(), getGameId());
        }
    }


    /*! ************************************************************************************************/
    /*! \brief initialize this player's context & session info (game id, player attributes, networkInfo,
        user name, id, session id, network QoS data, & team as needed)

        \param[in] request - joinGame request
    ***************************************************************************************************/
    void PlayerInfoMaster::setupJoiningPlayer(const JoinGameMasterRequest& request)
    {
        request.getUserJoinInfo().getUser().copyInto(mPlayerData->getUserInfo());

        mPlayerDataMaster->setProtoTunnelVer(gUserSessionManager->getProtoTunnelVer(getPlayerSessionId()));

        mPlayerData->setGameId(request.getJoinRequest().getGameId()); 

        // this flag should be deprecated as it is now replaced by player settings
        mPlayerData->setHasJoinFirstPartyGameSessionPermission(getUserInfo().getHasExternalSessionJoinPermission());

        (getUserInfo().getHasExternalSessionJoinPermission()) ? 
            mPlayerData->getPlayerSettings().setHasJoinFirstPartyGameSessionPermission() : mPlayerData->getPlayerSettings().clearHasJoinFirstPartyGameSessionPermission();

        (getUserInfo().getUserInfo().getVoipDisabled()) ?
            mPlayerData->getPlayerSettings().setHasVoipDisabled() : mPlayerData->getPlayerSettings().clearHasVoipDisabled();

        // conn group id can change on game entry, or it could be unset currently
        // if a user was reserved as part of a user group, they should retain that group id when claiming
        if ((mPlayerDataMaster->getOriginalUserGroupId() == EA::TDF::OBJECT_ID_INVALID) || (mPlayerDataMaster->getOriginalUserGroupId().type == ENTITY_TYPE_CONN_GROUP))
        {
            // We do not use the request.getJoinRequest().getPlayerJoinData().getGroupId() here because we do not want to change the group ids used by
            // CG MM join members.  Note: This code will only hit if the user group id was not already set (or was a CONN_GROUP join), so it shouldn't affect game group joins.
            mPlayerData->setUserGroupId(EA::TDF::ObjectId(ENTITY_TYPE_CONN_GROUP, request.getUserJoinInfo().getUser().getConnectionGroupId()));
        }
    
        // Find the player join data:  (Technically, this should always be the only member of the list, but we use the lookup function just in case)
        const PerPlayerJoinData* playerJoinData = lookupPerPlayerJoinDataConst(request.getJoinRequest().getPlayerJoinData(), request.getUserJoinInfo().getUser().getUserInfo());
        if (playerJoinData != nullptr)
            playerJoinData->getPlayerAttributes().copyInto(mPlayerData->getPlayerAttribs());

        mPlayerDataMaster->setReservationTimeoutOverride(request.getUserJoinInfo().getPlayerReservationTimeout());
        

        // if entry type is reservation-claim, current expected behavior is don't override orig reserved team.
        const bool isClaimingReservation = (request.getJoinRequest().getPlayerJoinData().getGameEntryType() == GameManager::GAME_ENTRY_TYPE_CLAIM_RESERVATION);

        // a reserved player may already have this information set, and if they are reserved from matchmaker, the join request may not have the correct
        // team id or team index. These checks prevent the fields from being overwritten if they are already set.
        // If co-creating MM session JoinMethod is not following group, overwrite mTeamIndex cached here, as we aren't part of the group.
        if (!isClaimingReservation && ((mPlayerData->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
            || (request.getJoinRequest().getJoinMethod() != GameManager::SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
            || (request.getJoinRequest().getJoinMethod() != GameManager::SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME_HOST)
            || (request.getJoinRequest().getJoinMethod() != GameManager::SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER)
            || (request.getJoinRequest().getJoinMethod() != GameManager::SYS_JOIN_BY_FOLLOWLEADER_RESERVEDEXTERNALPLAYER)))
        {
            mPlayerData->setTeamIndex(GameManager::getTeamIndex(request.getJoinRequest().getPlayerJoinData(), request.getUserJoinInfo().getUser().getUserInfo()));
        }

        // we don't allow a user claiming a reservation to change their join method, role or SlotType
        if (!isClaimingReservation)
        {
            const char8_t* role = lookupPlayerRoleName(request.getJoinRequest().getPlayerJoinData(), getPlayerId());
            if (!role)
            {
                ERR_LOG("[PlayerInfoMaster] Unable to find the role for player '" << getPlayerId() << "' in join request when joining game session(" << request.getJoinRequest().getGameId() << ").");
                return;
            }
            mPlayerData->setRoleName(role);
            mPlayerData->setSlotType(GameManager::getSlotType(request.getJoinRequest().getPlayerJoinData(), request.getUserJoinInfo().getUser().getUserInfo()));

            mPlayerDataMaster->setJoinMethod(request.getJoinRequest().getJoinMethod());
        }

        // JoinTimer Schedule, trigger the join timer if we're not already connected (ie, the host)
        const GameSessionMaster* gameSessionMaster = gGameManagerMaster->getReadOnlyGameSession(request.getJoinRequest().getGameId());
        bool hostPlayerSession = true;
        if (gameSessionMaster != nullptr)
        {
            hostPlayerSession = ((gameSessionMaster->getTopologyHostSessionId() == request.getUserJoinInfo().getUser().getSessionId()) ? true : false);
        }
        else
        {
            ERR_LOG("[PlayerInfoMaster] Unable to get the game session by the game id (" << request.getJoinRequest().getGameId() << ").");
            return;
        }

        if (!hostPlayerSession && (getPlayerState() != RESERVED) && (getPlayerState() != ACTIVE_CONNECTED))
        {
            startJoinGameTimer();
        }
    }


    /*! ************************************************************************************************/
    /*! \brief possibly update this player's joinedGame timestamp.  We only set the join once per player,
            since we don't want to cut the player's session length short due to host migration, etc.
    ***************************************************************************************************/
    void PlayerInfoMaster::setJoinedGameTimeStamp()
    {
        if (mPlayerData->getJoinedGameTimestamp() == JOINED_GAME_TIMESTAMP_NOT_JOINED)
        {
            mPlayerData->setJoinedGameTimestamp(TimeValue::getTimeOfDay().getMicroSeconds());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief merges the supplied attrib map with this player's attrib map.
        \param[in] updatedPlayerAttributes - attrib map to merge into player's map
        \param[out] changedPlayerAttributes - stores attributes actually changed
    ***************************************************************************************************/
    void PlayerInfoMaster::setPlayerAttributes(const Collections::AttributeMap &updatedPlayerAttributes, Collections::AttributeMap *changedPlayerAttributes/*= nullptr*/)
    {
        // Note: this is really more of an attribute merge - we only modify or add attributes (none are removed)
        Collections::AttributeMap::const_iterator updatedAttribIter = updatedPlayerAttributes.begin();
        Collections::AttributeMap::const_iterator updatedAttribEnd = updatedPlayerAttributes.end();
        for ( ; updatedAttribIter!=updatedAttribEnd; ++updatedAttribIter )
        {
            const EA::TDF::TdfString& name = updatedAttribIter->first;
            const EA::TDF::TdfString& value = updatedAttribIter->second;
            const Collections::AttributeMap::value_type mapPair(name, value);

            eastl::pair<Collections::AttributeMap::iterator, bool> inserted = mPlayerData->getPlayerAttribs().insert(mapPair);
            if (!inserted.second)
            {
                // pre-existing attribute. If not changing its value, skip
                if (blaze_strcmp(inserted.first->second.c_str(), value.c_str()) == 0)
                    continue;

                inserted.first->second = value;
            }
            // attribute got added or its value changed
            if (changedPlayerAttributes != nullptr)
            {
                (*changedPlayerAttributes).insert(mapPair);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief start the session's JoinGame timer
    ***************************************************************************************************/
    void PlayerInfoMaster::startJoinGameTimer()
    {
        EA_ASSERT(mPlayerDataMaster->getJoinGameTimeout() == 0); // we don't support multiple timers at a time.
        if (mPlayerDataMaster->getJoinGameTimeout() == 0)
        {
            TimeValue timeout = TimeValue::getTimeOfDay() + gGameManagerMaster->getJoinTimeout();
            if (gGameManagerMaster->getPlayerJoinGameTimerSet().scheduleTimer(GamePlayerIdPair(getGameId(), getPlayerId()), timeout))
                mPlayerDataMaster->setJoinGameTimeout(timeout);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief cancel the outstanding joinGame timer (if one exists)
    ***************************************************************************************************/
    void PlayerInfoMaster::cancelJoinGameTimer()
    {
        if (mPlayerDataMaster->getJoinGameTimeout() != 0)
        {
            gGameManagerMaster->getPlayerJoinGameTimerSet().cancelTimer(GamePlayerIdPair(getGameId(), getPlayerId()));
            mPlayerDataMaster->setJoinGameTimeout(0);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief reset (restart) this player's joinGameTimer.
            If the timer expires, the player will be removed from the game.
    ***************************************************************************************************/
    void PlayerInfoMaster::resetJoinGameTimer()
    {
        cancelJoinGameTimer();
        startJoinGameTimer();
    }

    /*!************************************************************************************************/
    /*! \brief callback function for the join game timer -- dispatched when this player's join times out.
    ***************************************************************************************************/
    void PlayerInfoMaster::onJoinGameTimeout(GamePlayerIdPair idPair)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(idPair.first);
        if (gameSession == nullptr)
            return;
        PlayerInfoMaster* player = gameSession->getPlayerRoster()->getPlayer(idPair.second);
        if (player == nullptr)
            return;
            
        // NOTE: Clear out the timer id here, now that it has expired, to avoid trying to cancel expired timers later.
        player->mPlayerDataMaster->setJoinGameTimeout(0);
        
        // remove the player from game
        RemovePlayerMasterRequest request;
        request.setGameId(idPair.first);
        request.setPlayerId(idPair.second);
        request.setPlayerRemovedReason(PLAYER_JOIN_TIMEOUT);
        request.setPlayerRemovedTitleContext(static_cast<PlayerRemovedTitleContext>(player->mPlayerDataMaster->getGameEntryType()));

        TRACE_LOG("join game timed out for player(" << idPair.second << ") in game(" << idPair.first << ")");

        RemovePlayerMasterError::Error removePlayerError;
        gGameManagerMaster->removePlayer(&request, idPair.second, removePlayerError, nullptr);
        if (removePlayerError != RemovePlayerMasterError::ERR_OK)
        {
            TRACE_LOG("failed to remove joining player(" << idPair.second << ") from game(" 
                      << idPair.first << ") - error: " << (ErrorHelp::getErrorName( (BlazeRpcError) removePlayerError)));
        }
    }

    // GM_AUDIT: refactor, consolidate joinGameTimeout w/ pending kick

    /*! ************************************************************************************************/
    /*! \brief start this player's pending kick timer (no-op if already started).
    ***************************************************************************************************/
    void PlayerInfoMaster::startPendingKickTimer(TimeValue timeoutDuration)
    {
        if (mPlayerDataMaster->getPendingKickTimeout() == 0)
        {
            TRACE_LOG("PlayerInfoMaster::startPendingKickTimer: scheduling pending kick for player(" << getPlayerId() << ":" 
                << getPlayerName() << ") in " << timeoutDuration.getSec() << " seconds");

            const TimeValue timeout = TimeValue::getTimeOfDay() + timeoutDuration;
            if (gGameManagerMaster->getPlayerPendingKickTimerSet().scheduleTimer(GamePlayerIdPair(getGameId(), getPlayerId()), timeout))
                mPlayerDataMaster->setPendingKickTimeout(timeout);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief callback for the pending kick timer.  removes the player from the game with reason PLAYER_CONN_LOST
    ***************************************************************************************************/
    void PlayerInfoMaster::onPendingKick(GamePlayerIdPair idPair)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(idPair.first);
        if (gameSession == nullptr)
            return;
        PlayerInfoMaster* player = gameSession->getPlayerRoster()->getPlayer(idPair.second);
        if (player == nullptr)
            return;
            
        // NOTE: Clear out the timer here, now that it has expired, to avoid trying to cancel expired timers later.
        player->mPlayerDataMaster->setPendingKickTimeout(0);
        
        PlayerRemovedReason playerRemovedReason = PLAYER_CONN_LOST;

        // remove the player from game
        RemovePlayerMasterRequest request;
        request.setGameId(idPair.first);
        request.setPlayerId(idPair.second);
        request.setPlayerRemovedReason(playerRemovedReason);

        TRACE_LOG("PlayerInfoMaster::onPendingKick: pending player kick fired for player(" << idPair.second
                  << "); removing from game(" << idPair.first << ") for reason " << PlayerRemovedReasonToString(playerRemovedReason));

        RemovePlayerMasterError::Error removePlayerError;
        gGameManagerMaster->removePlayer(&request, idPair.second, removePlayerError, nullptr);
        if (removePlayerError != RemovePlayerMasterError::ERR_OK)
        {
            TRACE_LOG("PlayerInfoMaster::onPendingKick: failed to remove player(" << idPair.second << ") from game(" 
                      << idPair.first << ") - error: " << (ErrorHelp::getErrorName( (BlazeRpcError) removePlayerError)));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief cancel & remove any pending kick timer event.
    ***************************************************************************************************/
    void PlayerInfoMaster::cancelPendingKickTimer()
    {
        if (mPlayerDataMaster->getPendingKickTimeout() != 0)
        {
            gGameManagerMaster->getPlayerPendingKickTimerSet().cancelTimer(GamePlayerIdPair(getGameId(), getPlayerId()));
            mPlayerDataMaster->setPendingKickTimeout(0);
        }
    }

    void PlayerInfoMaster::startReservationTimer(const TimeValue& origStartTime /*= 0*/, bool disconnect /*= false*/)
    {
        EA_ASSERT(mPlayerDataMaster->getReservationTimeout() == 0); // we don't support multiple timers at a time.
        
        // We don't do a read-only lock here because apparently that isn't allowed if the lock is already mutable...
        const GameSession* gameSession = gGameManagerMaster->getReadOnlyGameSession(getGameId());
        
        // Timeout duration: Either based on a config file version, or the value in the override (set by Scenarios or Game).
        TimeValue timeoutDuration = 0;
        if (disconnect)
        {
            timeoutDuration = gameSession ? gameSession->getDisconnectReservationTimeout() : 0;
            if (timeoutDuration == 0) { timeoutDuration = gGameManagerMaster->getDisconnectReservationTimeout(); }
        }
        else
        {
            timeoutDuration = mPlayerDataMaster->getReservationTimeoutOverride();
            if (timeoutDuration == 0) { timeoutDuration = gameSession ? gameSession->getPlayerReservationTimeout() : 0; }
            if (timeoutDuration == 0) { timeoutDuration = gGameManagerMaster->getPlayerReservationTimeout(); }
        }

        if (mPlayerDataMaster->getReservationTimeout() == 0 && timeoutDuration != 0)
        {
            mPlayerDataMaster->setIsDisconnectReservation(disconnect);
            mPlayerDataMaster->setReservationTimerStart(((origStartTime != 0)?
                origStartTime : TimeValue::getTimeOfDay()));
            if (origStartTime == 0)
            {
                const char8_t* reservationType = (disconnect) ? "disconnect" : "player";
                TRACE_LOG("[PlayerInfoMaster] Player(" << getPlayerId() << ":" << getPlayerName() << ") setting " << reservationType << " reservation timeout in game(" << getGameId() << ") for " << timeoutDuration.getSec() << "s");
            }
            mPlayerData->setReservationCreationTimestamp(TimeValue::getTimeOfDay());

            const TimeValue timeout = mPlayerDataMaster->getReservationTimerStart() + timeoutDuration;
            if (gGameManagerMaster->getPlayerReservationTimerSet().scheduleTimer(GamePlayerIdPair(getGameId(), getPlayerId()), timeout))
                mPlayerDataMaster->setReservationTimeout(timeout);
        }
    }

    void PlayerInfoMaster::cancelReservationTimer()
    {
        if (mPlayerDataMaster->getReservationTimeout() != 0)
        {
            const char8_t* reservationType = (mPlayerDataMaster->getIsDisconnectReservation()) ? "disconnect" : "player";
            TRACE_LOG("[PlayerInfoMaster] Player(" << getPlayerId() << ":" << getPlayerName() << ") cancelling " << reservationType << " reservation timeout for in game(" << getGameId() << ") set to expire at(" << mPlayerDataMaster->getReservationTimeout().getMicroSeconds() << ")");
            
            gGameManagerMaster->getPlayerReservationTimerSet().cancelTimer(GamePlayerIdPair(getGameId(), getPlayerId()));
            mPlayerDataMaster->setReservationTimeout(0);
            mPlayerDataMaster->setReservationTimerStart(0);
            mPlayerData->setReservationCreationTimestamp(NO_RESERVATION_TIMESTAMP);
        }
    }

    void PlayerInfoMaster::onReservationTimeout(GamePlayerIdPair idPair)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(idPair.first);
        if (gameSession == nullptr)
            return;

        PlayerInfoMaster* player = gameSession->getPlayerRoster()->getPlayer(idPair.second);
        if (player == nullptr)
            return;

        // NOTE: Clear out the timer here, now that it has expired, to avoid trying to cancel expired timers later.
        player->mPlayerDataMaster->setReservationTimeout(0);
        
        PlayerRemovedReason reason = (player->mPlayerDataMaster->getIsDisconnectReservation()) ? DISCONNECT_RESERVATION_TIMEOUT : PLAYER_RESERVATION_TIMEOUT;

        TRACE_LOG("[PlayerInfoMaster].onReservationTimeout: Player(" << idPair.second << 
            ") reservation in game(" << idPair.first << ") is being removed due to " << PlayerRemovedReasonToString(reason));

        RemovePlayerMasterRequest request;
        request.setGameId(idPair.first);
        request.setPlayerId(idPair.second);
        request.setPlayerRemovedReason(reason);

        RemovePlayerMasterError::Error removePlayerError;
        gGameManagerMaster->removePlayer(&request, idPair.second, removePlayerError, nullptr);
        if (removePlayerError != RemovePlayerMasterError::ERR_OK)
        {
            TRACE_LOG("[PlayerInfoMaster].onReservationTimeout: failed to remove player(" << idPair.second << ")" << " from game(" << idPair.first << ") - error: " << ErrorHelp::getErrorName( (BlazeRpcError) removePlayerError));
        }
    }

    void PlayerInfoMaster::addToCensusEntry(CensusEntry& entry)
    {
        CensusEntrySet::insert_return_type ret = mCensusEntrySet.insert(&entry);
        if (ret.second)
        {
            ++entry.playerCount;
        }
    }

    void PlayerInfoMaster::removeFromCensusEntries()
    {
        for (CensusEntrySet::const_iterator i = mCensusEntrySet.begin(), e = mCensusEntrySet.end(); i != e; ++i)
        {
            CensusEntry& entry = **i;
            if (entry.playerCount > 0)
                --entry.playerCount;
        }
        mCensusEntrySet.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief start the session's QosValidation timer
    ***************************************************************************************************/
    void PlayerInfoMaster::startQosValidationTimer()
    {
        EA_ASSERT(mPlayerDataMaster->getQosValidationTimeout() == 0); // we don't support multiple timers at a time.
        if ((mPlayerDataMaster->getQosValidationTimeout() == 0) && (gGameManagerMaster->getMmQosValidationTimeout() != 0))//0 disables
        {
            TimeValue timeout = TimeValue::getTimeOfDay() + gGameManagerMaster->getMmQosValidationTimeout();
            if (gGameManagerMaster->getPlayerQosValidationTimerSet().scheduleTimer(GamePlayerIdPair(getGameId(), getPlayerId()), timeout))
                mPlayerDataMaster->setQosValidationTimeout(timeout);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief cancel the outstanding QosValidation timer (if one exists)
    ***************************************************************************************************/
    void PlayerInfoMaster::cancelQosValidationTimer()
    {
        if (mPlayerDataMaster->getQosValidationTimeout() != 0)
        {
            gGameManagerMaster->getPlayerQosValidationTimerSet().cancelTimer(GamePlayerIdPair(getGameId(), getPlayerId()));
            mPlayerDataMaster->setQosValidationTimeout(0);
        }
    }

    /*!************************************************************************************************/
    /*! \brief callback function for the qos validation timer -- dispatched when this player's qos validation times out.
    ***************************************************************************************************/
    void PlayerInfoMaster::onQosValidationTimeout(GamePlayerIdPair idPair)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(idPair.first);
        if (gameSession == nullptr)
            return;
        PlayerInfoMaster* player = gameSession->getPlayerRoster()->getPlayer(idPair.second);
        if (player == nullptr)
            return;
            
        // NOTE: Clear out the timer id here, now that it has expired, to avoid trying to cancel expired timers later.
        player->mPlayerDataMaster->setQosValidationTimeout(0);
        
        // remove the player from game
        RemovePlayerMasterRequest request;
        request.setGameId(idPair.first);
        request.setPlayerId(idPair.second);
        request.setPlayerRemovedReason(PLAYER_CONN_POOR_QUALITY);//treat this as lack of a connection. Logging below differentiates this.
        request.setPlayerRemovedTitleContext(static_cast<PlayerRemovedTitleContext>(player->mPlayerDataMaster->getGameEntryType()));

        WARN_LOG("qos validation timed out for player(" << idPair.second << ") in game(" << idPair.first << "). An update mesh connection may not have been recieved for this player. Player will be removed.");
        // note: the player's removal will clean up timer and qos data
        RemovePlayerMasterError::Error removePlayerError;
        gGameManagerMaster->removePlayer(&request, idPair.second, removePlayerError, nullptr);
        if (removePlayerError != RemovePlayerMasterError::ERR_OK)
        {
            TRACE_LOG("failed to remove joining player(" << idPair.second << ") from game(" 
                      << idPair.first << ") - error: " << (ErrorHelp::getErrorName( (BlazeRpcError) removePlayerError)));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return player's PIN/connection metric join type
    ***************************************************************************************************/
    ConnectionJoinType PlayerInfoMaster::getConnectionJoinType() const
    {
        JoinMethod joinMethod = getJoinMethod();
        if (getJoinedViaMatchmaking() || (joinMethod == JOIN_BY_MATCHMAKING))
        {
            return MATCHMAKING;
        }
        else if ((joinMethod == SYS_JOIN_BY_CREATE) || (joinMethod == SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME) || (joinMethod == SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME_HOST) ||
            (joinMethod == SYS_JOIN_BY_RESETDEDICATEDSERVER) || (joinMethod == SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER))
        {
            return CREATED;
        }
        return DIRECTJOIN;
    }

    void intrusive_ptr_add_ref(PlayerInfoMaster* ptr) { intrusive_ptr_add_ref((PlayerInfo*)ptr); }
    void intrusive_ptr_release(PlayerInfoMaster* ptr) { intrusive_ptr_release((PlayerInfo*)ptr); }

}  // namespace GameManager
}  // namespace Blaze


