/*! ************************************************************************************************/
/*!
    \file   hostviabilityruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_HOST_VIABILITY_RULE_DEFINITION
#define BLAZE_MATCHMAKING_HOST_VIABILITY_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \class HostViabilityRuleDefinition
        \brief RuleDefinitions are initialized automatically from the matchmaking config file, and define
                a rule's name & minFitThresholdLists.

        Note: unlike other matchmaking rule definitions, the HostViabilityRule is unweighted and only contains
            minFitThreshold lists.

            This is because the rule doesn't affect a game (or player's) totalFitScore, this rule
                only affects when a createGame matchmaking session can finalize itself.
    *************************************************************************************************/
    class HostViabilityRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(HostViabilityRuleDefinition);
        DEFINE_RULE_DEF_H(HostViabilityRuleDefinition, HostViabilityRule);
    public:
        static const float CONNECTION_ASSURED;  
        static const float CONNECTION_LIKELY;   
        static const float CONNECTION_FEASIBLE;
        static const float CONNECTION_UNLIKELY; 

        /*! ************************************************************************************************/
        /*!
            \brief Construct a HostViabilityRuleDefinition.  NOTE: rule must contain at least 1 minFitThresholdList
                before use.
        *************************************************************************************************/
        HostViabilityRuleDefinition();

        //! destructor
        ~HostViabilityRuleDefinition() override {}

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override {}

        bool isReteCapable() const override { return false; }
        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[HostViabilityRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED YET.");
        }
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[HostViabilityRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED YET.");
        }


    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_HOST_VIABILITY_RULE_DEFINITION
