/*! ************************************************************************************************/
/*!
\file GameTypeRuleDefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gametyperuledefinition.h"
#include "gamemanager/matchmaker/rules/gametyperule.h"
#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameTypeRuleDefinition, "Predefined_GameTypeRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* GameTypeRuleDefinition::GAMETYPERULE_RETE_KEY = "GameType";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameTypeRuleDefinition::GameTypeRuleDefinition()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameTypeRuleDefinition::~GameTypeRuleDefinition()
    {
    }

    bool GameTypeRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // parses common configuration.  0 weight, this rule does not effect the final result.
        RuleDefinition::initialize(name, salience, 0);

        return true;
    }

    void GameTypeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {

    }

    void GameTypeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameManager::GameType gameType = gameSessionSlave.getGameType();
        const char8_t* gameTypeStr = GameManager::GameTypeToString(gameType);

        TRACE_LOG("[GameTypeRuleDefinition].insertWorkingMemoryElements game " << gameSessionSlave.getGameId() << " has game type " << gameTypeStr);

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), getGameReportNameStr(), getGameTypeValue(gameType), *this);
    }

    void GameTypeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameManager::GameType gameType = gameSessionSlave.getGameType();
        const char8_t* gameTypeStr = GameManager::GameTypeToString(gameType);

        TRACE_LOG("[GameTypeRuleDefinition].upsertWorkingMemoryElements game " << gameSessionSlave.getGameId() << " has game type " << gameTypeStr);

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), getGameReportNameStr(), getGameTypeValue(gameType), *this);
    }


    Rete::WMEAttribute GameTypeRuleDefinition::getGameTypeValue(const GameManager::GameType& gameType) const
    {
        return getStringTable().reteHash(GameManager::GameTypeToString(gameType));
    }

    Rete::WMEAttribute GameTypeRuleDefinition::getNoMatchValue() const
    {
        return getStringTable().reteHash("NO_MATCH");
    }
    
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
