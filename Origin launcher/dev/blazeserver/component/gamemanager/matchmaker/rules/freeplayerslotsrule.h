/*! ************************************************************************************************/
/*!
    \file freeplayerslotsrule.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE
#define BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/freeplayerslotsruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \brief A predefined rule that filters out games whose number of open joinable slots does not fall
            within the specified min and max

    ***************************************************************************************************/
    class FreePlayerSlotsRule : public SimpleRule
    {
        NON_COPYABLE(FreePlayerSlotsRule);
    public:

        FreePlayerSlotsRule(const FreePlayerSlotsRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        FreePlayerSlotsRule(const FreePlayerSlotsRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        ~FreePlayerSlotsRule() override {}

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        // this rule does not evaluate for create game
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        
        /*! returns true if this rule is disabled.  This rule is disabled if the default value for max free slots is passed in */
        bool isDisabled() const override { return (mMaxFreeParticipantSlots == INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE); }

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! FreePlayerSlotsRule is not time sensitive so don't need to calculate anything here. */
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override { return NO_RULE_CHANGE; }

        // RETE implementations
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        // end RETE

        /*! *********************************************************************************************/
        /*! \brief Rule doesn't contribute to the fit score because it is a filtering rule.
            \return 0
        *************************************************************************************************/
        FitScore getMaxFitScore() const override { return 0; }

    protected:
        void updateAsyncStatus() override;

    private:

        const FreePlayerSlotsRuleDefinition& getDefinition() const { return static_cast<const FreePlayerSlotsRuleDefinition&>(mRuleDefinition); }
    
    private:
        uint16_t mMinFreeParticipantSlots;
        uint16_t mMaxFreeParticipantSlots;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_FREE_PLAYER_SLOTS_RULE
