/*! ************************************************************************************************/
/*!
    \file teambalancerule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_BALANCE_RULE_H
#define BLAZE_MATCHMAKING_TEAM_BALANCE_RULE_H

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/teambalanceruledefinition.h" // used inline below
#include "gamemanager/matchmaker/matchlist.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class RangeList;
    struct MatchmakingSupplementalData;
 
    class TeamBalanceRule : public Rule
    {
        NON_COPYABLE(TeamBalanceRule);
    public:
        TeamBalanceRule(const TeamBalanceRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamBalanceRule(const TeamBalanceRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~TeamBalanceRule() override;


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        bool isDisabled() const override { return mRangeOffsetList == nullptr; }
        const TeamBalanceRuleDefinition& getDefinition() const { return static_cast<const TeamBalanceRuleDefinition&>(mRuleDefinition); }

        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        void updateAsyncStatus() override;

        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        uint16_t getDesiredTeamBalance() const { return mDesiredTeamBalance; }
        uint16_t getAcceptableTeamBalance() const { return (uint16_t)mAcceptableBalanceRange.mMaxValue; }   

        // RETE implementations
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        // Note: not called since addConditions always returns true
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        // end RETE

    private:
        FitScore calcWeightedFitScoreInternal(bool isExactMatch, float fitPercent, uint16_t actualTeamBalance) const;

        uint16_t mDesiredTeamBalance;

        const RangeList *mRangeOffsetList; // this rule has a rangeOffsetList instead of a minFitThreshold.
        const RangeList::Range *mCurrentRangeOffset;
        RangeList::Range mAcceptableBalanceRange; // updated when the rule decays

        uint16_t mJoiningPlayerCount;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_BALANCE_RULE_H
