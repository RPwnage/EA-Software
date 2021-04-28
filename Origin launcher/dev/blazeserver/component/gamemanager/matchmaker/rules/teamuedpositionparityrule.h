/*! ************************************************************************************************/
/*!
    \file teamuedpositionparityrule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_H
#define BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h" // used inline below

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    struct MatchmakingSupplementalData;
    class MatchmakingSession;
    class CreateGameFinalizationTeamInfo;
 
    class TeamUEDPositionParityRule : public SimpleRangeRule
    {
        NON_COPYABLE(TeamUEDPositionParityRule);
    public:
        TeamUEDPositionParityRule(const TeamUEDPositionParityRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamUEDPositionParityRule(const TeamUEDPositionParityRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~TeamUEDPositionParityRule() override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        const TeamUEDPositionParityRuleDefinition& getDefinition() const { return static_cast<const TeamUEDPositionParityRuleDefinition&>(mRuleDefinition); }

        bool isEvaluateDisabled() const override { return true; }//eval others vs me even if I'm disabled to ensure don't match if can't.

        
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override { WARN_LOG("[TeamUEDPositionParityRule].addConditions disabled, non RETE rule"); return true;}
        bool isDedicatedServerEnabled() const override { return false; }


        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        // SimpleRule
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        UserExtendedDataValue getAcceptableTopUEDImbalance() const { return (mMaxAcceptableTopImbalance); }
        UserExtendedDataValue getAcceptableBottomUEDImbalance() const { return (mMaxAcceptableBottomImbalance); }
        const TeamUEDVector& getUEDValueList() const { return mUEDValueList; }

        static const char8_t* makeSessionLogStr(const MatchmakingSession* session, eastl::string& buf);

        ////////////// Finalization Helpers
        bool areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings) const;
        bool removeSessionFromTeam(TeamUEDVector& teamUedVector) const;

    private:

        float doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const;
        // SimpleRule
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

        bool getHighestAndLowestUEDValueAtRankFinalizingTeams(uint16_t rankIndex, const CreateGameFinalizationSettings& finalizationSettings, const CreateGameFinalizationTeamInfo*& lowest, const CreateGameFinalizationTeamInfo*& highest, TeamId teamId = ANY_TEAM_ID) const;
        UserExtendedDataValue getLowestAcceptableTopUEDImbalanceInFinalization(const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession** lowestAcceptableTeamImbalanceSession = nullptr) const;
        UserExtendedDataValue getLowestAcceptableBottomUEDImbalanceInFinalization(const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession** lowestAcceptableTeamImbalanceSession = nullptr) const;

        // this post-RETE rule disabled for diagnostics. For CG finalization, has own metrics
        bool isRuleTotalDiagnosticEnabled() const override { return false; }

    private:
        UserExtendedDataValue mMaxAcceptableTopImbalance;
        UserExtendedDataValue mMaxAcceptableBottomImbalance;
        TeamUEDVector mUEDValueList;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_H
