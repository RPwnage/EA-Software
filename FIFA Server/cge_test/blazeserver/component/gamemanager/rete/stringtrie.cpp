/*! ************************************************************************************************/
/*!
    \file stringtrie.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "stringtrie.h"
#include <malloc.h>
#include <EASTL/stack.h> // for eastl::stack in removeSubstrings

#ifdef BLAZE_STRINGTRIE_DEBUG
#include "EATDF/time.h" // for metrics calcs
#include <EASTL/string.h>   // for WordMap in main()
#include <EASTL/hash_map.h> // for WordMap in main()
#include <EASTL/hash_set.h> // for hash_set in removeChildMatchIdsNotOnParent()
#endif

#include "EAStdC/EAString.h"

namespace Blaze
{

#ifdef BLAZE_STRINGTRIE_DEBUG
// internal memory metrics
/*static*/ size_t StringTrie::AllocationMetrics::sTotalAllocBytes = 0;
/*static*/ size_t StringTrie::AllocationMetrics::sAllocBytes = 0;
/*static*/ size_t StringTrie::AllocationMetrics::sPeakAllocBytes = 0;
/*static*/ size_t StringTrie::AllocationMetrics::sNodes = 0;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Class StringNormalizationTable ///////////////////////////////////////

/*! ************************************************************************************************/
/*! \brief ctor Specifies and inits internal normalization char translation table
    \param[in] relevantChars - null terminated string containing all relevant chars
***************************************************************************************************/
StringTrie::StringNormalizationTable::StringNormalizationTable(bool ignoreCase, const char8_t* relevantChars)
{
    // Setup the character translation table
    TRACE_LOG("[StringNormalizationTable].StringNormalizationTable: constructing table: ignoreCase=" << ignoreCase << ", relevantChars=" << ((relevantChars!=nullptr)? relevantChars : "<null>") << ".");
    memset(mXlat, 0, sizeof(mXlat));
    if ((relevantChars == nullptr) || (relevantChars[0] == '\0'))
    {
        // Map alpha characters to ignore case as needed
        for(char8_t ch = 'A'; ch <= 'Z'; ++ch)
        {
            mXlat[(unsigned char)ch] = ch;
            mXlat[(unsigned char)(ch | 32)] = (ignoreCase? ch : (ch | 32));
        }

        // Map numeric characters
        for(char8_t ch = '0'; ch <= '9'; ++ch)
            mXlat[(unsigned char)ch] = ch;
    }
    else
    {
        // map only relevant characters
        for (uint8_t i = 0; relevantChars[i] != '\0'; ++i)
        {
            // if either the lower or upper case is specified relevant and ignoreCase true,
            // make both the upper and lower cases relevant and mapped to the lower case
            char8_t ch = relevantChars[i];
            if ((ch >= 'A') && (ch <= 'Z') && ignoreCase)
            {
                const char8_t lc = (ch | 32);
                mXlat[(unsigned char)ch] = lc; // upper case maps to lower
                mXlat[(unsigned char)lc] = lc; // lower case maps to lower
            }
            else if ((ch >= 'a') && (ch <= 'z') && ignoreCase)
            {
                const char8_t uc = (ch & ~32);
                mXlat[(unsigned char)ch] = ch; // lower case maps to lower
                mXlat[(unsigned char)uc] = ch; // upper case maps to lower
            }
            else
            {
                mXlat[(unsigned char)ch] = ch; // map directly to itself
            }
        }
    }

    //MM_AUDIT: in future we may add in equivalentChar's mapping feature here, something like this:
    // After inserted relevantChars to normalization table, add additional chars needed based on 'equivalentChars'.
    // The thing to note is 'equivalentChars' might chain-map chars indirectly to a common 'final target char'.
    // First, from the initial 'equivalentChars', build a 2nd 'fullEquivalentChars' map storing lists of *all*
    // chars, a 'final target char'(which keys the map) is equivalent to.
    // Next, for each 'final target char', if any one of its 'fullEquivalentChars' chars is relevant,
    // normalization-table map all its equivalent chars to the same common relevant char normalization table value.
}

/*! ************************************************************************************************/
/*! \brief Normalize the word using translation table (e.g. case insensitive)
    \param[out] normalizedWord - buffer to store result. Pre: sufficient size allocated.
    \return the length of the normalized word
***************************************************************************************************/
uint32_t StringTrie::StringNormalizationTable::normalizeWord(const char* word, char* normalizedWord) const
{
    if ((word == nullptr) || (word[0] == '\0'))
    {
        ERR_LOG("[StringTrie::StringNormalizationTable].normalizeWord: null/empty input string");
        return 0;
    }
    const uint32_t wordlen = strlen(word);
    uint32_t normalizedWordLen = 0;
    for(uint32_t idx = 0; idx < wordlen; ++idx)
    {
        char ch = (char)mXlat[(uint8_t)word[idx]];
        if (ch == 0)
            continue; // skip unmapped values
        normalizedWord[normalizedWordLen++] = ch;
    }
    normalizedWord[normalizedWordLen] = 0;
    return normalizedWordLen;
}
/*! ************************************************************************************************/
/*! \brief revert back to original un-normalized form for the word using translation table
    \param[out] word - buffer to store result. Pre: sufficient size allocated.
***************************************************************************************************/
void StringTrie::StringNormalizationTable::unnormalizeWord(const char* normalizedWord, char* word) const
{
    if ((normalizedWord == nullptr) || (normalizedWord[0] == '\0'))
    {
        ERR_LOG("[StringTrie::StringNormalizationTable].unnormalizeWord: null/empty input string");
        return;
    }
    for(const char* n = normalizedWord; *n != '\0'; ++n)
    {
        for(uint32_t idx = 0; idx < 256; ++idx)
        {
            if (*n == mXlat[idx])
            {
                *word++ = (char)idx;
                break;
            }
        }
    }
    *word = '\0';
}
///////////////////////////// End Class StringNormalizationTable ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  Class StringTrie::Node ///////////////////////////////////////////
/* ************************************************************************************************/
/* Class StringTrie::Node. The radix trie head node is an empty sentinel to start traversals at.
    Each non-head node holds a >= 1 length char segment of search-able strings stored in the trie.
    I.e. level 1 holds prefixes including start char, their children hold following char segments, etc.
***************************************************************************************************/

/*! ************************************************************************************************/
/*! \brief ctor
***************************************************************************************************/
StringTrie::Node::Node()
    : mMatches(nullptr),
    mLinks(nullptr),
    mLinkCount(0),
    mMatchCount(0),
    mLinkChars(nullptr)
{
#ifdef BLAZE_STRINGTRIE_DEBUG
    StringTrie::AllocationMetrics::sNodes++;
#endif
}
/*! ************************************************************************************************/
/*! \brief destructor
***************************************************************************************************/
StringTrie::Node::~Node()
{    
#ifdef BLAZE_STRINGTRIE_DEBUG
    StringTrie::AllocationMetrics::sNodes--;
#endif
}

void StringTrie::Node::deleteMemberArrays()
{
    if (mMatches != nullptr)
    {
        deleteMatches();
    }
    if (mLinks != nullptr)
    {
        deleteLinks();
    }
    if (mLinkChars != nullptr)
    {
        deleteLinkChars();
    }
}


/*! ************************************************************************************************/
/*! \brief Locate a node in the links table which has a matching prefix as the provided 'prefix'.
    This function will allocate a new match link node if an appropriate one is not found.
    It will also split an existing node per the rules of a radix tree should that be necessary.

    \param[in] prefix - string to get child link for. If nullptr or empty, return nullptr.
    \param[in] len - length of input 'prefix'.
    \param[out] consumedChars - num chars in returned node. Its chars match (a portion of) 'prefix'.
        If 'consumedChars' < 'len', caller continues consuming remain chars via returned link.

    \return matched link node caller traverses to next. If split occurred, this node contains
        only the portion of the original chars, which were common with 'prefix'.
        If it has all 'prefix's chars, caller has found the end match node for 'prefix'.
***************************************************************************************************/
StringTrie::Node* StringTrie::Node::getOrAddLink(const char* prefix, uint32_t len, uint32_t& consumedChars)
{
    if ((prefix == nullptr) || (prefix[0] == '\0'))
    {
        ERR_LOG("[StringTrie::Node].getOrAddLink: null/empty input string");
        return nullptr;
    }

    if (mLinkCount == 0)
    {
        // This node doesn't have any links yet so just create a new one with given prefix.
        consumedChars = len;
        Node* n = addLinkNode(prefix, len);
        SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'): moving to added 1st link node(addr:" << n << ", chars:'" << prefix << "', parent's link index:" << (mLinkCount - 1) << ")");
        return n;
    }

    // Look for a node with a common prefix
    for(uint32_t idx = 0; idx < mLinkCount; ++idx)
    {
        // Find the longest matching prefix
        Node& origNode = mLinks[idx];
        consumedChars = 0;
        while ((consumedChars < len) && (origNode.mLinkChars[consumedChars] != '\0')
            && (prefix[consumedChars] == origNode.mLinkChars[consumedChars]))
        {
            ++consumedChars;
        }

        // If we found a link with a common prefix
        if (consumedChars != 0)
        {
            // If the link is a substring of the prefix we're looking, no need to split. Just return it. (caller reiterates on this fn to create a child of it containing remain chars)
            if (origNode.mLinkChars[consumedChars] == '\0')
            {
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'): move to found link node with prefix of " << prefix <<": node(addr:" << &origNode << ", chars:'" << origNode.mLinkChars << "', parent's link index:" << idx << ")");
                return &origNode;
            }
            
            // The common prefix is shorter than the link's prefix so make this link the
            // common prefix between the two and create a new link node with the leftover
            // characters at the next level down.
            
            // Transfer orig's links to the new node. Change orig to have just new link.
            const uint32_t linkCount = origNode.mLinkCount;
            Node* links = origNode.orphanLinks();
            origNode.mLinkCount = 0;
            Node* n = origNode.addLinkNode(&origNode.mLinkChars[consumedChars],
                strlen(&origNode.mLinkChars[consumedChars]));
            n->adoptLinks(links, linkCount);

            // Since we're splitting an existing node in two, make a copy of the match
            // list from the current node into the new link node.
            if (origNode.mMatchCount > 0)
            {
                n->mMatches = BLAZE_NEW_ARRAY(uint64_t, origNode.mMatchCount);
                n->mMatchCount = origNode.mMatchCount;
                memcpy(n->mMatches, origNode.mMatches,
                    sizeof(WordId) * origNode.mMatchCount);
            }

            // Shorten the prefix for the orig link node to the common matched length
            origNode.mLinkChars[consumedChars] = '\0';

            if (IS_LOGGING_ENABLED(Logging::SPAM))
            {
                const bool has2nd = (prefix[consumedChars] != '\0');
                eastl::string buf;
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:Orig node(addr:" << &origNode << ", pre-split:'" << origNode.mLinkChars << n->mLinkChars << "',post-split:'" << origNode.mLinkChars << "')..");
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:" << ".. and added 1st split-off child(addr:" << n << ",chars:'" << n->mLinkChars << "')");
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:" << ".. and (going to add) 2nd split-off child(chars:'" << (has2nd? &prefix[consumedChars] : "<no 2nd child needed>") << "')");
                n->printMatches(buf);
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:1st splitted-off child(addr:" << n << ",post-split matches:" << buf.c_str() << ")");
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:2nd splitted-off child(" << (has2nd? "addr:tbd..,post-split matches:tbd..)" : "<no 2nd child needed>)"));
                origNode.printMatches(buf);
                SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'):" << " Node-split:Orig node(addr:" << &origNode << ",pre-split matches:" << buf.c_str() << ",post-split matches:tbd..)");
            }
            // Return the updated link node containing the common prefix. (caller reiterates on this fn to create a child of it containing remain chars)
            return &origNode;
        }
    }

    // No common prefix found so create a new link node
    consumedChars = len;
    Node* n = addLinkNode(prefix, len);
    SPAM_LOG("[StringTrie::Node].getOrAddLink('" << prefix << "'): No pre-existing links with common chars found, move to newly added link node(addr:" << n << ", chars:'" << prefix << "', parent link index:" << (mLinkCount - 1) << ")");
    return n;
}

/*! ************************************************************************************************/
/*! \brief Helper. Allocate a new link node and add it to the links table based on the provided prefix.
    If the link table isn't allocated yet then do so.  Otherwise increase the table size
    by one to make room for the new link node.  This function should only be called if checks
    have already been made ensuring that there is not already an existing node with the same prefix.
***************************************************************************************************/
StringTrie::Node* StringTrie::Node::addLinkNode(const char* prefix, uint32_t prefixLen)
{
    if ((prefix == nullptr) || (prefix[0] == '\0'))
    {
        ERR_LOG("[StringTrie::Node].addLinkNode: null/empty input string");
        return nullptr;
    }

    Node* newLinks = BLAZE_NEW_ARRAY(Node, mLinkCount + 1);
    if (mLinkCount > 0)
    {
        EA_ASSERT(mLinks != nullptr);
        memcpy(newLinks, mLinks, sizeof(mLinks[0]) * mLinkCount);
        deleteLinks();
    }
    newLinks[mLinkCount].mLinkChars = BLAZE_NEW_ARRAY(char, prefixLen + 1);
    strncpy(newLinks[mLinkCount].mLinkChars, prefix, prefixLen);
    newLinks[mLinkCount].mLinkChars[prefixLen] = '\0';
    mLinks = newLinks;    
    SPAM_LOG("[StringTrie::Node].addLinkNode('" << prefix << "'): Node-update:added new trie link node(addr:" << &mLinks[mLinkCount] << "), parent link index " << mLinkCount);
    return &mLinks[mLinkCount++];
}


/*! ************************************************************************************************/
/*! \brief Helper. De-allocate a node and remove it from the links table
    Resize the link table one less or delete it if no more links
    \param[in] idx, the index of node in the link table to remove
***************************************************************************************************/
void StringTrie::Node::removeLinkNode(uint32_t idx)
{
    EA_ASSERT((idx < mLinkCount) && (mLinks != nullptr));
    Node& child = mLinks[idx];    
    SPAM_LOG("[StringTrie::Node].removeLinkNode: Node-update:removing node(addr:" << &child << ",'" << child.mLinkChars << "', at parent index " << idx << ")");
    EA_ASSERT(child.isRemovable());
    
    // ensure cleaned up the dynamically allocated link node array members (which should be empty)
    child.deleteMemberArrays();

    // reallocate my link table minus removed link
    if (mLinkCount == 1)
    {
        // delete children and set count to 0
        deleteLinks();
        mLinkCount--;
    }
    else
    {
        Node* newLinks = BLAZE_NEW_ARRAY(Node, mLinkCount -1);
        for (uint32_t i = 0, j = 0; i < mLinkCount; ++i)
        {
            // efficiency side note: test data shows we expect limited avg num links
            if (i != idx)
                newLinks[j++] = mLinks[i];
        }
        deleteLinks();
        mLinks = newLinks;
        mLinkCount--;
    }   
}


/*! ************************************************************************************************/
/*! \brief merge parent with its single remaining child link node.
    Pre: Node has single child link
    Pre: Either child had same matched ids set as this node, or this node had no ids (empty) as its
        path's string didn't meet min len requirement
    Post: If node's path's string meets trie's min length, node has the match ids.
***************************************************************************************************/
void StringTrie::Node::mergeWithMyLinkNode()
{
    EA_ASSERT(mLinks != nullptr);
    Node& child = mLinks[0];
    SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: Merge this parent(addr,'" << mLinkChars << "'), with single child(addr:"<< &child << ",'" << child.mLinkChars << "')..");
    if (IS_LOGGING_ENABLED(Logging::SPAM))
    {
        eastl::string buf;
        printMatches(buf);
        SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: .. parent(pre-merge matches:" << buf.c_str() << ")");
        child.printMatches(buf);
        SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: .. child(pre-merge matches:" << buf.c_str() << ")");
    }

    // if this node was empty due to not meeting min length, post merge it might
    // now meet min length. add the matched ids, as required.
    if (isEmpty() && !child.isEmpty())
    {
        // if child had matches it met min length requirements, then post-merge
        //  this node will meet min len also, so ensure it has the matches.
        const uint32_t matchCount = child.mMatchCount;
        WordId* matches = child.orphanMatches();
        child.mMatchCount = 0;
        adoptMatches(matches, matchCount);
    }
    
    // append child's chars to me. (my matches effectively become my child's string's matches here).
    const uint32_t myLen = getLinkCharsLen();
    const uint32_t childLen = child.getLinkCharsLen();
    char* newLinkChars = BLAZE_NEW_ARRAY(char, myLen + childLen + 1);
    memcpy(newLinkChars, mLinkChars, sizeof(char) * myLen);
    memcpy(&newLinkChars[myLen], child.mLinkChars, sizeof(char) * childLen);
    newLinkChars[myLen + childLen] = '\0';
    if (IS_LOGGING_ENABLED(Logging::SPAM))
    {
        eastl::string buf;
        printMatches(buf);
        SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: .. parent(post-merge matches:" << buf.c_str() << ")");
        SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: .. parent(pre-merge:'" << mLinkChars << "'), child(pre-merge:'" << child.mLinkChars << "')");
        SPAM_LOG("[StringTrie::Node].mergeWithMyLinkNode: Node-Merge: .. parent(post-merge:'" << newLinkChars << "'), child(post-merge:<deleted>)");
    }    
    deleteLinkChars();
    mLinkChars = newLinkChars;

    // delete my child link. copy orig child's links to me
    const uint32_t linkCount = child.mLinkCount;
    Node* links = child.orphanLinks();
    child.deleteMemberArrays(); // cleanup
    deleteLinks(); // deletes child node
    adoptLinks(links, linkCount);
}

/*! ************************************************************************************************/
/*! \brief Locate a link which matches the given prefix. Returns nullptr if none found
    \param [in,out] prefix - (ref to) ptr to string to find links for.
        Pre: prefix ptr starts at front of string to get the link for.
        Post: moves ptr forward past any chars link node has common chars with.
    \param[out] foundLinkIndex - if non-null store the found returned child's link table index value.
    \param[out] consumedChars - if non-null store no. chars returned child consumes in input prefix.
    \return the link node (matching at least 1st char in prefix), if it exists
***************************************************************************************************/
StringTrie::Node* StringTrie::Node::getLink(const char*& prefix, uint32_t* foundLinkIndex /*= nullptr*/,
                                            uint32_t* consumedChars /*= nullptr*/) const
{
    for(uint32_t idx = 0; idx < mLinkCount; ++idx)
    {
        // Find the longest matching prefix
        uint32_t prefixLen = 0;
        while ((prefix[prefixLen] != '\0')
            && (prefix[prefixLen] == mLinks[idx].mLinkChars[prefixLen]))
        {
            ++prefixLen;
        }

        if (prefixLen > 0)
        {
            // Found a match so consume the matched prefix and return the node
            prefix += prefixLen;

            if (foundLinkIndex != nullptr)
                *foundLinkIndex = idx;
            if (consumedChars != nullptr)
                *consumedChars = prefixLen;

            return &mLinks[idx];
        }
    }
    return nullptr;
}

/*! ************************************************************************************************/
/*! \brief Add a new id to the match list for this node.
    \param[in] wordId - The id to add
***************************************************************************************************/
void StringTrie::Node::addMatch(const WordId wordId)
{
    if (mMatches == nullptr)
    {
        mMatches = BLAZE_NEW_ARRAY(WordId, 1);
        mMatches[0] = wordId;
        mMatchCount = 1;
        SPAM_LOG("[StringTrie::Node].addMatch: Node-update:successfully added match id(" << wordId << ") to node(addr, '" << mLinkChars << "'). This was the first match added at this node!");
    }
    else
    {
        if (!findWordId(wordId))
        {
            WordId* newMatches = BLAZE_NEW_ARRAY(WordId, mMatchCount + 1);
            memcpy(newMatches, mMatches, sizeof(WordId) * mMatchCount);
            newMatches[mMatchCount] = wordId;
            deleteMatches();
            mMatches = newMatches;
            ++mMatchCount;
            SPAM_LOG("[StringTrie::Node].addMatch: Node-update:successfully added match id(" << wordId << ") to node(addr, '" << mLinkChars << "')");
        }
        else
        {
            SPAM_LOG("[StringTrie::Node].addMatch: Node-update:ignored attempt to re-added already-added match id(" << wordId << ") to node(addr, '" << mLinkChars << "')");
        }
    }
}

/*! ************************************************************************************************/
/*! \brief Remove a id from the match list for this node.
    \param[in] wordId - The id to remove
    \return whether wordId found on node, and thus removed.
***************************************************************************************************/
bool StringTrie::Node::removeMatch(const WordId wordId)
{
    // find the id in matches to remove.
    uint32_t foundIdx = UINT32_MAX;
    if ((mMatches == nullptr) || !findWordId(wordId, &foundIdx))
    {
        // words containing recurrence of substring could mean its already removed. Early out if so.
        SPAM_LOG("[StringTrie::Node].removeMatch: Not removing id " << wordId << " - not found or already removed for this node (mMatches size:" << mMatchCount << ")");
        return false;
    }

    
    SPAM_LOG("[StringTrie::Node].removeMatch: Node-update:removing id " << wordId << " from this node('" << mLinkChars << "')..");
    if (mMatchCount == 1)
    {
        // if last item, delete matches all together
        EA_ASSERT(mMatches[0] == wordId);
        clearMatches(); //sets mMatchCount 0
        return true;
    }
    else // if (mMatchCount > 1)
    {
        // Resize smaller array with id removed
        WordId* newMatches = BLAZE_NEW_ARRAY(WordId, mMatchCount - 1);
        if (foundIdx > 0)
        {
            memcpy(newMatches, mMatches, sizeof(WordId) * foundIdx);
        }
        if (foundIdx < (mMatchCount - 1))
        {
            memcpy(&newMatches[foundIdx], &mMatches[foundIdx+1], sizeof(WordId) * (mMatchCount - 1 - foundIdx));
        }
        deleteMatches();
        mMatches = newMatches;
        --mMatchCount;
        return true;
    }
}

/*! ************************************************************************************************/
/*! \brief Whether can merge with my 1 child. I.e. whether this node has no ids the child doesn't also have
    param[in] removedWordId The word whose removal is potentially triggering the merge.
***************************************************************************************************/
bool StringTrie::Node::canMergeWithMyLink(WordId removedWordId) const
{
    // we must have only 1 child.
    if (mLinkCount != 1)
    {
        return false;
    }

    // If I have no matches, we can merge
    if (isEmpty())
    {
        return true;
    }

    // I cant have more number of matches than on my child.
    if (mMatchCount > mLinks[0].mMatchCount)
    {
        return false;
    }

    // Though match counts equal, we still need to check the word id isn't actually on the child skewing the count. 
    // Reason: Even though in general, after every add/remove a parent's matches is a superset its children's,
    // there's edge cases, during a word's *repeated substrings* removal where the child temporarily still
    // has the word id on it, while its already been removed from the parent (see GOS-10765). This isn't
    // accounted for in the above simple match count check, we need a full scan for id on child here in that case.
    if (mLinks[0].findWordId(removedWordId))
    {
        // If here, this node is indeed for a repeated substring, and child still has the id on it. Minus that id,
        // child match count doesn't equal ours, i.e. we must have some other id NOT on the child, so cannot merge.
        return false;
    }

    // Match counts equal, and we can merge
    return true;
}

void StringTrie::Node::printMatches(eastl::string &dest) const
{
    dest.clear();
    dest.sprintf("'");
    for (uint32_t i = 0; i < mMatchCount; ++i)
    {
        dest.append_sprintf("%u..", mMatches[i]);
    }
    dest.append_sprintf("'");
}
void StringTrie::Node::print(eastl::string &dest) const
{
    dest.clear();
    eastl::string bufMatches;
    printMatches(bufMatches);    
    dest.sprintf("chars='%s', linkCount=%u, matchCount=%u, matches=%s", 
        (mLinkChars? mLinkChars : "<null>"), mLinkCount, mMatchCount, bufMatches.c_str());
}

/*! ************************************************************************************************/
/*! \brief returns whether id exists in matches and optionally return its index.
***************************************************************************************************/
inline bool StringTrie::Node::findWordId(const WordId wordId, uint32_t* outFoundIdx /*= nullptr*/)
{
    for(uint32_t i = 0; i < mMatchCount; ++i)
    {
        if (mMatches[i] == wordId)
        {
            if (outFoundIdx != nullptr)
            {
                *outFoundIdx = i;
            }
            return true;
        }
    }
    return false;
}

///////////////////////////// End Class StringTrie::Node ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Class StringTrie /////////////////////////////////////////////////////
/* ************************************************************************************************/
/* Class StringTrie. Blaze implementation of a Radix Trie for storing search-able strings compactly
   http://en.wikipedia.org/wiki/Radix_tree.
***************************************************************************************************/

/*! ************************************************************************************************/
/*! \brief StringTrie ctor.
    \param[in] maxLength - Only strings with length in mMinLength and mMaxLength are added to trie.
    \param[in] normalizationParams - strings stored/searched normalizations. Copy stored internally.
    \param[in] leadingPrefixOnly - controls if only prefix substrings including start char, supported.
        false allows substrings based at any index.
        E.g. true, means ‘maze’ does not match game named ‘amazing’, and only matches ‘maze*’.
***************************************************************************************************/
StringTrie::StringTrie(uint32_t minLength, uint32_t maxLength, const StringNormalizationTable& normalizationParams,
    bool leadingPrefixOnly /*= false*/) :
    mMinLength(minLength),
    mMaxLength(eastl::min(maxLength, STRINGTRIE_MAX_MAXLENGTH)),
    mNormalizationTable(normalizationParams),
    mLeadingPrefixOnly(leadingPrefixOnly)
{
}

/*! ************************************************************************************************/
/*! \brief StringTrie dtor.

***************************************************************************************************/
StringTrie::~StringTrie()
{
    mStartNode.deleteMemberArrays();
}

/*! ************************************************************************************************/
/*! \brief Find all added match ids that match given substring
    \param[in] substring - will be normalized before searching for its matches
    \param[out] matches - If "matches" parameter not nullptr, return match list in it.
    \return total number of matches.  0 if none found.
***************************************************************************************************/
uint32_t StringTrie::findMatches(const char* substring, WordId** matches)
{
    EA_ASSERT(mStartNode.isEmpty());
    if ((substring == nullptr) || (substring[0] == '\0'))
    {
        ERR_LOG("[StringTrie].findMatches: aborted: null/empty input string");
        return 0;
    }

    // Normalize search string
    char normalized[STRINGTRIE_MAX_MAXLENGTH];
    const uint32_t normalizedLen = mNormalizationTable.normalizeWord(substring, normalized);
    const char* remain = normalized;

    // Reject requests where the requested substring is too short
    if (normalizedLen < mMinLength)
    {
        TRACE_LOG("[StringTrie].findMatches('" << normalized << "'): aborted: string len < min length requirement");
        return 0;
    }

    // Walk tree to find substring's match node
    const Node* currState = &mStartNode;
    while (*remain != '\0')
    {
        // if leaf, nothing more to check
        if (currState->isLeaf())
            return 0;

        // get next common-prefix-with-'remain' link. advance 'remain' ptr past the chars
        Node* nextState = currState->getLink(remain);
        if (nextState == nullptr)
        {
            // no more links to go to
            return 0;
        }
        if (!currState->isEmpty() && !validateNodeMatchCountBound(*nextState, currState->mMatchCount, currState))
        {
            // probably shouldn't hit, but if do, clean after the fact.
            WARN_LOG("[StringTrie].findMatches('" << substring << "'): Failed node match counts validations. Proceeding with auto-cleanup of extra matchids on child node(addr:" << nextState << "), that are missing from parent(addr:" << currState << ")");
            removeChildMatchIdsNotOnParent(*currState, *nextState);
        }
        currState = nextState;
    }

    if (matches != nullptr)
        *matches = currState->mMatches;
    return currState->mMatchCount;
}

/*! ************************************************************************************************/
/*! \brief Insert a new word into the radix tree including all of its substrings. Substrings with
    >= normalized min len chars have the match id stored at their appropriate match node.
    If mLeadingPrefixOnly true only adds leading prefix substrings.
    \param[in] word - the super string to add substrings with id
    \param[in] id - unique id of external entity associated with the word to add
***************************************************************************************************/
bool StringTrie::addSubstrings(const char* word, const WordId id)
{
    if ((word == nullptr) || (word[0] == '\0'))
    {
        ERR_LOG("[StringTrie].addSubstrings(id:" << id << "): aborted: null/empty input string");
        return false;
    }

    char normalizedWord[STRINGTRIE_MAX_MAXLENGTH];
    const uint32_t normalizedWordLen = mNormalizationTable.normalizeWord(word, normalizedWord);
    if (normalizedWordLen < mMinLength)
    {
        // The word being added is shorter than the minimum substring length
        // so don't do anything since it will never match
        TRACE_LOG("[StringTrie].addSubstrings('" << normalizedWord << "',id:" << id << "): aborted: string len < min length requirement");
        return false;
    }

    TRACE_LOG("[StringTrie].addSubstrings('" << normalizedWord << "',id:" << id << "): Adding id for all substrings of word (pre-normalized) '" << word << "'");
    const int term = mLeadingPrefixOnly ? 0 : (normalizedWordLen - mMinLength);
    for(int j = 0; j <= term; ++j)
    {
        // add all substrings starting at the j'th character
        addNormalizedSubstrings(&normalizedWord[j], normalizedWordLen - j, id);
    }
    return true;
}
/*! ************************************************************************************************/
/*! \brief helper to add substring matches for the normalized input string. Substrings with
    >= normalized min len chars have the match id stored at their appropriate match node.
***************************************************************************************************/
void StringTrie::addNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id)
{
    TRACE_LOG("[StringTrie].addNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Adding id for all prefix substrings of '" << normalizedWord << "' to trie..");
    EA_ASSERT(mStartNode.isEmpty());
    Node* currState = &mStartNode;
    if (normalizedWordLen > mMaxLength)
        normalizedWordLen = mMaxLength;

    uint32_t i = 0;
    while (i < normalizedWordLen)
    {
        uint32_t consumedChars = 0;
        Node* nextState = currState->getOrAddLink(&normalizedWord[i], normalizedWordLen - i, consumedChars);
        i += consumedChars;
        if (i >= mMinLength)
        {
            // i-length prefix meets min len, add as match at the found node
            nextState->addMatch(id);
        }
        // check whether node was split but had matches from before split
        else if (!nextState->isEmpty())
        {
            // clear node's ids since post-split it no longer consumes min char length
            nextState->clearMatches();
        }

        const bool passEmptynessCheck = ((i < mMinLength) && nextState->isEmpty()) || ((i >= mMinLength) && !nextState->isEmpty());
        if (!passEmptynessCheck)
        {
            ERR_LOG("[StringTrie].addNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): node should only be empty if its path consumed chars < trie min len. Proceeding to empty node");
        }
        if (!(currState->isEmpty() || validateNodeMatchCountBound(*nextState, currState->mMatchCount, currState)))
        {
            WARN_LOG("[StringTrie].addNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Failed node match counts validations, for node containing " << consumedChars << "-length prefix substr of " << &normalizedWord[i] << ". Proceeding with auto-cleanup of extra matchids on child node(addr:" << nextState << "), that are missing from parent(addr:" << currState << ")");
            removeChildMatchIdsNotOnParent(*currState, *nextState);
        }

        currState = nextState;
        if (IS_LOGGING_ENABLED(Logging::SPAM))
        {
            eastl::string buf;
            currState->printMatches(buf);
            SPAM_LOG("[StringTrie].addNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Node-walk: successfully added-to/visited node(addr:" << currState << ",'" << currState->mLinkChars << "', which now has match ids:" << buf.c_str() << ")");
            SPAM_LOG("[StringTrie].addNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Node-walk: ... it consumed additional char count:" << consumedChars << ". curr total consumed:" << i << ", out of len: " << normalizedWordLen << ")");
        }
    }
}

/*! ************************************************************************************************/
/*! \brief Removes the id from all nodes matching substrings of the given added word
        Compact away nodes as per radix trie algo as needed.
    \param[in] word - used to locate id's to remove from nodes via searching its substrings
    \param[in] id - id to remove if found
    \return total nodes id removed from. 0 if unable to find any
***************************************************************************************************/
uint32_t StringTrie::removeSubstrings(const char* word, const WordId id)
{
    if ((word == nullptr) || (word[0] == '\0'))
    {
        ERR_LOG("[StringTrie].removeSubstrings(id:" << id << "): aborted: null/empty input string");
        return 0;
    }

    char normalizedWord[STRINGTRIE_MAX_MAXLENGTH];
    const uint32_t normalizedWordLen = mNormalizationTable.normalizeWord(word, normalizedWord);
    uint32_t removedCount = 0;

    if (normalizedWordLen < mMinLength)
    {
        // The word to remove is shorter than the minimum substring length
        // so don't do anything since it will never match
        return 0;
    }

    TRACE_LOG("[StringTrie].removeSubstrings('" << normalizedWord << "',id:" << id << "): Removing id for all substrings of word (pre-normalized) '" << word << "'");
    const int term = mLeadingPrefixOnly ? 0 : (normalizedWordLen - mMinLength);
    for(int j = 0; j <= term; ++j)
    {
        // remove substrings starting at the jth character
        removedCount += removeNormalizedSubstrings(&normalizedWord[j], normalizedWordLen - j, id);
    }
    return removedCount;
}
/*! ************************************************************************************************/
/*! \brief Travels down trie from root, removing the id from all end-match nodes for the prefix
    substrings of the given added normalized string.
    Compact away nodes as per radix trie algo as needed.
***************************************************************************************************/
uint32_t StringTrie::removeNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id)
{
    TRACE_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id=" << id << "): removing id for all prefix substrings from trie..");
    EA_ASSERT(mStartNode.isEmpty());
    return removeNormalizedSubstrings(normalizedWord, normalizedWordLen, id, mStartNode);
}
/*! ************************************************************************************************/
/*! \brief overload allowing specifying trie start node
***************************************************************************************************/
uint32_t StringTrie::removeNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id,
                                                Node& startNode)
{
    TRACE_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id=" << id << ",startNodeAddr=" << &startNode << "): removing id for all prefix substrings from trie..");
    Node* currState = &startNode;
    if (normalizedWordLen > mMaxLength)
        normalizedWordLen = mMaxLength;

    // init 'remaining' chars pointer to start of orig word
    const char* remain = normalizedWord;

    uint32_t totalRemoved = 0;

    // link to the last node we remove at, and ref to its parent
    uint32_t endLinkIndex = UINT32_MAX;
    Node* endParent = nullptr;
    Node* endParentParent = nullptr; // for assert validations below
           
    // Walk tree to find normalizedWord's substring match nodes, remove ids
    uint32_t i = 0;
    while (i <= normalizedWordLen)
    {
        // if leaf, nothing more to check
        if (currState->isLeaf())
            break;

        // get next common-prefix-with-'remain' link. advance 'remain' ptr past the chars
        uint32_t consumedChars = 0;
        Node* nextState = currState->getLink(remain, &endLinkIndex, &consumedChars);
        if (nextState == nullptr)
        {
            // no more children to go to
            break;
        }
        i += consumedChars;

        // found a match non-null link. remove any copy of id from it
        if ((i >= mMinLength) && (nextState->removeMatch(id)))
        {
            totalRemoved++;
        }
        if (!currState->isEmpty() && !validateNodeMatchCountBound(*nextState, currState->mMatchCount, currState))
        {
            // probably shouldn't hit, but if do, clean after the fact.
            WARN_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Failed check, pre remove id from child, that (non-empty) parent matches should otherwise be a superset of children's, for node containing " << consumedChars << "-length prefix substr of " << &normalizedWord[i] << ". Proceeding with auto-cleanup of extra matchids on child node(addr:" << nextState << "), that are missing from parent(addr:" << currState << ")");
            removeChildMatchIdsNotOnParent(*currState, *nextState);
        }
        endParentParent = endParent;
        endParent = currState;
        currState = nextState;

        if (IS_LOGGING_ENABLED(Logging::SPAM))
        {
            eastl::string buf;
            nextState->printMatches(buf);
            SPAM_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id=" << id << "): Node-walk: successfully removed-from/visited node(addr:" << nextState << ",'" << nextState->mLinkChars << "', which now has match ids:" << buf.c_str() << ")");
        }
    }//while

    // if no remove call made can return without radix compacting below. Side: check if remove *attempted* at end match-node (not totalRemoved)
    // to ensure its cleanup (if curr call is on repeat occurrence of substring in orig word, prior call might've removed id but w/o cleanup)
    if (i < mMinLength)
    {
        return 0;
    }
    EA_ASSERT((endParent != nullptr) && (endParent->mLinkCount > endLinkIndex));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Merge/compact trie as per radix Check merge-ability of:
    // 1) the last id-removed node's parent, since maybe removing its second last link
    // (in which case that parent may be able to merge with the remaining 1 child).
    // 2) the last id-removed node, since it may now have same ids as on its child
    // (in which case that id-removed node may be able to merge with its 1 child).
    
    // 1) check if id-removed node is deletable leaf
    Node& endNode = endParent->mLinks[endLinkIndex];
    if (endNode.isRemovable())
    {
        endParent->removeLinkNode(endLinkIndex);

        // check if its parent has only 1 other child left thats merge-able
        if ((endParent != &mStartNode) && endParent->canMergeWithMyLink(id))
        {
            endParent->mergeWithMyLinkNode();

            // after merge the parent should have at most 1 less match than child (if there are repeat substrings in orig word,
            // its possible id already removed from the sibling parent merged with, causing it now to have 1 less match id)
            if (!endParentParent && !endParentParent->isEmpty() &&
                !validateNodeMatchCountBound(*endParent, endParentParent->mMatchCount + 1, endParentParent))
            {
                WARN_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): After removed end-match node, its parent then merged with 1 remaining other (ex)sibling, however parent then Failed node match counts validations. Proceeding with auto-cleanup of extra matchids on child node(addr:" << endParent << "), that are missing from parent(addr:" << endParentParent << ")");
                removeChildMatchIdsNotOnParent(*endParentParent, *endParent);
            }  
        }
    }
    // 2) check if the id-removed node (not-deleted), can merge with own child
    else if (endNode.canMergeWithMyLink(id))
    {
        endNode.mergeWithMyLinkNode();
        // since removed id from me, should be <= parent's id count
        if(!endParent->isEmpty() && !validateNodeMatchCountBound(endNode, endParent->mMatchCount, endParent))
        {
            WARN_LOG("[StringTrie].removeNormalizedSubstrings('" << normalizedWord << "',id:" << id << "): Post end-match node merge with own child: Failed node match counts validations. Proceeding with auto-cleanup of extra matchids on child node(addr:" << &endNode << "), that are missing from parent(addr:" << endParent << ")");
            removeChildMatchIdsNotOnParent(*endParentParent, *endParent);
        }
    }
    return totalRemoved;
}

/*! ************************************************************************************************/
/*! \brief to check (non-empty) parent matches should generally be a superset of children's. logs appropriate errs.
    \param[in] value - the value to check node's match count <=. (unless in remove loop, probably parent's count)
    \param[in] parent - for extra logging info if err
***************************************************************************************************/
bool StringTrie::validateNodeMatchCountBound(const Node& node, uint32_t value, const Node* parent /*= nullptr*/) const
{
    if (node.mMatchCount > value)
    {
        ERR_LOG("[StringTrie].validateNodeMatchCountBound: currently expected child matchCount < " <<
            value << ", but child count=" << node.mMatchCount << "). node(chars='" << (node.mLinkChars? node.mLinkChars : "<null>"));

        if (IS_LOGGING_ENABLED(Logging::SPAM))
        {
            eastl::string buf;
            eastl::string bufParent;
            node.print(buf);
            if (parent)
                parent->print(bufParent);
            ERR_LOG("[StringTrie].validateNodeMatchCountBound: currently expected child matchCount < " <<
                value << ", but child(" << buf.c_str() << "),  parent(" << (parent? bufParent.c_str() : "?") << ")");
        }
        return false;
    }
    return true;
}
/*! ************************************************************************************************/
/*! \brief removes match ids from child, that may have been already cleared from parent. Used to 
    ensure clean up after edge case removals that may miss. Performs usual merges as needed.
    \param[in] parent - used to check for match ids missing from child
***************************************************************************************************/
void StringTrie::removeChildMatchIdsNotOnParent(const Node& parent, Node& child)
{
    eastl::hash_set<WordId> parentMatches;
    for (uint32_t i = 0; i < parent.mMatchCount; ++i)
    {
        parentMatches.insert(parent.mMatches[i]);
    }
    for (uint32_t i = 0; i < child.mMatchCount; ++i)
    {
        if (parentMatches.find(child.mMatches[i]) == parentMatches.end())
        {
            WARN_LOG("[StringTrie].removeChildMatchIdsNotOnParent: removing extra matchid " << child.mMatches[i] << " on child node(addr:" << &child << "), that are missing from parent(addr:" << &parent << ")");
            removeNormalizedSubstrings(child.mLinkChars, child.getLinkCharsLen(), child.mMatches[i], child);//bsze_todo: consider doing entire orig wordid's word (would need tracking map tho)
        }
    }
}


#ifdef BLAZE_STRINGTRIE_DEBUG

// test helper
void StringTrie::calculateLinkCounts(size_t linkCounts[]) const
{
    calculateLinkCounts(&mStartNode, linkCounts);
}
void StringTrie::calculateLinkCounts(const struct Node* node, size_t linkCounts[]) const
{
    int count = 0;
    if (node->mLinks != nullptr)
    {
        for(uint32_t idx = 0; idx < node->mLinkCount; ++idx)
        {
            ++count;
            calculateLinkCounts(&node->mLinks[idx], linkCounts);
        }
    }
    ++linkCounts[count];
}

// test helper
void StringTrie::buildDotFile(const char* dotFile) const
{
    FILE* fp = fopen(dotFile, "w");
    fprintf(fp, "digraph {\n");
    buildDotFile(&mStartNode, fp);
    fprintf(fp, "}\n");
    fclose(fp);
}
void StringTrie::buildDotFile(const struct Node* node, FILE* fp) const
{
    for(uint32_t idx = 0; idx < node->mLinkCount; ++idx)
    {
        char normalized[STRINGTRIE_MAX_MAXLENGTH];
        mNormalizationTable.unnormalizeWord(node->mLinks[idx].mLinkChars, normalized);
        fprintf(fp, "    \"%p\" -> \"%p\" [label=\"%s\"];\n", node, &node->mLinks[idx], normalized);
        buildDotFile(&node->mLinks[idx], fp);
    }
}
//#ifdef BLAZE_STRINGTRIE_DEBUG
#endif
///////////////////////////// End Class StringTrie /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


} // ns Blaze


#ifdef BLAZE_STRINGTRIE_DEBUG
namespace {

using namespace Blaze;
void* operator new[](size_t size, char const*, int, unsigned int, char const*, int)
{
    return BLAZE_NEW_ARRAY(uint8_t, size);
}

void* operator new[](size_t size, unsigned int, unsigned int, char const*, int, unsigned int, char const*, int)
{
    return BLAZE_NEW_ARRAY(uint8_t, size);
}

void* operator new(size_t size)
{
    StringTrie::AllocationMetrics::sAllocBytes += size;
    StringTrie::AllocationMetrics::sTotalAllocBytes += size;
    if (StringTrie::AllocationMetrics::sAllocBytes > StringTrie::AllocationMetrics::sPeakAllocBytes)
        StringTrie::AllocationMetrics::sPeakAllocBytes = StringTrie::AllocationMetrics::sAllocBytes;
    size_t* ptr = (size_t*)BLAZE_ALLOC(size + sizeof(size_t));
    *ptr = size;
    return ptr + 1;
}

void* operator new[](size_t size)
{
    StringTrie::AllocationMetrics::sAllocBytes += size;
    StringTrie::AllocationMetrics::sTotalAllocBytes += size;
    if (StringTrie::AllocationMetrics::sAllocBytes > StringTrie::AllocationMetrics::sPeakAllocBytes)
        StringTrie::AllocationMetrics::sPeakAllocBytes = StringTrie::AllocationMetrics::sAllocBytes;
    size_t* ptr = (size_t*)BLAZE_ALLOC(size + sizeof(size_t));
    *ptr = size;
    return ptr + 1;
}

void operator delete(void* ptr)
{
    size_t* p = ((size_t*)ptr) - 1;
    StringTrie::AllocationMetrics::sAllocBytes -= *p;
    BLAZE_FREE(p);
}

void operator delete[](void* ptr)
{
    size_t* p = ((size_t*)ptr) - 1;
    StringTrie::AllocationMetrics::sAllocBytes -= *p;
    BLAZE_FREE(p);
}
}//anon ns

/////////////// MAIN

using namespace Blaze::GameManager;

typedef eastl::hash_map<uint32_t,eastl::string> WordMap;

int main(int argc, char** argv)
{
    const int minArgsExpected = 4;
    if (argc < minArgsExpected)
    {
        printf("invalid num args, needs %d", minArgsExpected);
        exit(1);
    }
    const uint32_t maxLength = EA::StdC::AtoU32(argv[3]);

    FILE *fpoutput = nullptr;
    if (argc > 4)
    {
        fpoutput = fopen(argv[4], "w");
        if (fpoutput == nullptr)
        {
            printf("Unable to open '%s'\n", argv[4]);
            exit(1);
        }
    }

    StringTrie::StringNormalizationTable normTable(true, nullptr);
    WordMap wordMap;
    StringTrie sub(3, UINT32_MAX, normTable, false);

    FILE *fp = fopen(argv[1], "r");
    if (fp == nullptr)
    {
        printf("Unable to open '%s'\n", argv[1]);
        exit(1);
    }
    int wordLenSum = 0;
    int wordLenSumNorm = 0;
    int wordCount = 0;
    char word[STRINGTRIE_MAX_MAXLENGTH];
    while (fgets(word, sizeof(word), fp) != nullptr)
    {
        size_t wordlen = strlen(word) - 1;
        word[wordlen] = '\0';
        wordLenSum += wordlen;
        ++wordCount;
        uint32_t idx = (uint32_t)wordMap.size() + 1;
        wordMap[idx] = word;

        char normalizedWord[STRINGTRIE_MAX_MAXLENGTH];
        wordLenSumNorm += normTable.normalizeWord(word, normalizedWord);
    }
    fclose(fp);

    StringTrie::AllocationMetrics::sTotalAllocBytes = 0;
    StringTrie::AllocationMetrics::sAllocBytes = 0;
    StringTrie::AllocationMetrics::sPeakAllocBytes = 0;
    StringTrie::AllocationMetrics::sNodes = 0;
    int64_t readStart = StringTrie::AllocationMetrics::getTimestamp();
    WordMap::iterator itr = wordMap.begin();
    WordMap::iterator end = wordMap.end();
    for(int32_t idx = 0; itr != end; ++itr, ++idx)
    {
        sub.addSubstrings(itr->second.c_str(), itr->first);
        //         char fn[256];
        //         sprintf(fn, "%04d-%s.dot", idx, itr->second.c_str());
        //         sub.buildDotFile(fn);
    }
    int64_t readEnd = StringTrie::AllocationMetrics::getTimestamp();

    printf("%lldus to build FSM\n", readEnd - readStart);
    printf("alloc bytes: %zd\n", StringTrie::AllocationMetrics::sAllocBytes);
    printf("total alloc bytes: %zd\n", StringTrie::AllocationMetrics::sTotalAllocBytes);
    printf("peak alloc bytes: %zd\n", StringTrie::AllocationMetrics::sPeakAllocBytes);
    printf("nodes: %zd\n", StringTrie::AllocationMetrics::sNodes);
    printf("average word length: %f\n", ((float)wordLenSum) / wordCount);
    printf("average normalized word length: %f\n", ((float)wordLenSumNorm) / wordCount);
    if (fpoutput != nullptr)
    {
        fprintf(fpoutput, "%lld us (%lld s) to build FSM\n", readEnd - readStart, ((readEnd - readStart) / 1000000) );
        fprintf(fpoutput, "alloc bytes: %zd (%zd mb)\n", StringTrie::AllocationMetrics::sAllocBytes, (StringTrie::AllocationMetrics::sAllocBytes / 1000000));
        fprintf(fpoutput, "total alloc bytes: %zd (%zd mb)\n", StringTrie::AllocationMetrics::sTotalAllocBytes, (StringTrie::AllocationMetrics::sTotalAllocBytes / 1000000));
        fprintf(fpoutput, "peak alloc bytes: %zd (%zd mb)\n", StringTrie::AllocationMetrics::sPeakAllocBytes, (StringTrie::AllocationMetrics::sPeakAllocBytes / 1000000));
        fprintf(fpoutput, "average word length: %f\n", ((float)wordLenSum) / wordCount);
        fprintf(fpoutput, "average normalized word length: %f\n", ((float)wordLenSumNorm) / wordCount);
    }

    readStart = StringTrie::AllocationMetrics::getTimestamp();
    StringTrie::WordId* matches = nullptr;
    uint32_t matchCount;
    matchCount = sub.findMatches(argv[2], &matches);
    if (matchCount > 0)
    {
        readEnd = StringTrie::AllocationMetrics::getTimestamp();
        uint32_t idx;
        for(idx = 0; idx < matchCount; ++idx)
            printf("%8d : %s\n", matches[idx], wordMap[matches[idx]].c_str());
        printf("Total matches: %u\n", idx);
        fprintf(fpoutput, "Total matches: %u\n", idx);
    }
    else
    {
        readEnd = StringTrie::AllocationMetrics::getTimestamp();
        printf("No matches.\n");
        fprintf(fpoutput, "No matches.\n");
    }
    printf("%lldus to find matches\n", readEnd - readStart);
    fprintf(fpoutput, "%lldus (%lld s) to find matches\n", readEnd - readStart, ((readEnd - readStart) / 1000000));

    size_t linkCounts[40];
    memset(linkCounts, 0, sizeof(linkCounts));
    sub.calculateLinkCounts(linkCounts);
    printf("Link counts:\n");
    fprintf(fpoutput, "Link counts:\n");
    for(int i = 0; i < 40; ++i)
    {
        printf("    %2d: %d\n", i, linkCounts[i]);
        fprintf(fpoutput, "    %2d: %d\n", i, linkCounts[i]);
    }

    //    sub.buildDotFile("gs.dot");
    return 0;
}//main

//BLAZE_STRINGTRIE_DEBUG
#endif

