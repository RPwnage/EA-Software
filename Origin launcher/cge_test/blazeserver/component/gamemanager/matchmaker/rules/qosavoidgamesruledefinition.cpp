/*! ************************************************************************************************/
/*!
    \file   qosavoidgamesruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/qosavoidgamesruledefinition.h"
#include "gamemanager/matchmaker/rules/qosavoidgamesrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(QosAvoidGamesRuleDefinition, "Predefined_QosAvoidGamesRule", RULE_DEFINITION_TYPE_SINGLE);

    QosAvoidGamesRuleDefinition::QosAvoidGamesRuleDefinition() : mMaxGameIdListSize(0)
    {
    }
    QosAvoidGamesRuleDefinition::~QosAvoidGamesRuleDefinition()
    {
    }

    bool QosAvoidGamesRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {       
        const QosValidationRuleConfig& qosValidatonRuleConfig = matchmakingServerConfig.getRules().getQosValidationRule();
        if(!qosValidatonRuleConfig.isSet())
        {
            WARN_LOG("[QosAvoidGamesRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // init game id lists max size
         mMaxGameIdListSize = qosValidatonRuleConfig.getMaxAvoidListSize();
        if (mMaxGameIdListSize == 0)
        {
            WARN_LOG("[QosAvoidGamesRuleDefinition] Rule configured but disabled. Max avoid list size of 0 was specified.");
            return false;
        }

        // rule does not have weight
        uint32_t weight = 0;

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, weight);

        return true;
    }

    void QosAvoidGamesRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void QosAvoidGamesRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

