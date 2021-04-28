/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5matches.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_EXTERNAL_SESSION_UTIL_PS5_MATCHES_H
#define COMPONENT_EXTERNAL_SESSION_UTIL_PS5_MATCHES_H

#include "framework/connection/outboundhttpconnectionmanager.h"//for HttpConnectionManagerPtr mPsnConnMgrPtr
#include "gamemanager/externalsessions/externalsessionmemberstrackinghelper.h"//for mTrackhelper
#include "gamemanager/tdf/externalsessiontypes_server.h"
#include "gamemanager/tdf/externalsessionconfig_server.h" //for ExternalSessionServerConfig mConfig
#include "psnmatches/tdf/psnmatches.h"//for PsnMatchStatusEnum, GetMatchDetailResponse etc

namespace Blaze
{
    class LeaveGroupExternalSessionParameters;
    namespace GameManager { class GameExternalSessionMembersTrackUtilPs5; class GameManagerSlaveImpl; class GameSessionMaster; }
    namespace Arson { class ArsonSlaveImpl; }
    
    namespace GameManager
    {
        class ExternalSessionUtilPs5Matches
        {
        public:
            ExternalSessionUtilPs5Matches(const ExternalSessionServerConfig& config, GameManagerSlaveImpl* gameManagerSlave);
            ~ExternalSessionUtilPs5Matches();

            static bool verifyParams(const ExternalSessionServerConfig& config, const TeamNameByTeamIdMap* teamNameByTeamIdMap = nullptr);

            BlazeRpcError updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult);

            bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context);
            BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params);

            void prepForReplay(GameSessionMaster& gameSession);
            
            // reconfigure methods
            bool onPrepareForReconfigure(const ExternalSessionServerConfig& config) { return verifyParams(config); }

            static BlazeRpcError callPSN(const UserIdentification* user, const CommandInfo& cmdInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo, HttpConnectionManagerPtr psnConnMgrPtr, const ExternalServiceCallOptions &config);

            static const eastl::string& toPsnReqTeamId(eastl::string& psnTeamId, Blaze::GameManager::TeamIndex blazeTeamIndex) { return psnTeamId.sprintf("%u", blazeTeamIndex); }
            static const eastl::string& toPsnReqPlayerId(eastl::string& psnPlayerId, const UserIdentification& blazeUser) { return psnPlayerId.sprintf("%" PRIu64 "", blazeUser.getPlatformInfo().getExternalIds().getPsnAccountId()); }

        private:
            static bool isMatchActivitiesDisabled(const ExternalSessionServerConfig& config) { return (config.getPsnMatchesServiceLabel() == INVALID_EXTERNAL_SESSION_SERVICE_LABEL); }
            bool isMatchActivitiesDisabled() { return isMatchActivitiesDisabled(mConfig); }

            BlazeRpcError createMatch(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo, const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError joinMatch(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo, const UpdatePrimaryExternalSessionForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError leaveMatch(ExternalSessionErrorInfo& errorInfo, const UpdatePrimaryExternalSessionForUserParameters& callerParams);
            BlazeRpcError leaveMatchInternal(ExternalSessionErrorInfo& errorInfo, const UserIdentification& user, const char8_t* matchId, GameManager::GameId gameId, GameManager::PlayerRemovedReason removeReason, SimplifiedGamePlayState gamePlayState, PSNServices::Matches::PsnLeaveReasonEnum leaveReasonType = PSNServices::Matches::INVALID_LEAVE_REASON_TYPE);
            BlazeRpcError syncMatchAndTrackedMembership(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo& errorInfo, const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo, bool forCreate);

            BlazeRpcError getMatch(PSNServices::Matches::GetMatchDetailResponse& rsp, ExternalSessionErrorInfo& errorInfo, const char8_t* matchId, const UserIdentification* caller);

            typedef ExternalSessionUpdateEventContext::MembershipChangeList PlayerJoinFlagUpdateList;
            bool isUpdatePlayerJoinFlagRequired(PlayerJoinFlagUpdateList* requiredUpdates, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context);
            BlazeRpcError updatePlayerJoinFlag(const PlayerJoinFlagUpdateList& requiredUpdates, const ExternalSessionUpdateInfo& newValues);

            typedef eastl::vector<PSNServices::Matches::PsnMatchStatusEnum> PsnMatchStatusTypeList;
            bool isUpdateMatchStatusRequired(PsnMatchStatusTypeList* requiredUpdates, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context);
            BlazeRpcError updateMatchStatus(const PsnMatchStatusTypeList& requiredUpdates, const char8_t* matchId);

            bool isUpdateMatchTeamNameRequired(const char8_t* logTrace, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues);
            bool isUpdateMatchDetailInGameRosterRequired(bool logTrace, const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues);
            BlazeRpcError updateMatchDetailInGameRoster(const ExternalSessionUpdateInfo& newUpdates);
            bool shouldResyncUpdateMatchDetailInGameRoster(ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateInfo& origUpdates);

            void populateExtMemberInfo(ExternalMemberInfo& member, const UpdateExternalSessionPresenceForUserParameters& callerParams);
            bool isJustAdded1stPsnUser(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool hasPsPlayersInList(const UserIdentificationList& list) const;

            void toPsnReqPlayer(PSNServices::Matches::MatchPlayer& psnPlayer, const UserIdentification& blazeUser, Blaze::GameManager::TeamIndex blazeJoinTeamIndex = Blaze::GameManager::UNSPECIFIED_TEAM_INDEX) const;
            void toPsnReqTeams(PSNServices::Matches::MatchInGameRoster& createReqRoster, const UpdateExternalSessionPresenceForUserParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo) const;
            void toPsnReqTeam(PSNServices::Matches::MatchTeam& psnTeam, Blaze::GameManager::TeamIndex blazeTeamIndex, const UserIdentification* blazeUserToAdd, const ExternalSessionUpdateInfo& gameInfo) const;

            const char8_t* toPsnReqTeamName(GameManager::TeamIndex blazeTeamIndex, const ExternalSessionUpdateInfo& gameInfo) const;
            
            PSNServices::Matches::PsnLeaveReasonEnum toMatchLeaveReason(const UserIdentification& user, const char8_t* matchId, GameManager::GameId gameId, GameManager::PlayerRemovedReason removeReason, const PSNServices::Matches::GetMatchDetailResponse& matchDetail) const;

            static bool hasTeams(const TeamIdVector& gameInfo) { return (gameInfo.size() > 1); }//Pre: single team games are considered non team activities. No need to use team ids/names/reporting for those.
            static bool hasTeams(const GetExternalSessionInfoMasterResponse& gameInfo) { return hasTeams(gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTeamIdsByIndex()); }
            static bool isMatchAlreadyOver(const char8_t* currMatchStatus, const char8_t* unneededOpLogContext);
            static bool isMatchAlreadyOverErr(BlazeRpcError psnErr, const ExternalSessionErrorInfo& errorInfo, const CommandInfo& cmdInfo);
            static bool isMatchActivitiesDisabledForGame(const ExternalSessionIdentification& sessionIdent) { return (sessionIdent.getPs5().getMatch().getActivityObjectId()[0] == '\0'); }//by design for flexibility, PRESENCE_MODE_NONE has NO bearing
            static bool shouldPlayerBeInMatches(const UserIdentification& userIdent, PlayerState playerState);
            static bool isRetryError(BlazeRpcError psnErr);
            static bool isTokenExpiredError(BlazeRpcError psnErr, const PSNServices::PsnErrorResponse& psnRsp);

            PsnServiceLabel getServiceLabel() const { return mConfig.getPsnMatchesServiceLabel(); }
            ClientPlatformType getPlatform() const { return ps5; }
            const ExternalSessionActivityTypeInfo& getActivityTypeInfo() const { return mTrackHelper.getExternalSessionActivityTypeInfo(); }
            const FixedString256 toLogStr(const ExternalSessionIdentification& extSessIdent) const;
            const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalSessionUtilPs5Matches]").c_str() : mLogPrefix.c_str()); }

        private:
            BlazeRpcError callPSN(const UserIdentification* user, const CommandInfo& cmdInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo) { return callPSN(user, cmdInfo, reqHeader, req, rsp, errorInfo, mPsnConnMgrPtr, mConfig.getCallOptions()); }

            HttpConnectionManagerPtr mPsnConnMgrPtr;

            ExternalSessionServerConfig mConfig;
            ExternalSessionMembersTrackingHelper mTrackHelper;

            mutable eastl::string mLogPrefix;
#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            friend class Blaze::Arson::ArsonSlaveImpl;
#endif
        };
    }
}

#endif
