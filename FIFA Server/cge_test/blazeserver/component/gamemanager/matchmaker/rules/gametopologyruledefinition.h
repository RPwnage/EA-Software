/*! ************************************************************************************************/   
/*!
    \file   gametopologyruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE_DEFINITION
#define BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for finding a game that matches the desired topology.
        
    ***************************************************************************************************/
    class GameTopologyRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameTopologyRuleDefinition);
        DEFINE_RULE_DEF_H(GameTopologyRuleDefinition, GameTopologyRule);
    public:
        static const char8_t* GAME_NETWORK_TOPOLOGY_RETE_KEY;

        GameTopologyRuleDefinition();

        //! \brief destructor
        ~GameTopologyRuleDefinition() override;

        bool isDisabled() const override { return false; }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        //
        // From GameReteRuleDefinition
        //

        bool isReteCapable() const override { return true; }
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        // End from GameReteRuleDefintion

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE_DEFINITION
