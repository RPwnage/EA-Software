/*! ************************************************************************************************/
/*!
    \file   PreferredGamesRuledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/preferredgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/preferredgamesrule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/tdf/matchmaker_config_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PreferredGamesRuleDefinition, "Predefined_PreferredGamesRule", RULE_DEFINITION_TYPE_SINGLE);

    PreferredGamesRuleDefinition::PreferredGamesRuleDefinition() : mMaxGamesUsed(0)
    {
    }
    PreferredGamesRuleDefinition::~PreferredGamesRuleDefinition()
    {
    }

    bool PreferredGamesRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const PreferredGamesRuleConfig& preferredPlayersListRuleConfig = matchmakingServerConfig.getRules().getPredefined_PreferredGamesRule();
        if(!preferredPlayersListRuleConfig.isSet())
        {
            WARN_LOG("[PreferredGamesRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        mMaxGamesUsed = preferredPlayersListRuleConfig.getMaxGamesUsed();
        if (mMaxGamesUsed == 0)
        {
            WARN_LOG("[PreferredGamesRuleDefinition] Rule configureud but disabled. Max games used size of 0 was specified.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, preferredPlayersListRuleConfig.getWeight());

        return true;
    }

    void PreferredGamesRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void PreferredGamesRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

