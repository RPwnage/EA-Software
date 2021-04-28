//
//  IGOSetupOSX.h
//  EscalationService
//
//  Created by Frederic Meraud on 8/17/12.
//  Copyright (c) 2012 EA. All rights reserved.
//

#ifndef EscalationService_IGOSetupOSX_h
#define EscalationService_IGOSetupOSX_h

#include <stddef.h>
#include <sys/types.h>

#include "services/plugin/PluginAPI.h"

namespace Origin
{   
    namespace Engine
    {
        
        // Check the system preferences are setup appropriately for IGO
        // (right now, we require the 'Enable access for assistive devices' option to be on)
        ORIGIN_PLUGIN_API bool CheckIGOSystemPreferences(char* errorMsg, size_t maxErrorLen);
        
        // Open the Universal Access system preferences window
        ORIGIN_PLUGIN_API bool OpenUniversalAccessSystemPreferences(char* errorMsg, size_t maxErrorLen);
                        
        // Setup hooks for OIG: to bypass key equivalents when coming from OIG windows (ie disable CMD+Q), disable
        // the Flash plugin fullscreen, etc...
        ORIGIN_PLUGIN_API void SetupOIGHooks();
    }
}

#endif
