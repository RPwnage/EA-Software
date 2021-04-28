/*! ************************************************************************************************/
/*!
\file   exampleupstreambpsruledefinition.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gameprotocolversionrule.h"
#include "gamemanager/matchmaker/rules/gameprotocolversionruledefinition.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameProtocolVersionRuleDefinition, "gameProtocolVersionRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* GameProtocolVersionRuleDefinition::GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME = "Version";

    GameProtocolVersionRuleDefinition::GameProtocolVersionRuleDefinition() 
    { 
        eastl::hash<const char8_t*> stringHash;
        mMatchAnyHash = stringHash(GAME_PROTOCOL_VERSION_MATCH_ANY);
    }

    GameProtocolVersionRuleDefinition::~GameProtocolVersionRuleDefinition() { }

    void GameProtocolVersionRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), 
            GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME, getWMEAttributeValue(gameSessionSlave.getGameProtocolVersionString()), *this);
    }
    
    void GameProtocolVersionRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), 
            GAME_PROTOCOL_VERSION_RULE_ATTRIBUTE_NAME, getWMEAttributeValue(gameSessionSlave.getGameProtocolVersionString()), *this);
    }

    bool GameProtocolVersionRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // rule does not have a configuration.
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);
        return true;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
