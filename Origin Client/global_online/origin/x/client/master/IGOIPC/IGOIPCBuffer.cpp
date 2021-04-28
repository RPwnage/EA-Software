//
//  IPCBuffer.cpp
//  sharedUtils
//
//  Created by Frederic Meraud on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "IGOIPCBuffer.h"

#if defined(ORIGIN_MAC)

#include "IGOTrace.h"
#include "MacWindows.h"

#include "EABase/eabase.h"
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>


class IGOIPCBufferImpl
{
    public:
        IGOIPCBufferImpl(const char* bufferName, int64_t bufferSize, enum IGOIPCBuffer::OpenOptions options, enum IGOIPCBuffer::BufferOptions bufferOptions)
            : _lockObj(NULL)
            , _baseAddress(NULL)
            , _bufferObj(NULL)
            , _wasCreated(false)
        {
            if (bufferName == NULL || bufferName[0] == '\0' || bufferSize <= 0)
            {
                IGO_TRACE("Invalid parameters sent to create shared memory");
                return;
            }
            
            // Align data nicely
            int pageSize = getpagesize();
            if (pageSize > 0)
                bufferSize = ((bufferSize + pageSize - 1) / pageSize) * pageSize;
            
            // Create shared memory file
            int oflag = O_RDWR;
            bool wasCreated = false;
            if (options == IGOIPCBuffer::OpenOptions_CREATE || options == IGOIPCBuffer::OpenOptions_CREATE_EXCL)
            {
                sem_unlink(bufferName);                
                oflag |= O_CREAT | O_EXCL;
            }
            
            
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            int shmObj = shm_open(bufferName, oflag, mode);
            if (shmObj == -1)
            {
                if (errno == EEXIST && options == IGOIPCBuffer::OpenOptions_CREATE)
                {
                    oflag &= ~O_EXCL;
                    shmObj = shm_open(bufferName, oflag);
                    if (shmObj == -1)
                    {
                        IGO_TRACEF("Unable to create/open shared memory '%s' (%d)", bufferName, errno);
                    }
                }
                
                else 
                {
                    IGO_TRACEF("Unable to create/open shared memory '%s' (%d)", bufferName, errno);
                    return;
                }
            }
            
            else
            {
                if (options != IGOIPCBuffer::OpenOptions_ATTACH)
                {
                    if (ftruncate(shmObj, bufferSize) != 0)
                    {
                        Origin::Mac::LogError("Unable to resize shared memory '%s' to %lld bytes appropriately (%d)\n", bufferName, bufferSize, errno);
                        
                        close(shmObj);
                        return;
                    }
                    
                    wasCreated = true;
                }
            }
            
            
            // Map it to the process address space
            unsigned char* baseAddress = (unsigned char*)mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmObj, 0);
            if (baseAddress == MAP_FAILED)
            {
                Origin::Mac::LogError("Unable to map shared memory '%s' to process address space (%d)\n", bufferName, errno);
                
                close(shmObj);
                if (wasCreated)
                    shm_unlink(bufferName);
                
                return;
            }
            
            
            // Create semaphore to help with the lock/unlock of the buffer except if we are using "local" management
            // (which right now is mostly the case since we don't have a semaphore with a timed wait)
            if ((bufferOptions & IGOIPCBuffer::BufferOptions_USE_LOCAL_LOCK) == 0)
            {
                mode = S_IRWXU | S_IRWXG | S_IRWXO;
                snprintf(_lockName, sizeof(_lockName), "%s@LOCK", bufferName);            
                sem_t* semObj = sem_open(_lockName, O_CREAT, mode, 1);
                if (semObj == SEM_FAILED)
                {
                    IGO_TRACEF("Unable to allocate/open shared memory semaphore '%s' (%d)", _lockName, errno);
                    
                    munmap(_baseAddress, bufferSize);
                    close(shmObj);
                    if (wasCreated)
                        shm_unlink(bufferName);
                    
                    return;
                }
                
                _lockObj     = semObj;
            }
            
            _bufferObj   = shmObj;
            _baseAddress = baseAddress;
            _wasCreated  = wasCreated;
            _bufferSize  = bufferSize;
            
            strncpy(_bufferName, bufferName, sizeof(_bufferName));
            
            IGO_TRACEF("Successfully created shared buffer '%s'", bufferName);
        }
    
        ~IGOIPCBufferImpl()
        {
            if (IsValid())
            {
                if (_lockObj)
                    sem_close(_lockObj);
                
                if (msync(_baseAddress, _bufferSize, MS_SYNC) != 0)
                {
                    IGO_TRACEF("Unable to sync shared memory '%s' (%d)", _bufferName, errno);
                }
               
                if (munmap(_baseAddress, _bufferSize) != 0)
                {
                    IGO_TRACEF("Unable to undo shared memory '%s' mapping (%d)", _bufferName, errno);
                }
                
                if (close(_bufferObj) != 0)
                {
                    IGO_TRACEF("Unable to close shared memory '%s' (%d)", _bufferName, errno);
                }
                
                if (_wasCreated)
                {
                    if (_lockObj)
                    {
                        if (sem_unlink(_lockName) != 0)
                        {
                            IGO_TRACEF("Unable to unlink the shared memory lock '%s' (%d)", _lockName, errno);
                        }
                    }
                    
                    if (shm_unlink(_bufferName) != 0)
                    {
                        IGO_TRACEF("Unable to unlink the shared memory '%s' (%d)", _bufferName, errno);
                    }
                }
            }
        }
    
        bool IsValid() const { return _baseAddress != NULL; }
        bool WasCreated() const { return _wasCreated; }
        ssize_t GetSize() const { return _bufferSize; }
        unsigned char* GetBaseAddr() const { return _baseAddress; }
        
        void Lock()
        {
            if (IsValid())
            {
                if (_lockObj)
                    sem_wait(_lockObj);
                
                else
                    _localLockObj.Wait();
            }
        }

        void Unlock()
        {
            if (IsValid())
            {
                if (_lockObj)
                    sem_post(_lockObj);
                
                else
                    _localLockObj.Signal();
            }
        }

        bool TryLock(long timeoutInMS)
        {
            if (IsValid())
            {
                if (_lockObj)
                {
                    // FIXME: THIS IS WRONG
                    sem_wait(_lockObj);
                    return true;
                }
                
                else
                {
                    return _localLockObj.Wait(timeoutInMS);
                }
            }
            
            else
                return false;
        }
    
    private:
		IGOIPCBufferImpl(const IGOIPCBufferImpl &obj);
        char _lockName[MAX_PATH];
        char _bufferName[MAX_PATH];

        Origin::Mac::Semaphore _localLockObj;
    
        sem_t* _lockObj;
        unsigned char* _baseAddress;
    
        size_t _bufferSize;
        int _bufferObj;
        bool _wasCreated;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

IGOIPCBuffer::IGOIPCBuffer(const char* bufferName, int64_t bufferSize, enum IGOIPCBuffer::OpenOptions options, enum IGOIPCBuffer::BufferOptions bufferOptions)
    : _impl(NULL)
{
    _impl = new IGOIPCBufferImpl(bufferName, bufferSize, options, bufferOptions);
}

IGOIPCBuffer::~IGOIPCBuffer()
{
    delete _impl;
}

bool IGOIPCBuffer::IsValid() const 
{ 
    return _impl->IsValid(); 
}

bool IGOIPCBuffer::WasCreated() const
{
    return _impl->WasCreated();
}

int64_t IGOIPCBuffer::GetSize() const
{
    return _impl->GetSize();
}

unsigned char* IGOIPCBuffer::GetBaseAddr() const
{ 
    return _impl->GetBaseAddr(); 
}

void IGOIPCBuffer::Lock()
{
    _impl->Lock();
}

void IGOIPCBuffer::Unlock()
{
    _impl->Unlock();
}

bool IGOIPCBuffer::TryLock(long timeoutInMS)
{
    return _impl->TryLock(timeoutInMS);
}

#endif