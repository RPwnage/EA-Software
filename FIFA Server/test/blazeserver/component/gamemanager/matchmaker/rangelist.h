/*! ************************************************************************************************/
/*!
    \file rangelist.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_RANGE_LIST
#define BLAZE_MATCHMAKING_RANGE_LIST

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
    /*! \brief defines a named step-function of 'range endpoints' vs time (MM session age).  
            A matchmaking rule 'matches' when a value falls between the range (at a given session age)

        This is very similar to the MinFitThresholdList, except instead of a single fitScore as the threshold,
            we have a range of values (the range includes the endpoints).
    *************************************************************************************************/
    class RangeList
    {
        NON_COPYABLE(RangeList);
    public:
        static const int64_t INFINITY_RANGE_VALUE;
        static const int64_t EXACT_MATCH_REQUIRED_VALUE;

        struct Range
        {
            int64_t mMinValue;
            int64_t mMaxValue;

            bool isMatchAny() const { return mMinValue == INFINITY_RANGE_VALUE && mMaxValue == INFINITY_RANGE_VALUE; }

            bool isInRange(int64_t value) const { return ((value >= mMinValue) && (value <= mMaxValue)); }

            bool isExactMatchRequired() const { return (mMinValue == EXACT_MATCH_REQUIRED_VALUE) && (mMaxValue == EXACT_MATCH_REQUIRED_VALUE); }


            /*! **********************************************************************/
            /*!
                \brief sets the range values given the desired value, range offset
                    (ie. -10..2 -> -10, +2) and the min and max clamps.

                    example: value 10, range -5..4, min 0, max 12 -> minValue 5, maxValue 12
            **************************************************************************/
            void setRange(int64_t center, const Range &rangeOffset, int64_t minClamp = INT64_MIN, int64_t maxClamp = INT64_MAX);
        };

        typedef eastl::pair<uint32_t, Range> RangePair; // sessionAgeSeconds, Range
        typedef eastl::vector< RangePair > RangePairContainer;

        RangeList() {};
        ~RangeList() {};
        bool initialize(const char8_t *listName, const RangePairContainer &rangePairContainer);

        const char8_t *getName() const { return mName.c_str(); }
        const Range *getRange(uint32_t elapsedSeconds, uint32_t *nextThresholdSeconds) const;
        const Range *getLastRange() const { return !mRangePairContainer.empty() ? &(mRangePairContainer.back().second) : nullptr; }
        void toString(eastl::string &str) const;

        const RangePairContainer& getContainer() const { return mRangePairContainer; }

    private:

        bool initName(const char8_t* listName);
        bool initRangeList(const RangePairContainer &rangePairContainer);
        bool isRangeDefined(size_t elapsedSeconds) const;

        struct RangePairSortComparitor
        {
            // since we don't allow multiple ranges at a specific time, we just sort on the time
            bool operator()(const RangePair &a, const RangePair &b) const
            {
                return (a.first < b.first);
            }
        };

    private:
        MinFitThresholdName mName;
        RangePairContainer mRangePairContainer;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_RANGE_LIST
