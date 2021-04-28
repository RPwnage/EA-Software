/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/rawbufferpool.h"
#include "framework/util/environmenthelper.h"
#include "framework/tdf/controllertypes_server.h" //for MemoryStatus definition
#include "framework/system/allocator/coremanager.h"
#include "framework/system/allocator/allocatormanager.h"
#include "EAIO/Allocator.h"

//Constants that tune our memory setup - sizes MUST be a power of two for the address shifting
//to work correctly
static const uint64_t DEFAULT_CORE_MANAGER_SIZE = 1ULL << 34ULL; //16 GB
static const size_t DEFAULT_CORE_MANAGER_BUCKET_SIZE = 1ULL << 22ULL; //8 MB
static const uint64_t DEFAULT_CORE_MEMORY_BUFFER_SIZE = 1ULL << 32ULL; //4GB

const char8_t* BLAZE_INVALID_ALLOC_NAME = "INVALID_CONTEXT";

static const char8_t* ALLOC_TYPE_ENV_NAME = "BLAZE_MEM_ALLOC_TYPE";
static const char8_t* ALLOC_TRACK_ENV_NAME = "BLAZE_MEM_ALLOC_TRACKING";

const char8_t* ALLOC_TYPE_CMD_NAME = "--memAllocType";
const char8_t* ALLOC_TRACK_CMD_NAME = "--memAllocTracking";
const char8_t* ALLOC_TRACK_SETTINGS_ALL = "all";
const char8_t* ALLOC_TRACK_SETTINGS_NONE = "none";

const char8_t* ENV_ALLOC_TYPE_PASSTHROUGH = "PASSTHROUGH";
const char8_t* ENV_ALLOC_TYPE_NORMAL = "NORMAL";
const char8_t* ENV_ALLOC_TYPE_PPMALLOC = "PPMALLOC";
const char8_t* ENV_ALLOC_TYPE_PTMALLOC3 = "PTMALLOC3";


AllocatorManager* AllocatorManager::msInstance = nullptr;
static char8_t msAllocationMemory[sizeof(AllocatorManager)];

AllocatorManager& AllocatorManager::getInstance()
{
    static AllocatorManager* sInstance = new(msAllocationMemory) AllocatorManager;
    return *sInstance;
}

AllocatorManager::AllocatorManager() 
: mStartAddress(nullptr),
  mAllocatorType(BlazeAllocatorInterface::ALLOC_PASSTHROUGH),
  mAllocTrackSettings(ALLOC_TRACK_SETTINGS_ALL), 
  mGlobalMemTracker(mInternalAllocator)
{
    msInstance = this;

#if ENABLE_CLANG_SANITIZERS
    mAllocatorType = BlazeAllocatorInterface::ALLOC_PASSTHROUGH; // This is the default but we enforce it here anyway.
    (void)(ALLOC_TYPE_ENV_NAME);
#else
    //Determine our allocator type
    //First search the command line - explicit command line setting trumps environment
    const char8_t* allocType = Blaze::EnvironmentHelper::getCmdLineOption(ALLOC_TYPE_CMD_NAME, true);

    //Nothing passed on the command line, try the environment
    if (allocType == nullptr)
        allocType = getenv(ALLOC_TYPE_ENV_NAME);
    
    if (allocType == nullptr || blaze_stricmp(allocType, ENV_ALLOC_TYPE_NORMAL) == 0)
    {
#ifdef ENABLE_PTMALLOC3
        mAllocatorType = BlazeAllocatorInterface::ALLOC_PTMALLOC3;
#else
        mAllocatorType = BlazeAllocatorInterface::ALLOC_PPMALLOC;
#endif
    }
#endif
    
    mCategoryCount = Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY + Blaze::BlazeRpcComponentDb::getTotalComponentCount() + Blaze::BlazeRpcComponentDb::getTotalProxyComponentCount();
    if (mCategoryCount > 128) // we currently allocate 7 bits to the component/category ID
    {
        char8_t buf[1024];
        sprintf(buf, "Number of components(%d) exceed the current limitations.  Aborting execution...\n",(int)(mCategoryCount));
        OSFunctions::exitAndDie(buf);
    }

    // Allocate cores for each category, plus extra for Framework.
    mCoreManagerCount = 0;
    for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
    {
        size_t allocCount = getCategoryAllocCount(catIdx);
        mCoreManagerCount += allocCount;
        if (allocCount > Blaze::MEM_GROUP_MAX_ALLOC_COUNT)
        {
            char8_t buf[1024];
            sprintf(buf, "Component in index (%d) is using (%d) alloc groups (> %d max).  Aborting execution...\n",(int)(catIdx - Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY), (int)(allocCount), Blaze::MEM_GROUP_MAX_ALLOC_COUNT);
            OSFunctions::exitAndDie(buf);
        }
    }
    mMemGroupCategories = (AllocatorManager::MemGroupCategoryInfo*) CORE_ALLOC(&mInternalAllocator, sizeof(AllocatorManager::MemGroupCategoryInfo) * mCategoryCount, "AllocationManager::mMemGroupCategories", 0);
    memset(mMemGroupCategories, 0, sizeof(MemGroupCategoryInfo*) * mCategoryCount);

    mAllocators = (BlazeAllocatorInterface***) CORE_ALLOC(&mInternalAllocator, sizeof(BlazeAllocatorInterface**) * mCategoryCount, "AllocationManager::mAllocators", 0);
    memset(mAllocators, 0, sizeof(BlazeAllocatorInterface**) * mCategoryCount);
    for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
    {
        mAllocators[catIdx] = (BlazeAllocatorInterface**) CORE_ALLOC(&mInternalAllocator, sizeof(BlazeAllocatorInterface*) * getCategoryAllocCount(catIdx), "AllocationManager::mAllocators[]", 0);
        memset(mAllocators[catIdx], 0, sizeof(BlazeAllocatorInterface*) * getCategoryAllocCount(catIdx));
    }


    mCoreManagers = (CoreManager**) CORE_ALLOC(&mInternalAllocator, sizeof(CoreManager*) * mCoreManagerCount, "AllocationManager::mCoreManagers", 0);
    memset(mCoreManagers, 0, sizeof(CoreManager*) * mCoreManagerCount);

    if (isUsingPassthroughAllocator())
    {
        //if we don't use real allocators, skip initialization of core managers
        mCoreManagerCount = 0;
    }
    else
    {
        initializeCoreManagers();
    }

    //Determine our allocator memory tracking settings
    //First search the command line - explicit command line setting trumps environment
    const char8_t* allocTrackSettings = Blaze::EnvironmentHelper::getCmdLineOption(ALLOC_TRACK_CMD_NAME, true);
    //Nothing passed on the command line, try the environment
    if (allocTrackSettings == nullptr)
        allocTrackSettings = getenv(ALLOC_TRACK_ENV_NAME);
    if (allocTrackSettings != nullptr)
        mAllocTrackSettings = allocTrackSettings;

    //memory tracking
    bool trackingEnabled = blaze_stricmp(mAllocTrackSettings, ALLOC_TRACK_SETTINGS_ALL) == 0;
        
    //Initialize all the allocators with a core manager
    size_t coreCounter = 0;
    for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
    {
        for (size_t allocIdx = 0; allocIdx < getCategoryAllocCount(catIdx); allocIdx++)
        {
            const Blaze::MemoryGroupId mgId = MEM_GROUP_MAKE_ID(catIdx, allocIdx);
            const size_t coreIndex = (mCoreManagerCount <= 1) ? 0 : coreCounter++;
            mAllocators[catIdx][allocIdx] = createAllocator(mgId, mCoreManagers[coreIndex], trackingEnabled);
        }
    }

    EA::IO::SetAllocator(&mInternalAllocator);
}

AllocatorManager::~AllocatorManager()
{
    shutdownCoreManagers();

    CORE_FREE(&mInternalAllocator, mCoreManagers);

    if (mAllocators != nullptr)
    {
        for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
        {
            CORE_FREE(&mInternalAllocator, mAllocators[catIdx]);
        }
    }
    CORE_FREE(&mInternalAllocator, mAllocators);
    CORE_FREE(&mInternalAllocator, mMemGroupCategories);
}

BlazeAllocatorInterface* AllocatorManager::createAllocator(Blaze::MemoryGroupId id, CoreManager* coreManager, bool memTracking)
{
    switch (mAllocatorType)
    {
#ifdef ENABLE_PTMALLOC3
        case BlazeAllocatorInterface::ALLOC_PTMALLOC3:
            return memTracking ? 
                CORE_NEW(&mInternalAllocator, "Ptmalloc3AllocatorTracking", EA::Allocator::MEM_PERM) Ptmalloc3AllocatorTracking(id, *coreManager, *this) :
                CORE_NEW(&mInternalAllocator, "Ptmalloc3Allocator", EA::Allocator::MEM_PERM) Ptmalloc3Allocator(id, *coreManager, *this);
#endif
        case BlazeAllocatorInterface::ALLOC_PPMALLOC:
            return memTracking ? 
                CORE_NEW(&mInternalAllocator, "BlazeAllocatorTracking", EA::Allocator::MEM_PERM) BlazeAllocatorTracking(id, *coreManager, *this) :
                CORE_NEW(&mInternalAllocator, "BlazeAllocator", EA::Allocator::MEM_PERM) BlazeAllocator(id, *coreManager, *this);
        case BlazeAllocatorInterface::ALLOC_PASSTHROUGH:
        default:
            return memTracking ? 
                CORE_NEW(&mInternalAllocator, "PassthroughAllocatorTracking", EA::Allocator::MEM_PERM) PassthroughAllocatorTracking(id, *coreManager, *this) :
                CORE_NEW(&mInternalAllocator, "PassthroughAllocator", EA::Allocator::MEM_PERM) PassthroughAllocator(id, *coreManager, *this);
    }
}

/*! ************************************************************************************************/
/*! \brief Sets up our core managers

    This function reserves the address space for ALL core managers in one go.  This ensures we have
    a valid start address that the kernel will give us.  This memory is released after the coreas are
    initialized, as keeping it reserved would show a virtual memory space of multiple TBs.  Once its 
    allocated, the memory is split between each core manager.
***************************************************************************************************/
void AllocatorManager::initializeCoreManagers()
{    
    uint64_t coreManagerSize = DEFAULT_CORE_MANAGER_SIZE;
    uint64_t totalCoreSize = mCoreManagerCount * coreManagerSize;

#if defined(EA_PLATFORM_WINDOWS)
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa384271(v=vs.85).aspx
    // allocating more than 7 TeraBytes of virtual space may result in a failure on Windows-64bit. Die early. 
    if (totalCoreSize > 7LL * 1024LL * 1024LL * 1024LL * 1024LL)
    {
        char8_t buf[1024];
        sprintf(buf, "Current configuration exhausts Windows 64-bit virtual space. (%" PRIu64 ") size exceed the current limitations.  Aborting execution...\n",totalCoreSize);
        OSFunctions::exitAndDie(buf);
    }
#endif
    mAddressShift = Blaze::Log2(coreManagerSize);    

    //"Prime" the address space by putting a buffer between where the system will start allocating addresses
    //and where we actually allocate addresses.  This isn't a guarantee that we still won't get bad blocks, but
    //most VM managers seem to either use the first addressable or last used - they generally don't go for the
    //middle blocks
    void* bufferAddress = OSFunctions::reserveCore(nullptr, DEFAULT_CORE_MEMORY_BUFFER_SIZE);

    //Get the start address by reserving all the mem we'll need up front
    mStartAddress = OSFunctions::reserveCore(nullptr, totalCoreSize);

    if (mStartAddress == nullptr || bufferAddress == nullptr)
    {
        //Big problems - we can't get our memory.  At this point we pretty much just need to exit, there's 
        //nothing to be done by anyone

        //Need to clear out the hooks because our memory allocation doesn't work, and printf needs to 
        //get memory somehow
        OSFunctions::clearMemoryHooks();

        //Lets at least try and print a nice message - can't use existing logging features because at this point
        //they do not exist
        char8_t buf[1024];
        sprintf(buf, "Unable to reserve initial core memory request. totalCoreSize (%" PRIu64 "), defaultCoreSize(%" PRIu64 ") because of error %u.\n Aborting execution...\n", 
            totalCoreSize, DEFAULT_CORE_MEMORY_BUFFER_SIZE, OSFunctions::getLastError());

        //die immediately, there's no point in going on.
        OSFunctions::exitAndDie(buf);
        //lint -unreachable
    }

    for (size_t counter = 0; counter < mCoreManagerCount; ++counter)
    {
        void* address = (char8_t*) mStartAddress + coreManagerSize * counter;
        mCoreManagers[counter] = CORE_NEW(&mInternalAllocator, "CoreManager", EA::Allocator::MEM_PERM) CoreManager(mInternalAllocator, address, coreManagerSize, DEFAULT_CORE_MANAGER_BUCKET_SIZE);
    }

#ifndef EA_PLATFORM_WINDOWS
    //Once we've validated that the address range we have works, no need to keep the memory around
    //Free it and we'll recommit it later.
    OSFunctions::freeCore(mStartAddress, mCoreManagerCount * coreManagerSize); 
#endif

    //Now free the buffer
    OSFunctions::freeCore(bufferAddress, DEFAULT_CORE_MEMORY_BUFFER_SIZE);
}

void AllocatorManager::shutdownCoreManagers() const
{

}

const char8_t* AllocatorManager::getTypeName() const
{
    switch (mAllocatorType) 
    {
    case BlazeAllocatorInterface::ALLOC_PPMALLOC:
        return ENV_ALLOC_TYPE_PPMALLOC;
#ifdef ENABLE_PTMALLOC3
    case BlazeAllocatorInterface::ALLOC_PTMALLOC3:
        return ENV_ALLOC_TYPE_PTMALLOC3;
#endif // ENABLE_PTMALLOC3
    case BlazeAllocatorInterface::ALLOC_PASSTHROUGH:
        return ENV_ALLOC_TYPE_PASSTHROUGH;
    default:
        return "UNKNOWN";
    }
}

void AllocatorManager::getStatus(Blaze::MemoryStatus& status) const
{
#if ENABLE_BLAZE_MEM_SYSTEM
    if (!isUsingPassthroughAllocator())
    {
        uint64_t totalUsed = 0;
        uint64_t totalMax = 0;
        uint64_t totalCore = 0;

        status.getPerAllocatorStats().reserve(mCategoryCount);

        //Go through all our allocators and fill out a status macro.  Empty allocators are not reported
        for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
        {
            for (size_t allocIdx = 0; allocIdx < getCategoryAllocCount(catIdx); allocIdx++)
            {            
                Blaze::MemoryStatus::AllocatorStatus allocStatus;
                mAllocators[catIdx][allocIdx]->getStatus(allocStatus);

                Blaze::MemoryGroupId mgId = MEM_GROUP_MAKE_ID(catIdx, allocIdx);
                if (mgId == Blaze::MEM_GROUP_FRAMEWORK_RAWBUF)
                {
                    // RawBufferPool bypasses the heap for its allocations (in order to free memory back to the OS);
                    // therefore, we need to account for it's memory usage explicitly here.
                    allocStatus.getStats().setUsedMemory(allocStatus.getStats().getUsedMemory() + Blaze::gRawBufferPool->getUsedMemory());
                    allocStatus.getStats().setMaxUsedMemory(allocStatus.getStats().getMaxUsedMemory() + Blaze::gRawBufferPool->getMaxAllocatedMemory());
                    allocStatus.getStats().setAllocatedCoreMemory(allocStatus.getStats().getAllocatedCoreMemory() + Blaze::gRawBufferPool->getAllocatedMemory());
                }

                //If we've ever done allocation here, include it
                if (allocStatus.getStats().getAllocatedCoreMemory() > 0)
                {
                    totalUsed += allocStatus.getStats().getUsedMemory();
                    totalMax += allocStatus.getStats().getMaxUsedMemory();
                    totalCore += allocStatus.getStats().getAllocatedCoreMemory();

                    //push a copy of the status onto the back of the allocation list
                    allocStatus.copyInto(*status.getPerAllocatorStats().pull_back());
                }
            }
        }

        //set the totals
        status.getTotals().setUsedMemory(totalUsed);
        status.getTotals().setMaxUsedMemory(totalMax);
        status.getTotals().setAllocatedCoreMemory(totalCore);
    }
    else
#endif //ENABLE_BLAZE_MEM_SYSTEM 
    //We still run this malloc query even if the alloc system is compiled out.
    {
#ifdef EA_PLATFORM_UNIX
        //On linux try mallinfo
        struct mallinfo info = mallinfo();
        status.getTotals().setUsedMemory(info.uordblks);
        status.getTotals().setMaxUsedMemory(info.uordblks); //no history, 0 makes less sense so just fill in the current total
        status.getTotals().setAllocatedCoreMemory(info.arena + info.hblkhd);
#endif
    } 

    // get system reported resident memory
    uint64_t size;
    uint64_t resident;
    Blaze::Allocator::getProcessMemUsage(size, resident);
    status.setProcessResidentMemory(resident);
}

void AllocatorManager::initMetrics()
{
#if ENABLE_BLAZE_MEM_SYSTEM
    if (!isUsingPassthroughAllocator())
    {
        // Go through all our allocators and initialize their metrics
        for (size_t catIdx = 0; catIdx < mCategoryCount; catIdx++)
        {
            char8_t buf[64];
            const char8_t* categoryName = mMemGroupCategories[catIdx].mCategoryName;
            if (blaze_strcmp(categoryName, "unknowncomponent") == 0)
            {
                // because there can be more than one 'unknown' (see AllocatorManager::initMemGroupNames), use the index instead
                blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, catIdx);
                categoryName = buf;
            }

            for (size_t allocIdx = 0; allocIdx < getCategoryAllocCount(catIdx); allocIdx++)
            {
                const char8_t* subCategoryName = mMemGroupCategories[catIdx].mSubCategories[allocIdx].mSubName;

                mAllocators[catIdx][allocIdx]->initMetrics(Blaze::Metrics::gFrameworkCollection
                    ->getCollection(Blaze::Metrics::Tag::memory_category, categoryName)
                    .getCollection(Blaze::Metrics::Tag::memory_subcategory, subCategoryName)
                    );
            }
        }
    }
#endif //ENABLE_BLAZE_MEM_SYSTEM 
}

void AllocatorManager::performStartupTasks() const
{
}

void AllocatorManager::performShutdownTasks()
{
}

void AllocatorManager::initMemGroupNames()
{
    // Once all components have been registered, initialize the array of memory group names.
    const char8_t* frameworkSubCatNames[] = { "0", "log", "db", "config", "http", "rawbuf", "noleak", "server", "redis", "grpc_core" };
    static_assert(Blaze::MEM_GROUP_FRAMEWORK_ALLOC_COUNT == EAArrayCount(frameworkSubCatNames), "framework alloc count needs to match with frameworkSubCatNames listed above.");
    
    mMemGroupCategories[Blaze::MEM_GROUP_FRAMEWORK_CATEGORY].setInfo("framework", frameworkSubCatNames, Blaze::MEM_GROUP_FRAMEWORK_ALLOC_COUNT);
    const char8_t* defaultSubCatNames[] = { "0" };
    mMemGroupCategories[Blaze::MEM_GROUP_NONCOMPLIANCE_CATEGORY].setInfo("noncompliant", defaultSubCatNames, 1);
    const size_t count = Blaze::BlazeRpcComponentDb::getTotalComponentCount() + Blaze::BlazeRpcComponentDb::getTotalProxyComponentCount();
    for (size_t i = 0; i < count; ++i)
    {
        const Blaze::ComponentBaseInfo* info = Blaze::ComponentBaseInfo::getComponentBaseInfoAtIndex(i);
        if (info != nullptr)
        {
            AllocatorManager::MemGroupCategoryInfo& componentCatInfo = mMemGroupCategories[MEM_GROUP_GET_CATEGORY(info->memgroup)];
            componentCatInfo.setInfo(info->name, info->allocGroupNames, info->allocGroups);
        }
    }

    // Fill in any unknowns:
    for (size_t i = 0; i < count; ++i)
    {
        AllocatorManager::MemGroupCategoryInfo& componentCatInfo = mMemGroupCategories[i + Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY];
        if (componentCatInfo.mCategoryName == nullptr)
        {
            componentCatInfo.setInfo("unknowncomponent", defaultSubCatNames, 1);
        }
    }
}

char8_t* AllocatorManager::getMemoryGroupName(Blaze::MemoryGroupId id, char8_t* buf, size_t bufSize)
{
    const uint16_t category = MEM_GROUP_GET_CATEGORY(id);
    const uint16_t allocId = MEM_GROUP_GET_ALLOC_ID(id);
    if (category < AllocatorManager::getInstance().mCategoryCount && allocId < getCategoryAllocCount(category))
    {
        AllocatorManager::MemGroupCategoryInfo& catInfo = AllocatorManager::getInstance().mMemGroupCategories[category];
        blaze_snzprintf(buf, bufSize, "%s:%s", catInfo.mCategoryName, catInfo.mSubCategories[allocId].mSubName);
    }
    else
    {
        blaze_strnzcpy(buf, "unknownmemgroup", bufSize);
    }
    return buf;
}

size_t AllocatorManager::getCategoryAllocCount(size_t category)
{
    if (category == Blaze::MEM_GROUP_FRAMEWORK_CATEGORY)
        return Blaze::MEM_GROUP_FRAMEWORK_ALLOC_COUNT;

    if (category >= Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY)
    {
        return Blaze::BlazeRpcComponentDb::getAllocGroups(category - Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY);
    }

    return Blaze::MEM_GROUP_DEFAULT_ALLOC_COUNT;
}

bool AllocatorManager::parseMemoryGroupName(const char8_t* mgName, Blaze::MemoryGroupId& id)
{
    const char8_t* subCategoryDelimiter = strchr(mgName, ':');
    // NOTE: We intentionally require that the mgName always have a format: <categoryName>:<subCategoryName>
    // because given that stack tracking is fairly costly, we do not want to allow accidental blanket 
    // stack tracking for a whole category.
    if (subCategoryDelimiter != nullptr)
    {
        const uint16_t categoryCount = (uint16_t) AllocatorManager::getInstance().mCategoryCount;
        AllocatorManager::MemGroupCategoryInfo* categories = AllocatorManager::getInstance().mMemGroupCategories;
        const size_t mgCategoryNameLen = subCategoryDelimiter - mgName;
        const char8_t* mgSubCategoryName = subCategoryDelimiter + 1; // point to start of sub category name
        for (uint16_t i = 0; i < categoryCount; ++i)
        {
            if (blaze_strnicmp(categories[i].mCategoryName, mgName, mgCategoryNameLen) == 0)
            {
                if (mgCategoryNameLen < strlen(categories[i].mCategoryName))
                    continue; // skip partial category name matches
                for (uint16_t j = 0; j < getCategoryAllocCount(i); ++j)
                {
                    if (blaze_stricmp(categories[i].mSubCategories[j].mSubName, mgSubCategoryName) == 0)
                    {
                        id = MEM_GROUP_MAKE_ID(i, j);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


//Allocation init/shutdown hook functions

uint32_t Blaze::AllocatorInitHelper::msRefCount = 0;

Blaze::AllocatorInitHelper::AllocatorInitHelper()
{
    if (msRefCount == 0)
    {
        //Double check that our OS functions are allocated already (this can happen on the first malloc too)
        OSFunctions::initMemoryHooks();

        //perform startup routines
        AllocatorManager::getInstance().performStartupTasks();
    }
    ++msRefCount;
}

Blaze::AllocatorInitHelper::~AllocatorInitHelper()
{
    --msRefCount;

    if (msRefCount == 0)
    {
        //perform shutdown routines
        AllocatorManager::getInstance().performShutdownTasks();
    }
}

void Blaze::AllocatorInitHelper::initMemGroupNames()
{
    AllocatorManager::getInstance().initMemGroupNames();
}

