/*! ************************************************************************************************/
/*!
    \file   hostviabilityruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/hostviabilityruledefinition.h"
#include "gamemanager/matchmaker/rules/hostviabilityrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(HostViabilityRuleDefinition, "Predefined_HostViabilityRule", RULE_DEFINITION_TYPE_SINGLE);
    const float HostViabilityRuleDefinition::CONNECTION_ASSURED     = 1.0f;
    const float HostViabilityRuleDefinition::CONNECTION_LIKELY      = 0.75f;
    const float HostViabilityRuleDefinition::CONNECTION_FEASIBLE    = 0.5f;
    const float HostViabilityRuleDefinition::CONNECTION_UNLIKELY    = 0.25f;
    
    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized HostViabilityRuleDefinition.  NOTE: do not use until successfully
            initialized & at least 1 minFitThresholdList has been added.
    *************************************************************************************************/
    HostViabilityRuleDefinition::HostViabilityRuleDefinition()
    {
        mMinFitThresholdListContainer.setThresholdAllowedValueType(ALLOW_LITERALS);
        insertLiteralValueType("CONNECTION_ASSURED",  HostViabilityRuleDefinition::CONNECTION_ASSURED);
        insertLiteralValueType("CONNECTION_LIKELY",   HostViabilityRuleDefinition::CONNECTION_LIKELY);
        insertLiteralValueType("CONNECTION_FEASIBLE", HostViabilityRuleDefinition::CONNECTION_FEASIBLE);
        insertLiteralValueType("CONNECTION_UNLIKELY", HostViabilityRuleDefinition::CONNECTION_UNLIKELY);
    }

     bool HostViabilityRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
     {
         const HostViabilityRuleConfig& ruleConfig = matchmakingServerConfig.getRules().getPredefined_HostViabilityRule();
         if (!ruleConfig.isSet())
         {
             ERR_LOG("[HostViabilityRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
             return false;
         }

         return RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists());
     }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
