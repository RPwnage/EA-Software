/*! ************************************************************************************************/
/*!
\file playerattributeruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/fittable.h"
#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class DiagnosticsSearchSlaveHelper;

    class PlayerAttributeRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PlayerAttributeRuleDefinition);
        DEFINE_RULE_DEF_H(PlayerAttributeRuleDefinition, PlayerAttributeRule);
    public:
        static const char8_t* ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE;
        static const int32_t INVALID_POSSIBLE_VALUE_INDEX;

        PlayerAttributeRuleDefinition();
        ~PlayerAttributeRuleDefinition() override;

        /*! ************************************************************************************************/
        /*! \brief initialize the definition with any values needed from the configuration.
                This initialize function should be implemented by the derived class and should also
                call the base class implementation to gain the benefits of parsing the weight and
                minfitthresholdlists.

            \param[in] name The Rule Definition Name. An unique string identifier for this rule.
            \param[in] configMap The ConfigMap containing the rule config to initialize the rule definition.

            \returns false if there were any errors parsing the configuration
        *************************************************************************************************/
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        bool initAttributeName(const char8_t* attribName);
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;
        void initGenericRuleConfig( GenericRuleConfig &ruleConfig ) const;

        static void registerMultiInputValues(const char8_t* name);
        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;


        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        GroupValueFormula getGroupValueFormula() const { return mValueFormula; }
        bool isSupportedGroupValueFormula(GroupValueFormula formula) { return formula == GROUP_VALUE_FORMULA_AVERAGE || formula == GROUP_VALUE_FORMULA_MIN || formula == GROUP_VALUE_FORMULA_MAX; }

        void logConfigValues(eastl::string &dest, const char8_t* prefix) const override;

        Rete::WMEAttribute getWMEPlayerAttributeName(const char8_t* attributeName, const char8_t* attributeValue) const;
        const char8_t* getPlayerAttributeRuleType(const char8_t* ruleName, const PlayerAttributeRuleServerConfig& playerAttributeRule);
        
        typedef eastl::map<const char8_t*, uint32_t, CaseInsensitiveStringLessThan> AttributeValueCount;
        bool getPlayerAttributeValues(const Search::GameSessionSearchSlave& gameSession, PlayerAttributeRuleDefinition::AttributeValueCount& outAttrMap) const;


        bool parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const;
        bool initPossibleValueList(const PossibleValueContainer& possibleValues);

        int getPossibleValueIndex(const Collections::AttributeValue& value) const;
        int getPossibleActualValueIndex(const Collections::AttributeValue& value) const;
        bool addPossibleValue(const Collections::AttributeValue& possibleValue);
        float getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const;


        size_t getPossibleValueCount() const { return mPossibleValues.size(); }
        const char8_t* getAttributeName() const { return mAttributeName.c_str(); }
        AttributeRuleType getAttributeRuleType() const { return mAttributeRuleType; }

        bool isValueRandom(const char8_t* attribValue) const { return blaze_stricmp(attribValue, FitTable::RANDOM_LITERAL_CONFIG_VALUE) == 0; }
        bool isValueAbstain(const char8_t* attribValue) const { return blaze_stricmp(attribValue, FitTable::ABSTAIN_LITERAL_CONFIG_VALUE) == 0; }
        const PossibleValueContainer& getPossibleActualValues() const { return mPossibleActualValues; }

        const Collections::AttributeValue& getPossibleValue(size_t possibleValueIndex) const;

        virtual void updateAsyncStatus();

        bool isReteCapable() const override { return false; }

    protected:
        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;

    // Members
    private:
        GroupValueFormula mValueFormula;
        AttributeRuleType mAttributeRuleType;
        Collections::AttributeName mAttributeName;

        FitTable mFitTable;
        PossibleValueContainer mPossibleValues;

        // copy of char* from mPossibleValues w/o ABSTAIN/RANDOM.
        // When choosing a 'random' possible value, we just index into this.
        // Note: value string mem still owned by mPossibleValues
        PossibleValueContainer mPossibleActualValues;

    //protected:
        float mMatchingFitPercent;
        float mMismatchFitPercent;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYERATTRIBUTERULEDEFINITION_H
