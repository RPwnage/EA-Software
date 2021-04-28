/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/allocator/allocatormanager.h"
#include "framework/system/allocator/memtrackerinfo.h"

AllocInfoCommon::AllocInfoCommon()
{
    mAllocCount = mAllocSize = mFreeCount = mFreeSize = 0;
}

void AllocInfoCommon::getAllocSizeAndCount(int64_t& size, int64_t& count, bool total) const
{
    if (total)
    {
        size = mAllocSize;
        count = mAllocCount;
    }
    else
    {
        size = mAllocSize - mFreeSize;
        count = mAllocCount - mFreeCount;
    }
}

GAllocInfo::GAllocInfo() : mStackTrackingEnabled(false), mContext(nullptr), mStackInfoMap(Blaze::BlazeStlAllocator("GAllocInfo::mStackInfoMap", &AllocatorManager::getInstance().getInternalAllocator()))
{

}

void GAllocInfo::trackAlloc(int64_t size, int64_t count)
{
    mAllocCount += count;
    mAllocSize += size;
}

void GAllocInfo::trackFree(int64_t size, int64_t count)
{
    mFreeCount += count;
    mFreeSize += size;
}

GAllocInfo::GStackAllocInfo::GStackAllocInfo() : mGAllocParent(nullptr), mCallStack(nullptr)
{

}

void GAllocInfo::GStackAllocInfo::trackAlloc(int64_t size, int64_t count)
{
    mAllocCount += count;
    mAllocSize += size;
    mGAllocParent->trackAlloc(size, count);
}

void GAllocInfo::GStackAllocInfo::trackFree(int64_t size, int64_t count)
{
    mFreeCount += count;
    mFreeSize += size;
    mGAllocParent->trackFree(size, count);
}

const char8_t* GAllocInfo::GStackAllocInfo::getContext() const
{
    return mGAllocParent->getContext();
}

const CallStack* GAllocInfo::GStackAllocInfo::getCallstack() const
{
    return mCallStack;
}

namespace Blaze
{
    extern EA_THREAD_LOCAL PerThreadMemTracker* gPerThreadMemTracker;
}

TAllocInfoCommon::TAllocInfoCommon()
    : mTracker(Blaze::gPerThreadMemTracker)
{
    mAllocCount = mAllocSize = mFreeCount = mFreeSize = 0;
}

TAllocInfo::TAllocInfo() : mGAllocParent(nullptr), mStackInfoMap(Blaze::BlazeStlAllocator("TAllocInfo::mStackInfoMap", &AllocatorManager::getInstance().getInternalAllocator()))
{
}

void TAllocInfo::trackAlloc(int64_t size, int64_t count)
{
    mAllocCount += count;
    mAllocSize += size;
    if (mAllocSize < TRACKER_UPDATE_SIZE && mAllocCount < TRACKER_UPDATE_COUNT)
        return;
    mGAllocParent->trackAlloc(mAllocSize, mAllocCount);
    mAllocCount = mAllocSize = 0;
}

void TAllocInfo::trackFree(int64_t size, int64_t count)
{
    if (mTracker == Blaze::gPerThreadMemTracker)
    {
        mFreeCount += count;
        mFreeSize += size;
        if (mFreeSize < TRACKER_UPDATE_SIZE && mAllocCount < TRACKER_UPDATE_COUNT)
            return;
        mGAllocParent->trackFree(mFreeSize, mFreeCount);
        mFreeCount = mFreeSize = 0;
    }
    else
    {
        // trying to free from a different thread, bypass the thread cache
        mGAllocParent->trackFree(size, count);
    }
}

void TAllocInfo::flush()
{
    mGAllocParent->trackAlloc(mAllocSize, mAllocCount);
    mAllocCount = mAllocSize = 0;
    mGAllocParent->trackFree(mFreeSize, mFreeCount);
    mFreeCount = mFreeSize = 0;
    for (TStackAllocInfoMap::iterator i = mStackInfoMap.begin(), e = mStackInfoMap.end(); i != e; ++i)
    {
        TStackAllocInfo& allocInfo = i->second;
        allocInfo.flush();
    }
}

TAllocInfo::TStackAllocInfo::TStackAllocInfo() : mGStackAllocParent(nullptr)
{
}

void TAllocInfo::TStackAllocInfo::trackAlloc(int64_t size, int64_t count)
{
    mAllocCount += count;
    mAllocSize += size;
    if (mAllocSize < TRACKER_UPDATE_SIZE && mAllocCount < TRACKER_UPDATE_COUNT)
        return;
    mGStackAllocParent->trackAlloc(mAllocSize, mAllocCount);
    mAllocCount = mAllocSize = 0;
}

void TAllocInfo::TStackAllocInfo::trackFree(int64_t size, int64_t count)
{
    if (mTracker == Blaze::gPerThreadMemTracker)
    {
        mFreeCount += count;
        mFreeSize += size;
        if (mFreeSize < TRACKER_UPDATE_SIZE && mAllocCount < TRACKER_UPDATE_COUNT)
            return;
        mGStackAllocParent->trackFree(mFreeSize, mFreeCount);
        mFreeCount = mFreeSize = 0;
    }
    else
    {
        // trying to free from a different thread, bypass the thread cache
        mGStackAllocParent->trackFree(size, count);
    }
}

void TAllocInfo::TStackAllocInfo::flush()
{
    mGStackAllocParent->trackAlloc(mAllocSize, mAllocCount);
    mAllocCount = mAllocSize = 0;
    mGStackAllocParent->trackFree(mFreeSize, mFreeCount);
    mFreeCount = mFreeSize = 0;
}

