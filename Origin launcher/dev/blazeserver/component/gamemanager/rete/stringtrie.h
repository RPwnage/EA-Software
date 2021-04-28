/*! ************************************************************************************************/
/*!
    \file stringtrie.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_STRINGTRIE_H
#define BLAZE_STRINGTRIE_H

#include <EABase/eabase.h>
#include <stdio.h>

namespace Blaze
{
    const uint32_t STRINGTRIE_MAX_MAXLENGTH = 1024; // max word maxLength supportable by this trie

    /*! ************************************************************************************************/
    /*! \brief Blaze implementation of a Radix Trie for storing search-able strings compactly
        http://en.wikipedia.org/wiki/Radix_tree.
    ***************************************************************************************************/
    class StringTrie
    { 
        NON_COPYABLE(StringTrie);
    public:
#ifdef BLAZE_STRINGTRIE_DEBUG
        struct AllocationMetrics // internal memory metrics
        {
            static size_t sTotalAllocBytes;
            static size_t sAllocBytes;
            static size_t sPeakAllocBytes;
            static size_t sNodes;
            static int64_t getTimestamp()
            {
                Blaze::TimeValue tv = Blaze::TimeValue::getTimeOfDay();
                return (((int64_t)tv.getSec()) * 1000000) + tv.getMicroSeconds();
            }
        };
        void calculateLinkCounts(size_t linkCounts[]) const;
        void buildDotFile(const char* dotFile) const;
// BLAZE_STRINGTRIE_DEBUG
#endif

        /*! ************************************************************************************************/
        /*! \brief WordId - Unique id corresponding to instance of an added word (and its substrings).
            May be associated with some external entity; differentiates dupe word instances.
            Each node the word (or its substrings) 'matches' at in trie, gets a copy of this id.
        ***************************************************************************************************/
        typedef uint64_t WordId;

        /*! ************************************************************************************************/
        /*! \brief Class StringNormalizationTable. Specifies and inits internal normalization char translation table
        ***************************************************************************************************/
        class StringNormalizationTable
        {
        public:
            StringNormalizationTable(bool ignoreCase, const char8_t* relevantChars);
            uint32_t normalizeWord(const char* word, char* normalizedWord) const;
            void unnormalizeWord(const char* normalizedWord, char* word) const;
        private:
            int32_t mXlat[256];
        };
        
        /*! ************************************************************************************************/
        /*! \brief StringTrie ctor.
        ***************************************************************************************************/
        StringTrie(uint32_t minLength, uint32_t maxLength, const StringNormalizationTable& normalizationParams,
            bool leadingPrefixOnly = false);
        /*! ************************************************************************************************/
        /*! \brief StringTrie dtor.
        ***************************************************************************************************/
        ~StringTrie();

        uint32_t findMatches(const char* substring, WordId** matches);
        bool addSubstrings(const char* word, const WordId id);
        uint32_t removeSubstrings(const char* word, const WordId id);

        uint32_t getMaxLength() const { return mMaxLength; }
        uint32_t getMinLength() const { return mMinLength; }

    private:
                
        /*! ************************************************************************************************/
        /*! \brief StringTrie::Node. The radix trie head node is an empty sentinel to start traversals at.
            Each non-head node holds a >= 1 length char segment of search-able strings stored in the trie.
            I.e. level 1 holds prefixes including start char, their children hold following char segments, etc.
        ***************************************************************************************************/
        struct Node
        {
            // !!!!!!!!!
            // Due to the number of nodes which can be created, order of the member variables is
            // important to ensure that the Node structure isn't any larger than necessary due to
            // structure packing rules.
            // !!!!!!!!!

            // List of the word IDs that match to this node
            WordId* mMatches;

            // List of the children nodes
            Node* mLinks;

            // Current size of mLinks
            uint32_t mLinkCount;

            // Current size of mMatches
            uint32_t mMatchCount;

            // The leading prefix characters for the edge leading to this node from its parent
            char* mLinkChars;

            Node();
            ~Node();

            void deleteMemberArrays();

            Node* getOrAddLink(const char* prefix, uint32_t prefixLen, uint32_t& consumedChars);
            void removeLinkNode(uint32_t idx);
            void mergeWithMyLinkNode();
            Node* getLink(const char*& prefix, uint32_t* foundLinkIndex = nullptr, uint32_t* consumedChars = nullptr) const;
            void addMatch(const WordId wordId);
            bool removeMatch(const WordId wordId);

            // get held chars count
            uint32_t getLinkCharsLen() const { return (mLinkChars == nullptr)? 0 : strlen(mLinkChars); }

            // Whether leaf
            bool isLeaf() const { EA_ASSERT((mLinkCount != 0) || (mLinks == nullptr)); return (mLinkCount == 0); }

            // Whether has ids. Empty iff path from root to this node consumes < min length chars
            bool isEmpty() const { EA_ASSERT((mMatchCount != 0) || (mMatches == nullptr)); return (mMatchCount == 0); }

            // Whether this node can be deleted (i.e. leaf without match ids)
            bool isRemovable() const { return (isLeaf() && isEmpty()); }

            // Whether can merge with my 1 child. I.e. whether node has no ids child doesn't also have
            inline bool canMergeWithMyLink(WordId removedWordId) const;

            // empties ids from node and sets match count 0
            void clearMatches() { deleteMatches(); mMatchCount = 0; }

            void printMatches(eastl::string &dest) const;
            void print(eastl::string &dest) const;

        private:
            Node* addLinkNode(const char* prefix, uint32_t prefixLen);
            inline bool findWordId(const WordId wordId, uint32_t* outFoundIdx = nullptr);

            // return my object and set my ptr to nullptr
            Node* orphanLinks()
            {
                Node* t = mLinks;
                mLinks = nullptr;
                mLinkCount = 0;
                return t;
            }
            // sets my ptr and update count. old object deleted as needed
            void adoptLinks(Node* links, uint32_t linkCount)
            { 
                deleteLinks();
                mLinks = links;
                mLinkCount = linkCount;
            }
            // return my object and set my ptr to nullptr
            WordId* orphanMatches()
            { 
                WordId* t = mMatches;
                mMatches = nullptr;
                mMatchCount = 0;
                return t;
            }
            // sets my ptr and update count. old object deleted as needed
            void adoptMatches(WordId* matches, uint32_t matchCount)
            {
                deleteMatches();
                mMatches = matches;
                mMatchCount = matchCount;
            }
            // calls delete on object as needed. does not update count
            void deleteLinkChars() { if(mLinkChars != nullptr) { delete[] mLinkChars; } mLinkChars = nullptr; }
            void deleteLinks() { if(mLinks != nullptr) { delete[] mLinks; } mLinks = nullptr; }
            void deleteMatches() { if(mMatches!=nullptr) { delete[] mMatches; } mMatches = nullptr; }
        }; // class Node


    // StringTrie private vars
    private:

        Node mStartNode;

        // Minimum substring length
        uint32_t mMinLength;

        // Maximum substring length
        uint32_t mMaxLength;

        StringNormalizationTable mNormalizationTable;

        // Controls if only leading prefixes are supported.  For example, if this value is true, then
        // a search for the substring "foo" would match "football" but not "seafood".
        bool mLeadingPrefixOnly;

    private:
        void addNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id); 
        uint32_t removeNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id);
        uint32_t removeNormalizedSubstrings(const char* normalizedWord, uint32_t normalizedWordLen, const WordId id, Node& startNode);

        inline bool validateNodeMatchCountBound(const Node& child, uint32_t value, const Node* parent = nullptr) const;
        void removeChildMatchIdsNotOnParent(const Node& parent, Node& child);

#ifdef BLAZE_STRINGTRIE_DEBUG
        void calculateLinkCounts(const struct Node* node, size_t linkCounts[]) const;
        void buildDotFile(const struct Node* node, FILE* fp) const;
#endif
    };// class StringTrie

} // ns Blaze



/////////////// MAIN: INTERNAL TEST DRIVER
#ifdef BLAZE_STRINGTRIE_DEBUG
using namespace Blaze::GameManager;

typedef eastl::hash_map<uint32_t,eastl::string> WordMap;

int main(int argc, char** argv);
//BLAZE_STRINGTRIE_DEBUG
#endif

// BLAZE_STRINGTRIE_H
#endif 


