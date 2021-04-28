//
//  IGOSetupOSX.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/17/12.
//  Copyright (c) 2012 EA. All rights reserved.
//

#include "engine/igo/IGOSetupOSX.h"

// Temporary mac until the build system is fixed
#ifndef ORIGIN_MAC
#define ORIGIN_MAC
#endif

#include "engine/igo/IGOController.h"
#include "services/platform/JRSwizzleOSX.h"

#import <Cocoa/Cocoa.h>


// ===========================================================================================================
// ===========================================================================================================

// Command used to open Universal Access preference panel
static NSString* LAUNCH_UNIVERSAL_ACCESS_PREFERENCE_PANEL = @"/System/Library/PreferencePanes/UniversalAccessPref.prefPane";

// Unique path to global Flash plugin configuration file
static NSString* FLASH_GLOBAL_CONFIGURATION_FILENAME = @"/Library/Application Support/Macromedia/mms.cfg";

// Local Flash plugin configuration file
static NSString* FLASH_LOCAL_CONFIGURATION_FILENAME = @"~/Library/Application Support/Origin/mms.cfg";

// Additional entries we want in the Flash configuration file
static NSString* FLASH_FULLSCREEN_DISABLE_ENTRIES = @"\nFullScreenDisable=1\nFullScreenInteractiveDisable=1\n";

// ===========================================================================================================
// ===========================================================================================================

@implementation NSMenu (OriginExtension)

DEFINE_SWIZZLE_METHOD_HOOK(NSMenu, BOOL, performKeyEquivalent, :(NSEvent*)theEvent)
{
    // Is this the main application menu?
    if (self == [NSApp mainMenu])
    {
        // Don't perform key equivalent flow if the event is coming from an OIG window (don't want to be able
        // to close the client from OIG for example!)
        NSWindow* wnd = [theEvent window];
        if (wnd)
        {
            // We use some of the properties set in the function SetupIGOWindow in IGOMacHelpers.mm to detect whether
            // the event is coming from an OIG window
            CGFloat alpha = [wnd alphaValue];
            BOOL isOpaque = [wnd isOpaque];
            BOOL ignoreMouseEvents = [wnd ignoresMouseEvents];
            
            if (!isOpaque && ignoreMouseEvents && alpha == 0.0f)
                return NO;
        }    
    }
    
    return CALL_SWIZZLED_METHOD(self, performKeyEquivalent, :theEvent);
}

@end

// ===========================================================================================================
// ===========================================================================================================

// Create a duplicate of the Flash global configuration file
bool CreateTemporaryFlashConfigurationFile()
{
    static int isInitializedSuccessfully = 0;
    if (isInitializedSuccessfully == 0)
    {
        @try
        {
            NSError* error = NULL;
            NSString* path = [FLASH_LOCAL_CONFIGURATION_FILENAME stringByExpandingTildeInPath];
            
            BOOL success = NO;
            
            // Load real config file dictionary
            NSMutableString* config = [NSMutableString stringWithContentsOfFile:FLASH_GLOBAL_CONFIGURATION_FILENAME encoding:NSUTF8StringEncoding error:&error];
            if (config)
            {
                // No need to parse - simply append our settings to the end of the file - they will be overwrite any previous setting
                [config appendString:FLASH_FULLSCREEN_DISABLE_ENTRIES];
                
                // Save file (any existing file will be overwritten)
                error = NULL;
                success = [config writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:&error];
                
            }
            
            else
            {
                // That's not expected, but not an error either
                const char* configFileName = [FLASH_GLOBAL_CONFIGURATION_FILENAME cStringUsingEncoding:[NSString defaultCStringEncoding]];
                if (error)
                {
                    const char* errorMsg = [(NSString*)[[error userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]];
                    PlatformServiceLogError("No valid config file '%s' (%s)", configFileName, errorMsg);
                }
                
                else
                    PlatformServiceLogError("No valid config file '%s'", configFileName);
                

                // Simply save our additional settings
                error = NULL;
                success = [FLASH_FULLSCREEN_DISABLE_ENTRIES writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:&error];
            }

            // K, so can we use our local version or should just read the real configuration file?
            if (success == NO)
            {
                isInitializedSuccessfully = -1;
                
                if (error)
                {
                    const char* errorMsg = [(NSString*)[[error userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]];
                    PlatformServiceLogError("Unable to save local config file '%s' (%s)", [path cStringUsingEncoding:[NSString defaultCStringEncoding]], errorMsg);
                }
            }
            
            else
                isInitializedSuccessfully = 1;            
        }
        @catch (NSException* ex)
        {
            isInitializedSuccessfully = -1;
            PlatformServiceLogError("Got exception %s while checking helper daemon:\n%s\n", [[ex name] cStringUsingEncoding:[NSString defaultCStringEncoding]]
                                                                                    , [[ex reason] cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        }
    }
    
    return isInitializedSuccessfully > 0;
}

// Hook the method used by the Flash plugin to load the global configuration file. This is to disable the fullscreen option.
@implementation NSFileHandle (OriginExtension)

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSFileHandle, id, fileHandleForReadingAtPath, :(NSString *)path)
{
    if (path)
    {
        if ([path caseInsensitiveCompare:FLASH_GLOBAL_CONFIGURATION_FILENAME] == NSOrderedSame)
        {
            // We only want to disable fullscreen while in IGO - btw here we'd better be safe and check whether the controller was instantiated
            // (just don't want to create its instance here)
            if (Origin::Engine::IGOController::instantiated() && Origin::Engine::IGOController::instance()->isActive())
            {
                if (CreateTemporaryFlashConfigurationFile())
                {
                    NSString* newPath = [FLASH_LOCAL_CONFIGURATION_FILENAME stringByExpandingTildeInPath];
                    return CALL_SWIZZLED_CLASS_METHOD(self, fileHandleForReadingAtPath, :newPath);
                }
            }
        }
    }
    
    return CALL_SWIZZLED_CLASS_METHOD(self, fileHandleForReadingAtPath, :path);
}

@end

@implementation NSWindow (OriginExtension)

void CheckWindowStateForDocking(NSWindow* window)
{
    // To properly show the window title in the dock, we need to make sure the window has the NSMiniaturizableWindowMask mask set (which is not the case
    // for Frameless window in Qt right now)
    NSUInteger style = [window styleMask];
    if ((style & NSMiniaturizableWindowMask) == 0)
    {
        style |= NSMiniaturizableWindowMask;
        [window setStyleMask:style];
    }
    
    NSString* title = [window title];
    [window setMiniwindowTitle: title];
}

DEFINE_SWIZZLE_METHOD_HOOK(NSWindow, void, miniaturize, :(id)sender)
{
    CheckWindowStateForDocking(self);
    
    CALL_SWIZZLED_METHOD(self, miniaturize, :sender);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSWindow, void, performMiniaturize, :(id)sender)
{
    CheckWindowStateForDocking(self);

    CALL_SWIZZLED_METHOD(self, performMiniaturize, :sender);
}

@end

// ===========================================================================================================
// ========================================= PUBLIC APIS =====================================================

namespace Origin
{
    
namespace Engine
{
        
// Check the system preferences are setup appropriately for IGO
// (right now, we require the 'Enable access for assistive devices' option to be on)
bool CheckIGOSystemPreferences(char* errorMsg, size_t maxErrorLen)
{
#if 0 // MAC OIG DOESN'T NEED ASSISTIVE DEVICES ON ANYMORE - BUT KEEPING THE CODE AROUND A LITTLE WHILE, JUST IN CASE
    // Check whether we are to deal with the new authentication process for assistive devices (Maverick and later)
    typedef Boolean (*AXIsProcessTrustedWithOptionsFuncType)(CFDictionaryRef);
    
    static AXIsProcessTrustedWithOptionsFuncType AXIsProcessTrustedWithOptionsFunc = (AXIsProcessTrustedWithOptionsFuncType)dlsym(RTLD_DEFAULT, "AXIsProcessTrustedWithOptions");
    static CFStringRef* kAXTrustedCheckOptionPromptFunc = (CFStringRef*)dlsym(RTLD_DEFAULT, "kAXTrustedCheckOptionPrompt");
    
    if (AXIsProcessTrustedWithOptionsFunc && kAXTrustedCheckOptionPromptFunc)
    {
        // The authentication is temporarily going to happen on the game side
        return true;
    }

    return AXAPIEnabled();
#else
    return true;
#endif
}

// Open the Universal Access system preferences window
bool OpenUniversalAccessSystemPreferences(char* errorMsg, size_t maxErrorLen)
{
    bool retVal = true;
    if (errorMsg && maxErrorLen > 0)
        errorMsg[0] = '\0';
    
    @try
    {
        retVal = [[NSWorkspace sharedWorkspace] openFile:LAUNCH_UNIVERSAL_ACCESS_PREFERENCE_PANEL];
    }
    @catch (NSException* ex)
    {
        retVal = false;
        snprintf(errorMsg, maxErrorLen, "Got exception %s while trying to open universal access preference panel:\n%s\n", [[ex name] cStringUsingEncoding:[NSString defaultCStringEncoding]]
                                                                                                                        , [[ex reason] cStringUsingEncoding:[NSString defaultCStringEncoding]]);        
    }
    
    return retVal;
}

// Setup hooks for OIG: to bypass key equivalents when coming from OIG windows (ie disable CMD+Q), disable
// the Flash plugin fullscreen, etc...
void SetupOIGHooks()
{
    @autoreleasepool
    {
        static bool initialized = false;
        if (!initialized)
        {
            initialized = true;
            
            bool success = true;
            NSError* myError = nil;
            
            // Disable key eqiovalents for OIG windows (for example don't allow CMD+Q to close the client while in OIG)
            Class nsMenu = NSClassFromString(@"NSMenu");        
            if (nsMenu)
            {
                SWIZZLE_METHOD(success, NSMenu, nsMenu, performKeyEquivalent, :, myError)
            }
            
            // Disable Flash plugin fullscreen.
            Class nsFileHandle = NSClassFromString(@"NSFileHandle");
            if (nsFileHandle)
            {
                CreateTemporaryFlashConfigurationFile();
                SWIZZLE_CLASS_METHOD(success, NSFileHandle, nsFileHandle, fileHandleForReadingAtPath, :, myError)
            }
            
            Class nsWindow = NSClassFromString(@"NSWindow");
            if (nsWindow)
            {
                SWIZZLE_METHOD(success, NSWindow, nsWindow, miniaturize, :, myError)
                SWIZZLE_METHOD(success, NSWindow, nsWindow, performMiniaturize, :, myError)
            }
        }
    }
}
    
} // namespace Engine
} // namespace Origin
