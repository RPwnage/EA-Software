/*! ************************************************************************************************/
/*!
    \file   xblblockplayersrule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_XBLBLOCK_PLAYERS_RULE
#define BLAZE_MATCHMAKING_XBLBLOCK_PLAYERS_RULE

#include "gamemanager/matchmaker/rules/xblblockplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/playersrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class XblAccountIdWatcher;

    /*! ************************************************************************************************/
    /*! \brief slave use only. Provides access to games players are in. Notifies of updates.
        for the individual player ids.
    ***************************************************************************************************/
    class XblAccountIdToGamesMapping
    {
        NON_COPYABLE(XblAccountIdToGamesMapping);
    public:
        XblAccountIdToGamesMapping(Search::SearchSlaveImpl& searchSlaveImpl) : mSearchSlaveImpl(searchSlaveImpl),
                mPlayersToWatchers(BlazeStlAllocator("XblAccountIdToGamesMapping::mPlayersToWatchers", GameManagerSlave::COMPONENT_MEMORY_GROUP))  {}
        ~XblAccountIdToGamesMapping() {}

        inline void addPlayerWatcher(const ExternalXblAccountId extId, XblAccountIdWatcher& watcher);
        inline void removePlayerWatcher(const ExternalXblAccountId extId, const XblAccountIdWatcher& watcher);

        void markWatcherForInitialMatches(XblAccountIdWatcher& watcher);
        void clearWatcherForInitialMatches(const XblAccountIdWatcher& watcher);
        void processWatcherInitialMatches();

        void updateWatchersOnUserAction(const ExternalXblAccountId xblId, const Search::GameSessionSearchSlave& gameSession, bool isJoining);
        const GameManager::UserSessionGameSet *getPlayerCurrentGames(const ExternalXblAccountId xblId) const;
        Search::SearchSlaveImpl& getGameSessionRepository() { return mSearchSlaveImpl; }
    private:
        Search::SearchSlaveImpl &mSearchSlaveImpl;
        typedef eastl::hash_set<XblAccountIdWatcher*> WatcherList; // assumption: hash_set speed outweighs other containers.
        typedef eastl::hash_map<ExternalXblAccountId, WatcherList> PlayersToWatchers;
        PlayersToWatchers mPlayersToWatchers; // players' watchers

        WatcherList mWatchersWithInitialMatches;
    };
    /*! ************************************************************************************************/
    /*! \brief slave use only. Subscribes to be notified of a set of players' game membership changes
    ***************************************************************************************************/
    class XblAccountIdWatcher
    {
    public:
        virtual ~XblAccountIdWatcher() {}
        virtual void onUserAction(GameManager::Rete::WMEManager& wmeManager, const ExternalXblAccountId xblId, const Search::GameSessionSearchSlave& gameSession, bool isJoining) = 0;

        virtual void onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager) = 0;
    };

    /*! ************************************************************************************************/
    /*! \brief Predefined matchmaking rule for blocking games/sessions with specified players.
    ***************************************************************************************************/
    class XblBlockPlayersRule : public XblAccountIdWatcher, public SimpleRule
    {
        NON_COPYABLE(XblBlockPlayersRule);
    public:

        XblBlockPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        XblBlockPlayersRule(const XblBlockPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~XblBlockPlayersRule() override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW XblBlockPlayersRule(*this, matchmakingAsyncStatus); }

        bool isDisabled() const override { return (mXblAccountIdSet.empty()); }

        //! \brief Rule not time sensitive so don't calculate anything here.
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override
        { 
            return NO_RULE_CHANGE; 
        }


        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        bool isAvoidRule() const { return true; }
        bool isRequiresPlayers() const { return true; }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        bool isReteCapable() const override { return mRuleDefinition.isReteCapable() && mContextAllowsRete; }

        void onUserAction(GameManager::Rete::WMEManager& wmeManager, const ExternalXblAccountId xblId, const Search::GameSessionSearchSlave& gameSession, bool isJoining) override;
        void onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager) override;

        bool isSessionBlocked(const MatchmakingSession* otherSession) const;

        typedef eastl::hash_set<ExternalXblAccountId> ExternalXblAccountIdSet;
        static BlazeRpcError fillBlockList(const UserJoinInfoList& matchmakingUsers, ExternalXblAccountIdList& blockListXblIdOut, MatchmakingCriteriaError& err);

    protected:
        bool hasWatchedPlayer(GameId gameId) const
        { 
            GamesToWatchedPlayerCountMap::const_iterator it = mGamesToWatchCounts.find(gameId);
            return ((it != mGamesToWatchCounts.end()) && (it->second != 0));
        }

        void incrementGameWatchCount(GameId gameId) { mGamesToWatchCounts[gameId]++; }
        bool isEvaluateDisabled() const override { return true; } // others evaluate me even if I'm disabled
        void updateAsyncStatus() override {} // NO_OP. rule is a filter w/o decay

    protected:
        const XblBlockPlayersRuleDefinition& getDefinition() const { return static_cast<const XblBlockPlayersRuleDefinition&>(mRuleDefinition); }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;

        ExternalXblAccountIdSet mXblAccountIdSet;
        XblAccountIdToGamesMapping* mPlayerProvider;


    private:
        typedef eastl::hash_map<GameId, uint32_t> GamesToWatchedPlayerCountMap;
        GamesToWatchedPlayerCountMap mGamesToWatchCounts;
        eastl::string mRuleWmeName;
        typedef eastl::hash_set<GameId> GameIdSet;
        GameIdSet mInitialWatchedGames;

        bool mContextAllowsRete;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_XBLBLOCK_PLAYERS_RULE
#endif
