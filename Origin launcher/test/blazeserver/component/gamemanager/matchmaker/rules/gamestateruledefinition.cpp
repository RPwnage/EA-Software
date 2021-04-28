/*! ************************************************************************************************/
/*!
\file gamestateruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gamestateruledefinition.h"
#include "gamemanager/matchmaker/rules/gamestaterule.h"
#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameStateRuleDefinition, "Predefined_GameStateRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* GameStateRuleDefinition::GAMESTATERULE_RETE_KEY = "GameState";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameStateRuleDefinition::GameStateRuleDefinition()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameStateRuleDefinition::~GameStateRuleDefinition()
    {
    }

    bool GameStateRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // parses common configuration.  0 weight, this rule does not effect the final result.
        RuleDefinition::initialize(name, salience, 0);

        cacheWMEAttributeName(GAMESTATERULE_RETE_KEY);
        return true;
    }

    void GameStateRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {

    }

    void GameStateRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameManager::GameState gameState = gameSessionSlave.getGameState();

        const char8_t* gameStateStr = GameManager::GameStateToString(gameState);

        TRACE_LOG("[GameStateRuleDefinition].insertWorkingMemoryElements game " << gameSessionSlave.getGameId() << " in state " << gameStateStr);

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), getGameStateAttributeNameStr(), getGameStateAttributeValue(gameState), *this);
    }

    void GameStateRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameManager::GameState gameState = gameSessionSlave.getGameState();
        const char8_t* gameStateStr = GameManager::GameStateToString(gameState);

        TRACE_LOG("[GameStateRuleDefinition].upsertWorkingMemoryElements game " << gameSessionSlave.getGameId() << " in state " << gameStateStr);

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), getGameStateAttributeNameStr(), getGameStateAttributeValue(gameState), *this);
    }


    Rete::WMEAttribute GameStateRuleDefinition::getGameStateAttributeValue(const GameManager::GameState& gameState) const
    {
        return RuleDefinition::getWMEAttributeValue(GameManager::GameStateToString(gameState));
    }

    Rete::WMEAttribute GameStateRuleDefinition::getNoMatchAttributeValue() const
    {
        return RuleDefinition::getWMEAttributeValue("NO_MATCH");
    }
    
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
