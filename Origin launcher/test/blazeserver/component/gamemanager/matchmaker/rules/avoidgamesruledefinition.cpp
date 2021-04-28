/*! ************************************************************************************************/
/*!
    \file   avoidgamesruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/avoidgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/avoidgamesrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(AvoidGamesRuleDefinition, "Predefined_AvoidGamesRule", RULE_DEFINITION_TYPE_SINGLE);

    AvoidGamesRuleDefinition::AvoidGamesRuleDefinition() : mMaxGameIdListSize(0)
    {
    }
    AvoidGamesRuleDefinition::~AvoidGamesRuleDefinition()
    {
    }

    bool AvoidGamesRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {       
        const AvoidGamesRuleConfig& avoidGamesRuleConfig = matchmakingServerConfig.getRules().getPredefined_AvoidGamesRule();
        if(!avoidGamesRuleConfig.isSet())
        {
            WARN_LOG("[AvoidGamesRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // init game id lists max size
        mMaxGameIdListSize = avoidGamesRuleConfig.getMaxGameIdListSize();
        if (mMaxGameIdListSize == 0)
        {
            WARN_LOG("[AvoidGamesRuleDefinition] Rule configured but disabled. Max game id lists size of 0 was specified.");
            return false;
        }

        // rule does not have weight
        uint32_t weight = 0;

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, weight);

        return true;
    }

    void AvoidGamesRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void AvoidGamesRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

