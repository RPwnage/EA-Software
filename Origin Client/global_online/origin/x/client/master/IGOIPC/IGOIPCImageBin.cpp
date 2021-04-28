//
//  IPCImageAllocator.cpp
//  IGOIPC
//
//  Created by Frederic Meraud on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "IGOIPCImageBin.h"

#if defined(ORIGIN_MAC)

#include <EASTL/vector.h>
#include "IGOTrace.h"
#include "MacWindows.h"
#include "IGOIPCBuffer.h"
#include "IGOIPCConnection.h"

#if defined(DEBUG)
#include <syslog.h>
#else
#define syslog(...) ;
#endif


uint64_t AlignData(uint64_t size, uint64_t alignment) { return (size + alignment -1) & ~(alignment - 1); }




class IGOIPCImageBinImpl
{
    static const uint64_t BlockAlignment = 64;
    static const unsigned int BufferLockTimeoutInMS = 2000;

    public:
        IGOIPCImageBinImpl(const char* bufferName, int64_t bufferSize, enum IGOIPCBuffer::OpenOptions options, enum IGOIPCBuffer::BufferOptions bufferOptions)
            : _state(NULL)
            , _buffer(NULL)
        {
            _buffer = new IGOIPCBuffer(bufferName, bufferSize, options, bufferOptions);
            if (_buffer->IsValid())
            {
                _state = reinterpret_cast<BinState*>(_buffer->GetBaseAddr());
                
                if (_buffer->WasCreated())
                {
                    uint64_t headerSize = AlignData(sizeof(BinState), BlockAlignment);
                    memset(_state, 0, headerSize);
                    
                    _state->start = headerSize;
                    _state->read  = headerSize;
                    _state->last  = headerSize;
                    _state->write = headerSize;
                }
            }
            
            else
            {
                delete _buffer;
                _buffer = NULL;
            }
        }

        ~IGOIPCImageBinImpl()
        {
            delete _buffer;
        }
        
        bool IsValid() const { return _buffer != NULL; }

        bool Add(int ownerId, const char* data, int64_t dataSize, int64_t* offset)
        {
            unsigned char* storagePtr = NULL;
            int64_t storageDataSize = AlignData(sizeof(BlockHeader), BlockAlignment) + AlignData(dataSize, BlockAlignment);
            
            if (_buffer->TryLock(BufferLockTimeoutInMS))
            {
                if (_state->write >= _state->read)
                {
                    if ((_buffer->GetSize() - _state->write) >= storageDataSize)
                    {
                        storagePtr = _buffer->GetBaseAddr() + _state->write;
                        _state->last = _state->write;
                        _state->write += storageDataSize;
                    }
                    
                    else
                    {
                        if ((_state->read - _state->start) >= storageDataSize)
                        {
                            storagePtr = _buffer->GetBaseAddr() + _state->start;
                            _state->last = _state->write;
                            _state->write = _state->start + storageDataSize;
                        }
                    }
                }
                
                else
                {
                    if ((_state->read - _state->write) > storageDataSize)
                    {
                        storagePtr = _buffer->GetBaseAddr() + _state->write;
                        _state->last = _state->write;
                        _state->write += storageDataSize;
                    }
                }
            
                _buffer->Unlock();
            }
            
            else
            {
                IGO_TRACE("Failed to lock buffer");
            }
            
            if (storagePtr)
            {
                BlockHeader* header = reinterpret_cast<BlockHeader*>(storagePtr);
                header->ownerId = ownerId;
                header->dataSize = dataSize;
                header->nextBlock = _state->write;
                
                memcpy(storagePtr + BlockAlignment, data, dataSize);
                
                if (offset)
                    *offset = storagePtr - _buffer->GetBaseAddr();
                
                return true;
            }
            
            else
            {
                if (offset)
                    *offset = 0;
                
                return false;
            }
        }
    
        bool Replace(int ownerId, const char* data, int64_t dataSize, int64_t* offset)
        {
            if (!offset)
                return false;
            
            bool retVal = false;
            
            BlockHeader* header = reinterpret_cast<BlockHeader*>(_buffer->GetBaseAddr() + (*offset));
            if (header->ownerId == ownerId)
            {
                // If the same size, we can simply replace the content
                if (header->dataSize == dataSize)
                {
                    memcpy(_buffer->GetBaseAddr() + (*offset) + BlockAlignment, data, dataSize);
                    retVal = true;
                }
                
                else
                {
                    // Different content -> is it the last allocated block? at that point we can
                    // simply re-allocate it
                    int64_t newOffset = 0;
                    if (Add(ownerId, data, dataSize, &newOffset))
                    {
                        _skipped.insert(_skipped.begin(), *offset);
                        *offset = newOffset;
                        retVal = true;
                    }
                    
                    else
                    {
                        IGO_TRACEF("No more room to replace larger image data (ownerId=%d, offset=%ld, size=%ld)", ownerId, *offset, dataSize);
                    }
                }
            }
            
            else
            {
                IGO_TRACEF("Trying to replace invalid image data (ownerId=%d, offset=%ld, size=%ld)", ownerId, *offset, dataSize);
            }
            
            return retVal;
        }
        
        void Release(int ownerId, int64_t offset, int64_t size)
        {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(_buffer->GetBaseAddr() + offset);

            if (_buffer->TryLock(BufferLockTimeoutInMS))
            {
                if (header->ownerId == ownerId && header->dataSize == size)
                {
                    _state->read = header->nextBlock;
                    
                    // Keep on release if we have blocks that we skipped (during a resize?)
                    while (_skipped.size() > 0 && _skipped.back() == _state->read)
                    {
                        header = reinterpret_cast<BlockHeader*>(_buffer->GetBaseAddr() + _skipped.back());
            _skipped.pop_back();
                        _state->read = header->nextBlock;
                    }
                }
                
                else
                {
                    IGO_TRACEF("Trying to release invalid image data (ownerId=%d, offset=%ld, size=%ld)", ownerId, offset, size);
                }
                    
                _buffer->Unlock();
            }
            
            else
            {
                IGO_TRACE("Failed to lock buffer");
            }
        }
    
        char* GetDataAddr(int ownerId, int64_t offset, int64_t size)
        {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(_buffer->GetBaseAddr() + offset);
            if (header->ownerId == ownerId && header->dataSize == size)
                return reinterpret_cast<char*>(reinterpret_cast<char*>(header) + BlockAlignment);
            
            else
            {
                syslog(LOG_ERR, "Trying to access invalid image data (ownerId=%d, offset=%lld, size=%lld)\n", ownerId, offset, size);
                return NULL;
            }
        }
    
        IGOIPCImageBin::AutoReleaseContent Get(IGOIPCConnection* conn,int ownerId, int64_t offset, int64_t size)
        {
            if (conn)
                return IGOIPCImageBin::AutoReleaseContent(conn, this, ownerId, offset, size);
            
            else
                return IGOIPCImageBin::AutoReleaseContent(this, ownerId, offset, size);
        }
        
    private:
		IGOIPCImageBinImpl(const IGOIPCImageBinImpl &obj); 
        struct BinState
        {
            int64_t last;
            int64_t read;
            int64_t write;
            int64_t start;
        };
        
        struct BlockHeader
        {
            int64_t dataSize;
            int64_t nextBlock;
            int ownerId;
        };
    
        BinState* _state;
        IGOIPCBuffer* _buffer;
        eastl::vector<ptrdiff_t> _skipped;
};

////////////////////////////////////////////////////////////////////////////////////////////////

IGOIPCImageBin::AutoReleaseContent::AutoReleaseContent(IGOIPCImageBinImpl* bin, int ownerId, int64_t offset, int64_t size)
    : _ownerId(ownerId), _size(size), _offset(offset), _conn(NULL), _bin(bin)  
{
}

// Use this constructor to manage the buffers only on the server side (in our case to help with Mac not supporting a semaphore wait/signal with a timeout)
IGOIPCImageBin::AutoReleaseContent::AutoReleaseContent(IGOIPCConnection* connection, IGOIPCImageBinImpl* bin, int ownerId, int64_t offset, int64_t size)
    : _ownerId(ownerId), _size(size), _offset(offset), _conn(connection), _bin(bin) 
{
    
}

IGOIPCImageBin::AutoReleaseContent::~AutoReleaseContent()
{
    if (!IsEmpty())
    {
        // Can we release the buffer ourselves or do we need to notify the server?
        if (_conn)
        {
            // Let the server do it
            IGOIPC* ipc = IGOIPC::instance();
            if (ipc)
            {
                eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgReleaseBuffer(_ownerId, _offset, _size));
                _conn->send(msg);
            }
        }
        
        else
        {
            // Let's do it
            _bin->Release(_ownerId, _offset, _size);
        }
    }
}

bool IGOIPCImageBin::AutoReleaseContent::IsEmpty() const
{
    return _bin == NULL || _offset == 0 || _size == 0;
}

char* IGOIPCImageBin::AutoReleaseContent::Get() const
{
    if (!IsEmpty())
        return _bin->GetDataAddr(_ownerId, _offset, _size);
    
    else
        return NULL;
}

IGOIPCImageBin::IGOIPCImageBin(const char* bufferName, int64_t bufferSize, enum IGOIPCBuffer::OpenOptions options, enum IGOIPCBuffer::BufferOptions bufferOptions)
    :_impl(NULL)
{
    _impl = new IGOIPCImageBinImpl( bufferName, bufferSize, options, bufferOptions);
}

IGOIPCImageBin::~IGOIPCImageBin()
{
    delete _impl;
}

bool IGOIPCImageBin::IsValid() const
{
    return _impl->IsValid();
}

// Reserve a new bin in the memory buffer.
bool IGOIPCImageBin::Add(int ownerId, const char* data, int64_t dataSize, int64_t* offset)
{
    return _impl->Add(ownerId, data, dataSize, offset);
}

// Replace the content of a bin.
bool IGOIPCImageBin::Replace(int ownerId, const char* data, int64_t dataSize, int64_t* offset)
{
    return _impl->Replace(ownerId, data, dataSize, offset);
}

// Release a specific bin.
void IGOIPCImageBin::Release(int ownerId, int64_t offset, int64_t dataSize)
{
    return _impl->Release(ownerId, offset, dataSize);
}

// Gets a wrapper around a bin that will automatically release it when done.
IGOIPCImageBin::AutoReleaseContent IGOIPCImageBin::Get(int ownerId, int64_t offset, int64_t size)
{
    return _impl->Get(NULL, ownerId, offset, size);
}

// Gets a wrapper around abin that will automatically release it when done. 
// Using this version will use the the server/original creator to release the data.
IGOIPCImageBin::AutoReleaseContent IGOIPCImageBin::Get(IGOIPCConnection* connection, int ownerId, int64_t offset, int64_t size)
{
    return _impl->Get(connection, ownerId, offset, size);
}

#endif
