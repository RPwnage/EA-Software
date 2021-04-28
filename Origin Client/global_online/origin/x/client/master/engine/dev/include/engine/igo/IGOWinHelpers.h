//
//  IGOWinHelpers.h
//  engine
//
//  Created by Frederic Meraud on 2/24/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __engine__IGOWinHelpers__
#define __engine__IGOWinHelpers__

#ifdef ORIGIN_PC

#define NOMINMAX
#include <Windows.h>

namespace Origin
{
    namespace Engine
    {
        HMODULE LoadIGODll();
        void UnloadIGODll();
        
        void HookWin32();
        void UnhookWin32();
    }
}
#endif

#endif /* defined(__engine__IGOWinHelpers__) */
