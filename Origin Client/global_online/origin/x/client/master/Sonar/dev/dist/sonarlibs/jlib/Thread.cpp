#include "jlib/Thread.h"

#ifdef _WIN32
#else
#include <sys/types.h>
#endif

#include <stdio.h>

namespace jlib
{

#ifdef _WIN32

Thread::Thread ()
{
    invalidate();
}

Thread::Thread (HANDLE _handle, DWORD _id)
{
	m_handle = _handle;
	m_id = _id;
}

Thread::Thread (Thread const& other)
{
	this->m_handle = other.m_handle;
	this->m_id = other.m_id;

	other.invalidate();
}

Thread::~Thread (void)
{
	if (isValid())
	{
		::CloseHandle(m_handle);
        invalidate();
	}
}

Thread &Thread::operator= (Thread const& other)
{
	if (isValid())
	{
		::CloseHandle(m_handle);
        invalidate();
	}

	this->m_handle = other.m_handle;
	this->m_id = other.m_id;

	other.invalidate();

	return *this;
}
	
Thread Thread::createThread(THREADPROC proc, void *arg)
{
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) proc, arg, 0, &id);
	Thread ret(hThread, id);

	return ret;
}

void Thread::join()
{
	WaitForSingleObject(m_handle, INFINITE);
}

long Thread::getCurrentId()
{
	return (long) GetCurrentThreadId();
}

long Thread::getId()
{
	return (long) m_id;
}

void Thread::suspend()
{
	::SuspendThread(m_handle);
}
void Thread::resume()
{
	::ResumeThread(m_handle);
}

void Thread::cancel()
{
    if (isValid()) {
        BOOL wasTerminated = ::TerminateThread(m_handle, 0);
        (void) wasTerminated;
		::CloseHandle(m_handle);
        invalidate();

    }
}

#else

Thread::Thread ()
	: m_handle(-1)
	, m_isValidHandle(false)
{
}

Thread::Thread (pthread_t _handle)
	: m_handle(_handle)
    , m_isValidHandle(true)
{
}

Thread::Thread (Thread const& other)
{
    if (isValid()) {
        int error = pthread_cancel(m_handle); (void) error;
        invalidate();
    }

    m_handle = other.m_handle;
    m_isValidHandle = other.m_isValidHandle;

    other.invalidate();
}

Thread::~Thread (void)
{
    if (isValid()) {
        int error = pthread_cancel(m_handle); (void) error;
        invalidate();
    }
}

Thread &Thread::operator= (Thread const& other)
{
    if (isValid()) {
        int error = pthread_cancel(m_handle); (void) error;
        invalidate();
    }

    m_handle = other.m_handle;
    m_isValidHandle = other.m_isValidHandle;

    other.invalidate();
}

Thread Thread::createThread(THREADPROC proc, void *arg)
{
    pthread_t tid;
	int error = pthread_create(&tid, NULL, proc, arg);
	return (!error) ? Thread(tid) : Thread();
}

void Thread::join()
{
	pthread_join(m_handle, NULL);
}

long Thread::getCurrentId()
{
	return (long) pthread_self();
}

long Thread::getId()
{
	return (long) m_handle;
}

void Thread::suspend()
{
    printf("error Thread::suspend not implemented");
}

void Thread::resume()
{
    printf("error Thread::resume not implemented");
}

void Thread::cancel()
{
    if (isValid())
    {
        int error = pthread_cancel(m_handle);
        (void) error;
        invalidate();
    }
}

#endif

}
