/*! ************************************************************************************************/
/*!
\file dedicatedserverattributeevaluator.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "dedicatedserverattributeevaluator.h"

#include "gamemanager/matchmaker/rules/dedicatedserverattributeruledefinition.h"
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

            void DedicatedServerAttributeEvaluator::insertWorkingMemoryElements(const DedicatedServerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
            {
                // Only add WMEs for games that use a dedicated server topology, and aren't virtualized:
                if (!isDedicatedHostedTopology(gameSessionSlave.getGameNetworkTopology()))
                    return;

                const Collections::AttributeMap& attributeMap = gameSessionSlave.getDedicatedServerAttribs();

                // Find the attribute for this rule and insert it. Use default if not found.
                const char8_t* name = attributeRuleDefinition.getAttributeName();
                Collections::AttributeValue value = attributeRuleDefinition.getDefaultValue();

                Collections::AttributeMap::const_iterator itr = attributeMap.find(attributeRuleDefinition.getAttributeName());
                if (itr != attributeMap.end())
                {
                    name = itr->first;
                    value = itr->second;
                }

                validateDedicatedServerAttributeWmeValue(attributeRuleDefinition, value, gameSessionSlave.getGameId(), gameSessionSlave.getGameState());
                wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), name, attributeRuleDefinition.getWMEAttributeValue(value), attributeRuleDefinition);
            }

            void DedicatedServerAttributeEvaluator::upsertWorkingMemoryElements(const DedicatedServerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
            {
                // Only add WMEs for games that use a dedicated server topology, and aren't virtualized:
                if (!isDedicatedHostedTopology(gameSessionSlave.getGameNetworkTopology()) || gameSessionSlave.getGameSettings().getVirtualized())
                    return;

                const Collections::AttributeMap& attributeMap = gameSessionSlave.getDedicatedServerAttribs();

                // Find the attribute for this rule and upsert it. Use default if not found.
                Collections::AttributeMap::const_iterator itr = attributeMap.find(attributeRuleDefinition.getAttributeName());
                const Collections::AttributeValue& value = (itr != attributeMap.end()) ? itr->second : attributeRuleDefinition.getDefaultValue();

                validateDedicatedServerAttributeWmeValue(attributeRuleDefinition, value, gameSessionSlave.getGameId(), gameSessionSlave.getGameState());
                wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), attributeRuleDefinition.getAttributeName(), attributeRuleDefinition.getWMEAttributeValue(value.c_str()), attributeRuleDefinition);
            }

            // log warning if game's attribute has a value not in an explicit rule's possible values
            void DedicatedServerAttributeEvaluator::validateDedicatedServerAttributeWmeValue(const DedicatedServerAttributeRuleDefinition& definition, const Collections::AttributeValue& value, GameId gameId, GameState gameState)
            {
                if ((definition.getAttributeRuleType() == EXPLICIT_TYPE) && (definition.getPossibleValueIndex(value) == DedicatedServerAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX))
                {
                    WARN_LOG("[DedicatedServerAttributeEvaluator].validateDedicatedServerAttributeWmeValue: game(" << gameId << ") attribute(" << definition.getAttributeName() << ") has value(" << value.c_str() << ") which is not in associated rule(" << ((definition.getName() != nullptr) ? definition.getName() : "<nullptr>") << ")'s configured possible values. The rule as currently configured cannot match this value.");
                }
            }

        } // namespace Matchmaker
    } // namespace GameManager
} // namespace Blaze
