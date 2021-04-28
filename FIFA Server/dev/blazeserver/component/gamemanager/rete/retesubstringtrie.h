/*! ************************************************************************************************/
/*!
    \file retesubstringtrie.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETESUBSTRINGTRIE_H
#define BLAZE_GAMEMANAGER_RETESUBSTRINGTRIE_H

#include "gamemanager/rete/wmemanager.h" // for WMEManager mWMEManager in addWMEsFromFindMatches
#include "gamemanager/rete/retetypes.h" // for WorkingMemoryElmement in addSubstringFacts()
#include "gamemanager/rete/stringtrie.h" // for StringTrie

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class WMEManager;

    typedef eastl::vector_set<WMEId> SubstringMatchCache;
    /*! ************************************************************************************************/
    /*! \brief map of different substring's cached search results
    ***************************************************************************************************/
    class SubstringMatchCacheMap
    {
        NON_COPYABLE(SubstringMatchCacheMap);
    public:
        /*! \param[in] searchStringTable - to get or make string's numeric key for map */
        SubstringMatchCacheMap(StringTable& searchStringTable) : mSubstringMatchCacheMap(BlazeStlAllocator("mSubstringMatchCacheMap")), mSearchStringTable(searchStringTable) {}
        inline SubstringMatchCache* findMatchCacheForSubstring(const char8_t* substring) const;
        SubstringMatchCache& getOrCreateMatchCacheForSubstring(const char8_t* substring) { return mSubstringMatchCacheMap[getOrMakeMapKeyForString(substring)]; }
        inline void destroyMatchCacheForSubstring(const char8_t *searchSubstring);
    private:
        typedef eastl::hash_map<WMEAttribute, SubstringMatchCache> CacheMap;
        CacheMap mSubstringMatchCacheMap;
        StringTable& mSearchStringTable;
        WMEAttribute getOrMakeMapKeyForString(const char8_t *substring) { return mSearchStringTable.reteHash(substring); }
        bool findMapKeyForString(const char8_t* searchSubstring, uint64_t& key) const { return mSearchStringTable.findHash(searchSubstring, key); }
    };

    typedef eastl::list<JoinNode*> JoinNodeList;
    /*! ************************************************************************************************/
    /*! \brief map of join nodes listening for updates from a substring's set of matches 
    ***************************************************************************************************/
    class SubstringSearchersMap
    {
        NON_COPYABLE(SubstringSearchersMap);
    public:
        /*! \param[in] searchStringTable - to get or make string's numeric key for map */
        SubstringSearchersMap(StringTable& searchStringTable) : mSubstringListenersMap(BlazeStlAllocator("mSubstringListenersMap")), mSearchStringTable(searchStringTable) {}
        inline bool hasListeningJoinNodesForSubstring(const char8_t* substring);
        JoinNodeList& getOrCreateJoinNodeListForSubstring(const char8_t* substring) { return mSubstringListenersMap[getOrMakeMapKeyForString(substring)]; }
        inline void destroyJoinNodeListForSubstring(const char8_t *searchSubstring);
    private:
        typedef eastl::hash_map<WMEAttribute, JoinNodeList> SubtringListenersMap;
        SubtringListenersMap mSubstringListenersMap;
        StringTable& mSearchStringTable;
        WMEAttribute getOrMakeMapKeyForString(const char8_t *substring) { return mSearchStringTable.reteHash(substring); }
        bool findSubstringListenersMapKey(const char8_t* searchSubstring, uint64_t& key) const { return mSearchStringTable.findHash(searchSubstring, key); }
    };


    /*! ************************************************************************************************/
    /*! \brief Manages mapping between wme id with its substrings fact values, stored compactly here
        via character-trie-based data structure.
    ***************************************************************************************************/
    class ReteSubstringTrie
    {
        NON_COPYABLE(ReteSubstringTrie);
    public:
        typedef Blaze::StringTrie::StringNormalizationTable NormalizationTable;

        /*! ************************************************************************************************/
        /*! \brief ctor
            \param[in] searchStringTable - to get or make string's numeric key for my member maps.
                For memory cleanup, we'll destroy a term's hash/entry in searches string table, when
                unregistering term's last JoinNode (pre: no clients need hash/entry after cleanup)
        ***************************************************************************************************/
        ReteSubstringTrie(StringTable& searchStringTable, WMEManager& wmeManager) 
            :mSearchersMap(searchStringTable), mMatchCacheMap(searchStringTable),
            mSearchStringTable(searchStringTable), mWMEManager(wmeManager), mTrie(nullptr) {}
        ~ReteSubstringTrie();
        void configure(uint32_t minLength, uint32_t maxLength, const NormalizationTable& stringNormalizationTable);
        
        void addSubstringFacts(const WorkingMemoryElement& wme, const char8_t* stringKey);
        void removeSubstringFacts(const WorkingMemoryElement& wme, const char8_t* stringKey);
        void registerListener(const char8_t* searchSubstring, JoinNode& joinNode);
        void unregisterListener(const char8_t* searchSubstring, const JoinNode& joinNode);

        void addWMEsFromFindMatches(const char8_t* searchSubstring, JoinNode& joinNode) const;
        bool substringMatchesWME(const char8_t* searchSubstring, const WMEId id) const;

    private:
        void notifyJoinNodesOfWmeUpdate(const WorkingMemoryElement& wme, bool doRemoveInsteadOfAdd, const char8_t* stringKey);
        uint32_t addOrRemoveWmeFromJoinNodes(const WorkingMemoryElement& wme, bool doRemoveInsteadOfAdd, char8_t* substring2, uint32_t substring2Len);
        bool findOneOrAllMatches(const char8_t* searchSubstring, const WMEId* oneWmeToFind, JoinNode* joinNodeForAllMatches) const;
        void addWmeFromFindMatches(const WMEId id, JoinNode& joinNode) const;

    private:
        // map of join nodes listening for updates from a substring's set of matches (trie node)
        SubstringSearchersMap mSearchersMap;

        // map of cached matches for searched substrings
        SubstringMatchCacheMap mMatchCacheMap;
        
        // to clean string's numeric hash/entry when no JoinNodes/searchers using
        StringTable& mSearchStringTable;
        
        WMEManager& mWMEManager;

        // implementation
        Blaze::StringTrie* mTrie;
    };

    /*! ************************************************************************************************/
    /*! \brief Used to track when word string counts drop to 0, so system can reclaim its hash memory
        (avoid unbounded string table growth)
    ***************************************************************************************************/
    class ReteSubstringTrieWordCounter
    {
    public:
        ReteSubstringTrieWordCounter() : mWordCountMap(BlazeStlAllocator("mWordCountMap")) {}
        uint32_t incrementWordCount(uint64_t wordHash);
        uint32_t decrementWordCount(uint64_t wordHash);
    private:
        typedef eastl::hash_map<WMEAttribute, uint32_t> WordCountMap;
        WordCountMap mWordCountMap;
    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze

// BLAZE_GAMEMANAGER_RETESUBSTRINGTRIE_H
#endif

