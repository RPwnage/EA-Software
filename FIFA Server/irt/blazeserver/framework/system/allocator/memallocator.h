/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MEM_ALLOCATOR_H
#define BLAZE_MEM_ALLOCATOR_H

/*** Include files *******************************************************************************/

#include "PPMalloc/EAGeneralAllocator.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/system/allocator/memtracker.h"
#include "framework/metrics/metrics.h"
#if defined(EA_PLATFORM_LINUX)
#include <malloc.h>
#ifdef ENABLE_PTMALLOC3
#include "ptmalloc3/malloc-2.8.3.h"
#endif // ENABLE_PTMALLOC3
#endif


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*! ************************************************************************************************/
/*! \class BlazeAllocatorInterface

    Interface that adds additional debug support to the existing allocator interface.
***************************************************************************************************/
class BlazeAllocatorInterface : public Blaze::Allocator
{
public:
    enum AllocatorType
    {
        ALLOC_PASSTHROUGH,
        ALLOC_PTMALLOC3,
        ALLOC_PPMALLOC,
    };

    virtual AllocatorType getType() const = 0;
    virtual const char8_t* getTypeName() const = 0;
    virtual void getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const = 0;
    virtual void initMetrics(Blaze::Metrics::MetricsCollection& collection) = 0;
};


/*! ************************************************************************************************/
/*! \class InternalAllocator

    A simple wrapper around GeneralAllocator for the allocation systems internal allocations.  This
    allows us to use the convenient CORE_* macros to do allocations with the GeneralAllocator.
***************************************************************************************************/
class InternalAllocator : public Blaze::Allocator
{
    NON_COPYABLE(InternalAllocator);
public:
    InternalAllocator() {}
    ~InternalAllocator() override {}

    void *Alloc(size_t size, const char *name, unsigned int flags) override;
    void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0) override;
    void* Realloc(void *addr, size_t size, unsigned int flags) override;
    void Free(void *block, size_t size=0) override;

protected: 
    EA::Allocator::GeneralAllocator mAllocator;    
};


class CoreManager;
class AllocatorManager;

/*! ************************************************************************************************/
/*! \class BlazeAllocator

    This is the main allocator class used by all allocations outside of the allocator.  This 
    implementation feeds core memory back to the allocator as needed via the CoreManager.
***************************************************************************************************/
class BlazeAllocator : public BlazeAllocatorInterface
{
    NON_COPYABLE(BlazeAllocator);
public:

    BlazeAllocator(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager, EA::Allocator::GeneralAllocator* allocatorToUse = nullptr);
    ~BlazeAllocator() override;

    Blaze::MemoryGroupId getId() const { return mId; }

    void *Alloc(size_t size, const char *name, unsigned int flags) override;
    void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0) override;
    void* Realloc(void *addr, size_t size, unsigned int flags) override;
    void Free(void *block, size_t size=0) override;

    BlazeAllocatorInterface::AllocatorType getType() const override { return BlazeAllocatorInterface::ALLOC_PPMALLOC; }
    const char8_t* getTypeName() const override;
    void getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const override;
    void initMetrics(Blaze::Metrics::MetricsCollection& collection) override;

protected:

    static const size_t CORE_OVERHEAD = 65536; //Overage for core allocations to take into account accounting costs on the core mem itself.

    static bool mallocFailCallback(EA::Allocator::GeneralAllocator* pGeneralAllocator, size_t nMallocRequestedSize, size_t nAllocatorRequestedSize, void* pContext)
    {
        //The system is out of core, lets add some more
        BlazeAllocator *_this = (BlazeAllocator*) pContext;
        return _this->addCore(nMallocRequestedSize);
    }

    bool addCore(size_t requestedSize);

protected:
    Blaze::MemoryGroupId mId;
    CoreManager& mCoreManager;
    InternalAllocator& mInternalAllocator;
    EA::Allocator::GeneralAllocator& mAllocator;
    size_t mCoreMemorySize;

private:
    struct AllocatorMetrics
    {
        AllocatorMetrics(BlazeAllocator& allocator, Blaze::Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mAllocator(allocator)
            , mAllocatedCoreMemory(mMetricsCollection, "memory.allocatedCoreMemory", [this]() { return mAllocator.mCoreMemorySize; })
        {
        }

    private:
        Blaze::Metrics::MetricsCollection& mMetricsCollection;
        BlazeAllocator& mAllocator;

    public:
        Blaze::Metrics::PolledGauge mAllocatedCoreMemory;
    };

    AllocatorMetrics* mMetrics;
};

typedef TrackingAdapter<BlazeAllocator> BlazeAllocatorTracking;

/*! ************************************************************************************************/
/*! \class PassthroughAllocator

    This class effectively turns off allocations by sending them to the default malloc calls.
***************************************************************************************************/
class PassthroughAllocator : public BlazeAllocatorInterface
{
    NON_COPYABLE(PassthroughAllocator);
public:
    PassthroughAllocator(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager)
        : mId (id)
    {}
    void *Alloc(size_t size, const char *name, unsigned int flags) override;
    void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0) override;
    void* Realloc(void *addr, size_t size, unsigned int flags) override;
    void Free(void *block, size_t size=0) override;
    BlazeAllocatorInterface::AllocatorType getType() const override { return BlazeAllocatorInterface::ALLOC_PASSTHROUGH; }
    const char8_t* getTypeName() const override;
    void getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const override {}
    void initMetrics(Blaze::Metrics::MetricsCollection& collection) override {}

protected:
    Blaze::MemoryGroupId mId;
};

typedef TrackingAdapter<PassthroughAllocator> PassthroughAllocatorTracking;

#ifdef ENABLE_PTMALLOC3

/*! ************************************************************************************************/
/*! \class Ptmalloc3Allocator

    This class effectively routes allocations through ptmalloc3 using it's mspaces feature
***************************************************************************************************/
class Ptmalloc3Allocator : public BlazeAllocatorInterface
{
    NON_COPYABLE(Ptmalloc3Allocator);
public:
    Ptmalloc3Allocator(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager);
    ~Ptmalloc3Allocator() override;

    void* addCore(intptr_t requestedSize);
    void *Alloc(size_t size, const char *name, unsigned int flags) override;
    void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0) override;
    void* Realloc(void *addr, size_t size, unsigned int flags) override;
    void Free(void *block, size_t size=0) override;
    BlazeAllocatorInterface::AllocatorType getType() const override { return BlazeAllocatorInterface::ALLOC_PTMALLOC3; }
    const char8_t* getTypeName() const override;
    void getStatus(Blaze::MemoryStatus::AllocatorStatus& status) const override;
    void initMetrics(Blaze::Metrics::MetricsCollection& collection) override;

protected:
    Blaze::MemoryGroupId mId;
    CoreManager& mCoreManager;
    InternalAllocator& mInternalAllocator;
    size_t mCoreMemorySize;
    char8_t* mAllocatorControlBlock;
    mspace mAllocator;
    void* mLastAddCorePtr;

private:
    struct AllocatorMetrics
    {
        AllocatorMetrics(Ptmalloc3Allocator& allocator, Blaze::Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mAllocator(allocator)
            , mAllocatedCoreMemory(mMetricsCollection, "memory.allocatedCoreMemory", [this]() { return mAllocator.mCoreMemorySize; })
            , mUsedMemory(mMetricsCollection, "memory.usedMemory", [this]() { return mspace_used_memory(mAllocator.mAllocator); })
            , mMaxUsedMemory(mMetricsCollection, "memory.maxUsedMemory", [this]() { return mspace_max_used_memory(mAllocator.mAllocator); })
        {
        }

    private:
        Blaze::Metrics::MetricsCollection& mMetricsCollection;
        Ptmalloc3Allocator& mAllocator;

    public:
        Blaze::Metrics::PolledGauge mAllocatedCoreMemory;
        Blaze::Metrics::PolledGauge mUsedMemory;
        Blaze::Metrics::PolledGauge mMaxUsedMemory;
    };

    AllocatorMetrics* mMetrics;
};

typedef TrackingAdapter<Ptmalloc3Allocator> Ptmalloc3AllocatorTracking;

#endif // ENABLE_PTMALLOC3


#endif // BLAZE_MEM_ALLOCATOR_H

