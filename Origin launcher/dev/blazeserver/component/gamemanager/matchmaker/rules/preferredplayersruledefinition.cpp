/*! ************************************************************************************************/
/*!
    \file   preferredplayersruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/preferredplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/preferredplayersrule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PreferredPlayersRuleDefinition, "Predefined_PreferredPlayersRule", RULE_DEFINITION_TYPE_SINGLE);

    PreferredPlayersRuleDefinition::PreferredPlayersRuleDefinition()
        : PlayersRuleDefinition()
    {
    }
    PreferredPlayersRuleDefinition::~PreferredPlayersRuleDefinition()
    {
    }

    bool PreferredPlayersRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const PreferredPlayersRuleConfig& preferredPlayersListRuleConfig = matchmakingServerConfig.getRules().getPredefined_PreferredPlayersRule();
        if(!preferredPlayersListRuleConfig.isSet())
        {
            WARN_LOG("[PreferredPlayersRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        setMaxPlayersUsed(preferredPlayersListRuleConfig.getMaxPlayersUsed());
        if (getMaxPlayersUsed() == 0)
        {
            WARN_LOG("[PreferredPlayersRuleDefinition] Rule configured but disabled. Max players used size of 0 was specified.");
            return false;
        }
        
        if (!PlayersRuleDefinition::initialize(name, salience, matchmakingServerConfig))
        {
            WARN_LOG("[PreferredPlayersRuleDefinition] Failed PlayersRuleDefinition initialziation.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, preferredPlayersListRuleConfig.getWeight());

        return true;
    }

    void PreferredPlayersRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void PreferredPlayersRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

