//
//  MacGoodies.h
//  InjectedCode
//
//  Created by Frederic Meraud on 6/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef MacRenderHook_h
#define MacRenderHook_h

#include "IGO.h"

#if defined(ORIGIN_MAC)

#include <AGL/Agl.h>
#include <OpenGL/OpenGL.h>

typedef void (*SetViewPropertiesCallback)(int posX, int posY, int width, int height, int offsetX, int offsetY, bool hasFocus, bool isKeyWindow, bool isFullscreen, float pixelToPointScaleFactor);
typedef void (*MenuActivationCallback)(bool isActive);
bool SetupRenderHooks(SetViewPropertiesCallback viewCallback, MenuActivationCallback menuCallback);
void EnableRenderHooks(bool enabled);
void CleanupRenderHooks();

typedef void (*FocusChangedCallback)(bool hasFocus);
typedef void (*ApplicationDoneCallback)();

bool ExtractAGLContextViewProperties(AGLContext ctxt, SetViewPropertiesCallback callback);
void ExtractCGLContextViewProperties(CGLContextObj ctxt, SetViewPropertiesCallback callback);

void* GetMainWindow();
bool MainWindowHasFocus();

void InitializeCocoa();

#endif

#endif

