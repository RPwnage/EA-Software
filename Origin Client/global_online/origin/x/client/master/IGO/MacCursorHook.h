//
//  MacCursorHook.h
//  InjectedCode
//
//  Created by Frederic Meraud on 7/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef MacCursorHook_h
#define MacCursorHook_h

#include "IGO.h"

#if defined(ORIGIN_MAC)

#include "IGOIPC/IGOIPC.h"


bool SetupCursorHooks();
void EnableCursorHooks(bool enabled);
void CleanupCursorHooks();

void SetIGOCursor(IGOIPC::CursorType type);

bool IsIGOEnabled();
void SetCursorIGOState(bool enabled, bool immediateUpdate, bool skipLocationRestore);
void SetCursorFocusState(bool hasFocus);
void SetCursorWindowInfo(int posX, int posY, int width, int height);
void NotifyCursorDetachedFromMouse();
void NotifyMainThreadCursorEvent();
void GetRealMouseLocation(float* x, float *y);

#endif

#endif