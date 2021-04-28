#pragma once

#ifdef _WIN32
#include <jlib/windefs.h>
#else
#include <pthread.h>
#endif

namespace jlib
{

  class Condition
  {
  private:

#ifdef _WIN32
    CRITICAL_SECTION m_mutex;
    CONDITION_VARIABLE m_condition;
#else
    pthread_mutex_t m_mutex;
    pthread_cond_t m_condition;
#endif
  public:

    Condition ();
    ~Condition (void);

    void Signal (void);
    void Broadcast (void);
    void WaitForCondition (void);
    void WaitAndLock (void);
    void Unlock (void);

  };
}