/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/allocator/coremanager.h"
#include "framework/system/allocator/allocatormanager.h"
#include "framework/system/allocator/memallocator.h"

#if defined(EA_PLATFORM_LINUX)
#include <fcntl.h>
#elif defined(EA_PLATFORM_WINDOWS)
#include <psapi.h>
#endif

#include <new>

extern const char8_t* ENV_ALLOC_TYPE_PASSTHROUGH;
extern const char8_t* ENV_ALLOC_TYPE_NORMAL;
extern const char8_t* ENV_ALLOC_TYPE_PPMALLOC;
extern const char8_t* ENV_ALLOC_TYPE_PTMALLOC3;

#if !defined(BLAZE_DEFAULT_MALLOC_ALIGNMENT)

// IMPORTANT: This default is a deliberate fix for a CLANG compiler anomaly(https://bugs.llvm.org/show_bug.cgi?id=35901) 
// that causes code of a certain complexity to use aligned instructions (MOVAPS instead of MOVUPS) when initializing 
// memory for objects allocated with 'new'. Such code then generates a SEGFAULT on Linux because malloc() 
// (which is used by new(unsigned int) within the Blaze server) only guarantees word(8 byte) alignment for returned memory.
// To avoid this issue we take a 'small' memory waste penalty by ensuring all heap-allocated memory
// on Blaze server to be aligned to 16 bytes. Note that although we do not use CLANG on Windows platform, 
// it has its own issues that require using of aligned allocations; therefore, in the interest of consistency we enable
// 16 byte alignment for both Linux and Windows platforms. See: GOSOPS-192421 for more details on the original alignment issue.
#define BLAZE_DEFAULT_MALLOC_ALIGNMENT 16

#endif

namespace Blaze
{
extern EA_THREAD_LOCAL bool gAllowThreadExceptions;
}

//Allocator interface
Blaze::Allocator* Blaze::Allocator::getAllocator(Blaze::MemoryGroupId memGroupId)
{
    return AllocatorManager::getInstance().getAllocator(memGroupId);
}

Blaze::Allocator* Blaze::Allocator::getAllocatorByPtr(void* addr)
{
    return (addr != nullptr) ? AllocatorManager::getInstance().getAllocatorByPtr(addr) : 
        &AllocatorManager::getInstance().getInternalAllocator(); //This won't actually be used since its a no-op or crash
}

void Blaze::Allocator::getStatus(Blaze::MemoryStatus& status)
{
    return AllocatorManager::getInstance().getStatus(status);
}

// static
void Blaze::Allocator::getProcessMemUsage(uint64_t& size, uint64_t& resident)
{
// get system reported resident memory
#ifdef EA_PLATFORM_UNIX
    pid_t pid = ::getpid();

    char procFileName[64];
    blaze_snzprintf(procFileName, sizeof(procFileName), "/proc/%d/statm", pid);

    FILE *file = fopen(procFileName, "r");
    if (file != nullptr)
    {
        uint64_t programSize = 0;
        uint64_t residentSize = 0;
        fscanf(file, "%" PRIu64 " %" PRIu64, &programSize, &residentSize);
        fclose(file);
        long pageSize = ::sysconf(_SC_PAGESIZE);
        size = programSize * pageSize;
        resident = programSize * pageSize;
    }
    else
    {
        size = 0;
        resident = 0;
    }
#elif EA_PLATFORM_WINDOWS
    HANDLE hProc = ::GetCurrentProcess();

    PROCESS_MEMORY_COUNTERS memCounters;

    if (GetProcessMemoryInfo(hProc, &memCounters, sizeof(memCounters)))
    {
        size = memCounters.PeakWorkingSetSize;
        resident = memCounters.WorkingSetSize;
    }
    else
    {
        size = 0;
        resident = 0;
    }

#endif
}

// static
void Blaze::Allocator::getSystemMemorySize(uint64_t& totalMem, uint64_t& freeMem)
{
#ifdef EA_PLATFORM_LINUX
    totalMem = 0;
    freeMem = 0;
    int32_t fd = open("/proc/meminfo", O_RDONLY);
    if (fd >= 0)
    {
        char8_t buf[1024];
        int32_t br = read(fd, buf, sizeof(buf)-1);
        if (br > 0)
        {
            buf[br] = '\0';
            int64_t memTotal = 0;
            int64_t memFree = 0;
            int64_t buffers = 0;
            int64_t cached = 0;
            int32_t items = sscanf(buf, "MemTotal: %" SCNd64 " kB\nMemFree: %" SCNd64 " kB\nBuffers: %" SCNd64 " kB\nCached: %" SCNd64 " kB", &memTotal, &memFree, &buffers, &cached);
            if (items > 0)
            {
                totalMem = memTotal * 1024;
                if (items == 4)
                    freeMem = (memFree + buffers + cached) * 1024;
            }
        }
        close(fd);
    }
#elif EA_PLATFORM_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    totalMem = (uint64_t)status.ullTotalPhys;
    freeMem = (uint64_t)status.ullAvailPhys;
#endif
}

const char8_t* Blaze::Allocator::getTypeName()
{
    return AllocatorManager::getInstance().getTypeName();
}

void Blaze::Allocator::processConfigureMemoryMetrics(const Blaze::ConfigureMemoryMetricsRequest &request, Blaze::ConfigureMemoryMetricsResponse &response)
{
    AllocatorManager::getInstance().getGlobalMemTracker().processConfigureMemoryMetrics(request, response);
}

void Blaze::Allocator::processGetMemoryMetrics(const Blaze::GetMemoryMetricsRequest &request, Blaze::GetMemoryMetricsResponse &response)
{
    AllocatorManager::getInstance().getGlobalMemTracker().processGetMemoryMetrics(request, response);
}

namespace Blaze
{
    extern EA_THREAD_LOCAL Blaze::MemoryGroupId gCurrentNonCompliantMemGroupOverride;
    extern EA_THREAD_LOCAL const char8_t* gCurrentNonCompliantNameOverride;
}

void Blaze::Allocator::setNonCompliantDefaults(Blaze::MemoryGroupId id, const char8_t* name)
{
    gCurrentNonCompliantMemGroupOverride = id;
    gCurrentNonCompliantNameOverride = name;
}


//These are our special array operators which do an array allocation from the proper memgroup.
void* operator new[](size_t size, Blaze::MemoryGroupId memGroupId, const char8_t *allocName)
{
    return Blaze::Allocator::getAllocator(memGroupId)->Alloc(size, allocName, 0);
}

void* operator new[](size_t size, Blaze::MemoryGroupId memGroupId, const char8_t *allocName, uint32_t align, uint32_t alignOffset)
{
    return Blaze::Allocator::getAllocator(memGroupId)->Alloc(size, allocName, 0, align, alignOffset);
}

//These should never actually be called, but the compiler may generate them
void operator delete[](void* ptr, Blaze::MemoryGroupId x, const char8_t *allocName) 
{
    Blaze::Allocator::getAllocatorByPtr(ptr)->Free(ptr);
}

void operator delete[](void* ptr, Blaze::MemoryGroupId x, const char8_t *allocName, uint32_t align, uint32_t alignOffset) 
{
    Blaze::Allocator::getAllocatorByPtr(ptr)->Free(ptr);
}

EA::Allocator::ICoreAllocator* EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
    EA_ASSERT_MSG(false, "Invalid use of ICoreAllocator::GetDefaultAllocator()");
    return nullptr;  // This should never be used, so die horribly if we do.
}

//EASTL Allocators
//Everything in EASTL should route through a core allocator.  If they aren't, then it will eventually 
//bubble out here.  
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    void *result = nullptr;
    EA_FAIL_MSG("Invalid use of eastl::allocator - All usage of EASTL should route through ICoreAllocator");
    return result;
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    void *result = nullptr;
    EA_FAIL_MSG("Invalid use of eastl::allocator - All usage of EASTL should route through ICoreAllocator");
    return result;
}


#if ENABLE_BLAZE_MEM_SYSTEM
//These operators are our "non-compliant" operators which don't use the memory interface
void* operator new(size_t size)
{   
    return Blaze::Allocator::getAllocator(Blaze::gCurrentNonCompliantMemGroupOverride)->Alloc(size, Blaze::gCurrentNonCompliantNameOverride, 0);
}

void* operator new[](size_t size)
{
    return Blaze::Allocator::getAllocator(Blaze::gCurrentNonCompliantMemGroupOverride)->Alloc(size, Blaze::gCurrentNonCompliantNameOverride, 0);
}

//Global Delete operators - these replace the normal deletes and funnel through the allocator
void operator delete(void* ptr)
{
    if (ptr != nullptr)
    {
        Blaze::Allocator::getAllocatorByPtr(ptr)->Free(ptr);
    }
}

void operator delete[](void* ptr)
{
    if (ptr != nullptr)
    {
        Blaze::Allocator::getAllocatorByPtr(ptr)->Free(ptr);
    }
}
// Blaze code does not call std::nothrow_t variations but we provide these here in case some system libs do. These will avoid calling the compiler implementation which may or
// may not use the same heap. 
void* operator new(size_t size, const std::nothrow_t& /*nothrow_constant*/) EA_NOEXCEPT
{
    return operator new(size);
}

void operator delete(void *ptr, const std::nothrow_t& /*nothrow_constant*/)
{
    operator delete(ptr);
}

void* operator new[](size_t size, const std::nothrow_t& /*nothrow_constant*/) EA_NOEXCEPT
{
    return operator new[](size);
}

void operator delete[](void *ptr, const std::nothrow_t& /*nothrow_constant*/)
{
    operator delete[](ptr);
}
#endif


void *InternalAllocator::Alloc(size_t size, const char *name, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mAllocator.MallocAligned(size, BLAZE_DEFAULT_MALLOC_ALIGNMENT, 0, flags);
    Blaze::gAllowThreadExceptions = true;

    return mem;
}

void *InternalAllocator::Alloc(size_t size, const char *name, unsigned int flags,
                    unsigned int align, unsigned int alignOffset)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mAllocator.MallocAligned(size, align, alignOffset, flags);
    Blaze::gAllowThreadExceptions = true;

    return mem;
}

void* InternalAllocator::Realloc(void *addr, size_t size, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;

    size_t currentSize = 0;
    if (addr != nullptr)
        currentSize = mAllocator.GetUsableSize(addr); // workaround for PPMalloc: GeneralAllocator::GetUsableSize(nullptr) -> ((size_t)-1)
    void* mem = nullptr;
    if (size >= currentSize)
    {
        // NOTE: Can't use realloc() because it does not guarantee alignment, instead manually do aligned alloc && copy
        mem = mAllocator.MallocAligned(size, BLAZE_DEFAULT_MALLOC_ALIGNMENT, 0, flags);
        if (addr != nullptr)
        {
            if (mem != nullptr)
                memcpy(mem, addr, currentSize);
            mAllocator.Free(addr);
        }
    }
    else
    {
        // NOTE: when shrinking, guaranteed not to reallocate, so if it was aligned it stays aligned
        mem = mAllocator.Realloc(addr, size, flags);
    }

    Blaze::gAllowThreadExceptions = true;

    return mem;
}

void InternalAllocator::Free(void *block, size_t size)
{
    Blaze::gAllowThreadExceptions = false;
    mAllocator.Free(block);
    Blaze::gAllowThreadExceptions = true;
}


BlazeAllocator::BlazeAllocator(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager, EA::Allocator::GeneralAllocator* allocatorToUse) : 
    mId(id), 
    mCoreManager(coreManager),
    mInternalAllocator(manager.getInternalAllocator()),
    mAllocator((allocatorToUse != nullptr) ? *allocatorToUse : *(CORE_NEW(&manager.getInternalAllocator(), "GeneralAllocator", EA::Allocator::MEM_PERM) EA::Allocator::GeneralAllocator())),
    mCoreMemorySize(0),
    mMetrics(nullptr)
{
    //Turn off internal allocations.  We'll add core manually as needed
    mAllocator.SetOption(EA::Allocator::GeneralAllocator::kOptionEnableSystemAlloc, 0);
    mAllocator.SetMallocFailureFunction(&BlazeAllocator::mallocFailCallback, (void*) this);
}

BlazeAllocator::~BlazeAllocator()
{
    delete mMetrics;
    CORE_DELETE(&mInternalAllocator, &mAllocator);
}

bool BlazeAllocator::addCore(size_t requestedSize)
{
    char8_t* mem = nullptr;
    size_t memSize = 0;      
    if (mCoreManager.assignCore(*this, requestedSize + CORE_OVERHEAD, mem, memSize))
    {
        mCoreMemorySize += memSize;
        mAllocator.AddCore(mem, memSize);
        return true;
    }

    return false;
}

const char8_t* BlazeAllocator::getTypeName() const { return ENV_ALLOC_TYPE_NORMAL; }

void BlazeAllocator::getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const
{
    char8_t name[64];
    AllocatorManager::getMemoryGroupName(mId, name, sizeof(name));
    
    status.setId(mId);
    status.setName(name);
    status.getStats().setAllocatedCoreMemory(mCoreMemorySize);
}

void BlazeAllocator::initMetrics(Blaze::Metrics::MetricsCollection& collection)
{
    if (mMetrics != nullptr)
    {
        char8_t name[32];
        AllocatorManager::getMemoryGroupName(mId, name, sizeof(name));
        BLAZE_SPAM_LOG(Blaze::Log::SYSTEM, "[BlazeAllocator].initMetrics: metrics for memory group (" << name << ") have already been initialized");
        return;
    }

    mMetrics = BLAZE_NEW AllocatorMetrics(*this, collection);
}

void *BlazeAllocator::Alloc(size_t size, const char *name, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mAllocator.MallocAligned(size, BLAZE_DEFAULT_MALLOC_ALIGNMENT, 0, flags);
    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void *BlazeAllocator::Alloc(size_t size, const char *name, unsigned int flags,
                    unsigned int align, unsigned int alignOffset)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mAllocator.MallocAligned(size, align, alignOffset, flags);
    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void* BlazeAllocator::Realloc(void *addr, size_t size, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;

    size_t currentSize = 0;
    if (addr != nullptr)
        currentSize = mAllocator.GetUsableSize(addr); // workaround for PPMalloc: GeneralAllocator::GetUsableSize(nullptr) -> ((size_t)-1)
    void* mem = nullptr;
    if (size >= currentSize)
    {
        // NOTE: Can't use realloc() because it does not guarantee alignment, instead manually do aligned alloc && copy
        mem = mAllocator.MallocAligned(size, BLAZE_DEFAULT_MALLOC_ALIGNMENT, 0, flags);
        if (addr != nullptr)
        {
            if (mem != nullptr)
                memcpy(mem, addr, currentSize);
            mAllocator.Free(addr);
        }
    }
    else
    {
        // NOTE: when shrinking, guaranteed not to reallocate, so if it was aligned it stays aligned
        mem = mAllocator.Realloc(addr, size, flags);
    }

    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void BlazeAllocator::Free(void *block, size_t size)
{
    Blaze::gAllowThreadExceptions = false;
    mAllocator.Free(block);
    Blaze::gAllowThreadExceptions = true;
}

void *PassthroughAllocator::Alloc(size_t size, const char *name, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
#ifdef EA_PLATFORM_LINUX
    void* mem = memalign(BLAZE_DEFAULT_MALLOC_ALIGNMENT, size);
#else
    // On Windows, malloc needs to be coupled with free and _aligned_malloc needs to be coupled with _aligned_free.
    // But when calling free, the caller may not have an idea if the allocation was aligned. So we simply make _aligned_malloc even for a malloc call. 
    void* mem = _aligned_malloc(size, BLAZE_DEFAULT_MALLOC_ALIGNMENT);
#endif
    Blaze::gAllowThreadExceptions = true;

    return mem;
}

void *PassthroughAllocator::Alloc(size_t size, const char *name, unsigned int flags,
                    unsigned int align, unsigned int alignOffset)
{
    Blaze::gAllowThreadExceptions = false;
#ifdef EA_PLATFORM_LINUX
    void* mem = memalign(align, size);
#else
    void* mem = _aligned_malloc(size, align);
#endif

    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void* PassthroughAllocator::Realloc(void *addr, size_t size, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
#ifdef EA_PLATFORM_LINUX
    size_t currentSize = malloc_usable_size(addr);
    void* mem = nullptr;
    if (size >= currentSize)
    {
        // NOTE: Can't use realloc() because it does not guarantee alignment, instead manually do aligned alloc && copy
        mem = memalign(BLAZE_DEFAULT_MALLOC_ALIGNMENT, size);
        if (addr != nullptr)
        {
            if (mem != nullptr)
                memcpy(mem, addr, currentSize);
            free(addr);
        }
    }
    else
    {
        // NOTE: when shrinking, guaranteed not to reallocate, so if it was aligned it stays aligned
        mem = realloc(addr, size);
    }
#else
    void* mem = _aligned_realloc(addr, size, BLAZE_DEFAULT_MALLOC_ALIGNMENT);
#endif
    Blaze::gAllowThreadExceptions = true;

    return mem;
}

void PassthroughAllocator::Free(void *block, size_t size)
{
    Blaze::gAllowThreadExceptions = false;
#ifdef EA_PLATFORM_LINUX
    free(block);
#else
    _aligned_free(block);
#endif

    Blaze::gAllowThreadExceptions = true;
}

const char8_t* PassthroughAllocator::getTypeName() const { return ENV_ALLOC_TYPE_PASSTHROUGH; }

#ifdef ENABLE_PTMALLOC3

static void* ptmalloc3MoreCore(void* extp, intptr_t size)
{
    Ptmalloc3Allocator* allocator = (Ptmalloc3Allocator*)extp;
    return allocator->addCore(size);
}

Ptmalloc3Allocator::Ptmalloc3Allocator(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager)
    : mId(id),
      mCoreManager(coreManager),
      mInternalAllocator(manager.getInternalAllocator()),
      mCoreMemorySize(0),
      mAllocatorControlBlock(CORE_NEW_ARRAY(&mInternalAllocator, char, sizeof_mspace(), "ptmalloc3", EA::Allocator::MEM_PERM)),
      mAllocator(create_mspace_with_base(mAllocatorControlBlock, sizeof_mspace(), 1, ptmalloc3MoreCore, this)),
      mLastAddCorePtr(nullptr),
      mMetrics(nullptr)
{
}

Ptmalloc3Allocator::~Ptmalloc3Allocator()
{
    delete mMetrics;
    destroy_mspace(mAllocator);
    CORE_DELETE(&mInternalAllocator, &mAllocator);
}

void* Ptmalloc3Allocator::addCore(intptr_t requestedSize)
{
    if (requestedSize == 0)
        return mLastAddCorePtr;

    if (requestedSize > 0)
    {
        char8_t* mem = nullptr;
        size_t memSize = 0;      
        if (mCoreManager.assignCore(*this, (size_t)requestedSize, mem, memSize))
        {
            mCoreMemorySize += memSize;
            mLastAddCorePtr = mem + memSize;
            return mem;
        }
    }

    return (void*)(~(size_t)0);
}

void Ptmalloc3Allocator::getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const
{
    char8_t name[32];
    AllocatorManager::getMemoryGroupName(mId, name, sizeof(name));
    
    status.setId(mId);
    status.setName(name);    
    status.getStats().setAllocatedCoreMemory(mCoreMemorySize);
    status.getStats().setUsedMemory(mspace_used_memory(mAllocator));
    status.getStats().setMaxUsedMemory(mspace_max_used_memory(mAllocator));
}

void Ptmalloc3Allocator::initMetrics(Blaze::Metrics::MetricsCollection& collection)
{
    if (mMetrics != nullptr)
    {
        char8_t name[32];
        AllocatorManager::getMemoryGroupName(mId, name, sizeof(name));
        BLAZE_WARN_LOG(Blaze::Log::SYSTEM, "[Ptmalloc3Allocator].initMetrics: metrics for memory group (" << name << ") have already been initialized");
        return;
    }

    mMetrics = BLAZE_NEW AllocatorMetrics(*this, collection);
}

void *Ptmalloc3Allocator::Alloc(size_t size, const char *name, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mspace_memalign(mAllocator, BLAZE_DEFAULT_MALLOC_ALIGNMENT, size);
    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void *Ptmalloc3Allocator::Alloc(size_t size, const char *name, unsigned int flags,
                    unsigned int align, unsigned int alignOffset)
{
    Blaze::gAllowThreadExceptions = false;
    void* mem = mspace_memalign(mAllocator, align, size);
    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void* Ptmalloc3Allocator::Realloc(void *addr, size_t size, unsigned int flags)
{
    Blaze::gAllowThreadExceptions = false;
    size_t currentSize = mspace_usable_size(addr);
    void* mem = nullptr;
    if (size >= currentSize)
    {
        // NOTE: Can't use mspace_realloc() because it does not guarantee alignment, instead manually do aligned alloc && copy
        mem = mspace_memalign(mAllocator, BLAZE_DEFAULT_MALLOC_ALIGNMENT, size);
        if (addr != nullptr)
        {
            if (mem != nullptr)
                memcpy(mem, addr, currentSize);
            mspace_free(mAllocator, addr);
        }
    }
    else
    {
        // NOTE: when shrinking, guaranteed not to reallocate, so if it was aligned it stays aligned
        mem = mspace_realloc(mAllocator, addr, size);
    }
    Blaze::gAllowThreadExceptions = true;
    return mem;
}

void Ptmalloc3Allocator::Free(void *block, size_t size)
{
    Blaze::gAllowThreadExceptions = false;
    mspace_free(mAllocator, block);
    Blaze::gAllowThreadExceptions = true;
}

const char8_t* Ptmalloc3Allocator::getTypeName() const { return ENV_ALLOC_TYPE_PTMALLOC3; }


#endif // ENABLE_PTMALLOC3


