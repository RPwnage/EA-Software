/*! ************************************************************************************************/
/*!
\file wmemanager.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_WMEMANAGER_H
#define BLAZE_GAMEMANAGER_WMEMANAGER_H

#include "gamemanager/rete/retetypes.h"
#include "gamemanager/rete/stringtable.h"
#include "gamemanager/rete/retenetwork.h" // for ReteNetwork in getSubstringTrieWordCounter()


namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    typedef eastl::hash_map<WMEAttribute, WorkingMemoryElement*> WMEValueMap;
    typedef eastl::hash_map<WMEAttribute, WMEValueMap> WmeValueByNameMap;

    // Helper class to track named allocations:
    class WMEMapByNameHelper
    {
    public:
        WMEMapByNameHelper() : mWmeValueByNameMap(BlazeStlAllocator("WMEManager::WmeValueByNameMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)) {}
        WmeValueByNameMap mWmeValueByNameMap;
    };
    typedef eastl::hash_map<WMEId, WMEMapByNameHelper> WorkingMemoryElements;

    class ReteNetwork;
    class ReteSubstringTrieWordCounter; // in getSubstringTrieWordCounter()

    class WMEFactory
    {
    public:
        WMEFactory() : mWMEAllocator("MMAllocatorWME", sizeof(WorkingMemoryElement), GameManagerSlave::COMPONENT_MEMORY_GROUP) {}
        virtual ~WMEFactory() {}

        WorkingMemoryElement& createWorkingMemoryElement()
        {
            return *(new (getWMEAllocator()) WorkingMemoryElement());
        }
        void deleteWorkingMemoryElement(WorkingMemoryElement& wme)
        {
            wme.~WorkingMemoryElement();
            WorkingMemoryElement::operator delete (&wme, getWMEAllocator());
        }

        const NodePoolAllocator& getWMEAllocator() const { return mWMEAllocator; }

    private:
        NodePoolAllocator& getWMEAllocator() { return mWMEAllocator; }

    private:
        NodePoolAllocator mWMEAllocator;
    };

    class WMEManager
    {
        NON_COPYABLE(WMEManager);
    public:
        WMEManager(ReteNetwork* reteNetwork);
        virtual ~WMEManager();

        void registerMatchAnyWME(WMEId id);
        void unregisterMatchAnyWME(WMEId id);
        void unregisterWMEId(WMEId id);

        /*! ************************************************************************************************/
        /*!
            \brief insert/update working memory elements in the WME Manager.
        
            \param[in] id - Unique Id of the entity this attrib is for.
            \param[in] name - The name of the attribute.
            \param[in] value - The value of the attribute.
            \param[in] provider - The condition provider
            \param[in] isGarbageCollectable - if the WMEName is garbagecollectable
        
            \return void
        *************************************************************************************************/
        void upsertWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable = false);
        void insertWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable = false);
        void removeWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable = false);

        void upsertWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider);
        void insertWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider);
        void removeWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider);

        void insertWorkingMemoryElementToSubstringTrie(WMEId id, WMEAttribute name, const char8_t *stringKey);
        void removeWorkingMemoryElementToSubstringTrie(WMEId id, WMEAttribute name, const char8_t *stringKey);
        void removeAllWorkingMemoryElements(WMEId id);

        void removeWMEProvider(WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider);

        const StringTable& getStringTable() const { return mStringTable; }
        StringTable& getStringTable() { return mStringTable; }

        const WMEFactory& getWMEFactory() const { return mWMEFactory; }
        WMEValueMap* getWorkingMemoryElements(WMEId id, WMEAttribute wmeName);
        int32_t getLogCategory() const { return mReteNetwork->getLogCategory(); }

        // Provider for the root node ANY WME
        class WMEAnyProvider : public ConditionProviderDefinition
        {
            NON_COPYABLE(WMEAnyProvider);
        public:
            static const char8_t* WME_ANY_STRING;

            WMEAnyProvider() {};
            ~WMEAnyProvider() override {};

            uint32_t getID() const override { return 0; }
            const char8_t* getName() const override { return WME_ANY_STRING; }
            uint64_t reteHash(const char8_t* str, bool garbageCollectable = false) const override { return WME_ANY_HASH; }
            const char8_t* reteUnHash(WMEAttribute value) const override { return WME_ANY_STRING; }
            WMEAttribute getCachedWMEAttributeName(const char8_t* attributeName) const override { return WME_ANY_HASH; }
        };

        void eraseWorkingMemoryElement(WMEId id, WMEAttribute wmeName, WMEAttribute wmeValue, bool removeFromRete = false, const char8_t* stringKey = nullptr);

    private:
        bool addWorkingMemoryElement(WorkingMemoryElement& wme);
        void removeAllWME(WmeValueByNameMap& map);

        
    // Members
    private:
        ReteNetwork* mReteNetwork;
        WorkingMemoryElements mWorkingMemoryElements;

        StringTable mStringTable;

        WMEFactory mWMEFactory;
    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_WMEMANAGER_H
