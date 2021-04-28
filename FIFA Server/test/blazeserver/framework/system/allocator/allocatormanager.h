/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ALLOCATOR_MANAGER_H
#define BLAZE_ALLOCATOR_MANAGER_H

/*** Include files *******************************************************************************/
#include "framework/system/allocator/memallocator.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*! ************************************************************************************************/
/*! \class AllocatorManager

    This is the prime singleton that tracks all allocators and core managers.  Any allocation needs
    to go through this manager, and its lazily initialized from the first allocation to ensure that 
    nothing is allocated outside of this class.  All memory is initialized at construction time or 
    statically with the class to ensure the allocator is static and thus threadsafe after construction
***************************************************************************************************/
class AllocatorManager
{
public:

    /*! ************************************************************************************************/
    /*! \brief Gets the singleton instance

        The actual instance is done as a function static to avoid order of operation initialization 
        issues.
    ***************************************************************************************************/
    static AllocatorManager& getInstance();

    AllocatorManager();    
    ~AllocatorManager();

    InternalAllocator& getInternalAllocator() { return mInternalAllocator; }
    GlobalMemTracker& getGlobalMemTracker() { return mGlobalMemTracker; }

    /*! ************************************************************************************************/
    /*! \brief Gets an allocator representing the given memory group id.
    ***************************************************************************************************/
    BlazeAllocatorInterface* getAllocator(Blaze::MemoryGroupId id) const
    {
        //Determine where an id should come from
        return mAllocators[MEM_GROUP_GET_CATEGORY(id)][MEM_GROUP_GET_ALLOC_ID(id)];
    }

    /*! ************************************************************************************************/
    /*! \brief Gets whether or not we use the passthrough allocator
    ***************************************************************************************************/
    bool isUsingPassthroughAllocator() const { return mAllocatorType == BlazeAllocatorInterface::ALLOC_PASSTHROUGH; }

    /*! ************************************************************************************************/
    /*! \brief Gets the allocator that manages the given address
    ***************************************************************************************************/
    Blaze::Allocator* getAllocatorByPtr(void* addr) const
    {        
        //If we have multiple core managers we can get the allocator directly from our 
        //top level table based on the address range.  Otherwise, dive into the first core manager and pull the allocator
        //directly from there
        if (!isUsingPassthroughAllocator())
        {
            // This tells us the address index, but we don't track the [coreIndex] -> [category][alloc] mapping
            uint64_t coreIndex = ((uint64_t) ((intptr_t) ((char8_t*) addr - (char8_t*) mStartAddress))) >> mAddressShift;

            // Manually check each category: 
            size_t catIdx = 0;
            for (; catIdx < mCategoryCount; catIdx++)
            {
                if (coreIndex >= getCategoryAllocCount(catIdx))
                    coreIndex -= getCategoryAllocCount(catIdx);
                else
                    break;
            }

            if (catIdx == mCategoryCount)
                return nullptr;

            return mAllocators[catIdx][coreIndex];
        }
        else
        {
            //Pass through allocators are all the same, just return the first one
            return **mAllocators;
        }
    }

    void getStatus(Blaze::MemoryStatus& status) const;
    const char8_t* getTypeName() const;

    void initMetrics();
    void performStartupTasks() const;
    void performShutdownTasks();
    void initMemGroupNames();

    static char8_t* getMemoryGroupName(Blaze::MemoryGroupId id, char8_t* buf, size_t bufSize);
    static bool parseMemoryGroupName(const char8_t* name, Blaze::MemoryGroupId& id);
    static size_t getCategoryAllocCount(size_t category);

private:
    static const uint32_t MEM_GROUP_SUB_NAME_MAX = 16;
    
    BlazeAllocatorInterface* createAllocator(Blaze::MemoryGroupId id, CoreManager* coreManager, bool memTracking);
    void initializeCoreManagers();
    void shutdownCoreManagers() const;

    struct MemGroupSubCategoryInfo
    {
        char8_t mSubName[MEM_GROUP_SUB_NAME_MAX];
    };

    struct MemGroupCategoryInfo
    {
        const char8_t* mCategoryName;
        MemGroupSubCategoryInfo mSubCategories[Blaze::MEM_GROUP_MAX_ALLOC_COUNT]; // MEM_GROUP_FRAMEWORK_ALLOC_COUNT > MEM_GROUP_MAX_ALLOC_COUNT
        void setInfo(const char8_t* categoryName, const char8_t** allocGroupNames, size_t allocCount)
        {
            mCategoryName = categoryName;
            for (uint32_t i = 0; i < Blaze::MEM_GROUP_FRAMEWORK_ALLOC_COUNT; ++i)
            {
                if (i < allocCount)
                {
                    blaze_strnzcpy(mSubCategories[i].mSubName, allocGroupNames[i], sizeof(mSubCategories[i].mSubName));
                }
                else
                    blaze_snzprintf(mSubCategories[i].mSubName, sizeof(mSubCategories[i].mSubName), "%u", i);
            }
        }
    };

private:
    InternalAllocator mInternalAllocator;
    MemGroupCategoryInfo* mMemGroupCategories;
    BlazeAllocatorInterface*** mAllocators;         // mAllocators[Category][Group]
    CoreManager** mCoreManagers;
    size_t mCategoryCount;
    size_t mCoreManagerCount;
    void *mStartAddress;
    uint32_t mAddressShift;
    BlazeAllocatorInterface::AllocatorType mAllocatorType;
    const char8_t* mAllocTrackSettings;
    GlobalMemTracker mGlobalMemTracker;

    static AllocatorManager* msInstance; //Redundant with the static function, but this one is globally accessible from a debugger     
};


#endif // BLAZE_ALLOCATOR_H

