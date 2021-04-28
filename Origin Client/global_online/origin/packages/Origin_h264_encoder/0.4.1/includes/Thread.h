#ifndef __THREAD_H
#define __THREAD_H

// copy of eathread_futex.h - v 1.16.02 - use temporarily until we can properly include the package

/////////////////////////////////////////////////////////////////////////////
// eathread_futex.h
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana, Maxis
//
// Implements a fast, user-space mutex. Also known as a lightweight mutex.
/////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////
/// Futex data
///
/// This is used internally by class Futex.
/// Note that we don't use an EAThread semaphore, as the direct semaphore
/// we use here is more direct and avoid inefficiencies that result from 
/// the possibility of EAThread semaphores being optimized for being 
/// standalone.
/// 
extern "C"
{
	struct _RTL_CRITICAL_SECTION;

	__declspec(dllimport) void          __stdcall InitializeCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
	__declspec(dllimport) void          __stdcall DeleteCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
	__declspec(dllimport) void          __stdcall EnterCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
	__declspec(dllimport) void          __stdcall LeaveCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
	__declspec(dllimport) int           __stdcall TryEnterCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
	__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
}

typedef void* EAFutexSemaphore; // void* instead of HANDLE to avoid #including windows headers.

/////////////////////////////////////////////////////////////////////////




namespace EA
{
	namespace Thread
	{
#if defined(_WIN64)
		static const int FUTEX_PLATFORM_DATA_SIZE = 40; // CRITICAL_SECTION is 40 bytes on Win64.
#elif defined(_WIN32)
		static const int FUTEX_PLATFORM_DATA_SIZE = 32; // CRITICAL_SECTION is 24 bytes on Win32 and 28 bytes on XBox 360.
#endif


		/// class Futex
		///
		/// A Futex is a fast user-space mutex. It works by attempting to use
		/// atomic integer updates for the common case whereby the mutex is
		/// not already locked. If the mutex is already locked then the futex
		/// drops down to waiting on a system-level semaphore. The result is 
		/// that uncontested locking operations can be significantly faster 
		/// than contested locks. Contested locks are slightly slower than in 
		/// the case of a formal mutex, but usually not by much.
		///
		/// The Futex acts the same as a conventional mutex with respect to  
		/// memory synchronization. Specifically: 
		///     - A Lock or successful TryLock implies a read barrier (i.e. acquire).
		///     - A second lock by the same thread implies no barrier.
		///     - A failed TryLock implies no barrier.
		///     - A final unlock by a thread implies a write barrier (i.e. release).
		///     - A non-final unlock by a thread implies no barrier.
		///
		/// Futex limitations relative to Mutexes:
		///     - Futexes cannot be inter-process.
		///     - Futexes cannot be named.
		///     - Futexes cannot participate in condition variables. A special 
		///       condition variable could be made that works with them, though.
		///     - Futex locks don't have timeouts. This could probably be
		///       added with some work, though users generally shouldn't need timeouts. 
		///
		class Futex
		{
		public:
			/// Futex
			///
			/// Creates a Futex. There are no creation options.
			///
			Futex();

			/// ~Futex
			///
			/// Destroys an existing futex. The futex must not be locked by any thread
			/// upon this call, otherwise the resulting behaviour is undefined.
			///
			~Futex();

			/// Lock
			///
			/// Locks the futex; returns the new lock count.
			/// If the futex is locked by another thread, this call will block until
			/// the futex is unlocked. If the futex is not locked or is locked by the
			/// current thread, this call will return immediately.
			///
			void Lock();

			/// TryLock
			///
			/// Tries to lock the futex; returns true if possible.
			/// This function always returns immediately. It will return false if 
			/// the futex is locked by another thread, and it will return true 
			/// if the futex is not locked or is already locked by the current thread.
			///
			bool TryLock();

			/// Unlock
			///
			/// Unlocks the futex. The futex must already be locked at least once by 
			/// the calling thread. Otherwise the behaviour is not defined.
			///
			void Unlock();

			/// GetLockCount
			///
			/// Returns the number of locks on the futex. The return value from this 
			/// function is only reliable if the calling thread already has one lock on 
			/// the futex. Otherwise the returned value may not represent actual value
			/// at any point in time, as other threads lock or unlock the futex soon after the call.
			///
			int GetLockCount() const;

			/// HasLock
			/// Returns true if the current thread has the futex locked. 
			bool HasLock() const;

		protected:

		private:
			// Objects of this class are not copyable.
			Futex(const Futex&){}
			Futex& operator=(const Futex&){ return *this; }

		protected:
#if defined(_WIN64)
			uint64_t mCRITICAL_SECTION[FUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t)];
#else
			uint64_t mCRITICAL_SECTION[FUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t)];
#endif
		};



		/// FutexFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Futex.
		/// A primary use of this would be to allow the Futex implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		///
		class FutexFactory
		{
		public:
			static Futex*  CreateFutex();                    // Internally implemented as: return new Futex;
			static void    DestroyFutex(Futex* pFutex);      // Internally implemented as: delete pFutex;

			static size_t  GetFutexSize();                   // Internally implemented as: return sizeof(Futex);
			static Futex*  ConstructFutex(void* pMemory);    // Internally implemented as: return new(pMemory) Futex;
			static void    DestructFutex(Futex* pFutex);     // Internally implemented as: pFutex->~Futex();
		};



		/// class AutoFutex
		/// An AutoFutex locks the Futex in its constructor and 
		/// unlocks the Futex in its destructor (when it goes out of scope).
		class AutoFutex
		{
		public:
			AutoFutex(Futex& futex);
			~AutoFutex();

		protected:
			Futex& mFutex;

			// Prevent copying by default, as copying is dangerous.
			AutoFutex(const AutoFutex&);
			const AutoFutex& operator=(const AutoFutex&);
		};


		inline AutoFutex::AutoFutex(Futex& futex) 
			: mFutex(futex)
		{
			mFutex.Lock();
		}

		inline AutoFutex::~AutoFutex()
		{
			mFutex.Unlock();
		}

	} // namespace Thread

} // namespace EA






///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EAFutexReadBarrier
//
// For futexes, which are intended to be used only in user-space and without 
// talking to IO devices, DMA memory, or uncached memory, we directly use
// memory barriers.
#define EAFutexReadBarrier      EAReadBarrier
#define EAFutexWriteBarrier     EAWriteBarrier
#define EAFutexReadWriteBarrier EAReadWriteBarrier
///////////////////////////////////////////////////////////////////////////////



namespace EA
{
	namespace Thread
	{
		inline Futex::Futex()
		{
			InitializeCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
		}

		inline Futex::~Futex()
		{
			//					int count = GetLockCount();
			//                    EAT_ASSERT(!count);
			DeleteCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
		}

		inline void Futex::Lock()
		{
			EnterCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
		}

		inline bool Futex::TryLock()
		{
			return TryEnterCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION) != 0;
		}

		inline void Futex::Unlock()
		{
			LeaveCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
		} 

		inline int Futex::GetLockCount() const
		{
			// Return the RecursionCount member of RTL_CRITICAL_SECTION.

			// We use raw structure math because otherwise we'd expose the user to system headers, 
			// which breaks code and bloats builds. We validate our math in eathread_futex.cpp.
#if defined(_WIN64)
			return *((int*)mCRITICAL_SECTION + 3); 
#else
			return *((int*)mCRITICAL_SECTION + 2);
#endif
		}

		inline bool Futex::HasLock() const
		{
			// Check the OwningThread member of RTL_CRITICAL_SECTION.

			// We use raw structure math because otherwise we'd expose the user to system headers, 
			// which breaks code and bloats builds. We validate our math in eathread_futex.cpp.
#if defined(_WIN64)
			return (*((uint32_t*)mCRITICAL_SECTION + 4) == (uintptr_t)GetCurrentThreadId());
#else
			return (*((uint32_t*)mCRITICAL_SECTION + 3) == (uintptr_t)GetCurrentThreadId());
#endif
		}
	} // namespace Thread

} // namespace EA

#define USE_FRAME_BUFFERS_FROM_THREAD

void InitThreads();
void ExitThreads();
int DetectNumOfCores();
void ManageConversion(YUVFrameType *dst_frame, uint8_t *src_444_frame);
void ManageThreads(YUVFrameType *dst_frame);
bool AddInputFrameToQueue(Origin_H264_EncoderInputType *input);
bool WaitForSpaceInFrameQueue(int ms_wait);
int InputFramesInQueue();
void DropFrame();
void EmptyInputFrame();

#endif
