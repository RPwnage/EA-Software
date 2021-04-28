/*! ************************************************************************************************/
/*!
    \file   qosavoidplayersruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/qosavoidplayersruledefinition.h"
#include "gamemanager/matchmaker/rules/qosavoidplayersrule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(QosAvoidPlayersRuleDefinition, "Predefined_QosAvoidPlayersRule", RULE_DEFINITION_TYPE_SINGLE);

    QosAvoidPlayersRuleDefinition::QosAvoidPlayersRuleDefinition() 
        : PlayersRuleDefinition()
    {
    }
    QosAvoidPlayersRuleDefinition::~QosAvoidPlayersRuleDefinition()
    {
    }

    bool QosAvoidPlayersRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const QosValidationRuleConfig& qosValidatonRuleConfig = matchmakingServerConfig.getRules().getQosValidationRule();
        if(!qosValidatonRuleConfig.isSet())
        {
            WARN_LOG("[QosAvoidPlayersRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }
        // validate every entry in the validation criteria map correctly configured
        for (QosValidationCriteriaMap::const_iterator itr = qosValidatonRuleConfig.getConnectionValidationCriteriaMap().begin(),
            end = qosValidatonRuleConfig.getConnectionValidationCriteriaMap().end(); itr != end; ++itr)
        {
            if (itr->second->getQosCriteriaList().empty())
            {
                ERR_LOG("[QosAvoidPlayersRuleDefinition] Rule " << getConfigKey() << " configured but disabled. Connection validation criteria for topology " << Blaze::GameNetworkTopologyToString(itr->first) << " has empty or missing qosCriteriaList.");
                return false;
            }
        }

        setMaxPlayersUsed(qosValidatonRuleConfig.getMaxAvoidListSize());
        if (getMaxPlayersUsed() == 0)
        {
            WARN_LOG("[QosAvoidPlayersRuleDefinition] Rule configured but disabled. Max avoid list size of 0 was specified.");
            return false;
        }

        if (!PlayersRuleDefinition::initialize(name, salience, matchmakingServerConfig))
        {
            WARN_LOG("[QosAvoidPlayersRuleDefinition] Failed PlayersRuleDefinition initialziation.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        const uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);

        return true;
    }

    void QosAvoidPlayersRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    void QosAvoidPlayersRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

