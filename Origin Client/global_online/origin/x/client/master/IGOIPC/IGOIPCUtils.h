///////////////////////////////////////////////////////////////////////////////
// IGOIPCUtils.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPCUTILS_H
#define IGOIPCUTILS_H

#include "EASTL/list.h"

#if defined(ORIGIN_PC)

void IGOIPCGetNamedPipes(eastl::list<uint32_t> &pipes);

#elif defined(ORIGIN_MAC) // MAC OSX

#include <mach/mach.h>


struct ConnectionID
{
    ConnectionID() {}
    ConnectionID(uint32_t myPID) : pid(myPID) {}
    
    uint32_t pid;
};

ConnectionID FuzzConnectionID(const ConnectionID& id);
ConnectionID UnfuzzConnectionID(const ConnectionID& id);

namespace Origin
{
    namespace Mac
    {
        // Encapsulate a read/write mach port
        class MachPort
        {    
        public:
            // Maximum message size we support -> make sure this is large enough to hold the largest IGOIPCMessage
            static const int MAX_MSG_SIZE = 256;
            
            // Possible return values when send data
            enum CommResult
            {
                CommResult_SUCCEEDED,
                CommResult_CONNECTION_DEAD,
                CommResult_FAILED,
                CommResult_NOTIFY
            };
            
        public:
            // Create a new r/w mach port
            // Pass -1 for the queueSize to use the system default queue size.
            MachPort(int queueSize = 512);
            
            // Create a wrapper around an existing port
            // sendRightsOnly = false -> don't take over cleanup responsabilities - used for compares
            // sendRightsOnly = true -> create of a write only port -> takes over the cleanup responsabilities.
            MachPort(mach_port_t port, bool sendRightsOnly = false, bool isNotification = false);
            
            // Copy a mach port with the auto_ptr semantics (ie reference port is not responsible
            // anymore for cleanup the port)
            MachPort(const MachPort& other);
            
            // Create a port from a known registered service (only send rights)
            MachPort(const char* serviceName);
            
            virtual ~MachPort();
            
            
            bool IsValid() const { return _isValid; }
            bool IsNotification() const { return _isNotification; }
                                                       
            // Copy a mach port with the auto_ptr semantics (ie reference port is not responsible
            // anymore for cleanup the port)
            MachPort& operator=(const MachPort& rhs);
            
            bool operator==(const MachPort& other) const { return _port == other._port; }
            bool operator!=(const MachPort& other) const { return !(*this == other); }
            
            static const MachPort& Null() { return _nullPort; }
            
            
            // Return a string to help with debugging
            void ToString(char* buffer, size_t bufferSize);
            
            // Register as a service with the bootstrap
            bool RegisterService(const char* serviceName);
            
            // Setup port for watch for dead name notification on a different port.
            bool Watch(const MachPort& port);
            
            // Add send rights to the port -> return separate instance to manage it
            MachPort AddSendRights();
            
            // Give send rights to another port
            CommResult ForwardSendRights(const MachPort& other, void* buffer, size_t bufferSize, int timeoutInMS);
            
            // Send a message
            // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
            // Use 0 for immediate polling.
            CommResult Send(const void* buffer, size_t bufferSize, int timeoutInMS, void* oolData, size_t oolDataSize);
            
            // Receive a message (with or without send rights from another port)
            // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
            // Use 0 for immediate polling.
            CommResult Recv(void* buffer, size_t bufferSize, int timeoutInMS, size_t* dataSize, MachPort* srPort, void** oolData, size_t* oolDataSize);
            
            
        private:
            static const MachPort _nullPort;
            
            mutable mach_port_t _port;
            mutable bool _isValid;
            mutable bool _isNotification;
            mutable bool _sendRightsOnly;
            
            
            friend class MachPortSet;
        };
        
        // Encapsulate a mach port set
        class MachPortSet
        {
        public:
            explicit MachPortSet();
            virtual ~MachPortSet();
            
            bool IsValid() const { return _portset != MACH_PORT_NULL; }
            
            // Return a string to help with debugging
            void ToString(char* buffer, size_t bufferSize);
            
            // Move a port receive right to the set
            bool Add(MachPort* port);
            
            // Restore a port receive right from the set
            bool Remove(MachPort* port);
            
            // Listen for new data on any of the ports that attached themselves to the set (could contain send rights from another port).
            // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
            // Use 0 for immediate polling.
            // Returns a valid MachPort ID if we got some data.
            MachPort Listen(void* buffer, size_t bufferSize, int timeoutInMS, size_t* dataSize, MachPort* srPort);
            
        private:
            mach_port_t _portset;
        };
        
    } // namespace Mac
} // namespace Origin

#endif // ORIGIN_MAC

#endif
