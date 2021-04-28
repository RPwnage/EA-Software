/*! ************************************************************************************************/
/*!
    \file   virtualgamerule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/virtualgamerule.h"
#include "gamemanager/matchmaker/rules/virtualgameruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    VirtualGameRule::VirtualGameRule(const VirtualGameRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mDesiredValue(VirtualGameRulePrefs::ABSTAIN)
    {}


    VirtualGameRule::VirtualGameRule(const VirtualGameRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus), 
        mDesiredValue(other.getDesiredValue())
    {}


    /*! ************************************************************************************************/
    /*!
        \brief initialize a virtualGameRule instance for use in a matchmaking session. Return true on success.
    
        \param[in]  criteriaData - Criteria data used to initialize the rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    ***************************************************************************************************/
    bool VirtualGameRule::initialize(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        const VirtualGameRulePrefs& virtualGameRulePrefs = criteriaData.getVirtualGameRulePrefs();

        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        // init the desired value
        mDesiredValue = virtualGameRulePrefs.getDesiredVirtualGameValue();

        // lookup the minFitThreshold list we want to use
        const char8_t* listName = virtualGameRulePrefs.getMinFitThresholdName();

        const bool minFitIni = Rule::initMinFitThreshold( listName, mRuleDefinition, err );
        if (minFitIni)
        {
            updateAsyncStatus();
        }

        return minFitIni;
    }


    bool VirtualGameRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        // Desired Any evaluation does not need to be part of the RETE tree, since no matter what value comes in
        // for a game, we wish to match it.
        if (isMatchAny())
        {
            // abstain, don't need to evaluate further.
            return false;
        }
        // set these below
        bool isExactMatch = false;
        float fitPercent = -1;
        FitScore fitScore;

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& orConditions = conditions.push_back();

        // For each possible value, determine the best fitscore against all of the desired values.
        // If the fitscore is a match at the current threshold then add it to the search conditions, else skip it.
        // NOTE: this does not include the random or abstain values.
        VirtualGameRule::calcFitPercentInternal(VirtualGameRulePrefs::VIRTUALIZED, fitPercent, isExactMatch);
        fitScore = calcWeightedFitScore(isExactMatch, fitPercent);
        if (isFitScoreMatch(fitScore))
        {
            const char8_t* value = VirtualGameRulePrefs::VirtualGameDesiredValueToString(VirtualGameRulePrefs::VIRTUALIZED);
            orConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(VirtualGameRuleDefinition::VIRTUALRULE_RETE_KEY, mRuleDefinition.getWMEAttributeValue(value), mRuleDefinition), 
                fitScore, this));
        }

        calcFitPercentInternal(VirtualGameRulePrefs::STANDARD, fitPercent, isExactMatch);
        fitScore = calcWeightedFitScore(isExactMatch, fitPercent);
        if (isFitScoreMatch(fitScore))
        {
            const char8_t* value = VirtualGameRulePrefs::VirtualGameDesiredValueToString(VirtualGameRulePrefs::STANDARD);
            orConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(VirtualGameRuleDefinition::VIRTUALRULE_RETE_KEY, mRuleDefinition.getWMEAttributeValue(value), mRuleDefinition), 
                fitScore, this));
        }
        return true;
    }

    // Local helper
    void VirtualGameRule::calcFitPercentInternal(VirtualGameRulePrefs::VirtualGameDesiredValue otherValue, float &fitPercent, bool &isExactMatch) const
    {
        if ( (getDesiredValue() == otherValue)
            || (otherValue == VirtualGameRulePrefs::ABSTAIN)
            || (getDesiredValue() ==VirtualGameRulePrefs::ABSTAIN))
        {
            isExactMatch = true;
            fitPercent = VirtualGameRule::getDefinition().getMatchingFitPercent();
        }
        else
        {
            isExactMatch = false;
            fitPercent = VirtualGameRule::getDefinition().getMismatchFitPercent();
        }
    }


    void VirtualGameRule::calcFitPercent(const Search::GameSessionSearchSlave &gameSession, float &fitPercent, bool &isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        VirtualGameRulePrefs::VirtualGameDesiredValue otherValue = gameSession.getGameSettings().getVirtualized()?
            VirtualGameRulePrefs::VIRTUALIZED : VirtualGameRulePrefs::STANDARD;

        calcFitPercentInternal(otherValue, fitPercent, isExactMatch);

        
        debugRuleConditions.writeRuleCondition("%s %s %s", VirtualGameRulePrefs::VirtualGameDesiredValueToString(mDesiredValue), isExactMatch?"==":"!=", VirtualGameRulePrefs::VirtualGameDesiredValueToString(otherValue));
        
    }


    void VirtualGameRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const VirtualGameRule &otherVirtualGameRule = static_cast<const VirtualGameRule&>(otherRule);
        

        calcFitPercentInternal(otherVirtualGameRule.getDesiredValue(), fitPercent, isExactMatch);

        // This prevents sessions in CG matchmaking from finalizing while requiring VIRTUALIZED.
        if (getDesiredValue() == VirtualGameRulePrefs::VIRTUALIZED)
        {
            float stdFitPercent = 0.0f;
            bool stdIsExactMatch = false;
            //  Calculate against STANDARD to get our mismatch fit percent
            calcFitPercentInternal(VirtualGameRulePrefs::STANDARD, stdFitPercent, stdIsExactMatch);
            // Check against our threshold decay.
            uint32_t thresholdListSize = mMinFitThresholdList->getSize();
            for (uint32_t i = mMinFitThresholdList->isExactMatchRequired() ? 1 : 0; i < thresholdListSize; ++i)
            {
                float minFitThreshold = mMinFitThresholdList->getMinFitThresholdByIndex(i);

                // If our mismatch is in an acceptable value given our decay, just return.
                if ( stdFitPercent >= minFitThreshold )
                {
                    debugRuleConditions.writeRuleCondition("%s %s %s", VirtualGameRulePrefs::VirtualGameDesiredValueToString(mDesiredValue), isExactMatch?"==":"!=", VirtualGameRulePrefs::VirtualGameDesiredValueToString(otherVirtualGameRule.getDesiredValue()));
        
                    return;
                }
            }

            // Otherwise no match the session.
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("I require %s", VirtualGameRulePrefs::VirtualGameDesiredValueToString(mDesiredValue));

            return;
        }

        debugRuleConditions.writeRuleCondition("%s %s %s", VirtualGameRulePrefs::VirtualGameDesiredValueToString(mDesiredValue), isExactMatch?"==":"!=", VirtualGameRulePrefs::VirtualGameDesiredValueToString(otherVirtualGameRule.getDesiredValue()));
        
    }


    Rule* VirtualGameRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW VirtualGameRule(*this, matchmakingAsyncStatus);
    }


    void VirtualGameRule::updateAsyncStatus()
    {
        uint8_t matchedVirtualizedFlags = 0;

        if (isDisabled())
        {
            // rule is disabled, we'll match anything/anyone
            matchedVirtualizedFlags = (VirtualGameRulePrefs::VIRTUALIZED | VirtualGameRulePrefs::STANDARD);
        }
        else
        {
            // Note: this is a slightly inefficient way to determine what we match. However, this function
            //  is called infrequently and an efficient impl would require duplicated logic between eval & getStatus.
            //  I think the minor inefficiency is well worth the ease of maintenance.

            float fitPercent;
            bool isExactMatch;

            calcFitPercentInternal(VirtualGameRulePrefs::VIRTUALIZED, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedVirtualizedFlags |= VirtualGameRulePrefs::VIRTUALIZED;

            calcFitPercentInternal(VirtualGameRulePrefs::STANDARD, fitPercent, isExactMatch);
            if ( isFitScoreMatch(calcWeightedFitScore(isExactMatch, fitPercent)) )
                matchedVirtualizedFlags |= VirtualGameRulePrefs::STANDARD;

        }

        Rule::mMatchmakingAsyncStatus->getVirtualGameRuleStatus().setMatchedVirtualizedFlags(matchedVirtualizedFlags);
    }


    bool VirtualGameRule::isDisabled() const
    {
        if(getDesiredValue() == VirtualGameRulePrefs::ABSTAIN)
        {
            return true;
        }
        return SimpleRule::isDisabled();
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

