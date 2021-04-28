/*! ************************************************************************************************/
/*!
\file playerattributeevaluator.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/matchmaker/rules/playerattributeevaluator.h"

#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    //////////////////////////////////////////////////////////////////////////
    // GameReteRuleDefinition Functions
    //////////////////////////////////////////////////////////////////////////

    void PlayerAttributeEvaluator::insertWorkingMemoryElements(const PlayerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
    {
        // rule not currently RETE capable
    }

    void PlayerAttributeEvaluator::upsertWorkingMemoryElements(const PlayerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave)
    {
        // rule not currently RETE capable
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
