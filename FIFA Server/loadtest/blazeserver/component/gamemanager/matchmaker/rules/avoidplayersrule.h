/*! ************************************************************************************************/
/*!
    \file   avoidplayersrule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE
#define BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE

#include "gamemanager/matchmaker/rules/avoidplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/playersrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Predefined matchmaking rule for blocking games/sessions with specified players.
    ***************************************************************************************************/
    class AvoidPlayersRule : public PlayersRule
    {
        NON_COPYABLE(AvoidPlayersRule);
    public:

        AvoidPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        AvoidPlayersRule(const AvoidPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~AvoidPlayersRule() override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW AvoidPlayersRule(*this, matchmakingAsyncStatus); }

        bool isAvoidRule() const override { return true; }
        bool isRequiresPlayers() const override { return true; }

    protected:

        const AvoidPlayersRuleDefinition& getDefinition() const { return static_cast<const AvoidPlayersRuleDefinition&>(mRuleDefinition); }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_AVOID_PLAYERS_RULE
#endif
