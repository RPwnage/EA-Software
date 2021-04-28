/*************************************************************************************************/
/*! \file

    This file has basic functions/macros used by the custom Blaze allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_FRAMEWORK_ALLOCATION_H
#define BLAZE_FRAMEWORK_ALLOCATION_H

#include "EABase/eabase.h"
#include "coreallocator/icoreallocator_interface.h"

//If this is a component type library, define a type reference and link it back
#ifdef BLAZE_COMPONENT_TYPE_INDEX_NAME
namespace Blaze
{
    extern const size_t& BLAZE_COMPONENT_TYPE_INDEX_NAME;    // Even though this isn't used by the allocator, it's externed here since other systems assumed it was.
}
#endif

namespace Blaze
{
    typedef uint16_t MemoryGroupId;
}

// Example: BLAZE_COMPONENT_MEMGROUP_NAME=COMPONENT_MEMGROUP_arson
#ifdef BLAZE_COMPONENT_MEMGROUP_NAME
namespace Blaze 
{
    extern const MemoryGroupId& BLAZE_COMPONENT_MEMGROUP_NAME;
}
#define DEFAULT_BLAZE_COMPONENT_MEMGROUP ::Blaze::BLAZE_COMPONENT_MEMGROUP_NAME
#endif

// Convert the category index into the memgroup BlazeRpcComponentDb::getMemGroupCategoryIndex()
#define MEM_GROUP_MAKE_ID(_categoryIndex, _allocIdx) ((_categoryIndex << ::Blaze::MEM_GROUP_CATEGORY_TYPE_SHIFT) | _allocIdx )
#define MEM_GROUP_FROM_COMPONENT_CATEGORY(_componentCategoryIndex) ( MEM_GROUP_MAKE_ID(((::Blaze::MemoryGroupId) _componentCategoryIndex + ::Blaze::MEM_GROUP_FIRST_COMPONENT_CATEGORY), 0))

// Combine the component's memgroup, and the allocation index used. (This doesn't actually create the subgroup itself, that's done in the rpc file).
#define MEM_GROUP_GET_SUB_ALLOC_GROUP(_componentMemGroup, _allocIdx) ((::Blaze::MemoryGroupId)((_componentMemGroup) | _allocIdx) )

#define MEM_GROUP_GET_CATEGORY(_memGroupId) ((_memGroupId & ::Blaze::MEM_GROUP_CATEGORY_TYPE_MASK) >> ::Blaze::MEM_GROUP_CATEGORY_TYPE_SHIFT)
#define MEM_GROUP_GET_ALLOC_ID(_memGroupId) (_memGroupId & ::Blaze::MEM_GROUP_ALLOC_ID_MASK)

#ifdef DEFAULT_BLAZE_COMPONENT_MEMGROUP
#define DEFAULT_BLAZE_MEMGROUP DEFAULT_BLAZE_COMPONENT_MEMGROUP
#elif defined(COMPONENT_framework)
#define DEFAULT_BLAZE_MEMGROUP ::Blaze::MEM_GROUP_FRAMEWORK_DEFAULT
#elif defined(COMPONENT_server)
#define DEFAULT_BLAZE_MEMGROUP ::Blaze::MEM_GROUP_FRAMEWORK_SERVER
#else
#define DEFAULT_BLAZE_MEMGROUP ::Blaze::MEM_GROUP_NONCOMPLIANCE_DEFAULT
#endif

//This defines whether the memory system is on or off.  If off, the BLAZE_NEW macros default to using the standard
//underlying os counterparts, and things like the malloc hooks and global new override are not defined.
#ifndef ENABLE_BLAZE_MEM_SYSTEM
#define ENABLE_BLAZE_MEM_SYSTEM 1
#endif

//Make sure that without the system turned on linux files get malloc
#if !ENABLE_BLAZE_MEM_SYSTEM && defined(EA_PLATFORM_LINUX)
#include <malloc.h>
#endif

//Our interface defines
#define MEM_NAME(moduleName, allocationName) allocationName
#define DEFAULT_ALLOC_FLAGS EA::Allocator::MEM_TEMP

#if ENABLE_BLAZE_MEM_SYSTEM
#define BLAZE_NEW_MGID(_memModuleId, _allocName) \
    EA::TDF::TdfObjectAllocHelper() << CORE_NEW(::Blaze::Allocator::getAllocator(_memModuleId), MEM_NAME(_memModuleId, _allocName), DEFAULT_ALLOC_FLAGS)

#define BLAZE_NEW_ALIGN_MGID(_align, _memModuleId, _allocName) \
    CORE_NEW_ALIGN(::Blaze::Allocator::getAllocator(_memModuleId), ::Blaze::Allocator::getAllocator(_memModuleId), MEM_NAME(_memModuleId, _allocName), DEFAULT_ALLOC_FLAGS, _align)

//For the array stuff we use a regular operator overload rather than the CORE_NEW macros, as the core new macros
//use template classes that screw us up when we try to use a regular delete[] operator
#define BLAZE_NEW_ARRAY_MGID(_objtype, _count, _memModuleId, _allocName) \
    new (_memModuleId, _allocName) _objtype[_count]

#define BLAZE_NEW_ARRAY_ALIGN_MGID(_objtype, _count, _align, _memModuleId, _allocName) \
    new (_memModuleId, _allocName, _align, 0) _objtype[_count]

//The following functions are replacements for malloc, realloc, memalign, and free
#define BLAZE_ALLOC_MGID(_size, _memModuleId, _allocName) \
    CORE_ALLOC(::Blaze::Allocator::getAllocator(_memModuleId), _size, MEM_NAME(_memModuleId, _allocName), DEFAULT_ALLOC_FLAGS)

#define BLAZE_ALLOC_ALIGN_MGID( _size, _align, _alignoffset, _memModuleId, _allocName)  \
    CORE_ALLOC_ALIGN(::Blaze::Allocator::getAllocator(_memModuleId), _size, MEM_NAME(_memModuleId, _allocName), DEFAULT_ALLOC_FLAGS, _align, _alignoffset) 

//realloc(nullptr, _size) needs to call alloc(_size).  We only call getAllocatorByPtr() when the address is set, and we can get the allocator
#define BLAZE_REALLOC_MGID(_addr, _size, _memModuleId, _allocName) \
    (_addr ? ::Blaze::Allocator::getAllocatorByPtr(_addr)->Realloc(_addr, _size, DEFAULT_ALLOC_FLAGS) : BLAZE_ALLOC_MGID(_size, _memModuleId, _allocName))

#define BLAZE_FREE(_block) \
    CORE_FREE(::Blaze::Allocator::getAllocatorByPtr((void*) _block), (void*) _block) 

#define BLAZE_FREE_SIZE(_block, _size)  \
    CORE_FREE_SIZE(::Blaze::Allocator::getAllocatorByPtr((void*) _block), (void*) _block, _size) 

#else

//These are passthroughs to the underlying OS alternatives
#define BLAZE_NEW_MGID(_memModuleId, _allocName) \
    EA::TDF::TdfObjectAllocHelper() << new

#define BLAZE_NEW_ALIGN_MGID(_align, _memModuleId, _allocName) new
#define BLAZE_NEW_ARRAY_MGID(_objtype, _count, _memModuleId, _allocName) new _objtype[_count]
#define BLAZE_NEW_ARRAY_ALIGN_MGID(_objtype, _count, _align, _memModuleId, _allocName) new _objtype[_count]
#define BLAZE_ALLOC_MGID(_size, _memModuleId, _allocName) malloc(_size)

#ifdef EA_PLATFORM_LINUX
#define BLAZE_ALLOC_ALIGN_MGID( _size, _align, _alignoffset, _memModuleId, _allocName)  memalign(_align, _size)
#else
#define BLAZE_ALLOC_ALIGN_MGID( _size, _align, _alignoffset, _memModuleId, _allocName)  malloc(_size)
#endif

#define BLAZE_REALLOC_MGID(_addr, _size, _memModuleId, _allocName) realloc((void*) _addr, _size)
#define BLAZE_FREE(_block) free((void*) _block)
#define BLAZE_FREE_SIZE(_block, _size)  free((void*) _block)

#endif

//These allocation defines are different on the client and the server
#define BLAZE_DELETE_MGID(_mg, _ptr) delete _ptr
#define BLAZE_FREE_MGID(_mg, _ptr) BLAZE_FREE(_ptr)

#define BLAZE_UNNAMED_ALLOC_NAME "UNNAMED"

#ifndef BLAZE_ALLOC_NO_FILE_AND_LINE
#define BLAZE_DEFAULT_ALLOC_NAME __FILEANDLINE__
#else
#define BLAZE_DEFAULT_ALLOC_NAME BLAZE_UNNAMED_ALLOC_NAME
#endif

//These are defined if there is a default memgroup type present - otherwise you are forced to use the "MGID" format
#if defined(DEFAULT_BLAZE_MEMGROUP)
#define BLAZE_NEW BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_NAMED(_allocName) BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, _allocName)

#define BLAZE_NEW_ALIGN(_align) BLAZE_NEW_ALIGN_MGID(DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME, _align)
#define BLAZE_NEW_ALIGN_NAMED(_align, _allocName) BLAZE_NEW_ALIGN_MGID(DEFAULT_BLAZE_MEMGROUP, _allocName, _align)

#define BLAZE_NEW_ARRAY(_objtype, _count)  BLAZE_NEW_ARRAY_MGID(_objtype, _count, DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_ARRAY_NAMED(_objtype, _count, _allocName)  BLAZE_NEW_ARRAY_MGID(_objtype, _count, DEFAULT_BLAZE_MEMGROUP, _allocName)

#define BLAZE_NEW_ARRAY_ALIGN(_objtype, _count, _align)  BLAZE_NEW_ARRAY_ALIGN_MGID(_objtype, _count, _align, DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_ARRAY_ALIGN_NAMED(_objtype, _count, _align, _allocName)  BLAZE_NEW_ARRAY_ALIGN_MGID(_objtype, _count, _align, DEFAULT_BLAZE_MEMGROUP, _allocName)

#define BLAZE_ALLOC(_size) BLAZE_ALLOC_MGID(_size, DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_ALLOC_NAMED(_size, _allocName) BLAZE_ALLOC_MGID(_size, DEFAULT_BLAZE_MEMGROUP, _allocName)

#define BLAZE_ALLOC_ALIGN( _size, _align, _alignoffset)   BLAZE_ALLOC_ALIGN_MGID(_size, _align, _alignoffset, DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_ALLOC_ALIGN_NAMED( _size, _allocName, _align, _alignoffset)   BLAZE_ALLOC_ALIGN_MGID(_size, _align, _alignoffset, DEFAULT_BLAZE_MEMGROUP, _allocName)

#define BLAZE_REALLOC(_addr, _size) BLAZE_REALLOC_MGID(_addr, _size, DEFAULT_BLAZE_MEMGROUP, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_REALLOC_NAMED(_addr, _size, _allocName) BLAZE_REALLOC_MGID(_addr, _size, DEFAULT_BLAZE_MEMGROUP, _allocName)
#endif

//Some nice helpers for framework category
#ifdef COMPONENT_framework
#define BLAZE_NEW_LOG BLAZE_NEW_MGID(::Blaze::MEM_GROUP_FRAMEWORK_LOG, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_DB BLAZE_NEW_MGID(::Blaze::MEM_GROUP_FRAMEWORK_DB, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_CONFIG BLAZE_NEW_MGID(::Blaze::MEM_GROUP_FRAMEWORK_CONFIG, BLAZE_DEFAULT_ALLOC_NAME)
#define BLAZE_NEW_HTTP BLAZE_NEW_MGID(::Blaze::MEM_GROUP_FRAMEWORK_HTTP, BLAZE_DEFAULT_ALLOC_NAME)
#endif

#define BLAZE_PUSH_ALLOC_OVERRIDE(_id, _name) ::Blaze::Allocator::setNonCompliantDefaults(_id, _name)
#define BLAZE_POP_ALLOC_OVERRIDE()  ::Blaze::Allocator::setNonCompliantDefaults(::Blaze::MEM_GROUP_NONCOMPLIANCE_DEFAULT, BLAZE_UNNAMED_ALLOC_NAME)

namespace Blaze
{

class MemoryStatus;
class ConfigureMemoryMetricsRequest;
class ConfigureMemoryMetricsResponse;
class GetMemoryMetricsRequest;
class GetMemoryMetricsResponse;

//Typedefs, constants and macros for the MemoryGroupId interface
static const MemoryGroupId INVALID_MEMORY_GROUP_ID = UINT16_MAX;


//Our two special categories - one for framework, one for noncompliance
const MemoryGroupId MEM_GROUP_FRAMEWORK_CATEGORY = 0;
const MemoryGroupId MEM_GROUP_NONCOMPLIANCE_CATEGORY = 1;
const MemoryGroupId MEM_GROUP_FIRST_COMPONENT_CATEGORY = 2;
const MemoryGroupId MEM_GROUP_CATEGORY_TYPE_MASK = 0x7F00;

const MemoryGroupId MEM_GROUP_NONE = 0xFFFF; //constant for saying "no mem group in particular"


const MemoryGroupId MEM_GROUP_DEFAULT_ALLOC_COUNT = 1; // although one component can have up to 16 allocators (given 4-bits), only Framework uses more than 1, so we just special case framework.
const MemoryGroupId MEM_GROUP_MAX_ALLOC_COUNT = 16;    // 16 is the max 
const MemoryGroupId MEM_GROUP_ALLOC_ID_MASK = 0xF;     // 4-bits to represent the allocator index for a particular component
const MemoryGroupId MEM_GROUP_CATEGORY_TYPE_SHIFT = 8;

const MemoryGroupId MEM_GROUP_NONCOMPLIANCE_DEFAULT = MEM_GROUP_MAKE_ID(MEM_GROUP_NONCOMPLIANCE_CATEGORY, 0);

const MemoryGroupId MEM_GROUP_FRAMEWORK_DEFAULT     = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 0);
const MemoryGroupId MEM_GROUP_FRAMEWORK_LOG         = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 1);
const MemoryGroupId MEM_GROUP_FRAMEWORK_DB          = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 2);
const MemoryGroupId MEM_GROUP_FRAMEWORK_CONFIG      = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 3);
const MemoryGroupId MEM_GROUP_FRAMEWORK_HTTP        = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 4);
const MemoryGroupId MEM_GROUP_FRAMEWORK_RAWBUF      = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 5);
const MemoryGroupId MEM_GROUP_FRAMEWORK_NOLEAK      = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 6);
const MemoryGroupId MEM_GROUP_FRAMEWORK_SERVER      = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 7);
const MemoryGroupId MEM_GROUP_FRAMEWORK_REDIS       = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 8); 
const MemoryGroupId MEM_GROUP_FRAMEWORK_GRPC_CORE   = MEM_GROUP_MAKE_ID(MEM_GROUP_FRAMEWORK_CATEGORY, 9); // This list should match with the category names in AllocatorManager::initMemGroupNames():
const MemoryGroupId MEM_GROUP_FRAMEWORK_ALLOC_COUNT = 10;                                            // How many alloc groups does Framework use  (Ideally, this would be part of the component information built from the RPC file)
const MemoryGroupId MEM_GROUP_FRAMEWORK_LAST_ID     = MEM_GROUP_FRAMEWORK_GRPC_CORE;

static_assert(MEM_GROUP_GET_ALLOC_ID(MEM_GROUP_FRAMEWORK_LAST_ID) == (MEM_GROUP_FRAMEWORK_ALLOC_COUNT-1), "framework alloc count needs to match with allocs used");

class BlazeStlAllocator
{
public:
    // To do: Make this constructor explicit, when there is no known code dependent on it being otherwise.
    BlazeStlAllocator(const char* pName, EA::Allocator::ICoreAllocator* allocator);

    BlazeStlAllocator(const char* pName = "BlazeStlAllocator", MemoryGroupId memId = DEFAULT_BLAZE_MEMGROUP);

    BlazeStlAllocator(const BlazeStlAllocator& x);
    BlazeStlAllocator(const BlazeStlAllocator& x, const char* name);

    BlazeStlAllocator& operator=(const BlazeStlAllocator& x);

    void* allocate(size_t n, int flags = 0);
    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
    void  deallocate(void* p, size_t n);

    EA::Allocator::ICoreAllocator* get_allocator() const;
    void set_allocator(EA::Allocator::ICoreAllocator* pAllocator);

    int  get_flags() const;
    void set_flags(int flags);

    const char* get_name() const;
    void set_name(const char* pName);

public: // Public because otherwise VC++ generates (possibly invalid) warnings about inline friend template specializations.
    EA::Allocator::ICoreAllocator* mpCoreAllocator;
    int mnFlags;    // Allocation flags. See ICoreAllocator/AllocFlags.

    const char* mpName; // Debug name, used to track memory.
};

inline BlazeStlAllocator* getDefaultBlazeStlAllocatorInstance() 
{ 
    static BlazeStlAllocator gDefaultStlAllocator("DefaultSTLAllocator", MEM_GROUP_FRAMEWORK_DEFAULT);
    return &gDefaultStlAllocator;
}


/*! ************************************************************************************************/
/*! \class Allocator

   The primary Blaze allocator interface.  This borrows from ICoreAllocator, but also adds 
   functionality for Realloc which ICoreAllocator does not have.

***************************************************************************************************/
class Allocator : public EA::Allocator::ICoreAllocator
{
public:    
    //Naming slightly different to be homogeneous with ICoreAllocator
    virtual void *Realloc(void* addr, size_t size, unsigned int flags) = 0;    

public:
    //Gets an allocator corresponding to the given memGroupid
    static Allocator* getAllocator(MemoryGroupId memGroupId = DEFAULT_BLAZE_MEMGROUP);

    //Gets the allocator that previously allocated the memory at addr
    static Allocator* getAllocatorByPtr(void* addr);

    static void getStatus(MemoryStatus& metrics);
    static const char8_t* getTypeName();

    static void processConfigureMemoryMetrics(const ConfigureMemoryMetricsRequest &request, ConfigureMemoryMetricsResponse &response);
    static void processGetMemoryMetrics(const GetMemoryMetricsRequest &request, GetMemoryMetricsResponse &response);
    static void setNonCompliantDefaults(MemoryGroupId id, const char8_t* name);

    static void getProcessMemUsage(uint64_t& size, uint64_t& resident);
    static void getSystemMemorySize(uint64_t& totalMem, uint64_t& freeMem);
};

/*! ************************************************************************************************/
/*! \class AllocatorInitHelper

   This class uses an initialization trick to hook in to the static initialization startup and 
   shutdown methods.  A module level static is defined for each compilation unit.  The constructor
   and destructor of this allocator increment/decrement a global refcount.  When at zero, this ref count
   will trigger any startup/shutdown functionality the internal allocator might have, such as leak
   checking and memory cleanup.
***************************************************************************************************/
class AllocatorInitHelper
{
public:
    AllocatorInitHelper();
    ~AllocatorInitHelper();
    static void initMemGroupNames();

private:
    static uint32_t msRefCount;
};

//One of these instances is declared for each compilation unit
static AllocatorInitHelper msAllocInitHelperInstance;

extern uint32_t Log2(uint64_t x);

inline BlazeStlAllocator::BlazeStlAllocator(const char* pName, EA::Allocator::ICoreAllocator* allocator)
    : mpCoreAllocator(allocator),
      mnFlags(0),
      mpName(pName)
{
}

inline BlazeStlAllocator::BlazeStlAllocator(const char* pName, MemoryGroupId memId)
    : mpCoreAllocator(Allocator::getAllocator(memId)),
      mnFlags(0),
      mpName(pName)
{
}

inline BlazeStlAllocator::BlazeStlAllocator(const BlazeStlAllocator& x)
    : mpCoreAllocator(x.mpCoreAllocator),
      mnFlags(x.mnFlags),
      mpName(x.mpName)
{
}

inline BlazeStlAllocator::BlazeStlAllocator(const BlazeStlAllocator& x, const char* name)
    : mpCoreAllocator(x.mpCoreAllocator),
      mnFlags(x.mnFlags),
      mpName(name)
{
}

inline BlazeStlAllocator& BlazeStlAllocator::operator=(const BlazeStlAllocator& x)
{
    // In order to be consistent with EASTL's allocator implementation, 
    // we don't copy the name from the source object.
    mpCoreAllocator = x.mpCoreAllocator;
    mnFlags         = x.mnFlags;
    return *this;
}

inline void* BlazeStlAllocator::allocate(size_t n, int /*flags*/)
{
    // It turns out that EASTL itself doesn't use the flags parameter, 
    // whereas the user here might well want to specify a flags 
    // parameter. So we use ours instead of the one passed in.
    return mpCoreAllocator->Alloc(n, mpName, (uint32_t)mnFlags);
}

inline void* BlazeStlAllocator::allocate(size_t n, size_t alignment, size_t offset, int /*flags*/)
{
    // It turns out that EASTL itself doesn't use the flags parameter, 
    // whereas the user here might well want to specify a flags 
    // parameter. So we use ours instead of the one passed in.
    return mpCoreAllocator->Alloc(n, mpName, (uint32_t)mnFlags, (uint32_t)alignment, (uint32_t)offset);
}

inline void BlazeStlAllocator::deallocate(void* p, size_t n)
{
    return mpCoreAllocator->Free(p, n);
}

inline EA::Allocator::ICoreAllocator* BlazeStlAllocator::get_allocator() const
{
    return mpCoreAllocator;
}

inline void BlazeStlAllocator::set_allocator(EA::Allocator::ICoreAllocator* pAllocator)
{
    mpCoreAllocator = pAllocator;
}

inline int BlazeStlAllocator::get_flags() const
{
    return mnFlags;
}

inline void BlazeStlAllocator::set_flags(int flags)
{
    mnFlags = flags;
}

inline const char* BlazeStlAllocator::get_name() const
{
    return mpName;
}

inline void BlazeStlAllocator::set_name(const char* pName)
{
    mpName = pName;
}

inline bool operator==(const BlazeStlAllocator& a, const BlazeStlAllocator& b)
{
    return (a.mpCoreAllocator == b.mpCoreAllocator) &&
           (a.mnFlags         == b.mnFlags);
}

inline bool operator!=(const BlazeStlAllocator& a, const BlazeStlAllocator& b)
{
    return (a.mpCoreAllocator != b.mpCoreAllocator) ||
           (a.mnFlags         != b.mnFlags);
}


} //namespace Blaze


//These operators define our helpers to make the array new/delete operate correctly
void* operator new[](size_t size, ::Blaze::MemoryGroupId x, const char8_t *allocName);
void operator delete[](void*, ::Blaze::MemoryGroupId x, const char8_t *allocName);
void* operator new[](size_t size, ::Blaze::MemoryGroupId x, const char8_t *allocName, uint32_t align, uint32_t alignOffset);
void operator delete[](void*, ::Blaze::MemoryGroupId x, const char8_t *allocName, uint32_t align, uint32_t alignOffset);


//These defines set us as the default allocator, as well as ensuring there's a default setting for all allocator types
#define EASTLAllocatorType ::Blaze::BlazeStlAllocator
#define EASTLAllocatorDefault ::Blaze::getDefaultBlazeStlAllocatorInstance

#endif // BLAZE_FRAMEWORK_ALLOCATION_H
