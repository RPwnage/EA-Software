/*! ************************************************************************************************/
/*!
\file playerattributeevaluator.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PLAYERATTRIBUTEEVALUATOR_H
#define BLAZE_GAMEMANAGER_PLAYERATTRIBUTEEVALUATOR_H

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
    class AttributeRuleDefinition;

    class PlayerAttributeEvaluator
    {
    public:

        /*! ************************************************************************************************/
        /*!
            \brief This method should insert any WME's from the DataProvider.

            \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
            \param[in/out] dataProvider - The Data provider used to create the WMEs.
        *************************************************************************************************/
        static void insertWorkingMemoryElements(const PlayerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave);

        /*! ************************************************************************************************/
        /*!
            \brief This method should insert/update any WME's from the DataProvider that have changed.
        
            \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
            \param[in/out] dataProvider - The Data provider used to create the WMEs.
        
            \return 
        *************************************************************************************************/
        static void upsertWorkingMemoryElements(const PlayerAttributeRuleDefinition& attributeRuleDefinition, GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave);

    // Members
    private:
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PLAYERATTRIBUTEEVALUATOR_H
