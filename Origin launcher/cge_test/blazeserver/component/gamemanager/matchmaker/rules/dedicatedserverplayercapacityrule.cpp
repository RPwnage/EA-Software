/*! ************************************************************************************************/
/*!
    \file   dedicatedserverplayercapacityrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityrule.h"
#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerrostermaster.h" 

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DedicatedServerPlayerCapacityRule::DedicatedServerPlayerCapacityRule(const DedicatedServerPlayerCapacityRuleDefinition &definition, MatchmakingAsyncStatus* status)
    : SimpleRule(definition, status),
        mDesiredCapacity(BLAZE_DEFAULT_MAX_PLAYER_CAP)
    {
    }

    DedicatedServerPlayerCapacityRule::DedicatedServerPlayerCapacityRule(const DedicatedServerPlayerCapacityRule &other, MatchmakingAsyncStatus* status) 
        : SimpleRule(other, status),
        mDesiredCapacity(other.mDesiredCapacity)
    {
    }

    bool DedicatedServerPlayerCapacityRule::isDisabled() const
    {
        return (mDesiredCapacity == BLAZE_DEFAULT_MAX_PLAYER_CAP);
    }

    bool DedicatedServerPlayerCapacityRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        const DedicatedServerPlayerCapacityRuleDefinition &myDefn = getDefinition();

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& minCapacityConditions = conditions.push_back();

        // Note that FIT is still calculated post-rete due to needing to know the games actual max value.

        minCapacityConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(DedicatedServerPlayerCapacityRuleDefinition::MIN_CAPACITY_RETE_KEY, 0, mDesiredCapacity, mRuleDefinition),
            0, this
            ));

        Rete::OrConditionList& maxCapacityConditions = conditions.push_back();

        maxCapacityConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(DedicatedServerPlayerCapacityRuleDefinition::MAX_CAPACITY_RETE_KEY, mDesiredCapacity, myDefn.getMaxTotalPlayerSlots(), mRuleDefinition),
            0, this
            ));

        return true;
    }

    bool DedicatedServerPlayerCapacityRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Player capacity only needed when doing resettable games
            return true;
        }

        mDesiredCapacity = matchmakingSupplementalData.mMaxPlayerCapacity;

        return true;
    }

    void DedicatedServerPlayerCapacityRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // Here we require capacity to match what we accept, and 
        uint16_t minCap = gameSession.getMinPlayerCapacity();
        uint16_t maxCap = gameSession.getMaxPlayerCapacity();
        if (mDesiredCapacity < minCap || mDesiredCapacity > maxCap)
        {
            debugRuleConditions.writeRuleCondition("%u desired capacity outside (%u, %u) range", mDesiredCapacity, minCap, maxCap);

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("%u desired capacity within (%u, %u) range", mDesiredCapacity, minCap, maxCap);

            // Linear fit: 0 is best value, (max - min) is worst
            if (maxCap == 0)
                fitPercent = 1.0f;
            else
                fitPercent = (float)mDesiredCapacity / (float)maxCap;

            isExactMatch = (maxCap == mDesiredCapacity);
        }
    }

    void DedicatedServerPlayerCapacityRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // This code should never hit (rule is not used for rule-to-rule create game checks). 
        isExactMatch = false;
        fitPercent = 0;
    }

    // Override this for games
    FitScore DedicatedServerPlayerCapacityRule::calcWeightedFitScore(bool isExactMatch, float fitPercent) const
    {
        return FitScore(getDefinition().getWeight() * fitPercent);
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
