//
//  IPCImageAllocator.h
//  IGOIPC
//
//  Created by Frederic Meraud on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef IGOIPC_IPCImageBin_h
#define IGOIPC_IPCImageBin_h

#if defined(ORIGIN_MAC)

#include <sys/types.h>
#include "IGOIPCBuffer.h"

class IGOIPCConnection;
class IGOIPCImageBinImpl;
class IGOIPCImageContentImpl;




// Class used to manage a memory buffer as a set of bins (allocated as a cyclic queue)
class IGOIPCImageBin
{
    public:
        // Helper class to automatically release a specific entry/bin in the memory buffer
        class AutoReleaseContent
        {
            public:
                explicit AutoReleaseContent(IGOIPCImageBinImpl* bin, int ownerId, int64_t offset, int64_t size);
                explicit AutoReleaseContent(IGOIPCConnection* connection, IGOIPCImageBinImpl* bin, int ownerId, int64_t offset, int64_t size);
                ~AutoReleaseContent();
            
            bool IsEmpty() const;
            char* Get() const;
            
            private:
                int _ownerId;
                int64_t _size;
                int64_t _offset;
                IGOIPCConnection* _conn;
                IGOIPCImageBinImpl* _bin;
        };

    
    public:
        explicit IGOIPCImageBin(const char* bufferName, int64_t bufferSize, enum IGOIPCBuffer::OpenOptions options, enum IGOIPCBuffer::BufferOptions bufferOptions);
        IGOIPCImageBin(const IGOIPCImageBin &obj);
		~IGOIPCImageBin();

        bool IsValid() const;
    
        // Reserve a new bin in the memory buffer.
        bool Add(int ownerId, const char* data, int64_t dataSize, int64_t* offset);
    
        // Replace the content of a bin.
        bool Replace(int ownerId, const char* data, int64_t dataSize, int64_t* offset);
    
        // Release a specific bin.
        void Release(int ownerId, int64_t offset, int64_t dataSize);
        
        // Gets a wrapper around a bin that will automatically release it when done.
        AutoReleaseContent Get(int ownerId, int64_t offset, int64_t size);

        // Gets a wrapper around abin that will automatically release it when done. 
        // Using this version will use the the server/original creator to release the data.
        AutoReleaseContent Get(IGOIPCConnection* connection, int ownerId, int64_t offset, int64_t size);
    
    private:
        IGOIPCImageBinImpl* _impl;
};

#endif

#endif
