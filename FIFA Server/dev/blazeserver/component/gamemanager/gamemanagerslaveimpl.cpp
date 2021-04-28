/*! ************************************************************************************************/
/*!
    \file   gamemanagerslaveimpl.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "EATDF/tdfblobhash.h" // for tdfblobhash, tdfblob_equal_to in populateExternalPlayersInfo

#include "framework/replication/replicator.h"
#include "framework/controller/controller.h"
#include "framework/connection/connectionmanager.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/util/random.h" // for randomizing the start index when round-robining dedicated server games.
#include "framework/util/shared/blazestring.h"
#include "framework/util/networktopologycommoninfo.h" // for isDedicatedHostedTopology
#include "framework/util/connectutil.h"
#include "framework/database/dbscheduler.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/redis/redismanager.h"

#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/censusinfo.h" // for the genDataItem & genAttributeName utils
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/rules/gamenameruledefinition.h" // for GameNameRuleDefinition in configureReteSubstringTries()
#include "gamemanager/matchmakercomponent/timetomatchestimator.h" // for census TTM estimates
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/gamemanagerhelperutils.h" // for getHostSessionInfo
#include "gamemanager/commoninfo.h"

#include "framework/tdf/attributes.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanagermetrics_server.h"

#include "gamemanager/tdf/search_server.h"
#include "gamemanager/tdf/gamepacker_server.h"
#include "gamemanager/rpc/searchslave.h"
#include "gamemanager/rpc/gamepackermaster.h"


#include "gamemanager/gamemanagervalidationutils.h" // for ValidationUtils::setJoinUserInfoFromSession()
#include "gamemanager/externalsessions/externalsessionutil.h" // for mExternalSessionService
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/ccsutil.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/templateattributes.h"
#include "gamemanager/inputsanitizer.h"

#include "authentication/tdf/authentication.h" // for GetDecryptedBlazeIdsRequest in decryptBlazeIds()
#include "authentication/rpc/authenticationslave.h" // for AuthenticationSlave in decryptBlazeIds()

namespace Blaze
{
namespace GameManager
{
    // static factory method
    GameManagerSlave* GameManagerSlave::createImpl()
    {
        return BLAZE_NEW_NAMED("GameManagerSlaveImpl") GameManagerSlaveImpl();
    }

    GameManagerSlaveImpl::GameManagerSlaveImpl()
        : GameManagerSlaveStub(),
          mGameBrowser(*this),
          mTotalClientRequestsUpdatedDueToPoorReputation(getMetricsCollection(), "clientRequestsUpdatedDueToPoorReputation"),
          mActiveMatchmakingSlaves(getMetricsCollection(), "activeMatchmakingSlaves", [this]() { return mMatchmakingShardingInformation.mActiveMatchmakerCount; }),
          mExternalSessionFailuresImpacting(getMetricsCollection(), "externalSessionFailuresImpacting", Metrics::Tag::platform),
          mExternalSessionFailuresNonImpacting(getMetricsCollection(), "externalSessionFailuresNonImpacting", Metrics::Tag::platform),
          mUserSessionMMSessionReqCountMap(BlazeStlAllocator("mUserSessionMMSessionReqCountMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mScenarioManager(this),
          mExternalDataManager(this),
          mCcsJobQueue("GameManagerSlaveImpl:mCcsJobQueue"),
          mExternalSessionJobQueue("GameManagerSlaveImpl:mExternalSessionJobQueue"),
          mCleanupGameBrowserForInstanceJobQueue("GameManagerSlaveImpl:mCleanupGameBrowserForInstanceJobQueue"),
          mDedicatedServerOverrideMap(RedisId(COMPONENT_INFO.name, "dedicatedServerOverrideMap"))
    {
        mCcsJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
        mExternalSessionJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);

        gCensusDataManager->registerProvider(COMPONENT_ID, *this);

        mExternalSessionFailuresImpacting.defineTagGroups({ { Metrics::Tag::platform } });
        mExternalSessionFailuresNonImpacting.defineTagGroups({ { Metrics::Tag::platform } });
    }

    GameManagerSlaveImpl::~GameManagerSlaveImpl()
    {
    }


    // Note: components are shut down in a specific (safe) order, so unregister from maps/session manager here
    void GameManagerSlaveImpl::onShutdown()
    {
        mDedicatedServerOverrideMap.deregisterCollection();

        mGameBrowser.onShutdown();

        {
            // This work cancellation section should not be necessary; however, we do not have a strictly defined drain
            // state in the component, which means that some timers and fibers scheduled to execute after the isDrainComplete()
            // returns true may have a chance to run before onShutdown() is triggered. This in turn means that jobs can
            // be scheduled after isDrainComplete() == true, thus necessitating this cleanup. To be addressed by: GOS-30307.
            if (mCcsJobQueue.hasPendingWork())
            {
                ERR_LOG("[GameManagerSlave].onShutdown: CcsJobQueue not empty, cancelling work.");
                mCcsJobQueue.cancelAllWork();
            }
            if (mExternalSessionJobQueue.hasPendingWork())
            {
                ERR_LOG("[GameManagerSlave].onShutdown: ExternalSessionJobQueue not empty, cancelling work.");
                mExternalSessionJobQueue.cancelAllWork();
            }
            if (mCleanupGameBrowserForInstanceJobQueue.hasPendingWork())
            {
                ERR_LOG("[GameManagerSlave].onShutdown: CleanupGameBrowserForInstanceJobQueue not empty, cancelling work.");
                mCleanupGameBrowserForInstanceJobQueue.cancelAllWork();
            }
        }
        gController->deregisterDrainStatusCheckHandler(COMPONENT_INFO.name);

        gController->removeRemoteInstanceListener(*this);
        gSliverManager->removeSliverSyncListener(*this);

        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);
        gUserSessionManager->removeSubscriber(*this);
        gUserSetManager->deregisterProvider(COMPONENT_ID);
        gCensusDataManager->unregisterProvider(COMPONENT_ID);
    }

    bool GameManagerSlaveImpl::isDrainComplete()
    {
        eastl::string pendingTasks;

        if (mExternalSessionJobQueue.hasPendingWork())
            pendingTasks += "ExternalSessionJobQueue,";
        if (mCcsJobQueue.hasPendingWork())
            pendingTasks += "CcsJobQueue,";
        if (mCleanupGameBrowserForInstanceJobQueue.hasPendingWork())
            pendingTasks += "CleanupGameBrowserForInstanceJobQueue,";
        if (mPseudoGameCreateFiber.isValid())
            pendingTasks += "PseudoGameCreateFiber,";

        if (pendingTasks.empty())
            return true;

        pendingTasks.pop_back(); // trim comma

        BLAZE_INFO_LOG(getLogCategory(), "[GameManagerSlave].isDrainComplete: pending tasks (" << pendingTasks.c_str() << ")");

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief register ourself as a usersetprovider
    ***************************************************************************************************/
    bool GameManagerSlaveImpl::onActivate()
    {
        gUserSetManager->registerProvider(COMPONENT_ID, *this);
        gSliverManager->addSliverSyncListener(*this);
        mScenarioManager.onActivate();
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief register ourself as a listener to notifications from the master once we resolve it
    ***************************************************************************************************/
    bool GameManagerSlaveImpl::onResolve()
    {
        getMaster()->addNotificationListener(*this);
        mScenarioManager.onResolve();

        BlazeRpcError error = ERR_SYSTEM;
        {
            // NOTE: By using a remote component placeholder we are able to perform an apriori subscription for notifications *without*
            // waiting for the component to actually be active.
            GamePacker::GamePackerMaster* packerComponent = nullptr;
            error = gController->getComponentManager().addRemoteComponentPlaceholder(Blaze::GamePacker::GamePackerMaster::COMPONENT_ID, reinterpret_cast<Component*&>(packerComponent));
            if (error != ERR_OK || packerComponent == nullptr)
            {
                FATAL_LOG("[GameManagerSlave].onResolve() - Unable to obtain gamepacker master component placeholder.");
                return false;
            }

            packerComponent->addNotificationListener(*this);
            if (packerComponent->notificationSubscribe() != Blaze::ERR_OK)
            {
                FATAL_LOG("[GameManagerSlave].onResolve(): Unable to subscribe for notifications from gamepacker component.");
                return false;
            }
        }

        {
            // NOTE: By using a remote component placeholder we are able to perform an apriori subscription for notifications *without*
            // waiting for the component to actually be active.
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = nullptr;
            error = gController->getComponentManager().addRemoteComponentPlaceholder(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, reinterpret_cast<Component*&>(matchmakerComponent));
            if (error != ERR_OK || matchmakerComponent == nullptr)
            {
                FATAL_LOG("[GameManagerSlave].onResolve() - Unable to obtain matchmaker component placeholder.");
                return false;
            }

            matchmakerComponent->addNotificationListener(*this);
            if (matchmakerComponent->notificationSubscribe() != Blaze::ERR_OK)
            {
                FATAL_LOG("[GameManagerSlave].onResolve(): Unable to subscribe for notifications from matchmaker component.");
                return false;
            }
        }

        if (!mGameBrowser.hostingComponentResolved())
        {
            FATAL_LOG("[GameManagerSlave].onResolve() - Unable to setup Game Browser: onHostingComponentResolved() failed.");
            return false;
        }

        gController->addRemoteInstanceListener(*this);

        gUserSessionManager->addSubscriber(*this);
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);

        gController->registerDrainStatusCheckHandler(COMPONENT_INFO.name, *this);

        if (GameManager::ValidationUtils::isSetupToUseCCS(getConfig()))
        {
            if (!GameManager::ValidationUtils::hasValidCCSConfig(getConfig()))
            {
                ERR_LOG("[GameManagerSlaveImpl].onResolve: CCS config is invalid.");
                return false;
            }

            bool isBlankRedis = gUserSessionMaster->getUserSessionStorageTable().isFirstRegistration();
            if (isBlankRedis)
            {
                // We need to clean CCS state because redis is empty, yet CCS may not be
                // On Windows, the frequent kill actions on blaze server put CCS in a bad state because we don't get a chance to properly clean up. So on start up, we free up any left overs from the past
                // iteration. This is not a problem on Linux as the redis server lifetime is independent of the Blaze server and it is able to reconcile the state correctly. 
                CCSUtil ccsUtil;

                eastl::hash_set<eastl::string> pools;
                pools.insert(getConfig().getCcsConfig().getCcsPool());
                for (auto& itr : getConfig().getGameSession().getGameModeToCCSModeMap())
                {
                    const char8_t* pool = itr.second->getCCSPool();
                    if (pool[0] != '\0')
                    {
                        pools.insert(pool);
                    }
                }

                uint32_t ccsRequestId = BuildInstanceKey32(gController->getInstanceId(), 0);
                const eastl::string clientId = ccsUtil.getClientId();

                eastl::string nucleusToken;
                error = ccsUtil.getNucleusAccessToken(nucleusToken); // NOTE: This only works on the core slave right now, because it depends on authentication component, ugh!
                if (error == Blaze::ERR_OK)
                {

                    // Go through all CCS pool overrides in the game mode to CCS mode mapping and clean those too
                    for (const auto& pool : pools)
                    {
                        Blaze::CCS::FreeClientHostedConnectionsRequest req;
                        req.setCCSClientId(clientId.c_str());
                        req.setAPIVersion(getConfig().getCcsConfig().getCcsAPIVer());
                        req.setAccessToken(nucleusToken.c_str());
                        req.setPool(pool.c_str());
                        req.setRequestId(++ccsRequestId);

                        Blaze::CCS::FreeClientHostedConnectionsResponse response;
                        Blaze::CCS::CCSErrorResponse errorResponse;

                        error = ccsUtil.freeClientHostedConnections(req, response, errorResponse);
                        if (error != Blaze::ERR_OK)
                        {
                            ERR_LOG("[GameManagerSlaveImpl].onResolve: Request failed due to error in calling ccs service. Request Id(" << ccsRequestId << "). Error (" << ErrorHelp::getErrorName(error) << ")");
                        }
                    }
                }
                else
                {
                    ERR_LOG("[GameManagerSlaveImpl].onResolve: Request failed due to error in getting access token. Request Id("<< ccsRequestId <<"), Error ("<<ErrorHelp::getErrorName(error)<<")");
                }
            }
        }

        mDedicatedServerOverrideMap.registerCollection();

        return true;
    }
    
    bool GameManagerSlaveImpl::onValidatePreconfig(GameManagerServerPreconfig& config, const GameManagerServerPreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        if (!mScenarioManager.validatePreconfig(config))
        {
            ERR_LOG("[GameManagerSlaveImpl].onValidatePreconfig: Configuration problem while validating preconfig for ScenarioManager.");
            return false;
        }

        return true;
    }

    bool GameManagerSlaveImpl::onPreconfigure()
    {
        TRACE_LOG("[GameManagerSlaveImpl] Preconfiguring gamemanager slave component.");

        const GameManagerServerPreconfig& preconfigTdf = getPreconfig();

        if (!mScenarioManager.preconfigure(preconfigTdf))
        {
            ERR_LOG("[GameManagerSlaveImpl].onPreconfigure: Configuration problem while preconfiguring the ScenarioManager.");
            return false;
        }
        return true;
    }

    bool GameManagerSlaveImpl::onValidateConfig(GameManagerServerConfig& config, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        if (!inputSanitizerConfigValidation(config.getInputSanitizers(), validationErrors))
        {
            ERR_LOG("[GameManagerSlaveImpl].onValidateConfig: Configuration problem while configuring the InputSanitizers.");
            return false;
        }

        if (!validateCreateGameTemplateConfig(config, validationErrors))
        {
            ERR_LOG("[GameManagerSlaveImpl].onValidateConfig: Configuration problem while configuring the CreateGameTemplates.");
            return false;
        }

        if (!mScenarioManager.validateConfig(config.getScenariosConfig(), config.getMatchmakerSettings(), &config, getPreconfig(), validationErrors))
        {
            ERR_LOG("[GameManagerSlaveImpl].onValidateConfig: Configuration problem while configuring the ScenarioManager.");
            return false;
        }

        if (!mGameBrowser.validateConfig(config, referenceConfigPtr, validationErrors))
        {
            ERR_LOG("[GameManagerSlaveImpl].onValidateConfig: Configuration problem while configuring the Game Browser Configuration.");
            return false;
        }

        // Verify and clean up the configs:
        if (!validatePseudoGameConfig())
        {
            return false;   // (err already logged)
        }

        // ensure no duplicate user extended data keys ids have been configured
        eastl::set<uint16_t> uedKeyIdSet;
        GameManagerUserExtendedDataByKeyIdMap::const_iterator gameManagerUEDConfigItr = getConfig().getGameManagerUserExtendedDataConfig().begin();
        GameManagerUserExtendedDataByKeyIdMap::const_iterator gameManagerUEDConfigEnd = getConfig().getGameManagerUserExtendedDataConfig().end();
        for ( ; gameManagerUEDConfigItr != gameManagerUEDConfigEnd; ++gameManagerUEDConfigItr)
        {
            if (uedKeyIdSet.find(gameManagerUEDConfigItr->second) == uedKeyIdSet.end())
            {
                uedKeyIdSet.insert(gameManagerUEDConfigItr->second);
            }
            else
            {
                ERR_LOG("[GameManagerSlaveImpl].onValidateConfig: GameManagerUserExtendedDataMap keyname("<< gameManagerUEDConfigItr->first.c_str() <<") has a duplicate key id ("
                    << gameManagerUEDConfigItr->second << ") with another UserExtendedDataKey");
                return false;
            }
        }

        if (!mExternalDataManager.validateConfig(config, validationErrors))
        {
            ERR_LOG("[GameManagerSlave] Configuration problem while parsing External Data Manager.");
            return false;
        }

        if (validationErrors.getErrorMessages().size() > 0)
        {
            WARN_LOG("[GameManagerSlaveImpl].onValidateConfig: Validation failed, errors: " << validationErrors.getErrorMessages());
            return false;
        }

        return true;
    }

    bool GameManagerSlaveImpl::onPrepareForReconfigure(const GameManagerServerConfig& newConfig)
    {
        mExternalHostIpWhitelistStaging = BLAZE_NEW InetFilter();
        if (!mExternalHostIpWhitelistStaging->initialize(true, newConfig.getGameSession().getExternalHostIpWhitelist()))
        {
            FAIL_LOG("[GameManagerSlaveImpl].onPrepareForReconfigure: Failed to initialize external host ip whitelist");
            return false;
        }

        mInternalHostIpWhitelistStaging = BLAZE_NEW InetFilter();
        if (!mInternalHostIpWhitelistStaging->initialize(true, newConfig.getGameSession().getInternalHostIpWhitelist()))
        {
            FAIL_LOG("[GameManagerSlaveImpl].onPrepareForReconfigure: Failed to initialize internal host ip whitelist");
            return false;
        }

        if (!mExternalSessionServices.onPrepareForReconfigure(newConfig.getGameSession().getExternalSessions()))
        {
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief dispatched once the slave has received an initial configuration from the config master.
    ***************************************************************************************************/
    bool GameManagerSlaveImpl::onConfigure()
    {
        TRACE_LOG("[GameManagerSlaveImpl] Configuring gamemanager slave component.");

        const GameManagerServerConfig& configTdf = getConfig();

        //Setup our unique id management
        if (gUniqueIdManager->reserveIdType(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, Blaze::GameManager::MATCHMAKER_IDTYPE_SESSION) != ERR_OK)
        {
            ERR_LOG("[MatchmakerSlaveImpl].onConfigure: Failed reserving component id " <<
                COMPONENT_ID << " with unique id manager");
            return false;
        }
        if (gUniqueIdManager->reserveIdType(COMPONENT_ID, GAMEMANAGER_IDTYPE_GAMEBROWSER_LIST) != ERR_OK)
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed reserving component id " << 
                COMPONENT_ID << ", idType " << GAMEMANAGER_IDTYPE_GAMEBROWSER_LIST << " with unique id manager");
            return false;
        }
        if (gUniqueIdManager->reserveIdType(COMPONENT_ID, GAMEMANAGER_IDTYPE_EXTERNALSESSION) != ERR_OK)
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed reserving component id " << 
                COMPONENT_ID << ", idType " << GAMEMANAGER_IDTYPE_EXTERNALSESSION << " with unique id manager");
            return false;
        }

        if (gUniqueIdManager->reserveIdType(COMPONENT_ID, GAMEMANAGER_IDTYPE_SCENARIO) != ERR_OK)
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed reserving component id " << 
                COMPONENT_ID << ", idType " << GAMEMANAGER_IDTYPE_SCENARIO << " with unique id manager");
            return false;
        }

        // configure gamebrowser
        if (!mGameBrowser.configure(configTdf, configTdf.getGameSession().getEvaluateGameProtocolVersionString()))
        {
            ERR_LOG("GameManagerSlaveImpl: Configuration problem while parsing GameBrowserScenarios.");
            return false;
        }

        if (!mScenarioManager.loadScenarioConfigInfoFromDB())
        {
            WARN_LOG("[GameManagerSlave] Failed to load configurations for Scenario from database. These configurations are used for metrics.");
        }


        UserExtendedDataIdMap uedIdMap;
        GameManagerUserExtendedDataByKeyIdMap::const_iterator gameManagerUEDConfigItr = getConfig().getGameManagerUserExtendedDataConfig().begin();
        GameManagerUserExtendedDataByKeyIdMap::const_iterator gameManagerUEDConfigEnd = getConfig().getGameManagerUserExtendedDataConfig().end();
        for ( ; gameManagerUEDConfigItr != gameManagerUEDConfigEnd; ++gameManagerUEDConfigItr)
        {
            UserSessionManager::addUserExtendedDataId(uedIdMap, gameManagerUEDConfigItr->first, GameManagerSlave::COMPONENT_ID, gameManagerUEDConfigItr->second);
        }

        if (gUserSessionManager->updateUserSessionExtendedIds(uedIdMap) != ERR_OK)
        {
            WARN_LOG("[GameManageSlaveImpl].onConfigure: failed to updateUserSessionExtendedIds with getGameManagerUEDNameList");
        }

        uedIdMap.copyInto(mGameManagerUserExtendedDataIdMap);

        if (!mScenarioManager.configure(configTdf))
        {
            ERR_LOG("[GameManagerSlave] Configuration problem while parsing ScenarioManager.");
            return false;
        }

        mExternalDataManager.configure(configTdf);

        bool success = GameManager::ExternalSessionUtil::createExternalSessionServices(mExternalSessionServices, getConfig().getGameSession().getExternalSessions(),
            getConfig().getGameSession().getPlayerReservationTimeout(), UINT16_MAX,
            &getConfig().getGameSession().getTeamNameByTeamIdMap(), this);
        if (!success)
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed creating external session service");
            return false;
        }

        mExternalHostIpWhitelist = BLAZE_NEW InetFilter();
        if (!mExternalHostIpWhitelist->initialize(true, configTdf.getGameSession().getExternalHostIpWhitelist()))
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed configuring external host ip whitelist");
            return false;
        }

        mInternalHostIpWhitelist = BLAZE_NEW InetFilter();
        if (!mInternalHostIpWhitelist->initialize(true, configTdf.getGameSession().getInternalHostIpWhitelist()))
        {
            ERR_LOG("[GameManageSlaveImpl].onConfigure: Failed configuring internal host ip whitelist");
            return false;
        }

        buildInternalPseudoGameConfigs();
        gFiberManager->scheduleCall<GameManagerSlaveImpl>(this, &GameManagerSlaveImpl::generatePseudoGamesThread, Fiber::CreateParams());

        return true;
    }
    
    BlazeRpcError GameManagerSlaveImpl::internalGetCreateGameTemplateAttributesConfig(GetTemplatesAttributesResponse& response) const
    {
        // For each config, get the info:
        
        CreateGameTemplateMap::const_iterator curTemplate = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().begin();
        CreateGameTemplateMap::const_iterator endTemplate = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end();
        for (; curTemplate != endTemplate; ++curTemplate)
        {
            TemplateAttributeMapping* attrMapping = response.getTemplateAttributes().allocate_element();
            response.getTemplateAttributes()[curTemplate->first] = attrMapping;

            TemplateAttributeMapping::const_iterator curAttr = curTemplate->second->getAttributes().begin();
            TemplateAttributeMapping::const_iterator endAttr = curTemplate->second->getAttributes().end();
            for (; curAttr != endAttr; ++curAttr)
            {
                TemplateAttributeMapping::iterator iter = attrMapping->find(curAttr->first);
                if (iter == attrMapping->end())
                {
                    TemplateAttributeDefinition* attrDef = attrMapping->allocate_element();
                    curAttr->second->copyInto(*attrDef);
                    (*attrMapping)[curAttr->first] = attrDef;
                }
                else
                {
                    curAttr->second->copyInto(*(iter->second));
                }
            }
        }

        return ERR_OK;
    }

    BlazeRpcError GameManagerSlaveImpl::internalGetTemplatesAndAttributes(GetTemplatesAndAttributesResponse & response) const
    {
        // For each config, get the info:

        for (auto& curTemplate : getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates())
        {
            TemplateAttributeDescriptionMapping* attrMapping = response.getTemplateAttributeDescriptions().allocate_element();
            response.getTemplateAttributeDescriptions()[curTemplate.first] = attrMapping;

            for (auto& curAttr : curTemplate.second->getAttributes())
            {
                TemplateAttributeDescriptionMapping::iterator iter = attrMapping->find(curAttr.first);
                if (iter == attrMapping->end())
                {
                    const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr;
                    const EA::TDF::TypeDescription* typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(curAttr.first, &bfMember);
                    if (typeDesc)
                    {
                        auto* attrDef = attrMapping->allocate_element();
                        attrDef->setIsOptional(curAttr.second->getIsOptional());
                        attrDef->setAttrTdfId(typeDesc->getTdfId());
                        curAttr.second->getDefault().copyInto(attrDef->getDefault());
                        (*attrMapping)[curAttr.second->getAttrName()] = attrDef;
                    }
                }
            }
        }

        return ERR_OK;
    }

    bool GameManagerSlaveImpl::validateCreateGameTemplateConfig(GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors) const
    {
        AttributeToTypeDescMap attrToTypeDesc;

        CreateGameRequest validationRequest;
        EA::TDF::TdfGenericReference cgReqRef(validationRequest);

        const CreateGameTemplatesConfig& cgConfig = config.getCreateGameTemplatesConfig();

        // Verify that everything in the rule_attributes file is valid: 
        CreateGameTemplateMap::const_iterator curTemplate = cgConfig.getCreateGameTemplates().begin();
        CreateGameTemplateMap::const_iterator endTemplate = cgConfig.getCreateGameTemplates().end();
        for (; curTemplate != endTemplate; ++curTemplate)
        {
            const TemplateName& templateName = curTemplate->first;
            // Check that the template's name isn't too long:
            if (templateName.length() >= MAX_TEMPLATENAME_LEN)
            {
                StringBuilder strBuilder;
                strBuilder << "[GameManagerSlaveImpl].validateCreateGameTemplateConfig: Template Name: " << templateName.c_str() << 
                        " has length (" << templateName.length() << ") >= MAX_TEMPLATENAME_LEN (" << MAX_TEMPLATENAME_LEN <<
                        "). This would prevent the template from being used, because the string name couldn't be sent from the client.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            StringBuilder errorPrefix;
            errorPrefix << "[GameManagerSlaveImpl].validateCreateGameTemplateConfig: Template: " << templateName.c_str() << " "; 
            validateTemplateAttributeMapping(curTemplate->second->getAttributes(), attrToTypeDesc, cgReqRef, validationErrors, errorPrefix.get());

            ValidationUtils::validateFindDedicatedServerRules(templateName, config, validationErrors);

            const char8_t* gbScenarioName = curTemplate->second->getHostSelectionGameBrowserScenario();
            if (*gbScenarioName != '\0')
            {
                const GameBrowserScenariosConfig& cfg = config.getGameBrowserScenariosConfig();
                if (cfg.getScenarios().find(gbScenarioName) == cfg.getScenarios().end())
                {
                    StringBuilder strBuilder;
                    strBuilder << "[GameManagerSlaveImpl].validateCreateGameTemplateConfig: Template Name: " << templateName.c_str() <<
                        " has non existing GameBrowser Scenario (" << gbScenarioName << ").";
                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                    continue;
                }
            }
        }

        return (validationErrors.getErrorMessages().size() == 0);
    }

    BlazeRpcError GameManagerSlaveImpl::createGameTemplate(Command* cmd, CreateGameTemplateRequest& templateRequest, JoinGameResponse& response, EncryptedBlazeIdList* failedEncryptedIds /*= nullptr*/, bool isCreateTournamentGame /*= false*/)
    {
        CreateGameTemplateMap::const_iterator templateIter = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().find(templateRequest.getTemplateName());
        if (templateIter == getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end())
        {
            WARN_LOG("[GameManagerSlaveImpl].createGameTemplate: Invalid create game template requested - " << templateRequest.getTemplateName() << ".  No game will be created." );
            return GAMEMANAGER_ERR_INVALID_TEMPLATE_NAME;
        }
        
        CreateGameTemplate templateConfig = (*templateIter->second);

        // Build the request:
        CreateGameRequest request;

        // Start with the base:
        templateConfig.getBaseRequest().copyInto(request);     

        // Override data from external sources
        if (!templateConfig.getExternalDataSourceApiList().empty())
        {
            UserIdentificationList userIdList;
            // External Data Source's exist. If group of players are issuing the join, load the players into the player join data to make API calls.
            ValidationUtils::insertHostSessionToPlayerJoinData(templateRequest.getPlayerJoinData(), gCurrentLocalUserSession);
            BlazeRpcError blazeErr = ValidationUtils::loadGroupUsersIntoPlayerJoinData(templateRequest.getPlayerJoinData(), userIdList, *this);
            if (blazeErr != Blaze::ERR_OK)
            {
                gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(blazeErr), RiverPoster::game_creation_failed);
                return blazeErr;
            }

            if (!mExternalDataManager.fetchAndPopulateExternalData(templateRequest))
            {
                WARN_LOG("[GameManagerSlaveImpl].createGameTemplate: Failed to fetch and populate external data in tempate(" << templateRequest.getTemplateName() << ")");
            }
            templateConfig.getExternalDataSourceApiList().copyInto(request.getGameCreationData().getDataSourceNameList());
        }

        // Override with data from the client:
        templateRequest.getPlayerJoinData().copyInto(request.getPlayerJoinData());
        templateRequest.getCommonGameData().copyInto(request.getCommonGameData());

        // Decrypt encrypted blaze ids as needed
        BlazeRpcError decryptErr = decryptBlazeIds(request.getPlayerJoinData().getPlayerDataList(), failedEncryptedIds);
        if (decryptErr != ERR_OK)
        {
            return decryptErr;
        }

        // Grab the GameType setting from the config, not the client provided values.
        request.getCommonGameData().setGameType(templateConfig.getBaseRequest().getCommonGameData().getGameType());          
        
        // If Packer is used for the Create Game calls in the future, this may need ot be filled out. 
        PropertyNameMap properties;

        // Add in attribute data: 
        const char8_t* failingAttr = "";
        StringBuilder componentDescription;
        componentDescription << "In GameManagerSlaveImpl::createGameTemplate for template " << templateRequest.getTemplateName();
        BlazeRpcError err = applyTemplateAttributes(EA::TDF::TdfGenericReference(request), templateConfig.getAttributes(), templateRequest.getTemplateAttributes(), properties, failingAttr, componentDescription.get());
        if (err != ERR_OK)
        {
            WARN_LOG("[GameManagerSlaveImpl].createGameTemplate: Failure applying attribute - " << failingAttr );
            return err;
        }

        // Convert old Xbox session params to new params (for back compat). Do after applying attributes above
        oldToNewExternalSessionIdentificationParams(request.getGameCreationData().getExternalSessionTemplateName(), nullptr, nullptr, nullptr, request.getGameCreationData().getExternalSessionIdentSetup());


        // Special case for virtual games - Remove any PJD if reservations are not being used:
        bool noJoinsJustVirtual = false;
        if (request.getGameCreationData().getGameSettings().getVirtualized() && request.getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {
            PlayerJoinData clearPJD;
            clearPJD.copyInto(request.getPlayerJoinData());
            noJoinsJustVirtual = true;
        }
        // For external owner - we use current calling user session to populate master, to simplify, remove from PJD
        // For tournament creator - TOs never join, to avoid issues, ensure removed from PJD (it may been inserted above just to set external data attributes)
        if (request.getGameCreationData().getIsExternalOwner() || isCreateTournamentGame || request.getGameCreationData().getIsExternalCaller())
        {
            ValidationUtils::fixupPjdByRemovingCaller(request.getPlayerJoinData());
        }

        err = fixupPlayerJoinDataRoles(request.getPlayerJoinData(), true, getLogCategory());
        if (err != ERR_OK)
        {
            return err;
        }

        // Send off the create or reset request: 
        err = ERR_SYSTEM;
        switch (templateConfig.getCreateRequestType())
        {
        case CreateGameTemplate::CREATE_GAME:
        {
            // The CG response is different from the join game response. It is missing the JoinGameState. Which, stupidly, we do not expose on the client.
            CreateGameResponse cgResponse;
            CreateGameMasterErrorInfo errInfo;
            err = createGameInternal(cmd, templateRequest.getTemplateName(), request, cgResponse, errInfo);
            response.setGameId(cgResponse.getGameId());
            cgResponse.getExternalSessionIdentification().copyInto(response.getExternalSessionIdentification());
            cgResponse.getJoinedReservedPlayerIdentifications().copyInto(response.getJoinedReservedPlayerIdentifications());
            if (err != ERR_OK)
            {
                gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::game_creation_failed);
            }
            break;
        }
        case CreateGameTemplate::CREATE_OR_JOIN_GAME:
        {
            // After template attributes applied, prep req for PS5's UX boot to new game as needed.
            err = getExternalSessionUtilManager().prepForCreateOrJoin(request);
            if (err != ERR_OK)
            {
                return err;
            }

            err = createOrJoinGameInternal(cmd, templateRequest.getTemplateName(), request, response);
            break;
        }
        case CreateGameTemplate::RESET_GAME:
        {
            err = resetDedicatedServerInternal(cmd, templateRequest.getTemplateName(), request, response);
            break;
        }
        default: break;
        }
        
        if (noJoinsJustVirtual)
        {
            // If no one joined (virtual game), we need to inform the client so that it doesn't wait around for a NotifyGame update. 
            response.setJoinState(NO_ONE_JOINED);
        }
        if (request.getGameCreationData().getIsExternalCaller())
        {
            // If omitted caller, we need to inform the client so that it doesn't wait around for a NotifyGame update. 
            response.setIsExternalCaller(request.getGameCreationData().getIsExternalCaller());
        }

        return err;
    }

    void cleanupCreateGameRequest(CreateGameRequest& request)
    {
        // Slot Capacity currently continue to use the old vector.  Should update to the new request.
        for (auto& iter : request.getGameCreationData().getSlotCapacitiesMap())
        {
            request.getSlotCapacitiesMap().insert(iter);
        }
        request.getGameCreationData().getSlotCapacitiesMap().clear();

        for (uint32_t i = 0; i < (uint32_t)MAX_SLOT_TYPE; ++i)
        {
            SlotCapacitiesMap::const_iterator iter = request.getSlotCapacitiesMap().find((SlotType)i);
            if (iter != request.getSlotCapacitiesMap().end())
            {
                request.getSlotCapacities()[iter->first] = iter->second;
            }
        }
        request.getSlotCapacitiesMap().clear();

        // Backwards Compatibility for TeamIds:
        if (!request.getTeamIds().empty())
        {
            request.getTeamIds().copyInto(request.getGameCreationData().getTeamIds());
        }
    }

    BlazeRpcError GameManagerSlaveImpl::populateRequestCrossplaySettings(CreateGameMasterRequest& masterRequest)
    {
        bool crossplayEnabled = false;
        BlazeRpcError err = ValidationUtils::validateRequestCrossplaySettings(crossplayEnabled, masterRequest.getUsersInfo(),
            true, masterRequest.getCreateRequest().getGameCreationData(), getConfig().getGameSession(),
            masterRequest.getCreateRequest().getClientPlatformListOverride(), masterRequest.getExternalOwnerInfo().getUserInfo().getId(), false);

        // Set the Crossplay setting to the correct value:  (We only disable Crossplay, never enable by default)
        // validation method above has set/corrected the proper client platform list for this request.
        masterRequest.setIsCrossplayEnabled(crossplayEnabled);

        if (err != ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].populateRequestCrossplaySettings:  Crossplay enabled user attempted to create a game with invalid settings.  Unable to just disable crossplay to fix this.");
            return err;
        }

        return err;
    }

    BlazeRpcError GameManagerSlaveImpl::resetDedicatedServerInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, JoinGameResponse& response)
    {
        cleanupCreateGameRequest(request);


        CreateGameMasterRequest masterRequest;
        masterRequest.setCreateRequest(request);
        
        // Check whether this client have the permission to join, or "host" a game 
        // Host and permissions are required because the resetter will become a platform (but not topology) host.
        if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_JOIN_GAME_SESSION)
            || !UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_HOST_GAME_SESSION))  // TODO:   Remove this HOST check when PlatformHost is removed. (Fully S2S SDK calls)
        {
            return ERR_AUTHORIZATION_REQUIRED;
        }

        // Attempt to preserve previous forcing to c/s dedicated for clients that aren't specifying topology.
        // GM_TODO: 3.x refactor this to default in the txn.
        if (!isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology()))
        {
            request.getGameCreationData().setNetworkTopology(CLIENT_SERVER_DEDICATED);
        }

        if (!ValidationUtils::validateAttributeIsPresent(request.getGameCreationData().getGameAttribs(), getConfig().getGameSession().getGameModeAttributeName()))
        {
            return GAMEMANAGER_ERR_GAME_MODE_ATTRIBUTE_MISSING;
        }

        // NOTE: long-term, we'd like to create a separate TDF for resetting a dedicated server (one without the fields that reset won't change)
        //   that's why we haven't consolidated this validation with the validation in create game slave cmd.
        BlazeRpcError blazeErr = Blaze::ERR_OK;

        if (!request.getGameCreationData().getIsExternalCaller())
            ValidationUtils::insertHostSessionToPlayerJoinData(masterRequest.getCreateRequest().getPlayerJoinData(), gCurrentLocalUserSession);

        UserIdentificationList groupUserIds;
        // validate group existence & membership.  Populates our session id list.
        if (!request.getGameCreationData().getIsExternalCaller()) //omit its group also
        {
            blazeErr = ValidationUtils::loadGroupUsersIntoPlayerJoinData(masterRequest.getCreateRequest().getPlayerJoinData(), groupUserIds, *this);
            if (blazeErr != Blaze::ERR_OK)
            {
                return blazeErr;
            }
        }

        // find and add each user session id to session id list
        blazeErr = ValidationUtils::userIdListToJoiningInfoList(masterRequest.getCreateRequest().getPlayerJoinData(), groupUserIds, masterRequest.getUsersInfo(), &getExternalSessionUtilManager(), request.getGameCreationData().getIsExternalCaller());
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo());
        if (hostSessionInfo == nullptr)
        {
            // Indirect matchmaking should assign a host session, so this would only trigger for indirect game creation (which we don't support yet?)
            TRACE_LOG("[GameManagerSlaveImpl].resetDedicatedServerInternal: Missing a player host for the reset request.  Were you attempting indirect game creation?");
            return GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER;
        }

        populateExternalPlayersInfo(masterRequest.getCreateRequest().getPlayerJoinData(),
            groupUserIds,
            masterRequest.getUsersInfo(),
            hostSessionInfo->getExternalAuthInfo().getCachedExternalSessionToken());

        // note for create/reset game (unlike join game), we need to process the reputations here for the external ids too
        // as they're used to determine whether we need to allow any reputation in the game settings below.
        bool hasBadReputation = GameManagerSlaveImpl::updateAndCheckUserReputations(masterRequest.getUsersInfo());

        blazeErr = populateRequestCrossplaySettings(masterRequest);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // Accounts for gg members and reserved players.
        SlotTypeSizeMap slotTypeSizeMap;
        ValidationUtils::populateSlotTypeSizes(masterRequest, slotTypeSizeMap);

        // validate that we have enough space for the resetting/joining game group
        blazeErr = ValidationUtils::validateGameCapacity(slotTypeSizeMap, request);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // set up default teams, if needed
        uint16_t participantCapacity = request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) + request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT);
        uint16_t teamCapacity = setUpDefaultTeams(request.getGameCreationData().getTeamIds(), request.getPlayerJoinData(), participantCapacity);


        // set max capacity (if invalidly set)
        uint16_t maxPlayerCapacity = 0;
        for (uint16_t curSlot = 0; curSlot < (uint16_t)request.getSlotCapacities().size(); ++curSlot)
        {
            maxPlayerCapacity += request.getSlotCapacities()[curSlot];
        }
        if (maxPlayerCapacity > request.getGameCreationData().getMaxPlayerCapacity())
            masterRequest.getCreateRequest().getGameCreationData().setMaxPlayerCapacity(maxPlayerCapacity);

        // set up default role info if missing in request
        setUpDefaultRoleInformation(request.getGameCreationData().getRoleInformation(), teamCapacity);

        blazeErr = fixupPlayerJoinDataRoles(masterRequest.getCreateRequest().getPlayerJoinData(), true, getLogCategory());
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        //verify that the team capacities are large enough for the group
        blazeErr = ValidationUtils::validateTeamCapacity(slotTypeSizeMap, request, false);

        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // Verify that the team capacities are large enough for the group.
        blazeErr = ValidationUtils::validatePlayerRolesForCreateGame(masterRequest, true);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        if (cmd != nullptr)
        {
            if (request.getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION
                && request.getCommonGameData().getPlayerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
            {
                ConnectUtil::setExternalAddr(request.getCommonGameData().getPlayerNetworkAddress(), cmd->getPeerInfo()->getPeerAddress());
            }
        }
        else
        {
            TRACE_LOG("[GameManagerSlaveImpl].createGameInternal: User(" << UserSession::getCurrentUserBlazeId() << ") calling command is nullptr. No peerInfo.");
            return ERR_SYSTEM;
        }

        if (!gUserSessionManager->isReputationDisabled())
        {
            if (hasBadReputation && !request.getGameCreationData().getGameSettings().getAllowAnyReputation())
            {
                // poor rep user, force allowAnyReputationFlag
                incrementTotalRequestsUpdatedForReputation();
                TRACE_LOG("[resetDedicatedServerInternal]: User " << UserSession::getCurrentUserBlazeId()
                    << " attempted to reset game[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], had to set allowAnyReputationFlag.");
                masterRequest.getCreateRequest().getGameCreationData().getGameSettings().setAllowAnyReputation();
            }
            if (!hasBadReputation && request.getGameCreationData().getGameSettings().getDynamicReputationRequirement())
            {
                // all good rep users, force clear allowAnyReputationFlag if dynamic rep mode enabled
                masterRequest.getCreateRequest().getGameCreationData().getGameSettings().clearAllowAnyReputation();
            }
        }
        // if adding an external session, get its name and correlation id to initialize the blaze game session with
        BlazeRpcError sessErr = initRequestExternalSessionData(masterRequest.getExternalSessionData(), masterRequest.getCreateRequest().getGameCreationData().getExternalSessionIdentSetup(), request.getCommonGameData().getGameType(),
            masterRequest.getUsersInfo(), getConfig().getGameSession(), getExternalSessionUtilManager(), *this);
        if (sessErr != ERR_OK && masterRequest.getExternalSessionData().getJoinInitErr().getFailedOnAllPlatforms())
        {
            return sessErr;
        }

        if (getExternalSessionUtilManager().doesCustomDataExceedMaxSize(masterRequest.getCreateRequest().getClientPlatformListOverride(), request.getGameCreationData().getExternalSessionCustomData().getCount()))
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_CUSTOM_DATA_TOO_LARGE;
        
        FindDedicatedServerRulesMap findDedicatedServerRulesMap;
        if (templateName != nullptr)
        {
            CreateGameTemplateMap::const_iterator templateIter = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().find(templateName);
            if (templateIter == getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end())
            {
                return GAMEMANAGER_ERR_INVALID_TEMPLATE_NAME;
            }

            CreateGameTemplate templateConfig = (*templateIter->second);

            templateConfig.getFindDedicatedServerRulesMap().copyInto(findDedicatedServerRulesMap);

            // overwrite desire value from template attributes
            for (auto& attrib : request.getDedicatedServerAttribs())
            {
                // Check if attribute exists in any matchmaking DS attrib rule
                const DedicatedServerAttributeRuleServerConfigMap& configMap = getConfig().getMatchmakerSettings().getRules().getDedicatedServerAttributeRules();
                for (const auto& configRule : configMap)
                {
                    if (blaze_stricmp(configRule.second->getAttributeName(), attrib.first) == 0)
                    {
                        // If found, update find rules map
                        FindDedicatedServerRulesMap::iterator findItr = findDedicatedServerRulesMap.find(configRule.first);
                        if (findItr != findDedicatedServerRulesMap.end())
                        {
                            findItr->second->setDesiredValue(attrib.second);
                            break;
                        }
                    }
                }
            }
        }  

        // Initialize the lists:
        GameIdsByFitScoreMap gamesToSend;
        BlazeRpcError dedServerError = findDedicatedServerHelper(this, gamesToSend,
            true, nullptr,
            masterRequest.getCreateRequest().getCommonGameData().getGameProtocolVersionString(),
            maxPlayerCapacity,
            masterRequest.getCreateRequest().getGameCreationData().getNetworkTopology(),
            templateName,
            (templateName != nullptr) ? &findDedicatedServerRulesMap : nullptr,
            hostSessionInfo,
            masterRequest.getCreateRequest().getClientPlatformListOverride());
        if (dedServerError != ERR_OK)
        {
            ERR_LOG("[resetDedicatedServerInternal] No available resettable dedicated servers found. Error (" << ErrorHelp::getErrorName(dedServerError) << ")");
            if (dedServerError == GAMEBROWSER_ERR_INVALID_CRITERIA)
                return GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
            return dedServerError;
        }

        JoinGameMasterResponse masterResponse;
        masterResponse.setJoinResponse(response);
        BlazeRpcError resetErr = GAMEMANAGER_ERR_NO_DEDICATED_SERVER_FOUND;
        // NOTE: fit score map is ordered by lowest score; therefore, iterate in reverse
        for (GameIdsByFitScoreMap::reverse_iterator gameIdsMapItr = gamesToSend.rbegin(), gameIdsMapEnd = gamesToSend.rend(); gameIdsMapItr != gameIdsMapEnd; ++gameIdsMapItr)
        {
            GameIdList* gameIds = gameIdsMapItr->second;
            // random walk gameIds
            while (!gameIds->empty())
            {
                GameIdList::iterator i = gameIds->begin() + Random::getRandomNumber((int32_t)gameIds->size());
                GameId gameId = *i;
                gameIds->erase_unsorted(i);

                masterRequest.setGameIdToReset(gameId);
                resetErr = getMaster()->resetDedicatedServerMaster(masterRequest, masterResponse);
                if (resetErr == Blaze::ERR_OK)
                    break;
                TRACE_LOG("[resetDedicatedServerInternal] Unable to reset dedicated server " << gameId << ", reason: " << ErrorHelp::getErrorName(resetErr));
            }

            if (resetErr == Blaze::ERR_OK)
                break;
        }

        if (resetErr != Blaze::ERR_OK)
        {
            return resetErr;
        }

        // Update to external session, the players which were updated by the blaze master call
        return createAndJoinExternalSession(masterResponse.getExternalSessionParams(), true, masterRequest.getExternalSessionData().getJoinInitErr(), masterResponse.getJoinResponse().getExternalSessionIdentification());
    }

    void GameManagerSlaveImpl::buildInternalPseudoGameConfigs()
    {
        // Here we generate a bunch of processed versions of the configs (normalized percent value lists, compressed inheritence)
        const PseudoGameConfigMap& pgConfigMap = getConfig().getPseudoGamesConfig().getPseudoGameConfigMap();

        PseudoGameConfigMap::const_iterator curConfig = pgConfigMap.begin();
        PseudoGameConfigMap::const_iterator endConfig = pgConfigMap.end();

        for (; curConfig != endConfig; ++curConfig)
        {
            const PseudoGameVariantName& variantName = curConfig->first;
            const PseudoGameVariantConfig* variantConfig = curConfig->second;
            
            // Check that the config is not already created (as it would be for a dependency):
            if (mProcessedPseudoGameConfigMap.find(variantName) == mProcessedPseudoGameConfigMap.end())
            {
                ProcessedPseudoGameConfig& processedConfig = mProcessedPseudoGameConfigMap[variantName];
                buildInternalPseudoGameConfig(variantConfig, processedConfig, pgConfigMap);
            }
        }
    }

    void GameManagerSlaveImpl::buildInternalPseudoGameConfig(const PseudoGameVariantConfig* variantConfig, ProcessedPseudoGameConfig& processedConfig, const PseudoGameConfigMap& pgConfigMap)
    {
        const char* baseConfigName = variantConfig->getBaseConfig();
        if (baseConfigName[0] != '\0')
        {
            if (mProcessedPseudoGameConfigMap.find(baseConfigName) == mProcessedPseudoGameConfigMap.end())
            {
                ProcessedPseudoGameConfig& baseProcessedConfig = mProcessedPseudoGameConfigMap[baseConfigName];
                PseudoGameConfigMap::const_iterator curConfig = pgConfigMap.find(baseConfigName);
                if (curConfig == pgConfigMap.end())
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to find base config " << baseConfigName << "." );
                    return;
                }
                const PseudoGameVariantConfig* baseVariantConfig = curConfig->second;
                buildInternalPseudoGameConfig(baseVariantConfig, baseProcessedConfig, pgConfigMap);
            }

            // Copy the base once: 
            mProcessedPseudoGameConfigMap[baseConfigName].mConfig.copyInto(processedConfig.mConfig);
        }

        // Configure the min/max counts:
        uint32_t minGames = variantConfig->getMinGameCount();
        uint32_t maxGames = variantConfig->getMaxGameCount();
        maxGames = eastl::max(minGames, maxGames);
        if (minGames != 0 || maxGames != 0)
        {
            processedConfig.mConfig.setMinGameCount(minGames);
            processedConfig.mConfig.setMaxGameCount(maxGames);
        }

        uint32_t minPlayers = variantConfig->getMinPlayerCount();
        uint32_t maxPlayers = variantConfig->getMaxPlayerCount();
        maxPlayers = eastl::max(minPlayers, maxPlayers);
        if (minPlayers != 0 || maxPlayers != 0)
        {
            // If only the Max player count was set, set the min to 1 so a game can be created. 
            if (minPlayers == 0)
                minPlayers = 1;

            processedConfig.mConfig.setMinPlayerCount(minPlayers);
            processedConfig.mConfig.setMaxPlayerCount(maxPlayers);
        }

        buildInternalPseudoGameConfigValues(variantConfig->getCreateGameValues(), processedConfig.mConfig.getCreateGameValues());
        buildInternalPseudoGameConfigValues(variantConfig->getPerPlayerValues(), processedConfig.mConfig.getPerPlayerValues());
    }

    void GameManagerSlaveImpl::buildInternalPseudoGameConfigValues(const VariableVariantValueMap& inputMap, VariableVariantValueMap& outputMap)
    {
        VariableVariantValueMap::const_iterator curAttr = inputMap.begin();
        VariableVariantValueMap::const_iterator endAttr = inputMap.end();
        for (; curAttr != endAttr; ++curAttr)
        {
            // Lookup attribute by name:
            const VariableName& varName = curAttr->first;
            const VariableVariant* varConfig = curAttr->second;

            // Copy the inputMap into the output map:
            if (outputMap.find(varName) == outputMap.end()) 
                outputMap[varName] = outputMap.allocate_element();

            VariableVariant& outputVariant = *outputMap[varName];
            if (!varConfig->getDefaults().empty())
            {
                outputVariant.getDefaults().clear();            // Clear any defaults set by the base: 
                varConfig->getDefaults().copyInto(outputVariant.getDefaults()); 

                // Normalize:
                float sumTotal = 0.0f;
                PercentValueList::iterator curValue = outputVariant.getDefaults().begin();
                PercentValueList::iterator endValue = outputVariant.getDefaults().end();
                for (; curValue != endValue; ++curValue)
                {
                    sumTotal += (*curValue)->getPercent();
                }
                   
                curValue = outputVariant.getDefaults().begin();
                for (; curValue != endValue; ++curValue)
                {
                    (*curValue)->setPercent((*curValue)->getPercent() / sumTotal);
                }
            }

            // Override defaults if set: 
            int64_t minInt = varConfig->getMinRangeInt();
            int64_t maxInt = varConfig->getMaxRangeInt();
            maxInt = eastl::max(minInt, maxInt);
            if (minInt != 0 || maxInt != 0)
            {
                outputVariant.setMinRangeInt(minInt);
                outputVariant.setMaxRangeInt(maxInt);
            }

            float minFloat = varConfig->getMinRangeFloat();
            float maxFloat = varConfig->getMaxRangeFloat();
            maxFloat = eastl::max(minFloat, maxFloat);
            if (minFloat != 0.0f || maxFloat != 0.0f)
            {
                outputVariant.setMinRangeFloat(minFloat);
                outputVariant.setMaxRangeFloat(maxFloat);
            }

            if (varConfig->getRangePrefix()[0] != '\0')
            {
                outputVariant.setRangePrefix(varConfig->getRangePrefix());
            }
        }
    }

    void GameManagerSlaveImpl::generatePseudoGamesThread(void)
    {
        if (!getConfig().getPseudoGamesConfig().getUsePseudoGamesConfig())
        {
            return;
        }

        Fiber::sleep(TimeValue(15*1000*1000), "Waiting to start the PG stuff");

        // Just have the 1st instance do this for now:
        InstanceIdList instances;
        getComponentInstances(instances, false);
        InstanceId foo = instances[0];
        InstanceId bar = getLocalInstanceId();

        if (instances.begin() == instances.end() || foo != bar)
        {
            return;
        }

        PseudoGameVariantCountMap emptyMap;
        startCreatePseudoGamesThread(emptyMap);
    }

    void GameManagerSlaveImpl::startCreatePseudoGamesThread(const PseudoGameVariantCountMap& pseudoVariantMap)
    {
        // Cancel any existing fiber:
        cancelCreatePseudoGamesThread();

        gFiberManager->scheduleCall<GameManagerSlaveImpl, const PseudoGameVariantCountMap&>(this, &GameManagerSlaveImpl::createPseudoGamesThread, pseudoVariantMap, Fiber::CreateParams(), &mPseudoGameCreateFiber);
    }

    void GameManagerSlaveImpl::cancelCreatePseudoGamesThread()
    {
        if (mPseudoGameCreateFiber.isValid())
        {
            Fiber::cancel(mPseudoGameCreateFiber, ERR_CANCELED);
            mPseudoGameCreateFiber.reset();
        }
    }

    void GameManagerSlaveImpl::createPseudoGamesThread(const PseudoGameVariantCountMap& pseudoVariantMapSrc)
    {
        PseudoGameVariantCountMap pseudoVariantMap;
        pseudoVariantMapSrc.copyInto(pseudoVariantMap);

        TimeValue startTime = TimeValue::getTimeOfDay();

        // Here we use the processed versions of the configs (normalized percent value lists, compressed inheritance)
        eastl::map<PseudoGameVariantName, ProcessedPseudoGameConfig>::const_iterator curConfig = mProcessedPseudoGameConfigMap.begin();
        eastl::map<PseudoGameVariantName, ProcessedPseudoGameConfig>::const_iterator endConfig = mProcessedPseudoGameConfigMap.end();

        uint32_t totalGames = 0;
        UserSession::SuperUserPermissionAutoPtr ptr(true);
        for (; curConfig != endConfig; ++curConfig)
        {
            const PseudoGameVariantName& variantName = curConfig->first;
            const PseudoGameVariantConfig& variantConfig = curConfig->second.mConfig;

            // Build a CreateGameRequest from the given input config: 
            uint32_t minGames = variantConfig.getMinGameCount();
            uint32_t maxGames = variantConfig.getMaxGameCount();
            maxGames = eastl::max(minGames, maxGames);
            uint32_t randGameCount = (minGames != maxGames) ? Random::getRandomNumber(maxGames-minGames)+minGames : minGames;
            

            // Check for values provided:
            if (!pseudoVariantMap.empty())
            {
                PseudoGameVariantCountMap::iterator iter = pseudoVariantMap.find(variantName);
                if (iter ==  pseudoVariantMap.end())
                {
                    // This config was not desired
                    continue;
                }

                if (iter->second != 0)
                {
                    randGameCount = iter->second;
                }
            }

            uint32_t totalGamesCurVariant = 0;
            TRACE_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Creating " << randGameCount << " pseudo games for config " << variantName.c_str() << "." );

            for (uint32_t curGame = 0; curGame < randGameCount; ++curGame)
            {
                CreateGameRequest createReq;
                if (!buildPseudoGameRequest(createReq, variantConfig))
                {
                    continue;
                }

                CreateGameResponse createResp;
                BlazeRpcError error = createPseudoGame(createReq, createResp);
                if (error == ERR_OK)
                {
                    ++totalGamesCurVariant;
                    TRACE_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Created pseudo game id: " << createResp.getGameId() << " with " << createReq.getPlayerJoinData().getPlayerDataList().size() << " players, from variant " << variantName.c_str() << "." );
                }
                else if (error == ERR_CANCELED)
                {
                    // Early out if canceled.
                    TRACE_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Cancelled pseudo game creation on variant " << variantName.c_str() << "." );
                    break;
                }
                else
                {
                    WARN_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Error (" << ErrorHelp::getErrorName(error) << ") creating pseudo game for variant " << variantName.c_str() << "." );
                }
            }
            totalGames += totalGamesCurVariant;
            TRACE_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Created a total of " << totalGamesCurVariant << " pseudo games from config " << variantName.c_str() << "." );
        }

        TimeValue totalTime = TimeValue::getTimeOfDay() - startTime;
        INFO_LOG("[GameManagerMasterImpl].generatePseudoGamesThread: Created a total of " << totalGames << " pseudo games from all configs. Took " << totalTime.getSec() << " seconds." );
    }


    bool GameManagerSlaveImpl::buildPseudoGameRequest(CreateGameRequest& outputReq, const PseudoGameVariantConfig& config)
    {
        // Build a CreateGameRequest from the given input config: 
        uint32_t minPlayers = config.getMinPlayerCount();
        uint32_t maxPlayers = config.getMaxPlayerCount();
        maxPlayers = eastl::max(minPlayers, maxPlayers);
        uint32_t randPlayerCount = (minPlayers != maxPlayers) ? Random::getRandomNumber(maxPlayers-minPlayers)+minPlayers : minPlayers;

        if (randPlayerCount == 0)
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: Random player count for game was 0 (min: "<< minPlayers << " max: "<< maxPlayers <<")." );
            return false;
        }

        if (!buildPseudoGameRequestValues(EA::TDF::TdfGenericReference(outputReq), config.getCreateGameValues()))
        {
            ERR_LOG("[GameManagerMasterImpl].onConfigure: Problem adding in the CreateGameValues from the config." );
            return false;
        }

        for (uint32_t i = 0; i < randPlayerCount; ++i)
        {
            PerPlayerJoinData& playerJoinData = *outputReq.getPlayerJoinData().getPlayerDataList().pull_back();
            if (!buildPseudoGameRequestValues(EA::TDF::TdfGenericReference(playerJoinData), config.getPerPlayerValues()))
            {
                ERR_LOG("[GameManagerMasterImpl].onConfigure: Problem adding in the PerPlayervalues from the config." );
                return false;
            }
            else
            {
                // Assign a dummy blaze id to each user:
                uint64_t randomId = Random::getRandomNumber(Random::MAX);
                playerJoinData.getUser().setBlazeId(randomId);
            }
        }
        // Assign a dummy ip address:
        outputReq.getCommonGameData().getPlayerNetworkAddress().setIpAddress(nullptr);
        return true;
    }

    bool GameManagerSlaveImpl::buildPseudoGameRequestValues(EA::TDF::TdfGenericReference criteriaRef, const VariableVariantValueMap& valueMap)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        VariableVariantValueMap::const_iterator curAttr = valueMap.begin();
        VariableVariantValueMap::const_iterator endAttr = valueMap.end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const VariableName& varName = curAttr->first;
            const VariableVariant* varConfig = curAttr->second;
            EA::TDF::TdfGenericReference criteriaAttr;
            if (!tdfFactory.getTdfMemberReference(criteriaRef, varName, criteriaAttr))
            {
                ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to find a TdfMemberReference for the attribute " << varName.c_str() << "." );
                return false;
            }

            // Calculate the value to use: 
            if (!varConfig->getDefaults().empty())
            {
                // This assumes the values are normalized already: 
                float valueFloat = Random::getRandomFloat();

                PercentValueList::const_iterator curValue = varConfig->getDefaults().begin();
                PercentValueList::const_iterator endValue = varConfig->getDefaults().end();
                for (; curValue != endValue; ++curValue)
                {
                    valueFloat -= (*curValue)->getPercent();
                    if (valueFloat <= 0)
                    {
                        if (!criteriaAttr.setValue((*curValue)->getValue().getValue()))
                        {
                            ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to set value for attribute " << varName.c_str() << " from config.");
                            return false;
                        }
                        break;
                    }
                }
            }
            else
            {
                // Range Calculations:
                int32_t minInt = (int32_t)varConfig->getMinRangeInt();
                int32_t maxInt = (int32_t)varConfig->getMaxRangeInt();
                maxInt = eastl::max(minInt, maxInt);
                int32_t rangeInt = Random::getRandomNumber(maxInt - minInt) + minInt;  // TODO: Max a 64-bit rand function (steal from FB or whatever)
                bool useRangeInt = (minInt != maxInt || minInt != 0);

                float minFloat = varConfig->getMinRangeFloat();
                float maxFloat = varConfig->getMaxRangeFloat();
                maxFloat = eastl::max(minFloat, maxFloat);
                float rangeFloat = Random::getRandomFloat(minFloat, maxFloat);
                bool useRangeFloat = (minFloat != maxFloat || minFloat != 0.0f);

                    
                if (criteriaAttr.isTypeString())
                {
                    eastl::string rangeString; 
                    if (useRangeInt)
                    {
                        rangeString.sprintf("%s%i", varConfig->getRangePrefix(), rangeInt);
                    }
                    else if (useRangeFloat)
                    {
                        rangeString.sprintf("%s%f", varConfig->getRangePrefix(), rangeFloat);
                    }
                    EA::TDF::TdfGenericValue stringVal;
                    stringVal.set(rangeString.c_str());
                    criteriaAttr.setValue(stringVal);
                }
                else if (criteriaAttr.isTypeInt() && useRangeInt)
                {
                    // Need to do a better version of this.  With conversion and whatnot.
                    EA::TDF::TdfGenericValue intVal(rangeInt);
                    if (!intVal.convertToIntegral(criteriaRef))
                    {
                        ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to convert integral for attribute " << varName.c_str() << ".");
                        return false;                        
                    }
                }
                else if (criteriaAttr.getType() == EA::TDF::TDF_ACTUAL_TYPE_FLOAT && useRangeFloat)
                {
                    criteriaAttr.setValue(EA::TDF::TdfGenericValue(rangeFloat));
                }
                else
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Missing valid defaults or valid range for attribute " << varName.c_str() << ".");
                    return false;
                }
            }
        }

        return true;
    }

    bool GameManagerSlaveImpl::validatePseudoGameConfig() const
    {
        const PseudoGameConfigMap& pgConfigMap = getConfig().getPseudoGamesConfig().getPseudoGameConfigMap();

        PseudoGameConfigMap::const_iterator curConfig = pgConfigMap.begin();
        PseudoGameConfigMap::const_iterator endConfig = pgConfigMap.end();
        for (; curConfig != endConfig; ++curConfig)
        {
            const PseudoGameVariantName& variantName = curConfig->first;
            const PseudoGameVariantConfig* variantConfig = curConfig->second;

            // Check for circular dependencies and existence in the map:
            eastl::set<const char*> heirarchy;
            heirarchy.insert(variantName.c_str());
            const char* baseConfigName = variantConfig->getBaseConfig();
            while (baseConfigName[0] != '\0')
            {
                bool didInsert = heirarchy.insert(baseConfigName).second;
                if (!didInsert)
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Circular dependency found for base config: " << baseConfigName << " inherited by config " << variantName.c_str() << "." );
                    return false;
                }

                PseudoGameConfigMap::const_iterator baseConfig = pgConfigMap.find(baseConfigName);
                if (baseConfig == endConfig)
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable for find base config: " << baseConfigName << " inherited by config " << variantName.c_str() << "." );
                    return false;
                }

                baseConfigName = baseConfig->second->getBaseConfig();
            } 
                
            // Verify that all the values exist for CreateGameRequest:
            CreateGameRequest outputReq;
            PerPlayerJoinData& playerJoinData = *outputReq.getPlayerJoinData().getPlayerDataList().pull_back();
            bool createGameResult = validatePseudoGameConfigValues(EA::TDF::TdfGenericReference(outputReq), variantConfig->getCreateGameValues());
            bool playerDataResult = validatePseudoGameConfigValues(EA::TDF::TdfGenericReference(playerJoinData), variantConfig->getPerPlayerValues());

            if (createGameResult == false && playerDataResult == false)
                return false;
        }

        return true;
    }

    bool GameManagerSlaveImpl::validatePseudoGameConfigValues(EA::TDF::TdfGenericReference criteriaRef,  const VariableVariantValueMap& valueMap) const
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        VariableVariantValueMap::const_iterator curAttr = valueMap.begin();
        VariableVariantValueMap::const_iterator endAttr = valueMap.end();
        for (; curAttr != endAttr; ++curAttr)
        {
            // Lookup attribute by name:
            const VariableName& varName = curAttr->first;
            const VariableVariant* varConfig = curAttr->second;
            EA::TDF::TdfGenericReference criteriaAttr;
            if (!tdfFactory.getTdfMemberReference(criteriaRef, varName, criteriaAttr))
            {
                ERR_LOG("[GameManagerMasterImpl].onConfigure: Unable to find a TdfMemberReference for the attribute " << varName.c_str() << "." );
                return false;
            }

            // Verify that every value has a valid value set:
            // (If the defaults were used, the config decoder will already verify that every Generic was set correctly)
            if (varConfig->getDefaults().empty())
            {
                // Range Calculations:
                int32_t minInt = (int32_t)varConfig->getMinRangeInt();
                int32_t maxInt = (int32_t)varConfig->getMaxRangeInt();
                maxInt = eastl::max(minInt, maxInt);
                bool useRangeInt = (minInt != maxInt || minInt != 0);

                float minFloat = varConfig->getMinRangeFloat();
                float maxFloat = varConfig->getMaxRangeFloat();
                maxFloat = eastl::max(minFloat, maxFloat);
                bool useRangeFloat = (minFloat != maxFloat || minFloat != 0.0f);

                if (!useRangeInt && !useRangeFloat)
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Attribute " << varName.c_str() << " is missing a int/float range and defaults." );
                    return false;
                }
                    
                if (!criteriaAttr.isTypeString() && 
                    !(criteriaAttr.isTypeInt() && useRangeInt) &&
                    !(criteriaAttr.getType() == EA::TDF::TDF_ACTUAL_TYPE_FLOAT && useRangeFloat) )
                {
                    ERR_LOG("[GameManagerMasterImpl].onConfigure: Missing valid defaults or valid range for attribute " << varName.c_str() << ".");
                    return false;
                }
            }
        }

        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief Reconfigure GameManager slave.
        - parse the game manager & game browser configuration sections and ensure no errors.
        - If there are no errors we set new configuration values.
        
        \param[out] true always.  In case of error, old configuration values are kept so the server
        keeps running in a known state.
    ***************************************************************************************************/
    bool GameManagerSlaveImpl::onReconfigure()
    {
        const GameManagerServerConfig& configTdf = getConfig();

        if (!mScenarioManager.reconfigure(configTdf))
        {
            ERR_LOG("[GameManagerSlave] Configuration problem while parsing Reconfigure() method of ScenarioManager.");
            return false;
        }

        // configure gamebrowser
        if (!mGameBrowser.reconfigure(configTdf, getConfig().getGameSession().getEvaluateGameProtocolVersionString()))
        {
            ERR_LOG("[GameManagerSlave] Reconfiguration problem while parsing Reconfigure() method of GameManagerSlaveImpl.cpp; ignoring new config settings.");
            return true;
        }

        mExternalDataManager.reconfigure(configTdf);
        mExternalSessionServices.onReconfigure(getConfig().getGameSession().getExternalSessions(), getConfig().getGameSession().getPlayerReservationTimeout());

        mExternalHostIpWhitelist = mExternalHostIpWhitelistStaging;
        mExternalHostIpWhitelistStaging.reset();

        mInternalHostIpWhitelist = mInternalHostIpWhitelistStaging;
        mInternalHostIpWhitelistStaging.reset();

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief a matchmaking session has failed; forward the notification to the client.
        Successfully matchmaking sessions will simply be added to the game that they create/find.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyMatchmakingFailed(const NotifyMatchmakingFailed& notifyMatchmakingFinished, UserSession*)
    {
        removeMMSessionForUser(notifyMatchmakingFinished.getUserSessionId(), notifyMatchmakingFinished.getSessionId());

        if (notifyMatchmakingFinished.getScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[onNotifyMatchmakingFailed] MatchmakingFailed with no Scenario Id provided.  Check for possible side-effects from old MMing removal." )
        }
        else
        {
            mScenarioManager.onNotifyMatchmakingFailed(notifyMatchmakingFinished);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief a matchmaking session has finished (success or failure); cleanup the tracking for the player.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyMatchmakingFinished(const NotifyMatchmakingFinished& notifyMatchmakingFinished, UserSession*)
    {
        removeMMSessionForUser(notifyMatchmakingFinished.getUserSessionId(), notifyMatchmakingFinished.getSessionId());

        // This just tracks the success result:
        if (notifyMatchmakingFinished.getScenarioId() != INVALID_SCENARIO_ID)
        {
            mScenarioManager.onNotifyMatchmakingFinished(notifyMatchmakingFinished);
        }
        else
        {
            ERR_LOG("[onNotifyMatchmakingFinished] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
    }

    /*! ************************************************************************************************/
    /*! \brief a matchmaking session has validated its connection. Update the cached mm connection criteria and forward the notification to the client.
        onNotifyMatchmakingFailed will still be dispatched as appropriate
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated& data, UserSession*)
    {
        if (gUserSessionManager->getSessionExists(data.getUserSessionId()))
        {
            upsertMatchmakingQosCriteriaForUser(data.getUserSessionId(), data.getConnectionValidatedResults());
        }

        if (data.getScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[onNotifyMatchmakingSessionConnectionValidated] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
        else
        {
            // This is here for the cases where we connect successfully:
            mScenarioManager.onNotifyMatchmakingSessionConnectionValidated(data);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief an async matchmaking status notification from the matchmaker slave
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyMatchmakingAsyncStatus(const NotifyMatchmakingAsyncStatus& data, UserSession*)
    {
        // MM slave can't update the FG status's game count properly so we do it on the GM slave
        NotifyMatchmakingAsyncStatus updatedStatus;
        data.copyInto(updatedStatus);

        if (data.getMatchmakingScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[onNotifyMatchmakingAsyncStatus] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
        else
        {
            mScenarioManager.onNotifyMatchmakingAsyncStatus(data);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief a notification to members of a mm session that they have entered matchmaking
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyRemoteMatchmakingStarted(const NotifyRemoteMatchmakingStarted& data, UserSession*)
    {
        // This callback will trigger when sent from the Scenarios manager, in addition to the passthru notification. 
        // We check for MMSessionId != 0 so that we don't double notify the clients.
        if (data.getMMSessionId() != INVALID_MATCHMAKING_SESSION_ID)
        {
            sendNotifyRemoteMatchmakingStartedToUserSessionById(data.getUserSessionId(), &data);
        }
        else
        {
            ERR_LOG("[onNotifyRemoteMatchmakingStarted] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
    }

    /*! ************************************************************************************************/
    /*! \brief a notification to members of a mm session that they mm session they were pulled into has ended
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onNotifyRemoteMatchmakingEnded(const NotifyRemoteMatchmakingEnded& data, UserSession*)
    {
        // This callback will trigger when sent from the Scenarios manager, in addition to the passthru notification. 
        // We check for MMSessionId != 0 so that we don't double notify the clients.
        if (data.getMMSessionId() != INVALID_MATCHMAKING_SESSION_ID)
        {
            sendNotifyRemoteMatchmakingEndedToUserSessionById(data.getUserSessionId(), &data);
        }
        else
        {
            ERR_LOG("[onNotifyRemoteMatchmakingEnded] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
    }

    void GameManagerSlaveImpl::onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess& notifyMatchmakingDebugged, UserSession*)
    {
        if (notifyMatchmakingDebugged.getScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[onNotifyMatchmakingPseudoSuccess] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }
        else
        {
            mScenarioManager.onNotifyMatchmakingPseudoSuccess(notifyMatchmakingDebugged);    
        }
        removeMMSessionForUser(notifyMatchmakingDebugged.getUserSessionId(), notifyMatchmakingDebugged.getSessionId());
    }

    void GameManagerSlaveImpl::onNotifyMatchmakingStatusUpdate(const Blaze::GameManager::MatchmakingStatus& data, UserSession *)
    {
        SPAM_LOG("[GameManagerSlave].onNotifyMatchmakingStatusUpdate Received status update from Matchmaker Slave instance id(" << data.getInstanceId() << ").");
        updateMatchmakingSlaveStatus(data);
    }

    /*! ************************************************************************************************/
    /*! \brief a user has either logged out of blaze, or they've been disconnected.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
    {
        mGameBrowser.onUserSessionExtinction(userSession.getUserSessionId());
        mScenarioManager.onUserSessionExtinction(userSession.getUserSessionId());
        removeMatchmakingQosCriteriaForUser(userSession.getUserSessionId());
    }

    void GameManagerSlaveImpl::onLocalUserSessionImported(const UserSessionMaster& userSession)
    {
        mScenarioManager.onLocalUserSessionImported(userSession);
    }

    void GameManagerSlaveImpl::onLocalUserSessionExported(const UserSessionMaster& userSession)
    {
        mScenarioManager.onLocalUserSessionExported(userSession);
    }

    void GameManagerSlaveImpl::onLocalUserSessionLogout(const UserSessionMaster& userSession)
    {
        // clients cannot catch logout/disconnects to clean up their primary session so do so here. side: no need to additionally check isOwnedByThisInstance here to avoid redundant calls, as GameManagerSlave should not be on aux slaves (only run on one type of instance).
        if (getExternalSessionUtilManager().hasUpdatePrimaryExternalSessionPermission(userSession.getExistenceData()))
        {
            // onLocalUserSessionLogout is on a non-blockable fiber so schedule a blockable one
            ClearPrimaryExternalSessionParametersPtr clearParams = BLAZE_NEW ClearPrimaryExternalSessionParameters();
            userSession.getUserInfo().filloutUserIdentification(clearParams->getUserIdentification());
            clearParams->getAuthInfo().setServiceName(userSession.getServiceName());
            //NOTE: actual populate of auth token done in blockable fiber below instead.

            eastl::string platformInfoStr;
            TRACE_LOG("[GameManagerSlaveImpl].onLocalUserSessionLogout: clearing primary session for user with blazeId " << clearParams->getUserIdentification().getBlazeId() << ", " << platformInfoToString(clearParams->getUserIdentification().getPlatformInfo(), platformInfoStr));
            Fiber::CreateParams params;
            params.blocking = true;
            params.namedContext = "GameManagerSlaveImpl::clearPrimaryExternalSessionForUser";
            gFiberManager->scheduleCall(this, &GameManagerSlaveImpl::clearPrimaryExternalSessionForUserPtr, clearParams, params);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief a user has regained its connectivity.
        Only user sessions that enable connectivity checking will get this event.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onLocalUserSessionResponsive(const UserSessionMaster& userSession)
    {
        INFO_LOG("[GameManagerSlave].onLocalUserSessionResponsive: user session id " << userSession.getUserSessionId());
        setUserResponsive(userSession, true);
    }

    /*! ************************************************************************************************/
    /*! \brief a user has lost its connectivity.
        Only user sessions that enable connectivity checking will get this event.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::onLocalUserSessionUnresponsive(const UserSessionMaster& userSession)
    {
        INFO_LOG("[GameManagerSlave].onLocalUserSessionUnresponsive: user session id " << userSession.getUserSessionId());
        setUserResponsive(userSession, false);
    }

    /*! ************************************************************************************************/
    /*! \brief Tell the master that a user is responsive or not.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::setUserResponsive(const UserSessionMaster& user, bool responsive) const
    {
        // cache data locally before blocking calls below
        UserSessionId userSessionId = user.getSessionId();
        const BlazeObjectIdList& list = user.getExtendedData().getBlazeObjectIdList();
        BlazeObjectIdList::const_iterator itr = list.begin();
        BlazeObjectIdList actionList;
        actionList.reserve(list.size());
        for(; itr != list.end(); ++itr)
        {
            if (((*itr).type == ENTITY_TYPE_GAME) || ((*itr).type == ENTITY_TYPE_GAME_GROUP))
            {
                actionList.push_back(*itr);
            }
        }

        for (itr = actionList.begin(); itr != actionList.end(); ++itr)
        {
            SetGameResponsiveRequest request;
            request.setGameId((*itr).id);
            request.setResponsive(responsive);
            BlazeRpcError err = getMaster()->setGameResponsiveMaster(request);
            if (err != ERR_OK)
            {
                WARN_LOG("[GameManagerSlave].setUserResponsive(" << userSessionId << "," << (responsive ? "true" : "false")
                    << "): setGameResponsiveMaster(" << request.getGameId() <<") returned " << ErrorHelp::getErrorName(err));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Gets the next unique session id for GB.
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::getNextUniqueGameBrowserSessionId(uint64_t& sessionId) const
    {
        BlazeRpcError error = gUniqueIdManager->getId(RpcMakeSlaveId(GameManagerSlave::COMPONENT_ID), GAMEMANAGER_IDTYPE_GAMEBROWSER_LIST, sessionId);

        if (error == ERR_OK)
        {
            //set the id so it routes to us in a sharded situation
            sessionId = BuildInstanceKey64(gController->getInstanceId(), sessionId);
        }

        return error;
    }

    /*! ************************************************************************************************/
    /*! \brief entry point for the census data component to poll the GameManager slave for census info.
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameManagerSlaveImpl::getCensusData(CensusDataByComponent& censusData)
    {
        GameManagerCensusData* gmCensusData = BLAZE_NEW GameManagerCensusData();
        BlazeRpcError err = getCensusDataInternal(*gmCensusData, false);
        if(err == ERR_OK)
        {
            if (censusData.find(getComponentId()) == censusData.end())
            {
                censusData[getComponentId()] = censusData.allocate_element();
            }
            censusData[getComponentId()]->set(gmCensusData);
        }
        return err;
    }

    Blaze::BlazeRpcError GameManagerSlaveImpl::getCensusDataInternal(GameManagerCensusData& gmCensusData, bool getAllData /*= true*/)
    {
        // Gather the gm data from all the masters:  (This gets all platform data)
        Component::InstanceIdList gmInstances;
        getMaster()->getComponentInstances(gmInstances, true);
        Component::MultiRequestResponseHelper<GameManagerCensusData, void> gmCensusResponses(gmInstances);
        if (!gmInstances.empty())
            getMaster()->sendMultiRequest(GameManagerMaster::CMD_INFO_GETCENSUSDATA, nullptr, gmCensusResponses);

        // Number user sessions logged in; it excludes dedicated server hosts and HTTP/gRPC users.
        // If your title supports multiple logins, each login is represented here.
        // common just gets the total number of gameplay users logged in
        uint32_t loggedSessions = gUserSessionManager->getGameplayUserSessionCount();
        gmCensusData.setNumOfLoggedSession(loggedSessions);

        // number of player sessions from GM masters
        for (auto& curCensusResponse : gmCensusResponses)
        {
            if (curCensusResponse->error != ERR_OK)
                continue;

            GameManagerCensusData& instanceData = (GameManagerCensusData&) *(curCensusResponse->response);

            gmCensusData.setNumOfActiveGame(gmCensusData.getNumOfActiveGame() + instanceData.getNumOfActiveGame());

            for (auto& it : instanceData.getNumOfActiveGamePerPingSite())
            {
                GameManagerCensusData::CountsPerPingSite::iterator censusIt = gmCensusData.getNumOfActiveGamePerPingSite().find(it.first);
                if (censusIt == gmCensusData.getNumOfActiveGamePerPingSite().end())
                    censusIt = gmCensusData.getNumOfActiveGamePerPingSite().insert(eastl::make_pair(it.first, 0)).first;
                censusIt->second += it.second;
            }

            for (auto& it : instanceData.getNumOfJoinedPlayerPerPingSite())
            {
                GameManagerCensusData::CountsPerPingSite::iterator censusIt = gmCensusData.getNumOfJoinedPlayerPerPingSite().find(it.first);
                if (censusIt == gmCensusData.getNumOfJoinedPlayerPerPingSite().end())
                    censusIt = gmCensusData.getNumOfJoinedPlayerPerPingSite().insert(eastl::make_pair(it.first, 0)).first;
                censusIt->second += it.second;
            }

            gmCensusData.setNumOfJoinedPlayer(gmCensusData.getNumOfJoinedPlayer() + instanceData.getNumOfJoinedPlayer());
            gmCensusData.setNumOfGameGroup(gmCensusData.getNumOfGameGroup() + instanceData.getNumOfGameGroup());
            gmCensusData.setNumOfPlayersInGameGroup(gmCensusData.getNumOfPlayersInGameGroup() + instanceData.getNumOfPlayersInGameGroup());

            //Go through each list and combine them
            for (auto& instanceAttribItr : instanceData.getGameAttributesData())
            {
                bool found = false;
                for (GameManagerCensusData::GameAttributeCensusDataList::iterator gmAttribItr = gmCensusData.getGameAttributesData().begin(),
                    gmAttribEnd = gmCensusData.getGameAttributesData().end(); gmAttribItr != gmAttribEnd; ++gmAttribItr)
                {
                    if (blaze_strcmp((instanceAttribItr)->getAttributeName(), (*gmAttribItr)->getAttributeName()) == 0 &&
                        blaze_strcmp((instanceAttribItr)->getAttributevalue(), (*gmAttribItr)->getAttributevalue()) == 0)
                    {
                        found = true;
                        (*gmAttribItr)->setNumOfGames((*gmAttribItr)->getNumOfGames() + (instanceAttribItr)->getNumOfGames());
                        (*gmAttribItr)->setNumOfPlayers((*gmAttribItr)->getNumOfPlayers() + (instanceAttribItr)->getNumOfPlayers());
                    }
                }

                if (!found)
                {
                    GameAttributeCensusData* ad = gmCensusData.getGameAttributesData().pull_back();
                    (instanceAttribItr)->copyInto(*ad);
                }
            }
        }

        // The estimator that will collate data from both, packer and mmSlave instances
        Blaze::Matchmaker::TimeToMatchEstimator timeToMatchEstimator;

        uint32_t matchmakingSessionCount = 0;
        uint32_t matchmakingUserCount = 0;
        bool firstInstanceResponse = true;

        // Get packer data
        auto* packerMasterComponent = getScenarioManager().getGamePackerMaster();
        if (packerMasterComponent != nullptr)
        {
            Component::InstanceIdList packerMasterInstances;
            packerMasterComponent->getComponentInstances(packerMasterInstances, false);
            if (!packerMasterInstances.empty())
            {
                Component::MultiRequestResponseHelper<GameManager::PackerInstanceStatusResponse, void> packerStatusResponses(packerMasterInstances);
                packerMasterComponent->sendMultiRequest(Blaze::GamePacker::GamePackerMaster::CMD_INFO_GETPACKERINSTANCESTATUS, nullptr, packerStatusResponses);

                for (auto& statusResponse : packerStatusResponses)
                {
                    auto& packerStatusResponse = (GameManager::PackerInstanceStatusResponse&) *(statusResponse->response);

                    matchmakingSessionCount += (uint32_t)packerStatusResponse.getMatchmakingSessionsCount();
                    matchmakingUserCount += (uint32_t)packerStatusResponse.getMatchmakingUsersCount();

                    // For the first one, just grab the values and fill in the estimator.
                    if (firstInstanceResponse)
                    {
                        timeToMatchEstimator.initializeTtmEstimates(packerStatusResponse);
                        firstInstanceResponse = false;
                    }
                    else // otherwise just average them as before:
                    {
                        timeToMatchEstimator.collateMatchmakingCensusData(packerStatusResponse.getGlobalTimeToMatchEstimateData(),
                            packerStatusResponse.getScenarioTimeToMatchEstimateData(), getConfig().getScenariosConfig());
                    }
                }
            }
        }

        // Likewise, get all the mmSlave data:
        Blaze::Matchmaker::MatchmakerSlave* mmComponent = (Blaze::Matchmaker::MatchmakerSlave*) gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true);
        if (mmComponent != nullptr)
        {
            Component::InstanceIdList mmInstances;
            mmComponent->getComponentInstances(mmInstances, false);
            Component::MultiRequestResponseHelper<MatchmakingInstanceStatusResponse, void> mmStatusResponses(mmInstances);
            if (!mmInstances.empty())
                mmComponent->sendMultiRequest(Blaze::Matchmaker::MatchmakerSlave::CMD_INFO_GETMATCHMAKINGINSTANCESTATUS, nullptr, mmStatusResponses);

            for (auto& curStatusResponse : mmStatusResponses)
            {
                if (curStatusResponse->error != ERR_OK)
                    continue;

                MatchmakingInstanceStatusResponse& mmStatus = (MatchmakingInstanceStatusResponse&) *(curStatusResponse->response);

                matchmakingSessionCount += mmStatus.getNumOfMatchmakingSessions();
                matchmakingUserCount += mmStatus.getNumOfMatchmakingUsers();
                // update TTM estimates if enabled
                if (getConfig().getCensusData().getIncludeTimeToMatchEstimates() || getAllData)
                {
                    // For the first one, just grab the values and fill in the estimator.
                    if (firstInstanceResponse)
                    {
                        timeToMatchEstimator.initializeTtmEstimates(mmStatus);
                        firstInstanceResponse = false;
                    }
                    else // otherwise just average them as before:
                    {
                        timeToMatchEstimator.collateMatchmakingCensusData(mmStatus.getGlobalTimeToMatchEstimateData(),
                            mmStatus.getScenarioTimeToMatchEstimateData(), getConfig().getScenariosConfig());
                    }

                    // Scenario specific data based on every configured scenario
                    for (auto& scenarioConfigIter : getConfig().getScenariosConfig().getScenarios())
                    {
                        const ScenarioName& scenarioName = scenarioConfigIter.first;
                        auto outScenarioIter = gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData().find(scenarioName);
                        if (outScenarioIter == gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData().end())
                        {
                            auto outScenarioIterElem = gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData().allocate_element();
                            outScenarioIterElem->setScenarioName(scenarioName);  // Not sure why this is a member since the map already tracks it, but w/e. 
                            gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData()[scenarioName] = outScenarioIterElem;
                            outScenarioIter = gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData().find(scenarioName);
                        }
                        ScenarioMatchmakingCensusData& scenarioCensusData = *outScenarioIter->second;

                        // Fill out the ping site estimates 
                        for (auto& pingSiteName : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
                        {
                            const PingSiteAlias& pingSiteAlias = pingSiteName.first;
                            float totalMatchmakingRate = 0;

                            auto scenarioIter = mmStatus.getPlayerMatchmakingRateByScenarioPingsiteGroup().find(scenarioName);
                            if (scenarioIter != mmStatus.getPlayerMatchmakingRateByScenarioPingsiteGroup().end())
                            {
                                PlayerMatchmakingRateByPingsiteGroup& rateBySiteGroup = *scenarioIter->second;
                                auto pingSiteIter = rateBySiteGroup.find(pingSiteAlias);
                                if (pingSiteIter != rateBySiteGroup.end())
                                {
                                    for (auto groupIter : *pingSiteIter->second)
                                    {
                                        float matchmakingRate = groupIter.second;

                                        // Add the PingSite + Scenario PMR:
                                        auto pmrElement = scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup().allocate_element();
                                        (*pmrElement)[groupIter.first] += matchmakingRate;
                                        scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup()[pingSiteAlias] = pmrElement;

                                        totalMatchmakingRate += matchmakingRate;
                                    }
                                }
                            }

                            // Update Global PingSite PMR:
                            auto globalPsIter = scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup().find(pingSiteAlias);
                            if (globalPsIter == scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup().end())
                            {
                                auto pmrElement = scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup().allocate_element();
                                (*pmrElement)[""] = totalMatchmakingRate;
                                scenarioCensusData.getPlayerMatchmakingRatePerPingsiteGroup()[pingSiteAlias] = pmrElement;
                            }
                            else
                            {
                                (*globalPsIter->second)[""] += totalMatchmakingRate;
                            }

                            // Update the Global Scenario PMR:
                            scenarioCensusData.setGlobalPlayerMatchmakingRate(scenarioCensusData.getGlobalPlayerMatchmakingRate() + totalMatchmakingRate);
                        }

                        // Fill in the number of matchmaking sessions running per scenario
                        auto matchItr = mmStatus.getScenarioMatchmakingData().find(scenarioName);
                        if (matchItr != mmStatus.getScenarioMatchmakingData().end())
                        {
                            MatchmamkingSessionsByPingSite& sessionsBySite = *matchItr->second;
                            sessionsBySite.copyInto(scenarioCensusData.getMatchmakingSessionPerPingSite());

                            uint32_t numSessions = 0;
                            for (auto& pingSiteItr : sessionsBySite)
                            {
                                numSessions += pingSiteItr.second;
                            }
                            scenarioCensusData.setNumOfMatchmakingSession(scenarioCensusData.getNumOfMatchmakingSession() + numSessions);
                        }

                    }
                }
            }
        }
        
        gmCensusData.setNumOfMatchmakingSession(matchmakingSessionCount);
        gmCensusData.setNumOfMatchmakingUsers(matchmakingUserCount);

        if (getConfig().getCensusData().getIncludeTimeToMatchEstimates() || getAllData)
        {
            timeToMatchEstimator.getMatchmakingCensusData(gmCensusData.getMatchmakingCensusData(), getConfig().getScenariosConfig());
        }

        // Convert this to the old style if required:  (Don't bother doing this for the Scenarios variant usage - getAllData)
        if ((getConfig().getCensusData().getIncludeTimeToMatchEstimates() && !getAllData) &&
            getConfig().getCensusData().getIncludeNonGroupedData() == true)
        {
            // If DelineationGroups are being ignored, we assume that there's only one entry per-group, so we just use the front(). 
            for (auto& ttmGroup : gmCensusData.getMatchmakingCensusData().getEstimatedTimeToMatchPerPingsiteGroup())
            {
                if (!ttmGroup.second->empty())
                    gmCensusData.getMatchmakingCensusData().getEstimatedTimeToMatchPerPingsite()[ttmGroup.first] = ttmGroup.second->begin()->second;
            }

            for (auto& pmrGroup : gmCensusData.getMatchmakingCensusData().getPlayerMatchmakingRatePerPingsiteGroup())
            {
                if (!pmrGroup.second->empty())
                    gmCensusData.getMatchmakingCensusData().getPlayerMatchmakingRatePerPingsite()[pmrGroup.first] = pmrGroup.second->begin()->second;
            }

            for (auto& scenarioCensus : gmCensusData.getMatchmakingCensusData().getScenarioMatchmakingData())
            {
                // If DelineationGroups are being ignored, we assume that there's only one entry per-group, so we just use the front(). 
                for (auto& ttmGroup : scenarioCensus.second->getEstimatedTimeToMatchPerPingsiteGroup())
                {
                    if (!ttmGroup.second->empty())
                        scenarioCensus.second->getEstimatedTimeToMatchPerPingsite()[ttmGroup.first] = ttmGroup.second->begin()->second;
                }

                for (auto& pmrGroup : scenarioCensus.second->getPlayerMatchmakingRatePerPingsiteGroup())
                {
                    if (!pmrGroup.second->empty())
                        scenarioCensus.second->getPlayerMatchmakingRatePerPingsite()[pmrGroup.first] = pmrGroup.second->begin()->second;
                }
            }
        }

        return ERR_OK; 
    }

    /*! ************************************************************************************************/
    /*! \brief entry point for the framework to collect StatusInfo for the GM slave (healthcheck)
    ***************************************************************************************************/
    void GameManagerSlaveImpl::getStatusInfo(ComponentStatus& componentStatus) const 
    {
        // populate game browser info
        mGameBrowser.getStatusInfo(componentStatus);

        // populate GameManager slave info
        char8_t buf[128];
        ComponentStatus::InfoMap &componentInfoMap = componentStatus.getInfo();

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mTotalClientRequestsUpdatedDueToPoorReputation.getTotal());
        componentInfoMap["GMTotalClientRequestsUpdatedDueToPoorReputation"]=buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mExternalSessionFailuresImpacting.getTotal());
        componentInfoMap["GMTotalExternalSessionFailuresImpacting"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mExternalSessionFailuresNonImpacting.getTotal());
        componentInfoMap["GMTotalExternalSessionFailuresNonImpacting"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mActiveMatchmakingSlaves.get());
        componentInfoMap["GMActiveMatchmakingSlaves"] = buf;

        // populate scenario info
        mScenarioManager.getStatusInfo(componentStatus);
    }

    BlazeRpcError GameManagerSlaveImpl::updateDedicatedServerOverride(PlayerId playerId, GameId gameId)
    {
        RedisError rc = REDIS_ERR_OK;

        if (gameId == INVALID_GAME_ID)
        {
            rc = mDedicatedServerOverrideMap.erase(playerId);
        }
        else
        {
            rc = mDedicatedServerOverrideMap.upsert(playerId, gameId);
        }

        if (rc != REDIS_ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].updateDedicatedServerOverride: failed to update playerId(" << playerId << ") with gameId(" << gameId << "). Redis error: " << RedisErrorHelper::getName(rc));
            return ERR_SYSTEM;
        }
        return ERR_OK;
    }

    /////////////////  UserSetProvider interface functions  /////////////////////////////
    BlazeRpcError GameManagerSlaveImpl::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
    {
        Search::GetGamesResponse resp;
        BlazeRpcError result = getFullGameDataForBlazeObjectId(bobjId, resp);
        if (result == ERR_OK && !resp.getFullGameData().empty())
        {
            ListGameData& gameData = *resp.getFullGameData().front();
            if (!gameData.getGameRoster().empty())
                ids.reserve(gameData.getGameRoster().size());
                
            for (ReplicatedGamePlayerList::const_iterator itr = gameData.getGameRoster().begin(), end = gameData.getGameRoster().end(); itr != end; ++itr)
            {
                ids.push_back((*itr)->getPlayerSessionId());
            }
        }

        return result;
    }

    BlazeRpcError GameManagerSlaveImpl::getFullGameDataForBlazeObjectId(const EA::TDF::ObjectId& bobjId, Search::GetGamesResponse& resp)
    {
        BlazeRpcError result = ERR_OK;

        // pre: GameId's are unique across game sessions and game groups
        if ((bobjId.type == ENTITY_TYPE_GAME) || (bobjId.type == ENTITY_TYPE_GAME_GROUP))
        {
            Search::GetGamesRequest req;
            req.getGameIds().push_back(bobjId.id);
            req.setFetchOnlyOne(true);
            req.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);
            result = fetchGamesFromSearchSlaves(req, resp);
        }
        else
        {
            EA_FAIL_MSG("Attempted to lookup non-game id in game userset lookup.");
            result = ERR_SYSTEM;
        }

        return result;
    }

    BlazeRpcError GameManagerSlaveImpl::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
    {
        Search::GetGamesResponse resp;
        BlazeRpcError result = getFullGameDataForBlazeObjectId(bobjId, resp);
        if (result == ERR_OK && !resp.getFullGameData().empty())
        {
            ListGameData& gameData = *resp.getFullGameData().front();
            if (!gameData.getGameRoster().empty())
                ids.reserve(gameData.getGameRoster().size());

            for (ReplicatedGamePlayerList::const_iterator itr = gameData.getGameRoster().begin(), end = gameData.getGameRoster().end(); itr != end; ++itr)
            {
                BlazeId blazeId = gUserSessionManager->getBlazeId((*itr)->getPlayerSessionId());
                if (blazeId != INVALID_BLAZE_ID)
                {
                    ids.push_back(blazeId);
                }
            }
        }

        return result;
    }

    BlazeRpcError GameManagerSlaveImpl::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
    {
        Search::GetGamesResponse resp;
        BlazeRpcError result = getFullGameDataForBlazeObjectId(bobjId, resp);
        if (result == ERR_OK && !resp.getFullGameData().empty())
        {
            ListGameData& gameData = *resp.getFullGameData().front();
            if (!gameData.getGameRoster().empty())
                ids.reserve(gameData.getGameRoster().size());

            for (ReplicatedGamePlayerList::const_iterator itr = gameData.getGameRoster().begin(), end = gameData.getGameRoster().end(); itr != end; ++itr)
            {
                UserIdentificationPtr userIdent = ids.pull_back();
                (*itr)->getPlatformInfo().copyInto(userIdent->getPlatformInfo());
                userIdent->setBlazeId((*itr)->getPlayerId());
                userIdent->setExternalId(getExternalIdFromPlatformInfo((*itr)->getPlatformInfo()));
                userIdent->setName((*itr)->getPlayerName());
                userIdent->setAccountLocale((*itr)->getAccountLocale());
                userIdent->setAccountCountry((*itr)->getAccountCountry());
                userIdent->setPersonaNamespace((*itr)->getPersonaNamespace());
            }
        }

        return result;
    }

    BlazeRpcError GameManagerSlaveImpl::countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
    {
        count = 0;
        Search::GetGamesResponse resp;
        BlazeRpcError result = getFullGameDataForBlazeObjectId(bobjId, resp);
        if (result == ERR_OK && !resp.getFullGameData().empty())
        {
            count = resp.getFullGameData().front()->getGameRoster().size();
        }
        return result;
    }

    BlazeRpcError GameManagerSlaveImpl::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
    {
        count = 0;
        Search::GetGamesResponse resp;
        BlazeRpcError result = getFullGameDataForBlazeObjectId(bobjId, resp);
        if (result == ERR_OK && !resp.getFullGameData().empty())
        {
            //game can't have the same user twice, so this is safe for now
            count = resp.getFullGameData().front()->getGameRoster().size();
        }
        return result;
    }


    /*! ************************************************************************************************/
    /*! \brief updates load information for given matchmaker slave
    ***************************************************************************************************/
    void GameManagerSlaveImpl::updateMatchmakingSlaveStatus(const Blaze::GameManager::MatchmakingStatus& data)
    {
        data.copyInto(mMatchmakingShardingInformation.mStatusMap[data.getInstanceId()]);
    }

    BlazeRpcError GameManagerSlaveImpl::fetchGamesFromSearchSlaves(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse& response) const
    {
        if (request.getGameIds().size() + request.getPersistedGameIdList().size() + request.getBlazeIds().size()
            > getConfig().getGameBrowser().getMaxGameListSyncSize())
        {
            return GAMEBROWSER_ERR_EXCEED_MAX_SYNC_SIZE;
        }

        BlazeRpcError result = Blaze::ERR_OK;
        RpcCallOptions opts;

        Component* searchComponent = gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, false, true, &result);

        if (result == ERR_OK && searchComponent != nullptr)
        {
            bool needOnlyOne = request.getFetchOnlyOne();
            response.setResponseType(request.getResponseType());

            bool found = false;
            
            Blaze::Search::SearchShardingBroker::SlaveInstanceIdList instances;

            mGameBrowser.getFullCoverageSearchSet(instances);

            Component::MultiRequestResponseHelper<Blaze::Search::GetGamesResponse, void> responses(instances);
            result = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_GETGAMES, &request, responses);

            if (result == ERR_OK)
            {
                for (Component::MultiRequestResponseList::const_iterator itr = responses.begin(), end = responses.end(); (!needOnlyOne || !found) && itr != end; ++itr)
                {
                    if ((*itr)->error == ERR_OK)
                    {
                        const Blaze::Search::GetGamesResponse& instanceResponse = (const Blaze::Search::GetGamesResponse&) *((*itr)->response);

                        switch(instanceResponse.getResponseType())
                        {
                        case Blaze::Search::GET_GAMES_RESPONSE_TYPE_FULL:
                            {
                                for (Blaze::Search::GetGamesResponse::ListGameDataList::const_iterator gitr = instanceResponse.getFullGameData().begin(), 
                                    gend = instanceResponse.getFullGameData().end();  (!needOnlyOne || !found) && gitr != gend; ++gitr)
                                {
                                    Blaze::GameManager::ListGameData* data = response.getFullGameData().pull_back();
                                    (*gitr)->copyInto(*data);
                                    found = true;
                                }
                            }
                            break;
                        case Blaze::Search::GET_GAMES_RESPONSE_TYPE_GAMEBROWSER:
                            {
                                for (Blaze::Search::GetGamesResponse::GameBrowserGameDataList::const_iterator gitr = instanceResponse.getGameBrowserData().begin(), 
                                    gend = instanceResponse.getGameBrowserData().end();  (!needOnlyOne || !found) && gitr != gend; ++gitr)
                                {
                                    Blaze::GameManager::GameBrowserGameData* data = response.getGameBrowserData().pull_back();
                                    (*gitr)->copyInto(*data);
                                    found = true;

                                }
                            }
                            break;
                        case Blaze::Search::GET_GAMES_RESPONSE_TYPE_ID:
                            {
                                for (Blaze::GameManager::GameIdList::const_iterator gitr = instanceResponse.getGameIdList().begin(), 
                                    gend = instanceResponse.getGameIdList().end(); (!needOnlyOne || !found) && gitr != gend; ++gitr)
                                {
                                    response.getGameIdList().push_back(*gitr);
                                    found = true;
                                }
                            }
                            break;
                        }    
                    }                   
                }
            }            
        }

        return Blaze::Search::SearchShardingBroker::convertSearchComponentErrorToGameManagerError(result);
    }

    /*! ************************************************************************************************/
    /*! \brief fetch external session related data for the specified game.
        \param[out] externalSessionIdentification populates the session identification.
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::fetchExternalSessionDataFromSearchSlaves(GameId gameId,
        ExternalSessionIdentification& externalSessionIdentification) const
    {
        Blaze::Search::GetGamesRequest ggReq;
        Blaze::Search::GetGamesResponse ggResp;
        ggReq.getGameIds().push_back(gameId);
        ggReq.setFetchOnlyOne(true);
        ggReq.setResponseType(Blaze::Search::GET_GAMES_RESPONSE_TYPE_FULL);

        BlazeRpcError lookupUserGameError = fetchGamesFromSearchSlaves(ggReq, ggResp);
        if (lookupUserGameError != ERR_OK)
            return lookupUserGameError;
        if (ggResp.getFullGameData().empty())
        {
            ERR_LOG("[GameManagerSlaveImpl].fetchGameExternalSessionDataFromSearchSlaves could not find the game " << gameId << ". Unable to retrieve its external session data.");
            return GAMEMANAGER_ERR_INVALID_GAME_ID;
        }

        ggResp.getFullGameData().front()->getGame().getExternalSessionIdentification().copyInto(externalSessionIdentification);
        return ERR_OK;
    }

    void GameManagerSlaveImpl::addMMSessionForUser(UserSessionId userSessionId, Rete::ProductionId sessionId)
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            UserSessionMMStateDataPtr stateData = userSession->getCustomStateTdf<UserSessionMMStateData>(GameManagerSlave::COMPONENT_ID);
            if (stateData == nullptr)
                stateData = BLAZE_NEW UserSessionMMStateData();

            MatchmakingSessionSlaveIdList& slaveIdList = stateData->getMatchmakingSessionSlaveIdList();
            MatchmakingSessionSlaveIdList::iterator it = eastl::find(slaveIdList.begin(), slaveIdList.end(), sessionId);
            if (it == slaveIdList.end())
                slaveIdList.push_back(sessionId);

            userSession->upsertCustomStateTdf(COMPONENT_ID, *stateData);
        }
    }
    
    
    void GameManagerSlaveImpl::removeMMSessionForUser(UserSessionId userSessionId, Rete::ProductionId sessionId)
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            UserSessionMMStateDataPtr stateData = userSession->getCustomStateTdf<UserSessionMMStateData>(GameManagerSlave::COMPONENT_ID);
            if (stateData != nullptr)
            {
                MatchmakingSessionSlaveIdList& slaveIdList = stateData->getMatchmakingSessionSlaveIdList();
                MatchmakingSessionSlaveIdList::iterator it = eastl::find(slaveIdList.begin(), slaveIdList.end(), sessionId);
                if (it != slaveIdList.end())
                {
                    slaveIdList.erase_unsorted(it);
                    userSession->upsertCustomStateTdf(COMPONENT_ID, *stateData);
                }
            }
        }
    }

    void GameManagerSlaveImpl::incMMSessionReqForUser(UserSessionId userSessionId)
    {
        ++(mUserSessionMMSessionReqCountMap.insert(eastl::make_pair(userSessionId, 0)).first->second);
    }

    void GameManagerSlaveImpl::decMMSessionReqForUser(UserSessionId userSessionId)
    {
        UserSessionMMSessionReqCountMap::iterator it = mUserSessionMMSessionReqCountMap.find(userSessionId);
        if (it != mUserSessionMMSessionReqCountMap.end())
        {
            if (it->second > 1)
                --it->second;
            else
                mUserSessionMMSessionReqCountMap.erase(it);
        }
    }

    const PlayerQosValidationDataPtr GameManagerSlaveImpl::getMatchmakingQosCriteriaForUser(UserSessionId userSessionId, GameNetworkTopology networkTopology)
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            UserSessionMMStateDataPtr stateData = userSession->getCustomStateTdf<UserSessionMMStateData>(GameManagerSlave::COMPONENT_ID);
            if (stateData != nullptr)
            {
                MatchmakingQosDataMap::iterator it = stateData->getMatchmakingQosDataMap().find(networkTopology);
                if (it != stateData->getMatchmakingQosDataMap().end())
                    return it->second;
            }
        }

        return nullptr;
    }

    void GameManagerSlaveImpl::upsertMatchmakingQosCriteriaForUser(UserSessionId userSessionId, const ConnectionValidationResults& validationData)
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            UserSessionMMStateDataPtr stateData = userSession->getCustomStateTdf<UserSessionMMStateData>(GameManagerSlave::COMPONENT_ID);
            if (stateData == nullptr)
                stateData = BLAZE_NEW UserSessionMMStateData();

            PlayerQosValidationDataPtr& dataPtr = stateData->getMatchmakingQosDataMap()[validationData.getNetworkTopology()];
            if (dataPtr == nullptr)
                dataPtr = stateData->getMatchmakingQosDataMap().allocate_element();

            // matchmaker owns the data, so we clear and replace everything
            dataPtr->getAvoidedGameList().clear();
            dataPtr->getAvoidedPlayerList().clear();
            dataPtr->getAvoidedGameList().assign(validationData.getAvoidGameIdList().begin(), validationData.getAvoidGameIdList().end());
            dataPtr->getAvoidedPlayerList().assign(validationData.getAvoidPlayerIdList().begin(), validationData.getAvoidPlayerIdList().end());
            dataPtr->setCriteriaTier(validationData.getTier());
            dataPtr->setFailureCount(validationData.getFailCount());

            userSession->upsertCustomStateTdf(COMPONENT_ID, *stateData);
        }
    }

    void GameManagerSlaveImpl::removeMatchmakingQosCriteriaForUser(UserSessionId userSessionId)
    {
        UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(userSessionId);
        if (userSession != nullptr)
        {
            UserSessionMMStateDataPtr stateData = userSession->getCustomStateTdf<UserSessionMMStateData>(GameManagerSlave::COMPONENT_ID);
            if ((stateData != nullptr) && !stateData->getMatchmakingQosDataMap().empty())
            {
                stateData->getMatchmakingQosDataMap().clear();
                userSession->upsertCustomStateTdf(COMPONENT_ID, *stateData);
            }
        }
    }


    void GameManagerSlaveImpl::removePlayersFromGame(GameId gameId, bool joinLeader, const JoinExternalSessionParameters& params, const ExternalSessionApiError& apiErr)
    {
        for (const auto& platformErr : apiErr.getPlatformErrorMap())
        {
            incExternalSessionFailureImpacting(platformErr.first);

            ERR_LOG("[GameManagerSlaveImpl].removePlayersFromGame: Failed to join external session " << toLogStr(params.getSessionIdentification()) << "(" << gameId
                << "), error (" << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr()) << "), platform(" << ClientPlatformTypeToString(platformErr.first) << ")"
                << ", users (" << platformErr.second->getUsers() << ")");

            UserSessionIdList sessionIds;
            Blaze::GameManager::userIdentificationListToUserSessionIdList(platformErr.second->getUsers(), sessionIds);

            LeaveGameByGroupMasterRequest leaveRequest;
            leaveRequest.setGameId(gameId);
            leaveRequest.setPlayerRemovedReason(PLAYER_JOIN_EXTERNAL_SESSION_FAILED);
            leaveRequest.setLeaveLeader(joinLeader);
            sessionIds.copyInto(leaveRequest.getSessionIdList());

            LeaveGameByGroupMasterResponse leaveResponse;
            BlazeRpcError leaveErr = getMaster()->leaveGameByGroupMaster(leaveRequest, leaveResponse);
            if (ERR_OK != leaveErr)
            {
                ERR_LOG("[GameManagerSlaveImpl].removePlayersFromGame: Cleanup after failed external session update, failed to leave Blaze game with error " << ErrorHelp::getErrorName(leaveErr));
            }

            // clean up any external session members that should not be there, as needed
            if (shouldResyncExternalSessionMembersOnJoinError(platformErr.second->getBlazeRpcErr()))
            {
                Blaze::GameManager::ResyncExternalSessionMembersRequest resyncRequest;
                resyncRequest.setGameId(gameId);
                BlazeRpcError resyncErr = getMaster()->resyncExternalSessionMembers(resyncRequest);
                if (ERR_OK != resyncErr)
                {
                    ERR_LOG("[GameManagerSlaveImpl].removePlayersFromGame: Cleanup after failed external session update, failed to resync Blaze game(" << gameId << ")'s external session members with error " << ErrorHelp::getErrorName(resyncErr));
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Creates the game's external session if it doesn't exist. joins the specified users to it.
        \param[in] externalUserInfos - infos of users to join (including the caller). Pre: permissions checked as needed.
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::createAndJoinExternalSession(const JoinExternalSessionParameters& params, bool isResetDedicatedServer, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionIdentification& sessionIdentification)
    {
        GameId gameId = params.getExternalSessionCreationInfo().getExternalSessionId();

        ExternalSessionApiError apiErr;
        ExternalSessionResult result;
        BlazeRpcError sessErr = mExternalSessionServices.join(params, &result, true, joinInitErr, apiErr);
        if (ERR_OK != sessErr)
        {
            if (apiErr.getFailedOnAllPlatforms())
            {
                BlazeRpcError leaveErr = ERR_OK;
                if (isResetDedicatedServer)
                {
                    ReturnDedicatedServerToPoolRequest returnPoolRequest;
                    returnPoolRequest.setGameId(gameId);
                    leaveErr = returnDedicatedServerToPool(returnPoolRequest);
                }
                else
                {
                    UserSession::SuperUserPermissionAutoPtr permissionPtr(true);
                    DestroyGameRequest destroyGameRequest;
                    destroyGameRequest.setGameId(gameId);
                    destroyGameRequest.setDestructionReason(SYS_CREATION_FAILED);
                    DestroyGameResponse destroyGameResponse;
                    leaveErr = destroyGame(destroyGameRequest, destroyGameResponse);
                }
                if (ERR_OK != leaveErr)
                {
                    ERR_LOG("[GameManagerSlaveImpl].createAndJoinExternalSession: Cleanup after failed external session update, failed to leave Blaze game with error " << ErrorHelp::getErrorName(leaveErr));
                }
                
                return (convertExternalServiceErrorToGameManagerError(apiErr.getSinglePlatformBlazeRpcErr() != ERR_OK ? apiErr.getSinglePlatformBlazeRpcErr() : GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR));
            }
            else
            {
                removePlayersFromGame(gameId, true, params, apiErr);
            }
        }
        
        result.getSessionIdentification().copyInto(sessionIdentification);
        return ERR_OK;
    }

   
    /*! ************************************************************************************************/
    /*! \brief joins the specified users to the game's external session.
        \param[in] externalUserInfos - infos of users to join (including the caller). Pre: permissions checked as needed.
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::joinExternalSession(const JoinExternalSessionParameters& params, bool joinLeader, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionIdentification& sessionIdentification)
    {
        if (!validateExternalSessionParams(params))
            return ERR_SYSTEM;
        
        GameId gameId = params.getExternalSessionCreationInfo().getExternalSessionId();

        ExternalSessionApiError apiErr;
        ExternalSessionResult result;
        BlazeRpcError sessErr = mExternalSessionServices.join(params, &result, true, joinInitErr, apiErr);
        if (ERR_OK != sessErr)
        {
            removePlayersFromGame(gameId, joinLeader, params, apiErr);
        }
        
        if (apiErr.getFailedOnAllPlatforms())
            return (convertExternalServiceErrorToGameManagerError(apiErr.getSinglePlatformBlazeRpcErr() != ERR_OK ? apiErr.getSinglePlatformBlazeRpcErr() : GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR));
        else
        {
            result.getSessionIdentification().copyInto(sessionIdentification);
            return ERR_OK;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief leaves the specified id's from the game's external session.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::leaveExternalSession(LeaveGroupExternalSessionParametersPtr params)
    {
        if (params != nullptr)
        {
             ExternalSessionApiError apiErr;
             BlazeRpcError sessErr = mExternalSessionServices.leave(*params, apiErr);
             if (sessErr != ERR_OK)
             {
                 for (const auto& platformErr : apiErr.getPlatformErrorMap())
                 {
                     incExternalSessionFailureNonImpacting(platformErr.first);
                     //no Blaze cleanup on external session fail. Rely on external session to clean itself up by inactivity to keep things in sync.
                     WARN_LOG("[GameManagerSlaveImpl].leaveExternalSession: Failed to leave group from external session " << toLogStr(params->getSessionIdentification())
                         << ", error " << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr()) << ", platform(" << ClientPlatformTypeToString(platformErr.first) << ")");
                 }
             }
        }
        else
        {
            ERR_LOG("[GameManagerSlaveImpl].leaveExternalSession: Failed to leave group - nullptr params");
        }
    }

    /*! \brief if users will join an external session validate check params are valid for a GameSession */
    bool GameManagerSlaveImpl::validateExternalSessionParams(const JoinExternalSessionParameters& params) const
    {
        if (!params.getExternalUserJoinInfos().empty() && ((params.getExternalSessionCreationInfo().getExternalSessionType() == EXTERNAL_SESSION_MATCHMAKING_SESSION)
            || (params.getExternalSessionCreationInfo().getExternalSessionId() == INVALID_GAME_ID)))
        {
            ERR_LOG("[GameManagerSlaveImpl] Failed to update external session due to invalid params for external session " << toLogStr(params.getSessionIdentification()) << "(" << params.getExternalSessionCreationInfo().getExternalSessionId() << "), session type: " << Blaze::ExternalSessionTypeToString(params.getExternalSessionCreationInfo().getExternalSessionType()));
            return false;
        }
        return true;
    }

     /*! ************************************************************************************************/
    /*! \brief For each external user info from input list, adds its user session id to output session id list.
        param[in] externalUserInfos - list of external user join infos, to find user sessions for
        param[out] userSessionIdList - found user session ids. Pre: starts empty
    ***************************************************************************************************/
    void GameManagerSlaveImpl::externalUserJoinInfosToUserSessionIdList(const ExternalUserJoinInfoList& externalUserInfos,
        UserSessionIdList& userSessionIdList) const
    {
        for (ExternalUserJoinInfoList::const_iterator itr = externalUserInfos.begin(); itr != externalUserInfos.end(); ++itr)
        {
            UserInfoPtr userInfo;
            if (ERR_OK == lookupExternalUserByUserIdentification((*itr)->getUserIdentification(), userInfo))
            {
                UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(userInfo->getId());
                if (primarySessionId != INVALID_USER_SESSION_ID)
                    userSessionIdList.push_back(primarySessionId);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief clears the specified user's primary external session.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::clearPrimaryExternalSessionForUser(ClearPrimaryExternalSessionParameters& clearParams)
    {
        const UserIdentification& ident = clearParams.getUserIdentification();
        ExternalId extId = getExternalIdFromPlatformInfo(ident.getPlatformInfo());

        // the auth token must be retrieved here, in the blockable fiber.
        eastl::string tokenBuf;
        BlazeRpcError tokErr = getExternalSessionUtilManager().getAuthToken(ident, clearParams.getAuthInfo().getServiceName(), tokenBuf);
        if (tokErr != ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].clearPrimaryExternalSessionForUser failed to fetch external session auth token for user (blaze id " << ident.getBlazeId() << ", external id " << extId << ". Cannot clear primary session, error " << ErrorHelp::getErrorName(tokErr));
        }
        else
        {
            clearParams.getAuthInfo().setCachedExternalSessionToken(tokenBuf.c_str());
            BlazeRpcError clearErr = getExternalSessionUtilManager().clearPrimary(clearParams, nullptr);
            if (ERR_OK != clearErr)
            {
                incExternalSessionFailureNonImpacting(ident.getPlatformInfo().getClientPlatform());
                ERR_LOG("[GameManagerSlaveImpl].clearPrimaryExternalSessionForUser failed clearing primary session for user (blaze id " << ident.getBlazeId() << ", external id " << extId << ", error " << ErrorHelp::getErrorName(clearErr));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief verifies whether all members of the request will pass external session/first party join restrictions.
        \param[in,out] externalSessionIdentification If values are empty/unset this fn will fetch it by request's GameId.
        \param[in] joiningCaller info for the caller/leader. nullptr means none is joining.
        \param[in] groupUsers info for joining group members.
        \return a join restrictions error if restrictions fail at least one user, else an appropriate code.
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::checkExternalSessionJoinRestrictions(ExternalSessionIdentification& externalSessionIdentification, const JoinGameRequest& joinRequest,
        const UserJoinInfoList& userInfos)
    {
        // Join restrictions only apply to non-reservation claims and joining by player by spec. For efficiency if restrictions don't apply early out.
        if ((joinRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_CLAIM_RESERVATION) || (joinRequest.getJoinMethod() != JOIN_BY_PLAYER))
            return ERR_OK;

        CheckExternalSessionRestrictionsParameters params;
        // set up the list of users that may be joining. add caller, then group users, reserved users.
        params.getExternalUserJoinInfos().reserve(userInfos.size());

        bool isReserved = (joinRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION);
        for (UserJoinInfoList::const_iterator itr = userInfos.begin(); itr != userInfos.end(); ++itr)
        {
            if ((*itr)->getUser().getHasExternalSessionJoinPermission())
            {
                setExternalUserJoinInfoFromUserSessionInfo(*params.getExternalUserJoinInfos().pull_back(), (*itr)->getUser(), (isReserved || (*itr)->getIsOptional())); // Optional users are always reserved
                bool doSetGroupToken = ((params.getGroupExternalSessionToken() != nullptr) && (params.getGroupExternalSessionToken()[0] == '\0'));
                if (doSetGroupToken)
                    params.setGroupExternalSessionToken((*itr)->getUser().getExternalAuthInfo().getCachedExternalSessionToken());
            }
        }


        if (params.getExternalUserJoinInfos().empty())
            return ERR_OK;//nobody joins external session, early out
        // set up external session name, template if not already populated
        if (!isExtSessIdentSet(externalSessionIdentification))
        {
            BlazeRpcError err = fetchExternalSessionDataFromSearchSlaves(joinRequest.getGameId(), externalSessionIdentification);
            if (err != ERR_OK)
            {
                ERR_LOG("[GameManagerSlaveImpl].checkExternalSessionJoinRestrictions Failed to find any game " << joinRequest.getGameId() << " via search slaves, error " << ErrorHelp::getErrorName(err) << ".");
                return err;
            }
        }
        externalSessionIdentification.copyInto(params.getSessionIdentification());

        ExternalSessionApiError apiErr;
        BlazeRpcError sessErr = getExternalSessionUtilManager().checkRestrictions(params, apiErr);
        if (sessErr != ERR_OK)
        {
            for (const auto& platformErr : apiErr.getPlatformErrorMap())
            {
                incExternalSessionFailureImpacting(platformErr.first);
                TRACE_LOG("[GameManagerSlaveImpl].checkExternalSessionJoinRestrictions join restrictions check for game " << joinRequest.getGameId() << " (externalSession " << toLogStr(externalSessionIdentification)
                    << "), error (" << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr()) << "), platform (" << ClientPlatformTypeToString(platformErr.first) << ").");
            }

            if (apiErr.getFailedOnAllPlatforms())
            {
                // only publish the PIN event if we are going to fail this call right away
                const auto& platformErrItr = apiErr.getPlatformErrorMap().find(xone);
                if (platformErrItr != apiErr.getPlatformErrorMap().end())
                {
                    if ((*platformErrItr).second->getBlazeRpcErr() == ERR_COULDNT_CONNECT)
                        gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::MPSD_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
                }

                return (convertExternalServiceErrorToGameManagerError(apiErr.getSinglePlatformBlazeRpcErr() != ERR_OK ? apiErr.getSinglePlatformBlazeRpcErr() : GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR));
            }
        }
        
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief For each user from input playerJoinData that's (a) offline or (b) only specified by 1st party/external id, adds its user join info to output list.
        param[in] playerJoinData - list of players' (external or not) user identifications, to find user join info for.
        param[out] usersInfo - users from playerJoinData, not logged into blaze, or specified only by 1st party/external id,
            will be added to this list. The master uses these to create player objects in the game, which for offline users are temp
            'external player' place holder objects, with placeholder values for fields like ConnectionGroupId etc.
            These are later replaced with 'real' player objects, once those users log into the blaze server.
        param[in] groupAuthToken - if specified, uses this when accessing the external profile service.
    ***************************************************************************************************/
    void GameManagerSlaveImpl::populateExternalPlayersInfo(PlayerJoinData& playerJoinData,
        const UserIdentificationList& groupUserIds, UserJoinInfoList& usersInfo, const char8_t* groupAuthToken /*= nullptr*/) const
    {
        // for guarding vs dupes
        eastl::map<ClientPlatformType, eastl::set<ExternalId>> externalIdSetByPlatform;
        eastl::set<ExternalSwitchId> externalStringSet;

        // variables for setting up a batch request to retrieve user's data from an external profile service.
        ExternalIdList externalProfileIds;
        const char8_t* externalProfileToken = groupAuthToken;

        PerPlayerJoinDataList::iterator itr = playerJoinData.getPlayerDataList().begin();
        for (; itr != playerJoinData.getPlayerDataList().end(); ) // iterator may be deleted in loop
        {
            // Player list contains both offline/invalid-BlazeId, and not. Skip those that shouldn't be handled in this method
            UserIdentification userId = (*itr)->getUser();
            bool isHost = false;
            if (gCurrentLocalUserSession)
            {
                UserIdentification hostUserIdentification;
                gCurrentLocalUserSession->getUserInfo().filloutUserIdentification(hostUserIdentification);

                if (areUserIdsEqual(hostUserIdentification, userId))
                {
                    isHost = true;
                }
            }

            if (userId.getBlazeId() != INVALID_BLAZE_ID)
            {
                // Check if the user is currently logged in (and should be handled by userIdListToJoiningInfoList).
                // Special case: Dedicated servers don't have a valid primary session id, also handled in userIdListToJoiningInfoList.
                UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(userId.getBlazeId());
                if ((primarySessionId != INVALID_USER_SESSION_ID) || 
                    (isHost && (userId.getBlazeId() == UserSession::getCurrentUserBlazeId())))  // Same check as in userIdListToJoiningInfoList
                {
                    ++itr;
                    continue;
                }
            }

            // BlazeId or 1st party id etc may not have been specified in request, use values from Blaze lookup if available
            UserInfoPtr userInfo;
            BlazeRpcError blazeLookupErr = lookupExternalUserByUserIdentification(userId, userInfo);
            if (blazeLookupErr == ERR_OK)
            {
                userInfo->filloutUserIdentification(userId);
            }

            const BlazeId blazeId = userId.getBlazeId();

            const ExternalId externalId = getExternalIdFromPlatformInfo(userId.getPlatformInfo());
            const ExternalSwitchId& externalString = userId.getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
            if (!hasFirstPartyId(userId) &&
                ((userId.getPlatformInfo().getClientPlatform() != ClientPlatformType::pc) || (blazeId == INVALID_BLAZE_ID))) //separately checked below: if 'new to title', PC users also need an external/1p id
            {
                ++itr;
                continue;
            }

            bool existed = false;
            ClientPlatformType platform = userId.getPlatformInfo().getClientPlatform();
            if (gController->usesExternalString(platform))
            {
                existed = (!externalString.empty()) && !(externalStringSet.insert(externalString).second);
            }
            else
            {
                existed = (externalId != INVALID_EXTERNAL_ID) && !(externalIdSetByPlatform[platform].insert(externalId).second);
            }

            if (existed)
            {
                // skip over the duplicated external id
                eastl::string platformInfoStr;
                WARN_LOG("[GameManagerSlaveImpl].populateExternalPlayersInfo: Skipping over duplicated user (" << platformInfoToString(userId.getPlatformInfo(), platformInfoStr) << ").");
                ++itr;
                continue;
            }

            // Check if the user is in the group: 
            bool wasInGroupId = false;
            for (UserIdentificationList::const_iterator it=groupUserIds.begin(), itEnd=groupUserIds.end(); it != itEnd; ++it)  // iterator may be erased in loop.
            {
                if (areUserIdsEqual((**it), userId))
                {
                    wasInGroupId = true;
                    break;
                }
            }

            UserJoinInfo* joinInfo = usersInfo.pull_back();
            joinInfo->setIsOptional((*itr)->getIsOptionalPlayer());
            if (isHost)
            {
                joinInfo->setIsHost(true);
            }
            UserSessionInfo& userSessionInfo = joinInfo->getUser();

            if (ERR_OK == blazeLookupErr)
            {
                // try retrieving user session
                UserSessionPtr session;
                UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId);
                if (primarySessionId != INVALID_USER_SESSION_ID)
                    session = gUserSessionManager->getSession(primarySessionId);

                if (session != nullptr)
                {
                    bool joinInfoSet = false;
                    if (!ValidationUtils::isUserSessionInList(primarySessionId, usersInfo))
                    {
                        BlazeRpcError err = ValidationUtils::setJoinUserInfoFromSession(userSessionInfo, *session, &mExternalSessionServices);
                        if (err == ERR_OK)
                        {
                        // Update the player join data as well:
                        (*itr)->getUser().setBlazeId(session->getBlazeId());
                        if (wasInGroupId)
                        {
                            if ((*itr)->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                                (*itr)->setTeamIndex(playerJoinData.getDefaultTeamIndex());
                            joinInfo->setUserGroupId(playerJoinData.getGroupId());
                        }
                            joinInfoSet = true;
                        }
                        else if (err == USER_ERR_USER_NOT_FOUND || err == USER_ERR_SESSION_NOT_FOUND)
                        {
                            TRACE_LOG("[GameManagerSlaveImpl].populateExternalPlayersInfo: Failed to set join user info for session(" << session->getUserSessionId() << "), err=" << ErrorHelp::getErrorName(err));
                        }
                        else
                        {
                            WARN_LOG("[GameManagerSlaveImpl].populateExternalPlayersInfo: Failed to set join user info for session(" << session->getUserSessionId() << "), err=" << ErrorHelp::getErrorName(err));
                        }
                    }
                    if (!joinInfoSet)
                    {
                        // Remove user
                        usersInfo.pop_back();
                        itr = playerJoinData.getPlayerDataList().erase(itr);
                        continue;
                    }
                }
                else
                {
                    //note that not-logged-in known EA users here won't get added with a valid session id,
                    //however, we still want to attempt reserving their spaces, in case they choose to log in.
                    //Add them as external players, but using their actual valid account info.
                    userSessionInfo.setSessionId(INVALID_USER_SESSION_ID);
                    userSessionInfo.getUserInfo().setId(blazeId);
                    if (userInfo)
                    {
                        userInfo->getPlatformInfo().copyInto(userSessionInfo.getUserInfo().getPlatformInfo());
                    }
                    else
                    {   // If the user info was missing, then we need to take the data from the userId that was provided.
                        // We may need to do some extra work to set the platform based on which external id was set, or use the platform of the caller.   
                        userId.getPlatformInfo().copyInto(userSessionInfo.getUserInfo().getPlatformInfo());
                    }
                    userSessionInfo.getUserInfo().setAccountLocale(userInfo->getAccountLocale());
                    userSessionInfo.getUserInfo().setAccountCountry(userInfo->getAccountCountry());
                    userSessionInfo.getUserInfo().setPersonaName(userInfo->getPersonaName());
                    userSessionInfo.getUserInfo().setPersonaNamespace(userInfo->getPersonaNamespace());

                    // cache off join external sessions permission as true for now (update later when known)
                    userSessionInfo.setHasExternalSessionJoinPermission(true);
                    // cache off its external auth token
                    eastl::string extToken;
                    // GOS-32212 - We're registering the user session with the platform used in the request, which may not match the platform the user uses to join. 
                    userSessionInfo.setServiceName(gController->getDefaultServiceName(userSessionInfo.getUserInfo().getPlatformInfo().getClientPlatform()));
                    mExternalSessionServices.getAuthToken(userId, userSessionInfo.getServiceName(), extToken);
                    userSessionInfo.getExternalAuthInfo().setCachedExternalSessionToken(extToken.c_str());
                    userSessionInfo.getExternalAuthInfo().setServiceName(userSessionInfo.getServiceName());
                    // cache off user extended data (from db or defaults)
                    UserInfoData defaultUserInfo(userSessionInfo.getUserInfo());
                    defaultUserInfo.setId(INVALID_BLAZE_ID);
                    const UserInfoData* uedReqInfo = (getConfig().getGameSession().getUseDatabaseUedForOfflinePlayers()? &(*userInfo) : &defaultUserInfo);
                    UserSessionExtendedData extendedData;
                    if (ERR_OK == gUserSessionManager->requestUserExtendedData(*uedReqInfo, extendedData, false))//if err, logged
                        extendedData.getDataMap().copyInto(userSessionInfo.getDataMap());
                    if (wasInGroupId)
                    {
                        if ((*itr)->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                            (*itr)->setTeamIndex(playerJoinData.getDefaultTeamIndex());
                        joinInfo->setUserGroupId(playerJoinData.getGroupId());
                    }
                }
            }
            else if (blazeLookupErr == USER_ERR_USER_NOT_FOUND)
            {
                // GOS-32212 - This is the old xone (totally) external users code.  Need to make sure it makes sense for the crossplay

                //note users new to the title (never logged in before), get placeholder blaze id etc
                if (gCurrentLocalUserSession == nullptr)
                {
                    WARN_LOG("[GameManagerSlaveImpl].populateExternalPlayersInfo: Not joining user(" << userId << "), as it does not have an account for the title and no host was used in this join request.");
                    ++itr;
                    continue;
                }

                BlazeId dummyBlazeId = allocateTemporaryBlazeIdForExternalUser(userId);
                if (dummyBlazeId == INVALID_BLAZE_ID)
                {
                    ++itr;
                    continue;//logged
                }
                userSessionInfo.setSessionId(INVALID_USER_SESSION_ID);
                userSessionInfo.getUserInfo().setId(dummyBlazeId);
                if (userInfo)
                {
                    userInfo->getPlatformInfo().copyInto(userSessionInfo.getUserInfo().getPlatformInfo());
                }
                else
                {   // If the user info was missing, then we need to take the data from the userId that was provided.
                    // We may need to do some extra work to set the platform based on which external id was set, or use the platform of the caller.   
                    userId.getPlatformInfo().copyInto(userSessionInfo.getUserInfo().getPlatformInfo());
                }
                // cache off join external sessions permission as true for now (update later when known)
                userSessionInfo.setHasExternalSessionJoinPermission(true);
                // cache off its external auth token
                eastl::string extToken;
                // GOS-32212 - (Same as above) We're registering the user session with the platform used in the request, which may not match the platform the user uses to join. 
                userSessionInfo.setServiceName(gController->getDefaultServiceName(userSessionInfo.getUserInfo().getPlatformInfo().getClientPlatform()));
                mExternalSessionServices.getAuthToken(userId, userSessionInfo.getServiceName(), extToken);
                userSessionInfo.getExternalAuthInfo().setCachedExternalSessionToken(extToken.c_str());
                userSessionInfo.getExternalAuthInfo().setServiceName(userSessionInfo.getServiceName());
                // cache off user extended data. Use invalid blaze id to load defaults.
                UserInfoData defaultUserInfo(userSessionInfo.getUserInfo());
                defaultUserInfo.setId(INVALID_BLAZE_ID);
                UserSessionExtendedData extendedData;
                if (ERR_OK == gUserSessionManager->requestUserExtendedData(defaultUserInfo, extendedData, false))//if err, logged
                    extendedData.getDataMap().copyInto(userSessionInfo.getDataMap());

                // must omit this user if there's not enough info to identify it *without* the placeholder BlazeId
                if (!has1stPartyPlatformInfo(userSessionInfo.getUserInfo().getPlatformInfo()))
                {
                    ERR_LOG("[GameManagerSlaveImpl].populateExternalPlayersInfo: skipping including user, not enough info to identify it *without* its temporary BlazeId. Platform info: " << userSessionInfo.getUserInfo().getPlatformInfo());
                    // Remove user
                    usersInfo.pop_back();
                    itr = playerJoinData.getPlayerDataList().erase(itr);
                    continue;
                }

                // append to the external profiles batch request
                externalProfileIds.push_back(externalId);

                if (wasInGroupId)
                {
                    if ((*itr)->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                        (*itr)->setTeamIndex(playerJoinData.getDefaultTeamIndex());
                    joinInfo->setUserGroupId(playerJoinData.getGroupId());
                }
            }
            // save off 1st usable token for the external profiles batch request
            // This is Xbox MPSD sessions specific. Not expected used on other platforms
            if (!externalProfileToken && (userSessionInfo.getExternalAuthInfo().getCachedExternalSessionToken()[0] != '\0'))
                externalProfileToken = userSessionInfo.getExternalAuthInfo().getCachedExternalSessionToken();

            ++itr;
        }

        if (!externalProfileIds.empty())
        {
            mExternalSessionServices.updateUserProfileNames(usersInfo, externalProfileIds, externalProfileToken);
        }
    }

    /*! \brief return a new negative BlazeId */
    BlazeId GameManagerSlaveImpl::allocateTemporaryBlazeIdForExternalUser(const UserIdentification& externalUserIdentity) const
    {
        if (EA_UNLIKELY(gUserSessionManager == nullptr))
        {
            ERR_LOG("[GameManagerSlaveImpl].allocateExternalPlayerBlazeId: user session manager not available. Cannot allocate a negative id!");
            return INVALID_BLAZE_ID;
        }
        BlazeId blazeId = gUserSessionManager->allocateNegativeId();
        char8_t buf[256];
        TRACE_LOG("[GameManagerSlaveImpl].allocateExternalPlayerBlazeId: assigned external user(" << externalUserIdentificationToString(externalUserIdentity, buf, sizeof(buf)) << "), place holder blaze id(" << blazeId << ")");
        return blazeId;
    }

    BlazeRpcError GameManagerSlaveImpl::fetchAuditedUsers(const Blaze::GameManager::FetchAuditedUsersRequest& request, Blaze::GameManager::FetchAuditedUsersResponse& response)
    {
        if (request.getFetchActiveAudits())
        {
            // Active audits likely do not need to go to db, so we try to retrieve from caches first.
            // fetch the list of BlazeId/expiry cached on the master (no need to go to db)
            BlazeRpcError err = getMaster()->fetchAuditedUsersMaster(response);
            if (err != ERR_OK)
            {
                ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUsers: Unable to fetch audited users ids/expiries from master, error: " << ErrorHelp::getErrorName(err));
                return err;
            }

            // fetch core identification info, if available from usersessionmanager. Note if n/a just omit it.
            for (FetchAuditedUsersResponse::AuditedUserList::iterator itr = response.getAuditedUsers().begin(), end = response.getAuditedUsers().end();
                itr != end; ++itr)
            {
                BlazeId blazeId = (*itr)->getCoreIdentification().getBlazeId();

                UserInfoPtr user;
                BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, user);//errs logged
                if (lookupErr == ERR_OK)
                {
                    user->filloutUserCoreIdentification((*itr)->getCoreIdentification());
                }
            }
        }
        else
        {
            DbConnPtr gmConn = gDbScheduler->getReadConnPtr(getDbId());
            if (gmConn == nullptr)
            {
                ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUsers: Unable to obtain a database connection.");
                return ERR_DB_SYSTEM;
            }

            QueryPtr gmQuery = DB_NEW_QUERY_PTR(gmConn);
            gmQuery->append("SELECT blazeid, expiration");
            gmQuery->append(" FROM `gm_user_connection_audit`");
            gmQuery->append(" WHERE expiration > 0 AND expiration <= $D", TimeValue::getTimeOfDay().getMicroSeconds());

            DbResultPtr gmResult;
            DbError dbErr;
            dbErr = gmConn->executeQuery(gmQuery, gmResult);
            if (dbErr != ERR_OK)
            {
                ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUsers: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
                return dbErr;
            }

            // Early out if no users are audited currently:
            if (gmResult->empty())
            {
                TRACE_LOG("[GameManagerSlaveImpl].fetchAuditedUsers: No users were found when executing query.");
                return ERR_OK;
            }

            BlazeIdVector userList;
            for (DbResult::const_iterator itr = gmResult->begin(), end = gmResult->end(); itr != end; ++itr)
            {
                const DbRow* row = *itr;
                userList.push_back(row->getUInt64("blazeid"));
            }

            BlazeIdToUserInfoMap blazeIdToUserInfoMap;
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeIds(userList, blazeIdToUserInfoMap, false /*ignoreStatus*/, true /*useReadConn*/);
            if (err != ERR_OK)
            {
                ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUsers: Error occurred executing looking up userinfo [" << ErrorHelp::getErrorName(err) << "]");
                return err;
            }
            for (DbResult::const_iterator itr = gmResult->begin(), end = gmResult->end(); itr != end; ++itr)
            {
                const DbRow* gmRow = *itr;
                BlazeId blazeId = gmRow->getUInt64("blazeid");

                GameManager::AuditedUser* user = response.getAuditedUsers().pull_back();
                user->getCoreIdentification().setBlazeId(blazeId);
                user->setExpiration(TimeValue(gmRow->getInt64("expiration")));

                BlazeIdToUserInfoMap::const_iterator userItr = blazeIdToUserInfoMap.find(blazeId);
                if (userItr != blazeIdToUserInfoMap.end())
                    userItr->second->filloutUserCoreIdentification(user->getCoreIdentification());
            }
        }

        return ERR_OK;
    }

    BlazeRpcError GameManagerSlaveImpl::fetchAuditedUserData(const Blaze::GameManager::FetchAuditedUserDataRequest& request, Blaze::GameManager::FetchAuditedUserDataResponse& response)
    {
        if (request.getBlazeId() == INVALID_BLAZE_ID)
        {
            INFO_LOG("[GameManagerSlaveImpl].fetchAuditedUserData: No blazeId specified");
            return GAMEMANAGER_ERR_USER_NOT_FOUND;
        }

        DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
        if (conn == nullptr)
        {
            ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUserData: Unable to obtain a database connection.");
            return ERR_DB_SYSTEM;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT * FROM `gm_user_connection_metrics` WHERE blazeid=$D", request.getBlazeId());

        DbResultPtr dbResult;
        BlazeRpcError dbErr;
        if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].fetchAuditedUserData: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return dbErr;
        }

        if (dbResult->empty())
        {
            TRACE_LOG("[GameManagerSlaveImpl].fetchAuditedUserData: User " << request.getBlazeId() << " does not have any audit data available.");
        }

        for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
        {
            uint32_t col = 0;
            const DbRow* row = *itr;
            AuditedUserData* data = response.getAuditedUserDataList().pull_back();
            data->setBlazeId(row->getInt64(col++));
            data->setGameId(row->getInt64(col++));

            data->setLocalIp(row->getUInt(col++));
            data->setLocalPort(row->getUShort(col++));
            data->setLocalClientType((ClientType)row->getUInt(col++));
            data->setLocalCountry(row->getString(col++));

            data->setRemoteIp(row->getUInt(col++));
            data->setRemotePort(row->getUShort(col++));
            data->setRemoteClientType((ClientType)row->getUInt(col++));
            data->setRemoteCountry(row->getString(col++));

            data->setGameMode(row->getString(col++));
            data->setTopology((GameNetworkTopology)row->getUInt(col++));
            data->setMaxLatency(row->getShort(col++));
            data->setAverageLatency(row->getShort(col++));
            data->setMinLatency(row->getShort(col++));
            data->setPacketLoss(row->getShort(col++));
            data->setConnTermReason((PlayerRemovedReason)row->getUInt(col++));
            data->setConnInitTime(TimeValue(row->getUInt64(col++)));
            data->setConnTermTime(TimeValue(row->getUInt64(col++)));
            data->setClientInitiatedConnection((row->getUChar(col++)) != 0);
            data->setQosRuleEnabled((row->getUChar(col++)) != 0);
            data->setLocalNatType((Util::NatType)row->getUInt(col++));
            data->setRemoteNatType((Util::NatType)row->getUInt(col++));
            data->setLocalRouterInfo(row->getString(col++));
            data->setRemoteRouterInfo(row->getString(col++));
        }

        return ERR_OK;
    }

    BlazeRpcError GameManagerSlaveImpl::deleteUserAuditMetricData(BlazeId blazeId)
    {
        DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
        if (conn == nullptr)
        {
            ERR_LOG("[GameManagerSlaveImpl].deleteUserAuditMetricData: Unable to obtain a database connection.");
            return ERR_DB_SYSTEM;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("DELETE FROM `gm_user_connection_metrics` WHERE blazeid=$D", blazeId);

        DbResultPtr dbResult;
        BlazeRpcError dbErr;
        if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].deleteUserAuditMetricData: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return dbErr;
        }

        if (dbResult->getAffectedRowCount() == 0)
        {
            TRACE_LOG("[GameManagerSlaveImpl].deleteUserAuditMetricData: User " << blazeId << " does not have any audit data to delete.");
            return GAMEMANAGER_ERR_USER_NOT_FOUND;
        }

        return ERR_OK;
    }

    void GameManagerSlaveImpl::onNotifyServerMatchmakingReservedExternalPlayers(const Blaze::GameManager::NotifyServerMatchmakingReservedExternalPlayers& data, UserSession*)
    {
        // TODO_MC: Why not use a passthrough notification.  This seems a bit weird, or un-elegant.
        Blaze::GameManager::NotifyMatchmakingReservedExternalPlayers updatedNotification;
        data.getClientNotification().copyInto(updatedNotification);
        sendNotifyMatchmakingReservedExternalPlayersToUserSessionById(data.getUserSessionId(), &updatedNotification);
    }

    void GameManagerSlaveImpl::onClearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& data, UserSession *associatedUserSession)
    {
        mScenarioManager.onClearUserScenarioVariant(data);
    }

    void GameManagerSlaveImpl::onUserScenarioVariantUpdate(const Blaze::GameManager::SetUserScenarioVariantRequest& data, UserSession *associatedUserSession)
    {
        mScenarioManager.onUserScenarioVariantUpdate(data);
    }

    void GameManagerSlaveImpl::callCCSAllocate(CCSAllocateRequestPtr ccsRequest)
    {
        Blaze::GameManager::CCSAllocateRequestMaster ccsRequestMaster;
        ccsRequest->getCCSRequestPairs().copyInto(ccsRequestMaster.getCCSRequestPairs());
        ccsRequestMaster.setGameId(ccsRequest->getGameId());
        ccsRequestMaster.setSuccess(false);
        
        uint32_t requestId = (ccsRequest->getRequest().getActiveMemberIndex() == CCSAllocateRequestInternal::MEMBER_V1REQUEST) ? 
                              ccsRequest->getRequest().getV1Request()->getRequestBody().getRequestId() : ccsRequest->getRequest().getV2Request()->getRequestBody().getRequestId();
        CCSUtil ccsUtil;
        eastl::string nucleusToken;
        if (ccsUtil.getNucleusAccessToken(nucleusToken) == Blaze::ERR_OK)
        {
            if (ccsRequest->getRequest().getActiveMemberIndex() == CCSAllocateRequestInternal::MEMBER_V1REQUEST)
                ccsRequest->getRequest().getV1Request()->setAccessToken(nucleusToken.c_str());
            else 
                ccsRequest->getRequest().getV2Request()->setAccessToken(nucleusToken.c_str());

            Blaze::CCS::GetHostedConnectionResponse response;
            Blaze::CCS::CCSErrorResponse errorResponse;
            
            BlazeRpcError error = Blaze::ERR_OK;
            if (ccsRequest->getRequest().getActiveMemberIndex() == CCSAllocateRequestInternal::MEMBER_V1REQUEST)
                error = ccsUtil.allocateConnectivity(ccsRequest->getGameId(), *ccsRequest->getRequest().getV1Request(), response, errorResponse);
            else
                error = ccsUtil.allocateConnectivity(ccsRequest->getGameId(), *ccsRequest->getRequest().getV2Request(), response, errorResponse);

            if (error == Blaze::ERR_OK)
            {
                response.copyInto(ccsRequestMaster.getHostedConnectionResponse());
                ccsRequestMaster.setSuccess(true);
            }
            else
            {
                errorResponse.copyInto(ccsRequestMaster.getErr());
                ERR_LOG("[GameManagerSlaveImpl].callCCSAllocate: game("<< ccsRequest->getGameId() <<"). request failed due to error in calling ccs service. Request Id("<<requestId<<"). Error ("<<ErrorHelp::getErrorName(error)<<")");
            }
            ccsRequest->copyInto(ccsRequestMaster.getRequestOut()); //for info above response may omit like ConnectionSetId
        }
        else
        {
            ERR_LOG("[GameManagerSlaveImpl].callCCSAllocate: game("<< ccsRequest->getGameId() <<").  request failed due to error in getting access token. Request Id("<<requestId<<").");
        }
        
        // We call master both in case of success and failure so that it can take appropriate actions
        BlazeRpcError error = getMaster()->ccsConnectivityResultsAvailable(ccsRequestMaster);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].callCCSAllocate:  game("<< ccsRequest->getGameId() <<"). getMaster()->ccsConnectivityResultsAvailable failed. Request Id("<<requestId<<")."<<"Error (" <<ErrorHelp::getErrorName(error)<<")");
        }
    }

    void GameManagerSlaveImpl::callCCSFree(CCSFreeRequestPtr ccsRequest)
    {
        uint32_t requestId = ccsRequest->getFreeHostedConnectionRequest().getRequestBody().getRequestId(); 
       
        CCSUtil ccsUtil;
        eastl::string nucleusToken;
        if (ccsUtil.getNucleusAccessToken(nucleusToken) == Blaze::ERR_OK)
        {
            ccsRequest->getFreeHostedConnectionRequest().setAccessToken(nucleusToken.c_str()); 

            Blaze::CCS::FreeHostedConnectionResponse response;
            Blaze::CCS::CCSErrorResponse errorResponse;

            BlazeRpcError error = ccsUtil.freeConnectivity(ccsRequest->getGameId(), ccsRequest->getFreeHostedConnectionRequest(), response, errorResponse);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[GameManagerSlaveImpl].callCCSFree:  game("<< ccsRequest->getGameId() <<"). request failed due to error in calling ccs service. Request Id("<<requestId<<"). Error ("<<ErrorHelp::getErrorName(error)<<")");
            }
        }
        else
        {
            ERR_LOG("[GameManagerSlaveImpl].callCCSFree:  game("<< ccsRequest->getGameId() <<"). request failed due to error in getting access token. Request Id("<<requestId<<").");
        }
    }

    void GameManagerSlaveImpl::callCCSExtendLease(CCSLeaseExtensionRequestPtr ccsRequest)
    {
        Blaze::GameManager::CCSLeaseExtensionRequestMaster ccsRequestMaster;
        ccsRequestMaster.setGameId(ccsRequest->getGameId());
        ccsRequestMaster.setSuccess(false);

        uint32_t requestId = ccsRequest->getLeaseExtensionRequest().getRequestBody().getRequestId(); 
        CCSUtil ccsUtil;
        eastl::string nucleusToken;
        if (ccsUtil.getNucleusAccessToken(nucleusToken) == Blaze::ERR_OK)
        {
            ccsRequest->getLeaseExtensionRequest().setAccessToken(nucleusToken.c_str()); 

            Blaze::CCS::LeaseExtensionResponse response;
            Blaze::CCS::CCSErrorResponse errResponse;

            BlazeRpcError error = ccsUtil.extendLease(ccsRequest->getGameId(), ccsRequest->getLeaseExtensionRequest(), response, errResponse);
            if (error == Blaze::ERR_OK)
            {
                response.copyInto(ccsRequestMaster.getLeaseExtensionResponse());
                ccsRequestMaster.setSuccess(true);
            }
            else
            {
                errResponse.copyInto(ccsRequestMaster.getErr());
                ERR_LOG("[GameManagerSlaveImpl].callCCSExtendLease:  game("<< ccsRequest->getGameId() <<"). request failed due to error in calling ccs service. Request Id("<<requestId<<"). Error ("<<ErrorHelp::getErrorName(error)<<")");
            }
            ccsRequest->copyInto(ccsRequestMaster.getRequestOut()); //for info above response may omit like ConnectionSetId
        }
        else
        {
            ERR_LOG("[GameManagerSlaveImpl].callCCSExtendLease:  game("<< ccsRequest->getGameId() <<"). request failed due to error in getting access token. Request Id("<<requestId<<").");
        }

        // We call master both in case of success and failure so that it can take appropriate actions
        BlazeRpcError error = getMaster()->ccsLeaseExtensionResultsAvailable(ccsRequestMaster);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].callCCSExtendLease:  game("<< ccsRequest->getGameId() <<"). getMaster()->ccsLeaseExtensionResultsAvailable failed. Request Id("<<requestId<<")."<<"Error (" <<ErrorHelp::getErrorName(error)<<")");
        }
    }

    void GameManagerSlaveImpl::onRemoteInstanceChanged(const RemoteServerInstance& instance)
    {
        if (instance.hasComponent(Blaze::Search::SearchSlave::COMPONENT_ID))
        {
            if (!instance.isConnected())
            {
                // Nuke any data associated with the search instance that is going away
                WARN_LOG("[GameManagerSlaveImpl].onRemoteInstanceChanged: cleanup GameBrowser data for seachSlave(" << instance.getInstanceName() << ":" << instance.getInstanceId() << ").");
                mCleanupGameBrowserForInstanceJobQueue.queueFiberJob<GameBrowser, InstanceId>(
                    &mGameBrowser, &GameBrowser::cleanupDataForInstance, instance.getInstanceId(), "GameBrowser::cleanupDataForInstance");
            }
            else
            {
                INFO_LOG("[GameManagerSlaveImpl].onRemoteInstanceChanged: seachSlave(" << instance.getInstanceName() << ":" << instance.getInstanceId() << ") is now connected "
                    << (instance.isActive() ? "and active" : "but not active."));
            }
        }
    }

    void GameManagerSlaveImpl::onSliverSyncComplete(SliverNamespace sliverNamespace, InstanceId instanceId)
    {
        if (sliverNamespace == GameManagerMaster::COMPONENT_ID)
        {
            const RemoteServerInstance* remoteServerInstance = gController->getRemoteServerInstance(instanceId);
            if (remoteServerInstance != nullptr)
            {
                // Nuke any data associated with the search instance that has repartition
                if (remoteServerInstance->hasComponent(Blaze::Search::SearchSlave::COMPONENT_ID))
                {
                    TRACE_LOG("[GameManagerSlaveImpl].onSliverSyncComplete: cleanup GameBrowser data for seachSlave(" << instanceId << ").");
                    mCleanupGameBrowserForInstanceJobQueue.queueFiberJob<GameBrowser, InstanceId>(
                        &mGameBrowser, &GameBrowser::cleanupDataForInstance, instanceId, "GameBrowser::cleanupDataForInstance");
                }
            }
        }
    }

    BlazeRpcError GameManagerSlaveImpl::setUserScenarioVariant(const Blaze::GameManager::SetUserScenarioVariantRequest& request)
    {
        BlazeRpcError err = mScenarioManager.userScenarioVariantUpdate(request);
        if (err == ERR_OK)
        {
            sendUserScenarioVariantUpdateToRemoteSlaves(&request);
        }

        return err;
    }

    BlazeRpcError GameManagerSlaveImpl::clearUserScenarioVariant(const Blaze::GameManager::ClearUserScenarioVariantRequest& request)
    {
        BlazeRpcError err = mScenarioManager.clearUserScenarioVariant(request);
        if (err == ERR_OK)
        {
            sendClearUserScenarioVariantToRemoteSlaves(&request);
        }

        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief populate BlazeIds for users in the player join data, from their encrypted user id, as needed
        \param[in, out] playerDataList List of player data to update
    ***************************************************************************************************/
    BlazeRpcError GameManagerSlaveImpl::decryptBlazeIds(PerPlayerJoinDataList& playerDataList, EncryptedBlazeIdList* failedIds)
    {
        Authentication::GetDecryptedBlazeIdsRequest request;

        for (PerPlayerJoinDataList::const_iterator itr = playerDataList.begin(), end = playerDataList.end(); itr != end; ++itr)
        {
            if ((*itr)->getEncryptedBlazeId()[0] != '\0')
            {
                request.getEncryptedBlazeIds().push_back((*itr)->getEncryptedBlazeId());
            }
        }
        if (request.getEncryptedBlazeIds().empty())
            return ERR_OK;


        BlazeRpcError err = ERR_OK;
        Authentication::AuthenticationSlave* authComponent = static_cast<Authentication::AuthenticationSlave*>(gController->getComponent(Blaze::Authentication::AuthenticationSlave::COMPONENT_ID, false, true, &err));
        if (authComponent == nullptr)
        {
            ERR_LOG("[GameManagerSlaveImpl].decryptBlazeIds: Unable to resolve auth component, error " << ErrorHelp::getErrorName(err));
            return err;
        }
        Authentication::GetDecryptedBlazeIdsResponse response;
        Authentication::GetDecryptedBlazeIdsResponseError errorResponse;

        err = authComponent->getDecryptedBlazeIds(request, response, errorResponse);
        if (err != ERR_OK)
        {
            ERR_LOG("[GameManagerSlaveImpl].decryptBlazeIds: Unable to decrypt blaze ids, error " << ErrorHelp::getErrorName(err));

            if (!errorResponse.getFailedEncryptedBlazeIds().empty())
            {
                if (failedIds != nullptr)
                {
                    errorResponse.getFailedEncryptedBlazeIds().copyInto(*failedIds);
                }
                return GAMEMANAGER_ERR_USER_NOT_FOUND;
            }

            return err;
        }

        // add BlazeIds in player join data
        for (PerPlayerJoinDataList::const_iterator itr = playerDataList.begin(), end = playerDataList.end(); itr != end; ++itr)
        {
            if (((*itr)->getUser().getBlazeId() == INVALID_BLAZE_ID) && ((*itr)->getEncryptedBlazeId()[0] != '\0'))
            {
                Authentication::GetDecryptedBlazeIdsResponse::DecryptedIdsMap::const_iterator found = response.getDecryptedIds().find((*itr)->getEncryptedBlazeId());
                if (found != response.getDecryptedIds().end())
                {
                    TRACE_LOG("[GameManagerSlaveImpl].decryptBlazeIds: (" << (*itr)->getEncryptedBlazeId() << ") decrypted to(" << found->second << ")");
                    (*itr)->getUser().setBlazeId(found->second);
                }
            }
        }
        return err;
    }

    RedisError GameManagerSlaveImpl::testAndUpdateScenarioConcurrencyLimit(const UserGroupId& userGroupId, MatchmakingScenarioId scenarioId, int32_t limit, const TimeValue& expiryTtl, RedisResponsePtr& resp) const
    {
        RedisCommand cmd;
        cmd.addKey(userGroupId);
        cmd.add(scenarioId);
        cmd.add(limit);
        cmd.add(expiryTtl.getMillis());

        RedisCluster* cluster = gRedisManager->getRedisCluster();
        if (cluster != nullptr)
            return cluster->execScript(msAddScenarioIdIfUnderLimit, cmd, resp);

        return REDIS_ERR_SYSTEM;
    }

    RedisError GameManagerSlaveImpl::callRemoveMMId(const UserGroupId& userGroupId, MatchmakingScenarioId scenarioId, RedisResponsePtr& resp)
    {
        RedisCommand cmd;
        cmd.addKey(userGroupId);
        cmd.add(scenarioId);

        RedisCluster* cluster = gRedisManager->getRedisCluster();
        if (cluster != nullptr)
            return cluster->execScript(msRemoveMMScenariodId, cmd, resp);

        return REDIS_ERR_SYSTEM;
    }

    /*! ************************************************************************************************/
    /*! \brief Add Matchmaking Scenario Id to the redis keyed set stored at key(UserGroupId) with TTL if either of the following situations meets:
    1. key(UserGroupId) does not exist
    2. Key(UserGroupId) exists, the set size hasn't reach limit

    \param[in]      KEYS[1]    - UserGroup Id
    \param[in]      ARGV[1]    - Matchmaking Scenario Id needs to be added
    \param[in]      ARGV[2]    - maxActiveSessionsPerUserGroup setting
    \param[in]      ARGV[3]    - The new TTL for this set
    \param[out]     result     - the result of execution
    ***************************************************************************************************/
    RedisScript GameManagerSlaveImpl::msAddScenarioIdIfUnderLimit(1,
        "local result = redis.call('EXISTS', KEYS[1]);"
        "if (result == 0) then"
        "    if (redis.call('SADD', KEYS[1], ARGV[1])) then"
        "        result = redis.call('PEXPIRE', KEYS[1], ARGV[3]);"
        "    end;"
        "else"
        "    result = redis.call('SCARD', KEYS[1]);"
        "    if (result < tonumber(ARGV[2]) or tonumber(ARGV[2]) == 0) then" // Add key with TTL if set size is less than max allowed, or max allowed is unlimited
        "        if (redis.call('SADD', KEYS[1], ARGV[1])) then"
        "            result = redis.call('PEXPIRE', KEYS[1], ARGV[3]);"
        "        end;"
        "    else"
        "        return redis.call('SMEMBERS', KEYS[1]);"
        "    end;"
        "end;"
        "return result;");

    RedisScript GameManagerSlaveImpl::msRemoveMMScenariodId(1, "return redis.call('SREM', KEYS[1], ARGV[1]);");
    

    BlazeRpcError GameManagerSlaveImpl::createGameInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, Blaze::GameManager::CreateGameResponse &response,
        CreateGameMasterErrorInfo& errInfo, bool isPseudoGame)
    {
        cleanupCreateGameRequest(request);

        BlazeId blazeId = UserSession::getCurrentUserBlazeId();

        CreateGameMasterRequest masterRequest;
        masterRequest.setCreateRequest(request);
        masterRequest.setIsPseudoGame(isPseudoGame);
        masterRequest.setCreateGameTemplateName(templateName);

        if (request.getGameCreationData().getIsExternalOwner() &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_AS_EXTERNAL_OWNER, true))
        {
            eastl::string clientInfoLogStr;
            WARN_LOG("[GameManagerSlaveImpl].createGameInternal: External owner " << blazeId
                << " attempted to create game[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], no permission! " << makeClientInfoLogStr(clientInfoLogStr).c_str());
            return ERR_AUTHORIZATION_REQUIRED;
        }
        if (request.getGameCreationData().getIsExternalCaller() &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OMIT_CALLER_FROM_ROSTER, true))
        {
            eastl::string clientInfoLogStr;
            WARN_LOG("[GameManagerSlaveImpl].createGameInternal: Caller to be omitted from roster " << blazeId
                << " attempted to create game[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], no permission! " << makeClientInfoLogStr(clientInfoLogStr).c_str());
            return ERR_AUTHORIZATION_REQUIRED;
        }

        switch (request.getCommonGameData().getGameType())
        {
        case GAME_TYPE_GAMESESSION:
        {
            if (!isPseudoGame)
            {
                if (!(UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME, true)
                    || (request.getGameCreationData().getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED && UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME_PEER_HOSTED_SERVER, true))
                    || (request.getGameCreationData().getNetworkTopology() == CLIENT_SERVER_DEDICATED && UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME_DED_SERVER, true))
                    || (request.getGameCreationData().getNetworkTopology() == PEER_TO_PEER_FULL_MESH && UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME_P2P_FULL_MESH, true))
                    || (request.getGameCreationData().getNetworkTopology() == NETWORK_DISABLED && UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME_NET_DISABLED, true))
                    )
                    || !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_HOST_GAME_SESSION))
                {
                    eastl::string clientInfoLogStr;
                    WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                        << " attempted to create game[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], no permission! " << makeClientInfoLogStr(clientInfoLogStr).c_str());
                    return ERR_AUTHORIZATION_REQUIRED;
                }

                if (request.getGameCreationData().getVoipNetwork() == VOIP_DEDICATED_SERVER &&
                    ((request.getGameCreationData().getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED) ||
                    (request.getGameCreationData().getNetworkTopology() == PEER_TO_PEER_FULL_MESH) ||
                        (request.getGameCreationData().getNetworkTopology() == NETWORK_DISABLED)))
                {
                    // can't create game with unsupported topology (VoIP dedicated)
                    WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                        << " attempted to create a game with VOIP_DEDICATED_SERVER, but a non-dedicated Network Topology (" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << ")! ");
                    return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
                }
            }
            else
            {
                // Do auth check here:
                if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_PSEUDO_GAMES, true))
                {
                    WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                        << " attempted to create pseudo game with no permission! ");
                    return ERR_AUTHORIZATION_REQUIRED;
                }
            }
            break;
        }
        case GAME_TYPE_GROUP:
        {
            if (isPseudoGame)
            {
                WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId << " attempted to create 'pseudo' game group. This is not supported.");
                return ERR_SYSTEM;
            }

            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_GAME_GROUP, true))
            {
                eastl::string clientInfoLogStr;
                WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                    << " attempted to create game group[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], no permission! " << makeClientInfoLogStr(clientInfoLogStr).c_str());
                return ERR_AUTHORIZATION_REQUIRED;
            }
            if (request.getGameCreationData().getNetworkTopology() != NETWORK_DISABLED)
            {
                WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                    << " attempted to create a game group with a Network Topology (" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << ") not NETWORK_DISABLED! ");
                return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
            }

            if (request.getGameCreationData().getVoipNetwork() != VOIP_DISABLED &&
                request.getGameCreationData().getVoipNetwork() != VOIP_PEER_TO_PEER)
            {
                WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                    << " attempted to create a game group with a Voip Topology not VOIP_DISABLED or  VOIP_PEER_TO_PEER! ");
                return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
            }
            break;
        }
        }

        if (request.getGameCreationData().getGameSettings().getVirtualized() && (request.getGameCreationData().getNetworkTopology() != CLIENT_SERVER_DEDICATED))
        {
            // only dedicated server topology games can be virtual.
            return GAMEMANAGER_ERR_INVALID_GAME_SETTINGS;
        }

        BlazeRpcError blazeErr = Blaze::ERR_OK;

        // Can only create a game with reserved players if we don't need a game network.
        if ((request.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION) &&
            (isPlayerHostedTopology(request.getGameCreationData().getNetworkTopology())
                || ((request.getGameCreationData().getNetworkTopology() == NETWORK_DISABLED) && getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions())))
        {
            return GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
        }

        // Validate Game Mode Attribute is present.
        if (!isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology())
            && (request.getCommonGameData().getGameType() == GAME_TYPE_GAMESESSION)
            && !ValidationUtils::validateAttributeIsPresent(request.getGameCreationData().getGameAttribs(), getConfig().getGameSession().getGameModeAttributeNameAsTdfString()))
        {
            return GAMEMANAGER_ERR_GAME_MODE_ATTRIBUTE_MISSING;
        }

        bool isVirtualWithReservation = (request.getGameCreationData().getGameSettings().getVirtualized() && (request.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION));

        // Set calling session as external owner for a game they wont end up joining.
        if (masterRequest.getCreateRequest().getGameCreationData().getIsExternalOwner())
        {
            BlazeRpcError externalOwnerErr = ValidationUtils::setJoinUserInfoFromSession(masterRequest.getExternalOwnerInfo(), *gCurrentUserSession, &getExternalSessionUtilManager());
            if (externalOwnerErr != ERR_OK)
            {
                return externalOwnerErr;
            }
        }

        UserIdentificationList groupUserIds;
        if (!isPseudoGame)
        {
            // Virtual game creation doesn't add players, except if its reservations. No need to add any join infos if not reserving
            if (!request.getGameCreationData().getGameSettings().getVirtualized() || isVirtualWithReservation)
            {
                if (request.getGameCreationData().getDataSourceNameList().empty())
                {
                    if (!request.getGameCreationData().getGameSettings().getVirtualized() && !request.getGameCreationData().getIsExternalOwner())
                    {
                        ValidationUtils::insertHostSessionToPlayerJoinData(request.getPlayerJoinData(), gCurrentLocalUserSession);
                    }

                    blazeErr = ValidationUtils::loadGroupUsersIntoPlayerJoinData(request.getPlayerJoinData(), groupUserIds, *this);
                    if (blazeErr != Blaze::ERR_OK)
                    {
                        return blazeErr;
                    }
                }
                else
                {
                    PerPlayerJoinDataList::iterator perPlayerJoinDataListItr = request.getPlayerJoinData().getPlayerDataList().begin();
                    PerPlayerJoinDataList::iterator perPlayerJoinDataListEndItr = request.getPlayerJoinData().getPlayerDataList().end();
                    for (; perPlayerJoinDataListItr != perPlayerJoinDataListEndItr; ++perPlayerJoinDataListItr)
                    {
                        UserIdentificationPtr userIdentificationPtr = groupUserIds.pull_back();
                        (*perPlayerJoinDataListItr)->getUser().copyInto(*userIdentificationPtr);
                    }

                }

                // find and add each user session id to session id list
                blazeErr = ValidationUtils::userIdListToJoiningInfoList(request.getPlayerJoinData(), groupUserIds,
                    masterRequest.getUsersInfo(), &getExternalSessionUtilManager());
                if (blazeErr != Blaze::ERR_OK)
                {
                    return blazeErr;
                }
            }
            else if (!request.getPlayerJoinData().getPlayerDataList().empty())
            {
                WARN_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId << ") attempted to create a virtual game with (" <<
                    request.getPlayerJoinData().getPlayerDataList().size() << ") player join datas. Active players cannot be added when creating a virtual game.");
            }
        }
        else
        {
            // Update the team index held by the players, if a default is provided.
            setUnspecifiedTeamIndex(request.getPlayerJoinData(), request.getPlayerJoinData().getDefaultTeamIndex());

            // Build out a dummy UsersInfo list based on the player join data provided.
            PerPlayerJoinDataList::iterator curIter = request.getPlayerJoinData().getPlayerDataList().begin();
            PerPlayerJoinDataList::iterator endIter = request.getPlayerJoinData().getPlayerDataList().end();
            for (; curIter != endIter; ++curIter)
            {
                UserJoinInfo* joinInfo = masterRequest.getUsersInfo().pull_back();
                joinInfo->setIsOptional(false);
                joinInfo->setIsHost(curIter == request.getPlayerJoinData().getPlayerDataList().begin());
                joinInfo->getUser().getUserInfo().setId((*curIter)->getUser().getBlazeId());

                // Here we update the UED data for the (pseudo) users, based on the player attributes.
                Collections::AttributeMap::iterator curAttr = (*curIter)->getPlayerAttributes().begin();
                Collections::AttributeMap::iterator endAttr = (*curIter)->getPlayerAttributes().end();
                for (; curAttr != endAttr; ++curAttr)
                {
                    eastl::string attrName = curAttr->first.c_str();
                    eastl::string attrValue = curAttr->second.c_str();

                    eastl::string delimiter = "_";
                    eastl::string::size_type pos = attrName.find(delimiter);
                    eastl::string token = attrName.substr(0, pos);
                    if (token == "UED")
                    {
                        token = attrName.substr(pos + 1, attrName.size());
                        UserExtendedDataKey uedKey = EA::StdC::AtoU32(token.c_str());
                        UserExtendedDataValue uedValue = EA::StdC::AtoI64(attrValue.c_str());

                        joinInfo->getUser().getDataMap().insert(UserExtendedDataMap::value_type(uedKey, uedValue));
                    }
                }
            }
        }

        UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo());

        bool hasBadReputation = false;
        if (!isPseudoGame)
        {
            // Populate our offline/external players list. Virtual game creation doesn't add players, except if its reservations
            if (!request.getGameCreationData().getGameSettings().getVirtualized() || isVirtualWithReservation)
            {
                populateExternalPlayersInfo(request.getPlayerJoinData(),
                    groupUserIds,
                    masterRequest.getUsersInfo(),
                    hostSessionInfo ? hostSessionInfo->getExternalAuthInfo().getCachedExternalSessionToken() : nullptr);
            }

            // note for create/reset game (unlike join game), we need to process the reputations here for the external ids too
            // as they're used to determine whether we need to allow any reputation in the game settings below.
            hasBadReputation = !isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology()) &&
                GameManagerSlaveImpl::updateAndCheckUserReputations(masterRequest.getUsersInfo());
        }

        // Accounts for game group members and reserved slots.
        SlotTypeSizeMap slotTypeSizeMap;
        ValidationUtils::validatePlayersInCreateRequest(masterRequest, blazeErr, slotTypeSizeMap, isVirtualWithReservation);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // validate that we have enough space for the resetting/joining group
        blazeErr = ValidationUtils::validateGameCapacity(slotTypeSizeMap, request);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // set up default teams, if needed (will change when team index moves to PerPlayerJoinData)
        uint16_t participantCapacity = request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) + request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT);
        uint16_t teamCapacity = setUpDefaultTeams(masterRequest.getCreateRequest().getGameCreationData().getTeamIds(), request.getPlayerJoinData(), participantCapacity);

        // set up default role info if missing in request
        setUpDefaultRoleInformation(masterRequest.getCreateRequest().getGameCreationData().getRoleInformation(), teamCapacity);

        blazeErr = fixupPlayerJoinDataRoles(masterRequest.getCreateRequest().getPlayerJoinData(), true, getLogCategory());
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        // Verify that the team capacities are large enough for the group.
        blazeErr = ValidationUtils::validateTeamCapacity(slotTypeSizeMap, request, isVirtualWithReservation);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        blazeErr = ValidationUtils::validatePlayerRolesForCreateGame(masterRequest);
        if (blazeErr != Blaze::ERR_OK)
        {
            return blazeErr;
        }

        //Address sanitation
        if (!isPseudoGame)
        {
            //First set the external address if one is not provided.  
            if (request.getCommonGameData().getPlayerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
            {
                if (cmd != nullptr)
                { 
                    ConnectUtil::setExternalAddr(request.getCommonGameData().getPlayerNetworkAddress(), cmd->getPeerInfo()->getPeerAddress(), getConfig().getGameSession().getForceExternalAddressToConnAddress());
                    blazeErr = whitelistCheckAddr(request.getCommonGameData().getPlayerNetworkAddress(), request.getGameCreationData().getGameName(), blazeId);
                    if (blazeErr != Blaze::ERR_OK)
                    {
                        return blazeErr;
                    }
                }
                else
                {
                    TRACE_LOG("[GameManagerSlaveImpl].createGameInternal: User(" << blazeId << ") calling command is nullptr. No peerInfo.");
                    return ERR_SYSTEM;
                }                
            }
        }

        for (UserJoinInfoList::const_iterator sessionInfoItr = masterRequest.getUsersInfo().begin(), sessionInfoEnd = masterRequest.getUsersInfo().end(); sessionInfoItr != sessionInfoEnd; ++sessionInfoItr)
        {
            if (!(*sessionInfoItr)->getIsOptional())
            {
                // Optional users only: 
                continue;
            }

            const char8_t* sessionRole = lookupExternalPlayerRoleName(request.getPlayerJoinData(), (*sessionInfoItr)->getUser().getUserInfo());
            if (EA_UNLIKELY(sessionRole == nullptr))
            {
                TRACE_LOG("[GameManagerSlaveImpl].createGameInternal: User " << (*sessionInfoItr)->getUser().getUserInfo().getId() << ") role failed with error " << (ErrorHelp::getErrorName(GAMEMANAGER_ERR_ROLE_NOT_ALLOWED)));
                return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
            }
        }

        if (!gUserSessionManager->isReputationDisabled())
        {
            // finally, validate the reputation of players creating the game, if this isn't dedicated server
            if (hasBadReputation && !request.getGameCreationData().getGameSettings().getAllowAnyReputation())
            {
                // poor rep user, force allowAnyReputationFlag
                incrementTotalRequestsUpdatedForReputation();
                TRACE_LOG("[GameManagerSlaveImpl].createGameInternal: User " << blazeId
                    << " attempted to create game[" << request.getGameCreationData().getGameName() << "] with topology[" << GameNetworkTopologyToString(request.getGameCreationData().getNetworkTopology()) << "], had to set allowAnyReputationFlag.");
                masterRequest.getCreateRequest().getGameCreationData().getGameSettings().setAllowAnyReputation();
            }
            if (!hasBadReputation && request.getGameCreationData().getGameSettings().getDynamicReputationRequirement())
            {
                // all good rep users, force clear allowAnyReputationFlag if dynamic rep mode enabled
                masterRequest.getCreateRequest().getGameCreationData().getGameSettings().clearAllowAnyReputation();
            }
        }

        if (!isPseudoGame)
        {

            // if adding an external session, get its name and correlation id to initialize the blaze game session with
            // (no need to set up this info if creating a virtual game, with no reservations, as it won't have any external session)
            if (!request.getGameCreationData().getGameSettings().getVirtualized() || isVirtualWithReservation)
            {
                BlazeRpcError sessErr = initRequestExternalSessionData(masterRequest.getExternalSessionData(), masterRequest.getCreateRequest().getGameCreationData().getExternalSessionIdentSetup(),
                    request.getCommonGameData().getGameType(), masterRequest.getUsersInfo(),
                    getConfig().getGameSession(), getExternalSessionUtilManager(), *this);
                if (sessErr != ERR_OK && masterRequest.getExternalSessionData().getJoinInitErr().getFailedOnAllPlatforms())
                {
                    if (sessErr != ERR_COULDNT_CONNECT) // ERR_COULDNT_CONNECT is sent as RiverPoster::external_system_unavailable
                    {
                        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(sessErr), RiverPoster::game_creation_failed);
                    }

                    return sessErr;
                }

                if (getExternalSessionUtilManager().doesCustomDataExceedMaxSize(masterRequest.getCreateRequest().getClientPlatformListOverride(), request.getGameCreationData().getExternalSessionCustomData().getCount()))
                    return GAMEMANAGER_ERR_EXTERNAL_SESSION_CUSTOM_DATA_TOO_LARGE;
            }
        }

        // Create the game on master.
        CreateGameMasterResponse masterResponse;
        masterResponse.setCreateResponse(response);

        RpcCallOptions opts;
        opts.routeTo.setSliverNamespace(GameManager::GameManagerMaster::COMPONENT_ID);
        BlazeRpcError createGameError = getMaster()->createGameMaster(masterRequest, masterResponse, errInfo, opts);
        if (createGameError != ERR_OK)
        {
            TRACE_LOG("[GameManagerSlaveImpl:].createGameInternal: Failed to create Blaze game session. Error (" << ErrorHelp::getErrorName(createGameError) << ")");       
            return createGameError;
        }

        if (!isPseudoGame)
        {
            // Update to external session, the players which were updated by the blaze master call. Dedicated servers won't be joining external sessions
            if ((!isDedicatedHostedTopology(request.getGameCreationData().getNetworkTopology()) && !request.getGameCreationData().getGameSettings().getVirtualized()) ||
                isVirtualWithReservation)
            {
                // finally create and join. Remove failed players.
                createGameError = createAndJoinExternalSession(masterResponse.getExternalSessionParams(), false, masterRequest.getExternalSessionData().getJoinInitErr(), response.getExternalSessionIdentification());
            }
        }

        return createGameError;
    }

    BlazeRpcError GameManagerSlaveImpl::createOrJoinGameInternal(Command* cmd, const char8_t* templateName, CreateGameRequest& request, Blaze::GameManager::JoinGameResponse &response, bool isPseudoGame)
    {
        CreateGameMasterErrorInfo errInfo;
        Blaze::GameManager::CreateGameResponse cgResponse;
        BlazeRpcError err = createGameInternal(cmd, templateName, request, cgResponse, errInfo);
        if (err == ERR_OK)
        {
            // convert createGameResponse into joinGameResponse
            response.setGameId(cgResponse.getGameId());
            // copy JoinedReservedExternalPlayers
            joinedReservedPlayerIdentificationsToOtherResponse(cgResponse, response);
        }
        else if (err == Blaze::GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE)
        {
            GameId gameId = errInfo.getPreExistingGameIdWithPersistedId();
            TRACE_LOG("[GameManagerSlaveImpl].createOrJoinGameInternal: Failed to create new game, as a game(" << gameId <<
                ") already exists with persisted id(" << request.getPersistedGameId() << "). Attempting to join game instead.");

            // join the game if a game with the same persisted game id is found
            JoinGameRequest joinGameReq;
            joinGameReq.setGameId(gameId);
            joinGameReq.setJoinMethod(JOIN_BY_PERSISTED_GAME_ID);
            request.getPlayerJoinData().copyInto(joinGameReq.getPlayerJoinData());
            request.getCommonGameData().copyInto(joinGameReq.getCommonGameData());

            EntryCriteriaError entryCriteriaError;
            err = joinGame(joinGameReq, response, entryCriteriaError);
        }
        else
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::game_creation_failed);
        }

        return err;
    }

    eastl::string& GameManagerSlaveImpl::makeClientInfoLogStr(eastl::string& str) const
    {
        PeerInfo* peerInfo = ((gCurrentLocalUserSession != nullptr) ? gCurrentLocalUserSession->getPeerInfo() : nullptr);
        if (peerInfo != nullptr)
        {
            str.sprintf("Client info: ip[%s (real %s)]", peerInfo->getPeerAddress().getIpAsString(), peerInfo->getRealPeerAddress().getIpAsString());
            if (peerInfo->getClientInfo() != nullptr)
            {
                str.append_sprintf(", clientversion[%s], clientname[%s], sku[%s], platform[%s], sdkversion[%s]",
                    peerInfo->getClientInfo()->getClientVersion(), peerInfo->getClientInfo()->getClientName(), peerInfo->getClientInfo()->getClientSkuId(),
                    ClientPlatformTypeToString(peerInfo->getClientInfo()->getPlatform()), peerInfo->getClientInfo()->getBlazeSDKVersion());
            }
        }
        return str;
    }

    BlazeRpcError GameManagerSlaveImpl::whitelistCheckAddr(NetworkAddress & address, const char8_t* gameName, BlazeId blazeId)
    {
        InetAddress intAddr, extAddr;
        if (address.getActiveMember() == NetworkAddress::MEMBER_IPPAIRADDRESS)
        {
            intAddr.setIp(address.getIpPairAddress()->getInternalAddress().getIp(), InetAddress::HOST);
            extAddr.setIp(address.getIpPairAddress()->getExternalAddress().getIp(), InetAddress::HOST);

            //If the internal address fails the internal whitelist or isn't the same as the external address, clear the address
            if (!getInternalHostIpWhitelist()->match(intAddr) && intAddr != extAddr)
            {
                WARN_LOG("[GameManagerSlaveImpl:].createGameInternal: User " << blazeId
                    << " attempted to create game[" << gameName << "] with non-whitelisted internal address " << intAddr << ", setting to invalid");

                address.getIpPairAddress()->getInternalAddress().setIp(0);
                address.getIpPairAddress()->getInternalAddress().setPort(0);
            }
        }
        else if (address.getActiveMember() == NetworkAddress::MEMBER_IPADDRESS)
        {
            extAddr.setIp(address.getIpAddress()->getIp(), InetAddress::HOST);
        }

        //If the external address doesn't pass the whitelist, fail
        if (!getExternalHostIpWhitelist()->match(extAddr))
        {
            FAIL_LOG("[GameManagerSlaveImpl:].createGameInternal: User " << blazeId
                << " attempted to create game[" << gameName << "] with non-whitelisted exteranl address " << extAddr);
            return ERR_AUTHORIZATION_REQUIRED;
        }

        return ERR_OK;
    }


    bool GameManagerSlaveImpl::updateAndCheckUserReputations(const UserSessionIdList& userSessionIdList, const UserIdentificationList& externalUserList)
    {
        if (gUserSessionManager->isReputationDisabled())
            return false;

        bool foundPoorReputationUser = false;
        Blaze::ReputationService::ReputationServiceUtilPtr reputationUtil = gUserSessionManager->getReputationServiceUtil();

        foundPoorReputationUser = reputationUtil->doReputationUpdate(userSessionIdList, externalUserList);
        return foundPoorReputationUser;
    }

    bool GameManagerSlaveImpl::updateAndCheckUserReputations(UserJoinInfoList& usersInfo)
    {
        if (gUserSessionManager->isReputationDisabled())
            return false;

        UserSessionIdList sessionIds;
        UserIdentificationList externalUserList;

        sessionIds.reserve(usersInfo.size());
        externalUserList.reserve(usersInfo.size());
        for (UserJoinInfoList::const_iterator itr = usersInfo.begin(); itr != usersInfo.end(); ++itr)
        {
            if ((*itr)->getUser().getSessionId() != INVALID_USER_SESSION_ID)
            {
                sessionIds.push_back((*itr)->getUser().getSessionId());
            }
            else
            {
                UserInfo::filloutUserIdentification((*itr)->getUser().getUserInfo(), *externalUserList.pull_back());
            }
        }

        bool badRepFound = updateAndCheckUserReputations(sessionIds, externalUserList);

        // At this point, only the session's Extended DataMap stores the result of the update:

        // TODO: My understanding is that this will never get called somewhere other than a coreSlave
        EA_ASSERT(gUserSessionMaster != nullptr);

        // Update the data map and the bool value: 
        for (UserJoinInfoList::iterator itr = usersInfo.begin(); itr != usersInfo.end(); ++itr)
        {
            UserSessionMasterPtr session = gUserSessionMaster->getLocalSession((*itr)->getUser().getSessionId());
            if (session)
            {
                bool hasPoorReputation = ReputationService::ReputationServiceUtil::userHasPoorReputation(session->getExtendedData());
                (*itr)->getUser().setHasBadReputation(hasPoorReputation);
            }
        }
        return badRepFound;
    }

    bool GameManagerSlaveImpl::updateAndCheckUserReputations(UserSessionInfo& userInfo)
    {
        if (gUserSessionManager->isReputationDisabled())
            return false;

        UserJoinInfoList usersInfoList;
        userInfo.copyInto(usersInfoList.pull_back()->getUser());
        bool badRep = updateAndCheckUserReputations(usersInfoList);
        usersInfoList.back()->getUser().copyInto(userInfo);
        return badRep;
    }

    /*! ************************************************************************************************/
    /*! \brief sets up default team info if a teamID vector is empty and also calculates team capacity.

    \param[in\out] teamIds - teamIds provided, will insert ANY_TEAM_ID if empty.
    \param[in\out] joiningTeamIndex - if teamIds was empty, sets the joining team index to 0
    \param[in] participantCapacity - the total participant slots
    \return team capacity
    ***************************************************************************************************/
    uint16_t GameManagerSlaveImpl::setUpDefaultTeams(TeamIdVector &teamIds, PlayerJoinData& playerJoinData, uint16_t participantCapacity)
    {
        if (teamIds.empty())
        {
            // If the PJD already specifies player teams, they should be set, but here we make sure they're maintained:
            bool defaultTeamIdUsed = false;
            bool perPlayerTeamIdSet = false;
            for (auto& curPlayer : playerJoinData.getPlayerDataList())
            {
                if (curPlayer->getTeamId() != INVALID_TEAM_ID)
                {
                    perPlayerTeamIdSet = true;

                    bool teamFound = false;
                    TeamIndex teamIndex = 0;
                    for (auto& curTeam : teamIds)
                    {
                        if (curTeam == curPlayer->getTeamId())
                        {
                            teamFound = true;
                            break;
                        }
                        ++teamIndex;
                    }

                    if (teamFound == false)
                    {
                        teamIds.push_back(curPlayer->getTeamId());
                    }
                    curPlayer->setTeamIndex(teamIndex);
                }
                else
                {
                    defaultTeamIdUsed = true;
                }
            }

            if (perPlayerTeamIdSet == false)
            {
                teamIds.push_back(playerJoinData.getDefaultTeamId());
                playerJoinData.setDefaultTeamIndex(0);
            }
            else if (defaultTeamIdUsed)
            {
                bool teamFound = false;
                TeamIndex teamIndex = 0;
                for (auto& curTeam : teamIds)
                {
                    if (curTeam == playerJoinData.getDefaultTeamId())
                    {
                        teamFound = true;
                        break;
                    }
                    ++teamIndex;
                }

                if (teamFound == false)
                {
                    teamIds.push_back(playerJoinData.getDefaultTeamId());
                }
                playerJoinData.setDefaultTeamIndex(teamIndex);
            }
        }

        uint16_t perTeamPlayerCapacity  = participantCapacity / teamIds.size();

        // Assign players to teams if they don't set one:
        if (playerJoinData.getDefaultTeamIndex() == UNSPECIFIED_TEAM_INDEX)
        {
            bool anyPlayerHasTeamIndexSet = false;
            for (auto& curPlayer : playerJoinData.getPlayerDataList())
            {
                if (curPlayer->getTeamIndex() != UNSPECIFIED_TEAM_INDEX)
                {
                    anyPlayerHasTeamIndexSet = true;
                    break;
                }
            }
            // Make sure everyone is unspecified, since we only want to set a default use case:
            if (!anyPlayerHasTeamIndexSet)
            {
                // If everyone has no index set, just fill in the teams:
                uint16_t curPlayerIndex = 0;
                for (auto& curPlayerData : playerJoinData.getPlayerDataList())
                {
                    curPlayerData->setTeamIndex(curPlayerIndex / perTeamPlayerCapacity);
                    ++curPlayerIndex;
                }
            }
        }

        return perTeamPlayerCapacity;
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const
    {
        switch (tdf.getTdfId())
        {
        case CreateGameResponse::TDF_ID:
        {
            UserSessionManager::obfuscatePlatformInfo(platform, static_cast<CreateGameResponse*>(&tdf)->getJoinedReservedPlayerIdentifications());
        }
        break;
        case JoinGameResponse::TDF_ID:
        {
            UserSessionManager::obfuscatePlatformInfo(platform, static_cast<JoinGameResponse*>(&tdf)->getJoinedReservedPlayerIdentifications());
        }
        break;
        case NotifyMatchmakingReservedExternalPlayers::TDF_ID:
        {
            UserSessionManager::obfuscatePlatformInfo(platform, static_cast<NotifyMatchmakingReservedExternalPlayers*>(&tdf)->getJoinedReservedPlayerIdentifications());
        }
        break;
        case GameBrowserDataList::TDF_ID:
        {
            GameBrowserDataList::GameBrowserGameDataList& list = static_cast<GameBrowserDataList*>(&tdf)->getGameData();
            for (auto& it : list)
                obfuscatePlatformInfo(platform, *it);
        }
        break;
        case GetGameListSyncResponse::TDF_ID:
        {
            obfuscatePlatformInfo(platform, static_cast<GetGameListSyncResponse*>(&tdf)->getGameList());
        }
        break;
        case NotifyGameListUpdate::TDF_ID:
        {
            obfuscatePlatformInfo(platform, static_cast<NotifyGameListUpdate*>(&tdf)->getUpdatedGames());
        }
        break;
        case NotifyGameSetup::TDF_ID:
        {
            obfuscatePlatformInfo(platform, *(static_cast<NotifyGameSetup*>(&tdf)));
        }
        break;
        case GetFullGameDataResponse::TDF_ID:
        {
            GetFullGameDataResponse::ListGameDataList& list = static_cast<GetFullGameDataResponse*>(&tdf)->getGames();
            for (auto& it : list)
                obfuscatePlatformInfo(platform, *it);
        }
        break;
        case NotifyPlayerJoining::TDF_ID:
        {
            obfuscatePlatformInfo(platform, *(static_cast<NotifyPlayerJoining*>(&tdf)));
        }
        break;
        case NotifyMatchmakingPseudoSuccess::TDF_ID:
        {
            obfuscatePlatformInfo(platform, *(static_cast<NotifyMatchmakingPseudoSuccess*>(&tdf)));
        }
        break;
        case NotifyMatchmakingScenarioPseudoSuccess::TDF_ID:
        {
            PseudoSuccessList& list = static_cast<NotifyMatchmakingScenarioPseudoSuccess*>(&tdf)->getPseudoSuccessList();
            for (auto& it : list)
                obfuscatePlatformInfo(platform, *it);
        }
        break;
        default:
            ASSERT_LOG("[GameManagerSlaveImpl].obfuscatePlatformInfo: Platform info obfuscation not implemented for Tdf class " << tdf.getFullClassName());
            break;
        }
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::ReplicatedGameData& tdf)
    {
        if (platform != common)
        {
            if (platform != ps4)
            {
                tdf.setNpSessionId("");
                tdf.getExternalSessionIdentification().getPs4().setNpSessionId("");
            }
            if ((platform != xone) && (platform != xbsx))
            {
                tdf.setExternalSessionCorrelationId("");
                tdf.setExternalSessionName("");
                tdf.setExternalSessionTemplateName("");
                tdf.getExternalSessionIdentification().getXone().setCorrelationId("");
                tdf.getExternalSessionIdentification().getXone().setSessionName("");
                tdf.getExternalSessionIdentification().getXone().setTemplateName("");
            }
            if ((platform != ps4) && (platform != ps5))
            {
                tdf.getExternalSessionIdentification().getPs5().getMatch().setMatchId("");
                tdf.getExternalSessionIdentification().getPs5().getMatch().setActivityObjectId("");
                tdf.getExternalSessionIdentification().getPs5().getPlayerSession().setPlayerSessionId("");
            }
        }
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, Blaze::GameManager::NotifyGameSetup& tdf)
    {
        obfuscatePlatformInfo(platform, tdf.getGameData());
        obfuscatePlatformInfo(platform, tdf.getGameRoster());
        obfuscatePlatformInfo(platform, tdf.getGameQueue());
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, NotifyPlayerJoining& tdf)
    {
        if (platform != tdf.getJoiningPlayer().getPlatformInfo().getClientPlatform())
            tdf.getJoiningPlayer().setExternalId(INVALID_EXTERNAL_ID);
        UserSessionManager::obfuscate1stPartyIds(platform, tdf.getJoiningPlayer().getPlatformInfo());
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, GameBrowserMatchDataList& list)
    {
        for (auto& it : list)
            obfuscatePlatformInfo(platform, it->getGameData());
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, GameBrowserGameData& tdf)
    {
        if (platform != common)
        {
            tdf.setExternalSessionId(INVALID_EXTERNAL_ID);
            tdf.getHostNetworkAddressList().clear();
        }

        for (auto& it : tdf.getGameRoster())
        {
            if (platform != it->getPlatformInfo().getClientPlatform())
                it->setExternalId(INVALID_EXTERNAL_ID);
            UserSessionManager::obfuscate1stPartyIds(platform, it->getPlatformInfo());
        }
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, ListGameData& tdf)
    {
        obfuscatePlatformInfo(platform, tdf.getGame());
        obfuscatePlatformInfo(platform, tdf.getGameRoster());
        obfuscatePlatformInfo(platform, tdf.getQueueRoster());
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, ReplicatedGamePlayerList& list)
    {
        for (auto& it : list)
        {
            if (platform != it->getPlatformInfo().getClientPlatform())
                it->setExternalId(INVALID_EXTERNAL_ID);
            UserSessionManager::obfuscate1stPartyIds(platform, it->getPlatformInfo());
        }
    }

    void GameManagerSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, NotifyMatchmakingPseudoSuccess& tdf)
    {
        obfuscatePlatformInfo(platform, tdf.getFindGameResults().getFoundGame());
        for (auto& it : tdf.getCreateGameResults().getCreateGameRequest().getPlayerJoinData().getPlayerDataList())
            UserSessionManager::obfuscatePlatformInfo(platform, it->getUser());
    }

} // GameManager namespace
} // Blaze namespace
