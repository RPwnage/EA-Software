/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AVERAGE_H
#define BLAZE_AVERAGE_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "framework/callback.h"
#include "framework/connection/selector.h"
#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

template <typename AverageType>
class MovingAverageBase;

template <typename AverageType>
class MovingAverage;

template <typename AverageType>
class MovingTimeAverage;

/*!*********************************************************************************/
/*! \class MovingAverageBase
    \brief Simple base class to keep track of an unweighted moving average.
            Does not handle under/overflows. Is not thread-safe.
            Provides a unified interface to update/query averages.
            Supports number types as AverageType.
************************************************************************************/
template <typename AverageType>
class MovingAverageBase
{
public:
    MovingAverageBase() : mCurrentSize(0), mTotal(0) {}
    virtual ~MovingAverageBase() {}

public:
    uint64_t getSize() const { return mCurrentSize; }
    const char8_t* getName() const { return mName; }

    virtual AverageType getCurrentAverage() const = 0;
    virtual void add(AverageType value) = 0;
    virtual void clear() = 0;

protected:
    char8_t mName[256];
    uint64_t mCurrentSize;
    AverageType mTotal;
};


/*!*********************************************************************************/
/*! \class MovingAverage
    \brief Keeps track of an average over a maximum of "x" iterations.
            e.g. percentage of failures in the last 1000 tries.
************************************************************************************/
template <typename AverageType>
class MovingAverage : public MovingAverageBase<AverageType>
{
    typedef MovingAverageBase<AverageType> base_type;
    typedef MovingAverage<AverageType>  this_type;

public:
    using base_type::mCurrentSize;
    using base_type::mTotal;

public:
    MovingAverage(uint64_t maxSize = 1000);
    ~MovingAverage() override;

public:
    uint64_t getMaxSize() const { return mMaxSize; }
    void changeMaxSize(uint64_t maxSize);

    // MovingAverageBase interface
    AverageType getCurrentAverage() const override;
    void add(AverageType value) override;
    void clear() override;

private:
    uint64_t removeFromBack(uint64_t count);

private:
    typedef eastl::list<AverageType> ValuesList;

    uint64_t mMaxSize;
    ValuesList mValues;
};


template <typename AverageType>
MovingAverage<AverageType>::MovingAverage(uint64_t maxSize)
    : mMaxSize(maxSize)
{

}

template <typename AverageType>
MovingAverage<AverageType>::~MovingAverage()
{
    // Delete all the values inside the list.
    clear();
}


template <typename AverageType>
void MovingAverage<AverageType>::changeMaxSize(uint64_t maxSize)
{
    if (maxSize > mMaxSize)
    {
        uint64_t diff = mMaxSize - maxSize;
        removeFromBack(diff);
    }
    mMaxSize = maxSize;

} // changeMaxSize


template <typename AverageType>
AverageType MovingAverage<AverageType>::getCurrentAverage() const
{
    if (mCurrentSize == 0) return 0;

    return mTotal / (AverageType) mCurrentSize;
} // getCurrentAverage


template <typename AverageType>
void MovingAverage<AverageType>::add(AverageType value)
{
    mTotal += value;
    mCurrentSize++;

    // If larger than maxSize, we have to trim.
    if (mCurrentSize > mMaxSize)
    {
        uint64_t diff = mCurrentSize - mMaxSize;
        removeFromBack(diff);
    } // if

    mValues.push_front(value);
} // add


template <typename AverageType>
void MovingAverage<AverageType>::clear()
{
    mTotal = 0;
    mCurrentSize = 0;
    mValues.clear();
} // clear


template <typename AverageType>
uint64_t MovingAverage<AverageType>::removeFromBack(uint64_t count)
{
    uint64_t results = 0;
    for (; results < count; results++)
    {
        if (mValues.empty())
        {
            --results;
            break;
        }

        mCurrentSize--;
        AverageType value = mValues.back();
        mTotal -= value;
        mValues.pop_back();

    } // for

    return results;
} // removeFromBack



/*!*********************************************************************************/
/*! \class MovingTimeAverage
    \brief Keeps track of an average over a maximum of "t" time.
            e.g. percentage of failures in the last 5 minutes.

            TODO: Update this class to use the "bucket"/history model
            that the rest of component metrics is using.
************************************************************************************/
template <typename AverageType>
class MovingTimeAverage : public MovingAverageBase<AverageType>
{
    typedef MovingAverageBase<AverageType> base_type;
    typedef MovingTimeAverage<AverageType>  this_type;

public:
    using base_type::mCurrentSize;
    using base_type::mTotal;

public:
    // This callback is called when the average is not empty anymore.
    typedef Functor1<const MovingTimeAverage<AverageType>&> NotEmptyCb;

public:
    MovingTimeAverage(const NotEmptyCb& cb = NotEmptyCb(),
        EA::TDF::TimeValue windowTime = 5 * 60 * 1000 * 1000); // min x sec x milli x micro.

    ~MovingTimeAverage() override;

public:
    void updateCb(const NotEmptyCb& cb) { mNotEmptyCb = cb; }

    EA::TDF::TimeValue getWindowTime() const { return mWindowTime; }
    void changeWindowTime(EA::TDF::TimeValue windowTime) { mWindowTime = windowTime; }

    // It is up to the owner of this average to keep it updated regularly.
    // The owner decides on how often to do this.  Reports number of items remaining.
    uint64_t updateAverages();

    // MovingAverageBase interface
    AverageType getCurrentAverage() const override;
    void add(AverageType value) override;
    void clear() override;

private:
    uint64_t pruneFromBack(EA::TDF::TimeValue expireRelativeTime);

private:
    struct node
    {
        node(AverageType val = 0, EA::TDF::TimeValue tim = 0) : value(val), time(tim) {}

        AverageType value;
        EA::TDF::TimeValue time;
    };
    typedef eastl::list<node> ValuesList;

    NotEmptyCb mNotEmptyCb;
    EA::TDF::TimeValue mWindowTime;  // How long to keep entry for.
    ValuesList mValues;
};


template <typename AverageType>
MovingTimeAverage<AverageType>::MovingTimeAverage( const NotEmptyCb& cb, EA::TDF::TimeValue windowTime)
    : mNotEmptyCb(cb),
    mWindowTime(windowTime)
{

}


template <typename AverageType>
MovingTimeAverage<AverageType>::~MovingTimeAverage()
{
    // Delete all the values inside the list.
    clear();
}


template <typename AverageType>
AverageType MovingTimeAverage<AverageType>::getCurrentAverage() const
{
    // Note: Because this is time-based, you must call updateAverages
    // regularly before calling this for it to be accurate.
    if (mCurrentSize == 0) return 0;

    return mTotal / (AverageType) mCurrentSize;
} // getCurrentAverage


template <typename AverageType>
void MovingTimeAverage<AverageType>::add(AverageType value)
{
    bool bNeedsCb = (mCurrentSize == 0);

    // Prune expired entries.
    pruneFromBack(mWindowTime);

    mTotal += value;
    mCurrentSize++;

    mValues.push_front(node(value, EA::TDF::TimeValue::getTimeOfDay()));

    if (bNeedsCb && mNotEmptyCb.isValid())
    {
        mNotEmptyCb(*this);
    }
} // add


template <typename AverageType>
void MovingTimeAverage<AverageType>::clear()
{
    mTotal = 0;
    mCurrentSize = 0;
    mValues.clear();
} // clear


template <typename AverageType>
inline uint64_t MovingTimeAverage<AverageType>::updateAverages()
{
    // Prune expired entries.
    pruneFromBack(mWindowTime);

    // Return current size (so that callers know if they need to update).
    return mCurrentSize;
} // updateAverages


template <typename AverageType>
uint64_t MovingTimeAverage<AverageType>::pruneFromBack(EA::TDF::TimeValue expireRelativeTime)
{
    bool done = false;
    uint64_t results = 0;

    EA::TDF::TimeValue expireTime = EA::TDF::TimeValue::getTimeOfDay() - expireRelativeTime;

    while (!done || (!mValues.empty()))
    {
        node& current = mValues.back();
        if (current.time < expireTime)
        {
            AverageType value = current.value;
            mTotal -= value;
            mValues.pop_back();
            mCurrentSize--;
            results++;
        }
        else
        {
            done = true;
        }
    } // while

    return results;
} // pruneFromBack


} // Blaze
#endif // BLAZE_AVERAGE_H
