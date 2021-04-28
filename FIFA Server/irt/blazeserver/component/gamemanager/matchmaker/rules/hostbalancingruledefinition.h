/*! ************************************************************************************************/
/*!
    \file   hostbalancingruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_HOST_BALANCING_RULE_DEFINITION
#define BLAZE_MATCHMAKING_HOST_BALANCING_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \class HostBalancingRuleDefinition
        \brief RuleDefinitions are initialized automatically from the matchmaking config file, and define
                a rule's name, and weight
    *************************************************************************************************/
    class HostBalancingRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(HostBalancingRuleDefinition);
        DEFINE_RULE_DEF_H(HostBalancingRuleDefinition, HostBalancingRule);
    public:

        // the fitThresholds for the 3 tiers of balancing
        static const float HOSTS_STRICTLY_BALANCED;
        static const float HOSTS_BALANCED;
        static const float HOSTS_UNBALANCED;

        /*! ************************************************************************************************/
        /*!
            \brief Construct an uninitialized HostBalancingRuleDefinition.  NOTE: do not use until successfully
                initialized & at least 1 minFitThresholdList has been added.
        *************************************************************************************************/
        HostBalancingRuleDefinition();

        //! destructor
        ~HostBalancingRuleDefinition() override {}

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override {}

        bool isReteCapable() const override { return false; }
        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[HostBalancingRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED YET.");
        }
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[HostBalancingRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED YET.");
        }

        
        /*! ************************************************************************************************/
        /*!
            \brief calculates fit percentage based on current cached NatType versus targeted NatType
        
            \param[in]  otherValue The NatType being compared against.
            \return the UN-weighted fitScore for this rule. Will not return a negative fit score
        *************************************************************************************************/
        float getFitPercent(GameNetworkTopology networkTopology, Util::NatType natType, Util::NatType otherNatType) const;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_HOST_BALANCING_RULE_DEFINITION
