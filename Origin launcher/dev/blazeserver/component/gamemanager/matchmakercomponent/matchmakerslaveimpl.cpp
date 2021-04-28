/*! ************************************************************************************************/
/*!
    \file matchmakerslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersession.h"

#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/rpc/gamemanagermaster.h"

#include "component/gamemanager/externalsessions/externalsessionutil.h" // for mExternalMatchmakingSessionServices
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h" // for lookupExternalUserByUserIdentification() in externalUserJoinInfosToUserSessionIdList()
#include "gamemanager/pinutil/pinutil.h" // for mPinService
#include "gamemanager/matchmaker/diagnostics/diagnosticstallyutils.h"

namespace Blaze
{
namespace Matchmaker
{
    // static factory MatchmakerSlave
    MatchmakerSlave* MatchmakerSlave::createImpl()
    {
        return BLAZE_NEW_NAMED("MatchmakerSlaveImpl") MatchmakerSlaveImpl();
    }

    MatchmakerSlaveImpl::MatchmakerSlaveImpl()
        : MatchmakerSlaveStub(),
        mScenarioManager(nullptr),
        mMatchmaker(nullptr),
        mGameManagerMaster(nullptr)
    {
        mMatchmaker = BLAZE_NEW GameManager::Matchmaker::Matchmaker(*this);
    }

    MatchmakerSlaveImpl::~MatchmakerSlaveImpl()
    {
        delete mMatchmaker;
    }

    bool MatchmakerSlaveImpl::onValidatePreconfig(MatchmakerSlavePreconfig& config, const MatchmakerSlavePreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        GameManager::GameManagerServerPreconfig preconfig;
        config.getRuleAttributeMap().copyInto(preconfig.getRuleAttributeMap());

        if (!mScenarioManager.validatePreconfig(preconfig))
        {
            ERR_LOG("[MatchmakerSlaveImpl].onValidatePreconfig: Configuration problem while validating preconfig for ScenarioManager.");
            return false;
        }

        return true;
    }

    bool MatchmakerSlaveImpl::onPreconfigure()
    {
        TRACE_LOG("[MatchmakerSlaveImpl] Preconfiguring matchmaker slave component.");

        const MatchmakerSlavePreconfig& preconfigTdf = getPreconfig();

        GameManager::GameManagerServerPreconfig preconfig;
        preconfigTdf.getRuleAttributeMap().copyInto(preconfig.getRuleAttributeMap());

        // preconfig here because we're still loading the Generics
        if (!mScenarioManager.preconfigure(preconfig))
        {
            ERR_LOG("[MatchmakerSlaveImpl].onPreconfigure: Configuration problem while preconfiguring the ScenarioManager.");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief init config file parse for MatchmakerSlave
    ***************************************************************************************************/
    bool MatchmakerSlaveImpl::onConfigure()
    {

        //Setup unique id management for game external session updates
        if (gUniqueIdManager->reserveIdType(GameManager::GameManagerSlave::COMPONENT_ID, GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION) != ERR_OK)
        {
            ERR_LOG("[MatchmakerSlaveImpl].onConfigure: Failed reserving component id " << 
                GameManager::GameManagerSlave::COMPONENT_ID << ", idType " << GameManager::GAMEMANAGER_IDTYPE_EXTERNALSESSION << " with unique id manager");
            return false;
        }

        // configure matchmaker
        if (!mMatchmaker->configure(getConfig()))
        {
            return false; // fail blazeServer start (err already logged)
        }

         bool success = GameManager::ExternalSessionUtil::createExternalSessionServices(mMatchmakingExternalSessionUtilMgr, getConfig().getMatchmakerSettings().getExternalSessions(),
             getConfig().getMatchmakerSettings().getPlayerReservationTimeout(), UINT16_MAX,
             &getConfig().getGameSession().getTeamNameByTeamIdMap());
        if (!success)
        {
            return false;  // (err already logged)
        }

        success = GameManager::ExternalSessionUtil::createExternalSessionServices(mGameSessionExternalSessionUtilMgr, getConfig().getGameSession().getExternalSessions(),
            getConfig().getGameSession().getPlayerReservationTimeout(), (uint16_t)getConfig().getMatchmakerSettings().getGameSizeRange().getMax(),
            &getConfig().getGameSession().getTeamNameByTeamIdMap());
        if (!success)
        {
            return false;   // (err already logged)
        }

        // build out the estimated time to match data
        initializeTtmEstimates();

        return true;
    }

    void MatchmakerSlaveImpl::initializeTtmEstimates()
    {
        mTimeToMatchEstimator.initializeTtmEstimates(getConfig().getScenariosConfig());
    }

    bool MatchmakerSlaveImpl::onPrepareForReconfigure(const MatchmakerConfig& newConfig)
    {
        if (!mMatchmakingExternalSessionUtilMgr.onPrepareForReconfigure(newConfig.getMatchmakerSettings().getExternalSessions()) ||
            !mGameSessionExternalSessionUtilMgr.onPrepareForReconfigure(newConfig.getGameSession().getExternalSessions()))
        {
            return false;
        }

        // All of the matchmaking shutdown steps (stopping sessions, reseting the RETE trees, etc) currently happen in onReconfigure
        return true;
    }

    bool MatchmakerSlaveImpl::onReconfigure()
    {
        // reconfigure matchmaker
        if (!mMatchmaker->reconfigure(getConfig()))
        {
            ERR_LOG("[MatchmakerSlaveImpl].onReconfigure: Problem parsing " << (GameManager::Matchmaker::MatchmakingConfig::MATCHMAKER_SETTINGS_KEY) 
                << " config file, throwing away changes and using old values.");
            return true; // RECONFIGURE SHOULD NEVER RETURN FALSE.  We have logged an error, but we don't want the blaze server to shutdown.
        }

        mMatchmakingExternalSessionUtilMgr.onReconfigure(getConfig().getMatchmakerSettings().getExternalSessions(),
            getConfig().getMatchmakerSettings().getPlayerReservationTimeout());

        mGameSessionExternalSessionUtilMgr.onReconfigure(getConfig().getGameSession().getExternalSessions(),
                getConfig().getGameSession().getPlayerReservationTimeout());

        // clear the old and build out the new estimated time to match data
        initializeTtmEstimates();

        TRACE_LOG("[MatchmakerSlaveImpl].onReconfigure: reconfiguration of MatchmakerSlave complete.");
        return true;
    }

       /*! ************************************************************************************************/
    /*! \brief all components are up, so it's safe to lookup/cache/subscribe to them.
    ***************************************************************************************************/
    bool MatchmakerSlaveImpl::onResolve()
    {
        BlazeRpcError rc;
        // Need to ensure user manager is active and replicating. Otherwise when gamemanager initializes replication
        // after resolving, the slave will spew errors due to no user sessions.
        rc = gUserSessionManager->waitForState(ACTIVE, "MatchmakerSlaveImpl::onResolve");
        if (rc != ERR_OK)
        {
            FATAL_LOG("[MatchmakerSlave] Unable to resolve; user session manager not active.");
            return false;
        }
        gUserSessionManager->addSubscriber(*this);
        gUserSessionManager->getQosConfig().addSubscriber(*this);  // Subscribe for ping site updates, to keep TTM up to date

        if(!mGameManagerMaster)
        {
            BlazeRpcError error;
            mGameManagerMaster = static_cast<Blaze::GameManager::GameManagerMaster*>(gController->getComponent(Blaze::GameManager::GameManagerMaster::COMPONENT_ID, false, true, &error));
            if (mGameManagerMaster == nullptr)
            {
                FATAL_LOG("[MatchmakerSlave].onResolve() - Unable to resolve GameManagerMaster component.");
                return false;
            }
        }

        if (!mMatchmaker->hostingComponentResolved())
        {
            FATAL_LOG("[MatchmakerSlave].onResolve() - Unable to setup matchmaker: onHostComponentResolved() failed.");
            return false;
        }

        gController->registerDrainStatusCheckHandler(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_INFO.name, *this);

        return true;
    }

    void MatchmakerSlaveImpl::onPingSiteMapChanges(const PingSiteInfoByAliasMap& pingSiteMap)
    {
        // Reinit the TTM values when a pingsite change occurs, to ensure that we keep valid values:
        initializeTtmEstimates();
    }

    /*! ************************************************************************************************/
    /*! \brief components are shut down in a specific (safe) order, so unregister from maps/session manager here
    ***************************************************************************************************/
    void MatchmakerSlaveImpl::onShutdown()
    {
        mMatchmaker->onShutdown();
        gUserSessionManager->removeSubscriber(*this);
        gUserSessionManager->getQosConfig().removeSubscriber(*this);
        gController->deregisterDrainStatusCheckHandler(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_INFO.name);
    }

    /*! ************************************************************************************************/
    /*! \brief validate configuration
    ***************************************************************************************************/
    bool MatchmakerSlaveImpl::onValidateConfig(MatchmakerConfig& config, const MatchmakerConfig* referenceConfigPtr,
        ConfigureValidationErrors& validationErrors) const
    {
        eastl::string msg;
        if ((config.getMatchmakerSettings().getGameSizeRange().getMax() == 0))
        {
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            msg.sprintf("[MatchmakerSlaveImpl:%p].onValidateConfig: Game Size range max was zero", this);
            str.set(msg.c_str());
            return false;
        }
        if (config.getMatchmakerSettings().getGameSizeRange().getMax() < config.getMatchmakerSettings().getGameSizeRange().getMin())
        {
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            msg.sprintf("[MatchmakerSlaveImpl:%p].onValidateConfig: Game Size range max '%u' cannot be less the Game Size range min '%u'",
                config.getMatchmakerSettings().getGameSizeRange().getMax(), config.getMatchmakerSettings().getGameSizeRange().getMax());
            str.set(msg.c_str());
            return false;
        }

        if (!validateConfiguredGameSizeRules(config.getMatchmakerSettings(), msg))
        {
            return false;
        }

        return mMatchmaker->validateConfig(config, referenceConfigPtr, validationErrors);
    }

    /*! ************************************************************************************************/
    /*! \brief validate the set of configured game size related rules.
        \param[in] config the matchmaking config to validate for.
    *************************************************************************************************/
    bool MatchmakerSlaveImpl::validateConfiguredGameSizeRules(const Blaze::GameManager::MatchmakingServerConfig& config, eastl::string &err) const
    {
        const Blaze::GameManager::TotalPlayerSlotsRuleConfig& totSlotsRuleConfig = config.getRules().getPredefined_TotalPlayerSlotsRule();
        const Blaze::GameManager::PlayerCountRuleConfig& playerCountRuleConfig =  config.getRules().getPredefined_PlayerCountRule();
        const Blaze::GameManager::PlayerSlotUtilizationRuleConfig& playerSlotUtiliznRuleConfig =  config.getRules().getPredefined_PlayerSlotUtilizationRule();

        // total slots rule must be enabled (for create mode matchmaking to be able to finalize game sizes)
        if (!totSlotsRuleConfig.isSet())
        {
            err.sprintf("(for create mode matchmaking)  '%s' is missing from configuration.",
                totSlotsRuleConfig.getClassName());
            return false;
        }

        // want at least one game size rule to be enabled for limiting results.
        if (!playerCountRuleConfig.isSet() && !playerSlotUtiliznRuleConfig.isSet() && !totSlotsRuleConfig.isSet())
        {
            err.sprintf("Must enable at least at least one of '%s', '%s' or '%s' but these were missing.",
                playerCountRuleConfig.getClassName(), playerSlotUtiliznRuleConfig.getClassName(), totSlotsRuleConfig.getClassName());
            return false;
        }
        
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief when a user logs out of blaze, we kick them out of any matchmaking sessions they're still in.
        \param[in]  user    The UserSession for the leaving/disconnected user
    *************************************************************************************************/
    void MatchmakerSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
    {
        TRACE_LOG("[MatchmakerSlaveImpl] Player(" << userSession.getUserId() << ") logging out");
        
        // remove any active matchmaking sessions for the leaving player
        mMatchmaker->destroySessionForUserSessionId(userSession.getUserSessionId());
    }

        /*! ************************************************************************************************/
    /*! \brief Framework's entry point to collect healthcheck status for the MatchmakerSlave component
    ***************************************************************************************************/
    void MatchmakerSlaveImpl::getStatusInfo(ComponentStatus& status) const
    { 
        //ask match maker to populate its own metric
        mMatchmaker->getStatusInfo(status);
    }

    void MatchmakerSlaveImpl::onSlaveSessionAdded(SlaveSession& session)
    {
        if (mMatchmaker != nullptr)
        {
            mMatchmaker->clearLastSentMatchmakingStatus();
        }
    }

    bool MatchmakerSlaveImpl::getEvaluateGameProtocolVersionString() const 
    { 
        return getConfig().getGameSession().getEvaluateGameProtocolVersionString(); 
    }

    /*! ************************************************************************************************/
    /*! \brief return a subset of the matchmaking config file to the client (so they can have a data driven UI)
    ***************************************************************************************************/
    GetMatchmakingConfigError::Error MatchmakerSlaveImpl::processGetMatchmakingConfig(
        Blaze::GameManager::GetMatchmakingConfigResponse &response, const Message *message)
    {
        return mMatchmaker->setMatchmakingConfigResponse(&response);
    }

    /*! ************************************************************************************************/
    /*! \brief return the current number of active matchmaking sessions (for census data)
    ***************************************************************************************************/
    GetMatchmakingInstanceStatusError::Error MatchmakerSlaveImpl::processGetMatchmakingInstanceStatus(
        Blaze::GameManager::MatchmakingInstanceStatusResponse &response, const Message *message)
    {
        response.setNumOfMatchmakingSessions(response.getNumOfMatchmakingSessions() + mMatchmaker->getNumOfCreateGameMatchmakingSessions()
            + mMatchmaker->getNumOfFindGameMatchmakingSessions());
        response.setNumOfMatchmakingUsers(response.getNumOfMatchmakingUsers() + (uint32_t)mMatchmaker->getMatchmakerMetrics().mGaugeMatchmakingUsersInSessions.get());
        mTimeToMatchEstimator.populateMatchmakingCensusData(response.getGlobalTimeToMatchEstimateData(), response.getScenarioTimeToMatchEstimateData());
        mMatchmaker->getMatchmakingInstanceData(response.getPlayerMatchmakingRateByScenarioPingsiteGroup(), response.getScenarioMatchmakingData());
    
        return GetMatchmakingInstanceStatusError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Make the local user always attempt to match into the specified game.
    ***************************************************************************************************/
    MatchmakingDedicatedServerOverrideError::Error MatchmakerSlaveImpl::processMatchmakingDedicatedServerOverride(
      const Blaze::GameManager::MatchmakingDedicatedServerOverrideRequest &request, const Message *message)
    {
        mMatchmaker->dedicatedServerOverride(request.getPlayerId(), request.getGameId());
        return MatchmakingDedicatedServerOverrideError::ERR_OK;
    }

    GetMatchmakingDedicatedServerOverridesError::Error MatchmakerSlaveImpl::processGetMatchmakingDedicatedServerOverrides(
        Blaze::GameManager::GetMatchmakingDedicatedServerOverrideResponse &response, const Message* message)
    {
        mMatchmaker->getDedicatedServerOverrides().copyInto(response.getPlayerToGameMap());
        return GetMatchmakingDedicatedServerOverridesError::ERR_OK;
    }

    MatchmakingFillServersOverrideError::Error MatchmakerSlaveImpl::processMatchmakingFillServersOverride(
        const Blaze::GameManager::MatchmakingFillServersOverrideList &request, const Message* message)
    {
        mMatchmaker->fillServersOverride(request.getGameIdList());
        return MatchmakingFillServersOverrideError::ERR_OK;
    }

    GetMatchmakingFillServersOverrideError::Error MatchmakerSlaveImpl::processGetMatchmakingFillServersOverride(
        Blaze::GameManager::MatchmakingFillServersOverrideList &response, const Message* message)
    {
        mMatchmaker->getFillServersOverride().copyInto(response.getGameIdList());
        return GetMatchmakingFillServersOverrideError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief called after awaiting MM session members' QoS measurements, to complete QoS validation for the potential match
    ***************************************************************************************************/
    GameSessionConnectionCompleteError::Error MatchmakerSlaveImpl::processGameSessionConnectionComplete(
        const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response, const Message* message)
    {
        MatchmakingJobMap::iterator jobIter = mMatchmakingJobMap.find(request.getMatchmakingSessionId());
        if (jobIter == mMatchmakingJobMap.end())
        {
            return GameSessionConnectionCompleteError::MATCHMAKER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID;
        }

        if (jobIter->second == nullptr)
        {
            return GameSessionConnectionCompleteError::ERR_SYSTEM;
        }

        return GameSessionConnectionCompleteError::commandErrorFromBlazeError(jobIter->second->onMatchQosDataAvailable(request, response));
        
    }

    /// @deprecated [metrics-refactor] getScenarioMetrics() can be removed when getMatchmakingMetrics RPC is removed
    /*! ************************************************************************************************/
    /*!
        \brief Fetch Scenario's Matchmaking Metrics

            If the Scenario/Variant/Version doesn't already exist in the map, initialize it.

        \param[in]    tagList            - scenario information to fetch metric maps for (tags are SPECIFICALLY-defined and SPECIFICALLY-ordered)
        \param[in]    metricsMap         - Matchmaking Metrics Map to fetch from (can be modified)
        \param[in]    nonScenarioMetrics - use/return this metrics object if the information indicates non-scenario
        \param[out]   scenarioMetrics    - pointer to scenario metrics for requested scenario (pointer to nonScenarioMetrics if non-scenario)
    *************************************************************************************************/
    void getScenarioMetrics(const Metrics::TagPairList& tagList, GameManager::ScenarioMatchmakingMetricsMap& metricsMap, GameManager::MatchmakingMetrics* nonScenarioMetrics, GameManager::MatchmakingMetricsPtr& scenarioMetrics)
    {
        ASSERT_COND_LOG(tagList.size() >= 3, "getScenarioMetrics: tagList does not contain enough scenario information");
        const char8_t* scenarioName = tagList[0].second.c_str();
        if (blaze_strcmp(scenarioName, Metrics::Tag::NON_SCENARIO_NAME) == 0)
        {
            scenarioMetrics = nonScenarioMetrics;
            return;
        }
        const char8_t* scenarioVariant = tagList[1].second.c_str();
        uint32_t scenarioVersion = 0;
        blaze_str2int(tagList[2].second.c_str(), &scenarioVersion);

        GameManager::ScenarioMatchmakingMetricsMap::iterator itr = metricsMap.find(scenarioName);
        if (itr == metricsMap.end())
        {
            // No Scenario Found, initialize Scenario/Variant/Version
            GameManager::ScenarioVariantMatchmakingMetricsMap* scenarioVariantMatchmakingMetricsMap = metricsMap.allocate_element();
            GameManager::ScenarioVersionMatchmakingMetricsMap* scenarioVersionMatchmakingMetricsMap = scenarioVariantMatchmakingMetricsMap->allocate_element();
            GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = scenarioVersionMatchmakingMetricsMap->allocate_element();
            scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();

            (*scenarioVersionMatchmakingMetricsMap)[scenarioVersion] = scenarioMatchmakingMetrics;
            (*scenarioVariantMatchmakingMetricsMap)[scenarioVariant] = scenarioVersionMatchmakingMetricsMap;
            metricsMap[scenarioName] = scenarioVariantMatchmakingMetricsMap;
        }
        else
        {
            GameManager::ScenarioVariantMatchmakingMetricsMap::iterator sceVarMatchmakingMetricsMap = itr->second->find(scenarioVariant);
            if (sceVarMatchmakingMetricsMap == itr->second->end())
            {
                // Variant for Scenario Not Found, initialize Variant/Version
                GameManager::ScenarioVersionMatchmakingMetricsMap* scenarioVersionMatchmakingMetricsMap = itr->second->allocate_element();
                GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = scenarioVersionMatchmakingMetricsMap->allocate_element();
                scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();

                (*scenarioVersionMatchmakingMetricsMap)[scenarioVersion] = scenarioMatchmakingMetrics;
                (*itr->second)[scenarioVariant] = scenarioVersionMatchmakingMetricsMap;
            }
            else
            {
                GameManager::ScenarioVersionMatchmakingMetricsMap::iterator sceVerMatchmakingMetricsMapItr = sceVarMatchmakingMetricsMap->second->find(scenarioVersion);
                if (sceVerMatchmakingMetricsMapItr == sceVarMatchmakingMetricsMap->second->end())
                {
                    // Version for Scenario's Variant Not Found, initialize Version
                    GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = sceVarMatchmakingMetricsMap->second->allocate_element();
                    scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();

                    (*sceVarMatchmakingMetricsMap->second)[scenarioVersion] = scenarioMatchmakingMetrics;
                }
                else
                {
                    scenarioMetrics = &sceVerMatchmakingMetricsMapItr->second->getMetrics();
                }
            }
        }
    }

    /// @deprecated [metrics-refactor] getScenarioSubSessionMetrics() can be removed when getMatchmakingMetrics RPC is removed
    /*! ************************************************************************************************/
    /*!
        \brief Fetch Scenario and SubSession's Matchmaking Metrics

            If the Scenario/Variant/Version/SubSession doesn't already exist in the map, initialize it.

        \param[in]    tagList            - scenario information to fetch metric maps for (tags are SPECIFICALLY-defined and SPECIFICALLY-ordered)
        \param[in]    metricsMap         - Matchmaking Metrics Map to fetch from (can be modified)
        \param[in]    nonScenarioMetrics - use/return this metrics object if the information indicates non-scenario
        \param[out]   scenarioMetrics    - pointer to scenario metrics for requested scenario (pointer to nonScenarioMetrics if non-scenario)
        \param[out]   subSessionMetrics  - pointer to sub session metrics for requested scenario
    *************************************************************************************************/
    void getScenarioSubSessionMetrics(const Metrics::TagPairList& tagList, GameManager::ScenarioMatchmakingMetricsMap& metricsMap, GameManager::MatchmakingMetrics* nonScenarioMetrics, GameManager::MatchmakingMetricsPtr& scenarioMetrics, GameManager::MatchmakingMetricsPtr& subSessionMetrics)
    {
        ASSERT_COND_LOG(tagList.size() >= 4, "getScenarioSubSessionMetrics: tagList does not contain enough scenario information");
        const char8_t* scenarioName = tagList[0].second.c_str();
        if (blaze_strcmp(scenarioName, Metrics::Tag::NON_SCENARIO_NAME) == 0)
        {
            scenarioMetrics = nonScenarioMetrics;
            return;
        }
        const char8_t* scenarioVariant = tagList[1].second.c_str();
        uint32_t scenarioVersion = 0;
        blaze_str2int(tagList[2].second.c_str(), &scenarioVersion);
        const char8_t* subSessionName = tagList[3].second.c_str();

        GameManager::ScenarioMatchmakingMetricsMap::iterator itr = metricsMap.find(scenarioName);
        if (itr == metricsMap.end())
        {
            // No Scenario Found, initialize Scenario/Variant/Version/SubSession
            GameManager::ScenarioVariantMatchmakingMetricsMap* scenarioVariantMatchmakingMetricsMap = metricsMap.allocate_element();
            GameManager::ScenarioVersionMatchmakingMetricsMap* scenarioVersionMatchmakingMetricsMap = scenarioVariantMatchmakingMetricsMap->allocate_element();
            GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = scenarioVersionMatchmakingMetricsMap->allocate_element();
            scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();
            subSessionMetrics = scenarioMatchmakingMetrics->getSubSessionMetrics().allocate_element();
            //subSessionMetrics->setMaxFitScore(maxFitScore);

            scenarioMatchmakingMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
            (*scenarioVersionMatchmakingMetricsMap)[scenarioVersion] = scenarioMatchmakingMetrics;
            (*scenarioVariantMatchmakingMetricsMap)[scenarioVariant] = scenarioVersionMatchmakingMetricsMap;
            metricsMap[scenarioName] = scenarioVariantMatchmakingMetricsMap;
        }
        else
        {
            GameManager::ScenarioVariantMatchmakingMetricsMap::iterator sceVarMatchmakingMetricsMap = itr->second->find(scenarioVariant);
            if (sceVarMatchmakingMetricsMap == itr->second->end())
            {
                // Variant for Scenario Not Found, initialize Variant/Version/SubSession
                GameManager::ScenarioVersionMatchmakingMetricsMap* scenarioVersionMatchmakingMetricsMap = itr->second->allocate_element();
                GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = scenarioVersionMatchmakingMetricsMap->allocate_element();
                scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();
                subSessionMetrics = scenarioMatchmakingMetrics->getSubSessionMetrics().allocate_element();
                //subSessionMetrics->setMaxFitScore(maxFitScore);

                scenarioMatchmakingMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                (*scenarioVersionMatchmakingMetricsMap)[scenarioVersion] = scenarioMatchmakingMetrics;
                (*itr->second)[scenarioVariant] = scenarioVersionMatchmakingMetricsMap;
            }
            else
            {
                GameManager::ScenarioVersionMatchmakingMetricsMap::iterator sceVerMatchmakingMetricsMapItr = sceVarMatchmakingMetricsMap->second->find(scenarioVersion);
                if (sceVerMatchmakingMetricsMapItr == sceVarMatchmakingMetricsMap->second->end())
                {
                    // Version for Scenario's Variant Not Found, initialize Version/SubSession
                    GameManager::ScenarioMatchmakingMetrics* scenarioMatchmakingMetrics = sceVarMatchmakingMetricsMap->second->allocate_element();
                    scenarioMetrics = &scenarioMatchmakingMetrics->getMetrics();
                    subSessionMetrics = scenarioMatchmakingMetrics->getSubSessionMetrics().allocate_element();
                    //subSessionMetrics->setMaxFitScore(maxFitScore);

                    scenarioMatchmakingMetrics->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                    (*sceVarMatchmakingMetricsMap->second)[scenarioVersion] = scenarioMatchmakingMetrics;
                }
                else
                {
                    scenarioMetrics = &sceVerMatchmakingMetricsMapItr->second->getMetrics();
                    GameManager::SubSessionMetricsMap::iterator subSessionMetricsMapItr = sceVerMatchmakingMetricsMapItr->second->getSubSessionMetrics().find(subSessionName);
                    if (subSessionMetricsMapItr == sceVerMatchmakingMetricsMapItr->second->getSubSessionMetrics().end())
                    {
                        // SubSession for Scenario/Variant/Version Not Found, initialize SubSession
                        subSessionMetrics = sceVerMatchmakingMetricsMapItr->second->getSubSessionMetrics().allocate_element();
                        sceVerMatchmakingMetricsMapItr->second->getSubSessionMetrics()[subSessionName] = subSessionMetrics;
                        //subSessionMetrics->setMaxFitScore(maxFitScore);
                    }
                    else
                    {
                        subSessionMetrics = subSessionMetricsMapItr->second;
                    }
                }
            }
        }
    }

    /// @deprecated [metrics-refactor] getMatchmakingRuleDiagnostics() can be removed when getMatchmakingMetrics RPC is removed
    /*! ************************************************************************************************/
    /*!
        \brief Fetch Rule Diagnostics from Matchmaking Metrics

            If the Rule/Category/Value doesn't already exist in the rule diagnostics map, initialize it.

        \param[in]    tagList           - rule and scenario information to fetch rule diagnostics for (tags are SPECIFICALLY-defined and SPECIFICALLY-ordered)
        \param[in]    metrics           - Matchmaking Metrics to fetch rule diagnostics from (can be modified)
        \param[out]   subDiagnostics    - pointer to rule diagnostics for requested rule
    *************************************************************************************************/
    void getMatchmakingRuleDiagnostics(const Metrics::TagPairList& tagList, GameManager::MatchmakingMetrics& metrics, GameManager::MatchmakingRuleDiagnosticCountsPtr& subDiagnostics)
    {
        ASSERT_COND_LOG(tagList.size() >= 7, "getMatchmakingRuleDiagnostics: tagList does not contain enough rule information");
        const char8_t* ruleName = tagList[4].second.c_str();
        const char8_t* ruleCategory = tagList[5].second.c_str();
        const char8_t* ruleCategoryValue = tagList[6].second.c_str();
        GameManager::DiagnosticsByRuleMap& diagnosticsMap = metrics.getDiagnostics().getRuleDiagnostics();
        Blaze::GameManager::Matchmaker::DiagnosticsByRuleCategoryMapPtr ruleDiagnostics =
            Blaze::GameManager::Matchmaker::DiagnosticsTallyUtils::getOrAddRuleDiagnostic(diagnosticsMap, ruleName);
        if (ruleDiagnostics != nullptr)
        {
            subDiagnostics = Blaze::GameManager::Matchmaker::DiagnosticsTallyUtils::getOrAddRuleSubDiagnostic(*ruleDiagnostics, ruleCategory, ruleCategoryValue);
        }
    }

    /// @deprecated [metrics-refactor] getMatchmakingMetrics RPC is polled for Graphite (which, in turn, is used by GOSCC) and can be removed when no longer used
    GetMatchmakingMetricsError::Error MatchmakerSlaveImpl::processGetMatchmakingMetrics(Blaze::GameManager::GetMatchmakingMetricsResponse &response, const Message* message)
    {
        gController->tickDeprecatedRPCMetric("matchmaker", "getMatchmakingMetrics", message);

        GameManager::ScenarioMatchmakingMetricsMap& scenarioMmMetricsMap = response.getScenarioMatchmakingMetricsMap();
        GameManager::MatchmakingMetrics& nonScenarioMmMetrics = response.getNonScenarioMetrics();

        // metrics that exist in both ScenarioMetrics and SubsessionMetrics means that the scenario metric is 'not necessarily' an aggregate of its subsession counterpart
        // and we won't bother to do any aggregation when iterating thru the subsession metric (just use the already cached scenario counterpart)

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalRequests.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 4, "mMetrics.mTotalRequests.iterate: tagList does not contain group size information");
            uint32_t groupSize = 0;
            blaze_str2int(tagList[3].second.c_str(), &groupSize);

            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalRequests(scenarioMetrics->getTotalRequests() + (uint32_t)value.getTotal());

                if (groupSize == 1)
                {
                    scenarioMetrics->setTotalNonGroupRequests(scenarioMetrics->getTotalNonGroupRequests() + (uint32_t)value.getTotal());
                }
                else
                {
                    scenarioMetrics->setTotalGroupRequests(scenarioMetrics->getTotalGroupRequests() + (uint32_t)value.getTotal());
                }

                scenarioMetrics->getTotalRequestsBySize()[groupSize] += (uint32_t)value.getTotal();
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalRequests.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            ASSERT_COND_LOG(tagList.size() >= 5, "mSubsessionMetrics.mTotalRequests.iterate: tagList does not contain group size information");
            uint32_t groupSize = 0;
            blaze_str2int(tagList[4].second.c_str(), &groupSize);

            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalRequests(subsessionMetrics->getTotalRequests() + (uint32_t)value.getTotal());

                if (groupSize == 1)
                {
                    subsessionMetrics->setTotalNonGroupRequests(subsessionMetrics->getTotalNonGroupRequests() + (uint32_t)value.getTotal());
                }
                else
                {
                    subsessionMetrics->setTotalGroupRequests(subsessionMetrics->getTotalGroupRequests() + (uint32_t)value.getTotal());
                }

                subsessionMetrics->getTotalRequestsBySize()[groupSize] += (uint32_t)value.getTotal();
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalPlayersInGroupRequests.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalPlayersInGroupRequests((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalPlayersInGroupRequests.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalPlayersInGroupRequests((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalResults.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                GameManager::MatchmakingResult result;
                if (ParseMatchmakingResult(tagList[3].second.c_str(), result))   // Lookup scenario metric using the 4th tag -> (Scenario, Variant, Version, *ResultType*)
                {
                    switch (result)
                    {
                        case GameManager::SUCCESS_CREATED_GAME:
                            scenarioMetrics->setTotalCreatedNewGames((uint32_t)value.getTotal());
                            break;
                        case GameManager::SUCCESS_JOINED_NEW_GAME:
                            scenarioMetrics->setTotalJoinedNewGames((uint32_t)value.getTotal());
                            break;
                        case GameManager::SUCCESS_JOINED_EXISTING_GAME:
                            scenarioMetrics->setTotalJoinedExistingGame((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_TIMED_OUT:
                            scenarioMetrics->setTotalTimeouts((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_CANCELED:
                            scenarioMetrics->setTotalCanceled((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_TERMINATED:
                            scenarioMetrics->setTotalTerminated((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_QOS_VALIDATION_FAILED:
                            scenarioMetrics->setTotalQOSValidationFailure((uint32_t)value.getTotal());
                            break;

                        case GameManager::SESSION_ERROR_GAME_SETUP_FAILED:
                        case GameManager::SUCCESS_PSEUDO_CREATE_GAME:
                        case GameManager::SUCCESS_PSEUDO_FIND_GAME:
                        default:
                            break;
                    }
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalResults.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingResult result;
                if (ParseMatchmakingResult(tagList[4].second.c_str(), result))   // Lookup subssession metric using the 5th tag -> (Scenario, Variant, Version, Subsession, *ResultType*)
                {
                    switch (result)
                    {
                        case GameManager::SUCCESS_CREATED_GAME:
                            subsessionMetrics->setTotalCreatedNewGames((uint32_t)value.getTotal());
                            break;
                        case GameManager::SUCCESS_JOINED_NEW_GAME:
                            subsessionMetrics->setTotalJoinedNewGames((uint32_t)value.getTotal());
                            break;
                        case GameManager::SUCCESS_JOINED_EXISTING_GAME:
                            subsessionMetrics->setTotalJoinedExistingGame((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_TIMED_OUT:
                            subsessionMetrics->setTotalTimeouts((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_CANCELED:
                            subsessionMetrics->setTotalCanceled((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_TERMINATED:
                            subsessionMetrics->setTotalTerminated((uint32_t)value.getTotal());
                            break;
                        case GameManager::SESSION_QOS_VALIDATION_FAILED:
                            subsessionMetrics->setTotalQOSValidationFailure((uint32_t)value.getTotal());
                            break;

                        case GameManager::SESSION_ERROR_GAME_SETUP_FAILED:
                        case GameManager::SUCCESS_PSEUDO_CREATE_GAME:
                        case GameManager::SUCCESS_PSEUDO_FIND_GAME:
                        default:
                            break;
                    }
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalSuccessDurationMs.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalSuccessDurationMs(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalSuccessDurationMs.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalSuccessDurationMs(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalPlayerCountInGame.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalPlayerCountInGame((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalPlayerCountInGame.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalPlayerCountInGame((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalMaxPlayerCapacity.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalMaxPlayerCapacity((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalMaxPlayerCapacity.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalMaxPlayerCapacity((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalGamePercentFullCapacity.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalGamePercentFullCapacity((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalGamePercentFullCapacity.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalGamePercentFullCapacity((uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mTotalFitScore.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setTotalFitScore(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mTotalFitScore.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setTotalFitScore(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mSubsessionMetrics.mMaxFitScore.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics, subsessionMetrics);
            // don't care about max fit score in non-subsession, so let it be always zero there
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->setMaxFitScore(value.get());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mExternalSessionFailuresImpacting.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setExternalSessionFailuresImpacting(scenarioMetrics->getExternalSessionFailuresImpacting() + (uint32_t)value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mMetrics.mExternalSessionFailuresNonImpacting.iterate([&scenarioMmMetricsMap, &nonScenarioMmMetrics](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            getScenarioMetrics(tagList, scenarioMmMetricsMap, &nonScenarioMmMetrics, scenarioMetrics);
            if (scenarioMetrics != nullptr)
            {
                scenarioMetrics->setExternalSessionFailuresNonImpacting(scenarioMetrics->getExternalSessionFailuresNonImpacting() + (uint32_t)value.getTotal());
            }
        });

        // only want diagnostics for subsessions in the response

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mSessions.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->getDiagnostics().setSessions(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mCreateEvaluations.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->getDiagnostics().setCreateEvaluations(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mCreateEvaluationsMatched.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->getDiagnostics().setCreateEvaluationsMatched(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mFindRequestsGamesAvailable.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                subsessionMetrics->getDiagnostics().setFindRequestsGamesAvailable(value.getTotal());
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mSessions.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setSessions(value.getTotal());
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mCreateEvaluations.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setCreateEvaluations(value.getTotal());
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mCreateEvaluationsMatched.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setCreateEvaluationsMatched(value.getTotal());
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mFindRequests.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setFindRequests(value.getTotal());
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mFindRequestsHadGames.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setFindRequestsHadGames(value.getTotal());
                }
            }
        });

        mMatchmaker->getMatchmakingMetrics().mDiagnostics.mRuleDiagnostics.mFindRequestsGamesVisible.iterate([&scenarioMmMetricsMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameManager::MatchmakingMetricsPtr scenarioMetrics = nullptr;
            GameManager::MatchmakingMetricsPtr subsessionMetrics = nullptr;
            getScenarioSubSessionMetrics(tagList, scenarioMmMetricsMap, nullptr, scenarioMetrics, subsessionMetrics);
            if (subsessionMetrics != nullptr)
            {
                GameManager::MatchmakingRuleDiagnosticCountsPtr ruleDiagnostics = nullptr;
                getMatchmakingRuleDiagnostics(tagList, *subsessionMetrics, ruleDiagnostics);
                if (ruleDiagnostics != nullptr)
                {
                    ruleDiagnostics->setFindRequestsGamesVisible(value.getTotal());
                }
            }
        });

        return GetMatchmakingMetricsError::ERR_OK;
    }

    //////////////////////
    // Matchmaking Job
    /////////////////////

    bool MatchmakerSlaveImpl::addMatchmakingJob( MatchmakingJob& job )
    {
        MatchmakingJobMap::const_iterator jobIter = mMatchmakingJobMap.find(job.getOwningMatchmakingSessionId());
        if (jobIter == mMatchmakingJobMap.end())
        {
            mMatchmakingJobMap.insert( eastl::make_pair(job.getOwningMatchmakingSessionId(), &job) );
            TRACE_LOG("[MatchmakerSlaveImpl] adding matchmaking job " << &job << " owned by Session " << job.getOwningMatchmakingSessionId() << ".");
            return true;
        }
        else
        {
            ERR_LOG("MatchmakerSlaveImpl::addMatchmakingJob - job already exists for matchmakingSessionId " << job.getOwningMatchmakingSessionId() << "!");
            return false;
        }
    }

    MatchmakingJob* MatchmakerSlaveImpl::getMatchmakingJob(const Blaze::GameManager::MatchmakingSessionId &mmSessionId) const
    {
        MatchmakingJobMap::const_iterator jobIter = mMatchmakingJobMap.find(mmSessionId);
        if (jobIter != mMatchmakingJobMap.end())
        {
            return jobIter->second;
        }

        return nullptr;
    }

    void MatchmakerSlaveImpl::removeMatchmakingJob(const Blaze::GameManager::MatchmakingSessionId &mmSessionId)
    {
        MatchmakingJobMap::iterator jobIter = mMatchmakingJobMap.find(mmSessionId);
        if(jobIter != mMatchmakingJobMap.end())
        {
            MatchmakingJob *job = jobIter->second;
            TRACE_LOG("[MatchmakerSlaveImpl] removing matchmaking job " << job << " owned by Session " << job->getOwningMatchmakingSessionId() << ".");
            mMatchmakingJobMap.erase(jobIter);
            delete job;
        }
    }

    void MatchmakerSlaveImpl::sendNotifyMatchmakingStatusUpdate(Blaze::GameManager::MatchmakingStatus &status)
    {
        sendNotifyMatchmakingStatusUpdateToAllSlaves(&status);
        mNotificationDispatcher.dispatch<const Blaze::GameManager::MatchmakingStatus&, UserSession*>(&MatchmakerSlaveListener::onNotifyMatchmakingStatusUpdate, status, nullptr);
    }

    /*! \brief joins the specified external id's to the game's external session. Pre: permissions checked as needed. */
    BlazeRpcError MatchmakerSlaveImpl::joinGameExternalSession(const JoinExternalSessionParameters& joinParams, bool requiresSuperUser, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionApiError& apiErr)
    {
        Blaze::GameManager::GameId gameId = joinParams.getExternalSessionCreationInfo().getExternalSessionId();

        //joinParams already contain the ExternalSessionIdentification and we don't need it back so pass a nullptr for ExternalSessionResult.
        BlazeRpcError sessErr = mGameSessionExternalSessionUtilMgr.join(joinParams, nullptr, true, joinInitErr, apiErr);
        if (ERR_OK != sessErr)
        {
            for (auto& platformErr : apiErr.getPlatformErrorMap())
            {
                ERR_LOG("[MatchmakerSlaveImpl].joinGameExternalSession: Failed to join external session " << GameManager::toLogStr(joinParams.getSessionIdentification())
                    << "(" << gameId << "), error (" << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr()) << "), platform (" << ClientPlatformTypeToString(platformErr.first) << ")");
                
                UserSessionIdList sessionIds;
                Blaze::GameManager::userIdentificationListToUserSessionIdList(platformErr.second->getUsers(), sessionIds);
                
                Blaze::GameManager::LeaveGameByGroupMasterRequest leaveRequest;
                
                // leave leader is false as all users being joined/removed are in the session id list for this request
                leaveRequest.setLeaveLeader(false);
                leaveRequest.setGameId(gameId);
                leaveRequest.setPlayerRemovedReason(Blaze::GameManager::PLAYER_JOIN_EXTERNAL_SESSION_FAILED);
                sessionIds.copyInto(leaveRequest.getSessionIdList());

                UserSession::SuperUserPermissionAutoPtr permissionPtr(requiresSuperUser);
                Blaze::GameManager::LeaveGameByGroupMasterResponse leaveResponse;
                BlazeRpcError leaveErr = getGameManagerMaster()->leaveGameByGroupMaster(leaveRequest, leaveResponse);
                if (ERR_OK != leaveErr)
                {

                    ERR_LOG("[MatchmakerSlaveImpl].joinGameExternalSession: Cleanup after failed external session update, failed to leave Blaze game with error (" 
                        << ErrorHelp::getErrorName(leaveErr) << "), platform(" << ClientPlatformTypeToString(platformErr.first) << ")");
                }

                //cleanup game's external session, as needed
                if (GameManager::shouldResyncExternalSessionMembersOnJoinError(platformErr.second->getBlazeRpcErr()))
                {
                    Blaze::GameManager::ResyncExternalSessionMembersRequest resyncRequest;
                    resyncRequest.setGameId(gameId);
                    BlazeRpcError resyncErr = getGameManagerMaster()->resyncExternalSessionMembers(resyncRequest);
                    if (ERR_OK != resyncErr)
                    {
                        ERR_LOG("[MatchmakerSlaveImpl].joinGameExternalSession: Cleanup after failed external session update, failed to resync Blaze game(" << gameId << ")'s external session members with error " 
                            << ErrorHelp::getErrorName(resyncErr) << "), platform(" << ClientPlatformTypeToString(platformErr.first) << ")");
                    }
                }
            }

            if (apiErr.getFailedOnAllPlatforms())
                return apiErr.getSinglePlatformBlazeRpcErr() != ERR_OK ? apiErr.getSinglePlatformBlazeRpcErr() : GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR;

        }
        return ERR_OK;
    }
    
    /*! \brief joins the specified members to the matchmaking session's external session. */
    BlazeRpcError MatchmakerSlaveImpl::joinMatchmakingExternalSession(Blaze::GameManager::MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName,
        const GameManager::Matchmaker::MatchmakingSession::MemberInfoList& members, Blaze::ExternalSessionIdentification& externalSessionIdentification)
    {
        // This is Xbox MPSD sessions specific. Not expected used on other platforms

        JoinExternalSessionParameters joinExternalSessionParams;
        
        ExternalUserJoinInfoList& externalUserInfos = joinExternalSessionParams.getExternalUserJoinInfos();
        externalUserInfos.reserve(members.size());

        // Iterate over the members in the mm session, skip non xone players and use the token of the first member to make the external call. 
        for (GameManager::Matchmaker::MatchmakingSession::MemberInfoList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        {
            const GameManager::Matchmaker::MatchmakingSession::MemberInfo &member = static_cast<const GameManager::Matchmaker::MatchmakingSession::MemberInfo &>(*itr);
            if (member.mUserSessionInfo.getHasExternalSessionJoinPermission())
            {
                if (member.mUserSessionInfo.getUserInfo().getPlatformInfo().getClientPlatform() != xone)
                    continue;

                // note: don't add offline players as 'active' in the MM external session (they may not have an external token and will fail the join).
                setExternalUserJoinInfoFromUserSessionInfo(*externalUserInfos.pull_back(), member.mUserSessionInfo, member.mIsOptionalPlayer);
                
                if ((joinExternalSessionParams.getGroupExternalSessionToken() != nullptr) && (joinExternalSessionParams.getGroupExternalSessionToken()[0] == '\0'))
                    joinExternalSessionParams.setGroupExternalSessionToken(member.mUserSessionInfo.getExternalAuthInfo().getCachedExternalSessionToken());
            }
        }

        if (externalUserInfos.empty())
            return ERR_OK;

        GameManager::ExternalSessionUtil* util = mMatchmakingExternalSessionUtilMgr.getUtil(xone);
        if (util == nullptr)
            return ERR_SYSTEM;

        eastl::string sessionName;
        util->getExternalMatchmakingSessionName(sessionName, mmSessionId);
        joinExternalSessionParams.getSessionIdentification().getXone().setSessionName(sessionName.c_str());
        joinExternalSessionParams.getSessionIdentification().getXone().setTemplateName(sessionTemplateName);

        joinExternalSessionParams.getExternalSessionCreationInfo().setExternalSessionId(mmSessionId);
        joinExternalSessionParams.getExternalSessionCreationInfo().setExternalSessionType(EXTERNAL_SESSION_MATCHMAKING_SESSION);
        joinExternalSessionParams.getExternalSessionCreationInfo().getUpdateInfo().getFullMap()[xone] = true;//side: just in case MM session external sessions are used, closing MM external sessions to joins while in progress.
        
        ExternalSessionResult result;
        BlazeRpcError sessErr = util->join(joinExternalSessionParams, &result, true); // join creates an external session internally if one does not exist for our session name yet (which will be the case here).
        if (ERR_OK != sessErr)
        {
            ERR_LOG("[MatchmakerSlaveImpl].joinMatchmakingExternalSession: Failed to join external MM session " << sessionName.c_str() << "(MM session id " << mmSessionId << "), error " << ErrorHelp::getErrorName(sessErr));
            mMatchmaker->getMatchmakingMetrics().mMetrics.mExternalSessionFailuresNonImpacting.increment(1, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_VERSION, xone);
            // note: caller cleans up session. no need to do so here
            return sessErr;
        }

        result.getSessionIdentification().copyInto(externalSessionIdentification);
        return sessErr;
    }

    /*! \brief leaves the specified members from the matchmaking session's external session. */
    void MatchmakerSlaveImpl::leaveMatchmakingExternalSession(Blaze::GameManager::MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName, ExternalUserLeaveInfoList& members)
    {
        // This is Xbox MPSD sessions specific. Not expected used on other platforms

        LeaveGroupExternalSessionParameters leaveParams;
        ExternalUserLeaveInfoList& externalUsersList = leaveParams.getExternalUserLeaveInfos();

        for (auto member : members)
        {
            if (member->getUserIdentification().getPlatformInfo().getClientPlatform() == xone)
                externalUsersList.push_back(member);
        }

        if (externalUsersList.empty())
            return;

        GameManager::ExternalSessionUtil* util = mMatchmakingExternalSessionUtilMgr.getUtil(xone);
        if (util == nullptr)
        {
            ERR_LOG("[MatchmakerSlaveImpl].leaveMatchmakingExternalSession: Failed to leave external session (because we couldn't find the platform util) from mm session (" << mmSessionId << ")");
            return;
        }

        eastl::string sessionName;
        util->getExternalMatchmakingSessionName(sessionName, mmSessionId);
        leaveParams.getSessionIdentification().getXone().setSessionName(sessionName.c_str());
        leaveParams.getSessionIdentification().getXone().setTemplateName(sessionTemplateName.c_str());

        BlazeRpcError sessErr = util->leave(leaveParams);
        if (sessErr != ERR_OK)
        {
            // no Blaze cleanup on external session fail.Rely on external session to clean itself up by inactivity to keep things in sync.
            mMatchmaker->getMatchmakingMetrics().mMetrics.mExternalSessionFailuresNonImpacting.increment(1, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_VERSION, xone);
            TRACE_LOG("[MatchmakerSlaveImpl].leaveMatchmakingExternalSession: Failed to leave group from external session " << sessionName.c_str() << "(" << mmSessionId << "), error " << ErrorHelp::getErrorName(sessErr));
        }
    }


    void MatchmakerSlaveImpl::getExternalSessionTemplateName(XblSessionTemplateName& sessionTemplateName, const XblSessionTemplateName& overrideName, const Blaze::GameManager::UserJoinInfoList& userJoinInfoList)
    {
        // This is Xbox MPSD sessions specific. Not expected used on other platforms
        
        bool foundXoneUser = false;
        for (const auto& userJoinInfo : userJoinInfoList)
        {
            if (userJoinInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform() == xone)
            {
                foundXoneUser = true;
                break;
            }
        }

        if (foundXoneUser)
        {
            if (overrideName[0] != '\0')
            {
                sessionTemplateName.set(overrideName);
            }
            else
            {
                GameManager::ExternalSessionUtil* util = mMatchmakingExternalSessionUtilMgr.getUtil(xone);
                if (util != nullptr && !util->getConfig().getSessionTemplateNames().empty())
                {
                    sessionTemplateName.set(util->getConfig().getSessionTemplateNames().front());
                }
            }
        }
    }

    void MatchmakerSlaveImpl::getExternalSessionScid(EA::TDF::TdfString& scid)
    {
        GameManager::ExternalSessionUtil* util = mMatchmakingExternalSessionUtilMgr.getUtil(xone);
        if (util != nullptr)
            scid.set(util->getConfig().getScid());
    }

    // return true if the given topology needs QoS validation
    bool MatchmakerSlaveImpl::shouldPerformQosCheck(GameNetworkTopology topology) const
    {
        Blaze::GameManager::QosValidationCriteriaMap::const_iterator qosValidationIter = 
            getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().find(topology);
        
        // side: warning already logged up front if configuration missing qosCriteriaList
        return ((qosValidationIter != getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().end())
            && !qosValidationIter->second->getQosCriteriaList().empty());
    }

    bool MatchmakerSlaveImpl::isDrainComplete() 
    { 
        return (mMatchmaker->getTotalMatchmakingSessions() == 0);
    }

    void MatchmakerSlaveImpl::updateTimeToMatch(const TimeValue &scenarioDuration, const PingSiteAlias &pingSite, const GameManager::DelineationGroupType &delineationGroup,
        const GameManager::ScenarioName &scenarioName, GameManager::MatchmakingResult result)
    {
        mTimeToMatchEstimator.updateScenarioTimeToMatch(scenarioDuration, pingSite, delineationGroup, scenarioName, getConfig().getScenariosConfig(), result);
    }

    

}
}
