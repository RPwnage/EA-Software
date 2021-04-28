//
//  OriginESHelper.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/17/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <dlfcn.h>
#include <syslog.h>
#include <launch.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <version/version.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>


#import <Security/Security.h>
#import <Security/SecStaticCode.h>
#import <Security/SecRequirement.h>
#include <ApplicationServices/ApplicationServices.h>

#include "PlistUtils.h"


#define SHOW_LOGGING

#ifdef DEBUG
//#define REPORT_ERROR_AND_TRY_AGAIN
#endif

typedef int (*MainFcn)(int argc, char* argv[]);

// Name of this helper tool
static const char* WebHelperToolName = "com.ea.origin.WebHelper";

// Relative path (to this tool inside the main Origin bundle) to the library that actually implements the web helper functionality
static const char* WebHelperImplLibRelativePathT = "/../../../MacOS/OriginWebHelperImpl.dylib";

// Name of the file to check when deciding whether to log more info
static const char* OriginDebugFile = "/var/tmp/DebugWebHelper";

// Set of requirements to check against library to load
#ifdef ORIGIN_MAC_OFFICIAL_CERT
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.OriginWebHelperImpl and certificate leaf[subject.CN] = \"Developer ID Application: Electronic Arts Inc.\"";
#else
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.OriginWebHelperImpl and certificate leaf[subject.CN] = \"OriginDev\"";
#endif

// The socket id defined in the helper launchd plist to enable a connection with the Origin client
static const char* WebHelperSocketID = "com.ea.origin.WebHelper.sock";
static const char* WebHelperSSLSocketID = "com.ea.origin.WebHelper.securesock";

static const int RespawnThrottleDelay = 10; // 10 seconds is default

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Simple helper class
class LockFile
{
    public:
        LockFile(const char* name)
            : _handle(NULL)
        {
            if (name)
            {
                _handle = fopen(name, "rb");
            }
        }
        
        ~LockFile()
        {
            Release();
        }
    
        bool IsValid() const
        { 
            return _handle != NULL; 
        }
    
        void Release()
        {
            if (_handle)
                fclose(_handle);
        }
    
    private:
        FILE* _handle;
};

// Check requirements against a file
bool FulfillsRequirements(const char* fileName)
{
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "Checking requirements for library '%s'\n", fileName);
#endif
    
    CFURLRef libUrl = CFURLCreateFromFileSystemRepresentation(NULL, reinterpret_cast<const UInt8*>(fileName), (CFIndex)strlen(fileName), false);
    CFStringRef requirementsStr = CFStringCreateWithCString(NULL, DYNAMIC_LIB_REQUIREMENTS, kCFStringEncodingMacRoman);
    
    bool isValid = false;
    
    // Create specific requirement we are looking for - we do not consider invalid expired certificates
    SecRequirementRef requirements;
    OSStatus stErr = SecRequirementCreateWithString(requirementsStr, kSecCSDefaultFlags, &requirements);        
    if (stErr == errSecSuccess)
    {    
        // Extract the current code object for our library 
        SecStaticCodeRef staticCodeRef;
        stErr = SecStaticCodeCreateWithPath(libUrl, kSecCSDefaultFlags, &staticCodeRef); 
        if (stErr == errSecSuccess)
        {
            // Check the library fulfills all requirements
            stErr = SecStaticCodeCheckValidity(staticCodeRef, kSecCSDefaultFlags, requirements);
            if (stErr == errSecSuccess)
            {
                isValid = true;
#ifdef SHOW_LOGGING
                syslog(LOG_NOTICE, "Library certificate requirements verified\n");
#endif
            }
            
            else
            {
#ifdef SHOW_LOGGING
                syslog(LOG_NOTICE, "The library didn't pass the requirements (%ld)\n", stErr);
#endif
            }
            
            CFRelease(staticCodeRef);
        }
        
        else
        {
#ifdef SHOW_LOGGING
            syslog(LOG_NOTICE, "Unable to access code object for '%s' (%ld)\n", fileName, stErr);
#endif
        }
        
        CFRelease(requirements);
    }
    
    else
    {
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "Unable to create certificate requirements to check the helper library against (%ld)\n", stErr);
#endif
    }
    
    CFRelease(requirementsStr);
    CFRelease(libUrl);    
    
    return isValid;
}

void AcceptAndCloseIfPending(launch_data_t socket)
{
    // K, so look up the open socket and close it so that the deamon stops retrying over and over
    if (socket)
    {
        if (launch_data_get_type(socket) == LAUNCH_DATA_ARRAY && launch_data_array_get_count(socket) == 1)
        {
            launch_data_t socketDef = launch_data_array_get_index(socket, 0);
            if (socketDef && launch_data_get_type(socketDef) == LAUNCH_DATA_FD)
            {
                int socketID = launch_data_get_fd(socketDef);
                if (socketID >= 0)
                {
                    //syslog(LOG_NOTICE, "WebHelper - Error Handler; Got launchd socket: %d\n", socketID);
                    
                    fd_set rfds;
                    struct timeval tv;
                    int retval;
                    
                    FD_ZERO(&rfds);
                    FD_SET(socketID, &rfds);
                    
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                    
                    retval = select(socketID+1, &rfds, NULL, NULL, &tv);
                    
                    //syslog(LOG_NOTICE, "WebHelper - Error Handler; select returned: %d\n", retval);
                    
                    // -1 or 0 we can ignore
                    if (retval == -1 || !retval)
                        return;
                    
                    syslog(LOG_NOTICE, "WebHelper - Error Handler; Socket has pending connection: %d, accepting and closing.\n", socketID);
                    
                    // We need to accept and close
                    struct sockaddr_in dest;
                    socklen_t destLen = sizeof(struct sockaddr_in);
                    
                    int newConnectionID = ::accept(socketID, (sockaddr*)&dest, &destLen);
                    close(newConnectionID);
                }
            }
        }
    }
}

int ReportError(const char* format, const char* param, int errorCode)
{
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, format, param);
#endif
    
#ifdef REPORT_ERROR_AND_TRY_AGAIN
    return errorCode;
#else
    if (errorCode)
    {
        syslog(LOG_NOTICE, "WebHelper - Error Handler; Checking in with launchd\n");
        // Lookup the list of sockets the daemon supports 
        launch_data_t socket = NULL;
        launch_data_t socket_SSL = NULL;
        launch_data_t req = launch_data_new_string(LAUNCH_KEY_CHECKIN);
        if (req)
        {
            launch_data_t resp = launch_msg(req);
            if (resp && launch_data_get_type(resp) == LAUNCH_DATA_DICTIONARY)
            {
                launch_data_t sockets = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_SOCKETS);
                if (sockets && launch_data_get_type(sockets) == LAUNCH_DATA_DICTIONARY)
                {
                    socket = launch_data_dict_lookup(sockets, WebHelperSocketID);
                    socket_SSL = launch_data_dict_lookup(sockets, WebHelperSSLSocketID);
                }
            }
        }
        
        AcceptAndCloseIfPending(socket);
        AcceptAndCloseIfPending(socket_SSL);
    }
    
    // Wait for the throttle interval
    syslog(LOG_NOTICE, "WebHelper - Error Handler; Waiting %d seconds to avoid respawn throttle delay.\n", RespawnThrottleDelay);
    sleep(RespawnThrottleDelay);
    
    syslog(LOG_NOTICE, "WebHelper - Error Handler; Exiting with code: %d\n", errorCode);
    return errorCode;
#endif
}


// Get a path to the helper dylib
char* LookupHelperDylibPath()
{
    char* path = NULL;

    char fpath[2048];
    memset(fpath, 0, sizeof(fpath));
    char rpath[2048];
    memset(rpath, 0, sizeof(rpath));
    uint32_t fpathSize = sizeof(fpath);
    if (_NSGetExecutablePath(fpath, &fpathSize) == 0)
    {
        syslog(LOG_NOTICE, "WebHelper executable path: %s\n", fpath);
        
        strncat(fpath, WebHelperImplLibRelativePathT, strlen(WebHelperImplLibRelativePathT));
        
        realpath(fpath, rpath);
        
        syslog(LOG_NOTICE, "WebHelper dylib path: %s\n", rpath);
        
        uint32_t finalPathSize = strlen(rpath);
        if (finalPathSize > 0 && finalPathSize < sizeof(rpath)-1)
        {
            path = new char[finalPathSize+1];
            memset(path, 0, finalPathSize+1);
            strncpy(path, rpath, finalPathSize);
        }
    }
    
    return path;
}

void getEmbeddedLaunchdPlist()
{
    unsigned long sectsize = 0;
    void *sectptr = getsectiondata(&_mh_execute_header, "__TEXT", "__launchd_plist", &sectsize);
    if (sectptr && sectsize > 0)
    {
        syslog(LOG_NOTICE, "WebHelper got embedded launchd plist - %d bytes\n", (int)sectsize);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (strcasecmp(argv[i], "install") == 0)
            {
                syslog(LOG_NOTICE, "Starting WebHelper V.%s - Install mode....\n", ESCALATIONSERVICE_VERSION_P_DELIMITED);
                Origin::Platform::installLaunchAgent(WebHelperToolName);
                return 0;
            }
            else if (strcasecmp(argv[i], "uninstall") == 0)
            {
                syslog(LOG_NOTICE, "Starting WebHelper V.%s - Uninstall mode....\n", ESCALATIONSERVICE_VERSION_P_DELIMITED);
                Origin::Platform::uninstallLaunchAgent(WebHelperToolName);
                return 0;
            }
        }
    }
    
    syslog(LOG_NOTICE, "Starting WebHelper V.%s - Looking up Client bundle....\n", ESCALATIONSERVICE_VERSION_P_DELIMITED);

    // Check if we should log more than normal
    struct stat debugFileStat;
    bool debugEnabled = stat(OriginDebugFile, &debugFileStat) == 0;

    if (debugEnabled)
    {
        syslog(LOG_NOTICE, "WebHelper Debug mode enabled");
    }

    
    // Attempt to get the implementation dylib path
    char* implPath = LookupHelperDylibPath();
    
    // If we are still null return the error.
    if(!implPath)
    {
        return ReportError("%s", "WebHelper - Unable to locate implementation dylib...\n", -1);
    }
    
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "WebHelper - Found implementation dylib at: %s\n", implPath);
#endif
    
    // Check the signature of the dylib - make sure to open the file first to create "a virtual lock" 
    // so that nobody can replace it in between validation/loading. "Virtual lock" = somebody can still replace the file, but the process seems
    // to cache the previous inode of the open file and reuse it for the dlopen
    LockFile lock(implPath);
    if (!lock.IsValid())
    {
        int errorCode = ReportError("Unable to open file '%s'\n", implPath, -2);
        delete []implPath;
        return errorCode;
    }
    
    if (!FulfillsRequirements(implPath))
    {
        int errorCode = ReportError("File '%s' doesn't fulfill the security requirements\n", implPath, -3);
        delete []implPath;
        return errorCode;
    }
    
    // K, time to load the library and start the main process
    void* module = dlopen(implPath, RTLD_NOW);
    if (!module)
    {
        delete []implPath;
        return ReportError("Unable to load WebHelper implementation (%s)\n", dlerror(), -4);
    }
    
    // Don't need to lock lib anymore
    lock.Release();
    
    // Don't need path either
    delete []implPath;
    
    int retVal = 0;
    MainFcn implMain = (MainFcn)dlsym(module, "main");
    if (implMain)
    {
        retVal = (*implMain)(argc, argv);
    }

    dlclose(module);

    if (!implMain)
    {
        return ReportError("%s", "Unable to find WebHelper implementation entry point\n", -5);
    }
    
    if (retVal)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "Internal error (%d) while running server\n", retVal);
        
        return ReportError("%s", tmp, 0);
    }
    
    syslog(LOG_NOTICE, "Stopping WebHelper\n");
    
    return 0;
}


