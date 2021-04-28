/*! ************************************************************************************************/
/*!
    \file   playersrule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PLAYERS_RULE
#define BLAZE_MATCHMAKING_PLAYERS_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/tdf/matchmaker.h" // for MatchmakingCriteriaError in userSetToBlazeIds, GameId
#include "gamemanager/matchmaker/rules/playersruledefinition.h" // used inline below
#include "gamemanager/usersessiongameset.h"
#include "gamemanager/rete/retetypes.h" // for wme attribute
#include "EASTL/hash_set.h"

namespace Blaze
{

namespace Search
{
    class SearchSlaveImpl;
    class GameSessionSearchSlave; // for PlayerToGamesMapping::mGameSessionRepository 
}
namespace GameManager
{

namespace Matchmaker
{
    class PlayerWatcher;

    struct MatchmakingContext;

    typedef eastl::hash_set<BlazeId> BlazeIdSet;

    /*! ************************************************************************************************/
    /*! \brief slave use only. Provides access to games players are in. Notifies of updates.
        for the individual player ids.
    ***************************************************************************************************/
    class PlayerToGamesMapping
    {
        NON_COPYABLE(PlayerToGamesMapping);
    public:
        PlayerToGamesMapping(Search::SearchSlaveImpl& searchSlaveImpl) : mSearchSlaveImpl(searchSlaveImpl), 
                mPlayersToWatchers(BlazeStlAllocator("mPlayersToWatchers", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
                mAccountsToWatchers(BlazeStlAllocator("mAccountsToWatchers", GameManagerSlave::COMPONENT_MEMORY_GROUP)) {}
        ~PlayerToGamesMapping() {}

        inline void addPlayerWatcher(const BlazeId playerId, PlayerWatcher& watcher);
        inline void removePlayerWatcher(const BlazeId playerId, const PlayerWatcher& watcher);

        inline void addPlayerWatcherByAccountId(const AccountId accountId, PlayerWatcher& watcher);
        inline void removePlayerWatcherByAccountId(const AccountId accountId, const PlayerWatcher& watcher);

        void markWatcherForInitialMatches(PlayerWatcher& watcher);
        void clearWatcherForInitialMatches(const PlayerWatcher& watcher);
        void processWatcherInitialMatches();

        void updateWatchersOnUserAction(const BlazeId playerId, const AccountId accountId, const Search::GameSessionSearchSlave& gameSession, bool isJoining);
        const GameManager::UserSessionGameSet *getPlayerCurrentGames(const Blaze::BlazeId playerId) const;
        Search::SearchSlaveImpl& getGameSessionRepository() { return mSearchSlaveImpl; }

    private:
        Search::SearchSlaveImpl &mSearchSlaveImpl;
        typedef eastl::hash_set<PlayerWatcher*> WatcherList; // assumption: hash_set speed outweighs other containers.
        typedef eastl::hash_map<BlazeId, WatcherList> PlayersToWatchers;
        typedef eastl::hash_map<AccountId, WatcherList> AccountsToWatchers;
        PlayersToWatchers mPlayersToWatchers; // players' watchers
        AccountsToWatchers mAccountsToWatchers;

        WatcherList mWatchersWithInitialMatches;
    };
    /*! ************************************************************************************************/
    /*! \brief slave use only. Subscribes to be notified of a set of players' game membership changes
    ***************************************************************************************************/
    class PlayerWatcher
    {
    public:
        virtual ~PlayerWatcher() {}
        virtual void onUserAction(GameManager::Rete::WMEManager& wmeManager, const BlazeId playerId, const Search::GameSessionSearchSlave& gameSession, bool isJoining) = 0;

        virtual void onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager) = 0;
    };

    /*! ************************************************************************************************/
    /*! \brief Base for AvoidPlayersRule and PreferredPlayersRule.
    ***************************************************************************************************/
    class PlayersRule : public PlayerWatcher, public SimpleRule
    {
        NON_COPYABLE(PlayersRule);
    public:
        //! \brief You must initialize the rule before using it.
        PlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        PlayersRule(const PlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~PlayersRule() override;

        bool isDisabled() const override { return (mPlayerIdSet.empty() && mAccountIdSet.empty()); }

        //! \brief Rule not time sensitive so don't calculate anything here.
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override
        { 
            return NO_RULE_CHANGE; 
        }
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        bool isMatchAny() const override { return false; }
        virtual bool isAvoidRule() const = 0;
        virtual bool isRequiresPlayers() const = 0;
        bool isDedicatedServerEnabled() const override { return false; }

        bool isReteCapable() const override { return mRuleDefinition.isReteCapable() && mContextAllowsRete; }

        void setMatchmakingContext(const MatchmakingContext& matchmakingContext);

        const PlayersRuleDefinition& getDefinition() const { return static_cast<const PlayersRuleDefinition&>(mRuleDefinition); }

        void onUserAction(GameManager::Rete::WMEManager& wmeManager, const BlazeId blazeId, const Search::GameSessionSearchSlave& gameSession, bool isJoining) override;
        void onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager) override;

        static BlazeRpcError userSetToBlazeIds(const EA::TDF::ObjectId& blazeObjectIds,
            Blaze::BlazeIdList& blazeIdList, const char8_t* ruleName,
            Blaze::GameManager::MatchmakingCriteriaError& criteriaErr);

        static BlazeRpcError userSetsToBlazeIds(const Blaze::ObjectIdList& blazeObjectId,
            Blaze::BlazeIdList& blazeIdList, const char8_t* ruleName,
            Blaze::GameManager::MatchmakingCriteriaError& criteriaErr);

    protected:

        void initPlayerIdSetAndListenersForBlazeIds(const Blaze::BlazeIdList& inputBlazeIds, uint32_t maxUsersAdded);
        void initIdSetsAndListeners(const Blaze::BlazeIdList& inputBlazeIds, const AccountIdList& inputAccountIds, uint32_t maxUsersAdded);

        uint32_t getNumWatchedPlayers(GameId gameId) const
        {
            GamesToWatchedPlayerCountMap::const_iterator it = mGamesToWatchCounts.find(gameId);
            if (it != mGamesToWatchCounts.end())
                return it->second;
            return 0;
        }

        bool hasWatchedPlayer(GameId gameId) const
        { 
            return (getNumWatchedPlayers(gameId) != 0);
        }

        void incrementGameWatchCount(GameId gameId) { mGamesToWatchCounts[gameId]++; }

        bool isEvaluateDisabled() const override { return true; } // others evaluate me even if I'm disabled

        void updateAsyncStatus() override {} // NO_OP. rule is a filter w/o decay

        size_t getNumWatchedGames() const { return mGamesToWatchCounts.size(); }


    protected:
        void addWatcherForOnlinePlayer(BlazeId blazeId, AccountId accountId);
        void addWatcherForOnlineAccount(AccountId accountId);
        void addWatcherForOnlineAccount(AccountId accountId, BlazeIdSet& accountBlazeIds, BlazeIdSet* allBlazeIds);

        eastl::string mRuleWmeName;

        BlazeIdSet mPlayerIdSet;
        AccountIdSet mAccountIdSet;
        PlayerToGamesMapping* mPlayerProvider;

    private:
        //! \brief games filter for FG/GB non-rete evaluation.
        typedef eastl::hash_map<GameId, uint32_t> GamesToWatchedPlayerCountMap;
        GamesToWatchedPlayerCountMap mGamesToWatchCounts;
        
        typedef eastl::hash_set<GameId> GameIdSet;
        GameIdSet mInitialWatchedGames;
        bool mContextAllowsRete;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PLAYERS_RULE
#endif

