///////////////////////////////////////////////////////////////////////////////
// Thread.h
//
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_THREAD_H
#define EASTDC_THREAD_H


// ********************************************************
// To do: Now that EAStdC is dependent on EAThread, we can
//        remove this thread code and use EAThread instead.
// ********************************************************


#include <EABase/eabase.h>
#if defined(EA_PLATFORM_XENON)
	#pragma warning(push, 0)
	#include <XTL.h>
	#pragma warning(pop)
#elif defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(push, 0)
	#include <Windows.h>
	#pragma warning(pop)
#elif defined(EA_PLATFORM_PS3)
	 #include <sys/synchronization.h>
#elif defined(EA_PLATFORM_PS3_SPU)
	 //#include <spu_thread.h>
	 //#include <sys/spu_stop_and_signal.h>
#elif defined(EA_PLATFORM_POSIX)
	#include <unistd.h>         // sbrk(), getpagesize(), mmap, munmap, etc.
	#if EASTDC_SYS_MMAN_H_AVAILABLE
		#include <sys/mman.h>   // mmap, etc.
	#endif
	#include <fcntl.h>          // open
	#include <pthread.h>
#endif

#if defined (__APPLE__)
	#include <libkern/OSAtomic.h>
#endif


namespace EA
{
	namespace StdC
	{
		/// Safely sets a new value. Returns the old value.
		uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue);

		/// Safely increments the value. Returns the new value.
		/// This function acts the same as the C++ pre-increment operator.
		uint32_t AtomicIncrement(uint32_t* pValue);

		/// Safely decrements the value. Returns the new value.
		/// This function acts the same as the C++ pre-decrement operator.
		uint32_t AtomicDecrement(uint32_t* pValue);

		/// Safely sets the value to a new value if the original value is equal to
		/// a condition value. Returns true if the condition was met and the
		/// assignment occurred. The comparison and value setting are done as
		/// an atomic operation and thus another thread cannot intervene between
		/// the two as would be the case with simple C code.
		bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition);


		/// Mutex
		///
		/// Implements a very simple portable Mutex class.
		///
		class Mutex
		{
		public:
			Mutex();
		   ~Mutex();

			void Lock();
			void Unlock();

		protected:
			#if defined(EA_PLATFORM_MICROSOFT)
				CRITICAL_SECTION mMutex;
			#elif defined(EA_PLATFORM_PS3)
				sys_lwmutex_t mMutex;
			#elif defined(EA_PLATFORM_POSIX)
				pthread_mutex_t mMutex;
			#endif
		};
	}
}








///////////////////////////////////////////////////////////////////////////////
// inline implmentation
///////////////////////////////////////////////////////////////////////////////


#if defined(EA_COMPILER_MSVC) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON)
	#if defined(EA_COMPILER_MSVC) && (EA_COMPILER_VERSION >= 1300) // VC7+ has an intrinsic
		#if defined(EA_PLATFORM_XENON)
			#pragma warning(push)
			#pragma warning(disable: 4311 4312)
			#include <XTL.h>
			#pragma warning(pop)
		#else
			#if EA_COMPILER_VERSION >= 1400 // If VC8 or later
				extern "C" long __cdecl _InterlockedIncrement(long volatile* Addend);
				extern "C" long __cdecl _InterlockedDecrement(long volatile* Addend);
				extern "C" long __cdecl _InterlockedCompareExchange(long volatile* Dest, long Exchange, long Comp);
				extern "C" long __cdecl _InterlockedExchange(long volatile* Target, long Value);
				extern "C" long __cdecl _InterlockedExchangeAdd(long volatile* Addend, long Value);
			#else // VC7.1 or earlier
				extern "C" long __stdcall _InterlockedIncrement(long volatile* Addend);
				extern "C" long __stdcall _InterlockedDecrement(long volatile* Addend);
				extern "C" long __stdcall _InterlockedCompareExchange(long volatile* Dest, long Exchange, long Comp);
				extern "C" long __stdcall _InterlockedExchange(long volatile* Target, long Value);
				extern "C" long __stdcall _InterlockedExchangeAdd(long volatile* Addend, long Value);
			#endif
		#endif

		#pragma intrinsic (_InterlockedCompareExchange)
		#define InterlockedCompareExchangeEAStdC _InterlockedCompareExchange

		#pragma intrinsic (_InterlockedExchange)
		#define InterlockedExchangeEAStdC _InterlockedExchange

		#pragma intrinsic (_InterlockedExchangeAdd)
		#define InterlockedExchangeAddEAStdC _InterlockedExchangeAdd

		#pragma intrinsic (_InterlockedIncrement)
		#define InterlockedIncrementEAStdC _InterlockedIncrement

		#pragma intrinsic (_InterlockedDecrement)
		#define InterlockedDecrementEAStdC _InterlockedDecrement

	#endif

	#ifndef InterlockedCompareExchangeEAStdC // If the above intrinsics aren't used...
		extern "C" __declspec(dllimport) long __stdcall InterlockedIncrement(long* pAddend);
		extern "C" __declspec(dllimport) long __stdcall InterlockedDecrement(long* pAddend);
		extern "C" __declspec(dllimport) long __stdcall InterlockedExchange(long* pTarget, long value);
		extern "C" __declspec(dllimport) long __stdcall InterlockedExchangeAdd(long* pAddend, long value);
		extern "C" __declspec(dllimport) long __stdcall InterlockedCompareExchange(long* pDestination, long value, long compare);

		#define InterlockedCompareExchangeEAStdC InterlockedCompareExchange
		#define InterlockedExchangeEAStdC        InterlockedExchange
		#define InterlockedExchangeAddEAStdC     InterlockedExchangeAdd
		#define InterlockedIncrementEAStdC       InterlockedIncrement
		#define InterlockedDecrementEAStdC       InterlockedDecrement
	#endif
#endif


namespace EA
{
	namespace StdC
	{

		#if defined(InterlockedExchangeEAStdC)

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				return (uint32_t)InterlockedExchangeEAStdC((long*)pValue, (long)newValue);
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				return (uint32_t)InterlockedIncrementEAStdC((long*)pValue);
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				return (uint32_t)InterlockedDecrementEAStdC((long*)pValue);
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				return ((uint32_t)InterlockedCompareExchangeEAStdC((long*)pValue, (long)newValue, (long)condition) == condition);
			}

		#elif defined(__SNC__) && defined(EA_PROCESSOR_POWERPC)

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				return __builtin_cellAtomicStore32(pValue, newValue);
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				return __builtin_cellAtomicIncr32(pValue) + 1;
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				return __builtin_cellAtomicDecr32(pValue) - 1;
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				const uint32_t nOriginalValue = __builtin_cellAtomicCompareAndSwap32(pValue, condition, newValue);
				return (condition == nOriginalValue);
			}

		#elif defined(EA_COMPILER_GNUC) && defined(EA_PROCESSOR_POWERPC)

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				uint32_t nOriginalValue;
				__asm__ __volatile__("1: lwarx  %0,0,%2\n\
										 stwcx. %1,0,%2\n\
										 bne-    1b"
									   : "=&b" (nOriginalValue) : "r" (newValue), "b" (pValue) : "cc", "memory");
				return nOriginalValue;
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				uint32_t nNewValue;
				__asm__ __volatile__("1: lwarx  %0,0,%1\n\
										 addi    %0,%0,1\n\
										 stwcx. %0,0,%1\n\
										 bne-    1b"
									   : "=&b" (nNewValue) : "b" (pValue) : "cc", "memory");
				return nNewValue;
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				uint32_t nNewValue;
				__asm__ __volatile__("1: lwarx  %0,0,%1\n\
										 addi    %0,%0,-1\n\
										 stwcx. %0,0,%1\n\
										 bne-    1b"
									   : "=&b" (nNewValue) : "b" (pValue) : "cc", "memory");
				return nNewValue;
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				uint32_t nOriginalValue;
				__asm__ __volatile__("\n\
									 1: lwarx  %0,0,%1 \n\
										cmpw    0,%0,%2 \n\
										bne     2f \n\
										stwcx. %3,0,%1 \n\
										bne-    1b\n"
									"2:"
										: "=&b" (nOriginalValue)
										: "b" (pValue), "r" (condition), "r" (newValue)
										: "cc", "memory");
				return (condition == nOriginalValue);
			}

		#elif defined(EA_COMPILER_GNUC) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))

			int32_t InterlockedExchangeEAStdC(volatile int32_t* m, int32_t n)
			{
				int32_t result;

				__asm__ __volatile__ (
					"xchgl %%eax, (%2)" // The xchg instruction does an implicit lock instruction.
					: "=a" (result)     // outputs
					: "a" (n), "q" (m)  // inputs
					: "memory"          // clobbered
					);

				return result;
			}

			int32_t InterlockedCompareExchangeEAStdC(volatile int32_t* m, int32_t n, int32_t condition)
			{
				int32_t result;

				__asm__ __volatile__(
					"lock cmpxchgl %3, (%1) \n"         // Test *m against EAX, if same, then *m = n
					: "=a" (result), "=q" (m)           // outputs
					: "a" (condition), "q" (n), "1" (m) // inputs
					: "memory"                          // clobbered
					);

				return result;
			}

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				return (uint32_t)InterlockedExchangeEAStdC((int32_t*)pValue, newValue);
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				uint32_t result;

				__asm__ __volatile__ ("lock; xaddl %0, %1"
									: "=r" (result), "=m" (*pValue)
									: "0" (1), "m" (*pValue)
									: "memory"
									);
				return result + 1;
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				uint32_t result;

				__asm__ __volatile__ ("lock; xaddl %0, %1"
									: "=r" (result), "=m" (*pValue)
									: "0" (-1), "m" (*pValue)
									: "memory"
									);
				return result - 1;
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				return ((uint32_t)InterlockedCompareExchangeEAStdC((int32_t*)pValue, newValue, condition) == condition);
			}

		#elif defined(__APPLE__)

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				const uint32_t oldValue(*pValue);
				*pValue = newValue;
				return oldValue;
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				return OSAtomicIncrement32Barrier(reinterpret_cast<int32_t*>(pValue));
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				return OSAtomicDecrement32Barrier(reinterpret_cast<int32_t*>(pValue));
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				return OSAtomicCompareAndSwap32Barrier(condition, newValue, reinterpret_cast<int32_t*>(pValue));
			}

		#elif (defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 403)) && !defined(EA_PLATFORM_ANDROID)

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				__sync_synchronize();
				return __sync_lock_test_and_set(pValue, newValue);
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				return __sync_add_and_fetch(pValue, 1);
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				return __sync_sub_and_fetch(pValue, 1);
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				return (__sync_val_compare_and_swap(pValue, condition, newValue) == condition);
			}

		#else

			inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
			{
				const uint32_t nOldValue(*pValue);
				*pValue = newValue;
				return nOldValue;
			}

			inline uint32_t AtomicIncrement(uint32_t* pValue)
			{
				const uint32_t nNewValue(++*pValue);
				return nNewValue;
			}

			inline uint32_t AtomicDecrement(uint32_t* pValue)
			{
				const uint32_t nNewValue(--*pValue);
				return nNewValue;
			}

			inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
			{
				const bool bResult(*pValue == condition);
				if(bResult)
					*pValue = newValue;
				return bResult;
			}

		#endif

	} // namespace StdC

} // namespace EA




#endif // Header include guard

