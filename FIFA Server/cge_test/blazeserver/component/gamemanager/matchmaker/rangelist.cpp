/*! ************************************************************************************************/
/*!
    \file rangelist.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for matchmaking config token names (used in log messages)
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const int64_t RangeList::INFINITY_RANGE_VALUE = -1;
    const int64_t RangeList::EXACT_MATCH_REQUIRED_VALUE = 0;  // Should this be a special value to designate Exact Match?

    /*! ************************************************************************************************/
    /*!
        \brief Initialize & validate a RangeList with a given name & a vector of threshold pairs.
            Returns true on success, false on a fatal error.

        fatal errors:
            listName is nullptr or "".
            thresholdPairs is empty.

        warnings:
            listName is too long (name truncated)
            multiple pairs defined at time x (first pair at time X saved; other are ignored)
            no pair defined at time 0 (the 'fist' pair takes effect at time 0)

        \param[in]  listName - the name of the threshold list.  Case-insensitive, must be unique within
                        each rule.  Name will be truncated to 15 characters (not including the null char).
        \param[in]  rangeContainer - a vector of RangePairs that are validated and saved off
        \return true on success, false on fatal errors.
    *************************************************************************************************/
    bool RangeList::initialize(const char8_t* listName, const RangePairContainer &rangePairContainer)
    {
        return (initName(listName) && initRangeList(rangePairContainer));
    }

    /*! ************************************************************************************************/
    /*! \brief init the list's name.  Returns false if listName is nullptr or empty.

            Note: name will be truncated to 15 characters (to fit inside a MinFitThresholdName buffer)

        \param[in]  listName - the list's name
        \return true on success, false on fatal error
    *************************************************************************************************/
    bool RangeList::initName(const char8_t* listName)
    {
        // non-nullptr, non-empty
        if ( (listName == nullptr) || (listName[0] == '\0') )
        {
            WARN_LOG(LOG_PREFIX << "Error: trying to construct RangeList without a valid name.");
            return false;
        }

        mName.set(listName);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief init the list's range container.  Returns false on error (after logging error).

            Note: the list logs a warning and discards duplicated pairs for a given time.  Also logs
            a warning if no pair is defined for time 0.

        \param[in]  thresholdPairs - a vector containing the thresholdPairs to use for this list.
        \return true on success, false on fatal error
    *************************************************************************************************/
    bool RangeList::initRangeList(const RangePairContainer &rangePairContainer)
    {
        if (rangePairContainer.empty())
        {
            ERR_LOG(LOG_PREFIX << "RangeOffsetList \"" << mName.c_str() << "\" is empty; it must contain at least one rangePair (ex: <sessionAgeSeconds>:<minRange>..<maxRange>)");
            return false;
        }

        size_t numPairs = rangePairContainer.size();
        mRangePairContainer.reserve(numPairs);
        for(size_t i=0; i<numPairs; ++i)
        {
            // each time value must be unique
            uint32_t elapsedSeconds =  rangePairContainer[i].first;
            if( !isRangeDefined(elapsedSeconds) )
            {
                // unique time; add the pair
                mRangePairContainer.push_back(rangePairContainer[i]);
            }
            else
            {
                // duplicate time; skip this pair
                WARN_LOG(LOG_PREFIX << "RangeOffsetList \"" << mName.c_str() << "\" contains multiple definitions for time " << elapsedSeconds << "; ignoring redefinitions.");
            }
        }

        // now we sort the pairs by elapsed seconds
        eastl::sort(mRangePairContainer.begin(), mRangePairContainer.end(), RangePairSortComparitor());

        // check if a pair was defined at time 0.
        if ( !isRangeDefined(0))
        {
            // set the time of the first pair to zero
            mRangePairContainer[0].first = 0;
            WARN_LOG(LOG_PREFIX << "RangeOffsetList \"" << mName.c_str() << "\" has no Range for time 0; changing first range time to zero (0:" << mRangePairContainer[0].second.mMinValue << ".." << mRangePairContainer[0].second.mMaxValue << ")");
        }

        // enforce that the ranges never become more restrictive over time.
        if (mRangePairContainer.size() > 1)
        {
            // validate that the ranges expand over time - CreateGame MM assumes that rules relax/decay and will break if something constricts.

            RangePairContainer::const_iterator rangePairIter = mRangePairContainer.begin();
            const Range *prevRange = &rangePairIter->second;
            ++rangePairIter;
            RangePairContainer::const_iterator end = mRangePairContainer.end();
            for ( ; rangePairIter != end; ++rangePairIter )
            {
                uint32_t currentTime = rangePairIter->first;
                const Range &range = rangePairIter->second;

                if ((range.mMaxValue != RangeList::INFINITY_RANGE_VALUE) && ((prevRange->mMaxValue == RangeList::INFINITY_RANGE_VALUE) || (range.mMaxValue < prevRange->mMaxValue)))
                {
                    // err, max range has constricted over time
                    ERR_LOG(LOG_PREFIX << "RangeOffsetList \"" << mName.c_str() << "\" constricts over time; but values must remain the same or relax/decay.  Problem is at time " 
                            << currentTime << ", the max value constricts to " << range.mMaxValue << " from the previous value " << prevRange->mMaxValue << ".");
                    return false;
                }

                if ((range.mMinValue != RangeList::INFINITY_RANGE_VALUE) && ((prevRange->mMinValue == RangeList::INFINITY_RANGE_VALUE) || (range.mMinValue < prevRange->mMinValue)))
                {
                    // err, min range has constricted over time
                    ERR_LOG(LOG_PREFIX << "RangeOffsetList \"" << mName.c_str() << "\" constricts over time; but values must remain the same or relax/decay.  Problem is at time " 
                            << currentTime << ", the min value constricts to " << range.mMinValue << " from the previous value " << prevRange->mMinValue << ".");
                    return false;
                }

                prevRange = &range;
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief return the range at the given session age and the age of the next threshold change (next decay time).
    ***************************************************************************************************/
    const RangeList::Range* RangeList::getRange(uint32_t elapsedSeconds, uint32_t *nextThresholdSeconds) const
    {
        if (mRangePairContainer.empty())
        {
            EA_FAIL();
            if (nextThresholdSeconds != nullptr)
            {
                *nextThresholdSeconds = UINT32_MAX;
            }
            return nullptr;
        }

        // note: threshold list is sorted by elapsed time
        RangePairContainer::const_iterator pairIter = mRangePairContainer.begin();
        RangePairContainer::const_iterator end = mRangePairContainer.end();
        const Range *range = &(pairIter->second);
        for ( ; pairIter != end; ++pairIter )
        {
            uint32_t currentRangeAge = pairIter->first;
            if ( currentRangeAge <= elapsedSeconds)
            {
                range = &(pairIter->second);
            }
            else
            {
                // we've gone too far, this is the 'next' decay time
                if (nextThresholdSeconds != nullptr)
                {
                    *nextThresholdSeconds = currentRangeAge;
                }                
                return range;
            }
        }

        // not found, use 'last' value
        if (nextThresholdSeconds != nullptr)
        {
            //There is no higher level, so there will never be another threshold to go to.
            *nextThresholdSeconds = UINT32_MAX;
        }
        return range;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if this list contains a range at elapsedSeconds.
    
        \param[in]  elapsedSeconds - the time to examine for a defined thresholdPair
        \return true if a thresholdPair is defined at time elapsedSeconds; false otherwise
    *************************************************************************************************/
    bool RangeList::isRangeDefined(size_t elapsedSeconds) const
    {
        // note: called before list is sorted, so we can't rely on order to early-out

        RangePairContainer::const_iterator pairIter = mRangePairContainer.begin();
        RangePairContainer::const_iterator end = mRangePairContainer.end();
        for ( ; pairIter != end; ++pairIter )
        {
            if (elapsedSeconds == pairIter->first)
            {
                return true;
            }
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Write the RangeList into an eastl string for logging.  NOTE: inefficient.

        The RangeList is written into the string using the config file syntax:
            "<listName> = [ <time1>:<minRange1>..<maxRange1>, <time2>:<minRange2>..<maxRange2>, ... ]"

        \param[in/out]  str - the RangeList is written into this string, blowing away any previous value.
    *************************************************************************************************/
    void RangeList::toString(eastl::string &str) const
    {
        EA_ASSERT(!mRangePairContainer.empty());

        str.sprintf("%s = [ ", mName.c_str());
        size_t lastIndex = mRangePairContainer.size() - 1;
        for(size_t i=0; i<=lastIndex; ++i)
        {
            uint32_t elaspedSeconds = mRangePairContainer[i].first;
            const Range &range = mRangePairContainer[i].second;

            char8_t pairStr[48];
            blaze_snzprintf(pairStr, sizeof(pairStr), "%u:%" PRId64"..%" PRId64, elaspedSeconds, range.mMinValue, range.mMaxValue);

            if (i != lastIndex)
                str.append_sprintf("%s, ", pairStr);
            else
                str.append_sprintf("%s ]", pairStr);
        }
    }


    void RangeList::Range::setRange(int64_t center, const Range &rangeOffset, int64_t minClamp /* = INT64_MIN */, int64_t maxClamp /* = INT64_MAX*/)
    {
        if (rangeOffset.mMinValue == INFINITY_RANGE_VALUE)
        {
            mMinValue = minClamp;
        }
        else
        {
            mMinValue = center - rangeOffset.mMinValue;
        }

        // MM_AUDIT_TODO: This doesn't handle overflows properly...
        // clamp the min value
        if (mMinValue < minClamp)
        {
            SPAM_LOG("[RangeList].setRange center '" << center << "', minOffset '" << rangeOffset.mMinValue << "', mMinValue '" 
                      << mMinValue << "' outside of min range, clamping to minClamp '" << minClamp << "'");
            mMinValue = minClamp;
        }
        if (mMinValue > maxClamp)
        {
            SPAM_LOG("[RangeList].setRange center '" << center << "', minOffset '" << rangeOffset.mMinValue << "', mMinValue '" 
                      << mMinValue << "' outside of max range, clamping to maxClamp '" << minClamp << "'");
            mMinValue = maxClamp;
        }

        if (rangeOffset.mMaxValue == INFINITY_RANGE_VALUE)
        {
            mMaxValue = maxClamp;
        }
        else
        {
            mMaxValue = center + rangeOffset.mMaxValue;
        }

        // clamp the max value
        if (mMaxValue < minClamp)
        {
            SPAM_LOG("[RangeList].setRange center '" << center << "', maxOffset '" << rangeOffset.mMaxValue << "', mMaxValue '" 
                      << mMaxValue << "' outside of min range, clamping to minClamp '" << minClamp << "'");
            mMaxValue = minClamp;
        }
        if (mMaxValue > maxClamp)
        {
            SPAM_LOG("[RangeList].setRange center '" << center << "', maxOffset '" << rangeOffset.mMaxValue << "', mMaxValue '" 
                      << mMaxValue << "' outside of max range, clamping to maxClamp '" << maxClamp << "'");
            mMaxValue = maxClamp;
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
