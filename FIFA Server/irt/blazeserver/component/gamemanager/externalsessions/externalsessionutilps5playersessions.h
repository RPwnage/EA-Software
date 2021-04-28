/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5playersessions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_EXTERNAL_SESSION_UTIL_PS5_PLAYERSESSIONS_H
#define COMPONENT_EXTERNAL_SESSION_UTIL_PS5_PLAYERSESSIONS_H

#include "framework/controller/controller.h"//for gController in isMockPlatformEnabled
#include "gamemanager/externalsessions/externalaccesstokenutilps5.h"//for ExternalAccessTokenUtilPs5 mTokenUtil
#include "gamemanager/externalsessions/externalsessionmemberstrackinghelper.h"//for mTrackhelper
#include "gamemanager/tdf/externalsessiontypes_server.h" //for SetPrimaryExternalSessionResult, SetPrimaryExternalSessionParameters etc
#include "gamemanager/tdf/externalsessionconfig_server.h" //for ExternalSessionServerConfig mConfig

namespace Blaze
{
    namespace PSNServices { class PsnWebApiHeader; class PsnErrorResponse; class MultilingualText; }
    namespace PSNServices { namespace PlayerSessions { class PlayerSessionMember; class GetPlayerSessionResponseItem; class GetPlayerSessionIdResponseItem; typedef EA::TDF::TdfString PsnPlayerSessionId; class UpdatePlayerSessionPropertiesRequestBody; typedef EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString > PlayerSessionLeaderPrivilegeList; typedef EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString > PsnPlatformNameList; } }
    namespace GameManager { class GameManagerSlaveImpl; class GetExternalSessionInfoMasterResponse; }
    namespace Arson { class ArsonSlaveImpl; }
    
    namespace GameManager
    {
        class ExternalSessionUtilPs5PlayerSessions
        {
        public:
            ExternalSessionUtilPs5PlayerSessions(ClientPlatformType platform, const ExternalSessionServerConfig& config, GameManagerSlaveImpl* gameManagerSlave);
            ~ExternalSessionUtilPs5PlayerSessions();

            static bool verifyParams(const ExternalSessionServerConfig& config);
            static bool isPlayerSessionsDisabled(const ExternalSessionServerConfig& config) { return config.getPrimaryExternalSessionTypeForUx().empty(); }

            BlazeRpcError getAuthToken(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const char8_t* serviceName, bool forceRefresh = false);

            BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult);
            BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult);
            bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) const;



            bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) const;
            BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params);
            BlazeRpcError getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParam);


            Blaze::BlazeRpcError prepForCreateOrJoin(CreateGameRequest& request) const;

            const ExternalSessionActivityTypeInfo& getActivityTypeInfo() const { return mTrackHelper.getExternalSessionActivityTypeInfo(); }
            
            // reconfigure methods
            bool onPrepareForReconfigure(const ExternalSessionServerConfig& config) const { return verifyParams(config); }

        private:
            bool isPlayerSessionsDisabled() const { return isPlayerSessionsDisabled(mConfig); }

            BlazeRpcError createPlayerSession(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo, const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError joinPlayerSession(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo, const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError leavePlayerSession(ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo, const UserIdentification& user, const char8_t* playerSessionId, GameId gameId, bool untrackBlazeMembership);
            BlazeRpcError syncPlayerSessionAndTrackedMembership(ExternalSessionIdentification& resultSessionIdent, ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo, const SetPrimaryExternalSessionParameters& callerParams, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError getPrimaryPlayerSessionIdAndData(PSNServices::PlayerSessions::PsnPlayerSessionId& primaryPlayerSessionId, ExternalSessionBlazeSpecifiedCustomDataPs5& primarySessionData, ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo, const UserIdentification& caller);
            BlazeRpcError getPrimaryPlayerSessionId(PSNServices::PlayerSessions::GetPlayerSessionIdResponseItem& sessionResult, ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo, const UserIdentification& caller);
            BlazeRpcError getPlayerSession(PSNServices::PlayerSessions::GetPlayerSessionResponseItem& result, ExternalSessionErrorInfo* errorInfo, const ExternalUserAuthInfo& authInfo, const char8_t* playerSessionId, const UserIdentification& caller);

            BlazeRpcError addJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo, const UserIdentification& userToAdd, const GetExternalSessionInfoMasterResponse& gameInfo);
            BlazeRpcError addJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo, const UserIdentification& userToAdd, const GetExternalSessionInfoMasterResponse& gameInfo, const UserIdentification& caller);
            BlazeRpcError deleteJoinableSpecifiedUsers(ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo, const UserIdentification& userToDel, const GetExternalSessionInfoMasterResponse& gameInfo, const UserIdentification& caller);

            BlazeRpcError getPrimaryPlayerSessionId(PSNServices::PlayerSessions::GetPlayerSessionIdResponseItem& sessionResult, ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo, const UserIdentification& caller);
            BlazeRpcError getPlayerSession(PSNServices::PlayerSessions::GetPlayerSessionResponseItem& result, ExternalSessionErrorInfo* errorInfo, ExternalUserAuthInfo& authInfo, const char8_t* playerSessionId, const UserIdentification& caller);

            BlazeRpcError initAllPlayerSessionValues(const ExternalUserAuthInfo& authInfo, const ExternalSessionCreationInfo& params, const UserIdentification& caller);

            BlazeRpcError updateCustomDataBlazePart(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateCustomDataTitlePart(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionCreationInfo& params);
            BlazeRpcError updateJoinDisabled(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateJoinableUserType(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateInvitableUserType(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateLeaderPrivileges(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionCreationInfo& params);
            BlazeRpcError updateMaxPlayers(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateMaxSpectators(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);
            BlazeRpcError updateSessionName(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const ExternalSessionUpdateInfo& params);

            BlazeRpcError updatePlayerSessionProperty(ExternalUserAuthInfo& authInfo, const UserIdentification& caller, const char8_t* sessionId, const PSNServices::PlayerSessions::UpdatePlayerSessionPropertiesRequestBody& reqBody);
            bool isUpdateJoinDisabledRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool isUpdateMaxPlayersRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool isUpdateMaxSpectatorsRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool isUpdateInvitableUserTypeRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool isUpdateSessionNameRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            bool isUpdateCustomData1Required(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            void logUpdateErr(BlazeRpcError err, const char8_t* playerSessionId, const UserIdentification& caller) const;


            bool shouldBypassPlayerSessionRestrictionsForJoin(const GetExternalSessionInfoMasterResponse& gameInfo) const;

            static void toPsnReqPlayer(PSNServices::PlayerSessions::PlayerSessionMember& psnPlayer, const UserIdentification& blazeUser, const char8_t* pushContextId);
            static bool toPlayerSessionCustomData1(eastl::string& psnCustomDataJson, const ExternalSessionUpdateInfo& gameValues);
            static bool toPlayerSessionCustomData2(eastl::string& psnCustomDataJson, const ExternalSessionCustomData& gameValues);

            static uint16_t toMaxPlayers(const ExternalSessionUpdateInfo& gameValues);
            static uint16_t toMaxSpectators(const ExternalSessionUpdateInfo& gameValues);
            static void toMultilingualSessionName(PSNServices::MultilingualText& multilingualText, const ExternalSessionUpdateInfo& gameValues);
            static bool hasSameMultilingualSessionName(const ExternalSessionUpdateInfo& a, const ExternalSessionUpdateInfo& b);
            static const char8_t* toInvitableUserType(const ExternalSessionUpdateInfo& gameValues);
            const char8_t* toJoinableUserType(const ExternalSessionUpdateInfo& gameValues) const;
            static bool toJoinDisabledFlag(const ExternalSessionUpdateInfo& gameValues);
            static const char8_t* toSupportedPlatform(ClientPlatformType platform);
            void toSupportedPlatforms(PSNServices::PlayerSessions::PsnPlatformNameList& supportedPlatforms, const GetExternalSessionInfoMasterResponse& gameInfo) const;
            static void toLeaderPrivileges(PSNServices::PlayerSessions::PlayerSessionLeaderPrivilegeList& leaderPrivileges);

            static bool isClosedToAllNonInviteJoins(const GameSettings& settings) { return (!settings.getOpenToJoinByPlayer() && !settings.getOpenToBrowsing() && !settings.getOpenToMatchmaking()); }
            static void validateGameSettings(const GameSettings& settings);
            const char8_t* updatePushContextIdIfMockPlatformEnabled(const char8_t* origPushContextId) const;
            PresenceMode findPresenceModeForGamePlayerSessions(const ExternalSessionUpdateInfo::PresenceModeByPlatformMap& gamePresenceModeByPlatform) const;

            PsnServiceLabel getServiceLabel() const { return mConfig.getExternalSessionServiceLabel(); }
            ClientPlatformType getPlatform() const { return mPlatform; } //ps5, unless using crossgen, in which case maybe ps4 or ps5, depending which this util handles
            bool isMockPlatformEnabled() const { return (mConfig.getUseMock() || gController->getUseMockConsoleServices()); }
            const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalSessionUtilPs5PlayerSessions(%s)]", ClientPlatformTypeToString(getPlatform())).c_str() : mLogPrefix.c_str()); }

        private:
            BlazeRpcError callPSN(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo);
            bool isRetryError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const;
            bool isTokenExpiredError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const;

            static const size_t MAX_SERVICE_UNAVAILABLE_RETRIES = 2;
            static const uint64_t SERVICE_UNAVAILABLE_WAIT_SECONDS = 2;

            HttpConnectionManagerPtr mPsnConnMgrPtr;

        private:
            ExternalSessionServerConfig mConfig;
            ExternalAccessTokenUtilPs5 mTokenUtil;
            ExternalSessionMembersTrackingHelper mTrackHelper;
            ClientPlatformType mPlatform;

            mutable eastl::string mLogPrefix;
#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            friend class Blaze::Arson::ArsonSlaveImpl;
#endif
        };
    }
}

#endif
