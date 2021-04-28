/*! ************************************************************************************************/
/*!
    \file externalsessionutilps4.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_PS4_H
#define BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_PS4_H
#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "framework/protocol/restprotocolutil.h" //for getContentTypeFromEncoderType() in callPSN
#include "gamemanager/externalsessions/externalsessionmemberstrackinghelper.h" //for mTrackHelper

namespace Blaze
{
    namespace PSNServices { class PsnWebApiHeader; class PsnErrorResponse; class GetNpSessionIdsResponse; class NpSessionIdContainer; class GetNpSessionResponse; class LocalizedSessionStatusEntity; typedef EA::TDF::TdfStructVector<LocalizedSessionStatusEntity > LocalizedSessionStatusEntityList; }
    namespace GameManager { class GameManagerMaster; struct ExternalSessionBinaryDataHead; struct ExternalSessionChangeableBinaryDataHead; } //for GameManagerMaster in getMaster, getNpSessionData, updateChangeableData, toExternalSessionErrorInfo

    namespace Arson { class ArsonSlaveImpl; }

    namespace GameManager
    {
        class ExternalSessionUtilPs4 : public ExternalSessionUtil
        {
        public:
            ExternalSessionUtilPs4(const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, GameManagerSlaveImpl* gameManagerSlave);
            ~ExternalSessionUtilPs4() override;

            static bool verifyParams(const ExternalSessionServerConfig& config);
            BlazeRpcError getAuthToken(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf) override;

            // Note: create/join/leave are currently unimplemented, and session membership is determined by setPrimary/clearPrimary (see DevNet ticket 58807).
            BlazeRpcError create(const CreateExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit) override { return (isMultipleMembershipSupported()? ERR_SYSTEM : ERR_OK); }
            BlazeRpcError join (const JoinExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit) override { return (isMultipleMembershipSupported()? ERR_SYSTEM : ERR_OK); }
            BlazeRpcError leave(const LeaveGroupExternalSessionParameters& params) override { return (isMultipleMembershipSupported()? ERR_SYSTEM : ERR_OK); }

            bool isUpdatePresenceModeSupported() override { return false; }

            bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) override;
            BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params) override;
            BlazeRpcError updateImage(const UpdateExternalSessionImageParameters& params, UpdateExternalSessionImageErrorResult& errorResult) override;
            BlazeRpcError getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParams = nullptr) override;

            BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult) override;
            BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult) override;
            bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) override;

            Blaze::ClientPlatformType getPlatform() override { return ClientPlatformType::ps4; }

        protected:
            // reconfigure methods
            void setImageDefault(const ExternalSessionImageHandle& value) override;
            void setStatusValidLocales(const char8_t* value) override;
            bool onPrepareForReconfigure(const ExternalSessionServerConfig& config) override { return verifyParams(config); }

        private:
            // currently there's no distinction between being primary session and membership (see DevNet ticket 58807), so disabling depends only on the primaryExternalSessionTypeForUx
            static bool isDisabled(const ExternalSessionServerConfig& config) { return config.getPrimaryExternalSessionTypeForUx().empty(); }
            bool isDisabled() const { return isDisabled(mConfig); }
            static bool isMultipleMembershipSupported() { return false; }//currently always false (see DevNet ticket 58807)

            typedef eastl::hash_set<eastl::string> LocalesSet;
            static bool loadDefaultImage(Ps4NpSessionImage& imageData, const ExternalSessionImageHandle& fileName, const ExternalSessionServerConfig& config);
            static bool loadSupportedLocalesSet(LocalesSet& localesSet, const char8_t* configuredList);
            static bool loadImageDataFromFile(Ps4NpSessionImage& imageData, const ExternalSessionImageHandle& fileName, size_t maxImageSize);
            static bool isValidImageFileExtension(const ExternalSessionImageHandle& fileName, const char8_t* validExtensions[], size_t validExtensionsSize);
            static bool isValidImageData(const Ps4NpSessionImage& imageData, size_t maxImageSize);

            Blaze::BlazeRpcError getAuthTokenInternal(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf, bool forceRefresh);
            BlazeRpcError createNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, const GetExternalSessionInfoMasterResponse& gameInfo, ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId);
            BlazeRpcError joinNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, const GetExternalSessionInfoMasterResponse& gameInfo, ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId);
            BlazeRpcError leaveNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, GameId gameId, bool untrackBlazeMembership, ExternalSessionErrorInfo* errorInfo);
            BlazeRpcError syncNpsAndTrackedMembership(const UserIdentification& member, const ExternalUserAuthInfo& memberAuth, const GetExternalSessionInfoMasterResponse& gameInfo, ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId);

            bool isUpdateBasicPropertiesRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues);
            bool isUpdateChangeableDataRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues);
            BlazeRpcError updateBasicProperties(const UpdateExternalSessionPropertiesParameters& params, ExternalUserAuthInfo& authInfo);
            BlazeRpcError updateChangeableData(const UpdateExternalSessionPropertiesParameters& params, ExternalUserAuthInfo& authInfo);
            void logUpdateErr(BlazeRpcError err, const char8_t* context, const char8_t* npSessionId, const UserIdentification& caller);
            
            void toLocalizedSessionStatus(const ExternalSessionUpdateInfo& info, Blaze::PSNServices::LocalizedSessionStatusEntityList& statusList) const;
            const char8_t* toSessionPrivacy(PresenceMode presenceMode, GameSettings settings) const;
            bool toSessionLockFlag(const ExternalSessionUpdateInfo& updateInfo) const;
            void toSessionDataHead(const ExternalSessionCreationInfo& creationInfo, ExternalSessionBinaryDataHead& blazeDataHead) const;
            void toSessionChangeableDataHead(const ExternalSessionUpdateInfo& updateInfo, ExternalSessionChangeableBinaryDataHead& blazeDataHead) const;
            bool isSupportedLocale(Ps4NpSessionStatusStringLocaleMap::const_iterator& itr) const;
            const char8_t* getValidJsonUtf8(const char8_t* value, bool isNameNotStatus) const;

            BlazeRpcError getPrimaryNpsIdAndGameId(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, NpSessionId* npSessionId, GameId& gameId, ExternalSessionErrorInfo* errorInfo);
            BlazeRpcError getPrimaryNpsId(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, Blaze::PSNServices::NpSessionIdContainer& sessionResult, ExternalSessionErrorInfo* errorInfo);
            BlazeRpcError getNpSessionData(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, ExternalSessionBinaryDataHead& resultHead, ExternalSessionCustomData* resultTitle, ExternalSessionErrorInfo* errorInfo);
            

            bool isSetPrimaryRequired(const SetPrimaryExternalSessionParameters& newPrimaryInfo, GetExternalSessionInfoMasterResponse& existingPrimaryInfo) const;
            BlazeRpcError trackMembershipOnMaster(const UserIdentification& member, const ExternalUserAuthInfo& memberAuth, const GetExternalSessionInfoMasterResponse& gameInfo, const ExternalSessionIdentification& extSessionToAdvertise, GetExternalSessionInfoMasterResponse& result) const;
            BlazeRpcError untrackMembershipOnMaster(const UserIdentification& member, const ExternalUserAuthInfo& memberAuth, GameId gameId) const;
            BlazeRpcError untrackNpsIdOnMaster(GameId gameId, const char8_t* npSessionIdToClear) const;

            bool isConnectionExpiredError(BlazeRpcError psnErr) const;
            bool isRetryError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const;
            bool isTokenExpiredError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const;
            bool isUserSignedOutError(BlazeRpcError psnErr, int32_t psnErrorInfoCode) const;
            void toExternalSessionErrorInfo(const Blaze::PSNServices::PsnErrorResponse& psnRsp, Blaze::ExternalSessionErrorInfo& errorInfo, const char8_t* extraContextInfo) const;
            void toExternalSessionRateInfo(const Blaze::PSNServices::GetNpSessionIdsResponse& psnRsp, RateLimitInfo& rateInfo, const char8_t* extraContextInfo) const;
            const char8_t* getPsnAccountId(const UserIdentification& user, eastl::string& buf) const { return buf.sprintf("%" PRIu64"", user.getPlatformInfo().getExternalIds().getPsnAccountId()).c_str(); }
            Blaze::PsnServiceLabel getServiceLabel() const { return mConfig.getExternalSessionServiceLabel(); }

            BlazeRpcError callPSN(const UserIdentification& caller, const CommandInfo& cmdInfo, const ExternalUserAuthInfo& authInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, ExternalSessionErrorInfo* errorInfo = nullptr, const char8_t* contentType = RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), bool addEncodedPayload = false);
            BlazeRpcError callPSNInternal(const UserIdentification& caller, const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, PSNServices::PsnErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload, bool refreshBaseUrl = false);


            HttpConnectionManagerPtr getWebApiConnMgr(const UserIdentification& caller, const CommandInfo& cmdInfo, bool refreshBaseUrl, BlazeRpcError& err);
            BlazeRpcError getNewWebApiConnMgr(const UserIdentification& caller, const char8_t* psnWebApi, const HttpServiceConnection* config, HttpConnectionManagerPtr& connMgr);
            void metricCommandStatus(const CommandInfo& cmdInfo, const TimeValue& duration, HttpStatusCode statusCode) const;

            BlazeRpcError getBaseUrl(const UserIdentification& caller, const char8_t* apiName, eastl::string& baseUrlBuf);


            const char8_t* toAccountIdLogStr(const UserIdentification& user) const { return getPsnAccountId(user, mAccountIdBuf); }
            const char8_t* toLogStr(BlazeRpcError err, const Blaze::PSNServices::PsnErrorResponse *psnRsp = nullptr) const;
            const char8_t* toLogStr(const ExternalSessionBinaryDataHead& dataHead, const ExternalSessionCustomData* titleData) const;
            const char8_t* toLogStr(const ExternalSessionStatus& status) const;
            const char8_t* toLogStr(const uint8_t* bytes, size_t bytesLen, eastl::string& buf) const;

            const char8_t* updateTitleIdIfMockPlatformEnabled(const char8_t* origTitleId) const;

        private:
            Ps4NpSessionImage mDefaultImage;//cached default image
            LocalesSet mSupportedLocalesSet;//cached set of locales

            // Because PSN's Base URL changes at runtime, we use a local connection manager instead of the configured instance (metrics are tracked with the configured instance)
            mutable HttpConnectionManagerPtr mCachedSessionInvitationConnManager;
            ExternalSessionMembersTrackingHelper mTrackHelper;

            mutable eastl::string mAccountIdBuf;
            mutable eastl::string mUserIdentLogBuf;
            mutable eastl::string mErrorLogBuf;
            mutable eastl::string mBinaryDataLogBuf;
            mutable eastl::string mLocStatusLogBuf;
#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            friend class Blaze::Arson::ArsonSlaveImpl;
#endif
        };

    }
}

#endif
