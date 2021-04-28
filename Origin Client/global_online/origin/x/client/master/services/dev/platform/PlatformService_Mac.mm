//
//

#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"

#include <Foundation/Foundation.h>

#include <QString>

#include <sys/param.h>
#include <sys/mount.h>

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <AppKit/AppKit.h>
#import <Security/Security.h>
#import <Security/SecStaticCode.h>
#import <Security/SecRequirement.h>
#import <ServiceManagement/ServiceManagement.h>

#include <errno.h>
#include <syslog.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <AppKit/NSWindow.h>
#include <QWidget>

// Simple object to run an AppleScript scenario. We use a separate object/method to implement the functionality so that we can run it on the main thread
// since NSAppleScript is not thread-safe (and hopefully we're not going to break the rest of the client doing that!)
static const NSString* ASCodeKey = @"Code";
static const NSString* ASErrorDescKey = @"ErrorDesc";
static const NSString* ASReturnValue = @"ReturnValue";

@interface PlatformAppleScript : NSObject

-(void) run:(NSMutableDictionary*)params;

@end

@implementation PlatformAppleScript

-(void) run:(NSMutableDictionary*) params
{
    NSString* script = (NSString*)[params objectForKey:ASCodeKey];
    NSAppleScript* appleScript = [[NSAppleScript alloc] initWithSource:script];
    if (appleScript)
    {
        NSDictionary* errors = nil;
        NSAppleEventDescriptor* retDesc = [appleScript executeAndReturnError:&errors];
        [appleScript release];
        
        if (retDesc)
        {
            // We're good!
            [params setObject:[[NSNumber alloc] initWithBool:YES] forKey:ASReturnValue];
        }
        
        else
        {
            // Error
            [params setObject:[[NSNumber alloc] initWithBool:NO] forKey:ASReturnValue];
            [params setObject:[[NSString alloc] initWithString:[errors objectForKey:NSAppleScriptErrorBriefMessage]] forKey:ASErrorDescKey];
        }
    }
}
@end

// This only purpose of all of this is to support the Ctrl+Tab/Ctrl+Shift+Tab hot keys which are "eating up"
// by the Mac Keyboard Interface Control to support the key view loop (we may need to expect this function
// a bit to handle more of the fixed Mac hot keys).
@interface KICWatcher : NSObject
{
    id _tap;
    NSWindow* _window;
    KeyMap _hotKeyKdbMask;
    Origin::Services::PlatformService::KICWatchListener* _listener;
}

-(id)initWithView:(NSView*)view AndListener:(Origin::Services::PlatformService::KICWatchListener*)listener;
-(bool)isOnlyKeyPressed:(CGKeyCode)keyCode;
-(void)flagKeyboardKeyCode:(KeyMap&)map WithKeyCode:(CGKeyCode)keyCode;
-(void)cleanup;

@end

@implementation KICWatcher

-(id)initWithView:(NSView *)view AndListener:(Origin::Services::PlatformService::KICWatchListener*)listener
{
    self = [super init];
    if (self)
    {
        _window = [view window];
        _listener = listener;
        
        for (int idx = 0; idx < 4; ++idx)
            _hotKeyKdbMask[idx].bigEndianValue = 0;

        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_CapsLock];
        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_Function];
        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_Control];
        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_RightControl];
        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_Shift];
        [self flagKeyboardKeyCode:_hotKeyKdbMask WithKeyCode:kVK_RightShift];
        
        _tap = [NSEvent addLocalMonitorForEventsMatchingMask:(NSKeyDownMask | NSFlagsChangedMask) handler:^(NSEvent* event)
                {
                    if ([event window] == _window)
                    {
                    // Don't care if this is not a tab nor if the Ctrl modifier is not pressed
                    if ([event keyCode] == kVK_Tab)
                    {
                        NSUInteger flags = [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
                        if (flags == NSControlKeyMask || flags == (NSControlKeyMask | NSShiftKeyMask))
                        {
                                // Is tab the only key pressed?
                                if ([self isOnlyKeyPressed:kVK_Tab])
                                {
                            // Call the ... callback!
                            int key = Qt::Key_Tab;
                            Qt::KeyboardModifiers modifiers = Qt::ControlModifier;
                            if (flags & NSShiftKeyMask)
                                modifiers |= Qt::ShiftModifier;
                            
                            _listener->KICWatchEvent(key, modifiers);
                            
                            // No need to keep this one around (well, until Qt (and us) start using the KIC correctly!)
                            //return (NSEvent*)nil;
                        }
                    }
                        }
                    }
                    
                    return event;
                }];
    }
    
    return self;
}

// Helper to "activate" a particular keycode in a keyboard map
-(void)flagKeyboardKeyCode:(KeyMap&)map WithKeyCode:(CGKeyCode)keyCode
{
    uint8_t index = keyCode / 32;
    uint8_t shift = keyCode % 32;
    uint32_t value = 1 << shift;
    if (index < 4)
        map[index].bigEndianValue |= value;
}

-(bool)isOnlyKeyPressed:(CGKeyCode)keyCode
{
    // Get current keyboard state
    KeyMap kbdMap;
    GetKeys(kbdMap);
    
    // Update our mask with the current keycode we are considering
    KeyMap hotKeyMap;
    for (int idx = 0; idx < 4; ++idx)
        hotKeyMap[idx].bigEndianValue = _hotKeyKdbMask[idx].bigEndianValue;
    
    [self flagKeyboardKeyCode:hotKeyMap WithKeyCode:keyCode];
    
    // Check if anything else is pressed...
    for (int idx = 0; idx < 4; ++idx)
    {
        if ((kbdMap[idx].bigEndianValue & ~hotKeyMap[idx].bigEndianValue) != 0)
            return false;
    }
    
    return true;
}

-(void)cleanup
{
    if (_tap)
    {
        [NSEvent removeMonitor:_tap];
        _tap = nil;
    }
}

@end

QString lookupPathForAppBundleId( const QString& bundleId )
{
    @autoreleasepool
    {
        NSString* bundle = [NSString stringWithUTF8String:bundleId.toUtf8()];
        NSString* appPath = [[NSWorkspace sharedWorkspace] absolutePathForAppBundleWithIdentifier:bundle];
        
        QString ret;
        if ( appPath != nil ) ret = QString( [appPath UTF8String] );

        return ret;
    }
}
// ===========================================================================================================
// ===========================================================================================================


namespace Origin
{
namespace Services
{
namespace PlatformService
{
    void addWindowToExpose( QWidget* window )
    {
        // Make the window managed.  (I.e., The window participates in Spaces and Expose.)
        // Available in OS X v10.6 and later.
        NSView* view = (NSView*)window->winId();
        NSWindow* wnd = [view window];

        [wnd setCollectionBehavior:NSWindowCollectionBehaviorManaged];
    }
    
    extern bool isReadOnlyPath(QString path)
    {
        QByteArray ba = path.toUtf8();
        
        struct statfs mountStatus;
        
        statfs(ba.data(), &mountStatus);
        
        return mountStatus.f_flags & MNT_RDONLY;
    }
    
    extern QString applicationBundleRootPath()
    {
        NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
        return QString::fromUtf8([bundlePath UTF8String]) + '/';
    }
    
    extern QString systemPreferencesPath()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSLocalDomainMask, NO);
        NSUInteger numPaths = [paths count];
        ORIGIN_UNUSED(numPaths);
        ORIGIN_ASSERT(numPaths > 0);
        NSString* libraryDir = [paths objectAtIndex:0];
        QString qLibraryDir(QString::fromUtf8([libraryDir UTF8String]));
        return qLibraryDir;
    }
    
    extern QString userPreferencesPath()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSUInteger numPaths = [paths count];
        ORIGIN_UNUSED(numPaths);
        ORIGIN_ASSERT(numPaths > 0);
        NSString* libraryDir = [paths objectAtIndex:0];
        QString qLibraryDir(QString::fromUtf8([libraryDir UTF8String]));
        return qLibraryDir;
    }
    
    QString resourcesDirectoryMac()
    {
        NSString* resources = [[NSBundle mainBundle] resourcePath];
        return QString::fromUtf8([resources UTF8String]) + '/';
	}

    // Returns the unique identifier of the bundle specified by its path
    QString getBundleIdentifier(QString path)
    {
        QString bundleId;
        
        @autoreleasepool
        {
            NSBundle* bundle = [NSBundle bundleWithPath:[NSString stringWithUTF8String:path.toUtf8()]];
            if (bundle)
                bundleId = QString::fromUtf8([[bundle bundleIdentifier] UTF8String]);
        }
        
        return bundleId;
    }

    /// Returns the unique identifier of the bundle specified by its running executable pid
    QString getBundleIdentifier(pid_t pid)
    {
        QString bundleId;
        
        @autoreleasepool
        {
            NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
            if (app)
                bundleId = QString::fromUtf8([[app bundleIdentifier] UTF8String]);
        }
        
        return bundleId;
    }

    // The helper tool identifier corresponds to the bundle identifier defined in the helper tool .plist. The same identifier should be used
    // in the client .plist when defining the which client application can install the helper tool.
    static const NSString* HELPER_TOOL_IDENTIFIER = @"com.ea.origin.ESHelper";
#ifdef ORIGIN_MAC_OFFICIAL_CERT
    //static const NSString* HELPER_TOOL_CERTIFICATE_PATTERN = @"identifier com.ea.origin.ESHelper and certificate leaf[subject.CN] = \"Developer ID Application: Electronic Arts Inc.\"";
    static const char* HELPER_TOOL_IGO_CERTIFICATE_PATTERN = "identifier com.ea.origin.IGO and certificate leaf[subject.CN] = \"Developer ID Application: Electronic Arts Inc.\"";
#else
    //static const NSString* HELPER_TOOL_CERTIFICATE_PATTERN = @"identifier com.ea.origin.ESHelper and certificate leaf[subject.CN] = \"OriginDev\"";
    static const char* HELPER_TOOL_IGO_CERTIFICATE_PATTERN = "identifier com.ea.origin.IGO and certificate leaf[subject.CN] = \"OriginDev\"";
#endif
    
    enum HelperToolState
    {
        HelperToolState_NOT_INSTALLED,
        HelperToolState_OLDER_VERSION,
        HelperToolState_LATEST_VERSION
    };
    
    bool authExecDelete(AuthorizationRef authRef, char* pFilepath, const bool debugEnabled)
    {
        FILE *pipe = NULL;
        
        const char *tool = "/bin/rm";
        char *rmArgs[] = { "-f", pFilepath, NULL };
        
        OSStatus status = AuthorizationExecuteWithPrivileges(
            authRef,
            tool,
            kAuthorizationFlagDefaults,
            rmArgs,
            &pipe);
        
        NSString* logMessage = [NSString stringWithFormat:@"%s %s %s: result code %d\n",
            tool,
            rmArgs[0],
            rmArgs[1],
            (int) status];

        // Emit log messages
        if (debugEnabled)
        {
            syslog(LOG_NOTICE, [logMessage UTF8String]);
        }

        return errAuthorizationSuccess == status;
    }
    
    NSString * getLibrarySystemPath()
    {
        NSURL* libraryDir = [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:NO error:nil];
        
        return [libraryDir path];
    }
    
    bool forceDeleteHelper(AuthorizationRef authRef, const bool debugEnabled)
    {
        bool success = true;
        
        // Used for logging to system console
        NSMutableString* logMessage = [NSMutableString string];
        
        // Obtain the system library path
        NSString* systemLibraryPath = getLibrarySystemPath();
        
        if (nil == systemLibraryPath)
        {
            // Hardcode the library path if we can't obtain it for some reason.
            systemLibraryPath = @"/Library";
        }
        
        // Need non-const char* of helper tool because of Apple API restrictions
        const uint32_t HELPER_IDENTIFIER_ARRAY_SIZE = 128;
        char helperIdentifier[HELPER_IDENTIFIER_ARRAY_SIZE] = { 0 };
        snprintf(helperIdentifier, HELPER_IDENTIFIER_ARRAY_SIZE, "%s", [HELPER_TOOL_IDENTIFIER UTF8String]);
        
        // For "launchctl unload", we need full path to the helper
        char helperToolPath[PATH_MAX] = { 0 };
        snprintf(helperToolPath, PATH_MAX, "%s/PrivilegedHelperTools/%s",
                 [systemLibraryPath UTF8String],
                 [HELPER_TOOL_IDENTIFIER UTF8String]);
        
        // Remove launchd association with ESHelper via launchctl
        {
            FILE *pipe = NULL;
            const char *launchCtlCommand = "/bin/launchctl";
            
            // sudo launchctl remove com.ea.origin.EShelper
            char *launchCtlArgsRemove[] = { "remove", helperIdentifier, NULL };
            OSStatus status = AuthorizationExecuteWithPrivileges(
                 authRef,
                 launchCtlCommand,
                 kAuthorizationFlagDefaults,
                 launchCtlArgsRemove,
                 &pipe);
            
            success &= errAuthorizationSuccess == status;
            [logMessage appendFormat:@"%s %s %@: result code %d\n",
                launchCtlCommand,
                launchCtlArgsRemove[0],
                HELPER_TOOL_IDENTIFIER,
                (int) status];
            
            // sudo launchctl unload -w com.ea.origin.EShelper
            char *launchCtlArgsUnload[] = { "unload", "-w", helperToolPath, NULL };
            status = AuthorizationExecuteWithPrivileges(
                authRef,
                launchCtlCommand,
                kAuthorizationFlagDefaults,
                launchCtlArgsUnload,
                &pipe);
            
            success &= errAuthorizationSuccess == status;
            [logMessage appendFormat:@"%s %s %s %@: result code %d\n",
                launchCtlCommand,
                launchCtlArgsUnload[0],
                launchCtlArgsUnload[1],
                HELPER_TOOL_IDENTIFIER,
                (int) status];
        }
        
        // Emit log messages
        if (debugEnabled)
        {
            syslog(LOG_NOTICE, [logMessage UTF8String]);
        }
        
        // Construct path to helper plist file
        char helperPlistPath[PATH_MAX] = { 0 };
        snprintf(helperPlistPath, PATH_MAX, "%s/LaunchDaemons/%s.plist",
                 [systemLibraryPath UTF8String],
                 [HELPER_TOOL_IDENTIFIER UTF8String]);
        
#if defined( _DEBUG )
        {
            // Some sanity checks. If these asserts fire, our assumptions about
            // these paths have changed and this function should be re-assessed.
            const char* expectedHelperPath = "/Library/PrivilegedHelperTools/com.ea.origin.ESHelper";
            assert(strlen(helperToolPath) == strlen(expectedHelperPath));
            assert(0 == strncmp(helperToolPath, expectedHelperPath, strlen(expectedHelperPath)));
            
            const char* expectedHelperPlistPath = "/Library/LaunchDaemons/com.ea.origin.ESHelper.plist";
            assert(strlen(helperPlistPath) == strlen(expectedHelperPlistPath));
            assert(0 == strncmp(helperPlistPath, expectedHelperPlistPath, strlen(expectedHelperPlistPath)));
        }
#endif // #if defined( _DEBUG )

        // Delete helper files
        {
            success &= authExecDelete(authRef, helperToolPath, debugEnabled);
            success &= authExecDelete(authRef, helperPlistPath, debugEnabled);
        }
        
        return success;
    }
    
    // Install the helper tool, requesting user authorization if necessary.
    bool blessHelperWithLabel(const NSString* label, bool removeOlderVersion, bool debugEnabled, char* errorMsg, size_t maxErrorLen)
    {
        bool result = false;
        
        AuthorizationItem authItems[3]  = {
            { kSMRightBlessPrivilegedHelper, 0, NULL, 0 },
            { kSMRightModifySystemDaemons, 0, NULL, 0 },
            { kAuthorizationRightExecute, 0, NULL, 0 } };
        AuthorizationRights authRights	= { 3, authItems };
        AuthorizationFlags flags		=	kAuthorizationFlagDefaults              |
                                            kAuthorizationFlagPreAuthorize          |
                                            kAuthorizationFlagExtendRights;
        
        AuthorizationRef authRef = NULL;
        OSStatus status = errAuthorizationSuccess;
        
        const int MAX_AUTHORIZATION_ITEMS = 3;
        AuthorizationItem userData[MAX_AUTHORIZATION_ITEMS];
        int numAuthorizationItems = 0;
        
        // Can we get the credentials from the EACore.ini file? (for automation)
        QString userName = Origin::Services::readSetting(Origin::Services::SETTING_CredentialsUserName);
        QString password = Origin::Services::readSetting(Origin::Services::SETTING_CredentialsPassword);
        if (!userName.isEmpty() && !password.isEmpty())
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Using username/passwd from settings");
            
            char cUserName[128];
            strncpy(cUserName, qPrintable(userName), sizeof(cUserName));
            cUserName[sizeof(cUserName) - 1] = '\0';
            
            userData[numAuthorizationItems].name = kAuthorizationEnvironmentUsername;
            userData[numAuthorizationItems].valueLength = strlen(cUserName);
            userData[numAuthorizationItems].value = cUserName;
            userData[numAuthorizationItems++].flags = 0;
            
            char cPassword[128];
            strncpy(cPassword, qPrintable(password), sizeof(cPassword));
            cPassword[sizeof(cPassword) - 1] = '\0';
            
            userData[numAuthorizationItems].name = kAuthorizationEnvironmentPassword;
            userData[numAuthorizationItems].valueLength = strlen(cPassword);
            userData[numAuthorizationItems].value = cPassword;
            userData[numAuthorizationItems++].flags = 0;
        }
        
        else
        {
            flags |= kAuthorizationFlagInteractionAllowed;
        }

        // We are making a sys call that will retrive the Kernel version
        // This is used to dertermine which string to display for the Escalation Service prompt
        char str[256];
        size_t size = sizeof(str);
        sysctlbyname("kern.osrelease", str, &size, NULL, 0);
        
        // Kernel number are as follows
        // 12.x.x  OS X 10.8.x Mountain Lion
        // 11.x.x  OS X 10.7.x Lion
        // 10.x.x  OS X 10.6.x Snow Leopard
        
        QStringList osVersion = QString(str).split(("."));
        QString prompt;
        // This is to check that we are not running Snow Leopard
        if (osVersion.size() > 0 && osVersion.at(0).toInt() >= 11)
            prompt = (QObject::tr("ebisu_client_manage_your_games_107_108").arg(QCoreApplication::applicationName()));
        else
            prompt = (QObject::tr("ebisu_client_manage_your_games").arg(QCoreApplication::applicationName()));
        
        QByteArray utf8Prompt(prompt.toUtf8());
        userData[numAuthorizationItems].name = kAuthorizationEnvironmentPrompt;
        userData[numAuthorizationItems].valueLength = utf8Prompt.length();
        userData[numAuthorizationItems].value = utf8Prompt.data();
        userData[numAuthorizationItems++].flags = 0;

        ORIGIN_ASSERT(numAuthorizationItems <= MAX_AUTHORIZATION_ITEMS);
        
        /* Obtain the right to install privileged helper tools (kSMRightBlessPrivilegedHelper). */
        AuthorizationItemSet env = { static_cast<UInt32>(numAuthorizationItems), userData };
        status = AuthorizationCreate(&authRights, &env, flags, &authRef);
        
        if (status != errAuthorizationSuccess) 
        {
            GetTelemetryInterface()->Metric_APP_MAC_ESCALATION_DENIED();
            snprintf(errorMsg, maxErrorLen, "Failed to create AuthorizationRef, return code %ld", (long)status);            
        }
        
        else 
        {
            /* This does all the work of verifying the helper tool against the application
             * and vice-versa. Once verification has passed, the embedded launchd.plist
             * is extracted and placed in /Library/LaunchDaemons and then loaded. The
             * executable is placed in /Library/PrivilegedHelperTools.
             */
            NSError* error = NULL;
            
            // Remove it if it was already running/installed
            bool removeSuccess = true;
            if (removeOlderVersion)
            {
                if (debugEnabled)
                    syslog(LOG_NOTICE, "Removing previous service");
                
                if (!SMJobRemove(kSMDomainSystemLaunchd, (CFStringRef)HELPER_TOOL_IDENTIFIER, authRef, true, (CFErrorRef *)&error))
                {
                    if (debugEnabled)
                    {
                        syslog(LOG_NOTICE, "Failed to remove old helper");
                    }
                    
                    removeSuccess = forceDeleteHelper(authRef, debugEnabled);
                    if (error)
                    {
                        [error autorelease];
                        [[error description] getCString:errorMsg maxLength:maxErrorLen encoding:[NSString defaultCStringEncoding]];
                    }
                    
                    else
                        snprintf(errorMsg, maxErrorLen, "Unable to remove helper tool previous install");
                }                    
            }

            // ORPLAT-2640: On OS X 10.11 (El Capitan), SMJobRemove can report success even though the helper files haven't
            // been deleted preventing SMJobBless from working correctly so now we always force a manual deletion of the
            // helper files. Nuking it from orbit; it's the only way to be sure.
            forceDeleteHelper(authRef, debugEnabled);
            
            if (removeSuccess)
            {
                if (debugEnabled)
                    syslog(LOG_NOTICE, "Installing new service");

                if (!SMJobBless(kSMDomainSystemLaunchd, (CFStringRef)HELPER_TOOL_IDENTIFIER, authRef, (CFErrorRef *)&error))
                {
                    if (debugEnabled)
                    {
                        const NSString* logMessage = [NSString stringWithFormat:@"Failed to bless %@", HELPER_TOOL_IDENTIFIER];
                        syslog(LOG_NOTICE, [logMessage UTF8String]);
                    }
                    
                    forceDeleteHelper(authRef, debugEnabled);
                    
                    if (error)
                    {
                        [error autorelease];
                        [[error description] getCString:errorMsg maxLength:maxErrorLen encoding:[NSString defaultCStringEncoding]];
                    }
                    
                    else
                        snprintf(errorMsg, maxErrorLen, "Unable to install helper tool (no specific error msg)");
                }
                
                else
                {
                    result = true;
                }
            }
        }
        
        if (authRef)
            AuthorizationFree(authRef, kAuthorizationFlagDefaults);
        
        return result;
    }
    
    // Check if the helper tool is already installed and whether the file contents sha1 are the same (we used to try comparing the version/certificate requirements,
    // however accessing the bundle breaks the flow when we have to remove a previously installed service - likely due to the caching of the bundle data -> the
    // SMJobBless API call then breaks inside the CodeSigning framework)
    HelperToolState CheckHelperToolCurrentInstall(bool debugEnabled)
    {
        // Is the tool installed?
        NSDictionary* helperToolInfo = (NSDictionary*)SMJobCopyDictionary(kSMDomainSystemLaunchd, (CFStringRef)HELPER_TOOL_IDENTIFIER);
        if (!helperToolInfo)
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Helper tool not installed");
            
            return HelperToolState_NOT_INSTALLED;
        }
        
        HelperToolState installState = HelperToolState_OLDER_VERSION;
        
        // Find the URL of the currently installed service
        NSString* installedPath = [[helperToolInfo objectForKey:@"ProgramArguments"] objectAtIndex:0];
        NSURL* installedPathURL = [NSURL fileURLWithPath:installedPath];
        CFRelease((CFDictionaryRef)helperToolInfo);
        if (!installedPathURL)
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Unable to access installed service URL");
            
            return installState;
        }
        
        // Find the URL of the new service to install
        NSBundle* appBundle	= [NSBundle mainBundle];
        NSURL* appBundleURL	= [appBundle bundleURL];
        NSURL* currentHelperToolURL	= [appBundleURL URLByAppendingPathComponent:[NSString stringWithFormat:@"Contents/Library/LaunchServices/%@", HELPER_TOOL_IDENTIFIER]];
        if (!currentHelperToolURL)
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Unable to access internal service URL");
            
            return installState;
        }

        // Get some nice file system filenames
        char fileName1[512];
        char fileName2[512];
        Boolean isValid1 = CFURLGetFileSystemRepresentation((CFURLRef)installedPathURL, true, (UInt8*)fileName1, sizeof(fileName1));
        if (!isValid1)
        {
            if (debugEnabled)
            {
                char tmp[512];
                if ([[installedPathURL relativeString] getCString:tmp maxLength:sizeof(tmp) encoding:[NSString defaultCStringEncoding]] == NO)
                    snprintf(tmp, sizeof(tmp), "??");
                
                syslog(LOG_NOTICE, "Unable to get file representation for URL: '%s'", tmp);
            }
            
            return installState;
        }
        
        Boolean isValid2 = CFURLGetFileSystemRepresentation((CFURLRef)currentHelperToolURL, true, (UInt8*)fileName2, sizeof(fileName2));
        if (!isValid2)
        {
            if (debugEnabled)
            {
                char tmp[512];
                if ([[currentHelperToolURL relativeString] getCString:tmp maxLength:sizeof(tmp) encoding:[NSString defaultCStringEncoding]] == NO)
                    snprintf(tmp, sizeof(tmp), "??");
                
                syslog(LOG_NOTICE, "Unable to get file representation for URL: '%s'", tmp);
            }
            
            return installState;
        }
        
        
        // And compare the sha1 values
        if (Origin::Services::PlatformService::haveIdenticalContents(fileName1, fileName2, debugEnabled))
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Detected same version of service");
            
            installState = HelperToolState_LATEST_VERSION;
        }
        
        else
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Detected different version of service");
        }
        
        return installState;
    }
    
    // ===========================================================================================================
    // ============================================= PUBLIC API ==================================================
    extern bool isBeta()
	{
		NSString* plistPath = [NSString stringWithFormat:@"%@/Contents/ClientInfo.plist", [[NSBundle mainBundle] bundlePath]];
		if([[NSFileManager defaultManager] fileExistsAtPath:plistPath])
		{
			NSDictionary* dict = [NSDictionary dictionaryWithContentsOfFile:plistPath];
			if(dict != nil)
			{
				NSString* isBeta = [dict objectForKey:@"IsBeta"];
				if(isBeta != nil)
				{
					return ([isBeta caseInsensitiveCompare:@"true"] == 0);
				}
			}
		}

		return false;
	}
	
    extern QString getSystemPreferencesPath()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSLocalDomainMask, NO);
        NSUInteger numPaths = [paths count];
        ORIGIN_UNUSED(numPaths);
        ORIGIN_ASSERT(numPaths > 0);
        NSString* libraryDir = [paths objectAtIndex:0];
        QString qLibraryDir(QString::fromUtf8([libraryDir UTF8String]));
        return qLibraryDir;
    }
    
    extern QString getUserPreferencesPath()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSUInteger numPaths = [paths count];
        ORIGIN_UNUSED(numPaths);
        ORIGIN_ASSERT(numPaths > 0);
        NSString* libraryDir = [paths objectAtIndex:0];
        QString qLibraryDir(QString::fromUtf8([libraryDir UTF8String]));
        return qLibraryDir;
    }

    // Install the Escalation Services helper tool if:
    // 1) first-time install
    // 2) the version numbers don't match
    // 3) the certificates don't match
    bool installEscalationServiceHelperDaemon(char* errorMsg, size_t maxErrorLen)
    {
        char tmpErrorMsg[256];

        bool success = false;
        bool debugEnabled = false;
        if (!errorMsg || maxErrorLen <= 0)
        {
            // Just to simplify code/help with debugging
            errorMsg = tmpErrorMsg;
            maxErrorLen = sizeof(tmpErrorMsg);                    
        }
        
        errorMsg[0] = '\0';
        
        @try
        {
            // Check if we should log more than normal
            struct stat debugFileStat;
            static const char* OriginDebugFile = "/var/tmp/DebugES";
            debugEnabled = stat(OriginDebugFile, &debugFileStat) == 0;

            HelperToolState install = CheckHelperToolCurrentInstall(debugEnabled);
            if (install != HelperToolState_LATEST_VERSION)
            {        
                bool removeOlderVersion = install == HelperToolState_OLDER_VERSION;
                success = blessHelperWithLabel(HELPER_TOOL_IDENTIFIER, removeOlderVersion, debugEnabled, errorMsg, maxErrorLen);
            }
            
            else
                success = true;
        }
        @catch (NSException* ex)
        {
            snprintf(errorMsg, maxErrorLen, "Got exception %s while checking helper daemon:\n%s\n", [[ex name] cStringUsingEncoding:[NSString defaultCStringEncoding]]
                     , [[ex reason] cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        }
        
        if (success)
        {
            if (debugEnabled)
                syslog(LOG_NOTICE, "Helper tool ok");
        }
        
        else
            syslog(LOG_NOTICE, "Helper tool install failed - %s", errorMsg);
        
        return success;
    }
    
    /// Run an AppleScript scenario.
    bool runAppleScript(const char* script, char* errorMsg, size_t maxErrorLen)
    {
        if (!script)
            return false;

        // NSAppleScript is not thread-safe -> run the command on the main thread
        // Use a dictionary to send over code and extract the final result/error description
        NSString* code = [[NSString alloc] initWithCString:script encoding:[NSString defaultCStringEncoding]];
        NSMutableDictionary* params = [[NSMutableDictionary alloc] initWithCapacity:3];
        [params setObject:code forKey:ASCodeKey];

        // Trigger script
        PlatformAppleScript* scriptExe = [[PlatformAppleScript alloc] init];
        [scriptExe performSelectorOnMainThread:@selector(run:) withObject:params waitUntilDone:YES];
        [scriptExe release];
        
        // Check result data in dictionary
        NSNumber* result = (NSNumber*)[params objectForKey:ASReturnValue];
        BOOL retVal = [result boolValue];
        if (retVal == NO)
        {
            NSString* errorStr = (NSString*)[params objectForKey:ASErrorDescKey];
            if (errorMsg && maxErrorLen > 0)
            {
                if ([errorStr getCString:errorMsg maxLength:maxErrorLen encoding:[NSString defaultCStringEncoding]] == NO)
                {
                    errorMsg[0] = '\0';
                }
            }
        }
            
        [code release];
        [params release];
        
        return retVal == YES;
    }
    
    // Enables the System Preferences' Enable Assistive Devices setting.
    bool enableAssistiveDevices()
    {
        if (!AXAPIEnabled())
        {
            // Create a special file which contains a single 'a' character. That's very hacking, however the only
            // other way I know is to use an AppleScript - but we can't run an AppleScript from the Escalation Services
            // daemon (not to ask the user his credentials yet again)
            FILE* adFile = fopen("/private/var/db/.AccessibilityAPIEnabled", "wb");
            if (adFile)
            {
                fputc('a', adFile);
                fclose(adFile);
            }
        }
        
        // Call the same API again, in case it helps with "settling" our magic file configuration
        return AXAPIEnabled();
    }
    
    // Returns the certificate requirements to use when injected a dylib (IGO).
    bool getIGOCertificateRequirements(char* buffer, size_t bufferSize)
    {
        if (!buffer)
            return false;
        
        int length = snprintf(buffer, bufferSize, "%s", HELPER_TOOL_IGO_CERTIFICATE_PATTERN);
        return (length > 0 && size_t(length) < bufferSize);
    }
    
    // Reads the certificates from the local keychain.
    void readLocalCertificates(QList<QSslCertificate>* certs)
    {
        // NOTE: the initial reason for this was to load the Fiddler certificate to use Fiddler through the Mac proxy.
        // So for now, simply limit to that guy!
#ifdef DEBUG
        syslog(LOG_NOTICE, "Reading local certificates...");
#endif
        CFArrayRef cfCerts;
        
        OSStatus status = SecTrustSettingsCopyCertificates(kSecTrustSettingsDomainUser, &cfCerts);
        if (status != errSecNoTrustSettings)
        {
            CFIndex cnt = CFArrayGetCount(cfCerts);
#ifdef DEBUG
            syslog(LOG_NOTICE, "Found %ld certificates in Login keychain", cnt);
#endif
            for (CFIndex idx = 0; idx < cnt; ++idx)
            {
                bool useCert = false;
                SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, idx);
                
                CFStringRef info = SecCertificateCopySubjectSummary(cfCert);
                if (info)
                {
                    CFIndex length = CFStringGetLength(info);
                    CFIndex bufferSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
                    char* buffer = new char[bufferSize];
                    
                    Boolean displayInfo = CFStringGetCString(info, buffer, bufferSize, kCFStringEncodingUTF8);
                    if (displayInfo)
                    {
                        static const char* FiddlerCertificateName = "DO_NOT_TRUST_FiddlerRoot";
                        
                        if (strcmp(FiddlerCertificateName, buffer) == 0)
                            useCert = true;
#ifdef DEBUG
                        syslog(LOG_NOTICE, "    %02ld. %s", idx + 1, buffer);
#endif
                    }
                    
#ifdef DEBUG
                    else
                        syslog(LOG_NOTICE, "    %02ld. Unable to show certificate summary", idx + 1);
#endif
                    
                    delete []buffer;
                    CFRelease(info);
                }
      
#ifdef DEBUG
                else
                    syslog(LOG_NOTICE, "    %ld. Skipping invalid certificate", idx + 1);
#endif
                
                if (useCert)
                {
                    CFDataRef certData = SecCertificateCopyData(cfCert);
                    if (certData)
                    {
                        int len = CFDataGetLength(certData);
                        const char* rawData = reinterpret_cast<const char*>(CFDataGetBytePtr(certData));
                        QByteArray rawCert(rawData, len);
                        
                        certs->append(QSslCertificate::fromData(rawCert, QSsl::Der));
                        CFRelease(certData);
                    }
                }
            }
            
            CFRelease(cfCerts);
        }
    }

    // Deletes a specific folder.
    bool deleteDirectory(const char* dirName, char* errorMsg, size_t maxErrorLen)
    {
        bool success = false;
        
        @autoreleasepool
        {
            char tmpErrorMsg[256];
            if (!errorMsg || maxErrorLen <= 0)
            {
                // Just to simplify code/help with debugging
                errorMsg = tmpErrorMsg;
                maxErrorLen = sizeof(tmpErrorMsg);
            }
            
            errorMsg[0] = '\0';
            
            @try
            {
                NSError* error = nil;
                NSString* path = [NSString stringWithCString:dirName encoding:[NSString defaultCStringEncoding]];
                if ([[NSFileManager defaultManager ]removeItemAtPath:path error:&error] == YES)
                    success = true;
                
                else
                {
                    if (error)
                        [[error description] getCString:errorMsg maxLength:maxErrorLen encoding:[NSString defaultCStringEncoding]];
                    
                    else
                        snprintf(errorMsg, maxErrorLen, "Directory couldn't be deleted but no reason returned");
                }
                
            }
            @catch (NSException* ex)
            {
                snprintf(errorMsg, maxErrorLen, "Got exception %s:\n%s\n", [[ex name] cStringUsingEncoding:[NSString defaultCStringEncoding]]
                         , [[ex reason] cStringUsingEncoding:[NSString defaultCStringEncoding]]);
            }
        }
        
        return success;
    }

    bool restoreApplicationFromTrash(QString oldPath, QString newPath, QString appPath)
    {
        bool success = false;
        @try
        {
            NSFileManager* fm = [NSFileManager defaultManager];
            NSString* oldString = [NSString stringWithUTF8String:oldPath.toUtf8().constData()];
            NSString* newString = [NSString stringWithUTF8String:newPath.toUtf8().constData()];
            
            success = [fm moveItemAtPath:oldString toPath:newString error:nil];
        }
        @catch (NSException* ex)
        {
            return false;
        }
        
        if (success)
        {
           CFStringRef appString = CFStringCreateWithCString(kCFAllocatorDefault, appPath.toUtf8(), kCFStringEncodingUTF8);
            
            CFURLRef newUrl = CFURLCreateWithFileSystemPath(NULL, appString, kCFURLPOSIXPathStyle, true);
            LSRegisterURL(newUrl, true);
            
            CFRelease(appString);
            CFRelease(newUrl);
        }
        
        return success;
    }

    /// Logs a message to the system console
    void LogConsoleMessage(const char* format, ...)
    {
        va_list argp;
        va_start(argp, format);
        vsyslog(LOG_NOTICE, format, argp);
        va_end(argp);
    }

    // Setup a tap to watch the CTRL+Tab/CTRL+SHIFT+Tab combos to bypass the Mac Keyboard Interface Control
    // behavior for the key view loop.
    void* setupKICWatch(void* viewId, KICWatchListener* listener)
    {
        if (!viewId || !listener)
            return NULL;
        
        return [[KICWatcher alloc] initWithView:(NSView*)viewId AndListener:listener];
    }
    
    // Remove tap used to watch CTRL+Tab/CTRL+SHIFT+Tab combos.
    void cleanupKICWatch(void* tap)
    {
        if (tap)
        {
            KICWatcher* watcher = (KICWatcher*)tap;
            
            // Immediately cleanup the tap
            [watcher cleanup];
            [watcher release];
        }
    }

    // Make a widget window invisible.
    // winId - the identifier returned from the widget winId() method.
    // pushOnTop - whether the window should be pushed on top of all other windows.
    void MakeWidgetInvisible(int winId, bool pushOnTop)
    {
        if (!winId)
            return;
        
        NSView* view = (NSView*)winId;
        NSWindow* wnd = [view window];
        
        [wnd setIgnoresMouseEvents:YES];
        [wnd setBackgroundColor:[NSColor clearColor]];
        [wnd setOpaque:NO];
        [wnd setAlphaValue:0.0];
        
        if (pushOnTop)
            [wnd setLevel:NSPopUpMenuWindowLevel];
    }
    
    QString getRequiredOSVersionFor(QString const& bundlePath)
    {
        QString NO_VERSION;
        
        NSString* nsBundlePath = [NSString stringWithUTF8String:bundlePath.toUtf8().data()];
        if (!nsBundlePath)
            return NO_VERSION;
        
        ScopedCFRef<CFURLRef> bundleURL(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef) nsBundlePath, kCFURLPOSIXPathStyle, false));
        if (bundleURL.isNull())
            return NO_VERSION;
        
        // We use CFBundleCopyInfoDictionaryInDirectory instead of NSBundle to avoid the NSBundle caching
        // NSBundle* bundle = [NSBundle bundleWithPath:nsBundlePath];
        ScopedCFRef<CFDictionaryRef> bundleInfo(CFBundleCopyInfoDictionaryInDirectory(*bundleURL));
        if (bundleInfo.isNull())
            return NO_VERSION;
        
        NSString* minOS = (NSString*)[(NSDictionary*)*bundleInfo valueForKey:@"LSMinimumSystemVersion"];
        if (!minOS)
            return NO_VERSION;
        
        return QString::fromUtf8([minOS UTF8String]);
    }

    const QString UNKNOWN_VERSION("0.0.0");
    
    QString KernelToOSVersion(char const* kernel_version)
    {
        if (kernel_version == NULL || kernel_version[0] == 0)
            return UNKNOWN_VERSION;
        
        NSString* version = [NSString stringWithCString:kernel_version encoding:NSASCIIStringEncoding];
        if (!version)
            return UNKNOWN_VERSION;
        
        NSArray* components = [version componentsSeparatedByString:@"."];
        if (!components)
            return UNKNOWN_VERSION;
        
        int numComponents = [components count];
        int from_major = (numComponents > 0) ? [[components objectAtIndex:0] intValue] : 0;
        int from_minor = (numComponents > 1) ? [[components objectAtIndex:1] intValue] : 0;
        //int from_build = (numComponents > 2) ? [[components objectAtIndex:2] intValue] : 0;
        
        int to_major = 10;
        int to_minor = from_major - 4;
        int to_build = from_minor;
        
        return QString("%1.%2.%3").arg(to_major).arg(to_minor).arg(to_build);
    }

    QString getOSXVersion()
    {
        // try reading from SystemVersion.plist
        NSDictionary* versionDict = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
        if (versionDict)
        {
            NSString *versionString = [versionDict objectForKey:@"ProductVersion"];
            if (versionString)
            {
                QString version = QString::fromLatin1([versionString cStringUsingEncoding:NSASCIIStringEncoding]);
                if (version.length() > 0) // improve error checking here
                    return version;
            }
        }

        // fallback, try getting the kernel version from uname
        struct utsname name;
        int err = uname(&name);
        if (!err)
        {
            QString os_version = KernelToOSVersion(name.release);
            return os_version;
        }
        
        return UNKNOWN_VERSION;
    }
    
#if 0 // Keeping this code around for when I figure out to properly/entirely disable App Nap using the NSProcessInfo API

    // Notify the OS that the process is performing some user-triggered activity and should
    // allow the process to go to "sleep" / minimize its CPU/energy-consumption (App Nap)
    void* beginUserActivity(const char* reason)
    {
        if (reason)
        {
            // Make sure this is supported!
            Class nsProcessInfoClass = NSClassFromString(@"NSProcessInfo");
            if (nsProcessInfoClass)
            {
                NSProcessInfo* processInfo = [nsProcessInfoClass processInfo];
                if ([processInfo respondsToSelector:@selector(beginActivityWithOptions:reason:)] == YES)
                {
                    SEL selector = NSSelectorFromString(@"beginActivityWithOptions:reason:");
                    NSString* nsReason = [NSString stringWithCString:reason encoding:NSUTF8StringEncoding];
                    
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
                    NSActivityOptions options = NSActivityUserInitiatedAllowingIdleSystemSleep;
#else
                    // From 10100 SDK (Yosemite):
                    
                    // To include one of these individual flags in one of the sets, use bitwise or:
                    //   NSActivityUserInitiated | NSActivityIdleDisplaySleepDisabled
                    // (this may be used during a presentation, for example)
                    
                    // To exclude from one of the sets, use bitwise and with not:
                    //   NSActivityUserInitiated & ~NSActivitySuddenTerminationDisabled
                    // (this may be used during a user intiated action that may be safely terminated with no application interaction in case of logout)
                    
                    // Used for activities that require the screen to stay powered on.
                    const uint64_t NSActivityIdleDisplaySleepDisabled = (1ULL << 40);
                    
                    // Used for activities that require the computer to not idle sleep. This is included in NSActivityUserInitiated.
                    const uint64_t NSActivityIdleSystemSleepDisabled = (1ULL << 20);
                    
                    // Prevents sudden termination. This is included in NSActivityUserInitiated.
                    const uint64_t NSActivitySuddenTerminationDisabled = (1ULL << 14);
                    
                    // Prevents automatic termination. This is included in NSActivityUserInitiated.
                    const uint64_t NSActivityAutomaticTerminationDisabled = (1ULL << 15);
                    
                    // ----
                    // Sets of options.
                    
                    // App is performing a user-requested action.
                    const uint64_t NSActivityUserInitiated = (0x00FFFFFFULL | NSActivityIdleSystemSleepDisabled);
                    const uint64_t NSActivityUserInitiatedAllowingIdleSystemSleep = (NSActivityUserInitiated & ~NSActivityIdleSystemSleepDisabled);
                    
                    // App has initiated some kind of work, but not as the direct result of user request.
                    const uint64_t NSActivityBackground = 0x000000FFULL;
                    
                    // Used for activities that require the highest amount of timer and I/O precision available. Very few applications should need to use this constant.
                    const uint64_t NSActivityLatencyCritical = 0xFF00000000ULL;

                    uint64_t options = NSActivityUserInitiatedAllowingIdleSystemSleep;
#endif
                    
                    NSInvocation* nsActivityCall = [NSInvocation invocationWithMethodSignature:[processInfo methodSignatureForSelector:selector]];
                    [nsActivityCall setSelector:selector];
                    [nsActivityCall setTarget:processInfo];
                    [nsActivityCall setArgument:&options atIndex:2];
                    [nsActivityCall setArgument:&nsReason atIndex:3];
                    [nsActivityCall invoke];
                    
                    void* activityID = NULL;
                    [nsActivityCall getReturnValue:&activityID];
                    
                    LogConsoleMessage("BeginUserActivity: %s - %p", reason, activityID);
                    
                    return activityID;
                }
            }
        }
        
        return NULL;
    }
    
    /// \brief Notify the OS that the previously declared activity is over and the system can put the
    /// the process to "sleep" / minimize its CPU/energy-consumption (App Nap) if desired.
    void endUserActivity(void* activityID)
    {
        if (activityID)
        {
            // Still make sure this is supported!
            Class nsProcessInfoClass = NSClassFromString(@"NSProcessInfo");
            if (nsProcessInfoClass)
            {
                NSProcessInfo* processInfo = [nsProcessInfoClass processInfo];
                if ([processInfo respondsToSelector:@selector(endActivity:)] == YES)
                {
                    SEL selector = NSSelectorFromString(@"endActivity:");
                    [processInfo performSelector:selector withObject:(id)activityID];
                    LogConsoleMessage("EndUserActivity: %p", activityID);
                }
            }
        }
    }
    
#endif

} // PlatformService
} // Services
} // Origin
