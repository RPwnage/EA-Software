/*! ************************************************************************************************/
/*!
    \file   qosavoidplayersrule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_QOS_AVOID_PLAYERS_RULE
#define BLAZE_MATCHMAKING_QOS_AVOID_PLAYERS_RULE

#include "gamemanager/matchmaker/rules/qosavoidplayersruledefinition.h"
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
    class QosAvoidPlayersRule : public PlayersRule
    {
        NON_COPYABLE(QosAvoidPlayersRule);
    public:

        QosAvoidPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        QosAvoidPlayersRule(const QosAvoidPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~QosAvoidPlayersRule() override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW QosAvoidPlayersRule(*this, matchmakingAsyncStatus); }

        bool isAvoidRule() const override { return true; }
        bool isRequiresPlayers() const override { return true; }
        bool isDisabled() const override { return (mPlayerIdSet.empty()); }

        /*! ************************************************************************************************/
        /*! \brief Processes an update of the avoided player Ids when QoS fails in create mode.
        ***************************************************************************************************/
        void updateQosAvoidPlayerIdsForCreateMode(const MatchmakingSupplementalData &mmSupplementalData);

    protected:

        const QosAvoidPlayersRuleDefinition& getDefinition() const { return static_cast<const QosAvoidPlayersRuleDefinition&>(mRuleDefinition); }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_QOS_AVOID_PLAYERS_RULE
#endif
