/*! ************************************************************************************************/
/*!
    \file teamcountrule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_COUNT_RULE_H
#define BLAZE_MATCHMAKING_TEAM_COUNT_RULE_H

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/teamcountruledefinition.h" // used inline below
#include "gamemanager/matchmaker/matchlist.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    struct MatchmakingSupplementalData;
 
    class TeamCountRule : public Rule
    {
        NON_COPYABLE(TeamCountRule);
    public:
        TeamCountRule(const TeamCountRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamCountRule(const TeamCountRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~TeamCountRule() override;


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        bool isDisabled() const override { return mTeamCount == 0; }
        const TeamCountRuleDefinition& getDefinition() const { return static_cast<const TeamCountRuleDefinition&>(mRuleDefinition); }

        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        void updateAsyncStatus() override;

        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;

        uint16_t getTeamCount() const { return mTeamCount; }

        // RETE implementations
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        // Note: not called since addConditions always returns true
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        // end RETE

    private:
        uint16_t mTeamCount;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_COUNT_RULE_H
