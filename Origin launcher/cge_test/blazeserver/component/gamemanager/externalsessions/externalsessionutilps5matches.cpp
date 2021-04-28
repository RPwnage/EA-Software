/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5matches.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5matches.h"

#include "framework/logger.h"
#include "framework/oauth/oauthslaveimpl.h" // for PSN s2s access token
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"//for isExtSessIdentSet in updatePresenceForUser
#include "gamemanager/externalsessions/externalsessionutilps5.h"//for getPsnAccountId() in toReqPlayer
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamesessionmaster.h"//for GameSessionMaster in prepForReplay
#include "gamemanager/gamemanagermasterimpl.h"//for gGameManagerMaster in shouldResyncUpdateMatchDetailInGameRoster

#include "psnmatches/rpc/psnmatchesslave.h"
#include "psnsessionmanager/tdf/psncommondefines.h"


namespace Blaze
{
using namespace PSNServices::Matches;
namespace GameManager
{
    ExternalSessionUtilPs5Matches::ExternalSessionUtilPs5Matches(const ExternalSessionServerConfig& config, GameManagerSlaveImpl* gameManagerSlave)
        : mTrackHelper(gameManagerSlave, ps5, EXTERNAL_SESSION_ACTIVITY_TYPE_PSN_MATCH)
    {
        config.copyInto(mConfig);
        ExternalSessionUtilPs5::getNewWebApiConnMgr(mPsnConnMgrPtr, PSNMatchesSlave::COMPONENT_INFO.name);
    }

    ExternalSessionUtilPs5Matches::~ExternalSessionUtilPs5Matches()
    {
        if (mPsnConnMgrPtr != nullptr)
            mPsnConnMgrPtr.reset();
    }

    bool ExternalSessionUtilPs5Matches::verifyParams(const ExternalSessionServerConfig& config, const TeamNameByTeamIdMap* teamNameByTeamIdMap)
    {
        if (isMatchActivitiesDisabled(config))
        {
            return true;
        }

        if (config.getPsnMatchesServiceLabel() < 0)
        {
            ERR_LOG("[ExternalSessionUtilPs5Matches].verifyParams: Invalid PSN Matches service label(" << config.getPsnMatchesServiceLabel() << ").");
            return false;
        }

        if (config.getCallOptions().getServiceUnavailableRetryLimit() > config.getCallOptions().getTotalRetryLimit())
        {
            ERR_LOG("[ExternalSessionUtilPs5Matches].verifyParams: Invalid serviceUnavailable retry limit(" << config.getCallOptions().getServiceUnavailableRetryLimit() << "). Max allowed(" << config.getCallOptions().getTotalRetryLimit() << ")");
            return false;
        }

        if (teamNameByTeamIdMap != nullptr)
        {
            for (auto it : *teamNameByTeamIdMap)
            {
                if (it.second.length() > PSNServices::Matches::MAX_PSNMATCH_TEAMNAME_LEN)
                {
                    ERR_LOG("[ExternalSessionUtilPs5Matches].verifyParams: Invalid teamName(" << it.second << ") of length(" << it.second.length() << "), exceeds max(" << PSNServices::Matches::MAX_PSNMATCH_TEAMNAME_LEN << ").");
                    return false;
                }
            }
        }

        return true;
    }


    /*!************************************************************************************************/
   /*! \brief updates PS5 Match Activity presence for the user (separate from PlayerSession presence)
   ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtilPs5Matches::updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result,
        UpdateExternalSessionPresenceForUserErrorInfo& errorResult)
    {
        // If blaze handling of Matches is disabled, no-op
        if (isMatchActivitiesDisabled() || isMatchActivitiesDisabledForGame(params.getChangedGame().getSessionIdentification()))
        {
            return ERR_OK;
        }
        auto& user = params.getUserIdentification();
        if (!shouldPlayerBeInMatches(user, params.getChangedGame().getPlayer().getPlayerState()))
        {
            return ERR_OK;
        }

        BlazeRpcError err = ERR_OK;
        GameId changedGameId = params.getChangedGame().getGameId();
        const auto& forType = getActivityTypeInfo();
        
        if (params.getChange() == UPDATE_PRESENCE_REASON_GAME_LEAVE)
        {
            // leave
            return leaveMatch(errorResult.getErrorInfo(), params);
        }
        // create or join as needed
        // (PSN's join method is also used to update user's team index)

        GetExternalSessionInfoMasterResponse gameInfoBuf;
        if (!mTrackHelper.isAddUserToExtSessionRequired(gameInfoBuf, changedGameId, user) &&
            (params.getChange() != UPDATE_PRESENCE_REASON_TEAM_CHANGE))//DO call join again to change team
        {
            // If already joined, no need to rejoin
            gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().copyInto(result.getUpdated().getSession());
        }
        else
        {
            // If game already has an 1st party match, join, otherwise create. store result
            if (gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getMatch().getMatchId()[0] != '\0')
            {
                err = joinMatch(result.getUpdated().getSession(), errorResult.getErrorInfo(), params, gameInfoBuf);
            }
            else
            {
                err = createMatch(result.getUpdated().getSession(), errorResult.getErrorInfo(), params, gameInfoBuf);
            }
        }
        if (err == ERR_OK)
        {
            ASSERT_COND_LOG(result.getUpdated().getActivity().find(forType.getType()) == result.getUpdated().getActivity().end(), "Unexpected state: PS5 should only have 1 ExternalSessionUtilPs5xx called per activity type: multiple for type(" << ExternalSessionActivityTypeToString(forType.getType()) << ").");
            result.getUpdated().getActivity()[forType.getType()] = true;
            result.getUpdated().setGameId(params.getChangedGame().getGameId());
            if (result.getCurrentPrimary().getGameId() == params.getChangedGame().getGameId())
            {
                // primary and updated game is same. copy to RSP's primary
                result.getCurrentPrimary().getActivity()[forType.getType()] = true;
                setExtSessIdent(result.getCurrentPrimary().getSession(), result.getUpdated().getSession(), forType);
            }
        }

        return ((err == GAMEMANAGER_ERR_INVALID_GAME_ID) ? ERR_OK : err);
    }


    /*! ************************************************************************************************/
    /*! \brief creates the 1st party match
        \param[in,out] authInfo Cached S2S access token. If n/a or expired, this call will populate a new
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::createMatch(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo,
        const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo)
    {
        auto& caller = callerParams.getUserIdentification();
        const char8_t* activityId = callerParams.getChangedGame().getSessionIdentification().getPs5().getMatch().getActivityObjectId();
        if (activityId[0] == '\0')
        {
            ERR_LOG(logPrefix() << ".createMatch: cannot create a Match without an ActivityId. No-op for game(" << callerParams.getChangedGame().getGameId() << "). Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        PSNServices::Matches::CreateMatchResponse rsp;
        PSNServices::Matches::CreateMatchRequest req;
        req.getBody().setNpServiceLabel(getServiceLabel());
        req.getBody().setActivityId(activityId);
        auto reqPlayer = req.getBody().getInGameRoster().getPlayers().pull_back();
        toPsnReqPlayer(*reqPlayer, callerParams.getUserIdentification());
        toPsnReqTeams(req.getBody().getInGameRoster(), callerParams, gameInfo);

        TRACE_LOG(logPrefix() << ".createMatch: creating user(" << toExtUserLogStr(caller) << ")'s Match for game(" << gameInfo.getExternalSessionCreationInfo().getExternalSessionId() << "): activity(" <<
            req.getBody().getActivityId() << "), serviceLabel(" << req.getBody().getNpServiceLabel() << ").");

        // send request to PSN
        const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_CREATEMATCH;

        BlazeRpcError err = callPSN(&caller, cmd, req.getHeader(), req, &rsp, &errorInfo);
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".createMatch: failed to create user(" << toExtUserLogStr(caller) << ")'s game(" << gameInfo.getExternalSessionCreationInfo().getExternalSessionId() << ")'s Match, " << ErrorHelp::getErrorName(err));
            return err;
        }
        // created
        gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().copyInto(resultSessionIdent.getPs5());
        resultSessionIdent.getPs5().getMatch().setMatchId(rsp.getMatchId());//add the new MatchId
        resultSessionIdent.getPs5().getMatch().setActivityObjectId(activityId);

        // Ensure Blaze Game and its session, and member tracking are in sync
        err = syncMatchAndTrackedMembership(resultSessionIdent, errorInfo, callerParams, gameInfo, true);
        if (err == ERR_OK)
        {
            TRACE_LOG(logPrefix() << ".createMatch: successfully created user(" << toExtUserLogStr(caller) << ")'s Match(" << toLogStr(resultSessionIdent) << ").");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief joins the 1st party match
        \param[in,out] authInfo Cached S2S access token. If n/a or expired, this call will populate a new
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::joinMatch(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo,
        const UpdatePrimaryExternalSessionForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo)
    {
        auto& caller = callerParams.getUserIdentification();
        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        const char8_t* matchId = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getMatch().getMatchId();
        if ((matchId == nullptr) || (matchId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".joinMatch: invalid/missing input matchId(" << ((matchId != nullptr) ? matchId : "<nullptr>") << "). No op for user(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        PSNServices::Matches::JoinMatchRequest req;
        req.setMatchId(matchId);
        req.getBody().setNpServiceLabel(getServiceLabel());
        auto reqPlayer = req.getBody().getPlayers().pull_back();
        auto joinToTeam = (hasTeams(gameInfo) ? callerParams.getChangedGame().getPlayer().getTeamIndex() : UNSPECIFIED_TEAM_INDEX);
        toPsnReqPlayer(*reqPlayer, caller, joinToTeam);
        TRACE_LOG(logPrefix() << ".joinMatch: joining user(" << toExtUserLogStr(caller) << ") to game(" << gameId << ")'s Match(" << matchId << ") or team(" << (reqPlayer->isTeamIdSet() ? reqPlayer->getTeamId() : "<n/a>") << "), change(" << UpdateExternalSessionPresenceForUserReasonToString(callerParams.getChange()) << ").");

        const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_JOINMATCH;
        BlazeRpcError err = callPSN(&caller, cmd, req.getHeader(), req, nullptr, &errorInfo);
        if (err == PSNMATCHES_RESOURCE_NOT_FOUND)
        {
            // 1st party match somehow went away before could join it. outside of replays, a game's MatchId
            // shouldn't change, log warning. Try to recover by creating or joining the game's Match
            ExternalMemberInfo member;
            populateExtMemberInfo(member, callerParams);
            mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, gameId, &member, &gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification());//in case cleanup needed
            WARN_LOG(logPrefix() << ".joinMatch: 1st party Match(" << req.getMatchId() << ") somehow removed before user(" << toExtUserLogStr(caller) << ") could join it. Trying to recover by creating or joining game's Match.");
            return createMatch(resultSessionIdent, errorInfo, callerParams, gameInfo);
        }
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".joinMatch: failed to add user(" << toExtUserLogStr(caller) << ") to game(" << gameId << ")'s Match(" << matchId << ") or team(" << (reqPlayer->isTeamIdSet() ? reqPlayer->getTeamId() : "<n/a>") << "), change(" << UpdateExternalSessionPresenceForUserReasonToString(callerParams.getChange()) << "), " << ErrorHelp::getErrorName(err) << ".");
            return err;
        }
        // else joined!
        gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().copyInto(resultSessionIdent.getPs5());

        err = syncMatchAndTrackedMembership(resultSessionIdent, errorInfo, callerParams, gameInfo, false);
        return err;
    }


    /*! ************************************************************************************************/
    /*! \brief leave the 1st party match
        \note PSN technically never explicitly removes Match 'player' objects but toggles their bool 'joined' flag to false.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::leaveMatch(ExternalSessionErrorInfo& errorInfo, const UpdatePrimaryExternalSessionForUserParameters& callerParams)
    {
        auto& caller = callerParams.getUserIdentification();
        GameId gameId = callerParams.getChangedGame().getGameId();
        const char8_t* matchId = callerParams.getChangedGame().getSessionIdentification().getPs5().getMatch().getMatchId();
        PlayerRemovedReason removeReason = callerParams.getRemoveReason();

        // Before leaving, track as not in the Match centrally on master.
        ExternalMemberInfo memberInfo;
        caller.copyInto(memberInfo.getUserIdentification());
        UntrackExtSessionMembershipResponse rsp;
        mTrackHelper.untrackExtSessionMembershipOnMaster(&rsp, gameId, &memberInfo, nullptr);

        return leaveMatchInternal(errorInfo, caller, matchId, gameId, removeReason, rsp.getSimplifiedState());
    }

    BlazeRpcError ExternalSessionUtilPs5Matches::leaveMatchInternal(ExternalSessionErrorInfo& errorInfo, const UserIdentification& caller, const char8_t* matchId, GameId gameId, PlayerRemovedReason removeReason,
        SimplifiedGamePlayState gamePlayState,
        PsnLeaveReasonEnum leaveReasonType /*= INVALID_LEAVE_REASON_TYPE*/)
    {
        eastl::string acctIdBuf;
        ExternalSessionUtilPs5::getPsnAccountId(acctIdBuf, caller);

        if ((matchId == nullptr) || (matchId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".leaveMatch: invalid matchId(" << ((matchId != nullptr) ? matchId : "<null>") << "). No op.");
            return ERR_SYSTEM;
        }
        if (gamePlayState == GAME_PLAY_STATE_POST_GAME)
        {
            // PS5 UX 2.00 no longer shows users w/joinFlag false in results, for consistency no op if game play done
            TRACE_LOG(logPrefix() << ".leaveMatch: SKIP leaving (" << toExtUserLogStr(caller) << ") from Match(" << matchId << "), as game(" << gameId << ") is in state(" << SimplifiedGamePlayStateToString(gamePlayState) << ")");
            return ERR_OK;
        }

        TRACE_LOG(logPrefix() << ".leaveMatch: leaving (" << toExtUserLogStr(caller) << ") from Match(" << matchId << "). PlayerRemoveReason(" << PlayerRemovedReasonToString(removeReason) << ")");

        // get PSN Match to determine user's Match playerId to leave by
        GetMatchDetailResponse getRsp;
        auto err = getMatch(getRsp, errorInfo, matchId, &caller);
        if (err != ERR_OK)
        {
            WARN_LOG(logPrefix() << ".leaveMatch: Failed getting Match(" << matchId << "). Not leaving, " << ErrorHelp::getErrorName(err));
            return err;
        }
        if (isMatchAlreadyOver(getRsp.getStatus(), "leave"))
            return ERR_OK;//logged

        PSNServices::Matches::LeaveMatchRequest req;
        const auto leaveReason = (PsnLeaveReasonEnumToString((leaveReasonType != INVALID_LEAVE_REASON_TYPE) ? leaveReasonType : toMatchLeaveReason(caller, matchId, gameId, removeReason, getRsp)));
        for (auto it : getRsp.getInGameRoster().getPlayers())
        {
            if (blaze_strcmp(acctIdBuf.c_str(), it->getAccountId()) == 0)
            {
                // found player id
                if (!it->getJoinFlag())
                {
                    TRACE_LOG(logPrefix() << ".leaveMatch: Match(" << matchId << ") joinFlag was already false for user(" << toExtUserLogStr(caller) << ")");
                    return ERR_OK;//already left
                }
                auto player = req.getBody().getPlayers().pull_back();
                player->setPlayerId(it->getPlayerId());
                player->setReason(leaveReason);
                break;
            }
        }
        if (req.getBody().getPlayers().empty())
        {
            TRACE_LOG(logPrefix() << ".leaveMatch: warning: NOOP: Match(" << matchId << ") missing player object for user(" << toExtUserLogStr(caller) << ")");
            return ERR_OK;//PSN deleted history or never joined
        }
        req.getBody().setNpServiceLabel(getServiceLabel());
        req.setMatchId(matchId);
        const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_LEAVEMATCH;

        err = callPSN(&caller, cmd, req.getHeader(), req, nullptr, &errorInfo);
        if (isMatchAlreadyOverErr(err, errorInfo, cmd))
        {
            TRACE_LOG(logPrefix() << ".leaveMatch: NOOP: Match(" << matchId << ") may have ended/canceled before (" << toExtUserLogStr(caller) << ") could leave, " << ErrorHelp::getErrorName(err));
            err = ERR_OK;
        }
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".leaveMatch: failed to leave (" << toExtUserLogStr(caller) << ") from Match(" << matchId << "), " << ErrorHelp::getErrorName(err));
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief gets the 1st party match
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::getMatch(GetMatchDetailResponse& rsp, ExternalSessionErrorInfo& errorInfo,
        const char8_t* matchId, const UserIdentification* caller)
    {
        if ((matchId == nullptr) || (matchId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".getMatch: invalid/missing matchId(" << ((matchId != nullptr) ? matchId : "<nullptr>") << "). No op.");
            return ERR_SYSTEM;
        }

        GetMatchDetailRequest req;
        req.setNpServiceLabel(getServiceLabel());
        req.setMatchId(matchId);

        const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_GETMATCHDETAIL;

        BlazeRpcError err = callPSN(caller, cmd, req.getHeader(), req, &rsp, &errorInfo);
        if (err != ERR_OK)
        {
            TRACE_LOG(logPrefix() << ".getMatch: failed to get match(" << matchId << "), " << ErrorHelp::getErrorName(err));
        }
        return err;
    }




    /*! ************************************************************************************************/
    /*! \brief Call after adding user to a PSN 1st party match, to ensure the Blaze game, its 1st party session id and members in sync.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::syncMatchAndTrackedMembership(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo,
        const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo,
        bool forCreate)
    {
        const GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        ExternalMemberInfo member;
        populateExtMemberInfo(member, callerParams);
        GetExternalSessionInfoMasterResponse rspGameInfo;
        auto err = mTrackHelper.trackExtSessionMembershipOnMaster(rspGameInfo, gameInfo, member, resultSessionIdent);
        if (err != ERR_OK)
        {
            auto origMatchId = resultSessionIdent.getPs5().getMatch().getMatchId();
            auto currMatchId = rspGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getMatch().getMatchId();

            // on failure, leave/cancel-match, to prevent issues
            if (forCreate)
            {
                // failed to stick our new MatchId onto Game for whatever reason. Its not enough just to leave
                // the orphaned Match, as that won't close it for PS5 UX, must cancel:
                PsnMatchStatusTypeList s;
                s.push_back(CANCELLED);
                updateMatchStatus(s, origMatchId);
            }
            else
            {
                // as track call above failed no need to untrack
                leaveMatchInternal(errorInfo, callerParams.getUserIdentification(), origMatchId, gameId, SYS_PLAYER_REMOVE_REASON_INVALID, rspGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSimplifiedState(), PSNServices::Matches::DISCONNECTED);
            }

            // If game's MatchId changed, resync by joining that instead, update resultSessionIdent
            if (EA_UNLIKELY(err == GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION))
            {
                TRACE_LOG(logPrefix() << ".syncMatchAndTrackedMembership: warning could not track user(" << toExtUserLogStr(callerParams.getUserIdentification()) << ") with Match(" << origMatchId << ") for Blaze game(" << gameId << "), as game has a different PSN Match(" << currMatchId << "). Resyncing, by moving user to game current PSN Match instead.");
                return joinMatch(resultSessionIdent, errorInfo, callerParams, rspGameInfo);
            }
            TRACE_LOG(logPrefix() << ".syncMatchAndTrackedMembership: warning could not track user(" << toExtUserLogStr(callerParams.getUserIdentification()) << ") with Match(" << origMatchId << ") for Blaze game(" << gameId << "), after match (" << (forCreate ? "create" : "join") << ").");
        }
        return err;
    }




    /*!************************************************************************************************/
    /*! \brief Return whether to update changeable properties of the game's PSN Match or Match members
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
    {
        if (isMatchActivitiesDisabled() || isMatchActivitiesDisabledForGame(newValues.getSessionIdentification()))
        {
            return false;
        }
        // if game doesn't have a Match created for it yet, no updates needed (any Match will be init with up to date values at its creation)
        if (newValues.getSessionIdentification().getPs5().getMatch().getMatchId()[0] == '\0')
        {
            return false;
        }
        if (isUpdateMatchDetailInGameRosterRequired(false, origValues, newValues))
        {
            return true;
        }
        if (isUpdatePlayerJoinFlagRequired(nullptr, origValues, newValues, context))
        {
            return true;
        }
        if (isUpdateMatchStatusRequired(nullptr, origValues, newValues, context))
        {
            return true;
        }
        return false;
    }
    /*!************************************************************************************************/
    /*! \brief updates the 1st party match's status and/or detail
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::update(const UpdateExternalSessionPropertiesParameters& params)
    {
        BlazeRpcError retErr = ERR_OK;

        // update InGameRoster as needed. Note after match status CANCELLED you cannot update teams, so for consistency doing this 1st
        if (isUpdateMatchDetailInGameRosterRequired(true, params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            BlazeRpcError statusErr = updateMatchDetailInGameRoster(params.getExternalSessionUpdateInfo());//logged if err
            if (statusErr != ERR_OK)
                retErr = statusErr;
        }
        else // (updateMatchDetailInGameRoster includes doing below. no need doing twice)
        {
            // update player joinFlag as needed. Note after match status CANCELLED you cannot update joinFlag, so for consistency doing this 1st
            PlayerJoinFlagUpdateList requiredJoinFlagUpdate;
            if (isUpdatePlayerJoinFlagRequired(&requiredJoinFlagUpdate, params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo(), params.getContext()))
            {
                BlazeRpcError statusErr = updatePlayerJoinFlag(requiredJoinFlagUpdate, params.getExternalSessionUpdateInfo());//logged if err
                if (statusErr != ERR_OK)
                    retErr = statusErr;
            }
        }

        // update match status as needed
        PsnMatchStatusTypeList requiredStatusUpdate;
        if (isUpdateMatchStatusRequired(&requiredStatusUpdate, params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo(), params.getContext()))
        {
            BlazeRpcError statusErr = updateMatchStatus(requiredStatusUpdate, params.getSessionIdentification().getPs5().getMatch().getMatchId());//logged if err
            if (statusErr != ERR_OK)
                retErr = statusErr;
        }

        // For Blaze, currently no need to update match details (expirationTime, zoneId unsupported, rosters updated via direct join/leave RPCs)

        return retErr;
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether to set any Match player joinFlag false. Covers the server disconnect case which connected leave flow can't.
        \param[out] requiredUpdates If not null, stores the players to ensure updated.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isUpdatePlayerJoinFlagRequired(PlayerJoinFlagUpdateList* requiredUpdates,
        const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
    {
        bool isRequired = false;
        for (auto it : context.getMembershipChangedContext())
        {
            //Covers the server disconnect case which connected leave flow can't:
            bool isServerDisconnect = (it->getRemoveReason() == BLAZESERVER_CONN_LOST);
            if (EA_LIKELY(isServerDisconnect) && shouldPlayerBeInMatches(it->getUser().getUserIdentification(), it->getPlayerState()))
            {
                isRequired = true;
                if (requiredUpdates == nullptr)
                {
                    return true;// can return if not storing
                }
                it->copyInto(*requiredUpdates->pull_back());
            }
        }
        if (isRequired && requiredUpdates)
        {
            TRACE_LOG(logPrefix() << ".isUpdatePlayerJoinFlagRequired: should update (" << requiredUpdates->size() << ") memberships for game(" << newValues.getGameId() << ")'s Match(" << toLogStr(newValues.getSessionIdentification()) << ").");
        }
        return isRequired;
    }
    /*! ************************************************************************************************/
    /*! \brief Update Match players joinFlags
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::updatePlayerJoinFlag(const PlayerJoinFlagUpdateList& requiredUpdates, const ExternalSessionUpdateInfo& newValues)
    {
        const char8_t* matchId = newValues.getSessionIdentification().getPs5().getMatch().getMatchId();
        GameId gameId = newValues.getGameId();
        TRACE_LOG(logPrefix() << ".updatePlayerJoinFlag: updating (" << requiredUpdates.size() << ") player memberships in game(" << gameId << ")'s Match(" << matchId << "): " << requiredUpdates);
        BlazeRpcError retErr = ERR_OK;
        ExternalSessionErrorInfo errorInfo;
        for (auto it : requiredUpdates)
        {
            BlazeRpcError err = leaveMatchInternal(errorInfo, it->getUser().getUserIdentification(), matchId, gameId, it->getRemoveReason(), newValues.getSimplifiedState());
            if (err != ERR_OK)
            {
                ERR_LOG(logPrefix() << ".updatePlayerJoinFlag: failed to leave Match(" << matchId << ") player(" << toExtUserLogStr(it->getUser().getUserIdentification()) << "), for removeReason(" << PlayerRemovedReasonToString(it->getRemoveReason()) << "), err(" << ErrorHelp::getErrorName(err) << ").");
                retErr = err;
            }
        }
        return retErr;
    }
        
    /*! ************************************************************************************************/
    /*! \brief Return whether Match may need transitioning to a new Match Status to reflect the updated game. Valid transitions:
            WAITING -> PLAYING or CANCELLED
            PLAYING -> CANCELLED or COMPLETED
        \param[out] requiredUpdates If not null, stores the statuses to ensure Match is transitioned to.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isUpdateMatchStatusRequired(PsnMatchStatusTypeList* requiredUpdates,
        const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
    {
        bool result = false;

        const auto origState = origValues.getSimplifiedState();
        const auto newState = newValues.getSimplifiedState();

        // When 1st PSN user creates Game's Match, it could already have 'missed' Game updates (e.g. w/crossplay, skipInitializing), must ensure synced to latest Game values
        const bool is1stPsnUser = isJustAdded1stPsnUser(origValues, newValues);

        // Ensure WAITING -> PLAYING as needed.  (Do this before below, in case need WAITING -> PLAYING -> CANCELLED)
        if (((origState == GAME_PLAY_STATE_PRE_GAME) || is1stPsnUser) && ((newState == GAME_PLAY_STATE_IN_GAME) || (newState == GAME_PLAY_STATE_POST_GAME)))
        {
            result = true;
            if (requiredUpdates)
                requiredUpdates->push_back(PLAYING);
        }

        // Ensure WAITING or PLAYING -> CANCELLED as needed.
        if ((isFinalExtSessUpdate(context) && (newState != GAME_PLAY_STATE_POST_GAME)) ||
            // For simplicity/usability also just cancel if all PS5 players DNF (e.g. xplay cases):
            (((origState != GAME_PLAY_STATE_POST_GAME) && (newState == GAME_PLAY_STATE_POST_GAME)) && hasPsPlayersInList(newValues.getDnfPlayers()) && !hasExternalMembersInMap(newValues.getTrackedExternalSessionMembers(), getActivityTypeInfo())))
        {
            result = true;
            if (requiredUpdates)
                requiredUpdates->push_back(CANCELLED);
        }

        // Note: GameReporting handles PLAYING -> COMPLETED, by submitting Match Result.

        return result;
    }
    /*! ************************************************************************************************/
    /*! \brief Transitions Match to the specified Match Statuses
        \param[in] requiredUpdates Status(es) to ensure Match transitioned to
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::updateMatchStatus(const PsnMatchStatusTypeList& requiredUpdates, const char8_t* matchId)
    {
        ASSERT_COND_LOG(matchId[0] != '\0', logPrefix() << ".updateMatchStatus: invalid/missing input matchId(" << ((matchId != nullptr) ? matchId : "<nullptr>") << "). No op.");
        for (auto itr : requiredUpdates)
        {
            PSNServices::Matches::UpdateMatchStatusRequest req;
            req.setMatchId(matchId);
            req.getBody().setNpServiceLabel(getServiceLabel());
            req.getBody().setStatus(PsnMatchStatusEnumToString(itr));

            const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_UPDATEMATCHSTATUS;

            ExternalSessionErrorInfo errorInfo;
            BlazeRpcError err = callPSN(nullptr, cmd, req.getHeader(), req, nullptr, &errorInfo);
            if (isMatchAlreadyOverErr(err, errorInfo, cmd))
            {
                TRACE_LOG(logPrefix() << ".updateMatchStatus: NOOP: Match(" << matchId << ") may be over, err(" << ErrorHelp::getErrorName(err) << ")");
                err = ERR_OK;
            }
            if (err != ERR_OK)
            {
                ERR_LOG(logPrefix() << ".updateMatchStatus: failed to update Match(" << matchId << ")'s status to(" << PsnMatchStatusEnumToString(itr) << "), err(" << ErrorHelp::getErrorName(err) << ").");
                return err;
            }
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether to update team names
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isUpdateMatchTeamNameRequired(const char8_t* logTrace, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues)
    {
        const TeamIdVector& origBlazeTeamIds = origValues.getTeamIdsByIndex();//Blaze TeamIds map to PSN teamNames
        const TeamIdVector& newBlazeTeamIds = newValues.getTeamIdsByIndex();
        if (!hasTeams(newBlazeTeamIds) || (origValues.getTeamNameByIndex().empty() && newValues.getTeamNameByIndex().empty()))
        {
            return false;
        }
        if (EA_UNLIKELY(origBlazeTeamIds.size() != newBlazeTeamIds.size()))
        {
            WARN_LOG(logPrefix() << ".isUpdateMatchTeamNameRequired: Poss unexpected internal state: PSN Matches cannot change groupingType, but Blaze game's teamCount(" << origBlazeTeamIds.size() << ") -> (" << newBlazeTeamIds.size() << ") ?");
            return true;
        }
        for (TeamIndex i = 0; i < newBlazeTeamIds.size(); ++i)
        {
            auto origTeamName = toPsnReqTeamName(i, origValues);
            auto newTeamName = toPsnReqTeamName(i, newValues);
            if ((origTeamName == nullptr) != (newTeamName == nullptr))
                return true;
            // else both null or non null
            if (origTeamName && newTeamName && blaze_stricmp(origTeamName, newTeamName) != 0)
            {
                if (logTrace)
                {
                    TRACE_LOG(logPrefix() << ".isUpdateMatchTeamNameRequired: (" << logTrace << ") should update Match(" << toLogStr(newValues.getSessionIdentification()) << ")'s teamIndex(" << i << ")'s name(" << (origTeamName ? origTeamName : "<unspecified>") << ") -> (" << (newTeamName ? newTeamName : "<unspecified>") << ").  (Blaze TeamId(" << origBlazeTeamIds[i] << ") -> (" << newBlazeTeamIds[i] << ")).");
                }
                return true;
            }
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief Return whether to update Match detail's InGameRoster. Used to update team names (while Sony has no simpler way)
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isUpdateMatchDetailInGameRosterRequired(bool logTrace, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues)
    {
        if (isUpdateMatchTeamNameRequired((logTrace ? "on game update" : nullptr), origValues, newValues))
        {
            return true;
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief Updates Match detail's InGameRoster. Used to update team names (while Sony has no simpler way)
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::updateMatchDetailInGameRoster(const ExternalSessionUpdateInfo& requiredUpdates)
    {
        auto matchId = requiredUpdates.getSessionIdentification().getPs5().getMatch().getMatchId();
        UpdateMatchDetailRequest req;
        req.setMatchId(matchId);
        req.getBody().setNpServiceLabel(getServiceLabel());
        //set req players.   Note: By Sony spec include ALL (including unchanged) InGameRoster values (what's left out maybe *deleted*)
        auto playerAndTeamIndexInfos = getListFromMap(requiredUpdates.getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if (playerAndTeamIndexInfos)
        {
            for (auto it : *playerAndTeamIndexInfos)
                toPsnReqPlayer(*req.getBody().getInGameRoster().getPlayers().pull_back(), it->getUserIdentification());
        }
        //set req teams
        for (TeamIndex i = 0; i < requiredUpdates.getTeamIdsByIndex().size(); ++i)
        {
            // pre: any team existing in Blaze game, goes in PS5 Match (even if has no PS5 members)
            toPsnReqTeam(*req.getBody().getInGameRoster().getTeams().pull_back(), i, nullptr, requiredUpdates);
        }

        TRACE_LOG(logPrefix() << ".updateMatchDetailInGameRoster: update game(" << requiredUpdates.getGameId() << ")'s Match(" << matchId << "): teams(" << req.getBody().getInGameRoster().getTeams() << ")");

        const CommandInfo& cmd = PSNMatchesSlave::CMD_INFO_UPDATEMATCHDETAIL;

        ExternalSessionErrorInfo errorInfo;
        auto err = callPSN(nullptr, cmd, req.getHeader(), req, nullptr, &errorInfo);
        if (isMatchAlreadyOverErr(err, errorInfo, cmd))
        {
            TRACE_LOG(logPrefix() << ".updateMatchDetailInGameRoster: NOOP: Match(" << matchId << ") may be over, err(" << ErrorHelp::getErrorName(err) << ")");
            err = ERR_OK;
        }
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".updateMatchDetailInGameRoster: failed to update game(" << requiredUpdates.getGameId() << ")'s Match(" << matchId << "), err(" << ErrorHelp::getErrorName(err) << ").");
            return err;
        }

        // Sony has nothing like XBL's OCC eTags, just resync if need:
        ExternalSessionUpdateInfo newRequiredUpdates;
        if (shouldResyncUpdateMatchDetailInGameRoster(newRequiredUpdates, requiredUpdates))
        {
            return updateMatchDetailInGameRoster(newRequiredUpdates);
        }
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether Game ext membership tracking and Match InGameRoster out of sync on an updateMatchDetail.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::shouldResyncUpdateMatchDetailInGameRoster(ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateInfo& origUpdates)
    {
        auto matchId = origUpdates.getSessionIdentification().getPs5().getMatch().getMatchId();
        auto gameId = origUpdates.getGameId();
        auto gameSession = (gGameManagerMaster ? gGameManagerMaster->getReadOnlyGameSession(gameId) : nullptr);
        if ((gameSession == nullptr) || (matchId[0] == '\0') || (blaze_strcmp(matchId, gameSession->getExternalSessionIdentification().getPs5().getMatch().getMatchId()) != 0))
        {
            return false;//removed?
        }
        auto origTrackedMembers = getListFromMap(origUpdates.getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if (!origTrackedMembers || origTrackedMembers->empty())
        {
            WARN_LOG(logPrefix() << ".shouldResyncUpdateMatchDetailInGameRoster: Poss unexpected state. Game(" << gameId << ") TrackedExternalSessionMembers missing/empty. NOOP for Match(" << matchId << ").");
            return false;
        }
        setExternalSessionUpdateInfoFromGame(newValues, *gameSession);
        // check teamNames
        if (isUpdateMatchTeamNameRequired("resync", origUpdates, newValues))
        {
            return true;
        }
        // check members
        auto currTrackedMembers = getListFromMap(newValues.getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if (!currTrackedMembers || !areExternalMemberListsEquivalent(*currTrackedMembers, *origTrackedMembers))
        {
            TRACE_LOG(logPrefix() << ".shouldResyncUpdateMatchDetailInGameRoster: Match(" << matchId << ") resync needed, Game(" << gameId << ") tracked members(" << *origTrackedMembers << ") was outdated -> (" << (currTrackedMembers ? (StringBuilder() << *currTrackedMembers).c_str() : "<null>") << ")");
            return true;
        }
        return false;
    }


    void ExternalSessionUtilPs5Matches::populateExtMemberInfo(ExternalMemberInfo& member, const UpdateExternalSessionPresenceForUserParameters& callerParams)
    {
        callerParams.getUserIdentification().copyInto(member.getUserIdentification());
      //member.getAuthInfo() not required filled, as Matches use common client cred token not user tokens
        member.setTeamIndex(callerParams.getChangedGame().getPlayer().getTeamIndex());
    }
    bool ExternalSessionUtilPs5Matches::isJustAdded1stPsnUser(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return (!hasExternalMembersInMap(origValues.getTrackedExternalSessionMembers(), getActivityTypeInfo()) && hasExternalMembersInMap(newValues.getTrackedExternalSessionMembers(), getActivityTypeInfo()));
    }
    bool ExternalSessionUtilPs5Matches::hasPsPlayersInList(const UserIdentificationList& list) const
    {
        for (auto it : list)
        {
            if (shouldPlayerBeInMatches(*it, GameManager::ACTIVE_CONNECTED))
                return true;
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief PS5 Matches must be cleared during each replay. (BlazeSDK clients then simply automatically call
            updateExternalSessionPresenceForUser() RPC to properly create/join a new MatchId, under the hood).

        Note: the last round MatchId should already have been shipped off to game reporting by GM. No need to deal w/that here.
    ***************************************************************************************************/
    void ExternalSessionUtilPs5Matches::prepForReplay(GameSessionMaster& gameSession)
    {
        gameSession.getReplicatedGameSession().getExternalSessionIdentification().getPs5().getMatch().setMatchId("");

        auto* trackedMembers = getListFromMap(gameSession.getGameDataMaster()->getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if (trackedMembers)
            trackedMembers->clear();
    }


    /*! ************************************************************************************************/
    /*! \brief helper to populate PSN request's member, from UserIdentification
        \param[in] blazeJoinTeamIndex If multi Teams Game, the TeamIndex (PSN Match TeamId) to join. Else UNSPECIFIED_TEAM_INDEX.
    ***************************************************************************************************/
    void ExternalSessionUtilPs5Matches::toPsnReqPlayer(MatchPlayer& psnPlayer, const UserIdentification& blazeUser,
        Blaze::GameManager::TeamIndex blazeJoinTeamIndex /*= UNSPECIFIED_TEAM_INDEX*/) const
    {
        eastl::string buf;
        psnPlayer.setAccountId(ExternalSessionUtilPs5::getPsnAccountId(buf, blazeUser).c_str());
        psnPlayer.setPlayerId(toPsnReqPlayerId(buf, blazeUser).c_str());
        psnPlayer.setPlayerType(PsnPlayerTypeEnumToString(blazeUser.getPlatformInfo().getClientPlatform() == getPlatform() ? PSN_PLAYER : NON_PSN_PLAYER));//support in case of future crossplay non psn players. No NPC support currently
        psnPlayer.setPlayerName(ExternalSessionUtil::getValidJsonUtf8(blazeUser.getName(), blazeUser.getNameMaxStringLength()));
        psnPlayer.setJoinFlag(true);

        if (blazeJoinTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            //only join req has a teamId in it
            psnPlayer.setTeamId(toPsnReqTeamId(buf, blazeJoinTeamIndex).c_str());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief helper to populate PSN create request's teams
    ***************************************************************************************************/
    void ExternalSessionUtilPs5Matches::toPsnReqTeams(MatchInGameRoster& createReqRoster, 
        const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo) const
    {
        if (!hasTeams(gameInfo))
            return;

        auto callerTeamIndex = callerParams.getChangedGame().getPlayer().getTeamIndex();
        if (callerTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            toPsnReqTeam(*createReqRoster.getTeams().pull_back(), callerTeamIndex, &callerParams.getUserIdentification(), gameInfo.getExternalSessionCreationInfo().getUpdateInfo());
        }
        for (TeamIndex i = 0; i < gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTeamIdsByIndex().size(); ++i)
        {
            // create the other empty teams
            if (callerTeamIndex != i)
                toPsnReqTeam(*createReqRoster.getTeams().pull_back(), i, nullptr, gameInfo.getExternalSessionCreationInfo().getUpdateInfo());
        }
    }
    /*! ************************************************************************************************/
    /*! \brief helper to populate PSN Match request's team
    ***************************************************************************************************/
    void ExternalSessionUtilPs5Matches::toPsnReqTeam(MatchTeam& psnTeam, TeamIndex blazeTeamIndex, const UserIdentification* blazeUserToAdd, const ExternalSessionUpdateInfo& gameInfo) const
    {
        eastl::string buf;
        psnTeam.setTeamId(toPsnReqTeamId(buf, blazeTeamIndex).c_str());
        if (blazeUserToAdd)
        {
            auto psnTeamMember = psnTeam.getMembers().pull_back();
            psnTeamMember->setPlayerId(toPsnReqPlayerId(buf, *blazeUserToAdd).c_str());
            psnTeamMember->setJoinFlag(true);
        }
        auto psnTeamName = toPsnReqTeamName(blazeTeamIndex, gameInfo);
        if (psnTeamName)
        {
            psnTeam.setTeamName(psnTeamName);
        }
        // (re)sync joined tracked Match team members as needed.
        auto trackedMatchMembers = getListFromMap(gameInfo.getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if (trackedMatchMembers)
        {
            ASSERT_COND_LOG(!blazeUserToAdd || trackedMatchMembers->empty(), "toPsnReqTeam: unexpected: user(" << toExtUserLogStr(*blazeUserToAdd) << ") creating Match but master tracking already has other Match members? Expected empty.");
            for (auto player : *trackedMatchMembers)
            {
                if (player->getTeamIndex() != blazeTeamIndex)
                    continue;
                auto psnTeamMember = psnTeam.getMembers().pull_back();
                psnTeamMember->setPlayerId(toPsnReqPlayerId(buf, player->getUserIdentification()).c_str());
                psnTeamMember->setJoinFlag(true);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief helper to populate PSN request's team name from the Blaze TeamId, and configured TeamNameByTeamIdMap.
        side: input blazeTeamId is not a PSN teamId (which comes from a Blaze TeamIndex)
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs5Matches::toPsnReqTeamName(GameManager::TeamIndex blazeTeamIndex, const ExternalSessionUpdateInfo& gameInfo) const
    {
        ASSERT_COND_LOG((blazeTeamIndex == UNSPECIFIED_TEAM_INDEX) || (blazeTeamIndex < gameInfo.getTeamIdsByIndex().size()), "toPsnReqTeamName: teamIndex(" << blazeTeamIndex << ") > max avail(" << gameInfo.getTeamIdsByIndex().size() << "), no poss teamName");
        if ((blazeTeamIndex == UNSPECIFIED_TEAM_INDEX) || !hasTeams(gameInfo.getTeamIdsByIndex()) || (blazeTeamIndex >= gameInfo.getTeamIdsByIndex().size()))
        {
            return nullptr;
        }
        auto toNameIt = gameInfo.getTeamNameByIndex().find(blazeTeamIndex);
        return ((toNameIt != gameInfo.getTeamNameByIndex().end()) ? toNameIt->second.c_str() : nullptr);
    }

    /*! ************************************************************************************************/
    /*! \brief Return the Match LeaveReason, based on the inputs
    ***************************************************************************************************/
    PsnLeaveReasonEnum ExternalSessionUtilPs5Matches::toMatchLeaveReason(const UserIdentification& userIdentification, const char8_t* matchId, GameId gameId, PlayerRemovedReason removeReason, const GetMatchDetailResponse& matchDetail) const
    {
        PsnLeaveReasonEnum leaveReason = INVALID_LEAVE_REASON_TYPE;
        switch (removeReason)
        {
        case GameManager::PLAYER_CONN_POOR_QUALITY:
        case GameManager::PLAYER_JOIN_TIMEOUT:
        case GameManager::PLAYER_CONN_LOST:
        case GameManager::BLAZESERVER_CONN_LOST:
        case GameManager::MIGRATION_FAILED:
            leaveReason = PSNServices::Matches::DISCONNECTED;
            break;
        default:
        {
            PsnMatchStatusEnum status = INVALID_MATCH_STATUS_TYPE;
            ASSERT_COND_LOG(ParsePsnMatchStatusEnum(matchDetail.getStatus(), status), logPrefix() << ".toMatchLeaveReason: unknown match status type(" << matchDetail.getStatus() << ")");
            switch (status)
            {
            case PLAYING:
            {
                // Once PLAYING, we pick FINISHED or QUIT:
                // Based on whether Blaze game finished
                GetExternalSessionInfoMasterResponse gameInfo;
                auto err = mTrackHelper.getGameInfoFromMaster(gameInfo, gameId, userIdentification);
                leaveReason = (((gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSimplifiedState() == GAME_PLAY_STATE_POST_GAME) || (err == GAMEMANAGER_ERR_INVALID_GAME_ID)) ?
                    PSNServices::Matches::FINISHED :
                    PSNServices::Matches::QUIT);
                break;
            }
            default:
                leaveReason = PSNServices::Matches::QUIT;
            }
        }
        }
        TRACE_LOG(logPrefix() << ".toMatchLeaveReason: (" << PsnLeaveReasonEnumToString(leaveReason) << "), for Blaze removeReason(" << PlayerRemovedReasonToString(removeReason) << ").");
        return leaveReason;
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether the PSN Match already completed or canceled, in which case further ops on it would just error.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isMatchAlreadyOver(const char8_t* currMatchStatus, const char8_t* opLogContext)
    {
        PsnMatchStatusEnum parsedMatchStatus = INVALID_MATCH_STATUS_TYPE;
        bool parseOk = (currMatchStatus && ParsePsnMatchStatusEnum(currMatchStatus, parsedMatchStatus));
        ASSERT_COND_LOG(parseOk, "[ExternalSessionUtilPs5Matches].isMatchAlreadyOver: unknown status(" << currMatchStatus << ").");
        if (parsedMatchStatus == PSNServices::Matches::CANCELLED || parsedMatchStatus == PSNServices::Matches::COMPLETED)
        {
            TRACE_LOG("[ExternalSessionUtilPs5Matches].isMatchAlreadyOver: Match over in status(" << currMatchStatus << " (no further " << (opLogContext ? opLogContext : "updates") << " possible/needed on it).");
            return true;
        }
        return false;
    }
    bool ExternalSessionUtilPs5Matches::isMatchAlreadyOverErr(BlazeRpcError psnErr, const ExternalSessionErrorInfo& errorInfo, const CommandInfo& cmdInfo)
    {
        // This rsp code (with HTTP 400) typically means Match already CANCELLED/COMPLETED, not necessarily an error to log. Caller should handle as appropriate
        bool result = ((psnErr == PSNMATCHES_BAD_REQUEST) && (errorInfo.getCode() == 2270211));
        if (result)
        {
            TRACE_LOG("[ExternalSessionUtilPs5Matches].isMatchAlreadyOverErr: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " failed, Match likely already over, not necessarily an error, err(" << ErrorHelp::getErrorName(psnErr) << "), msg: " << errorInfo.getMessage());
        }
        return result;
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether the Blaze player should be in matches for games
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::shouldPlayerBeInMatches(const UserIdentification& userIdent, PlayerState playerState)
    {
        // If reserved you won't be in the external session by GM spec.
        return ((userIdent.getPlatformInfo().getClientPlatform() == ps5) && (playerState != RESERVED));
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether error code indicates whether PSN call maybe retry-able
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isRetryError(BlazeRpcError psnErr)
    {
        return
            (psnErr == ERR_TIMEOUT) ||
            (psnErr == PSNMATCHES_SERVICE_UNAVAILABLE) ||
            (psnErr == PSNMATCHES_SERVICE_INTERNAL_ERROR) ||
            (psnErr == PSNMATCHES_CONFLICTING_REQUEST);//rare Sony issue seen in tests (https://p.siedev.net/support/issue/8425#n101574)
    }

    /*! ************************************************************************************************/
    /*! \brief Return whether error code indicates whether PSN call maybe retry-able, after refreshing token
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5Matches::isTokenExpiredError(BlazeRpcError psnErr, const PSNServices::PsnErrorResponse& psnRsp)
    {
        bool result(psnErr == PSNMATCHES_AUTHENTICATION_REQUIRED);
        if (result)
        {
            WARN_LOG("[ExternalSessionUtilPs5Matches].isTokenExpiredError: authentication with Sony failed, token may have expired. PSN error(" <<
                ErrorHelp::getErrorName(psnErr) << "), " << ErrorHelp::getErrorDescription(psnErr) << "). (Code: " <<
                psnRsp.getError().getCode() << " Message: " << psnRsp.getError().getMessage() << " ) Blaze may refresh and retry the request.");
        }
        return result;
    }
    
    /*! ************************************************************************************************/
    /*! \brief log helper
    ***************************************************************************************************/
    const FixedString256 ExternalSessionUtilPs5Matches::toLogStr(const ExternalSessionIdentification& extSessIdent) const
    {
        return GameManager::toLogStr(extSessIdent, getActivityTypeInfo());
    }


    ////////////////////////////// PSN Connection //////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief Call to PSN
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5Matches::callPSN(const UserIdentification* user, const CommandInfo& cmdInfo,
        PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo, HttpConnectionManagerPtr psnConnMgrPtr,
        const ExternalServiceCallOptions &config)
    {
        BlazeError err = ERR_OK;

        int32_t retryCountTotal = 0; // used to limit the maximum number of retry attempts, overall.
        int32_t retryCountServiceUnavailable = 0; // used to limit the number of retries *if* the HTTP response is Service Unavailable.
        while ((retryCountServiceUnavailable <= config.getServiceUnavailableRetryLimit()) && (retryCountTotal++ <= config.getTotalRetryLimit()))
        {
            PSNServices::PsnErrorResponse rspErr;

            // Get the current PSN s2s access token. May have been refreshed if this is a second/third/etc. retry.
            // This is kinda weird, but 'reqHeader' is actually always a member of 'req', and is just passed in this way
            // in order to make callPSN be able to take a generic Tdf* request object. This could be cleaned up by using
            // a templated function.
            reqHeader.setAuthToken(gOAuthManager->getPsnServerAccessToken(user).c_str());

            err = ExternalSessionUtilPs5::callPSNInternal(psnConnMgrPtr, cmdInfo, req, rsp, &rspErr);

            // If everything is ERR_OK, then we can break out of this loop.
            if (err == ERR_OK)
            {
                TRACE_LOG("[ExternalSessionUtilPs5Matches].callPSN: " << cmdInfo.context << " result(" << err << "), user(" << (user ? toExtUserLogStr(*user) : "N/A") << ")");
                break;
            }

            // The initial call failed for some reason. If the error indicates that our token is
            // expired (which should happen very rarely) then ask the TokenUtil to go get a new one.
            if (isTokenExpiredError(err, rspErr))
                err = gOAuthManager->requestNewPsnServerAccessTokenAndWait();

            // If either the original call to PSN or the request for a new token, failed with an error that
            // indicates we should try again "shortly", then we wait.
            if (isRetryError(err))
            {
                //On http conflict, add randomize waits, to mitigate issues seen if 2 calls are o.w. too close together (https://p.siedev.net/support/issue/8425)
                auto& waitTime = ((err == PSNMATCHES_CONFLICTING_REQUEST) ? (config.getConflictBackoff() + (gRandom->getRandomNumber((int)config.getConflictBackoffRange().getMillis()) * 1000)) : config.getServiceUnavailableBackoff());
                if (err != PSNMATCHES_CONFLICTING_REQUEST)
                    retryCountServiceUnavailable++;
                err = ExternalSessionUtilPs5::waitTime(waitTime, cmdInfo.context, retryCountTotal);
            }

            // If we're all the way down here, and 'err' is not ERR_OK, then we've hit an unexpected case
            // and we should not retry the request.
            if (err != ERR_OK)
            {
                // Error is logged by caller as well, hence lowering verbosity here
                ExternalSessionUtilPs5::toExternalSessionErrorInfo(errorInfo, rspErr);
                TRACE_LOG("[ExternalSessionUtilPs5Matches].callPSN: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " failed, " << ExternalSessionUtilPs5::toLogStr(err, &rspErr) << ". User(" << (user ? toExtUserLogStr(*user) : "N/A") << ")");
                break;
            }

            TRACE_LOG("[ExternalSessionUtilPs5Matches].callPSN: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " result(" << ErrorHelp::getErrorName(err) << "). User(" << (user ? toExtUserLogStr(*user) : "N/A") << ").");
        }

        return err;
    }
    
}//GameManager
}//Blaze
