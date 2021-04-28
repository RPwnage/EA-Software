/*! ************************************************************************************************/
/*!
    \file   avoidplayersruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/avoidplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/avoidplayersrule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(AvoidPlayersRuleDefinition, "Predefined_AvoidPlayersRule", RULE_DEFINITION_TYPE_SINGLE);

    AvoidPlayersRuleDefinition::AvoidPlayersRuleDefinition() 
        : PlayersRuleDefinition()
    {
    }
    AvoidPlayersRuleDefinition::~AvoidPlayersRuleDefinition()
    {
    }

    bool AvoidPlayersRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const AvoidPlayersRuleConfig& avoidPlayersRuleConfig = matchmakingServerConfig.getRules().getPredefined_AvoidPlayersRule();
        if(!avoidPlayersRuleConfig.isSet())
        {
            WARN_LOG("[AvoidPlayersRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        setMaxPlayersUsed(avoidPlayersRuleConfig.getMaxPlayersUsed());
        if (getMaxPlayersUsed() == 0)
        {
            WARN_LOG("[AvoidPlayersRuleDefinition] Rule configured but disabled. Max players used size of 0 was specified.");
            return false;
        }

        if (!PlayersRuleDefinition::initialize(name, salience, matchmakingServerConfig))
        {
            WARN_LOG("[AvoidPlayersRuleDefinition] Failed PlayersRuleDefinition initialziation.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        const uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);

        return true;
    }

    void AvoidPlayersRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void AvoidPlayersRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

