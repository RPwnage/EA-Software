///////////////////////////////////////////////////////////////////////////////
// IGOIPCServer.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPC.h"
#include "IGOIPCConnection.h"
#include "IGOIPCServer.h"
#include "IGOIPCMessage.h"
#include "IGOIPCUtils.h"

#include "EASTL/vector.h"

#if defined(ORIGIN_PC)

IGOIPCServer::IGOIPCServer() :
    mHandler(NULL),
    mQuit(false),
    mMessageThread(NULL),
    mHwnd(NULL),
    mhInstance(0)
{
    mhInstance = GetModuleHandle(NULL);

    mThreadInitialized = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!mThreadInitialized || mThreadInitialized == INVALID_HANDLE_VALUE)
        IGO_TRACE("mThreadInitialized == NULL.");
   
    mThreadQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!mThreadQuit || mThreadQuit == INVALID_HANDLE_VALUE)
        IGO_TRACE("mThreadQuit == NULL.");
}

IGOIPCServer::~IGOIPCServer()
{
    stop();
}

void IGOIPCServer::setHandler(IGOIPCHandler *handler)
{
    mHandler = handler;
}

void IGOIPCServer::stop()
{
    mQuit = true;
    IGO_TRACE("IGOIPCServer::stop.");
    SetEvent(mThreadQuit);

    if (mMessageThread)
    {
        WaitForSingleObject(mMessageThread, INFINITE);
        CloseHandle(mMessageThread);
        mMessageThread = NULL;
    }

    if (mHwnd)
    {
        DestroyWindow(mHwnd);
        mHwnd = NULL;
    }

    UnregisterClass(IGOIPC_WINDOW_CLASS, mhInstance);

    ClientList::const_iterator iter;
    for (iter = mClients.begin(); iter != mClients.end(); iter++)
    {
        delete (*iter);
    }
    
    mClients.clear();
}

bool IGOIPCServer::start()
{
    IGO_TRACE("IGOIPCServer::start.");
	    
    mQuit = false;
    if (ResetEvent(mThreadInitialized) == 0)
    {
        IGO_TRACEF("IGOIPCServer::unable to reset mThreadInitialized (%d)", GetLastError());
    }

    if (ResetEvent(mThreadQuit) == 0)
    {       
        IGO_TRACEF("IGOIPCServer::unable to reset mThreadQuit (%d)", GetLastError());
    }

    HWND tmpWindow = FindWindowEx(HWND_MESSAGE, NULL, IGOIPC_WINDOW_CLASS, IGOIPC_WINDOW_TITLE);
    if (tmpWindow)
    {
        DWORD exitCode = 0;
        if (mMessageThread && GetExitCodeThread(mMessageThread, &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
            {
                if (WaitForSingleObject(mMessageThread, 1000) == WAIT_TIMEOUT)  // do a small check if the tread really runs
                {
                    return true;    // everything is fine
                }
                else
                {
                    IGO_TRACE("WaitForSingleObject(mMessageThread, 1000) != WAIT_TIMEOUT");
                }
            }
            else
            {
                IGO_TRACEF("exitCode != STILL_ACTIVE %x.", exitCode);
            }
        }
        else
        {
            if (!mMessageThread)
                IGO_TRACE("mMessageThread == NULL.");
        }
    }
    else
    {
        IGO_TRACE("FindWindowEx(HWND_MESSAGE, NULL, IGOIPC_WINDOW_CLASS, IGOIPC_WINDOW_TITLE) == NULL.");
    }

    // cleanup old thread
    if (mMessageThread != NULL)
    {
        mQuit = true;
        SetEvent(mThreadQuit);
        WaitForSingleObject(mMessageThread, 2500);
        TerminateThread(mMessageThread, 0);
        CloseHandle(mMessageThread);
        mMessageThread = NULL;
    }
    DestroyWindow(tmpWindow);
    UnregisterClass(IGOIPC_WINDOW_CLASS, mhInstance);


    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = IGOIPCServer::WndProc;
    wc.hInstance = mhInstance;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszClassName = IGOIPC_WINDOW_CLASS;

    if (RegisterClassEx(&wc))
    {
        mMessageThread = CreateThread(NULL, 0, IGOIPCServer::processMessageThread, this, 0, NULL);
        if (mMessageThread)
        {
            // wait 2.5 seconds, then terminate the thread
            if (WaitForSingleObject(mThreadInitialized, 2500) == WAIT_OBJECT_0 )
            {
                IGO_TRACE("WaitForSingleObject(mThreadInitialized, 2500) == WAIT_OBJECT_0");
                DWORD exitCode = 0;
                if (mMessageThread && GetExitCodeThread(mMessageThread, &exitCode))
                {
                    if (exitCode == STILL_ACTIVE)
                    {
                        if (WaitForSingleObject(mMessageThread, 1000) == WAIT_TIMEOUT)  // do a small check if the tread really runs
                        {
                            return true;    // everything is fine
                        }
                        else
                        {
                            IGO_TRACE("WaitForSingleObject(mMessageThread, 1000) != WAIT_TIMEOUT");
                        }
                    }
                    else
                    {
                        IGO_TRACE("exitCode != STILL_ACTIVE.");
                    }
                }
                else
                {
                    if (!mMessageThread)
                        IGO_TRACE("mMessageThread == NULL.");
                }            
            }
            else
            {
                IGO_TRACE("WaitForSingleObject(mThreadInitialized, 2500) != WAIT_OBJECT_0");
            }

            mQuit = true;
            SetEvent(mThreadQuit);
            WaitForSingleObject(mMessageThread, 2500);
            TerminateThread(mMessageThread, 0);
            CloseHandle(mMessageThread);
            mMessageThread = NULL;
        }
        DestroyWindow(tmpWindow);
        UnregisterClass(IGOIPC_WINDOW_CLASS, mhInstance);
    }
    else
    {
        IGO_TRACEF("RegisterClassEx failed %d.", GetLastError());
    }

    return false;
}

void IGOIPCServer::handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData)
{
	if (mHandler)
		mHandler->handleIPCMessage(conn, msg, userData);
}

LRESULT IGOIPCServer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    IGOIPCServer *server = (IGOIPCServer *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (!server)
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch (message)
    {
    case WM_NEW_CONNECTION:
        {
            server->addNewConnection((uint32_t)wParam);
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

DWORD IGOIPCServer::processMessageThread(LPVOID lpParam)
{
	IGOIPCServer *_this = static_cast<IGOIPCServer *>(lpParam);
	eastl::vector<HANDLE> handles;
	eastl::vector<IGOIPCConnection *> clients;

    _this->mHwnd = NULL;

    // create the window and use the result as the handle
    _this->mHwnd = CreateWindowEx(NULL,
                            IGOIPC_WINDOW_CLASS,    // name of the window class
                            IGOIPC_WINDOW_TITLE,    // title of the window
                            0,    // window style
                            0,    // x-position of the window
                            0,    // y-position of the window
                            0,    // width of the window
                            0,    // height of the window
                            HWND_MESSAGE,    // message only - otherwise we see heavy slowdowns
                            NULL,    // we aren't using menus, NULL
                            GetModuleHandle(NULL),    // application handle
                            NULL);    // used with multiple windows, NULL

    if (!_this->mHwnd)
    {
        IGO_TRACEF("processMessageThread: createwindow failed. %x", GetLastError());
        return 0;
    }
    SetWindowLongPtr(_this->mHwnd, GWLP_USERDATA, (LONG_PTR)_this);

    // find opened pipes
    _this->findOpenPipes();

    // we are starting loop
    if (!SetEvent(_this->mThreadInitialized))
    {
        IGO_TRACEF("processMessageThread: SetEvent failed. %x", GetLastError());
    }

    while (!_this->mQuit)
    {
        ClientList::iterator iter;

        clients.clear();
        handles.clear();
        handles.push_back(_this->mThreadQuit);
        for (iter = _this->mClients.begin(); iter != _this->mClients.end(); iter++)
        {
            clients.push_back((*iter));
            handles.push_back((*iter)->getReadEvent());
            handles.push_back((*iter)->getWriteEvent());
        }

        DWORD result;
        result = MsgWaitForMultipleObjectsEx(static_cast<DWORD>(handles.size()), &handles[0], INFINITE, QS_ALLEVENTS, 0);

        // thread quit called
        if (result == WAIT_OBJECT_0)
        {
            IGO_TRACE("processMessageThread: result == WAIT_OBJECT_0");
            break;
        }

        // window message is being processed
        if (result == WAIT_OBJECT_0 + handles.size())
        {
            MSG msg;
            while (PeekMessage(&msg, _this->mHwnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            continue;
        }

        // this is one of the events
        //IGO_TRACEF("MsgWaitForMultipleObjectsEx() = %d", result);
        result = result - 1; // remove the quit event from play

        int index = result/2; // index of clients
        IGOIPCConnection *conn = NULL;

        if (index >= 0 && index < (int) clients.size() )
            conn = clients[index];

        if (!conn)
        {
            continue;
        }
        else if (result&1)
        {
            // write
            conn->processWrite();
        }
        else
        {
            // read
            conn->processRead();

			if (conn->messageReady())
			{
				if (_this->mHandler)
				{
					eastl::shared_ptr<IGOIPCMessage> msg (conn->getMessage());
					_this->mHandler->handleIPCMessage(conn, msg, NULL);
				}
			}
		}

		// close the connection
		if (conn && conn->isClosed())
		{
			if (_this->mHandler)
				_this->mHandler->handleDisconnect(conn, NULL);

            iter = find(_this->mClients.begin(), _this->mClients.end(), conn);
            if (iter != _this->mClients.end())
                _this->mClients.erase(iter);
            delete conn;
        }
    }

    SetWindowLongPtr(_this->mHwnd, GWLP_USERDATA, (LONG)NULL);

    return 0;
}

bool IGOIPCServer::addNewConnection(uint32_t id)
{
    IGOIPCConnection *conn = new IGOIPCConnection();
    bool s = conn->connect(id, true);
    if (!s)
    {
        IGO_TRACE("IGOIPCServer::addNewConnection failed\n");
        delete conn;
        return false;
    }
    mClients.push_back(conn);

    if (mHandler)
        mHandler->handleConnect(conn);

    return true;
}

void IGOIPCServer::findOpenPipes()
{
    eastl::list<uint32_t> pipes;
    eastl::list<uint32_t>::const_iterator iter;
    
    IGOIPCGetNamedPipes(pipes);

    for (iter = pipes.begin(); iter != pipes.end(); iter++)
    {
        addNewConnection(*iter);
    }
}

#elif defined(ORIGIN_MAC) // MAC OSX ==================================================================

IGOIPCServer::IGOIPCServer(const char* serviceName, size_t messageQueueSize) :
mHandler(NULL),
mMessageThread(NULL),
mQuitRequest(NULL),
mWriteRequest(NULL),
mNewConnListener(NULL),
mTrafficListener(NULL),
mServiceName(serviceName),
mMessageQueueSize(messageQueueSize)
{
}

IGOIPCServer::~IGOIPCServer()
{
    stop();
}

// Stop the server and clean up.
void IGOIPCServer::stop()
{
    IGO_TRACE("IGOIPCServer::stop - Closing connection");
    
    if (mTrafficListener)
    {
        // Signal we want to stop the server
        int notUsed = 0;
        mQuitRequest->Send(&notUsed, sizeof(notUsed), 0, NULL, NULL);
        
        pthread_join(mMessageThread, NULL);
        mMessageThread = NULL;
        
        // Close connection
        ClientList::iterator iter;
        for (iter = mClients.begin(); iter != mClients.end(); iter++)
        {
            // Detach from main listener
            mTrafficListener->Remove((*iter)->getReadChannel());
            
            delete (*iter);
        }
        
        mClients.clear();
        
        // Clean local ports too
        CleanupPorts();    
    }
}

// Simply close all the ports we use when the server is running.
void IGOIPCServer::CleanupPorts()
{
    if (mTrafficListener)
    {
        mTrafficListener->Remove(mNewConnListener);
        mTrafficListener->Remove(mQuitRequest);
        mTrafficListener->Remove(mWriteRequest);    
    }
    
    delete mQuitRequest;
    delete mWriteRequest;
    delete mTrafficListener;
    delete mNewConnListener;
    
    mQuitRequest = NULL;
    mWriteRequest = NULL;
    mTrafficListener = NULL;
    mNewConnListener = NULL;    
}

// Start the server/listen for new connection requests from newly injected games.
bool IGOIPCServer::start(bool unitTest)
{
    // Register service to listen for new connection from games injected with IGO
    bool serviceRegistered = false;
    mNewConnListener = new Origin::Mac::MachPort();    
    if (mNewConnListener->IsValid())
    {
        serviceRegistered = mNewConnListener->RegisterService(mServiceName.c_str());
    }
    
    if (!serviceRegistered)
    {
        IGO_TRACE("IGOIPCServer::start - Unable to setup main service to listen for request from newly injected games!");
        CleanupPorts();        
        
        return false;
    }
    
    // We use a port to signal when to stop the server and another one to signal we have data to send over
    mQuitRequest = new Origin::Mac::MachPort();
    mWriteRequest = new Origin::Mac::MachPort();
    
    // And a portset to listen for new data on valid connections with games - use it also to
    // listen for new connection requests, check if we have data to send out or if we need to stop the server.
    mTrafficListener = new Origin::Mac::MachPortSet();
    if (!mQuitRequest->IsValid() || !mWriteRequest->IsValid() || !mTrafficListener->IsValid())
    {
        IGO_TRACE("IGOIPCServer::start - Unable to setup the supporting ports");
        CleanupPorts();
        
        return false;
    }
    
    mTrafficListener->Add(mNewConnListener);
    mTrafficListener->Add(mQuitRequest);
    mTrafficListener->Add(mWriteRequest);
    
#ifdef SHOW_MACHPORT_SETUP
    char portInfo[256];
    
    IGO_TRACE("IGO Server:");
    
    mNewConnListener->ToString(portInfo, sizeof(portInfo));
    IGO_TRACEF("- Connection port: %s", portInfo);
    
    mQuitRequest->ToString(portInfo, sizeof(portInfo));
    IGO_TRACEF("- Quit request port: %s", portInfo);
    
    mWriteRequest->ToString(portInfo, sizeof(portInfo));
    IGO_TRACEF("- Internal write request port: %s", portInfo);
    
    mTrafficListener->ToString(portInfo, sizeof(portInfo));
    IGO_TRACEF("- Global traffic listener (portset): %s", portInfo);
#endif
    
    if (!unitTest)
    {
        if (pthread_create(&mMessageThread, NULL, IGOIPCServer::processMessageThread, this) < 0)
        {
            IGO_TRACEF("IGOIPCServer::start - Failed to start processMessageThread (%d)", errno);
            CleanupPorts();
            
            return false;
        }
    }
    
    else
    {
        if (pthread_create(&mMessageThread, NULL, IGOIPCServer::processMessageThreadUnitTest, this) < 0)
        {
            IGO_TRACEF("IGOIPCServer::start - Failed to start processMessageThreadUnitTest (%d)", errno);
            CleanupPorts();
            
            return false;
        }
    }
    return true;
}

// Register a new handler to process received messages.
void IGOIPCServer::setHandler(IGOIPCHandler *handler)
{
    mHandler = handler;
}

// Forward a message to the registered handler.
void IGOIPCServer::handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData)
{
	if (mHandler)
		mHandler->handleIPCMessage(conn, msg, userData);
}

// Main listener thread.
void* IGOIPCServer::processMessageThread(void* lpParam)
{
    pthread_setname_np("IGOIPCServer::processMessageThread");
    
	IGOIPCServer *_this = static_cast<IGOIPCServer *>(lpParam);
	eastl::vector<IGOIPCConnection *> clients;
    
    while (true)
    {
        ClientList::iterator iter;
        
        int timeoutInMS = -1; // block
        
        // Do a "select" on the open channels + listen for new connections, quit and write requests
        size_t dataSize = 0;
        char buffer[Origin::Mac::MachPort::MAX_MSG_SIZE];
        Origin::Mac::MachPort srPort(0, false); // send rights port
        const Origin::Mac::MachPort eventPort = _this->mTrafficListener->Listen(buffer, sizeof(buffer), timeoutInMS, &dataSize, &srPort);
        
        if (eventPort != Origin::Mac::MachPort::Null())
        {
            // Is this a request for a new connection (ie new game injected with IGO?)
            if (eventPort == *(_this->mNewConnListener))
            {
                _this->handleConnectionRequest(buffer, dataSize, srPort);
            }
            
            else  // Do we have data to send over?
            if (eventPort == *(_this->mWriteRequest))
            {
                // Do we have any write to send over?
                for (iter = _this->mClients.begin(); iter != _this->mClients.end(); )
                {
                    IGOIPCConnection* conn = *iter;                    
                    conn->processWrite();
                    
                    // close the connection
                    if (conn->isClosed())
                    {
                        if (_this->mHandler)
                            _this->mHandler->handleDisconnect(conn, NULL);
                        
                        iter = _this->mClients.erase(iter);
                        
                        // remove from main listener
                        _this->mTrafficListener->Remove(conn->getReadChannel());
                        
                        delete conn;
                    }
                    
                    else
                        ++iter;
                }    
            }
            
            else  // Is this a request to exist the thread/quit?
            if (eventPort == *(_this->mQuitRequest))
            {
                break;
            }
    
            else
            if (eventPort.IsNotification()) // Is it a dead name (= lost connection) notification
            {
                for (iter = _this->mClients.begin(); iter != _this->mClients.end(); )
                {
                    IGOIPCConnection* conn = *iter;
                    
                    if (eventPort == *conn->getNotifyChannel())
                    {
                        if (_this->mHandler)
                            _this->mHandler->handleDisconnect(conn, NULL);
                        
                        iter = _this->mClients.erase(iter);
                        
                        // remove from main listener
                        _this->mTrafficListener->Remove(conn->getReadChannel());
                        _this->mTrafficListener->Remove(conn->getNotifyChannel());
                        
                        delete conn;
                        
#ifdef SHOW_MACHPORT_SETUP
                        char portInfo[256];
                        _this->mWriteRequest->ToString(portInfo, sizeof(portInfo));
                        IGO_TRACEF("Internal server notify new state: %s", portInfo);
#endif
                        
                        break;
                    }
                    
                    else
                        ++iter;
                }
            }
            
            else // I guess this is an actual read
            {
                // Nope, so lookup which connection the data is for
                for (iter = _this->mClients.begin(); iter != _this->mClients.end(); ++iter)
                {
                    IGOIPCConnection* conn = *iter;
                    if (eventPort == *conn->getReadChannel())
                    {
                        // Process the data
                        if (conn->processRead(buffer, dataSize))
                        {
                            if (_this->mHandler)
                            {
                                eastl::shared_ptr<IGOIPCMessage> msg (conn->getMessage());
                                _this->mHandler->handleIPCMessage(conn, msg, NULL);
                            }
                        }
                        break;
                    }
                }
                
                if (iter == _this->mClients.end())
                {
                    IGO_TRACE("IGOIPCServer::processMessageThread - Received data but can't find corresponding port!");
                }
            }
        }
        
    }
    
    return 0;
}

// Debug listener thread for unit testing.
void* IGOIPCServer::processMessageThreadUnitTest(void* lpParam)
{
    pthread_setname_np("IGOIPCServer::processMessageThreadUnitTest");
    
	IGOIPCServer *_this = static_cast<IGOIPCServer *>(lpParam);
	eastl::vector<IGOIPCConnection *> clients;
    
    while (true)
    {
        ClientList::iterator iter;
        
        int timeoutInMS = -1; // block
        
        // Do a "select" on the open channels + listen for new connections, quit and write requests
        size_t dataSize = 0;
        char buffer[Origin::Mac::MachPort::MAX_MSG_SIZE];
        Origin::Mac::MachPort srPort(0, false); // send rights port
        const Origin::Mac::MachPort eventPort = _this->mTrafficListener->Listen(buffer, sizeof(buffer), timeoutInMS, &dataSize, &srPort);
        
        if (eventPort != Origin::Mac::MachPort::Null())
        {
            // Is this a request for a new connection (ie new game injected with IGO?)
            if (eventPort == *(_this->mNewConnListener))
            {
                _this->handleConnectionRequest(buffer, dataSize, srPort);
            }
            
            else  // Is this a request to exist the thread/quit?
            if (eventPort == *(_this->mQuitRequest))
            {
                break;
            }
            
            // Quit as soon as we got a connection setup
            if (_this->mClients.size() > 0)
            {
                break;
            }
        }
    }
    
    return 0;
}

// Extract the request data/create new connections.
void IGOIPCServer::handleConnectionRequest(char* buffer, size_t bufferSize, Origin::Mac::MachPort srPort)
{
    
    if (bufferSize != sizeof(ConnectionID))
    {
        IGO_TRACE("IGOIPCServer::handleConnectionRequest - receive invalid connection data");
        
        // The port will cleanup automatically
        return;
    }
    
    ConnectionID connID = *((ConnectionID*)buffer);
    addNewConnection(connID, srPort);    
}

// Add a new connection to listen to.
bool IGOIPCServer::addNewConnection(ConnectionID id, Origin::Mac::MachPort srPort)
{
    // Is this a duplicate?
    for (ClientList::iterator iter = mClients.begin(); iter != mClients.end(); ++iter)
    {
        if ((*iter)->id() == id.pid)
        {
            // TODO - Check if the connection is actually dead -> we should re-crearte it!
            // OR IMPLEMENT NOTIFICATIONS!
            
            IGO_TRACEF("IGOIPCServer::addNewConnection - received request for duplicate id(%d)", id.pid);
            
            // The port will cleanup automatically
            return false;
        }
    }
    
    // Create a new connection
    
    // Make sure we can add send rights to the write request port
    Origin::Mac::MachPort connWriteRequest = mWriteRequest->AddSendRights();
    if (!connWriteRequest.IsValid())
    {
        IGO_TRACE("IGOIPCServer::addNewConnection - unable to add new send rights to the write request port");
        return false;
    }
    
    IGOIPCConnection *conn = new IGOIPCConnection(mServiceName.c_str(), mMessageQueueSize);
    if (!conn->connect(id, srPort, connWriteRequest))
    {
        IGO_TRACE("IGOIPCServer::addNewConnection failed");
        delete conn;
        
        return false;
    }
    
    mClients.push_back(conn);
    
    // Do we need to do something special when connecting to a game?
    if (mHandler)
        mHandler->handleConnect(conn);
    
    // Add it to our main listener
    mTrafficListener->Add(conn->getReadChannel());
    mTrafficListener->Add(conn->getNotifyChannel());
    
    return true;
}

#endif // ORIGIN_MAC
