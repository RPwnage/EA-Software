/*! ************************************************************************************************/
/*!
    \file retestringtrie.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rete/retesubstringtrie.h"
#include "gamemanager/rete/retenetwork.h" // for JoinNode in addOrRemoveWmeFromJoinNodes(), registerListener()

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    ReteSubstringTrie::~ReteSubstringTrie()
    {
        if (mTrie != nullptr)
            delete mTrie;
    }

    /*! ************************************************************************************************/
    /*! \brief setup the structure for storing sub string match facts
        \param[in] minLength - minimum string length allowed
        \param[in] stringNormalizationTable - normalization translation params applied to all strings
    ***************************************************************************************************/
    void ReteSubstringTrie::configure(uint32_t minLength, uint32_t maxLength, 
        const NormalizationTable& stringNormalizationTable)
    {
        if (mTrie != nullptr)
            delete mTrie;

        mTrie = BLAZE_NEW Blaze::StringTrie(minLength, maxLength, stringNormalizationTable, false);
    }

    /*! ************************************************************************************************/
    /*! \brief adds string key's substrings associated to wme id as facts, search-able in trie.
        Notifies join nodes as needed.
        \param[in] stringKey - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    void ReteSubstringTrie::addSubstringFacts(const WorkingMemoryElement& wme, const char8_t* stringKey)
    {
        if ((stringKey == nullptr) || (stringKey[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].addSubstringFacts: null/empty input stringKey: aborted substrings add for id:" << wme.getId());
            return;
        }

        EA_ASSERT(mTrie != nullptr);
        if (!mTrie->addSubstrings(stringKey, wme.getId()))
        {
            return;
        }
        TRACE_LOG("[RETE][ReteSubstringTrie].addSubstringFacts: added substrings-wmeid facts to trie, for: wme(wmename=" << wme.getName() << ",id=" << wme.getId() << ",superstr=" << stringKey  << ")");

        // notify listening join nodes
        const bool doRemove = false;
        notifyJoinNodesOfWmeUpdate(wme, doRemove, stringKey);
    }

    /*! ************************************************************************************************/
    /*! \brief removes the wme string key's substring facts added. Notifies join nodes as needed.
        \param[in] stringKey - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    void ReteSubstringTrie::removeSubstringFacts(const WorkingMemoryElement& wme, const char8_t* stringKey)
    {
        if ((stringKey == nullptr) || (stringKey[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].removeSubstringFacts: null/empty input stringKey: aborted substrings remove for id:" << wme.getId());
            return;
        }

        EA_ASSERT(mTrie != nullptr);
        if (0 == mTrie->removeSubstrings(stringKey, wme.getId()))
        {
            return;
        }
        TRACE_LOG("[RETE][ReteSubstringTrie].removeSubstringFacts: removed substrings-wmeid facts from trie, for: wme(wmename=" << wme.getName() << ",id=" << wme.getId() << ",superstr=" << stringKey  << ")");

        // notify listening join nodes
        const bool doRemove = true;
        notifyJoinNodesOfWmeUpdate(wme, doRemove, stringKey);
    }

    /*! ************************************************************************************************/
    /*! \brief notifies join nodes listening to substrings of the wme's string key, wme updated.
        \param[in] doRemoveInsteadOfAdd - if true notifies of remove. Otherwise notifies of add.
    ***************************************************************************************************/
    void ReteSubstringTrie::notifyJoinNodesOfWmeUpdate(const WorkingMemoryElement& wme, bool doRemoveInsteadOfAdd, const char8_t* stringKey)
    {
        EA_ASSERT(mTrie != nullptr);
        const uint32_t origStringLen = static_cast<uint32_t>(strlen(stringKey));
        char8_t origString[Blaze::STRINGTRIE_MAX_MAXLENGTH];
        blaze_strnzcpy(origString, stringKey, (origStringLen + 1));

        uint32_t numJoinNodesUpdated = 0;
        const uint32_t minLength = mTrie->getMinLength();
        const uint32_t maxj = origStringLen - minLength;
        for (uint32_t j = 0; j <= maxj; ++j)
        {
            // do next substring starting at 'j' (in orig str).
            numJoinNodesUpdated += addOrRemoveWmeFromJoinNodes(wme, doRemoveInsteadOfAdd, &origString[j], origStringLen - j);
        }
        TRACE_LOG("[RETE][ReteSubstringTrie].notifyJoinNodesOfWmeUpdate: " << (doRemoveInsteadOfAdd? "removed wme from" : "added wme to") << " total " << numJoinNodesUpdated << " join nodes currently searching on substrings of: wme(wmename=" << wme.getName() << ",id=" << wme.getId() << ",superstr=" << stringKey << ")");
    }
    /*! ************************************************************************************************/
    /*! \brief helper, enumerates substrings to send notifications to add or remove the wme from
        join nodes listening on the specified substring.

        For efficiency also add/rem wme to a substring's match cache here, just before notifying
        its join nodes

        \param[in] substring2 - do for all prefixes. Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    uint32_t ReteSubstringTrie::addOrRemoveWmeFromJoinNodes(const WorkingMemoryElement& wme, bool doRemoveInsteadOfAdd,
        char8_t* substring2, uint32_t substring2Len)
    {
        // do next substring of length 'i+1'.
        uint32_t numWmeUpdates = 0;
        for (uint32_t i = 0; i < substring2Len; ++i)
        {
            // temp swap origString[i] with null terminator for next substring
            const char8_t tempOrigTailChar = substring2[i + 1];
            substring2[i + 1] = '\0';

            // while at it, update match cache
            SubstringMatchCache* matchCache = mMatchCacheMap.findMatchCacheForSubstring(substring2);
            if (matchCache != nullptr)
            {    
                if (doRemoveInsteadOfAdd)
                {
                    // side: no ops if prior loop erased (if substr repeated in word)
                    matchCache->erase(wme.getId());
                }
                else
                {
                    matchCache->insert(wme.getId());
                }
            }

            // notify join nodes
            if (mSearchersMap.hasListeningJoinNodesForSubstring(substring2))
            {
                SPAM_LOG("[RETE][ReteSubstringTrie].notifyJoinNodesOfWmeUpdate: Notifying join nodes listening, for " << (doRemoveInsteadOfAdd? "remov" : "add") << " of wme substring fact (wmename=" << wme.getName() << ",id=" << wme.getId() << ",substr=" << substring2 << ")");
                JoinNodeList& joinNodes = mSearchersMap.getOrCreateJoinNodeListForSubstring(substring2);
                JoinNodeList::iterator end = joinNodes.end();
                for (JoinNodeList::iterator itr = joinNodes.begin(); itr != end; ++itr)
                {
                    JoinNode* production = (*itr);
                    if (doRemoveInsteadOfAdd)
                    {
                        production->notifyRemoveWMEFromAlphaMemory(wme.getId());
                    }
                    else
                    {
                        production->notifyAddWMEToAlphaMemory(wme.getId());
                    }
                    numWmeUpdates++;
                }
            }
            else
                SPAM_LOG("[RETE][ReteSubstringTrie].notifyJoinNodesOfWmeUpdate: No join-nodes/production listeners detected for newly " << (doRemoveInsteadOfAdd? "remov" : "add") << "ed substring fact (wmename=" << wme.getName() << ",id=" << wme.getId() << ",substr=" << substring2 << "). No updates made.");

            // reset null terminator to original string's char
            substring2[i + 1] = tempOrigTailChar;
        }
        return numWmeUpdates;
    }

    /*! ************************************************************************************************/
    /*! \brief register join node to listen to the search string for match updates

        \param[in] searchSubstring - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    void ReteSubstringTrie::registerListener(const char8_t* searchSubstring, JoinNode& joinNode)
    {
        if ((searchSubstring == nullptr) || (searchSubstring[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].registerListener: null/empty input search string: aborted registering joinNode(addr:" << &joinNode << ") for (wmename=" << joinNode.getProductionTest().getName() << ")");
            return;
        }
        TRACE_LOG("[RETE][ReteSubstringTrie].registerListener: registering joinNode(addr:" << &joinNode << ") as listener for sub string(wmename=" << joinNode.getProductionTest().getName() << ",str=" << searchSubstring << ")");
        JoinNodeList& joinNodesListening = mSearchersMap.getOrCreateJoinNodeListForSubstring(searchSubstring);
        joinNodesListening.push_back(&joinNode);
    }

    /*! ************************************************************************************************/
    /*! \brief unregister join node from listening to the search string
        For memory cleanup, we'll destroy a term's hash/entry in searches string table, when
        unregistering term's last JoinNode (pre: no clients need hash/entry after cleanup)

        \param[in] searchSubstring - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    void ReteSubstringTrie::unregisterListener(const char8_t* searchSubstring, const JoinNode& joinNode)
    {
        if ((searchSubstring == nullptr) || (searchSubstring[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].unregisterListener: null/empty input search string: aborted unregistering joinNode(addr:" << &joinNode << ") for (wmename=" << joinNode.getProductionTest().getName() << ")");
            return;
        }
        if (!mSearchersMap.hasListeningJoinNodesForSubstring(searchSubstring))
        {
            WARN_LOG("[RETE][ReteSubstringTrie].unregisterListener: attempted to unregister joinNode(addr:" << &joinNode << ") when there are no listeners on the sub string(wmename=" << joinNode.getProductionTest().getName() << ",str=" << searchSubstring << ")");
            return;
        }
        TRACE_LOG("[RETE][ReteSubstringTrie].unregisterListener: unregistering joinNode(addr:" << &joinNode << ") as listener for sub string(wmename=" << joinNode.getProductionTest().getName() << ",str=" << searchSubstring << ")");

        JoinNodeList& joinNodesListening = mSearchersMap.getOrCreateJoinNodeListForSubstring(searchSubstring);
        joinNodesListening.remove(const_cast<JoinNode*>(&joinNode));        

        if (joinNodesListening.empty())
        {
            TRACE_LOG("[RETE][ReteSubstringTrie].unregisterListener: removing join nodes listeners list since no one left listening for the sub string (wmename=" << joinNode.getProductionTest().getName() << ", substr=" << searchSubstring << ")");
            mSearchersMap.destroyJoinNodeListForSubstring(searchSubstring);
            mMatchCacheMap.destroyMatchCacheForSubstring(searchSubstring);

            // clean the string table entry
            uint64_t searchHash = UINT64_MAX;
            if (mSearchStringTable.findHash(searchSubstring, searchHash))
            {
                SPAM_LOG("[RETE][ReteSubstringTrie].unregisterListener: clearing hash/entry from searches string table for search term, since no one left listening for the sub string (wmename=" << joinNode.getProductionTest().getName() << ", substr=" << searchSubstring << ")");
                mSearchStringTable.removeHash(searchHash);
            }
            else
                WARN_LOG("[RETE][ReteSubstringTrie].unregisterListener: Aborted trying clearing hash/entry from searches string table for search term, no string table hash for sub string (wmename=" << joinNode.getProductionTest().getName() << ", substr=" << searchSubstring << ")");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Find all facts matching the substring, and add their associated wmes to the joinNode.
        Pre: if had existing match cache, it must be up to date
        \param[in] searchSubstring - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    void ReteSubstringTrie::addWMEsFromFindMatches(const char8_t* searchSubstring, JoinNode& joinNode) const
    {
        if ((searchSubstring == nullptr) || (searchSubstring[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].addWMEsFromFindMatches: null/empty input search string: aborted adding wmes for joinNode(addr:" << &joinNode << ") for (wmename=" << joinNode.getProductionTest().getName() << ")");
            return;
        }
        TRACE_LOG("[ReteSubstringTrie].addWMEsFromFindMatches('" << searchSubstring << "', joinNode addr=" << &joinNode << "), populating JoinNode from found existing match cache.");
        findOneOrAllMatches(searchSubstring, nullptr, &joinNode);
    }
    
    /*! ************************************************************************************************/
    /*! \brief helper to get substring's matches, and either check for a wme or save all to join node.
        Pre: if had existing match cache, it must be up to date.

        \param[in] searchSubstring - to get matches. Pre: normalized(for keying internal maps), non null.
        \param[in] oneToFindInMatches - if non null, checks whether wme is in matches.
        \param[in] joinNodeForAllMatches - if non null, saves all wmes in matches to this JoinNode.
        \return whether oneToFindInMatches was in matches. false if oneToFindInMatches nullptr.
    ***************************************************************************************************/
    bool ReteSubstringTrie::findOneOrAllMatches(const char8_t* searchSubstring, const WMEId* oneToFindInMatches,
        JoinNode* joinNodeForAllMatches) const
    {
        // ensure exactly one mode used (find one or else all)
        EA_ASSERT(((oneToFindInMatches == nullptr) && (joinNodeForAllMatches != nullptr)) || ((oneToFindInMatches != nullptr) && (joinNodeForAllMatches == nullptr)));
        EA_ASSERT(mTrie != nullptr);

        uint32_t matchCount = 0;
        bool isOneWmeFound = false;
        SubstringMatchCache* cache = mMatchCacheMap.findMatchCacheForSubstring(searchSubstring);
        if (cache != nullptr)
        {
            // use cache prior searchers initialized
            matchCount = static_cast<uint32_t>(cache->size());
            TRACE_LOG("[ReteSubstringTrie].findOneOrAllMatches('" << searchSubstring << "'), found existing match cache, processing " << matchCount << " matched wmes");
            SubstringMatchCache::const_iterator itCur = cache->begin();
            SubstringMatchCache::const_iterator itEnd = cache->end();
            for (; itCur != itEnd; ++itCur)
            {
                if (joinNodeForAllMatches != nullptr)
                    addWmeFromFindMatches(*itCur, *joinNodeForAllMatches);
                if ((oneToFindInMatches != nullptr) && (*itCur == *oneToFindInMatches))
                    isOneWmeFound = true;
            }
        }
        else
        {
            // populate new cache
            cache = &const_cast<ReteSubstringTrie*>(this)->mMatchCacheMap.getOrCreateMatchCacheForSubstring(searchSubstring);
            WMEId* matches = nullptr;
            matchCount = mTrie->findMatches(searchSubstring, &matches);
            TRACE_LOG("[ReteSubstringTrie].findOneOrAllMatches('" << searchSubstring << "'), creating new match cache. Match count: " << matchCount << " matched wmes");
            for (uint32_t idx = 0; idx < matchCount; ++idx)
            {
                const WMEId wmeId = matches[idx];
                SPAM_LOG("[ReteSubstringTrie].findOneOrAllMatches('" << searchSubstring << "'), inserting wme (id=" << wmeId << " to match cache");
                cache->insert(wmeId);

                if (joinNodeForAllMatches != nullptr)
                    addWmeFromFindMatches(wmeId, *joinNodeForAllMatches);
                if ((oneToFindInMatches != nullptr) && (wmeId == *oneToFindInMatches))
                    isOneWmeFound = true;
            }
        }
        return isOneWmeFound;
    }
    /*! ************************************************************************************************/
    /*! \brief helper, adds wme for the id to join node
    ***************************************************************************************************/
    void ReteSubstringTrie::addWmeFromFindMatches(const WMEId id, JoinNode& joinNode) const
    {
        auto* valueMap = mWMEManager.getWorkingMemoryElements(id, joinNode.getProductionTest().getName());

        EA_ASSERT(valueMap != nullptr);
        if (valueMap == nullptr)
        {
            ERR_LOG("[ReteSubstringTrie].addWmeFromFindMatches ERROR - no wme(" << id << ") in WorkingMemory for JN production test(" << joinNode.getProductionTest().getName() << ").");
            return;
        }

        joinNode.notifyAddWMEToAlphaMemory(id);
    }

    /*! ************************************************************************************************/
    /*! \brief Returns whether wme is matched by the trie search on the substring
        Pre: if had existing match cache, it must be up to date
        \param[in] searchSubstring - Pre: normalized(for keying internal maps)
    ***************************************************************************************************/
    bool ReteSubstringTrie::substringMatchesWME(const char8_t* searchSubstring, const WMEId id) const
    {
        if ((searchSubstring == nullptr) || (searchSubstring[0] == '\0'))
        {
            ERR_LOG("[ReteSubstringTrie].substringMatchesWME: null/empty input search string: aborted checking for wme id(" << id << ")");
            return false;
        }
        bool isFound = findOneOrAllMatches(searchSubstring, &id, nullptr);
        TRACE_LOG("[ReteSubstringTrie].substringMatchesWME('" << searchSubstring << "', wme id=" << id << "), wme was" << (isFound? "" : " NOT") << " found in trie");
        return isFound;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether there are join nodes searching the substring.
    ***************************************************************************************************/
    inline bool SubstringSearchersMap::hasListeningJoinNodesForSubstring(const char8_t* substring)
    {
        uint64_t hash = UINT64_MAX;
        if (!findSubstringListenersMapKey(substring, hash))
        {
            SPAM_LOG("[ReteSubstringTrie].hasListeningJoinNodesForSubstring('" << substring << "') no string table hash key");
            return false;
        }
        return (mSubstringListenersMap.find(hash) != mSubstringListenersMap.end());
    }

    /*! ************************************************************************************************/
    /*! \brief Remove the join nodes list for the search string.
    ***************************************************************************************************/
    inline void SubstringSearchersMap::destroyJoinNodeListForSubstring(const char8_t *searchSubstring)
    {
        uint64_t hash = UINT64_MAX;
        if (findSubstringListenersMapKey(searchSubstring, hash))
        {
            mSubstringListenersMap.erase(hash);
        }
        else
            WARN_LOG("[ReteSubstringTrie].destroyJoinNodeListForSubstring: abort due to no hash key for search string " << searchSubstring);
    }
    /*! ************************************************************************************************/
    /*! \brief if there is cached matches for the substring, return the cache. Otherwise return nullptr
    ***************************************************************************************************/
    inline SubstringMatchCache* SubstringMatchCacheMap::findMatchCacheForSubstring(const char8_t* substring) const
    {
        SubstringMatchCache* foundCache = nullptr;
        uint64_t hash = UINT64_MAX;
        if (findMapKeyForString(substring, hash))
        {
            CacheMap::iterator itr = const_cast<SubstringMatchCacheMap*>(this)->mSubstringMatchCacheMap.find(hash);
            if (itr != mSubstringMatchCacheMap.end())
            {
                foundCache = &((*itr).second);
                EA_ASSERT_MSG((foundCache != nullptr), "[ReteSubstringTrie].findMatchCacheForSubstring found nullptr match cache!");
            }
        }
        else
        {
            SPAM_LOG("[ReteSubstringTrie].findMatchCacheForSubstring('" << substring << "') no string table hash key");
        }
        return foundCache;
    }
    /*! ************************************************************************************************/
    /*! \brief Remove the cached matches for the search string.
    ***************************************************************************************************/
    inline void SubstringMatchCacheMap::destroyMatchCacheForSubstring(const char8_t *searchSubstring)
    {
        uint64_t hash = UINT64_MAX;
        if (findMapKeyForString(searchSubstring, hash))
        {
            TRACE_LOG("[ReteSubstringTrie].destroyMatchCacheForSubstring: removing match cache for search string " << searchSubstring);
            mSubstringMatchCacheMap.erase(hash); // destroys value if present
        }
        else
            WARN_LOG("[ReteSubstringTrie].destroyJoinNodeListForSubstring: couldn't find cache, no hash key for search string " << searchSubstring);
    }

    /*! ************************************************************************************************/
    /*! \brief increments word count. returns count remaining
    ***************************************************************************************************/
    uint32_t ReteSubstringTrieWordCounter::incrementWordCount(uint64_t wordHash)
    {
        if (mWordCountMap.find(wordHash) != mWordCountMap.end())
        {
            return (++(mWordCountMap[wordHash]));
        }
        else
        {
            mWordCountMap[wordHash] = 1;
            return 1;
        }
    }
    /*! ************************************************************************************************/
    /*! \brief decrement word count. returns count remaining or UINT32_MAX if failed.
    ***************************************************************************************************/
    uint32_t ReteSubstringTrieWordCounter::decrementWordCount(uint64_t wordHash)
    {
        if (mWordCountMap.find(wordHash) != mWordCountMap.end())
        {
            uint32_t& count = mWordCountMap[wordHash];
            if (count == 0)
            {
                WARN_LOG("decrementWordCount: aborted attempt to decrement the word's count below zero");
                return UINT32_MAX;
            }
            --count;
            const uint32_t returnResult = count;
            if (count == 0)
            {
                mWordCountMap.erase(wordHash);
            }
            return returnResult;
        }
        ERR_LOG("decrementWordCount: failed on the word, may not be hashed in string table");
        return UINT32_MAX;
    }

} // namespace Rete
} // namespace GameManager
} // namespace Blaze

