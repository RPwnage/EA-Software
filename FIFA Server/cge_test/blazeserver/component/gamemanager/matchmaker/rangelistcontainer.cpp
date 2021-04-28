/*! ************************************************************************************************/
/*!
    \file   rangelistcontainer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    const char8_t* RangeListContainer::RANGE_OFFSET_LISTS_KEY = "rangeOffsetLists";

    const char8_t* RangeListContainer::INFINITY_RANGE_TOKEN = "INF";
    const char8_t* RangeListContainer::EXACT_MATCH_REQUIRED_TOKEN = "EXACT_MATCH_REQUIRED";

    /*! \brief ctor for MinFitThresholdListContainer takes the associated rule name as a param for logging purposes. */
    RangeListContainer::RangeListContainer()
        : mRuleName(nullptr)
    {}

    RangeListContainer::~RangeListContainer()
    {
        // free range offset lists
        RangeOffsetListMap::const_iterator mapIter = mRangeOffsetListMap.begin();
        RangeOffsetListMap::const_iterator mapEnd = mRangeOffsetListMap.end();
        for ( ; mapIter != mapEnd; ++mapIter)
        {
            delete mapIter->second;
        }
    }

    /*! Helper function to ensure that the rule definition has been properly initialized. */
    const char8_t* RangeListContainer::getRuleName() const { return mRuleName == nullptr ? "ERR_NO_NAME" : mRuleName; }

    /*! \param[in] allowOffsetPairs if true, the range offset list offsets allow (in addition to the single valued form 'offset=[x]')
        the (negative/positive) pair form 'offset=[x,y]'. If false, only the single valued offset form is allowed. */
    bool RangeListContainer::initialize(const char8_t* ruleName, const GameManager::RangeOffsetLists& lists, bool allowOffsetPairs /*= true*/)
    {
        if (lists.empty())
        {
            ERR_LOG("[RangeListContainer].initialize - rule '" << ruleName << "' : contains no range offset lists.");
            return false;
        }

        GameManager::RangeOffsetLists::const_iterator itr = lists.begin();
        GameManager::RangeOffsetLists::const_iterator end = lists.end();

        size_t numListsAdded = 0;
        for (; itr != end; ++itr)
        {
            const GameManager::RangeOffsetList& rangeOffsetList = **itr;

            RangeList* newList = createRangeList(rangeOffsetList, allowOffsetPairs);
            if (newList != nullptr)
            {
                if( addRangeList(*newList) )
                {
                    // add success: newList now owned by ruleDefn
                    numListsAdded++;
                }
                else
                {
                    // error adding newList
                    delete newList;
                }
            }
            else
            {
                WARN_LOG("[RangeListContainer].initialize - rule '" << ruleName << "' : cannot parse rangeOffsetList '" << rangeOffsetList.getName() << "', skipping.");
                // note: we don't fail an entire rule because it has a single bad list, continue parsing the other lists
            }
        }

        // fail a rule that didn't add any rangeOffsetList
        if (numListsAdded == 0)
        {
            ERR_LOG("[RangeListContainer].initialize - rule '" << ruleName << "' : couldn't add any rangeOffsetLists.");
            return false;
        }

        return true;
    }

    RangeList* RangeListContainer::createRangeList(const GameManager::RangeOffsetList& rangeOffsetList, bool allowOffsetPairs) const
    {
        RangeList::RangePairContainer pairContainer;
        RangeList::RangePair rangePair;

        if (rangeOffsetList.getRangeOffsets().empty())
        {
            ERR_LOG("[RangeListContainer].createRangeList ERROR Range list '" << rangeOffsetList.getName() << "' is empty.");
            return nullptr;
        }

        GameManager::RangeOffsets::const_iterator itr = rangeOffsetList.getRangeOffsets().begin();
        GameManager::RangeOffsets::const_iterator end = rangeOffsetList.getRangeOffsets().end();
        for (; itr != end; ++itr)
        {
            const GameManager::RangeOffset& rangeOffset = **itr;

            rangePair.first = rangeOffset.getT();
            if (!parseRange(rangeOffset.getOffset(), rangePair.second))
            {
                ERR_LOG("[RangeListContainer].createRangeList ERROR Failed to parse range for list '" << rangeOffsetList.getName() << "' @ time '" << rangeOffset.getT() << "'.");
                return nullptr;
            }

            if (!allowOffsetPairs && (rangeOffset.getOffset().size() == 2))
            {
                ERR_LOG("[RangeListContainer].createRangeList ERROR Range Offset list may only contain 1 value, for list '" << rangeOffsetList.getName() << "' @ time '" << rangeOffset.getT() << "'.");
                return nullptr;
            }

            pairContainer.push_back(rangePair);
        }

        // now, try to construct an RangeList from the container
        RangeList* newList = BLAZE_NEW RangeList();
        if (!newList->initialize(rangeOffsetList.getName(), pairContainer))
        {
            // note: init errors already logged
            delete newList;
            return nullptr;
        }

        return newList;
    }

    bool RangeListContainer::parseRange(const GameManager::OffsetList& offsetList, RangeList::Range& range)
    {
        if ((offsetList.size() != 1) && (offsetList.size() != 2))
        {
            ERR_LOG("[RangeListContainer].createRangeList ERROR Range Offset list must contain 1 or 2 values.");
            return false;
        }

        if (!parseRangeToken(offsetList[0], range.mMinValue))
        {
            return false;
        }

        // If only 1 value is specified, use it for both the min and max.
        if (offsetList.size() == 1)
        {
            range.mMaxValue = range.mMinValue;
        }
        else if (!parseRangeToken(offsetList[1], range.mMaxValue))
        {
            return false;
        }

        return true;
    }

    bool RangeListContainer::parseRangeToken(const char8_t* token, int64_t& rangeValue)
    {
        if ((token == nullptr) || (token[0] == '\0'))
        {
            ERR_LOG("[RangeListContainer].parseRangeToken ERROR Failed to parse nullptr token");
            return false;
        }

        if (blaze_stricmp(token, EXACT_MATCH_REQUIRED_TOKEN) == 0)
        {
            rangeValue = RangeList::EXACT_MATCH_REQUIRED_VALUE;
            return true;
        }
        else if (blaze_stricmp(token, INFINITY_RANGE_TOKEN) == 0)
        {
            rangeValue = RangeList::INFINITY_RANGE_VALUE;
            return true;
        }
        else
        {
            const char8_t* retVal = blaze_str2int(token, &rangeValue);
            if ((retVal == nullptr) || (retVal[0] != '\0'))
            {
                ERR_LOG("[RangeListContainer].parseRangeToken ERROR Failed to parse token '" << token << "'");
                return false;
            }

            if (rangeValue < 0)
            {
                ERR_LOG("[RangeListContainer].parseRangeToken ERROR token out of valid range - '" << rangeValue << "'");
                return false;
            }
            return true;
        }
    }

    /*! ************************************************************************************************/
    /*!
    \brief Add a new MinFitThresholdList to this ruleDefinition.  On success, the newList pointer
    is managed by this rule, and will be deleted with the rule.

    Note: Each MinFitThresholdList in a ruleDefn must have a unique name.

    \param[in] newList - a newly allocated MinFitThresholdList (heap pointer).
    \return true on success, false on failure.  NOTE: on success, the ruleDefinition takes responsibility
    for freeing the pointer (it's deleted when the rule is).  On failure, the pointer is unaltered,
    and it's the caller's responsibility.
    *************************************************************************************************/
    bool RangeListContainer::addRangeList(RangeList& newList)
    {
        const char8_t* listName = newList.getName();

        if (mRangeOffsetListMap.find(listName) == mRangeOffsetListMap.end())
        {
            // list name is unique; add it
            mRangeOffsetListMap[listName] = &newList;
            return true;
        }
        else
        {
            //error: list name is a duplicate
            ERR_LOG(LOG_PREFIX << "rule \"" << mRuleName << "\" : already contains a RangeOffsetList named \"" << listName << "\".");
            return false;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief lookup and return one of this rule's minFitThresholdLists.  Returns nullptr if no list exists
    with name.

    \param[in] name The name of the thresholdList to get.  Case-insensitive.
    \return The MinFitThresholdList, or nullptr if the name wasn't found.
    *************************************************************************************************/
    const RangeList* RangeListContainer::getRangeList(const char8_t *listName) const
    {
        RangeOffsetListMap::const_iterator iter = mRangeOffsetListMap.find(listName);
        if (iter != mRangeOffsetListMap.end())
        {
            return iter->second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief write the minFitThresholdListInfo into the given thresholdNameList
            The threshold list passed in should be empty or method will assert

        \param[in\out] thresholdNameList - an empty list to fill with threshold names
    ***************************************************************************************************/
    void RangeListContainer::initRangeListInfo( MinFitThresholdNameList& thresholdNameList ) const
    {
        EA_ASSERT(thresholdNameList.empty());

        RangeOffsetListMap::const_iterator listIter = mRangeOffsetListMap.begin();
        RangeOffsetListMap::const_iterator listEnd = mRangeOffsetListMap.end();
        while(listIter != listEnd)
        {
            //insert threshold name
            thresholdNameList.push_back(listIter->first);
            ++listIter;
        }
    }

    /*! ************************************************************************************************/
    /*!
    \brief logging helper: write minFitThresholdLists into dest using the matchmaking config format.

    \param[out] dest - a destination eastl string to print into
    \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
    ***************************************************************************************************/
    void RangeListContainer::writeRangeListsToString(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = {\n", prefix, MinFitThresholdList::MIN_FIT_THRESHOLD_LISTS_CONFIG_KEY);
        RangeOffsetListMap::const_iterator listIter = mRangeOffsetListMap.begin();
        RangeOffsetListMap::const_iterator listEnd = mRangeOffsetListMap.end();
        eastl::string listStr;
        while(listIter != listEnd)
        {
            listIter->second->toString(listStr);
            dest.append_sprintf("%s    %s\n", prefix, listStr.c_str());
            ++listIter;
        }
        dest.append_sprintf("%s  }\n", prefix);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
