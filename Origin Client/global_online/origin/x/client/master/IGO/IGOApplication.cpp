///////////////////////////////////////////////////////////////////////////////
// IGOApplication.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "IGO.h"
#include "IGOApplication.h"

#include "IGOTrace.h"
#include "IGOWindow.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCClient.h"
#include "IGOIPC/IGOIPCMessage.h"
#include "HookAPI.h"
#include "IGOInfoDisplay.h"

#if !defined(ORIGIN_MAC) // FIXME: NEED TO PORT OVER
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "InputHook.h"
#include "DX12Hook.h"
#endif

#if defined(ORIGIN_MAC)
#import <OpenGL/OpenGL.h>
#include "MacInputHook.h"
#include "MacCursorHook.h"
#include "MacRenderHook.h"
#include "OGLHook.h"
#endif

#include "IGOLogger.h"
#include "IGOSharedStructs.h"
#include "EASTL/functional.h"

namespace OriginIGO {

    // detect concurrencies - assert if our mutex is already locked
#ifdef _DEBUG
    #define DEBUG_MUTEX_TEST(x){\
	    bool s=x.TryLock();\
	    if(s)\
		    x.Unlock();\
        else \
            IGOLogWarn("IGO threading lock");\
	    }
#else
    #define DEBUG_MUTEX_TEST(x) void(0)
#endif

    // This is just here so I can turn it back on if there's another OIG key
    // bug in the near future. I'm going to remove it once we release 9.0
    #if 1
    #define keyDebugAdd(a,b,c,d,e)
    #else
    typedef struct keyDebug_t
    {
        char context[5];
        int info[4];
    } keyDebug_t;

    #define KEYDEBUGS_SIZE 200
    static keyDebug_t keyDebugs[KEYDEBUGS_SIZE];
    static int keyDebugIdx = 0;

    static void keyDebugAdd(char* context, int info1, int info2, int info3, int info4)
    {
        // Clamp and increment the circular index first, so we're guaranteed
        // it's valid. This also means the index points to the last item written.
        if (keyDebugIdx < 0 || keyDebugIdx >= (KEYDEBUGS_SIZE - 1))
            keyDebugIdx = 0;
        else
            keyDebugIdx++;

        strncpy(keyDebugs[keyDebugIdx].context, context, 4);
        keyDebugs[keyDebugIdx].context[4] = '\0';
        keyDebugs[keyDebugIdx].info[0] = info1;
        keyDebugs[keyDebugIdx].info[1] = info2;
        keyDebugs[keyDebugIdx].info[2] = info3;
        keyDebugs[keyDebugIdx].info[3] = info4;
    }
    #endif



    #if defined(ORIGIN_PC)
    extern HWND gHwnd;
    #endif

    extern DWORD gIGOShowCursor;
    extern DWORD gIGOUpdateMessage;
    extern DWORD gIGOSwitchGameToBackground;
    extern volatile DWORD gPresentHookCalled;

    #if !defined(ORIGIN_MAC)
    extern DWORD gIGOUpdateMessageTime;
    #else
    DWORD gIGOUpdateMessageTime = 0;
    #endif

    extern BYTE gIGOKeyboardData[256];
    extern POINT gSavedMousePos;
    extern int gShowCursorCount;

    #if defined(ORIGIN_PC)
    extern HCURSOR gPrevCursor;
    #endif

    extern void releaseInputs();
    extern void resetRawInputDeviceFlags();
    extern void restoreRawInputDeviceFlags();

    IGOApplication * volatile IGOApplication::mInstance = NULL;
    uint32_t IGOApplication::mGFXThread = 0;

    bool IGOApplication::mPendingInstanceDelete = false;
       
#if defined(ORIGIN_PC)
    EA::Thread::RWSpinLock IGOApplication::mInstanceMutex;
    EA::Thread::RWSpinLock IGOApplication::mInstanceOwnerMutex;
	EA::Thread::Futex IGOApplication::mHelperMutex;
#else
    EA::Thread::Futex IGOApplication::mInstanceMutex;
#endif

    #if defined(ORIGIN_PC)

    EXTERN_NEXT_FUNC(HCURSOR, SetCursorHook, (HCURSOR hCursor));
    EXTERN_NEXT_FUNC(int, ShowCursorHook, (BOOL bShow));
    EXTERN_NEXT_FUNC(BOOL, SetCursorPosHook, (int X, int Y));
    EXTERN_NEXT_FUNC(BOOL, GetCursorPosHook, (LPPOINT lpPoint));
    EXTERN_NEXT_FUNC(SHORT, GetAsyncKeyStateHook, (int vKey));
    EXTERN_NEXT_FUNC(SHORT, GetKeyStateHook, (int vKey));
    EXTERN_NEXT_FUNC(BOOL, GetKeyboardStateHook, (PBYTE lpKeyState));
    EXTERN_NEXT_FUNC(UINT, SendInputHook, (UINT cInputs, LPINPUT pInputs, int cbSize));


    #define SetCursorFunc(cursor) ((SetCursorHookNext && isSetCursorHooked) ? SetCursorHookNext(cursor) : SetCursor(cursor))
    #define ShowCursorFunc(visible) ((ShowCursorHookNext && isShowCursorHooked) ? ShowCursorHookNext(visible) : ShowCursor(visible))
    #define SetCursorPosFunc(X, Y) ((SetCursorPosHookNext && isSetCursorPosHooked) ? SetCursorPosHookNext(X, Y) : SetCursorPos(X, Y))
    #define GetCursorPosFunc(point) ((GetCursorPosHookNext && isGetCursorPosHooked) ? GetCursorPosHookNext(point) : GetCursorPos(point))
    #define GetAsyncKeyStateFunc(key) ((GetAsyncKeyStateHookNext && isGetAsyncKeyStateHooked) ? GetAsyncKeyStateHookNext(key) : GetAsyncKeyState(key))
    #define GetKeyStateFunc(key) ((GetKeyStateHookNext && isGetKeyStateHooked) ? GetKeyStateHookNext(key) : GetKeyState(key))
    #define GetKeyboardStateFunc(lpState) ((GetKeyboardStateHookNext && isGetKeyboardStateHooked) ? GetKeyboardStateHookNext(lpState) : GetKeyboardState(lpState))
    #define SendInputFunc(cInputs, pInputs, cbSize) ((SendInputHookNext && isSendInputHooked) ? SendInputHookNext(cInputs, pInputs, cbSize) : SendInput(cInputs, pInputs, cbSize))

    #endif // !MAC OSX


    IGOApplication::IGOApplication(RendererType type, void *renderer, cleanupCallback cleanupFunc) :
	    mCleanupCall(cleanupFunc),
        mRendererType(type),
        mIPC(NULL),
        mHaveFocus(false),
        mFocusDefined(false),
        mPendingToggleState(false),
        mPendingIGOReset(false),
        mPendingIGORefresh(false),
        mSkipIGOLocationRestore(false),
        mIsActive(false),
        mActivated(false),
//we need to default the mIsPinKeyActive to true for PC only
//Mac goes through a different path where it needs to start as false (it toggles it before use)
//otherwise the first time you press the toggle to unpin the widgets it will be incorrect
#if defined(ORIGIN_PC)
        mIsPinKeyActive(true),
#else
		mIsPinKeyActive(false),
#endif
        mPinKeyActivated(false),
        mIsPinningInProgress(false),
	    mWindowed(false),
        mResetContext(false),
        mDumpWindowBitmaps(false),
	    mLanguageChangeInProgress(false),
        mClientReady(false),
        mSetupCompleted(false),
        mSkippingRenderUpdate(false),
        mSkipReconnection(false),
        mWindowScreenWidth(0),
        mWindowScreenHeight(0),
        mWindowScreenXOffset(0),
	    mWindowScreenYOffset(0),
        mScreenWidth(0),
        mScreenHeight(0),
        mMinIGOScreenWidth(OriginIGO::MIN_IGO_DISPLAY_WIDTH),
        mMinIGOScreenHeight(OriginIGO::MIN_IGO_DISPLAY_HEIGHT),
        mSRGB(false),
        mPixelToPointScaleFactor(1.0f),
        mHotkeySet(0),
        mIgnoreNextKeyUp(false),
        mCursorType(IGOIPC::CURSOR_ARROW),
#if defined(ORIGIN_PC)
        mPrevCursor(NULL),
#endif
        mInfoDisplay(NULL),
        mHideTS(0),
        mGameKeyboardLayout(0),
        mIGOKeyboardLayout(0),
        mWasActive(false),
        mEmitStateChange(true),
		mRenderContext(NULL),
        mAnimDurationScale(1.0f)
    {
	    IGOLogWarn("IGOApplication created %i", (int)mRendererType);

	    mKey[0] = VK_SHIFT;
	    mKey[1] = VK_F1;
	    mKey[2] = VK_ESCAPE; // This will always be Escape
	    mKey[3] = VK_SHIFT;
	    mKey[4] = VK_F2;

	    memset(mCurrentKeyState, 0, sizeof(mCurrentKeyState));
	    memset(mPreviousKeyState, 0, sizeof(mPreviousKeyState));
	
	    mInstance = this;
	    mGFXThread = 0;
	    IGOIPC *ipc = IGOIPC::instance();
#if defined(ORIGIN_MAC)
	    mIPC = ipc->createClient(IGOIPC_REGISTRATION_SERVICE, IGOIPC_SHARED_MEMORY_BUFFER_SIZE);
#elif defined(ORIGIN_PC)
        mIPC = ipc->createClient();
#endif
	    mIPC->setHandler(this);
	    mIPC->start();

	    mInfoDisplay = new IGOInfoDisplay();
    }

	void IGOApplication::setRenderContext(void *userData)
	{
		mRenderContext = userData;
	}

    IGOApplication::~IGOApplication()
    {
		IGOLogWarn("IGOApplication destroyed");
	    mInstance = NULL;	// reset the instance pointer, otherwise we call a "dead" pointer whe using ::instance() 
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("~IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

		clearWindows(mRenderContext);
        
#if defined(ORIGIN_PC)
        if (isActive()) // disable IGO, to restore original game states
        {
            toggleIGO();

#if defined(ORIGIN_PC)
            if (InputHook::IsMessagHookActive())
            {
                if (IsWindowUnicode(gHwnd))
                    PostMessageW(gHwnd, gIGOUpdateMessage, 0, 0);	// send a dummy message to invoke the games main message loop and update IGO 
                else
                    PostMessageA(gHwnd, gIGOUpdateMessage, 0, 0);	// send a dummy message to invoke the games main message loop and update IGO 
                Sleep(500); // give the message loop some time to process the posted message
            }
            else
#endif
            {
                processPendingIGOToggle();
            }
        }
#endif
        
	    // clear the ipc handler
		mIPC->stop(mRenderContext);
	    DELETE_IF(mIPC);
	    DELETE_IF(mInfoDisplay);

        if (mCleanupCall)
			mCleanupCall(mRenderContext);
    }

    bool IGOApplication::isPendingDeletion()
    {
#if defined(ORIGIN_PC)
        EA::Thread::AutoRWSpinLock lock(mInstanceMutex, EA::Thread::AutoRWSpinLock::kLockTypeRead);
#else
        EA::Thread::AutoFutex m(mInstanceMutex);
#endif
        return mPendingInstanceDelete;
    }

    IGOApplication* IGOApplication::instance()
    { 

#if defined(ORIGIN_PC)
        EA::Thread::AutoRWSpinLock lock(mInstanceMutex, EA::Thread::AutoRWSpinLock::kLockTypeRead);
#else
        EA::Thread::AutoFutex m(mInstanceMutex);
	    if (mPendingInstanceDelete)
	    {
		    // if we get a deleting request, do it and exit
		    deleteInstance();
		    return NULL;
	    }
#endif   
        return mInstance; 
    }

    void IGOApplication::createInstance(RendererType rendererType, void *renderer, cleanupCallback cleanupFunc) 
    { 
        IGO_ASSERT(renderer!=NULL);

#if defined(ORIGIN_PC)
		{
        EA::Thread::AutoRWSpinLock lock(mInstanceMutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite);

        bool isOpenGL = rendererType == RendererOGL;

        if (isOpenGL)
        {
            if (!mInstance && !mInstanceOwnerMutex.IsWriteLocked()) 
	        {
                mInstanceOwnerMutex.WriteLock();    // remember the thread that create the instance - openGL is picky!!!
		        mGFXThread = 0;
		        mInstance = new IGOApplication(rendererType, renderer, cleanupFunc); 
	        }
            else
            {
                mPendingInstanceDelete = true;
            }
        }
        else
        {
            if (!mInstance)
            {
		        mGFXThread = 0;
		        mInstance = new IGOApplication(rendererType, renderer, cleanupFunc); 
	        }
        }
		}

#else
        EA::Thread::AutoFutex m(mInstanceMutex);
		mGFXThread = 0;
		mInstance = new IGOApplication(rendererType, renderer); 

#endif 

    }

	void IGOApplication::deleteInstance(void *userData)
    {
#if defined(ORIGIN_PC)
		IGOLogWarn("IGO deletion started.");
        EA::Thread::AutoRWSpinLock lock(mInstanceMutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite);

        if (mInstance)
        {
            bool isOpenGL = mInstance->rendererType() == RendererOGL;
            
            if (isOpenGL)
            {
                //WriteTryLock always returns false when the current thread already has a lock
                if (!mInstanceOwnerMutex.WriteTryLock()) // make sure only the thread that created the instance can delete it!!!!  - openGL is picky!!!
                {
	                mPendingInstanceDelete = false; 
					mInstance->setRenderContext(userData);
	                DELETE_IF(mInstance);
                    IGOLogWarn("IGO destroyed.");

	                mGFXThread = 0;
                    mInstanceOwnerMutex.WriteUnlock(); // lock from IGOApplication::createInstance
                }
            }
            else
            {
	            mPendingInstanceDelete = false; 
				mInstance->setRenderContext(userData);
	            DELETE_IF(mInstance);
                IGOLogWarn("IGO destroyed.");

	            mGFXThread = 0;
            }
        }
        else
            IGOLogWarn("IGO destroyed (no instance found!)");

#else
        EA::Thread::AutoFutex m(mInstanceMutex);
	    mPendingInstanceDelete = false;
	    DELETE_IF(mInstance);
        IGOLogWarn("IGO destroyed.");

	    mGFXThread = 0;
#endif 
    }

    IGOWindow *IGOApplication::getWindow(uint32_t id, bool* created)
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }
	
        IGOWindow* window = IGOWindowGovernor::instance()->create(id, mRendererType, created);
	    return window;
    }

   	bool IGOApplication::getWindowIDs(char* buffer, size_t bufferSize)
	{
        return IGOWindowGovernor::instance()->getWindowIDs(buffer, bufferSize);
		}

	void IGOApplication::closeWindow(uint32_t id)
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

        IGOWindowGovernor::instance()->close(id);
	    }

    void IGOApplication::clearWindows(void *context)
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

        IGOWindowGovernor::instance()->clear(context);
    }

    void IGOApplication::refreshWindows(eastl::vector<uint32_t>& windowIDs)
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

        IGOWindowGovernor::instance()->refresh(mRendererType, windowIDs);
        
    }

    void IGOApplication::moveWindow(uint32_t windowId, uint32_t index)
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

        IGOWindowGovernor::instance()->move(windowId, index);
		    }

    // force reset/refresh to be called only from the rendering thread!!!
    void IGOApplication::reset()
    {
	    mPendingIGOReset = true;
    }

    void IGOApplication::forceRefresh()
    {
        mPendingIGOReset = true;
        mPendingIGORefresh = true;
    }
    
	void IGOApplication::processPendingIGOreset(void *context)
	{
		if (mPendingIGOReset)
		{
            if (!mPendingIGORefresh)
            {
                IGOLogInfo("IGOApplication reset (%d/%d)", (uint16_t)mScreenWidth, (uint16_t)mScreenHeight);
                clearWindows(context);
            }
            else
            {
                IGOLogInfo("IGOApplication refresh (%d/%d)", (uint16_t)mScreenWidth, (uint16_t)mScreenHeight);
                mPendingIGORefresh = false;
            }
            
		    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgReset((uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
		    mIPC->send(msg);
            
		    mPendingIGOReset = false;
	    }
    }

	void IGOApplication::unrender(void *context)
	{
        IGOWindowGovernor::instance()->unrender(context, this);
		}

	void IGOApplication::render(void *context)
    {
        // set up logging flag - do it here, because we always want to log the starting phase !
        if (OriginIGO::IGOLogger::instance())
        {
            OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
        }

        /*
        if (mGFXThread != 0)
        {
    	   if (mGFXThread != GetCurrentThreadId())
           {
               deleteInstanceLater();
               return;
           }
        }
        */
	    mGFXThread = GetCurrentThreadId();
	    processPendingIGOreset(context);

#if defined(ORIGIN_MAC)
	    doHotKeyCheck();
#endif

	    if (mPendingInstanceDelete)
	    {
		    // if we get a deleting request, do it and exit
#if defined(ORIGIN_MAC)
            deleteInstance();
#else
            if (SAFE_CALL_LOCK_ISREADLOCK_COND)
            {
                SAFE_CALL_LOCK_LEAVE
					deleteInstance(context);
                SAFE_CALL_LOCK_ENTER
            }
            else
            {
				deleteInstance(context);
            }
#endif
            return;
	    }

        
	    // check and process any messages
        // ForMac, although we may want to skip the rendering, we still want to process some of the IPC messages
	    if (mIPC)
        {
            // Only PC right - check whether the Twitch stats have changed -> send them over
            GameLaunchInfo* info = gameInfo();
            if (info->broadcastStatsChanged())
            {
                bool isBroadcasting;
                uint64_t streamId;
                uint32_t minViewers;
                uint32_t maxViewers;
                const char16_t* channel;
                
                info->broadcastStats(isBroadcasting, streamId, &channel, minViewers, maxViewers);
                eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcastStats(isBroadcasting, streamId, minViewers, maxViewers, channel, info->gliStrlen(channel)));
                mIPC->send(msg);
            }
            
		    mIPC->processMessages(context);
        }

#ifdef ORIGIN_MAC
        // Do we need to bypass the update/rendering of the windows - for example because the
        // game window size is too small (but we still need to process some of the messages!)
        if (mSkippingRenderUpdate || !context)
            return;
#else
		if (!mClientReady)
			return;
#endif
        
	    if (mInfoDisplay)
        {
            mInfoDisplay->update(context);
        }

        // get our animation delta
        mAnimDelta.Update();
        float deltaT = mAnimDelta.deltaInMS();
        
        IGOWindowGovernor::AppContext appCtxt;
        appCtxt.screenWidth = mWindowScreenWidth;
        appCtxt.screenHeight = mWindowScreenHeight;
        appCtxt.minScreenWidth = mMinIGOScreenWidth;
        appCtxt.minScreenHeight = mMinIGOScreenHeight;
        appCtxt.deltaT = deltaT;
        appCtxt.isActive = isActive();
        appCtxt.hasFocus = mHaveFocus;
        IGOWindowGovernor::instance()->render(context, this, appCtxt);

#ifdef ORIGIN_PC
	    // send this message pump always! (hotkey check, named pipe check, etc.)
	    if ((GetTickCount() - gIGOUpdateMessageTime) > 250)	// 4 times a second is enough, to force a win32 message pump event
		    enforceIGOMessagePump();
#endif
    }

    void IGOApplication::setScreenSize(uint32_t width, uint32_t height)
    {
        setScreenSizeAndRenderState(width, height, 1.0f, false);
    }

    void IGOApplication::setScreenSizeAndRenderState(uint32_t width, uint32_t height, float pixelToPointScaleFactor, bool skipRenderUpdate)
    {
#ifdef ORIGIN_MAC
        static uint32_t prevWidth = 0;
        static uint32_t prevHeight = 0;
        
        // Don't send this before the connection is setup (ie we received the MSG_START and we responded with
        // the initial screen size, except if we started with a window too small - use Bejeweled 3 to test this)
        mSkippingRenderUpdate = skipRenderUpdate;
        if (!mSetupCompleted && mClientReady && !skipRenderUpdate)
            mSetupCompleted = true;
        
        if (mSetupCompleted && (prevWidth != width || prevHeight != height))
        {
            prevWidth = width;
            prevHeight = height;
            
            IGOLogInfo("IGOApplication screens size = %dx%d", width, height);
            
            eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgScreenSize((uint16_t)width, (uint16_t)height));
            send(msg);
        }
	    // usually the game screen aka. back buffer has the same size as the front buffer, but some games like Spore do some scaling in windowed mode
	    mWindowScreenXOffset = mWindowScreenYOffset = 0;
	    mWindowScreenWidth = mScreenWidth = width;
	    mWindowScreenHeight = mScreenHeight = height;
        mPixelToPointScaleFactor = pixelToPointScaleFactor;
#endif // ORIGIN_MAC
        
		// only set the back buffer values, if we are "ready"
        if (mClientReady && ((mScreenWidth!=width || mScreenHeight!=height)))
		{
			// usually the game screen aka. back buffer has the same size as the front buffer, but some games like Spore do some scaling in windowed mode
			mWindowScreenXOffset = mWindowScreenYOffset = 0;
			mWindowScreenWidth = mScreenWidth = width;
			mWindowScreenHeight = mScreenHeight = height;
            
            IGOLogInfo("IGOApplication screens size = %dx%d", width, height);

            // hide IGO if we switch to a resolution below IGO minimum
            if (isActive() && (mWindowScreenWidth < mMinIGOScreenWidth || mWindowScreenHeight < mMinIGOScreenHeight))
			{
                toggleIGO();
			}

		    reset();	// retrieve all windows again, once we have the focus back, because a filter in IGOWindowManagerIPC::send(...)
					    // might have blocked large updates, while the IGO featuring game was in background!!!

		}
    }

    void IGOApplication::setWindowedMode(bool windowed)
    {
	    mWindowed = windowed;
    }

    void IGOApplication::setWindowScreenSize(uint32_t width, uint32_t height)
    {
#ifdef ORIGIN_PC
		if (mClientReady)
		{
#endif
			if (mWindowed)
			{
				mWindowScreenXOffset = 0;
				mWindowScreenYOffset = 0;
				mWindowScreenWidth = width;
				mWindowScreenHeight = height;
			}
			else
			{
				// only use the offset values, if our real window is larger than the backbuffer, otherwise the back buffer has the sizing priority
				// and we do not need the offsets (fifa09 for example)
				if (width>mScreenWidth || height>mScreenHeight)
				{
					mWindowScreenXOffset = mScreenWidth - width;
					mWindowScreenYOffset = mScreenHeight - height;
				}
		
				mWindowScreenWidth = mScreenWidth;
				mWindowScreenHeight = mScreenHeight;
			}
#ifdef ORIGIN_PC
		}
#endif

        static uint32_t prevWidth = 0;
        static uint32_t prevHeight = 0;
        if (prevWidth != width || prevHeight != height)
        {
            prevWidth = width;
            prevHeight = height;
            
            IGOLogInfo("IGOApplication window screens size = %dx%d", width, height);
            IGOLogInfo("IGOApplication window screens offset = %d  %d", mWindowScreenXOffset, mWindowScreenYOffset);
            IGOLogInfo("IGOApplication window screens scaling = %d  %d", (int)(getScalingX()*1000000.0f), (int)(getScalingY()*1000000.0f));
        }
    }

#ifdef ORIGIN_MAC

    const char *IGOApplication::rendererString()
    {
        switch (mRendererType)
        {
            case RendererOGL:
                return "OGL";
                
            default:
                return "Unknown";
        }
    }
    
#else

    const WCHAR *IGOApplication::rendererString()
    {
	    switch (mRendererType)
	    {
	    case RendererDX8:
		    return L"DX8";
	    case RendererDX9:
		    return L"DX9";
	    case RendererDX10:
		    return L"DX10";
	    case RendererDX11:
		    return L"DX11";
        case RendererOGL:
            return L"OGL";
        case RendererMantle:
            return L"MANTLE";
        }
	    return L"Unknown";
    }

#endif // ORIGIN_MAC
    
    void IGOApplication::send(eastl::shared_ptr<IGOIPCMessage> msg)
    {
	    if (!mPendingInstanceDelete && mIPC)
                mIPC->send(msg);
    }

	void IGOApplication::handleDisconnect(IGOIPCConnection *conn, void *userData)
    {
	    if (isActive())
		    toggleIGO();
		void *context = userData;

		clearWindows(context);
    }

	bool IGOApplication::handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData)
    {
        bool retVal = false;
		void *context = userData;
#ifdef ORIGIN_MAC
		if (context)
        {
			((OGLHook::DevContext*)context)->SetRequiresRendering();
        }
#endif

	    switch (msg->type())
        {
		    // we expect to get this first
		    case IGOIPC::MSG_START:
			    {
                    uint32_t key1 = 0;
                    uint32_t key2 = 0;
                    uint32_t key3 = 0;
                    uint32_t key4 = 0;
                    uint32_t minWidth = OriginIGO::MIN_IGO_DISPLAY_WIDTH;
                    uint32_t minHeight = OriginIGO::MIN_IGO_DISPLAY_HEIGHT;

                    bool parsed = IGOIPC::instance()->parseMsgStart(msg.get(), key1, key2, key3, key4, minWidth, minHeight);
                    if (parsed && minWidth > 0 && minHeight > 0)
                    {
                        if (minWidth != mMinIGOScreenWidth || minHeight != mMinIGOScreenHeight)
                        {
                            mMinIGOScreenWidth = minWidth;
                            mMinIGOScreenHeight = minHeight;
                            IGOLogWarn("Setting new min resolution: %d x %d", minWidth, minHeight);
                        }
                    }

				    // send screen size
                    mClientReady = true;
                    if (mSkippingRenderUpdate)
                        return retVal;
                    
                    mSetupCompleted = true;
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_START -> send back width=%d/height=%d", mScreenWidth, mScreenHeight);
#endif
                    if (parsed)
                        setHotKeyMessageHandler(key1, key2, key3, key4);
                    
                    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgScreenSize((uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
                    mIPC->send(msg);
                    
				    createBackgroundDisplay(context);
			    }
			    break;

		    case IGOIPC::MSG_RESET_WINDOWS:
			    {
                    if (mSkippingRenderUpdate)
                        return retVal;

                    eastl::vector<uint32_t> windowIDs;
                    if (IGOIPC::instance()->parseMsgResetWindows(msg.get(), windowIDs))
                    {
#ifdef SHOW_IPC_MESSAGES
                        OriginIGO::IGOLogInfo("IGOIPC::MSG_RESET_WINDOWS (%d)", windowIDs.size());
#endif
                        // Make sure we resize the background if necessary
                        createBackgroundDisplay(context);
                        
                        
                        refreshWindows(windowIDs);
                    }                    
			    }
			    break;

		    case IGOIPC::MSG_NEW_WINDOW:
            {
#ifdef ORIGIN_MAC
                if (mSkippingRenderUpdate)
                    return retVal;
#endif

                uint32_t windowId;
                uint32_t index;
                IGOIPC::instance()->parseMsgNewWindow(msg.get(), windowId, index);
                
#ifdef SHOW_IPC_MESSAGES
                OriginIGO::IGOLogInfo("IGOIPC::MSG_NEW_WINDOW %d (%d)", windowId, index);
#endif
                getWindow(windowId);
                moveWindow(windowId, index);
            }
			    break;

		case IGOIPC::MSG_WINDOW_UPDATE:
			{
#ifdef SHOW_IPC_MESSAGES
				IGOLogDebug("IGOCIP::MSG_WINDOW_UPDATE");
#endif

                //dx9 performance test...
                
				uint32_t windowId = 0;
				bool visible = false;
				uint8_t alpha = 0;
				int16_t x = 0, y = 0;
				uint16_t width = 0, height = 0;
				uint32_t flags = 0;
				const char *data = NULL;
				int size = 0;

#if !defined(ORIGIN_MAC)
				    IGOIPC::instance()->parseMsgWindowUpdate(msg.get(), windowId, visible, alpha, x, y, width, height, data, size, flags);
#else
                    int64_t dataOffset;
                    IGOIPC::instance()->parseMsgWindowUpdateEx(msg.get(), windowId, visible, alpha, x, y, width, height, dataOffset, size, flags);
                    IGOIPCImageBin::AutoReleaseContent image = mIPC->GetImageData(windowId, dataOffset, size);
                    data = image.Get();
                
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_WINDOW_UPDATE wnd=%d, visible=%i, alpha=%i, x=%i, y=%i, width=%i, height=%i, dataSize=%d, flags=%d", windowId, visible, alpha, x, y, width, height, size, flags);
#endif
                    IGOWindow *win = getWindow(windowId);
                    win->update(context, visible, alpha, x, y, width, height, data, size, flags);

#ifdef ORIGIN_MAC
                    //retVal = true;
#endif
			    }
			    break;

		case IGOIPC::MSG_WINDOW_UPDATE_RECTS:
			{
#ifdef SHOW_IPC_MESSAGES
				IGOLogDebug("IGOCIP::MSG_WINDOW_UPDATE_RECTS");
#endif
				uint32_t windowId = 0;
				int size = 0;

#if !defined(ORIGIN_MAC)
					const char *data = NULL;
                    eastl::vector<IGORECT> rects;
				    IGOIPC::instance()->parseMsgWindowUpdateRects(msg.get(), windowId, rects, data, size);
#else
                    int64_t dataOffset;
                    eastl::vector<IGORECT> rects;
				    IGOIPC::instance()->parseMsgWindowUpdateRectsEx(msg.get(), windowId, rects, dataOffset, size);
                    IGOIPCImageBin::AutoReleaseContent image = mIPC->GetImageData(windowId, dataOffset, size);
                    image.Get();
                
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif
                
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_WINDOW_UPDATE_RECTS wnd=%d, dataSize=%d", windowId, size);
#endif
                
#ifndef ORIGIN_MAC // NOT USED RIGHT NOW
                    IGOWindow *win = getWindow(windowId);
                    win->updateRects(context, rects, data, size);
#endif
                
#ifdef ORIGIN_MAC
                    //retVal = true;
#endif
			    }
			    break;

            case IGOIPC::MSG_WINDOW_SET_ANIMATION:
                {
                    if (mSkippingRenderUpdate)
                        return retVal;

                    uint32_t windowId = 0;
                    uint32_t version = 0;
                    uint32_t context = 0;
                    const char* data = NULL;
                    uint32_t dataSize = 0;
                    if (IGOIPC::instance()->parseMsgWindowSetAnimation(msg.get(), windowId, version, context, data, dataSize))
                    {
                        if (version == AnimPropertySet::Version && dataSize == sizeof(AnimPropertySet))
                        {
#ifdef SHOW_IPC_MESSAGES
                            IGOLogDebug("IGOIPC::MSG_WINDOW_SET_ANIMATION %d (%d)", windowId, context);
#endif
                            IGOWindow* window = getWindow(windowId);
                            window->setAnimProps(static_cast<WindowAnimContext>(context), reinterpret_cast<const AnimPropertySet*>(data));
                        }

                        else
                            CALL_ONCE_ONLY(IGOLogWarn("IGOIPC::MSG_WINDOW_SET_ANIMATION mismatch data (version=%d/%d, size=%d/%d)", version, AnimPropertySet::Version, dataSize, sizeof(AnimPropertySet)));
                    }
                }
                break;

		    case IGOIPC::MSG_SET_HOTKEY:
			    {
                    uint32_t key1=0, key2=0, key3=0, key4=0;
                    if (IGOIPC::instance()->parseMsgSetHotkey(msg.get(), key1, key2, key3, key4))
                    {
#ifdef SHOW_IPC_MESSAGES
                        IGOLogDebug("IGOIPC::MSG_SET_HOTKEY %d, %d, %d, %d", key1, key2, key3, key4);
#endif
                        setHotKeyMessageHandler(key1, key2, key3, key4);
                    }
			    }
			    break;

		    case IGOIPC::MSG_SET_CURSOR:
			    {
#ifdef ORIGIN_MAC
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

				    IGOIPC::instance()->parseMsgSetCursor(msg.get(), mCursorType);
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_SET_CURSOR %d", mCursorType);
#endif

				    if (isActive())
					    updateCursor(mCursorType);
			    }
			    break;

		    case IGOIPC::MSG_STOP:
			    {
#ifdef SHOW_IPC_MESSAGES
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_STOP");
#endif
				    // We disable IGO because Ebisu quit
			    }
			    break;

		    case IGOIPC::MSG_SET_IGO_MODE:
			    {
#ifdef ORIGIN_MAC
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_SET_IGO_MODE");
#endif
				    bool active = false;
				    IGOIPC::instance()->parseMsgSetIGOMode(msg.get(), active);
				    if (isActive() != active)
                    {
                        // If we're enabling via command then we shouldn't be
                        // looking for a hotkey key-up.
                        if (!isActive())
                        {
                            mActivated = false;
                        }

                        toggleIGO();
                    }
			    }
			    break;
		
            case IGOIPC::MSG_SET_IGO_ISPINNING:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_SET_IGO_ISPINNING");
#endif
                    IGOIPC::instance()->parseMsgIsPinning(msg.get(), mIsPinningInProgress);
                }
                break;
            
            
            case IGOIPC::MSG_OPEN_BROWSER:
			    {
#ifdef ORIGIN_MAC
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_OPEN_BROWSER");
#endif
				    eastl::string url;
				    IGOIPC::instance()->parseMsgOpenBrowser(msg.get(), url);
				    if (!isActive())
					    toggleIGO();
				    // TODO:
				    //open new web browser
			    }
			    break;

		    case IGOIPC::MSG_MOVE_WINDOW:
			    {
#ifdef ORIGIN_MAC
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

				    uint32_t windowId;
                    uint32_t index;
				    IGOIPC::instance()->parseMsgMoveWindow(msg.get(), windowId, index);

#ifdef SHOW_IPC_MESSAGES
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_MOVE_WINDOW %d (%d)", windowId, index);
#endif

				    moveWindow(windowId, index);
			    }
			    break;

		    case IGOIPC::MSG_CLOSE_WINDOW:
			    {
#ifdef ORIGIN_MAC
                    if (mSkippingRenderUpdate)
                        return retVal;
#endif

				    uint32_t windowId;
				    IGOIPC::instance()->parseMsgCloseWindow(msg.get(), windowId);

#ifdef SHOW_IPC_MESSAGES
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_CLOSE_WINDOW %d", windowId);
#endif

				    closeWindow(windowId);
			    }
			    break;

            case IGOIPC::MSG_LOGGING_ENABLED:
                {
                    bool enabled = false;
                    IGOIPC::instance()->parseMsgLoggingEnabled(msg.get(), enabled);

#ifdef SHOW_IPC_MESSAGES
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_LOGGING_ENABLED %d", enabled);
#endif
                    OriginIGO::IGOLogger::instance()->enableLogging(enabled);
                }
                break;

#if defined(ORIGIN_PC)

		    case IGOIPC::MSG_IGO_BROADCAST:
			    {
                    bool start = false;
                    int resolution = 0;
                    int framerate = 0;
                    int quality = 0;
                    int bitrate = 0;
                    bool showViewersNum = false;
                    bool broadcastMic = false;
                    bool micVolumeMuted = false;
                    int micVolume = 0;
                    bool gameVolumeMuted = false;
                    int gameVolume = 0;
					bool useOptEncoder = false;
                    eastl::string token;

                    IGOIPC::instance()->parseMsgBroadcast(msg.get(), start, resolution, framerate, quality, bitrate, showViewersNum, broadcastMic, micVolumeMuted, micVolume, gameVolumeMuted, gameVolume, useOptEncoder, token);
#ifdef SHOW_IPC_MESSAGES
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST %d %d %d %d %d %d %d %d %d %d %d %d %s", start, resolution, framerate, quality, bitrate, showViewersNum, broadcastMic, micVolumeMuted, micVolume, gameVolumeMuted, gameVolume, useOptEncoder, token.c_str());
#endif
                    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST %d", start);
                    if (rendererType() == RendererDX8)
                    {
                        if (start == true)
                        {
                            TwitchManager::SetBroadcastError(0, TwitchManager::ERROR_CATEGORY_UNSUPPORTED); // unsupported gfx API
                            OriginIGO::IGOLogWarn("IGOIPC::MSG_IGO_BROADCAST not supported");
                        }
                        return true;
                    }

                    if (start == true)  // we ignore settings values on stop events!!
                        TwitchManager::SetBroadcastSettings(resolution, framerate, quality, bitrate, showViewersNum, broadcastMic, micVolumeMuted, micVolume, gameVolumeMuted, gameVolume, useOptEncoder, token);
                    
                    TwitchManager::SetBroadcastCommand(start);
			    }
			    break;

		    case IGOIPC::MSG_IGO_BROADCAST_MIC_VOLUME:
			    {
                    int volume = 0;

                    IGOIPC::instance()->parseMsgBroadcastMicVolume(msg.get(), volume);
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST_MIC_VOLUME %d", volume);
#endif
                    TwitchManager::SetBroadcastMicVolumeSetting(volume);
			    }
			    break;

		    case IGOIPC::MSG_IGO_BROADCAST_GAME_VOLUME:
			    {
                    int volume = 0;

                    IGOIPC::instance()->parseMsgBroadcastGameVolume(msg.get(), volume);
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST_GAME_VOLUME %d", volume);
#endif
                    TwitchManager::SetBroadcastGameVolumeSetting(volume);
			    }
			    break;

		    case IGOIPC::MSG_IGO_BROADCAST_MIC_MUTED:
			    {
                    bool muted = false;

                    IGOIPC::instance()->parseMsgBroadcastMicMuted(msg.get(), muted);
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST_MIC_MUTED %d", muted);
#endif
                    TwitchManager::SetBroadcastMicMutedSetting(muted);
			    }
			    break;

		    case IGOIPC::MSG_IGO_BROADCAST_GAME_MUTED:
			    {
                    bool muted = false;

                    IGOIPC::instance()->parseMsgBroadcastGameMuted(msg.get(), muted);
#ifdef SHOW_IPC_MESSAGES
				    OriginIGO::IGOLogInfo("IGOIPC::MSG_IGO_BROADCAST_GAME_MUTED %d", muted);
#endif
                    TwitchManager::SetBroadcastGameMutedSetting(muted);
			    }
			    break;

            case IGOIPC::MSG_IGO_BROADCAST_RESET_STATS:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_IGO_BROADCAST_RESET_STATS");
#endif
                    if (IGOIPC::instance()->parseMsgBroadcastResetStats(msg.get()))
                        gameInfo()->resetBroadcastStats();
                }
                break;

#endif // ORIGIN_PC
                
			case IGOIPC::MSG_FORCE_GAME_INTO_BACKGROUND:
			    {

#ifdef SHOW_IPC_MESSAGES
				    IGOLogDebug("IGOIPC::MSG_FORCE_GAME_INTO_BACKGROUND");
#endif
#if defined(ORIGIN_PC)
                    if (IsWindowUnicode(gHwnd))
                        PostMessageW(gHwnd, gIGOSwitchGameToBackground, 0, 0);
                    else
                        PostMessageA(gHwnd, gIGOSwitchGameToBackground, 0, 0);
#endif
			    }
			    break;

            case IGOIPC::MSG_GAME_REQUEST_INFO:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_REQUEST_INFO");
#endif
                    if (IGOIPC::instance()->parseMsgGameRequestInfo(msg.get()))
                        sendGameInfo();
                }
                break;
                
            case IGOIPC::MSG_GAME_PRODUCT_ID:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_PRODUCT_ID");
#endif
                    const char16_t* productId = NULL;
                    size_t length = 0;
                    if (IGOIPC::instance()->parseMsgGameProductId(msg.get(), productId, length))
                        gameInfo()->setProductId(productId, length);
                }
                break;

            case IGOIPC::MSG_GAME_ACHIEVEMENT_SET_ID:
            {
#ifdef SHOW_IPC_MESSAGES
                IGOLogDebug("IGOIPC::MSG_GAME_ACHIEVEMENT_SET_ID");
#endif
                const char16_t* achievementSetId = NULL;
                size_t length = 0;
                if (IGOIPC::instance()->parseMsgGameAchievementSetId(msg.get(), achievementSetId, length))
                    gameInfo()->setAchievementSetId(achievementSetId, length);
            }
                break;

            case IGOIPC::MSG_GAME_EXECUTABLE_PATH:
            {
#ifdef SHOW_IPC_MESSAGES
                IGOLogDebug("IGOIPC::MSG_GAME_EXECUTABLE_PATH");
#endif
                const char16_t* executablePath = NULL;
                size_t length = 0;
                if (IGOIPC::instance()->parseMsgGameExecutablePath(msg.get(), executablePath, length))
                    gameInfo()->setExecutablePath(executablePath, length);
            }
                break;

            case IGOIPC::MSG_GAME_TITLE:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_TITLE");
#endif
                    const char16_t* title = NULL;
                    size_t length = 0;
                    if (IGOIPC::instance()->parseMsgGameTitle(msg.get(), title, length))
                        gameInfo()->setTitle(title, length);
                }
                break;

            case IGOIPC::MSG_GAME_DEFAULT_URL:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_DEFAULT_URL");
#endif
                    const char16_t* url = NULL;
                    size_t length = 0;
                    if (IGOIPC::instance()->parseMsgGameDefaultURL(msg.get(), url, length))
                        gameInfo()->setDefaultURL(url, length);
                }
                break;

            case IGOIPC::MSG_GAME_MASTER_TITLE:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_MASTER_TITLE");
#endif
                    const char16_t* title = NULL;
                    size_t length = 0;
                    if (IGOIPC::instance()->parseMsgGameMasterTitle(msg.get(), title, length))
                        gameInfo()->setMasterTitle(title, length);
                }
                break;
                
            case IGOIPC::MSG_GAME_MASTER_TITLE_OVERRIDE:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_MASTER_TITLE_OVERRIDE");
#endif
                    const char16_t* title = NULL;
                    size_t length = 0;
                    if (IGOIPC::instance()->parseMsgGameMasterTitleOverride(msg.get(), title, length))
                        gameInfo()->setMasterTitleOverride(title, length);
                }
                break;

            case IGOIPC::MSG_GAME_IS_TRIAL:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_IS_TRIAL");
#endif
                    bool isTrial = false;
                    if (IGOIPC::instance()->parseMsgGameIsTrial(msg.get(), isTrial))
                        gameInfo()->setIsTrial(isTrial);
                }
                break;

            case IGOIPC::MSG_GAME_TRIAL_TIME_REMAINING:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_TRIAL_TIME_REMAINING");
#endif
                    int64_t timeRemaining = 0;
                    if (IGOIPC::instance()->parseMsgGameTrialTimeRemaining(msg.get(), timeRemaining))
                        gameInfo()->setTimeRemaining(timeRemaining);
                }
                break;

            case IGOIPC::MSG_GAME_INITIAL_INFO_COMPLETE:
                {
#ifdef SHOW_IPC_MESSAGES
                    IGOLogDebug("IGOIPC::MSG_GAME_INITIAL_INFO_COMPLETE");
#endif
                    if (IGOIPC::instance()->parseMsgGameInitialInfoComplete(msg.get()))
                        gameInfo()->setIsValid(true);
                }
                break;

            default:
                {
    #ifdef SHOW_IPC_MESSAGES
                    //OriginIGO::IGOLogInfo("IGOIPC::UNSUPPORTED MESSAGE %d", msg->type());
    #endif
                }
                break;
	    }

        return retVal;
    }

    // Send the current game info if requested (ie the Origin client shut down and now it's trying to reconnect
    void IGOApplication::sendGameInfo()
    {
        eastl::shared_ptr<IGOIPCMessage> msg;
        
        // Make sure we do have something to send!
        GameLaunchInfo* info = gameInfo();
        size_t length = info->gliStrlen(info->productId());
        if (length > 0)
        {
            msg = IGOIPC::instance()->createMsgGameProductId(info->productId(), length);
            mIPC->send(msg);

            msg = IGOIPC::instance()->createMsgGameAchievementSetId(info->achievementSetId(), info->gliStrlen(info->achievementSetId()));
            mIPC->send(msg);

            msg = IGOIPC::instance()->createMsgGameExecutablePath(info->executablePath(), info->gliStrlen(info->executablePath()));
            mIPC->send(msg);

            msg = IGOIPC::instance()->createMsgGameDefaultURL(info->defaultURL(), info->gliStrlen(info->defaultURL()));
            mIPC->send(msg);
            
            msg = IGOIPC::instance()->createMsgGameTitle(info->title(), info->gliStrlen(info->title()));
            mIPC->send(msg);
            
            msg = IGOIPC::instance()->createMsgGameMasterTitle(info->masterTitle(), info->gliStrlen(info->masterTitle()));
            mIPC->send(msg);

            msg = IGOIPC::instance()->createMsgGameMasterTitleOverride(info->masterTitleOverride(), info->gliStrlen(info->masterTitleOverride()));
            mIPC->send(msg);
        }
        
        msg = IGOIPC::instance()->createMsgGameRequestInfoComplete();
        mIPC->send(msg);
    }
    
    // clamp mouse cursor inside the gaming window
    void IGOApplication::restoreMouseCursorClipping()
    {
        // mouse cursor clipping is a game responsibility as of https://developer.origin.com/support/browse/OPUB-3832
    }
			
    void IGOApplication::setMouseCursorClipping()
    {
        // mouse cursor clipping is a game responsibility as of https://developer.origin.com/support/browse/OPUB-3832
    }

    bool IGOApplication::hasFocus()
    {
    #if defined(ORIGIN_PC)

	    HWND hwnd = NULL;
	    ALTTABINFO altTabInfo = {0};
	    altTabInfo.cbSize = sizeof(ALTTABINFO);
	
	    BOOL ok = GetAltTabInfo(hwnd, -1, &altTabInfo, NULL, 0);
	    if (ok)
	    {
		    if (mHaveFocus)
		    {
			    mHaveFocus = false;
			    IGOLogInfo("IGOApplication hasFocus = %d", mHaveFocus);
			    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOFocus(mHaveFocus, (uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
			    if( !mIPC->send(msg) )
			    {
				    // if this IPC message failed, return and try again!
				    IGOLogWarn("IGOApplication focus message was lost!");
				    mHaveFocus = true;
				    return mHaveFocus;
			    }

			    if (!mWindowed && isActive())
			    {
				    // reset ClipCursor to game state
				    restoreMouseCursorClipping();
			    }
                mWasActive = isActive();
                mEmitStateChange = !mWasActive; //we don't want to emit a state change to the SDK, so that it in turn won't notify the game (e.g. DS3's pause state is based on whether IGO is visible or not)
		    }

		    return mHaveFocus;
	    }
	
	    if (gHwnd && GetForegroundWindow() != gHwnd)
	    {
		    if (mHaveFocus)
		    {
			    mHaveFocus = false;
			    IGOLogInfo("IGOApplication hasFocus = %d", mHaveFocus);
			    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOFocus(mHaveFocus, (uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
			    if( !mIPC->send(msg) )
			    {
				    // if this IPC message failed, return and try again!
				    IGOLogWarn("IGOApplication focus message was lost!");
				    mHaveFocus = true;
				    return mHaveFocus;
			    }
			
			    if (!mWindowed && isActive())
			    {
				    // reset ClipCursor to game state
				    restoreMouseCursorClipping();
			    }
                mWasActive = isActive();
                mEmitStateChange = !mWasActive;
		    }

		    return mHaveFocus;
	    }
	
	    if (!mHaveFocus)
	    {
		    mHaveFocus = true;
		    IGOLogInfo("IGOApplication hasFocus = %d", mHaveFocus);
		    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOFocus(mHaveFocus, (uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
		    if( !mIPC->send(msg) )
		    {
			    // if this IPC message failed, return and try again!
			    IGOLogWarn("IGOApplication focus message was lost!");
			    mHaveFocus = false;
			    return mHaveFocus;
		    }

            // update the current RendererType for Twitch
            TwitchManager::TTV_SetRendererType(mRendererType);

		    reset();	// retrieve all windows again, once we have the focus back, because a filter in IGOWindowManagerIPC::send(...)
					    // might have blocked large updates, while the IGO featuring game was in background!!!
            if (mWasActive)
            {
                toggleIGO();
                mWasActive = false;
            }
	    }

	    if (!mWindowed && mHaveFocus && isActive())
	    {
		    setMouseCursorClipping();
	    }

    #elif defined(ORIGIN_MAC) // MAC OSX 

        // Make sure the initial setup phase is complete (ie the size of the window is
        // appropriate and we're not inside a launcher)
        if (mSetupCompleted)
        {
            // When checking the focus, check that the main window/render window has focus, so that we automatically
            // de-activate OIG when we show an About window, or ... so that we don't steal their inputs if they overlap
            // the render window
            bool isFrontProcess = IsFrontProcess() && (MainWindowHasFocus() || !mWindowed);
        
            if (!mFocusDefined || mHaveFocus != isFrontProcess)
            {
                IGOLogInfo("IGOApplication hasFocus = %d", isFrontProcess);
                
                eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOFocus(isFrontProcess, (uint16_t)mScreenWidth, (uint16_t)mScreenHeight));
                if( !mIPC->send(msg) )
                {
                    // if this IPC message failed, return and try again!
                    IGOLogWarn("IGOApplication focus message was lost!");
                    return mHaveFocus;
                }

                mFocusDefined = true;
                mHaveFocus = isFrontProcess;
                
                if (isFrontProcess)
                {
                    forceRefresh(); // We want to get all the windows again, BUT we don't want to remove the current set, otherwise we're going to see all the windows "flash" quickly
                    
                    // Re-enable OIG if it was on before we lost focus
                    if (mWasActive)
                    {
                        IGOLogWarn("Re-enable OIG after lost focus");
                        
                        toggleIGO();
                        mWasActive = false;
                    }
                }
                else
                {
                    // Do like PC and close IGO automatically
                    mWasActive = isActive();
                    mEmitStateChange = !mWasActive;
                    
                    if (mWasActive)
                    {
                        IGOLogWarn("Disable OIG because lost focus");
                        
                        // We don't want to restore the previous location registered as the user may have clicked on the top bar of a window -> the window would snap
                        // to the previously registered cursor location
                        mSkipIGOLocationRestore = true;
                        
                        toggleIGO();
                    }
                }
            }
	    }
    
#endif // MAC OSX

	    return mHaveFocus;
    }

    bool IGOApplication::checkLanguageChangeRequest()
    {
	    static DWORD mLanguageChangeRequestTS = 0;	    
	    bool retval = false;

	    // toggle keyboard language on alt+shift
	    if ((GetTickCount() - mLanguageChangeRequestTS) > 250)	// add a small timeout, so that we do not toggle this function too often, because it can be very CPU intensive!!!
	    {
			static bool languageChangeRequestKeyRelease = false;
			static bool languageChangeRequestKeyPress = false;
			static bool languageChangeRequestInitiated = false;

    #if defined(ORIGIN_PC)

		    // capture alt+shift key press event - only if no other keys are pressed
		    languageChangeRequestKeyPress = ((gIGOKeyboardData[VK_SHIFT]&0x80) && (gIGOKeyboardData[VK_MENU]&0x80) && otherKeyPress(VK_MENU, VK_SHIFT));

		    if (!((gIGOKeyboardData[VK_SHIFT]&0x80) && (gIGOKeyboardData[VK_MENU]&0x80))) // capture the key release event, ignore other keys
		    {
			    languageChangeRequestKeyRelease = true;
		
			    // only activate this on the press->release signal edge
			    if (languageChangeRequestInitiated)
			    {
				    mLanguageChangeInProgress = true;
			
                    HKL igoKeyboardLayout = mIGOKeyboardLayout;
				    HKL inputLanguageList[256] ={0};
				    UINT num=GetKeyboardLayoutList(256, inputLanguageList);
				    for(UINT i=0; i<num; i++)
				    {
                        if (igoKeyboardLayout == 0 || inputLanguageList[i] == igoKeyboardLayout)
					    { 
                            igoKeyboardLayout = inputLanguageList[(i + 1) % num];
                            if (IsWindowUnicode(gHwnd))
                                PostMessageW(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)igoKeyboardLayout);
                            else
                                PostMessageA(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)igoKeyboardLayout);
                            
                            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgIGOKeyboardLanguage(igoKeyboardLayout));
						    mIPC->send(msg);
						
						    //memset(gIGOKeyboardData, 0, sizeof(gIGOKeyboardData));
						    break;
					    }
				    }

				    mLanguageChangeRequestTS = GetTickCount();
				    languageChangeRequestInitiated = false;
				    mLanguageChangeInProgress = false;
				    retval = true;
			    }
		    }
		
    #endif // !MAC OSX

		    // init the language change, so that we can process it once the keys are released
		    if ( languageChangeRequestKeyPress /*key was pressed*/ && languageChangeRequestKeyRelease /*key was released, before pressing*/)
		    {
			    languageChangeRequestInitiated = true;
			    languageChangeRequestKeyRelease = false;
		    }
	    }

	    return retval;
    }

    const uint8_t IMMEDIATE_HOT_KEY_DOWN = 2;

        
    void IGOApplication::dumpActiveState() const
	    {
        IGOLogWarn("mWasActive=%d, mPendingToggleState=%d, mIsActive=%d", mWasActive, mPendingToggleState, mIsActive);
	    }

    // call checkHotKey && toggleIGO only from then main win32 message thread!!!
    bool IGOApplication::doHotKeyCheck()
    {
	    bool retval = false;

        // First check whether we have focus, as this may change the IGO toggle state and we want to immediately
        // apply those changes
        bool focused = hasFocus();
        
	    // process pending IGO toggle command
	    processPendingIGOToggle();

#ifdef ORIGIN_MAC
        // process pending pinned widgets toggle command
        processPendingPinnedWidgetsToggle();
#endif
        
        // Those are used for debugging only
        mResetContext = false;
        mDumpWindowBitmaps = false;
        
        if (!focused)
            return retval;
	
	    // enable/disable info window
	    // Control + Alt + S
#ifdef ORIGIN_MAC
        static bool prevDisplayInfoState = false;
        static bool prevStatsState = false;
        static bool prevResetState = false;
        static bool prevDumpTexState = false;
        static bool prevResetContextState = false;
        static bool prevAnimState = false;
        
        bool currDisplayInfoState = ((gIGOKeyboardData[VK_CONTROL]&0x80) && (gIGOKeyboardData[VK_MENU]&0x80) && (gIGOKeyboardData[VK_S]&0x80));
        if (currDisplayInfoState && !prevDisplayInfoState)
        {
            if (mInfoDisplay)
                mInfoDisplay->toggle();

            if (!mInfoDisplay->isEnabled())
            {
                prevStatsState = false;
                prevResetState = false;
                prevDumpTexState = false;
                prevResetContextState = false;
                prevAnimState = false;
            }
        }
        
        prevDisplayInfoState = currDisplayInfoState;
        
        
        // While the info window is on, enable a few options
        if (mInfoDisplay->isEnabled())
        {
            // Press R = force a full reset of the set of windows
            bool currResetState = gIGOKeyboardData[VK_R] & 0x80;
            if (currResetState && !prevResetState)
                reset();
            
            prevResetState = currResetState;
            
            
            // Press C = force reset of context
            bool currResetContextState = gIGOKeyboardData[VK_C] & 0x80;
            if (currResetContextState && !prevResetContextState)
                mResetContext = true;
            
            prevResetContextState = currResetContextState;
            
            // Press D = dump the textures to disc as well as the framebuffer content before and after us
            bool currDumpTexState = gIGOKeyboardData[VK_D] & 0x80;
            if (currDumpTexState && !prevDumpTexState)
                mDumpWindowBitmaps = true;
                
            prevDumpTexState = currDumpTexState;
            
            // Press S = dump stats to system console
            bool currStatsState = gIGOKeyboardData[VK_S] & 0x80;
            if (currStatsState && !prevStatsState)
                IGOWindowGovernor::instance()->dumpStats();
            
            prevStatsState = currStatsState;

            // Press A = toggle speed of animations
            bool currAnimState = gIGOKeyboardData[VK_A] & 0x80;
            if (currAnimState && !prevAnimState)
                mAnimDurationScale = mAnimDurationScale == 1.0f ? 10.0f : 1.0f;

            prevAnimState = currAnimState;
        }
#else
		static bool prevDisplayInfoState = false;
		static bool prevTDRState = false;
		static bool prevScreenCaptureState = false;
        static bool prevResetState = false;
        static bool prevStatsState = false;
        static bool prevResetAppState = false;
		static bool prevAnimScaleState = false;

		bool currDisplayInfoState = ((gIGOKeyboardData[VK_CONTROL] & 0x80) && (gIGOKeyboardData[VK_MENU] & 0x80) && (gIGOKeyboardData['S'] & 0x80));
		if (currDisplayInfoState && !prevDisplayInfoState)
		{
        if (mInfoDisplay)
				mInfoDisplay->enable(currDisplayInfoState);

			if (!mInfoDisplay->isEnabled())
			{
				prevTDRState = false;
                prevResetState = false;
                prevStatsState = false;
                prevResetAppState = false;
				prevAnimScaleState = false;
			}
		}

		prevDisplayInfoState = currDisplayInfoState;

		// While the info window is on, eanble a few options
		if (mInfoDisplay->isEnabled())
		{
			// Press T = force a TDR
			bool currTDRState = (gIGOKeyboardData['T'] & 0x80) != 0;
			if (currTDRState && !prevTDRState)
			{
#ifdef _WIN64
				DX12Hook::ForceTDR();
#endif
			}
			prevTDRState = currTDRState;

			// Press C = save screen captured from Twitch
			bool currScreenCaptureState = (gIGOKeyboardData['C'] & 0x80) != 0;
			if (currScreenCaptureState && !prevScreenCaptureState)
			{
#ifdef _WIN64
				DX12Hook::ForceScreenCapture();
#endif
			}
			prevScreenCaptureState = currScreenCaptureState;
		}

		prevDisplayInfoState = currDisplayInfoState;

		// While the info window is on, eanble a few options
		if (mInfoDisplay->isEnabled())
		{
            // Press R = force a full reset of the set of windows
            bool currResetState = (gIGOKeyboardData['R'] & 0x80) != 0;
            if (currResetState && !prevResetState)
                reset();
            
            prevResetState = currResetState;

            // Press S = dump stats to system console
            bool currStatsState = (gIGOKeyboardData['S'] & 0x80) != 0;
            if (currStatsState && !prevStatsState)
                IGOWindowGovernor::instance()->dumpStats();
            
            prevStatsState = currStatsState;

            // Press C = force reset of IGOApp
            bool currAppState = (gIGOKeyboardData['C'] & 0x80) != 0;
            if (currAppState && !prevResetAppState)
                IGOApplication::deleteInstanceLater();
            
            prevResetAppState = currAppState;

			// Press A = toggle scale used to slow down animations
			bool currAnimScaleState = (gIGOKeyboardData['A'] & 0x80) != 0;
			if (currAnimScaleState && !prevAnimScaleState)
			    mAnimDurationScale = mAnimDurationScale == 1.0f ? 10.0f : 1.0f;

			prevAnimScaleState = currAnimScaleState;
        }

	    // check for language changing keys, in that case, return without checking for hotkeys
	    if (checkLanguageChangeRequest())
        {
            keyDebugAdd("HKLC", 0, 0, 0, 0);
		    return false;
        }

        // same logic to filter other keys pressed
	    // whenever open IGO or close IGO by using hotkeys
	    if (!otherKeyPress(mKey[0], mKey[1]) && !otherKeyPress(mKey[3], mKey[4]))
        {
            keyDebugAdd("HKOK", mKey[0], mKey[1], 0, 0);
		    return false;
        }

        // if hot key was just set, keep track of when we get the next key down
        if( mHotkeySet > 0 )
        {
            keyDebugAdd("HKKS", mHotkeySet, IMMEDIATE_HOT_KEY_DOWN, 0, 0);
            mHotkeySet++;
            if( mHotkeySet > IMMEDIATE_HOT_KEY_DOWN )
                mHotkeySet = 0;
        }

	    for (size_t i = 0; i < 3; i++)
	    {
		    mPreviousKeyState[mKey[i]] = mCurrentKeyState[mKey[i]];
		    mCurrentKeyState[mKey[i]] = gIGOKeyboardData[mKey[i]];

		    // these keys are equivalent in 102 keyboards.  The \| key
		    if (mKey[i] == VK_OEM_5)
            {
                keyDebugAdd("HKM5", i, mKey[i], mCurrentKeyState[mKey[i]],
                    gIGOKeyboardData[VK_OEM_102]);

			    mCurrentKeyState[mKey[i]] |= gIGOKeyboardData[VK_OEM_102];
            }
		    if (mPreviousKeyState[mKey[i]] != mCurrentKeyState[mKey[i]])
		    {
                keyDebugAdd("HKPK", i, mKey[i], mPreviousKeyState[mKey[i]],
                    mCurrentKeyState[mKey[i]]);
    //            keyDebugAdd("XTRA", gIGOKeyboardData[mKey[i]],
    //                checkHotKey(mKey[i], (gIGOKeyboardData[mKey[i]]&0x80) ? true : false), 0, 0);
			    retval |= checkHotKey(mKey[i], (gIGOKeyboardData[mKey[i]]&0x80) ? true : false, false);
		    }
	    }


        // pin key
        bool isPinKeyPressed = false;
	    for (size_t i = 3; i < 5; i++)
	    {
		    mPreviousKeyState[mKey[i]] = mCurrentKeyState[mKey[i]];
		    mCurrentKeyState[mKey[i]] = gIGOKeyboardData[mKey[i]];

		    // these keys are equivalent in 102 keyboards.  The \| key
		    if (mKey[i] == VK_OEM_5)
            {
                keyDebugAdd("HKM5", i, mKey[i], mCurrentKeyState[mKey[i]],
                    gIGOKeyboardData[VK_OEM_102]);

			    mCurrentKeyState[mKey[i]] |= gIGOKeyboardData[VK_OEM_102];
            }
		    if (mPreviousKeyState[mKey[i]] != mCurrentKeyState[mKey[i]])
		    {
                keyDebugAdd("HKPK", i, mKey[i], mPreviousKeyState[mKey[i]],
                    mCurrentKeyState[mKey[i]]);
    //            keyDebugAdd("XTRA", gIGOKeyboardData[mKey[i]],
    //                checkHotKey(mKey[i], (gIGOKeyboardData[mKey[i]]&0x80) ? true : false), 0, 0);
			    isPinKeyPressed |= checkHotKey(mKey[i], (gIGOKeyboardData[mKey[i]]&0x80) ? true : false, true);
		    }
	    }

#endif
	    return retval;
    }

    // this function is only for information, if our hotkeys are pressed
    bool IGOApplication::isHotKeyPressed()
    {
        // If another key is pressed other than the 2-key IGO Hotkey, then consider the IGO Hotkey not pressed.
        if(!otherKeyPress(mKey[0], mKey[1]) && !otherKeyPress(mKey[3], mKey[4]))
            return false;

	    int retval = 0;

	    int8_t tmpKeyState[256] = {0};
	    for (int i = 0; i < 2/*exclude the ESCAPE key*/; i++)
	    {
		    tmpKeyState[mKey[i]] = gIGOKeyboardData[mKey[i]];

		    // these keys are equivalent in 102 keyboards.  The \| key
		    if (mKey[i] == VK_OEM_5)
			    tmpKeyState[mKey[i]] |= gIGOKeyboardData[VK_OEM_102];

		    retval += (tmpKeyState[mKey[i]]&0x80) ? 1 : 0;
	    }

        keyDebugAdd("IHKP", retval, tmpKeyState[mKey[0]], tmpKeyState[mKey[1]], 0);

        if (retval == 2)
            return true;

        retval = 0;
	    for (int i = 3; i < 5; i++)
	    {
		    tmpKeyState[mKey[i]] = gIGOKeyboardData[mKey[i]];

		    // these keys are equivalent in 102 keyboards.  The \| key
		    if (mKey[i] == VK_OEM_5)
			    tmpKeyState[mKey[i]] |= gIGOKeyboardData[VK_OEM_102];

		    retval += (tmpKeyState[mKey[i]]&0x80) ? 1 : 0;
	    }
        
        return retval == 2;
    }

    bool IGOApplication::checkHotKey(uint8_t key, bool keyDown, bool pinKey)
    {
	    // check escape first
        if (!pinKey)
        {
	        if (mKey[0] != VK_ESCAPE && mKey[1] != VK_ESCAPE)
	        {
		        if (isActive() && key == VK_ESCAPE && !keyDown)
		        {
                    keyDebugAdd("CKES", key, keyDown, 0, 0);
			        toggleIGO();
			        processPendingIGOToggle();	// we are in the main windows message thread, so call this here
			        return true;
		        }
	        }
        }


        if (!pinKey)
        {
	        // This is complicated
	        // After hotkey is pressed, we don't want keyup to deactivate
	        // Condition to reset the hotkey is:
	        // 1. IGO is active (mIsActive)
	        // 2. We just activated IGO (mActivated)
	        // 3. Key event is one of the hotkeys
	        // 4. Key event is key up
	        if (isActive() && mActivated && (mKey[0] == key || mKey[1] == key))
	        {
                keyDebugAdd("CKKU", key, keyDown, mActivated, 0);
		        if (!keyDown)
			        mActivated = false;
		        return false;
	        }
        }
        else
        {
	        if (mIsPinKeyActive && mPinKeyActivated && (mKey[3] == key || mKey[4] == key))
	        {
                keyDebugAdd("CKKU", key, keyDown, mPinKeyActivated, 0);
		        if (!keyDown)
			        mPinKeyActivated = false;
		        return false;
	        }
        }
	
		if (!pinKey)
		{
			// EBIBUGS-15546
			// Prevent key down / key up that immediately follows a SetHotKey to close the IGO.

			// Sometimes, we do not immediately get a key down after the hot key is set,
			// so only ignore next key up if that is the case
			if (mHotkeySet == IMMEDIATE_HOT_KEY_DOWN && isActive() && keyDown)
			{
				keyDebugAdd("CKID", mHotkeySet, key, keyDown, mIgnoreNextKeyUp);
				mIgnoreNextKeyUp = true;
				mHotkeySet = 0;
			}

			// don't process key up when the hot key was just set
			if (mIgnoreNextKeyUp && !keyDown)
			{
				keyDebugAdd("CKIU", mIgnoreNextKeyUp, key, keyDown, 0);
				mIgnoreNextKeyUp = false;
				return false;
			}
		}
     

	    // reorder keys
	    uint8_t key1 = 0, key2 = 0;
	    bool matched = false;
	    if (key == mKey[pinKey ? 3 : 0])
	    {
            keyDebugAdd("CKM1", key, mKey[pinKey ? 3 : 0], mKey[pinKey ? 4 : 1], 0);
		    key1 = mKey[pinKey ? 3 : 0];
		    key2 = mKey[pinKey ? 4 : 1];
		    matched = true;
	    }
	    else if (key == mKey[pinKey ? 4 : 1])
	    {
            keyDebugAdd("CKM2", key, mKey[pinKey ? 3 : 0], mKey[pinKey ? 4 : 1], 0);
		    key1 = mKey[pinKey ? 4 : 1];
		    key2 = mKey[pinKey ? 3 : 0];
		    matched = true;
	    }

        if (!pinKey)
        {
	        // check which edge to check
	        if (matched)
	        {
		        if (!isActive())
			        // if activating IGO, we check for key down
			        matched = keyDown;
		        else
			        // if disabling IGO, we check for key up
			        //matched = !keyDown;
			
			        // if disabling IGO, we check for key down too, because:

			        // since we call "releaseInputs()" on activation of IGO, we would have to release all hotkeys physically
			        // and the repress them in order generate a keyup event -> this sucks
			        // 14728 	User must press IGO hotkey multiple times to get it to work, and multiple times to hide it.
			
			        matched = !keyDown;
                keyDebugAdd("CKWE", key, matched, isActive(), keyDown);
                keyDebugAdd("XTRA", key, mIsActive, mPendingToggleState, 0);
	        }

	        if (matched)
	        {
		        uint8_t state = gIGOKeyboardData[key2];
		        if (key2 == VK_OEM_5)
			        state |= gIGOKeyboardData[VK_OEM_102];

                keyDebugAdd("CKTC", key, keyDown, state, gIGOKeyboardData[key2]);
                keyDebugAdd("XTRA", key1, key2, mActivated, gIGOKeyboardData[VK_OEM_102]);

		        if ((state&0x80) || key1 == key2)
		        {
    #ifdef ORIGIN_PC
                    // if IGO was not rendered withing the last 500ms, our hooks could be broken, so toggle IGO off
                    if( (GetTickCount() - gPresentHookCalled)>500 )
                    {
                        //ignore the hotkey since we aren't going to display IGO
                        if (!isActive())
                            return false;
                        else
                        {
			                mActivated = true;
			                toggleIGO();
			                processPendingIGOToggle();	// we are in the main windows message thread, so call this here
			                return true;
                        }

                    }
    #endif
                    //check and see if the window size is larger than minimum supported size to display IGO
                    //but if this is to turn off IGO, then allow it to get processed
                    if (!isActive() && (mWindowScreenWidth < mMinIGOScreenWidth || mWindowScreenHeight < mMinIGOScreenHeight))
                    {
						// Signal Origin That the user tried to open the IGO.
						eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgUserIGOOpenAttemptIgnored());
						if( !mIPC->send(msg) )
						{
							IGOLogWarn("IGOApplication IGO Open Attempt Ignored Message was lost!");
						}
						
                        //ignore the hotkey since we aren't going to display IGO
                        return false;
                    }
                    else
                    {
			            mActivated = true;
			            toggleIGO();
			            processPendingIGOToggle();	// we are in the main windows message thread, so call this here
			            return true;
                    }
		        }
	        }
        }
        else
        {
	        // check which edge to check
	        if (matched)
	        {

				if (!mIsPinKeyActive)
			        // if activating IGO, we check for key down
			        matched = keyDown;
		        else
			        // if disabling IGO, we check for key up
			        //matched = !keyDown;
			
			        // if disabling IGO, we check for key down too, because:

			        // since we call "releaseInputs()" on activation of IGO, we would have to release all hotkeys physically
			        // and the repress them in order generate a keyup event -> this sucks
			        // 14728 	User must press IGO hotkey multiple times to get it to work, and multiple times to hide it.
			
			        matched = !keyDown;
                keyDebugAdd("CKWE", key, matched, mPinKeyActivated, keyDown);
                keyDebugAdd("XTRA", key, mIsPinKeyActive, mPendingToggleState, 0);
	        }

	        if (matched)
	        {
		        uint8_t state = gIGOKeyboardData[key2];
		        if (key2 == VK_OEM_5)
			        state |= gIGOKeyboardData[VK_OEM_102];

                keyDebugAdd("CKTC", key, keyDown, state, gIGOKeyboardData[key2]);
                keyDebugAdd("XTRA", key1, key2, mPinKeyActivated, gIGOKeyboardData[VK_OEM_102]);

				if ((state & 0x80) || key1 == key2)
		        {
					// send the pin key toggle
					if (mIPC)
					{
						eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgTogglePins(mIsPinKeyActive));
						mIPC->send(msg);
					}

					mPinKeyActivated = true;
					mIsPinKeyActive = !mIsPinKeyActive;
					
					return true;
		        }
	        }
        }

	    return false;
    }

    bool IGOApplication::isActive()
    {
	    if (mPendingToggleState==true && mIsActive==false)	// IGO will be active on the next cycle
		    return true;
	    else
	    if (mPendingToggleState==true && mIsActive==true)	// IGO is active
		    return true;
	    else
	    if (mPendingToggleState==false && mIsActive==true)	// IGO will be deactivated on the next cycle
		    return false;
	    else
	    //if (mPendingToggleState==false && mIsActive==false)	// IGO is deactivated
		    return false;
    }

    bool IGOApplication::recentlyHidden()
    {
	    // when we toggle off, make sure we throw away recently pressed keys
	    // from the game
	    return (GetTickCount() - mHideTS) < 100;
    }

#ifdef ORIGIN_MAC
    bool IGOApplication::checkPipe()
    {
        if( mIPC )
	    {
            // Are we trying to re-connect after Origin was closed/crashed?
#ifdef DISABLE_RECONNECT
            if (mSkipReconnection)
                return false;
#endif
            
            // Prepare to detect re-connection (we don't want to allow it right now since it's not fully supported from the
            // client side (doesn't show the game name in friend list, doesn't watch process for when closing, etc...))
#ifdef DISABLE_RECONNECT
            bool restart = !mClientReady;
#else
            bool restart = true;
#endif
		    if(!mIPC->checkPipe(restart))	// if our pipe was in a broken state, try to repair it
		    {
                // In the case we lose the connection = Origin is gone, we need to really remove all the IGOWindows, not
                // only their internal render states.
                IGOWindowGovernor::instance()->prepareForCompleteCleanup();
			    reset();	// retrieve all windows again, because we might be in a incomplete state!
                
                if (restart)
                {
                    // For Mac, a broken pipe = the client closed down (most likely crashed)
                    // -> we will need to restart the whole connection setup flow
                    mClientReady = false;
                    mFocusDefined = false;
                    mSetupCompleted = false;
                    mSkippingRenderUpdate = false;
                    
                    if(!mIPC->checkPipe(restart))	// see if the repair attempt worked
                        return false;
                }
                
                else
                {
                    mSkipReconnection = true;
                    return false;
                }
		    }
	    }
	    return true;
    }
#else // ORIGIN_PC
    bool IGOApplication::checkPipe()
    {
        if( mIPC )
	    {
		    if(!mIPC->checkPipe())	// if our pipe was in a broken state, try to repair it
		    {
                // In the case we lose the connection = Origin is gone, we need to really remove all the IGOWindows, not
                // only their internal render states.
                IGOWindowGovernor::instance()->prepareForCompleteCleanup();

			    reset();	// retrieve all windows again, because we might be in a incomplete state!

			    IGO_TRACE("IGO: broken pipe detected, attemping to repair it.");

			    if(!mIPC->checkPipe())	// see if the repair attempt worked
			    {
				    IGO_TRACE("IGO: broken pipe detected, repairing failed.");
				    return false;
			    }
		    }
	    }
	    return true;
    }
#endif // ORIGIN_PC
    
    bool IGOApplication::languageChangeInProgress()
    {
	    return mLanguageChangeInProgress;
    }

    void IGOApplication::sendBroadcastStatus(bool started, bool archivingEnabled, const eastl::string& fixArchivingURL, const eastl::string& channelURL)
    {
        GameLaunchInfo* info = gameInfo();
        if (info->broadcastStatsChanged())
        {
            bool isBroadcasting;
            uint64_t streamId;
            uint32_t minViewers;
            uint32_t maxViewers;
            const char16_t* channel;

            info->broadcastStats(isBroadcasting, streamId, &channel, minViewers, maxViewers);
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcastStats(isBroadcasting, streamId, minViewers, maxViewers, channel, info->gliStrlen(channel)));
            mIPC->send(msg);

            if (!mIPC->send(msg))
            {
                IGOLogWarn("IGOApplication broadcast message was lost!");
            }
        }
        
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcastStatus(started, archivingEnabled, fixArchivingURL, channelURL));
	    if( !mIPC->send(msg) )
	    {
		    IGOLogWarn("IGOApplication broadcast message was lost!");
	    }
    }
    void IGOApplication::sendBroadcastStreamInfo(int viewers)
    {                          
        eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgBroadcastStreamInfo(viewers));
	    if( !mIPC->send(msg) )
	    {
		    IGOLogWarn("IGOApplication broadcast message was lost!");
	    }
    }

    void IGOApplication::sendBroadcastError(int errorCategory, int errorCode)
    {
	    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgBroadcastError(errorCategory, errorCode));
	    if( !mIPC->send(msg) )
	    {
		    IGOLogWarn("IGOApplication error message was lost!");
	    }
    }

	void IGOApplication::sendBroadcastOptEncoderAvailable(bool available)
	{
		eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgBroadcastOptEncoderAvailable(available));

		if( !mIPC->send(msg) )
		{
			IGOLogWarn("IGOApplication error message was lost!");
		}
	}
    
    void IGOApplication::processPendingIGOToggle()
    {
	    if (mPendingToggleState != mIsActive)
	    {
		    bool newActiveState = !mIsActive;

		    IGOLogInfo("IGOApplication toggleIGO = %d -> %d", mIsActive, newActiveState);

		    mIsActive = newActiveState;

    #if defined(ORIGIN_MAC)
            SetCursorIGOState(mIsActive, false, mSkipIGOLocationRestore);
            SetInputIGOState(mIsActive);
            
            mSkipIGOLocationRestore = false; // reset flag
    #endif

		    // when hiding, we save the time so we can throw away hotkeys
		    if (!newActiveState)
		    {
			    mHideTS = GetTickCount();

    #if defined(ORIGIN_PC)
			    restoreRawInputDeviceFlags();
			
			    if (mGameKeyboardLayout!=0)
    #elif defined(ORIGIN_MAC)
    #ifdef USE_SEPARATE_KBD_LAYOUT_FOR_IGO
                mIGOKeyboardLayout = Origin::Mac::CurrentInputSource();
    #endif
    #endif
			    {
				    mLanguageChangeInProgress = true;

    #if defined(ORIGIN_PC)
                    if (IsWindowUnicode(gHwnd))
                        PostMessageW(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)mGameKeyboardLayout);
                    else
                        PostMessageA(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)mGameKeyboardLayout);

				    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOKeyboardLanguage(mGameKeyboardLayout));
				    mIPC->send(msg);
    #elif defined(ORIGIN_MAC)
    #ifdef USE_SEPARATE_KBD_LAYOUT_FOR_IGO
                    Origin::Mac::SetInputSource(mGameKeyboardLayout);
    #endif
    #endif
				    //memset(gIGOKeyboardData, 0, sizeof(gIGOKeyboardData));
				    mLanguageChangeInProgress = false;
			    }
		    }
		    else
		    {
    #if defined(ORIGIN_PC)
			    mGameKeyboardLayout = GetKeyboardLayout(0);
                HKL igoKeyboardLayout = mIGOKeyboardLayout;
                if (igoKeyboardLayout!=0)
    #elif defined(ORIGIN_MAC)
    #ifdef USE_SEPARATE_KBD_LAYOUT_FOR_IGO
                mGameKeyboardLayout = Origin::Mac::CurrentInputSource();
    #endif
    #endif
			    {
				    mLanguageChangeInProgress = true;

    #if defined(ORIGIN_PC)
                    if (IsWindowUnicode(gHwnd))
                        PostMessageW(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)igoKeyboardLayout);
                    else
                        PostMessageA(gHwnd, WM_INPUTLANGCHANGE, 0, (LPARAM)igoKeyboardLayout);

                    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOKeyboardLanguage(igoKeyboardLayout));
				    mIPC->send(msg);
    #elif defined(ORIGIN_MAC)
    #ifdef USE_SEPARATE_KBD_LAYOUT_FOR_IGO
                    Origin::Mac::SetInputSource(mIGOKeyboardLayout);
    #endif
    #endif

				    //memset(gIGOKeyboardData, 0, sizeof(gIGOKeyboardData));
				    mLanguageChangeInProgress = false;
			    }

    #if defined(ORIGIN_PC)
			    resetRawInputDeviceFlags();
			    releaseInputs();	// if we show IGO, we release all pressed keys + mouse buttons to prevent "endless running or fireing dudes in game"
    #endif
		    }

		    // send the igo state
		    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgIGOMode(mIsActive, mEmitStateChange));
		    mIPC->send(msg);

            if (newActiveState && !mEmitStateChange)    //restoring IGO after focus restored
                mEmitStateChange = true;    //reset

    #if defined(ORIGIN_PC)

		    // show or hide the IGO cursor
		    if (newActiveState)
		    {		
			    // save old cursor
			    gPrevCursor = GetCursor();

			    // update to IGO cursor
			    updateCursor();

			    // show the IGO cursor
			    ShowCursorFunc(FALSE);
			    gShowCursorCount = ShowCursorFunc(TRUE);
			    if (gShowCursorCount < 0)
				    while (ShowCursorFunc(TRUE) < 0);

			    GetCursorPosFunc(&gSavedMousePos);

			    // clamp cursor to game window
			    // set new state
			    if (!mWindowed)
				    setMouseCursorClipping();
		    }
		    else
		    {
			    // restore the previous cursor
			    SetCursorFunc(gPrevCursor);

			    // restore the previous cursor show state
			    ShowCursorFunc(FALSE);
			    int cursorCount = ShowCursorFunc(TRUE);
				
			    if (gShowCursorCount < cursorCount)
				    while (gShowCursorCount < ShowCursorFunc(FALSE));

                else
                if (gShowCursorCount > cursorCount)
                    while (gShowCursorCount > ShowCursorFunc(TRUE));
			
			    // reset ClipCursor to game state
			    if (!mWindowed)
				    restoreMouseCursorClipping();
		    }

    #endif // !MAC OSX

    #if defined(ORIGIN_PC)

		    // send a focus reset to the game & clear keyboard buffers - this should cause a game to reset it's keyboard buffer
		    // not having this caused Syndicate to not recognize certain keys after we toggled IGO
		    {
                if (IsWindowUnicode(gHwnd))
                    PostMessageW(gHwnd, WM_SETFOCUS, (WPARAM)gHwnd, NULL);
                else
                    PostMessageA(gHwnd, WM_SETFOCUS, (WPARAM)gHwnd, NULL);

			    // do not send a WM_KILLFOCUS, this breaks some games like Crysis 2!!!
	
			    BYTE dummy[256];
			    int result=0;	// result is a dummy variable, so that the compiler does not optimize those calls away...
			    GetKeyboardStateFunc(dummy);
			    for (int i=0; i<256; i++)
				    result+=dummy[i];
			    for (int i=0; i<256; i++)
				    result+=GetAsyncKeyStateFunc(i);
			    for (int i=0; i<256; i++)
				    result+=GetKeyStateFunc(i);

			    IGOLogInfo("IGOApplication keyboard buffer reseted: %i", result);

                if (!newActiveState)    // send "hard" reset for our hotkeys only!!! (http://online.ea.com/support/browse/EBIBUGS-21530)
                {
                    // no need to allocate them for every call
                    INPUT keyResetSnapshot[4] = {0};
	                // reset counters
	                int keyResetSnapshotCounter = 0;

                    //we only need to clear the hot keys for the first key in our key combos, since we do not send the 2nd hotkey along to the game
                    if (!(gIGOKeyboardData[mKey[0]]&0x80)) // fixing https://developer.origin.com/support/browse/EBIBUGS-22965
                    {
			            keyResetSnapshot[keyResetSnapshotCounter].type = INPUT_KEYBOARD;
                        keyResetSnapshot[keyResetSnapshotCounter].ki.wScan = (WORD)MapVirtualKey((UINT)mKey[0], MAPVK_VK_TO_VSC);
			            keyResetSnapshot[keyResetSnapshotCounter].ki.wVk = (WORD)mKey[0];
			            keyResetSnapshot[keyResetSnapshotCounter].ki.dwExtraInfo =  GetMessageExtraInfo();
			            keyResetSnapshot[keyResetSnapshotCounter].ki.dwFlags = KEYEVENTF_KEYUP;
			            keyResetSnapshotCounter++;
                    }

					if (!(gIGOKeyboardData[mKey[3]] & 0x80)) // fixing https://developer.origin.com/support/browse/EBIBUGS-22965
					{
						keyResetSnapshot[keyResetSnapshotCounter].type = INPUT_KEYBOARD;
						keyResetSnapshot[keyResetSnapshotCounter].ki.wScan = (WORD)MapVirtualKey((UINT)mKey[3], MAPVK_VK_TO_VSC);
						keyResetSnapshot[keyResetSnapshotCounter].ki.wVk = (WORD)mKey[3];
						keyResetSnapshot[keyResetSnapshotCounter].ki.dwExtraInfo = GetMessageExtraInfo();
						keyResetSnapshot[keyResetSnapshotCounter].ki.dwFlags = KEYEVENTF_KEYUP;
						keyResetSnapshotCounter++;
					}

					if (keyResetSnapshotCounter>0)
                        SendInputFunc(keyResetSnapshotCounter, keyResetSnapshot, sizeof(INPUT));

                }
		    }
		    // this code is currently not needed, because we make sure to call IGOApplication::toggleIGO() only from the main win32 message thread!!!
			    // post a message to display window to show the cursor
            //PostMessage(gHwnd, gIGOShowCursor, newActiveState ? TRUE : FALSE, 0);
    #endif

	    }
    }

    // process pending pinned widgets toggle command
#ifdef ORIGIN_MAC
    void IGOApplication::processPendingPinnedWidgetsToggle()
    {
        if (mPinKeyActivated)
        {
            // send the pin key toggle
            if (mIPC)
            {
                eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgTogglePins(mIsPinKeyActive));
                mIPC->send(msg);
            }
            
            mPinKeyActivated = false;
        }
    }
#endif

    #ifdef ORIGIN_PC
    void IGOApplication::enforceIGOMessagePump()
    {
	    gIGOUpdateMessageTime = GetTickCount();
        if (IsWindowUnicode(gHwnd))
            PostMessageW(gHwnd, gIGOUpdateMessage, 0, 0);	// send a dummy message to invoke the games main message loop and update IGO 
        else
            PostMessageA(gHwnd, gIGOUpdateMessage, 0, 0);	// send a dummy message to invoke the games main message loop and update IGO 
    }
    
	bool IGOApplication::isOriginAlive()
	{
		HWND hwnd = FindWindowW(L"QWidget", L"Origin");
		return hwnd != NULL && IsWindow(hwnd);
	}
	#endif

    // toggleIGO can be called from any thread, so we store the pending command and update IGO once we enter the windows message thread,
    // otherwise our cursor code will not work!!!
    void IGOApplication::toggleIGO()
    {
        keyDebugAdd("TOGL", mPendingToggleState, true, mActivated, 0);
	
	#ifdef ORIGIN_PC
		if (mPendingToggleState == false && !isOriginAlive())
		{
			// we do not allow to activate OIG if Origin is gone!
		}
		else
	#endif
		{
			mPendingToggleState = !mPendingToggleState;
		}

	#ifdef ORIGIN_PC
	    enforceIGOMessagePump();
    #endif
    }

#ifdef ORIGIN_MAC
    void IGOApplication::togglePinnedWidgets()
    {
        mPinKeyActivated = true;
        mIsPinKeyActive = !mIsPinKeyActive;
    }
#endif
    
    bool IGOApplication::isThreadSafe()
    {
	    if ( (mGFXThread == 0) || (mGFXThread != 0 && GetCurrentThreadId() == mGFXThread) )
		    return true;

	    return false;
    }

    void IGOApplication::createBackgroundDisplay(void *context)
    {	
    #if !defined(ORIGIN_MAC)

	    // create the background display
	    uint32_t data = 0xAD000000;
        bool created = false;
	    IGOWindow *win = getWindow(BACKGROUND_WINDOW_ID, &created);
	
	    moveWindow(BACKGROUND_WINDOW_ID, 0); // make sure we are the first to draw

        if (created)
        {
            AnimPropertySet anim;
            anim.props.alpha = 0;
            anim.propCurves.alpha.type = WindowAnimBlendFcn_EASE_IN_OUT;
            anim.propCurves.alpha.durationInMS = 300;

            anim.props.customValue = 0;
            anim.propCurves.customValue.type = WindowAnimBlendFcn_LINEAR;
            anim.propCurves.customValue.durationInMS = 300;

            // Let's add a base fade in/out animation
            win->setAnimProps(WindowAnimContext_OPEN, &anim);
            win->setAnimProps(WindowAnimContext_CLOSE, &anim);
        }
	    win->update(context, false, 255, 0, 0, 1, 1, (const char *)&data, sizeof(data), IGOIPC::WINDOW_UPDATE_ALL);
        static const int32_t BACKGROUND_DIMMING_FRAME_STRETCH = 1000;
        win->updateAnimValue(WindowAnimPropIndex_CUSTOM_VALUE, BACKGROUND_DIMMING_FRAME_STRETCH);

	    win->resize(mScreenWidth, mScreenHeight);

    #else // MAC OSX

	    // create the background display
        // FIXME: THERE IS SOMETHING WRONG WITH FIFA12/LOOKS LIKE THE TEXTURE DATA GETS OVERWRITTEN WHEN THE TEXTURE SIZE IS TOO SMALL!!!
        const uint32_t BackgroundDataSize = 32;

        static bool backgroundDataInitialized = false;
        static uint32_t backgroundData[BackgroundDataSize * BackgroundDataSize];
        if (!backgroundDataInitialized)
        {
            backgroundDataInitialized = true;
            for (size_t idx = 0; idx < BackgroundDataSize * BackgroundDataSize; ++idx)
                backgroundData[idx] = 0xAD000000;
        }    

        bool created = false;
	    IGOWindow *win = getWindow(BACKGROUND_WINDOW_ID, &created);
	
	    moveWindow(BACKGROUND_WINDOW_ID, 0); // make sure we are the first to draw
        
        if (created)
        {
            AnimPropertySet anim;
            anim.props.alpha = 0;
            anim.propCurves.alpha.type = WindowAnimBlendFcn_EASE_IN_OUT;
            anim.propCurves.alpha.durationInMS = 300;
            
            anim.props.customValue = 0;
            anim.propCurves.customValue.type = WindowAnimBlendFcn_LINEAR;
            anim.propCurves.customValue.durationInMS = 300;
            
            // Let's add a base fade in/out animation
            win->setAnimProps(WindowAnimContext_OPEN, &anim);
            win->setAnimProps(WindowAnimContext_CLOSE, &anim);
        }
    
        win->update(context, false, 255, 0, 0, BackgroundDataSize, BackgroundDataSize, (const char *)backgroundData, sizeof(backgroundData), IGOIPC::WINDOW_UPDATE_ALL);
        
        static const int32_t BACKGROUND_DIMMING_FRAME_STRETCH = 1000;
        win->updateAnimValue(WindowAnimPropIndex_CUSTOM_VALUE, BACKGROUND_DIMMING_FRAME_STRETCH);
        
	    win->resize(mScreenWidth, mScreenHeight);

    #endif // MAC OSX
    }

    void IGOApplication::updateCursor(IGOIPC::CursorType type)
    {
    #if defined(ORIGIN_PC)

    #define SET_CURSOR(x, y) case IGOIPC::x: cursorId = y; break

	    if (type != IGOIPC::CURSOR_UNKNOWN)
		    mCursorType = type;
	    else
		    type = mCursorType;

	    LPWSTR cursorId = 0;
	    switch (type)
	    {
		    //SET_CURSOR(CURSOR_UNKNOWN, IDC_ARROW);
		    SET_CURSOR(CURSOR_ARROW, IDC_ARROW);
		    SET_CURSOR(CURSOR_CROSS, IDC_CROSS);
		    SET_CURSOR(CURSOR_HAND, IDC_HAND);
		    SET_CURSOR(CURSOR_IBEAM, IDC_IBEAM);
		    SET_CURSOR(CURSOR_SIZEALL, IDC_SIZEALL);
		    SET_CURSOR(CURSOR_SIZENESW, IDC_SIZENESW);
		    SET_CURSOR(CURSOR_SIZENS, IDC_SIZENS);
		    SET_CURSOR(CURSOR_SIZENWSE, IDC_SIZENWSE);
		    SET_CURSOR(CURSOR_SIZEWE, IDC_SIZEWE);
	    }

	    if (cursorId)
	    {
		    SetCursorFunc(LoadCursor(NULL, cursorId));
	    }

    #elif defined(ORIGIN_MAC) // MAC OSX

	    SetIGOCursor(type);

    #endif
    }

    void IGOApplication::startTime()
    {
	    if (mInfoDisplay)
		    mInfoDisplay->startTime();
    }

    void IGOApplication::stopTime()
    {
	    if (mInfoDisplay)
		    mInfoDisplay->stopTime();
    }

    uint32_t IGOApplication::textureSize()
    {
	    if ( mGFXThread!=0 && GetCurrentThreadId() != mGFXThread )
	    {
		    IGOLogWarn("IGO is called from the wrong thread (calling thread %i - GFX thread %i)", GetCurrentThreadId(), mGFXThread);
	    }

        return IGOWindowGovernor::instance()->textureSize();
    }

    bool IGOApplication::otherKeyPress(uint8_t key1, uint8_t key2)
    {
    #if defined(ORIGIN_PC)

	    //F1~F12
	    for (uint8_t i = VK_F1; i <= VK_F12; i ++)
	    {
		    if (i != key1 && i != key2)
		    {
			    if (gIGOKeyboardData[i] & 0x80)
			    {
				    return false;
			    }
		    }
	    }

	    //0~9
	    for (uint8_t j = 0x30; j <= 0x39; j ++)
	    {
		    if (j != key1 && j != key2)
		    {
			    if (gIGOKeyboardData[j] & 0x80)
			    {
				    return false;
			    }
		    }
	    }

	    //A~Z
	    for (uint8_t k = 0x41; k <= 0x5A; k ++)
	    {
		    if (k != key1 && k != key2)
		    {
			    if (gIGOKeyboardData[k] & 0x80)
			    {
				    return false;
			    }
		    }
	    }

	    //Special key
	    // ;: + , - /? `~ [{ \| ]} "'
	    for ( uint8_t l = 0xBA; l <= 0xDE; l ++)
	    {
		    if (l != key1 && l != key2)
		    {
			    if (gIGOKeyboardData[l] & 0x80)
			    {
				    return false;
			    }
		    }
	    }

	    //Other key

	    //Caps lock
	    if (VK_CAPITAL != key1 && VK_CAPITAL != key2)
	    {
		    if (gIGOKeyboardData[VK_CAPITAL] & 0x80) return false;
	    }

	    //Scroll
	    if (VK_SCROLL != key1 && VK_SCROLL != key2)
	    {
		    if (gIGOKeyboardData[VK_SCROLL] & 0x80) return false;
	    }

	    //Space
	    if (VK_SPACE != key1 && VK_SPACE != key2)
	    {
		    if (gIGOKeyboardData[VK_SPACE] & 0x80) return false;
	    }

	    //Print
	    if (VK_PRINT != key1 && VK_PRINT != key2)
	    {
		    if (gIGOKeyboardData[VK_PRINT] & 0x80) return false;
	    }

	    //Pause
	    if (VK_PAUSE != key1 && VK_PAUSE != key2)
	    {
		    if (gIGOKeyboardData[VK_PAUSE] & 0x80) return false;
	    }

	    //Print Screen
	    if (VK_SNAPSHOT != key1 && VK_SNAPSHOT != key2)
	    {
		    if (gIGOKeyboardData[VK_SNAPSHOT] & 0x80) return false;
	    }

	    //Control-break processing
	    if (VK_CANCEL != key1 && VK_CANCEL != key2)
	    {
		    if (gIGOKeyboardData[VK_CANCEL] & 0x80) return false;
	    }

	    if (VK_UP != key1 && VK_UP != key2)
	    {
		    if (gIGOKeyboardData[VK_UP] & 0x80) return false;
	    }

	    if (VK_DOWN != key1 && VK_DOWN != key2)
	    {
		    if (gIGOKeyboardData[VK_DOWN] & 0x80) return false;
	    }

	    if (VK_LEFT != key1 && VK_LEFT != key2)
	    {
		    if (gIGOKeyboardData[VK_LEFT] & 0x80) return false;
	    }

	    if (VK_RIGHT != key1 && VK_RIGHT != key2)
	    {
		    if (gIGOKeyboardData[VK_RIGHT] & 0x80) return false;
	    }

	    if (VK_SHIFT != key1 && VK_SHIFT != key2)
	    {
		    if (gIGOKeyboardData[VK_SHIFT] & 0x80) return false;
	    }

	    if (VK_CONTROL != key1 && VK_CONTROL != key2)
	    {
		    if (gIGOKeyboardData[VK_CONTROL] & 0x80) return false;
	    }

	    //Alt
	    if (VK_MENU != key1 && VK_MENU != key2)
	    {
		    if (gIGOKeyboardData[VK_MENU] & 0x80) return false;
	    }

	    if (VK_INSERT != key1 && VK_INSERT != key2)
	    {
		    if (gIGOKeyboardData[VK_INSERT] & 0x80) return false;
	    }

	    if (VK_HOME != key1 && VK_HOME != key2)
	    {
		    if (gIGOKeyboardData[VK_HOME] & 0x80) return false;
	    }

	    //PgUp
	    if (VK_PRIOR != key1 && VK_PRIOR != key2)
	    {
		    if (gIGOKeyboardData[VK_PRIOR] & 0x80) return false;
	    }

	    if (VK_END != key1 && VK_END != key2)
	    {
		    if (gIGOKeyboardData[VK_END] & 0x80) return false;
	    }

	    //PgDn
	    if (VK_NEXT != key1 && VK_NEXT != key2)
	    {
		    if (gIGOKeyboardData[VK_NEXT] & 0x80) return false;
	    }

    #elif defined(ORIGIN_MAC) // MAC OSX
    
        for (int idx = 0; idx <= MAX_VKEY_ENTRY; ++idx)
	    {
            if (idx == VK_ESCAPE)
                continue;
        
            if (idx == VK_CAPITAL)
                continue;
            
		    if (idx != key1 && idx != key2)
		    {
                if (gIGOKeyboardData[idx] & 0x80)
                    return false;
            }
        }
    
    #endif

	    return true;
    }

    void IGOApplication::setHotKeyMessageHandler(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2)
    {
#if defined(ORIGIN_PC)
        if (key1>=VK_BACK && key1<=VK_OEM_CLEAR &&
            key2>=VK_BACK && key2<=VK_OEM_CLEAR)	// just to be sure, no crap came over the pipe
#elif defined(ORIGIN_MAC)
        if (key1< MAX_VKEY_ENTRY && key2< MAX_VKEY_ENTRY)	// just to be sure, no crap came over the pipe
#endif
        {
            mKey[0] = (uint8_t)key1;
            mKey[1] = (uint8_t)key2;

            keyDebugAdd("MSHV", mKey[0], mKey[1], 0, 0);
        }
        else
        {
            // back to default
            mKey[0] = VK_SHIFT;
            mKey[1] = VK_F1;

            keyDebugAdd("MSHI", 0, 0, 0, 0);
        }

#ifdef SHOW_IPC_MESSAGES
        OriginIGO::IGOLogInfo("IGOIPC::MSG_SET_HOTKEY %d, %d", key1, key2);
#endif

        // duplicate logic for the pin Hotkey (FYI: mkey[2] is reserved!!!)

#if defined(ORIGIN_PC)
        if (pinKey1>=VK_BACK && pinKey1<=VK_OEM_CLEAR &&
            pinKey2>=VK_BACK && pinKey2<=VK_OEM_CLEAR)	// just to be sure, no crap came over the pipe
#elif defined(ORIGIN_MAC)
        if (pinKey1< MAX_VKEY_ENTRY && pinKey2< MAX_VKEY_ENTRY)	// just to be sure, no crap came over the pipe
#endif
        {
            mKey[3] = (uint8_t)pinKey1;
            mKey[4] = (uint8_t)pinKey2;

            keyDebugAdd("MSHV", mKey[3], mKey[4], 0, 0);
        }
        else
        {
            // back to default
            mKey[3] = VK_SHIFT;
            mKey[4] = VK_F2;

            keyDebugAdd("MSHI", 0, 0, 0, 0);
        }

#ifdef SHOW_IPC_MESSAGES
        OriginIGO::IGOLogInfo("IGOIPC::MSG_SET_HOTKEY(pin) %d, %d", pinKey1, pinKey2);
#endif
        mHotkeySet = 1;
        mIgnoreNextKeyUp = false;

#ifdef ORIGIN_MAC
        SetInputHotKeys(mKey[0], mKey[1], mKey[3], mKey[4]);
#endif
    }
}
