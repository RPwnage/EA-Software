/*! ************************************************************************************************/
/*!
    \file   ruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/minfitthresholdlistcontainer.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const char8_t* MinFitThresholdListContainer::MIN_FIT_THRESHOLD_LISTS_KEY = "minFitThresholdLists";

    /*! \brief ctor for MinFitThresholdListContainer takes the associated rule name as a param for logging purposes. */
    MinFitThresholdListContainer::MinFitThresholdListContainer()
        : mRuleDefinition(nullptr)
    {}

    MinFitThresholdListContainer::~MinFitThresholdListContainer()
    {
        // free minFitThresholdLists
        while( !mMinFitThresholdLists.empty() )
        {
            MinFitThresholdList* list = mMinFitThresholdLists.begin()->second;
            mMinFitThresholdLists.erase(list->getName());
            delete list;
        }
    }

    /*! Helper function to ensure that the rule definition has been properly initialized. */
    const char8_t* MinFitThresholdListContainer::getRuleName() const { return mRuleDefinition == nullptr ? "ERR_NO_NAME" : mRuleDefinition->getName(); }

    bool MinFitThresholdListContainer::parseMinFitThresholdList(const FitThresholdsMap& minFitThresholdMap)
    {
        // try parsing each individual minFitThresholdList
        size_t numListsAdded = 0;

        FitThresholdsMap::const_iterator iter = minFitThresholdMap.begin();
        FitThresholdsMap::const_iterator end = minFitThresholdMap.end();
        for (; iter != end; ++iter)
        {
            const char8_t* listName = iter->first;
            const FitThresholdList* fitThresholdList = iter->second;
            MinFitThresholdList* newList = parseMinFitThresholdFromList(listName, *fitThresholdList);
            if (newList != nullptr)
            {
                if( addMinFitThresholdList(*newList) )
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
                ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : cannot parse minFitThresholdList \"" << listName << "\", skipping minFitList.");
                // note: we don't fail an entire rule because it has a single bad list, continue parsing the other lists
            }
        }

        // fail a rule that didn't add any minFitThresholdLists
        if (numListsAdded == 0)
        {
            ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : couldn't add any minFitThresholdLists to the rule!");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*!
    \brief parse the named minFitThresholdList out of the supplied map, and return a new
    MinFitThresholdList obj representing it.  Returns nullptr on error.

    \param[in]  listName - the name of the minFitThresholdList to load from the map.
    \param[in]  minFitThresholdListMap - the ruleDefn's minFitThresholdList map
    \return a newly allocated MinFitThresholdList, or nullptr (on error)
    *************************************************************************************************/
    MinFitThresholdList* MinFitThresholdListContainer::parseMinFitThresholdFromList(const char8_t* listName, const FitThresholdList& fitThresholdList)
    {
        EA_ASSERT(listName != nullptr);
        mCurrentThresholdListName = listName;

        MinFitThresholdList::MinFitThresholdPairContainer pairContainer;

        // pairs are stored as strings in the sequence: "<uint elapsedSeconds>:<float minFitThresholdValue>"
        // parse 'em out & put 'em into the pairContainer
        MinFitThresholdList::MinFitThresholdPair thresholdPair;
        const char8_t* thresholdPairString;

        FitThresholdList::const_iterator iter = fitThresholdList.begin();
        FitThresholdList::const_iterator end = fitThresholdList.end();
        for (; iter != end; ++iter)
        {
            thresholdPairString = *iter;
            if (parseMinFitThresholdPair(thresholdPairString, thresholdPair))
            {
                // we parsed a pair out of the sequence, add it to the container
                pairContainer.push_back(thresholdPair);
            }
            else
            {
                // errors have been logged, bail on this minFitThresholdList
                return nullptr;
            }
        }

        // now, try to construct a MinFitThresholdList from the container
        MinFitThresholdList* newList = BLAZE_NEW MinFitThresholdList();
        if (!newList->initialize(listName, pairContainer))
        {
            // note: init errors already logged
            delete newList;
            return nullptr;
        }

        return newList;
    }

    /*! ************************************************************************************************/
    /*!
    \brief parse a string encoding of a thresholdPair (ie: "<elapsedSeconds>:<minFitThreshold>"),
    and init the supplied thresholdPair to the parsed value.

    \param[in]  pairString - the string encoding fo a thresholdPair
    \param[in,out] thresholdPair - dest for the parsed thresholdPair values
    \param[in]  listName - the name of the thresholdList that's being parsed (used for error messages)
    \return true if the pair was parsed successfully, false if the pairString is invalid.
    *************************************************************************************************/
    bool MinFitThresholdListContainer::parseMinFitThresholdPair(const char8_t* pairString, MinFitThresholdList::MinFitThresholdPair &thresholdPair) const
    {
        // early test for negative numbers (none allowed)
        // we need to do this, since sscanf %u will read a signed number and cast it to unsigned, rather than error out
        if (strchr(pairString, '-') == nullptr)
        {
            // parse out the left hand side (the elapsed seconds uint32_t)
            if ( (pairString != nullptr) && (pairString[0] != '\0') )
            {
                // get elapsed seconds
                uint32_t elapsedSeconds = 0;
                char8_t nextChar = '\0';
                int numScanned = sscanf(pairString, "%u%c", &elapsedSeconds, &nextChar);
                if (numScanned == 2)
                {
                    // ensure char after number is either whitespace or ':'
                    if ( (isspace(nextChar) || nextChar == ':') )
                    {

                        // get delimiter ':'
                        const char8_t* delimiter = strchr(pairString, ':');
                        if (delimiter != nullptr)
                        {
                            float minFitThreshold = 0;
                            const char8_t* minFitString = delimiter + 1;

                            // parse the right hand side of the ':' (the minFitThreshold value)
                            // should be a float, or "EXACT_MATCH_REQUIRED"
                            if (parseMinFitThresholdValue(minFitString, minFitThreshold))
                            {
                                // success
                                thresholdPair.first = elapsedSeconds;
                                thresholdPair.second = minFitThreshold;
                                return true;
                            }
                        }
                    }
                }
            }
        }

        // syntax error; log error
        ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : Problem parsing minFitThreshold \"" 
                << mCurrentThresholdListName << "\", found \"" << pairString << "\", but expected one of:\n"
                << "\t<uint elapsedSeconds>:<float minFitThreshold (between 0 and 1)>\n"
                << "\t<uint elapsedSeconds>:" << (MatchmakingConfig::EXACT_MATCH_REQUIRED_TOKEN) << "\n"
                << "Ignoring minFitThreshold pair.");

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief parse the minFitThresholdString into a float.  Note: handles the "EXACT_MATCH_REQUIRED"
    token or a plain old float value.

    \param[in]  minFitThresholdStr - a minFitThreshold value string.  Either a float, or "EXACT_MATCH_REQUIRED".
    \param[in]  ruleDefn - the rule definition to add parsed minFitThresholdLists to.
    \param[out] minFitThreshold - output float for the parsed minFitThresholdStr.
    \return true if the string was parsed successfully, false if it's invalid.
    *************************************************************************************************/
    bool MinFitThresholdListContainer::parseMinFitThresholdValue(const char8_t* minFitThresholdStr, float &minFitThreshold) const
    {
        // note: we use strtok so we don't have to deal with trimming whitespace from the EXACT_MATCH_REQUIRED token...
        char8_t* saveptr;
        char8_t* buffer = blaze_strdup(minFitThresholdStr);
        char8_t* token = blaze_strtok(buffer, " ", &saveptr);

        // check if there are multiple tokens (expecting only 1)
        char8_t* nextToken = blaze_strtok(nullptr, " ", &saveptr);
        if (nextToken != nullptr)
        {
            // error: unexpected 2nd token in minFitThresholdString
            ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : minFitThreshold list \"" 
                    << mCurrentThresholdListName << "\" has invalid threshold \"" << minFitThresholdStr << "\" (unexpected token encountered)");
            BLAZE_FREE(buffer);
            return false;
        }

        // try to get a float from the token
        uint8_t allowedType = getLiteralThresholdInfo().getThresholdAllowedValueType();
        if (allowedType & ALLOW_NUMERIC)
        {
            int numScanned = sscanf(token, "%f", &minFitThreshold);
            if (numScanned == 1)
            {
                BLAZE_FREE(buffer);
                if ( (minFitThreshold >= 0.0f) && (minFitThreshold <= 1.0f) )
                {
                    return true;
                }
                else
                {
                    char8_t buf[32];
                    blaze_snzprintf(buf, sizeof(buf), "%.2f", minFitThreshold);
                    ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : minFitThreshold list \"" 
                            << mCurrentThresholdListName << "\" has invalid threshold (" << buf << "); should be between 0..1 (inclusive)");
                    return false;
                }
            }
        }

        if (allowedType & ALLOW_EXACT_MATCH)
        {
            // no float, check for EXACT_MATCH_REQUIRED in the token
            if (blaze_stricmp(MatchmakingConfig::EXACT_MATCH_REQUIRED_TOKEN, token) == 0)
            {
                BLAZE_FREE(buffer);
                minFitThreshold = MinFitThresholdList::EXACT_MATCH_REQUIRED;
                return true;
            }
        }

        if (allowedType & ALLOW_LITERALS)
        {
            if ((minFitThreshold = getLiteralThresholdInfo().getFitscoreByThresholdLiteral(minFitThresholdStr)) == -1)
            {
                ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : minFitThreshold list \"" << mCurrentThresholdListName 
                        << "\" has invalid threshold literal value (" << minFitThresholdStr << "); did not find in literal list of the rule.");
            }
            else
            {
                BLAZE_FREE(buffer);
                return true;
            }
        }

        // error: invalid token (caller will log error)
        BLAZE_FREE(buffer);
        return false;
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
    bool MinFitThresholdListContainer::addMinFitThresholdList(MinFitThresholdList& newList)
    {
        const char8_t* listName = newList.getName();

        if (mMinFitThresholdLists.find(listName) == mMinFitThresholdLists.end())
        {
            // list name is unique; add it
            mMinFitThresholdLists[newList.getName()] = &newList;
            return true;
        }

        //error: list name is a duplicate
        ERR_LOG("[MinFitThresholdListContainer] - Parsing rule \"" << getRuleName() << "\" : already contains a minFitThresholdList named \"" << listName << "\".");
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief lookup and return one of this rule's minFitThresholdLists.  Returns nullptr if no list exists
    with name.

    \param[in] name The name of the thresholdList to get.  Case-insensitive.
    \return The MinFitThresholdList, or nullptr if the name wasn't found.
    *************************************************************************************************/
    const MinFitThresholdList* MinFitThresholdListContainer::getMinFitThresholdList(const char8_t* name) const
    {
        MinFitThresholdLists::const_iterator iter = mMinFitThresholdLists.find(name);
        if (iter != mMinFitThresholdLists.end())
        {
            return iter->second;
        }

        return nullptr; // not found
    }

    /*! ************************************************************************************************/
    /*! \brief write the minFitThresholdListInfo into the given thresholdNameList
            The threshold list passed in should be empty or method will assert

        \param[in\out] thresholdNameList - an empty list to fill with threshold names
    ***************************************************************************************************/
    void MinFitThresholdListContainer::initMinFitThresholdListInfo( MinFitThresholdNameList& thresholdNameList ) const
    {
        EA_ASSERT(thresholdNameList.empty());

        MinFitThresholdLists::const_iterator listIter = mMinFitThresholdLists.begin();
        MinFitThresholdLists::const_iterator listEnd = mMinFitThresholdLists.end();
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
    void MinFitThresholdListContainer::writeMinFitThresholdListsToString(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = {\n", prefix, MinFitThresholdList::MIN_FIT_THRESHOLD_LISTS_CONFIG_KEY);
        MinFitThresholdLists::const_iterator listIter = mMinFitThresholdLists.begin();
        MinFitThresholdLists::const_iterator listEnd = mMinFitThresholdLists.end();
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
