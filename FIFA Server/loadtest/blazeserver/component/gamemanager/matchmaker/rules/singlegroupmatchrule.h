/*! ************************************************************************************************/
/*!
\file singlegroupmatchrule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_SINGLEGROUPMATCHRULE_H
#define BLAZE_GAMEMANAGER_SINGLEGROUPMATCHRULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class SingleGroupMatchRuleDefinition;

    class SingleGroupMatchRule : public SimpleRule
    {
        NON_COPYABLE(SingleGroupMatchRule);
    public:
        SingleGroupMatchRule(const SingleGroupMatchRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        SingleGroupMatchRule(const SingleGroupMatchRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~SingleGroupMatchRule() override {};

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockid, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void updateAsyncStatus() override {};
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW SingleGroupMatchRule(*this, matchmakingAsyncStatus); }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        
        /*! *********************************************************************************************/
        /*! \brief Single Group Match Rule doesn't contribute to the fit score because it is a filtering rule.
        
            \return 0
        *************************************************************************************************/
        FitScore getMaxFitScore() const override { return 0; }

    private:

    private:
        UserGroupId mUserGroupId;
        bool mIsSingleGroupMatch;
        bool mIsDisabled;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif

