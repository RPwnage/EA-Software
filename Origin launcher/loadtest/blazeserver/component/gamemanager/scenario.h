/*! ************************************************************************************************/
/*!
    \file   scenario.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_SCENARIO
#define BLAZE_MATCHMAKING_SCENARIO

#include "framework/util/safe_ptr_ext.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/tdf/scenarios.h"
#include "gamemanager/tdf/scenarios_config_server.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/matchmakingmetrics_server.h"
#include "gamemanager/externaldatamanager.h"
#include "framework/util/expression.h"
#include "gamemanager/matchmakingfiltersutil.h"
#include "framework/redis/rediskeymap.h"

namespace Blaze
{
    namespace Matchmaker
    {
        class MatchmakerMaster;
        class MatchmakerSlave;
    }

    namespace GamePacker
    {
        class GamePackerMaster;
    }

namespace GameManager
{
    class GameManagerSlaveImpl;
    class MatchmakingScenario;
    class MatchmakingScenarioSession;
    class MatchmakingScenarioConfigInfo;
    class MatchmakingScenarioVariantConfigInfo;
    class MatchmakingSubSessionConfigInfo;

    typedef safe_ptr_ext<class MatchmakingScenario> MatchmakingScenarioRef;
    typedef safe_ptr_ext<class MatchmakingScenarioConfigInfo> MatchmakingScenarioConfigInfoRef;
    typedef safe_ptr_ext<class MatchmakingScenarioVariantConfigInfo> MatchmakingScenarioVariantConfigInfoRef;
    typedef eastl::map<ScenarioVariantName, MatchmakingScenarioVariantConfigInfoRef> ScenarioVariantsMap;
    typedef eastl::hash_map<eastl::string, PropertyNameList> TemplateToPropertyListMap;

    class MatchmakingScenarioManager
    {
    public:
        MatchmakingScenarioManager(GameManagerSlaveImpl* componentImpl);
        ~MatchmakingScenarioManager();

        // Notifications: 
        void onNotifyMatchmakingFailed(const NotifyMatchmakingFailed& notifyMatchmakingFinished);
        void onNotifyMatchmakingFinished(const NotifyMatchmakingFinished& notifyMatchmakingFinished);
        void onNotifyMatchmakingAsyncStatus(const NotifyMatchmakingAsyncStatus& data);
        void onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess& notifyMatchmakingDebugged);
        void onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated& data);

        void onUserSessionExtinction(UserSessionId sessionId);
        void onLocalUserSessionImported(const UserSessionMaster& userSession);
        void onLocalUserSessionExported(const UserSessionMaster& userSession);

        BlazeRpcError validateMaxActiveMMScenarios(UserSessionId userSessionId) const;

        BlazeRpcError getScenarioAttributesConfig(GetScenariosAttributesResponse& response);
        BlazeRpcError getScenariosAndAttributes(GetScenariosAndAttributesResponse& response);
        BlazeRpcError getScenarioDetails(GetScenarioDetails& response) const;

        BlazeRpcError createScenario(StartMatchmakingScenarioRequest& request, StartMatchmakingScenarioResponse& response, 
            MatchmakingCriteriaError& critErr, const InetAddress& callerNetworkAddress, const OverrideUserSessionInfo *overrideUserSessionInfo = nullptr);

        bool fetchRecoGroup(MatchmakingScenarioConfigInfoRef variantConfig, StartMatchmakingScenarioRequest& request, ScenarioVariantName &variant, eastl::string &tag);
        bool selectScenarioVariant(MatchmakingScenarioConfigInfoRef variantConfig, StartMatchmakingScenarioRequest& request, ScenarioVariantName &variant, eastl::string &tag, const OverrideUserSessionInfo *overrideUserSessionInfo);
        BlazeRpcError cancelScenario(MatchmakingScenarioId scenarioId, bool skipUserSessionCheck = false);
        
        void getStatusInfo(ComponentStatus& componentStatus) const;
        bool hashScenarioConfiguration(const GameManagerServerConfig& config);
        bool validatePreconfig(const GameManagerServerPreconfig& config) const;
        bool preconfigure(const GameManagerServerPreconfig& config);
        bool configure(const GameManagerServerConfig& configTdf);
        bool reconfigure(const GameManagerServerConfig& configTdf);
        bool validateConfig(const ScenariosConfig& config, const MatchmakingServerConfig& mmSettings, const GameManagerServerConfig* configTdf, const GameManagerServerPreconfig& preConfig, ConfigureValidationErrors& validationErrors) const;
        void cancelAllScenarios();
        bool loadScenarioConfigInfoFromDB();

        const char8_t* getGlobalScenarioAttributeNameByTdfMemberName(const char8_t* memberName) const;

        GameManagerSlaveImpl& getComponent() const { return *mComponent; }
        Blaze::Matchmaker::MatchmakerSlave* getMatchmakerSlave() const;
        Blaze::GamePacker::GamePackerMaster* getGamePackerMaster() const;
        void removeScenario(MatchmakingScenarioId scenarioId);
        const TemplateToPropertyListMap& getTemplateToPropertyListMap() { return mTemplateToPropertyListMap; }

        
        /*! ************************************************************************************************/
        /*! \brief send a matchmaking request to a matchmaking slave
        
            \param[in] request - the matchmaking request to issue 
            \param[out] response - rpc response from StartMatchmakingInternalRpc
            \param[out] error - error response object, if needed
            \param[in] callerNetworkAddress - the network address from the connection that called the RPC
            \param[in] overrideUserSessionInfo - usersession info (including UED) to use as a replacement for values in gCurrentUsersession
            \param[in] session - scenario session that triggered the matchmaking request
            \return ERR_OK if successful
        ***************************************************************************************************/
        BlazeRpcError startMatchmaking(const StartMatchmakingRequest &request, StartMatchmakingInternalResponse &response, 
            MatchmakingCriteriaError &error, const InetAddress& callerNetworkAddress, MatchmakingScenarioSession& scenarioSession, 
            const StartMatchmakingInternalRequest* subMasterRequest = nullptr, bool hasBadReputation = false, uint32_t sessionSeq = 0);

        // Helper to get the matchmaker instance used by the scenario's subsessions:
        InstanceId selectMatchmakerInstance(const MatchmakingSessionMode& sessionMode) const;

        // matchmaking request prep methods
        BlazeRpcError fillInSubMasterRequest(StartMatchmakingInternalRequest& masterRequest,
            MatchmakingCriteriaError &errorResponse, const InetAddress& callerNetworkAddress, const OverrideUserSessionInfoPtr overrideUserSessionInfo, 
            bool& hasBadReputation, UserIdentificationList& groupUserIds, bool bByPassBlockList = false);

        BlazeRpcError getScenarioVariants(const Blaze::GameManager::GetScenarioVariantsRequest& request, Blaze::GameManager::GetScenarioVariantsResponse& response);

        BlazeRpcError clearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& data);
        BlazeRpcError userScenarioVariantUpdate(const Blaze::GameManager::SetUserScenarioVariantRequest& data);

        void onClearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& data);
        void onUserScenarioVariantUpdate(const Blaze::GameManager::SetUserScenarioVariantRequest& data);

        /*! ************************************************************************************************/
        /*! \brief Resolve original criteria's UserSet inputs to member blaze ids and add them to the master
        criteria's blaze id lists. Works around fact UserSets are n/a on master, for create game MM.
        ***************************************************************************************************/
        static BlazeRpcError playersRulesUserSetsToBlazeIds(MatchmakingCriteriaData& criteriaData, MatchmakingCriteriaError& err);

        // Activates the CensusDataFiber (always returns true)
        bool onActivate();
        void onResolve();
        const GameManagerCensusData& getGameManagerCensusData() const { return mGMCensusData; }
        const PropertyManager& getPropertyManager() const { return mPropertyManager; }
        const RedisKeyMap<GameManager::PlayerId, GameManager::GameId>& getDedicatedServerOverrideMap() const { return mDedicatedServerOverrideMap; }
        
    private:
        void clearScenarios();
        void clearScenarioConfigs();
        void clearActiveScenarioConfigInfoMap();
        void addAttributeMapping(ScenarioAttributeMapping* attrMapping, const ScenarioAttributeMapping& scenarioAttributeMapping);
        void addAttributeDefinition(TemplateAttributeDescriptionMapping* attrDefinitionMapping, const ScenarioAttributeMapping& scenarioAttributeMapping, bool includeDebugAttributes);
        void addAttributeTdfDefinition(TemplateAttributeDescriptionMapping* attrDefinitionMapping, const TemplateAttributeTdfMapping& tdfAttributeMapping, bool includeDebugAttributes);
        void checkForAttrNameOverlap(eastl::map<eastl::string, const EA::TDF::TypeDescription*>& scenarioAttrToTypeDesc, const char* attrName, const EA::TDF::TypeDescription* typeDesc, ConfigureValidationErrors& validationErrors) const;
        EA::TDF::TdfHashValue getScenarioRuleAttributesHash(ScenarioRuleAttributes& scenarioRuleAttributes);
        
        struct ScenarioConfigInfo
        {
            ScenarioName mName;
            ScenarioVersion mVersion;
            ScenarioHash mHash;
            ScenarioVariantName mVariant;
            TimeValue mConfigurationTime;
        };

        typedef eastl::map<ScenarioVariantName, ScenarioConfigInfo*> ScenarioConfigInfoByVariantMap;
        typedef eastl::map<ScenarioName, ScenarioConfigInfoByVariantMap> ScenarioVariantsConfigInfoByNameMap;
        ScenarioVariantsConfigInfoByNameMap mActiveScenarioConfigInfoMapByName;
        
        ScenarioVariantsConfigInfoByNameMap& getScenarioConfigInfoMapByName() { return mActiveScenarioConfigInfoMapByName; }
        void updateScenarioConfigInDB(eastl::vector<ScenarioConfigInfo*>* updateScenarioConfigList);

        void computeScenarioHash(const SubSessionName& subSessionName, const SubSessionConfig* subSessionConfig, const ScenarioRuleMap &scenarioRuleMap, EA::TDF::TdfHashValue& scenarioHash);

        void updateScenarioConfigInfo(const ScenarioName& scenarioName, EA::TDF::TdfHashValue& scenarioHash, const ScenarioVariantName& variant, eastl::vector<ScenarioConfigInfo*>* updateScenarioConfigInfoDBList);

        BlazeRpcError fillBlockList(StartMatchmakingInternalRequest& masterRequest, MatchmakingCriteriaError& err) const;
        // method to convert a MM component error to a GM component error
        BlazeRpcError convertMatchmakingComponentErrorToGameManagerError(const BlazeRpcError &mmRpcError);

        // Use reference counted Scenarios:
        typedef eastl::map<MatchmakingScenarioId, MatchmakingScenarioRef> ScenarioByIdMap;
        ScenarioByIdMap mScenarioByIdMap;
        typedef eastl::map<ScenarioName, MatchmakingScenarioConfigInfoRef> ScenarioConfigByNameMap;
        ScenarioConfigByNameMap mScenarioConfigByNameMap;

        // Fiber used to update the GM Census Data cache
        void getGameManagerCensusDataFiber();

        // Census Data cached by Scenario, used for variant criteria checks
        GameManagerCensusData mGMCensusData;
        Fiber::FiberHandle mCensusDataFiber;

        GameManagerSlaveImpl* mComponent;
        mutable Blaze::Matchmaker::MatchmakerSlave* mMatchmakerSlave;
        const ScenariosConfig* mConfig;
        PropertyManager mPropertyManager;

        RedisKeyMap<GameManager::PlayerId, GameManager::GameId> mDedicatedServerOverrideMap;

        TemplateToPropertyListMap mTemplateToPropertyListMap;
        void populateTemplateToPropertyListMap(const GameManagerServerConfig& config);
    };
        

    class MatchmakingScenario :
        public eastl::safe_object
    {
    public:
        MatchmakingScenario(MatchmakingScenarioId scenarioId, const StartMatchmakingScenarioRequest& request, 
            MatchmakingScenarioVariantConfigInfoRef config, MatchmakingScenarioManager& owner, const OverrideUserSessionInfo *overrideUserSessionInfo);
        MatchmakingScenario(MatchmakingScenarioDataPtr& scenarioData, MatchmakingScenarioVariantConfigInfoRef config, MatchmakingScenarioManager& owner);
        ~MatchmakingScenario();

        BlazeRpcError cancelSessions();
        GameManagerSlaveImpl& getComponent() const { return mManager.getComponent(); }
        Blaze::Matchmaker::MatchmakerSlave* getMatchmakerSlave() { return mManager.getMatchmakerSlave(); }

        void upsertMMScenarioForUser(bool allowInsert = true);
        void removeMMScenarioForUser();

        bool areAllSessionsComplete();

        void onNotifyMatchmakingFailed(const NotifyMatchmakingFailed& notifyMatchmakingFailed);
        void onNotifyMatchmakingFinished(const NotifyMatchmakingFinished& notifyMatchmakingFinished);
        void onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated& data);
        void onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess& notifyMatchmakingDebugged);
        
        void onSessionFinished(bool success, MatchmakingSessionId sessionId);

        MatchmakingScenarioDataPtr getScenarioData() const { return mScenarioData; }
        MatchmakingScenarioId getScenarioId() const { return mScenarioData->getScenarioId(); }
        UserSessionId getHostUserSessionId() const { return mScenarioData->getHostUserSessionId(); }

        BlazeRpcError setupAndStartSessions(StartMatchmakingScenarioResponse& response, MatchmakingCriteriaError& critErr, const InetAddress& callerNetworkAddress, UserIdentificationList& groupUserIds, CallerProperties& callerSourceProperties);

        void deleteIfUnreferenced(bool clearRefs = false);

        const MatchmakingScenarioVariantConfigInfo& getConfigInfo() const { return *mConfigInfoRef; }

        void setTrackingTag(eastl::string& tag) { mTrackingTag = tag; }
        const char8_t* getTrackingTag() { return mTrackingTag.c_str(); }

        // mTotalDurationOverride is used to override the Scenario's duration. 
        TimeValue getTotalDuration() const;
        MatchmakingScenarioManager& getManager() const { return mManager; }

    private:
        BlazeRpcError cancelSessionsFiber(MatchmakingScenarioRef scenarioRef);
        void finishScenario();
        void initializeOwnedSessions();

        void sendNotifyRemoteMatchmakingStartedToGroup();
        void sendNotifyRemoteMatchmakingEndedToGroup(GameManager::MatchmakingResult result);

        friend class MatchmakingScenarioSession;

        // Copy of the original request:
        OverrideUserSessionInfoPtr mOverrideUserSessionInfo;
        typedef eastl::vector<MatchmakingScenarioSession*> MatchmakingScenarioSessionList;  
        MatchmakingScenarioSessionList mOwnedSessions;
        MatchmakingScenarioManager& mManager;
        MatchmakingScenarioVariantConfigInfoRef mConfigInfoRef;

        NotifyMatchmakingFinished mFailureResults;
        NotifyMatchmakingScenarioPseudoSuccess mPseudoResults;
        NotifyMatchmakingSessionConnectionValidated mSuccessResults;

        bool mIsStartingSessions;
        bool mIsCancellingSessions;
        bool mShouldCancelSessions;
        bool mScenarioHasResults;
        bool mIsScenarioFinished;

        eastl::string mTrackingTag;
        TimeValue mTotalDurationOverride;

        NON_COPYABLE(MatchmakingScenario);

        MatchmakingScenarioDataPtr mScenarioData;
    };

    class MatchmakingScenarioSession
    {
    public:
        MatchmakingScenarioSession(const MatchmakingSubSessionConfigInfo& config, MatchmakingScenario& owner, MatchmakingScenarioSessionDataPtr sessionData);
        ~MatchmakingScenarioSession();

        BlazeRpcError start(StartMatchmakingScenarioResponse& response, MatchmakingCriteriaError& critErr, const InetAddress& callerNetworkAddress,
                            StartMatchmakingInternalRequest& masterRequest, bool hasBadReputation, uint32_t sessionSeq, CallerProperties& callerSourceProperties);
        void finish();
        BlazeRpcError cancel();

        MatchmakingScenarioSessionDataPtr getSessionData() const { return mSessionData; }
        bool isFinished() const { return (mSessionData->getSessionState() == MatchmakingScenarioSessionData::FINISHED); }

        void setSessionId(MatchmakingSessionId id) { return mSessionData->setSessionId(id); }
        MatchmakingSessionId getSessionId() const { return mSessionData->getSessionId(); }
        GameManagerSlaveImpl& getComponent() { return mScenarioOwner.getComponent(); }
        Blaze::Matchmaker::MatchmakerSlave* getMatchmakerSlave() { return mScenarioOwner.getMatchmakerSlave(); }
        const SubSessionName& getSessionName() const;

        const StartMatchmakingScenarioRequest& getScenarioRequest() const { return mScenarioOwner.mScenarioData->getScenarioRequest(); }
        const OverrideUserSessionInfoPtr getOverrideUserSessionInfo() const 
            { return mScenarioOwner.mOverrideUserSessionInfo; }
        MatchmakingScenario& getScenario() { return mScenarioOwner; }
        bool getIsPackerSession() const;

    private:

        MatchmakingScenario& mScenarioOwner;
        const MatchmakingSubSessionConfigInfo& mConfigInfo;

        NON_COPYABLE(MatchmakingScenarioSession);

        MatchmakingScenarioSessionDataPtr mSessionData;

    };


    class MatchmakingScenarioConfigInfo :
        public eastl::safe_object
    {
    public:
        MatchmakingScenarioConfigInfo(const ScenarioName& scenarioName, const MatchmakingScenarioManager* manager);
        ~MatchmakingScenarioConfigInfo();

        void deleteIfUnreferenced(bool clearRefs = false);
        void clearScenarioVariants();

        bool configure(const ScenariosConfig& config);
        
        // This class holds things like the timers and other values:
        ScenarioName mScenarioName;
        TimeValue mTotalDuration;
        int64_t mConfigurationTimestamp;

        const ScenariosConfig* mOverallConfig;
        const ScenarioConfig* mScenarioConfig;

        // variants map        
        ScenarioVariantsMap mScenarioVariantsMap;

        UserVariantsMap mUserByVariantMap;

        const MatchmakingScenarioManager* mScenarioManager;

        NON_COPYABLE(MatchmakingScenarioConfigInfo);
    };

    class MatchmakingScenarioVariantConfigInfo :
        public eastl::safe_object
    {
    public:
        MatchmakingScenarioVariantConfigInfo(const ScenarioVariantName& variant, const MatchmakingScenarioManager* manager, const MatchmakingScenarioConfigInfo* owner);
        ~MatchmakingScenarioVariantConfigInfo();

        void deleteIfUnreferenced(bool clearRefs = false);
        void clearSessionConfigs();

        bool configure(const ScenariosConfig& config);

        struct VariantCriteriaCheckInfo
        {
            // Set by Manager:
            UserSessionInfo* mHostUserInfo;
            ScenarioVariantName* mRecoGroup;
            StartMatchmakingScenarioRequest* mRequest;
            ClientPlatformType mPlatform;

            // Set by Variant:
            bool mResolveError;
            const MatchmakingScenarioVariantConfigInfo* mVariantInfo;
        };

        bool createCriteriaExpression();
        bool checkCriteria(VariantCriteriaCheckInfo& info) const;
        static void resolveCriteriaVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val);
        static void resolveCriteriaValueType(const char8_t* nameSpace, const char8_t* name, void* context, Blaze::ExpressionValueType& type);
        static void resolveCriteriaFunctions(char8_t* name, class Blaze::ExpressionArgumentList& args, void* context, Blaze::Expression*& outNewExpression);

        BlazeRpcError applyGlobalAttributes(const ScenarioAttributes& attributes, const PropertyNameMap& clientProperties, StartMatchmakingRequest& outRequest, MatchmakingCriteriaError &outErr) const;

        // Gathers properties used by Scenarios/CG Templates (Just the globals)
        void getGlobalScenarioPropertiesUsed(PropertyNameList& outProperties) const;
        void getGlobalFilterPropertiesUsed(PropertyNameList& outProperties) const;

        const ScenarioName& getScenarioName() const { return mOwner->mScenarioName; }
        EA::TDF::TimeValue getTotalDurationFromConfig() const;

        ScenarioVariantName mVariant;
        EA::TDF::TdfHashValue mScenarioHash;
        ScenarioVersion mVersion;

        bool mIsSubVariant;
        ScenarioVariantNameList mSubVariants; // Variants that use this one as a base (and have criteria that chooses them)
        Expression* mCriteriaExpression;

        const MatchmakingScenarioConfigInfo* mOwner;
        const ScenarioVariantConfig* mScenarioVariantConfig;

        typedef eastl::list<MatchmakingSubSessionConfigInfo*> SessionConfigList;
        SessionConfigList mScenarioSessionConfigList;
        const ScenariosConfig* mOverallConfig;
        const MatchmakingScenarioManager* mScenarioManager;
        
        MatchmakingSessionMode mCombinedSessionMode; // Combined mode, based on the modes used by all subsessions

        NON_COPYABLE(MatchmakingScenarioVariantConfigInfo);
    };

    class MatchmakingSubSessionConfigInfo
    {
    public:
        MatchmakingSubSessionConfigInfo(const SubSessionName& subSessionName,
                                             const MatchmakingScenarioVariantConfigInfo* owner);
        ~MatchmakingSubSessionConfigInfo() {};

        bool configure(const ScenariosConfig& config);
        bool addGlobalAndScenarioRules(const ScenariosConfig& config);
        bool addRule(const ScenarioRuleAttributes& ruleAttributes);

        BlazeRpcError createStartMatchmakingRequest(const ScenarioAttributes& attributes, const PropertyNameMap& clientProperties, StartMatchmakingRequest& outRequest, StartMatchmakingInternalRequest& masterRequest, MatchmakingCriteriaError &outErr) const;

        const ScenarioName& getScenarioName() const { return mOwner->getScenarioName(); }

        // Gathers properties used by Scenarios/CG Templates
        void getScenarioPropertiesUsed(PropertyNameList& outProperties) const;
        void getFilterPropertiesUsed(PropertyNameList& outProperties) const;
        void getPackerPropertiesUsed(PropertyNameList& outProperties) const;

        SubSessionName mSubSessionName;
        const MatchmakingScenarioVariantConfigInfo* mOwner;
        
        // Pre-calculated matchmaking request.
        StartMatchmakingRequest mRequest;

        const ScenariosConfig* mOverallConfig;
        const SubSessionConfig* mSubSessionConfig;             
        ScenarioAttributeMapping mScenarioAttributeMapping;

        NON_COPYABLE(MatchmakingSubSessionConfigInfo);
    };

} // namespace GameManager
} // namespace Blaze


#endif // BLAZE_MATCHMAKING_SCENARIO
