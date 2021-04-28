/*! ************************************************************************************************/
/*!
    \file   minfitthresholdlist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for matchmaking config token names (used in log messages)
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    const float MinFitThresholdList::EXACT_MATCH_REQUIRED(-1.0f);
    const char8_t* MinFitThresholdList::MIN_FIT_THRESHOLD_LISTS_CONFIG_KEY = "minFitThresholdLists";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized MinFitThresholdList.  NOTE: do not use until successfully
            initialized.

        MinFitThresholdLists are typically defined in the matchmaking config file and initialized
        automatically by the component at startup.

    *************************************************************************************************/
    MinFitThresholdList::MinFitThresholdList() : mIsExactMatchRequired(false), mRequiresExactMatchDecayAge(0)
    {
    }

    /*! ************************************************************************************************/
    /*!
        \brief Initialize & validate a MinFitThresholdList with a given name & a vector of threshold pairs.
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

        \param[in]  thresholdPairs - an list of elapsedSecond,minFitThreshold pairs defining the
                        minFitThresholdLists's step function (an unordered eastl vector of pair<uint32_t,float>).
                        Pairs are unordered, but you should define one for time 0.  The fitValues
                        range between 0 and 1.0 (inclusive), or should be set to 
                        MinFitThresholdList::EXACT_MATCH_REQUIRED.
        \return true on success, false on fatal errors.
    *************************************************************************************************/
    bool MinFitThresholdList::initialize(const char8_t* listName, const MinFitThresholdPairContainer &thresholdPairs)
    {
        bool initResult = (initName(listName) && initList(thresholdPairs));

        if (initResult)
        {
            //Cache off if we're an exact match required list
            mIsExactMatchRequired = isThresholdExactMatchRequired(getMinFitThresholdByIndex(0));

            //Cache off the time where we become a
            if (mIsExactMatchRequired)
            {
                if (mMinThresholdList.size() > 1)
                {
                    // we require an exact match until the session reaches this age
                    mRequiresExactMatchDecayAge = getSecondsByIndex(1);
                }
                else
                {
                    // session always requires an exact match
                    mRequiresExactMatchDecayAge = NO_MATCH_TIME_SECONDS; 
                }
            }
            //otherwise the default of "0" stands
        }

        return initResult;
    }

    /*! ************************************************************************************************/
    /*!
        \brief init the list's name.  Returns false if listName is nullptr or empty.

            Note: name will be truncated to 15 characters (to fit inside a MinFitThresholdName buffer)

        \param[in]  listName - the list's name
        \return true on success, false on fatal error
    *************************************************************************************************/
    bool MinFitThresholdList::initName(const char8_t* listName)
    {
        // non-nullptr, non-empty
        if ( (listName == nullptr) || (listName[0] == '\0') )
        {
            ERR_LOG(LOG_PREFIX << "Error: trying to construct MinFitThresholdList without a valid name.");
            EA_FAIL_MSG("listName is nullptr or empty");
            return false;
        }

        mName.set(listName);
        
        return true;
    }

    /*! ************************************************************************************************/
    /*!
        \brief init the list's minFitThresholdPairs.  Returns false if thresholdPairs is empty.

            Note: the list logs a warning and discards duplicated pairs for a given time.  Also logs
            a warning if no pair is defined for time 0.

        \param[in]  thresholdPairs - a vector containing the thresholdPairs to use for this list.
        \return true on success, false on fatal error
    *************************************************************************************************/
    bool MinFitThresholdList::initList(const MinFitThresholdPairContainer &thresholdPairs)
    {
        if (thresholdPairs.empty())
        {
            ERR_LOG(LOG_PREFIX << "minFitThresholdList \"" << mName.c_str() << "\" is empty; it must contain at least one minFitThresholdPair");
            return false;
        }

        size_t numPairs = thresholdPairs.size();
        mMinThresholdList.reserve(numPairs);
        for(size_t i=0; i<numPairs; ++i)
        {
            // each time value must be unique
            uint32_t elapsedSeconds =  thresholdPairs[i].first;
            if( !isMinFitThresholdDefined(elapsedSeconds) )
            {
                // unique time; add the pair
                mMinThresholdList.push_back(thresholdPairs[i]);
            }
            else
            {
                // duplicate time; skip this pair
                TRACE_LOG(LOG_PREFIX << "minFitThresholdList \"" << mName.c_str() << "\" contains multiple thresholds for time " << elapsedSeconds << "; ignoring redefinitions.");
            }
        }

        // now we sort the pairs by elapsed seconds
        eastl::sort(mMinThresholdList.begin(), mMinThresholdList.end());

        // check if a pair was defined at time 0.
        if ( !isMinFitThresholdDefined(0))
        {
            // set the time of the first pair to zero
            mMinThresholdList[0].first = 0;
            char8_t buf[32];
            blaze_snzprintf(buf, sizeof(buf), "%.2f", mMinThresholdList[0].second);
            WARN_LOG(LOG_PREFIX << "minFitThresholdList \"" << mName.c_str() << "\" has no minFitThreshold for time 0; list will start with 0:" << buf);
        }

        // enforce that the thresholds never become more restrictive over time.
        if (mMinThresholdList.size() > 1)
        {
            // validate that the thresholds decay over time - CreateGame MM assumes that rules relax/decay and will break if something constricts.
            MinFitThresholdPairContainer::const_iterator pairIter = mMinThresholdList.begin();
            float prevThreshold = pairIter->second;
            ++pairIter;
            MinFitThresholdPairContainer::const_iterator end = mMinThresholdList.end();
            for ( ; pairIter != end; ++pairIter )
            {
                uint32_t currentTime = pairIter->first;
                float currentThreshold = pairIter->second;

                if (currentThreshold < 0)
                {
                    // current threshold is EXACT_MATCH_REQUIRED (-1) - shouldn't be possible
                    // err, EXACT_MATCH_REQUIRED should only occur at time 0 and this is the 2nd position (or later).
                    ERR_LOG(LOG_PREFIX << "minFitThresholdList \"" << mName.c_str() << "\" is invalid (thresholds must decay/decrease over time), so EXACT_MATCH_REQUIRED should only exist at time 0 - but we found it at time " << currentTime << "!");
                    return false;
                }


                if (prevThreshold >= 0)
                {
                    // prev value is NOT EXACT_MATCH_REQUIRED - make sure we decay.
                    if (currentThreshold > prevThreshold)
                    {
                        // err, threshold has become more restrictive over time!
                        ERR_LOG(LOG_PREFIX << "minFitThresholdList \"" << mName.c_str() << "\" is invalid (thresholds must decay/decrease over time), but the minFitThreshold increases from " << prevThreshold << " to " << currentThreshold << " at time " << currentTime << "!");
                        return false;
                    }
                }
                else
                {
                    // no-op: prev value was exact match, so anything is a valid decay
                }

                prevThreshold = currentThreshold;
            }
        }


        return true;
    }



    /*! ************************************************************************************************/
    /*!
        \brief return the minFitThreshold in effect at time elapsedSeconds.

        Note: since the minFitThresholdList defines a step function, you'll get thresholds back for times
        that aren't explicitly setup.

        For example:
            quickMatch = [ 0:1.00, 5:0.80, 10:0.50, 15:0.20 ]
            getMinFitThreshold(0) == 1.0
            getMinFitThreshold(4) == 1.0
            getMinFitThreshold(5) == 0.8
            getMinFitThreshold(30) == 0.2

        \param[in]  elapsedSeconds  - the time (in elapsed seconds) to check
        \return the minimum fit threshold in effect at time elapsedSeconds.
    *************************************************************************************************/
    float MinFitThresholdList::getMinFitThreshold(uint32_t elapsedSeconds, uint32_t *nextThresholdSeconds) const
    {
        if (mMinThresholdList.empty())
        {
            EA_FAIL_MSG("MinFitThreshold list should not be empty");
            if (nextThresholdSeconds != nullptr)
            {
                *nextThresholdSeconds = UINT32_MAX;
            }
            return 0.0f;
        }

        // note: threshold list is sorted by elapsed time
        MinFitThresholdPairContainer::const_iterator pairIter = mMinThresholdList.begin();
        MinFitThresholdPairContainer::const_iterator end = mMinThresholdList.end();
        float minFitThreshold = pairIter->second;
        for( ; pairIter != end; ++pairIter )
        {
            if (pairIter->first <= elapsedSeconds)
            {
                minFitThreshold = pairIter->second;
            }
            else
            {
                if (nextThresholdSeconds != nullptr)
                {
                    *nextThresholdSeconds = pairIter->first;
                }                
                return minFitThreshold;
            }
        }

        // not found, use 'last' value
        if (nextThresholdSeconds != nullptr)
        {
            //There is no higher level, so there will never be another threshold to go to.
            *nextThresholdSeconds = UINT32_MAX;
        }
        return minFitThreshold;
    }


    /*! ************************************************************************************************/
    /*!
        \brief return true if this list contains a thresholdPair at elapsedSeconds.
    
        \param[in]  elapsedSeconds - the time to examine for a defined thresholdPair
        \return true if a thresholdPair is defined at time elapsedSeconds; false otherwise
    *************************************************************************************************/
    bool MinFitThresholdList::isMinFitThresholdDefined(size_t elapsedSeconds) const
    {
        // note: called before list is sorted, so we can't rely on order to early-out

        for(size_t i=0; i<mMinThresholdList.size(); ++i)
        {
            if (elapsedSeconds == mMinThresholdList[i].first)
            {
                return true;
            }
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*!
        \brief Write the minFitThresholdList into an eastl string for logging.  NOTE: inefficient.

        The minFitThresholdList is written into the string using the config file syntax:
            "<listName> = [ <time1>:<fit1>, <time2>:<fit2>, ... ]"
        Ex:
            "quickMatch = [ 0:1.00, 5:0.80, 10:0.50, 15:0.20 ]"

        Note: the fit values are printed with "%0.2f", but are stored with more precisions internally (as regular floats).

        \param[in/out]  str - the minFitThresholdList is written into this string, blowing away any previous value.
    *************************************************************************************************/
    void MinFitThresholdList::toString(eastl::string &str) const
    {
        str.sprintf("%s = [ ", mName.c_str());
        size_t lastIndex = mMinThresholdList.size() - 1;
        for(size_t i=0; i<=lastIndex; ++i)
        {
            uint32_t elaspedSeconds = mMinThresholdList[i].first;
            float fitValue = mMinThresholdList[i].second;

            char8_t pairStr[48];
            if (isThresholdExactMatchRequired(fitValue))
                blaze_snzprintf(pairStr, sizeof(pairStr), "%u:%s", elaspedSeconds, MatchmakingConfig::EXACT_MATCH_REQUIRED_TOKEN);
            else
                blaze_snzprintf(pairStr, sizeof(pairStr), "%u:%0.2f", elaspedSeconds, fitValue);

            if (i != lastIndex)
                str.append_sprintf("%s, ", pairStr);
            else
                str.append_sprintf("%s ]", pairStr);
        }
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
