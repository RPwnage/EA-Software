/*! ************************************************************************************************/
/*!
    \file   PreferredGamesRule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_PREFERRED_GAMES_RULE
#define BLAZE_MATCHMAKING_PREFERRED_GAMES_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/matchmaker/rules/preferredgamesruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Predefined rule for preferring matching games/sessions with specified players
    ***************************************************************************************************/
    class PreferredGamesRule : public SimpleRule
    {
        NON_COPYABLE(PreferredGamesRule);
    public:

        PreferredGamesRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        PreferredGamesRule(const PreferredGamesRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~PreferredGamesRule() override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        //! \brief Rule not time sensitive so don't calculate anything here.
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override
        { 
            return NO_RULE_CHANGE; 
        }

        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override
        {
            WARN_LOG("[PlayersRule].addConditions disabled, non rete");
            return true; 
        }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW PreferredGamesRule(*this, matchmakingAsyncStatus); }

        bool isDisabled() const override { return (mGameIdSet.empty() && !mRequirePreferredGame); }

        bool isEvaluateDisabled() const override { return true; } // others evaluate me even if I'm disabled

        void updateAsyncStatus() override {} // NO_OP. rule is a filter w/o decay

        bool getRequiredGames() const { return mRequirePreferredGame; }

        typedef eastl::hash_set<GameId> GameIdSet;
        const GameIdSet& getGameIdSet() const { return mGameIdSet; }

    protected:

        const PreferredGamesRuleDefinition& getDefinition() const { return static_cast<const PreferredGamesRuleDefinition&>(mRuleDefinition); }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

    private:
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;

        bool mRequirePreferredGame;
        GameIdSet mGameIdSet;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE
#endif
