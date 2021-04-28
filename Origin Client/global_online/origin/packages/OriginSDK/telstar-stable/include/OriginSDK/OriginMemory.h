#ifndef __ORIGIN_MEMORY_H__
#define __ORIGIN_MEMORY_H__

#include "Origin.h"
#include "OriginTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup mem Memory
/// \brief This module contains the typedefs and functions that allow Origin SDK integrators to provide their own memory
/// management framework.

/// \ingroup mem
/// \brief A function pointer declaration for 'normal' memory allocations.
/// 
/// \param size The amount of memory to allocate.
/// \return The allocated memory block or NULL.
typedef void * (*OriginAllocFunc)(size_t size);


/// \ingroup mem
/// \brief A function pointer declaration for aligned memory allocations.
/// 
/// \param size The amount of memory to allocate.
/// \param alignment The alignment you want the memory block to have.
/// \return The allocated aligned memory block or NULL.
typedef void * (*OriginAllocAlignedFunc)(size_t size, size_t alignment);


/// \ingroup mem
/// \brief Frees the 'normal' memory block.
/// 
/// \param pMemory The memory block to be freed.
typedef void (*OriginFreeFunc)(void *pMemory);


/// \ingroup mem
/// \brief Frees the aligned memory block.
/// 
/// \param pMemory The aligned memory block to be freed.
typedef void (*OriginFreeAlignedFunc)(void *pMemory);

/// \ingroup mem
/// \brief Allows an Origin SDK user to provide their own memory management framework.
/// 
/// In order to get a handle on the memory allocations made by the OriginSDK one can provide a couple of functions to
/// hook the Origin memory allocations into the games memory system.
/// \param alloc [optional] Function for allocating normal memory blocks.
/// \param free [optional] Function for freeing normal memory blocks.
/// \param alloc_align [optional] Function for allocating aligned memory blocks.
/// \param free_align [optional] Function for freeing aligned memory blocks.
/// \return \ref ORIGIN_SUCCESS if the new memory framework is applied.
/// <br>\ref ORIGIN_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION if no proper alloc, dealloc combination is provided.
/// \note This function can only be called when the SDK is not running; otherwise, an \ref ORIGIN_ERROR_SDK_IS_RUNNING error
/// will be returned.
OriginErrorT ORIGIN_SDK_API OriginSetupMemory(
	OriginAllocFunc alloc, 
	OriginFreeFunc free, 
	OriginAllocAlignedFunc alloc_align, 
	OriginFreeAlignedFunc free_align);


/// \ingroup mem
/// \brief [Optional] Disconnects your supplied memory framework from the OriginSDK.
/// 
/// \return \ref ORIGIN_SUCCESS is returned if the framework was disconnected successfully.
/// \note This function can only be called when the SDK is not running; otherwise, an \ref ORIGIN_ERROR_SDK_IS_RUNNING error
/// will be returned.
OriginErrorT ORIGIN_SDK_API OriginResetMemory();

#ifdef __cplusplus
}
#endif

#endif //__ORIGIN_MEMORY_H__
