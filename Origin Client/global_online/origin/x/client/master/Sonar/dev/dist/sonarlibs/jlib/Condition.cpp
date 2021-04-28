#include <jlib/Condition.h>

namespace jlib
{
	Condition::Condition ()
	{
#ifdef _WIN32
		InitializeConditionVariable(&m_condition);
		InitializeCriticalSection(&m_mutex);
#else
		pthread_mutex_init (&m_mutex, NULL);
		pthread_cond_init(&m_condition, NULL);
#endif
	}

	Condition::~Condition (void)
	{
#ifdef _WIN32
		//FIXME: No delete for conditions?
		DeleteCriticalSection(&m_mutex);
#else
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy (&m_condition);
#endif
	}

	void Condition::Signal (void)
	{
#ifdef _WIN32
		WakeConditionVariable(&m_condition);
#else
		pthread_cond_signal (&m_condition);
#endif
	}

	void Condition::Broadcast (void)
	{
#ifdef _WIN32
		WakeAllConditionVariable(&m_condition);
#else
		pthread_cond_broadcast (&m_condition);
#endif
	}

	void Condition::WaitForCondition (void)
	{
#ifdef _WIN32
		SleepConditionVariableCS(&m_condition, &m_mutex, INFINITE);
#else
		pthread_cond_wait (&m_condition, &m_mutex);
#endif
	}

	void Condition::WaitAndLock (void)
	{
#ifdef _WIN32
		EnterCriticalSection(&m_mutex);
#else
		pthread_mutex_lock (&m_mutex);
#endif
	}

	void Condition::Unlock (void)
	{
#ifdef _WIN32
		LeaveCriticalSection(&m_mutex);
#else
		pthread_mutex_unlock (&m_mutex);
#endif

	}
}