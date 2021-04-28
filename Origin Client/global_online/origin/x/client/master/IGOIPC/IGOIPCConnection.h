///////////////////////////////////////////////////////////////////////////////
// IGOIPCConnection.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPCCONNECTION_H
#define IGOIPCCONNECTION_H

#ifdef ORIGIN_PC
#pragma warning(disable: 4996)    // 'sprintf': name was marked as #pragma deprecated
#endif

#include "EASTL/list.h"
#include "EASTL/shared_ptr.h"
#include <deque>

#include "IGOIPCMessage.h"
#include "IGOIPC.h"

#if defined(ORIGIN_PC)

class IGOIPCConnection
{
public:
    IGOIPCConnection();
    virtual ~IGOIPCConnection();

#if defined(ORIGIN_MAC)
  	virtual bool start(const char* serviceName, size_t messageQueueSize) { return true; };
#elif defined(ORIGIN_PC)
	virtual bool start() { return true; };
#endif
	virtual void stop(void *userData = NULL) { };
	bool send(eastl::shared_ptr<IGOIPCMessage> msg);
	int sendRaw(const void *msg, int size);
	int recvRaw(void *msg, int size);
	bool hasMessage();
    bool checkPipe();
    bool messageReady() { return mMessageReady; }

    uint32_t id() { return mId; }
    eastl::shared_ptr<IGOIPCMessage> getMessage() { return mRecvMessage; }
    void processRead();
    void processWrite();

    bool connect(DWORD id, bool async = false);
    HANDLE getWriteEvent() { return mWriteOverlapped.hEvent; }
    HANDLE getReadEvent() { return mReadOverlapped.hEvent; }
    bool isClosed() { return mPipe == INVALID_HANDLE_VALUE; }


protected:
    enum  CONNECTION_STATE {
        READY_STATE,
        CONNECTING_STATE,
        READING_HEADER_STATE,
        READING_BODY_STATE,
        WRITING_STATE,
        WRITING_WAIT_STATE,
    };

    enum BUFFER_TYPE  {
        BUFFER_READ,
        BUFFER_WRITE,
    };

    HANDLE mPipe;
    uint32_t mId;

    // read buffer
    char *mReadBuffer;
    uint32_t mReadBufferSize;
    char *mWriteBuffer;
    uint32_t mWriteBufferSize;

    // overlapped structure
    OVERLAPPED mReadOverlapped;
    OVERLAPPED mWriteOverlapped;
    bool mIOWritePending;
    bool mIOReadPending;

    bool mAsync;
    
    // IPC message
    typedef eastl::list< eastl::shared_ptr<IGOIPCMessage> > IGOIPCMessageQueue;
    IGOIPCMessageQueue mSendMessages;
    bool mMessageReady;
    eastl::shared_ptr<IGOIPCMessage> mRecvMessage;
    IGOIPC::MessageHeader mHeader;
    CRITICAL_SECTION mCS;

    // state
    CONNECTION_STATE mReadState;
    CONNECTION_STATE mWriteState;

    char *verifyBufferSize(uint32_t size, BUFFER_TYPE type);

    // read/write handlers
    void handleReadHeader();
    void handleReadBody();
    void handleWrite();
    //bool hasMessageAsync();
};

#elif defined(ORIGIN_MAC) //////////////////////////////////////////////////////////////////////

#include "IGOIPCBuffer.h"
#include "IGOIPCImageBin.h"
#include "IGOIPCUtils.h"

// This class contains common behaviors for the server/each separate connection to a code-injected game.
class IGOIPCConnection
{
public:
    // MAKE SURE THIS IS LARGE ENOUGH TO HANDLE ALL OUR IPC MESSAGES!
    static const size_t MAX_MSG_SIZE = Origin::Mac::MachPort::MAX_MSG_SIZE;
    
    
public:
    IGOIPCConnection(const char* serviceName, size_t messageQueueSize);
    virtual ~IGOIPCConnection();
    
    // Those are used by the server/dummy base implementations
	virtual bool start() { return true; };
	virtual void stop(void *userData = NULL) { };

    // Connect to a client/game injected with IGO (= only called from server/async is true)
    bool connect(ConnectionID id, Origin::Mac::MachPort writePort, Origin::Mac::MachPort serverPort);

    // Get the port used to receive messages so that it can be added to a portset for "select" behavior.
    Origin::Mac::MachPort* getReadChannel() { return mReadPort; }
    
    // Get the port used to notify the connection is dead.
    Origin::Mac::MachPort* getNotifyChannel() { return mNotifyPort; }
    
    // Used on the client/game side to check if the connection to the server/Origin client is broken.
    // If it's the case, clean up and try to connect to the server again.
    bool checkPipe(bool restart = false);

    // Return connection unique id (based on pid).
    uint32_t id() { return mId; }

    // Check if the connection died.
    bool isClosed() { return !mReadPort || !mWritePort; }
    
    // Reset the ports/shared memory used to communicate with the server.
    void cleanupComm(bool writing = false);
    

    
    // Send a message over - If mASync = true, this is used by the server (Origin client) and the messages are queued up,
    // otherwise this is used by the client (injected game) and the message is pushed immediately.
    bool send(eastl::shared_ptr<IGOIPCMessage> msg);
    
    // Send raw byte data over a mach port connection (pushing/timeout = 0).
    int sendRaw(const void *msg, int size);
    
    // Receive raw byte data from a mach port connection (polling/timeout = 0).
    int recvRaw(void* buffer, size_t bufferSize);
    
    
    // Prepare data received on the server from the client (injected code)
    bool processRead(void* buffer, size_t dataSize);
    
    // Check/process the message if this is an IPC system internal message. Returns true if it was handled, otherwise false.
    bool handleInternalMessage();

    // Push any server pending messages to the client (injected code)
    bool processWrite();
    
    // Package last return message for dispatching.
    eastl::shared_ptr<IGOIPCMessage> getMessage() { return mRecvMessage; }
    
    // Returns a bitmap stored in shared memory.
    IGOIPCImageBin::AutoReleaseContent GetImageData(int ownerId, ssize_t dataOffset, size_t size);
    
    
protected:
    IGOIPCImageBin* mImageBuffer;           // shared memory used to transfer window bitmaps

    Origin::Mac::MachPort* mReadPort;       // port used to listen for messages
    Origin::Mac::MachPort* mWritePort;      // port used to write messages back to the server
    Origin::Mac::MachPort* mNotifyPort;     // port used to receive async notification of dead names (client connection dead)
    Origin::Mac::MachPort* mServerPort;     // port used to notify there are pending messages to send

    eastl::string mServiceName;             // unique id for the service to be found on the machine
    size_t mMessageQueueSize;               // size of the shared memory buffer used to store messages to send

    uint32_t mId;                           // unique connection id (based on pid)
    bool mAsync;                            // true = running in a separatre thread(server), false = synchronuous behavior (client/code-injected game)
    
    // IPC message
    typedef eastl::list<eastl::shared_ptr<IGOIPCMessage> > IGOIPCMessageQueue;
    IGOIPCMessageQueue mSendMessages;       // list of pending messages (used by server/mAsync = true)
    CRITICAL_SECTION mCS;                   // lock used to manage pending messages across two threads
    
    eastl::shared_ptr<IGOIPCMessage> mRecvMessage;  // last receive message
};

#endif // ORIGIN_MAC

#endif // IGOIPCCONNECTION_H
