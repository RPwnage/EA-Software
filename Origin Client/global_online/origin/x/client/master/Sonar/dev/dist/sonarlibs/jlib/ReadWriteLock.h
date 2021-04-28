#pragma once

#ifdef _WIN32
#include <jlib/windefs.h>
#else
#error "Port this!"
#endif


namespace jlib
{

  class ReadWriteLock
  {

  public:
    ReadWriteLock();
    ~ReadWriteLock();

    void AcquireWriteLock();
    void AcquireReadLock();

    void ReleaseReadLock();
    void ReleaseWriteLock();

  private:
#ifdef _WIN32
    SRWLOCK m_rwLock;
#else
    enum LOCKTYPE
    {
      NONE,
      READ,
      WRITE,
    };

    volatile LOCKTYPE m_lockType;
#endif
  };




  class ReadLocker
  {
  public:

    ReadLocker(ReadWriteLock &lock);
    ~ReadLocker(void);

  private:
    ReadWriteLock *m_lock;
  };

  class WriteLocker
  {
  public:

    WriteLocker(ReadWriteLock &lock);
    ~WriteLocker(void);

  private:
    ReadWriteLock *m_lock;
  };

}