//
//  OriginApplicationDelegateOSX.h
//  originClient
//
//  Created by Kiss, Bela on 12-08-23.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#ifndef ORIGINAPPLICATIONDELEGATEOSX_H
#define ORIGINAPPLICATIONDELEGATEOSX_H

#include <QApplication>

#include "services/plugin/PluginAPI.h"

namespace Origin {
    namespace Client {
    
        /// Registers the protocol scheme handlers (i.e., "origin://" and "eadm://") with the operating system.
        ORIGIN_PLUGIN_API void registerOriginProtocolSchemes();
        
        /// Registers for system sleep/wake events with the operating system.
        ORIGIN_PLUGIN_API void interceptSleepEvents();
        
        /// Installs our own handler for GetURL Apple Events, bypassing the Qt-builtin.
        /// The old Qt handler is not saved, so this action is not reversible.
        ORIGIN_PLUGIN_API void interceptGetUrlEvents();
        
        /// Installs our own handler for system shutdown events with the operating system.
        /// Because the implementation currently uses a forwarding app delegate (on OSX)
        /// this must be called affter any framework-installed app delegates (such as
        /// a Qt-internal QCocoaApplicationDelegate) are installed.  Call this function after
        /// all such initialization is performed, preferably from an event handler early on,
        /// but after QApplication construction.
        ORIGIN_PLUGIN_API void interceptShutdownEvents();
        
        /// Returns the list of windows in the application ordered back-to-front.
        ORIGIN_PLUGIN_API QWidgetList GetOrderedTopLevelWidgets();
        
        /// Sets the dock icon badge number
        ORIGIN_PLUGIN_API void setDockTile(int count);
        
        // Trigger normal NSApp flow to terminate the application (equivalent to the default Command+Q)
        ORIGIN_PLUGIN_API void triggerAppTermination();
    }
}

#endif // ORIGINAPPLICATIONDELEGATEOSX_H
