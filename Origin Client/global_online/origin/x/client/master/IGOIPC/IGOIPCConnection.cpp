///////////////////////////////////////////////////////////////////////////////
// IGOIPCConnection.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPC.h"
#include "IGOIPCConnection.h"
#include "IGOIPCMessage.h"


#if defined(ORIGIN_PC)

#pragma warning(disable: 4189)
#pragma warning(disable: 4996)    // 'sprintf': name was marked as #pragma deprecated
#pragma warning(disable: 4995)    // 'wsprintfW': name was marked as #pragma deprecated

IGOIPCConnection::IGOIPCConnection() :
    mPipe(INVALID_HANDLE_VALUE),
    mId(0),
    mReadBuffer(NULL),
    mReadBufferSize(0),
    mWriteBuffer(NULL),
    mWriteBufferSize(0),
    mIOWritePending(false),
    mIOReadPending(false),
    mAsync(false),
    mReadState(READY_STATE),
    mWriteState(READY_STATE)
{
    InitializeCriticalSection(&mCS);

    mSendMessages.clear();

    mRecvMessage = eastl::shared_ptr<IGOIPCMessage>(new IGOIPCMessage(IGOIPC::MSG_UNKNOWN));

    ZeroMemory(&mReadOverlapped, sizeof(mReadOverlapped));
    mReadOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ZeroMemory(&mWriteOverlapped, sizeof(mWriteOverlapped));
	mWriteOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	mHeader.length = 0;
	mHeader.type = IGOIPC::MSG_UNKNOWN;

}

IGOIPCConnection::~IGOIPCConnection()
{
    EnterCriticalSection(&mCS);

    if (mReadBuffer)
    {
        free(mReadBuffer);
        mReadBuffer = NULL;
        mReadBufferSize = 0;
    }

    if (mWriteBuffer)
    {
        free(mWriteBuffer);
        mWriteBuffer = NULL;
        mWriteBufferSize = 0;
    }

    if (mPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(mPipe);
        mPipe = INVALID_HANDLE_VALUE;
    }

    if (mReadOverlapped.hEvent)
    {
        CloseHandle(mReadOverlapped.hEvent);
        mReadOverlapped.hEvent = NULL;
    }

    if (mWriteOverlapped.hEvent)
    {
        CloseHandle(mWriteOverlapped.hEvent);
        mWriteOverlapped.hEvent = NULL;
    }

    mSendMessages.clear();
    LeaveCriticalSection(&mCS);
    
    DeleteCriticalSection(&mCS);
}

bool IGOIPCConnection::send(eastl::shared_ptr<IGOIPCMessage> msg)
{
    EnterCriticalSection(&mCS);
    if (mAsync)
    {
        if (msg.get()->type() == IGOIPC::MSG_WINDOW_UPDATE)
        {
            uint32_t windowId;
            bool visible;
            uint8_t alpha;
            int16_t x, y;
            uint16_t width, height;
            uint32_t flags;
            const char *data;
            int size;
            IGOIPC::instance()->parseMsgWindowUpdate(msg.get(), windowId, visible, alpha, x, y, width, height, data, size, flags);

            IGOIPCMessageQueue::iterator iter = mSendMessages.begin();
            while(size>0 && iter != mSendMessages.end())    // if size is 0, do not filter...
            {
                IGOIPCMessage *_msg = (*iter).get();

                uint32_t _windowId;
                bool _visible;
                uint8_t _alpha;
                int16_t _x, _y;
                uint16_t _width, _height;
                uint32_t _flags;
                const char *_data;
                int _size;
                bool isWindowsUpdateMsg = IGOIPC::instance()->parseMsgWindowUpdate(_msg, _windowId, _visible, _alpha, _x, _y, _width, _height, _data, _size, _flags);

                if (isWindowsUpdateMsg && (_windowId == windowId) && _size>0)
                {
                    iter=mSendMessages.erase(iter);
                    IGO_TRACEF("IGOIPCConnection::send: filter saved: %i bytes", _size);
                }
                else
                    ++iter;
            }
        }
        IGO_TRACEF("IGOIPCConnection::send: messages in queue: %i", mSendMessages.size());
        mSendMessages.push_back(msg);
        
        IGO_ASSERT(!mSendMessages.empty());
        if (mWriteState == WRITING_STATE || mWriteState == READY_STATE)
        {
            mWriteState = WRITING_STATE;
            SetEvent(mWriteOverlapped.hEvent);
        }

        LeaveCriticalSection(&mCS);
        return true;
    }

    bool retval = true;
    IGOIPC::MessageHeader hdr;
    hdr.type = msg.get()->type();
    hdr.length = msg.get()->size();
    int result = sendRaw(&hdr, sizeof(hdr));

    if (result != sizeof(hdr))
    {
        retval = false;
    }

    if (retval && msg.get()->size() > 0)
    {
        result = sendRaw(msg.get()->data(), msg.get()->size());
        if (result != (int) msg.get()->size())
            retval = false;
    }

    LeaveCriticalSection(&mCS);
    
    if (!retval)
    {
        IGO_TRACE("IGOIPCConnection::send failed!");
    }
    return retval;
}

int IGOIPCConnection::sendRaw(const void *msg, int size)
{
    DWORD size_written = 0;
    if (mPipe != INVALID_HANDLE_VALUE && WriteFile(mPipe, msg, size, &size_written, NULL))
        return size_written;
    return 0;
}

int IGOIPCConnection::recvRaw(void *msg, int size)
{
    DWORD size_written = 0;
    if (mPipe != INVALID_HANDLE_VALUE && ReadFile(mPipe, msg, size, &size_written, NULL))
        return size_written;

    return 0;
}

bool IGOIPCConnection::hasMessage()
{
    DWORD availableBytes = 0;
    if (mPipe != INVALID_HANDLE_VALUE && PeekNamedPipe(mPipe, NULL, 0, NULL, NULL, &availableBytes))
        return (availableBytes > 0);

    DWORD error = GetLastError();
    if (mPipe == INVALID_HANDLE_VALUE || error == ERROR_BROKEN_PIPE || error == ERROR_BAD_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
    {
        // restart server
        IGO_TRACE("IPC disconnected. Closing Pipe...");
        // do not restart the pipe here, that's done in "checkPipe" !!!
        stop();
    }

    return false;
}

bool IGOIPCConnection::checkPipe()
{
    DWORD availableBytes = 0;
    SetLastError(0);
    if (mPipe == INVALID_HANDLE_VALUE || !PeekNamedPipe(mPipe, NULL, 0, NULL, NULL, &availableBytes))
    {
        DWORD error = GetLastError();
        if (mPipe == INVALID_HANDLE_VALUE || error == ERROR_BROKEN_PIPE || error == ERROR_BAD_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
        {
            // restart server
            IGO_TRACEF("IPC disconnected. Restarting... (%i / %x)", error, error);
            stop();
            start();

            return false;
        }
    }
    return true;
}

bool IGOIPCConnection::connect(DWORD id, bool async)
{
    WCHAR pNameStr[128]={0};

    mId = id;

    wsprintf(pNameStr, L"\\\\.\\pipe\\IGO_pipe_%d", mId);

    IGO_TRACEF("WaitNamedPipe begin (%S)\n", pNameStr);
    if (!WaitNamedPipe(pNameStr, NMPWAIT_USE_DEFAULT_WAIT))
    {
        IGO_TRACEF("WaitNamedPipe end failed (%S)\n", pNameStr);
        //IGO_ASSERT(0);
        return false;
    }
    IGO_TRACEF("WaitNamedPipe end succeeded(%S)\n", pNameStr);

    mAsync = async;

    mPipe = CreateFile( 
        pNameStr,       // pipe name 
        GENERIC_READ |  // read and write access 
        GENERIC_WRITE, 
        0,              // no sharing 
        NULL,           // default security attributes
        OPEN_EXISTING,  // opens existing pipe 
        FILE_FLAG_OVERLAPPED, // nonblocking
        NULL);          // no template file 

    if (mPipe != INVALID_HANDLE_VALUE)
    {
        DWORD mode = PIPE_READMODE_MESSAGE|PIPE_WAIT;
        SetNamedPipeHandleState(mPipe, &mode, NULL, NULL);
    
        // try to start reading
        if (mAsync)
        {
            mReadState = READING_HEADER_STATE;
            mIOReadPending = false;

            handleReadHeader();
        }
    }
    else
    {
        IGO_TRACE("IGOIPCConnection::connect failed\n");
    }

    return mPipe != INVALID_HANDLE_VALUE;
}


char *IGOIPCConnection::verifyBufferSize(uint32_t size, BUFFER_TYPE type)
{
	switch (type)
	{
	case BUFFER_READ:
		{
			if (mReadBufferSize < size)
			{
				char * temp = (char *)realloc(mReadBuffer, size);
				if(temp == NULL) {
					IGO_ASSERT(false);
					return NULL;
				}
				mReadBuffer = temp;
				mReadBufferSize = size;
			}
			return mReadBuffer;
		}
		break;

	case BUFFER_WRITE:
		{
			if (mWriteBufferSize < size)
			{
				char * temp = (char *)realloc(mWriteBuffer, size);
				if(temp == NULL) {
					IGO_ASSERT(false);
					return NULL;
				}
				mWriteBuffer = temp;
				mWriteBufferSize = size;
			}
			return mWriteBuffer;
		}
		break;
	}

    IGO_ASSERT(false);
    return NULL;
}

// read/write handlers
void IGOIPCConnection::handleReadHeader()
{
    //IGO_TRACE("READING_HEADER_STATE");
    mMessageReady = false;
    DWORD bytesRead = 0;
    ZeroMemory(&mHeader, sizeof(mHeader));
    BOOL result = (mPipe != INVALID_HANDLE_VALUE);
    
    if (result)
        result = ReadFile(mPipe, &mHeader, sizeof(mHeader), &bytesRead, &mReadOverlapped);

    if (result && bytesRead>=sizeof(mHeader))
    {
        mIOReadPending = false;
        mReadState = READING_BODY_STATE;
        SetEvent(mReadOverlapped.hEvent);
        //IGO_TRACE("\t-> READING_BODY_STATE");
    }
    else
    {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            //IGO_TRACE("\t -> PENDING");
            mIOReadPending = true;
        }
        else
        {
            IGO_TRACEF("ERROR = %d", error);
            if (error == ERROR_MORE_DATA)
                mIOReadPending = false;
            else
            {
                CloseHandle(mPipe);
                mPipe = INVALID_HANDLE_VALUE;
            }
        }
    }
}

void IGOIPCConnection::handleReadBody()
{
    //IGO_TRACEF("READING_BODY_STATE type:%d length:%d", mHeader.type, mHeader.length);

    DWORD bytesRead = 0;
    char *buffer = verifyBufferSize(mHeader.length, BUFFER_READ);
    BOOL result = true;
    if (!buffer)
        result = false;
    
    if (result)
        result = (mPipe != INVALID_HANDLE_VALUE);

    if (mHeader.length > 0)
        result = result && ReadFile(mPipe, buffer, mHeader.length, &bytesRead, &mReadOverlapped);
    else
        result = true;

    if (result && bytesRead>=mHeader.length)
    {
        mIOReadPending = false;
        mReadState = READING_HEADER_STATE;
        //IGO_TRACEF("\t-> READING_HEADER_STATE");
        mMessageReady = true;
        mRecvMessage.get()->setType(mHeader.type);
        mRecvMessage.get()->setData(buffer, eastl::min(mReadBufferSize, mHeader.length));
        SetEvent(mReadOverlapped.hEvent);
    }
    else
    {
        mMessageReady = false;
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            //IGO_TRACE("\t -> PENDING");
            mIOReadPending = true;
        }
        else
        {
            IGO_TRACEF("ERROR = %d", error);
            if (error == ERROR_MORE_DATA)
                mIOReadPending = false;
            else
            {
                CloseHandle(mPipe);
                mPipe = INVALID_HANDLE_VALUE;
            }
        }
    }
}

void IGOIPCConnection::handleWrite()
{
    EnterCriticalSection(&mCS);
    if (mSendMessages.empty())
    {
        mIOWritePending = false;
        mWriteState = READY_STATE;
        ResetEvent(mWriteOverlapped.hEvent);
        //IGO_TRACE("\t-> READY_STATE");
        LeaveCriticalSection(&mCS);
        return;
    }

    IGOIPCMessage* msg = mSendMessages.front().get();

    BOOL result;
    IGOIPC::MessageHeader header;
    uint32_t size = sizeof(header) + msg->size();
    char *buffer = verifyBufferSize(size, BUFFER_WRITE);
    if (!buffer)
        result = false;
    else
    {
        header.type = msg->type();
        header.length = msg->size();
        memcpy(buffer, &header, sizeof(header));
        memcpy(buffer + sizeof(header), msg->data(), msg->size());

        DWORD bytesWritten;
        if (mPipe != INVALID_HANDLE_VALUE)
            result = WriteFile(mPipe, buffer, size, &bytesWritten, &mWriteOverlapped);
        else
            result = 0;
    }
    mSendMessages.pop_front();    // delete message!

    if (result)
    {
        mIOWritePending = false;
        SetEvent(mWriteOverlapped.hEvent);
        //IGO_TRACEF("\t-> WRITING_STATE (%d)", GetCurrentThreadId());
    }
    else
    {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
            mWriteState = WRITING_WAIT_STATE;
            //IGO_TRACE("\t -> PENDING");
            mIOWritePending = true;
        }
        else
        {
            IGO_TRACEF("ERROR = %d", error);
            if (error == ERROR_MORE_DATA)
                mIOWritePending = false;
            else
            {
                CloseHandle(mPipe);
                mPipe = INVALID_HANDLE_VALUE;
            }
        }
    }

    LeaveCriticalSection(&mCS);
}


void IGOIPCConnection::processRead()
{
    //IGO_TRACE("IGOIPCConnection::processRead()");
    if (mIOReadPending)
    {
        DWORD size;
        BOOL result = (mPipe != INVALID_HANDLE_VALUE);
        
        if (result)
        {
            result = GetOverlappedResult( 
                                        mPipe,                // handle to pipe 
                                        &mReadOverlapped,        // OVERLAPPED structure 
                                        &size,                // bytes transferred 
                                        FALSE);                // do not wait 
        }
        // state change
        switch (mReadState)
        {
        case READING_HEADER_STATE:
            {
                if (!result)
                {
                    CloseHandle(mPipe);
                    mPipe = INVALID_HANDLE_VALUE;
                    return;
                }
                mReadState = READING_BODY_STATE;
                mIOReadPending = false;
                //IGO_TRACE("W\t-> READING_BODY_STATE");
            }
            break;

        case READING_BODY_STATE:
            {
                if (!result)
                {
                    CloseHandle(mPipe);
                    mPipe = INVALID_HANDLE_VALUE;
                    return;
                }
                mReadState = READING_HEADER_STATE;
                //IGO_TRACEF("W\t-> READING_HEADER_STATE");
                mMessageReady = true;
                mIOReadPending = false;
                mRecvMessage.get()->setType(mHeader.type);
                mRecvMessage.get()->setData(mReadBuffer, eastl::min(mReadBufferSize, mHeader.length));
                SetEvent(mReadOverlapped.hEvent);
                return;
            }
            break;
        }
    }

    switch (mReadState)
    {
    case READING_HEADER_STATE:
        handleReadHeader();
        break;

    case READING_BODY_STATE:
        handleReadBody();
    }
}

void IGOIPCConnection::processWrite()
{
    //IGO_TRACE("IGOIPCConnection::processWrite()");
    if (mIOWritePending)
    {
        switch (mWriteState)
        {
        case WRITING_WAIT_STATE:
            {
                mWriteState = WRITING_STATE;
                mIOWritePending = false;
                //IGO_TRACEF("W\t-> WRITING_STATE (%d)", GetCurrentThreadId());
            }
            break;
        }
    }

    handleWrite();
}

#elif defined(ORIGIN_MAC) // MAC OSX ================================================================

IGOIPCConnection::IGOIPCConnection(const char* serviceName, size_t messageQueueSize) :
mImageBuffer(NULL),
mReadPort(NULL),
mWritePort(NULL),
mNotifyPort(NULL),
mServerPort(NULL),
mServiceName(serviceName),
mMessageQueueSize(messageQueueSize),
mId(0),
mAsync(false)
{
    mSendMessages.clear();
    mRecvMessage = eastl::shared_ptr<IGOIPCMessage>(new IGOIPCMessage(IGOIPC::MSG_UNKNOWN));
    
    InitializeCriticalSection(&mCS);
}

IGOIPCConnection::~IGOIPCConnection()
{
    cleanupComm();
    
    // We clear the server port here to avoid a race condition between the server thread and the Origin client
    delete mServerPort;
    mServerPort = NULL;
    
    DeleteCriticalSection(&mCS);
}

// Reset the ports/shared memory used to communicate with the server.
void IGOIPCConnection::cleanupComm(bool writing)
{
    // Only lock critical section if this is not called from one of our "write" methods
    if (!writing)
    {
        EnterCriticalSection(&mCS);
        
        mSendMessages.clear();
        LeaveCriticalSection(&mCS);
    }
    
    else
    {
        mSendMessages.clear();
    }
    
    delete mReadPort;
    delete mWritePort;
    delete mNotifyPort;
    delete mImageBuffer;
    
    mReadPort = NULL;
    mWritePort = NULL;
    mNotifyPort = NULL;
    mImageBuffer = NULL;
}

// Send a message over - If mASync = true, this is used by the server (Origin client) and the messages are queued up,
// otherwise this is used by the client (injected game) and the message is pushed immediately.
bool IGOIPCConnection::send(eastl::shared_ptr<IGOIPCMessage> msg)
{
    EnterCriticalSection(&mCS);
    
    bool retVal = true;
    
    if (mWritePort)
    {
        if (mAsync) // Is this coming from the Origin client? -> yes, buffer the message and notify the server...
        {
            int64_t replaceOffset = 0;
            
            // Special processing when sending bitmaps -> we use a message to notify of the bitmap but we store the bitmap
            // itself on the shared memory region
            if (msg.get()->type() == IGOIPC::MSG_WINDOW_UPDATE)
            {
                uint32_t windowId;
                bool visible;
                uint8_t alpha;
                int16_t x, y;
                uint16_t width, height;
                uint32_t flags;
                const char *data;
                int size;
                
                // Parse the separate elements of the message so that we can detect a pending bitmap message from the same window source
                // -> we can then reuse the location in shared memory/not send unnecessary duplicate messages
                IGOIPC::instance()->parseMsgWindowUpdate(msg.get(), windowId, visible, alpha, x, y, width, height, data, size, flags);
                
                // So go through queued messages...
                IGOIPCMessageQueue::iterator iter = mSendMessages.begin();
                while (size > 0 && iter != mSendMessages.end())    // if size is 0, do not filter...
                {
                    IGOIPCMessage *_msg = (*iter).get();
                    
                    uint32_t _windowId;
                    bool _visible;
                    uint8_t _alpha;
                    int16_t _x, _y;
                    uint16_t _width, _height;
                    uint32_t _flags;
                    int64_t _dataOffset;
                    int _size;
                    
                    bool isWindowsUpdateMsg = IGOIPC::instance()->parseMsgWindowUpdateEx(_msg, _windowId, _visible, _alpha, _x, _y, _width, _height, _dataOffset, _size, _flags);
                    
                    // Pending duplicate window update message?
                    if (isWindowsUpdateMsg && _windowId == windowId && _size > 0)
                    {
                        // Yep, remove that message/grab the already reserved location in shared memory
                        iter = mSendMessages.erase(iter);
                        replaceOffset = _dataOffset;
                    }
                    else
                        ++iter;
                }
                
                bool dataAllocated = false;
                
                // Is this a bitmap?
                if (size > 0)
                {
                    // Are we replacing a pending message?
                    if (replaceOffset != 0)
                        dataAllocated = mImageBuffer->Replace(windowId, data, size, &replaceOffset);
                    
                    else // Nope, need to append new bitmap to the queue on shared memory
                        dataAllocated = mImageBuffer->Add(windowId, data, size, &replaceOffset);
                    
                    if (dataAllocated)
                    {
                        // Create an WINDOW_UPDATE_EX message that specifies the offset of the bitmap in shared memory
                        eastl::shared_ptr<IGOIPCMessage> msgEx(IGOIPC::instance()->createMsgWindowUpdateEx(windowId, visible, alpha, x, y, width, height, replaceOffset, size, flags));
                        mSendMessages.push_back(msgEx);
                    }
                    
                    else
                    {
                        // This may happen a lot while the game is not running its render loop (ie loading screen)
                        //IGO_TRACE("IGOIPCConnection::send: unable to allocate space in image buffer\n");
                        retVal = false;
                    }
                }
                
                else
                {
                    // This window update message is not to specify a bitmap but simply new window parameters (size, location, blending, etc...)
                    eastl::shared_ptr<IGOIPCMessage> msgEx(IGOIPC::instance()->createMsgWindowUpdateEx(windowId, visible, alpha, x, y, width, height, replaceOffset, size, flags));
                    mSendMessages.push_back(msgEx);
                }
            }
            
            else // This is not a window update message
            {
                mSendMessages.push_back(msg);
                IGO_ASSERT(!mSendMessages.empty());
            }
            
            // We need to signal the server that we have some data available for writting
            if (retVal)
            {
                int notUsed = 0;
                mServerPort->Send(&notUsed, sizeof(notUsed), 0, NULL, NULL); 
            }
        }
        
        else
        {
            // Message from the client (injected game) -> directly push it to the server
            char body[Origin::Mac::MachPort::MAX_MSG_SIZE];
            
            size_t msgSize = msg.get()->size() + sizeof(IGOIPC::MessageHeader);
            if (msgSize <= sizeof(body))
            {
                IGOIPC::MessageHeader hdr;
                hdr.type = msg.get()->type();
                hdr.length = msg.get()->size();
                
                memcpy(body, &hdr, sizeof(hdr));
                if (msg.get()->size() > 0)
                    memcpy(body + sizeof(hdr), msg.get()->data(), msg.get()->size());
                
                retVal = sendRaw(body, msgSize) != 0;
            }
            
            else
            {
                IGO_TRACEF("Skipping large message (%ld/%ld)", msgSize, sizeof(body));
                retVal = false;
            }
        }
    }
    
    LeaveCriticalSection(&mCS);
    
    return retVal;
}

// Returns a bitmap stored in shared memory.
IGOIPCImageBin::AutoReleaseContent IGOIPCConnection::GetImageData(int ownerId, ssize_t dataOffset, size_t size)
{
    return mImageBuffer->Get(this, ownerId, dataOffset, size);
}

// Send raw byte data over a mach port connection (pushing/timeout = 0).
int IGOIPCConnection::sendRaw(const void* msg, int size)
{
    if (mWritePort)
    {
        Origin::Mac::MachPort::CommResult result = mWritePort->Send(msg, size, 0, NULL, NULL);
        if (result == Origin::Mac::MachPort::CommResult_CONNECTION_DEAD)
        {
            IGO_TRACE("IGOIPCConnection::sendRaw - detected dead write port, closing connection");
            cleanupComm(true);
        }
        
        return result == Origin::Mac::MachPort::CommResult_SUCCEEDED;
    }
    
    return 0;    
}

// Receive raw byte data from a mach port connection (polling/timeout = 0).
int IGOIPCConnection::recvRaw(void* buffer, size_t bufferSize)
{
    if (mReadPort)
    {
        size_t dataSize;
        Origin::Mac::MachPort::CommResult result = mReadPort->Recv(buffer, bufferSize, 0, &dataSize, NULL, NULL, NULL);
        if (result == Origin::Mac::MachPort::CommResult_CONNECTION_DEAD)
        {
            IGO_TRACE("IGOIPCConnection::recvRaw - detected dead read port!!!, closing connection");
            cleanupComm();
        }
        
        return result == Origin::Mac::MachPort::CommResult_SUCCEEDED;
    }
    
    return 0;
}

// Used on the client/game side to check if the connection to the server/Origin client is broken.
// If it's the case, clean up and try to connect to the server again.
bool IGOIPCConnection::checkPipe(bool restart)
{
    static bool prevCheckPipe = true; // this is to limit logging
    
    if (isClosed())
    {
        if (prevCheckPipe)
        {
            if (restart)
            {
                IGO_TRACE("Detected broken connection, attempting to repair it");
            }
            
            else
            {
                IGO_TRACE("Detected broken connection, closing");
                prevCheckPipe = false;
            }
        }
        
        stop();
        
        if (restart)
        {
            start();
            if (restart && prevCheckPipe)
            {
                if (isClosed())
                {
                    IGO_TRACE("Repair failed... will keep on trying every 10 seconds...");
                }
                
                else
                {
                    IGO_TRACE("Repair succeeded");
                }
                
                prevCheckPipe = false;
            }
        }
        
        return false;
    }
    
    else
        if (!prevCheckPipe)
            prevCheckPipe = true;
    
    return true;
}

// Connect to a client/game injected with IGO (= only called from server/async is true)
bool IGOIPCConnection::connect(ConnectionID id, Origin::Mac::MachPort writePort, Origin::Mac::MachPort serverPort)
{
    mId = id.pid;
    
    char sharedMemName[MAX_PATH];    
    snprintf(sharedMemName, sizeof(sharedMemName), "%s%s_%d", IGOIPC_SHARED_MEMORY_BASE_NAME, mServiceName.c_str(), mId);
    mImageBuffer = new IGOIPCImageBin(sharedMemName, mMessageQueueSize, IGOIPCBuffer::OpenOptions_ATTACH, IGOIPCBuffer::BufferOptions_USE_LOCAL_LOCK);
    if (!mImageBuffer->IsValid())
    {
        IGO_TRACEF("IGOIPCConnection::start - failed to connect to image shared memory for pid %d (%d)\n", id.pid, errno);
        cleanupComm();
        
        return false;
    }
    
    
    bool setup = false;
    
    // Create a new communication channel
    mReadPort = new Origin::Mac::MachPort();
    if (mReadPort->IsValid())
    {
        // Forward write rights to the write port we got from the server
        if(mReadPort->ForwardSendRights(writePort, NULL, 0, 2000) == Origin::Mac::MachPort::CommResult_SUCCEEDED)
        {
            // K, we're good to go -> copy over the port to send data and the server port used to notify the server we have
            // pending messages            
            mWritePort = new Origin::Mac::MachPort(writePort);
            mServerPort = new Origin::Mac::MachPort(serverPort);
            
            // Create port to watch for client dead connection
            mNotifyPort = new Origin::Mac::MachPort(-1);
            mNotifyPort->Watch(*mWritePort);
            
#ifdef SHOW_MACHPORT_SETUP
            char portInfo[256];
            
            IGO_TRACE("IGO New server connection:");
            
            mReadPort->ToString(portInfo, sizeof(portInfo));
            IGO_TRACEF("- Local port: %s", portInfo);
            
            mWritePort->ToString(portInfo, sizeof(portInfo));
            IGO_TRACEF("- Remote port: %s", portInfo);
            
            mNotifyPort->ToString(portInfo, sizeof(portInfo));
            IGO_TRACEF("- Notify port: %s", portInfo);
            
            mServerPort->ToString(portInfo, sizeof(portInfo));
            IGO_TRACEF("- Internal server notify port: %s", portInfo);
#endif
            setup = true;
        }
        
        else
        {
            IGO_TRACE("IGOIPCConnection::connect - failed to forward rights to client (injected game)");
        }
    }
    
    else
    {
        IGO_TRACE("IGOIPCConnection::connect - failed to allocate r/w port");
    }
    
    if (!setup)
    {
        cleanupComm();
        return false;
    }
    
    mAsync = true; // always for the server
    
    IGO_TRACEF("IGOIPCConnection::connect - Successful connection with pid=%d", id.pid);
    
    return true;
}

// Prepare data received on the server from the client (injected code)
bool IGOIPCConnection::processRead(void* buffer, size_t dataSize)
{
    IGOIPC::MessageHeader* header = reinterpret_cast<IGOIPC::MessageHeader*>(buffer);
    if (sizeof(IGOIPC::MessageHeader) + header->length == dataSize)
    {
        mRecvMessage.get()->setType(header->type);
        mRecvMessage.get()->setData(reinterpret_cast<const char*>(header + 1), header->length);
        
        // Handle IPC system internal messages
        return !handleInternalMessage();
    }
    
    IGO_TRACE("IGOIPCConnection::processRead - Received data size doesn't match message length");
    return false;
}

// Check/process the message if this is an IPC system internal message. Returns true if it was handled, otherwise false.
bool IGOIPCConnection::handleInternalMessage()
{
    bool retVal = false;
    
    switch (mRecvMessage.get()->type())
    {
        case IGOIPC::MSG_RELEASE_BUFFER:
        {
            int ownerId;
            int64_t offset;
            int64_t dataSize;
            
            if (IGOIPC::instance()->parseMsgReleaseBuffer(mRecvMessage.get(), ownerId, offset, dataSize))
            {
#ifdef SHOW_IPC_MESSAGES
                IGO_TRACEF("IGOIPCConnection::handleInternalMessage - Got MSG_RELEASE_BUFFER %d, %ld, %ld", ownerId, offset, dataSize);
#endif
                if (mImageBuffer)
                    mImageBuffer->Release(ownerId, offset, dataSize);
            }
            
            retVal = true;
        }
            break;
            
        default:
            break;
    }
    
    return retVal;
}

// Push any server pending messages to the client (injected code)
bool IGOIPCConnection::processWrite()
{   
    if (!mWritePort)
        return false;
    
    
    bool retVal = false;
    
    EnterCriticalSection(&mCS);
    
    if (!mSendMessages.empty())
    {
        char buffer[Origin::Mac::MachPort::MAX_MSG_SIZE];
        
        IGOIPCMessage* refMsg = mSendMessages.front().get();
        uint32_t dataSize = sizeof(IGOIPC::MessageHeader) + refMsg->size();
        
        if (dataSize <= sizeof(buffer))
        {
            IGOIPC::MessageHeader header;
            header.type = refMsg->type();
            header.length = refMsg->size();
            
            memcpy(buffer, &header, sizeof(header));
            memcpy(buffer + sizeof(header), refMsg->data(), refMsg->size());
            
            Origin::Mac::MachPort::CommResult result = mWritePort->Send(buffer, dataSize, 0, NULL, NULL);
            if (result == Origin::Mac::MachPort::CommResult_CONNECTION_DEAD)
            {
                IGO_TRACE("IGOIPCConnection::processWrite - Detected dead port, closing connection");
                cleanupComm(true);
            }
            
            retVal = result == Origin::Mac::MachPort::CommResult_SUCCEEDED;
        }
        
        else
        {
            IGO_TRACEF("IGOIPCConnection::processWrite - Skipping truncated message (type=%d)", refMsg->type());
        }
        
        // Remove the message whatever the end result was (careful: we may have cleared the buffer already if we
        // got a dead connection while trying to send the message)
        if (!mSendMessages.empty())
        {
            mSendMessages.pop_front();
        }
    }
    
    LeaveCriticalSection(&mCS);
    
    return retVal;
}

#endif // ORIGIN_MAC
