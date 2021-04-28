/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CALLSTACK_H
#define BLAZE_CALLSTACK_H

/*** Include files *******************************************************************************/
#include "framework/tdf/controllertypes_server.h" //for MemoryStatus definition

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

struct CallStack
{
    static const uint32_t MEM_TRACK_CALLSTACK_DEPTH = 25;
    static const uint32_t MAX_FRAMES_TO_IGNORE = 5;
    static const uint32_t DEFAULT_FRAMES_TO_IGNORE = 1;
    static const uint32_t EA_ASSERT_FRAMES_TO_IGNORE = 4;
    static const uint32_t MEM_TRACKER_FRAMES_TO_IGNORE = 2;

    CallStack()
        : mCallStackSize(0)
    {
    }

    CallStack(const CallStack& other)
        : mCallStackSize(other.mCallStackSize)
    {
        memcpy(mData, other.mData, sizeof(mData[0])*other.mCallStackSize);
    }

    void reset() { mCallStackSize = 0; }

    void getStack(uint32_t ignoreFrames = DEFAULT_FRAMES_TO_IGNORE, uint32_t keepFrames = MEM_TRACK_CALLSTACK_DEPTH);
    void getSymbols(Blaze::MemTrackerStatus::StackTraceList& list) const;
    void getSymbols(eastl::string& callstack) const;

    static void getCurrentStackSymbols(Blaze::MemTrackerStatus::StackTraceList& list);
    static void getCurrentStackSymbols(eastl::string& callstack, uint32_t ignoreFrames = DEFAULT_FRAMES_TO_IGNORE, uint32_t keepFrames = MEM_TRACK_CALLSTACK_DEPTH);

    void* mData[MEM_TRACK_CALLSTACK_DEPTH];
    size_t mCallStackSize;
};

struct CallStackCompare
{
    bool operator()(const CallStack& a, const CallStack& b) const
    {
        return less(a, b);
    }

    static bool less(const CallStack& a, const CallStack& b)
    {
        if (a.mCallStackSize == b.mCallStackSize)
            return memcmp(a.mData, b.mData, sizeof(a.mData[0])*a.mCallStackSize) < 0;
        return a.mCallStackSize < b.mCallStackSize;
    }
};

#endif // BLAZE_CALLSTACK_H

