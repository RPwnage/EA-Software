/*! ************************************************************************************************/
/*!
    \file   matchmakingconfig.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"

#include <math.h>

#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h" // for custom rules
#include "gamemanager/matchmaker/rules/gameattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h"
#include "gamemanager/matchmaker/rules/teamcompositionruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h"
#include "gamemanager/matchmaker/voting.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const char8_t* MatchmakingConfig::MATCHMAKER_SETTINGS_KEY = "matchmaker";

    /*! ************************************************************************************************/
    /*!
        \brief Create a matchmaking config obj, allowing the config to be parsed easily.

        \note
            MatchmakerConfig is intended to be constructed as a stack obj when the config file is loaded or
        modified.  This is because the obj stores a copy of the matchmaking map (which can be invalidated
        if blaze reloads its config files).

        \param[in]  configMap - pointer to the ConfigMap containing the matchmaking config.
            (contains "matchmaking = {...})
    *************************************************************************************************/
    MatchmakingConfig::MatchmakingConfig()
        : mCurrentRuleName(""),
          mCurrentThresholdListName("")
    {
    }

    MatchmakingConfig::~MatchmakingConfig()
    {
    }

    const char8_t* MatchmakingConfig::MATCHING_FIT_PERCENT_KEY = "matchingFitPercent";
    const char8_t* MatchmakingConfig::MISMATCH_FIT_PERCENT_KEY = "mismatchFitPercent";

    const char8_t* MatchmakingConfig::EXACT_MATCH_REQUIRED_TOKEN = "EXACT_MATCH_REQUIRED";

    const char8_t* MatchmakingConfig::CUSTOM_RULES_KEY = "customRules";
    const char8_t* MatchmakingConfig::MATCHMAKER_RULES_KEY = "rules";
    const uint32_t MatchmakingConfig::PRIORITY_RULE_COUNT = 4; // this ensures that priority rules get processed first

    bool MatchmakingConfig::initializeRuleDefinitionCollection(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig)
    {
        if (serverConfig.getRuleSalience().getSalienceList().empty())
        {
            WARN_LOG("[MatchmakingConfig].initializeRuleDefinitionCollection Rule salience list is empty, will use defaults.");
        }

        // we set mNextSalience to the size of the salience list + the count of the priority rule definitions to ensure that rules with unconfigured salience
        // are initialized with salience behind all configured rules.
        mNextSalience = serverConfig.getRuleSalience().getSalienceList().size() + ruleDefinitionCollection.getPriorityRuleDefinitionList().size();

        if (!initializeRuleDefinitionsFromConfig(ruleDefinitionCollection, serverConfig))
            return false;

        initializePriorityRules(ruleDefinitionCollection, serverConfig);

        return true;
    }



    void MatchmakingConfig::initializePriorityRules(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig)
    {
        const RuleDefinitionList& priorityRuleDefinitionList = ruleDefinitionCollection.getPriorityRuleDefinitionList();
        uint32_t priorityRuleDefinitionListSize = priorityRuleDefinitionList.size();

        for(uint32_t i = 0; i < priorityRuleDefinitionListSize; ++i)
        {
            const RuleDefinition* priorityRuleDefinition = priorityRuleDefinitionList.at(i);
            const char8_t* ruleName = priorityRuleDefinition->getName();
            // find the rule in the salience map
            uint32_t ruleSalience = RuleDefinition::INVALID_RULE_SALIENCE;
            for(uint32_t j = 0; j < serverConfig.getRuleSalience().getSalienceList().size(); ++j)
            {
                const char8_t* name = serverConfig.getRuleSalience().getSalienceList().at(j);
                if(blaze_stricmp(name, ruleName) == 0)
                {
                    ruleSalience = j + priorityRuleDefinitionListSize;
                    ruleDefinitionCollection.initPriorityRule(ruleName, ruleSalience, serverConfig);
                    break;
                }
            }

            if(ruleSalience == RuleDefinition::INVALID_RULE_SALIENCE)
            {
                ruleDefinitionCollection.initPriorityRule(ruleName, i, serverConfig);
            }
        }
        
    }

    bool MatchmakingConfig::registerMultiInputValues(const MatchmakingServerConfig& serverConfig, const ScenarioRuleAttributeMap& ruleAttrMap)
    {
        // This function is responsible for mapping the generic values of multi-rules.
        // This mapping is needed for any values that were previously mapped by a (single) map in the criteria data. 
        // Rules that have multiple configs, but only use one per-matchmaking session, do not need this mapping. 
        bool success = true;
        success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, serverConfig.getRules().getGameAttributeRules(), GameAttributeRuleDefinition::getConfigKey());
        success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, serverConfig.getRules().getDedicatedServerAttributeRules(), DedicatedServerAttributeRuleDefinition::getConfigKey());
        success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, serverConfig.getRules().getPlayerAttributeRules(), PlayerAttributeRuleDefinition::getConfigKey());
        success = success && registerMultiInputValuesHelper(serverConfig, ruleAttrMap, serverConfig.getRules().getUserExtendedDataRuleMap(), UEDRuleDefinition::getConfigKey());

        success = success && RuleDefinitionCollection::registerCustomMultiInputValues(serverConfig, ruleAttrMap);

        return success;
    }

    bool MatchmakingConfig::initializeRuleDefinitionsFromConfig(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig)
    {
        // Game Attribute rules.  Can have multiple game attribute rules with different name.
        GameAttributeRuleServerConfigMap::const_iterator gameAttrIter = serverConfig.getRules().getGameAttributeRules().begin();
        GameAttributeRuleServerConfigMap::const_iterator gameAttrEnd = serverConfig.getRules().getGameAttributeRules().end();
        for (; gameAttrIter != gameAttrEnd; ++gameAttrIter)
        {
            const RuleName& ruleName = gameAttrIter->first;
            const char8_t* ruleDefinitionType = GameAttributeRuleDefinition::getConfigKey();
            if (ruleDefinitionType == nullptr)
            {
                continue;
            }

            TRACE_LOG("[MatchmakingConfig] Attempting to create game attribute rule " << ruleName);
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, ruleDefinitionType, serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("Game Attribute rule " << ruleName << " had configuration errors.");
                return false;
            }
        }

        // Dedicated Server Attribute rules.  Can have multiple dedicated server attribute rules with different name.
        DedicatedServerAttributeRuleServerConfigMap::const_iterator dsAttrIter = serverConfig.getRules().getDedicatedServerAttributeRules().begin();
        DedicatedServerAttributeRuleServerConfigMap::const_iterator dsAttrEnd = serverConfig.getRules().getDedicatedServerAttributeRules().end();
        for (; dsAttrIter != dsAttrEnd; ++dsAttrIter)
        {
            const RuleName& ruleName = dsAttrIter->first;
            const char8_t* ruleDefinitionType = DedicatedServerAttributeRuleDefinition::getConfigKey();
            if (ruleDefinitionType == nullptr)
            {
                continue;
            }

            TRACE_LOG("[MatchmakingConfig] Attempting to create dedicated server attribute rule " << ruleName);
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, ruleDefinitionType, serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("Dedicated Server Attribute rule " << ruleName << " had configuration errors.");
                return false;
            }
        }

        // Player Attribute rules.  Can have multiple player attribute rules with different name.
        PlayerAttributeRuleServerConfigMap::const_iterator playerAttrIter = serverConfig.getRules().getPlayerAttributeRules().begin();
        PlayerAttributeRuleServerConfigMap::const_iterator playerAttrEnd = serverConfig.getRules().getPlayerAttributeRules().end();
        for (; playerAttrIter != playerAttrEnd; ++playerAttrIter)
        {
            const RuleName& ruleName = playerAttrIter->first;
            const char8_t* ruleDefinitionType = PlayerAttributeRuleDefinition::getConfigKey();
            if (ruleDefinitionType == nullptr)
            {
                continue;
            }

            TRACE_LOG("[MatchmakingConfig] Attempting to create player attribute rule " << ruleName);
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, ruleDefinitionType, serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("Player Attribute rule " << ruleName << " had configuration errors.");
                return false;
            }
        }

        // UED rules.  Can have multiple UED rules with different names.
        UEDRuleConfigMap::const_iterator uedIter = serverConfig.getRules().getUserExtendedDataRuleMap().begin();
        UEDRuleConfigMap::const_iterator uedEnd = serverConfig.getRules().getUserExtendedDataRuleMap().end();
        for (; uedIter != uedEnd; ++uedIter)
        {
            const RuleName& ruleName = uedIter->first;
            TRACE_LOG("[MatchmakingConfig] Attempting to create ued rule '" << ruleName << "'.");
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, UEDRuleDefinition::getConfigKey(), serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("UED rule '" << ruleName << "' had configuration errors.");
                return false;
            } 
        }

        TeamUEDBalanceRuleConfigMap::const_iterator tuedIter = serverConfig.getRules().getTeamUEDBalanceRuleMap().begin();
        TeamUEDBalanceRuleConfigMap::const_iterator tuedEnd = serverConfig.getRules().getTeamUEDBalanceRuleMap().end();
        for (; tuedIter != tuedEnd; ++tuedIter)
        {
            const RuleName& ruleName = tuedIter->first;

            TRACE_LOG("[MatchmakingConfig] Attempting to create team ued balance rule '" << ruleName << "'.");
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, TeamUEDBalanceRuleDefinition::getConfigKey(), serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("Team UED Balance rule '" << ruleName << "' had configuration errors.");
                return false;
            } 
        }

        TeamUEDPositionParityRuleConfigMap::const_iterator tppuedIter = serverConfig.getRules().getTeamUEDPositionParityRuleMap().begin();
        TeamUEDPositionParityRuleConfigMap::const_iterator tppuedEnd = serverConfig.getRules().getTeamUEDPositionParityRuleMap().end();
        for (; tppuedIter != tppuedEnd; ++tppuedIter)
        {
            const RuleName& ruleName = tppuedIter->first;

            TRACE_LOG("[MatchmakingConfig] Attempting to create team ued position parity rule '" << ruleName << "'.");
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, TeamUEDPositionParityRuleDefinition::getConfigKey(), serverConfig))
            {
                // Not necessarily critical failure.  Rule could just end up disabled.
                ERR_LOG("Team UED Positon Parity rule '" << ruleName << "' had configuration errors.");
                return false;
            } 
        }

        TeamCompositionRuleConfigMap::const_iterator tcmpnIter = serverConfig.getRules().getTeamCompositionRuleMap().begin();
        TeamCompositionRuleConfigMap::const_iterator tcmpnEnd = serverConfig.getRules().getTeamCompositionRuleMap().end();
        for (; tcmpnIter != tcmpnEnd; ++tcmpnIter)
        {
            const RuleName& ruleName = tcmpnIter->first;

            TRACE_LOG("[MatchmakingConfig] Attempting to create team composition rule '" << ruleName << "'.");
            uint32_t ruleSalience = getRuleSalience(ruleName, ruleDefinitionCollection.getPriorityRuleDefinitionList().size());

            if (!ruleDefinitionCollection.createRuleDefinition(ruleName, ruleSalience, TeamCompositionRuleDefinition::getConfigKey(), serverConfig))
            {
                // TeamCompositionRule's configuration errors are treated as critical failures, since they require special formatting for possible values
                ERR_LOG("Team Composition rule '" << ruleName << "' had configuration errors.");
                return false;
            } 
        }

        // Add any custom matchmaking rules using RULE_DEFINITION_TYPE_MULTI
        ruleDefinitionCollection.createCustomRuleDefinitions(serverConfig, *this);

        // All others are predefined rules.  There is only one instance of these rules.
        // We now loop through each defined rule in the collection and attempt to parse a config for it.
        // If no configuration was specified (TDF parsing will read it w/o errors)
        // We skip that configuration and the rule is disabled.
        if (!ruleDefinitionCollection.createPredefinedRuleDefinitions(*this, serverConfig))
        {
            // TODO: move to validation.
            WARN_LOG("[MatchmakingConfig] One or more predefined rules were not configured and were disabled.");
        }

        return true;
    }

    uint32_t MatchmakingConfig::getRuleSalience(const char8_t* ruleName, uint32_t priorityRuleCount)
    {
        // find the rule in the salience map
        uint32_t ruleSalience = RuleDefinition::INVALID_RULE_SALIENCE;
        for(uint32_t i = 0; i < mMatchmakingConfigTdf->getRuleSalience().getSalienceList().size(); ++i)
        {
            const char8_t* name = mMatchmakingConfigTdf->getRuleSalience().getSalienceList().at(i);
            if(blaze_stricmp(name, ruleName) == 0)
            {
                ruleSalience = i;
                // This is to make room for "priority rules" such as 
                // GameProtocolVersionRule, RosterSizeRule, GameStateRule and GameSettingsRule
                // to ensure that they are configured to have the highest salience.
                ruleSalience += priorityRuleCount;
                break;
            }
        }

        if(ruleSalience == RuleDefinition::INVALID_RULE_SALIENCE)
        {
            ruleSalience = mNextSalience;
            ++mNextSalience;
        }

        return ruleSalience;
    }

    bool MatchmakingConfig::isUsingLegacyRules(const Blaze::Matchmaker::MatchmakerConfig& config)
    {
        // Titles should all now be on scenarios, but just in case, we'll assume its still using rules without them, if none configured:
        if (config.getScenariosConfig().getScenarios().empty())
            return true;

        // check whether scenarios use rules
        if (!config.getScenariosConfig().getGlobalRules().empty())
            return true;
        for (auto& scenItr : config.getScenariosConfig().getScenarios())
        {
            for (auto& subsess : scenItr.second->getSubSessions())
            {
                if (!subsess.second->getRulesList().empty())
                    return true;
            }
        }
        return false;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
