/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class IntervalMap

    This class is responsible for assigning unique integer identifiers to the caller and ensuring
    that no identifier is reused before it has been released.  Simple implementations of id
    allocators use an incrementing counter to assign identifiers and wrap around to 0 once the
    maximum value is hit.  The problem with this model is that it becomes possible for the same
    identifier to be allocated twice.  This is especially true if the range of values is small or
    if the identifiers remain allocated for a long period of time.

    This implementation maintains a list of interval ranges of allocated identifiers to ensure
    that no identifier can be allocated twice.  It also allows certain ranges of values to be
    reserved such that the caller can be guaranteed that the class will not return an identifier
    in that range.  This can be useful when the entire int32 range is not needed or certain values
    have special meaning.

    When identifiers are released back into the pool, intervals are automatically coallesced to
    reduce memory usage.

    \notes
        This class must only be used when the usage pattern of the assigned identifiers is such
        that identifiers are released back into the pool in the approximate order that they were
        allocated.  If this is not the case, the memory needed to track the intervals will become
        unacceptable.  A usage model where identifiers are allocated but then released randomly
        back into the pool will degenerate very quickly.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/intervalmap.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/


/*************************************************************************************************/
/*!
     \brief IntervalMap
 
      Create a new interval map that will assign values only between the given range.  The start
      and end values are inclusive.  For example, if IntervalMap(5,8) is call then the only valid
      values will be 5, 6, 7, and 8.
  
     \param[in] start - Start of the valid range.
     \param[in] end - End of the valid range.
*/
/*************************************************************************************************/
IntervalMap::IntervalMap(int32_t start, int32_t end)
    : mHead(nullptr),
      mNext(nullptr),
      mStart(start),
      mEnd(end)
{
}

/*************************************************************************************************/
/*!
     \brief ~IntervalMap
 
      Destroy the given interval map.
*/
/*************************************************************************************************/
IntervalMap::~IntervalMap()
{
    Interval* curr = mHead;
    while (curr != nullptr)
    {
        Interval* tmp = curr;
        curr = curr->next;
        delete tmp;
    }
}

void IntervalMap::init()
{
    mHead = mNext = BLAZE_NEW Interval;
    mHead->start = mStart;
    mHead->size = 0;
    mHead->next = nullptr;
}

/*************************************************************************************************/
/*!
     \brief allocate
 
      Allocate a new value from the map.
  
     \return - -1 if the map is full; >=0 on success.

*/
/*************************************************************************************************/
int32_t IntervalMap::allocate()
{
    if (mHead == nullptr)
        init();

    // IF we've reached the limit
    if ((mNext->start + mNext->size) == (mEnd + 1))
    {
        // Wrap back to the start and locate the next available id
        mNext = mHead;

        // IF there is a gap at the head of the list
        if (mHead->start > mStart)
        {
            // IF there is only one available id at the head
            if (mHead->start == mStart + 1)
            {
                // UnitTest Allocate1 should hit this code path

                // THEN consume it instead of making a new interval
                mHead->start = mStart;
                ++mHead->size;
            }
            else
            {
                // UnitTest Allocate2 should hit this code path

                // Create a new interval at the head of the list
                Interval* ival = BLAZE_NEW Interval;
                ival->start = mStart;
                ival->size = 1;
                ival->next = mHead;
                mHead = mNext = ival;

            }
            return mStart;
        }

        if ((mNext->start + mNext->size) == (mEnd + 1))
        {
            // UnitTest Allocate3 should hit this code path
            // Uh-oh.. the entire address space has been used!
            return -1;
        }

        // UnitTest Allocate4 should hit this code path
    }

    int32_t val = (mNext->start + mNext->size);
    ++mNext->size;

    // IF this interval is now full (eg. bumping right up against the next one)
    if ((mNext->next != nullptr) && (val == (mNext->next->start - 1)))
    {
        // UnitTest Allocate5 should hit this code path

        // Coallesce interval
        mNext->size += mNext->next->size;
        Interval* del = mNext->next;
        mNext->next = mNext->next->next;
        delete del;
    }
    return val;
}

/*************************************************************************************************/
/*!
     \brief release
 
      Release a value back into the map.  This must only be called with a value that was acquired
      via the allocate() call.  The given value will be made available again to be returned in
      a subsequent allocate() call.  Assume a usage pattern described in the class definition,
      the value will not be returned from an allocate() call for a very long time.
  
     \param[in] id - Value to release.
 
     \return - true on success; false if the value wasn't currently allocated.

*/
/*************************************************************************************************/
bool IntervalMap::release(int32_t id)
{
    if ((id < mStart) || (id > mEnd))
    {
        // UnitTest Release1 should hit this code path
        return false;
    }

    Interval* curr = mHead;
    Interval* prev = nullptr;
    while (curr != nullptr)
    {
        // Check if the id is in this interval
        if (id < curr->start)
        {
            // UnitTest Release2 should hit this code path
            return false;
        }
        if (id >= (curr->start + curr->size))
        {
            prev = curr;
            curr = curr->next;
            continue;
        }

        if (id == curr->start)
        {
            // UnitTest Release3 should hit this code path
            ++curr->start;
            curr->size--;

            if ((curr != mNext) && (curr->size == 0))
            {
                // UnitTest DaveMScenario1

                // Remove the empty interval if it isn't the one we're currently assigning from
                if (prev != nullptr)
                    prev->next = curr->next;
                else
                    mHead = curr->next;
                delete curr;
            }
            return true;
        }
        else if (id == (curr->start + curr->size - 1))
        {
            curr->size--;
            if (curr == mNext)
            {
                // This node is where we're currently assigning new IDs from and we just released
                // the last assigned ID.  Make a new empty interval and set that as mNext otherwise
                // the next allocate() will re-assign this released ID

                // However, if this release means that there is only one free entry still available
                // then don't create a new node since we need to re-assign this entry on the
                // next allocate call.
                if ((mHead->next == nullptr)
                        && (mHead->start == mStart) && (mHead->size == mEnd - mStart))
                {
                    // UnitTest Release4 should hit this code path
                    return true;
                }

                if ((curr->start + curr->size) >= mEnd)
                {
                    // UnitTest DaveMScenario2

                    // We're at the end of the available range so point next back to the head
                    // of the interval map
                    mNext = mHead;
                    return true;
                }

                // UnitTest Release5 should hit this code path
                Interval* ival = BLAZE_NEW Interval;
                ival->next = curr->next;
                ival->start = curr->start + curr->size + 1;
                ival->size = 0;
                curr->next = ival;
                mNext = ival;
            }
            return true;
        }

        // UnitTest Release6 should hit this code path

        // The id being released is in the middle of the interval so split it into two
        Interval* add = BLAZE_NEW Interval;
        add->start = curr->start;
        add->size = id - add->start;
        add->next = curr;
        curr->start = id + 1;
        curr->size -= (id - add->start + 1);
        if (prev != nullptr)
            prev->next = add;
        else
            mHead = add;
        return true;
    }
    return false;
}

/*************************************************************************************************/
/*!
     \brief print
 
      Format debug info about the current state of the map into the provided buffer.
  
     \param[in] buf - Buffer to populate.
     \param[in] len - Length of buffer.
 
     \return - Pointer to buf.

*/
/*************************************************************************************************/
char8_t* IntervalMap::print(char8_t* buf, uint32_t len)
{
    if ((buf == nullptr) || (len == 0))
        return buf;

    if (mHead == nullptr)
    {
        *buf = '\0';
        return buf;
    }

    uint32_t used = 0;
    for (Interval* curr = mHead; curr != nullptr; curr = curr->next)
    {
        char8_t brace = (curr == mNext) ? '{' : '[';
        if (curr->size == 0)
        {
            used += blaze_snzprintf(buf + used, len - used, "%c%d%c",
                    brace, curr->start, brace + 2);
        }
        else
        {
            used += blaze_snzprintf(buf + used, len - used, "%c%d-%d%c",
                    brace, curr->start, curr->start + curr->size - 1, brace + 2);
        }
    }
    return buf;
}

void IntervalMap::exportIntervals(IntervalMapData& mapData)
{
    if (mHead == nullptr)
        return;

    uint32_t index = 0;
    for (Interval* curr = mHead; curr != nullptr; curr = curr->next, ++index)
    {
        IntervalData& intervalData = mapData.mDataList.push_back();
        intervalData.start = curr->start;
        intervalData.size = curr->size;

        if (curr == mNext)
            mapData.mIndexOfNext = index;
    }
}

void IntervalMap::importIntervals(const IntervalMapData& mapData)
{
    Interval* prev = nullptr;
    for (uint32_t index = 0; index < mapData.mDataList.size(); ++index)
    {
        const IntervalData& data = mapData.mDataList[index];

        EA_ASSERT(data.start >= mStart && (data.start + data.size) <= (mEnd + 1));

        Interval* curr = BLAZE_NEW Interval;
        curr->start = data.start;
        curr->size = data.size;
        curr->next = prev;

        if (index == 0)
            mHead = curr;

        if (index == mapData.mIndexOfNext)
            mNext = curr;

        prev = curr;
    }
}

} // Blaze
