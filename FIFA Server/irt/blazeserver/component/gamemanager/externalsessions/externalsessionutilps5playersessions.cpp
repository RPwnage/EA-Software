/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5playersessions.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/externalsessions/externalsessionutilps5playersessions.h"
#include "framework/logger.h"
#include "framework/usersessions/usersessionmanager.h" // for gUserSessionManager in hasUpdatePrimaryExternalSessionPermission()
#include "framework/connection/outboundhttpservice.h" // for gOutboundHttpService in createPlayersession
#include "framework/rpc/oauth_defines.h"//for OAUTH_ERR_INVALID_REQUEST in callPSN
#include "gamemanager/shared/externalsessionjsondatashared.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"//for shouldRetryAfterExternalSessionUpdateError in addJoinableSpecifiedUsers
#include "gamemanager/externalsessions/externalsessionutilps5.h"//for getPsnAccountId() in toReqPlayer
#include "gamemanager/commoninfo.h"//for getTotalParticipantCapacity in toMaxPlayers
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagermasterimpl.h"//for GameManagerMasterImpl::isUUIDFormat in verifyParams

#include "psnsessionmanager/tdf/psnsessionmanager.h"
#include "psnsessionmanager/rpc/psnsessionmanagerslave.h"


namespace Blaze
{
using namespace PSNServices::PlayerSessions;
namespace GameManager
{
    ExternalSessionUtilPs5PlayerSessions::ExternalSessionUtilPs5PlayerSessions(ClientPlatformType platform, const ExternalSessionServerConfig& config, GameManagerSlaveImpl* gameManagerSlave):
        mTokenUtil(config.getUseMock()),
        mTrackHelper(gameManagerSlave, ps5, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY, platform), //activity platform is the same 'ps5' here, even for ps4 clients, to ensure PlayerSessions 'shared' crossgen
        mPlatform(platform)
    {
        config.copyInto(mConfig);
        ExternalSessionUtilPs5::getNewWebApiConnMgr(mPsnConnMgrPtr, PSNSessionManagerSlave::COMPONENT_INFO.name);
    }

    ExternalSessionUtilPs5PlayerSessions::~ExternalSessionUtilPs5PlayerSessions()
    {
        if (mPsnConnMgrPtr != nullptr)
            mPsnConnMgrPtr.reset();
    }

    bool ExternalSessionUtilPs5PlayerSessions::verifyParams(const ExternalSessionServerConfig& config)
    {
        if (isPlayerSessionsDisabled(config))
        {
            return true;
        }

        if (config.getExternalSessionServiceLabel() == INVALID_EXTERNAL_SESSION_SERVICE_LABEL)
        {
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].verifyParams: Invalid PSN service label(" << config.getExternalSessionServiceLabel() << ").");
            return false;
        }

        if ((config.getPsnPlayerSessionPersistedIdPrefix()[0] != '\0') && GameManagerMasterImpl::isUUIDFormat(config.getPsnPlayerSessionPersistedIdPrefix()))
        {
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].verifyParams: Configured PsnPlayerSessionPersistedIdPrefix(" << config.getPsnPlayerSessionPersistedIdPrefix() << ") must not be in UUID format.");
            return false;
        }

        if ((strlen(config.getPsnPlayerSessionPersistedIdPrefix()) + Blaze::MAX_UUID_LEN) > Blaze::GameManager::MAX_PERSISTEDGAMEID_LEN)
        {
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].verifyParams: Configured PsnPlayerSessionPersistedIdPrefix(" << config.getPsnPlayerSessionPersistedIdPrefix() << ") too long by (" << ((strlen(config.getPsnPlayerSessionPersistedIdPrefix()) + Blaze::MAX_UUID_LEN) - Blaze::GameManager::MAX_PERSISTEDGAMEID_LEN) << ") characters.");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief get auth token used for updating PlayerSessions
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::getAuthToken(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const char8_t* serviceName, bool forceRefresh)
    {
        eastl::string buf;
        BlazeRpcError err = mTokenUtil.getUserOnlineAuthToken(buf, caller, serviceName, forceRefresh, mConfig.getCrossgen());
        if (err == ERR_OK)
            authInfo.setCachedExternalSessionToken(buf.c_str());
        return err;
    }


    bool ExternalSessionUtilPs5PlayerSessions::hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) const
    {
        return (ExternalSessionUtil::hasJoinExternalSessionPermission(userSession) &&
            gUserSessionManager->getClientTypeDescription(userSession.getClientType()).getPrimarySession()
            && (userSession.getUserSessionType() != USER_SESSION_GUEST));
    }

    /*! ************************************************************************************************/
    /*! \brief set a PlayerSession as user's primary joinable activity for the 1st party UX
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult)
    {
        const UserIdentification& user = params.getUserIdentification();
        const auto& authInfo = params.getAuthInfo();
        if (isPlayerSessionsDisabled())
        {
            TRACE_LOG(logPrefix() << ".setPrimary: External sessions explicitly disabled. No primary will be set for user(" << toExtUserLogStr(user) << ").");
            return ERR_OK;
        }
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (user.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (params.getNewPrimaryGame().getGameId() == INVALID_GAME_ID))
        {
            WARN_LOG(logPrefix() << ".setPrimary: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << "), psnId(" << user.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or GameId(" << params.getNewPrimaryGame().getGameId() << "). No op. user(" << toExtUserLogStr(user) << ").");
            return ERR_SYSTEM;
        }

        const char8_t* pushContextId = updatePushContextIdIfMockPlatformEnabled(params.getPsnPushContextId());
        if (pushContextId[0] == '\0')
        {
            WARN_LOG(logPrefix() << ".setPrimary: missing pushContextId. No op. user(" << toExtUserLogStr(user) << ").");
            return ERR_SYSTEM;
        }


        // If already did it, we're done
        GetExternalSessionInfoMasterResponse gameInfoBuf;
        if (!mTrackHelper.isAddUserToExtSessionRequired(gameInfoBuf, params.getNewPrimaryGame().getGameId(), params.getUserIdentification()))
        {
            gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().copyInto(result);
            return ERR_OK;
        }
        // 1st track as left any old primary session on master (guard vs timing issues)
        if (params.getOldPrimaryGameId() != INVALID_GAME_ID)
        {
            ExternalMemberInfo memberInfo;
            user.copyInto(memberInfo.getUserIdentification());
            authInfo.copyInto(memberInfo.getAuthInfo());
            auto err = mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, params.getOldPrimaryGameId(), &memberInfo, nullptr);
            if (err != ERR_OK)
                return err;
        }

        // Ensure have primary game's latest params for the PSN call
        BlazeRpcError err = mTrackHelper.getGameInfoFromMaster(gameInfoBuf, params.getNewPrimaryGame().getGameId(), user);
        if (err != ERR_OK || !gameInfoBuf.getIsGameMember())
        {
            // ok if target game was left, its remove notifications will trigger re-choosing primary session soon enough
            return ((err == GAMEMANAGER_ERR_INVALID_GAME_ID) ? ERR_OK : err);
        }


        // If game already has a PlayerSession, join, otherwise create. store result
        if (gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId()[0] != '\0')
        {
            err = joinPlayerSession(result, errorResult, params, gameInfoBuf);
        }
        else
        {
            err = createPlayerSession(result, errorResult, params, gameInfoBuf);
        }

        // ok if target game was left, its remove notifications will trigger re-choosing primary session soon enough
        return ((err == GAMEMANAGER_ERR_INVALID_GAME_ID) ? ERR_OK : err);
    }

    /*! ************************************************************************************************/
    /*! \brief clear user's primary PlayerSession
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult)
    {
        auto& caller = params.getUserIdentification();
        if (isPlayerSessionsDisabled())
        {
            return ERR_OK;
        }

        PsnPlayerSessionId origPrimarySessId;
        ExternalSessionBlazeSpecifiedCustomDataPs5 origPrimarySessData;
        BlazeRpcError getErr = getPrimaryPlayerSessionIdAndData(origPrimarySessId, origPrimarySessData, errorResult, params.getAuthInfo(), caller);
        if (getErr == PSNSESSIONMANAGER_ACCESS_FORBIDDEN)
        {
            WARN_LOG(logPrefix() << ".clearPrimary: No op. Could not get primary PlayerSession, PSN may have already signed out / cleaned up for the caller(" << toExtUserLogStr(caller) << ").");
            return ERR_OK;
        }
        if (getErr != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".clearPrimary: Failed checking for existing primary PlayerSession. Caller(" << toExtUserLogStr(caller) << ", " << ErrorHelp::getErrorName(getErr) << ").");
            return getErr;
        }
        if (origPrimarySessId.empty())
        {
            TRACE_LOG(logPrefix() << ".clearPrimary: No existing primary PlayerSession to clear for user(" << toExtUserLogStr(caller) << "). No op.");
            return ERR_OK;
        }
        TRACE_LOG(logPrefix() << ".clearPrimary: clearing existing primary PlayerSession(" << origPrimarySessId << ") for user(" << toExtUserLogStr(caller) << ").");

        BlazeRpcError err = leavePlayerSession(errorResult, params.getAuthInfo(), params.getUserIdentification(), origPrimarySessId.c_str(), origPrimarySessData.getGameId(), true);
        if (err == PSNSESSIONMANAGER_RESOURCE_NOT_FOUND)
        {
            // Sony already cleaned
            err = ERR_OK;
        }
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".clearPrimary: failed to clear user's primary PlayerSession(" << origPrimarySessId.c_str() << "). Caller(" << toExtUserLogStr(caller) << "), " << ErrorHelp::getErrorName(err));
        }
        return err;
    }


    /*! ************************************************************************************************/
    /*! \brief creates the 1st party PlayerSession
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::createPlayerSession(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo,
        const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo)
    {
        auto& caller = callerParams.getUserIdentification();
        auto authInfo = callerParams.getAuthInfo();//orig is const, just to ensure owner etc unchanged
        auto& creationInfo = gameInfo.getExternalSessionCreationInfo();
        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID))
        {
            WARN_LOG(logPrefix() << ".createPlayerSession: missing authtoken(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "). Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        if (callerParams.getPsnPushContextId()[0] == '\0')
        {
            ERR_LOG(logPrefix() << ".createPlayerSession: user(" << toExtUserLogStr(caller) << ") missing pushContextId, cannot create game(" << gameId << ")'s PlayerSession.");
            return ERR_SYSTEM;
        }
        validateGameSettings(creationInfo.getUpdateInfo().getSettings());
        
        // setup req:
        PSNServices::PlayerSessions::CreatePlayerSessionResponse rsp;
        PSNServices::PlayerSessions::CreatePlayerSessionRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        auto& sessReq = *req.getBody().getPlayerSessions().pull_back();
        // setup serviceLabel (needed for S2S)
        sessReq.setNpServiceLabel(getServiceLabel());
        
        // setup req customdata1 (Blaze-specified, by Blaze spec)
        eastl::string custData1Buf;
        if (!toPlayerSessionCustomData1(custData1Buf, creationInfo.getUpdateInfo()))
        {
            return ERR_SYSTEM;//logged
        }
        sessReq.setCustomData1(custData1Buf.c_str());

        // setup req customdata2 (title-specifiable, by Blaze spec)
        if (creationInfo.getExternalSessionCustomData().getCount() > 0)//PSN errs if try send empty
        {
            eastl::string custData2Buf;
            if (!toPlayerSessionCustomData2(custData2Buf, creationInfo.getExternalSessionCustomData()))
            {
                return ERR_SYSTEM;
            }
            sessReq.setCustomData2(custData2Buf.c_str());
        }

        // setup req player
        toPsnReqPlayer(*sessReq.getMember().getPlayers().pull_back(), caller, callerParams.getPsnPushContextId());

        // setup req sessionName
        toMultilingualSessionName(sessReq.getLocalizedSessionName(), creationInfo.getUpdateInfo());

        // setup req leaderPrivileges. (Blaze marks as set to send empty 'omit-all' list by default:)
        toLeaderPrivileges(sessReq.getLeaderPrivileges());

        // setup req rest
        sessReq.setMaxPlayers(toMaxPlayers(creationInfo.getUpdateInfo()));
        sessReq.setMaxSpectators(toMaxSpectators(creationInfo.getUpdateInfo()));
        sessReq.setJoinableUserType(toJoinableUserType(creationInfo.getUpdateInfo()));
        sessReq.setJoinDisabled(toJoinDisabledFlag(creationInfo.getUpdateInfo()));
        toSupportedPlatforms(sessReq.getSupportedPlatforms(), gameInfo);
        sessReq.setInvitableUserType(toInvitableUserType(creationInfo.getUpdateInfo()));
        //swapSupported kept by default false. slot changes via UX currently unsupported by Blaze
        
        TRACE_LOG(logPrefix() << ".createPlayerSession: creating PSN PlayerSession for game(" << gameId << ") with user(" << toExtUserLogStr(caller) << "), serviceLabel(" << getServiceLabel() <<
            "), name(" << creationInfo.getUpdateInfo().getGameName() << "), maxPlayers(" << sessReq.getMaxPlayers() << "), maxSpectators(" << sessReq.getMaxSpectators() << "), joinDisabled(" << sessReq.getJoinDisabled() << "), supportedPlatforms(" << sessReq.getSupportedPlatforms() << "), crossgenEnabled(" << (mConfig.getCrossgen() ? "true" : "false") << ").");


        // send request to PSN
        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_CREATEPLAYERSESSION;
        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, errorInfo);
        if ((err != ERR_OK) || EA_UNLIKELY(rsp.getPlayerSessions().empty()))
        {
            ERR_LOG(logPrefix() << ".createPlayerSession: failed to create user(" << toExtUserLogStr(caller) << ")'s game(" << gameId << ")'s PlayerSesion, rsp gamecount(" << rsp.getPlayerSessions().size() << "), " << ErrorHelp::getErrorName(err));
            return err;
        }
        // else created!
        gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().copyInto(resultSessionIdent.getPs5());
        resultSessionIdent.getPs5().getPlayerSession().setPlayerSessionId(rsp.getPlayerSessions().front()->getSessionId());//add the new PlayerSessionId

        // Ensure Blaze Game and its PlayerSession, and member tracking are in sync
        err = syncPlayerSessionAndTrackedMembership(resultSessionIdent, errorInfo, authInfo, callerParams, gameInfo);
        if (err == ERR_OK)
        {
            TRACE_LOG(logPrefix() << ".createPlayerSession: user(" << toExtUserLogStr(caller) << ") successfully created game(" << gameId << ")'s PlayerSession(" << resultSessionIdent.getPs5().getPlayerSession().getPlayerSessionId() << ").");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief For the shell 'boot to new create-or-join game', 1st to join the empty PlayerSession should init its values, using this.
    ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtilPs5PlayerSessions::initAllPlayerSessionValues(const ExternalUserAuthInfo& origAuthInfo, const ExternalSessionCreationInfo& params, const UserIdentification& caller)
    {
        auto authInfo = origAuthInfo;
        BlazeRpcError retErr = ERR_OK, err = ERR_OK;

        // Mirror createPlayerSession, except update.  Order:
        // customData2 as title client may often use it to determine joinGame params.
        // customData1 to ungate regular join flow that needs its GameId (plus avoids poss excess create-or-join tries).
        // De-prioritize others that have Blaze already as backup to prevent the joins, or purely aesthetic ones.

        if (params.getExternalSessionCustomData().getCount() > 0)//PSN errs if try send empty
        {
            err = updateCustomDataTitlePart(authInfo, caller, params);
            if (err != ERR_OK)
                retErr = err;
        }

        err = updateCustomDataBlazePart(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;
        
        err = updateJoinableUserType(authInfo, caller, params.getUpdateInfo());// do joinableUserType, invitableUserType before any removing needed leaderPrivileges below
        if (err != ERR_OK)
            retErr = err;

        err = updateInvitableUserType(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;

        err = updateLeaderPrivileges(authInfo, caller, params);
        if (err != ERR_OK)
            retErr = err;

        err = updateJoinDisabled(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;

        err = updateMaxPlayers(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;

        err = updateMaxSpectators(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;

        err = updateSessionName(authInfo, caller, params.getUpdateInfo());
        if (err != ERR_OK)
            retErr = err;

        return retErr;
    }


    /*! ************************************************************************************************/
    /*! \brief joins the specified 1st party PlayerSession. Creates game's PlayerSession if specified PlayerSession is n/a.
        \param[out] resultSessionIdent the joined PlayerSession's info, populated/updated on success
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::joinPlayerSession(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo,
        const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo)
    {
        const bool isSpectator = isSpectatorSlot(callerParams.getNewPrimaryGame().getPlayer().getSlotType());

        auto& caller = callerParams.getUserIdentification();
        auto authInfo = callerParams.getAuthInfo();
        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        const char8_t* playerSessionId = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (callerParams.getPsnPushContextId()[0] == '\0') || (playerSessionId[0] == '\0'))
        {
            //(note PS5 external sessions do not currently support members without EA PSN accounts)
            WARN_LOG(logPrefix() << ".joinPlayerSession: missing authToken(" << authInfo.getCachedExternalSessionToken() << "), psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << ", pushContextId(" << callerParams.getPsnPushContextId() << ") or playerSessionId(" << playerSessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        PSNServices::PlayerSessions::JoinPlayerSessionResponse rsp;
        PSNServices::PlayerSessions::JoinPlayerSessionRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(playerSessionId);
        auto& reqMemberList = (isSpectator ? req.getBody().getSpectators() : req.getBody().getPlayers());
        toPsnReqPlayer(*reqMemberList.pull_back(), caller, callerParams.getPsnPushContextId());
        TRACE_LOG(logPrefix() << ".joinPlayerSession: joining PSN PlayerSession(" << playerSessionId << ") for game(" << gameId << ") user(" << toExtUserLogStr(caller) << ")");

        const CommandInfo& cmd = (isSpectator ? PSNSessionManagerSlave::CMD_INFO_JOINPLAYERSESSIONASSPECTATOR : PSNSessionManagerSlave::CMD_INFO_JOINPLAYERSESSIONASPLAYER);

        //At this pt user already got in Blaze Game, to ensure in sync, and, allow MM/internal bypassing of restrictions, Sony recommends white-listing by joinableSpecifiedUsers:
        bool bypass = shouldBypassPlayerSessionRestrictionsForJoin(gameInfo);
        if (bypass)
        {
            addJoinableSpecifiedUsers(errorInfo, caller, gameInfo);
        }

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, errorInfo);
        if (bypass)
        {
            //revert. (joinableSpecifiedUsers is only used by Blaze here, this IS source of truth for whether a user goes in it)
            deleteJoinableSpecifiedUsers(errorInfo, authInfo, caller, gameInfo, caller);
        }
        if (err == PSNSESSIONMANAGER_RESOURCE_NOT_FOUND)
        {
            // PlayerSession went away before could join it, Sony recommends simply creating one for game.
            // If Blaze game somehow still has that PlayerSession id (possible Sony/network issues), remove it first:
            mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, gameId, nullptr, &gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification());
            TRACE_LOG(logPrefix() << ".joinPlayerSession: PlayerSession(" << playerSessionId << ") was removed before user could join it. Creating new PlayerSession for game(" << gameId << ") instead. Caller(" << toExtUserLogStr(caller) << ").");
            return createPlayerSession(resultSessionIdent, errorInfo, callerParams, gameInfo);
        }
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".joinPlayerSession: failed to join user(" << toExtUserLogStr(caller) << ") to game(" << gameId << ")'s PlayerSession(" << playerSessionId << "), " << ErrorHelp::getErrorName(err));
            return err;
        }
        // joined
        gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().copyInto(resultSessionIdent.getPs5());

        // handle create or join as needed
        const auto* trackedMembers = getListFromMap(gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), getActivityTypeInfo());
        if ((trackedMembers == nullptr) || trackedMembers->empty())
        {
            // the first to join from the shell created (empty) PlayerSession, should init all values
            initAllPlayerSessionValues(authInfo, gameInfo.getExternalSessionCreationInfo(), caller);
        }

        // Ensure Blaze Game and its PlayerSession, and member tracking are in sync
        err = syncPlayerSessionAndTrackedMembership(resultSessionIdent, errorInfo, authInfo, callerParams, gameInfo);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief leaves the specified PSN PlayerSession
        \param[in] untrackBlazeMembership If true, track user as not in the external session, on master game
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::leavePlayerSession(ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& origAuthInfo,
        const UserIdentification& user, const char8_t* playerSessionId, GameId gameId, bool untrackBlazeMembership)
    {
        ExternalPsnAccountId psnAccountId = user.getPlatformInfo().getExternalIds().getPsnAccountId();
        auto authInfo = origAuthInfo;
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (psnAccountId == INVALID_PSN_ACCOUNT_ID) || (playerSessionId == nullptr) || (playerSessionId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".leavePlayerSession: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnAcctId(" << psnAccountId << "), or PlayerSessionId(" << ((playerSessionId != nullptr) ? playerSessionId : "<null>") << "). No op. Caller(" << toExtUserLogStr(user) << ").");
            return ERR_SYSTEM;
        }
        TRACE_LOG(logPrefix() << ".leavePlayerSession: leaving user(" << toExtUserLogStr(user) << ") from game(" << gameId << ")'s PlayerSession(" << playerSessionId << ").");

        // Before leaving, track as not in the PSN PlayerSession centrally on master. (prevent further joins to a dying session)
        if (untrackBlazeMembership)
        {
            ExternalMemberInfo memberInfo;
            user.copyInto(memberInfo.getUserIdentification());
            authInfo.copyInto(memberInfo.getAuthInfo());
            mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, gameId, &memberInfo, nullptr);
        }

        PSNServices::PlayerSessions::LeavePlayerSessionRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(playerSessionId);
        eastl::string accountBuf;
        req.setAccountId(ExternalSessionUtilPs5::getPsnAccountId(accountBuf, user).c_str());
        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_LEAVEPLAYERSESSION;

        BlazeRpcError err = callPSN(user, cmd, authInfo, req.getHeader(), req, nullptr, errorInfo);
        if (err == PSNSESSIONMANAGER_RESOURCE_NOT_FOUND)
        {
            WARN_LOG(logPrefix() << ".leavePlayerSession: failed to leave user(" << toExtUserLogStr(user) << ") from game(" << gameId << ")'s PlayerSession(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err));
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".leavePlayerSession: failed to leave user(" << toExtUserLogStr(user) << ") from game(" << gameId << ")'s PlayerSession(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err));
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Call after adding user to a 1st party PlayerSession, to ensure the Blaze game, its PSN PlayerSession id and members in sync.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::syncPlayerSessionAndTrackedMembership(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo,
        const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& origGameInfo)
    {
        const GameId gameId = origGameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        ExternalMemberInfo member;
        callerParams.getUserIdentification().copyInto(member.getUserIdentification());
        authInfo.copyInto(member.getAuthInfo());

        GetExternalSessionInfoMasterResponse rspGameInfo;
        auto err = mTrackHelper.trackExtSessionMembershipOnMaster(rspGameInfo, origGameInfo, member, resultSessionIdent);
        if (err != ERR_OK)
        {
            if (EA_UNLIKELY(err == GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION))
            {
                // someone changed the game's PSN PlayerSession. Resync by joining that instead, update resultSessionIdent
                TRACE_LOG(logPrefix() << ".syncPlayerSessionAndTrackedMembership: could not track user(" << toExtUserLogStr(member.getUserIdentification()) << ") with PlayerSession(" << resultSessionIdent.getPs5().getPlayerSession().getPlayerSessionId() << ") for Blaze game(" << gameId << "), as game has a different PlayerSession(" << rspGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId() << "). Resyncing, by moving user to game current PlayerSession instead.");
                return joinPlayerSession(resultSessionIdent, errorInfo, callerParams, rspGameInfo);//PSN auto-leaves old PlayerSession on joining another
            }
            // on failure, leave PlayerSession, to prevent issues
            leavePlayerSession(errorInfo, authInfo, member.getUserIdentification(), resultSessionIdent.getPs5().getPlayerSession().getPlayerSessionId(), gameId, false);
        }
        return err;
    }
    /*! ************************************************************************************************/
    /*! \brief Check whether should bypass first party join restrictions, at point of join call
        (to keep in sync w/blaze, and, allow MM/internal flows to bypass e.g. to fill just-vacated slots)
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::shouldBypassPlayerSessionRestrictionsForJoin(const GetExternalSessionInfoMasterResponse& gameInfo) const
    {
        auto& gameProps = gameInfo.getExternalSessionCreationInfo().getUpdateInfo();
        PresenceMode presenceMode = findPresenceModeForGamePlayerSessions(gameProps.getPresenceModeByPlatform());
        // As done since Gen3,4 have pres private block UX but not title level calls
        if (presenceMode == PRESENCE_MODE_PRIVATE)
        {
            auto joinableUserType = INVALID_JOINABLE_USER_TYPE;
            ParsePsnJoinableUserTypeEnum(toJoinableUserType(gameProps), joinableUserType);
            if (joinableUserType != SPECIFIED_USERS)//for consistency PRESENCE_MODE_PRIVATE should always be SPECIFIED_USERS. See toJoinableUserType
            {
                ERR_LOG(logPrefix() << ".addJoinableSpecifiedUsers: unexpected likely joinableUserType(" << toJoinableUserType(gameProps) << "), for game(" << gameProps.getGameId() << ")'s  PlayerSession(" << gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId() << "), presenceMode(" << PresenceModeToString(presenceMode) << ").");
                return false;
            }
            return true;
        }
        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief adds the specified user to the PSN PlayerSession joinableSpecifiedUsers list
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::addJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo,
        const UserIdentification& userToAdd, const GetExternalSessionInfoMasterResponse& gameInfo)
    {
        ExternalSessionUtil::BlazeIdSet tried;
        BlazeRpcError sessErr = ERR_OK;
        size_t attemptNum = 0;
        do
        {
            ExternalMemberInfo updater;
            sessErr = getUpdater(gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), updater, tried, nullptr);
            if (sessErr == ERR_OK)//logged if err
            {
                sessErr = addJoinableSpecifiedUsers(errorInfo, updater.getAuthInfo(), userToAdd, gameInfo, updater.getUserIdentification());
            }
        } while (shouldRetryAfterExternalSessionUpdateError(sessErr) && (++attemptNum <= mConfig.getExternalSessionUpdateRetryLimit()));
        return sessErr;
    }
    /*! ************************************************************************************************/
    /*! \brief adds the specified user to the PSN PlayerSession joinableSpecifiedUsers list
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::addJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo,
        const UserIdentification& userToAdd, const GetExternalSessionInfoMasterResponse& gameInfo, const UserIdentification& caller)
    {
        const char8_t* playerSessionId = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (playerSessionId == nullptr) || (playerSessionId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".addJoinableSpecifiedUsers: missing authtoken(" << authInfo.getCachedExternalSessionToken() << ") or PlayerSessionId(" << ((playerSessionId != nullptr) ? playerSessionId : "<null>") << "). Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }
        eastl::string accountBuf;
        AddPlayerSessionJoinableSpecifiedUsersResponse rsp;
        AddPlayerSessionJoinableSpecifiedUsersRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(playerSessionId);
        req.getBody().getJoinableSpecifiedUsers().pull_back()->setAccountId(ExternalSessionUtilPs5::getPsnAccountId(accountBuf, userToAdd).c_str());
 
        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_ADDPLAYERSESSIONJOINABLESPECIFIEDUSERS;
        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, errorInfo);
        if (err != ERR_OK)
        {
            WARN_LOG(logPrefix() << ".addJoinableSpecifiedUsers: failed to add user(" << toExtUserLogStr(userToAdd) << ") to PlayerSession(" << req.getSessionId() << ") joinableSpecifiedUsers, " << ErrorHelp::getErrorName(err));
            return err;
        }
        TRACE_LOG(logPrefix() << ".addJoinableSpecifiedUsers: add(" << req.getBody().getJoinableSpecifiedUsers() << ") -> rsp(" << rsp.getJoinableSpecifiedUsers() << ") for PlayerSession(" << playerSessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return err;
    }
    /*! ************************************************************************************************/
    /*! \brief delete the specified user from the PSN PlayerSession joinableSpecifiedUsers list
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::deleteJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo,
        const UserIdentification& userToDel, const GetExternalSessionInfoMasterResponse& gameInfo, const UserIdentification& caller)
    {
        const char8_t* playerSessionId = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        eastl::string accountBuf;
        DeletePlayerSessionJoinableSpecifiedUsersRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(playerSessionId);
        req.setAccountIds(ExternalSessionUtilPs5::getPsnAccountId(accountBuf, userToDel).c_str());
 
        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_DELETEPLAYERSESSIONJOINABLESPECIFIEDUSERS;
        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, errorInfo);
        if (err != ERR_OK)
        {
            WARN_LOG(logPrefix() << ".deleteJoinableSpecifiedUsers: failed to delete user(" << toExtUserLogStr(userToDel) << ") from PlayerSession(" << req.getSessionId() << ") joinableSpecifiedUsers, " << ErrorHelp::getErrorName(err));
            return err;
        }
        TRACE_LOG(logPrefix() << ".deleteJoinableSpecifiedUsers: deleted(" << req.getAccountIds() << ") from PlayerSession(" << playerSessionId << ") joinableSpecifiedUsers. Caller(" << toExtUserLogStr(caller) << ").");
        return err;
    }




    /*! ************************************************************************************************/
    /*! \brief retrieves the user's current primary PlayerSession and the GameId stored in its data, from PSN.
        \param[out] primaryPlayerSessionId set to the primary PlayerSession's id if user had one. Null omits.
        \param[out] gameId game id stored in session custom data of user's primary PlayerSession. INVALID_GAME_ID if user didn't have one.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::getPrimaryPlayerSessionIdAndData(PsnPlayerSessionId& primaryPlayerSessionId, ExternalSessionBlazeSpecifiedCustomDataPs5& primarySessionData,
        ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& origAuthInfo,
        const UserIdentification& caller)
    {
        auto authInfo = origAuthInfo;//orig is const, just to ensure owner etc unchanged
        primaryPlayerSessionId.set("");

        BlazeRpcError err = ERR_OK;
        // get primary session if user has one
        PSNServices::PlayerSessions::GetPlayerSessionIdResponseItem idRsp;
        err = getPrimaryPlayerSessionId(idRsp, errorInfo, authInfo, caller);
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getPrimaryPlayerSessionIdAndData: Failed to get pre-existing primary PlayerSessionId, Caller(" << toExtUserLogStr(caller) << "). " << ErrorHelp::getErrorName(err));
            return err;
        }
        if (idRsp.getSessionId()[0] == '\0')
        {
            // didn't have one
            return ERR_OK;
        }

        PSNServices::PlayerSessions::GetPlayerSessionResponseItem dataRsp;
        err = getPlayerSession(dataRsp, errorInfo, authInfo, idRsp.getSessionId(), caller);
        if (err == PSNSESSIONMANAGER_RESOURCE_NOT_FOUND)
        {
            // just went away
            return ERR_OK;
        }
        else if (err == PSNSESSIONMANAGER_ACCESS_FORBIDDEN)
        {
            WARN_LOG(logPrefix() << ".getPrimaryPlayerSessionIdAndData: cannot get PlayerSession(" << idRsp.getSessionId() << ") data, " << ErrorHelp::getErrorName(err) << ". Caller(" << toExtUserLogStr(caller) << ") may have just signed out?");
            return err;
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getPrimaryPlayerSessionIdAndData: Failed to get pre-existing primary PlayerSession(" << idRsp.getSessionId() << ") data. Caller(" << toExtUserLogStr(caller) << "). " << ErrorHelp::getErrorName(err));
            return err;
        }

        primaryPlayerSessionId.set(idRsp.getSessionId());
        eastl::string custDat1 = dataRsp.getCustomData1();
        if (!GameManager::psnPlayerSessionCustomDataToBlazeData(primarySessionData, custDat1) || (primarySessionData.getGameId() == INVALID_GAME_ID))
        {
            ERR_LOG(logPrefix() << ".getPrimaryPlayerSessionIdAndData: parse of PlayerSession(" << idRsp.getSessionId() << ") custom data failed, possible PSN or decoding issue. Caller(" << toExtUserLogStr(caller) << "). Parsed GameId(" << primarySessionData.getGameId() << ")");
            return ERR_SYSTEM;
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieves the user's current primary PSN PlayerSession's id.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::getPrimaryPlayerSessionId(PSNServices::PlayerSessions::GetPlayerSessionIdResponseItem& sessionResult, ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo,
        const UserIdentification& caller)
    {
        auto psnId = caller.getPlatformInfo().getExternalIds().getPsnAccountId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (psnId == INVALID_PSN_ACCOUNT_ID))
        {
            WARN_LOG(logPrefix() << ".getPrimaryPlayerSessionId: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << psnId << "). No op. Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        PSNServices::PlayerSessions::GetPlayerSessionIdsResponse rsp;
        PSNServices::PlayerSessions::GetPlayerSessionIdsRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        eastl::string accountBuf;
        req.setAccountId(ExternalSessionUtilPs5::getPsnAccountId(accountBuf, caller).c_str());
        req.setPlatformFilter(toSupportedPlatform(caller.getPlatformInfo().getClientPlatform())); //each user has max 1 PlayerSession and its for current client platform logged in as

        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_GETPLAYERSESSIONIDS;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, errorInfo);
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getPrimaryPlayerSessionId: Failed getting user(" << toExtUserLogStr(caller) << ") primary PlayerSession, " << ErrorHelp::getErrorName(err));
            return err;
        }

        // store to result
        if (!rsp.getPlayerSessions().empty())
        {
            ASSERT_COND_LOG((rsp.getPlayerSessions().size() < 2), logPrefix() << ".getPrimaryPlayerSessionId: user unexpectedly has " << rsp.getPlayerSessions().size() << " PlayerSessions. Having more than one may cause errors, as multiple PlayerSession membership is not currently supported. Assuming primary is the first. Caller(" << toExtUserLogStr(caller) << ").");
            rsp.getPlayerSessions().front()->copyInto(sessionResult);
        }
        TRACE_LOG(logPrefix() << ".getPrimaryPlayerSessionId: (" << toExtUserLogStr(caller) << ") currently has (" << ((sessionResult.getSessionId()[0] == '\0') ? "NO session" : sessionResult.getSessionId()) << ") as its primary PlayerSession.");
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieves the user's PSN PlayerSession.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::getPlayerSession(PSNServices::PlayerSessions::GetPlayerSessionResponseItem& result, ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo,
        const char8_t* playerSessionId, const UserIdentification& caller)
    {
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (playerSessionId == nullptr) || (playerSessionId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".getPlayerSession: missing authtoken(" << authInfo.getCachedExternalSessionToken() << ") or PlayerSessionId(" << ((playerSessionId != nullptr) ? playerSessionId : "<null>") << "). Caller(" << toExtUserLogStr(caller) << ").");
            return ERR_SYSTEM;
        }

        PSNServices::PlayerSessions::GetPlayerSessionResponse rsp;
        PSNServices::PlayerSessions::GetPlayerSessionRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(playerSessionId);

        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_GETPLAYERSESSION;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, errorInfo);
        if (err == PSNSESSIONMANAGER_ACCESS_FORBIDDEN)
        {
            WARN_LOG(logPrefix() << ".getPlayerSession: cannot get PlayerSession(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ". Caller(" << toExtUserLogStr(caller) << ") may have just signed out?");
            return err;
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getPlayerSession: failed to get PlayerSession(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err));
            return err;
        }

        // store to result
        if (!rsp.getPlayerSessions().empty())
        {
            ASSERT_COND_LOG((rsp.getPlayerSessions().size() < 2), logPrefix() << ".getPlayerSession: user unexpectedly has " << rsp.getPlayerSessions().size() << " PlayerSessions. Having more than one may cause errors, as multiple PlayerSession membership is not currently supported. Assuming primary is the first. Caller(" << toExtUserLogStr(caller) << ").");
            rsp.getPlayerSessions().front()->copyInto(result);
        }
        TRACE_LOG(logPrefix() << ".getPlayerSession: retrieved PlayerSession(" << req.getSessionId() << "). Caller(" << toExtUserLogStr(caller) << ").");
        return err;
    }




    /*! ************************************************************************************************/
    /*! \brief helper to populate PSN request's member, from UserIdentification
    ***************************************************************************************************/
    void ExternalSessionUtilPs5PlayerSessions::toPsnReqPlayer(PSNServices::PlayerSessions::PlayerSessionMember& psnPlayer, const UserIdentification& blazeUser, const char8_t* pushContextId)
    {
        eastl::string accountBuf;
        psnPlayer.setAccountId(ExternalSessionUtilPs5::getPsnAccountId(accountBuf, blazeUser).c_str());
        psnPlayer.setPlatform(toSupportedPlatform(blazeUser.getPlatformInfo().getClientPlatform()));
        if (pushContextId)
            psnPlayer.getPushContexts().pull_back()->setPushContextId(pushContextId);
    }

    /*! ************************************************************************************************/
    /*! \brief Populate the Blaze-specified section of the PSN PlayerSession's customdata
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::toPlayerSessionCustomData1(eastl::string& psnCustomDataJson, const ExternalSessionUpdateInfo& gameValues)
    {
        ExternalSessionBlazeSpecifiedCustomDataPs5 psnCustomDataTdf;
        if (gameValues.getGameId() == INVALID_GAME_ID)
        {
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].toPlayerSessionCustomData1: poss internal error: attempted to set invalid GameId to customData1.");
            return false;
        }
        psnCustomDataTdf.setGameId(gameValues.getGameId());
        psnCustomDataTdf.setMode(gameValues.getGameMode());
        return blazeDataToPsnPlayerSessionCustomData(psnCustomDataJson, psnCustomDataTdf);
    }
    /*! ************************************************************************************************/
    /*! \brief Populate the title-specified section of the PSN PlayerSession's customdata
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::toPlayerSessionCustomData2(eastl::string& psnCustomDataJson, const ExternalSessionCustomData& gameValues)
    {
        char8_t tmp[MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN];
        auto result = gameValues.encodeBase64(tmp, MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN);
        if (result < 0)
            return false;
        psnCustomDataJson.reserve(result);
        psnCustomDataJson = tmp;
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Get the max players for the request to create/update the PSN PlayerSession, from Game's values
    ***************************************************************************************************/
    uint16_t ExternalSessionUtilPs5PlayerSessions::toMaxPlayers(const ExternalSessionUpdateInfo& gameValues)
    {
        const GameManager::SlotCapacitiesVector& slotCapacities = gameValues.getSlotCapacities();
        return getTotalParticipantCapacity(slotCapacities);
    }
    /*! ************************************************************************************************/
    /*! \brief Get the max spectators for the request to create/update the PSN PlayerSession, from Game's values
    ***************************************************************************************************/
    uint16_t ExternalSessionUtilPs5PlayerSessions::toMaxSpectators(const ExternalSessionUpdateInfo& gameValues)
    {
        const GameManager::SlotCapacitiesVector& slotCapacities = gameValues.getSlotCapacities();
        uint16_t cap = 0;
        for (size_t i = 0; i < slotCapacities.size(); ++i)
        {
            if (isSpectatorSlot((SlotType)i))
                cap += slotCapacities[i];
        }
        return cap;
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze GameName to the PSN PlayerSession's sessionName
    ***************************************************************************************************/
    void ExternalSessionUtilPs5PlayerSessions::toMultilingualSessionName(PSNServices::MultilingualText& multilingualText, const ExternalSessionUpdateInfo& gameValues)
    {
        multilingualText.setDefaultLanguage("en-US");
        multilingualText.getLocalizedText()["en-US"] = ExternalSessionUtil::getValidJsonUtf8(gameValues.getGameName(), gameValues.getGameNameMaxStringLength());
    }

    bool ExternalSessionUtilPs5PlayerSessions::hasSameMultilingualSessionName(const ExternalSessionUpdateInfo& a, const ExternalSessionUpdateInfo& b)
    {
        // comparing multilingualTextA.equalsValue(multilingualTextB) wont work correctly always due to isSet, the below is a bit more efficient also
        return (blaze_strcmp(a.getGameName(), b.getGameName()) == 0);
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game's values to PSN PlayerSession invitableUserType.
        Note: This method may only be used to init the session's starting state, but Blaze won't drive *updates* to it.
        The invitableUserType updates are by Sony spec/requirements driven/controlled via PS5's system UX alone.
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs5PlayerSessions::toInvitableUserType(const ExternalSessionUpdateInfo& gameValues)
    {
        //LEADER not used by Blaze, wouldn't line up with Blaze admins anyhow, as Blaze allows multiple or 0

        const GameManager::GameSettings& gameSettings = gameValues.getSettings();
        return (gameSettings.getOpenToInvites() ? "MEMBER" : "NO_ONE");
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game's values to PSN PlayerSession joinableUserType
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs5PlayerSessions::toJoinableUserType(const ExternalSessionUpdateInfo& gameValues) const
    {
        auto joinableUserType = INVALID_JOINABLE_USER_TYPE;
        auto presenceMode = findPresenceModeForGamePlayerSessions(gameValues.getPresenceModeByPlatform());
        switch (presenceMode)
        {
        case PRESENCE_MODE_STANDARD:
            // Close to non-invite joins as needed.  If you want to also close to invite joins, see toJoinDisabledFlag()
            joinableUserType = PSNServices::PlayerSessions::ANYONE;
            
            // Blaze does not yet support toggling of joinableUserType post creation. Otherwise, the following line could be used instead:
            //joinableUserType = (isClosedToAllNonInviteJoins(gameValues.getSettings()) ? PSNServices::PlayerSessions::NO_ONE : PSNServices::PlayerSessions::ANYONE);

            // FRIENDS, FRIENDS_OF_FRIENDS unsupported by Blaze currently.
            //if (isClosedToAllNonInviteJoins(gameValues.getSettings()) && !gameSettings.getFriendsBypassClosedToJoinByPlayer())
            //    joinableUserType = PSNServices::PlayerSessions::FRIENDS;
            break;
        case PRESENCE_MODE_PRIVATE:
            // close to non-invite joins, except allow Blaze to optionally bypass (for MM etc) via joinableSpecifiedUsers
            joinableUserType = PSNServices::PlayerSessions::SPECIFIED_USERS;
            break;
        default:
            joinableUserType = PSNServices::PlayerSessions::ANYONE;
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].toJoinableUserType: Unhandled presence mode(" << PresenceModeToString(presenceMode) << "). Returning default(" << PsnJoinableUserTypeEnumToString(joinableUserType) << ").");
            break;
        };
        TRACE_LOG("[ExternalSessionUtilPs5PlayerSessions].toJoinableUserType: (" << PsnJoinableUserTypeEnumToString(joinableUserType) << "), for Blaze presenceMode(" << PresenceModeToString(presenceMode) << ").");
        return PsnJoinableUserTypeEnumToString(joinableUserType);
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game's values to PSN PlayerSession joinDisabled. True disables UX join buttons
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::toJoinDisabledFlag(const ExternalSessionUpdateInfo& gameValues)
    {
        // True if in-progress but join-in-progress disabled, or closed to UI joins. Note NO need to deal w/Game fullness here, as maxPlayers/maxSpectators handles that.

        if ((gameValues.getGameType() == GAME_TYPE_GAMESESSION) &&
            (gameValues.getSimplifiedState() == GAME_PLAY_STATE_IN_GAME) && !gameValues.getSettings().getJoinInProgressSupported())
            return true;

        // The joinDisabled disables all UX join buttons.  If you want to allow invite joins, see toJoinableUserType()
        if (!gameValues.getSettings().getOpenToInvites() && isClosedToAllNonInviteJoins(gameValues.getSettings()))
            return true;

        return false;
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game platform to PSN PlayerSession supported platform string.
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs5PlayerSessions::toSupportedPlatform(ClientPlatformType platform)
    {
        switch (platform)
        {
        case ps4:
            return "PS4";
        case ps5:
            return "PS5";
        default:
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].toSupportedPlatform: Unhandled/unsupported platform(" << ClientPlatformTypeToString(platform) << "). Returning default PS5");
            return "PS5";
        }
    }
    /*! ************************************************************************************************/
    /*! \brief setup req supportedPlatforms.
    ***************************************************************************************************/
    void ExternalSessionUtilPs5PlayerSessions::toSupportedPlatforms(PsnPlatformNameList& supportedPlatforms, const GetExternalSessionInfoMasterResponse& gameInfo) const
    {
        supportedPlatforms.push_back(toSupportedPlatform(getPlatform()));
        // add additional supported platforms
        if (mConfig.getCrossgen())
        {
            auto alternatePlatform = ((getPlatform() == ps4) ? ps5 : ps4);
            auto itr = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getPresenceModeByPlatform().find(alternatePlatform);
            if ((itr != gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getPresenceModeByPlatform().end()))
            {
                supportedPlatforms.push_back(toSupportedPlatform(alternatePlatform));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief setup req leaderPrivileges.
    ***************************************************************************************************/
    void ExternalSessionUtilPs5PlayerSessions::toLeaderPrivileges(PlayerSessionLeaderPrivilegeList& leaderPrivileges)
    {
        leaderPrivileges.markSet();//(Blaze marks as set to send empty 'omit-all' list by default)
    }



    void ExternalSessionUtilPs5PlayerSessions::validateGameSettings(const GameSettings& settings)
    {
        // by TRC R5112, and UX spec, the PS5 system features governs ability to send invites to PlayerSessions including from within an application.
        if (!settings.getOpenToInvites() && settings.getOpenToJoinByPlayer())
        {
            TRACE_LOG("[ExternalSessionUtilPs5PlayerSessions].validateGameSettings: Warning: open session settings are 'closed to invites'. Blaze may prevent joins by invite at the title level but won't block ability to send invites by PS5 spec.");
        }
    }


    /*! ************************************************************************************************/
    /*! \brief if mock platform is enabled, update client push context id as needed
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs5PlayerSessions::updatePushContextIdIfMockPlatformEnabled(const char8_t* origPushContextId) const
    {
        if (((origPushContextId == nullptr) || (origPushContextId[0] == '\0')) && isMockPlatformEnabled())
        {
            return "mockClientPushContextId";
        }
        return origPushContextId;
    }


    /*! ************************************************************************************************/
    /*! \brief find the PresenceMode used for the Game's PlayerSessions, from the map.
    ***************************************************************************************************/
    PresenceMode ExternalSessionUtilPs5PlayerSessions::findPresenceModeForGamePlayerSessions(const ExternalSessionUpdateInfo::PresenceModeByPlatformMap& gamePresenceModeByPlatform) const
    {
        PresenceMode presenceMode = PRESENCE_MODE_NONE;
        auto itr = gamePresenceModeByPlatform.find(getPlatform());
        if (itr != gamePresenceModeByPlatform.end())
        {
            presenceMode = itr->second;
        }
        // for crossgen, use most restrictive supported platform
        if (mConfig.getCrossgen())
        {
            itr = gamePresenceModeByPlatform.find(getPlatform() == ps5 ? ps4 : ps5);
            if ((itr != gamePresenceModeByPlatform.end()) && (itr->second != PRESENCE_MODE_NONE) &&
                ((presenceMode == PRESENCE_MODE_NONE) || ((presenceMode == PRESENCE_MODE_STANDARD) && (itr->second == PRESENCE_MODE_PRIVATE)))) //if have both plats, pick more restrictive
            {
                presenceMode = itr->second;
            }
        }
        return presenceMode;
    }





    ////////////////////////////// update() helpers //////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief Checks whether game change should update its PSN PlayerSession
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) const
    {
        // w/crossgen the updates can be done via ps5 or ps4's util. To avoid redundant calls, just use ps5's (see getUpdater() which will also try ps4 callers).
        if ((getPlatform() == ps4) && EA_LIKELY(mConfig.getCrossgen() && ExternalSessionUtilPs5::isCrossgenConfiguredForPlatform(ps5, nullptr)))
        {
            TRACE_LOG(logPrefix() << ".isUpdateRequired: Skipping updates with PS4 instance of this module, updates are handled by PS5's.");
            return false;
        }

        // if game doesn't have an PSN PlayerSession, no updates needed. Any PSN PlayerSession will be initialized up to date at its creation.
        if (newValues.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId()[0] == '\0')
            return false;

        // final updates unneeded since PlayerSessions go away
        if (isFinalExtSessUpdate(context))
            return false;

        if (isUpdateInvitableUserTypeRequired(origValues, newValues))
            return true;

        if (isUpdateJoinDisabledRequired(origValues, newValues))
            return true;

        if (isUpdateMaxPlayersRequired(origValues, newValues))
            return true;

        if (isUpdateMaxSpectatorsRequired(origValues, newValues))
            return true;

        if (isUpdateSessionNameRequired(origValues, newValues))
            return true;

        if (isUpdateCustomData1Required(origValues, newValues))
            return true;

        return false;
    }
    /*! ************************************************************************************************/
    /*! \brief Checks whether game change should update its PSN PlayerSession joinDisabled
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateJoinDisabledRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return (toJoinDisabledFlag(origValues) != toJoinDisabledFlag(newValues));
    }
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateInvitableUserTypeRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return (blaze_strcmp(ExternalSessionUtilPs5PlayerSessions::toInvitableUserType(origValues), ExternalSessionUtilPs5PlayerSessions::toInvitableUserType(newValues)) != 0);
    }
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateMaxPlayersRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return (toMaxPlayers(origValues) != toMaxPlayers(newValues));
    }
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateMaxSpectatorsRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return (toMaxSpectators(origValues) != toMaxSpectators(newValues));
    }
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateSessionNameRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        return !hasSameMultilingualSessionName(origValues, newValues);
    }
    bool ExternalSessionUtilPs5PlayerSessions::isUpdateCustomData1Required(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
    {
        eastl::string origCustData1, newCustData1;
        if (!toPlayerSessionCustomData1(origCustData1, origValues) || !toPlayerSessionCustomData1(newCustData1, newValues))
        {
            ERR_LOG(logPrefix() << ".isUpdateCustomData1Required: Failed to encode customdata1s for comparison, check log.");
            return false;
        }
        return (blaze_strcmp(origCustData1.c_str(), newCustData1.c_str()) != 0);
    }

    /*! ************************************************************************************************/
    /*! \brief updates the PSN PlayerSession's basic properties as needed
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::update(const UpdateExternalSessionPropertiesParameters& params)
    {
        BlazeRpcError retErr = ERR_OK;
        auto& caller = params.getCallerInfo().getUserIdentification();
        auto authInfo = params.getCallerInfo().getAuthInfo();

        // final updates unneeded since PlayerSessions go away
        if (isFinalExtSessUpdate(params.getContext()))
        {
            return ERR_OK;
        }

        // Note order may be relevant below. For instance, we ensure the GameId in customdata1 is prioritized for edge flows where it needs to be imprinted onto the session,
        // to at least allow the Blaze Game to check joinability etc a the title level, while we fill in rest of the properties for the UX.
        if (isUpdateCustomData1Required(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateCustomDataBlazePart(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        if (isUpdateInvitableUserTypeRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateInvitableUserType(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        if (isUpdateJoinDisabledRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateJoinDisabled(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        if (isUpdateMaxPlayersRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateMaxPlayers(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        if (isUpdateMaxSpectatorsRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateMaxSpectators(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        if (isUpdateSessionNameRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            auto err = updateSessionName(authInfo, caller, params.getExternalSessionUpdateInfo());
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        return retErr;
    }

    /*! ************************************************************************************************/
    /*! \brief updates the PSN PlayerSession's customData1
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateCustomDataBlazePart(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        eastl::string custData1Buf;
        if (!toPlayerSessionCustomData1(custData1Buf, params))
        {
            ERR_LOG(logPrefix() << ".updateCustomDataBlazePart: fail updating customData1 for PlayerSession(" << sessionId << "), could not convert Blaze struct to custom data. See log");
            return ERR_SYSTEM;
        }
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setCustomData1(custData1Buf.c_str());
        TRACE_LOG(logPrefix() << ".updateCustomDataBlazePart: -> customData1(" << reqBody.getCustomData1() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateCustomDataTitlePart(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionCreationInfo& params)
    {
        auto* sessionId = params.getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        eastl::string custData2Buf;
        if (!toPlayerSessionCustomData2(custData2Buf, params.getExternalSessionCustomData()))
        {
            ERR_LOG(logPrefix() << ".updateCustomDataTitlePart: fail updating customData2 for PlayerSession(" << sessionId << "), could not convert Blaze struct to custom data. See log");
            return ERR_SYSTEM;
        }
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setCustomData2(custData2Buf.c_str());
        TRACE_LOG(logPrefix() << ".updateCustomDataTitlePart: -> customData2(" << reqBody.getCustomData2() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateJoinDisabled(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setJoinDisabled(toJoinDisabledFlag(params));
        TRACE_LOG(logPrefix() << ".updateJoinDisabled: -> joinDisabled(" << (reqBody.getJoinDisabled() ? "true" : "false") << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateJoinableUserType(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setJoinableUserType(toJoinableUserType(params));
        TRACE_LOG(logPrefix() << ".updateJoinableUserType: -> joinableUserType(" << reqBody.getJoinableUserType() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateInvitableUserType(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setInvitableUserType(toInvitableUserType(params));
        TRACE_LOG(logPrefix() << ".updateInvitableUserType: -> invitableUserType(" << reqBody.getInvitableUserType() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateMaxPlayers(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setMaxPlayers(toMaxPlayers(params));
        TRACE_LOG(logPrefix() << ".updateMaxPlayers: -> maxPlayers(" << reqBody.getMaxPlayers() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateMaxSpectators(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        reqBody.setMaxSpectators(toMaxSpectators(params));
        TRACE_LOG(logPrefix() << ".updateMaxSpectators: -> maxSpectators(" << reqBody.getMaxSpectators() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }
    
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateSessionName(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params)
    {
        auto* sessionId = params.getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        toMultilingualSessionName(reqBody.getLocalizedSessionName(), params);
        TRACE_LOG(logPrefix() << ".updateSessionName: -> localizedSessionName(" << reqBody.getLocalizedSessionName() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updateLeaderPrivileges(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionCreationInfo& params)
    {
        auto* sessionId = params.getUpdateInfo().getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
        UpdatePlayerSessionPropertiesRequestBody reqBody;
        toLeaderPrivileges(reqBody.getLeaderPrivileges());
        TRACE_LOG(logPrefix() << ".updateLeaderPrivileges: -> leaderPrivileges(" << reqBody.getLeaderPrivileges() << ") for PlayerSession(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << ").");
        return updatePlayerSessionProperty(authInfo, caller, sessionId, reqBody);
    }

    /*! ************************************************************************************************/
    /*! \brief By Sony spec each PSN PlayerSession property must be individually updated in a separate update RPC call.
            This helper calls to PSN to update a single property, specified by caller in reqBody.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::updatePlayerSessionProperty(ExternalUserAuthInfo& authInfo, const UserIdentification& caller,
        const char8_t* sessionId, const UpdatePlayerSessionPropertiesRequestBody& reqBody)
    {
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (sessionId[0] == '\0'))
        {
            WARN_LOG(logPrefix() << ".updatePlayerSessionProperty: invalid authToken(" << authInfo.getCachedExternalSessionToken() << "), psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or PlayerSessionId(" << sessionId << "). Caller(" << toExtUserLogStr(caller) << "). ... Failed on attempted update: " << reqBody);
            return ERR_SYSTEM;
        }
        EA::TDF::MemberVisitOptions opts;
        opts.onlyIfSet = true;

        UpdatePlayerSessionPropertiesRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setSessionId(sessionId);
        reqBody.copyInto(req.getBody(), opts);

        const CommandInfo& cmd = PSNSessionManagerSlave::CMD_INFO_UPDATEPLAYERSESSIONPROPERTIES;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr);
        logUpdateErr(err, req.getSessionId(), caller);
        return err;
    }

    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParam)
    {
        auto possPlayerSessUpdaters = getListFromMap(possibleUpdaters, getActivityTypeInfo());
        if (!possPlayerSessUpdaters)
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;

        const bool isNeedsLeader = (updatePropertiesParam && isUpdateInvitableUserTypeRequired(updatePropertiesParam->getExternalSessionOrigInfo(), updatePropertiesParam->getExternalSessionUpdateInfo()));

        BlazeRpcError err = ExternalSessionUtil::getUpdaterWithUserToken(getPlatform(), *possPlayerSessUpdaters, updater, avoid, !isNeedsLeader); //(!isNeedsLeader if just for getPlayerSession below)
        if ((err != ERR_OK) && mConfig.getCrossgen())
        {
            err = ExternalSessionUtil::getUpdaterWithUserToken((getPlatform() == ps5 ? ps4 : ps5), *possPlayerSessUpdaters, updater, avoid, !isNeedsLeader); //w/crossgen, can also try PS4 users (works same)
        }
        if (err != ERR_OK)
        {
            WARN_LOG("[ExternalSessionUtilPs5PlayerSessions].getUpdater: found NO user who can make the next update call, out of (" << possibleUpdaters.size() << ") potential callers.");
            return err;
        }

        // If leader unneeded, above found user is good. Else use it to getPlayerSession to find leader:
        if (isNeedsLeader)
        {
            auto sessionId = updatePropertiesParam->getSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
            GetPlayerSessionResponseItem getRsp;
            err = getPlayerSession(getRsp, nullptr, updater.getAuthInfo(), sessionId, updater.getUserIdentification());
            if (err != ERR_OK)
            {
                return err;
            }
            ExternalPsnAccountId leaderId = EA::StdC::AtoU64(getRsp.getLeader().getAccountId());
            for (auto itr : *possPlayerSessUpdaters)
            {
                if (leaderId == itr->getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId())
                {
                    itr->copyInto(updater);
                    //updating avoid unneeded, rate limit practically un-hittable
                    return ERR_OK;
                }
            }
            WARN_LOG(logPrefix() << ".getUpdater: PlayerSession(" << sessionId << ") leader acctid(" << getRsp.getLeader().getAccountId() << ") invalid for update needing leader.");
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief log update error
    ***************************************************************************************************/
    void ExternalSessionUtilPs5PlayerSessions::logUpdateErr(BlazeRpcError err, const char8_t* playerSessionId, const UserIdentification& caller) const
    {
        if ((err == PSNSESSIONMANAGER_RESOURCE_NOT_FOUND) || (err == PSNSESSIONMANAGER_ACCESS_FORBIDDEN))
        {
            TRACE_LOG(logPrefix() << ".logUpdateErr: not updating externalSession(" << playerSessionId << "), error(" << ErrorHelp::getErrorName(err) << "). Caller(" << toExtUserLogStr(caller) << ") may no longer be in the session.");
        }
        else if (shouldRetryAfterExternalSessionUpdateError(err))
        {
            WARN_LOG(logPrefix() << ".logUpdateErr: failed updating externalSession(" << playerSessionId << "), error(" << ErrorHelp::getErrorName(err) << "), with Caller(" << toExtUserLogStr(caller) << "). Blaze may retry using another caller.");
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".logUpdateErr: failed updating externalSession(" << playerSessionId << "), error(" << ErrorHelp::getErrorName(err) << "). Caller(" << toExtUserLogStr(caller) << ")");
        }
    }



    /*! ************************************************************************************************/
    /*! \brief extend Blaze's create or join feature to support the PS5 shell 'boot to new game' flow.
       To create a unique PersistedGameId, use a Blaze provided prefix, with the PSN PlayerSessionId as suffix.
       \param[in,out] request Request to validate/update. Pre: template attributes resolved and populated.
   ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtilPs5PlayerSessions::prepForCreateOrJoin(CreateGameRequest& request) const
    {
        // spec: If PS5 client sent non empty playerSessionId, handle as the PS5 boot to create or join. Else return.
        const char8_t* playerSessionId = request.getGameCreationData().getExternalSessionIdentSetup().getPs5().getPlayerSession().getPlayerSessionId();
        if ((playerSessionId[0] == '\0') || request.getPlayerJoinData().getPlayerDataList().empty() || ((*request.getPlayerJoinData().getPlayerDataList().begin())->getUser().getPlatformInfo().getClientPlatform() != getPlatform()))//Pre: PJD populated
        {
            return ERR_OK;
        }
        // for security
        if (!GameManagerMasterImpl::isUUIDFormat(playerSessionId))
        {
            ERR_LOG(logPrefix() << ".prepForCreateOrJoin: invalid non UUID format PlayerSessionId(" << playerSessionId << ") specified. NOOP");
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR;
        }
        if (request.getPersistedGameId()[0] != '\0')
        {
            WARN_LOG(logPrefix() << ".prepForCreateOrJoin: Client specified non-empty persistedGameId(" << request.getPersistedGameId() << "), with external session setup playerSessionId(" << playerSessionId <<
                "). Blaze will use an internally generated PersistedGameId, client value will be ignored.");
        }
        // Length was validated in verifyParams():
        eastl::string newPersistedId(eastl::string::CtorSprintf(), "%s%s", mConfig.getPsnPlayerSessionPersistedIdPrefix(), playerSessionId);
        request.setPersistedGameId(newPersistedId.c_str());
        request.getGameCreationData().getGameSettings().setEnablePersistedGameId(true);
        TRACE_LOG(logPrefix() << ".prepForCreateOrJoin: set to use generated PersistedGameId(" << newPersistedId << ") to CreateGameRequest");
        return ERR_OK;
    }



    /*! ************************************************************************************************/
    /*! \brief Return whether error code indicates call should be retried
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5PlayerSessions::isRetryError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const
    {
        return ((psnErr == ERR_TIMEOUT) ||
            (psnErr == PSNSESSIONMANAGER_SERVICE_UNAVAILABLE) ||
            (psnErr == PSNSESSIONMANAGER_SERVICE_INTERNAL_ERROR));
    }

    bool ExternalSessionUtilPs5PlayerSessions::isTokenExpiredError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const
    {
        bool result(psnErr == PSNSESSIONMANAGER_AUTHENTICATION_REQUIRED);
        if (result)
        {
            WARN_LOG(logPrefix() << ".isTokenExpiredError: authentication with Sony failed, token may have expired. PSN error(" <<
                ErrorHelp::getErrorName(psnErr) << "), " << ErrorHelp::getErrorDescription(psnErr) << "). (Code: " <<
                psnRsp.getError().getCode() << " Message: " << psnRsp.getError().getMessage() << " ) Blaze may forcing token refresh and retry the request.");
        }
        return result;
    }




    ////////////////////////////// PSN Connection //////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief Call to PSN
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5PlayerSessions::callPSN(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo,
        PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo)
    {
        PSNServices::PsnErrorResponse rspErr;

        BlazeRpcError err = ExternalSessionUtilPs5::callPSNInternal(mPsnConnMgrPtr, cmdInfo, req, rsp, &rspErr);
        if (isTokenExpiredError(err, rspErr))
        {
            auto tokErr = getAuthToken(authInfo, caller, authInfo.getServiceName(), true);
            if (OAUTH_ERR_INVALID_REQUEST == tokErr)
                return tokErr;//retries unneeded
            if (ERR_OK == tokErr)
            {
                reqHeader.setAuthToken(authInfo.getCachedExternalSessionToken());
                err = ExternalSessionUtilPs5::callPSNInternal(mPsnConnMgrPtr, cmdInfo, req, rsp, &rspErr);
            }
        }
        for (size_t i = 0; (isRetryError(err, rspErr) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
        {
            if (ERR_OK != ExternalSessionUtilPs5::waitTime(mConfig.getCallOptions().getServiceUnavailableBackoff(), cmdInfo.context, i + 1))
                return ERR_SYSTEM;
            err = ExternalSessionUtilPs5::callPSNInternal(mPsnConnMgrPtr, cmdInfo, req, rsp, &rspErr);
        }

        if (err != ERR_OK)
        {
            // NOT_FOUND (HTTP 404) typically session N/A, not necessarily an error. Caller should handle as appropriate
            if (err != PSNSESSIONMANAGER_RESOURCE_NOT_FOUND)
            {
                ERR_LOG(logPrefix() << ".callPSN: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " failed, " << ExternalSessionUtilPs5::toLogStr(err, &rspErr) << ". Caller(" << toExtUserLogStr(caller) << ")");
            }
            ExternalSessionUtilPs5::toExternalSessionErrorInfo(errorInfo, rspErr);
        }
        TRACE_LOG(logPrefix() << ".callPSN: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "call") << " result(" << ErrorHelp::getErrorName(err) << "). Caller(" << toExtUserLogStr(caller) << ").");
        return err;
    }

}//GameManager
}//Blaze
