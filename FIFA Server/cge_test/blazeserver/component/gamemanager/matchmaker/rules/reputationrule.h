/*! ************************************************************************************************/
/*!
    \file   reputationrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_REPUTATION_RULE
#define BLAZE_MATCHMAKING_REPUTATION_RULE

#include "gamemanager/matchmaker/rules/reputationruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class MatchmakingCriteriaError;
namespace Matchmaker
{

    class ReputationRule : public SimpleRule
    {
        NON_COPYABLE(ReputationRule);
    public:
        ReputationRule(const ReputationRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ReputationRule(const ReputationRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~ReputationRule() override {};

        //////////////////////////////////////////////////////////////////////////
        // from ReteRule grandparent
        //////////////////////////////////////////////////////////////////////////
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        // we don't treat ACCEPT_ANY_REPUTATION as a MatchAny value as there are some restrictive one-way requirements
        bool isMatchAny() const override { return false; }

        FitScore getMaxFitScore() const override { return 0; }

        //////////////////////////////////////////////////////////////////////////
        // from Rule
        //////////////////////////////////////////////////////////////////////////
        /*! ************************************************************************************************/
        /*!
            \brief initialize a reputationRule instance for use in a matchmaking session. Return true on success.
        
            \param[in]  criteriaData - Criteria data used to initialize the rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        ***************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        /*! returns true if this rule is disabled.  This rule is always enabled*/
        bool isDisabled() const override { return mIsDisabled; }

        // this rule is always owner wins
        void voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override {};

    protected:
        // Create game evaluations from SimpleRule
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        // Find game evaluations from SimpleRule
        void calcFitPercent(const Search::GameSessionSearchSlave &gameSession, float &fitPercent, bool &isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        /*! ************************************************************************************************/
        /*! \brief post-eval ensures that a session isn't able to pull in others with a more restrictive requirement
        ****************************************************************************************************/
        void postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;

        ReputationRequirement getDesiredValue() const { return mDesiredValue; }

    private:

        void enforceRestrictiveOneWayMatchRequirements(const ReputationRule &otherRule, MatchInfo &myMatchInfo) const;
        void calcFitPercentInternal(ReputationRequirement otherValue, float &fitPercent, bool &isExactMatch) const;
        const ReputationRuleDefinition& getDefinition() const { return static_cast<const ReputationRuleDefinition&>(mRuleDefinition); }

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

    private:

        ReputationRequirement mDesiredValue;
        bool mIsDisabled;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_REPUTATION_RULE
