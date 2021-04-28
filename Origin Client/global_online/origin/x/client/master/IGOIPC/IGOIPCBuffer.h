//
//  IPCBuffer.h
//  sharedUtils
//
//  Created by Frederic Meraud on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef sharedUtils_IPCBuffer_h
#define sharedUtils_IPCBuffer_h

#if defined(ORIGIN_MAC)

#include <sys/types.h>


class IGOIPCBufferImpl;

class IGOIPCBuffer
{
    public:
        enum OpenOptions
        {
            OpenOptions_CREATE,
            OpenOptions_CREATE_EXCL,
            OpenOptions_ATTACH
        };
    
        enum BufferOptions
        {
            BufferOptions_NONE,
            BufferOptions_USE_LOCAL_LOCK,   // used when we want only one process to manage the content of the buffer
        };
        
    public:
        explicit IGOIPCBuffer(const char* bufferName, int64_t bufferSize, enum OpenOptions options, enum BufferOptions bufferOptions);
        ~IGOIPCBuffer();
    
        bool IsValid() const;
        bool WasCreated() const;
        int64_t GetSize() const;
        unsigned char* GetBaseAddr() const;
    
        void Lock();
        void Unlock();
        bool TryLock(long timeoutInMS);
    
    private:
        IGOIPCBufferImpl* _impl;
		IGOIPCBuffer(const IGOIPCBuffer &obj);
};

class IGOIPCBufferAutoLock
{
public:
    explicit IGOIPCBufferAutoLock(IGOIPCBuffer* buffer)
    {
        _buffer = buffer;
        buffer->Lock();
    }
    
    ~IGOIPCBufferAutoLock()
    {
        _buffer->Unlock();
    }
    
private:
    IGOIPCBuffer* _buffer;
};

#endif

#endif
