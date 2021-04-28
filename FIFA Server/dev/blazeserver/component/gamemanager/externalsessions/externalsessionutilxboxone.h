/*! ************************************************************************************************/
/*!
    \file externalsessionutilxboxone.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_XBOXONE_H
#define BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_XBOXONE_H

#include "component/gamemanager/externalsessions/externalsessionutil.h"

namespace Blaze
{
    namespace XBLServices { class ProfileUser; class XBLClientSessionDirectorySlave; class MultiplayerSessionResponse; class MultiplayerSessionErrorResponse; class MultiplayerSessionTemplateResponse; class HandlesGetActivityResult; class PutMultiplayerSessionCreateRequest; class GetMultiplayerSessionsResponse; class ArenaTeamParams; typedef EA::TDF::TdfStructVector<ArenaTeamParams > ArenaTeamParamsList; class ArenaTournamentTeamInfo; class Member;  }
    namespace Arson { class ArsonSlaveImpl; class ArsonExternalTournamentUtilXboxOne; }

    namespace GameManager
    {
        class ExternalSessionUtilXboxOne : public ExternalSessionUtil
        {
            public:
                /*! ************************************************************************************************/
                /*! \brief interfaces with MPSD and handles XBL services
                    \param[in] platform The Xbox platform (xone or xbox series x)
                ***************************************************************************************************/
                ExternalSessionUtilXboxOne(ClientPlatformType platform, const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, bool titleIdIsHex, GameManagerSlaveImpl* gameManagerSlave)
                    : ExternalSessionUtil(config, reservationTimeout),
                    //Note: To 'not' specify inviteProtocol Blaze will now simply omit it in request, if its empty here. Note that MPSD rejects requests with inviteProtocol empty string.
                    mInviteProtocol(config.getExternalSessionInviteProtocol()),
                    mPlatform(platform),
                    mGameManagerSlave(gameManagerSlave)
                {
                    if (isDisabled())
                        WARN_LOG("[ExternalSessionUtilXboxOne] External sessions explicitly disabled. Blaze will not use MPSD.");

                    if (titleIdIsHex)
                    {
                        // convert the titleId to decimal before caching it
                        int32_t titleIdDec = strtol(config.getExternalSessionTitle(), nullptr, 16);
                        char8_t buf[128] = "";
                        blaze_snzprintf(buf, sizeof(buf), "%" PRId32, titleIdDec);
                        mTitleId.set(buf);
                    }
                    else
                        mTitleId.set(config.getExternalSessionTitle());
                }
               ~ExternalSessionUtilXboxOne() override{}

               static bool verifyParams(const ExternalSessionServerConfig& config, bool& titleIdIsHex, uint16_t maxMemberCount, bool validateSessionNames);

               Blaze::BlazeRpcError getAuthToken(const UserIdentification& ident, const char8_t* serviceName, eastl::string& buf) override;
               Blaze::BlazeRpcError create(const CreateExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit) override;
               Blaze::BlazeRpcError join  (const JoinExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit) override;
               Blaze::BlazeRpcError leave(const LeaveGroupExternalSessionParameters& params) override;
               Blaze::BlazeRpcError getMembers(const ExternalSessionIdentification& sessionIdentification, ExternalIdList& memberExternalIds) override;
               bool isUpdateRequired(const ExternalSessionUpdateInfo& oldValues, const ExternalSessionUpdateInfo& currentValues, const ExternalSessionUpdateEventContext& context) override;
               Blaze::BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params) override;
               bool isUpdatePresenceModeSupported() override { return false; }
               Blaze::BlazeRpcError checkRestrictions(const CheckExternalSessionRestrictionsParameters& params) override;
               bool isFriendsOnlyRestrictionsChecked() override { return true; }

               Blaze::BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult) override;
               Blaze::BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult) override;
               bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) override;

               Blaze::BlazeRpcError updateNonPrimaryPresence(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult) override;

               Blaze::BlazeRpcError getProfiles(const ExternalIdList& externalIds, ExternalUserProfiles& buf, const char8_t* groupAuthToken) override;

               Blaze::ClientPlatformType getPlatform() override { return mPlatform; }

        private:

            Blaze::TitleId mTitleId;
            eastl::string mInviteProtocol;
            ClientPlatformType mPlatform;
            GameManagerSlaveImpl* mGameManagerSlave;

        private:
            bool isDisabled() const { return isDisabled(mConfig.getContractVersion()); }
            static bool isDisabled(const char8_t* contractVersion) { return ((contractVersion == nullptr) || (contractVersion[0] == '\0')); }
            bool isHandlesDisabled() const { return isHandlesDisabled(mConfig.getExternalSessionHandlesContractVersion()); }
            static bool isHandlesDisabled(const char8_t* contractVersion) { return ((contractVersion == nullptr) || (contractVersion[0] == '\0')); }

            Blaze::BlazeRpcError createInternal(const char8_t* sessionTemplateName, const char8_t* sessionName, const ExternalSessionCreationInfo& params, const ExternalUserJoinInfo& userJoinInfo, const XBLServices::ArenaTeamParamsList* teamParams, Blaze::ExternalSessionResult* result, bool commit);
            Blaze::BlazeRpcError leaveUserFromLargeMps(const UserIdentification& user, const LeaveGroupExternalSessionParameters& params);
            Blaze::BlazeRpcError mpsJoinErrorToSpecificError(Blaze::BlazeRpcError origErr, const XBLServices::MultiplayerSessionErrorResponse& errRsp, const XBLServices::MultiplayerSessionResponse& mps);
            void scheduleCleanupAfterFailedJoin(ExternalUserJoinInfoList& cleanupList, const char8_t* sessionName, const char8_t* sessionTemplate);
            void cleanupAfterFailedJoin(LeaveGroupExternalSessionParametersPtr paramsLocal);

            Blaze::BlazeRpcError getAuthTokenInternal(ExternalXblAccountId xblId, const char8_t* serviceName, eastl::string& buf, bool forceRefresh);

            Blaze::BlazeRpcError getExternalSession(const char8_t* sessionTemplateName, const char8_t* sessionName, XBLServices::MultiplayerSessionResponse& result, const UserIdentification* onBehalfOfUser = nullptr);
            Blaze::BlazeRpcError getPrimaryExternalSession(ExternalXblAccountId xblId, XBLServices::HandlesGetActivityResult& result);

            bool isUpdateBasicPropertiesRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const;
            BlazeRpcError updateBasicProperties(const UpdateExternalSessionPropertiesParameters& params);

            bool isRecentPlayersBlazeManaged(const ExternalSessionUpdateInfo& gameInfo) const { return ((toXblLarge(gameInfo) == true) && (gameInfo.getRecentPlayersSettings().getGrouping() != ExternalSessionRecentPlayersSettings::NO_GROUPING) && (gameInfo.getRecentPlayersSettings().getMaxPlayersPerUpdate() != 0)); }
            bool isUpdateRecentPlayerGroupIdRequired(GetExternalSessionInfoMasterResponse& gameInfoRsp, const UpdateExternalSessionPresenceForUserParameters& updateReason) const;
            Blaze::BlazeRpcError updateRecentPlayerGroupId(const UserIdentification& user, const ExternalSessionUpdateInfo& gameInfo, TeamIndex teamIndex);
            static eastl::string& toXblRecentPlayerGroupId(eastl::string& buf, const ExternalSessionUpdateInfo& gameInfo, TeamIndex teamIndex) { return buf.sprintf("%" PRIu64 "_%u", gameInfo.getGameId(), ((gameInfo.getRecentPlayersSettings().getGrouping() == ExternalSessionRecentPlayersSettings::TEAM) ? teamIndex : UNSPECIFIED_TEAM_INDEX)); }
            static eastl::string& toXblOnBehalfOfUsersHeader(eastl::string& buf, const UserIdentification& user) { return (buf.append_sprintf("%s%" PRIu64 ";priv=multiplayer", (buf.empty() ? "" : ","), user.getPlatformInfo().getExternalIds().getXblAccountId())); }

            static bool isRetryError(BlazeRpcError mpsdCallErr);
            static bool isMpsAlreadyExistsError(BlazeRpcError mpsdCallErr);

            bool validateSessionTemplateName(const char8_t*& sessionTemplateName);

            const char8_t* toVisibility(const ExternalSessionUpdateInfo& updateInfo) const;
            const char8_t* toJoinRestriction(const ExternalSessionUpdateInfo& updateInfo) const;
            bool toClosed(const ExternalSessionUpdateInfo& updateInfo) const;
            bool toCrossPlay(const ExternalSessionUpdateInfo& updateInfo) const;
            bool toXblLarge(const ExternalSessionUpdateInfo& updateInfo) const;

            void validateGameSettings(const GameSettings& settings);

            static bool verifyXdpConfiguration(const char8_t* scid, const XblSessionTemplateNameList& sessionTemplateNameList, const char8_t* contractVersion, const char8_t* inviteProtocol, uint16_t maxMemberCount, const ExternalServiceCallOptions& callOptions);
            static Blaze::BlazeRpcError getExternalSessionTemplate(const char8_t* scid, const char8_t* sessionTemplateName, const char8_t* contractVersion, XBLServices::MultiplayerSessionTemplateResponse& result, const ExternalServiceCallOptions& callOptions);
        private:
            BlazeRpcError prepareArenaParams(XBLServices::PutMultiplayerSessionCreateRequest& xblRequest, const ExternalSessionCreationInfo& params, const XBLServices::ArenaTeamParamsList* teamParams, const ExternalUserJoinInfo& caller);
            BlazeRpcError validateArenaParams(const ExternalSessionCreationInfo& params, const ExternalUserJoinInfo& caller, eastl::string& startTimeStr);

            bool validateTeamsRegistered(XBLServices::ArenaTeamParamsList& foundTeams, const ExternalUserJoinInfoList& joiners, const TournamentIdentification& tournament);
            const XBLServices::ArenaTeamParams* findUserTeamInList(const UserIdentification& user, const XBLServices::ArenaTeamParamsList& list);
            XBLServices::ArenaTeamParams* findTeamInList(const char8_t* teamUniqueName, XBLServices::ArenaTeamParamsList& list);
            Blaze::BlazeRpcError getTeamForJoiningUser(const UserIdentification& user, const ExternalUserAuthInfo& authInfo, const TournamentIdentification& tournament, XBLServices::ArenaTournamentTeamInfo& result);
            bool addUserTeamParamIfMockPlatformEnabled(XBLServices::ArenaTeamParamsList& teamParams, const TournamentIdentification& tournament, const UserIdentification& user);

            bool stripUserClaim(eastl::string& authToken) const;

            const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalSessionUtilXboxOne(%s)]", ClientPlatformTypeToString(mPlatform)).c_str() : mLogPrefix.c_str()); }
            mutable eastl::string mLogPrefix;

#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            // for getExternalSession, getPrimaryExternalSession, getExternalSessionsForUser
            friend Blaze::Arson::ArsonSlaveImpl;
#endif
            friend Blaze::Arson::ArsonExternalTournamentUtilXboxOne;
        };
    }
}

#endif
