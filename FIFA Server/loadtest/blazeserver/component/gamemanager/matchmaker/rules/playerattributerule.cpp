/*! ************************************************************************************************/
/*!
\file playerattributerule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/playerattributerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    PlayerAttributeRule::PlayerAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mRuleOmitted(false)
    {
    }

    PlayerAttributeRule::PlayerAttributeRule(const PlayerAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mRuleOmitted(otherRule.mRuleOmitted),
        mAttributeValuePlayerCountMap(otherRule.mAttributeValuePlayerCountMap)
    {
        if (!otherRule.isDisabled())
        {
            mPlayerAttributeRuleStatus.setRuleName(getRuleName()); // rule name
            mMatchmakingAsyncStatus->getPlayerAttributeRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mPlayerAttributeRuleStatus));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    PlayerAttributeRule::~PlayerAttributeRule()
    {
        mMatchmakingAsyncStatus->getPlayerAttributeRuleStatusMap().erase(getRuleName());
    }

    Rule* PlayerAttributeRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW PlayerAttributeRule(*this, matchmakingAsyncStatus);
    }

    bool PlayerAttributeRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {

        const PlayerAttributeRuleCriteria* playerAttributeRulePrefs = getPlayerAttributRulePrefs(criteriaData);

        mRuleOmitted = (playerAttributeRulePrefs == nullptr);

        if (!mRuleOmitted)
        {
            // NOTE: initMinFitThreshold should be called in the initialize method of any rule that uses threshold lists.
            const char8_t* listName = playerAttributeRulePrefs->getMinFitThresholdName();
            if (!initMinFitThreshold(listName, mRuleDefinition, err))
            {
                return false;
            }

            mPlayerAttributeRuleStatus.setRuleName(getRuleName()); // rule name
            mMatchmakingAsyncStatus->getPlayerAttributeRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mPlayerAttributeRuleStatus));



            // if the rule is not omitted, but is disabled, we still need to set up the attribute map for evaluations

            // PlayerAttributeMap is invalid during GameBrowser evaluations. It only does find evaluations
            // against games, so were only going to call evaluatePlayerAttribute against a game, which
            // does not require supplemental information.
            PerPlayerJoinDataList::const_iterator curIter = matchmakingSupplementalData.mPlayerJoinData.getPlayerDataList().begin();
            PerPlayerJoinDataList::const_iterator endIter = matchmakingSupplementalData.mPlayerJoinData.getPlayerDataList().end();
            for (; curIter != endIter; ++curIter)
            {                
                const Collections::AttributeMap *playerAttributeMap = &(*curIter)->getPlayerAttributes();
                if ((*curIter)->getUser().getBlazeId() == matchmakingSupplementalData.mPrimaryUserInfo->getUserInfo().getId())
                {
                    // For the host only-
                    // check that a player attribute exists for this rule to operate on (so other create game sessions can match against me)
                    Collections::AttributeMap::const_iterator iter = playerAttributeMap->find(getDefinition().getAttributeName());
                    if (EA_UNLIKELY(iter == playerAttributeMap->end()))
                    {
                        char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                        blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" : requires a playerAttribute named \"%s\" in MatchmakingParameters::mPlayerAttributeMap.",
                            getRuleName(), getDefinition().getAttributeName());
                        err.setErrMessage(msg);
                        return false;
                    }
                    mOwnerAttributeValue = iter->second;
                }

                if (playerAttributeMap->size() != 0)
                {
                    Collections::AttributeMap::const_iterator iter = playerAttributeMap->find(getDefinition().getAttributeName());
                    if (EA_UNLIKELY(iter == playerAttributeMap->end()))
                    {
                        // Non-leader player missing attribute. This is acceptable, but not preferred. May want to output a LOG here. 
                        continue;
                    }

                    // cache off the value and check that it's one of the possible values defined by this rule
                    const char8_t* attribValue = iter->second.c_str();
                    if (getDefinition().getAttributeRuleType() == EXPLICIT_TYPE)
                    {
                        int possActualValueIndex = getDefinition().getPossibleActualValueIndex(attribValue);
                        if (EA_UNLIKELY(possActualValueIndex == PlayerAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX))
                        {
                            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                            blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" : MatchmakingParameters::mPlayerAttributeMap[\"%s\"] has undefined value (\"%s\").",
                                getRuleName(), getDefinition().getAttributeName(), attribValue);
                            err.setErrMessage(msg);
                            return false;
                        }

                        ++mAttributeValuePlayerCountMap[(uint64_t)possActualValueIndex];
                    }
                    else if (getDefinition().getAttributeRuleType() == ARBITRARY_TYPE)
                    {
                        ++mAttributeValuePlayerCountMap[(uint64_t)getDefinition().getWMEAttributeValue(attribValue)];
                    }
                }
            }
        }

        // FG diagnostics require the bitset be cached up front
        updateCachedMatchedValues();

        return true;
    }

    const PlayerAttributeRuleCriteria* PlayerAttributeRule::getPlayerAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const
    {   
        PlayerAttributeRuleCriteriaMap::const_iterator iter = criteriaData.getPlayerAttributeRuleCriteriaMap().find(getRuleName());

        if (iter != criteriaData.getPlayerAttributeRuleCriteriaMap().end())
        {
            return iter->second;
        }

        return nullptr;
    }

    float PlayerAttributeRule::getFitPercent(const AttributeValuePlayerCountMap &playerAttrValues, bool& outExactMatch) const
    {
        // Exact match has the same size attribute map:
        outExactMatch = mAttributeValuePlayerCountMap.size() == playerAttrValues.size();

        // compare this rule's desired values against the other entity's values
        // return the best fit, or FIT_PERCENT_NO_MATCH if no fits exist
        float maxFitPercent = -1;   // FIT_PERCENT_NO_MATCH
        float minFitPercent = 1000; // Any value > 1 should work here
        float totalValues = 0;
        float totalFitPercent = 0;

        AttributeValuePlayerCountMap::const_iterator myCurIter = mAttributeValuePlayerCountMap.begin();
        AttributeValuePlayerCountMap::const_iterator myEndIter = mAttributeValuePlayerCountMap.end();
        for (; myCurIter != myEndIter; ++myCurIter)
        {
            uint64_t myValue = myCurIter->first;
            int myCount = myCurIter->second;
            outExactMatch = outExactMatch && (playerAttrValues.find(myValue) != playerAttrValues.end());  // Exact match has the same attributes set (counts don't matter, 1 player with attr[A]==FOO == 12 players with attr[A]==FOO)

            AttributeValuePlayerCountMap::const_iterator curIter = playerAttrValues.begin();
            AttributeValuePlayerCountMap::const_iterator endIter = playerAttrValues.end();
            for (; curIter != endIter; ++curIter)
            {
                uint64_t otherValue = curIter->first;
                int otherCount = curIter->second;

                switch (getDefinition().getGroupValueFormula())
                {
                case GROUP_VALUE_FORMULA_MAX:
                {
                    maxFitPercent = eastl::max_alt(maxFitPercent, getDefinition().getFitPercent(myValue, otherValue));
                    break;
                }
                case GROUP_VALUE_FORMULA_MIN:
                {
                    minFitPercent = eastl::min_alt(minFitPercent, getDefinition().getFitPercent(myValue, otherValue));
                    break;
                }
                case GROUP_VALUE_FORMULA_AVERAGE:
                {
                    float curCount = (float)myCount * otherCount;    // This is the number of player combinations that match this set:
                    totalFitPercent += curCount * getDefinition().getFitPercent(myValue, otherValue);
                    totalValues += curCount;
                    break;
                }
                default:
                    ERR_LOG("[ExplicitAttributeRule].getFitPercent. Invalid GroupValueFormula (" << 
                            GroupValueFormulaToString(getDefinition().getGroupValueFormula()) << ") specified for rule. Only AVERAGE, MIN, and MAX are supported.");
                    break;
                }
            }
        }


        switch (getDefinition().getGroupValueFormula())
        {
        case GROUP_VALUE_FORMULA_MAX:
        {
            return maxFitPercent;
        }
        case GROUP_VALUE_FORMULA_MIN:
        {
            if (minFitPercent >= 1000)
                return FIT_PERCENT_NO_MATCH;

            return minFitPercent;
        }
        case GROUP_VALUE_FORMULA_AVERAGE:
        {
            if (totalValues == 0)
                return FIT_PERCENT_NO_MATCH;

            return totalFitPercent / totalValues;
        }
        default:
            break;
        }
        return FIT_PERCENT_NO_MATCH;
    }


    bool PlayerAttributeRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        if (isMatchAny())
        {
            return false;
        }

        // Do a simple pass/fail RETE evaluation to help manage the post-rete calcs needed.
        return true;
    }

    void PlayerAttributeRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const PlayerAttributeRuleDefinition& ruleDef = static_cast<const PlayerAttributeRuleDefinition&>(mRuleDefinition);
        const AttributeValuePlayerCountMap *cachedPlayerAttribValues = gameSession.getMatchmakingGameInfoCache()->getCachedPlayerAttributeValues(ruleDef);
        if (cachedPlayerAttribValues == nullptr)
        {
            debugRuleConditions.writeRuleCondition("%s attribute not found in game.", ruleDef.getAttributeName());

            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            return;
        }

        debugRuleConditions.writeRuleCondition("%s attribute found in game.", ruleDef.getAttributeName());
        fitPercent = getFitPercent(*cachedPlayerAttribValues, isExactMatch);
    }

    void PlayerAttributeRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const
    {
        const PlayerAttributeRule& attribRule = static_cast<const PlayerAttributeRule&>(otherRule);
        fitPercent = getFitPercent(attribRule.mAttributeValuePlayerCountMap, isExactMatch);
    }


    /*! ************************************************************************************************/
    /*! \brief from SimpleRule. Overridden for AttributeRule-specific behavior.
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void PlayerAttributeRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const PlayerAttributeRule* otherRule = static_cast<const PlayerAttributeRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));

        // prevent crash if for some reason other rule is not defined
        if (otherRule == nullptr)
        {
            ERR_LOG("[PlayerAttributeRule].evaluateForMMCreateGame Rule '" << getRuleName() << "' MM session '" 
                    << getMMSessionId() << "': Unable to find other rule by id '" << getId() << "' for MM session '" 
                    << otherSession.getMMSessionId() << "'");
            return;
        }
        // 2.11x/3.03 specs (is same for all AttributeRule rules):
        // omitter matchable by enablers: always no
        // omitter matchable by disabler: always yes
        if (otherRule->mRuleOmitted && !isDisabled())
        {
            // reject: other pool doesn't define rule for this attrib (same as if a game didn't define the attrib)
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("rule not instantiated (used) by other session");
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();
            return;
        }
        else if(mRuleOmitted && !otherRule->isDisabled())
        {
            // reject: I didn't define this rule, and the other guy isn't disabled.
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("rule not instantiated (used) by my session");
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();
            return;
        }

        if (handleDisabledRuleEvaluation(*otherRule, sessionsMatchInfo, getMaxFitScore()))
        { 
            //ensure a session with a rule disabled doesn't pull in a session with it enabled, but allow them to match.
            enforceRestrictiveOneWayMatchRequirements(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
            otherRule->enforceRestrictiveOneWayMatchRequirements(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
            return;
        }


        // don't eval the disabled rule(s) because they won't have a desired value set and will never match
        if (!isDisabled())
        {
            evaluateRule(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
        }

        if (!otherRule->isDisabled())
        {
            otherRule->evaluateRule(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
        }

        fixupMatchTimesForRequirements(sessionsEvalInfo, otherRule->mMinFitThresholdList, sessionsMatchInfo, isBidirectional());

        postEval(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
        otherRule->postEval(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief local helper
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void PlayerAttributeRule::enforceRestrictiveOneWayMatchRequirements(const PlayerAttributeRule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        // Ensure a session with this rule disabled can't pull in a session with it enabled
        // This check is not executed when evaluating bidirectionally.
        if (isDisabled() && (!otherRule.isDisabled()))
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("Session (%" PRIu64 ") with rule disabled can't add session (%" PRIu64 ") with rule enabled.", getMMSessionId(), otherRule.getMMSessionId());
            matchInfo.setNoMatch();
            return;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    ***************************************************************************************************/
    void PlayerAttributeRule::updateAsyncStatus()
    {
        updateCachedMatchedValues();
    }

    /*! ************************************************************************************************/
    /*! \brief update cached matchmaking matched bitset and async status
    ***************************************************************************************************/
    void PlayerAttributeRule::updateCachedMatchedValues()
    {
        if (getDefinition().getAttributeRuleType() == ARBITRARY_TYPE)
        {
            // no-op, desired async values don't change
            //  MM_TODO: in theory, we should have a way to indicate if we match ThisValue or match !ThisValue
            return;
        }

        // clear the old values
        if (MinFitThresholdList::isThresholdExactMatchRequired(mCurrentMinFitThreshold))
        {
            AttributeValuePlayerCountMap::const_iterator myCurIter = mAttributeValuePlayerCountMap.begin();
            AttributeValuePlayerCountMap::const_iterator myEndIter = mAttributeValuePlayerCountMap.end();
            for (; myCurIter != myEndIter; ++myCurIter)
            {
                if (!mMatchedValuesBitset[myCurIter->first])
                {
                    const char8_t* possibleValue = getDefinition().getPossibleValue(myCurIter->first);
                    mPlayerAttributeRuleStatus.getMatchedValues().push_back(possibleValue);
                    mMatchedValuesBitset[myCurIter->first] = true;
                }
            }
        }
        else
        {
            // get all matched values at this point
            size_t possibleValueCount = getDefinition().getPossibleValueCount();
            for (size_t possibleValueIndex = 0; possibleValueIndex < possibleValueCount; possibleValueIndex++)
            {
                if (!mMatchedValuesBitset[possibleValueIndex])
                {
                    // Here we only check against 1 player's attribute, as opposed to the whole group's
                    AttributeValuePlayerCountMap possibleValueMap;
                    possibleValueMap[possibleValueIndex] = 1;

                    bool exactMatch = false;
                    const char8_t* possibleValue = getDefinition().getPossibleValue(possibleValueIndex);
                    float fitscore = getFitPercent(possibleValueMap, exactMatch);
                    if (fitscore >= mCurrentMinFitThreshold)
                    {
                        mMatchedValuesBitset[possibleValueIndex] = true;
                        mPlayerAttributeRuleStatus.getMatchedValues().push_back(possibleValue);
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics
    ****************************************************************************************************/
    void PlayerAttributeRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        const char8_t* categoryTag = (getDefinition().getAttributeRuleType() == ARBITRARY_TYPE ? "desiredArbPlayerAttr" : "desiredPlayerAttrs");
        RuleDiagnosticSetupInfo setupInfo(categoryTag);
        getDiagnosticDisplayString(setupInfo.mValueTag);

        setupInfos.push_back(setupInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t PlayerAttributeRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (isMatchAny())
        {
            return helper.getOpenToMatchmakingCount();
        }
        
        // player attribute rules just count for the owner's request attributes.
        // Whether explicit or arbitrary, will be uint64:
        uint64_t intAttrVal = (getDefinition().getAttributeRuleType() == EXPLICIT_TYPE) ?
            getDefinition().getPossibleValueIndex(mOwnerAttributeValue) :
            getDefinition().getWMEAttributeValue(mOwnerAttributeValue);

        return helper.getPlayerAttributeGameCount(getDefinition().getName(), intAttrVal);
    }

    void PlayerAttributeRule::getDiagnosticDisplayString(eastl::string& desiredValuesTag) const
    {
        // For FG, tricky if there's multiple desired values, just count the owner's attribute
        desiredValuesTag = mOwnerAttributeValue;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
