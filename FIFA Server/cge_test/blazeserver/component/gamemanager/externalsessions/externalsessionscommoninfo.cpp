/*! ************************************************************************************************/
/*!
    \file   externalsessionscommoninfo.cpp

    Helpers for populating the data passed between GM/MM and ExternalSessionUtil.

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/matchmaker_config_server.h"
#include "gamemanager/rpc/gamemanager_defines.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "component/gamemanager/externalsessions/externalsessionutilmanager.h"
#include "gamemanager/gamesessionmaster.h" 
#include "proxycomponent/xblserviceconfigs/rpc/xblserviceconfigs_defines.h" 
#include "gamemanager/rpc/matchmaker_defines.h" 
#include "proxycomponent/psnbaseurl/rpc/psnbaseurl_defines.h" 
#include "proxycomponent/psnsessioninvitation/rpc/psnsessioninvitation_defines.h"
#include "proxycomponent/psnsessionmanager/rpc/psnsessionmanager_defines.h"
#include "proxycomponent/psnmatches/rpc/psnmatches_defines.h"
#include "gamemanager/tdf/gamemanager_server.h" 
#include "framework/rpc/oauth_defines.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameManager
{
    uint64_t getNextGameExternalSessionUniqueId()
    { 
        uint64_t id = 0;
        BlazeRpcError rc = ((gUniqueIdManager == nullptr)? ERR_SYSTEM :
            gUniqueIdManager->getId(RpcMakeSlaveId(GameManagerMaster::COMPONENT_ID), GAMEMANAGER_IDTYPE_EXTERNALSESSION, id));
        if (rc != ERR_OK)
        {
            WARN_LOG("getNextGameExternalSessionUniqueId: failed to get the next id. Err: " << ErrorHelp::getErrorName(rc));
        }
        return id;
    }
    
    /*! \brief convert the error code from external service util to game manager specific error */
    BlazeRpcError convertExternalServiceErrorToGameManagerError(BlazeRpcError err)
    {
        switch (err)
        {
        case Blaze::ERR_OK:
        case Blaze::ERR_COULDNT_CONNECT:
            return err;
        
        case Blaze::GAMEMANAGER_ERR_EXTERNAL_SESSION_IMAGE_INVALID:
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_IMAGE_INVALID;
        
        case Blaze::XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED:
            return GAMEMANAGER_ERR_SESSION_TEMPLATE_NOT_SUPPORTED;
        
        case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_VISIBLE:
        case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_PRIVATE:
            return GAMEMANAGER_ERR_FAILED_DUE_TO_PRESENCE_MODE_RESTRICTION;
        
        case Blaze::XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_FOLLOWED:
            return GAMEMANAGER_ERR_FAILED_DUE_TO_FRIENDS_ONLY_RESTRICTION;
        
        case Blaze::PSNBASEURL_TOO_MANY_REQUESTS:
        case Blaze::PSNSESSIONINVITATION_TOO_MANY_REQUESTS:
        case Blaze::PSNSESSIONMANAGER_TOO_MANY_REQUESTS:
        case Blaze::PSNMATCHES_TOO_MANY_REQUESTS:
        case Blaze::GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY:
            return GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY;
        
        case Blaze::GAMEMANAGER_ERR_EXTERNAL_SESSION_CUSTOM_DATA_TOO_LARGE:
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_CUSTOM_DATA_TOO_LARGE;
        
        case Blaze::OAUTH_ERR_INVALID_SANDBOX_ID:
            return GAMEMANAGER_ERR_EXTERNALSESSION_INVALID_SANDBOX_ID;
        
        default:
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR;
        }
    }

    bool shouldRetryAfterExternalSessionUpdateError(BlazeRpcError err)
    {
        //ACCESS_FORBIDDEN typically means server just removed user from session before call finished. Allow retrying w/another caller
        return ((err == GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY) || (err == PSNBASEURL_TOO_MANY_REQUESTS) ||
            (err == PSNSESSIONINVITATION_TOO_MANY_REQUESTS) || (err == PSNSESSIONINVITATION_ACCESS_FORBIDDEN) ||
            (err == PSNSESSIONMANAGER_TOO_MANY_REQUESTS) || (err == PSNSESSIONMANAGER_ACCESS_FORBIDDEN));//side: PSN MATCHES not caller based, retrying at *GM* level unhelpful/omitted here
    }

    bool isExternalSessionJoinRestrictionError(BlazeRpcError err)
    {
        return ((err == GAMEMANAGER_ERR_FAILED_DUE_TO_FRIENDS_ONLY_RESTRICTION) || (err == GAMEMANAGER_ERR_FAILED_DUE_TO_PRESENCE_MODE_RESTRICTION));
    }

    /*! \brief convert the error code from external service util to matchmaking specific error */
    BlazeRpcError convertExternalServiceErrorToMatchmakingError(BlazeRpcError err)
    {
        switch (err)
        {
        case Blaze::ERR_OK:
        case Blaze::ERR_COULDNT_CONNECT:
            return err;
        
        case Blaze::XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED:
            return MATCHMAKER_ERR_SESSION_TEMPLATE_NOT_SUPPORTED;
        
        default:
            return MATCHMAKER_ERR_EXTERNAL_SESSION_ERROR;
        }
    }

    /*! \brief get the GameType's appropriate ExternalSessionType */
    ExternalSessionType gameTypeToExternalSessionType(GameType gameType)
    {
        return ((gameType == GAME_TYPE_GROUP)? EXTERNAL_SESSION_GAME_GROUP : EXTERNAL_SESSION_GAME);
    }

    /*! \brief get the ExternalSessionType's GameType */
    GameType externalSessionTypeToGameType(ExternalSessionType externalSessionType)
    {
        return ((externalSessionType == EXTERNAL_SESSION_GAME_GROUP)? GAME_TYPE_GROUP : GAME_TYPE_GAMESESSION);
    }

    /*! ************************************************************************************************/
    /*! \brief Set up the slave-to-master request's external session data, to cache/init on the master game as needed.
        \param[in] sessionTemplateName needs to be non empty if not using the default
    ***************************************************************************************************/
    BlazeRpcError initRequestExternalSessionData(ExternalSessionMasterRequestData& req, const ExternalSessionIdentification& externalSessionIdentSetup, GameType gameType,
        const UserJoinInfoList& usersInfo, const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& externalSessionUtilMgr, GameManagerSlaveImpl& slaveImpl)
    {
        return initRequestExternalSessionData(req, externalSessionIdentSetup, gameType, usersInfo, gameSessionConfig, externalSessionUtilMgr, &slaveImpl);
    }
    BlazeRpcError initRequestExternalSessionData(ExternalSessionMasterRequestData& req, const ExternalSessionIdentification& externalSessionIdentSetup, GameType gameType,
        const UserJoinInfoList& usersInfo, const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& externalSessionUtilMgr, GameManagerSlaveImpl* slaveImpl)
    {
        // Start with req's specified params. This is done for all platforms now:
        externalSessionIdentSetup.copyInto(req.getExternalSessionIdentSetupMaster());
        // Xbox specific implementation below:

        const UserSessionInfo* caller = getFirstExternalSessionJoinerInfo(usersInfo, xone);
        if (caller == nullptr)
        {
            //pre: only need one user from any Xbox platform to populate all needed values below
            caller = getFirstExternalSessionJoinerInfo(usersInfo, xbsx);
        }
        if (caller == nullptr)
        {
            TRACE_LOG("[initRequestExternalSessionData] skipping, unneeded for current join request.");
            return ERR_OK;
        }

        BlazeRpcError err = initRequestExternalSessionDataXbox(req, externalSessionIdentSetup.getXone().getTemplateName(), gameType, *caller, gameSessionConfig, externalSessionUtilMgr);
        if (err != ERR_OK)
        {
            req.getJoinInitErr().getPlatformRpcErrorMap()[caller->getUserInfo().getPlatformInfo().getClientPlatform()] = err;
            if (slaveImpl)
                slaveImpl->incExternalSessionFailureImpacting(caller->getUserInfo().getPlatformInfo().getClientPlatform());
            if (err == ERR_COULDNT_CONNECT)
            {
                gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::MPSD_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
            }
            // partial fails only allowed if there's other non Xbox platforms
            bool hasNonXblUsers = false;
            for (const auto& userJoinInfo : usersInfo)
            {
                auto nextPlat = userJoinInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform();
                if (((nextPlat != xone) && (nextPlat != xbsx)) && gController->isPlatformHosted(nextPlat))
                {
                    hasNonXblUsers = true;
                    break;
                }
            }
            req.getJoinInitErr().setFailedOnAllPlatforms(!hasNonXblUsers);
        }

        return err;
    }

    BlazeRpcError initRequestExternalSessionDataXbox(ExternalSessionMasterRequestData& req, const char8_t* sessionTemplateName, GameType gameType,
        const UserSessionInfo& caller, const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& extSessionUtilManager)
    {
        ClientPlatformType callerPlatform = caller.getUserInfo().getPlatformInfo().getClientPlatform();
        if (((callerPlatform != xone) && (callerPlatform != xbsx)) || !caller.getHasExternalSessionJoinPermission())
        {
            ERR_LOG("[initRequestExternalSessionData] unable to init request external session data: caller " << (caller.getHasExternalSessionJoinPermission() ? "has" : "does not have") << " permission to join external sessions, is on platform(" << ClientPlatformTypeToString(callerPlatform) << ").");
            return ERR_SYSTEM;
        }
        ExternalSessionUtil* externalSessionUtil = extSessionUtilManager.getUtil(callerPlatform);
        if (externalSessionUtil == nullptr)
        {
            ERR_LOG("[initRequestExternalSessionData] unable to init request external session data: external session util is nullptr, for platform(" << ClientPlatformTypeToString(callerPlatform) << ")");
            return ERR_SYSTEM;
        }

        eastl::string sessionName;
        BlazeRpcError err = externalSessionUtil->getNextGameExternalSessionName(sessionName);
        if (err != ERR_OK)
            return err;
        ExternalSessionResult result;
        // don't commit external session here, just 'trial swing' to retrieve correlation id.
        bool commit = false;
        CreateExternalSessionParameters params;
        caller.getExternalAuthInfo().copyInto(params.getExternalUserJoinInfo().getAuthInfo());
        UserInfo::filloutUserIdentification(caller.getUserInfo(), params.getExternalUserJoinInfo().getUserIdentification());
        params.setXblSessionTemplateName(sessionTemplateName);
        params.setXblSessionName(sessionName.c_str());
        params.getExternalSessionCreationInfo().setExternalSessionType(gameTypeToExternalSessionType(gameType));
        params.getExternalSessionCreationInfo().setExternalSessionId(INVALID_GAME_ID);
 
        err = externalSessionUtil->create(params, &result, commit);
        if (err == ERR_OK)
            result.getSessionIdentification().getXone().copyInto(req.getExternalSessionIdentSetupMaster().getXone());

        return convertExternalServiceErrorToGameManagerError(err);
    }

    void setExternalUserJoinInfoFromUserSessionInfo(ExternalUserJoinInfo& externalInfo, const UserSessionInfo& userInfo, bool isReserved)
    {
        if (EA_UNLIKELY(!userInfo.getHasExternalSessionJoinPermission()))
        {
            ERR_LOG("[setExternalUserJoinInfoFromUserSessionInfo] warn: user session info does not have permission to join external sessions");
            return;
        }

        UserInfo::filloutUserIdentification(userInfo.getUserInfo(), externalInfo.getUserIdentification());
        externalInfo.setReserved(isReserved);
        userInfo.getExternalAuthInfo().copyInto(externalInfo.getAuthInfo());
    }

    /*! brief populate the game's info for initializing its external session properties. Note: We decouple GM from knowledge of info's mappings to the actual first party specific external session properties here. The actual mappings are done at the external session util. */
    void setExternalSessionCreationInfoFromGame(ExternalSessionCreationInfo& createInfo, const GameSessionMaster& gameSession)
    {
        createInfo.setExternalSessionId(gameSession.getGameId());
        createInfo.setExternalSessionType(gameTypeToExternalSessionType(gameSession.getGameType()));
        gameSession.getGameDataMaster()->getExternalSessionCustomData().copyInto(createInfo.getExternalSessionCustomData());
        gameSession.getGameDataMaster()->getTournamentIdentification().copyInto(createInfo.getTournamentIdentification());
        gameSession.getGameDataMaster()->getTournamentSessionData().copyInto(createInfo.getTournamentInfo());
        setExternalSessionUpdateInfoFromGame(createInfo.getUpdateInfo(), gameSession);
    }
    
    /*! brief populate the game's update info for updating its external session properties. Note: We decouple GM from knowledge of info's mappings to the actual first party specific external session properties here. The actual mappings are done at the external session util. */
    void setExternalSessionUpdateInfoFromGame(ExternalSessionUpdateInfo& updateInfo, const GameSessionMaster& gameSession)
    {
        uint16_t maxUsers = 0;

        for (const auto& platform : gController->getHostedPlatforms())
            updateInfo.getPresenceModeByPlatform()[platform] = gameSession.getPresenceMode();

        for (const auto& platform : gameSession.getPresenceDisabledList())
        {
            ExternalSessionUpdateInfo::PresenceModeByPlatformMap::iterator it = updateInfo.getPresenceModeByPlatform().find(platform);
            if (it != updateInfo.getPresenceModeByPlatform().end())
                it->second = PRESENCE_MODE_NONE;
        }

        // The FullMap seems dumb, but the alternative is to send over the full roster data (or hook up to the normal event system)
        gameSession.getExternalSessionUtilManager().isFullForExternalSessions(gameSession, maxUsers, updateInfo.getFullMap());
        updateInfo.setMaxUsers(maxUsers);
        gameSession.getSlotCapacities().copyInto(updateInfo.getSlotCapacities());
        gameSession.getExternalSessionIdentification().copyInto(updateInfo.getSessionIdentification());
        updateInfo.setGameName(gameSession.getGameName());
        updateInfo.setGameMode(gameSession.getGameMode());
        gameSession.getGameDataMaster()->getExternalSessionStatus().copyInto(updateInfo.getStatus());
        updateInfo.getSettings().setBits(gameSession.getGameSettings().getBits());
        updateInfo.setState(gameSession.getGameState());
        updateInfo.setSimplifiedState(getSimplifiedGamePlayState(gameSession));
        gameSession.getTeamIds().copyInto(updateInfo.getTeamIdsByIndex());
        updateInfo.setGameType(gameSession.getGameType());
        updateInfo.setGameId(gameSession.getGameId());
        gameSession.getGameDataMaster()->getTrackedExternalSessionMembers().copyInto(updateInfo.getTrackedExternalSessionMembers());
        updateInfo.getDnfPlayers().clear();
        updateInfo.getDnfPlayers().reserve(gameSession.getGameDataMaster()->getDnfPlayerList().size());
        for (auto itr : gameSession.getGameDataMaster()->getDnfPlayerList())
        {
            itr->getUserIdentification().copyInto(*updateInfo.getDnfPlayers().pull_back());
        }
        updateInfo.getTeamNameByIndex().clear();
        updateInfo.getTeamNameByIndex().reserve(gameSession.getTeamCount());
        for (TeamIndex i = 0; i < gameSession.getTeamCount(); ++i)
        {
            const char8_t* teamName = gameSession.getTeamName(i);
            if (teamName)
                updateInfo.getTeamNameByIndex().insert(eastl::make_pair(i, TeamName(teamName)));
        }
    }

    /*! brief whether the external session properties update is for the final snapshot before game cleanup. */
    bool isFinalExtSessUpdate(const ExternalSessionUpdateEventContext& context)
    {
        return (context.getFinalUpdateContext().getGameId() != GameManager::INVALID_GAME_ID);
    }

    /*! brief get the game session's simplified game play state. */
    SimplifiedGamePlayState getSimplifiedGamePlayState(const GameSessionMaster& gameSession)
    {
        if (gameSession.getGameType() == GAME_TYPE_GROUP)
        {
            if ((gameSession.getGameState() == INITIALIZING) || (gameSession.getGameDataMaster()->getPreHostMigrationState() == INITIALIZING))
                return GAME_PLAY_STATE_PRE_GAME;
            else if ((gameSession.getGameState() == GAME_GROUP_INITIALIZED) || (gameSession.getGameDataMaster()->getPreHostMigrationState() == GAME_GROUP_INITIALIZED))
                return GAME_PLAY_STATE_IN_GAME;
        }
        else if (gameSession.isGamePostMatch() || gameSession.getGameDataMaster()->getGameFinished())//Do before RESETABLE check below, as GameFinished maybe RESETABLE.
            return GAME_PLAY_STATE_POST_GAME;
        else if (gameSession.isGamePreMatch() || (gameSession.getGameState() == RESETABLE))
            return GAME_PLAY_STATE_PRE_GAME;
        else if (gameSession.isGameInProgress())
            return GAME_PLAY_STATE_IN_GAME;
        WARN_LOG("[getSimplifiedGamePlayState] can't resolve a simplified state for game(" << gameSession.getGameId() << "), in GameState(" << GameStateToString(gameSession.getGameState()) << ")");
        return GAME_PLAY_STATE_INVALID;
    }


    /*! brief append user to param's user list, and add any other missing fields for the external session update, from game session. */
    void updateExternalSessionJoinParams(JoinExternalSessionParameters& params, const UserSessionInfo& userToAdd, bool isReserved, const GameSessionMaster& gameSession)
    {
        ExternalId extId = getExternalIdFromPlatformInfo(userToAdd.getUserInfo().getPlatformInfo());
        if (!userToAdd.getHasExternalSessionJoinPermission())
        {
            TRACE_LOG("[updateExternalSessionJoinParams] note user (" << userToAdd.getUserInfo().getId() << "), extid(" << extId << ") does not have permission to join external sessions");
            return;
        }

        for (const auto& exUserJoinInfo : params.getExternalUserJoinInfos())
        {
            if (exUserJoinInfo->getUserIdentification().getBlazeId() == userToAdd.getUserInfo().getId())
            {
                TRACE_LOG("[updateExternalSessionJoinParams] user (" << userToAdd.getUserInfo().getId() << "), extid(" << extId << ") is already in the list. Skip!");
                return;
            }
        }

        setExternalUserJoinInfoFromUserSessionInfo(*params.getExternalUserJoinInfos().pull_back(), userToAdd, isReserved);

        // will need the session name, id, etc as well if missing.
        if (!isExtSessIdentSet(params.getSessionIdentification()))
        {
            gameSession.getExternalSessionIdentification().copyInto(params.getSessionIdentification());
            setExternalSessionCreationInfoFromGame(params.getExternalSessionCreationInfo(), gameSession);
        }
        // unlike constants above, ensure got the 'end' properties values for a group create by re-updating it here.
        setExternalSessionUpdateInfoFromGame(params.getExternalSessionCreationInfo().getUpdateInfo(), gameSession);

        bool doSetGroupToken = ((params.getGroupExternalSessionToken() != nullptr) && (params.getGroupExternalSessionToken()[0] == '\0'));
        if (doSetGroupToken)
            params.setGroupExternalSessionToken(userToAdd.getExternalAuthInfo().getCachedExternalSessionToken());

        TRACE_LOG("[updateExternalSessionJoinParams] added user(blazeId=" << userToAdd.getUserInfo().getId() << ", extid=" << extId << "), platform("
            << ClientPlatformTypeToString(userToAdd.getUserInfo().getPlatformInfo().getClientPlatform()) << ") to external session join parameters, for game(" << gameSession.getGameId()
            << "), ext session(" << toLogStr(params.getSessionIdentification()) << "), group token(" << userToAdd.getExternalAuthInfo().getCachedExternalSessionToken() << ")");
    }

    /*! brief Helper for adding copy of users to another result's list. Note: this *appends* users to the target */
    void externalSessionJoinParamsToOtherResponse(const JoinExternalSessionParameters& source, JoinExternalSessionParameters& target)
    {
        // append users
        for (ExternalUserJoinInfoList::const_iterator itr = source.getExternalUserJoinInfos().begin(); itr != source.getExternalUserJoinInfos().end(); ++itr)
            (*itr)->copyInto(*target.getExternalUserJoinInfos().pull_back());

        if (source.getExternalUserJoinInfos().empty())
            return;

        // will need the session name, id, etc as well if missing.
        if (!isExtSessIdentSet(target.getSessionIdentification()))
        {
            source.getSessionIdentification().copyInto(target.getSessionIdentification());
            source.getExternalSessionCreationInfo().copyInto(target.getExternalSessionCreationInfo());
        }
        // unlike constants above, ensure got the 'end' properties values for a group create by re-updating it here.
        source.getExternalSessionCreationInfo().getUpdateInfo().copyInto(target.getExternalSessionCreationInfo().getUpdateInfo());

        bool doSetGroupToken = ((target.getGroupExternalSessionToken() != nullptr) && (target.getGroupExternalSessionToken()[0] == '\0'));
        if (doSetGroupToken)
            target.setGroupExternalSessionToken(source.getGroupExternalSessionToken());

        TRACE_LOG("[externalSessionJoinParamsToOtherResponse] added " << source.getExternalUserJoinInfos().size() << " users to external session join parameters, for external session id(" << target.getExternalSessionCreationInfo().getExternalSessionId() << "), ext session(" << toLogStr(target.getSessionIdentification()) << "), group token(" << target.getGroupExternalSessionToken() << "). Current users size(" << target.getExternalUserJoinInfos().size() << ")");
    }

    /*! brief clear param's fields so that they can be re-set via updateExternalSessionJoinParams() */
    void clearExternalSessionJoinParams(JoinExternalSessionParameters& params)
    {
        params.getSessionIdentification().getXone().setTemplateName("");
        params.getSessionIdentification().getXone().setSessionName("");
        params.getSessionIdentification().getXone().setCorrelationId("");
        params.getSessionIdentification().getPs4().setNpSessionId("");
        params.getSessionIdentification().getPs5().getMatch().setActivityObjectId("");
        params.getSessionIdentification().getPs5().getMatch().setMatchId("");
        params.getSessionIdentification().getPs5().getPlayerSession().setPlayerSessionId("");
        params.setGroupExternalSessionToken("");

        params.getExternalUserJoinInfos().clear();
        params.getExternalSessionCreationInfo().setExternalSessionId(INVALID_GAME_ID);
    }

    void setExternalUserLeaveInfoFromUserSessionInfo(ExternalUserLeaveInfo& externalInfo, const UserSessionInfo& userInfo)
    {
        if (EA_UNLIKELY(!userInfo.getHasExternalSessionJoinPermission()))
        {
            ERR_LOG("[setExternalUserLeaveInfoFromUserSessionInfo] warn: user session info does not have permission to join external sessions");
            return;
        }
        UserInfo::filloutUserIdentification(userInfo.getUserInfo(), externalInfo.getUserIdentification());
    }

    /*! brief append user to param's user list, and add any other missing fields for the external session update, from game session. */
    void updateExternalSessionLeaveParams(LeaveGroupExternalSessionParameters& params, const UserSessionInfo& userToAdd, const ReplicatedGameData& gameSession)
    {
        ExternalId extId = getExternalIdFromPlatformInfo(userToAdd.getUserInfo().getPlatformInfo());
        if (!userToAdd.getHasExternalSessionJoinPermission())
        {
            TRACE_LOG("[updateExternalSessionLeaveParams] note user " << userToAdd.getUserInfo().getId() << "(extid=" << extId << ") does not have permission to join external sessions");
            return;
        }

        setExternalUserLeaveInfoFromUserSessionInfo(*params.getExternalUserLeaveInfos().pull_back(), userToAdd);

        // will need the session name, etc as well if missing.
        if (!isExtSessIdentSet(params.getSessionIdentification()))
        {
            gameSession.getExternalSessionIdentification().copyInto(params.getSessionIdentification());
        }

        TRACE_LOG("[updateExternalSessionLeaveParams] added user " << userToAdd.getUserInfo().getId() << "(extid=" << extId << ") to external session leave parameters, for game(" << gameSession.getGameId() << "), ext session(" << toLogStr(params.getSessionIdentification()) << ")");
    }

    /*! brief overload based on user's external id. */
    void updateExternalSessionLeaveParams(LeaveGroupExternalSessionParameters& params, ExternalId externalId, const ReplicatedGameData& gameSession, ClientPlatformType platform)
    {
        ExternalUserLeaveInfo* externalInfo = params.getExternalUserLeaveInfos().pull_back();
        convertToPlatformInfo(externalInfo->getUserIdentification().getPlatformInfo(), externalId, nullptr, INVALID_ACCOUNT_ID, platform);

        // will need the session name, etc as well if missing.
        if (!isExtSessIdentSet(params.getSessionIdentification()))
        {
            gameSession.getExternalSessionIdentification().copyInto(params.getSessionIdentification());
        }
        TRACE_LOG("[updateExternalSessionLeaveParams] added " << ClientPlatformTypeToString(platform) << " user extid=" << externalId << " to external session leave parameters, for game(" << gameSession.getGameId() << "), ext session(" << toLogStr(params.getSessionIdentification()) << ")");
    }

    /*! brief whether error from an external session join attempt indicates external session possibly got out of sync with Blaze game.*/
    bool shouldResyncExternalSessionMembersOnJoinError(BlazeRpcError error)
    {
        return (error == XBLSERVICECONFIGS_BAD_REQUEST);
    }

    /*! brief whether the external session identification is initialized/set. */
    bool isExtSessIdentSet(const ExternalSessionIdentification& sessionIdentification)
    {
        return ((sessionIdentification.getPs4().getNpSessionId()[0] != '\0') ||
            ((sessionIdentification.getXone().getTemplateName()[0] != '\0') && (sessionIdentification.getXone().getSessionName()[0] != '\0')) ||
            ((sessionIdentification.getPs5().getMatch().getMatchId()[0] != '\0') || (sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId()[0] != '\0')));
    }

    /*! brief whether the external session identification is initialized/set, for the platform activity type. */
    bool isExtSessIdentSet(const ExternalSessionIdentification& sessionIdentification, const ExternalSessionActivityTypeInfo& forType)
    {
        switch (forType.getPlatform())
        {
        case ps4:
            return (sessionIdentification.getPs4().getNpSessionId()[0] != '\0');
            break;
        case xone:
        case xbsx:
            return ((sessionIdentification.getXone().getTemplateName()[0] != '\0') && (sessionIdentification.getXone().getSessionName()[0] != '\0'));
        case ps5:
            switch (forType.getType())
            {
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY:
                return (sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId()[0] != '\0');
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH:
                return (sessionIdentification.getPs5().getMatch().getMatchId()[0] != '\0');
            default:
                ERR_LOG("[isExtSessIdentSet] unhandled activityType(" << forType.getType() << ") for platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
            }
            break;
        default:
            ERR_LOG("[isExtSessIdentSet] unhandled platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
        }
        return false;
    }

    /*! brief whether the external session identifications have equivalent values, for the platform activity type. */
    bool isExtSessIdentSame(const ExternalSessionIdentification& a, const ExternalSessionIdentification& b, const ExternalSessionActivityTypeInfo& forType)
    {
        switch (forType.getPlatform())
        {
        case ps4:
            return (blaze_strcmp(a.getPs4().getNpSessionId(), b.getPs4().getNpSessionId()) == 0);
        case xone:
        case xbsx:
            return ((blaze_strcmp(a.getXone().getTemplateName(), b.getXone().getTemplateName()) == 0) && (blaze_strcmp(a.getXone().getSessionName(), b.getXone().getSessionName()) == 0));
        case ps5:
            switch (forType.getType())
            {
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY:
                return (blaze_strcmp(a.getPs5().getPlayerSession().getPlayerSessionId(), b.getPs5().getPlayerSession().getPlayerSessionId()) == 0);
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH:
                return (blaze_strcmp(a.getPs5().getMatch().getMatchId(), b.getPs5().getMatch().getMatchId()) == 0);
            default:
                ERR_LOG("[isExtSessIdentSame] unhandled activityType(" << forType.getType() << ") for platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
            }
            return false;
        default:
            ERR_LOG("[isExtSessIdentSame] unhandled platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
        }
        return false;
    }

    /*! brief copies one external session identification's values, for the platform activity type, to another. */
    void setExtSessIdent(ExternalSessionIdentification& to, const ExternalSessionIdentification& from, const ExternalSessionActivityTypeInfo& forType)
    {
        switch (forType.getPlatform())
        {
        case ps4:
            from.getPs4().copyInto(to.getPs4());
            return;
        case xone:
        case xbsx:
            from.getXone().copyInto(to.getXone());
            return;
        case ps5:
            switch (forType.getType())
            {
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY:
                from.getPs5().getPlayerSession().copyInto(to.getPs5().getPlayerSession());
                return;
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH:
                from.getPs5().getMatch().copyInto(to.getPs5().getMatch());
                return;
            default:
                ERR_LOG("[setExtSessIdent] unhandled activityType(" << forType.getType() << ") for platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
            }
            return;
        default:
            ERR_LOG("[setExtSessIdent] unhandled platform(" << ClientPlatformTypeToString(forType.getPlatform()) << ")");
        }
    }

    /*! brief fetch the list by platform activity type, or insert new record if missing. */
    ExternalMemberInfoList& getOrAddListInMap(ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType)
    {
        auto byPlatIt = map.find(forType.getPlatform());
        if (byPlatIt == map.end())
        {
            auto* newByPlat = map.allocate_element();
            byPlatIt = map.insert(eastl::make_pair(forType.getPlatform(), newByPlat)).first;
        }
        auto byTypeIt = byPlatIt->second->find(forType.getType());
        if (byTypeIt == byPlatIt->second->end())
        {
            auto* newByType = byPlatIt->second->allocate_element();
            byTypeIt = byPlatIt->second->insert(eastl::make_pair(forType.getType(), newByType)).first;
        }
        return *byTypeIt->second;
    }
    ExternalMemberInfoList* getListFromMap(const ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType)
    {
        auto byPlatIt = map.find(forType.getPlatform());
        if (byPlatIt == map.end())
        {
            return nullptr;
        }
        auto byTypeIt = byPlatIt->second->find(forType.getType());
        if (byTypeIt == byPlatIt->second->end())
            return nullptr;
        return byTypeIt->second;
    }

    /*! brief whether there is a non empty members list for the platform activity type in the map. */
    bool hasExternalMembersInMap(const ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType)
    {
        auto* memberList = getListFromMap(map, forType);
        return ((memberList != nullptr) && !memberList->empty());
    }

    /*! brief transfer deprecated external session parameters to the ExternalSessionIdentification. For backward compatibility with older clients.*/
    void oldToNewExternalSessionIdentificationParams(const char8_t* sessionTemplate, const char8_t* externalSessionName, const char8_t* externalSessionCorrelationId,
        const char8_t* npSessionId, ExternalSessionIdentification& newParam)
    {
        if ((sessionTemplate != nullptr) && (sessionTemplate[0] != '\0') && (newParam.getXone().getTemplateName()[0] == '\0'))
            newParam.getXone().setTemplateName(sessionTemplate);
        if ((externalSessionName != nullptr) && (externalSessionName[0] != '\0') && (newParam.getXone().getSessionName()[0] == '\0'))
            newParam.getXone().setSessionName(externalSessionName);
        if ((externalSessionCorrelationId != nullptr) && (externalSessionCorrelationId[0] != '\0') && (newParam.getXone().getCorrelationId()[0] == '\0'))
            newParam.getXone().setCorrelationId(externalSessionCorrelationId);
        if ((npSessionId != nullptr) && (npSessionId[0] != '\0') && (newParam.getPs4().getNpSessionId()[0] == '\0'))
            newParam.getPs4().setNpSessionId(npSessionId);
    }

    /*! brief transfer the replicated game data's ExternalSessionIdentification to its deprecated external session members. For backward compatibility with older clients. Pre: replicatedGameData's mExternalSessionIdentification has been populated.*/
    void newToOldReplicatedExternalSessionData(ReplicatedGameData& replicatedGameData)
    {
        const ExternalSessionIdentification& newData = replicatedGameData.getExternalSessionIdentification();
        replicatedGameData.setExternalSessionTemplateName(newData.getXone().getTemplateName());
        replicatedGameData.setExternalSessionName(newData.getXone().getSessionName());
        replicatedGameData.setExternalSessionCorrelationId(newData.getXone().getCorrelationId());
        replicatedGameData.setNpSessionId(newData.getPs4().getNpSessionId());
    }

    /*! brief transfer deprecated external session GameActivity parameters to the new members. For backward compatibility with older clients.*/
    void oldToNewGameActivityParams(GameActivity& gameActivity)
    {
        oldToNewExternalSessionIdentificationParams(gameActivity.getExternalSessionTemplateName(), gameActivity.getExternalSessionName(), nullptr, nullptr, gameActivity.getSessionIdentification());
        if ((gameActivity.getPlayer().getPlayerState() == RESERVED) && (gameActivity.getPlayerState() != RESERVED))//(unset default is RESERVED)
            gameActivity.getPlayer().setPlayerState(gameActivity.getPlayerState());
        if ((gameActivity.getPlayer().getJoinedGameTimestamp().getMicroSeconds() == 0) && (gameActivity.getJoinedGameTimestamp().getMicroSeconds() != 0))
            gameActivity.getPlayer().setJoinedGameTimestamp(gameActivity.getJoinedGameTimestamp());
    }

    /*! brief transfer new external session GameActivity parameters to the deprecated members. For backward compatibility with older clients.*/
    void newToOldGameActivityResults(UpdatePrimaryExternalSessionForUserResult& result)
    {
        result.setCurrentPrimaryGameId(result.getCurrentPrimary().getGameId());
        result.getCurrentPrimary().getSession().copyInto(result.getCurrentPrimarySession());
    }
    void newToOldGameActivityResults(UpdatePrimaryExternalSessionForUserErrorResult& result)
    {
        result.setCurrentPrimaryGameId(result.getCurrentPrimary().getGameId());
    }

    /*! brief return the ExternalSessionIdentification's string for logging.*/
    const Blaze::FixedString256 toLogStr(const ExternalSessionIdentification& sessionIdentification)
    {
        Blaze::FixedString256 sessionStr;
        if (sessionIdentification.getPs4().getNpSessionId()[0] != '\0')
        {
            sessionStr.append_sprintf("ps4:%s ", sessionIdentification.getPs4().getNpSessionId());
        }
        
        if (sessionIdentification.getXone().getSessionName()[0] != '\0')
        {
            sessionStr.append_sprintf("xone:%s ", sessionIdentification.getXone().getSessionName());
        }

        if ((sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId()[0] != '\0'))
        {
            sessionStr.append_sprintf("ps5:ps:%s ", sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId());
        }
        if ((sessionIdentification.getPs5().getMatch().getMatchId()[0] != '\0'))
        {
            sessionStr.append_sprintf("ps5:m:%s ", sessionIdentification.getPs5().getMatch().getMatchId());
        }
        
        if (sessionStr.empty())
            sessionStr.append_sprintf("<UNKNOWN EXTERNAL SESSION>");

        return sessionStr;
    }

    const Blaze::FixedString256 toLogStr(const ExternalSessionIdentification& sessionIdentification, const ExternalSessionActivityTypeInfo& forType)
    {
        Blaze::FixedString256 sessionStr;
        switch (forType.getPlatform())
        {
        case ps4:
            sessionStr.append_sprintf("ps4:%s ", sessionIdentification.getPs4().getNpSessionId());
            break;
        case xone:
        case xbsx:
            sessionStr.append_sprintf("xone:%s ", sessionIdentification.getXone().getSessionName());
            break;
        case ps5:
            switch (forType.getType())
            {
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY:
                sessionStr.append_sprintf("ps5:ps:%s ", sessionIdentification.getPs5().getPlayerSession().getPlayerSessionId());
                break;
            case EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH:
                sessionStr.append_sprintf("ps5:m:%s ", sessionIdentification.getPs5().getMatch().getMatchId());
                break;
            }
            break;
        default:
            sessionStr.append_sprintf("<UNKNOWN EXTERNAL SESSION>");
            break;
        }
        return sessionStr;
    }
   
    /*! brief whether specified user is in the list. */
    bool getUserInExternalMemberInfoList(PlayerId blazeId, ExternalMemberInfoList& list, ExternalMemberInfoList::iterator& found)
    {
        for (found = list.begin(); found != list.end(); ++found)
        {
            if (blazeId == (*found)->getUserIdentification().getBlazeId())
            {
                return true;
            }
        }
        return false;
    }

    const ExternalMemberInfo* getUserInExternalMemberInfoList(PlayerId blazeId, const ExternalMemberInfoList& list)
    {
        for (ExternalMemberInfoList::const_iterator it = list.begin(), end = list.end(); it != end; ++it)
        {
            if ((*it)->getUserIdentification().getBlazeId() == blazeId)
            {
                return *it;
            }
        }
        return nullptr;
    }

    /*! brief whether the lists have same set of users (ignoring order due to rejoins), with same values on them. */
    bool areExternalMemberListsEquivalent(const ExternalMemberInfoList& a, const ExternalMemberInfoList& b)
    {
        if (a.size() != b.size())
            return false;
        for (auto itA : a)
        {
            auto inB = getUserInExternalMemberInfoList(itA->getUserIdentification().getBlazeId(), b);
            if ((inB == nullptr) ||
                (inB->getTeamIndex() != itA->getTeamIndex()))
            {
                return false;
            }
        }
        return true;
    }

} // namespace GameManager
} // namespace Blaze

