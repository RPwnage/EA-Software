/*! ************************************************************************************************/
/*!
    \file teamchoicerule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_H
#define BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_H

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/teamchoiceruledefinition.h" // used inline below
#include "gamemanager/matchmaker/matchlist.h"
#include "gamemanager/rolescommoninfo.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    struct MatchmakingSupplementalData;
 
    class TeamChoiceRule : public Rule
    {
        NON_COPYABLE(TeamChoiceRule);
    public:
        TeamChoiceRule(const TeamChoiceRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamChoiceRule(const TeamChoiceRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~TeamChoiceRule() override;


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        bool isDisabled() const override { return mTeamIdJoinSizes.empty(); }
        const TeamChoiceRuleDefinition& getDefinition() const { return static_cast<const TeamChoiceRuleDefinition&>(mRuleDefinition); }

        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        void updateAsyncStatus() override;

        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        
        const TeamIdSizeList& getTeamIdSizeList() const { return mTeamIdJoinSizes; }

        // RETE implementations
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        // Note: not called since addConditions always returns true
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        // end RETE

        bool getDuplicateTeamsAllowed() const { return mDuplicateTeamsAllowed; }
        uint16_t getUnspecifiedTeamCount() const { return mUnspecifiedTeamCount; }
        const TeamIdVector& getTeamIds() const { return mTeamIds; }

    private:

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
        void getDiagnosticDisplayString(eastl::string& desiredValuesTag, const TeamIdSizeList& desiredTeamIds) const;

        TeamIdSizeList mTeamIdJoinSizes;
        uint16_t mTeamCount;
        bool mDuplicateTeamsAllowed;

        // Duplicated for compatibility with old team size rule:
        // The vector is sized to match the team count rule, and then is filled in with ANY_TEAM_ID values. 
        TeamIdVector mTeamIds;
        uint16_t mUnspecifiedTeamCount;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_CHOICE_RULE_H
