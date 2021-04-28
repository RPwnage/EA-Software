/*! ************************************************************************************************/
/*!
    \file   rostersizeruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/matchmaker/rules/rostersizeruledefinition.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    // Special case code for RosterSizeRule which is actually a facade over PlayerCountRule.
    EA_THREAD_LOCAL RuleDefinitionId RosterSizeRuleDefinition::mRuleDefinitionId = UINT32_MAX; /*RuleDefinition::INVALID_DEFINITION_ID - Not usable by THREAD_LOCAL*/
    EA_THREAD_LOCAL const char* RosterSizeRuleDefinition::mRuleDefConfigKey = "Predefined_RosterSizeRule"; 
    EA_THREAD_LOCAL RuleDefinitionType RosterSizeRuleDefinition::mRuleDefType = RULE_DEFINITION_TYPE_SINGLE; 
    struct RosterSizeRuleDefinitionHelper { /*This struct is used to do static init (via its ctor call) of RuleDefinitionConfigSet*/
        RosterSizeRuleDefinitionHelper() { const_cast<RuleDefinitionConfigSet&>(getRuleDefinitionConfigSet())[RosterSizeRuleDefinition::getConfigKey()] = RosterSizeRuleDefinition::getRuleDefType(); }
        ~RosterSizeRuleDefinitionHelper() { destroyRuleDefinitionConfigSet(); }
    } g_RosterSizeRuleDefinitionHelper;
    Rule* RosterSizeRuleDefinition::createRule(MatchmakingAsyncStatus* matchmakingAsyncStatus) const { return nullptr; }

    const char8_t* RosterSizeRuleDefinition::ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME = "_RosterSizeMatchAny_";

    RosterSizeRuleDefinition::RosterSizeRuleDefinition()
    {
    }

    bool RosterSizeRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // rule does not have configuration
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);
        return true;
    }

    void RosterSizeRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void RosterSizeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        ERR_LOG("[RosterSizeRuleDefinition] insertWorkingMemoryElements is deprecated, no op. Use PlayerCountRule definition instead.");
    }

    void RosterSizeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        ERR_LOG("[RosterSizeRuleDefinition] upsertWorkingMemoryElements is deprecated, no op. Use PlayerCountRule definition instead.");
    }

    void RosterSizeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        // roster size rule only has configurable salience
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
