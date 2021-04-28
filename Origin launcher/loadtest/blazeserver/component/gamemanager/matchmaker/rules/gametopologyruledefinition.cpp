/*! ************************************************************************************************/
/*!
    \file   GameTopologyRuleDefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/gametopologyruledefinition.h"
#include "gamemanager/matchmaker/rules/gametopologyrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameTopologyRuleDefinition, "Predefined_GameTopologyRule", RULE_DEFINITION_TYPE_SINGLE); 

    const char8_t* GameTopologyRuleDefinition::GAME_NETWORK_TOPOLOGY_RETE_KEY = "GameNetworkTopology";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized GameTopologyRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    GameTopologyRuleDefinition::GameTopologyRuleDefinition()
    {
    }

    GameTopologyRuleDefinition::~GameTopologyRuleDefinition() 
    {
    }

    bool GameTopologyRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 0);
        return  true;
    }

    void GameTopologyRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

    void GameTopologyRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Topology check:
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            GAME_NETWORK_TOPOLOGY_RETE_KEY, getWMEInt32AttributeValue((int32_t)gameSessionSlave.getGameNetworkTopology()), *this);
    }

    void GameTopologyRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        insertWorkingMemoryElements(wmeManager, gameSessionSlave);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
