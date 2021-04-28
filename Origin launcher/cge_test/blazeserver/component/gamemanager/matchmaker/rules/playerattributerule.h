/*! ************************************************************************************************/
/*!
\file playerattributerule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULE_H
#define BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULE_H

#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class PlayerAttributeRule : public SimpleRule
    {
        NON_COPYABLE(PlayerAttributeRule);
    public:
        PlayerAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        PlayerAttributeRule(const PlayerAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~PlayerAttributeRule() override;

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
        const PlayerAttributeRuleCriteria* getPlayerAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const;

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const override;

        float getFitPercent(const AttributeValuePlayerCountMap &entityValueBitset, bool& outExactMatch) const;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        const PlayerAttributeRuleDefinition& getDefinition() const { return static_cast<const PlayerAttributeRuleDefinition&>(mRuleDefinition); }

        // Rete
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDisabled() const override { return mRuleOmitted || SimpleRule::isDisabled(); }

        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        
        void updateAsyncStatus() override;

    // Members
    protected:
        PlayerAttributeRuleStatus mPlayerAttributeRuleStatus;
        bool mRuleOmitted;

        AttributeValuePlayerCountMap mAttributeValuePlayerCountMap;  // How many players are using the given index 

        Collections::AttributeValue mOwnerAttributeValue;

    private:
        void enforceRestrictiveOneWayMatchRequirements(const PlayerAttributeRule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const;

        void updateCachedMatchedValues();

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
        void getDiagnosticDisplayString(eastl::string& desiredValuesTag) const;

        GameAttributeRuleValueBitset mMatchedValuesBitset; // Used by async status, and diagnostics, to indicate previously matched values.

    // Members
    private:
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULE_H
