///////////////////////////////////////////////////////////////////////////////
// IGOWindowManagerIPC.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <limits>
#include <QDateTime>
#include "IGOWindowManagerIPC.h"

#include "IGOQWidget.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCServer.h"
#include "IGOIPC/IGOIPCConnection.h"
#include "IGOIPC/IGOIPCMessage.h"
#include "IGOWindowManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include "IGOController.h"


namespace Origin
{
namespace Engine
{

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

#include <Tlhelp32.h>
#include <psapi.h>
#include <strsafe.h>

#elif defined(ORIGIN_MAC)

#include <errno.h>
#include <libproc.h>

#endif

#include <QString>

bool IGOWindowManagerIPC::mIsRunning = false;
    
IGOWindowManagerIPC::IGOWindowManagerIPC(IGOWindowManager* igowm):
mWindowManager(igowm),
mTwitch(NULL),
mCurrentConnection(NULL),
mLastStartedConnection(NULL),
mIGOKeyboardLanguage(NULL),
mOriginKeyboardLanguage(NULL),
mIsStarted(false),
mExplicitStart(false)
{
	IGOIPC *ipc = IGOIPC::instance();
#if defined(ORIGIN_MAC)
	mServer = ipc->createServer(IGOIPC_REGISTRATION_SERVICE, IGOIPC_SHARED_MEMORY_BUFFER_SIZE);
    mServer->setHandler(this);
#elif defined(ORIGIN_PC)
    mServer = ipc->createServer();
    mServer->setHandler(this);
	mOriginKeyboardLanguage = GetKeyboardLayout(0);
#endif
}

IGOWindowManagerIPC::~IGOWindowManagerIPC()
{
	mIsRunning = false;
	mIsStarted = false;
    mServer->stop();
	delete mServer;
	mServer = NULL;
    mTwitch = NULL;
}

void IGOWindowManagerIPC::setTwitch(IGOTwitch *twitch)
{
    mTwitch = twitch;
}

void IGOWindowManagerIPC::stop()
{
    mIsStarted = false;
    mExplicitStart = false;

    if (mServer)
        mServer->stop();

    mCurrentConnection = NULL;
    mIGOKeyboardLanguage = NULL;
    mOriginKeyboardLanguage = NULL;

    mGames.clear();
#ifdef ORIGIN_PC
    mGameProcessMap.reset();
#endif
}

void IGOWindowManagerIPC::resetHotKeys(uint32_t toggleKey1, uint32_t toggleKey2, uint32_t pinKey1, uint32_t pinKey2)
{
    mHotKeys[0] = toggleKey1;
    mHotKeys[1] = toggleKey2;
    mHotKeys[2] = pinKey1;
    mHotKeys[3] = pinKey2;
    
    eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgSetHotkey(mHotKeys[0], mHotKeys[1], mHotKeys[2], mHotKeys[3]));
    send(msg);
}
    
void IGOWindowManagerIPC::handleConnect(IGOIPCConnection *conn)
{
	mWindowManager->injectIGOVisible(false, true /*emitStateChange*/);

    // Adding a sleep here, as early connections from Origin to the IGO seems to cause issues.
    Sleep(250);

	IGO_TRACEF("Connected with PID %d", conn->id());

	mIsRunning = true;

	// add our game to the game list
    {
        DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
        
        GameInfo info(conn);
        mGames[conn->id()] = info;

#ifdef ORIGIN_PC
        mGameProcessMap[conn->id()] = true;
#endif
        
    }
    
	// get the games process folder
	QString processFolder;
	getExeFolderNameFromId(processFolder, conn->id());

    // send the start message when we connect
    QSize minSize = mWindowManager->minimumScreenSize();
    eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgStart(mHotKeys[0], mHotKeys[1], mHotKeys[2], mHotKeys[3], minSize.width(), minSize.height()));
    conn->send(msg);

	mWindowManager->injectIgoConnect((uint)conn->id(), processFolder);
}

void IGOWindowManagerIPC::handleDisconnect(IGOIPCConnection *conn, void *userData)
{
	IGO_TRACEF("Disconnecting with PID %d", conn->id());
	
	// remove the game process id from the window manager - the process may still live!!!
#ifdef ORIGIN_MAC
    bool processIsGone = true;
#else
    bool processIsGone = false;
#endif
    
	emit mWindowManager->injectIgoDisconnect((uint)conn->id(), processIsGone);
	
    bool stopIGO = false;
    {
        DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);

        // remove our game info
        {
            GamesCollection::iterator iter = mGames.find(conn->id());
            if (iter != mGames.end())
                mGames.erase(iter);
        }

#if defined(ORIGIN_PC)
        // is the process gone as well?
        if (!mWindowManager->isGameProcessStillRunning(conn->id()))
        {
            // remove the closed process
            {
                GameProcessMap::iterator iter = mGameProcessMap.find(conn->id());
                if (iter != mGameProcessMap.end())
                {
	                // remove the game process id from the window manager
	                mWindowManager->injectIgoDisconnect((uint)iter->first, true);
                    mGameProcessMap.erase(iter);
                }
            }
        }

        // check remaining processes....
        {
            GameProcessMap::iterator iter = mGameProcessMap.begin();
            while (iter != mGameProcessMap.end())
            {
                if (!mWindowManager->isGameProcessStillRunning(iter->first))
                {
	                // remove the game process id from the window manager
	                mWindowManager->injectIgoDisconnect((uint)iter->first, true);
                    iter = mGameProcessMap.erase(iter);
                }
                else
                    ++iter;
            }
        }
#endif

        if (conn == mCurrentConnection)
        {
            IGO_TRACE("Current connection is NULL");

            // try to find a connection with an active game that has the focus
            bool hasFocus = updateCurrentConnection();

            if (!mCurrentConnection)
            {
                IGO_TRACE("No current connection");
            }
            
            else
            if (!hasFocus)
            {
                IGO_TRACE("No game has the focus");
            }
        }

        stopIGO = mGames.empty();
    }
    
	if (stopIGO)
	{
		mIsRunning = false;
		
		// reset Origin's keyboard language
#if defined(ORIGIN_PC)
		if (mOriginKeyboardLanguage!=0)
			ActivateKeyboardLayout((HKL)mOriginKeyboardLanguage, KLF_SETFORPROCESS);
#endif

#if defined(ORIGIN_PC)
        if (mGameProcessMap.empty())    // only stop IGO, if all game processes are gone, not just switched to background!!!
#endif
            mWindowManager->injectIGOStop();		// only stop IGO if no other game is running!
	}


}

void IGOWindowManagerIPC::checkConnectionProcesses()
{
#ifdef ORIGIN_PC
    DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);

    GameProcessMap::iterator iter = mGameProcessMap.begin();
    while (iter != mGameProcessMap.end())
    {
        if (!mWindowManager->isGameProcessStillRunning(iter->first))
        {
	        // remove the game process id from the window manager
	        mWindowManager->injectIgoDisconnect((uint)iter->first, true);
            iter = mGameProcessMap.erase(iter);
        }
        else
            ++iter;
    }
#endif
}

bool IGOWindowManagerIPC::handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData)
{
    bool retVal = false;
    
	// make sure we have that connection in our list!!!
    {
        DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
        if (mGames.find(conn->id())==mGames.end())
        {
            IGO_TRACE("Missing connection was added to our list.");
            GameInfo info(conn);
            mGames[conn->id()] = info;
        }
    }
    
	switch (msg.get()->type())
	{
#if defined(ORIGIN_PC)

	case IGOIPC::MSG_IGO_BROADCAST_STREAM_INFO:
		{
			IGOIPC *ipc = IGOIPC::instance();

			int viewers;
			ipc->parseMsgBroadcastStreamInfo(msg.get(), viewers);
			
            mTwitch->injectBroadcastStreamInfo(viewers);
		}
		break;

    case IGOIPC::MSG_IGO_BROADCAST_USERNAME:
		{
			IGOIPC *ipc = IGOIPC::instance();

			eastl::string userName;
			ipc->parseMsgBroadcastUserName(msg.get(), userName);
			
            mTwitch->injectBroadcastUserName(userName.c_str());
		}
		break;

	case IGOIPC::MSG_IGO_BROADCAST_STATUS:
		{
			IGOIPC *ipc = IGOIPC::instance();

			bool status;
            bool archivingEnabled;
            eastl::string fixArchivingURL;
            eastl::string channelURL;

			ipc->parseMsgBroadcastStatus(msg.get(), status, archivingEnabled, fixArchivingURL, channelURL);
			
            // we disabled auto arciving feature for 9.2, so always pretend archiving is enabled so we never kick in the new logic
            //mWindowManager->injectBroadcastStatus(status, archivingEnabled, fixArchivingURL.c_str(), channelURL.c_str());
            mTwitch->injectBroadcastStatus(status, true, "", channelURL.c_str());
		}
		break;

	case IGOIPC::MSG_IGO_BROADCAST_ERROR:
		{
			IGOIPC *ipc = IGOIPC::instance();

			int errorCode;
            int errorCategory;
			ipc->parseMsgBroadcastError(msg.get(), errorCategory, errorCode);
			
            mTwitch->injectBroadcastError(errorCategory, errorCode);
		}
		break;

	case IGOIPC::MSG_IGO_BROADCAST_OPT_ENCODER_AVAILABLE:
		{
			IGOIPC *ipc = IGOIPC::instance();

			bool available;
			ipc->parseMsgBroadcastOptEncoderAvailable(msg.get(), available);

			mTwitch->injectBroadcastOptEncoderAvailable(available);
		}
		break;

    case IGOIPC::MSG_IGO_BROADCAST_STATS:
        {
#ifdef SHOW_IPC_MESSAGES
            ORIGIN_LOG_EVENT << "IPC(MSG_IGO_BROADCAST_STATS - " << conn->id() << ")";
#endif
            bool isBroadcasting;
            uint64_t streamId;
            uint32_t minViewers;
            uint32_t maxViewers;
            const char16_t* channel = NULL;
            size_t channelLength = 0;
            
            if (IGOIPC::instance()->parseMsgBroadcastStats(msg.get(), isBroadcasting, streamId, minViewers, maxViewers, channel, channelLength))
                IGOController::instance()->gameTracker()->setBroadcastStats(conn->id(), isBroadcasting, streamId, minViewers, maxViewers, channel, channelLength);
        }
        break;
#endif

	case IGOIPC::MSG_INPUT_EVENT:
		{
			IGOIPC *ipc = IGOIPC::instance();

			IGOIPC::InputEventType type;
			int32_t param1, param2, param3, param4;
            float param5, param6;
#ifdef ORIGIN_MAC
            uint64_t param7;
			ipc->parseMsgInputEvent(msg.get(), type, param1, param2, param3, param4, param5, param6, param7);
#else
			ipc->parseMsgInputEvent(msg.get(), type, param1, param2, param3, param4, param5, param6);
#endif
		
#ifdef SHOW_RECEIVED_EVENTS
            switch (type)
            {
                case IGOIPC::EVENT_MOUSE_MOVE:
#ifdef ORIGIN_MAC
                    IGO_TRACEF("MOUSE_MOVE EVENT: correctedX=%d, correctedY=%d, offsetX=%d, offsetY=%d, sX=%f, sY=%f, timestamp=%lld", param1, param2, param3, param4, param5, param6, param7);
#else
                    IGO_TRACEF("MOUSE_MOVE EVENT: correctedX=%d, correctedY=%d, offsetX=%d, offsetY=%d, sX=%f, sY=%f", param1, param2, param3, param4, param5, param6);
#endif
                    break;
                    
                case IGOIPC::EVENT_MOUSE_LEFT_DOWN:
                    IGO_TRACEF("MOUSE_LEFT_DOWN EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_LEFT_UP:
                    IGO_TRACEF("MOUSE_LEFT_UP EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_RIGHT_DOWN:
                    IGO_TRACEF("MOUSE_RIGHT_DOWN EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_RIGHT_UP:
                    IGO_TRACEF("MOUSE_RIGHT_UP EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_LEFT_DOUBLE_CLICK:
                    IGO_TRACEF("MOUSE_LEFT_DOUBLE_CLICK EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_RIGHT_DOUBLE_CLICK:
                    IGO_TRACEF("MOUSE_RIGHT_DOUBLE_CLICK EVENT: X=%d, Y=%d (%d)", param1, param2, param3);
                    break;
                    
                case IGOIPC::EVENT_MOUSE_WHEEL:
                    IGO_TRACEF("MOUSE_WHEEL EVENT: X=%d, Y=%d (Vertical=%d, Horizontal=%d)", param1, param2, param3, param4);
                    break;
                    
                case IGOIPC::EVENT_KEYBOARD:
                    IGO_TRACE("KEYBOARD INPUT EVENT");
                    break;
                    
                default:
                    IGO_TRACE("UNKNOWN INPUT EVENT");
                    break;
            }
#endif
#ifdef ORIGIN_MAC
			mWindowManager->injectInputEvent(type, param1, param2, param3, param4, param5, param6, param7);
#else
			mWindowManager->injectInputEvent(type, param1, param2, param3, param4, param5, param6);
#endif
		}
		break;

#if defined(ORIGIN_MAC)

    case IGOIPC::MSG_KEYBOARD_EVENT:
        {
            IGOIPC *ipc = IGOIPC::instance();
            
            const char* data;
            int size;
            ipc->parseMsgKeyboardEvent(msg.get(), data, size);
            
            //IGO_TRACEF("GOT KEYBOARD DATA SIZE=%d", size);
            mWindowManager->injectKeyboardEvent(data, size);
        }
    break;

#endif

	case IGOIPC::MSG_SCREEN_SIZE:
		{
			IGOIPC *ipc = IGOIPC::instance();

			// only process part of this message for the game that has the focus
            DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
            updateCurrentConnection();
            IGO_ASSERT(mCurrentConnection!=0);

			if (mGames[conn->id()].hasFocus == true)
			{
				uint16_t width, height;
				ipc->parseMsgScreenSize(msg.get(), width, height);

				emit mWindowManager->igoScreenResize(conn->id(), width, height, false);
#ifdef SHOW_IPC_MESSAGES
				ORIGIN_LOG_EVENT << "IPC(MSG_SCREEN_SIZE - " << conn->id() << "): width = " << width << ", height = " << height;
#endif
			}
            
            else
            {
#ifdef SHOW_IPC_MESSAGES
                ORIGIN_LOG_EVENT << "IPC(MSG_SCREEN_SIZE - " << conn->id() << ") RECEIVED BUT NO FOCUS";
#endif
            }
            
			// igo started
			mWindowManager->injectIGOStart(conn->id());
		}
		break;

        
	case IGOIPC::MSG_RESET:
		{
            DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
            updateCurrentConnection();
            IGO_ASSERT(mCurrentConnection!=0);

			// only process this message for the game that has the focus
			if (mGames[conn->id()].hasFocus == true)
			{
				IGOIPC *ipc = IGOIPC::instance();

				uint16_t width, height;
				ipc->parseMsgReset(msg.get(), width, height);
#ifdef SHOW_IPC_MESSAGES
				ORIGIN_LOG_EVENT << "IPC(MSG_RESET - " << conn->id() << "): width = " << width << ", height = " << height;
#endif

#ifndef ORIGIN_MAC
                QSize minSize = mWindowManager->minimumScreenSize();
                eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgStart(mHotKeys[0], mHotKeys[1], mHotKeys[2], mHotKeys[3], minSize.width(), minSize.height()));
				conn->send(msg);
#endif
				emit mWindowManager->igoScreenResize(conn->id(), width, height, true);
			}
            
            else
            {
#ifdef SHOW_IPC_MESSAGES
                ORIGIN_LOG_EVENT << "IPC(MSG_RESET - " << conn->id() << ") RECEIVED BUT NO FOCUS";
#endif
            }
		}
		break;

	case IGOIPC::MSG_IGO_MODE:
		{
			IGOIPC *ipc = IGOIPC::instance();

			bool visible = false;
            bool emitStateChange = true;
			ipc->parseMsgIGOMode(msg.get(), visible, emitStateChange);

			if (visible == true)
				GetTelemetryInterface()->Metric_IGO_OVERLAY_START();
			else
				GetTelemetryInterface()->Metric_IGO_OVERLAY_END();

#ifdef SHOW_IPC_MESSAGES
            ORIGIN_LOG_EVENT << "IPC(MSG_IGO_MODE - " << conn->id() << "): " << visible;
#endif
			mWindowManager->injectIGOVisible(visible, emitStateChange);

            if(!visible && emitStateChange)
            {
                mWindowManager->closeWindowsOnIgoClose();
            }
            
            //emit reset to force pinnable widgtes to update it's alpha value
            mWindowManager->invalidate();
		}
		break;
	case IGOIPC::MSG_IGO_KEYBOARD_LANGUAGE:
		{
#ifdef SHOW_IPC_MESSAGES
            ORIGIN_LOG_EVENT << "IPC(MSG_IGO_KEYBOARD_LANGUAGE - " << conn->id() << ")";
#endif
			IGOIPC *ipc = IGOIPC::instance();

			ipc->parseMsgIGOKeyboardLanguage(msg.get(), &mIGOKeyboardLanguage);

#if defined(ORIGIN_PC)
			if (mIGOKeyboardLanguage!=0)
				ActivateKeyboardLayout((HKL)mIGOKeyboardLanguage, KLF_SETFORPROCESS);
#endif
		}
		break;

	case IGOIPC::MSG_IGO_FOCUS:
		{
			IGOIPC *ipc = IGOIPC::instance();

			bool hasFocus = false;
            uint16_t width = 0;
            uint16_t height = 0;
			ipc->parseMsgIGOFocus(msg.get(), hasFocus, width, height);
            DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
			
#ifdef SHOW_IPC_MESSAGES
            ORIGIN_LOG_EVENT << "IPC(MSG_IGO_FOCUS - " << conn->id() << "): " << hasFocus << ", width=" << width << " height=" << height;
#endif

			// update our focus list
			mGames[conn->id()].hasFocus = hasFocus;
			
			// check all games that have IGO if anyone of them has the focus...
			if (!hasFocus)
			{
				hasFocus = updateCurrentConnection();
			}
			else
			{
                mCurrentConnection = conn;
                
                // Make sure we use the correct screen dims if we have multiple games running at once
                if (hasFocus)
                    emit mWindowManager->igoScreenResize(conn->id(), width, height, false);
                
				emit mWindowManager->igoUpdateActiveGameData(conn->id());
			}
			// update our global variable that controls API hooking
			// *no focus means we can disable certain API hooks
			if (mIsRunning != hasFocus)
				mIsRunning = hasFocus;

#if defined(ORIGIN_PC)
			if (mIsRunning)
			{
				// set IGO's keyboard language to the Origin client
				if (mIGOKeyboardLanguage!=0)
					ActivateKeyboardLayout((HKL)mIGOKeyboardLanguage, KLF_SETFORPROCESS);
			}
			else
			{
				// reset Origin's keyboard language
				if (mOriginKeyboardLanguage!=0)
					ActivateKeyboardLayout((HKL)mOriginKeyboardLanguage, KLF_SETFORPROCESS);
			}
#endif
		}
        break;

    case IGOIPC::MSG_IGO_PIN_TOGGLE:
		{
			IGOIPC *ipc = IGOIPC::instance();
			bool pinState = false;
            ipc->parseMsgTogglePins(msg.get(), pinState);

            emit mWindowManager->igoTogglePins(pinState);
        }
        break;

    case IGOIPC::MSG_IGO_OPEN_ATTEMPT_IGNORED:
        {
            IGOIPC *ipc = IGOIPC::instance();
            if(ipc->parseMsgUserIGOOpenAttemptIgnored(msg.get()))
            {
                emit mWindowManager->igoUserIgoOpenAttemptIgnored();
            }
        }
        break;
        
    case IGOIPC::MSG_GAME_PRODUCT_ID:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_PRODUCT_ID - %d)", conn->id());
#endif
            const char16_t* productId = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameProductId(msg.get(), productId, length))
                IGOController::instance()->gameTracker()->restoreProductId(conn->id(), productId, length);
        }
            break;

    case IGOIPC::MSG_GAME_ACHIEVEMENT_SET_ID:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_ACHIEVEMENT_SET_ID - %d)", conn->id());
#endif
            const char16_t* achievementSetId = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameAchievementSetId(msg.get(), achievementSetId, length))
                IGOController::instance()->gameTracker()->restoreAchievementSetId(conn->id(), achievementSetId, length);
        }
            break;

    case IGOIPC::MSG_GAME_EXECUTABLE_PATH:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_EXECUTABLE_PATH - %d)", conn->id());
#endif
            const char16_t* executablePath = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameExecutablePath(msg.get(), executablePath, length))
                IGOController::instance()->gameTracker()->restoreExecutablePath(conn->id(), executablePath, length);
        }
            break;

    case IGOIPC::MSG_GAME_TITLE:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_TITLE - %d)", conn->id());
#endif
            const char16_t* title = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameTitle(msg.get(), title, length))
                IGOController::instance()->gameTracker()->restoreTitle(conn->id(), title, length);
        }
            break;
            
    case IGOIPC::MSG_GAME_DEFAULT_URL:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_DEFAULT_URL - %d)", conn->id());
#endif
            const char16_t* url = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameDefaultURL(msg.get(), url, length))
                IGOController::instance()->gameTracker()->restoreDefaultURL(conn->id(), url, length);
        }
            break;
            
    case IGOIPC::MSG_GAME_MASTER_TITLE:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_MASTER_TITLE - %d)", conn->id());
#endif
            const char16_t* title = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameMasterTitle(msg.get(), title, length))
                IGOController::instance()->gameTracker()->restoreMasterTitle(conn->id(), title, length);
        }
            break;
            
    case IGOIPC::MSG_GAME_MASTER_TITLE_OVERRIDE:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_MASTER_TITLE_OVERRIDE - %d)", conn->id());
#endif
            const char16_t* title = NULL;
            size_t length = 0;
            if (IGOIPC::instance()->parseMsgGameMasterTitleOverride(msg.get(), title, length))
                IGOController::instance()->gameTracker()->restoreMasterTitleOverride(conn->id(), title, length);
        }
            break;

        case IGOIPC::MSG_GAME_REQUEST_INFO_COMPLETE:
        {
#ifdef SHOW_IPC_MESSAGES
            IGO_TRACEF("IPC(MSG_GAME_REQUEST_INFO_COMPLETE - %d)", conn->id());
#endif
            if (IGOIPC::instance()->parseMsgGameRequestInfoComplete(msg.get()))
                IGOController::instance()->gameTracker()->restoreComplete(conn->id());
        }
            break;

    default:
        break;
	}
    
    return retVal;
}

bool IGOWindowManagerIPC::updateCurrentConnection()
{
	bool hasFocus = false;
    DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
	
    mCurrentConnection = NULL;

    GamesCollection::const_iterator iter = mGames.begin();
	while (iter != mGames.end())
	{
		hasFocus = iter->second.hasFocus;
		if (hasFocus)	// if one game has the focus, set it and abort
		{
            mCurrentConnection = iter->second.connection;
            emit mWindowManager->igoUpdateActiveGameData(mCurrentConnection->id());
            
			break;
		}
		++iter;
	}

    if (mCurrentConnection == 0)
    {
        GamesCollection::const_iterator connectionIter = mGames.begin();
        if (connectionIter != mGames.end())
            mCurrentConnection = connectionIter->second.connection;	// set a new default connection...
    }

    return hasFocus;
}

uint32_t IGOWindowManagerIPC::getCurrentConnectionId()
{
    DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
    
	return mCurrentConnection!=NULL ? mCurrentConnection->id() : 0;
}

bool IGOWindowManagerIPC::isConnectionValid(uint32_t connectionId)
{
    DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
    if (connectionId!=0)
    {
        GamesCollection::const_iterator iter = mGames.find(connectionId);
        if (iter != mGames.end())
            return true;

    }
    return false;
}

bool IGOWindowManagerIPC::restart(bool explicitStart)
{
    if (isStarted())
        return true;

    if (explicitStart)
        mExplicitStart = true;

    if (!isStarted() && mExplicitStart && mServer)
    {
        mIsStarted = mServer->start();
        return mIsStarted;
    }
    else
        return true;
}
bool IGOWindowManagerIPC::send(eastl::shared_ptr<IGOIPCMessage> msg, uint32_t connectionId)
{
    if (!isStarted())
        if (!restart(false))
            return false;

	// IGO is in the foreground
	// we dont update window update messages because they are large
	// when the game is in the background
	
	if (!mIsRunning && msg &&
		(msg->type() == IGOIPC::MSG_WINDOW_UPDATE ||
		msg->type() == IGOIPC::MSG_WINDOW_UPDATE_RECTS
		))
	{
		// this code path is a bit critical, when we are in background, IGO may miss windows updates, so we may end up with incomplete windows once we return
		// to work around this problem, the IGO state change from "no focus" to "focus" is requesting all IGO windows !!!
		//delete msg;
		return false;
	}

	// we have IPC connection
    {
        DEBUG_MUTEX_TEST(mConnectionMutex); EA::Thread::AutoFutex m(mConnectionMutex);
        // just send to one connection
        if (connectionId!=0)
        {
            GamesCollection::const_iterator iter = mGames.find(connectionId);
            if (msg && iter != mGames.end() && !iter->second.connection->isClosed())
            {
                iter->second.connection->send(msg);
            }
        }
        else // send to all games
        {
            GamesCollection::const_iterator iter = mGames.begin();
            while (iter != mGames.end())
            {
                if (msg && !iter->second.connection->isClosed())
                {
                    iter->second.connection->send(msg);
                }
                ++iter;

            }
        }
    }
	
	return false;
}

#if defined(ORIGIN_PC)

void IGOWindowManagerIPC::translateDeviceNameToDriveLetters( wchar_t* pszFilename )
{
	const int BUFSIZE = 512;

	// Translate path with device name to drive letters.
	wchar_t szTemp[BUFSIZE];
	szTemp[0] = L'\0';

	if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
	{
		wchar_t szName[MAX_PATH];
		wchar_t szDrive[3] = L" :";
		BOOL bFound = FALSE;
		wchar_t* p = szTemp;

		do 
		{
			// Copy the drive letter to the template string
			*szDrive = *p;

			// Look up each device name
			if (QueryDosDevice(szDrive, szName, MAX_PATH))
			{
				size_t uNameLen = wcslen(szName);

				if (uNameLen < MAX_PATH) 
				{
					bFound = _wcsnicmp(pszFilename, szName, uNameLen) == 0 && *(pszFilename + uNameLen) == L'\\';

					if (bFound) 
					{
						// Reconstruct pszFilename using szTempFile
						// Replace device path with DOS path
						wchar_t szTempFile[MAX_PATH];
						StringCchPrintf(szTempFile,
							MAX_PATH,
							L"%s%s",
							szDrive,
							pszFilename+uNameLen);
						StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, wcsnlen_s(szTempFile, _countof(szTempFile)));
					}
				}
			}

			// Go to the next NULL character.
			while (*p++);
		} while (!bFound && *p); // end of string
	}
}
	
	
bool IGOWindowManagerIPC::getExeFolderNameFromId(QString & folderName, DWORD pid)
{
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if( handle == NULL || handle == INVALID_HANDLE_VALUE )
		return false;

	const int UNICODE_MAX_PATH = 32767;
	wchar_t exePath[UNICODE_MAX_PATH] ={0};
	DWORD bytes = GetProcessImageFileName(handle, exePath, UNICODE_MAX_PATH);
	if( bytes == 0 )
	{
		return false;
	}

	translateDeviceNameToDriveLetters( exePath );

	// extend path length to 32767 wide chars
	QString prependedPath("\\\\?\\");
	prependedPath.append( QString::fromWCharArray(exePath) );

	bytes = GetLongPathName(prependedPath.utf16(), exePath, UNICODE_MAX_PATH);
	if( bytes == 0 )
	{
		return false;
	}

	QString str = QString::fromWCharArray(exePath);
	int pos = str.lastIndexOf("\\");
	folderName = str.left(pos);

	// remove trailing path separators
	while ( folderName.endsWith(QDir::separator(), Qt::CaseInsensitive) )
		folderName.chop(1);
	
	::CloseHandle(handle);

	return true;
}

#elif defined(ORIGIN_MAC) // MAC OSX

bool IGOWindowManagerIPC::getExeFolderNameFromId(QString & folderName, DWORD pid)
{
    folderName.clear();

    char path[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, path, sizeof(path)))
    {
        folderName = QDir::cleanPath(QString(path));
        return true;
    }
    
    else
    {
        IGO_TRACEF("Unable to retrieve executable path from pid=%ld (%s)", pid, strerror(errno));
        return false;
    }
}

#endif
    
} // Engine
} // Origin
