/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CORE_MANAGER_H
#define BLAZE_CORE_MANAGER_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_mutex.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


class BlazeAllocatorInterface;

/*! ************************************************************************************************/
/*! \class CoreManager

    A core manager manages a contiguous array of virtual memory.  It keeps an array of buckets
    and marks them in use as allocators request them.  Each bucket can be marked as "in use"
    or "bad".  Bad buckets result from allocations on top of the space we've laid out for ourselves
    and remain unused if we can't commit the memory.
***************************************************************************************************/
class CoreManager
{
    NON_COPYABLE(CoreManager);

public:
    CoreManager(Blaze::Allocator &defaultAllocator, void* startAddress, uint64_t totalSize, size_t bucketSize);
    ~CoreManager();

    size_t getBucketSize() const { return mBucketSize; }

    bool assignCore(BlazeAllocatorInterface& allocator, size_t minCoreSize, char8_t*& resultMem, size_t& resultSize);
   
    BlazeAllocatorInterface* getAllocator(void* addr) const;

private:
    
    bool addBucket();

private:
    struct CoreEntry
    {
        CoreEntry() : mCore(nullptr), mAllocator(nullptr), mBadBlock(false) {}

        bool isFree() const { return mAllocator == nullptr && !mBadBlock; }

        char8_t* mCore;
        BlazeAllocatorInterface* mAllocator;

        //If true this means the block we previously tried this block but failed to obtain it
        bool mBadBlock;  
    };
    typedef eastl::vector<CoreEntry, Blaze::BlazeStlAllocator> CoreEntryVector;

    Blaze::Allocator& mDefaultAllocator;
    EA::Thread::Mutex mLock;
    uint64_t mTotalSize;
    size_t mMaxBucketCount;
    size_t mBucketSize;
    size_t mBucketShift;
    size_t mUsedBucketCount;


    CoreEntryVector mCoreEntryVector;

    char8_t* mAllCoreMem;    
};


/*! ************************************************************************************************/
/*! \class OSFunctions

    A set of functions abstracting common OS layer memory functions
***************************************************************************************************/
class OSFunctions
{
public:

    static void* reserveCore(void* address, uint64_t size);    
    static void freeCore(void *address, uint64_t size);
    static void* commitCore(void *address, size_t size);

    static uint32_t getLastError();

    static void *mallocHook(size_t size, const void *);
    static void *reallocHook(void *ptr, size_t size, const void *caller);
    static void *memalignHook(size_t alignment, size_t size, const void *caller);
    static void freeHook(void *ptr, const void *);

    static void initMemoryHooks();
    static void clearMemoryHooks();
    
    static uint32_t getProcessId();
        
    static void exitAndDie(const char8_t* buf);

private:
#ifdef EA_PLATFORM_LINUX
    static void *(*msOldMallocHook)(size_t size, const void *caller);
    static void *(*msOldReallocHook)(void *ptr, size_t size, const void *caller);
    static void *(*msOldMemalignHook)(size_t alignment, size_t size, const void *caller);
    static void (*msOldFreeHook)(void *ptr, const void *caller);
    static bool msMemHooksEnabled;
#endif
};


#endif // BLAZE_CORE_MANAGER_H

