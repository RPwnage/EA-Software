///////////////////////////////////////////////////////////////////////////////
// IGOApplication.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOAPPLICATION_H

#define IGOAPPLICATION_H

#ifdef ORIGIN_PC
#pragma warning(disable: 4996)    // 'sprintf': name was marked as #pragma deprecated
#endif

#include "eathread/eathread_rwspinlock.h"

#include "IGO.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCMessage.h"
#include "IGOTrace.h"
#include "Helpers.h"
#include "IGOWindowGovernor.h"

class IGOIPC;
class IGOIPCMessage;
class IGOIPCConnection;
class IGOIPCClient;

namespace OriginIGO {

    class IGOWindow;
    class IGOInfoDisplay;

    // reserved window ID's for our dark background and the performance overlay window(ctrl+alt+s)
    const int BACKGROUND_WINDOW_ID = 0;
    const int DISPLAY_WINDOW_ID = 1;
    

    class IGOApplication : public IGOIPCHandler
    {
    public:
        typedef void (*cleanupCallback)(void *userData); 

    public:
        static bool isPendingDeletion();
	    static IGOApplication* instance();
		static void deleteInstanceLater() { if (mInstance) mPendingInstanceDelete = true; }

        static void createInstance(RendererType rendererType, void *renderer, cleanupCallback cleanupFunc = NULL);
	    static void deleteInstance(void *userData = NULL);

		void setRenderContext(void *userData);

    private:
        IGOApplication(RendererType rendererType, void *renderer, cleanupCallback cleanupFunc = NULL);
        virtual ~IGOApplication();
        void setHotKeyMessageHandler(uint32_t key1, uint32_t key2, uint32_t pinKey1, uint32_t pinKey2);
    public:
	    IGOWindow *getWindow(uint32_t id, bool* created = NULL);
        bool getWindowIDs(char* buffer, size_t bufferSize);
		void closeWindow(uint32_t id);
	    void clearWindows(void *context);
        void refreshWindows(eastl::vector<uint32_t>& windowIDs);
		void moveWindow(uint32_t id, uint32_t index);
        void reset();
        void forceRefresh();
        bool isThreadSafe();

        // twitch messages to Origin
        void sendBroadcastStatus(bool started, bool archivingEnabled, const eastl::string& fixArchivingURL, const eastl::string& channelURL);
        void sendBroadcastError(int errorCategory, int errorCode);
        void sendBroadcastStreamInfo(int viewers);
		void sendBroadcastOptEncoderAvailable(bool available);

		void render(void *pDev);
		void unrender(void *pDev);
	    void setScreenSize(uint32_t width, uint32_t height);
	    void setScreenSizeAndRenderState(uint32_t width, uint32_t height, float pixelToPointScaleFactor, bool skipRenderUpdate);
        
#if defined(ORIGIN_MAC)
        const char* rendererString();
#else
        const WCHAR *rendererString();
#endif
	    RendererType rendererType() { return mRendererType; }
	
	    void send(eastl::shared_ptr<IGOIPCMessage> msg);
		virtual void handleDisconnect(IGOIPCConnection *conn, void *userData = NULL);
		virtual bool handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL);

        bool isHotKeyPressed();
        bool doHotKeyCheck();
        void updateCursor(IGOIPC::CursorType type = IGOIPC::CURSOR_UNKNOWN);
        bool recentlyHidden();
        bool checkPipe();


        // state information
        void toggleIGO();   // thread safe
        void processPendingIGOToggle(); // not thread safe, only call from main meesage thread!!!!
#ifdef ORIGIN_MAC
        void togglePinnedWidgets();
        void processPendingPinnedWidgetsToggle();
#endif
        bool hasFocus();
        bool isActive();
        bool wasActive() const { return mWasActive; }
        bool isPinningInProgress() { return mIsPinningInProgress; }
        void dumpActiveState() const;
#ifdef ORIGIN_PC
        bool isOriginAlive();
#endif
        uint32_t getScreenWidth() { return mScreenWidth; }
        uint32_t getScreenHeight() { return mScreenHeight; }
        bool isSRGB() { return mSRGB; }
        void setSRGB(bool srgb) { mSRGB = srgb; }
        float    getPixelToPointScaleFactor() { return mPixelToPointScaleFactor; }
        uint32_t getWindowCount() { return IGOWindowGovernor::instance()->count(); }
    
        // window screen size is used internally to determinate the mouse cursor scaling
        void setWindowScreenSize(uint32_t width, uint32_t height);
        void setWindowedMode(bool windowed);
        int getOffsetX() { return mWindowScreenXOffset; }
        int getOffsetY() { return mWindowScreenYOffset; }
        float getScalingX() { if (mScreenWidth==0 || mWindowScreenWidth == 0) return 1; else return (float)mScreenWidth/(float)mWindowScreenWidth; }
        float getScalingY() { if (mScreenHeight==0 || mWindowScreenHeight == 0) return 1; else return (float)mScreenHeight/(float)mWindowScreenHeight; }
        uint32_t getWindowScreenWidth() { return mWindowScreenWidth; }
        uint32_t getWindowScreenHeight() { return mWindowScreenHeight; }

        // timing information
        void startTime();
        void stopTime();
        float animDurationScale() const { return mAnimDurationScale; }
        uint32_t textureSize();
    
        bool otherKeyPress(uint8_t key1, uint8_t key2);
        bool languageChangeInProgress();
        HKL getIGOKeyboardLayout() { return mIGOKeyboardLayout; }
        void setIGOKeyboardLayout(HKL layout) { mIGOKeyboardLayout = layout; }

#if defined(ORIGIN_PC)
        static EA::Thread::RWSpinLock mInstanceMutex;
        static EA::Thread::RWSpinLock mInstanceOwnerMutex;
        static EA::Thread::Futex mHelperMutex;
#else
        static EA::Thread::Futex mInstanceMutex;

        // For debugging purposes
        void dumpStats();
        bool resetContext() const { return mResetContext; }
        bool dumpWindowBitmaps() const { return mDumpWindowBitmaps; }
#endif

    private:
        void sendGameInfo();
        void restoreMouseCursorClipping();
        void setMouseCursorClipping();
        bool checkLanguageChangeRequest();
        void enforceIGOMessagePump();
    
	    void processPendingIGOreset(void *context);

        bool checkHotKey(uint8_t key, bool keyDown, bool pinKey);

        static IGOApplication *volatile mInstance;
        static bool mPendingInstanceDelete;

        cleanupCallback mCleanupCall;
	    RendererType mRendererType;
	    static uint32_t mGFXThread;

        IGOIPCClient *mIPC;
        uint8_t mCurrentKeyState[256];
        uint8_t mPreviousKeyState[256];
    
        friend class IGOInfoDisplay;

        // state
        bool mHaveFocus;
        bool mFocusDefined;
        bool mPendingToggleState;
        bool mPendingIGOReset;
        bool mPendingIGORefresh;
        bool mSkipIGOLocationRestore;
        bool mIsActive;
        bool mActivated;
        bool mIsPinKeyActive;
        bool mPinKeyActivated;
        bool mIsPinningInProgress;
        bool mWindowed;
        bool mResetContext;
        bool mDumpWindowBitmaps;
        bool mLanguageChangeInProgress;
        bool mClientReady;
        bool mSetupCompleted;
        bool mSkippingRenderUpdate;
        bool mSkipReconnection;
        uint32_t mWindowScreenWidth;
        uint32_t mWindowScreenHeight;
        int mWindowScreenXOffset;
        int mWindowScreenYOffset;
        uint32_t mScreenWidth;
        uint32_t mScreenHeight;
        uint32_t mMinIGOScreenWidth;
        uint32_t mMinIGOScreenHeight;
        bool mSRGB;
        float mPixelToPointScaleFactor;
        uint8_t mKey[5];
        uint8_t mHotkeySet;
        bool mIgnoreNextKeyUp;
        IGOIPC::CursorType mCursorType;

    #if defined(ORIGIN_PC)
        HCURSOR mPrevCursor;
    #endif

        int mShowCursorCount;
        IGOInfoDisplay *mInfoDisplay;

        DWORD mHideTS;
        HKL mGameKeyboardLayout;
        volatile HKL mIGOKeyboardLayout;

        bool mWasActive;        //to remember whether IGO was up or not before we lost focus
        bool mEmitStateChange;  //to indicate whether igoStateChange signal should be emitted (passes as param of MSG_IGO_MODE)

		void *mRenderContext;	// user data payload for our internal calls
	    void createBackgroundDisplay(void *context);

        // Support for animations
        IntervalKeeper mAnimDelta;  // keeps track of the deltaT from one frame to the next
        float mAnimDurationScale;   // used to slow down animations (from the debug menu)
    };
}
#endif
