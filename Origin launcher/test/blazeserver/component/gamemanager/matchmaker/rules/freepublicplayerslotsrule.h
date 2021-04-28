/*! ************************************************************************************************/
/*!
    \file   freepublicplayerslotsrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE
#define BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE

#include "gamemanager/matchmaker/rules/freepublicplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class FreePublicPlayerSlotsRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
        \brief Blaze Matchmaking predefined rule.  

        Matchmaking joins by public slot, so check that there are enough public slots for
        the session's group size. When doing matchmaking searches, this rule is internally always enabled
        This rule is not exposed to clients.

    *************************************************************************************************/
    class FreePublicPlayerSlotsRule : public SimpleRule
    {
    public:

        FreePublicPlayerSlotsRule(const FreePublicPlayerSlotsRuleDefinition &definition, MatchmakingAsyncStatus* status);
        FreePublicPlayerSlotsRule(const FreePublicPlayerSlotsRule &other, MatchmakingAsyncStatus* status);
        ~FreePublicPlayerSlotsRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW FreePublicPlayerSlotsRule(*this, matchmakingAsyncStatus); }

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDisabled() const override { return (mMaxFreePublicPlayerSlots == INVALID_FREE_PUBLIC_PLAYER_SLOTS_RULE_MAX_SIZE); }
        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }
        FitScore getMaxFitScore() const override { return 0; }

        /*! FreePublicPlayerSlotsRule is not time sensitive so don't need to calculate anything here. */
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override { return NO_RULE_CHANGE; }

        /*! FreePublicPlayerSlotsRule is essentially a filter, with no decay. */
        void updateAsyncStatus() override {}

        const FreePublicPlayerSlotsRuleDefinition& getDefinition() const { return static_cast<const FreePublicPlayerSlotsRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

    private:
        FreePublicPlayerSlotsRule(const FreePublicPlayerSlotsRule& rhs);
        FreePublicPlayerSlotsRule &operator=(const FreePublicPlayerSlotsRule &);

        uint16_t calcMinFreePublicPlayerSlotsAccepted() const;
        uint16_t calcMaxFreePublicPlayerSlotsAccepted() const;

    private:

        // used to indicate rule is disabled.
        static const uint16_t INVALID_FREE_PUBLIC_PLAYER_SLOTS_RULE_MAX_SIZE;

        uint16_t mMyGroupParticipantCount; // minimum bound
        uint16_t mMaxFreePublicPlayerSlots; // maximum bound
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_FREE_PUBLIC_PLAYER_SLOT_RULE
