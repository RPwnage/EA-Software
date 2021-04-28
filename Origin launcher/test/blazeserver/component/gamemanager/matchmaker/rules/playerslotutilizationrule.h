/*! ************************************************************************************************/
/*!
    \file   playerslotutilizationrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_DESIRED_PLAYER_SLOT_UTILIZATION
#define BLAZE_MATCHMAKING_DESIRED_PLAYER_SLOT_UTILIZATION

#include "gamemanager/matchmaker/rules/playerslotutilizationruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class PlayerSlotUtilizationRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
        \brief Blaze Matchmaking predefined rule.  The Player Slot Utilization Rule evaluates matches based on the game
            percent of the total player slots that are filled. This rule tries to match players based off of 
            their preferences (desired percent full) and requirements (min, max percent full, if specified).
    *************************************************************************************************/
    class PlayerSlotUtilizationRule : public SimpleRangeRule
    {
    public:
        PlayerSlotUtilizationRule(const PlayerSlotUtilizationRuleDefinition &definition, MatchmakingAsyncStatus* status);
        PlayerSlotUtilizationRule(const PlayerSlotUtilizationRule &other, MatchmakingAsyncStatus* status);
        ~PlayerSlotUtilizationRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW PlayerSlotUtilizationRule(*this, matchmakingAsyncStatus); }

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
        
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDisabled() const override { return ((mRangeOffsetList == nullptr) || mForAsyncStatusOnly); }
        bool isDedicatedServerEnabled() const override { return false; }

        /*! ************************************************************************************************/
        /*! \brief override from MMRule; we cache off the current range, not a rangeOffsetThreshold value
        ***************************************************************************************************/
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        const PlayerSlotUtilizationRuleDefinition& getDefinition() const { return static_cast<const PlayerSlotUtilizationRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

    private:
        void calcFitPercentInternal(uint16_t otherDesiredPercentFull, float &fitPercent, bool &isExactMatch) const;
        uint8_t calcMinPercentFullAccepted() const;
        uint8_t calcMaxPercentFullAccepted() const;
        
        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;

    private:
        PlayerSlotUtilizationRule(const PlayerSlotUtilizationRule& rhs);
        PlayerSlotUtilizationRule &operator=(const PlayerSlotUtilizationRule &);

        uint8_t mDesiredPercentFull;
        uint8_t mMinPercentFull;
        uint8_t mMaxPercentFull;
        uint16_t mMyGroupParticipantCount;

        bool mForAsyncStatusOnly;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_DESIRED_PLAYER_SLOT_UTILIZATION
