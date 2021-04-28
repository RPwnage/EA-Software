/*! ************************************************************************************************/
/*!
\file reteruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_RETERULEDEFINITION_H
#include "gamemanager/rete/stringtable.h"
#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/rete/retetypes.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    // an unfortunate side effect of our bad namespacing.
    typedef uint32_t RuleDefinitionId;
    class RuleDefinitionCollection;
}
namespace Rete
{
    class WMEManager;

    class ReteRuleDefinition : public ConditionProviderDefinition
    {   
    public:
        ReteRuleDefinition() {}
        ~ReteRuleDefinition() override {}

        void setRuleDefinitionCollection(GameManager::Matchmaker::RuleDefinitionCollection& ruleDefinitionCollection);
        GameManager::Matchmaker::RuleDefinitionCollection* getRuleDefinitionCollection() const { return mRuleDefinitionCollection; }

        Matchmaker::RuleDefinitionId getID() const override = 0;
        const char8_t* getName() const override = 0;
        WMEAttribute getCachedWMEAttributeName(const char8_t* attributeName) const override = 0;

        uint64_t reteHash(const char8_t* str, bool garbageCollectable = false) const override;
        const char8_t* reteUnHash(WMEAttribute value) const override;

    protected:
        StringTable& getStringTable() const;

        bool isInitialized() const { return mRuleDefinitionCollection != nullptr; }

    // Members
    protected:
        GameManager::Matchmaker::RuleDefinitionCollection* mRuleDefinitionCollection = nullptr;
    };


    template <class DataProvider>
    class WMEProvider : public ReteRuleDefinition
    {   
    public:
        /*! ************************************************************************************************/
        /*!
        \brief This method should insert any WME's from the DataProvider.

        \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
        \param[in/out] dataProvider - The Data provider used to create the WMEs.
        *************************************************************************************************/
        virtual void insertWorkingMemoryElements(WMEManager& wmeManager, const DataProvider& dataProvider) const = 0;

        /*! ************************************************************************************************/
        /*!
        \brief This method should insert/update any WME's from the DataProvider that have changed.

        \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
        \param[in/out] dataProvider - The Data provider used to create the WMEs.
        *************************************************************************************************/
        virtual void upsertWorkingMemoryElements(WMEManager& wmeManager, const DataProvider& dataProvider) const = 0;

    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_RETERULEDEFINITION_H
