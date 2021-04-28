/*! ************************************************************************************************/
/*!
    \file   totalplayerslotsrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_DESIRED_TOTAL_PLAYER_SLOTS
#define BLAZE_MATCHMAKING_DESIRED_TOTAL_PLAYER_SLOTS

#include "gamemanager/matchmaker/rules/totalplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class TotalPlayerSlotsRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
        \brief Blaze Matchmaking predefined rule.  The Total Slots Rule evaluates matches based on the game's
        total number of player slots (public, private, filled or free). This rule tries to match players
        based off of their total slots rule preferences (desired total slots) and requirements (min, max total slots).

        This rule's maximum total slots also acts as a checked upper bound for other size rules.
    *************************************************************************************************/
    class TotalPlayerSlotsRule : public SimpleRangeRule
    {
    public:
        //! \brief You must initialize the rule before using it.
        TotalPlayerSlotsRule(const TotalPlayerSlotsRuleDefinition &definition, MatchmakingAsyncStatus* status);
        TotalPlayerSlotsRule(const TotalPlayerSlotsRule &other, MatchmakingAsyncStatus* status);
        ~TotalPlayerSlotsRule() override {}
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW TotalPlayerSlotsRule(*this, matchmakingAsyncStatus); }

        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
    
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isDedicatedServerEnabled() const override { return false; }

        /*! ************************************************************************************************/
        /*!
            \brief calculate the size of a game based off of the desired total player slots and current 
                range offset list upper bound.  Value will be between mMinTotalParticipantSlots and mMaxTotalParticipantSlots.

            We prefer to put as many players into a game as the session owner will allow (depending on this
            rule's min/max, desired total player slots, and current range offset list threshold).

            For example, if my range offset is +/- 3, my desiredTotalPlayerSlots is 12, the
            total slots would be 15, since my threshold allows me to accept any games in the range
            9..15 (ie: 12 +/- 3).

            \return the total player slots for the game to create
        ***************************************************************************************************/
        uint16_t calcCreateGameParticipantCapacity() const;

        static bool calcParticipantCapacityAtInitialAndFinalTimes(const TotalPlayerSlotsRuleCriteria& totalSlotRuleCriteria, const RuleDefinitionCollection& ruleDefinitionCollection, uint16_t& capacityAtInitialTime, uint16_t& capacityAtFinalTime);

        uint16_t getMaxTotalParticipantSlots() const { return mMaxTotalParticipantSlots; }


        /*! ************************************************************************************************/
        /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
        ***************************************************************************************************/
        void updateAsyncStatus() override;
        void updateAcceptableRange() override;

        const TotalPlayerSlotsRuleDefinition& getDefinition() const { return static_cast<const TotalPlayerSlotsRuleDefinition&>(mRuleDefinition); }


    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override;
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override;

    private:
        void calcFitPercentInternal(uint16_t otherDesiredTotalPlayerSlots, float &fitPercent, bool &isExactMatch) const;
        uint16_t calcMinTotalParticipantSlotsAccepted() const;
        static uint16_t calcCreateGameParticipantCapacity(const RangeList::Range& desiredRange, uint16_t desiredTotalParticipantSlots, uint16_t maxTotalParticipantSlots, uint16_t minTotalParticipantSlots);
    private:
        TotalPlayerSlotsRule(const TotalPlayerSlotsRule& rhs);
        TotalPlayerSlotsRule &operator=(const TotalPlayerSlotsRule &);

        uint16_t mDesiredTotalParticipantSlots;
        uint16_t mMinTotalParticipantSlots;
        uint16_t mMaxTotalParticipantSlots;
        uint16_t mMyGroupParticipantCount;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_DESIRED_TOTAL_PLAYER_SLOTS
