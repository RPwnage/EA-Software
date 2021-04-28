///////////////////////////////////////////////////////////////////////////////
// EAMemory.inl
//
// Copyright (c) 2010 Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


// No include guards should be necessary since this is an internal file which
// is only ever #included in one place.

#if defined(EA_PLATFORM_PS3)
	#include <ppu_intrinsics.h>
#elif defined(EA_PLATFORM_XENON)
	#include <xtl.h>
#endif

#include <EAAssert/eaassert.h>


// If EASTDC_MEMORY_INLINE_ENABLED is 1 (usually optimized builds) then this .inl file is 
// #included from EAMemory.h and thus we want the functions to be declared inline and don't 
// want them ever declared with DLL-export tags like EASTDC_API does. But otherwise (usually
// debug builds) this .inl file is #included from EAMemory.cpp and we don't want 'inline' 
// but do want EASTDC_API.
#if EASTDC_MEMORY_INLINE_ENABLED
	#define EASTDC_EAMEMORY_DECL inline
#else
	#define EASTDC_EAMEMORY_DECL EASTDC_API  // Maps to __declspec(dllexport) in DLL-builds (with VC++).
#endif


namespace EA
{
namespace StdC
{

	EASTDC_EAMEMORY_DECL void Memclear(void* pDestination, size_t n)
	{
		memset(pDestination, 0, n);
	}


	EASTDC_EAMEMORY_DECL void MemclearC(void* pDestination, size_t n)
	{
		#if defined(EA_PLATFORM_XENON)
			if(n <= 64) // XMemSet performs poorly at smaller sizes.
				memset(pDestination, 0, n);
			else
				XMemSet(pDestination, 0, n);        // XMemset requires pDestination to be cacheable (i.e. conventional) memory.
		#elif defined(EA_PLATFORM_PS3)
			if(n < (2 * kCacheLineSize))
				memset(pDestination, 0, n);
			else
			{
				// This was benchmarked as about 2x - 3.x the speed of memset(pDestination, 0, n) with PS3 SDK 220 / GCC 4.1.1.
				void MemclearDcbzPS3(void* pDestination, size_t n);
				MemclearDcbzPS3(pDestination, n);   // MemclearDcbzPS3 requires pDestination to be cacheable (i.e. conventional) memory.
			}
		#else
			memset(pDestination, 0, n);
		#endif
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8(void* pDestination, uint8_t c, size_t uint8Count)
	{
		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8C(void* pDestination, uint8_t c, size_t uint8Count)
	{
		if(c == 0)
		{
			Memclear(pDestination, uint8Count);
			return (uint8_t*)pDestination;
		}

		#if defined(EA_PLATFORM_XENON)
			if(uint8Count <= 64) // XMemSet performs poorly at smaller sizes.
				return (uint8_t*)memset(pDestination, c, uint8Count);
			else
				return (uint8_t*)XMemSet(pDestination, c, uint8Count); // XMemset requires pDestination to be cacheable (i.e. conventional) memory.
		#elif defined(EA_PLATFORM_PS3)
			// We don't currently have anything fancy for PS3.
			return (uint8_t*)memset(pDestination, c, uint8Count);
		#else
			return (uint8_t*)memset(pDestination, c, uint8Count);
		#endif
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8_128(void* pDestination, uint8_t c, size_t uint8Count)
	{
		// To do: Make an optimized version of this.
		return (uint8_t*)memset(pDestination, c, uint8Count);
	}


	EASTDC_EAMEMORY_DECL uint8_t* Memset8_128C(void* pDestination, uint8_t c, size_t uint8Count)
	{
		#if defined(EA_PLATFORM_XENON)
			// XMemSet128 already has smarts for (c == 0).
			return (uint8_t*)XMemSet128(pDestination, c, uint8Count);  // XMemset128 requires pDestination to be cacheable (i.e. conventional) memory.
		#elif defined(EA_PLATFORM_PS3)
			if(c == 0)
			{
				Memclear(pDestination, uint8Count);
				return (uint8_t*)pDestination;
			}

			// We don't currently have anything fancy for PS3.
			return (uint8_t*)memset(pDestination, c, uint8Count);
		#else
			if(c == 0)
			{
				Memclear(pDestination, uint8Count);
				return (uint8_t*)pDestination;
			}

			return (uint8_t*)memset(pDestination, c, uint8Count);
		#endif
	}


	EASTDC_EAMEMORY_DECL void* MemsetPointer(void* pDestination, const void* const pValue, size_t ptrCount)
	{
		#if (EA_PLATFORM_PTR_SIZE == 8)
			return Memset64(pDestination, (uint64_t)(uintptr_t)pValue, ptrCount);
		#else
			return Memset32(pDestination, (uint32_t)(uintptr_t)pValue, ptrCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* Memcpy(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		#if defined(EA_PLATFORM_PS3_SPU)
			// The SPU compiler-provided memcpy is broken for the case of unaligned pointers.
			if(((uintptr_t(pDestination) | uintptr_t(pSource)) & 0xf) == 0) // If both are aligned to 16 bytes...
				memcpy(pDestination, pSource, nByteCount);
			else
			{
				unsigned       char* dst = static_cast<unsigned char*>(pDestination);
				const unsigned char* src = static_cast<const unsigned char*>(pSource);

				while (nByteCount > 0)
				{
					*dst++ = *src++;
					--nByteCount;
				}
			}

			return (char8_t*)pDestination;
		#else
			// Some compilers offer __builtin_memcpy, but we haven't found it to be faster than memcpy for any platforms
			// and it's significantly slower than memcpy for some platform/compiler combinations (e.g. SN compiler on PS3).
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* MemcpyC(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		#if defined(EA_PLATFORM_XENON)
			return (char8_t*)XMemCpy(pDestination, pSource, nByteCount); // XMemcpy requires pDestination to be cacheable (i.e. conventional) memory.
		#elif defined(EA_PLATFORM_PS3) 
			// So far our metrics have shown that the PS3 memcpy is better at this 
			// than the various custom memcpy implementations done at EA.
			// If you think you have a better implementation, feel free to submit it.
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#elif defined(EA_PLATFORM_PS3_SPU)
			return Memcpy(pDestination, pSource, nByteCount); // Need to use our custom version.
		#else
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* MemcpyS(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		#if defined(EA_PLATFORM_XENON)
			return (char8_t*)XMemCpyStreaming(pDestination, pSource, nByteCount);
		#elif defined(EA_PLATFORM_PS3_SPU)
			return Memcpy(pDestination, pSource, nByteCount); // Need to use our custom version.
		#else
			// To do: Port this to PS3.
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* Memcpy128(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		// This is expected to work with both cacheable and uncacheable memory, 
		// thus we can't use all alternative optimized functions that exist for memcpy.
		#if defined(EA_PLATFORM_PS3)
			return (char8_t*)memcpy16(pDestination, pSource, nByteCount); // PPU only, pointers must be alinged to 16 bytes, n must be a multiple of 16.
		#else
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* Memcpy128C(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount)
	{
		EA_ASSERT((pSource      >= (const uint8_t*)pDestination + nByteCount) || // Verify the memory doesn't overlap.
				  (pDestination >= (const uint8_t*)pSource      + nByteCount));

		#if defined(EA_PLATFORM_XENON)
			return (char8_t*)XMemCpy128(pDestination, pSource, nByteCount);
		#elif defined(EA_PLATFORM_PS3)
			return (char8_t*)memcpy16(pDestination, pSource, nByteCount); // PPU only, pointers must be alinged to 16 bytes, n must be a multiple of 16.
		#else
			return (char8_t*)memcpy(pDestination, pSource, nByteCount);
		#endif
	}


	EASTDC_EAMEMORY_DECL char8_t* Memmove(void* pDestination, const void* pSource, size_t nByteCount)
	{
		// Some compilers offer __builtin_memmove, but we haven't found it to be faster than memcpy for any platforms
		// and it's significantly slower than memcpy for some platform/compiler combinations (e.g. SN compiler on PS3).
		return (char8_t*)memmove(pDestination, pSource, nByteCount);
	}


	EASTDC_EAMEMORY_DECL char8_t* MemmoveC(void* pDestination, const void* pSource, size_t nByteCount)
	{
		#if defined(EA_PLATFORM_XENON)
			// To do: Try to come up with an improved memmove for 360. There currently 
			// isn't a specialized XMemmove function, and memmove is harder to implement
			// than memcpy. In practice, most uses of memmove are for small regions
			// of overlapping memory whereby fancy tricks don't help much.
			return (char8_t*)memmove(pDestination, pSource, nByteCount);
		#elif defined(EA_PLATFORM_PS3) 
			// The PS3 built-in memcpy is well optimized.
			return (char8_t*)memmove(pDestination, pSource, nByteCount);
		#else
			return (char8_t*)memmove(pDestination, pSource, nByteCount);
		#endif
	}

} // namespace StdC
} // namespace EA


// See the top of this file for #define EASTDC_API and an explanation of it.
#undef EASTDC_EAMEMORY_DECL

