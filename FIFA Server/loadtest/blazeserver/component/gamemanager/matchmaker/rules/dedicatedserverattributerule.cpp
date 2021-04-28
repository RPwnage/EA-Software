/*! ************************************************************************************************/
/*!
\file DedicatedServerAttributeRule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributerule.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            /*! ************************************************************************************************/
            /*! \brief ctor
            ***************************************************************************************************/
            DedicatedServerAttributeRule::DedicatedServerAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
                : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
                mRuleOmitted(false),
                mDesireValueIndex(-1),
                mCurMinFitThresholdValue(0.0f)
            {
            }

            DedicatedServerAttributeRule::DedicatedServerAttributeRule(const DedicatedServerAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
                : SimpleRule(otherRule, matchmakingAsyncStatus),
                mRuleOmitted(otherRule.mRuleOmitted),
                mDesireValueIndex(otherRule.mDesireValueIndex),
                mCurMinFitThresholdValue(otherRule.mCurMinFitThresholdValue)
            {
            }

            /*! ************************************************************************************************/
            /*! \brief destructor
            ***************************************************************************************************/
            DedicatedServerAttributeRule::~DedicatedServerAttributeRule()
            {
            }

            bool DedicatedServerAttributeRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
            {
                const DedicatedServerAttributeRuleCriteria* dsAttrRuleCriteria = getDedicatedServerAttributRulePrefs(criteriaData);

                // This rule can only be activate if from a dedicated server search or a GB search, not FG MM or CG MM
                mRuleOmitted = (!matchmakingSupplementalData.mMatchmakingContext.canSearchResettableGames() 
                    || (dsAttrRuleCriteria == nullptr && matchmakingSupplementalData.mNetworkTopology != CLIENT_SERVER_DEDICATED));

                if (!mRuleOmitted)
                {
                    if (isExplicitType() && !isDisabled())
                    {
                        if (dsAttrRuleCriteria == nullptr)
                        {
                            if (!initDesiredValue(getDefinition().getDefaultValue(), err))
                            {
                                return false; // err already filled out
                            }
                        }
                        else if (*(dsAttrRuleCriteria->getDesiredValue()) != '\0')
                        {
                            // rule is not disabled, or is present and has set desired value
                            // init the desired value bitset
                            if (!initDesiredValue(dsAttrRuleCriteria->getDesiredValue(), err))
                            {
                                return false; // err already filled out                                
                            }
                        }                            
                    }
                }

                float threhold = (dsAttrRuleCriteria != nullptr) ? dsAttrRuleCriteria->getMinFitThresholdValue() : getDefinition().getMinFitThresholdValue();
                if ((threhold >= 0.0f) && (threhold <= 1.0f))
                {
                    mCurMinFitThresholdValue = threhold;
                }
                else
                {
                    TRACE_LOG("[DedicatedServerAttributeRule].initialize: has invalid threshold (" << threhold << "); should be between 0..1 (inclusive). Use default threshold.");
                    mCurMinFitThresholdValue = getDefinition().getMinFitThresholdValue();
                }

                return true;
            }

            const DedicatedServerAttributeRuleCriteria* DedicatedServerAttributeRule::getDedicatedServerAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const
            {
                DedicatedServerAttributeRuleCriteriaMap::const_iterator iter = criteriaData.getDedicatedServerAttributeRuleCriteriaMap().find(getRuleName());

                if (iter != criteriaData.getDedicatedServerAttributeRuleCriteriaMap().end())
                {
                    return iter->second;
                }

                return nullptr;
            }

            Rule* DedicatedServerAttributeRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
            {
                return BLAZE_NEW DedicatedServerAttributeRule(*this, matchmakingAsyncStatus);
            }
            
            bool DedicatedServerAttributeRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
            {
                float fitPercent;

                Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
                Rete::OrConditionList& orConditions = conditions.push_back();
                const char8_t* name = getDefinition().getAttributeName();

                FitScore fitScore;

                if (isExplicitType())
                {
                    // For each possible value, determine the best fitscore against all of the desired values.
                    // If the fitscore is a match at the current threshold then add it to the search conditions, else skip it.
                    // NOTE: this does not include the random or abstain values.

                    for (uint32_t i = 0; i < getDefinition().getPossibleValueCount(); ++i)
                    {
                        const Collections::AttributeValue& possibleValue = getDefinition().getPossibleValue(i);
                        int possibleValueIndex = getDefinition().getPossibleValueIndex(possibleValue);

                        calcFitPercentInternal(possibleValueIndex, fitPercent);

                        if (fitPercent < mCurMinFitThresholdValue)
                        {
                            // log threshold fail message
                            fitScore = FIT_SCORE_NO_MATCH;
                        }
                        else
                        {
                            fitScore = static_cast<FitScore>(fitPercent * getDefinition().getWeight());
                        }                        

                        if (isFitScoreMatch(fitScore))
                        {
                            orConditions.push_back(Rete::ConditionInfo(
                                Rete::Condition(name, getDefinition().getWMEAttributeValue(possibleValue), mRuleDefinition),
                                fitScore, this));
                        }
                    }

                    if (orConditions.empty())
                    {
                        eastl::string myDesiredValue;
                        buildDesiredValueListString(myDesiredValue, mDesireValueIndex);
                        WARN_LOG("[DedicatedServerAttributeRule].addConditions: possible badly configured fit table or min fit threshold lists for rule(" << getRuleName() << "), the requested search on desired value (" << myDesiredValue.c_str() << ") cannot currently match any of rules possible values.");

                        // empty orConditions by default would match any, for correctness/CG-consistency ensure
                        // won't match anything by specifying impossible-to-match value
                        orConditions.push_back(Rete::ConditionInfo(
                            Rete::Condition(name, getDefinition().getWMEAttributeValue(DedicatedServerAttributeRuleDefinition::ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE), mRuleDefinition),
                            0, this));
                    }
                    return true;
                }

                // This should be unreachable
                return false;
            }
            
            void DedicatedServerAttributeRule::calcFitPercentInternal(int DedicatedServerAttribValueIndex, float &fitPercent) const
            {
                fitPercent = getFitPercent(DedicatedServerAttribValueIndex);
            }
            
            void DedicatedServerAttributeRule::buildDesiredValueListString(eastl::string& buffer, const int &desiredValueIndex) const
            {
                if (desiredValueIndex > 0)
                    buffer.append_sprintf(getDefinition().getPossibleValue(desiredValueIndex));
            }

            bool DedicatedServerAttributeRule::initDesiredValue(const Collections::AttributeValue& desiredValue, MatchmakingCriteriaError &err)
            {
                // you must specify 1 desired value
                if (desiredValue.empty())
                {
                    // error: each rule must specify at least 1 desired value
                    char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" must define 1 desired value.",
                        getDefinition().getName());
                    err.setErrMessage(msg);
                    return false;
                }

                mDesireValueIndex = getDefinition().getPossibleValueIndex(desiredValue);
                if (mDesireValueIndex < 0)
                {
                    // error: desired value not found in rule defn's possible value list.
                    char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" contains an invalid desired value (\"%s\") - values must match a rule's possibleValue (in rule definition).",
                        getDefinition().getName(), desiredValue.c_str());
                    err.setErrMessage(msg);
                    return false;
                }
                return true; // no error
            }
             
            /*! ************************************************************************************************/
            /*! \brief Implemented to enable per desired value break down metrics
            ****************************************************************************************************/
            void DedicatedServerAttributeRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
            {
                if (isExplicitType())
                {
                    RuleDiagnosticSetupInfo setupInfo("desiredDedicatedServerAttrs");
                    buildDesiredValueListString(setupInfo.mValueTag, mDesireValueIndex);
                    setupInfos.push_back(setupInfo);
                }
            }

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
