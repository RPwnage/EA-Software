///////////////////////////////////////////////////////////////////////////////
// IGO.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGO_H
#define IGO_H

#include "EABase/eabase.h"
#include "IGOTrace.h"

// This is here really to help when developing in VS IDE
#if !defined(ORIGIN_MAC) && !defined(ORIGIN_PC)

#if defined(_WIN32) || defined(_WIN64)
#define ORIGIN_PC
#elif defined(__APPLE__)
#define ORIGIN_MAC
#endif

#endif

namespace OriginIGO {

    extern int volatile gCalledFromInsideIGO;
	extern bool gWindowedMode;

#if defined(ORIGIN_PC)
    //disable display of IGO if window size is less than 1024x720 (so 1024x768 and 1028x720 will work fine!)
    // can be overwritten from EACore.ini with [IGO] ForceMinIGOResolution=900x600 for example
    const int MIN_IGO_DISPLAY_WIDTH = 1024;
    const int MIN_IGO_DISPLAY_HEIGHT = 720;
	extern HWND gRenderingWindow;
#else
    // can be overwritten from EACore.ini with [IGO] ForceMinIGOResolution=900x600 for example
    const int MIN_IGO_DISPLAY_WIDTH = 1024;
    const int MIN_IGO_DISPLAY_HEIGHT = 720;
#endif

    const unsigned int REHOOKCHECK_DELAY = 15000;    // 15 second delay for any API re-hooking
    const unsigned int DOUBLE_CLICK_DT = 250;        // dbl. click timeout 250 ms
    const unsigned int KEYBOARD_POLL_RATE = 125;    // 8 times a second


    #define RELEASE_IF(x) \
        if ((x) != NULL) { gCalledFromInsideIGO++; (x)->Release(); (x) = NULL; gCalledFromInsideIGO--; }

    #define DELETE_IF(x) \
        if ((x) != NULL) { delete (x); (x) = NULL; }

	#define DELETE_MANTLE_IF(x, y) \
		if ((x) != NULL) { (x)->dealloc(y); delete (x); (x) = NULL; }

	#define DELETE_ARRAY_IF(x) \
		if ((x) != NULL) { delete[] (x); (x) = NULL; }

    #define FREE_IF(x) \
        if ((x) != NULL) { free (x); (x) = NULL; }

    #define ARRSIZE(x) (sizeof(x)/sizeof(x[0]))

    #define SAFE_CALL_LOCK_ISREADLOCK_COND OriginIGO::IGOApplication::mInstanceMutex.IsReadLocked()
    #define SAFE_CALL_LOCK_TRYREADLOCK_COND OriginIGO::IGOApplication::mInstanceMutex.ReadTryLock()
    #define SAFE_CALL_LOCK_AUTO EA::Thread::AutoRWSpinLock lock(OriginIGO::IGOApplication::mInstanceMutex, EA::Thread::AutoRWSpinLock::kLockTypeRead);
    #define MANTLE_OBJECTS_AUTO EA::Thread::AutoFutex lock(OriginIGO::IGOApplication::mHelperMutex);
    #define SAFE_CALL_LOCK_ENTER OriginIGO::IGOApplication::mInstanceMutex.ReadLock();
    #define SAFE_CALL_LOCK_LEAVE OriginIGO::IGOApplication::mInstanceMutex.ReadUnlock();
    #define SAFE_CALL(ptrToObject, ptrToMember, ...) ((ptrToObject!=NULL) ? ((ptrToObject)->*(ptrToMember))(##__VA_ARGS__) : 0)
#ifdef ORIGIN_PC
    #define SAFE_CALL_LOCKED(ptrToObject, ptrToMember, ...) SAFE_CALL_LOCK_ENTER SAFE_CALL(ptrToObject, ptrToMember, ##__VA_ARGS__); SAFE_CALL_LOCK_LEAVE
	#define SAFE_CALL_NOARGS_LOCKED(ptrToObject, ptrToMember) SAFE_CALL_LOCK_ENTER SAFE_CALL(ptrToObject, ptrToMember); SAFE_CALL_LOCK_LEAVE
#endif

    #ifndef SAFE_DELETE
    #define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
    #endif    

    #ifndef SAFE_DELETE_ARRAY
    #define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
    #endif    

    #ifndef SAFE_RELEASE
    #define SAFE_RELEASE(p) { if (p) { (p)->Release();   (p)=NULL; } }
    #endif   

}
    
#endif
