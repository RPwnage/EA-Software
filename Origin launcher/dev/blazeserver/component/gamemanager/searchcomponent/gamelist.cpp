/*! ************************************************************************************************/
/*!
    \file gamelist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/tdf/matchmaker_config_server.h"
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/tdf/search_config_server.h"
#include "gamemanager/searchcomponent/gamelist.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/rolescommoninfo.h"
#include "gamemanager/tdf/gamemanager_server.h"

using namespace Blaze::GameManager;

namespace Blaze
{

namespace Search
{

    
    /*! ************************************************************************************************/
    /*! \brief constructor; list is not ready for use until it's also been initialized.
    ***************************************************************************************************/
    GameListBase::GameListBase(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId originalInstanceSessionId, 
        GameManager::ListType listType, UserSessionId ownerSessionId )
        : mSearchSlave(searchSlave),
          mGameBrowserListId(gameBrowserListId),
          mListType(listType),
          mOwnerSessionId(ownerSessionId),
          mOriginalInstanceSessionId(originalInstanceSessionId)
    { 
    }

    void GameListBase::markGameAsUpdated(GameManager::GameId gameId, GameManager::FitScore fitScore, bool newGame)
    {

        if(newGame && (mRemovedGames.find(gameId) == mRemovedGames.end()))
        {
            mNewGames[gameId] = fitScore;
        }
        else if (mNewGames.find(gameId) == mNewGames.end())
        {
            mUpdatedGames[gameId] = fitScore;
        }

        mRemovedGames.erase(gameId);
    }

    void GameListBase::markGameAsRemoved(GameManager::GameId gameId)
    {
        // add the game to the removed game list and remove it from the updated list (in case it had
        //  previously been updated).  This sort of 'jitter' can occur as dirty games are evaluated and
        //  moved between the visible and reserve containers.
        FitScoreByGameId::iterator it = mNewGames.find(gameId);
        if(it == mNewGames.end())
        {
            //game already sent to client, add to removed list
            mRemovedGames.insert(gameId);
        }
        else
        {
            mNewGames.erase(it);
        }

        mUpdatedGames.erase(gameId);
    }

    bool GameListBase::hasUpdates() const
    {
        return !mNewGames.empty() || !mUpdatedGames.empty() || !mRemovedGames.empty();
    }


    /*! ************************************************************************************************/
    /*! \brief Builds a notification of game browser list updates.
        \param[in/out] instanceUpdate - the notification to build off
        \param[in/out] encodedGamesByListId - the ids of encoded games
        \param[in/out] searchIdleMetrics - metrics for the search slave idle to update during processing
    ***************************************************************************************************/
    void GameListBase::getUpdates(NotifyGameListUpdate& instanceUpdate, EncodedGamesByListConfig& encodedGamesByListConfig, SearchIdleMetrics& searchIdleMetrics)
    {
        uint32_t totalGamesSentThisIdle = (uint32_t)searchIdleMetrics.mTotalGamesSentToGameLists.get();
        uint32_t uniqueGamesSentThisIdle = (uint32_t)searchIdleMetrics.mUniqueGamesSentToGameLists.get();
        uint32_t newGamesSentThisIdle = (uint32_t)searchIdleMetrics.mTotalNewGamesSentToGameLists.get();
        uint32_t uniqueNewGamesSentThisIdle = (uint32_t)searchIdleMetrics.mUniqueNewGamesSentToGameLists.get();
        uint32_t updatedGamesSentThisIdle = (uint32_t)searchIdleMetrics.mTotalUpdatedGamesSentToGameLists.get();
        uint32_t uniqueUpdatedGamesSentThisIdle = (uint32_t)searchIdleMetrics.mUniqueUpdatedGamesSentToGameLists.get();
        uint32_t gamesRemovedThisIdle = (uint32_t)searchIdleMetrics.mTotalRemovedGamesSentToGameLists.get();


        bool gotUpdated = false;

        auto& encodedGameSet = encodedGamesByListConfig[mListConfigName];
        EA::TDF::TdfStructVector<GameManager::GameBrowserGameData >* list = nullptr;
        if (!mNewGames.empty() || !mUpdatedGames.empty())
        {
            list = instanceUpdate.getGameDataMap()[mListConfigName.c_str()];
            if (list == nullptr)
            {
                instanceUpdate.getGameDataMap()[mListConfigName.c_str()] = list = instanceUpdate.getGameDataMap().allocate_element();

            }
        }

        GameListUpdate* update = instanceUpdate.getGameListUpdate().pull_back();
        update->setListId(mGameBrowserListId);

        for (GameIdSet::const_iterator it = mRemovedGames.begin(),
            itEnd = mRemovedGames.end();
            it != itEnd; ++it)
        {
            update->getRemovedGameList().push_back(*it);
            SPAM_LOG("[GameList.getUpdates] listId=" << mGameBrowserListId << " REMOVE gameId=" << *it);
            ++gamesRemovedThisIdle;
        }

        mRemovedGames.clear();

        FitScoreByGameId::iterator newGamesIt = mNewGames.begin();
        FitScoreByGameId::iterator newGamesEnd = mNewGames.end();
        while (newGamesIt != newGamesEnd && totalGamesSentThisIdle < mSearchSlave.getConfig().getMaxGamesUpdatedPerIdle())
        {
            GameFitScoreInfo &fitInfo = *update->getAddedGameFitScoreList().pull_back();
            fitInfo.setGameId(newGamesIt->first);
            fitInfo.setFitScore(newGamesIt->second);

            ++totalGamesSentThisIdle;
            ++newGamesSentThisIdle;
            if (encodedGameSet.count(newGamesIt->first) == 0)
            {
                encodedGameSet.insert(newGamesIt->first);
                GameSessionSearchSlave* sGame = mSearchSlave.getGameSessionSlave(newGamesIt->first);
                if (sGame != nullptr)
                {
                    ++uniqueGamesSentThisIdle;
                    ++uniqueNewGamesSentThisIdle;
                    GameBrowserGameData* data = list->pull_back();
                    mSearchSlave.initGameBrowserGameData(*data, mListConfigName, *sGame, getPropertyNameList());
                }
            }

            SPAM_LOG("[GameList.getUpdates] listId=" << mGameBrowserListId << " ADD gameId=" << newGamesIt->first << " fitScore=" << newGamesIt->second);
            ++newGamesIt;
        }

        FitScoreByGameId::iterator updatedGamesIt = mUpdatedGames.begin();
        FitScoreByGameId::iterator updatedGamesEnd = mUpdatedGames.end();
        while (updatedGamesIt != updatedGamesEnd && totalGamesSentThisIdle < mSearchSlave.getConfig().getMaxGamesUpdatedPerIdle())
        {
            GameFitScoreInfo &fitInfo = *update->getUpdatedGameFitScoreList().pull_back();
            fitInfo.setGameId(updatedGamesIt->first);
            fitInfo.setFitScore(updatedGamesIt->second);

            ++totalGamesSentThisIdle;
            ++updatedGamesSentThisIdle;

            if (encodedGameSet.count(updatedGamesIt->first) == 0)
            {
                encodedGameSet.insert(updatedGamesIt->first);
                GameSessionSearchSlave* sGame = mSearchSlave.getGameSessionSlave(updatedGamesIt->first);
                if (sGame != nullptr)
                {
                    ++uniqueGamesSentThisIdle;
                    ++uniqueUpdatedGamesSentThisIdle;
                    GameBrowserGameData* data = list->pull_back();
                    mSearchSlave.initGameBrowserGameData(*data, mListConfigName, *sGame, getPropertyNameList());
                }
            }

            SPAM_LOG("[GameList.getUpdates] listId=" << mGameBrowserListId << " UPDATE gameId=" << updatedGamesIt->first << " fitScore=" << updatedGamesIt->second);
            ++updatedGamesIt;
        }

        if (newGamesIt != mNewGames.begin())
        {
            mNewGames.erase(mNewGames.begin(), newGamesIt);
            gotUpdated = true;
        }
        if (updatedGamesIt != mUpdatedGames.begin())
        {
            mUpdatedGames.erase(mUpdatedGames.begin(), updatedGamesIt);
            gotUpdated = true;
        }
        


        
        if (gotUpdated)
        {
            searchIdleMetrics.mTotalGameListsUpdated.increment();
        }
        else if (totalGamesSentThisIdle < mSearchSlave.getConfig().getMaxGamesUpdatedPerIdle())
        {
            searchIdleMetrics.mTotalGameListsNotUpdated.increment();
        }
        else
        {
            searchIdleMetrics.mTotalGameListsDeniedUpdates.increment();
        }
        

        searchIdleMetrics.mTotalGamesSentToGameLists.set(totalGamesSentThisIdle);
        searchIdleMetrics.mUniqueGamesSentToGameLists.set(uniqueGamesSentThisIdle);
        searchIdleMetrics.mTotalNewGamesSentToGameLists.set(newGamesSentThisIdle);
        searchIdleMetrics.mUniqueNewGamesSentToGameLists.set(uniqueNewGamesSentThisIdle);
        searchIdleMetrics.mTotalUpdatedGamesSentToGameLists.set(updatedGamesSentThisIdle);
        searchIdleMetrics.mUniqueUpdatedGamesSentToGameLists.set(uniqueUpdatedGamesSentThisIdle);
        searchIdleMetrics.mTotalRemovedGamesSentToGameLists.set(gamesRemovedThisIdle);
    }


    /*! ************************************************************************************************/
    /*! \brief constructor; list is not ready for use until it's also been initialized.
    ***************************************************************************************************/
    GameList::GameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId originalInstanceSessionId, GameManager::ListType listType, UserSessionId ownerSessionId, bool filterOnly)
        : GameListBase(searchSlave, gameBrowserListId, originalInstanceSessionId, listType, ownerSessionId),
          mVisibleListCapacity(0),
          mMatchmakingAsyncStatus(),
          mCriteria(searchSlave.getMatchmakingSlave().getRuleDefinitionCollection(), mMatchmakingAsyncStatus, filterOnly ? GameManager::Matchmaker::PropertyRuleDefinition::getRuleDefinitionId() : GameManager::Matchmaker::RuleDefinition::INVALID_DEFINITION_ID),
          mIgnoreGameEntryCriteria(false),
          mIgnoreGameJoinMethod(true),
          mInitialSearchComplete(false),
          mMembersUserSessionInfo(DEFAULT_BLAZE_ALLOCATOR, "MembersUserSessionInfo")
    { 
         TRACE_LOG("[GameList].ctor: List created listId=" << mGameBrowserListId << (filterOnly ? ", filterOnly=true" : ""));
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameList::~GameList()
    {
        TRACE_LOG("[GameList].dtor: List is being destroyed listId=" << mGameBrowserListId);

        // generic rule and skill rule objs are owned by each individual rule
        // instead of session. need to clear here in case double destructed
        mMatchmakingAsyncStatus.getGameAttributeRuleStatusMap().clear();
        mMatchmakingAsyncStatus.getPlayerAttributeRuleStatusMap().clear();
        mMatchmakingAsyncStatus.getUEDRuleStatusMap().clear();
        
        //  GB_TODO: required to explicitly cleanup criteria so Rule objects have access to mMatchmakingAsyncStatus prior to its implicit destruction.
        mCriteria.cleanup();
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize this list with the list's criteria and values.  On failure writes an errMsg into
            err.  List should be destroyed if initialization fails.

        \param[in] request  The getGameListRequest
        \param[in] evaluteGameProtocolVersionString the GameBrowser's config-file override setting for evaluating the GameProtocolVersionString
        \param[out] err We fill out the errMessage field if we encounter an error
        \return ERR_OK on success
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameList::initialize(const GameManager::UserSessionInfo& ownerInfo, const GameManager::GetGameListRequest& request, bool evaluateGameProtocolVersionString, 
        GameManager::MatchmakingCriteriaError &err, uint16_t maxPlayerCapacity, const char8_t* preferredPingSite, GameNetworkTopology netTopology, const GameManager::MatchmakingFilterCriteriaMap& filterMap)
    {
        mVisibleListCapacity = request.getListCapacity();

        ownerInfo.copyInto(mOwnerSessionInfo);
        mMembersUserSessionInfo.push_back(&mOwnerSessionInfo);

        // lookup & store the list's listConfig
        mListConfigName = request.getListConfigName();
        const GameManager::GameBrowserListConfig * listConfig = mSearchSlave.getListConfig(mListConfigName.c_str());
        if (listConfig == nullptr)
        {
            return SEARCH_ERR_INVALID_LIST_CONFIG_NAME;
        }

        mIgnoreGameJoinMethod = listConfig->getIgnoreGameJoinMethod(); // we let the config override the request here
        listConfig->getGameStateWhitelist().copyInto(mGameStateWhitelist);
        listConfig->getDownloadGameTypes().copyInto(mGameTypeList);

        mIgnoreGameEntryCriteria = request.getIgnoreGameEntryCriteria();
        mVisibleListCapacity = request.getListCapacity();

        // populate the matchmaking supplemental data (needed to init the mm criteria)
        GameManager::Matchmaker::MatchmakingSupplementalData matchmakingSupplementalData(listConfig->getGameBrowserSearchContext());
        matchmakingSupplementalData.mGameProtocolVersionString.set(request.getCommonGameData().getGameProtocolVersionString());
        matchmakingSupplementalData.mEvaluateGameProtocolVersionString = evaluateGameProtocolVersionString;
        matchmakingSupplementalData.mPrimaryUserInfo = &mOwnerSessionInfo;
        mGameStateWhitelist.copyInto(matchmakingSupplementalData.mGameStateWhitelist);
        mGameTypeList.copyInto(matchmakingSupplementalData.mGameTypeList);
        matchmakingSupplementalData.mIgnoreGameSettingsRule = mIgnoreGameJoinMethod;
        matchmakingSupplementalData.mIgnoreCallerPlatform = UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_SEARCH_BY_PLATFORM, true);
        const Util::NetworkQosData &networkQosData = mOwnerSessionInfo.getQosData(); 
        networkQosData.copyInto(matchmakingSupplementalData.mNetworkQosData);
        matchmakingSupplementalData.mPlayerToGamesMapping = &mSearchSlave.getMatchmakingSlave().getPlayerGamesMapping();
        matchmakingSupplementalData.mXblAccountIdToGamesMapping = &mSearchSlave.getMatchmakingSlave().getXblAccountIdGamesMapping();
        request.getPlayerJoinData().copyInto(matchmakingSupplementalData.mPlayerJoinData);
        matchmakingSupplementalData.mMaxPlayerCapacity = maxPlayerCapacity;
        matchmakingSupplementalData.mPreferredPingSite.set(preferredPingSite);
        matchmakingSupplementalData.mMembersUserSessionInfo = &mMembersUserSessionInfo;
        matchmakingSupplementalData.mNetworkTopology = netTopology;
        filterMap.copyInto(matchmakingSupplementalData.mFilterMap);
        
        // build role space requirements
        GameManager::buildTeamIdRoleSizeMap(request.getPlayerJoinData(), matchmakingSupplementalData.mTeamIdRoleSpaceRequirements, request.getPlayerJoinData().getPlayerDataList().size(), true);
        GameManager::buildTeamIdPlayerRoleMap(request.getPlayerJoinData(), matchmakingSupplementalData.mTeamIdPlayerRoleRequirements, request.getPlayerJoinData().getPlayerDataList().size(), true);

        // initialize the list's matchmaking criteria
        if ( !mCriteria.initialize(request.getListCriteria(), matchmakingSupplementalData, err) )
        {
            return SEARCH_ERR_INVALID_CRITERIA;
        }

        request.getPropertyNameList().copyInto(mPropertyNameList);

        return ERR_OK;
    }

    void GameList::cleanup()
    {
        mSearchSlave.getReteNetwork().getProductionManager().removeProduction(*this);
    }

    // insertion sort by fitscore
    // NOTE: this assumes that the production info will only get created one time
    // If a second round of pinfo are added then the game browser list
    // needs to get rebuilt and the worst pinfo itr needs to get updated.
    void GameList::ProductionInfoByFitScore::insert(GameManager::Rete::ProductionInfo& pInfo)
    {
        iterator i = begin();
        iterator e = end();

        TRACE_LOG("[ProductionInfoByFitScore].insert pInfo:" << &pInfo << " id " << pInfo.id << " token count " << (uint32_t)pInfo.pNode->getTokenCount());

        if (mProductionInfoList.empty())
        {
            mProductionInfoList.push_back(&pInfo);
            return;
        }

        for (; i != e; ++i)
        {
            // Insert the production info in sorted order by largest fit score
            // take care to insert after info of the same fit score value
            // such that the worst p info iterator is still valid.
            GameManager::Rete::ProductionInfo& currPInfo = **i;
            if (pInfo.fitScore > currPInfo.fitScore)
            {
                mProductionInfoList.insert(i, &pInfo);
                return;
            }
        }

        // If we reached the end and didn't insert then put this node at the end
        mProductionInfoList.push_back(&pInfo);
    }

    // Initial rete search is complete and the production nodes have been
    // ordered so fill out the game browser list with the top games.
    void GameList::initialSearchComplete()
    {
        ProductionInfoByFitScore::iterator itr = mProductionInfoByFitscore.begin();
        ProductionInfoByFitScore::iterator end = mProductionInfoByFitscore.end();

        mWorstPInfoItr = end;
        mInitialSearchComplete = true;

        for (; itr != end; ++itr)
        {
            GameManager::Rete::ProductionInfo& pinfo = **itr;
            GameManager::Rete::ProductionNode& pnode = *pinfo.pNode;

            GameManager::Rete::BetaMemory::const_iterator memItr = pnode.getTokens().begin();
            GameManager::Rete::BetaMemory::const_iterator memEnd = pnode.getTokens().end();

            for (; memItr != memEnd; ++memItr)
            {
                if (mVisibleGameSessionContainer.size() == mVisibleListCapacity)
                {
                    mWorstPInfoItr = itr;
                    return;
                }
                const GameManager::Rete::ProductionToken& token = *memItr->second;
                if (!onTokenAdded(token, pinfo))
                {
                    mWorstPInfoItr = itr;
                    TRACE_LOG("[GameList].initialSearchComplete onTokenAdded is full list (" << mGameBrowserListId << ").");
                    return;
                }
            }
        }
    }

    // 
    void GameList::onProductionNodeHasTokens(GameManager::Rete::ProductionNode& pNode, GameManager::Rete::ProductionInfo& pInfo)
    {
        // Save the production node and production info in a sorted list by fit score
        // Game Browser doesn't want token notifications until all production nodes
        // have been sorted.  It will pull from the best nodes and listen
        // to updates as needed.
        // 
        if (!pInfo.isTracked)
        {
            mProductionInfoByFitscore.insert(pInfo);
            pInfo.isTracked = true;
        }
    }

    void GameList::onProductionNodeDepletedTokens(GameManager::Rete::ProductionNode& pNode)
    {
    }


} // namespace GameManager
} // namespace Blaze
