/*! ************************************************************************************************/
/*!
\file   ruledefinitioncollectioncustom.cpp

    This file is provided as a hook to add custom rules to the rule definition collection.
    To add a custom rule, add the rule definition and rule code to the custom rule folder
    then attach a call to "addRuleDefinition<>" to the "addCustomRuleDefinitions" 
    method defined below.

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "rules/exampleupstreambpsruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    void RuleDefinitionCollection::addCustomRuleDefinitions()
    {       
        //////////////////////////////////////////////////////////////////////////
        // MM_CUSTOM_CODE - MM CUSTOM RULES
        //
        // Add new custom rules here to hook into the matchmaking system.
        //////////////////////////////////////////////////////////////////////////
        
        // Hook for ExampleUpstreamBPSRuleDefinition
        // MM_CUSTOM_CODE - BEGIN exampleUpstreamBPSRule
        //addRuleDefinition<ExampleUpstreamBPSRuleDefinition>();
        // MM_CUSTOM_CODE - END exampleUpstreamBPSRule

        //////////////////////////////////////////////////////////////////////////
        // MM_CUSTOM_CODE - END MM CUSTOM RULES
        //////////////////////////////////////////////////////////////////////////

    }

    void RuleDefinitionCollection::createCustomRuleDefinitions(const MatchmakingServerConfig& serverConfig, MatchmakingConfig& matchmakingConfig)
    {
        // This function is called when setting up RULE_DEFINITION_TYPE_MULTI rules, similar to the game attribute and player attribute rules. 

        // 
        // See MatchmakingConfig::initializeRuleDefinitionsFromConfig for examples.
        // 
    }

    bool RuleDefinitionCollection::registerCustomMultiInputValues(const MatchmakingServerConfig& serverConfig, const ScenarioRuleAttributeMap& ruleAttrMap)
    {
        // This is a static function used by Scenarios for RULE_DEFINITION_TYPE_MULTI rules, similar to the game attribute and player attribute rules. 
        // It registers the types used with the 
        // You can use the registerMultiInputValuesHelper to register your class's members, as long as the serverConfig has a map of your class's rules by name. 
        bool success = true;

        // Example:
        // // Register game attribute rules: 
        // success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, serverConfig.getRules().getGameAttributeRules(), GameAttributeRuleDefinition::getConfigKey());
        //
        // // If all of your custom rules use a map (key == name) in the config, then you can just pass that map in directly. (as above ^^)
        // // Otherwise, you'll need to make a dummy map (eastl::map<const char*, foo>) with a string key (value doesn't matter).
        // eastl::map<const char*, int> mapOfRules;
        // mapOfRules.insert("exampleRule");
        // mapOfRules.insert("modeMaskRule");
        // mapOfRules.insert("superModeMaskRule");
        // success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, mapOfRules, "bitmaskRules");

        return success;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
