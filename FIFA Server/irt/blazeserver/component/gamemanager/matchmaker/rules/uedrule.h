/*! ************************************************************************************************/
/*!
\file uedrule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_UEDRULE_H
#define BLAZE_GAMEMANAGER_UEDRULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class UEDRule : public SimpleRangeRule
    {
        NON_COPYABLE(UEDRule);
    public:
        UEDRule(const UEDRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        UEDRule(const UEDRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~UEDRule() override;

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool isEvaluateDisabled() const override { return true; }

        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDedicatedServerEnabled() const override { return false; }

        const UEDRuleDefinition& getDefinition() const { return static_cast<const UEDRuleDefinition&>(mRuleDefinition); }

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        //////////////////////////////////////////////////////////////////////////
        // SimpleRule Interface
        //////////////////////////////////////////////////////////////////////////
        /*! ************************************************************************************************/
        /*! \brief Access to the rule and eval info to fix up results after evaluations.  This function
            is used to make any modifications to the eval or match info before the rule finishes evaluation
            of the evaluateForMMCreateGame.  This can be used for restrictive one way matches where only
            one sessio should be allowed to pull the other session in.  This method is called for both
            this rule and the other rule so only need to worry about my match info.

            \param[in] otherRule The other rule to post evaluate.
            \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
            \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
        ****************************************************************************************************/
        void postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;

        UserExtendedDataValue getSessionUedValue() const { return mUEDValue; }

        bool isBidirectional() const override { return getDefinition().isBidirectional(); }

    protected:
        bool fitFunction(int64_t value, FitScore& fitScore) const;

        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
        void calcFitPercentInternal(const UserExtendedDataValue& otherSkillValue, float &fitPercent, bool &isExactMatch) const;

    // Members
    protected:
        UserExtendedDataValue mUEDSearchValue;
        UserExtendedDataValue mUEDValue;
        bool mAcceptEmptyGames;

        UEDRuleStatus mUEDRuleStatus;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_UEDRULE_H
