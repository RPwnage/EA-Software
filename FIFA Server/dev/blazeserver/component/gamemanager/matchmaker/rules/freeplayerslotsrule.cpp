/*! ************************************************************************************************/
/*!
    \file   FreePlayerSlotsRule

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/freeplayerslotsrule.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    FreePlayerSlotsRule::FreePlayerSlotsRule(const FreePlayerSlotsRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mMinFreeParticipantSlots(0),
        mMaxFreeParticipantSlots(INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE)
    {}

    FreePlayerSlotsRule::FreePlayerSlotsRule(const FreePlayerSlotsRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mMinFreeParticipantSlots(otherRule.mMinFreeParticipantSlots),
        mMaxFreeParticipantSlots(otherRule.mMaxFreeParticipantSlots)
    {}

    bool FreePlayerSlotsRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        // This rule only applies to Game Browser Searches
        if (!matchmakingSupplementalData.mMatchmakingContext.canSearchJoinableGames())
        {
            return true;
        }

        // if not browsing, account for group size
        uint16_t joiningPlayerCount = (matchmakingSupplementalData.mMatchmakingContext.hasPlayerJoinInfo()) ? (uint16_t)matchmakingSupplementalData.mJoiningPlayerCount : 0;
       
        // min is requested min plus however many i bring with me
        mMinFreeParticipantSlots = criteriaData.getFreePlayerSlotsRuleCriteria().getMinFreePlayerSlots() + joiningPlayerCount;

        // max is either disabled (the default) or the requested value (capped by global max)
        uint16_t requestedMaxFreeParticipantSlots = criteriaData.getFreePlayerSlotsRuleCriteria().getMaxFreePlayerSlots();
        if((requestedMaxFreeParticipantSlots != INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE) 
            && (requestedMaxFreeParticipantSlots <= getDefinition().getGlobalMaxTotalPlayerSlotsInGame()))
        {
            // account on the top end for the joining group
            requestedMaxFreeParticipantSlots = requestedMaxFreeParticipantSlots + joiningPlayerCount;
            // cap at global max
            mMaxFreeParticipantSlots = (requestedMaxFreeParticipantSlots > getDefinition().getGlobalMaxTotalPlayerSlotsInGame()) ? getDefinition().getGlobalMaxTotalPlayerSlotsInGame() : requestedMaxFreeParticipantSlots;
        }
        else
        {
            // either we've disabled the rule, or the requester has gone out of bounds on the max free slots requested, don't touch it so we can error out in the validation below.
            mMaxFreeParticipantSlots = requestedMaxFreeParticipantSlots;
        }

        TRACE_LOG("[FreePlayerSlotsRule].initialize: min(" << mMinFreeParticipantSlots << "), max(" << mMaxFreeParticipantSlots << ")"); 
        
        // max must be no greater than global max
        if((mMaxFreeParticipantSlots > getDefinition().getGlobalMaxTotalPlayerSlotsInGame()) &&
            (mMaxFreeParticipantSlots != INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE))
        {
            char8_t buf[256];
            blaze_snzprintf(buf, sizeof(buf),
                "ERROR: FreePlayerSlotsRule.initialize() MaxFreePlayerSlots '%u' is greater than configured maximum game size '%u'",
                mMaxFreeParticipantSlots, getDefinition().getGlobalMaxTotalPlayerSlotsInGame());
            err.setErrMessage(buf);
            return false;
        }

        if (mMaxFreeParticipantSlots < mMinFreeParticipantSlots ) 
        {                    
            err.setErrMessage("ERROR: FreePlayerSlotsRule.initialize() Bad input: MaxFreePlayerSlots < MinFreePlayerSlots");
            return false;
        }

        if(isDisabled())
        {
            TRACE_LOG("[FreePlayerSlotsRule] disabled, as max size was set to default of " << INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE); 
        }

        return true;
    }


    bool FreePlayerSlotsRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if(!isDisabled())
        {
            // we don't eval this rule if the mMaxFreeParticipantSlots is the default value of INVALID_FREE_PLAYER_SLOTS_RULE_MAX_SIZE for memory reasons.

            Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
            Rete::OrConditionList& capacityOrConditions = conditions.push_back();

            uint16_t minCount = (uint16_t) mMinFreeParticipantSlots;

            uint16_t maxCount = (uint16_t) mMaxFreeParticipantSlots;

            capacityOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(FreePlayerSlotsRuleDefinition::FREESLOTSRULE_SIZE_RETE_KEY, minCount, maxCount, mRuleDefinition),
                0, this
                ));

            return true;
        }

        return false;
    }


    void FreePlayerSlotsRule::calcFitPercent(const Search::GameSessionSearchSlave& game, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // Nothing needed here as the rule should be fully evaluated through RETE.
        fitPercent = 0.0;
        isExactMatch = false;
    }


    void FreePlayerSlotsRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        fitPercent = 0.0;
        isExactMatch = false;
    }


    Rule* FreePlayerSlotsRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW FreePlayerSlotsRule(*this, matchmakingAsyncStatus);
    }

    void FreePlayerSlotsRule::updateAsyncStatus()
    {
        // NO_OP. rule is a filter w/o decay
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
