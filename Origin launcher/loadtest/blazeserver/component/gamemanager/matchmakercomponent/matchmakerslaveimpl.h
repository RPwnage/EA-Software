/*! ************************************************************************************************/
/*!
    \file matchmakerslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKER_SLAVE_IMPL_H
#define BLAZE_MATCHMAKER_SLAVE_IMPL_H

#include "gamemanager/rpc/matchmakerslave_stub.h"
#include "gamemanager/matchmakercomponent/matchmakingjob.h"
#include "gamemanager/matchmakercomponent/timetomatchestimator.h"
#include "gamemanager/scenario.h"
#include "gamemanager/externalsessions/externalsessionutilmanager.h"

#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/usersessions/usersessionsubscriber.h" // for UserSessionSubscriber
#include "framework/usersessions/qosconfig.h"

#include "EASTL/string.h" // for string in validateGameSizeRulesConfiguration()
#include "EASTL/vector_set.h"


namespace Blaze
{

    class UserSession;

namespace GameManager
{
    namespace Matchmaker
    {
        class Matchmaker;
    }
}
namespace Matchmaker
{

class MatchmakerSlaveImpl : public MatchmakerSlaveStub, 
                            private UserSessionSubscriber,
                            private PingSiteUpdateSubscriber,
                            private DrainStatusCheckHandler
{

public:

    MatchmakerSlaveImpl();
    ~MatchmakerSlaveImpl() override;

    // UserSessionProvider interface impls    
    void onUserSessionExtinction(const UserSession& userSession) override;

    GameManager::Matchmaker::Matchmaker *getMatchmaker() const { return mMatchmaker; }
    Blaze::GameManager::GameManagerMaster* getGameManagerMaster() const { return mGameManagerMaster; }

    bool getEvaluateGameProtocolVersionString() const;

    // MM jobs
    bool addMatchmakingJob(MatchmakingJob& job);
    MatchmakingJob* getMatchmakingJob(const Blaze::GameManager::MatchmakingSessionId &mmSessionId) const;
    void removeMatchmakingJob(const Blaze::GameManager::MatchmakingSessionId &mmSessionId);

    void sendNotifyMatchmakingStatusUpdate(Blaze::GameManager::MatchmakingStatus &status);
    
    //+ External sessions
    // external session for the game session
    BlazeRpcError DEFINE_ASYNC_RET(joinGameExternalSession(const JoinExternalSessionParameters& params, bool requiresSuperUser, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionApiError& apiErr));
    
    // external session for the matchmaking session
    BlazeRpcError DEFINE_ASYNC_RET(joinMatchmakingExternalSession(Blaze::GameManager::MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName, const GameManager::Matchmaker::MatchmakingSession::MemberInfoList& members, Blaze::ExternalSessionIdentification& externalSessionIdentification));
    void leaveMatchmakingExternalSession(Blaze::GameManager::MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName, ExternalUserLeaveInfoList& members);// intentionally not marked async, as this is fire and forget.
    
    // The external session template name for a mm session. Currently only established if the mm session contains xone users.
    // The template name can be overriden by a client request otherwise it's the first configured template.
    // Used during session evaluation
    void getExternalSessionTemplateName(XblSessionTemplateName& sessionTemplateName, const XblSessionTemplateName& overrideName, const Blaze::GameManager::UserJoinInfoList& userJoinInfoList);
    
    const GameManager::ExternalSessionUtilManager& getGameSessionExternalSessionUtilMgr() const { return mGameSessionExternalSessionUtilMgr; }

    // Use in internal matchmaking response to pass along - startMatchmakingInternal rpc
    void getExternalSessionScid(EA::TDF::TdfString& scid);
    //-

    // DrainStatusCheckHandler interface
    bool isDrainComplete() override;

    // return true if the given topology needs QoS validation
    bool shouldPerformQosCheck(GameNetworkTopology topology) const;

    // update time to match estimates
    void updateTimeToMatch(const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType &delineationGroup, const GameManager::ScenarioName &scenarioName, GameManager::MatchmakingResult result);
    
    const TimeToMatchEstimator& getTimeToMatchEstimator() const 
    { 
        return mTimeToMatchEstimator;
    }
    
private:

    bool onValidatePreconfig(MatchmakerSlavePreconfig& config, const MatchmakerSlavePreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onPreconfigure() override;
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onResolve() override;
    void onShutdown() override;
    bool onValidateConfig(MatchmakerConfig& config, const MatchmakerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onPrepareForReconfigure(const MatchmakerConfig& newConfig) override;
    bool validateConfiguredGameSizeRules(const Blaze::GameManager::MatchmakingServerConfig& config, eastl::string &err) const;
    void getStatusInfo(ComponentStatus& status) const override;
    void onSlaveSessionAdded(SlaveSession& session) override;

    // rpc handlers
    GetMatchmakingConfigError::Error processGetMatchmakingConfig(Blaze::GameManager::GetMatchmakingConfigResponse &response, const Message *message) override;
    GetMatchmakingInstanceStatusError::Error processGetMatchmakingInstanceStatus(Blaze::GameManager::MatchmakingInstanceStatusResponse &response, const Message *message) override;
    MatchmakingDedicatedServerOverrideError::Error processMatchmakingDedicatedServerOverride(const Blaze::GameManager::MatchmakingDedicatedServerOverrideRequest &request, const Message *message) override;
    GetMatchmakingDedicatedServerOverridesError::Error processGetMatchmakingDedicatedServerOverrides(Blaze::GameManager::GetMatchmakingDedicatedServerOverrideResponse &response, const Message* message) override;
    GetMatchmakingFillServersOverrideError::Error processGetMatchmakingFillServersOverride(Blaze::GameManager::MatchmakingFillServersOverrideList &response, const Message* message) override;
    MatchmakingFillServersOverrideError::Error processMatchmakingFillServersOverride(const Blaze::GameManager::MatchmakingFillServersOverrideList &request, const Message* message) override;
    GameSessionConnectionCompleteError::Error processGameSessionConnectionComplete(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response, const Message* message) override;
    GetMatchmakingMetricsError::Error processGetMatchmakingMetrics(Blaze::GameManager::GetMatchmakingMetricsResponse &response, const Message* message) override;

    // mm listener (NO-OPs), implementation required by generated code.
    void onNotifyMatchmakingFailed(const Blaze::GameManager::NotifyMatchmakingFailed& data, UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingFinished(const Blaze::GameManager::NotifyMatchmakingFinished& data,UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingAsyncStatus(const Blaze::GameManager::NotifyMatchmakingAsyncStatus& data, UserSession *associatedUserSession) override {}    
    void onNotifyMatchmakingSessionConnectionValidated(const Blaze::GameManager::NotifyMatchmakingSessionConnectionValidated& data,UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingPseudoSuccess(const Blaze::GameManager::NotifyMatchmakingPseudoSuccess& data, UserSession *associatedUserSession) override {}
    void onNotifyRemoteMatchmakingStarted(const Blaze::GameManager::NotifyRemoteMatchmakingStarted& data,UserSession *associatedUserSession) override {}
    void onNotifyRemoteMatchmakingEnded(const Blaze::GameManager::NotifyRemoteMatchmakingEnded& data,UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingDedicatedServerOverride(const Blaze::GameManager::NotifyMatchmakingDedicatedServerOverride& data, Blaze::UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingStatusUpdate(const Blaze::GameManager::MatchmakingStatus& data, Blaze::UserSession *associatedUserSession) override {}
    void onNotifyServerMatchmakingReservedExternalPlayers(const Blaze::GameManager::NotifyServerMatchmakingReservedExternalPlayers &data,Blaze::UserSession *associatedUserSession) override {}

    void onPingSiteMapChanges(const PingSiteInfoByAliasMap& pingSiteMap) override;

    GameManager::MatchmakingScenarioManager mScenarioManager; // for preconfiguration
    GameManager::Matchmaker::Matchmaker *mMatchmaker;
    Blaze::GameManager::GameManagerMaster *mGameManagerMaster;

    GameManager::ExternalSessionUtilManager mMatchmakingExternalSessionUtilMgr; // external sessions manager for a matchmaking session
    GameManager::ExternalSessionUtilManager mGameSessionExternalSessionUtilMgr; // external sessions manager for a game session created/found via matchmaking
    
                                                                                // map of matchmaking jobs, which complete the rpc calls required to finalize matchmaking
    typedef eastl::map<Blaze::GameManager::MatchmakingSessionId, MatchmakingJob*> MatchmakingJobMap;
    MatchmakingJobMap mMatchmakingJobMap;

    TimeToMatchEstimator mTimeToMatchEstimator;

    void initializeTtmEstimates();
};

}
}
#endif
