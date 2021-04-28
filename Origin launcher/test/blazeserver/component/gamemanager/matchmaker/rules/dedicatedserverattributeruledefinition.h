/*! ************************************************************************************************/
/*!
\file dedicatedserverattributeruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rules/rule.h"

#include "gamemanager/matchmaker/fittable.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            class DedicatedServerAttributeRuleDefinition : public RuleDefinition
            {
                NON_COPYABLE(DedicatedServerAttributeRuleDefinition);
                DEFINE_RULE_DEF_H(DedicatedServerAttributeRuleDefinition, DedicatedServerAttributeRule);
            public:
                static const int32_t INVALID_POSSIBLE_VALUE_INDEX;

                static const char8_t* ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE; // internal sentinel to indicate an unmatchable value in RETE


                DedicatedServerAttributeRuleDefinition();
                ~DedicatedServerAttributeRuleDefinition() override;

                // Init Logic:
                bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;
                void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;
                virtual void initGenericRuleConfig(GenericRuleConfig &ruleConfig) const;
                bool initAttributeName(const char8_t* attribName);
                bool initPossibleValueList(const PossibleValueContainer& possibleValues);
                bool addPossibleValue(const Collections::AttributeValue& possibleValue);
                bool parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const;

                //static void registerMultiInputValues(const char8_t* name);

                void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;


                // WME Logic:
                void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
                void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;


                bool isReteCapable() const override { return true; }

                const char8_t* getAttributeName() const { return mAttributeName.c_str(); }
                AttributeRuleType getAttributeRuleType() const { return mAttributeRuleType; }

                float getMinFitThresholdValue() const { return mMinFitThreshold; }

                //MM_AUDIT: This looks unsafe.  Should have a check and ERR if possibleValueIndex > mPossibleValues.size();
                const Collections::AttributeValue& getPossibleValue(size_t possibleValueIndex) const 
                { 
                    return mPossibleValues[possibleValueIndex]; 
                }

                // Returns the value of this rule's attribute from the game session.
                // Returns nullptr if the attribute does not exist, or if multiple attributes would be required (player attributes).
                const char8_t* getDedicatedServerAttributeValue(const GameSession& gameSession) const;
                static const char8_t* getDedicatedServerAttributeRuleType(const char8_t* ruleName, const DedicatedServerAttributeRuleServerConfig& genericRule);


                /*! ************************************************************************************************/
                /*!
                \brief return the integer index of a possibleValue string (or INVALID_POSSIBLE_VALUE_INDEX (-1) for error).

                \param[in]  value - the possibleValue string to get the index of.  (note: case-insensitive)
                \return The index, or INVALID_POSSIBLE_VALUE_INDEX (-1) if value isn't found in the possibleValue.
                *************************************************************************************************/
                int getPossibleValueIndex(const Collections::AttributeValue& value) const;

                size_t getPossibleValueCount() const { return mPossibleValues.size(); }
                
                float getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const;
                
                Collections::AttributeValue getDefaultValue() const { return mDefaultValue; }

                bool isDisabled() const override { return mPossibleValues.empty(); }

            protected:
                void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;


                // Members
            private:
                Collections::AttributeName mAttributeName;
                Collections::AttributeValue mDefaultValue;
                float mMinFitThreshold;

                AttributeRuleType mAttributeRuleType;

                FitTable mFitTable;

                PossibleValueContainer mPossibleValues;
            };

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_DEDICATEDSERVERATTRIBUTERULEDEFINITION_H
