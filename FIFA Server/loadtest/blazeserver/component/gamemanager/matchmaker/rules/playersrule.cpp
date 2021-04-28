/*! ************************************************************************************************/
/*!
    \file   playersrule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/playersrule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PlayerToGamesMapping
  
    /*! ************************************************************************************************/
    /*! \brief subscribes the watcher to get the player's/account's updates.
    ***************************************************************************************************/
    inline void PlayerToGamesMapping::addPlayerWatcher(const BlazeId playerId, PlayerWatcher& watcher)
    {
        mPlayersToWatchers[playerId].insert(&watcher);
    }
    inline void PlayerToGamesMapping::addPlayerWatcherByAccountId(const AccountId accountId, PlayerWatcher& watcher)
    {
        mAccountsToWatchers[accountId].insert(&watcher);
    }
    /*! ************************************************************************************************/
    /*! \brief if subscribed, unsubscribes watcher from getting player's/account's updates. Cleans internal maps as needed.
    ***************************************************************************************************/
    inline void PlayerToGamesMapping::removePlayerWatcher(const BlazeId playerId,
        const PlayerWatcher& watcher)
    {
        PlayersToWatchers::iterator iter = mPlayersToWatchers.find(playerId);
        if ((iter == mPlayersToWatchers.end()) ||
            (0 == iter->second.erase(const_cast<PlayerWatcher*>(&watcher))))
        {
            TRACE_LOG("[PlayerToGamesMapping].removePlayerWatcher no op due to attempt to clear invalid/non-existent watcher, " << ((iter == mPlayersToWatchers.end())? "" :" for player with no watchers"));
            return;
        }
        // clean up map if player has no one watching it anymore
        if (iter->second.empty())
        {
            TRACE_LOG("[PlayerToGamesMapping].removePlayerWatcher removed last watcher for player(" << playerId << ")");
            mPlayersToWatchers.erase(iter);
        }
    }
    inline void PlayerToGamesMapping::removePlayerWatcherByAccountId(const AccountId accountId,
        const PlayerWatcher& watcher)
    {
        AccountsToWatchers::iterator iter = mAccountsToWatchers.find(accountId);
        if ((iter == mAccountsToWatchers.end()) ||
            (0 == iter->second.erase(const_cast<PlayerWatcher*>(&watcher))))
        {
            TRACE_LOG("[PlayerToGamesMapping].removePlayerWatcherByAccountId no op due to attempt to clear invalid/non-existent watcher, " << ((iter == mAccountsToWatchers.end()) ? "" : " for account with no watchers"));
            return;
        }
        // clean up map if account has no one watching it anymore
        if (iter->second.empty())
        {
            TRACE_LOG("[PlayerToGamesMapping].removePlayerWatcherByAccountId removed last watcher for account(" << accountId << ")");
            mAccountsToWatchers.erase(iter);
        }
    }
    /*! ************************************************************************************************/
    /*! \brief notify all the player's watchers about the action
        \param[in] blazeId - the player doing the action
        \param[in] accountId - the NucleusAccountId of the player doing the action
        \param[in] gameSession - the game where the action occurred
        \param[in] isJoining - whether player was joining, or else leaving
    ***************************************************************************************************/
    void PlayerToGamesMapping::updateWatchersOnUserAction(const BlazeId blazeId, const AccountId accountId,
        const Search::GameSessionSearchSlave& gameSession, bool isJoining)
    {
        WatcherList* watchers = nullptr;
        PlayersToWatchers::iterator itPlayerWatchers = mPlayersToWatchers.find(blazeId);
        if (itPlayerWatchers != mPlayersToWatchers.end())
        {
            watchers = &itPlayerWatchers->second;
        }
        else
        {
            AccountsToWatchers::iterator itAccountWatchers = mAccountsToWatchers.find(accountId);
            if (itAccountWatchers != mAccountsToWatchers.end())
                watchers = &itAccountWatchers->second;
        }
        if (watchers != nullptr)
        {
            // notify the watchers of the player
            if (IS_LOGGING_ENABLED(Logging::TRACE))
            {
                TRACE_LOG("[PlayerToGamesMapping].updateWatchersOnUserAction: notify all(" << watchers->size() << ") watchers for user(" << blazeId << "), account(" << accountId << ") about " << (isJoining? "join":"leave") << " of game(" << gameSession.getGameId() << ")");
            }
            WatcherList::iterator listSetIter = watchers->begin();
            WatcherList::iterator listSetEnd = watchers->end();
            for (; listSetIter != listSetEnd; ++listSetIter)
            {
                PlayerWatcher *watcher = *listSetIter;
                watcher->onUserAction(mSearchSlaveImpl.getReteNetwork().getWMEManager(), blazeId, gameSession, isJoining);
            }
        }
        else
        {
            TRACE_LOG("[PlayerToGamesMapping].updateWatchersOnUserAction: no update for user(" << blazeId << "), account(" << accountId << "), not being watched. Not notifying anyone of " << (isJoining? "join":"leave") << " of game(" << gameSession.getGameId() << ")");
        }
    }
    /*! ************************************************************************************************/
    /*! \brief Gets current set of games player is in.
    ***************************************************************************************************/
    const UserSessionGameSet* PlayerToGamesMapping::getPlayerCurrentGames(const Blaze::BlazeId playerId) const
    {
        UserSessionId sessionId = gUserSessionManager->getPrimarySessionId(playerId); 
        if (sessionId == INVALID_USER_SESSION_ID)
        {
            TRACE_LOG("[PlayerToGamesMapping] Attempt to find game sessions for player blaze id(" << playerId << ") who is not online. No games found.");
            return nullptr;
        }
        return mSearchSlaveImpl.getUserSessionGameSetBySessionId(sessionId);
    }


    void PlayerToGamesMapping::markWatcherForInitialMatches(PlayerWatcher& watcher)
    {
        mWatchersWithInitialMatches.insert(&watcher);
    }

    void PlayerToGamesMapping::clearWatcherForInitialMatches(const PlayerWatcher& watcher)
    {
        if (!mWatchersWithInitialMatches.erase(const_cast<PlayerWatcher*>(&watcher)))
        {
            WARN_LOG("[PlayerToGamesMapping] Attempt to clearWatcherForInitialMatches not in the mWatchersWithInitialMatches map.");
        }
    }

    void PlayerToGamesMapping::processWatcherInitialMatches()
    {
        // iterate over the matches, add their WMEs
        for (auto& watchter : mWatchersWithInitialMatches)
        {
            watchter->onProcessInitalMatches(mSearchSlaveImpl.getReteNetwork().getWMEManager());
        }

        mWatchersWithInitialMatches.clear();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PlayersRule

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    PlayersRule::PlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
          mRuleWmeName(""),
          mPlayerProvider(nullptr),
          mGamesToWatchCounts(BlazeStlAllocator("mGamesToWatchCounts", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mContextAllowsRete(ruleDefinition.isReteCapable())
          
    {
    }

    PlayersRule::PlayersRule(const PlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
          mRuleWmeName(otherRule.mRuleWmeName),
          mGamesToWatchCounts(BlazeStlAllocator("mGamesToWatchCounts", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mContextAllowsRete(otherRule.mContextAllowsRete)
    {
        mPlayerIdSet = otherRule.mPlayerIdSet;
        mAccountIdSet = otherRule.mAccountIdSet;
        mPlayerProvider = otherRule.mPlayerProvider;
        mGamesToWatchCounts = otherRule.mGamesToWatchCounts;
    }

    PlayersRule::~PlayersRule()
    {
        if (mPlayerProvider != nullptr)
        {
            for (const auto& playerId : mPlayerIdSet)
                mPlayerProvider->removePlayerWatcher(playerId, *this);

            for (const auto& accountId : mAccountIdSet)
                mPlayerProvider->removePlayerWatcherByAccountId(accountId, *this);

            if (isReteCapable())
            {
                // remove initial matches if we haven't processed them yet
                if (!mInitialWatchedGames.empty())
                {
                    // get initial matches set up to be inserted into the WME tree.
                    mPlayerProvider->clearWatcherForInitialMatches(*this);
                }

                // clean up WMEs
                if (!mPlayerIdSet.empty() || !mAccountIdSet.empty())
                {
                    getDefinition().removeAllWMEsForRuleInstance(mPlayerProvider->getGameSessionRepository().getReteNetwork(), mRuleWmeName.c_str());
                }
            }
        }



        mPlayerIdSet.clear();
        mAccountIdSet.clear();
        mPlayerProvider = nullptr;
        mGamesToWatchCounts.clear();
        mInitialWatchedGames.clear();
    }

    void PlayersRule::setMatchmakingContext(const MatchmakingContext& matchmakingContext)
    {
        // we're not a RETE capable rule if we're in create game or trying to reset a dedicated server
        if (matchmakingContext.isSearchingGames())
        {
            mContextAllowsRete = true;
        }
        else
        {
            mContextAllowsRete = false;
        }
    }

    bool PlayersRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        if (isReteCapable())
        {
            if (EA_LIKELY(!mRuleWmeName.empty()))
            {
                if (!isDisabled())
                {

                    Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
                    Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

                    baseOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(mRuleWmeName.c_str(),
                        mRuleDefinition.getWMEBooleanAttributeValue(true), getDefinition(), isAvoidRule()),
                        mRuleDefinition.getWeight(), this));


                    if (!isRequiresPlayers())
                    {
                        // rule accepts games that don't have the listed players
                        baseOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(mRuleWmeName.c_str(),
                            mRuleDefinition.getWMEBooleanAttributeValue(true), getDefinition(), !isAvoidRule()),
                            0, this));

                    }
                }
            }
            else
            {
                ERR_LOG("[PlayersRule].addConditions disabled, no ruleWmeName set.");
            }
        }
        else
        {
            WARN_LOG("[PlayersRule].addConditions disabled, non-rete");
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Populates WMEs for initial matches
    ***************************************************************************************************/
    void PlayersRule::onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager)
    {
        for (auto& initialWatchedGameId : mInitialWatchedGames)
        {
            getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), initialWatchedGameId, true);
        }

        mInitialWatchedGames.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief slave use only. Update my tracking of the games players are in as needed.
    ***************************************************************************************************/
    void PlayersRule::onUserAction(GameManager::Rete::WMEManager& wmeManager, const BlazeId blazeId, const Search::GameSessionSearchSlave& gameSession,
        bool isJoining)
    {
        // update tracked count. Pre: Assumption is there's no duplicate hits here for the same
        // player join/leave op, so that for efficiency we can avoid tracking set of actual players (just counts).
        uint32_t& count = mGamesToWatchCounts[gameSession.getGameId()];
        if (isJoining)
        {
            count++;
            // upsert a wme, a player was added, if required
            if (isReteCapable())
            {
                getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), gameSession.getGameId(), true);
            }
        }
        else
        {
            if (count == 0)
            {
                ERR_LOG("[PlayerWatcher:" << getRuleName() << "].onUserAction internal error, attempted decrementing count below zero, game(" << gameSession.getGameId() << ") user(" << blazeId << ")");
                return;
            }
            
            count--;
            if (count == 0)
            {
                // remove the WME
                // upsert a wme, a player was added, if required
                if (isReteCapable())
                {
                    getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), gameSession.getGameId(), false);
                }
                // remove the entry from the map, too, as this may stick around if we're using the browser
                mGamesToWatchCounts.erase(gameSession.getGameId());
                mInitialWatchedGames.erase(gameSession.getGameId());
            }
        }
        TRACE_LOG("[PlayerWatcher:" << getRuleName() << "].onUserAction game(" << gameSession.getGameId() << ") user(" << blazeId << ") , doing " << (isJoining? "join":"leave") << ", changed count to " << count);
    }

    /*! ************************************************************************************************/
    /*! \brief Rule initialize helpers. For CG cache blaze ids / account ids to quicker hash sets
              'mPlayerIdSet'/ 'mAccountIdSet'  (dupes stripped). 
               For FG/GB setup my PlayerWatcher listener to mPlayerProvider.
    ***************************************************************************************************/
    void PlayersRule::initPlayerIdSetAndListenersForBlazeIds(const Blaze::BlazeIdList& inputBlazeIds, uint32_t maxUsersAdded)
    {
        AccountIdList emptyList;
        initIdSetsAndListeners(inputBlazeIds, emptyList, maxUsersAdded);
    }

    void PlayersRule::initIdSetsAndListeners(const Blaze::BlazeIdList& inputBlazeIds, const AccountIdList& inputAccountIds, uint32_t maxUsersAdded)
    {
        BlazeIdSet onlineBlazeIds;
        AccountIdSet onlineAccounts;
        BlazeIdSet offlineBlazeIds;
        AccountIdSet offlineAccounts;
        for (const auto& accountId : inputAccountIds)
        {
            if (gUserSessionManager->isAccountOnline(accountId))
                onlineAccounts.insert(accountId);
            else
                offlineAccounts.insert(accountId);
        }

        for (const auto& blazeId : inputBlazeIds)
        {
            if (gUserSessionManager->isUserOnline(blazeId))
                onlineBlazeIds.insert(blazeId);
            else
                offlineBlazeIds.insert(blazeId);
        }

        // If we're going to clip players, add potential matches in the following order:
        // (1) Online players specified both by blaze id and account id
        // (2) Online players specified by blaze id only
        // (3) Online players specified by account id only
        // (4) Offline players specified by blaze id
        // (5) Offline players specified by account id
        // To avoid making a blocking call in the rule initialization, we don't look up 
        // user info for offline players; so there may be overlap between offline players
        // specified by blaze id and offline players specified by account id
        bool includeAllAccounts = (onlineBlazeIds.size() + onlineAccounts.size()) <= maxUsersAdded;
        uint32_t added = 0;
        AccountIdSet::iterator accountIt = onlineAccounts.begin();
        while (accountIt != onlineAccounts.end() && added < maxUsersAdded)
        {
            bool includeAccount = includeAllAccounts;
            BlazeIdSet blazeIds;
            gUserSessionManager->getOnlineBlazeIdsByAccountId(*accountIt, blazeIds);
            if (!includeAccount)
            {
                for (const auto& blazeId : blazeIds)
                {
                    BlazeIdSet::iterator it = onlineBlazeIds.find(blazeId);
                    if (it != onlineBlazeIds.end())
                    {
                        includeAccount = true;
                        break;
                    }
                }
            }

            if (!includeAccount)
            {
                ++accountIt;
                continue;
            }

            addWatcherForOnlineAccount(*accountIt, blazeIds, &onlineBlazeIds);
            accountIt = onlineAccounts.erase(accountIt);
            ++added;
        }

        // Add any remaining online accounts, if we freed up enough room
        for (accountIt = onlineAccounts.begin(); accountIt != onlineAccounts.end() && (added + onlineBlazeIds.size() < maxUsersAdded); ++accountIt)
        {
            addWatcherForOnlineAccount(*accountIt);
            ++added;
        }

        // Now add online players that were specified by BlazeId only
        BlazeIdSet::const_iterator blazeIdIt = onlineBlazeIds.begin();
        for (; blazeIdIt != onlineBlazeIds.end() && added < maxUsersAdded; ++blazeIdIt)
        {
            addWatcherForOnlinePlayer(*blazeIdIt, INVALID_ACCOUNT_ID);
            ++added;
        }

        // Finally, add as many offline players as we can, starting with players specified by BlazeId
        for (blazeIdIt = offlineBlazeIds.begin(); blazeIdIt != offlineBlazeIds.end() && added < maxUsersAdded; ++blazeIdIt)
        {
            mPlayerIdSet.insert(*blazeIdIt);
            ++added;
        }

        for (accountIt = offlineAccounts.begin(); accountIt != offlineAccounts.end() && added < maxUsersAdded; ++accountIt)
        {
            mAccountIdSet.insert(*accountIt);
            ++added;
        }

        if ((mPlayerProvider != nullptr) && !mInitialWatchedGames.empty() && isReteCapable())
        {
            // get initial matches set up to be inserted into the WME tree.
            mPlayerProvider->markWatcherForInitialMatches(*this);
        }

        TRACE_LOG("[PlayersRule].initIdSetsAndListeners: Rule " << (getRuleName() ? getRuleName() : "?") << " added '" << mPlayerIdSet.size() << "' players and '" << mAccountIdSet.size() << "' accounts for this matchmaking session.");
    }

    void PlayersRule::addWatcherForOnlineAccount(AccountId accountId)
    {
        BlazeIdSet accountBlazeIds;
        gUserSessionManager->getOnlineBlazeIdsByAccountId(accountId, accountBlazeIds);
        addWatcherForOnlineAccount(accountId, accountBlazeIds, nullptr);
    }

    void PlayersRule::addWatcherForOnlineAccount(AccountId accountId, BlazeIdSet& accountBlazeIds, BlazeIdSet* allBlazeIds)
    {
        mAccountIdSet.insert(accountId);
        if (mPlayerProvider != nullptr)
            mPlayerProvider->addPlayerWatcherByAccountId(accountId, *this);

        for (const auto& blazeId : accountBlazeIds)
        {
            if (allBlazeIds != nullptr)
            {
                BlazeIdSet::iterator it = allBlazeIds->find(blazeId);
                if (it != allBlazeIds->end())
                {
                    SPAM_LOG("[PlayersRule].addWatcherForOnlineAccount Rule " << (getRuleName() ? getRuleName() : "?") << " will match player with blaze id(" << blazeId << "), account id(" << accountId << ") by account id instead of by blaze id.");
                    allBlazeIds->erase(it);
                }
            }

            if (mPlayerProvider != nullptr)
                addWatcherForOnlinePlayer(blazeId, accountId);
        }
    }

    void PlayersRule::addWatcherForOnlinePlayer(BlazeId blazeId, AccountId accountId)
    {
        if (accountId == INVALID_ACCOUNT_ID)
        {

            mPlayerIdSet.insert(blazeId);
            if (mPlayerProvider != nullptr)
            {
                mPlayerProvider->addPlayerWatcher(blazeId, *this);
            }

        }

        if (mPlayerProvider != nullptr)
        {
            // increment game's watched-players count by 1
            const UserSessionGameSet *userGameSet = mPlayerProvider->getPlayerCurrentGames(blazeId);
            if ((userGameSet != nullptr) && !userGameSet->empty())
            {
                for (const auto& gameId : *userGameSet)
                {

                    Search::GameSessionSearchSlave* gameSession = mPlayerProvider->getGameSessionRepository().getGameSessionSlave(gameId);
                    if (gameSession != nullptr)
                    {
                        SPAM_LOG("[PlayersRule].addWatcherForOnlinePlayer Rule " << (getRuleName() ? getRuleName() : "?") << " adding found game id(" << gameId << ") having player blaze id(" << blazeId << ").");
                        incrementGameWatchCount(gameId);
                        mInitialWatchedGames.insert(gameId);

                    }
                    else
                    {
                        ERR_LOG("[PlayersRule].addWatcherForOnlinePlayer Rule " << (getRuleName() ? getRuleName() : "?") << " game id(" << gameId << ") having player blaze id(" << blazeId << ") was nullptr when lookup was attempted.");
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief slave use only. Resolves UserSet members and appends their ids to the blaze id list.
        \param[in] ruleName for optional logging.
    ***************************************************************************************************/
    BlazeRpcError PlayersRule::userSetToBlazeIds(const EA::TDF::ObjectId& blazeObjectId,
        Blaze::BlazeIdList& blazeIdList, const char8_t* ruleName,
        Blaze::GameManager::MatchmakingCriteriaError& criteriaErr)
    {
        if (blazeObjectId == EA::TDF::OBJECT_ID_INVALID)
        {
            TRACE_LOG("[PlayersRule].userSetToBlazeIds ignoring disabled user set id for rule " << (ruleName? ruleName : "?"));
            return Blaze::ERR_OK;
        }
        BlazeRpcError err = ERR_OK;

        {
            UserSession::SuperUserPermissionAutoPtr permPtr(true);
            if (blazeIdList.empty())
            {
                err = gUserSetManager->getUserBlazeIds(blazeObjectId, blazeIdList);
            }
            else
            {
                BlazeIdList tempList;
                err = gUserSetManager->getUserBlazeIds(blazeObjectId, tempList);
                if (err == Blaze::ERR_OK)
                {
                    for (BlazeIdList::iterator j = tempList.begin(); j != tempList.end(); ++j)
                        blazeIdList.push_back(*j);
                }
            }
        }

        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[PlayersRule] Rule " << (ruleName? ruleName : "?") << " failed getting user set id("<< blazeObjectId.toString().c_str() 
                       << ")list blaze ids: "<< ErrorHelp::getErrorDescription(err));

            // We no longer fail the rule if a provided userset does not exist. Instead, we output a warning and ignore the userset.
        }
        return Blaze::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief slave use only. Resolves UserSets members and appends their ids to the blaze id list.
    \param[in] ruleName for optional logging.
    ***************************************************************************************************/
    BlazeRpcError PlayersRule::userSetsToBlazeIds(const Blaze::ObjectIdList& blazeObjectIds,
        Blaze::BlazeIdList& blazeIdList, const char8_t* ruleName,
        Blaze::GameManager::MatchmakingCriteriaError& criteriaErr)
    {
        for (ObjectIdList::const_iterator i = blazeObjectIds.begin(), e = blazeObjectIds.end();
             i != e; ++i)
        {
            BlazeRpcError err = userSetToBlazeIds(*i, blazeIdList, ruleName, criteriaErr);

            if (err != Blaze::ERR_OK)
                return err;
        }

        return ERR_OK;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

