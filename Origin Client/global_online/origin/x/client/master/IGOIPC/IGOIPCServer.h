///////////////////////////////////////////////////////////////////////////////
// IGOIPCServer.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPCSERVER_H
#define IGOIPCSERVER_H

#include "IGOIPC.h"
#include "EASTL/list.h"
#include "EASTL/string.h"
#include "IGOIPCUtils.h"

class IGOIPCHandler;
class IGOIPCConnection;


#if defined(ORIGIN_PC)

class IGOIPCServer
{
public:
    IGOIPCServer();
    virtual ~IGOIPCServer();

    void setHandler(IGOIPCHandler *cb);
    bool start();
    void stop();

	virtual void handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL);

private:
    IGOIPCHandler *mHandler;
    volatile bool mQuit;
    HANDLE mMessageThread;
    HANDLE mThreadInitialized;
    HANDLE mThreadQuit;
    HWND mHwnd;
    HINSTANCE mhInstance;

    typedef eastl::list<IGOIPCConnection *> ClientList;
    ClientList mClients;

	static DWORD WINAPI processMessageThread(LPVOID lpParam);
	static LRESULT WINAPI WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	//char *verifyBufferSize(uint32_t size);
	bool addNewConnection(uint32_t id);
	void findOpenPipes();
};

#elif defined(ORIGIN_MAC) // MAC OSX ========================================================

// This class defines the server running in the Origin client that's responsible for
// setting up connections with code-injected games (IGO).
class IGOIPCServer
{
public:
    IGOIPCServer(const char* serviceName, size_t messageQueueSize);
    virtual ~IGOIPCServer();
    
    // Register a new handler to process received messages.
    void setHandler(IGOIPCHandler *cb);
    
    // Start the server/listen for new connection requests from newly injected games.
    bool start(bool unitTest = false);
    
    // Stop the server and clean up.
    void stop();
    
    // Forward a message to the registered handler.
	virtual void handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL);
    
    // Extract the request data/create new connections.
    virtual void handleConnectionRequest(char* buffer, size_t dataSize, Origin::Mac::MachPort srPort);
    
    IGOIPCConnection* UnitTestGetFirstClient() { return (mClients.size() > 0) ? *(mClients.begin()) : NULL; }
    
    
private:
    IGOIPCHandler *mHandler;                    // dispatches game messages to the IGOWindowManager
    pthread_t mMessageThread;                   // server runs in a separate thread

    Origin::Mac::MachPort* mQuitRequest;        // port used to notify the server to stop
    Origin::Mac::MachPort* mWriteRequest;       // port used to notify the server there are pending messages to send to the games
    Origin::Mac::MachPort* mNewConnListener;    // port used to listen for new requests from games newly injected with IGO
    Origin::Mac::MachPortSet* mTrafficListener; // portset used on all the previous ports to perform global "select"
    
    typedef eastl::list<IGOIPCConnection *> ClientList;
    ClientList mClients;                        // list of current connections with code-injected games
    
    eastl::string mServiceName;                 // unique id for the service to be found on the machine
    size_t mMessageQueueSize;                   // size of the shared memory buffer used to store messages to send
    
    //////////
    
    // Main listener thread.
    static void* processMessageThread(void* lpParam);
    
    // Debug listener thread for unit testing.
    static void* processMessageThreadUnitTest(void* lpParam);
    
    // Add a new connection to listen to.
    bool addNewConnection(ConnectionID id, Origin::Mac::MachPort srPort);
    
    // Simply close all the ports we use when the server is running.
    void CleanupPorts();
};

#endif // MAC OSX

#endif
