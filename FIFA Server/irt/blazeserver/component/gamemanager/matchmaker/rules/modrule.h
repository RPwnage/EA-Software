#ifndef BLAZE_MATCHMAKING_MOD_RULE_H
#define BLAZE_MATCHMAKING_MOD_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "modruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief A Matchmaking rule to match clients to clients or servers with same or desired set of mods.
               An example of a mod is Downlodable Content pack installed on client (DLC) which may be required
               to enter or create a game with certain set of features.
    *************************************************************************************************/
    class ModRule : public SimpleRule
    {

    public:

        ModRule(const ModRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ModRule(const ModRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        
        ~ModRule() override;

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        bool isDedicatedServerEnabled() const override { return true; }

        // the rule is "match any" if client has all the mods (all bits are set)
        bool isMatchAny() const override { return ((~mDesiredClientMods) == 0); }
        bool isDisabled() const override { return mIsDisabled; }

        // the rule is not bidirectional (reflexive, i.e. A matches B doesn't imply B maches A)
        // so if A matches B attempt finalization immediately, don't wait for 
        // the other rule to decay (it does not decay anyway)
        bool isBidirectional() const override { return false; }

    protected:

        void updateAsyncStatus() override;

    private:

        NON_COPYABLE(ModRule);

        uint32_t mDesiredClientMods;
        bool mIsDisabled;

    private:
        const ModRuleDefinition& getDefinition() const { return static_cast<const ModRuleDefinition&>(mRuleDefinition); }

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MOD_RULE_H
