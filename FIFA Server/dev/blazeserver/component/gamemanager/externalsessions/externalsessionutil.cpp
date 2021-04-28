/*! ************************************************************************************************/
/*!
    \file externalsessionutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "component/gamemanager/externalsessions/externalsessionutilmanager.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5.h"
#include "framework/controller/controller.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "framework/usersessions/usersessionmanager.h"
#include <EAStdC/EATextUtil.h> // for UTF8validate getValidJsonUtf8

namespace Blaze
{
namespace GameManager
{

    bool ExternalSessionUtil::createExternalSessionServices(ExternalSessionUtilManager& utilOut, const ExternalSessionServerConfigMap& configMap,
        const TimeValue& reservationTimeout, uint16_t maxMemberCount /*= UINT16_MAX*/, const TeamNameByTeamIdMap* teamNameByTeamIdMap /*= nullptr*/,
        GameManagerSlaveImpl* gameManagerSlave /*= nullptr*/)
    {
        ExternalSessionServerConfig dummyConfig;
        // Iterate over the platforms supported, create the session utils needed.
        // Then, when we go to use the utils, always include the platform version when doing the operations. 
        // There needs to be some extra logic for multiplatform support, but the basics should just be handled by multiple pointers. 
        for (ClientPlatformType curPlatform : gController->getHostedPlatforms())
        {
            // We have to have a config for all Utils, and we need to use the dummy Util by default. 
            auto curConfig = configMap.find(curPlatform);
            const ExternalSessionServerConfig& config = (curConfig != configMap.end()) ? *curConfig->second : dummyConfig;

            ExternalSessionUtil* util = createExternalSessionService(curPlatform, config, reservationTimeout, maxMemberCount, teamNameByTeamIdMap, gameManagerSlave);
            if (util == nullptr)
            {
                return false;
            }
            utilOut.mUtilMap[curPlatform] = util;
        }

        return true;
    }


    ExternalSessionUtil* ExternalSessionUtil::createExternalSessionService(ClientPlatformType platform, const ExternalSessionServerConfig& config,
        const TimeValue& reservationTimeout, uint16_t maxMemberCount /*= UINT16_MAX*/, const TeamNameByTeamIdMap* teamNameByTeamIdMap /*= nullptr*/,
        GameManagerSlaveImpl* gameManagerSlave /*= nullptr*/)
    {
        switch (platform)
        {
        case xone:
        case xbsx:
        {
            // Verify params: (Outputs an ERR message)
            bool titleIdIsHex;
            if (!ExternalSessionUtilXboxOne::verifyParams(config, titleIdIsHex, maxMemberCount, (gameManagerSlave != nullptr)))
                return nullptr;

            return BLAZE_NEW ExternalSessionUtilXboxOne(platform, config, reservationTimeout, titleIdIsHex, gameManagerSlave);
        }
        case ps4:
        {
            if (!config.getCrossgen())
            {
                if (!ExternalSessionUtilPs4::verifyParams(config))
                    return nullptr;
 
                return BLAZE_NEW ExternalSessionUtilPs4(config, reservationTimeout, gameManagerSlave);
            }
            // else for crossgen, fall through to create a separate instance of ps5 util here, for ps4 clients
            // (use a separate instance in case platform-specific behavior needed)
        }
        case ps5:
        {
            if (!ExternalSessionUtilPs5::verifyParams(config, teamNameByTeamIdMap, gameManagerSlave))
                return nullptr;

            return BLAZE_NEW ExternalSessionUtilPs5(platform, config, reservationTimeout, gameManagerSlave);
        }
        default:
            // return default util
            return BLAZE_NEW ExternalSessionUtil(config, reservationTimeout);
        }
    }

    bool ExternalSessionUtil::hasJoinExternalSessionPermission(const Blaze::UserSessionExistenceData& userSession)
    {
        return gUserSessionManager->isSessionAuthorized(userSession.getUserSessionId(), Blaze::Authorization::PERMISSION_JOIN_EXTERNAL_SESSION, true);
    }

    /*!************************************************************************************************/
    /*! \brief return whether to treat game full for purpose of updating the external session. Platform independent impln.
    ***************************************************************************************************/
    bool ExternalSessionUtil::isFullForExternalSession(const GameSessionMaster& gameSession, uint16_t& maxUsers)
    {
        if (EA_UNLIKELY(gameSession.getPlayerRoster() == nullptr))
            return false;

        maxUsers = gameSession.getTotalPlayerCapacity();

        bool isFullSlots = (mConfig.getExternalSessionFullnessIncludesQueue()?
            gameSession.isGameAndQueueFull() : (gameSession.getTotalPlayerCapacity() <= gameSession.getPlayerRoster()->getRosterPlayerCount()));

        // we don't really need to test this, aside from logging reasons if the slots were full
        bool isRoleFull = isFullForExternalSessionRole(gameSession);
      
        bool isFullExcludingReservations = isFullSlots;
        if (isFullSlots && doesExternalSessionFullnessExcludeReservationsForGame(gameSession))
        {
             isFullExcludingReservations = isFullForExternalSessionExcludingReservations(gameSession);
        }

        bool isGameFull = isFullSlots && isFullExcludingReservations;

        if (isGameFull || isRoleFull)
        {
            TRACE_LOG("[ExternalSessionUtil].isFull general slots " << (isFullSlots ? "full" : "not full")
                << ", role '" << mConfig.getExternalSessionFullnessRole() << "' " << (isRoleFull ? "full" : "not full")
                << ", for game(" << gameSession.getGameId() << "). Non-queued count is "
                << gameSession.getPlayerRoster()->getRosterPlayerCount() << "/" << gameSession.getTotalPlayerCapacity()
                << ". Queue count is " << gameSession.getPlayerRoster()->getQueueCount() << "/" << gameSession.getQueueCapacity() << ". Platform(" << ClientPlatformTypeToString(getPlatform()) << ").");
        }
        
        return (isGameFull || isRoleFull);
    }


    /*! \brief return the external session name for the given game id.
        NOTE: Length will now always be the same for the same input config values, and service name */
    BlazeRpcError ExternalSessionUtil::getNextGameExternalSessionName(eastl::string& result) const
    {
        uint64_t uid = getNextGameExternalSessionUniqueId();
        if (uid == 0)
        {
            return ERR_SYSTEM;
        }

        // to ensure consistent length, pad zeros for 20 unique id digits (uint64)
        // If these change - Update ExternalSessionUtilXboxOne::verifyParams
        result.append_sprintf("%s%s_%u_%u_%020" PRIu64, mConfig.getExternalSessionNamePrefix(), ((gController != nullptr) ? gController->getDefaultServiceName() : ""), ENTITY_TYPE_GAME.component, ENTITY_TYPE_GAME.type, uid);
        return ERR_OK;
    }

    /*! \brief return the external session name for the given matchmaking session id.
        NOTE: Length will now always be the same for the same input config values, and service name */
    void ExternalSessionUtil::getExternalMatchmakingSessionName(eastl::string& result, Blaze::GameManager::MatchmakingSessionId mmSessionId) const
    {
        // If these change - Update ExternalSessionUtilXboxOne::verifyParams
        result.append_sprintf("%s%s_%u_%020" PRIu64, mConfig.getExternalSessionNamePrefix(), ((gController != nullptr) ? gController->getDefaultServiceName() : ""), Blaze::Matchmaker::MATCHMAKER_COMPONENT_BASE_INFO.id, mmSessionId);
    }

    bool ExternalSessionUtil::doesExternalSessionFullnessExcludeReservationsForGame(const GameSessionMaster& gameSession) const
    {
        if (!mConfig.getExternalSessionFullnessExcludesReservations())
            return false;

        // excluding reservations is enabled, now check if the game type is in the list.
        const auto& gameTypeList = mConfig.getExternalSessionFullnessExcludesReservationsGameTypes();
        for (const auto& itr : gameTypeList)
        {
            if (gameSession.getGameType() == itr)
            {
                return true;
            }
        }

        // game type wasn't in the list
        return false;
    }

    bool ExternalSessionUtil::isFullForExternalSessionExcludingReservations(const GameSessionMaster& gameSession) const
    {
        // if fullness includes queue, queue having space counts as there being space too.
        if (mConfig.getExternalSessionFullnessIncludesQueue() && !gameSession.isQueueFull())
            return false;

        bool noReservations = false;
        if (gameSession.getTotalPlayerCapacity() <= gameSession.getPlayerRoster()->getPlayerCount(PlayerRoster::ACTIVE_PLAYERS))
        {
            noReservations = true;
        }

        if (noReservations && mConfig.getExternalSessionFullnessIncludesQueue())
        {
            // this is most expensive, check so done last.
            // if any reservations in queue, early out, we're not "full"
            const auto& queueRoster = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::QUEUED_PLAYERS);
            for (const auto& itr : queueRoster)
            {
                if (itr->getPlayerState() == PlayerState::RESERVED)
                {
                    noReservations = false;
                    break;
                }
            }
        }

        return noReservations;
    }

    bool ExternalSessionUtil::isFullForExternalSessionRole(const GameSessionMaster& gameSession) const
    {
        if (!mConfig.getExternalSessionFullnessIncludesRole())
            return false;

        // if fullness includes queue, queue having space counts as there being potential role space too.
        if (mConfig.getExternalSessionFullnessIncludesQueue() && !gameSession.isQueueFull())
            return false;

        const RoleName& roleName = mConfig.getExternalSessionFullnessRole();
        if (EA_UNLIKELY(gameSession.getPlayerRoster() == nullptr))
        {
            return false;
        }
        uint16_t roleCapacity = gameSession.getRoleCapacity(roleName);
        if (roleCapacity == 0)
        {
            return false; // role n/a
        }
        uint16_t totalRoleSize = 0;
        
        for (TeamIndex i = 0; i < gameSession.getTeamCount(); ++i)
        {
            totalRoleSize += gameSession.getPlayerRoster()->getRoleSize(i, roleName);
        }
        
        bool roleFull = ((roleCapacity * gameSession.getTeamCount()) <= totalRoleSize); //total role cap is roleCapacity * teamCount

        if (roleFull && doesExternalSessionFullnessExcludeReservationsForGame(gameSession))
        {
            // iterate over the roster to check if anyone in the target role is reserved
            const auto& gameRoster = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
            for (const auto& itr : gameRoster)
            {
                if ((itr->getPlayerState() == PlayerState::RESERVED) && (blaze_strcmp(roleName.c_str(), itr->getRoleName()) == 0))
                {
                    // someone in the roster was reserved for the target role, we're not "full"
                    roleFull = false;
                }
            }
        }
        
        return roleFull;
    }

    /*!************************************************************************************************/
    /*! \brief updates user's game presence. Default implementation calls:
            updatePrimaryPresence(), followed by updateNonPrimaryPresence()
    ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtil::updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params,
        UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult)
    {
        UserSessionId primarySessionId = ((gUserSessionManager != nullptr) ? gUserSessionManager->getPrimarySessionId(params.getUserIdentification().getBlazeId()) : INVALID_USER_SESSION_ID);
        UserSessionPtr userSession = gUserSessionManager->getSession(primarySessionId);
        if ((userSession == nullptr) || !ExternalSessionUtil::hasJoinExternalSessionPermission(userSession->getExistenceData()))
        {
            TRACE_LOG("[ExternalSessionUtil].updatePresenceForUser: user(" << params.getUserIdentification().getBlazeId() << ") does not have a usersession with external session permissions. No updates required.");
            return ERR_OK;
        }

        Blaze::BlazeRpcError err = ERR_OK;
        err = updatePrimaryPresence(params, result, errorResult, userSession);
        if (err != ERR_OK)
            return err;

        err = updateNonPrimaryPresence(params, result, errorResult);
        if (err != ERR_OK)
        {
            // ensure error rsp includes prior successes from updatePrimaryPresence:
            result.getCurrentPrimary().copyInto(errorResult.getCurrentPrimary());
            result.getUpdated().copyInto(errorResult.getUpdated());
        }

        return err;
    }

    /*!************************************************************************************************/
    /*! \brief Users might be in multiple external sessions but only one should be designated as the primary.
        This fn determines and then updates the primary external session for the user. Platform independent impln.
        Calls setPrimary() and clearPrimary() to execute the platform specific updates.
    ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtil::updatePrimaryPresence(const UpdatePrimaryExternalSessionForUserParameters& params,
        UpdatePrimaryExternalSessionForUserResult& result, UpdatePrimaryExternalSessionForUserErrorResult& errorResult,
        const UserSessionPtr userSession)
    {
        result.setBlazeId(params.getUserIdentification().getBlazeId());
        // If blaze handling of primary sessions is disabled, no-op
        if (mConfig.getPrimaryExternalSessionTypeForUx().empty())
        {
            result.setBlazeId(INVALID_BLAZE_ID);
            return ERR_OK;
        }

        auto& forType = getPrimaryActivityType();

        ExternalId extId = getExternalIdFromPlatformInfo(params.getUserIdentification().getPlatformInfo());
        if ((userSession == nullptr) || !hasUpdatePrimaryExternalSessionPermission(userSession->getExistenceData()))
        {
            TRACE_LOG("[ExternalSessionUtil].updatePrimaryPresence: target user does not have a user session or update primary external session permissions. No updates required. User(" << params.getUserIdentification().getBlazeId() << "), external id(" << extId << "), isPrimaryUser=" << ((userSession == nullptr)? false : userSession->isPrimarySession()) << ", isGuest=" << ((userSession == nullptr)? false : userSession->isGuestSession()) << ", hasExtSessJoinPermission(" << ((userSession == nullptr)? false : hasJoinExternalSessionPermission(userSession->getExistenceData())) << ").");
            result.setBlazeId(INVALID_BLAZE_ID);
            return ERR_OK;
        }

        // get token. pre: has external session join permissions, checked above.
        eastl::string tokenBuf;
        eastl::string servNameBuf = userSession->getServiceName();
        BlazeRpcError tokErr = getAuthToken(params.getUserIdentification(), servNameBuf.c_str(), tokenBuf);
        if (tokErr != ERR_OK)
            return tokErr;

        // determine the top candidate game user is currently in. Pre: current games list includes any just-joined game and excludes any just-left game.
        const GameActivity* topCurrentCandidate = choosePrimary(params.getCurrentGames());
        if (topCurrentCandidate == nullptr)
        {
            // current games list must be empty, or, had only non-valid game types and we must've left last valid game.
            // Clear the primary session for the user.
            TRACE_LOG("[ExternalSessionUtil].updatePrimaryPresence: current game activity list doesn't have anything that can be the primary. Ensuring current primary session cleared. User(" << params.getUserIdentification().getBlazeId() << "), external id(" << extId << ").");
            ClearPrimaryExternalSessionParameters clearParams;
            params.getUserIdentification().copyInto(clearParams.getUserIdentification());
            clearParams.getAuthInfo().setCachedExternalSessionToken(tokenBuf.c_str());
            clearParams.getAuthInfo().setServiceName(servNameBuf.c_str());

            BlazeRpcError clearErr = clearPrimary(clearParams, &errorResult.getErrorInfo());
            if (ERR_OK != clearErr)
            {
                ERR_LOG("[ExternalSessionUtil].updatePrimaryPresence: Failed to clear primary session. User(" << params.getUserIdentification().getBlazeId() << "), external id(" << extId << "), error(" << ErrorHelp::getErrorName(clearErr) << ").");
                errorResult.getCurrentPrimary().setGameId(INVALID_GAME_ID);
                newToOldGameActivityResults(errorResult);//for back compat
            }
            result.getCurrentPrimary().setGameId(INVALID_GAME_ID);
            newToOldGameActivityResults(result);//for back compat
            return clearErr;
        }

        // Set the primary session for the user.
        SetPrimaryExternalSessionParameters setParams;
        topCurrentCandidate->getSessionIdentification().copyInto(setParams.getSessionIdentification());
        params.getUserIdentification().copyInto(setParams.getUserIdentification());
        setParams.getAuthInfo().setCachedExternalSessionToken(tokenBuf.c_str());
        setParams.getAuthInfo().setServiceName(servNameBuf.c_str());
        topCurrentCandidate->copyInto(setParams.getNewPrimaryGame());
        setParams.setOldPrimaryGameId(params.getOldPrimaryGameId());
        setParams.setTitleId(params.getTitleId());
        setParams.setPsnPushContextId(params.getPsnPushContextId());
        
        SetPrimaryExternalSessionResult& setResult = result.getCurrentPrimarySession();

        BlazeRpcError sessErr = setPrimary(setParams, setResult, &errorResult.getErrorInfo());
        if (ERR_OK != sessErr)
        {
            // we now return the primary game with the error, in case clients wish to disable invite menus etc
            errorResult.getCurrentPrimary().setGameId(topCurrentCandidate->getGameId());
            errorResult.getCurrentPrimary().getActivity()[forType.getType()] = true;
            setExtSessIdent(errorResult.getCurrentPrimary().getSession(), topCurrentCandidate->getSessionIdentification(), forType);

            newToOldGameActivityResults(errorResult);//for back compat
            return sessErr;
        }

        // we now return full session activity infos
        ASSERT_COND_LOG(result.getCurrentPrimary().getActivity().find(forType.getType()) == result.getCurrentPrimary().getActivity().end(), "Unexpected state: PS5 should only have 1 ExternalSessionUtilPs5xx called per primary joinable type(" << ExternalSessionActivityTypeToString(forType.getType()) << ").");
        result.getCurrentPrimary().getActivity()[forType.getType()] = true;
        result.getCurrentPrimary().setGameId(topCurrentCandidate->getGameId());
        result.getCurrentPrimary().setPsnPushContextId(params.getPsnPushContextId());
        setExtSessIdent(result.getCurrentPrimary().getSession(), setResult, forType);

        if (result.getCurrentPrimary().getGameId() == params.getChangedGame().getGameId())
        {
            // primary and updated game is same. copy to RSP's updated
            result.getUpdated().getActivity()[forType.getType()] = true;
            result.getUpdated().setGameId(topCurrentCandidate->getGameId());
            result.getUpdated().setPsnPushContextId(params.getPsnPushContextId());
            setExtSessIdent(result.getUpdated().getSession(), setResult, forType);
        }

        newToOldGameActivityResults(result);//for back compat

        TRACE_LOG("[ExternalSessionUtil].updatePrimaryPresence: set new primary session(" << GameManager::toLogStr(setResult) << ", gameId=" << topCurrentCandidate->getGameId() << ", gameType=" << GameTypeToString(topCurrentCandidate->getGameType()) << ", timeStamp=" << topCurrentCandidate->getPlayer().getJoinedGameTimestamp().getMicroSeconds() <<
            ", playerState=" << PlayerStateToString(topCurrentCandidate->getPlayer().getPlayerState()) << "). User(" << params.getUserIdentification().getName() << ", " << params.getUserIdentification().getBlazeId() << ", external id " << extId << ").");
        return ERR_OK;
    }
    
    /*! ************************************************************************************************/
    /*! \brief determine the top candidate game user is currently in, to be the user's primary session.
    ***************************************************************************************************/
    const GameActivity* ExternalSessionUtil::choosePrimary(const GameActivityList& candidates) const
    {
        // determine the top candidate game user is currently in. Pre: current games list includes any just-joined game and excludes any just-left game.
        const GameActivity* topCurrentCandidate = nullptr;
        for (GameActivityList::const_iterator itr = candidates.begin(), end = candidates.end(); (itr != end); ++itr)
        {
            //skip games with presence disabled
            if ((*itr)->getPresenceMode() == PRESENCE_MODE_NONE || (*itr)->getPresenceMode() == PRESENCE_MODE_NONE_2)
                continue;
            //skip game types that can't be made the primary
            if (getGameTypePriority((*itr)->getGameType()) == SIZE_MAX)
                continue;
            //if queued you won't be in the external session by GM spec. We also won't count primary-eligible if only reserved.
            if (((*itr)->getPlayer().getPlayerState() == QUEUED) || ((*itr)->getPlayer().getPlayerState() == RESERVED))
                continue;
            
            if ((topCurrentCandidate == nullptr) || isHigherPriority(**itr, *topCurrentCandidate))
            {
                topCurrentCandidate = *itr;
            }
        }
        return topCurrentCandidate;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether the first game is higher in the priority chain for being the user's primary session.
    ***************************************************************************************************/
    bool ExternalSessionUtil::isHigherPriority(const GameActivity& a, const GameActivity& b) const
    {
        if (getGameTypePriority(a.getGameType()) < getGameTypePriority(b.getGameType()))
        {
            // first priority is game type
            return true;
        }
        else if ((getGameTypePriority(a.getGameType()) == getGameTypePriority(b.getGameType())) &&
            (a.getPlayer().getJoinedGameTimestamp() > b.getPlayer().getJoinedGameTimestamp()))
        {
            // second priority latest time stamp
            return true;
        }
        else if ((getGameTypePriority(a.getGameType()) == getGameTypePriority(b.getGameType())) &&
            (a.getPlayer().getJoinedGameTimestamp() == b.getPlayer().getJoinedGameTimestamp()) &&
            (EA_UNLIKELY(GetBaseFromInstanceKey64(a.getGameId()) > GetBaseFromInstanceKey64(b.getGameId()))))
        {
            // third priority game id, if somehow timestamps are same!? shouldn't be same game if here
            WARN_LOG("[ExternalSessionUtil].isHigherPriority: found supposedly two games with identical time stamps, picking the higher base value of the two. Game 1: " << a.getGameId() << ", Game 2: " << b.getGameId());
            return true;
        }
        return false;//b had higher game type priority
    }

    /*! ************************************************************************************************/
    /*! \brief return priority index of the game type in the configured external session game types list.
        Lower number is considered higher priority. If game type is not in the list returns SIZE_MAX.
    ***************************************************************************************************/
    size_t ExternalSessionUtil::getGameTypePriority(GameType gameType) const
    {
        size_t i = 0;
        for (GameTypeList::const_iterator itr = mConfig.getPrimaryExternalSessionTypeForUx().begin(), end = mConfig.getPrimaryExternalSessionTypeForUx().end(); itr != end; ++itr)
        {
            if (*itr == gameType)
                return i;
            ++i;
        }
        return SIZE_MAX;
    }

    /*! ************************************************************************************************/
    /*! \brief helper to sleep the current fiber for the specified seconds
        \param[in] logRetryNumber retry number for logging.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtil::waitSeconds(uint64_t seconds, const char8_t* context, size_t logRetryNumber)
    {
        TimeValue t;
        t.setSeconds(seconds);
        return waitTime(t, context, logRetryNumber);
    }
    BlazeRpcError ExternalSessionUtil::waitTime(const TimeValue& time, const char8_t* context, size_t logRetryNumber)
    {
        TRACE_LOG(((context != nullptr)? context : "[ExternalSessionUtil].waitTime") << ": first party service unavailable. Retry #" << logRetryNumber << " after sleeping " << time.getMillis() << "ms.");
        BlazeRpcError serr = Fiber::sleep(time, context);
        if (ERR_OK != serr)
        {
            ERR_LOG(((context != nullptr)? context : "[ExternalSessionUtil].waitTime") << ": failed to sleep fiber, error(" << ErrorHelp::getErrorName(serr) << ").");
        }
        return serr;
    }

    /*! ************************************************************************************************/
    /*! \brief reconfigure the external session util.
    ***************************************************************************************************/
    void ExternalSessionUtil::onReconfigure(const ExternalSessionServerConfig& config, const TimeValue reservationTimeout)
    {
        setReservationTimeout(reservationTimeout);
        setInactivityTimeout(config.getExternalSessionInactivityTimeout());
        setReadyTimeout(config.getExternalSessionReadyTimeout());
        setSessionEmptyTimeout(config.getExternalSessionEmptyTimeout());
        //note: call setImageMaxSize before setImageDefault to ensure image size validated
        setImageMaxSize(config.getExternalSessionImageMaxSize());
        setImageDefault(config.getExternalSessionImageDefault());
        setStatusValidLocales(config.getExternalSessionStatusValidLocales());
        setStatusMaxLocales(config.getExternalSessionStatusMaxLocales());
    }

    /*!************************************************************************************************/
    /*! \return The value, or empty string if it cannot be used as a valid UTF-8 JSON string value
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtil::getValidJsonUtf8(const char8_t* value, size_t maxLen)
    {
        if (value == nullptr)
        {
            ASSERT_LOG("[ExternalSessionUtil].getValidJsonUtf8: value nullptr. Returning default empty string.");
            return "";
        }
        size_t valueLen = strlen(value);
        if ((valueLen > maxLen) || !EA::StdC::UTF8Validate(value, valueLen))
        {
            WARN_LOG("[ExternalSessionUtil].getValidJsonUtf8: (" << value << ") of length(" << valueLen << "), utf-8 length(" << EA::StdC::UTF8Length(value) << "), max allowed len(" << maxLen << ") was not valid. Returning default empty string.");
            return "";
        }
        // PSN rejects most non-displayable ASCII chars. For safety just omit them
        for (size_t i = 0; i < valueLen; i += EA::StdC::UTF8CharSize(&value[i]))
        {
            if ((EA::StdC::UTF8CharSize(&value[i]) == 1) && ((value[i] < 32)))
            {
                WARN_LOG("[ExternalSessionUtil].getValidJsonUtf8: (" << value << ") contained invalid character(" << value[i] << "). Returning default empty string.");
                return "";
            }
        }
        return value;
    }

    /*! ************************************************************************************************/
    /*! \brief helper to get the default primary activity type info
    ***************************************************************************************************/
    const ExternalSessionActivityTypeInfo& ExternalSessionUtil::getPrimaryActivityType()
    {
        mPrimaryActivityTypeInfo.setType(EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY);
        mPrimaryActivityTypeInfo.setPlatform(getPlatform());
        return mPrimaryActivityTypeInfo;
    }


    /*! ************************************************************************************************/
    /*! \brief Finds the next member with an S2S user token for the platform from the list, who is not in the avoid set.
        \return GAMEMANAGER_ERR_PLAYER_NOT_FOUND on failure, otherwise ERR_OK.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtil::getUpdaterWithUserToken(ClientPlatformType platform, const ExternalMemberInfoList& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, bool addToAvoid /*= true*/)
    {
        const ExternalMemberInfo* candidate = nullptr;

        // If there's a current user session, and its not in the avoid list, prefer it.
        const BlazeId currentBlazeId = (UserSession::getCurrentUserSessionId() != INVALID_USER_SESSION_ID ? UserSession::getCurrentUserBlazeId() : INVALID_BLAZE_ID);
        if ((currentBlazeId != INVALID_BLAZE_ID) && (avoid.find(currentBlazeId) == avoid.end()))
        {
            avoid.insert(currentBlazeId);
            candidate = getUserInExternalMemberInfoList(currentBlazeId, possibleUpdaters);
            if ((candidate != nullptr) && (candidate->getUserIdentification().getPlatformInfo().getClientPlatform() == platform))
            {
                if (EA_LIKELY(candidate->getAuthInfo().getCachedExternalSessionToken()[0] != '\0'))
                {
                    candidate->copyInto(updater);
                    return ERR_OK;
                }
                ERR_LOG("[ExternalSessionUtil].getUpdaterWithUserToken: unexpected internal state: user(" << candidate->getUserIdentification().getBlazeId() << ") has no authentication token but is tracked in the external session member info list.");
                candidate = nullptr;
            }
        }

        for (const auto & possibleUpdater : possibleUpdaters)
        {
            // ensure has matching platform, and has a cached external session S2S user token.
            if ((possibleUpdater->getUserIdentification().getPlatformInfo().getClientPlatform() != platform) ||
                (possibleUpdater->getAuthInfo().getCachedExternalSessionToken()[0] == '\0') || (avoid.find(possibleUpdater->getUserIdentification().getBlazeId()) != avoid.end()))
            {
                continue;
            }
            candidate = possibleUpdater;
            break;
        }
        if (candidate == nullptr)
        {
            TRACE_LOG("[ExternalSessionUtil].getUpdaterWithUserToken: found NO user who can make the next update call. Number of potential callers was(" << possibleUpdaters.size() << "), number in avoid/tried list was(" << avoid.size() << ").");
            return (avoid.empty() ? GAMEMANAGER_ERR_PLAYER_NOT_FOUND : GAMEMANAGER_ERR_EXTERNAL_SERVICE_BUSY);
        }

        TRACE_LOG("[ExternalSessionUtil].getUpdaterWithUserToken: found user(" << candidate->getUserIdentification().getBlazeId() << ") who can make the next update call. Number of potential callers was(" << possibleUpdaters.size() << "), number in avoid/tried list was(" << avoid.size() << ").");
        if (addToAvoid)
            avoid.insert(candidate->getUserIdentification().getBlazeId());
        candidate->copyInto(updater);
        return ERR_OK;
    }
    
}//ns ExternalSessionService
}//ns Blaze

