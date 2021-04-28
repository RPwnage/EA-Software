/*! ************************************************************************************************/
/*!
    \file   xblblockplayersruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/xblblockplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/xblblockplayersrule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(XblBlockPlayersRuleDefinition, "Predefined_XblBlockPlayersRule", RULE_DEFINITION_TYPE_SINGLE);

    XblBlockPlayersRuleDefinition::XblBlockPlayersRuleDefinition() 
        : PlayersRuleDefinition()
    {
    }
    XblBlockPlayersRuleDefinition::~XblBlockPlayersRuleDefinition()
    {
    }

    bool XblBlockPlayersRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {

        if (!PlayersRuleDefinition::initialize(name, salience, matchmakingServerConfig))
        {
            WARN_LOG("[XblBlockPlayersRuleDefinition] Failed PlayersRuleDefinition initialziation.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        const uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);

        return true;
    }

    void XblBlockPlayersRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void XblBlockPlayersRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

