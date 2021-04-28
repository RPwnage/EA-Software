#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

namespace jlib
{

	class Thread
	{
	public:
		typedef void *(*THREADPROC)(void *arg);

	private:

#ifdef _WIN32
		Thread (HANDLE _handle, DWORD _id);
#else
		Thread (pthread_t _handle);
#endif


	public:
		Thread ();
		~Thread (void);
		Thread (Thread const& other);

		Thread & operator= (Thread const& other);

		static Thread createThread(THREADPROC proc, void *arg);
		void join();
		long getId();

		static long getCurrentId();

		void suspend();
		void resume();
        void cancel();

        bool isValid() const;

	private:

        void invalidate() const;

#ifdef _WIN32
		mutable HANDLE m_handle;
		mutable DWORD m_id;
#else
		mutable pthread_t m_handle;
        mutable bool m_isValidHandle;
#endif

	};

// inlines

inline bool Thread::isValid() const
{
#ifdef _WIN32
    return m_handle != INVALID_HANDLE_VALUE;
#else
    return m_isValidHandle;
#endif
}

inline void Thread::invalidate() const
{
#ifdef _WIN32
    m_handle = INVALID_HANDLE_VALUE;
    m_id = (DWORD) -1;
#else
    m_isValidHandle = false;
#endif
}

}
