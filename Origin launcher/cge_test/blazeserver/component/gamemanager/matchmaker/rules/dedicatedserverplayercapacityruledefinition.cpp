/*! ************************************************************************************************/
/*!
    \file   DedicatedServerPlayerCapacityRuleDefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityruledefinition.h"
#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(DedicatedServerPlayerCapacityRuleDefinition, "Predefined_DedicatedServerPlayerCapacityRule", RULE_DEFINITION_TYPE_SINGLE); 

    const char8_t* DedicatedServerPlayerCapacityRuleDefinition::MIN_CAPACITY_RETE_KEY = "MinCapacity";
    const char8_t* DedicatedServerPlayerCapacityRuleDefinition::MAX_CAPACITY_RETE_KEY = "MaxCapacity";
    const char8_t* DedicatedServerPlayerCapacityRuleDefinition::GAME_NETWORK_TOPOLOGY_RETE_KEY = "GameNetworkTopology";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized DedicatedServerPlayerCapacityRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    DedicatedServerPlayerCapacityRuleDefinition::DedicatedServerPlayerCapacityRuleDefinition()
        : mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    DedicatedServerPlayerCapacityRuleDefinition::~DedicatedServerPlayerCapacityRuleDefinition() 
    {
    }

    bool DedicatedServerPlayerCapacityRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 1000);

        cacheWMEAttributeName(MIN_CAPACITY_RETE_KEY);
        cacheWMEAttributeName(MAX_CAPACITY_RETE_KEY);


        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();
        return true;
    }

    void DedicatedServerPlayerCapacityRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

    void DedicatedServerPlayerCapacityRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Only add WMEs for games that use a dedicated server topology, and aren't virtualized:
        if (!isDedicatedHostedTopology(gameSessionSlave.getGameNetworkTopology()) || gameSessionSlave.getGameSettings().getVirtualized())
            return;

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), MIN_CAPACITY_RETE_KEY, gameSessionSlave.getMinPlayerCapacity(), *this);
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), MAX_CAPACITY_RETE_KEY, gameSessionSlave.getMaxPlayerCapacity(), *this);
    }

    void DedicatedServerPlayerCapacityRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Capacities don't change, should never happen.
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
