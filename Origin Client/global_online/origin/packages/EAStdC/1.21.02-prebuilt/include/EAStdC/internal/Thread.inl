///////////////////////////////////////////////////////////////////////////////
// Thread.inl
//
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


// ********************************************************
// To do: Now that EAStdC is dependent on EAThread, we can  
//        remove this thread code and use EAThread instead.
// ********************************************************


#ifndef EASTDC_THREAD_INL
#define EASTDC_THREAD_INL


#include <EABase/eabase.h>
#include <EAStdC/internal/Thread.h>
#include <EAAssert/eaassert.h>


namespace EA
{
	namespace StdC
	{

		#if defined(EA_PLATFORM_MICROSOFT)
			inline Mutex::Mutex()
			{
				#if defined(EA_PLATFORM_XENON) || !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0403)
					InitializeCriticalSection(&mMutex);
				#elif !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
					BOOL result = InitializeCriticalSectionEx(&mMutex, 10, 0);
					EA_ASSERT(result != 0); EA_UNUSED(result);
				#else
					BOOL result = InitializeCriticalSectionAndSpinCount(&mMutex, 10);
					EA_ASSERT(result != 0); EA_UNUSED(result);
				#endif
			}

			inline Mutex::~Mutex()
			{
				DeleteCriticalSection(&mMutex);
			}

			inline void Mutex::Lock()
			{
				EnterCriticalSection(&mMutex);
			}

			inline void Mutex::Unlock()
			{
				LeaveCriticalSection(&mMutex);
			}

		#elif defined(EA_PLATFORM_PS3)
			inline Mutex::Mutex()
			{
				sys_lwmutex_attribute_t lwattr;
				sys_lwmutex_attribute_initialize(lwattr);
				lwattr.attr_recursive = SYS_SYNC_RECURSIVE;

				sys_lwmutex_create(&mMutex, &lwattr);

				#ifdef EA_DEBUG
					sys_lwmutex_lock(&mMutex, 0);
					sys_lwmutex_unlock(&mMutex);
				#endif
			}

			inline Mutex::~Mutex()
			{
				sys_lwmutex_destroy(&mMutex);
			}

			inline void Mutex::Lock()
			{
				sys_lwmutex_lock(&mMutex, 0);
			}

			inline void Mutex::Unlock()
			{
				sys_lwmutex_unlock(&mMutex);
			}

		#elif defined(EA_PLATFORM_POSIX)
			inline Mutex::Mutex()
			{
				pthread_mutexattr_t attr;

				pthread_mutexattr_init(&attr);
				pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE); 
				pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
				pthread_mutex_init(&mMutex, &attr);
				pthread_mutexattr_destroy(&attr);
			}

			inline Mutex::~Mutex()
			{
				pthread_mutex_destroy(&mMutex);
			}

			inline void Mutex::Lock()
			{
				pthread_mutex_lock(&mMutex);
			}

			inline void Mutex::Unlock()
			{
				pthread_mutex_unlock(&mMutex);
			}

		#else

			inline Mutex::Mutex()
			{
			}

			inline Mutex::~Mutex()
			{
			}

			inline void Mutex::Lock()
			{
			}

			inline void Mutex::Unlock()
			{
			}

		#endif

	}
}




#endif // Header include guard

