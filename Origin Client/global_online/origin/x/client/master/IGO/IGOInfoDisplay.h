///////////////////////////////////////////////////////////////////////////////
// IGOInfoDisplay.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef IGOINFODISPLAY_H
#define IGOINFODISPLAY_H

#if defined(ORIGIN_PC)

namespace OriginIGO {

    class IGOInfoDisplay
    {
    public:
        IGOInfoDisplay();
        ~IGOInfoDisplay();

	    void write(INT x, INT y, const WCHAR *fmt, ...);
	    void update(void *context);
	    void enable(bool enable);
        bool isEnabled() const { return mEnabled; }
	    
        void startTime();
	    void stopTime();

        static void allowDisplay(bool allowed) { mAllowed = allowed; }  // no need to make this thread-safe since only set once when injecting code
        
    private:
        int mWidth, mHeight;
        HBITMAP mBitmap;
        HFONT mFont;
        HBRUSH mBrush;
        BYTE *mBytes;
        HDC mDC;
        DWORD mTick;
        LONGLONG mStart;
        LONGLONG mStop;
        DOUBLE mFrequency;
        DWORD mDT;
        DWORD mCount;
        bool mEnabled;
        int mPosX, mPosY;
        
        static bool mAllowed;
        
		void copyToWindow(uint32_t id, void *context);
    };
}

#elif defined(ORIGIN_MAC)

#include <ApplicationServices/ApplicationServices.h>

#include "OGLHook.h"

namespace OriginIGO
{
    class IGOInfoDisplay
    {
    public:
        IGOInfoDisplay();
        ~IGOInfoDisplay();
        
	    void write(int x, int y, const char *fmt, ...);
	    void update(void *context);
	    void toggle();
        void startTime();
        void stopTime();
        
        bool isEnabled() const { return mEnabled; }
        
        static void allowDisplay(bool allowed) { mAllowed = allowed; }  // no need to make this thread-safe since only set once when injecting code

    private:
        UInt64 mStart;
        UInt64 mStop;
        
        DWORD mDT;
        DWORD mCount;
        DWORD mTick;
        
        int mWidth;
        int mHeight;
        int mPosX;
        int mPosY;
        
        CGContextRef mCtxt;
        unsigned char* mData;
        
        bool mEnabled;
        static bool mAllowed;

		void copyToWindow(uint32_t id, void *context);
    };
}

#else
#error "Need platform-specific implementation"
#endif

#endif

