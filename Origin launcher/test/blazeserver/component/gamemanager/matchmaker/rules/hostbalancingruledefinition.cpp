/*! ************************************************************************************************/
/*!
    \file   hostbalancingruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/networktopologycommoninfo.h" // for isPlayerHostedTopology
#include "gamemanager/matchmaker/rules/hostbalancingruledefinition.h"
#include "gamemanager/matchmaker/rules/hostbalancingrule.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(HostBalancingRuleDefinition, "Predefined_HostBalancingRule", RULE_DEFINITION_TYPE_SINGLE);
    // const definition for threshold fitscore defined in config file
    const float HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED    = 1.0f;
    const float HostBalancingRuleDefinition::HOSTS_BALANCED = 0.5f;
    const float HostBalancingRuleDefinition::HOSTS_UNBALANCED       = 0.0f;
    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized HostBalancingRuleDefinition.  NOTE: do not use until successfully
            initialized & at least 1 minFitThresholdList has been added.
    *************************************************************************************************/
    HostBalancingRuleDefinition::HostBalancingRuleDefinition()
    {
        mMinFitThresholdListContainer.setThresholdAllowedValueType(ALLOW_LITERALS);
        insertLiteralValueType("HOSTS_STRICTLY_BALANCED", HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED);
        insertLiteralValueType("HOSTS_BALANCED", HostBalancingRuleDefinition::HOSTS_BALANCED);
        insertLiteralValueType("HOSTS_UNBALANCED", HostBalancingRuleDefinition::HOSTS_UNBALANCED);
    }

    bool HostBalancingRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        const HostBalancingRuleConfig& ruleConfig = matchmakingServerConfig.getRules().getPredefined_HostBalancingRule();
        if(!ruleConfig.isSet())
        {
            WARN_LOG("[HostBalancingRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        return RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists());
    }

    /*! ************************************************************************************************/
    /*!
        \brief calculates fit percentage based on current cached NatType versus targeted NatType (taking
            the game's network topology into account).

        \param[in]  otherNatType The NatType being compared against.
        \return the UN-weighted fitScore for this rule. Will not return a negative fit score
    *************************************************************************************************/
    float HostBalancingRuleDefinition::getFitPercent(GameNetworkTopology networkTopology, Util::NatType natType, Util::NatType otherNatType) const
    {
        // MM_AUDIT: shouldn't this check that the network topologies are compatible?

        // MM_TODO: consolidate this rule with Host Viability when API removal restriction is lifted

        // the idea here is to provide a perfect fit when comparing good hosts to bad hosts.
        // this is an attempt to keep bad hosts (NAT_TYPE_STRICT, STRICT_SEQUENTIAL, UNKNOWN) 
        // from causing or having a bad game experience (matching to games they won't connect to)
        // by rationing "good" hosts (NAT_TYPE_OPEN, MODERATE) and preventing similar hosts from 
        // grouping together when there are other players available
        
        if (!isPlayerHostedTopology(networkTopology))
        {
            // hostBalancing is irrelevant for dedicated servers (we only connect to the ded server and it's open),
            //  so lie and act as if we're perfectly balanced.
            return HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED;
        }
        
        if (networkTopology == PEER_TO_PEER_FULL_MESH)
        {
            if (natType == Util::NAT_TYPE_OPEN || otherNatType == Util::NAT_TYPE_OPEN)
            {
                // if either of us is open, consider us balanced
                return HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED;
            }
            else if (natType == Util::NAT_TYPE_MODERATE || otherNatType == Util::NAT_TYPE_MODERATE)
            {
                // if either of us is balanced, consider us mixed
                return HostBalancingRuleDefinition::HOSTS_BALANCED;
            }
            else
            {
                // otherwise, we're unbalanced
                return HostBalancingRuleDefinition::HOSTS_UNBALANCED;
            }
        }

        // below is for CLIENT_SERVER_PEER_HOSTED

        bool iAmGoodHost = (natType <= Util::NAT_TYPE_MODERATE); // good or moderate
        bool otherIsGoodHost = (otherNatType <= Util::NAT_TYPE_MODERATE); // good or moderate

        if (iAmGoodHost)
        {
            if (!otherIsGoodHost)
            {
                // good hosts prefer bad hosts
                return HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED;
            }
            else
            {
                // good hosts avoid other good hosts
                return HostBalancingRuleDefinition::HOSTS_BALANCED;
            }
        }
        else
        {
            if (otherIsGoodHost)
            {
                // bad hosts prefer matching with good hosts
                return HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED;
            }
            else
            {
                // we're both bad hosts - this would be a terrible match
                return HostBalancingRuleDefinition::HOSTS_UNBALANCED;
            }
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
