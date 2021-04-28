//
//  DllMain.h
//  QtInjector
//
//  Created by Frederic Meraud on 6/1/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef QtInjector_DllMain_h
#define QtInjector_DllMain_h

#include "IGO.h"

#if defined(ORIGIN_MAC)

bool AllocateSharedMemory(bool injected);
void FreeSharedMemory(bool injected);

void EnableIGOSupportedOnly(bool enable);
void EnableIGO(bool enable);
void EnableIGOLog(bool enable);
void EnableIGODebugMenu(bool enable);

bool IsIGOLogEnabled();
bool IsIGODebugMenuEnabled();

#endif

#endif
