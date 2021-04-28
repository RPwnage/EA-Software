/*! ************************************************************************************************/
/*!
    \file   searchslaveimpl.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/


#include "framework/blaze.h"
#include "framework/storage/storagemanager.h"
#include "framework/controller/processcontroller.h"
#include "framework/replication/replicator.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/connection/connectionmanager.h"
#include "framework/usersessions/usersession.h"

#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/tdf/search_config_server.h"
#include "gamemanager/matchmaker/rules/gamenameruledefinition.h"
#include "gamemanager/searchcomponent/gamelist.h"
#include "gamemanager/searchcomponent/sortedgamelist.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/rete/productionlistener.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/rete/productionlistener.h"

namespace Blaze
{

namespace Search
{

    // static factory method
    SearchSlave* SearchSlave::createImpl()
    {
        return BLAZE_NEW_NAMED("SearchSlaveImpl") SearchSlaveImpl();
    }

    SearchSlaveImpl::SearchSlaveImpl()
        : SearchSlaveStub()
        , IdlerUtil("[SearchSlaveImpl]")
        , mGameListener(GameManager::GameManagerMaster::COMPONENT_ID, GameManager::GameManagerMaster::COMPONENT_MEMORY_GROUP, GameManager::GameManagerMaster::LOGGING_CATEGORY)
        , mGameSessionSlaveMap(BlazeStlAllocator("GameSessionSlaveMap", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mUserSessionGamesSlaveMap(BlazeStlAllocator("UserSessionGamesSlaveMap", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mGameIntrusiveNodeAllocator("GBAllocatorGameIntrusiveNode", sizeof(GameManager::GbListGameIntrusiveNode), SearchSlave::COMPONENT_MEMORY_GROUP)
        , mGameMatchAllocator("GameMatchAllocator", sizeof(GameMatch), SearchSlave::COMPONENT_MEMORY_GROUP)
        , mReteNetwork(getMetricsCollection(), SearchSlave::LOGGING_CATEGORY)
        , mMatchmakingSlave(*this)
        , mGameIdByPersistedIdMap(BlazeStlAllocator("GameIdByPersistedIdMap", COMPONENT_MEMORY_GROUP))
        , mUserSetSearchManager(*this)
        , mSubscriptionLists(BlazeStlAllocator("mSubscriptionLists", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mAllLists(BlazeStlAllocator("mAllLists", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mGameListIdsByOwnerSessionId(BlazeStlAllocator("mGameListIdsByOwnerSessionId", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mGameListIdsByInstanceId(BlazeStlAllocator("mGameListIdsByInstanceId", SearchSlave::COMPONENT_MEMORY_GROUP))
        , mLastListProcessed(0)
        , mIdleMetrics(getMetricsCollection())
        , mMetrics(getMetricsCollection())
        , mAllGameLists(getMetricsCollection(), "allGameLists", [this]() { return mAllLists.size(); })
        , mSubscriptionGameLists(getMetricsCollection(), "subscriptionGameLists", [this]() { return mSubscriptionLists.size(); })
        , mLastIdleTimeMs(getMetricsCollection(), "lastIdleTimeMs", [this]() { return mLastIdleLength.getMillis(); })
        , mLastIdleTimeToIdlePeriodRatioPercent(getMetricsCollection(), "lastIdleTimeToIdlePeriodRatioPercent", [this]() { return getDefaultIdlePeriod().getMicroSeconds() == 0 ? 0 : (uint64_t)((100 * mLastIdleLength.getMicroSeconds()) / getDefaultIdlePeriod().getMicroSeconds()); })
        , mDiagnosticsHelper(*this),
          mScenarioManager(nullptr)
    {
        // init storage field/record handlers
        mGameFieldHandler.fieldNamePrefix = GameManager::PUBLIC_GAME_FIELD;
        mGameFieldHandler.upsertFieldCb = UpsertStorageFieldCb(this, &SearchSlaveImpl::onUpsertGame);
        mGameFieldHandler.upsertFieldFiberContext = "SearchSlaveImpl::onUpsertGame";
        mPlayerFieldHandler.fieldNamePrefix = GameManager::PUBLIC_PLAYER_FIELD;
        mPlayerFieldHandler.upsertFieldCb = UpsertStorageFieldCb(this, &SearchSlaveImpl::onUpsertPlayer);
        mPlayerFieldHandler.upsertFieldFiberContext = "SearchSlaveImpl::onUpsertPlayer";
        mPlayerFieldHandler.eraseFieldCb = UpsertStorageFieldCb(this, &SearchSlaveImpl::onErasePlayer);
        mPlayerFieldHandler.eraseFieldFiberContext = "SearchSlaveImpl::onErasePlayer";
        mGameRecordHandler.eraseRecordCb = EraseStorageRecordCb(this, &SearchSlaveImpl::onEraseGame);
        mGameRecordHandler.eraseRecordFiberContext = "SearchSlaveImpl::onEraseGame";
    }

    SearchSlaveImpl::~SearchSlaveImpl()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Gets GameSessionSearchSlave for given GameId
    ***************************************************************************************************/
    GameSessionSearchSlave* SearchSlaveImpl::getGameSessionSlave(GameManager::GameId id) const
    {
        GameSessionSlaveMap::const_iterator it = mGameSessionSlaveMap.find(id);

        if (it == mGameSessionSlaveMap.end())
            return nullptr;

        return it->second.get();
    }

    /*! ************************************************************************************************/
    /*! \brief Gets UserSessionGameSet for given UserSessionId
    ***************************************************************************************************/
    const GameManager::UserSessionGameSet* SearchSlaveImpl::getUserSessionGameSetBySessionId(UserSessionId userSessionId) const
    {
        UserSessionGamesSlaveMap::const_iterator iter = mUserSessionGamesSlaveMap.find(userSessionId);
        if (iter == mUserSessionGamesSlaveMap.end())
        {
            return nullptr;
        }

        return &(iter->second);
    }

    /*! ************************************************************************************************/
    /*! \brief Gets UserSessionGameSet for given BlazeId. Uses primary user session id of the user, if its logged in
    ***************************************************************************************************/
    const GameManager::UserSessionGameSet* SearchSlaveImpl::getUserSessionGameSetByBlazeId(BlazeId blazeId) const
    {
        if (blazeId == INVALID_BLAZE_ID)
            return nullptr;

        UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId); 
        // Check to see if we found a session
        if (primarySessionId == INVALID_USER_SESSION_ID)
        {
            TRACE_LOG("[SearchSlaveImpl] Attempt to find game sessions by user with BlazeId (" << blazeId << ") who does not have a primary session.");
            return nullptr;
        }

        UserSessionGamesSlaveMap::const_iterator iter = mUserSessionGamesSlaveMap.find(primarySessionId);
        if (iter != mUserSessionGamesSlaveMap.end())
        {
            return &(iter->second);
        }

        return getDisconnectReservationsByBlazeId(blazeId);
    }

    /*! ************************************************************************************************/
    /*! \brief Gets disconnect reservations UserSessionGameSet for given user
    ***************************************************************************************************/
    const GameManager::UserSessionGameSet* SearchSlaveImpl::getDisconnectReservationsByBlazeId(BlazeId blazeId) const
    {
        BlazeIdGamesSlaveMap::const_iterator gsIter = mDisconnectedBlazeIdGamesSlaveMap.find(blazeId);
        if (gsIter != mDisconnectedBlazeIdGamesSlaveMap.end())
        { 
            return &(gsIter->second);
        }

        return nullptr;
    }


    void SearchSlaveImpl::onShutdown()
    {
        shutdownIdler();

        // destroying outstanding lists
        destroyAllGameLists();

        mMatchmakingSlave.onShutdown();
        mReteNetwork.onShutdown();

        // clean up outstanding WMEs
        GameSessionSlaveMap::const_iterator gameMapItr = mGameSessionSlaveMap.begin();
        GameSessionSlaveMap::const_iterator gameMapEnd = mGameSessionSlaveMap.end();
        for (; gameMapItr != gameMapEnd; ++gameMapItr)
        {
            GameSessionSearchSlave *currentGame = gameMapItr->second.get();
            if (currentGame != nullptr)
            {
                removeWorkingMemoryElements(*currentGame);
            }
        }

        // free any remaining game instances
        mGameSessionSlaveMap.clear();

        gUserSessionManager->removeSubscriber(*this);
        
        gController->removeRemoteInstanceListener(*this);
        gController->deregisterDrainStatusCheckHandler(Blaze::Search::SearchSlave::COMPONENT_INFO.name);
    }

    bool SearchSlaveImpl::onValidatePreconfig(SearchSlavePreconfig& config, const SearchSlavePreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        if (!validatePreconfig(config))
        {
            ERR_LOG("[SearchSlaveImpl].onValidatePreconfig: Configuration problem while validating preconfig for Game and Session Properties.");
            return false;
        }

        return true;
    }

    bool SearchSlaveImpl::validatePreconfig(const SearchSlavePreconfig& config) const
    {
        GameManager::GameManagerServerPreconfig preconfig;
        config.getRuleAttributeMap().copyInto(preconfig.getRuleAttributeMap());

        if (!mScenarioManager.validatePreconfig(preconfig))
        {
            ERR_LOG("[SearchSlaveImpl].validatePreconfig: Configuration problem while validating preconfig for ScenarioManager.");
            return false;
        }
        return true;
    }

    bool SearchSlaveImpl::onPreconfigure()
    {
        TRACE_LOG("[SearchSlaveImpl] Preconfiguring search slave component.");

        const SearchSlavePreconfig& preconfigTdf = getPreconfig();

        GameManager::GameManagerServerPreconfig preconfig;
        preconfigTdf.getRuleAttributeMap().copyInto(preconfig.getRuleAttributeMap());

        if (!mScenarioManager.preconfigure(preconfig))
        {
            ERR_LOG("[SearchSlaveImpl].onPreconfigure: Configuration problem while validating preconfig for ScenarioManager.");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief register ourself as a listener to notifications from the master once we resolve it
    ***************************************************************************************************/
    bool SearchSlaveImpl::onResolve()
    {
        // NOTE: deliberately register game field handler first, because we always want it to be invoked first when processing updates
        mGameListener.registerFieldHandler(mGameFieldHandler);
        mGameListener.registerFieldHandler(mPlayerFieldHandler);
        mGameListener.registerRecordHandler(mGameRecordHandler);

        gStorageManager->registerStorageListener(mGameListener);

        // subscribe to instance notifications to check for master assignment when new instance comes up
        gController->addRemoteInstanceListener(*this);

        // do master assignment for all existing active instances
        gController->syncRemoteInstanceListener(*this);

        // register as DrainStatusCheckHandler
        gController->registerDrainStatusCheckHandler(Blaze::Search::SearchSlave::COMPONENT_INFO.name, *this);

        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief Read in configuration which includes rule definitions from GameManager component.
     *!        Since we're accessing other component's configuration we cannot rely entirely on 
     *!        generated code.
    ***************************************************************************************************/
    bool SearchSlaveImpl::onConfigure()
    {
        const SearchConfig& configTdf = getConfig();

        TRACE_LOG("[SearchSlaveImpl] Configuring search slave component.");

        if (!mPropertyManager.onConfigure(configTdf.getProperties()))
        {
            ERR_LOG("[SearchSlaveImpl] Configuration problem while parsing PropertyManager's config file.");
            return false;
        }
        
        // configure matchmaking slave
        if (!mMatchmakingSlave.configure(configTdf.getMatchmakerSettings()))
        {
            // TODO correct error message after parsing, with string value from a config parser.
            ERR_LOG("[SearchSlaveImpl] Configuration problem while parsing " << (Blaze::GameManager::Matchmaker::MatchmakingConfig::MATCHMAKER_SETTINGS_KEY) << " config file.");
            return false;
        }

        // Use rule definitions to init rete substring tries
        configureReteSubstringTries();

        // configure rete network
        // TODO: MOVE OVER THIS CONFIG FROM GAMEMANAGER
        if(!mReteNetwork.configure(configTdf.getRete()))
        {
            ERR_LOG("[SearchSlaveImpl] Configuration problem while parsing Rete Network config file.");
            return false;
        }

       gUserSessionManager->addSubscriber(*this);
       return true;
    }

    bool SearchSlaveImpl::onValidateConfig(SearchConfig& config, const SearchConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
    {
        
        GameManager::GameManagerServerPreconfig preconfig;
        getPreconfig().getRuleAttributeMap().copyInto(preconfig.getRuleAttributeMap());

        if (!mScenarioManager.validateConfig(config.getScenariosConfig(), config.getMatchmakerSettings(), nullptr, preconfig, validationErrors))
        {
            return false;
        }

        mPropertyManager.validateConfig(config.getProperties(), validationErrors);
        mMatchmakingSlave.validateConfig(config.getMatchmakerSettings(), &referenceConfigPtr->getMatchmakerSettings(), validationErrors);
        return validationErrors.getErrorMessages().empty();
    }

    bool SearchSlaveImpl::onPrepareForReconfigure(const SearchConfig& newConfig)
    {
        // All of the matchmaking shutdown steps (stopping sessions, reseting the RETE trees, etc) currently happen in onReconfigure.
        // We currently don't need to anything to prepare for reconfigure.
        return true;
    }

    bool SearchSlaveImpl::onReconfigure()
    {
        const SearchConfig& configTdf = getConfig();

        TRACE_LOG("[SearchSlaveImpl] Reconfiguring search slave component.");

        // This removes any references to the old mMatchmakingSlave rule definitions (which will be destroyed next)
        destroyAllGameLists();

        // Remove all the old WMEs:
        for (GameSessionSlaveMap::const_iterator gameMapIter = mGameSessionSlaveMap.begin(), gameMapEnd = mGameSessionSlaveMap.end(); 
            gameMapIter != gameMapEnd; ++gameMapIter)
        {
            GameSessionSearchSlave *currentGame = gameMapIter->second.get();
            if (currentGame != nullptr)
            {
                // before updating matchmaking game info cache in removeWorkingMemoryElements() update diagnostic cache
                mDiagnosticsHelper.updateGameCountsCaches(*currentGame, false);

                removeWorkingMemoryElements(*currentGame);
            }
        }

        // configure matchmaking slave
        if (!mMatchmakingSlave.reconfigure(configTdf.getMatchmakerSettings()))
        {
            // TODO correct error message after parsing, with string value from a config parser.
            ERR_LOG("[SearchSlaveImpl] Reonfiguration problem while parsing " << (Blaze::GameManager::Matchmaker::MatchmakingConfig::MATCHMAKER_SETTINGS_KEY) << " config file.");
            return true; // RECONFIGURE SHOULD NEVER RETURN FALSE.  We have logged an error, but we don't want the blaze server to shutdown.
        }

        // Use rule definitions to (re)init rete substring tries
        configureReteSubstringTries();

        if(!mReteNetwork.reconfigure(configTdf.getRete()))
        {
            ERR_LOG("[SearchSlaveImpl] Reconfiguration problem while parsing Rete Network config file.");
            return true; // RECONFIGURE SHOULD NEVER RETURN FALSE.  We have logged an error, but we don't want the blaze server to shutdown.
        }

        // We have recreated our MM rule definitions.  All game's MatchmakingInfoCaches are invalid.       
        for (GameSessionSlaveMap::const_iterator gameMapIter = mGameSessionSlaveMap.begin(), gameMapEnd = mGameSessionSlaveMap.end(); 
            gameMapIter != gameMapEnd; ++gameMapIter)
        {
            GameSessionSearchSlave *currentGame = gameMapIter->second.get();
            if (currentGame != nullptr)
            {
                // Clear any caches dependent on rule definitions.
                currentGame->getMatchmakingGameInfoCache()->clearRuleCaches();

                // Reinsert the WME data, using the new definitions:
                insertWorkingMemoryElements(*currentGame); 

                // after updating matchmaking game info cache in insertWorkingMemoryElements() update diagnostic cache
                mDiagnosticsHelper.updateGameCountsCaches(*currentGame, true);
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief entry point for the framework to collect StatusInfo for the GM slave (healthcheck)
    ***************************************************************************************************/
    void SearchSlaveImpl::getStatusInfo(ComponentStatus& componentStatus) const 
    {   
        // populate Search slave info

        char8_t buf[64];
        ComponentStatus::InfoMap &componentInfoMap = componentStatus.getInfo();

        // counters
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalSearchSessionsStarted.getTotal());
        componentInfoMap["SSTotalSearchMatchmakingSessionsStarted"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGameMatchesSent.getTotal());
        componentInfoMap["SSTotalGameMatchesSent"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGameNotifsSent.getTotal());
        componentInfoMap["SSTotalGameNotifsSent"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalMatchUpdatesInGameListsSent.getTotal());
        componentInfoMap["SSTotalMatchUpdatesInGameListsSent"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalUniqueMatchUpdatesInGameListsSent.getTotal());
        componentInfoMap["SSTotalUniqueMatchUpdatesInGameListsSent"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mDirtyGameSetSize);
        componentInfoMap["SSGaugeLastIdleDirtyGameSetSize"] = buf;

        // gb counters
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mUniqueGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalUniqueGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalNewGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalNewGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mUniqueNewGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalUniqueNewGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalUpdatedGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalUpdatedGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mUniqueUpdatedGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalUniqueUpdatedGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalRemovedGamesSentToGameLists.getTotal());
        componentInfoMap["GBTotalRemovedGamesSentToGameLists"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGameListsDeniedUpdates.getTotal());
        componentInfoMap["GBTotalGameListsDeniedUpdatesInIdle"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGameListsUpdated.getTotal());
        componentInfoMap["GBTotalGameListsUpdatedInIdle"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalGameListsNotUpdated.getTotal());
        componentInfoMap["GBTotalGameListsNotUpdatedInIdle"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mTotalIdles.getTotal());
        componentInfoMap["SSTotalIdles"] = buf; 

        // gauges
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesTrackedOnThisSlaveCount.get());
        componentInfoMap["SSGaugeGamesTrackedOnThisSlave"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mAllGameLists.get());
        componentInfoMap["SSGaugeGameLists"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mSubscriptionGameLists.get());
        componentInfoMap["SSGaugeSubscriptionGameLists"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mLastIdleTimeMs.get());
        componentInfoMap["SSGaugeLastIdleTimeMs"] = buf;

        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mLastIdleTimeToIdlePeriodRatioPercent.get());
        componentInfoMap["SSGaugeLastIdleTimeToIdlePeriodRatioPercent"] = buf;

        mMatchmakingSlave.getStatusInfo(componentStatus);
   }

    /*! ************************************************************************************************/
    /*! \brief entry point for the framework to dump debug info
    ***************************************************************************************************/
    DumpDebugInfoError::Error SearchSlaveImpl::processDumpDebugInfo(const Message* message) 
    {   
        mMatchmakingSlave.dumpDebugInfo();
        return DumpDebugInfoError::ERR_OK;
    }

    GetGamesError::Error SearchSlaveImpl::processGetGames(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse& response, const Message* message)
    {
        if (request.getResponseType() == GET_GAMES_RESPONSE_TYPE_GAMEBROWSER)
        {
            if (getListConfig(request.getListConfigName()) == nullptr)
            {
                return GetGamesError::SEARCH_ERR_INVALID_LIST_CONFIG_NAME;
            }
        }

        const GameManager::GameIdList &gameIdList = request.getGameIds();
        const GameManager::PersistedGameIdList& gamePersistedIdList = request.getPersistedGameIdList();

        response.setResponseType(request.getResponseType());

        bool needOnlyOne = request.getFetchOnlyOne();
        bool found = false;

        if (!gameIdList.empty())
        {
            response.getFullGameData().reserve(gameIdList.size());

            for(GameManager::GameIdList::const_iterator gameIdIter = gameIdList.begin(), gameIdEnd = gameIdList.end(); 
                (!needOnlyOne || !found) && gameIdIter != gameIdEnd; ++gameIdIter)
            {
                GameSessionSearchSlave *gameSessionSlave = getGameSessionSlave(*gameIdIter);
                if(gameSessionSlave == nullptr) 
                {
                    // We don't return an error if a gameId was invalid; the client will know that a gameId wasn't returned.
                    continue;
                }

                storeGameInGetGameResponse(request, response, request.getListConfigName(), *gameSessionSlave);       
                found = true;
            }
        }

        if (!gamePersistedIdList.empty())
        {
            response.getFullGameData().reserve(gamePersistedIdList.size());
            for(GameManager::PersistedGameIdList::const_iterator gamepIdIter = gamePersistedIdList.begin(), gamepIdEnd = gamePersistedIdList.end();
                (!needOnlyOne || !found) && gamepIdIter != gamepIdEnd; ++gamepIdIter)
            {
                const GameManager::PersistedGameId& persistedGameId = *gamepIdIter;

                // if the persisted id is empty, check next one
                if (persistedGameId.c_str()[0] == '\0')
                    continue;

                GameSessionSearchSlave *gameSessionSlave = getGameSessionSlaveByPersistedId(persistedGameId);
                if(gameSessionSlave == nullptr) 
                {
                    // We don't return an error if a persisted game persisted id was invalid; the client will know that a persisted id wasn't returned.
                    continue;
                }

                storeGameInGetGameResponse(request, response, request.getListConfigName(), *gameSessionSlave);
                found = true;
            }
        }

        bool mismatchedProtocolVer = false;

        if (!request.getBlazeIds().empty())
        {
            GameTypeSet gameTypeSet;

            Blaze::GameManager::GameTypeList::const_iterator itr, end;
            for (itr = request.getGameTypeList().begin(), end = request.getGameTypeList().end(); itr != end; ++itr)
            {
                gameTypeSet.insert(*itr);
            }

            const BlazeIdList& blazeIdList = request.getBlazeIds();
            for (BlazeIdList::const_iterator uitr = blazeIdList.begin(), uend = blazeIdList.end(); (!needOnlyOne || !found) && uitr != uend; ++uitr)
            {
                // get games for all the user's user sessions
                UserSessionIdList userSessionIds;
                gUserSessionManager->getUserSessionIdsByBlazeId(*uitr, userSessionIds);

                for (UserSessionIdList::const_iterator usItr = userSessionIds.begin(), usEnd = userSessionIds.end(); (!needOnlyOne || !found) && usItr != usEnd; ++usItr)
                {
                    const GameManager::UserSessionGameSet* userGameSet = getUserSessionGameSetBySessionId(*usItr);
                    storeGamesInGetGameResponse(request, response, gameTypeSet, userGameSet, needOnlyOne, found, mismatchedProtocolVer);
                }

                // get games user has a disconnect reservation in
                if (!needOnlyOne || !found)
                {
                    const GameManager::UserSessionGameSet* userGameSet = getDisconnectReservationsByBlazeId(*uitr);
                    storeGamesInGetGameResponse(request, response, gameTypeSet, userGameSet, needOnlyOne, found, mismatchedProtocolVer);
                }
            }
        }

        if (request.getErrorIfGameProtocolVersionStringsMismatch() && mismatchedProtocolVer && (found == false))
        {
            // if we don't have a match only due to mismatched protocol version, return error as needed
            return GetGamesError::SEARCH_ERR_GAME_PROTOCOL_VERSION_MISMATCH;
        }
        return GetGamesError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief store games from the UserSessionGameSet to the response as needed applying the list config
        \param[in,out] found Whether a game was found
    ***************************************************************************************************/
    void SearchSlaveImpl::storeGamesInGetGameResponse(const Blaze::Search::GetGamesRequest& request,
        Blaze::Search::GetGamesResponse &response, const GameTypeSet& gameTypeSet,
        const GameManager::UserSessionGameSet* userGameSet, bool needOnlyOne, bool& found, bool& mismatchedProtocolVer)
    {
        if (userGameSet != nullptr)
        {
            for (GameManager::UserSessionGameSet::const_iterator it = userGameSet->begin(), itEnd = userGameSet->end(); (!needOnlyOne || !found) && it != itEnd; ++it)
            {
                GameSessionSearchSlave* userGame = getGameSessionSlave(*it);
                if (userGame != nullptr)
                {
                    //test protocol version
                    mismatchedProtocolVer = !GameManager::isGameProtocolVersionStringCompatible(getConfig().getGameSession().getEvaluateGameProtocolVersionString(), 
                        request.getGameProtocolVersionString(), userGame->getGameProtocolVersionString());
                    if (!mismatchedProtocolVer && (gameTypeSet.empty() || gameTypeSet.find(userGame->getGameType()) != gameTypeSet.end()))
                    {
                        storeGameInGetGameResponse(request, response, request.getListConfigName(), *userGame);
                        found = true;
                    }
                }
            }
        }   
    }

    /*! ************************************************************************************************/
    /*! \brief store game to the response as needed applying the list config
    ***************************************************************************************************/
    void SearchSlaveImpl::storeGameInGetGameResponse(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse &response, const eastl::string& listConfigName, GameSessionSearchSlave& game)
    {
        // check whether to include game based on list config downloadGameTypes. By spec if downloadGameTypes is empty we ignore this filter.
        const GameManager::GameBrowserListConfig* listConfig = getListConfig(request.getListConfigName());
        const GameManager::GameTypeList& gameTypes = listConfig->getDownloadGameTypes();
        if ((listConfig != nullptr) && !gameTypes.empty() && (eastl::find(gameTypes.begin(), gameTypes.end(), game.getGameType()) == gameTypes.end()))
        {
            SPAM_LOG("[SearchSlaveImpl].storeGameInGetGameResponse: not adding game(" << game.getGameId() << ") to response as its game type(" <<
                GameTypeToString(game.getGameType()) << ") is not in the (non-empty) download game types list for list config " << request.getListConfigName() << ".");
            return;
        }

        switch(response.getResponseType())
        {
            case GET_GAMES_RESPONSE_TYPE_FULL:
                {
                    GameManager::ListGameData *gameData = response.getFullGameData().pull_back();                       
                    
                    // set game data
                    gameData->setGame(game.getGameData());

                    // hide the secret
                    gameData->getGame().getPersistedGameIdSecret().setData(nullptr, 0);
                    
                    // insert the mode (this is for tools access)
                    gameData->setGameMode(game.getGameMode());

                    // set player roster data
                      const GameManager::PlayerRoster::PlayerInfoList& activePlayerList = game.getPlayerRoster()->getPlayers(GameManager::PlayerRoster::ROSTER_PLAYERS);
                    for ( auto rosterMember : activePlayerList)
                    {
                        GameManager::ReplicatedGamePlayer* repPlayer = gameData->getGameRoster().pull_back(); 
                        rosterMember->fillOutReplicatedGamePlayer(*repPlayer);
                    }

                    // set queue roster data
                    const GameManager::PlayerRoster::PlayerInfoList& queuedPlayerList = game.getPlayerRoster()->getPlayers(GameManager::PlayerRoster::QUEUED_PLAYERS);
                    for (auto rosterMember : queuedPlayerList)
                    {
                        GameManager::ReplicatedGamePlayer* repPlayer = gameData->getQueueRoster().pull_back();
                        rosterMember->fillOutReplicatedGamePlayer(*repPlayer);
                    }

                }
            break;
            case GET_GAMES_RESPONSE_TYPE_GAMEBROWSER:
                {
                    GameManager::GameBrowserGameData *gameData = response.getGameBrowserData().pull_back();
                    initGameBrowserGameData(*gameData, listConfigName, game);
                }                
            break;
            case GET_GAMES_RESPONSE_TYPE_ID:
                response.getGameIdList().push_back(game.getGameId());
            break;
        }
    }

    void SearchSlaveImpl::processDisconnectReservations(UserSessionId dyingUserSessionId, BlazeId dyingBlazeId)
    {
        UserSessionGamesSlaveMap::iterator iter = mUserSessionGamesSlaveMap.find(dyingUserSessionId);
        if (iter != mUserSessionGamesSlaveMap.end())
        {
            // Loop through known games the user is in, and remove from the session.
            GameManager::UserSessionGameSet& gameSet = iter->second;
            GameManager::UserSessionGameSet::const_iterator gameIter = gameSet.begin();
            GameManager::UserSessionGameSet::const_iterator gameEnd = gameSet.end();
            while (gameIter != gameEnd)
            {
                Search::GameSessionSearchSlave *gameSessionSlave = getGameSessionSlave(*gameIter);
                // increment first to avoid iter invalidation on remove.
                ++gameIter;
                if (gameSessionSlave == nullptr)
                {
                    continue;
                }

                if (gameSessionSlave->getGameSettings().getDisconnectReservation())
                {
                    // We only insert the game into the disconnected set if the player still exists,
                    // and will therefore be able to clean up the reservation on removal from the roster.
                    const GameManager::PlayerInfo* player = gameSessionSlave->getPlayerRoster()->getPlayer(dyingBlazeId);
                    if (player != nullptr)
                    {
                        // Spectator/queued players who join a match probably don't need reservations created when they leave the match.
                        if (!getConfig().getGameSession().getEnableSpectatorDisconnectReservations() && player->isSpectator())
                        {
                            TRACE_LOG("[SearchSlaveImpl].processDisconnectReservations: Not making disconnect reservation for spectator player (" << player->getPlayerId() << ").");
                        }
                        else if (!getConfig().getGameSession().getEnableQueuedDisconnectReservations() && player->isInQueue())
                        {
                            TRACE_LOG("[SearchSlaveImpl].processDisconnectReservations: Not making disconnect reservation for in-queue player (" << player->getPlayerId() << ").");
                        }
                        else
                        {
                            mDisconnectedBlazeIdGamesSlaveMap[dyingBlazeId].insert(gameSessionSlave->getGameId());
                        }
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Sets up rete's substring tries for appropriate enabled rules.
    ***************************************************************************************************/
    void SearchSlaveImpl::configureReteSubstringTries()
    {
        const GameManager::Matchmaker::GameNameRuleDefinition* ruleDefn = mMatchmakingSlave.getRuleDefinitionCollection().getRuleDefinition<GameManager::Matchmaker::GameNameRuleDefinition>();
        
        if ((ruleDefn == nullptr) || ruleDefn->isDisabled())
        {
            TRACE_LOG("[SearchSlaveImpl].configureReteSubstringTries no substring search rules configured to init for.");
            return;
        }

        if (ruleDefn->getNormalizationTable() == nullptr)
        {
            ERR_LOG("[SearchSlaveImpl].configureReteSubstringTries no normalization table was configured for substring trie.");
            return;
        }

        mReteNetwork.getReteSubstringTrie().configure(ruleDefn->getSearchStringMinLength(),
            ruleDefn->getSearchStringMaxLength(), *ruleDefn->getNormalizationTable());
    }

    bool SearchSlaveImpl::isGameForRete(const GameSessionSearchSlave& gameSessionSlave) const
    {
        if (getConfig().getGameSession().getHideClosedGamesFromRETE()
            && !gameSessionSlave.getGameSettings().getOpenToBrowsing()
            && !gameSessionSlave.getGameSettings().getOpenToMatchmaking())
        {
            return false;
        }
        else if ((gameSessionSlave.getGameType() == GameManager::GAME_TYPE_GROUP) && getConfig().getOmitGameGroupsFromRete())
        {
            return false;
        }
        else
        {
            for (const auto& templateName : getConfig().getGameTemplatesOmittedFromRete())
            {
                if (blaze_stricmp(templateName, gameSessionSlave.getCreateGameTemplate()) == 0)
                {
                    return false; // this game template is omitted from RETE.
                }
            }
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Insert WMEs into rete network for given game session
    ***************************************************************************************************/
    void SearchSlaveImpl::insertWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave)
    {
        if(!isGameForRete(gameSessionSlave))
        {
            //we've hidden games closed to browsing & mm from RETE, early out
            return;
        }

        gameSessionSlave.setInRete(true);

        mReteNetwork.getWMEManager().registerMatchAnyWME(gameSessionSlave.getGameId());
        const GameManager::Matchmaker::ReteRuleDefinitionList& ruleDefinitionList = mMatchmakingSlave.getRuleDefinitionCollection().getReteRuleDefinitionList();
        GameManager::Matchmaker::ReteRuleDefinitionList::const_iterator itr = ruleDefinitionList.begin();
        GameManager::Matchmaker::ReteRuleDefinitionList::const_iterator end = ruleDefinitionList.end();

        if(gameSessionSlave.getMatchmakingGameInfoCache()->isDirty())
        {
            gameSessionSlave.updateCachedGameValues(mMatchmakingSlave.getRuleDefinitionCollection());
            gameSessionSlave.getMatchmakingGameInfoCache()->clearDirtyFlags();
        }

        // Always update (though isDirty should always be true at insertion time)
        gameSessionSlave.updateGameProperties();

        for (; itr != end; ++itr)
        {
            // TODO: Ugly cast required until insert working memory elements is const.
            GameManager::Matchmaker::GameReteRuleDefinition* defn = *itr;
            defn->insertWorkingMemoryElements(mReteNetwork.getWMEManager(), gameSessionSlave);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Remove WMEs from rete network for given game session
    ***************************************************************************************************/
    void SearchSlaveImpl::removeWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave)
    {
        if(!gameSessionSlave.isInRete())
        {
            //we've hidden games closed to browsing & mm from RETE, early out
            return;
        }

        mReteNetwork.getWMEManager().unregisterMatchAnyWME(gameSessionSlave.getGameId());
        mReteNetwork.getWMEManager().removeAllWorkingMemoryElements(gameSessionSlave.getGameId());
        mReteNetwork.getWMEManager().unregisterWMEId(gameSessionSlave.getGameId());
        
        gameSessionSlave.setInRete(false);
    }

    /*! ************************************************************************************************/
    /*! \brief Update WMEs from rete network for given game session
    ***************************************************************************************************/
    void SearchSlaveImpl::updateWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave)
    {
        if(!gameSessionSlave.isInRete())
        {
            //we've hidden games closed to browsing & mm from RETE, early out
            return;
        }

        const GameManager::Matchmaker::ReteRuleDefinitionList& ruleDefinitionList = mMatchmakingSlave.getRuleDefinitionCollection().getReteRuleDefinitionList();
        GameManager::Matchmaker::ReteRuleDefinitionList::const_iterator itr = ruleDefinitionList.begin();
        GameManager::Matchmaker::ReteRuleDefinitionList::const_iterator end = ruleDefinitionList.end();

        if(gameSessionSlave.getMatchmakingGameInfoCache()->isDirty())
        {
            gameSessionSlave.updateCachedGameValues(mMatchmakingSlave.getRuleDefinitionCollection());
            gameSessionSlave.getMatchmakingGameInfoCache()->clearDirtyFlags();
        }

        // Always update the Game Properties after the MM cache is updated (needed for TeamUED checks) 
        gameSessionSlave.updateGameProperties();

        for (; itr != end; ++itr)
        {
            const GameManager::Matchmaker::GameReteRuleDefinition* defn = *itr;
            defn->upsertWorkingMemoryElements(mReteNetwork.getWMEManager(), gameSessionSlave);
        }

    }

    /*! ************************************************************************************************/
    /*! \brief Evaluates rules that added themselves as "any" to the rete tree. This means they did
        not actually add a node, but we still need to consider the fit score they can provide.  Also
        checks for rules that did not evaluate against a dedicated server.
    
        \param[in] gameSession - the game that we have found to be used to calculate the additional fitscore against.
        \param[in] pl - the production listener that initiated the search.
        \return the additional fit score to be added due to any rules.
    ***************************************************************************************************/
    GameManager::FitScore SearchSlaveImpl::evaluateAdditionalFitScoreForGame(GameSessionSearchSlave &gameSession, 
        const GameManager::Rete::ProductionInfo& info, GameManager::DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const
    {
        GameManager::FitScore additionalFitScore = 0;

        if (!info.mListener->isLegacyProductionlistener())
        {
            ERR_LOG("[SearchSlaveImpl].evaluateAdditionalFitScoreForGame for Game Id(" << gameSession.getGameId() 
                << ")- prodiction info node listener is not a legacy listener. This should not happen outside the GamePacker Production Listeners.");
            return additionalFitScore;
        }

        GameManager::Rete::LegacyProductionListener* listener = static_cast<GameManager::Rete::LegacyProductionListener*>(info.mListener);

        GameManager::Rete::MatchAnyRuleList::const_iterator iter = listener->getMatchAnyRuleList().begin();
        GameManager::Rete::MatchAnyRuleList::const_iterator end = listener->getMatchAnyRuleList().end();

        // Additional dedicated server fit score from rules that do not evaluate dedicated servers.
        if (gameSession.isResetable())
        {
            additionalFitScore += listener->getUevaluatedDedicatedServerFitScore();
        }

        for (; iter != end; ++iter)
        {
            const GameManager::Rete::ReteRule* rule = *iter;
            if (!rule->isDisabled())
            {
                // skip rules should only calculate *after* rete (avoid double counting thier score)
                if (rule->calculateFitScoreAfterRete())
                {
                    continue;
                }

                GameManager::Matchmaker::ReadableRuleConditions debugRuleConditions;
                if (useDebug)
                {
                    debugRuleConditions.setIsDebugSession();
                }

                const GameManager::FitScore ruleAnyFitScore = rule->evaluateGameAgainstAny(gameSession, info.conditionBlockId, debugRuleConditions);
                additionalFitScore += ruleAnyFitScore;
                TRACE_LOG("[MatchmakingEvaluation] Game '" << gameSession.getGameId() << "' Type 'MatchAny' Rule '" << rule->getRuleName()
                    << "' fitScore '" << ruleAnyFitScore << "'.");

                if (useDebug)
                {
                    GameManager::DebugRuleResult debugResult;
                    // shouldn't ever be duplicates here...
                    debugResult.setFitScore(ruleAnyFitScore);
                    debugResult.setMaxFitScore(rule->getMaxFitScore());
                    debugResult.setResult(GameManager::MATCH); // match any implies we always match
                    debugRuleConditions.getRuleConditions().copyInto(debugResult.getConditions());

                    eastl::pair<GameManager::DebugRuleResultMap::iterator, bool> result = debugResultMap.insert(eastl::make_pair(rule->getRuleName(), (GameManager::DebugRuleResult*)nullptr));
                    if (result.second)
                        result.first->second = debugResult.clone();
                }
            }
        }

        return additionalFitScore;
    }

    /*! ************************************************************************************************/
    /*! \brief a user has either logged out of blaze, or they've been disconnected.
    ***************************************************************************************************/
    void SearchSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
    {
        TRACE_LOG("SearchSlaveImpl::onUserSessionExtinction: blazeId=" << userSession.getBlazeId());

        // This seems really odd. Can't we just track games by blazeid?
        processDisconnectReservations(userSession.getUserSessionId(), userSession.getBlazeId());
        mMatchmakingSlave.onUserSessionExtinction(userSession.getUserSessionId());

        // destroy owned GB lists for the user
        // Cache off gb list id's into a seperate vector since destoryGameList erases from mGameListIdsByOwnerSessionId
        eastl::pair<GameListIdsByOwnerSessionId::iterator, GameListIdsByOwnerSessionId::iterator>
            range = mGameListIdsByOwnerSessionId.equal_range(userSession.getUserSessionId());

        typedef eastl::vector<GameManager::GameBrowserListId> GameBrowserListIdList;
        GameBrowserListIdList gameListIdList;
        for (GameListIdsByOwnerSessionId::iterator it = range.first; it != range.second; ++it)
            gameListIdList.push_back(it->second);

        GameBrowserListIdList::const_iterator iter = gameListIdList.begin();
        GameBrowserListIdList::const_iterator iterEnd = gameListIdList.end();

        for( ; iter != iterEnd; ++iter )
        {
            destroyGameList(*iter);
        }
    }



    /*! ************************************************************************************************/
    /*! \brief Adds an entry for quick lookup, associating the game to the user
    ***************************************************************************************************/
    void SearchSlaveImpl::addGameForUser(UserSessionId userSessionId, Search::GameSessionSearchSlave &game)
    {
        if (!UserSession::isValidUserSessionId(userSessionId))
        {
            // if we don't have a valid user session ID, we shouldn't do an insert.
            return;
        }

        mUserSessionGamesSlaveMap[userSessionId].insert(game.getGameId());
    }

    /*! ************************************************************************************************/
    /*! \brief removes an entry for quick lookup, disassociating the game from the user
    ***************************************************************************************************/
    void SearchSlaveImpl::removeGameForUser(UserSessionId userSessionId, BlazeId blazeId, Search::GameSessionSearchSlave &game)
    {
        if (!UserSession::isValidUserSessionId(userSessionId))
        {
            // if we don't have a valid user session ID, we didn't do an insert (see addGameForUser).
            return;
        }

        UserSessionGamesSlaveMap::iterator iter = mUserSessionGamesSlaveMap.find(userSessionId);
        if (iter != mUserSessionGamesSlaveMap.end())
        {
            // Remove existing game from the GameSet
            GameManager::UserSessionGameSet& gameSet = iter->second;

            gameSet.erase(game.getGameId());

            if (gameSet.empty())
            {
                // GameSet is empty, remove the usersession entry from the map
                mUserSessionGamesSlaveMap.erase(iter);
                TRACE_LOG("[SearchSlaveImpl] Removed User Session Id(" << userSessionId << ") from game session map, not in any more games.");
            }

            BlazeIdGamesSlaveMap::iterator gsIter = mDisconnectedBlazeIdGamesSlaveMap.find(blazeId);
            if (gsIter != mDisconnectedBlazeIdGamesSlaveMap.end())
            {
                // Remove existing game from the GameSet
                GameManager::UserSessionGameSet& gameSetTemp = gsIter->second;
                gameSetTemp.erase(game.getGameId());

                if (gameSetTemp.empty())
                {
                    // GameSet is empty, remove the blaze id entry from the map
                    mDisconnectedBlazeIdGamesSlaveMap.erase(gsIter);
                    TRACE_LOG("[SearchSlaveImpl] Removed Blaze Id(" << blazeId << ") from disconnected game session map, not in any more games.");
                }
            }
        }
        else 
        {
            BlazeIdGamesSlaveMap::iterator gsIter = mDisconnectedBlazeIdGamesSlaveMap.find(blazeId);
            EA_ASSERT_MSG(gsIter == mDisconnectedBlazeIdGamesSlaveMap.end(), "BlazeId->GameIdSet should not exist without UserSessionId->GameIdSet!");
        }
    }



    void SearchSlaveImpl::addList(GameListBase& list)
    {
        if(list.getListType() == GameManager::GAME_BROWSER_LIST_SUBSCRIPTION)
        {
            mSubscriptionLists[list.getListId()] = &list;
        }

        mAllLists[list.getListId()] = &list;

        //match users to lists to improve onUserSessionLogout efficiency
        if (list.getOwnerUserSessionId() != UserSession::INVALID_SESSION_ID)
            mGameListIdsByOwnerSessionId.insert(eastl::make_pair(list.getOwnerUserSessionId(), list.getListId()));

        mGameListIdsByInstanceId[GetInstanceIdFromInstanceKey64(list.getListId())].insert(list.getListId());

        ++mMetrics.mTotalGameListCreated;
    }

    GameListBase* SearchSlaveImpl::getList(GameManager::GameBrowserListId listId)
    {
        GameListByGameBrowserListId::iterator itr = mAllLists.find(listId);
        return (itr != mAllLists.end()) ? itr->second : nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief destroys game list, returns false if provided listId is invalid
    ***************************************************************************************************/
    bool SearchSlaveImpl::destroyGameList(GameManager::GameBrowserListId listId)
    {
        GameListByGameBrowserListId::iterator it = mAllLists.find(listId);
        if (it == mAllLists.end())
            return false;

        if (it->second->getListType() == GameManager::GAME_BROWSER_LIST_SUBSCRIPTION)
            mSubscriptionLists.erase(listId);

        //eliminate the session id to list mapping
        UserSessionId userSessionId = it->second->getOwnerUserSessionId();
        if (userSessionId != UserSession::INVALID_SESSION_ID)
        {
            eastl::pair<GameListIdsByOwnerSessionId::iterator, GameListIdsByOwnerSessionId::iterator>
                range = mGameListIdsByOwnerSessionId.equal_range(userSessionId);
            for (GameListIdsByOwnerSessionId::iterator iter = range.first; iter != range.second; ++iter)
            {
                if (iter->second == listId)
                {
                    mGameListIdsByOwnerSessionId.erase(iter);
                    break;
                }
            }
        }

        mGameListIdsByInstanceId[GetInstanceIdFromInstanceKey64(it->second->getListId())].erase(it->second->getListId());

        it->second->cleanup();
        delete it->second;

        mAllLists.erase(it);

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief destroys all game lists
    ***************************************************************************************************/
    void SearchSlaveImpl::destroyAllGameLists()
    {
        shutdownIdler();

        // destroying outstanding lists
        GameListByGameBrowserListId::iterator listIter = mAllLists.begin();
        GameListByGameBrowserListId::iterator listEnd = mAllLists.end();
        for(;listIter != listEnd; listIter++)
        {
            GameListBase *list = listIter->second;
            list->cleanup();          
            delete list;
        }

        mAllLists.clear();
        mGameListIdsByInstanceId.clear();
        mGameListIdsByOwnerSessionId.clear();
        mSubscriptionLists.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief sends notification about find game matchmaking sessin to target slave (local slave if nullptr)
    ***************************************************************************************************/
    void SearchSlaveImpl::sendFindGameFinalizationUpdateNotification(SlaveSession *session, const Blaze::Search::NotifyFindGameFinalizationUpdate& data)
    {
        mMetrics.mTotalGameNotifsSent.increment();
        mMetrics.mTotalGameMatchesSent.increment((uint64_t)data.getFoundGames().getMatchedGameList().size());

        if (session != nullptr)
        {
            sendNotifyFindGameFinalizationUpdateToSlaveSession(session, &data);
        }
        else if (hasNotificationListener()) 
        { 
            mNotificationDispatcher.dispatch<const NotifyFindGameFinalizationUpdate&, UserSession*>(&SearchSlaveListener::onNotifyFindGameFinalizationUpdate, data, nullptr);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief sends notification about game list to target slave (local slave if nullptr)
    ***************************************************************************************************/
    void SearchSlaveImpl::sendGameListUpdateNotification(SlaveSession *session, const Blaze::Search::NotifyGameListUpdate& data)
    {
        if (session != nullptr)
        {
            sendNotifyGameListUpdateToSlaveSession(session, &data);
        }
        else if (hasNotificationListener()) 
        { 
            mNotificationDispatcher.dispatch<const NotifyGameListUpdate&, UserSession*>(&SearchSlaveListener::onNotifyGameListUpdate, data, nullptr);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief gets game browser list configuration
    ***************************************************************************************************/
    const GameManager::GameBrowserListConfig *SearchSlaveImpl::getListConfig(const char8_t *name) const
    {
        GameManager::GameBrowserListConfigMap::const_iterator listConfigMapIter = getConfig().getGameBrowser().getListConfig().find(name);
        if (listConfigMapIter != getConfig().getGameBrowser().getListConfig().end())
        {
            return listConfigMapIter->second;
        }
        listConfigMapIter = getConfig().getGameBrowser().getInternalListConfig().find(name);
        if (listConfigMapIter != getConfig().getGameBrowser().getInternalListConfig().end())
        {
            return listConfigMapIter->second;
        }
        return nullptr;
    }

    const GameManager::MatchmakingServerConfig& SearchSlaveImpl::getMatchmakingConfig() const 
    { 
        return getConfig().getMatchmakerSettings(); 
    }

    bool SearchSlaveImpl::getDoGameProtocolVersionStringCheck() const 
    { 
        return getConfig().getGameSession().getEvaluateGameProtocolVersionString(); 
    }

    TimeValue SearchSlaveImpl::getDefaultIdlePeriod() const
    { 
        // TODO: This can be transfered over to Search Config
        return getConfig().getGameBrowser().getDefaultIdlePeriod(); 
    } 

    /*! ************************************************************************************************/
    /*! \brief Intrusive structures cleanup
    ***************************************************************************************************/
    Blaze::GameManager::GbListGameIntrusiveNode* SearchSlaveImpl::createGameInstrusiveNode(GameSessionSearchSlave& game, GameList& gbList)
    {
        return new (mGameIntrusiveNodeAllocator) Blaze::GameManager::GbListGameIntrusiveNode(game, gbList);
    }

    void SearchSlaveImpl::deleteGameIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode *node)
    {
        node->~GbListGameIntrusiveNode();
        GameManager::GbListGameIntrusiveNode::operator delete (node, mGameIntrusiveNodeAllocator);
    }

    GameMatch* SearchSlaveImpl::createGameMatch(GameManager::FitScore fitScore, GameManager::GameId gameId, GameManager::Rete::ProductionInfo* pInfo)
    { 
        return new (mGameMatchAllocator.allocate(sizeof(GameMatch))) GameMatch(fitScore, gameId, pInfo); 
    }
    
    void SearchSlaveImpl::deleteGameMatch(GameMatch* node) 
    { 
        mGameMatchAllocator.deallocate(node); 
    }

    /*! ************************************************************************************************/
    /*! \brief 'runs' the search slave for one time slot
        \return true if another idle loop needs to be scheduled
    *************************************************************************************************/
    GameManager::IdlerUtil::IdleResult SearchSlaveImpl::idle()
    {
        mMetrics.mTotalIdles.increment();
        TimeValue idleStartTime = TimeValue::getTimeOfDay();
        // capture the size of the dirty game set before it's cleared during the idle
        mMetrics.mDirtyGameSetSize = mDirtyGameSet.size();
        
        TRACE_LOG("[SearchSlaveImpl] Entering idle " << mMetrics.mTotalIdles.getTotal() << " at time " << idleStartTime.getMillis());

        //Update any gamelists with dirty games
        uint32_t dirtyEvals = 0;
        // need to iterate matches, otherwise we will miss GB list updates for members that aren't tracked via RETE
        // ex: admin list, which is in GB game data, but has no rule associated with it.
        // Need to do this because onTokenUpdated / onTokenAdded isn't exercised if there's no WME upsert/insert
        for (GameIdSet::iterator itr = mDirtyGameSet.begin(), end = mDirtyGameSet.end(); itr != end; ++itr)
        {
            GameManager::GameId gameId = *itr;
            GameSessionSearchSlave *game = getGameSessionSlave(gameId);

            mUserSetSearchManager.onUpdate(gameId);

            if (game != nullptr && game->getGbListIntrusiveListSize() != 0)
            {
                //Now update each list that holds a ref to the game
                for (GameManager::GbListIntrusiveList::iterator setItr = game->getGbListIntrusiveList().begin(),
                    setEnd = game->getGbListIntrusiveList().end(); setItr != setEnd; ++setItr)
                {
                    GameManager::GbListGameIntrusiveNode &n = static_cast<GameManager::GbListGameIntrusiveNode &>(*setItr);
                    n.mGameBrowserList.updateGame(*game);
                }

                dirtyEvals += game->getGbListIntrusiveListSize();
            }
        }
        mDirtyGameSet.clear();

        // get dirty eval time
        TimeValue idleTime1 = TimeValue::getTimeOfDay();
        TimeValue processDirtyGamesDuration = idleTime1 - idleStartTime;

        sendListUpdateNotifications();

        mMetrics.mTotalGameUpdates += mIdleMetrics.mGameUpdatesByIdleEnd;
        mMetrics.mTotalMatchUpdatesInGameListsSent.increment(mIdleMetrics.mTotalGamesSentToGameLists.get());
        mMetrics.mTotalUniqueMatchUpdatesInGameListsSent.increment(mIdleMetrics.mUniqueGamesSentToGameLists.get());
        mMetrics.mTotalGamesSentToGameLists.increment(mIdleMetrics.mTotalGamesSentToGameLists.get());
        mMetrics.mUniqueGamesSentToGameLists.increment(mIdleMetrics.mUniqueGamesSentToGameLists.get());
        mMetrics.mTotalNewGamesSentToGameLists.increment(mIdleMetrics.mTotalNewGamesSentToGameLists.get());
        mMetrics.mUniqueNewGamesSentToGameLists.increment(mIdleMetrics.mUniqueNewGamesSentToGameLists.get());
        mMetrics.mTotalUpdatedGamesSentToGameLists.increment(mIdleMetrics.mTotalUpdatedGamesSentToGameLists.get());
        mMetrics.mUniqueUpdatedGamesSentToGameLists.increment(mIdleMetrics.mUniqueUpdatedGamesSentToGameLists.get());
        mMetrics.mTotalRemovedGamesSentToGameLists.increment(mIdleMetrics.mTotalRemovedGamesSentToGameLists.get());
        mMetrics.mTotalGameListsUpdated.increment(mIdleMetrics.mTotalGameListsUpdated.get());
        mMetrics.mTotalGameListsNotUpdated.increment(mIdleMetrics.mTotalGameListsNotUpdated.get());
        mMetrics.mTotalGameListsDeniedUpdates.increment(mIdleMetrics.mTotalGameListsDeniedUpdates.get());

        
        TimeValue endTime = TimeValue::getTimeOfDay();
        TimeValue sendListUpdateNotificationsTime = endTime - idleTime1;
        mLastIdleLength = endTime - idleStartTime;
        TRACE_LOG("[SearchSlaveImpl] Exiting idle " << mMetrics.mTotalIdles.getTotal() << "; idle duration " << mLastIdleLength.getMillis() << " ms, timeSinceLastIdle " 
                  << (endTime - mIdleMetrics.mLastIdleEndTime).getMillis() << " ms, (processDG " << processDirtyGamesDuration.getMillis() 
                  << " ms, sendListUpdateNotifs " << sendListUpdateNotificationsTime.getMillis() << " ms), '" << mAllLists.size() 
                  << "' lists('" << mSubscriptionLists.size() << "' subscriptions), '" << mIdleMetrics.mNewListsAtIdleStart << "' new lists, '" 
                  << mIdleMetrics.mListsDestroyedAtIdleStart << "' newly destroyed, '" << dirtyEvals << "' dirty evals, '" 
                  << mGameSessionSlaveMap.size() << "' total games");

        SPAM_LOG("[SearchSlaveImpl] idle " << mMetrics.mTotalIdles.getTotal() << "; Create List Timmings: list avg " 
            << (mIdleMetrics.mNewListsAtIdleStart == 0? 0 : mIdleMetrics.mFilledAtCreateList_SumTimes.getMillis() / mIdleMetrics.mNewListsAtIdleStart)
            << " ms, max " << mIdleMetrics.mFilledAtCreateList_MaxTime.getMillis() << " ms, min " << mIdleMetrics.mFilledAtCreateList_MinTime.getMillis() 
            << " ms '" << mIdleMetrics.mGamesMatchedAtCreateList << "' initial matched games, '"  << mIdleMetrics.mGameUpdatesByIdleEnd 
            << "' games updates, '" << mIdleMetrics.mGamesRemovedFromMatchesByIdleEnd 
            << "' games removed, '" << mIdleMetrics.mVisibleGamesUpdatesByIdleEnd << "' visible games updated");
        

        // Clear idle metrics
        mIdleMetrics.reset();
        mIdleMetrics.mLastIdleEndTime = TimeValue::getTimeOfDay();
        
        return mAllLists.empty() ? NO_NEW_IDLE : SCHEDULE_IDLE_NORMALLY;
    }


    void SearchSlaveImpl::sendListUpdateNotifications()
    {
        typedef eastl::map<InstanceId, NotifyGameListUpdate> NotifyGameListUpdateByInstanceSessionId;
        NotifyGameListUpdateByInstanceSessionId notifyGameListUpdateByInstanceSessionId;

        //store game ids in a set so we don't encode them more than once per update
        GameListBase::EncodedGamesPerInstanceId idleEncodedGames;

        typedef eastl::list<GameManager::GameBrowserListId> SnapshotListIds; 
        SnapshotListIds snapshotListIds;
        GameListByGameBrowserListId::iterator it = mAllLists.begin();
        GameListByGameBrowserListId::iterator itEnd = mAllLists.end();
        // find the last list processed to see if we can resume where we left off last idle.
        if (mLastListProcessed != 0)
        {
            GameListByGameBrowserListId::iterator resumeIt = mAllLists.find(mLastListProcessed);
            // if we didn't find our previous last list, we'll just start over at the beginning of mAllLists
            if (resumeIt != itEnd)
            {
                // potentially, we'll reach the end before we hit our game max, but can pick back up next idle.
                // resumeIt got *some* updates last idle, but since it was the one that exceeded our game limit, it may not have been fully processed
                // pick up again here
                it = resumeIt;
            }
        }

        // iterate the game lists and create notifications for each instances
        for (; it != itEnd; ++it)
        {
            GameManager::GameBrowserListId listId = it->first;
            GameListBase* gameList = it->second;
            InstanceId instanceId = GetInstanceIdFromInstanceKey64(gameList->getOriginalInstanceSessionId());

            SPAM_LOG("[SearchSlaveImpl.sendListUpdateNotifications] Sending notifications for list id: " << listId);
            bool exceededGameLimit = false;
            NotifyGameListUpdate &instanceUpdate = notifyGameListUpdateByInstanceSessionId[instanceId];            
            auto& instanceEncodedGames = idleEncodedGames[instanceId];

            if (gameList->hasUpdates())
            {
                
                gameList->getUpdates(instanceUpdate, instanceEncodedGames, mIdleMetrics);
                exceededGameLimit = (uint32_t)mIdleMetrics.mUniqueGamesSentToGameLists.get() >= getConfig().getMaxGamesUpdatedPerIdle();
            }

            if ((gameList->getListType() == GameManager::GAME_BROWSER_LIST_SNAPSHOT) && (gameList->getUpdatedGamesSize() == 0))
            {     
                snapshotListIds.push_back(listId);
            }
            else
            {
                // update here because if it was a snapshot that got a final update, we know it will be gone the next idle.
                // any other list is likely to exist until the next time around.
                mLastListProcessed = listId;
            }

            if (exceededGameLimit)
            {
                break;
            }
        }

        if (itEnd == it)
        {
            // got to the end of the list, start from the beginning next idle
            mLastListProcessed = 0;
        }

        // send notifications to each instance
        for (NotifyGameListUpdateByInstanceSessionId::iterator iter = notifyGameListUpdateByInstanceSessionId.begin(),
                                                                     iterEnd = notifyGameListUpdateByInstanceSessionId.end();
             iter != iterEnd; ++iter)
        {        
            if (!iter->second.getGameListUpdate().empty())
            {
                SlaveSessionPtr session = gController->getConnectionManager().getSlaveSessionByInstanceId(iter->first);
                sendGameListUpdateNotification(session.get(), iter->second);
            }
        }

        // destroy snapshot lists, that have no more games to send
        for (SnapshotListIds::const_iterator iter = snapshotListIds.begin(),
                                             iterEnd = snapshotListIds.end();
             iter != iterEnd; ++iter)
        {
            destroyGameList(*iter);
        }
    }

    void SearchSlaveImpl::onGameSessionUpdated(const GameSessionSearchSlave &gameSession)
    {
        mDirtyGameSet.insert(gameSession.getGameId());
        scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());
    }

    void SearchSlaveImpl::onRemoteInstanceChanged(const RemoteServerInstance& instance)
    {
        if (!instance.isActive())
        {
            // If we detect a lost game manager slave, kill any game lists from that instance
            if (instance.hasComponent(GameManager::GameManagerSlave::COMPONENT_ID))
            {
                GameListIdSet& gameListIds = mGameListIdsByInstanceId[instance.getInstanceId()];
                while (!gameListIds.empty())
                {
                    destroyGameList(*(gameListIds.begin()));
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*!\brief lookup & return the slave's GameSessionSearchSlave object for a given persited game id.  nullptr if none found.

        \param[in] persistedGameId the persisted game id to lookup
        \return the game session, or nullptr if no game is found
    ***************************************************************************************************/
    GameSessionSearchSlave* SearchSlaveImpl::getGameSessionSlaveByPersistedId(const char8_t* persistedGameId) const
    {
        GameManager::GameIdByPersistedIdMap::const_iterator gameSessionMapIter = mGameIdByPersistedIdMap.find(persistedGameId);
        if (gameSessionMapIter != mGameIdByPersistedIdMap.end())
        {
            return getGameSessionSlave(gameSessionMapIter->second);
        }
        return nullptr;
    }

    void SearchSlaveImpl::copyNamedAttributes(Collections::AttributeMap &destAttribMap, const Collections::AttributeMap &srcAttribMap, const Collections::AttributeNameList &attribNameList) 
    {
        Collections::AttributeMap::const_iterator srcAttribMapEnd = srcAttribMap.end();

        // copy over only named attrib(s)
        for (Collections::AttributeNameList::const_iterator attribNameListIter = attribNameList.begin(), attribNameListEnd = attribNameList.end(); attribNameListIter != attribNameListEnd; ++attribNameListIter)
        {
            Collections::AttributeMap::const_iterator attribIter = srcAttribMap.find(*attribNameListIter);
            if (attribIter != srcAttribMapEnd) 
            {
                destAttribMap[attribIter->first] = attribIter->second;
            }
        }
    }

    void SearchSlaveImpl::initGameBrowserPlayerData(GameManager::GameBrowserPlayerData &gameBrowserPlayerData, const eastl::string &listConfigName, GameManager::PlayerInfo &playerInfo) 
    {
        const GameManager::GameBrowserListConfig* listConfigPtr = getListConfig(listConfigName.c_str());
        if (listConfigPtr == nullptr)
        {
            ERR_LOG("[SearchSlaveImpl].initGameBrowserPlayerData: Unsupported game list name used: " << listConfigName);
            return;
        }
        const GameManager::GameBrowserListConfig& listConfig = *listConfigPtr;


        // set common player data that we always send to the client
        playerInfo.getPlatformInfo().copyInto(gameBrowserPlayerData.getPlatformInfo());
        gameBrowserPlayerData.setPlayerId(playerInfo.getPlayerId());
        gameBrowserPlayerData.setExternalId(getExternalIdFromPlatformInfo(gameBrowserPlayerData.getPlatformInfo()));
        gameBrowserPlayerData.setAccountLocale(playerInfo.getAccountLocale());
        gameBrowserPlayerData.setAccountCountry(playerInfo.getAccountCountry());
        gameBrowserPlayerData.setPlayerName(playerInfo.getPlayerName());
        gameBrowserPlayerData.setPlayerNamespace(playerInfo.getPlayerNameSpace());
        gameBrowserPlayerData.setTeamIndex(playerInfo.getTeamIndex());
        gameBrowserPlayerData.setPlayerState(playerInfo.getPlayerState());
        gameBrowserPlayerData.setRoleName(playerInfo.getRoleName());
        gameBrowserPlayerData.setSlotType(playerInfo.getSlotType());
        gameBrowserPlayerData.setJoinedGameTimestamp(playerInfo.getJoinedGameTimestamp());
        gameBrowserPlayerData.setReservationCreationTimestamp(playerInfo.getReservationCreationTimestamp());
        gameBrowserPlayerData.setEncryptedBlazeId(playerInfo.getPlayerData()->getEncryptedBlazeId());

        // copy only the player attributes that this listConfig specifies
        if (listConfig.getDownloadAllPlayerAttributes())
        {
            playerInfo.getPlayerAttribs().copyInto(gameBrowserPlayerData.getPlayerAttribs());
        }
        else
        {
            copyNamedAttributes(gameBrowserPlayerData.getPlayerAttribs(), playerInfo.getPlayerAttribs(), listConfig.getDownloadPlayerAttributes());
        }
    }

    // inits gb game data including current fit score.
    void SearchSlaveImpl::initGameBrowserGameData(GameManager::GameBrowserGameData &gameBrowserGameData, const eastl::string &listConfigName, const Search::GameSessionSearchSlave& gameSession, const GameManager::PropertyNameList* propertiesUsed)
    {
        const GameManager::GameBrowserListConfig* listConfigPtr = getListConfig(listConfigName.c_str());
        if (listConfigPtr == nullptr)
        {
            ERR_LOG("[SearchSlaveImpl].initGameBrowserGameData: Unsupported game list name used: " << listConfigName);
            return;
        }
        const GameManager::GameBrowserListConfig& listConfig = *listConfigPtr;


        // set common data that we always send to the client
        gameBrowserGameData.setGameId(gameSession.getGameId());

        // Find all of the Properties that are required for Packer:
        if (propertiesUsed != nullptr && !propertiesUsed->empty())
        {
            const char8_t* missingProperty = nullptr;
            if (!mPropertyManager.convertProperties(gameSession.getGameProperties(), *propertiesUsed, gameBrowserGameData.getPropertyNameMap(), &missingProperty))
            {
                ERR_LOG("[SearchSlaveImpl].initGameBrowserGameData: Unable to find property '" << missingProperty << "' in Game.");
                return;
            }

            return;
        }

        // Early out for dedicated server request, where we only care about the game id
        if (listConfigPtr->getDownloadOnlyGameId())
            return;

        gameBrowserGameData.setGameName(gameSession.getGameName());
        gameBrowserGameData.setGameProtocolVersionString(gameSession.getGameProtocolVersionString());
        gameBrowserGameData.setVoipTopology(gameSession.getVoipNetwork());
        gameBrowserGameData.setPresenceMode(gameSession.getPresenceMode());
        gameSession.getPresenceDisabledList().copyInto(gameBrowserGameData.getPresenceDisabledList());
        gameBrowserGameData.setGameStatusUrl(gameSession.getGameStatusUrl());

        gameBrowserGameData.setGameMode(gameSession.getGameMode());

        gameBrowserGameData.setGameModRegister(gameSession.getGameModRegister());

        gameBrowserGameData.setPingSiteAlias(gameSession.getBestPingSiteAlias());
        gameBrowserGameData.setNetworkTopology(gameSession.getGameNetworkTopology());
        gameBrowserGameData.setGameType(gameSession.getGameType());

        gameBrowserGameData.getSlotCapacities().assign(gameSession.getSlotCapacities().begin(), 
            gameSession.getSlotCapacities().end());      

        // Populate slot player counts
        gameSession.getPlayerRoster()->getSlotPlayerCounts(gameBrowserGameData.getPlayerCounts());


        GameManager::GameBrowserTeamInfoVector &teamInfoVector = gameBrowserGameData.getGameBrowserTeamInfoVector();
        GameManager::TeamIdVector::const_iterator teamIdsIter = gameSession.getTeamIds().begin();
        GameManager::TeamIdVector::const_iterator teamIdsEnd = gameSession.getTeamIds().end();
        teamInfoVector.clearVector(); // need to make sure we're not double-adding teams
        teamInfoVector.reserve(gameSession.getTeamCount());
        for (GameManager::TeamIndex i = 0; teamIdsIter != teamIdsEnd; ++ teamIdsIter)
        {
            teamInfoVector.pull_back();
            teamInfoVector.back()->setTeamId((*teamIdsIter));
            teamInfoVector.back()->setTeamSize(gameSession.getPlayerRoster()->getTeamSize(i));
            GameManager::RoleMap& gbRoleSizeMap = teamInfoVector.back()->getRoleSizeMap();
            GameManager::RoleSizeMap teamRoleSizes;
            gameSession.getPlayerRoster()->getRoleSizeMap(teamRoleSizes, i);
            if (!teamRoleSizes.empty()) // we know the desired team index is 'in bounds' because iteration is controlled by the game's team vector
            {
                GameManager::RoleSizeMap::const_iterator teamRoleSizeIter = teamRoleSizes.begin();
                GameManager::RoleSizeMap::const_iterator teamRoleSizeEnd = teamRoleSizes.end();
                for (; teamRoleSizeIter != teamRoleSizeEnd; ++teamRoleSizeIter)
                {
                    gbRoleSizeMap[teamRoleSizeIter->first] = teamRoleSizeIter->second;
                }
            }

            ++i;
        }

        gameBrowserGameData.setTeamCapacity(gameSession.getTeamCapacity());
        gameSession.getRoleInformation().copyInto(gameBrowserGameData.getRoleInformation());


        gameBrowserGameData.setGameState(gameSession.getGameState());
        gameBrowserGameData.getGameSettings() = gameSession.getGameSettings();
        gameSession.getTopologyHostNetworkAddressList().copyInto(gameBrowserGameData.getHostNetworkAddressList());
        gameBrowserGameData.setHostId(gameSession.getTopologyHostInfo().getPlayerId());
        gameBrowserGameData.setTopologyHostSessionId(gameSession.getTopologyHostSessionId());
        gameBrowserGameData.getAdminPlayerList().assign(gameSession.getAdminPlayerList().begin(), gameSession.getAdminPlayerList().end());
        gameBrowserGameData.setQueueCapacity(gameSession.getQueueCapacity());
        const GameManager::PlayerRoster::PlayerInfoList& queuedPlayers = gameSession.getPlayerRoster()->getPlayers(GameManager::PlayerRoster::QUEUED_PLAYERS);
        gameBrowserGameData.setQueueCount((uint16_t)queuedPlayers.size());

        gameSession.getEntryCriteria().copyInto(gameBrowserGameData.getEntryCriteriaMap());

        // copy only the game attributes that this listConfig specifies
        if (listConfig.getDownloadAllGameAttributes())
        {
            gameSession.getGameAttribs().copyInto(gameBrowserGameData.getGameAttribs());
        }
        else
        {
            copyNamedAttributes(gameBrowserGameData.getGameAttribs(), gameSession.getGameAttribs(), listConfig.getDownloadGameAttributes());
        }

        gameSession.getDedicatedServerAttribs().copyInto(gameBrowserGameData.getDedicatedServerAttribs());
        gameSession.getCurrentlyAcceptedPlatformList().copyInto(gameBrowserGameData.getClientPlatformList());
        gameBrowserGameData.setIsCrossplayEnabled(gameSession.isCrossplayEnabled());

        // see if we're sending the player roster
        if (listConfig.getDownloadPlayerRosterType() != GameManager::GAMEBROWSER_ROSTER_NONE)
        {
            // GB_AUDIT: consider exposing the roster enumeration direction in the configuration.
            GameManager::PlayerRoster::PlayerRosterType playerRoster = GameManager::PlayerRoster::ALL_PLAYERS;
            if (listConfig.getDownloadPlayerRosterType() == GameManager::GAMEBROWSER_ROSTER_ACTIVE)
            {
                playerRoster = GameManager::PlayerRoster::ACTIVE_PARTICIPANTS;
            }
            else if (listConfig.getDownloadPlayerRosterType() == GameManager::GAMEBROWSER_ROSTER_ALL)
            {
                playerRoster = GameManager::PlayerRoster::ALL_PLAYERS;
            }
            const GameManager::PlayerRoster::PlayerInfoList& activePlayerList = gameSession.getPlayerRoster()->getPlayers(playerRoster);

            gameBrowserGameData.getGameRoster().clearVector();
            gameBrowserGameData.getGameRoster().reserve(activePlayerList.size());

            for (auto& playerInfo : activePlayerList)
            {
                GameManager::GameBrowserPlayerData *gameBrowserPlayerData = gameBrowserGameData.getGameRoster().pull_back();
                initGameBrowserPlayerData(*gameBrowserPlayerData, listConfigName, *playerInfo);
            }
        }

        // see if we need to send the persisted game id
        if (listConfig.getDownloadPersistedGameId())
        {
            gameBrowserGameData.setPersistedGameId(gameSession.getPersistedGameId());
        }
    }


    void SearchSlaveImpl::printReteJoinNodeBackTrace(const GameManager::Rete::JoinNode& joinNode, const GameManager::Rete::ProductionId& id, const GameManager::Rete::ProductionListener* listener, GameManager::DebugRuleResultMap& debugResultMap, bool recurse) const
    {
        GameManager::FitScore fitScore = 0;
        const GameManager::Rete::ConditionProvider* ruleProvider = nullptr;
        eastl::string conditionString;
        // Bit of string building we need to do here as an optimization.  Caching off the string on every join node gets expensive as we create and
        // destroy them often.   Better to incur the cost when debugging, or when TRACE logging is specifically enabled.
        if ( (joinNode.getProductionTest().toString() != nullptr) && (joinNode.getProductionTest().toString()[0] != '\0') )
        {
            conditionString = joinNode.getProductionTest().toString();
        }
        else
        {
            // Deprecated condition - no-op to empty string
        }


        if (listener != nullptr)
        {
            const GameManager::Rete::ConditionInfo* conditionInfo = listener->getConditionInfo(joinNode.getProductionTest());
            if (conditionInfo != nullptr)
            {
                fitScore = conditionInfo->fitScore;
                ruleProvider = conditionInfo->provider;
                if (ruleProvider == nullptr)
                {
                    WARN_LOG("[SearchSlaveImpl] Condition " << conditionString.c_str() << " was added by a rule that did not link itself for debugging information!  Debug results will be incomplete.");
                }
            }
        }

        if (ruleProvider != nullptr)
        {
            // Pull the rule name out of the condition string to make it look pretty
            size_t loc = conditionString.find(ruleProvider->getProviderName());
            if (loc != eastl::string::npos)
            {
                conditionString.replace(loc, strlen(ruleProvider->getProviderName())+1, "");
            }

            TRACE_LOG("[MatchmakingEvaluation] Game '" << id << "' Type 'RETE' Rule '" << ruleProvider->getProviderName() 
                << "' Condition '" << joinNode.getProductionTest().toString() << "' fitScore '" << fitScore << "'." );

            GameManager::DebugRuleResultMap::iterator iter = debugResultMap.find(ruleProvider->getProviderName());
            if (iter != debugResultMap.end())
            {
                // add our conditions and fitScore to an existing result for the rule.
                // Can happen when rete rules specify multiple and conditions, or a rule uses rete and non rete together.
                GameManager::DebugRuleResult *debugResult = iter->second;
                if (debugResult->getResult() == GameManager::MATCH)
                {
                    debugResult->setFitScore(debugResult->getFitScore() + fitScore);
                }
                debugResult->getConditions().push_back(conditionString.c_str());
            }
            else
            {
                GameManager::DebugRuleResult debugResult;

                debugResult.setMaxFitScore(ruleProvider->getProviderMaxFitScore());
                GameManager::Rete::BetaMemory::const_iterator it = joinNode.getBetaMemory().find(id);
                if (it != joinNode.getBetaMemory().end())
                {
                    debugResult.setResult(GameManager::MATCH);
                    debugResult.setFitScore(fitScore);
                }
                else
                {
                    debugResult.setResult(GameManager::NO_MATCH);
                    debugResult.setFitScore(0);
                }
                
                debugResult.getConditions().push_back(conditionString.c_str());
                debugResultMap.insert(eastl::make_pair(ruleProvider->getProviderName(), debugResult.clone()));
            }

        }
        else
        {
            TRACE_LOG("[MatchmakingEvaluation] Game '" << id << "' Type 'RETE' Condition '" << conditionString.c_str() << "' fitScore '" << fitScore << "'." );
        }

        // recurse further up the tree by looking at our parent join node.  A nullptr parent indicates we made it to the top of the tree.
        if(joinNode.getParent() != nullptr && recurse)
        {
            printReteJoinNodeBackTrace(*(joinNode.getParent()), id, listener, debugResultMap);
        }
    }

    void SearchSlaveImpl::printReteBackTrace(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info, GameManager::DebugRuleResultMap& debugResultMap) const
    {
        // the pNode is our matching endpoint.  Our parent is the last join node in the tree that we matched.
        GameManager::Rete::ProductionNode* pNode = info.pNode;
        if (pNode != nullptr)
        {
            printReteJoinNodeBackTrace(pNode->getParent(), token.mId, info.mListener, debugResultMap);
        }
    }

    bool SearchSlaveImpl::printReteBackTrace(const GameManager::Rete::ProductionBuildInfo& info, GameManager::GameId gameId, const GameManager::Rete::ProductionListener* listener, GameManager::DebugRuleResultMap& debugResultMap, bool recurse) const
    {
        // parent is the mismatch node, which is where our production build info lives.
        const GameManager::Rete::JoinNode &parentNode = info.getParent();
        // grandparent will be last matching node.
        const GameManager::Rete::JoinNode *grandParentNode = parentNode.getParent();
        if (grandParentNode != nullptr)
        {
            GameManager::Rete::BetaMemory::const_iterator iter = grandParentNode->getBetaMemory().find(gameId);
            if (iter != grandParentNode->getBetaMemory().end())
            {
                // found the game waiting to be built out.
                const GameManager::Rete::ProductionToken *token = iter->second;
                mMatchmakingSlave.getSearchSlave().printReteJoinNodeBackTrace(parentNode, token->mId, listener, debugResultMap, recurse);
                return true;
            }
            else
            {
                TRACE_LOG("[SearchSlaveImpl] Grandparent " << mReteNetwork.getStringFromHash(grandParentNode->getProductionTest().getName()) 
                    << (grandParentNode->getProductionTest().getNegate() ? " != " : " == ") << mReteNetwork.getStringFromHashedConditionValue(grandParentNode->getProductionTest()) << 
                    " did not contain token for game " << gameId << " from parent " << mReteNetwork.getStringFromHash(parentNode.getProductionTest().getName()) 
                    << (parentNode.getProductionTest().getNegate() ? " != " : " == ") << mReteNetwork.getStringFromHashedConditionValue(parentNode.getProductionTest()));
            }
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Scan the RETE tree to attempt and find the lowest point our game token got to.
               We start with the lowest PBI level, as hopefully our game is in that  section of the tree.
    ***************************************************************************************************/
    const GameManager::Rete::JoinNode* SearchSlaveImpl::scanReteForToken(const GameManager::Rete::ProductionBuildInfo& info, GameManager::GameId gameId) const
    {
        // parent of the PBI was guaranteed not to be a match (sicne the PBI got created)
        // so we start at our grandparent, and scan upwards till we find a token for 
        // the game we are looking for.
        const GameManager::Rete::JoinNode &parentNode = info.getParent();
        const GameManager::Rete::JoinNode *curNode = parentNode.getParent();
        while (curNode != nullptr)
        {
            uint32_t baseLevel = 0;
            uint32_t furthestLevel = 0;
            const GameManager::Rete::JoinNode *furthestNode = nullptr;
            recurseReteForLowestToken(curNode, baseLevel, gameId, furthestNode, furthestLevel);
            if (furthestNode != nullptr)
            {
                return furthestNode;
            }
            curNode = curNode->getParent();
        }
        return nullptr;
    }


    void SearchSlaveImpl::recurseReteForLowestToken(const GameManager::Rete::JoinNode* curNode, uint32_t& curLevel, GameManager::GameId gameId, const GameManager::Rete::JoinNode*& furthestJoinNode, uint32_t& furthestLevel) const
    {
        ++curLevel;
        GameManager::Rete::BetaMemory::const_iterator iter = curNode->getBetaMemory().find(gameId);
        // we found our starting token here.
        if (iter != curNode->getBetaMemory().end())
        {
            if (curLevel > furthestLevel)
            {
                furthestLevel = curLevel;
                furthestJoinNode = curNode;
            }
            GameManager::Rete::JoinNode::JoinNodeMap::const_iterator itr = curNode->getChildren().begin();
            GameManager::Rete::JoinNode::JoinNodeMap::const_iterator end = curNode->getChildren().end();
            for (; itr != end; ++itr)
            {
                const GameManager::Rete::JoinNode* joinNode = itr->second;
                if (joinNode != nullptr)
                {
                    recurseReteForLowestToken(joinNode, curLevel, gameId, furthestJoinNode, furthestLevel);
                }
            }
        }
        --curLevel;
    }

    // return true if the given topology needs QoS validation
    bool SearchSlaveImpl::shouldPerformQosCheck(GameNetworkTopology topology) const
    {
        Blaze::GameManager::QosValidationCriteriaMap::const_iterator qosValidationIter = 
            getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().find(topology);

        // side: warning already logged up front if configuration missing qosCriteriaList
        return ((qosValidationIter != getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().end())
            && !qosValidationIter->second->getQosCriteriaList().empty());
    }

    StorageRecordVersion SearchSlaveImpl::getGameRecordVersion(GameManager::GameId gameId) const
    {
        return mGameListener.getRecordVersion(gameId);
    }

    void SearchSlaveImpl::onUpsertGame(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue)
    {
        GameManager::GameId gameId = recordId;
        GameManager::ReplicatedGameDataServer& replicatedGameData = static_cast<GameManager::ReplicatedGameDataServer&>(fieldValue);
        GameSessionSlaveMap::insert_return_type ret = mGameSessionSlaveMap.insert(gameId);
        bool inserted = ret.second;
        GameSessionSearchSlave* gameSessionSlave = nullptr;
        if (inserted)
        {
            TRACE_LOG("[SearchSlaveImpl].onUpsertGame: gameId=" << gameId << " is inserted");
            gameSessionSlave = BLAZE_NEW GameSessionSearchSlave(*this, replicatedGameData);
            ret.first->second = gameSessionSlave;
            scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());
            if (gameSessionSlave->hasPersistedGameId())
            {
                mGameIdByPersistedIdMap[gameSessionSlave->getPersistedGameId()] = gameSessionSlave->getGameId();
            }
            insertWorkingMemoryElements(*gameSessionSlave);
            mMetrics.mGamesTrackedOnThisSlaveCount.increment();
           
            // after updating matchmaking game info cache in insertWorkingMemoryElements() update diagnostic cache
            mDiagnosticsHelper.updateGameCountsCaches(*gameSessionSlave, true);
        }
        else
        {
            TRACE_LOG("[SearchSlaveImpl].onUpsertGame: gameId=" << gameId << " is updated");
            gameSessionSlave = ret.first->second.get();

            // before updating the game below, decrement diagnostic cache on game's old values
            bool incrementDiagnostics = false;
            mDiagnosticsHelper.updateGameCountsCaches(*gameSessionSlave, incrementDiagnostics);

            gameSessionSlave->updateReplicatedGameData(replicatedGameData);
            // (Game property updates are handled inside of updateWorkingMemoryElements)

            const GameManager::ReplicatedGameDataServer* prevGameSnapshot = gameSessionSlave->getPrevSnapshot();
            if (prevGameSnapshot != nullptr)
            {
                if (blaze_strcmp(prevGameSnapshot->getReplicatedGameData().getPersistedGameId(), gameSessionSlave->getPersistedGameId()) != 0)
                {
                    // use the existing game session's persistent id to find the existing mapping
                    // we can skip the lookup if the old persisted id is the empty string
                    if (prevGameSnapshot->getReplicatedGameData().getPersistedGameId()[0] != '\0')
                    {
                        GameManager::GameIdByPersistedIdMap::iterator it = mGameIdByPersistedIdMap.find(prevGameSnapshot->getReplicatedGameData().getPersistedGameId());
                        if (it != mGameIdByPersistedIdMap.end())
                        {
                            // to be safe we never check if the persisted game id is enabled during updates
                            // if the mapping exists, it *must* be updated.
                            if (gameSessionSlave->getGameId() == it->second)
                            {
                                // erase the mapping
                                mGameIdByPersistedIdMap.erase(it);
                            }
                            else
                            {
                                // this should never happen!
                                ERR_LOG("[SearchSlave] Ignoring update for replicated game persisted map erase for unknown persisted id (id: " 
                                    << gameSessionSlave->getPersistedGameId() << ") of game(" << gameSessionSlave->getGameId() 
                                    << ") because the slave game session mapped by the former != slave game session mapped by the latter");
                            }
                        }
                        else
                        {
                            // the above search should always find a mapping, since we only do it if the old persisted id is non-empty
                            ERR_LOG("[SearchSlave] Ignoring update for replicated game persisted map erase for unknown persisted id (id: " 
                                << gameSessionSlave->getPersistedGameId() << ") of game " << gameSessionSlave->getGameId() 
                                << " because no persistent mapping exists");
                        }
                    }

                    if (gameSessionSlave->hasPersistedGameId())
                    {
                        // create mapping if it is needed
                        mGameIdByPersistedIdMap[gameSessionSlave->getPersistedGameId()] = gameSessionSlave->getGameId();
                    }
                }

                if (!prevGameSnapshot->getReplicatedGameData().getEntryCriteriaMap().equalsValue(gameSessionSlave->getEntryCriteriaMap()) ||
                    !prevGameSnapshot->getReplicatedGameData().getRoleInformation().equalsValue(gameSessionSlave->getRoleInformation()))
                {
                    // replaces: GameManager::GameReplicationReason::GAME_ENTRY_CRITERIA_CHANGED:,
                    // replaces: GameManager::GameReplicationReason::GAME_RESET:, game name rule
                    gameSessionSlave->clearCriteriaExpressions();
                    gameSessionSlave->createCriteriaExpressions();
                    gameSessionSlave->updateRoleEntryCriteriaEvaluators();
                }

                if (!prevGameSnapshot->getReplicatedGameData().getGameAttribs().equalsValue(gameSessionSlave->getGameAttribs()))
                {
                    // replaces: GameManager::GameReplicationReason::GAME_ATTRIB_CHANGED:
                    gameSessionSlave->getMatchmakingGameInfoCache()->setGameAttributeDirty();
                }

                if (!prevGameSnapshot->getReplicatedGameData().getDedicatedServerAttribs().equalsValue(gameSessionSlave->getDedicatedServerAttribs()))
                {
                    // replaces: GameManager::GameReplicationReason::DEDICATED_SERVER_ATTRIB_CHANGED:
                    gameSessionSlave->getMatchmakingGameInfoCache()->setDedicatedServerAttributeDirty();
                }

                if (prevGameSnapshot->getReplicatedGameData().getMaxPlayerCapacity() != gameSessionSlave->getMaxPlayerCapacity() ||
                    prevGameSnapshot->getReplicatedGameData().getMinPlayerCapacity() != gameSessionSlave->getMinPlayerCapacity() ||
                    !prevGameSnapshot->getReplicatedGameData().getSlotCapacities().equalsValue(gameSessionSlave->getSlotCapacities()) ||
                    !prevGameSnapshot->getReplicatedGameData().getTeamIds().equalsValue(gameSessionSlave->getTeamIds()))
                {
                    // replaces: GameManager::GameReplicationReason::PLAYER_CAPACITY_CHANGED: and GameManager::GameReplicationReason::TEAM_ID_CHANGED:

                    // teams may have changed with capacity change
                    gameSessionSlave->getMatchmakingGameInfoCache()->setTeamInfoDirty();

                    // Update the role criterias
                    gameSessionSlave->updateRoleEntryCriteriaEvaluators();
                }

                if (!prevGameSnapshot->getReplicatedGameData().getTopologyHostInfo().equalsValue(gameSessionSlave->getTopologyHostInfo()))
                {
                    // replaces: GameManager::GameReplicationReason::HOST_MIGRATION_STARTED:
                    if (!gameSessionSlave->hasDedicatedServerHost() || gameSessionSlave->getTopologyHostSessionExists())
                    {
                        // When the virtual game is 'ejected/injected' the host info will be cleared, but we do not want to dirty the geo location
                        // if its a dedicated server topology and the host info has been cleared
                        gameSessionSlave->getMatchmakingGameInfoCache()->setGeoLocationDirty();
                    }
                }
                if (!prevGameSnapshot->getReplicatedGameData().getPlatformHostInfo().equalsValue(gameSessionSlave->getPlatformHostInfo()))
                {
                    // replaces: GameManager::GameReplicationReason::HOST_MIGRATION_FINISHED:
                    // Set the roster dirty once host migration is finished.
                    // This will recalculate UED values which are dependent on the new host.
                    gameSessionSlave->getMatchmakingGameInfoCache()->setRosterDirty();
                }

                TRACE_LOG("[SearchSlaveImpl].onUpsertGame: updating RETE tree for game Id: " << gameId);
                updateWorkingMemoryElements(*gameSessionSlave);
                mMatchmakingSlave.onUpdate(*gameSessionSlave);
                mDirtyGameSet.insert(gameSessionSlave->getGameId());

                scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());

                gameSessionSlave->discardPrevSnapshot(); // clear the last snapshot
            }

            // after updating game above, increment diagnostic cache on game's new values
            incrementDiagnostics = true;
            mDiagnosticsHelper.updateGameCountsCaches(*gameSessionSlave, incrementDiagnostics);
        }
    }

    void SearchSlaveImpl::onUpsertPlayer(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue)
    {
        GameManager::GameId gameId = recordId;
        GameSessionSlaveMap::iterator it = mGameSessionSlaveMap.find(gameId);
        if (it != mGameSessionSlaveMap.end())
        {
            GameSessionSearchSlave *gameSessionSlave = it->second.get();
            GameManager::ReplicatedGamePlayerServer& player = static_cast<GameManager::ReplicatedGamePlayerServer&>(fieldValue);
            
            gameSessionSlave->onUpsert(player);
        }
    }

    void SearchSlaveImpl::onErasePlayer(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue)
    {
        GameManager::GameId gameId = recordId;
        GameSessionSlaveMap::iterator it = mGameSessionSlaveMap.find(gameId);
        if (it != mGameSessionSlaveMap.end())
        {
            GameSessionSearchSlave *gameSessionSlave = it->second.get();
            GameManager::ReplicatedGamePlayerServer& player = static_cast<GameManager::ReplicatedGamePlayerServer&>(fieldValue);
            gameSessionSlave->onErase(player);
        }
    }

    void SearchSlaveImpl::onEraseGame(StorageRecordId recordId)
    {
        GameManager::GameId gameId = recordId;
        GameSessionSlaveMap::iterator it = mGameSessionSlaveMap.find(gameId);
        if (it != mGameSessionSlaveMap.end())
        {
            GameSessionSearchSlave* gameSessionSlave = it->second.get();

            mUserSetSearchManager.onErase(gameSessionSlave->getGameId());

            // always try to erase the persistent mapping(even if it might not be created)
            mGameIdByPersistedIdMap.erase(gameSessionSlave->getPersistedGameId());

            // if replicated players are still present then make them go away
            GameManager::PlayerRoster::PlayerInfoList players = gameSessionSlave->getPlayerRoster()->getPlayers(GameManager::PlayerRoster::ALL_PLAYERS);
            for (GameManager::PlayerRoster::PlayerInfoList::const_iterator i = players.begin(), e = players.end(); i != e; ++i)
                gameSessionSlave->onErase(*(*i)->getPlayerData());

            // after updating diagnostics on the erase of the players, do final diagnostics decrement for the game
            mDiagnosticsHelper.updateGameCountsCaches(*gameSessionSlave, false);

            mMetrics.mGamesTrackedOnThisSlaveCount.decrement();
            TRACE_LOG("[SearchSlaveImpl].onErase: removing info from RETE tree for game Id: " << gameId);
            removeWorkingMemoryElements(*gameSessionSlave);
            scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());

            mGameSessionSlaveMap.erase(gameId);
        }
    }

} // Search namespace
} // Blaze namespace
