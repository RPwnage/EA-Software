/*! ************************************************************************************************/
/*!
\file gameattributeevaluator.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEATTRIBUTEEVALUATOR_H
#define BLAZE_GAMEMANAGER_GAMEATTRIBUTEEVALUATOR_H

#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
}
namespace GameManager
{    

namespace Rete
{
    class WMEManager;
}

namespace Matchmaker
{
    class GameAttributeRuleDefinition;

    class GameAttributeEvaluator
    {
    public:
        /*! ************************************************************************************************/
        /*!
            \brief This method should insert any WME's from the DataProvider.

            \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
            \param[in/out] dataProvider - The Data provider used to create the WMEs.
        *************************************************************************************************/
        static void insertWorkingMemoryElements(const GameAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave);

        /*! ************************************************************************************************/
        /*!
            \brief This method should insert/update any WME's from the DataProvider that have changed.
        
            \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
            \param[in/out] dataProvider - The Data provider used to create the WMEs.
        
            \return 
        *************************************************************************************************/
        static void upsertWorkingMemoryElements(const GameAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave);

    private:
        static void validateGameAttributeWmeValue(const GameAttributeRuleDefinition& attributeRuleDefinition, const Collections::AttributeValue& value, GameId gameId, GameState gameState);
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMEATTRIBUTEEVALUATOR_H
