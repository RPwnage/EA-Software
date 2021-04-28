/*! ************************************************************************************************/
/*!
    \file teamuedbalancerule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_H
#define BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h" // used inline below

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class MatchmakingSession;
    struct MatchmakingSessionMatchNode;
    struct MatchedSessionsBucket;
    class CreateGameFinalizationTeamInfo;
    class CreateGameFinalizationSettings;
    struct MatchmakingSupplementalData;
 
    class TeamUEDBalanceRule : public SimpleRangeRule
    {
        NON_COPYABLE(TeamUEDBalanceRule);
    public:
        TeamUEDBalanceRule(const TeamUEDBalanceRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamUEDBalanceRule(const TeamUEDBalanceRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~TeamUEDBalanceRule() override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        const TeamUEDBalanceRuleDefinition& getDefinition() const { return static_cast<const TeamUEDBalanceRuleDefinition&>(mRuleDefinition); }

        bool isEvaluateDisabled() const override { return true; }//eval others vs me even if I'm disabled to ensure don't match if can't.

        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }
        bool isDedicatedServerEnabled() const override { return false; }

        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        // SimpleRule
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        UserExtendedDataValue getAcceptableTeamUEDImbalance() const { return (mAcceptableRange.mMaxValue - mAcceptableRange.mMinValue); }
        UserExtendedDataValue getUEDValue() const { return mUEDValue; }
        UserExtendedDataValue getMaxUserUEDValue() const { return mMaxUserUEDValue; }
        UserExtendedDataValue getMinUserUEDValue() const { return mMinUserUEDValue; }
        uint16_t getPlayerCount() const { return mJoiningPlayerCount; }

        static const char8_t* makeSessionLogStr(const MatchmakingSession* session, eastl::string& buf);

        ////////////// Finalization Helpers
        CreateGameFinalizationTeamInfo* selectNextTeamForFinalization(CreateGameFinalizationSettings& finalizationSettings) const;
        const MatchmakingSessionMatchNode* selectNextSessionForFinalizationByTeamUEDBalance(const MatchmakingSession& pullingSession, const MatchedSessionsBucket& bucket, 
                const CreateGameFinalizationTeamInfo& teamToFill, const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession &autoJoinSession) const;
        bool areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings, UserExtendedDataValue& largestTeamUedDiff) const;

    private:

        void addConditionsMaxOrMin(Rete::OrConditionList& capacityOrConditions) const;
        void addConditionsSum(Rete::OrConditionList& capacityOrConditions, UserExtendedDataValue mySessionSum) const;
        void addConditionsAvg(Rete::OrConditionList& capacityOrConditions) const;

        float doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const;
        // SimpleRule
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

        bool getHighestAndLowestUEDValuedFinalizingTeams(const CreateGameFinalizationSettings& finalizationSettings, const CreateGameFinalizationTeamInfo*& lowest, 
                const CreateGameFinalizationTeamInfo*& highest, TeamId teamId = ANY_TEAM_ID, bool ignoreEmptyTeams = false) const;
        UserExtendedDataValue getLowestAcceptableUEDImbalanceInFinalization(const CreateGameFinalizationSettings& finalizationSettings, const MatchmakingSession** lowestAcceptableTeamImbalance = nullptr) const;

        //pre: initialize() has set mJoiningPlayerCount
        bool isGameBrowsing() const { return (mJoiningPlayerCount == 0); }

        // this post-RETE rule disabled for diagnostics. For CG finalization, has own metrics
        bool isRuleTotalDiagnosticEnabled() const override { return false; }
    private:
        UserExtendedDataValue mUEDValue;
        UserExtendedDataValue mMaxUserUEDValue; // for metrics. MM_TODO: this should probably be copied or moved to TeamUEDPositionParityRule once that rule is available in future Blaze releases.
        UserExtendedDataValue mMinUserUEDValue; // for metrics
        uint16_t mJoiningPlayerCount;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_H
