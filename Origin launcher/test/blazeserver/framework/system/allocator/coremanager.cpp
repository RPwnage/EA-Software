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
#include "framework/system/allocator/coremanager.h"


#if defined(EA_PLATFORM_LINUX)
#include <sys/mman.h>
#elif defined(EA_PLATFORM_WINDOWS)
#include <process.h>
#endif


// Turns out that glibc deprecated the malloc hooks in 2012 with a note that they would be removed in next release (https://gitlab.com/bminor/glibc/commit/7d17596c198f11fa85cbcf9587443f262e63b616 ) but they 
// still exist after 6 years (checked the source code of glibc 2.27). The problem is that there isn't a good equivalent solution. Couple of approaches.

// 1. I tried using wrapper functions for malloc et al. While the application calls routed correctly to it, the glibc calls itself did not. So for example, if you call strdup in the application and later free it,
// the allocation does not happen via __wrapper_malloc but free happens via __wrapper_free. This, of course, crashes. Our allocator allows for checking whether the allocation was made via it and it could be used
// to call __real_free in this scenario (which would fix the crash) but this seems overly complicated.

// 2. The most reliable solution I found over the internet was to implement the malloc et al symbols in it's own shared lib(.so) and then use LD_PRELOAD trick to make sure that those symbols get resolved before
// glibc. But this is again more complicated than it ought to be. 

// So at the moment, I have decided to just disable the warnings. It is unlikely that these hooks are going away anytime soon as there are many postings over the internet that are filled with the problem I described
// above. If the hooks go away and we still want to redirect the malloc et al calls, the #2 from above is the way to go.

#if defined(EA_PLATFORM_LINUX)
#if ENABLE_BLAZE_MEM_SYSTEM
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

namespace Blaze 
{

uint32_t Log2(uint64_t x)
{
    //There are much faster ways of doing this, but we only do this a few thousand times during startup
    //and then never again, so we can live with it for now without cluttering the code.  (Also the _really_ fast ways don't seem
    //to work with int64s
    uint32_t result = 0;
    while (x >>= 1) result++;
    return result;
}

}

CoreManager::CoreManager(Blaze::Allocator &defaultAllocator, void* startAddress, uint64_t totalSize, size_t bucketSize) :
    mDefaultAllocator(defaultAllocator),
    mTotalSize(0),
    mMaxBucketCount(totalSize / bucketSize),
    mBucketSize(bucketSize),
    mBucketShift(0),
    mUsedBucketCount(0),
    mCoreEntryVector(Blaze::BlazeStlAllocator("CoreManagerEntryVector", &defaultAllocator)),        
    mAllCoreMem((char8_t*) startAddress)
{
    //Calc the bucket shift
    mBucketShift = Blaze::Log2(mBucketSize);       
}

CoreManager::~CoreManager()
{
    OSFunctions::freeCore(mAllCoreMem, mCoreEntryVector.size() * mBucketSize);
}


/*! ************************************************************************************************/
/*! \brief Assigns a core memory to the given allocator that will handle the given allocation

    This function is called in response to the allocator not having enough core memory to satisfy
    a malloc request.  The function cycles through the vector of assigned memory and tries to find 
    a free bucket that has not been used.  If the requested allocation does not fit within a single 
    bucket, the function attempts to find a contiguous range of buckets that are free.  If it reaches
    the end of the array without finding a free bucket or range of buckets it attempts to add a 
    new bucket.  If that in turn fails, then the operation fails (and the application will most 
    likely core with no memory available).
***************************************************************************************************/
bool CoreManager::assignCore(BlazeAllocatorInterface& allocator, size_t minCoreSize, char8_t*& resultMem, size_t& resultSize)
{
    //Only one thread at a time please
    EA::Thread::AutoMutex lock(mLock);

    //how many buckets do we need?  Remainder equals a whole extra core
    size_t needBucketCount= (minCoreSize / mBucketSize) + 1;

    bool attemptToGetCore = true;
    do
    {
        //Find the first free core memory we can.          
        for (CoreEntryVector::iterator itr = mCoreEntryVector.begin(); itr != mCoreEntryVector.end(); ++itr)
        {
            if (itr->isFree())
            {
                //Make sure we have all the consecutive buckets we need for a single 
                //large allocation
                bool enoughBuckets = true;
                CoreEntryVector::iterator startItr = itr;
                CoreEntryVector::iterator endItr = itr;
                if ((size_t) eastl::distance(itr, mCoreEntryVector.end()) >= needBucketCount)
                {
                    eastl::advance(endItr, needBucketCount);

                    for (++itr; enoughBuckets && itr != endItr; ++itr)
                    {
                        enoughBuckets = itr->isFree();
                    }

                    if (enoughBuckets)
                    {
                        //We have enough.  Go through each bucket and assign them to the allocator
                        for (itr = startItr; itr != endItr; ++itr)
                        {
                            //Assign this bucket 
                            itr->mAllocator = &allocator;   
                            ++mUsedBucketCount;            
                        }

                        resultMem = startItr->mCore;
                        resultSize = needBucketCount * mBucketSize;
                        return true;
                    }
                }                
            }
        }

        //If we got here then that means we couldn't get any core with our existing pages.
        //If possible, add some more core
        attemptToGetCore = addBucket();
    } while (attemptToGetCore);

    //We filled up our allocated area.  We're most likely heading for a crash
    resultMem = nullptr;
    resultSize = 0;
    return false;
}

/*! ************************************************************************************************/
/*! \brief Gets the allocator that owns the specified address

     This function works by getting the offset from the overall base address of the core manager 
     and then bit shifting by our range of allowed memory.  The resulting value should be an index into the 
     core memory allocator.
***************************************************************************************************/
inline BlazeAllocatorInterface* CoreManager::getAllocator(void* addr) const
{
    BlazeAllocatorInterface *result = nullptr;

    uint64_t address = (uint64_t) ((intptr_t) ((char8_t*) addr - mAllCoreMem));
    size_t index = address >> mBucketShift;
    result = mCoreEntryVector[index].mAllocator;

    return result;
}

/*! ************************************************************************************************/
/*! \brief Adds a new bucket to the manager
***************************************************************************************************/
bool CoreManager::addBucket()
{            
    //Try to add core until we beat our address range
    while (mCoreEntryVector.size() < mMaxBucketCount)
    {
        //if we're empty our address is the first one, otherwise we have the last address + bucket size
        char8_t* desiredAddress = !mCoreEntryVector.empty() ? mCoreEntryVector.back().mCore + mBucketSize : mAllCoreMem;

        CoreEntry& entry = mCoreEntryVector.push_back();
        entry.mCore = desiredAddress;

        char8_t* newCoreBlock = (char8_t*) OSFunctions::commitCore(entry.mCore,  mBucketSize);
        entry.mBadBlock = (newCoreBlock == nullptr);

        if (!entry.mBadBlock)
        {
            mTotalSize += mBucketSize;
            return true;
        }
    }

    //At this point we've exhausted our initial allocation block.  Before giving up (and surely crashing) try to see if 
    //any bad blocks are no longer so bad
    size_t counter = 0;
    for (CoreEntryVector::iterator itr = mCoreEntryVector.begin(), end = mCoreEntryVector.end(); itr != end; ++itr, ++counter)
    {
        if (itr->mBadBlock)
        {
            char8_t* newCoreBlock = (char8_t*) OSFunctions::commitCore(itr->mCore, mBucketSize);
            if (newCoreBlock != nullptr)
            {
                //Its good again!
                itr->mBadBlock = true;
                itr->mCore = newCoreBlock;
                return true;
            }
        }
    }

    return false;
}

/*! ************************************************************************************************/
/*! \brief reserve some (typically large) amount of core memory
***************************************************************************************************/
void* OSFunctions::reserveCore(void* address, uint64_t size)
{
#ifdef EA_PLATFORM_WINDOWS
    // Pre: this is a 64 bit enabled build which ensures size received by VirtualAlloc is valid.
    return VirtualAlloc(address, size, MEM_RESERVE, PAGE_READWRITE);
#else
    int flags = MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS;

    void* result = mmap(address, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    result = (result != (void*) (intptr_t) -1) ? result : nullptr;

    if (address != nullptr && result != nullptr && result != address)
    {
        //it didn't give us what we wanted, which probably means this block was taken.  Bail
        munmap(result, size);
        return nullptr;
    }

    return result;
#endif
}

void OSFunctions::freeCore(void *address, uint64_t size)
{
#ifdef EA_PLATFORM_WINDOWS
    VirtualFree(address, 0, MEM_RELEASE);
#else
    munmap(address, size);
#endif
}

void* OSFunctions::commitCore(void *address, size_t size)
{
#ifdef EA_PLATFORM_WINDOWS
    // non-nullptr address used for reserved pages:
    if (address != nullptr)
        return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
    else
        return VirtualAlloc(address, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

    void *result = mmap(address, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    result = (result != (void*) (intptr_t) -1) ? result : nullptr;

    if (address != nullptr && result != nullptr && result != address)
    {
        //it didn't give us what we wanted, which probably means this block was taken.  Bail
        munmap(result, size);
        return nullptr;
    }

    return result;
   
#endif
}

uint32_t OSFunctions::getLastError()
{
#ifdef EA_PLATFORM_WINDOWS
    return ::GetLastError();
#else
    return errno;
#endif
}

namespace Blaze
{
    extern EA_THREAD_LOCAL Blaze::MemoryGroupId gCurrentNonCompliantMemGroupOverride;
    extern EA_THREAD_LOCAL const char8_t* gCurrentNonCompliantNameOverride;
}

void *OSFunctions::mallocHook(size_t size, const void *)
{
    return BLAZE_ALLOC_MGID(size, Blaze::gCurrentNonCompliantMemGroupOverride, Blaze::gCurrentNonCompliantNameOverride);
}

void *OSFunctions::reallocHook(void *ptr, size_t size, const void *caller)
{
    return BLAZE_REALLOC_MGID(ptr, size, Blaze::gCurrentNonCompliantMemGroupOverride, Blaze::gCurrentNonCompliantNameOverride);
}

void *OSFunctions::memalignHook(size_t alignment, size_t size, const void *caller)
{
    return BLAZE_ALLOC_ALIGN_MGID(size, alignment, 0, Blaze::gCurrentNonCompliantMemGroupOverride, Blaze::gCurrentNonCompliantNameOverride);
}


//This function intercepts free
void OSFunctions::freeHook(void *ptr, const void *)
{
    BLAZE_FREE(ptr);
}

//This function initializes all our hooks
void OSFunctions::initMemoryHooks() 
{    
#ifdef EA_PLATFORM_LINUX
    //Ensure that we only do this once.
    if (!msMemHooksEnabled)
    {
        msMemHooksEnabled = true;

#if ENABLE_BLAZE_MEM_SYSTEM
        if (!AllocatorManager::getInstance().isUsingPassthroughAllocator())
        {
            //Temporary hack to avoid a circular dependency with the "backtrace()" call and
            //our allocators
            void* buf[1];
            backtrace(buf, EAArrayCount(buf));

            msOldMallocHook = __malloc_hook;        
            __malloc_hook = &OSFunctions::mallocHook;

            msOldReallocHook = __realloc_hook;
            __realloc_hook = &OSFunctions::reallocHook;

            msOldMemalignHook = __memalign_hook;
            __memalign_hook = &OSFunctions::memalignHook;

            msOldFreeHook = __free_hook;
            __free_hook = &OSFunctions::freeHook;
        }
#endif

    }
#endif
}

//The init hooks function is not actually called directly by the program, but instantiated by the libc
//directly via this special variable set to our init function  
#ifdef EA_PLATFORM_LINUX
#if ENABLE_BLAZE_MEM_SYSTEM 
#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif
void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = &OSFunctions::initMemoryHooks;
#endif

void *(*OSFunctions::msOldMallocHook)(size_t size, const void *caller) = nullptr;
void *(*OSFunctions::msOldReallocHook)(void *ptr, size_t size, const void *caller) = nullptr;
void *(*OSFunctions::msOldMemalignHook)(size_t alignment, size_t size, const void *caller) = nullptr;
void (*OSFunctions::msOldFreeHook)(void *ptr, const void *caller) = nullptr;
bool OSFunctions::msMemHooksEnabled = false;
#endif

void OSFunctions::clearMemoryHooks()
{
#ifdef EA_PLATFORM_LINUX

#if ENABLE_BLAZE_MEM_SYSTEM
    if (!AllocatorManager::getInstance().isUsingPassthroughAllocator())
    {
        __malloc_hook = msOldMallocHook;
        __realloc_hook = msOldReallocHook;
        __memalign_hook = msOldMemalignHook;
        __free_hook = msOldFreeHook;
    }
#endif

    msMemHooksEnabled = false;
#endif
}

uint32_t OSFunctions::getProcessId()
{
#ifdef EA_PLATFORM_WINDOWS
    return _getpid();
#else
    return getpid();
#endif
}

void OSFunctions::exitAndDie(const char8_t* msg)
{
#ifdef EA_PLATFORM_WINDOWS
    fprintf(stderr, "%s\n", msg);
    exit(-1);
#else
    fprintf(stderr, "%s\n", msg);
    raise(SIGSEGV); //toss a core
    exit(-1); //should never get here
#endif
}

#if defined(EA_PLATFORM_LINUX)
#if ENABLE_BLAZE_MEM_SYSTEM
    #pragma clang diagnostic pop
#endif
#endif