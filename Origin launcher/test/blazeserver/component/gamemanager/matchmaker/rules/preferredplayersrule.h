/*! ************************************************************************************************/
/*!
    \file   preferredplayersrule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE
#define BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE

#include "gamemanager/matchmaker/rules/preferredplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/playersrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Predefined rule for preferring matching games/sessions with specified players
    ***************************************************************************************************/
    class PreferredPlayersRule : public PlayersRule
    {
        NON_COPYABLE(PreferredPlayersRule);
    public:

        PreferredPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        PreferredPlayersRule(const PreferredPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~PreferredPlayersRule() override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW PreferredPlayersRule(*this, matchmakingAsyncStatus); }

        bool isDisabled() const override { return (PlayersRule::isDisabled() && !mRequirePreferredPlayer); }

        bool isAvoidRule() const override { return false; }
        bool isRequiresPlayers() const override { return mRequirePreferredPlayer; }

    protected:

        const PreferredPlayersRuleDefinition& getDefinition() const { return static_cast<const PreferredPlayersRuleDefinition&>(mRuleDefinition); }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

    private:
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;

        bool mRequirePreferredPlayer;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_PREFERRED_PLAYERS_RULE
#endif
