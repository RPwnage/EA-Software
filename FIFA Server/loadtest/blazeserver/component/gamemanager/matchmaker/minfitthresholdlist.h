/*! ************************************************************************************************/
/*!
    \file   minfitthresholdlist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST
#define BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST

#include "gamemanager/tdf/matchmaker.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \class MinFitThresholdList
        \brief defines a named step-function of 'minimum fit thresholds' vs time.  
            A matchmaking rule 'matches'if the fitValue is >= minFitThreshold[elapsedMatchmakingSeconds].

        The step function is represented by a collection of 'elapsedTime',minFitThreshold pairs.  Each
        thresholdList should define at least one pair at time 0.

        Call getMinFitThreshold to get the step function value (minFitThreshold) for a given time.

        MinFitThresholdLists are typically defined in the matchmaking config file, and are initialized
        automatically by the component at startup.  You shouldn't need to call a constructor explicitly
    *************************************************************************************************/
    class MinFitThresholdList
    {
        NON_COPYABLE(MinFitThresholdList);
    public:
        static const char8_t* MIN_FIT_THRESHOLD_LISTS_CONFIG_KEY;

        static const float EXACT_MATCH_REQUIRED; //!< indicates that an exact match is required at this time

        /*! ************************************************************************************************/
        /*!
            \brief Construct an uninitilized MinFitThresholdList.  NOTE: do not use until successfully
                initialized.

            MinFitThresholdLists are typically defined in the matchmaking config file and initialized
            automatically by the component at startup.

        *************************************************************************************************/
        MinFitThresholdList();
        ~MinFitThresholdList() {};

        typedef eastl::pair<uint32_t, float> MinFitThresholdPair; //!< elapsedSeconds, minFitThreshold pairs
        typedef eastl::vector< MinFitThresholdPair > MinFitThresholdPairContainer;

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
        bool initialize(const char8_t* listName, const MinFitThresholdPairContainer &thresholdPairs);

        /*! ************************************************************************************************/
        /*! \brief Returns whether or not this list has an EXACT MATCH requirement.
        *************************************************************************************************/
        bool isExactMatchRequired() const { return mIsExactMatchRequired; }

        /*! ************************************************************************************************/
        /*! \brief Returns the time period where exact match is no longer required for this threshold list.
        *************************************************************************************************/
        uint32_t getRequiresExactMatchDecayAge() const { return mRequiresExactMatchDecayAge; }


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
            \param[out] nextThresholdSeconds (optional) The next event after this one.  (MAX UINT32_T if the returned fit is the last event).
            \return the minimum fit threshold in effect at time elapsedSeconds.
        *************************************************************************************************/
        float getMinFitThreshold(uint32_t elapsedSeconds, uint32_t *nextThresholdSeconds = nullptr) const;


        /*! ************************************************************************************************/
        /*! \brief return the last minFitThreshold value in the list.
        ***************************************************************************************************/
        float getLastMinFitThreshold() const { return !mMinThresholdList.empty() ? mMinThresholdList.back().second : 0.0f; }

        /*! ************************************************************************************************/
        /*! \brief return the size of this minFitThreshold list (number of elements)
        ***************************************************************************************************/
        uint32_t getSize() const { return (uint32_t) mMinThresholdList.size(); }

        /*! ************************************************************************************************/
        /*! \brief return the minFitThreshold value at a particular index in the list.
        ***************************************************************************************************/
        float getMinFitThresholdByIndex( uint32_t index ) const { return mMinThresholdList[index].second; }

        /*! ************************************************************************************************/
        /*! \brief returns the seconds value at a particular index in the list
        ***************************************************************************************************/
        uint32_t getSecondsByIndex( uint32_t index ) const { return mMinThresholdList[index].first; }

        /*! ************************************************************************************************/
        /*!
            \brief return true if a fitThreshold indicates that an exact match is required
            \return  true if a fitThreshold indicates that an exact match is required
        *************************************************************************************************/
        static bool isThresholdExactMatchRequired(float threshold) { return threshold < 0; }

        /*! ************************************************************************************************/
        /*!
            \brief return the name of this minFitThresholdList
            \return the list's name
        *************************************************************************************************/
        const char8_t* getName() const { return mName.c_str(); }
    
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
        void toString(eastl::string &str) const;

    private:

        /*! ************************************************************************************************/
        /*!
            \brief init the list's name.  Returns false if listName is nullptr or empty.

                Note: name will be truncated to 15 characters (to fit inside a MinFitThresholdName buffer)

            \param[in]  listName - the list's name
            \return true on success, false on fatal error
        *************************************************************************************************/
        bool initName(const char8_t* listName);

        /*! ************************************************************************************************/
        /*!
            \brief init the list's minFitThresholdPairs.  Returns false if thresholdPairs is empty.

                Note: the list logs a warning and discards duplicated pairs for a given time.  Also logs
                a warning if no pair is defined for time 0.

            \param[in]  thresholdPairs - a vector containing the thresholdPairs to use for this list.
            \return true on success, false on fatal error
        *************************************************************************************************/
        bool initList(const MinFitThresholdPairContainer &thresholdPairs);

        /*! ************************************************************************************************/
        /*!
            \brief return true if this list contains a thresholdPair at elapsedSeconds.
        
            \param[in]  elapsedSeconds - the time to examine for a defined thresholdPair
            \return true if a thresholdPair is defined at time elapsedSeconds; false otherwise
        *************************************************************************************************/
        bool isMinFitThresholdDefined(size_t elapsedSeconds) const;

    private:
        MinFitThresholdName mName;
        MinFitThresholdPairContainer mMinThresholdList;
        bool mIsExactMatchRequired;
        uint32_t mRequiresExactMatchDecayAge;            

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST
