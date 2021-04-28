/*! ************************************************************************************************/
/*!
\file   ruledefinitioncollection.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmaker/rules/gameattributerule.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributerule.h"
#include "gamemanager/matchmaker/rules/geolocationruledefinition.h"
#include "gamemanager/matchmaker/rules/rankedgameruledefinition.h"
#include "gamemanager/matchmaker/rules/virtualgameruledefinition.h"
#include "gamemanager/matchmaker/rules/rostersizeruledefinition.h"
#include "gamemanager/matchmaker/rules/expandedpingsiteruledefinition.h"
#include "gamemanager/matchmaker/rules/teamminsizeruledefinition.h"
#include "gamemanager/matchmaker/rules/teamchoiceruledefinition.h"
#include "gamemanager/matchmaker/rules/teambalanceruledefinition.h"
#include "gamemanager/matchmaker/rules/teamcountruledefinition.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"
#include "gamemanager/matchmaker/rules/gameattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"
#include "gamemanager/matchmaker/rules/modruledefinition.h"
#include "gamemanager/matchmaker/rules/gamenameruledefinition.h"
#include "gamemanager/matchmaker/rules/gametyperuledefinition.h"
#include "gamemanager/matchmaker/rules/gametopologyruledefinition.h"
#include "gamemanager/matchmaker/rules/pseudogameruledefinition.h"
#include "gamemanager/matchmaker/rules/platformruledefinition.h"
#include "gamemanager/matchmaker/rules/hostbalancingruledefinition.h"
#include "gamemanager/matchmaker/rules/avoidgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/avoidplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/preferredplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/preferredgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/totalplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/freeplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationruledefinition.h"
#include "gamemanager/matchmaker/rules/freepublicplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/rolesruledefinition.h"
#include "gamemanager/matchmaker/rules/reputationruledefinition.h"
#include "gamemanager/matchmaker/rules/xblblockplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h"
#include "gamemanager/matchmaker/rules/qosavoidgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/qosavoidplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityruledefinition.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"
#include "gamemanager/rete/stringtable.h"

#include "EASTL/sort.h" // for eastl::sort in RuleDefinitionCollection::initRuleDefinitions

namespace Blaze
{ 
namespace Metrics
{
namespace Tag
{
    extern TagInfo<GameManager::RuleName>* rule_name;
}
}
namespace GameManager
{

bool bypassNetworkTopologyQosValidation(GameNetworkTopology networkTopology)
{
    return (networkTopology == Blaze::CLIENT_SERVER_DEDICATED) || (networkTopology == Blaze::NETWORK_DISABLED);
}

namespace Matchmaker
{
    RuleDefinitionCollection::RuleDefinitionCollection(Metrics::MetricsCollection& collection, Rete::StringTable& stringTable, int32_t logCategory, bool filterOnly)
        : mLogCategory(logCategory)
        , mRuleDefinitionParseFactoryMap(BlazeStlAllocator("mRuleDefinitionParseFactoryMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
        , mReteRuleDefinitionList(BlazeStlAllocator("mReteRuleDefinitionList", GameManagerSlave::COMPONENT_MEMORY_GROUP))
        , mRuleDefinitionMap(BlazeStlAllocator("mRuleDefinitionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
        , mStringTable(stringTable)
        , mPredefinedRuleDefinitionList(BlazeStlAllocator("mPredefinedRuleDefinitionList", GameManagerSlave::COMPONENT_MEMORY_GROUP))
        , mMetricsCollection(collection)
        , mRuleUseCount(mMetricsCollection, "ruleUseCount", Metrics::Tag::rule_name)
        , mFilterOnly(filterOnly)
    {
        if (filterOnly)
        {
            addRuleDefinition<PropertyRuleDefinition>();
            return;
        }

        // Special cased rules that are not added via the config file, but always exist.
        addRuleDefinition<GameProtocolVersionRuleDefinition>();
        addRuleDefinition<SingleGroupMatchRuleDefinition>();
        addRuleDefinition<RosterSizeRuleDefinition>();
        addRuleDefinition<ModRuleDefinition>();
        addRuleDefinition<FreePlayerSlotsRuleDefinition>();
        addRuleDefinition<FreePublicPlayerSlotsRuleDefinition>();
        addRuleDefinition<RolesRuleDefinition>();
        addRuleDefinition<ReputationRuleDefinition>();
        addRuleDefinition<PropertyRuleDefinition>();

        // Xbox One only blocklist rule:
        if (gController->isPlatformHosted(xone) || gController->isPlatformHosted(xbsx))
        {
            addRuleDefinition<XblBlockPlayersRuleDefinition>();
        }

        // Game type rules
        addRuleDefinition<GameTypeRuleDefinition>();

        // Game & DS & Player attribute rules
        addRuleDefinition<GameAttributeRuleDefinition>();
        addRuleDefinition<DedicatedServerAttributeRuleDefinition>();
        addRuleDefinition<PlayerAttributeRuleDefinition>();

        // Predefined Matchmaking Rules
        addRuleDefinition<GameStateRuleDefinition>();
        addRuleDefinition<GameTopologyRuleDefinition>();
        addRuleDefinition<PseudoGameRuleDefinition>();
        addRuleDefinition<PlatformRuleDefinition>();
        addRuleDefinition<GameSettingsRuleDefinition>();
        addRuleDefinition<GeoLocationRuleDefinition>();
        addRuleDefinition<RankedGameRuleDefinition>();
        addRuleDefinition<VirtualGameRuleDefinition>();

        addRuleDefinition<TeamMinSizeRuleDefinition>();
        addRuleDefinition<TeamBalanceRuleDefinition>();
        addRuleDefinition<TeamUEDBalanceRuleDefinition>();
        addRuleDefinition<TeamUEDPositionParityRuleDefinition>();
        addRuleDefinition<TeamCompositionRuleDefinition>();
        addRuleDefinition<TeamChoiceRuleDefinition>();
        addRuleDefinition<TeamCountRuleDefinition>();
        addRuleDefinition<PlayerCountRuleDefinition>();
        addRuleDefinition<TotalPlayerSlotsRuleDefinition>();
        addRuleDefinition<PlayerSlotUtilizationRuleDefinition>();

        addRuleDefinition<HostBalancingRuleDefinition>();
        addRuleDefinition<HostViabilityRuleDefinition>();
        addRuleDefinition<ExpandedPingSiteRuleDefinition>();
        addRuleDefinition<GameNameRuleDefinition>();
        
        addRuleDefinition<DedicatedServerPlayerCapacityRuleDefinition>();

        // avoid games rule definitions
        addRuleDefinition<AvoidGamesRuleDefinition>();
        addRuleDefinition<QosAvoidGamesRuleDefinition>();

        // avoid or prefer players rule definitions
        addRuleDefinition<AvoidPlayersRuleDefinition>();
        addRuleDefinition<PreferredPlayersRuleDefinition>();
        addRuleDefinition<QosAvoidPlayersRuleDefinition>();
        addRuleDefinition<PreferredGamesRuleDefinition>();

        // UED rule
        addRuleDefinition<UEDRuleDefinition>();

        //Add any custom rules we may have, defined in custom code.
        addCustomRuleDefinitions();

        TRACE_LOG("[RuleDefinitionCollection].ctor Number of rules added " << mRuleDefinitionParseFactoryMap.size());
    }

    RuleDefinitionCollection::~RuleDefinitionCollection()
    {
        RuleDefinitionList::iterator itr = mRuleDefinitionList.begin();
        RuleDefinitionList::iterator end = mRuleDefinitionList.end();

        for (; itr != end; ++itr)
        {
            // We set all the IDs to INVALID, since some classes store this as a static value:
            (*itr)->setID(RuleDefinition::INVALID_DEFINITION_ID);
            delete *itr;
        }
    }

    void RuleDefinitionCollection::addParseFactoryMethod(const char8_t* definitionName, ParseDefinitionFunctionPointer definitionFactoryMethod, RuleDefinitionType ruleDefinitionType /*= RULE_DEFINITION_TYPE_SINGLE*/)
    {
        TRACE_LOG("[RuleDefinitionCollection].addParseFactoryMethod Adding Rule Definition Key '" << definitionName << "', ptr '" << (void*) definitionFactoryMethod << "', type " << ruleDefinitionType);
        RuleDefinitionParseFactoryMap::iterator existingFactory = mRuleDefinitionParseFactoryMap.find(definitionName);

        if (existingFactory == mRuleDefinitionParseFactoryMap.end())
        {
            DefinitionCreationInfo info = {definitionName, definitionFactoryMethod};
            mRuleDefinitionParseFactoryMap[definitionName] = info;

            if (ruleDefinitionType == RULE_DEFINITION_TYPE_SINGLE)
            {
                mPredefinedRuleDefinitionList.push_back(definitionName);
            }
        }
        else
        {
            ERR_LOG("[RuleDefinitionCollection].addParseFactoryMethod Duplicate Rule Definition Key '" << definitionName << "', ptr '" << (void*) definitionFactoryMethod << "'");
        }
    }

    bool RuleDefinitionCollection::createRuleDefinition( const char8_t* name, uint32_t salience, const char8_t* ruleDefinitionType, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        RuleDefinitionParseFactoryMap::iterator itr = mRuleDefinitionParseFactoryMap.find(ruleDefinitionType);

        if (itr != mRuleDefinitionParseFactoryMap.end())
        {
            DefinitionCreationInfo& info = itr->second;
            return (*info.parseFunction)(name, salience, matchmakingServerConfig, *this);
        }
        else
        {
            ERR_LOG("[RuleDefinitionCollection].createRuleDefinition Failed to find Definition for rule '" << name << "', type '" << ruleDefinitionType << "'");
            return false;
        }
        
    }

    bool RuleDefinitionCollection::createPredefinedRuleDefinitions(MatchmakingConfig& matchmakingConfig, const MatchmakingServerConfig& serverConfig)
    {
        bool result = true;

        PredefinedRuleDefinitionList::const_iterator iter = mPredefinedRuleDefinitionList.begin();
        PredefinedRuleDefinitionList::const_iterator end = mPredefinedRuleDefinitionList.end();
        for (; iter != end; ++iter)
        {
            const char8_t* ruleName = *iter;
            TRACE_LOG("[RuleDefinitionColleciton] Attempting to create predefined rule " << ruleName);
            RuleDefinitionParseFactoryMap::iterator parseIter = mRuleDefinitionParseFactoryMap.find(ruleName);
            if (parseIter != mRuleDefinitionParseFactoryMap.end())
            {
                uint32_t salience = matchmakingConfig.getRuleSalience(ruleName, mPriorityRuleDefinitionList.size());
                DefinitionCreationInfo& info = parseIter->second;
                bool parseVal = (*info.parseFunction)(ruleName, salience, serverConfig, *this);
                if (result)
                    result = parseVal;
            }
            else
            {
                result = false;
                WARN_LOG("[RuleDefinitionCollection].createPredefinedRuleDefinitions Failed to find Definition for rule " << ruleName);
            }
        }
        return result;
    }

    void RuleDefinitionCollection::addPriorityRuleDefinition(RuleDefinition& ruleDefinition)
    {
        addRuleDefinition(ruleDefinition);
        mPriorityRuleDefinitionList.push_back(&ruleDefinition);
    }

    void RuleDefinitionCollection::addRuleDefinition(RuleDefinition& ruleDefinition)
    {
        ruleDefinition.setRuleDefinitionCollection(*this);
        mRuleDefinitionList.push_back(&ruleDefinition);  // Takes ownership of the rule here and must delete it later.
        RuleNameToRuleDefinitionMap::insert_return_type ret = mRuleDefinitionMap.insert(eastl::make_pair(ruleDefinition.getName(), &ruleDefinition));
        if (ret.second == false)
        {
            ERR_LOG( "[RuleDefinitionCollection].addRuleDefinition: Unable to add rule definition to map. Multiple rules exist with name: "<< ruleDefinition.getName() << "." );
        }
    }

    const RuleDefinition* RuleDefinitionCollection::getRuleDefinitionById(RuleDefinitionId id) const
    {
        const RuleDefinition* rule = nullptr;
        if (id < mRuleDefinitionList.size())
            rule = mRuleDefinitionList.at(id);
        return rule;
    }

    const RuleDefinition* RuleDefinitionCollection::getRuleDefinitionByName(const char* ruleName) const
    {
        if (ruleName == nullptr || ruleName[0] == '\0')
            return nullptr;

        RuleNameToRuleDefinitionMap::const_iterator iter = mRuleDefinitionMap.find(ruleName);
        return (iter != mRuleDefinitionMap.end()) ? iter->second : nullptr;
    }

    bool RuleDefinitionCollection::initRuleDefinitions( MatchmakingConfig& matchmakingConfig, const MatchmakingServerConfig& serverConfig, const Blaze::Matchmaker::MatchmakerConfig* mmComponentConfig)
    {
        if (mFilterOnly)
        {
            return false;
        }

        if (!matchmakingConfig.initializeRuleDefinitionCollection(*this, serverConfig))
            return false;
        eastl::sort(mRuleDefinitionList.begin(), mRuleDefinitionList.end(), &CompareRuleDefinitionsBySalience);
        setRuleDefinitionIds();
        initReteRuleDefinitions();

        if ((mmComponentConfig != nullptr) && MatchmakingConfig::isUsingLegacyRules(*mmComponentConfig))
            return validateRuleDefinitions(serverConfig);
        return true;
    }


    bool RuleDefinitionCollection::initFilterDefinitions(const MatchmakingFilterMap& filters, const MatchmakingFilterList& globalFilters, const PropertyConfig& propertyConfig)
    {
        if (mFilterOnly)
        {
            setRuleDefinitionIds();
            initReteRuleDefinitions();

            // filters use property rule. Ensure its values initialized
            PropertyRuleDefinition* propRuleDefn = nullptr;
            if (PropertyRuleDefinition::getRuleDefinitionId() < mRuleDefinitionList.size())
            {
                propRuleDefn = static_cast<PropertyRuleDefinition*>(mRuleDefinitionList.at(PropertyRuleDefinition::getRuleDefinitionId()));
            }
            if (propRuleDefn == nullptr)
            {
                ERR_LOG("[RuleDefinitionCollection].initRuleDefinitions: internal error: Filter rule collection missing required property rule definition.");
                return false;
            }
            return propRuleDefn->initialize(filters, globalFilters, propertyConfig);
        }

        return false;
    }

    void RuleDefinitionCollection::setRuleDefinitionIds()
    {
        for (size_t i = 0; i < mRuleDefinitionList.size(); ++i)
        {
            mRuleDefinitionList[i]->setID((RuleDefinitionId)i);
        }
    }

    bool RuleDefinitionCollection::CompareRuleDefinitionsBySalience(const RuleDefinition* a, const RuleDefinition* b)
    {
        return (a->getSalience() < b->getSalience());
    }

    void RuleDefinitionCollection::initReteRuleDefinitions()
    {
        for (RuleDefinitionList::iterator itr = mRuleDefinitionList.begin(), end = mRuleDefinitionList.end();
            itr != end; ++itr)
        {
            if ((*itr)->isReteCapable())
            {
                mReteRuleDefinitionList.push_back(*itr);
            }
        }
    }

    // Returns whether minimum rules for pre-filter/factor Matchmaking are enabled.
    bool RuleDefinitionCollection::validateRuleDefinitions(const MatchmakingServerConfig& serverConfig) const
    {
        bool requiredRuleDefinitionsPresent = true;
        if (!isRuleDefinitionInUse<TotalPlayerSlotsRuleDefinition>())
        {
            //a rule is required for finalizing game sizes in create mode
            ERR_LOG("[RuleDefinitionCollection].validateRuleDefinitions - failed to init required total player slots rule definition: '" 
                << (TotalPlayerSlotsRuleDefinition::getConfigKey()) << "', Create Game Matchmaking cannot function without it.");
            requiredRuleDefinitionsPresent = false;
        }
        if (!isRuleDefinitionInUse<HostViabilityRuleDefinition>())
        {
            //host viability rule is required
            ERR_LOG("[RuleDefinitionCollection].validateRuleDefinitions - failed to init required rule definition: '" 
                    << (HostViabilityRuleDefinition::getConfigKey()) << "' Matchmaking cannot function without it.");
            requiredRuleDefinitionsPresent = false;
        }

        TeamCompositionRuleConfigMap::const_iterator tcmpnIter = serverConfig.getRules().getTeamCompositionRuleMap().begin();
        TeamCompositionRuleConfigMap::const_iterator tcmpnEnd = serverConfig.getRules().getTeamCompositionRuleMap().end();
        for (; tcmpnIter != tcmpnEnd; ++tcmpnIter)
        {
            //TeamCompositionRule's configuration errors are treated as critical failures, since they require special formatting for possible values
            const RuleName& ruleName = tcmpnIter->first;
            if (!isRuleDefinitionInUse<TeamCompositionRuleDefinition>(ruleName.c_str()))
            {
                ERR_LOG("[RuleDefinitionCollection].validateRuleDefinitions - failed to init configuration specified rule definition: '" 
                    << (TeamCompositionRuleDefinition::getConfigKey()) << " " << ruleName << "'.");
                requiredRuleDefinitionsPresent = false;
            }
        }

        return requiredRuleDefinitionsPresent;
    }

    template<typename RuleDef>
    bool validateRuleDefConfig(RuleDefinitionCollection& collection, const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // Now that all statics held by the rule definitions have been removed, we can safely init/delete them without any side effects.
        RuleDef tempRule;
        tempRule.setRuleDefinitionCollection(collection);
        return tempRule.initialize(name, salience, matchmakingServerConfig);
    }

    template<typename MapType>
    bool validateMultiRuleNames(const MapType& multiRuleMap, eastl::set<eastl::string>& multiRuleDefinitionConfigKeySet, ConfigureValidationErrors& validationErrors)
    {
        const RuleDefinitionConfigSet& ruleDefinitionConfigKeySet = getRuleDefinitionConfigSet();

        bool isValid = true;

        typename MapType::const_iterator iter = multiRuleMap.begin();
        typename MapType::const_iterator end = multiRuleMap.end();
        for (; iter != end; ++iter)
        {
            const RuleName& ruleName = iter->first;
            if (ruleDefinitionConfigKeySet.find(ruleName.c_str()) != ruleDefinitionConfigKeySet.end() ||
                multiRuleDefinitionConfigKeySet.find(ruleName.c_str()) != multiRuleDefinitionConfigKeySet.end())
            {
                eastl::string msg;
                msg.sprintf("[RuleDefinitionCollection].validateMultiRuleNames - failed to init. Multiple rules of name: '%s' found.", ruleName.c_str());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());

                isValid = false;
            }
            else
            {
                multiRuleDefinitionConfigKeySet.insert(eastl::string(ruleName.c_str()));
            }
        }
        return isValid;
    }

    bool RuleDefinitionCollection::validateConfig(const MatchmakingServerConfig& config, const MatchmakingServerConfig *referenceConfig, ConfigureValidationErrors& validationErrors)
    {
        bool isValid = true;
        if (!validateRuleDefConfig<TotalPlayerSlotsRuleDefinition>(*this, TotalPlayerSlotsRuleDefinition::getConfigKey(), 0, config))
        {
            //a rule is required for finalizing game sizes in create mode
            eastl::string msg;
            msg.sprintf("[RuleDefinitionCollection].validateConfig - failed to init required total player slots rule definition: '%s"
                        "', Create Game Matchmaking cannot function without it.", TotalPlayerSlotsRuleDefinition::getConfigKey());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());

            isValid = false;
        }
        if (!validateRuleDefConfig<HostViabilityRuleDefinition>(*this, HostViabilityRuleDefinition::getConfigKey(), 0, config))
        {
            //host viability rule is required
            eastl::string msg;
            msg.sprintf("[RuleDefinitionCollection].validateConfig - failed to init required rule definition: '%s" 
                        "' Matchmaking cannot function without it.", HostViabilityRuleDefinition::getConfigKey());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());

            isValid = false;
        }

        TeamCompositionRuleConfigMap::const_iterator tcmpnIter = config.getRules().getTeamCompositionRuleMap().begin();
        TeamCompositionRuleConfigMap::const_iterator tcmpnEnd = config.getRules().getTeamCompositionRuleMap().end();
        for (; tcmpnIter != tcmpnEnd; ++tcmpnIter)
        {
            //TeamCompositionRule's configuration errors are treated as critical failures, since they require special formatting for possible values
            const RuleName& ruleName = tcmpnIter->first;
            if (!validateRuleDefConfig<TeamCompositionRuleDefinition>(*this, ruleName.c_str(), 0, config))
            {
                eastl::string msg;
                msg.sprintf("[RuleDefinitionCollection].validateConfig - failed to init configuration's specified rule definition: '%s %s'.",
                            TeamCompositionRuleDefinition::getConfigKey(), ruleName.c_str());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());

                isValid = false;
            }
        }

        // Verify that no rules have overlapping names. (Uses config key for non-multi rules).
        eastl::set<eastl::string> multiRuleDefinitionConfigKeySet; // Temp set to track any overlap of multi-to-multi rule names
        if (!validateMultiRuleNames(config.getRules().getGameAttributeRules(), multiRuleDefinitionConfigKeySet, validationErrors) ||
            !validateMultiRuleNames(config.getRules().getDedicatedServerAttributeRules(), multiRuleDefinitionConfigKeySet, validationErrors) ||
            !validateMultiRuleNames(config.getRules().getPlayerAttributeRules(), multiRuleDefinitionConfigKeySet, validationErrors) ||
            !validateMultiRuleNames(config.getRules().getUserExtendedDataRuleMap(), multiRuleDefinitionConfigKeySet, validationErrors) ||
            !validateMultiRuleNames(config.getRules().getTeamUEDBalanceRuleMap(), multiRuleDefinitionConfigKeySet, validationErrors) ||
            !validateMultiRuleNames(config.getRules().getTeamCompositionRuleMap(), multiRuleDefinitionConfigKeySet, validationErrors))
        {
            isValid = false;
        }

        const QosValidationCriteriaMap& qosRuleMap = config.getRules().getQosValidationRule().getConnectionValidationCriteriaMap();
        for (QosValidationCriteriaMap::const_iterator itr = qosRuleMap.begin(), end = qosRuleMap.end(); itr != end; ++itr)
        {
            if (bypassNetworkTopologyQosValidation(itr->first))
            {
                // dedicated servers, being in common data centers, wouldn't benefit using QoS validation rule. To simplify and prevent misconfiguration, we don't support the rule with dedicated server topology.
                eastl::string msg;
                msg.sprintf("[RuleDefinitionCollection].validateConfig - failed to init qos validation rule definition. An invalid or unsupported topology %s was specified in its connection validation criteria map.", GameNetworkTopologyToString(itr->first));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                isValid = false;
            }
        }
        
        return isValid;
    }

    void RuleDefinitionCollection::initPriorityRule(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // find the given rule
        for(uint32_t i = 0; i < mPriorityRuleDefinitionList.size(); ++i)
        {
            RuleDefinition *priorityRuleDefinition = mPriorityRuleDefinitionList.at(i);
            if(blaze_stricmp(priorityRuleDefinition->getName(), name) == 0)
            {
                priorityRuleDefinition->initialize(name, salience, matchmakingServerConfig);
                break;
            }
        }
    }

    // Pre: mRuleDefinitionList initialized
    // Side: rete uses Separated substring search string tables, for easily tracking #users still needing an entry.
    // (Rete must clean entries when no longer used, as string search rule/defn may add 'infinite' poss values)
    void RuleDefinitionCollection::initSubstringTrieRuleStringTables(Rete::ReteNetwork& reteNetwork)
    {
        GameNameRuleDefinition* ruleDefn = const_cast<GameNameRuleDefinition*>(getRuleDefinition<GameNameRuleDefinition>());
        if ((ruleDefn == nullptr) || (ruleDefn->isDisabled()))
        {
            INFO_LOG("[RuleDefinitionCollection].initSubstringTrieRuleStringTables no substring search rules configured to init for.");
            return;
        }
        ruleDefn->setSearchStringTable(reteNetwork.getSubstringTrieSearchTable());
    }
    
    void RuleDefinitionCollection::incRuleDefinitionTotalUseCount(const char8_t* ruleName)
    {
        mRuleUseCount.increment(1, ruleName);
    }
    
    void RuleDefinitionCollection::decRuleDefinitionTotalUseCount(const char8_t* ruleName)
    {
        mRuleUseCount.decrement(1, ruleName);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
