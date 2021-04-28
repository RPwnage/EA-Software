///////////////////////////////////////////////////////////////////////////////
// IGOInfoDisplay.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "IGOInfoDisplay.h"
#include "IGOApplication.h"
#include "IGOWindow.h"
#include "IGOIPC/IGOIPC.h"

#if defined(ORIGIN_PC)

#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"

namespace OriginIGO {

    #ifdef _DEBUG
    #include "HookAPI.h"
    EXTERN_NEXT_FUNC(SHORT, GetAsyncKeyStateHook, (int vKey));
    EXTERN_NEXT_FUNC(SHORT, GetKeyStateHook, (int vKey));
    EXTERN_NEXT_FUNC(BOOL, GetKeyboardStateHook, (PBYTE lpKeyState));

	void getPurgeIGOMemoryInformation(int &mem, int &obj);

    #endif
    extern BYTE gIGOKeyboardData[256];

    #define UPDATE_TIME 1000

    bool IGOInfoDisplay::mAllowed = false;
    
    IGOInfoDisplay::IGOInfoDisplay() :
        mWidth(300),
    #ifdef _DEBUG
	    mHeight(240+50),
    #else
        mHeight(170+40),
    #endif
        mBitmap(NULL),
        mFont(NULL),
        mBrush(NULL),
        mBytes(NULL),
        mDC(NULL),
        mTick(0),
        mStart(0),
        mStop(0),
        mFrequency(1),
        mDT(0),
        mCount(0),
        mEnabled(false),
        mPosX(0),
        mPosY(0)
    {
        BITMAPINFO bmi;

        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = mWidth;
        bmi.bmiHeader.biHeight = -mHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;
        mBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (VOID**)&mBytes, NULL, 0); 

        mDC = CreateCompatibleDC(NULL);
        SelectObject(mDC, mBitmap);

        mFont = CreateFont( 16, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 0, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial" );
        SelectObject(mDC, mFont);

        mBrush = CreateSolidBrush(0x80000000);
        SelectObject(mDC, mBrush);

        SetBkColor(mDC, 0x000000);
        SetTextColor(mDC, 0xffffff);

        RECT rect =  { 0, 0, mWidth, mHeight };
        FillRect(mDC, &rect, mBrush);

        LARGE_INTEGER freq;
        if (QueryPerformanceFrequency(&freq))
            mFrequency = (DOUBLE)freq.QuadPart;
    }

    IGOInfoDisplay::~IGOInfoDisplay()
    {
        DeleteObject(mBrush);
        DeleteObject(mFont);
        DeleteObject(mBitmap);
        DeleteDC(mDC);
    }

    void IGOInfoDisplay::write(INT x, INT y, const WCHAR *fmt, ...)
    {
        WCHAR msg[1024];

        va_list ap;
        va_start(ap, fmt);
        int len = wvsprintf(msg, fmt, ap);
        va_end(ap);

        RECT rect = { x, y, mWidth, mHeight };
        DrawText(mDC, msg, len, &rect, DT_LEFT);
    }

    void IGOInfoDisplay::update(void *context)
    {
	    static bool enabled = false;
	
	    if (enabled != mEnabled)
	    {
		    enabled = mEnabled;
		
		    // destroy the window...
		    if (!mEnabled)
				SAFE_CALL(IGOApplication::instance(), &IGOApplication::closeWindow, DISPLAY_WINDOW_ID);
	    }

        if (!mEnabled)
            return;

    
        int screenWidth = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
        int screenHeight = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);

        // check move
        int rate = 1;
        if ((gIGOKeyboardData[VK_LEFT]&0x80) && mPosX > 0)
            mPosX -= rate;
        if ((gIGOKeyboardData[VK_RIGHT]&0x80) && mPosX < screenWidth - mWidth)
            mPosX += rate;
        if ((gIGOKeyboardData[VK_UP]&0x80) && mPosY > 0)
            mPosY -= rate;
        if ((gIGOKeyboardData[VK_DOWN]&0x80) && mPosY < screenHeight - mHeight)
            mPosY += rate;

        // update window pos
        IGOWindow *win = (IGOApplication::instance() != NULL) ? IGOApplication::instance()->getWindow(DISPLAY_WINDOW_ID) : NULL;
        if (win)
            win->setPos(mPosX, mPosY);

        DWORD now = GetTickCount();
        mCount++;

        if (now - mTick > UPDATE_TIME)
        {
            // clear the image
            RECT rect =  { 0, 0, mWidth, mHeight };
            FillRect(mDC, &rect, mBrush);

            int width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
            int height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);

            // draw text
            int y = 5;
            write(5, y, L"Renderer: %s", SAFE_CALL(IGOApplication::instance(), &IGOApplication::rendererString));
            write(5, y += 20, L"FPS: %d", mCount);
            write(5, y += 20, L"IGO Time: %d us", mDT/mCount);
			if (IGOApplication::instance())
			{
				int count = IGOApplication::instance()->getWindowCount();
				char windowIDs[128] = { 0 };
				if (IGOApplication::instance()->getWindowIDs(windowIDs, sizeof(windowIDs)))
					write(5, y += 20, L"Windows (%d): %S", count, windowIDs);
			}
            write(5, y += 20, L"Windows: %d", SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowCount));
            write(5, y += 20, L"Texture Size: %d bytes", SAFE_CALL(IGOApplication::instance(), &IGOApplication::textureSize));
            write(5, y += 20, L"Screen Size: %dx%d", width, height);
    #ifdef _DEBUG
		    write(5, y += 20, L"hot keys: %x %x %x", gIGOKeyboardData[IGOApplication::instance()->mKey[0]], gIGOKeyboardData[IGOApplication::instance()->mKey[1]], gIGOKeyboardData[IGOApplication::instance()->mKey[2]]);
		    write(5, y += 20, L"gaks hot keys: %x %x %x", GetAsyncKeyStateHookNext(IGOApplication::instance()->mKey[0]), GetAsyncKeyStateHookNext(IGOApplication::instance()->mKey[1]), GetAsyncKeyStateHookNext(IGOApplication::instance()->mKey[2]));
		    write(5, y += 20, L"gks hot keys: %x %x %x", GetKeyStateHookNext(IGOApplication::instance()->mKey[0]), GetKeyStateHookNext(IGOApplication::instance()->mKey[1]), GetKeyStateHookNext(IGOApplication::instance()->mKey[2]));
		    BYTE tempKeyState[256]={0};
		    GetKeyboardStateHookNext(tempKeyState);
			write(5, y += 20, L"gkbds hot keys: %x %x %x", tempKeyState[IGOApplication::instance()->mKey[0]], tempKeyState[IGOApplication::instance()->mKey[1]], tempKeyState[IGOApplication::instance()->mKey[2]]);
			
			int mem = 0;
			int obj = 0;
			getPurgeIGOMemoryInformation(mem, obj);
			write(5, y += 20, L"Mantle refs: obj %i mem %i", obj, mem);
	#endif
            WCHAR keyboardLanguage[32] = {0};
            GetLocaleInfoW(MAKELCID(LOWORD(SAFE_CALL(IGOApplication::instance(), &IGOApplication::getIGOKeyboardLayout)), 0), LOCALE_SISO3166CTRYNAME, &keyboardLanguage[0], sizeof(keyboardLanguage)/sizeof(keyboardLanguage[0]));
            write(5, y += 20, L"Active Keyboard Language: %s", keyboardLanguage);
            write(5, y += 20, L"video stream pixel buffers: %i(%i)", TwitchManager::GetUsedPixelBufferCount(), TwitchManager::GetPixelBufferCount());
			write(5, y += 20, L"Options: (T)dr, (C)apture");
		
		    // Set of option keys available while the info display is visible
            float scale = SAFE_CALL(IGOApplication::instance(), &IGOApplication::animDurationScale);
            write(5, y += 20, L"Options: C(txt), R(st), S(tats), A(nim=%d)", (int)scale);

		    // patch up alpha
		    DWORD *pixels = (DWORD *)mBytes;
		    for (int i = 0; i < mWidth*mHeight; i++)
		    {
			    pixels[i] |= (pixels[i] == 0) ? 0xc0000000 : 0xff000000;
		    }
		
		    // update window
			copyToWindow(DISPLAY_WINDOW_ID, context);
		    mTick = now;
		    mCount = 0;
		    mDT = 0;
	    }
    }

    void IGOInfoDisplay::enable(bool enable)
    {
        if (mAllowed)
            mEnabled = enable;
    }

    void IGOInfoDisplay::startTime()
    {
        if (!mEnabled)
            return;

        LARGE_INTEGER ts;
        if (QueryPerformanceCounter(&ts))
            mStart = ts.QuadPart;
    }

    void IGOInfoDisplay::stopTime()
    {
        if (!mEnabled)
            return;

        LARGE_INTEGER ts;
        if (QueryPerformanceCounter(&ts))
        {
            mStop = ts.QuadPart;

            mDT += (DWORD)((mStop - mStart)*1000000/mFrequency);
        }
    }

    void IGOInfoDisplay::copyToWindow(uint32_t windowId, void* context)
    {
	    if (IGOApplication::instance())
	    {
		    IGOWindow *win = IGOApplication::instance()->getWindow(windowId);
		    win->update(context, true, 255, mPosX, mPosY, mWidth, mHeight, (const char *)mBytes, mWidth*mHeight*4, IGOIPC::WINDOW_UPDATE_ALL);
	    }
    }
}

#elif defined(ORIGIN_MAC)

#include "MacInputHook.h"
#include "OGLShader.h"

namespace OriginIGO
{
    extern BYTE gIGOKeyboardData[256];

    bool IGOInfoDisplay::mAllowed = false;

#define UPDATE_TIME 1000
    
    
    IGOInfoDisplay::IGOInfoDisplay() :
    mCount(0),
    mTick(0),
    mWidth(400),
#ifdef _DEBUG
    mHeight(240+20),
#else
    mHeight(170+20),
#endif
    mPosX(0),
    mPosY(0),
    mEnabled(false)
    {
        int bitmapBytesPerRow = mWidth * 4;
        int bitmapByteCount = bitmapBytesPerRow * mHeight;
        
        mData = new unsigned char[bitmapByteCount];
        CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        mCtxt = CGBitmapContextCreate(mData, mWidth, mHeight, 8, bitmapBytesPerRow, colorSpace, kCGImageAlphaPremultipliedLast);
        if (!mCtxt)
        {
            delete[] mData;
            mData = NULL;
        }
        
        CGColorSpaceRelease(colorSpace);
        
        CGContextSetRGBFillColor(mCtxt, 0.0f, 0.0f, 0.0f, 1.0f);
        CGContextFillRect(mCtxt, CGRectMake(0, 0, mWidth, mHeight));
        CGContextSelectFont(mCtxt, "Helvetica-Bold", 16, kCGEncodingMacRoman);
        CGContextSetCharacterSpacing(mCtxt, 0.0f);
        CGContextSetTextDrawingMode(mCtxt, kCGTextFill);
    }
    
    IGOInfoDisplay::~IGOInfoDisplay()
    {
        if (mCtxt)
        {
            delete[] mData;
            CGContextRelease(mCtxt);
        }
    }
    
    void IGOInfoDisplay::write(int x, int y, const char* fmt, ...)
    {
        char msg[1024];
        
        va_list ap;
        va_start(ap, fmt);
        int len = vsprintf(msg, fmt, ap);
        va_end(ap);
        
        CGContextShowTextAtPoint(mCtxt, x, y, msg, len);
    }
    
    void IGOInfoDisplay::update(void *context)
    {
        static bool enabled = false;
     
		OGLHook::DevContext* pDev = (OGLHook::DevContext*)context;
        IGOApplication* app = IGOApplication::instance();
        
        if (!mCtxt || !app)
            return;
        
        if (enabled != mEnabled)
        {
            enabled = mEnabled;
            
		    // destroy the window...
		    if (!mEnabled)
			    app->closeWindow(DISPLAY_WINDOW_ID);
	    }
        
        if (!mEnabled)
            return;
        
        if (pDev)
            pDev->SetRequiresRendering();

        int screenWidth = app->getScreenWidth();
        int screenHeight = app->getScreenHeight();
        
        // check move
        int rate = 1;
        if ((gIGOKeyboardData[VK_LEFT]&0x80) && mPosX > 0)
            mPosX -= rate;
        if ((gIGOKeyboardData[VK_RIGHT]&0x80) && mPosX < screenWidth - mWidth)
            mPosX += rate;
        if ((gIGOKeyboardData[VK_UP]&0x80) && mPosY > 0)
            mPosY -= rate;
        if ((gIGOKeyboardData[VK_DOWN]&0x80) && mPosY < screenHeight - mHeight)
            mPosY += rate;
        
        // update window pos
        IGOWindow *win = IGOApplication::instance()->getWindow(DISPLAY_WINDOW_ID);
        win->setPos(mPosX, mPosY);
        
        DWORD now = GetTickCount();
        mCount++;
        
        if (now - mTick > UPDATE_TIME)
        {
            // clear the image
            CGContextSetRGBFillColor(mCtxt, 0.0f, 0.0f, 0.0f, 1.0f);
            CGContextFillRect(mCtxt, CGRectMake(0, 0, mWidth, mHeight));
            
            int width = app->getScreenWidth();
            int height = app->getScreenHeight();
            float pixelToPointScaleFactor = app->getPixelToPointScaleFactor();
            
            // draw text
            CGContextSetRGBFillColor(mCtxt, 1.0f, 1.0f, 1.0f, 1.0f);
            
            int y = mHeight - 16;
            if (pDev)
                write(5, y, "Renderer: %s / %s", app->rendererString(), pDev->base.oglProperties.profile);
            else
                write(5, y, "Renderer: %s", app->rendererString());
            
            write(5, y -= 20, "FPS: %d", mCount);
            write(5, y -= 20, "IGO Time: %d us", mDT/mCount);
            write(5, y -= 20, "Windows: %d", app->getWindowCount());
            write(5, y -= 20, "Texture Size: %d bytes", app->textureSize());
            write(5, y -= 20, "Screen Size: %dx%d (x%.2f)", width, height, pixelToPointScaleFactor);
            write(5, y -= 20, "Hot keys: %x %x %x", gIGOKeyboardData[app->mKey[0]], gIGOKeyboardData[app->mKey[1]], gIGOKeyboardData[app->mKey[2]]);

            bool foundLanguage = false;
            TISInputSourceRef inputSource = TISCopyCurrentKeyboardInputSource();
            if (inputSource)
            {
                CFStringRef language = (CFStringRef)TISGetInputSourceProperty(inputSource, kTISPropertyLocalizedName);
                if (language)
                {
                    char tmp[64];
                    if (CFStringGetCString(language, tmp, sizeof(tmp), kCFStringEncodingMacRoman))
                    {
                        foundLanguage = true;
                        write(5, y -= 20, "Kbd Language: %s", tmp);
                    }
                }
                
                CFRelease(inputSource);
            }
            
            if (!foundLanguage)
                write(5, y -= 20, "Kbd Language: UNDEFINED");

            // Set of option keys available while the info display is visible
            float scale = SAFE_CALL(IGOApplication::instance(), &IGOApplication::animDurationScale);
            write(5, y -= 20, "Options: C(txt), R(st), D(ump), S(tats), A(nim=%i)", (int)scale);

		    // update window
			copyToWindow(DISPLAY_WINDOW_ID, context);
		    mTick = now;
		    mCount = 0;
		    mDT = 0;
	    }
    }
    
    void IGOInfoDisplay::toggle()
    {
        if (mAllowed)
            mEnabled = !mEnabled;
    }
    
    void IGOInfoDisplay::startTime()
    {
        if (!mEnabled)
            return;
        
        mStart = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
    }
    
    void IGOInfoDisplay::stopTime()
    {
        if (!mEnabled)
            return;
        
        mStop = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
        mDT  += (DWORD)((mStop - mStart) / (UInt64)1000.0);
    }
    
    void IGOInfoDisplay::copyToWindow(uint32_t windowId, void *context)
    {
        if (IGOApplication::instance())
        {
            IGOWindow *win = IGOApplication::instance()->getWindow(windowId);
            
            CGContextFlush(mCtxt);
		    win->update(context, true, 255, mPosX, mPosY, mWidth, mHeight, (const char *)CGBitmapContextGetData(mCtxt), mWidth*mHeight*4, IGOIPC::WINDOW_UPDATE_ALL);
	    }
    }
}

#endif // ORIGIN_MAC