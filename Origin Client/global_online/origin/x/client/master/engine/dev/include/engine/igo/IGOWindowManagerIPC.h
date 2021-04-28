///////////////////////////////////////////////////////////////////////////////
// IGOWindowManagerIPC.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOWINDOWMANAGERIPC_H
#define IGOWINDOWMANAGERIPC_H

#include <EASTL/hash_map.h>
#include "eathread/eathread_futex.h"

#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCMessage.h"

#include <QString>

class IGOIPCServer;
class IGOIPCConnection;

namespace Origin
{
namespace Engine
{

class IGOTwitch;
class IGOWindowManager;
    
class IGOWindowManagerIPC : public IGOIPCHandler
{
public:
    static const int NO_CONNECTION = 0;

public:
    IGOWindowManagerIPC(IGOWindowManager* igowm);
    virtual ~IGOWindowManagerIPC();

    bool restart(bool explicitStart);
    void stop();
    void setTwitch(IGOTwitch *twitch);

    bool send(eastl::shared_ptr<IGOIPCMessage> msg, uint32_t connectionId = 0);
    bool isConnected() { return mCurrentConnection != NULL; }
    bool isStarted() { return mIsStarted; }
    bool isConnectionValid(uint32_t connectionId);
    void checkConnectionProcesses();

    void setOriginKeyboardLanguage(HKL language) { mOriginKeyboardLanguage = language; };
    uint32_t getCurrentConnectionId();
    
    void resetHotKeys(uint32_t toggleKey1, uint32_t toggleKey2, uint32_t pinKey1, uint32_t pinKey2);

    static bool isRunning() { return mIsRunning; }
    
private:

    virtual void handleConnect(IGOIPCConnection *conn);
	virtual void handleDisconnect(IGOIPCConnection *conn, void *userData = NULL);
    virtual bool handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL);
    
#ifdef ORIGIN_MAC
    void DisableOIGForAllButGameWithFocus();
#endif
    bool updateCurrentConnection();

    IGOIPCServer *mServer;
    
    // Set of metadata about a connection/game injected with OIG
    struct GameInfo
    {
        GameInfo()
        {
            connection = NULL;
            hasFocus = false;
        }
        
        GameInfo(IGOIPCConnection* conn)
        {
            connection = conn;
            hasFocus = false;
        }
        
        IGOIPCConnection* connection;
        bool hasFocus;
    };
    
    typedef eastl::hash_map<uint32_t, GameInfo> GamesCollection;
    GamesCollection mGames;

#ifdef ORIGIN_PC
    // We keep track of the game process separately in case the connection closes but the process is still alive
    typedef eastl::hash_map<uint32_t, bool> GameProcessMap;
    GameProcessMap mGameProcessMap;
#endif
    
    uint32_t mHotKeys[4];
    
    IGOWindowManager* mWindowManager;
    IGOTwitch* mTwitch;
    IGOIPCConnection* mCurrentConnection;
    IGOIPCConnection* mLastStartedConnection;

    HKL mIGOKeyboardLanguage;
    HKL mOriginKeyboardLanguage;

    bool mIsStarted;
    bool mExplicitStart;
    static bool mIsRunning;

    EA::Thread::Futex mConnectionMutex;
    
    bool getExeFolderNameFromId(QString & folderName, DWORD pid);
    void translateDeviceNameToDriveLetters( wchar_t* pszFilename );
};

} // Engine
} // Origin

#endif