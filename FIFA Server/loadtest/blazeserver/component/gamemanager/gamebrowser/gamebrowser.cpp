/*! ************************************************************************************************/
/*!
    \file gamebrowser.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/componentmanager.h"
#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/gamebrowser/gamebrowserlist.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/playerroster.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "gamemanager/rete/reterule.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/scenario.h"
#include "gamemanager/templateattributes.h"

#include "framework/userset/userset.h"

#include "EASTL/string.h" // for dumping list configurations to the log
#include "EASTL/functional.h"



namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<GameManager::ListType>* list_type = BLAZE_NEW TagInfo<GameManager::ListType>("list_type", [](const GameManager::ListType& value, Metrics::TagValue&) { return ListTypeToString(value); });
TagInfo<GameManager::GameBrowserListConfigName>* gamebrowser_list_config_name = BLAZE_NEW TagInfo<GameManager::GameBrowserListConfigName>("gamebrowser_list_config_name", [](const GameManager::GameBrowserListConfigName& value, Metrics::TagValue&) { return value.c_str(); });
}
}

namespace GameManager
{
    const GameBrowserListConfigName GameBrowser::DEDICATED_SERVER_LIST_CONFIG = "dedicatedServer";
    const GameBrowserListConfigName GameBrowser::USERSET_GAME_LIST_CONFIG = "usersetGameList";

    /*! ************************************************************************************************/
    /*! \brief Construct the GameBrowser obj. Must call configure before attempting to use GameBrowser.
    *************************************************************************************************/
    GameBrowser::GameBrowser(GameManagerSlaveImpl &gmSlave)
        : IdlerUtil("[GameBrowser]"),
          mObjectIdToGameListMap(BlazeStlAllocator("GameBrowser::mObjectIdToGameListMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mComponentSubscriptionCounts(BlazeStlAllocator("GameBrowser::mComponentSubscriptionCounts", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mGameManagerSlave(gmSlave),
          mAllLists(BlazeStlAllocator("GameBrowser::mAllLists", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mSubscriptionLists(BlazeStlAllocator("GameBrowser::mSubscriptionLists", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mTotalIdles(0),
          mGBMetrics(gmSlave.getMetricsCollection()),
          mActiveAllLists(gmSlave.getMetricsCollection(), "activeAllLists", [this]() { return mAllLists.size(); }),
          mActiveSubscriptionLists(gmSlave.getMetricsCollection(), "activeSubscriptionLists", [this]() { return mSubscriptionLists.size(); }),
          mLastIdleLength(gmSlave.getMetricsCollection(), "lastIdleLength"),
          mGameBrowserConfig(nullptr)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameBrowser::~GameBrowser()
    {
        mScenarioConfigByNameMap.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief Called when component that hosts game browser (Game Manager) has been resolved
    ***************************************************************************************************/
    bool GameBrowser::hostingComponentResolved()
    {
        return Search::SearchShardingBroker::registerForSearchSlaveNotifications();
    }

    /*! ************************************************************************************************/
    /*! \brief GB component is going down, kill anything outstanding.
    ***************************************************************************************************/
    void GameBrowser::onShutdown()
    {
        shutdownIdler();

        // we must destroy the lists before the games that the list references are destroyed
        GameBrowserListMap::iterator listIter = mAllLists.begin();
        GameBrowserListMap::iterator listEnd = mAllLists.end();
        for (;listIter != listEnd; listIter++)
        {
            delete listIter->second;
        }

        mAllLists.clear();
    }

    bool GameBrowser::validateConfig(const GameManagerServerConfig& configTdf, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        AttributeToTypeDescMap scenarioAttrToTypeDesc;

        GetGameListRequest validationRequest;
        EA::TDF::TdfGenericReference getGameListReqRef(validationRequest);
        EA::TDF::TdfGenericReference criteriaRef(validationRequest.getListCriteria());
        GameBrowserScenarioData scenarioData;
        EA::TDF::TdfGenericReference scenarioDataRef(scenarioData);

        const GameBrowserScenariosConfig& config = configTdf.getGameBrowserScenariosConfig();

        // Validate Global Attributes:
        {
            StringBuilder errorPrefix;
            errorPrefix << "[GameBrowser].validateConfig: Global attribute: ";
            validateTemplateAttributeMapping(config.getGlobalAttributes(), scenarioAttrToTypeDesc, getGameListReqRef, validationErrors, errorPrefix.get());
        }

        typedef eastl::map<eastl::string, eastl::set<eastl::string> > RuleAttributeMappingHelper;

        // Validate the Non-global Rules:
        for (const auto & curScenarioRule : config.getScenarioRuleMap())
        {
            if (curScenarioRule.second->size() != 1)
            {
                StringBuilder strBuilder;
                strBuilder << "[GameBrowser].validateConfig: Rule: " << curScenarioRule.first.c_str() <<
                    ", has " << ((curScenarioRule.second->size() > 1) ? "more" : "less") <<
                    " than one rule set currently. Custom rule name mappings must be one-to-one with the rule.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            ScenarioRuleAttributes::const_iterator ruleAttributes = curScenarioRule.second->begin();
            {
                StringBuilder errorPrefix;
                errorPrefix << "[GameBrowser:" << "].validateConfig: Rule: " << curScenarioRule.first.c_str() << " ";
                validateTemplateAttributeMapping(*ruleAttributes->second, scenarioAttrToTypeDesc, criteriaRef, validationErrors, errorPrefix.get(), ruleAttributes->first.c_str());
            }
        }

        // Validate the Scenarios themselves: 
        for (const auto & curScenario: config.getScenarios())
        {
            const ScenarioName& scenarioName = curScenario.first;
            const GameBrowserScenarioConfig& scenarioConfig = *(curScenario.second);
            // Check that the scenario's name isn't too long:
            if (scenarioName.length() >= MAX_SCENARIONAME_LEN)
            {
                StringBuilder strBuilder;
                strBuilder << "[GameBrowser].validateConfig: Scenario Name: " << scenarioName.c_str() <<
                    " has length (" << scenarioName.length() << ") >= MAX_SCENARIONAME_LEN (" << MAX_SCENARIONAME_LEN <<
                    "). This would prevent the scenario from being used, because the string name couldn't be sent from the client.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            if (scenarioConfig.getSubsessions().size() != 1)
            {
                StringBuilder strBuilder;
                strBuilder << "[GameBrowser].validateConfig: Scenario: " << scenarioName.c_str() <<
                    ", has " << ((scenarioConfig.getSubsessions().size() > 1) ? "more" : "less") <<
                    " than one subsession set currently. Game Browser Scenarios must have at exactly one subsession currently.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            // Verify all game browser setting attributes:
            {
                StringBuilder errorPrefix;
                errorPrefix << "[GameBrowser].validateConfig: Scenario: " << scenarioName.c_str() << ", Game browser setting";
                validateTemplateAttributeMapping(curScenario.second->getGameBrowserAttributes(), scenarioAttrToTypeDesc, scenarioDataRef, validationErrors, errorPrefix.get());
            }

            for (const auto& curSubSession: scenarioConfig.getSubsessions())
            {
                const SubSessionName& subSessionName = curSubSession.first;

                // Check that all of the rules listed actually exist: 
                RuleAttributeMappingHelper subsessionRuleList;

                for (const auto& curRuleName: curSubSession.second->getRulesList())
                {
                    ScenarioRuleMap::const_iterator curScenarioRuleTemp = config.getScenarioRuleMap().find(curRuleName);
                    if (curScenarioRuleTemp == config.getScenarioRuleMap().end())
                    {
                        StringBuilder strBuilder;
                        strBuilder << "[GameBrowser].validateConfig: Scenario: " << scenarioName.c_str() << ", SubSession: " << subSessionName.c_str() <<
                            " is using an unknown rule: " << curRuleName.c_str() << ".";
                        validationErrors.getErrorMessages().push_back(strBuilder.get());
                    }
                    else
                    {
                        // Check that the rule attributes in use don't overlap any of the other rules
                        ScenarioRuleAttributes::const_iterator ruleAttributes = curScenarioRuleTemp->second->begin();
                        if (ruleAttributes != curScenarioRuleTemp->second->end())
                        {
                            RuleName ruleName = ruleAttributes->first;
                            for (const auto& curRuleAttr: *(ruleAttributes->second))
                            {
                                ScenarioAttributeName attrName = curRuleAttr.first;
                                RuleAttributeMappingHelper::iterator subsessionRuleListItr = subsessionRuleList.find(ruleName.c_str());
                                if (subsessionRuleListItr == subsessionRuleList.end() ||
                                    subsessionRuleListItr->second.find(attrName.c_str()) == subsessionRuleListItr->second.end())
                                {
                                    subsessionRuleList[ruleName.c_str()].insert(attrName.c_str());
                                }
                                else
                                {
                                    StringBuilder strBuilder;
                                    strBuilder << "[GameBrowser].validateConfig: Scenario: " << scenarioName.c_str() << ", SubSession: " << subSessionName.c_str() <<
                                        " has overlapping rules for rulename: " << ruleName.c_str() << " attrName: " << attrName.c_str() << ".";
                                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                                }

                            }
                        }
                    }
                }

            }
        }

        const char8_t* dsDefaultScenario = config.getDedicatedServerLookupGBScenarioName();
        if (*dsDefaultScenario == '\0')
        {
            validationErrors.getErrorMessages().push_back("Default Dedicated Server lookup GameBrowser Scenario must be specified");
        }
        else
        {
            if (config.getScenarios().find(dsDefaultScenario) == config.getScenarios().end())
            {
                StringBuilder strBuilder;
                strBuilder << "[GameBrowser].validateConfig: Default Dedicated Server lookup GameBrowser Scenario(" << dsDefaultScenario <<
                    ") does not exist.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }


        if (configTdf.getGameBrowser().getMaxSortedListCapacityServerOverride() == 0)
        {
            validationErrors.getErrorMessages().push_back("Max Sorted List Capacity Server Override must be > 0");
        }

        if (configTdf.getGameBrowser().getUserSetListCapacity() == 0)
        {
            validationErrors.getErrorMessages().push_back("Max userset list capacity must be > 0");
        }

        return (validationErrors.getErrorMessages().size() == 0);
    }

    GameBrowserScenarioConfigInfo::GameBrowserScenarioConfigInfo(const ScenarioName& scenarioName, const GameBrowser* manager) :
        mScenarioName(scenarioName),
        mScenarioManager(manager),
        mOverallConfig(nullptr),
        mScenarioConfig(nullptr)
    {}

    /*! ************************************************************************************************/
    /*! \brief perform game browser scenario configuration (includes parsing the matchmaking rules config file & init
    all rule definitions).

    \param[in]  config - the config map containing the GameBrowserScenarios config block
    \returns false upon fatal config error
    *************************************************************************************************/
    bool GameBrowserScenarioConfigInfo::configure(const GameBrowserScenariosConfig& config)
    {
        GameBrowserScenarioMap::const_iterator scenarioIter = config.getScenarios().find(mScenarioName);
        if (scenarioIter == config.getScenarios().end())
        {
            ERR_LOG("[GameBrowserScenarioConfigInfo].configure: Scenario named " << mScenarioName.c_str() << " does not exist.");
            return false;
        }
        clearSessionConfigs();
        mOverallConfig = &config;
        mScenarioConfig = scenarioIter->second;

        for (auto const& subSession : mScenarioConfig->getSubsessions())
        {
            GameBrowserSubSessionConfigInfo* newSubSessionConfig = BLAZE_NEW GameBrowserSubSessionConfigInfo(subSession.first, this);
            mScenarioSessionConfigList.push_back(newSubSessionConfig);
            if (!newSubSessionConfig->configure(config))
            {
                ERR_LOG("[GameBrowserScenarioConfigInfo].configure: Error occurred while configuring subSession for scenario " << mScenarioName.c_str());
                return false;
            }
        }
        return true;
    }

    GameBrowserScenarioConfigInfo::~GameBrowserScenarioConfigInfo()
    {
        clearSessionConfigs();
    }

    void GameBrowserScenarioConfigInfo::clearSessionConfigs()
    {
        for (auto& itr : mScenarioSessionConfigList)
        {
            delete itr;
        }
        mScenarioSessionConfigList.clear();
    }

    void GameBrowserScenarioConfigInfo::deleteIfUnreferenced(bool clearRefs)
    {
        // When the last reference is destroyed, we use this to clear the reference:
        if (clearRefs)
            clear_references();

        if (is_unreferenced())
            delete this;
    }

    GameBrowserSubSessionConfigInfo::GameBrowserSubSessionConfigInfo(const SubSessionName& subSessionName, const GameBrowserScenarioConfigInfo* owner) :
        mSubSessionName(subSessionName),
        mOwner(owner),
        mOverallConfig(nullptr),
        mSubSessionConfig(nullptr)
    {}

    /*! ************************************************************************************************/
    /*! \brief perform  game browser subsession configuration.
    \param[in]  config - the config map containing the GameBrowserScenarios config block
    \returns false upon fatal config error
    *************************************************************************************************/

    bool GameBrowserSubSessionConfigInfo::configure(const GameBrowserScenariosConfig& config)
    {
        mOverallConfig = mOwner->getOverallConfig();

        const char8_t* scenarioName = getScenarioName();
        GameBrowserScenarioMap::const_iterator scenarioIter = config.getScenarios().find(scenarioName);
        if (scenarioIter == config.getScenarios().end())
        {
            ERR_LOG("[GameBrowserSubSessionConfigInfo:].configure: Scenario named " << scenarioName << " does not exist.");
            return false;
        }

        const GameBrowserScenarioConfig* scenarioConfig = scenarioIter->second;
        const GameBrowserSubSessionConfigMap& subSessions = scenarioConfig->getSubsessions();
        if (subSessions.find(mSubSessionName) == subSessions.end())
        {
            ERR_LOG("[GameBrowserSubSessionConfigInfo:].configure: Scenario subsession named " << mSubSessionName.c_str() << " does not exist.");
            return false;
        }

        // Collect subsession rules
        mSubSessionConfig = subSessions.find(mSubSessionName)->second;
        for (const auto & rule : mSubSessionConfig->getRulesList())
        {
            ScenarioRuleMap::const_iterator ruleIter = config.getScenarioRuleMap().find(rule);
            if (ruleIter == config.getScenarioRuleMap().end())
            {
                // Using a rule that doesn't exist anymore
                ERR_LOG("[GameBrowserSubSessionConfigInfo:].configure: Unable to find rule named " << rule.c_str()
                    << " for Scenario " << scenarioName << " subsession " << mSubSessionName.c_str() << ".");
                return false;
            }

            if (!addRule(*(ruleIter->second)))
            {
                // Invalid config
                ERR_LOG("[GameBrowserSubSessionConfigInfo:].configure: Unable to add rule " << rule.c_str()
                    << " for Scenario " << scenarioName << " subsession " << mSubSessionName.c_str() << ".");
                return false;
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief perform type registration for rule name and prepare rule attribute for the subsession which consists of local and global rules.

    \param[out]  ruleAttributes - attribute map
    \returns false upon fatal config error
    *************************************************************************************************/
    bool  GameBrowserSubSessionConfigInfo::addRule(const ScenarioRuleAttributes& ruleAttributes)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();

        for (auto const& curRule : ruleAttributes)
        {
            for (auto const& curRuleAttr : *(curRule.second))
            {
                // Lookup the name: 
                const char8_t* mappedValue = nullptr;
                if (!tdfFactory.lookupTypeName(curRule.first.c_str(), curRuleAttr.first.c_str(), mappedValue, true))
                {
                    // Unable to lookup mapped value:
                    ERR_LOG("[GameBrowserSubSessionConfigInfo].addRule: Unable to lookup type for Rule: " << curRule.first.c_str()
                        << " type: " << curRuleAttr.first.c_str());
                    return false;
                }

                EA::TDF::TdfGenericReference refValue;
                EA::TDF::TdfString stringValue = mappedValue;
                EA::TDF::TdfGenericReferenceConst refKey(stringValue);
                if (!mScenarioAttributeMapping.insertKeyGetValue(refKey, refValue))
                {
                    // Value is already mapped:
                    ERR_LOG("[GameBrowserSubSessionConfigInfo].addRule: Data value already exists for value: " << mappedValue << " Rule: " << curRule.first.c_str());
                    return false;
                }

                refValue.setValue(EA::TDF::TdfGenericReferenceConst(curRuleAttr.second));
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief builds up mScenarioConfigByNameMap which maps scenario name to scenario config.

    \param[in]  configTdf - the config map containing the GameManager server config.
    \param[in]  evaluateGameProtocolVersionString - the new value from the game session for evaluateGameProtocolVersionString.
    \returns false upon fatal config error
    *************************************************************************************************/
    bool GameBrowser::configure(const GameManagerServerConfig& configTdf, bool evaluateGameProtocolVersionString)
    {
        // Map the gb config 
        const GameBrowserScenariosConfig& gbConfig = configTdf.getGameBrowserScenariosConfig();

        // iterate over the scenarios:
        for (auto const& curScenario : gbConfig.getScenarios())
        {
            GameBrowserScenarioConfigInfo* newScenarioConfig = BLAZE_NEW GameBrowserScenarioConfigInfo(curScenario.first, this);
            mScenarioConfigByNameMap[curScenario.first] = newScenarioConfig;

            if (!newScenarioConfig->configure(gbConfig))
            {
                WARN_LOG("[Gamebrowser:" << "].configure(): Failed to load config for scenario (" << curScenario.first << ").");
                return false;
            }

        }

        mGameBrowserConfig = &(configTdf.getGameBrowser());
        GameBrowserListConfigMap::const_iterator iter = mGameBrowserConfig->getListConfig().begin();
        for (; iter != mGameBrowserConfig->getListConfig().end(); ++iter)
        {
            TRACE_LOG("[GameBrowser] List configureation " << iter->first << " has been configured.");
        }

        mGameBrowserScenarioConfig = &gbConfig;
        mEvaluateGameProtocolVersionString = evaluateGameProtocolVersionString;
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief reconfigure game browser configuration (includes parsing the matchmaking rules config file & init
    all rule definitions).

    \param[in]  configTdf - the config map containing the GameManagerServer config block
    \param[in]  evaluateGameProtocolVersionString - the new value from the game session for evaluateGameProtocolVersionString.
    \returns true if the new settings configured correctly, false if the old settings remain in use.
    *************************************************************************************************/
    bool GameBrowser::reconfigure(const GameManagerServerConfig& configTdf, bool evaluateGameProtocolVersionString)
    {
        TRACE_LOG("[GameBrowser] Starting GameBrowser reconfiguration.");

        // Delete all current lists since their MM criteria will be invalidated.
        GameBrowserListMap::iterator terminatedListIter;
        GameBrowserList *terminatedList;
        while (!mAllLists.empty())
        {
            // GB_AUDIT: efficiency: this list iteration could be faster - popping the elements like this makes the red/black tree
            //   pivot all over the place.  It's faster to just iterate once then clear.
            terminatedListIter = mAllLists.begin();
            terminatedList = terminatedListIter->second;

            // GB_AUDIT: efficiency: we build a 'proper' notification and send it down (contains a list of all removed games).
            //  we should refactor and only send a flag/enum when a list is being destroyed.  GameBrowser::NotifyGameListReason 
            //  would migrate to the tdf file and become part of the notification (extending/replacing the bool isFinished value).

            // Mark all games in the list as removed and send the notification.
            terminatedList->removeAllGames();
            terminatedList->sendListUpdateNotifications(getMaxGamesPerListUpdateNotification(), getMaxListUpdateNotificationsPerIdle(), LIST_DESTROYED);
            destroyGameList(terminatedListIter);
        }

        mScenarioConfigByNameMap.clear();

        if (!configure(configTdf, evaluateGameProtocolVersionString))
        {
            ERR_LOG("[GameBrowser] Unexpected problem with GameBrowser::configure (should always return true). Please update your configuration settings and retry.");
            return false;
        }

        TRACE_LOG("[GameBrowser] GameBrowser reconfiguration complete");
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief return a ListConfig object provided its name

        \param[in]  name - the name of the ListConfig object to get
        \returns the ListConfig object corresponding to name or nullptr if there is no ListConfig object of that name
    *************************************************************************************************/
    const GameBrowserListConfig *GameBrowser::getListConfig(const char8_t *name) const
    {
        if (mGameBrowserConfig)
        {
            GameBrowserListConfigMap::const_iterator listConfigMapIter = mGameBrowserConfig->getListConfig().find(name);
            if (listConfigMapIter != mGameBrowserConfig->getListConfig().end())
            {
                return listConfigMapIter->second;
            }
            listConfigMapIter = mGameBrowserConfig->getInternalListConfig().find(name);
            if (listConfigMapIter != mGameBrowserConfig->getInternalListConfig().end())
            {
                return listConfigMapIter->second;
            }
        }
        return nullptr;
    }



    /*! ************************************************************************************************/
    /*! \brief 'runs' the gameBrowser for one simulation tick.
        \return true if another idle loop needs to be scheduled
    *************************************************************************************************/
    IdlerUtil::IdleResult GameBrowser::idle()
    {
        ++mTotalIdles;
        TimeValue idleStartTime = TimeValue::getTimeOfDay();

        TRACE_LOG("[GameBrowser] Entering idle " << mTotalIdles << " at time " << idleStartTime.getMillis());

        sendListUpdateNotifications();

        mMetrics.mTotalGameUpdates += mIdleMetrics.mNewGameUpdatesByIdleEnd;

        TimeValue endTime = TimeValue::getTimeOfDay();
        mLastIdleLength.set((uint64_t)(endTime - idleStartTime).getMillis());
        TRACE_LOG("[GameBrowser] Exiting idle " << mTotalIdles << "; idle duration " << mLastIdleLength.get() << " ms, timeSinceLastIdle " 
                  << (endTime - mIdleMetrics.mLastIdleEndTime).getMillis() 
                  << " ms,  '" << mAllLists.size() 
                  << "' lists('" << mSubscriptionLists.size() << "' subscriptions), '" << mIdleMetrics.mNewListsAtIdleStart << "' new lists, '" 
                  << mIdleMetrics.mListsDestroyedAtIdleStart << "' newly destroyed, '");

        SPAM_LOG("[GameBrowser] idle " << mTotalIdles << "; Create List Timmings: list avg "
            << (mIdleMetrics.mNewListsAtIdleStart == 0 ? 0 : mIdleMetrics.mFilledAtCreateList_SumTimes.getMillis() / mIdleMetrics.mNewListsAtIdleStart)
            << " ms, max " << mIdleMetrics.mFilledAtCreateList_MaxTime.getMillis() << " ms, min " << mIdleMetrics.mFilledAtCreateList_MinTime.getMillis()
            << " ms '" << mIdleMetrics.mGamesMatchedAtCreateList << "' initial matched games, '" << mIdleMetrics.mNewGameUpdatesByIdleEnd
            << "' total new games matched ('" << mIdleMetrics.mNewGameUpdatesByIdleEnd - mIdleMetrics.mGamesMatchedAtCreateList
            << "' non-immed");

        // Clear idle metrics
        mIdleMetrics.reset();
        mIdleMetrics.mLastIdleEndTime = TimeValue::getTimeOfDay();

        return (mAllLists.empty() ? NO_NEW_IDLE : SCHEDULE_IDLE_NORMALLY);
    }


    /*! ************************************************************************************************/
    /*! \brief sends list update notifications for all game browser lists.
    ***************************************************************************************************/
    void GameBrowser::sendListUpdateNotifications()
    {
        GameBrowserListMap::iterator iter = mAllLists.begin();
        GameBrowserListMap::iterator end = mAllLists.end();
        while (iter != end)
        {
            GameBrowserList *list = iter->second;
            ++iter;
            if (!list->getHiddenFromIdle())
            {
                // for snapshot list don't send anything until full result set is formed
                if (list->getListType() == GAME_BROWSER_LIST_SNAPSHOT && !list->isFullResultSetReceived())
                    continue;

                bool sentFinalUpdate = list->sendListUpdateNotifications(getMaxGamesPerListUpdateNotification(), getMaxListUpdateNotificationsPerIdle(), LIST_UPDATED, list->getListCreateMsgNum());
                if (sentFinalUpdate && list->getListType() == GAME_BROWSER_LIST_SNAPSHOT)
                {
                    // ignore error here as there's nothing we can do about it
                    BlazeRpcError err = destroyGameList(list->getListId());
                    if (err != Blaze::ERR_OK)
                    {
                        TRACE_LOG("[GameBrowser].sendListUpdateNotifications: Failed to destroy list [id] " << list->getListId());
                    }
                }
            }
        }

    }


    /*! ************************************************************************************************/
    /*! \brief destroy any/all game browser lists a user is subscribed to.
        \param[in] user the user who's logging out
    ***************************************************************************************************/
    void GameBrowser::onUserSessionExtinction(UserSessionId sessionId)
    {
        eastl::pair<GameBrowserListUserSessionMap::const_iterator, GameBrowserListUserSessionMap::const_iterator>
            range = mGameBrowserListUserSessionMap.equal_range(sessionId);

        GameBrowserListUserSessionMap::const_iterator listDestroyIter = range.first;

        while (listDestroyIter != range.second)
        {
            //destroy owned lists
            GameBrowserListId listId = listDestroyIter->second;

            // the element will be removed from mGameBrowserListUserSessionMap
            // in one of the following destroyGameList calls, so just increment iterator
            // here before it becomes invalid
            listDestroyIter++;

            GameBrowserListMap::iterator listIter = mAllLists.find(listId);
            if (listIter != mAllLists.end())
            {
                // we don't care about remote lists on search slave here 
                // because search slave has its own onUserSessionLogout handler
                terminateGameBrowserList(*listIter->second, false /* terminateRemoteList */);

                // listIter may become invalid after terminateGameBrowserList() which blocks
                listIter = mAllLists.find(listId);
                if (listIter != mAllLists.end())
                    destroyGameList(listIter);
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief update the internal data for mapping game and user for "userset game list"

        \param[in]  EA::TDF::ObjectId for the updated game.
        \param[in] blazeId the user
        \param[in] userSession userSession
    ***************************************************************************************************/
    void GameBrowser::onAddSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession*)
    {
        //Schedule a blocking fiber since we can't be sure what context we'll be in when this executes
        Fiber::CreateParams params;
        params.blocking = true;
        params.namedContext = "GameBrowser::executeSubscriptionChange";
        gFiberManager->scheduleCall(this, &GameBrowser::updateUserSetGameList, compObjectId, blazeId, true, params);
    }


    /*! ************************************************************************************************/
    /*! \brief update the internal data for mapping game and user for "userset game list"

        \param[in]  EA::TDF::ObjectId for the updated game.
        \param[in] blazeId the user
        \param[in] userSession userSession
    ***************************************************************************************************/
    void  GameBrowser::onRemoveSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession*)
    {
        //Schedule a blocking fiber since we can't be sure what context we'll be in when this executes
        Fiber::CreateParams params;
        params.blocking = true;
        params.namedContext = "GameBrowser::executeSubscriptionChange";
        gFiberManager->scheduleCall(this, &GameBrowser::updateUserSetGameList, compObjectId, blazeId, false, params);
    }

    BlazeRpcError GameBrowser::processGetUserSetGameListSubscription(
        const GetUserSetGameListSubscriptionRequest &request, GetGameListResponse &response, MatchmakingCriteriaError &error, const Message *message)
    {
        if (message == nullptr)
        {
            return ERR_SYSTEM;
        }

        BlazeIdList friendsBlazeIdList;
        EA::TDF::ObjectId blazeObjectId = request.getUserSetId();
        BlazeRpcError rc = gUserSetManager->getUserBlazeIds(blazeObjectId, friendsBlazeIdList);
        if (rc != Blaze::ERR_OK)
        {
            return GAMEBROWSER_ERR_CANNOT_GET_USERSET;
        }

        GameBrowserListId listId;
        if (getNextUniqueSessionId(listId) != ERR_OK)
        {
            WARN_LOG("[GameBrowser::processGetUserSetGameListSubscription]: Error fetching next session id.")
                return ERR_SYSTEM;
        }

        const GameBrowserListConfig *listConfig = getListConfig(USERSET_GAME_LIST_CONFIG);
        if (listConfig == nullptr)
        {
            WARN_LOG("[GameBrowserList].initializeUsersetGameList config UsersetGameList required");
            return GAMEBROWSER_ERR_INVALID_LIST_CONFIG_NAME;
        }

        UserSetGameBrowserList* newList = BLAZE_NEW UserSetGameBrowserList(*this, listId, USERSET_GAME_LIST_CONFIG.c_str(), blazeObjectId, listConfig->getMaxUpdateInterval(), UserSession::getCurrentUserSessionId(), message->getMsgNum());

        if ((++mComponentSubscriptionCounts[blazeObjectId.type.component]) == 1)
        {
            rc = gUserSetManager->addSubscriberToProvider(blazeObjectId.type.component, *this);
        }
        mObjectIdToGameListMap[blazeObjectId].insert(newList);

        // hide from idle until the list is fully created
        newList->setHiddenFromIdle(true);

        mAllLists.insert(eastl::make_pair(newList->getListId(), newList));
        mSubscriptionLists.insert(eastl::make_pair(newList->getListId(), newList));

        Search::CreateUserSetGameListRequest req;
        req.setGameBrowserListId(newList->getListId());
        req.setUserSessionId(newList->getOwnerSessionId());
        for (BlazeIdList::const_iterator friendsIt = friendsBlazeIdList.begin(), friendsItEnd = friendsBlazeIdList.end(); friendsIt != friendsItEnd; ++friendsIt)
        {
            if (*friendsIt == UserSession::getCurrentUserBlazeId())
                continue;

            req.getInitialWatchList().push_back(*friendsIt);
        }

        //Now create the list on all search slaves
        // create remote list  (Async event - List may be deleted!)
        uint32_t numberOfGamesToBeDownloaded;
        BlazeRpcError errCode = SearchShardingBroker::convertSearchComponentErrorToGameManagerError(createUserSetGameList(*newList, req, numberOfGamesToBeDownloaded));
        
        // Check if the List is still alive after Async op:
        if (mAllLists.find(listId) == mAllLists.end())
        {
            // new list is gone after blocking call ... possibly due to a reconfigure
            TRACE_LOG("[GameBrowser::processGetUserSetGameListSubscription]: User [id] " << UserSession::getCurrentUserBlazeId()
                << " tried to create a new game list, but the new game list disappeared: " << ErrorHelp::getErrorName(errCode)
                << " (ERR_OK will be changed to ERR_SYSTEM)");

            // The removal code decremented this value before it was incremented, so we have to up it here so the metrics don't get out of sync:
            mGBMetrics.mActiveLists.increment(1, GAME_BROWSER_LIST_SUBSCRIPTION, USERSET_GAME_LIST_CONFIG.c_str());
            if (errCode == ERR_OK)
            {
                errCode = ERR_SYSTEM;
            }

            // assume that it's already been deleted & removed from any tracking lists/maps
            return errCode;
        }
        
        if (errCode != ERR_OK)
        {
            mAllLists.erase(listId);
            mSubscriptionLists.erase(listId);
            destroyUserSetGameList(*newList);
            delete newList;
            return errCode;
        }

        // list is ready to start sending results
        newList->setHiddenFromIdle(false);
        newList->setActualResultSetSize(numberOfGamesToBeDownloaded);

        //match users to lists to improve onUserSessionLogout efficiency
        mGameBrowserListUserSessionMap.insert(eastl::make_pair(newList->getOwnerSessionId(), newList->getListId()));

        TRACE_LOG("[GameBrowser].processGetUserSetGameListSubscription: UserSetGameList GameBrowserList(" << newList->getListId()
            << ") created, (" << friendsBlazeIdList.size() << ") watchee added");

        mGBMetrics.mCreatedLists.increment(1, newList->getListType(), newList->getListConfigName().c_str());
        mGBMetrics.mActiveLists.increment(1, newList->getListType(), newList->getListConfigName().c_str());
        mGBMetrics.mInitialGamesSent.increment(numberOfGamesToBeDownloaded, newList->getListType(), newList->getListConfigName().c_str());

        response.setListId(newList->getListId());
        response.setNumberOfGamesToBeDownloaded(numberOfGamesToBeDownloaded);
        response.setMaxUpdateInterval(newList->getMaxUpdateIntervalPlusGrace());

        return rc;
    }

    BlazeRpcError GameBrowser::getGameBrowserScenarioAttributesConfig(GetGameBrowserScenariosAttributesResponse& response)
    {
        // For each config, get the info:
        for (const auto& scenarioIter : mScenarioConfigByNameMap)
        {
            ScenarioAttributeMapping* attrMapping = response.getGameBrowserScenarioAttributes().allocate_element();
            response.getGameBrowserScenarioAttributes()[scenarioIter.first] = attrMapping;
            
            for (const auto& subsession : scenarioIter.second->getScenarioSessionConfigList())
            {
                // Add the Rule attributes: (local)
                addAttributeMapping(attrMapping, subsession->getScenarioAttributeMapping());
            }

            // Add the Game Browser attributes:
            addAttributeMapping(attrMapping, scenarioIter.second->getScenarioConfig()->getGameBrowserAttributes());
        }

        return ERR_OK;
    }

    void GameBrowser::addAttributeMapping(ScenarioAttributeMapping* attrMapping, const ScenarioAttributeMapping& scenarioAttributeMapping)
    {
        for (const auto& curAttr : scenarioAttributeMapping)
        {
            ScenarioAttributeMapping::iterator iter = attrMapping->find(curAttr.first);
            if (iter == attrMapping->end())
            {
                ScenarioAttributeDefinition* attrDef = attrMapping->allocate_element();
                curAttr.second->copyInto(*attrDef);
                (*attrMapping)[curAttr.first] = attrDef;
            }
            else
            {
                curAttr.second->copyInto(*(iter->second));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Transforms GetGameListScenarioRequest to GetGameListRequest. It is the entry point for gamebrowser scenarios.

    \param[in] request the request object
    \param[out] response the response object
    \param[out] error the matchmaking error object
    \param[in] msgNum allows using notification cache's fetch by seq no
    \return the blaze error code
    *************************************************************************************************/
    Blaze::BlazeRpcError GameBrowser::processCreateGameListForScenario(GetGameListScenarioRequest& request,
        GetGameListResponse &response, MatchmakingCriteriaError &error, uint32_t msgNum /*=0*/)
    {
        // Convert the Player Join Data (for back compat)
        for (auto playerJoinData : request.getPlayerJoinData().getPlayerDataList())
            convertToPlatformInfo(playerJoinData->getUser());

        // BLOCKING CALL - All Reconfigurable data Must be REACQUIRED.
        if (!mGameManagerSlave.getExternalDataManager().fetchAndPopulateExternalData(request))
        {
            WARN_LOG("[GameBrowser].processCreateGameListForScenario: Failed to fetch and populate external data in scenario(" << request.getGameBrowserScenarioName() << ")");
        }

        GameBrowserScenarioData scenarioData;
        BlazeRpcError errCode = transformScenarioRequestToBaseRequest(scenarioData, request, error);
        if (errCode != ERR_OK)
        {
            return errCode;
        }

        ListType listType = scenarioData.getIsSnapshotList() ? GAME_BROWSER_LIST_SNAPSHOT : GAME_BROWSER_LIST_SUBSCRIPTION;
        errCode = processCreateGameList(scenarioData.getGetGameListRequest(), listType, response, error, msgNum);

        // Fill additional attributes of GetGameListResponse 
        response.setIsSnapshotList(scenarioData.getIsSnapshotList());
        response.setListCapacity(scenarioData.getGetGameListRequest().getListCapacity());
        return errCode;
    }

    /*! ************************************************************************************************/
    /*! \brief Transforms GetGameListScenarioRequest to GameBrowserScenarioData using GameBrowser scenario attributes and GameBrowser scenario name.
    \param[out] game browser scenario data
    \param[in] request the request object
    \return the blaze error code
    *************************************************************************************************/
    Blaze::BlazeRpcError GameBrowser::transformScenarioRequestToBaseRequest(GameBrowserScenarioData& scenarioData, GetGameListScenarioRequest& request, MatchmakingCriteriaError &error)
    {
        const char8_t* failingAttr = "";
        BlazeRpcError finalResult = ERR_OK;
        StringBuilder componentDescription;
        componentDescription << "In GameBrowser::transformScenarioRequestToBaseRequest for scenario: " << request.getGameBrowserScenarioName();

        StringBuilder buf;

        // Check if the scenario name is present:
        ScenarioConfigByNameMap::const_iterator curScenarioConfig = mScenarioConfigByNameMap.find(request.getGameBrowserScenarioName());
        if (curScenarioConfig == mScenarioConfigByNameMap.end())
        {
            error.setErrMessage((buf << "[GameBrowser:" << "].transformScenarioRequestToBaseRequest() - Client requested non-existent scenario: " << request.getGameBrowserScenarioName() << ".").get());
            ERR_LOG(buf.get());
            return GAMEMANAGER_ERR_INVALID_SCENARIO_NAME;
        }

        // Apply Common data (protocol version) first, so it can be overridden via config attributes:
        request.getCommonGameData().copyInto(scenarioData.getGetGameListRequest().getCommonGameData());

        // PACKER_TODO:  Gather the properties used by the filters and build the property map that is required.
        PropertyNameMap properties;

        BlazeRpcError globalResult = applyTemplateAttributes(EA::TDF::TdfGenericReference(scenarioData.getGetGameListRequest()), mGameBrowserScenarioConfig->getGlobalAttributes(), request.getGameBrowserScenarioAttributes(), properties, failingAttr, componentDescription.get());
        if (globalResult != ERR_OK)
        {
            error.setErrMessage((buf << "Unable to apply default from config for the attribute \"" << failingAttr << "\" in scenario \"" << request.getGameBrowserScenarioName() << "\", sub session \"(Global)\"").get());

            // Switch from the TEMPLATE error, to the SCENARIO one:
            globalResult = (globalResult == GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE) ? GAMEMANAGER_ERR_INVALID_SCENARIO_ATTRIBUTE : GAMEMANAGER_ERR_MISSING_SCENARIO_ATTRIBUTE;
            return globalResult;
        }

        BlazeRpcError result1 = applyTemplateAttributes(EA::TDF::TdfGenericReference(scenarioData), curScenarioConfig->second->getScenarioConfig()->getGameBrowserAttributes(), request.getGameBrowserScenarioAttributes(), properties, failingAttr, componentDescription.get());
    
        // Just 1 subsession
        for (const auto &subsession : curScenarioConfig->second->getScenarioSessionConfigList())
        {
            componentDescription.reset();
            componentDescription << "In GameBrowser::transformScenarioRequestToBaseRequest for scenario: " << request.getGameBrowserScenarioName() << " and subsession: " << subsession->getSubsessionName().c_str();
            BlazeRpcError result2 = applyTemplateAttributes(EA::TDF::TdfGenericReference(scenarioData.getGetGameListRequest().getListCriteria()), subsession->getScenarioAttributeMapping(), request.getGameBrowserScenarioAttributes(), properties, failingAttr, componentDescription.get());
            finalResult = ((result1 != ERR_OK) ? (result1) : ((result2 != ERR_OK) ? result2 : ERR_OK)) ;

            if (finalResult != ERR_OK)
            {
                error.setErrMessage((buf << "Unable to apply default from config for the attribute \"" << failingAttr << "\" in scenario \"" << request.getGameBrowserScenarioName() << "\", sub session \"" << subsession->getSubsessionName().c_str()<< "\"").get());

                // Switch from the TEMPLATE error, to the SCENARIO one:
                finalResult = (finalResult == GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE) ? GAMEMANAGER_ERR_INVALID_SCENARIO_ATTRIBUTE : GAMEMANAGER_ERR_MISSING_SCENARIO_ATTRIBUTE;
                return finalResult;
            }
        }
        
        // Override with data from the client: (Player Join Data cannot be overridden)
        request.getPlayerJoinData().copyInto(scenarioData.getGetGameListRequest().getPlayerJoinData());

        return finalResult;
    }

    /*! ************************************************************************************************/
    /*! \brief create a game list

        \param[in] request the request object
        \param[in] listType the type of list, subscription or snapshot
        \param[out] response the response object
        \param[out] error the matchmaking error object
        \param[in] msgNum allows using notification cache's fetch by seq no
        \return the blaze error code
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameBrowser::processCreateGameList(GetGameListRequest& request, ListType listType,
        GetGameListResponse &response, MatchmakingCriteriaError &error, uint32_t msgNum /*=0*/)
    {
        // Convert the Player Join Data (for back compat)
        for (auto playerJoinData : request.getPlayerJoinData().getPlayerDataList())
            convertToPlatformInfo(playerJoinData->getUser());

        GameBrowserList* newList = nullptr;
        uint32_t gamesToBeDownloaded;
        FitScore maxPossibleFitScore = 0;
        BlazeRpcError errCode = createGameList(request, listType, newList, error, msgNum, gamesToBeDownloaded, maxPossibleFitScore);
        if (errCode != ERR_OK)
        {
            return errCode;
        }

        // setup response
        response.setListId(newList->getListId());
        response.setMaxUpdateInterval(newList->getMaxUpdateIntervalPlusGrace());
        response.setMaxPossibleFitScore(maxPossibleFitScore);

        if (newList->getListType() == GAME_BROWSER_LIST_SNAPSHOT)
        {
            response.setNumberOfGamesToBeDownloaded(gamesToBeDownloaded);
            if (gamesToBeDownloaded == 0)
            {
                newList->sendListUpdateNotifications(getMaxGamesPerListUpdateNotification(), getMaxListUpdateNotificationsPerIdle(), LIST_DESTROYED, msgNum);
                errCode = destroyGameList(newList->getListId());
                if (errCode != Blaze::ERR_OK)
                {
                    TRACE_LOG("[GameBrowser:" << "].processCreateGameList()"
                        " Failed to terminate Snapshot list, please look at log lines above for list id.");
                }
            }
        }

        scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());
        ++mIdleMetrics.mNewListsAtIdleStart;

        return Blaze::ERR_OK;
    }

    void GameBrowser::sendNotifyGameListUpdateToUserSessionById(UserSessionId ownerSessionId, NotifyGameListUpdate* notifyGameListUpdate, bool sendImmediately, uint32_t msgNum)
    {
        // TODO_MC: Do we need to keep the sequence number?
        //NotificationSendOpts opts;
        //opts.sendImmediately = sendImmediately;
        //opts.seqNo = msgNum;
        mGameManagerSlave.sendNotifyGameListUpdateToUserSessionById(ownerSessionId, notifyGameListUpdate, sendImmediately);
    }

    BlazeRpcError GameBrowser::getNextUniqueSessionId(uint64_t& listId) const
    {
        return mGameManagerSlave.getNextUniqueGameBrowserSessionId(listId);
    }

    Blaze::BlazeRpcError GameBrowser::processGetGameListSnapshotSync(GetGameListRequest &request, GetGameListSyncResponse &response, MatchmakingCriteriaError &error, uint16_t requiredPlayerCapacity,
        const char8_t* preferredPingSite, GameNetworkTopology netTopology, const UserSessionInfo* ownerUserInfo)
    {
        // Convert the Player Join Data (for back compat)
        for (auto playerJoinData : request.getPlayerJoinData().getPlayerDataList())
            convertToPlatformInfo(playerJoinData->getUser());

        if (request.getListCapacity() > getMaxGameListSyncSize())
        {
            if (request.getListCapacity() == GAME_LIST_CAPACITY_UNLIMITED)
            {
                request.setListCapacity(getMaxGameListSyncSize());
            }
            else
            {
                return GAMEBROWSER_ERR_EXCEED_MAX_SYNC_SIZE;
            }
        }

        uint32_t gamesToBeDownloaded = 0;
        FitScore maxPossibleFitScore = 0;
        GameBrowserList* newList = nullptr;
        uint32_t msgNum = 0; // async notification cache not used
        BlazeRpcError errCode = createGameList(request, GAME_BROWSER_LIST_SNAPSHOT, newList, error, msgNum, gamesToBeDownloaded, maxPossibleFitScore, requiredPlayerCapacity, preferredPingSite, netTopology, ownerUserInfo);
        if (errCode != ERR_OK)
        {
            return errCode;
        }

        response.setMaxPossibleFitScore(maxPossibleFitScore);
        response.setNumberOfGamesToBeDownloaded(gamesToBeDownloaded);

        BlazeRpcError returnErr = newList->fillSyncGameList(response.getNumberOfGamesToBeDownloaded(), response.getGameList());

        errCode = destroyGameList(newList->getListId());

        if (errCode != Blaze::ERR_OK)
        {
            TRACE_LOG("[GameBrowser:" << "].processGetGameListSnapshotSync()"
                " Failed to terminate Snapshot Sync list, please look at log lines above for list id.");
        }

        return returnErr;
    }

    /*! ************************************************************************************************/
    /*! \brief destroy a game list

        \param[in] request the request object
        \return the id of the newly created list.
    ***************************************************************************************************/
    BlazeRpcError GameBrowser::processDestroyGameList(const DestroyGameListRequest& request)
    {
        GameBrowserListMap::iterator listMapIter = mAllLists.find(request.getListId());

        if (listMapIter == mAllLists.end())
        {
            return ERR_OK;
        }

        if (EA_UNLIKELY(listMapIter->second->getOwnerSessionId() != UserSession::getCurrentUserSessionId()))
        {
            return GAMEBROWSER_ERR_NOT_LIST_OWNER;
        }

        return destroyGameList(request.getListId());
    }

    /*! ************************************************************************************************/
    /*! \brief destroys a game list given a listId
        \param[in] gameBrowserListId - The id of the list to be destroyed
    *************************************************************************************************/
    BlazeRpcError GameBrowser::destroyGameList(GameBrowserListId gameBrowserListId)
    {
        TRACE_LOG("[GameBrowser].destroyGameList() looking for remote list " << gameBrowserListId);
        GameBrowserListMap::iterator listIter = mAllLists.find(gameBrowserListId);
        if (listIter != mAllLists.end())
        {
            TRACE_LOG("[GameBrowser].destroyGameList() terminating remote list " << gameBrowserListId);
            BlazeRpcError err = terminateGameBrowserList(*listIter->second);
            if (err != Blaze::ERR_OK)
            {
                WARN_LOG("[GameBrowser].destroyGameList()"
                    " Unable to terminate remote list " << gameBrowserListId << ", err:" << ErrorHelp::getErrorName(err));
            }

            // listIter may become invalid after terminateGameBrowserList() which blocks
            listIter = mAllLists.find(gameBrowserListId);
            if (listIter != mAllLists.end())
                destroyGameList(listIter);
        }

        return ERR_OK;
    }

    void GameBrowser::removeGameListFromUserOwnership(UserSessionId userSessionId, GameBrowserListId listId)
    {
        eastl::pair<GameBrowserListUserSessionMap::iterator, GameBrowserListUserSessionMap::iterator>
            range = mGameBrowserListUserSessionMap.equal_range(userSessionId);

        for (GameBrowserListUserSessionMap::iterator iter = range.first; iter != range.second; ++iter)
        {
            if (iter->second == listId)
            {
                mGameBrowserListUserSessionMap.erase(iter);
                break;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destroys a game list given an iterator into mAllLists
        \param[in] gameBrowserListId - The id of the list to be destroyed
    *************************************************************************************************/
    void GameBrowser::destroyGameList(const GameBrowserListMap::iterator& listIter)
    {
        GameBrowserList *gameBrowserList = listIter->second;
        mAllLists.erase(listIter);
        ++mIdleMetrics.mListsDestroyedAtIdleStart;


        if (gameBrowserList->getListType() == GAME_BROWSER_LIST_SUBSCRIPTION)
        {
            mSubscriptionLists.erase(gameBrowserList->getListId());
        }

        removeGameListFromUserOwnership(gameBrowserList->getOwnerSessionId(), gameBrowserList->getListId());

        if (gameBrowserList->isUserSetList())
        {
            destroyUserSetGameList(*(UserSetGameBrowserList*)gameBrowserList);
        }

        // User set lists are tracked by hardcoded config name.
        mGBMetrics.mActiveLists.decrement(1, gameBrowserList->getListType(), gameBrowserList->getListConfigName().c_str());

        TRACE_LOG("[GameBrowser] GameBrowserList " << gameBrowserList->getListId() << " destroyed.");
        delete gameBrowserList;
    }

    void GameBrowser::destroyUserSetGameList(UserSetGameBrowserList& list)
    {
        BlazeObjectIdToGameListMap::iterator it = mObjectIdToGameListMap.find(list.getUserSetId());
        if (it != mObjectIdToGameListMap.end())
        {
            it->second.erase(&list);
            if (it->second.empty())
            {
                mObjectIdToGameListMap.erase(it);
                if ((--mComponentSubscriptionCounts[list.getUserSetId().type.component]) == 0)
                {
                    BlazeRpcError rc = gUserSetManager->removeSubscriberFromProvider(list.getUserSetId().type.component, *this);
                    if (rc != ERR_OK)
                    {
                        WARN_LOG("[GameBrowser].destroyUserSetGameList removeSubscriberFromProvider failed with err " << ErrorHelp::getErrorName(rc) << ".");
                    }
                }
            }
        }
    }

    void GameBrowser::onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data, UserSession*)
    {
        // lists were updated, we need idle run to send down notifications
        eastl::hash_map<GameId, eastl::hash_map<eastl::string, GameBrowserGameDataPtr> > gameNotifications(BlazeStlAllocator("GameBrowser::gameNotifications", GameManagerSlave::COMPONENT_MEMORY_GROUP));
        for (Search::NotifyGameListUpdate::GameDataMapMap::const_iterator itr = data.getGameDataMap().begin(), end = data.getGameDataMap().end(); itr != end; ++itr)
        {
            const GameBrowserListConfigName& name = itr->first;
            const GameBrowserListConfig* config = getListConfig(name.c_str());
            if (config != nullptr)
            {
                EA::TDF::TdfStructVector<GameManager::GameBrowserGameData >& vec = *itr->second;
                for (EA::TDF::TdfStructVector<GameManager::GameBrowserGameData >::iterator vitr = vec.begin(), vend = vec.end(); vitr != vend; ++vitr)
                {
                    GameBrowserGameDataPtr ptr = BLAZE_NEW GameBrowserGameData;
                    (*vitr)->copyInto(*ptr);
                    gameNotifications[(*vitr)->getGameId()][name.c_str()] = ptr;
                }
            }
        }

        for (Search::GameListUpdateList::const_iterator it = data.getGameListUpdate().begin(), itEnd = data.getGameListUpdate().end();
            it != itEnd; ++it)
        {
            GameManager::GameBrowserListId listId = (*it)->getListId();

            TRACE_LOG("[GameBrowser].processNotifyGameListUpdates received onNotifyGameListUpdate for list id '"
                << listId << "', containing '" << (*it)->getAddedGameFitScoreList().size() << "' added games, '"
                << (*it)->getUpdatedGameFitScoreList().size() << "' updated games and '"
                << (*it)->getRemovedGameList().size() << "' removed games.");

            GameBrowserListMap::iterator itGameList = mAllLists.find(listId);
            if (itGameList != mAllLists.end())
            {
                GameBrowserList &list = *itGameList->second;

                // iterate over all removed games and update listener
                GameManager::GameIdList::const_iterator itRemovedEnd = (*it)->getRemovedGameList().end();
                for (GameManager::GameIdList::const_iterator itRemoved = (*it)->getRemovedGameList().begin();
                    itRemoved != itRemovedEnd;
                    itRemoved++)
                {
                    list.onGameRemoved(*itRemoved);
                }

                // iterate over all added games and update listener
                Search::GameFitScoreInfoList::const_iterator itAddedEnd = (*it)->getAddedGameFitScoreList().end();
                for (Search::GameFitScoreInfoList::const_iterator itAdded = (*it)->getAddedGameFitScoreList().begin();
                    itAdded != itAddedEnd;
                    itAdded++)
                {
                    GameBrowserGameDataPtr& gamedata = gameNotifications[(*itAdded)->getGameId()][list.getListConfigName()];
                    if (gamedata != nullptr)
                    {
                        list.onGameAdded((*itAdded)->getGameId(), (*itAdded)->getFitScore(), gamedata);
                    }
                    else
                    {
                        //Should never happen - all updates should include all notifications to process the updates
                        WARN_LOG("[GameBrowser].processNotifyGameListUpdate: No notification passed down for game id " << (*itAdded)->getGameId());

                        // This game may have been erased after this list was created and before this notification was sent.
                        // We'll never receive GameBrowserGameData for this game, but we still include it in our received games count.
                        // (This prevents getGameListSnapshot requests from timing out while the game browser waits for updates
                        // that will never come.)
                        // TODO: find a solution that allows us to send complete game list updates to the client, 
                        //       that are accurate as of the time of the getGameListSnapshot call.
                        list.incrementReceivedGamesCount();
                    }
                }

                // iterate over all updated games and update listener
                Search::GameFitScoreInfoList::const_iterator itUpdatedEnd = (*it)->getUpdatedGameFitScoreList().end();
                for (Search::GameFitScoreInfoList::const_iterator itUpdated = (*it)->getUpdatedGameFitScoreList().begin();
                    itUpdated != itUpdatedEnd;
                    itUpdated++)
                {
                    GameBrowserGameDataPtr& gamedata = gameNotifications[(*itUpdated)->getGameId()][list.getListConfigName()];
                    if (gamedata != nullptr)
                    {
                        list.onGameUpdated((*itUpdated)->getGameId(), (*itUpdated)->getFitScore(), gamedata);
                    }
                    else
                    {
                        //Should never happen - all updates should include all notifications to process the updates
                        WARN_LOG("[GameBrowser].processNotifyGameListUpdate: No notification passed down for game id " << (*itUpdated)->getGameId());
                    }

                }

                // signal fibers waiting on this game list update
                list.signalGameListUpdate();
            }

            // notify object that the lists have been updated
            scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());
        }
    }

    void GameBrowser::cleanupDataForInstance(InstanceId searchInstanceId)
    {
        GameBrowserListMap::iterator itr = mAllLists.begin();
        while (itr != mAllLists.end())
        {
            GameBrowserListId id = itr->first;
            GameBrowserList& list = *itr->second;
            bool exist = false;

            InstanceIdList::const_iterator instanceItr = list.getListOfHostingSearchSlaves().begin();
            InstanceIdList::const_iterator instanceEnd = list.getListOfHostingSearchSlaves().end();
            for (; instanceItr != instanceEnd; ++instanceItr)
            {
                if (*instanceItr == searchInstanceId)
                {
                    exist = true;
                    break;
                }
            }
            if (exist)
            {
                TRACE_LOG("[GameBrowser].cleanupDataForInstance(" << searchInstanceId << ") destroying game list " << list.getListId());
                // remove this unavailable searchSlave from our list of hosting SearchSlaves to avoid sending any further request
                list.removeSearchInstance(searchInstanceId);
                BlazeRpcError err = destroyGameList(list.getListId());
                if (err != ERR_OK)
                {
                    WARN_LOG("[GameBrowser].cleanupDataForInstance(" << searchInstanceId << ") error destroy game list " << list.getListId());
                }
            }
            // iterators may be invalid after blocking call, use upper bound here to find the next element
            itr = mAllLists.upper_bound(id);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief fill out the gameBrowser's component status tdf.
    ***************************************************************************************************/
    void GameBrowser::getStatusInfo(ComponentStatus& status) const
    {
        mGBMetrics.getStatuses(status);

        Blaze::ComponentStatus::InfoMap& parentStatusMap = status.getInfo();
        char8_t buf[64];

        // Backwards compatibility metrics

        // Counters
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGBMetrics.mCreatedLists.getTotal());
        parentStatusMap["GBTotalGameListCreated"] = buf;

        // Gauges
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mActiveSubscriptionLists.get());
        parentStatusMap["GBGaugeActiveSubscriptions"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mLastIdleLength.get());
        parentStatusMap["GBGaugeLastIdleLength"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mActiveAllLists.get());
        parentStatusMap["GBGaugeActiveAllLists"] = buf;
    }


    Blaze::BlazeRpcError GameBrowser::createGameList(GetGameListRequest& request, ListType listType, GameBrowserList*& newList,
        MatchmakingCriteriaError &error, uint32_t msgNum, uint32_t& numberOfGamesToBeDownloaded, FitScore& maxPossibleFitScore,
        uint16_t maxPlayerCapacity, const char8_t* preferredPingSite, GameNetworkTopology netTopology, const UserSessionInfo* ownerUserInfo)
    {
        StringBuilder buf;

        if (request.getListCapacity() == 0)
        {
            error.setErrMessage((buf << "[GameBrowser::createGameList]: Invalid list capacity " << request.getListCapacity() << ". Failing list creation.").get());
            WARN_LOG(buf.get());

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(GAMEBROWSER_ERR_INVALID_CAPACITY), RiverPoster::gamelist_fetch_failed);
            return GAMEBROWSER_ERR_INVALID_CAPACITY;
        }

        GameBrowserListId gameBrowserListId;
        if (getNextUniqueSessionId(gameBrowserListId) != ERR_OK)
        {
            error.setErrMessage((buf << "[GameBrowser::createGameList]: Error fetching next session id.").get());
            WARN_LOG(buf.get());
            return ERR_SYSTEM;
        }

        const GameBrowserListConfig *listConfig = getListConfig(request.getListConfigName());
        if (listConfig == nullptr)
        {
            error.setErrMessage((buf << "[GameBrowser::createGameList]: Invalid list config '" << request.getListConfigName() << "'. Failing list creation.").get());
            WARN_LOG(buf.get());
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(GAMEBROWSER_ERR_INVALID_LIST_CONFIG_NAME), RiverPoster::gamelist_fetch_failed);
            return GAMEBROWSER_ERR_INVALID_LIST_CONFIG_NAME;
        }

        // Special handling to resolve BlazeObjectIds to BlazeIds, for CG on master
        BlazeRpcError errResult = MatchmakingScenarioManager::playersRulesUserSetsToBlazeIds(request.getListCriteria(), error);
        if (Blaze::ERR_OK != errResult)
        {
            ERR_LOG("[GameBrowser::createGameList]: Avoid/Prefer Players Rule - " << error.getErrMessage());
            return (errResult == GAMEMANAGER_ERR_INVALID_MATCHMAKING_CRITERIA) ? GAMEBROWSER_ERR_INVALID_CRITERIA : errResult;
        }

        if (listType == GAME_BROWSER_LIST_SUBSCRIPTION && listConfig->getSortType() == UNSORTED_LIST)
        {
            // we don't allow unsorted subscription lists
            error.setErrMessage((buf << "[GameBrowser::createGameList]: Client attempting to create an unsorted subscription list. Failing list creation.").get());
            WARN_LOG(buf.get());
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(GAMEBROWSER_ERR_INVALID_CRITERIA), RiverPoster::gamelist_fetch_failed);

            return GAMEBROWSER_ERR_INVALID_CRITERIA;
        }

        if (listConfig->getSortType() == SORTED_LIST)
        {
            request.setListCapacity(eastl::min_alt(request.getListCapacity(), mGameBrowserConfig->getMaxSortedListCapacityServerOverride()));
        }

        request.setListCapacity(eastl::min_alt(request.getListCapacity(), listConfig->getMaxListCapacityOverride()));

        // check if this user has maxed out the number of allowed lists per user
        UserSessionId sessionId = (ownerUserInfo == nullptr) ? UserSession::getCurrentUserSessionId() : ownerUserInfo->getSessionId();
        GameBrowserListUserSessionMap::const_iterator itLower = mGameBrowserListUserSessionMap.lower_bound(sessionId);
        GameBrowserListUserSessionMap::const_iterator itEnd = mGameBrowserListUserSessionMap.end();
        GameBrowserListUserSessionMap::const_iterator it = itLower;
        int listCountForUser = 0;
        int countLimitForUser = mGameBrowserConfig->getMaxGameListsPerUser();
        while (it != itEnd && it->first == sessionId && listCountForUser < countLimitForUser)
        {
            listCountForUser++;
            it++;
        }

        if (listCountForUser >= countLimitForUser)
        {
            error.setErrMessage((buf << "[GameBrowser::createGameList]: User [id] " << UserSession::getCurrentUserBlazeId()
                << " is already at the maximum number of allowed game lists of " << countLimitForUser << ". Failing list creation.").get());
            WARN_LOG(buf.get());
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(GAMEBROWSER_ERR_EXCEEDED_MAX_REQUESTS), RiverPoster::gamelist_fetch_failed);

            // TODO: needs better error code in next major release
            return GAMEBROWSER_ERR_EXCEEDED_MAX_REQUESTS;
        }

        newList = BLAZE_NEW GameBrowserList(*this, gameBrowserListId, listType, request.getListConfigName(), request.getListCapacity(),
            listConfig->getMaxUpdateInterval(), sessionId, msgNum);
        if (listType == GAME_BROWSER_LIST_SUBSCRIPTION)
        {
            mSubscriptionLists.insert(eastl::make_pair(gameBrowserListId, newList));
        }

        // hide from idle until the list is fully created
        newList->setHiddenFromIdle(true);

        mAllLists.insert(eastl::make_pair(gameBrowserListId, newList));

        // create remote list
        BlazeRpcError errCode = SearchShardingBroker::convertSearchComponentErrorToGameManagerError(
            createGameBrowserList(
                gameBrowserListId,
                *newList,
                request,
                listType == GAME_BROWSER_LIST_SNAPSHOT,
                mEvaluateGameProtocolVersionString,
                maxPossibleFitScore,
                numberOfGamesToBeDownloaded,
                error,
                maxPlayerCapacity, preferredPingSite, netTopology, ownerUserInfo));

        if (mAllLists.find(gameBrowserListId) == mAllLists.end())
        {
            // new list is gone after blocking call ... possibly due to a reconfigure
            TRACE_LOG("[GameBrowser::createGameList]: User [id] " << UserSession::getCurrentUserBlazeId()
                << " tried to create a new game list, but the new game list disappeared: " << ErrorHelp::getErrorName(errCode)
                << " (ERR_OK will be changed to ERR_SYSTEM)");

            // The removal code decremented this value before it was incremented, so we have to up it here so the metrics don't get out of sync:
            mGBMetrics.mActiveLists.increment(1, listType, request.getListConfigName());
            if (errCode == ERR_OK)
            {
                errCode = ERR_SYSTEM;
            }

            // assume that it's already been deleted & removed from any tracking lists/maps
            return errCode;
        }

        if (errCode != ERR_OK)
        {
            mAllLists.erase(gameBrowserListId);
            if (listType == GAME_BROWSER_LIST_SUBSCRIPTION)
                mSubscriptionLists.erase(gameBrowserListId);

            delete newList;
            newList = nullptr;

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(errCode), RiverPoster::gamelist_fetch_failed);

            return errCode;
        }

        // list is ready to start sending results
        newList->setHiddenFromIdle(false);

        //match users to lists to improve onUserSessionLogout efficiency
        mGameBrowserListUserSessionMap.insert(eastl::make_pair(newList->getOwnerSessionId(), gameBrowserListId));

        newList->setActualResultSetSize(numberOfGamesToBeDownloaded);
        numberOfGamesToBeDownloaded = eastl::min_alt(request.getListCapacity(), numberOfGamesToBeDownloaded); //trim to the max capacity

        // update metrics
        mIdleMetrics.mGamesMatchedAtCreateList += newList->getUpdatedGamesSize();
        mMetrics.mTotalGamesMatchedAtCreateList += mIdleMetrics.mGamesMatchedAtCreateList;

        mGBMetrics.mCreatedLists.increment(1, newList->getListType(), newList->getListConfigName().c_str());
        mGBMetrics.mActiveLists.increment(1, newList->getListType(), newList->getListConfigName().c_str());
        mGBMetrics.mInitialGamesSent.increment(numberOfGamesToBeDownloaded, newList->getListType(), newList->getListConfigName().c_str());

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Returns the size of game list container that should be enough to achieve given
            game list correctness requirements

            This function returns a list size based on a configurable load factor of 0.0 to 1.0.  This
            formula yields (requestSize / partitionCount) at 0.0, or the minimum request size to potentially
            meet the required number of games in the list, or 1.0 which guarantees the possibility
            of getting the requested game size (assuming there are enough matches in the first place).
    ***************************************************************************************************/
    uint32_t GameBrowser::getGameSessionContainerSize(uint32_t visibleListGameSize) const
    {
        uint32_t partitionCount = gSliverManager->getSliverNamespacePartitionCount(GameManagerMaster::COMPONENT_ID);
        if (partitionCount > 0)
        {
            double minGmCount = (double)visibleListGameSize / (double)partitionCount;
            visibleListGameSize = (uint32_t)(minGmCount + ((double)visibleListGameSize - minGmCount) * mGameBrowserConfig->getListContainerSizeFactor());
        }
        return visibleListGameSize;
    }

    BlazeRpcError GameBrowser::createGameBrowserList(
        GameManager::GameBrowserListId listId,
        GameBrowserList &list,
        const GameManager::GetGameListRequest &getGameListRequest,
        bool isSnapshot,
        bool evaluateVersionProtocol,
        GameManager::FitScore &maxPossibleFitScore,
        uint32_t &numberOfGamesToBeDownloaded,
        Blaze::GameManager::MatchmakingCriteriaError &criteriaError,
        uint16_t maxPlayerCapacity, const char8_t* preferredPingSite, GameNetworkTopology netTopology, const UserSessionInfo* ownerUserInfo)
    {
        maxPossibleFitScore = 0;
        numberOfGamesToBeDownloaded = 0;

        SlaveInstanceIdList resolvedSearchSlaveList;
        getFullCoverageSearchSet(resolvedSearchSlaveList);
        StringBuilder buf;

        if (resolvedSearchSlaveList.empty())
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList() No search slave instances available.").get());
            ERR_LOG(buf.get());
            return Blaze::ERR_SYSTEM;
        }

        EA_ASSERT(gUserSessionMaster != nullptr);
        UserSessionPtr ownerSession = gUserSessionManager->getSession(list.getOwnerSessionId());
        if (ownerSession == nullptr && ownerUserInfo == nullptr)
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList(): Session does not exist for session id " << list.getOwnerSessionId()).get());
            ERR_LOG(buf.get());
            return Blaze::ERR_SYSTEM;
        }

        Component* searchComponent = gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID);
        if (searchComponent == nullptr)
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList() Unable to get instance - this list will be bogus, list id " << listId << ".").get());
            ERR_LOG(buf.get());
            return Blaze::ERR_SYSTEM;
        }

        // for non-snapshot list we need list of hosting slaves so that we
        // can terminate the list once it's not needed anymore
        if (!isSnapshot)
            list.setListOfHostingSearchSlaves(resolvedSearchSlaveList);

        RpcCallOptions opts;
        opts.timeoutOverride = mGameManagerSlave.getConfig().getMatchmakerSettings().getSearchMultiRpcTimeout();

        Search::CreateGameListRequest request;
        request.setGameBrowserListId(listId);
        request.setIsSnapshot(isSnapshot);
        request.setEvaluateGameVersionString(evaluateVersionProtocol);
        request.setMaxPlayerCapacity(maxPlayerCapacity);
        request.setPreferredPingSite(preferredPingSite);
        request.setNetTopology(netTopology);

        BlazeRpcError result = ERR_OK;
        if (ownerUserInfo != nullptr)
            ownerUserInfo->copyInto(request.getOwnerUserInfo());
        else
        {
            result = ValidationUtils::setJoinUserInfoFromSession(request.getOwnerUserInfo(), *ownerSession, nullptr);
            if (result != ERR_OK)
            {
                criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList() Unable to set user info from session(" << ownerSession->getUserSessionId() << "), err=" << ErrorHelp::getErrorName(result)).get());
                ERR_LOG(buf.get());
                return result;
            }
        }

        request.setGetGameListRequest(const_cast<GetGameListRequest&>(getGameListRequest));

        // NOTE: This also modifies the original getGameListRequest
        const uint32_t perSlaveCapacity = request.getGetGameListRequest().getListCapacity();
        if (perSlaveCapacity == 0)
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList: Requested list(" << list.getListId() << ") capacity(0) is invalid, dropping request.").get());
            WARN_LOG(buf.get());
            return SEARCH_ERR_INVALID_CAPACITY;
        }
        const uint32_t adjustedCapacity = getGameSessionContainerSize(perSlaveCapacity);
        if (adjustedCapacity == 0)
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList: Adjusted list(" << list.getListId() << ") capacity(0) is invalid, dropping request.").get());
            WARN_LOG(buf.get());
            return SEARCH_ERR_INVALID_CAPACITY;
        }
        request.getGetGameListRequest().setListCapacity(adjustedCapacity);

        result = fixupPlayerJoinDataRoles(request.getGetGameListRequest().getPlayerJoinData(), true, mGameManagerSlave.getLogCategory());
        if (result != ERR_OK)
        {
            criteriaError.setErrMessage((buf << "[GameBrowser].createGameBrowserList: Failed to validate playerJoinData roles!").get());
            WARN_LOG(buf.get());
            return SEARCH_ERR_INVALID_CRITERIA;
        }

        // Copy off the request in case we need to re-send it later for resubscriptions
        request.copyInto(list.getCreateGameListRequest());

        Component::MultiRequestResponseHelper<Search::CreateGameListResponse, Blaze::GameManager::MatchmakingCriteriaError> responses(resolvedSearchSlaveList);
        result = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_CREATEGAMELIST, &request, responses, opts);

        // Reset the original getGameListRequest
        request.getGetGameListRequest().setListCapacity(perSlaveCapacity);

        bool success = false;
        bool timeout = false;
        for (Component::MultiRequestResponseList::const_iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
        {
            if ((*itr)->error == ERR_OK)
            {
                success = true;
                const Blaze::Search::CreateGameListResponse& instanceResponse = (const Blaze::Search::CreateGameListResponse&) *((*itr)->response);
                if (maxPossibleFitScore < instanceResponse.getMaxPossibleFitScore())
                    maxPossibleFitScore = instanceResponse.getMaxPossibleFitScore();
                numberOfGamesToBeDownloaded += instanceResponse.getNumberOfGamesToBeDownloaded();
            }
            else
            {
                // Take the error message from any of the slaves' responses:
                criteriaError.setErrMessage(((const Blaze::GameManager::MatchmakingCriteriaError&) *((*itr)->errorResponse)).getErrMessage());

                if ((*itr)->error == ERR_TIMEOUT)
                {
                    timeout = true;
                }
            }
        }

        // Only fail now if every SS failed.
        if (!success)
        {
            // The list may have been deleted during the blocking sendMultiRequest above, only need to terminate if it still exists
            if (mAllLists.find(listId) != mAllLists.end())
                terminateGameBrowserList(list, timeout);
            return result;
        }

        if (Logger::isEnabled(LOGGER_CATEGORY, Logging::TRACE, __FILE__, __LINE__) && !isSnapshot)
        {
            StringBuilder instanceIds;
            for (auto ssIdItr : resolvedSearchSlaveList)
            {
                instanceIds << ssIdItr << ",";
            }
            instanceIds.trim(1);
            TRACE_LOG("[GameBrowser].createGameBrowserList: list(" << listId << ") is subscribed to instances(" << instanceIds.get() << ")");
        }
        return ERR_OK;
    }

    BlazeRpcError GameBrowser::terminateGameBrowserList(GameManager::GameBrowserList& list, bool terminateRemoteList)
    {
        BlazeRpcError error = Blaze::ERR_OK;

        if (terminateRemoteList && !list.getListOfHostingSearchSlaves().empty())
        {
            Component* searchComponent = gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID);
            if (searchComponent == nullptr)
            {
                ERR_LOG("[GameBrowser].terminateGameBrowserList() Unable to get search instance for list id " << list.getListId() << ".");
                return Blaze::ERR_SYSTEM;
            }

            Search::TerminateGameListRequest request;
            request.setListId(list.getListId());

            Component::MultiRequestResponseHelper<void, void> responses(list.getListOfHostingSearchSlaves());
            error = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_TERMINATEGAMELIST, &request, responses);
        }

        return error;
    }

    BlazeRpcError GameBrowser::createUserSetGameList(GameBrowserList& list, const Search::CreateUserSetGameListRequest& req, uint32_t& numberOfGamesToBeDownloaded)
    {
        numberOfGamesToBeDownloaded = 0;

        SlaveInstanceIdList resolvedSearchSlaveList;
        getFullCoverageSearchSet(resolvedSearchSlaveList);

        if (resolvedSearchSlaveList.empty())
        {
            ERR_LOG("[GameBrowser].createUserSetGameList()"
                " No search slave instances available.");
            return Blaze::ERR_SYSTEM;
        }

        Component* searchComponent = gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID);
        if (searchComponent == nullptr)
        {
            ERR_LOG("[GameBrowser].createUserSetGameList()"
                " Unable to get search slave - this list will be bogus, list id " << req.getGameBrowserListId() << ".");
            return ERR_SYSTEM;
        }

        TRACE_LOG("[GameBrowser].createUserSetGameList()"
            << " Resolved " << resolvedSearchSlaveList.size() << " instances of Search component.");

        list.setListOfHostingSearchSlaves(resolvedSearchSlaveList);

        Component::MultiRequestResponseHelper<Blaze::Search::CreateGameListResponse, void> responses(resolvedSearchSlaveList);
        BlazeRpcError result = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_CREATEUSERSETGAMELIST, &req, responses);
        bool success = false;
        bool timeout = false;
        for (Component::MultiRequestResponseList::const_iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
        {
            if ((*itr)->error == ERR_OK)
            {
                success = true;
                const Blaze::Search::CreateGameListResponse& instanceResponse = (const Blaze::Search::CreateGameListResponse&) *((*itr)->response);
                numberOfGamesToBeDownloaded += instanceResponse.getNumberOfGamesToBeDownloaded();
            }
            else if ((*itr)->error == ERR_TIMEOUT)
            {
                timeout = true;
            }
        }

        if (!success)
        {
            if (mAllLists.find(req.getGameBrowserListId()) != mAllLists.end())
                terminateGameBrowserList(list, timeout);
            return result;
        }

        return ERR_OK;
    }

    void GameBrowser::updateUserSetGameList(EA::TDF::ObjectId compObjectId, BlazeId blazeId, bool add)
    {
        eastl::hash_set<InstanceId> sessionIds;
        Search::UserSetWatchListUpdate req;

        if (add)
        {
            req.getUsersAdded().push_back(blazeId);
        }
        else
        {
            req.getUsersRemoved().push_back(blazeId);
        }

        BlazeObjectIdToGameListMap::iterator itr = mObjectIdToGameListMap.find(compObjectId);
        if (itr != mObjectIdToGameListMap.end())
        {
            for (GameBrowserListSet::const_iterator sitr = itr->second.begin(), send = itr->second.end(); sitr != send; ++sitr)
            {
                req.getGameBrowserLists().push_back((*sitr)->getListId());
                sessionIds.insert((*sitr)->getListOfHostingSearchSlaves().begin(), (*sitr)->getListOfHostingSearchSlaves().end());
            }
        }

        if (!sessionIds.empty())
        {

            Component* searchComponent = gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID);
            if (searchComponent == nullptr)
            {
                ERR_LOG("[GameBrowser].updateUserSetGameList(): Unable to get search slave.");
                return;
            }

            Component::MultiRequestResponseHelper<void, void> responses(sessionIds);
            BlazeRpcError error = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_UPDATEUSERSETGAMELIST, &req, responses);
            if (error != Blaze::ERR_OK)
            {
                TRACE_LOG("[GameBrowser].updateUserSetWatchListFiber: Unable to update user set list.");
            }
        }
    }

} // namespace GameManager
} // namespace Blaze
