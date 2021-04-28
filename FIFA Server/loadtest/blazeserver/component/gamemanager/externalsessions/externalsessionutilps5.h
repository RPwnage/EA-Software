/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_EXTERNAL_SESSION_UTIL_PS5_H
#define COMPONENT_EXTERNAL_SESSION_UTIL_PS5_H

#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5matches.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5playersessions.h"
#include "framework/connection/outboundhttpconnectionmanager.h"//for HttpConnectionManagerPtr in callPSNInternal

namespace Blaze
{
    namespace GameManager { class GameManagerSlaveImpl; class GameSessionMaster; class CreateGameRequest; class ExternalSessionServerConfig; }
    namespace Arson { class ArsonSlaveImpl; }

    namespace GameManager
    {
        class ExternalSessionUtilPs5 : public ExternalSessionUtil
        {
        public:
            ExternalSessionUtilPs5(ClientPlatformType platform, const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, GameManagerSlaveImpl* gameManagerSlave);
            ~ExternalSessionUtilPs5() override;

            static bool verifyParams(const ExternalSessionServerConfig& config, const TeamNameByTeamIdMap* teamNameByTeamIdMap, const GameManagerSlaveImpl* gameManagerSlave);
            static bool isCrossgenConfiguredForPlatform(ClientPlatformType platform, const GameManagerSlaveImpl* gameManagerSlave);

            BlazeRpcError updateNonPrimaryPresence(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult) override;

            BlazeRpcError getAuthToken(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf) override;
            BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult) override;
            BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult) override;
            bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) override;


            bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) override;
            BlazeRpcError update(const UpdateExternalSessionPropertiesParameters& params) override;
            BlazeRpcError getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParams = nullptr) override;



            void prepForReplay(GameSessionMaster& gameSession) override;
            Blaze::BlazeRpcError prepForCreateOrJoin(CreateGameRequest& request) override;


            bool isUpdatePresenceModeSupported() override { return false; }
            ClientPlatformType getPlatform() override { return mPlatform; }
            const ExternalSessionActivityTypeInfo& getPrimaryActivityType() override { return mPlayerSessions.getActivityTypeInfo(); }
            static const eastl::string& getPsnAccountId(eastl::string& buf, const UserIdentification& user) { return buf.sprintf("%" PRIu64 "", user.getPlatformInfo().getExternalIds().getPsnAccountId()); }
            static const FixedString256 toLogStr(BlazeRpcError err, const PSNServices::PsnErrorResponse *psnRsp = nullptr);
            static void toExternalSessionErrorInfo(Blaze::ExternalSessionErrorInfo *errorInfo, const Blaze::PSNServices::PsnErrorResponse& psnRsp);

            static BlazeRpcError callPSNInternal(HttpConnectionManagerPtr connMgr, const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, PSNServices::PsnErrorResponse* rspErr, RawBuffer* rspRaw = nullptr);
            static BlazeRpcError waitTime(const TimeValue& time, const char8_t* context, size_t logRetryNumber) { return ExternalSessionUtil::waitTime(time, context, logRetryNumber); }
            static void getNewWebApiConnMgr(HttpConnectionManagerPtr& connMgr, HttpServiceName serviceCompName);

            bool onPrepareForReconfigure(const ExternalSessionServerConfig& config) override;

        private:
            const char8_t* logPrefix() const;

        private:
            Blaze::GameManager::ExternalSessionUtilPs5Matches mMatches;
            Blaze::GameManager::ExternalSessionUtilPs5PlayerSessions mPlayerSessions;
            ClientPlatformType mPlatform;

            mutable eastl::string mLogPrefix;
#ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
            friend class Blaze::Arson::ArsonSlaveImpl;
#endif
        };
    }
}

#endif
