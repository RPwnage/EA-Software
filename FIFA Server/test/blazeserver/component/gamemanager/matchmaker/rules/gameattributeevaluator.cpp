/*! ************************************************************************************************/
/*!
\file gameattributeevaluator.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gameattributeevaluator.h"

#include "gamemanager/matchmaker/rules/gameattributeruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    //////////////////////////////////////////////////////////////////////////
    // GameReteRuleDefinition Functions
    //////////////////////////////////////////////////////////////////////////

    void GameAttributeEvaluator::insertWorkingMemoryElements(const GameAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
    {
        const Collections::AttributeMap& attributeMap = gameSessionSlave.getGameAttribs();

        // Find the attribute for this rule and insert it if found.
        Collections::AttributeMap::const_iterator itr = attributeMap.find(attributeRuleDefinition.getAttributeName());

        if (itr != attributeMap.end())
        {
            const Collections::AttributeName& name = itr->first;
            const Collections::AttributeValue& value = itr->second;

            validateGameAttributeWmeValue(attributeRuleDefinition, value, gameSessionSlave.getGameId(), gameSessionSlave.getGameState());
            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), name.c_str(), attributeRuleDefinition.getWMEAttributeValue(value.c_str(), (attributeRuleDefinition.getAttributeRuleType() == ARBITRARY_TYPE)), attributeRuleDefinition);
        }
    }

    void GameAttributeEvaluator::upsertWorkingMemoryElements(const GameAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
    {
        const Collections::AttributeMap& attributeMap = gameSessionSlave.getGameAttribs();
        bool isArbitraryRule = (attributeRuleDefinition.getAttributeRuleType() == ARBITRARY_TYPE);
        // Find the attribute for this rule and upsert it if found.
        Collections::AttributeMap::const_iterator itr = attributeMap.find(attributeRuleDefinition.getAttributeName());
        if (itr != attributeMap.end())
        {
            const Collections::AttributeValue& value = itr->second;
            
            validateGameAttributeWmeValue(attributeRuleDefinition, value, gameSessionSlave.getGameId(), gameSessionSlave.getGameState());
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), attributeRuleDefinition.getAttributeName(), attributeRuleDefinition.getWMEAttributeValue(value.c_str(), isArbitraryRule), attributeRuleDefinition);
        }
        else
        {
            const GameManager::ReplicatedGameDataServer* prevGameSnapshot = gameSessionSlave.getPrevSnapshot();
            if (prevGameSnapshot != nullptr)
            {
                const Collections::AttributeMap& prevAttributeMap = prevGameSnapshot->getReplicatedGameData().getGameAttribs();
                Collections::AttributeMap::const_iterator prevItr = prevAttributeMap.find(attributeRuleDefinition.getAttributeName());
                if (prevItr != prevAttributeMap.end())
                {
                    wmeManager.removeWorkingMemoryElement(gameSessionSlave.getGameId(), prevItr->first.c_str(), attributeRuleDefinition.getWMEAttributeValue(prevItr->second.c_str(), isArbitraryRule), attributeRuleDefinition);
                }
            }
        }
    }

    // log warning if game's attribute has a value not in an explicit rule's possible values
    void GameAttributeEvaluator::validateGameAttributeWmeValue(const GameAttributeRuleDefinition& definition, const Collections::AttributeValue& value, GameId gameId, GameState gameState)
    {
        if ((definition.getAttributeRuleType() == EXPLICIT_TYPE) && (gameState != RESETABLE) && (definition.getPossibleValueIndex(value) == GameAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX))
        {
            WARN_LOG("[GameAttributeEvaluator].validateGameAttributeWmeValue: game(" << gameId << ") attribute(" << definition.getAttributeName() << ") has value(" << value.c_str() << ") which is not in associated rule(" << ((definition.getName() != nullptr) ? definition.getName() : "<nullptr>") << ")'s configured possible values. The rule as currently configured cannot match this value.");
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
