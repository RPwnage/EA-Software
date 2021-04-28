/*! ************************************************************************************************/
/*!
\file   ruledefinitioncollection.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef RULE_DEFINITION_COLLECTION_H
#define RULE_DEFINITION_COLLECTION_H

#include "framework/tdf/controllertypes_server.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "EASTL/hash_map.h"

#include "gamemanager/matchmaker/rules/gamereteruledefinition.h"
#include "gamemanager/matchmaker/rules/gamestateruledefinition.h"
#include "gamemanager/matchmaker/rules/gamesettingsruledefinition.h"
#include "gamemanager/matchmaker/rules/singlegroupmatchruledefinition.h"
#include "gamemanager/matchmaker/rules/gameprotocolversionruledefinition.h"

namespace Blaze
{
namespace Matchmaker
{
    class MatchmakerConfig;
}

namespace GameManager
{

namespace Rete
{
    class StringTable;
}

namespace Matchmaker
{
    class GameNameRuleDefinition; // for GameNameRuleDefinition in getGameNameRuleDefinition()

    class RuleDefinitionCollection;

    typedef eastl::vector<RuleDefinition*> RuleDefinitionList;
    typedef eastl::vector<GameReteRuleDefinition*> ReteRuleDefinitionList;

        /*! ************************************************************************************************/
        /*! \brief Parse a rule definition from given the config map for the rule.  The generated rule
            definition should be added to the RuleDefinitionCollection before returning.

            \param[in] matchmakingServerConfig - the matchmaking config.
            \param[in] salience - the salience for this rule
            \param[in/out] ruleDefinitionCollection - The entire current collection of rule definitions.  Once
                you have successfully created your own rule definition, add it to the collection.

            \return success/failure
        *************************************************************************************************/
    typedef bool (*ParseDefinitionFunctionPointer)(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig, RuleDefinitionCollection&);

    struct DefinitionCreationInfo
    {
        const char8_t* definitionName;
        ParseDefinitionFunctionPointer parseFunction;
    };
    typedef eastl::hash_map<const char8_t*, DefinitionCreationInfo, eastl::hash<const char8_t*>,eastl::str_equal_to<const char8_t*> > RuleDefinitionParseFactoryMap;

    /*! ************************************************************************************************/
    /*! \brief MatchmakingRuleDefinitionCollection's contain rule definitions parsed from the components 
        config file. Instances of this object are stored in the GameBrowser and Matchmaker objects.
    *************************************************************************************************/
    class RuleDefinitionCollection
    {
    public:
        RuleDefinitionCollection(Metrics::MetricsCollection& collection, Rete::StringTable& stringTable, int32_t logCategory, bool filterOnly);

        ~RuleDefinitionCollection();
        int32_t getLogCategory() const { return mLogCategory; }
        void addRuleDefinition(RuleDefinition& ruleDefinition);
        const RuleDefinition* getRuleDefinitionById(RuleDefinitionId id) const;
        const RuleDefinition* getRuleDefinitionByName(const char* ruleName) const;

        const RuleDefinitionList& getRuleDefinitionList() const { return mRuleDefinitionList; }
        RuleDefinitionList& getRuleDefinitionList() { return mRuleDefinitionList; }
        const ReteRuleDefinitionList& getReteRuleDefinitionList() const { return mReteRuleDefinitionList; }
        const RuleDefinitionList& getPriorityRuleDefinitionList() const { return mPriorityRuleDefinitionList; }

        void initSubstringTrieRuleStringTables(Rete::ReteNetwork& reteNetwork);

        // Once 'rules' are removed, no need for the 'MatchmakingServerConfig' param:
        bool initRuleDefinitions(MatchmakingConfig& matchmakingConfig, const MatchmakingServerConfig& serverConfig, const Blaze::Matchmaker::MatchmakerConfig* mmComponentConfig = nullptr);
        bool initFilterDefinitions(const MatchmakingFilterMap& filters, const MatchmakingFilterList& globalFilters, const PropertyConfig& propertyConfig);

        bool createRuleDefinition(const char8_t* name, uint32_t salience, const char8_t* ruleDefinitionType, const MatchmakingServerConfig& matchmakingServerConfig);
        bool createPredefinedRuleDefinitions(MatchmakingConfig& matchmakingConfig, const MatchmakingServerConfig& serverConfig);

        void initReteRuleDefinitions();
        void initPriorityRule(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig);
        
        void incRuleDefinitionTotalUseCount(const char8_t* ruleName);
        void decRuleDefinitionTotalUseCount(const char8_t* ruleName);
        
        const SingleGroupMatchRuleDefinition& getSingleGroupMatchRuleDefinition() const { return mSGMRuleDefinition; }
        const Metrics::TaggedGauge<GameManager::RuleName>& getRuleDefinitionTotalUseCount() const { return mRuleUseCount; }

        bool validateConfig(const MatchmakingServerConfig& config, const MatchmakingServerConfig *referenceConfig, ConfigureValidationErrors& validationErrors);

        // Templated addition functions:
        template<typename RuleDef>
        void addRuleDefinition(bool isForcedPriority = false);
        
        // Looks up the rule based on the type.
        // RuleName is only used/required when accessing multi-entry rules (UED, Game/Player Attributes, etc.).
        template<typename RuleDef>
        const RuleDef* getRuleDefinition(const char* ruleName = nullptr) const;
        template<typename RuleDef>
        bool isRuleDefinitionInUse(const char* ruleName = nullptr) const;

        // Helper factory to create and initialize RuleDefinitions:
        template <typename RuleDef>
        static bool parseDefinition(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig, RuleDefinitionCollection& ruleDefinitionCollection);

        // For Multi-rule definitions:
        void createCustomRuleDefinitions(const MatchmakingServerConfig& serverConfig, MatchmakingConfig& matchmakingConfig);
        static bool registerCustomMultiInputValues(const MatchmakingServerConfig& serverConfig, const ScenarioRuleAttributeMap& ruleAttrMap);
        Rete::StringTable& getStringTable() const { return mStringTable; }

    private:
        void addCustomRuleDefinitions();

        void addPriorityRuleDefinition(RuleDefinition& ruleDefinition);
        void addParseFactoryMethod(const char8_t* definitionName, ParseDefinitionFunctionPointer definitionFactoryMethod, RuleDefinitionType ruleDefinitionType = RULE_DEFINITION_TYPE_SINGLE);

        void setRuleDefinitionIds();
        static bool CompareRuleDefinitionsBySalience(const RuleDefinition* a, const RuleDefinition* b);
        bool validateRuleDefinitions(const MatchmakingServerConfig& serverConfig) const;

    private:
        int32_t mLogCategory = 0;
        RuleDefinitionList mRuleDefinitionList;
        RuleDefinitionList mPriorityRuleDefinitionList;
        RuleDefinitionParseFactoryMap mRuleDefinitionParseFactoryMap;

        ReteRuleDefinitionList mReteRuleDefinitionList;

        typedef eastl::hash_map<eastl::string, RuleDefinition*, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> RuleNameToRuleDefinitionMap;
        RuleNameToRuleDefinitionMap mRuleDefinitionMap;

        // TODO: Use the similar factory as with the other mm rules.
        SingleGroupMatchRuleDefinition mSGMRuleDefinition;

        Rete::StringTable& mStringTable;

        // List of rules that have a single predefined name, and need to be initialized.
        typedef eastl::list<const char8_t*> PredefinedRuleDefinitionList;
        PredefinedRuleDefinitionList mPredefinedRuleDefinitionList;

        Metrics::MetricsCollection& mMetricsCollection;
        Metrics::TaggedGauge<GameManager::RuleName> mRuleUseCount;

        bool mFilterOnly;
    };

    /*! ************************************************************************************************/
    /*! \brief Create and parse a rule definition from given the config map for the rule.  The
            new rule rule definition should be added to the RuleDefinitionCollection.

        This is templatized for convenience.  All classes that use this method should be of RuleDefinition Type and
            derive from the class RuleDefinition.
            
        \param[in] configMap - the config map for the rule.  This is only the section of the config map
        that matches the current rules name.
        \param[in] salience - the salience for this rule
        \param[in/out] ruleDefinitionCollection - The entire current collection of rule definitions.  Once
        you have successfully created your own rule definition, add it to the collection.

        \return success/failure
    *************************************************************************************************/
    template<typename RuleDef>
    bool RuleDefinitionCollection::parseDefinition(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig, RuleDefinitionCollection& ruleDefinitionCollection)
    {
        RuleDef *defn = BLAZE_NEW RuleDef();

        defn->setRuleDefinitionCollection(ruleDefinitionCollection);

        // Initialize the definition, pulls configuration values out of the matchmaking configuration.
        if (defn->initialize(name, salience, matchmakingServerConfig))
        {
            TRACE_LOG("[RuleDefinitionCollection].parseDefinition Added Rule Definition '" << defn->getName() << "' type '" << defn->getType() << "' to the rule definition collection.");
            // Add our created rule definition to the entire collection of custom rules
            ruleDefinitionCollection.addRuleDefinition(*defn);  // Rule Definition Collection will handle the memory
            return true;
        }

        WARN_LOG("[RuleDefinitionCollection].parseDefinition Failed to init Rule Definition '" << name << "' this rule will be disabled.");
        delete defn;  // On failure delete the rule definition.

        return false;
    }

    template<typename RuleDef>
    void RuleDefinitionCollection::addRuleDefinition(bool isForcedPriority)
    {
        if (RuleDef::getRuleDefType() == RULE_DEFINITION_TYPE_PRIORITY || isForcedPriority)
        {
            addPriorityRuleDefinition(*(BLAZE_NEW RuleDef()));
        }
        else
        {
            addParseFactoryMethod(RuleDef::getConfigKey(), &parseDefinition<RuleDef>, RuleDef::getRuleDefType());
        }
    }

    template<typename RuleDef>
    const RuleDef* RuleDefinitionCollection::getRuleDefinition(const char* ruleName) const
    {
        if (RuleDef::getRuleDefType() != RULE_DEFINITION_TYPE_MULTI)
        {
            if (ruleName != nullptr && blaze_stricmp(ruleName, RuleDef::getConfigKey()) != 0)
            {
                WARN_LOG("[RuleDefinitionCollection::getRuleDefinition] Attempting to lookup a non-RULE_DEFINITION_TYPE_MULTI rule, while specifying a rule name (" << ruleName << ") which does not match the config name (" << ruleName<< "). Ignoring the rule name.");
            }

            return static_cast<const RuleDef*>(getRuleDefinitionByName(RuleDef::getConfigKey()));
        }
        else
        {
            if (ruleName == nullptr)
            {
                ERR_LOG("[RuleDefinitionCollection::getRuleDefinition] Attempting to lookup a RULE_DEFINITION_TYPE_MULTI rule without specifying a rule name. Nothing will be found.");
                return nullptr;
            }

            const RuleDefinition* ruleDef = getRuleDefinitionByName(ruleName);

            if (ruleDef && !ruleDef->isType(RuleDef::getConfigKey()))
            {
                ERR_LOG("[RuleDefinitionCollection::getRuleDefinition] Rule found from lookup does not match expected type ("<< ruleDef->getType() <<" != "<< RuleDef::getConfigKey() <<"). Returning nullptr.");
                return nullptr;
            }
            return static_cast<const RuleDef*>(ruleDef);
        }
    }

    template<typename RuleDef>
    inline bool RuleDefinitionCollection::isRuleDefinitionInUse(const char* ruleName) const
    {
        return getRuleDefinition<RuleDef>(ruleName) != nullptr;
    }

} //namespace Matchmaker
} //namespace GameManager
} //namespace Blaze
#endif

