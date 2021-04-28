/*! ***************************************************************************/
/*!
    \file allocdefines.h

    Internal include file for blaze allocation macros.  The macros in this file are
    defined for internal Blaze use and remap the CoreAllocator macros to point
    to the Blaze allocator (defined in alloc.h).  These should generally not be 
    used by SDK users unless you have a need to allocate into the Blaze heap.


    \attention
    (c) Electronic Arts. All Rights Reserved.

*******************************************************************************/

#ifndef BLAZE_SDK_ALLOC_DEFINES_H
#define BLAZE_SDK_ALLOC_DEFINES_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "coreallocator/icoreallocatormacros.h"
#include "BlazeSDK/alloc.h"

//Blaze specific defines for coreallocator macros that pass in the allocator.

#ifdef EA_DEBUG
#define MEM_NAME(_memModuleId, _name) Blaze::Allocator::buildDebugNameString(_memModuleId, _name)
#else
#define MEM_NAME(_memModuleId, _name) _name
#endif 

//ifndef the Blaze Alloc/Free Macros in case titles override with their own customized implementation.
#ifndef BLAZE_NEW
#define BLAZE_NEW(_memModuleId, _allocName) \
    EA::TDF::TdfObjectAllocHelper() << CORE_NEW(Blaze::Allocator::getAllocator(_memModuleId), MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM))
#endif

#ifndef BLAZE_NEW_ALLOCATOR
#define BLAZE_NEW_ALLOCATOR(_allocator, _allocName) \
    EA::TDF::TdfObjectAllocHelper() << CORE_NEW(_allocator, MEM_NAME(Blaze::Allocator::getMemGroupId(_allocator), _allocName), EA::Allocator::MEM_PERM)
#endif

#ifndef BLAZE_NEW_ALLOCATOR_FLAGS
#define BLAZE_NEW_ALLOCATOR_FLAGS(_allocator, _allocName, _flags) \
    EA::TDF::TdfObjectAllocHelper() << CORE_NEW(_allocator, MEM_NAME(Blaze::Allocator::getMemGroupId(_allocator), _allocName), _flags)
#endif

#ifndef BLAZE_NEW_ALIGN
#define BLAZE_NEW_ALIGN(_memModuleId, _allocName, _align) \
    EA::TDF::TdfObjectAllocHelper() << CORE_NEW_ALIGN(Blaze::Allocator::getAllocator(_memModuleId), Blaze::Allocator::getAllocator(_memModuleId), MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM), _align)
#endif

#ifndef BLAZE_DELETE
#define BLAZE_DELETE(_memModuleId,_ptr) \
    CORE_DELETE(Blaze::Allocator::getAllocator(_memModuleId), _ptr)
#endif

#ifndef BLAZE_DELETE_ALLOCATOR
#define BLAZE_DELETE_ALLOCATOR(_allocator,_ptr) \
    CORE_DELETE(_allocator, _ptr)
#endif

#ifndef BLAZE_DELETE_PRIVATE
#define BLAZE_DELETE_PRIVATE(_memModuleId,_type, _ptr) \
    CORE_DELETE_PRIVATE(Blaze::Allocator::getAllocator(_memModuleId), _type, _ptr)
#endif

#ifndef BLAZE_DELETE_PRIVATE_WITHGROUP
#define BLAZE_DELETE_PRIVATE_WITHGROUP(_memModuleId, _type, _ptr) \
    CORE_DELETE_PRIVATE(Blaze::Allocator::getAllocator(_memModuleId), _type, _ptr)
#endif

#ifndef BLAZE_NEW_ARRAY
#define BLAZE_NEW_ARRAY(_objtype, _count, _memModuleId, _allocName)   \
    CORE_NEW_ARRAY(Blaze::Allocator::getAllocator(_memModuleId), _objtype, _count, MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM))   
#endif

#ifndef BLAZE_NEW_ARRAY_ALIGN
#define BLAZE_NEW_ARRAY_ALIGN(_objtype, _count, _memModuleId, _allocName, _align) \
    CORE_NEW_ARRAY_ALIGN(Blaze::Allocator::getAllocator(_memModuleId), _objtype, _count, MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM), _align) 
#endif

#ifndef BLAZE_DELETE_ARRAY
#define BLAZE_DELETE_ARRAY(_memModuleId,_ptr)  \
    CORE_DELETE_ARRAY(Blaze::Allocator::getAllocator(_memModuleId), _ptr) 
#endif

#ifndef BLAZE_ALLOC
#define BLAZE_ALLOC(_size, _memModuleId, _allocName) \
    CORE_ALLOC(Blaze::Allocator::getAllocator(_memModuleId), _size, MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM)) 
#endif

#ifndef BLAZE_ALLOC_ALIGN
#define BLAZE_ALLOC_ALIGN( _size, _memModuleId, _allocName, _align, _alignoffset)  \
    CORE_ALLOC_ALIGN(Blaze::Allocator::getAllocator(_memModuleId), _size, MEM_NAME(_memModuleId, _allocName), \
        ((_memModuleId & Blaze::MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM), _align, _alignoffset) 
#endif

#ifndef BLAZE_FREE
#define BLAZE_FREE(_memModuleId,_block) \
    CORE_FREE(Blaze::Allocator::getAllocator(_memModuleId), _block) 
#endif

#ifndef BLAZE_FREE_SIZE
#define BLAZE_FREE_SIZE(_memModuleId,_block, _size)  \
    CORE_FREE_SIZE(Blaze::Allocator::getAllocator(_memModuleId), _block, _size) 
#endif

//These allocation defines are different on the client and the server
#define BLAZE_DELETE_MGID(_mg, _ptr) BLAZE_DELETE(_mg, _ptr)
#define BLAZE_FREE_MGID(_mg, _ptr) BLAZE_FREE(_mg, _ptr)

#endif    // BLAZE_SDK_ALLOCDEFINES_H

