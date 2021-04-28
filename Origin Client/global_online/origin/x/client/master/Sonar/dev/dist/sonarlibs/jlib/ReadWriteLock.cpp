#include <jlib/ReadWriteLock.h>

#include <assert.h>

namespace jlib
{

ReadWriteLock::ReadWriteLock()
{
#ifdef _WIN32
  InitializeSRWLock(&m_rwLock);
#else
#error "Port this!"
#endif

}

ReadWriteLock::~ReadWriteLock()
{
#ifdef _WIN32

#else
#error "Port this!"
#endif
}

void ReadWriteLock::AcquireWriteLock()
{
#ifdef _WIN32
  AcquireSRWLockExclusive(&m_rwLock);
#else
#error "Port this!"
#endif
}

void ReadWriteLock::AcquireReadLock()
{
#ifdef _WIN32
  AcquireSRWLockShared(&m_rwLock);
#else
#error "Port this!"
#endif
}

void ReadWriteLock::ReleaseReadLock()
{
#ifdef _WIN32
  ReleaseSRWLockShared(&m_rwLock);
#else
#error "Port this!"
#endif
}

void ReadWriteLock::ReleaseWriteLock()
{
#ifdef _WIN32
  ReleaseSRWLockExclusive(&m_rwLock);
#else
#error "Port this!"
#endif
}

WriteLocker::WriteLocker(ReadWriteLock &lock) :
  m_lock(&lock)
{
  m_lock->AcquireWriteLock();
}

WriteLocker::~WriteLocker(void)
{
  m_lock->ReleaseWriteLock();
}

ReadLocker::ReadLocker(ReadWriteLock &lock) :
  m_lock(&lock)
{
  m_lock->AcquireReadLock();
}

ReadLocker::~ReadLocker(void)
{
  m_lock->ReleaseReadLock();
}


}