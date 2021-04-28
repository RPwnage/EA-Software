/*! ************************************************************************************************/
/*!
    \file teamminsizerule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_H
#define BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_H

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/teamminsizeruledefinition.h" // used inline below
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
 
    class TeamMinSizeRule : public Rule
    {
        NON_COPYABLE(TeamMinSizeRule);
    public:
        TeamMinSizeRule(const TeamMinSizeRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamMinSizeRule(const TeamMinSizeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~TeamMinSizeRule() override;


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        bool isDisabled() const override { return mRangeOffsetList == nullptr || mDesiredTeamMinSize == 0; }
        const TeamMinSizeRuleDefinition& getDefinition() const { return static_cast<const TeamMinSizeRuleDefinition&>(mRuleDefinition); }

        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        void updateAsyncStatus() override;

        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        uint16_t getDesiredTeamMinSize() const { return mDesiredTeamMinSize; }
        uint16_t getAcceptableTeamMinSize() const { return (uint16_t)mAcceptableSizeRange.mMinValue; }   // The one that reduces with time:

        // RETE implementations
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        // Note: not called since addConditions always returns true
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        // end RETE

    private:
        FitScore calcWeightedFitScoreInternal(bool isExactMatch, float fitPercent, uint16_t actualTeamMinSize) const;


        uint16_t mDesiredTeamMinSize;
        uint16_t mAbsoluteTeamMinSize;  // Set by cfg

        const RangeList *mRangeOffsetList; // this rule has a rangeOffsetList instead of a minFitThreshold.
        const RangeList::Range *mCurrentRangeOffset;
        RangeList::Range mAcceptableSizeRange; // updated when the rule decays

        // Number of players joining in the smallest team that is joining (since multiple teams can be joined with a single MM session)
        uint16_t mSmallestTeamJoiningPlayerCount;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_MINSIZE_RULE_H
