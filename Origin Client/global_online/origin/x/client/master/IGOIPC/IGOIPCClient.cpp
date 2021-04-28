///////////////////////////////////////////////////////////////////////////////
// IGOIPCClient.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPC.h"
#include "IGOIPCClient.h"
#include "IGOIPCMessage.h"

// detect concurrencies - assert if our mutex is already locked
#ifdef _DEBUG_MUTEXES
#define DEBUG_MUTEX_TEST(x){\
bool s=x.TryLock();\
if(s)\
x.Unlock();\
IGO_ASSERT(s==true);\
}
#else
#define DEBUG_MUTEX_TEST(x) void(0)
#endif

#if defined(ORIGIN_PC)

#include <windows.h>
#include <Aclapi.h>
#include <Sddl.h>
#pragma warning(disable: 4189)
#pragma warning(disable: 4996)    // 'sprintf': name was marked as #pragma deprecated
#pragma warning(disable: 4995)    // 'wsprintfW': name was marked as #pragma deprecated

IGOIPCClient::IGOIPCClient() :
    mHandler(NULL),
    mConnectionCreationTime(0)
{
    mPipe = INVALID_HANDLE_VALUE;
}


IGOIPCClient::~IGOIPCClient()
{
    stop();
}

volatile static bool isIGOIPCClientPipeConnectionHanging = false;
volatile static bool wasIGOIPCClientPipeConnectionAborted = false;

static DWORD WINAPI IGOIPCClientPipeConnectionHangDetectionThread(LPVOID *lpParam)
{
    int counter = 160; // give the game and Origin max. 4000 ms to connect, then abort...
    while(isIGOIPCClientPipeConnectionHanging && counter>=0)
    {
        if (counter == 0)
        {
            wasIGOIPCClientPipeConnectionAborted = true;
            // fake connection to unblock ConnectNamedPipe
            IGOIPCConnection *tmp = new IGOIPCConnection();
            tmp->connect(static_cast<DWORD>(reinterpret_cast<size_t>(lpParam)), true);
            delete tmp;
            return 0;
        }
        else
        {
            Sleep(25);
        }
        counter--;
    }
    wasIGOIPCClientPipeConnectionAborted = false;
    return 0;
}
  
bool IGOIPCClient::start()
{
    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    isIGOIPCClientPipeConnectionHanging = false;

    if (mPipe != INVALID_HANDLE_VALUE)
        return false;
        
    // we only try to reconnect every 10 seconds, if the connection was aborted!
    if (wasIGOIPCClientPipeConnectionAborted && ((GetTickCount() - mConnectionCreationTime) < 10000)) 
    {
        return false;    // do nothing...
    }
    
    mConnectionCreationTime = GetTickCount();
    
    HANDLE hThread = INVALID_HANDLE_VALUE;

    wasIGOIPCClientPipeConnectionAborted = false;

    mId = GetCurrentProcessId();

    WCHAR pNameStr[128] = {0};

    wsprintf(pNameStr, L"\\\\.\\pipe\\IGO_pipe_%d", mId);


	// Create the named pipe with the proper DACL for Windows
	SECURITY_DESCRIPTOR   sd;
	SECURITY_ATTRIBUTES   sa;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = &sd;

    // Define the SDDL for the DACL. This example sets the following access:
    //      Built-in guests are denied all access.
    //      Anonymous Logon is denied all access.
    //      Authenticated Users are allowed read/write/execute access.
    //      Administrators are allowed full control.
    // Modify these values as needed to generate the proper
    // DACL for your application.

    wchar_t* dacl = L"D:"                   // Discretionary ACL
                L"(D;OICI;GA;;;BG)"     // Deny access to Built-in Guests
                L"(D;OICI;GA;;;AN)"     // Deny access to Anonymous Logon
                L"(A;OICI;GRGWGX;;;AU)" // Allow read/write/execute to Authenticated Users
                L"(A;OICI;GA;;;BA)";    // Allow full control to Administrators

    ConvertStringSecurityDescriptorToSecurityDescriptor(dacl, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);
    
    mPipe = CreateNamedPipe(pNameStr, PIPE_ACCESS_DUPLEX, 
        PIPE_TYPE_MESSAGE|PIPE_WAIT, 1, 512, 512, 2000/*2 second timeout*/, &sa);
    
    if (mPipe != INVALID_HANDLE_VALUE)
    {
        // Post the message to the server
        // See IGOIPCServer for more info
        HWND hHwnd = FindWindowEx(HWND_MESSAGE, NULL, IGOIPC_WINDOW_CLASS, IGOIPC_WINDOW_TITLE);
        if (hHwnd)
        {
            // if PosetMessage fails, ConnectNamedPipe will hang forever, so we create a thread, that kills the connection,
            // just in case WM_NEW_CONNECTION is not processed!!!
            if(PostMessage(hHwnd, WM_NEW_CONNECTION, mId, 0))
            {
                isIGOIPCClientPipeConnectionHanging = true;
                wasIGOIPCClientPipeConnectionAborted = false;
                hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOIPCClientPipeConnectionHangDetectionThread, (LPVOID)mId, 0, NULL);
                if (!ConnectNamedPipe(mPipe, NULL))
                {
                    if(GetLastError() != ERROR_PIPE_CONNECTED)
                    {
                        CloseHandle(mPipe);
                        mPipe = INVALID_HANDLE_VALUE;
                    }
                }
                isIGOIPCClientPipeConnectionHanging = false;
            }
        }
        else
        {
            // Origin not running? kill the pipe and retry it later...
            if (mHandler)
                mHandler->handleConnect(this);
            CloseHandle(mPipe);
            mPipe = INVALID_HANDLE_VALUE;
        }
    }

	// kill the hang detection thread, if it is still running...
	isIGOIPCClientPipeConnectionHanging = false;
	if (hThread != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(hThread, 1000) != WAIT_OBJECT_0)
			TerminateThread(hThread, 0);
		
		CloseHandle(hThread);
	}

    bool retval = !wasIGOIPCClientPipeConnectionAborted && (mPipe != INVALID_HANDLE_VALUE);

    if (retval && mHandler)
        mHandler->handleConnect(this);
    else
    {
        if (mPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mPipe);
            mPipe = INVALID_HANDLE_VALUE;
        }
    }

    return retval;
}

void IGOIPCClient::stop(void *userData)
{
    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    if (mPipe != INVALID_HANDLE_VALUE)
	{
		if (mHandler)
			mHandler->handleDisconnect(this, userData);

        CloseHandle(mPipe);
        mPipe = INVALID_HANDLE_VALUE;
    }
}

void IGOIPCClient::processMessages(void *userData)
{
    IGOIPC::MessageHeader hdr;
    hdr.length = 0;
    hdr.type = IGOIPC::MSG_UNKNOWN;

    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    while (hasMessage())
    {
        uint32_t retval = recvRaw(&hdr, sizeof(hdr));
        IGO_ASSERT(retval == sizeof(hdr));

        if (retval == sizeof(hdr))
        {
            char *buffer = verifyBufferSize(hdr.length, BUFFER_READ);
            if (hdr.length > 0 && buffer!=NULL)
            {
                retval = recvRaw(buffer, hdr.length);
                IGO_ASSERT(retval == hdr.length);
            }

			if (mHandler)
			{
				eastl::shared_ptr<IGOIPCMessage> msg (new IGOIPCMessage(hdr.type, buffer, hdr.length));
				mHandler->handleIPCMessage(this, msg, userData);
			}
		}
	}
}

#elif defined(ORIGIN_MAC) // MAC OSX =================================================================

#include "MacWindows.h"
#include "IGOIPCUtils.h"

IGOIPCClient::IGOIPCClient(const char* serviceName, size_t messageQueueSize) :
IGOIPCConnection(serviceName, messageQueueSize),
mHandler(NULL),
mConnectionCreationTime(0)
{
}


IGOIPCClient::~IGOIPCClient()
{
    stop();
}

bool IGOIPCClient::start()
{
    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    if (mReadPort)
        return false;
    
    // we only try to reconnect every 10 seconds, if the connection was aborted!
    DWORD tc = GetTickCount();
    if ((tc - mConnectionCreationTime) < 10000)
    {
        return false;    // do nothing...
    }
    
    mConnectionCreationTime = tc;
    
    IGO_TRACE("Starting client...");
    
    
    // Allocate the shared memory region we use to send bitmaps over.
    uint32_t pid = (uint32_t)getpid();
    
    char sharedMemName[MAX_PATH];    
    snprintf(sharedMemName, sizeof(sharedMemName), "%s%s_%d", IGOIPC_SHARED_MEMORY_BASE_NAME, mServiceName.c_str(), pid);
    mImageBuffer = new IGOIPCImageBin(sharedMemName, mMessageQueueSize, IGOIPCBuffer::OpenOptions_CREATE_EXCL, IGOIPCBuffer::BufferOptions_USE_LOCAL_LOCK);
    if (!mImageBuffer->IsValid())
    {
        IGO_TRACEF("IGOIPCClient::start - failed to create shared memory for pid %u (%d)\n", pid, errno);
        cleanupComm();
        
        return false;
    }
    
    
    bool setup = false;
    
    // Look up for the Origin client/server
    Origin::Mac::MachPort server(mServiceName.c_str());
    if (server.IsValid())
    {        
        // Create new communication channel
        mReadPort = new Origin::Mac::MachPort();
        if (mReadPort->IsValid())
        {
            // Send a request to the server/send the rights for the communication port at the same time
            ConnectionID connID(pid);
            if (mReadPort->ForwardSendRights(server, &connID, sizeof(connID), 2000) == Origin::Mac::MachPort::CommResult_SUCCEEDED)
            {
                // K, we need to wait for the reply from the server/get send rights to the new server communication channel
                Origin::Mac::MachPort srPort(0, false);
                if (mReadPort->Recv(NULL, 0, 2000, NULL, &srPort, NULL, NULL) == Origin::Mac::MachPort::CommResult_SUCCEEDED)
                {
                    mWritePort = new Origin::Mac::MachPort(srPort);
                    setup = true;
                    
#ifdef SHOW_MACHPORT_SETUP
                    char portInfo[256];
                    
                    IGO_TRACE("IGO New client connection:");
                    
                    mReadPort->ToString(portInfo, sizeof(portInfo));
                    IGO_TRACEF("- Local port:%s", portInfo);
                    
                    mWritePort->ToString(portInfo, sizeof(portInfo));
                    IGO_TRACEF("- Remote port:%s", portInfo);
#endif
                }
                
                else
                {
                    IGO_TRACE("IGOIPCClient::start - failed to get response from Origin client/server");
                }
            }
            
            else
            {
                IGO_TRACE("IGOIPCClient::start - failed to forward rights to Origin client/server");
            }
        }
        
        else
        {
            IGO_TRACE("IGOIPCClient::start - failed to allocate r/w port");
        }
    }
    
    else
    {
        IGO_TRACE("IGOIPCClient::start - failed to locate the Origin client/server");
    }
    
    if (!setup)
    {
        cleanupComm();
        return false;
    }
    
    IGO_TRACEF("IGOIPCClient::start - Successful connection on pid=%d", pid);
    if (mHandler)
        mHandler->handleConnect(this);
    
    return true;
}

void IGOIPCClient::stop(void *userData)
{
    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    if (mReadPort)
    {
        IGO_TRACE("IGOIPCClient::stop - Closing connection\n");
        
		if (mHandler)
			mHandler->handleDisconnect(this, userData);
        
        cleanupComm();
    }
}

// Process pending messages on the client/injected code side.
void IGOIPCClient::processMessages(void *userData)
{
    DEBUG_MUTEX_TEST(mPipeMutex); EA::Thread::AutoFutex m(mPipeMutex);

    char buffer[Origin::Mac::MachPort::MAX_MSG_SIZE];
    
    while (recvRaw(buffer, sizeof(buffer)))
    {
        if (mHandler)
        {
            eastl::shared_ptr<IGOIPCMessage> msg (new IGOIPCMessage(buffer));
            if (mHandler->handleIPCMessage(this, msg, userData))
                break;
        }
    }
}

#endif // ORIGIN_MAC


