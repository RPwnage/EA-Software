/*! ************************************************************************************************/
/*!
    \file   freepublicplayerslotsrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/freepublicplayerslotsrule.h"
#include "gamemanager/matchmaker/rules/freepublicplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const uint16_t FreePublicPlayerSlotsRule::INVALID_FREE_PUBLIC_PLAYER_SLOTS_RULE_MAX_SIZE = 65535;

    FreePublicPlayerSlotsRule::FreePublicPlayerSlotsRule(const FreePublicPlayerSlotsRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mMyGroupParticipantCount(0),
        mMaxFreePublicPlayerSlots(INVALID_FREE_PUBLIC_PLAYER_SLOTS_RULE_MAX_SIZE)
    {
    }

    FreePublicPlayerSlotsRule::FreePublicPlayerSlotsRule(const FreePublicPlayerSlotsRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRule(other, status),
        mMyGroupParticipantCount(other.mMyGroupParticipantCount),
        mMaxFreePublicPlayerSlots(other.mMaxFreePublicPlayerSlots)
    {
    }


    bool FreePublicPlayerSlotsRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if(isDisabled())
        {
            return true;
        }

        // side: resettable dedicated servers are considered always match for this rule. Omit from tree.

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
 
        uint16_t minCount = 0;
        uint16_t maxCount = 0;

        Rete::OrConditionList& slotOrConditions = conditions.push_back();
        // Public slots 
        // Matchmaking joins by public slot, so check that there are enough public slots.
        // We loop over all possible values for open public slots that still restrict us by our max player capacity.
        minCount = (uint16_t) mMyGroupParticipantCount;

        // max player capacity is determined from all criteria.
        maxCount = (uint16_t) mMaxFreePublicPlayerSlots;

        slotOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(FreePublicPlayerSlotsRuleDefinition::FREEPUBLICPLAYERSLOTSRULE_SLOT_RETE_KEY, minCount, maxCount, mRuleDefinition),
            0, this 
            ));

        return true;
    }

    bool FreePublicPlayerSlotsRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!matchmakingSupplementalData.mMatchmakingContext.canSearchJoinableGames())
        {
            return true;
        }

        if (!matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo())
        {
            // This rule is only required for FG MM, or GB with a Game Size Rule (CG already ensures space via the TotalPlayerSlotsRule)
            return true;
        }

        // we ensure max not over max possible based on other rules
        mMaxFreePublicPlayerSlots = getCriteria().calcMaxPossPlayerSlotsFromRules(criteriaData, matchmakingSupplementalData, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
        mMyGroupParticipantCount = (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount;

        return true;
    }

    void FreePublicPlayerSlotsRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        const Blaze::GameManager::PlayerRoster& gameRoster = *gameSession.getPlayerRoster();

        uint16_t totalPossiblePublicParticipants = gameRoster.getPlayerCount(SLOT_PUBLIC_PARTICIPANT) + mMyGroupParticipantCount;
        uint16_t totalPublicParticipantCapacity = gameSession.getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT);

        //Check count conditions that would make us fail.
        if (totalPossiblePublicParticipants > totalPublicParticipantCapacity)
        {
            debugRuleConditions.writeRuleCondition("%u public participants > %u allowed", totalPossiblePublicParticipants, totalPublicParticipantCapacity);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }
        
        debugRuleConditions.writeRuleCondition("%u public participants <= %u allowed", totalPossiblePublicParticipants, totalPublicParticipantCapacity);
        
        fitPercent = 0;
        isExactMatch = true;

    }


    void FreePublicPlayerSlotsRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // This rule is not evaluated on create game, no-op.
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
