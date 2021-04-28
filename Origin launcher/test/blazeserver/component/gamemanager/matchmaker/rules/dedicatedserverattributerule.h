/*! ************************************************************************************************/
/*!
\file dedicatedserverattributerule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULE_H
#define BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributeruledefinition.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            class DedicatedServerAttributeRule : public SimpleRule
            {
                NON_COPYABLE(DedicatedServerAttributeRule);
            public:

                DedicatedServerAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
                DedicatedServerAttributeRule(const DedicatedServerAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
                virtual ~DedicatedServerAttributeRule();

                /*! ************************************************************************************************/
                /*!
                \brief compare each desired value against the supplied actual value, returning the highest
                fitTable entry (the highest fitPercent).  If the entity's value is invalid/unknown, return -1.

                This is a N-to-one comparison (many desired values vs a single actual value).

                \param[in]  actualValueIndex the possibleValue index representing another entity's actual value.
                \return the highest fitPercent (0..1), or < 0 if the entity's value is invalid/unknown.
                *************************************************************************************************/
                float getFitPercent(int actualValueIndex) const
                {
                    if (actualValueIndex < 0 || mDesireValueIndex < 0)
                        return (float)DedicatedServerAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX;

                    return getDefinition().getFitPercent(mDesireValueIndex, actualValueIndex);;
                }
                                
                // Rete
                bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

                // Rule Interface
                bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
                Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;
                virtual const DedicatedServerAttributeRuleDefinition& getDefinition() const { return static_cast<const DedicatedServerAttributeRuleDefinition&>(mRuleDefinition); }

                void updateAsyncStatus() override {}
                bool isDisabled() const override { return mRuleOmitted; }
                bool isMatchAny() const override { return (Rule::isMatchAny()); }

                void buildDesiredValueListString(eastl::string& buffer, const int &desiredValueIndex) const;

                const DedicatedServerAttributeRuleCriteria* getDedicatedServerAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const;

                bool isExplicitType() const { return getDefinition().getAttributeRuleType() == EXPLICIT_TYPE; }
                bool isArbitraryType() const { return getDefinition().getAttributeRuleType() == ARBITRARY_TYPE; }

                bool isDedicatedServerEnabled() const override
                {
                    return true;
                }

            private:
                
                /*! ************************************************************************************************/
                /*!
                \brief Init this rule's desired rule value index based on the desire value.

                \param[in]  desiredValue   the desired value for this rule.
                \param[out] err the errMessage field is filled out if there's a problem
                \return true on success, false on error (see the errMessage field of err)
                *************************************************************************************************/
                bool initDesiredValue(const Collections::AttributeValue& desiredValue, MatchmakingCriteriaError &err);

                // Used for addConditions
                void calcFitPercentInternal(int gameAttribValueIndex, float &fitPercent) const;

                // Members
            private:
                bool mRuleOmitted;

                int mDesireValueIndex;
                float mCurMinFitThresholdValue;
            public:
                void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
            };

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULE_H
