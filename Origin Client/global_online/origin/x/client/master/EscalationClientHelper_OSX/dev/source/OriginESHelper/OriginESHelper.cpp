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
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <version/version.h>


#import <Security/Security.h>
#import <Security/SecStaticCode.h>
#import <Security/SecRequirement.h>
#include <ApplicationServices/ApplicationServices.h>


#define SHOW_LOGGING

#ifdef DEBUG
//#define REPORT_ERROR_AND_TRY_AGAIN
#endif

typedef int (*MainFcn)(int argc, char* argv[]);


// Name of the main Origin executable
static const char* OriginBundleEXE = "/Origin";

// Used to check agianst a dev build not using Bootstrap
static const char* OriginClientBundleEXE = "/OriginClient";

// Relative path (inside the main Origin bundle) to the library that actually implements the escalation services functionality
static const char* EscalationServicesImplLibRelativePath = "/OriginESImpl.dylib";

// Name of the file to check when deciding whether to log more info
static const char* OriginDebugFile = "/var/tmp/DebugES";

// How many times to try looking up the Origin client executable
static const int MaxOriginClientLookupTries = 5;

// How long do we want to wait between each try
static const int OriginClientLookupDelayInSeconds = 2;

// How big a buffer do we need when requesting a process filename + arguments?
static const int DefaultMaxArgsBufferSize = 1024 * 1024 / 4;

// Set of requirements to check against library to load
#ifdef ORIGIN_MAC_OFFICIAL_CERT
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.OriginESImpl and certificate leaf[subject.CN] = \"Developer ID Application: Electronic Arts Inc.\"";
#else
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.OriginESImpl and certificate leaf[subject.CN] = \"OriginDev\"";
#endif

// The socket id defined in the helper launchd plist to enable a connection with the Origin client
static const char* EscalationServicesHelperSocketID = "com.ea.origin.ESHelper.sock";

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// We tried to use GetNextProcess but found issues when immediately starting Origin after logging in
// We haven't found an explanation for it yet, but let's try a different way (and hopefully more stable)
// way to collect the list of processes

// From http://developer.apple.com/legacy/mac/library/#qa/qa2001/qa1123.html
typedef struct kinfo_proc kinfo_proc;

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
// Returns a list of all BSD processes on the system.  This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount.  You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
{
    int                 err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;
    
    //assert( procList != NULL);
    //assert(*procList == NULL);
    //assert(procCount != NULL);
    
    *procCount = 0;
    
    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.
    
    result = NULL;
    done = false;
    do {
        //assert(result == NULL);
        
        // Call sysctl with a NULL buffer.
        
        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                     NULL, &length,
                     NULL, 0);
        if (err == -1) {
            err = errno;
        }
        
        // Allocate an appropriately sized buffer based on the results
        // from the previous call.
        
        if (err == 0) {
            result = static_cast<kinfo_proc*>(malloc(length));
            if (result == NULL) {
                err = ENOMEM;
            }
        }
        
        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.
        
        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                         result, &length,
                         NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                //assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);
    
    // Clean up and establish post conditions.
    
    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }
    *procList = result;
    if (err == 0) {
        *procCount = length / sizeof(kinfo_proc);
    }
    
    //assert( (err == 0) == (*procList != NULL) );
    
    return err;
}

// Gets the maximum buffer size to use when requesting program name + args at the lowest level
// Just because I don't want to keep on reallocating that guy.
size_t GetMaxArgsBufferSize()
{
    int mib[2] = { CTL_KERN, KERN_ARGMAX };
    int argMax;
    size_t size = sizeof(argMax);
    
    if (sysctl(mib, 2, &argMax, &size, NULL, 0) == 0)
        return argMax;
    
    return 0;
}

// Extract the module name for a specific pid
char* GlobalArgsBuffer = NULL;
size_t GlobalArgsBufferSize = 0;
bool GetFullName(int pid, char* buffer, size_t bufferSize)
{
    bool success = false;
    
    if (!GlobalArgsBuffer || !buffer || bufferSize == 0)
        return success;

    // Look up how much space all the arguments may take
    int mib[3];
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;
    
    size_t size = GlobalArgsBufferSize;
    if (sysctl(mib, 3, GlobalArgsBuffer, &size, NULL, 0) == 0)
    {
        char* fullPath = GlobalArgsBuffer + sizeof(int);
        const char* fpStart = strchr(fullPath, '/');
        if (fpStart && *fpStart)
        {
            size_t len = ::strlen(fpStart);
            if (len < bufferSize)
            {
                strcpy(buffer, fpStart);
                success = true;
            }
        }
    }
    
    return success;
    
}

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
        // Lookup the list of sockets the daemon supports 
        launch_data_t socket = NULL;
        launch_data_t req = launch_data_new_string(LAUNCH_KEY_CHECKIN);
        if (req)
        {
            launch_data_t resp = launch_msg(req);
            if (resp && launch_data_get_type(resp) == LAUNCH_DATA_DICTIONARY)
            {
                launch_data_t sockets = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_SOCKETS);
                if (sockets && launch_data_get_type(sockets) == LAUNCH_DATA_DICTIONARY)
                {
                    socket = launch_data_dict_lookup(sockets, EscalationServicesHelperSocketID);
                }
            }
        } 
        
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
                        struct sockaddr_un name;
                        socklen_t nameLen = SUN_LEN(&name);
                        
                        int newConnectionID = ::accept(socketID, (sockaddr*)&name, &nameLen);                        
                        close(newConnectionID);
                    }
                }
            }
        }
    }
    
    return 0;
#endif
}

// Log the list of processes we find from our GetBSDProcessList
void LogSearchableProcesses()
{
    size_t procCount = 0;
    kinfo_proc* procList = NULL;
    
    int errorCode = GetBSDProcessList(&procList, &procCount);
    if (!errorCode)
    {
        syslog(LOG_NOTICE, "Found %ld processes:", procCount);
        for (size_t idx = 0; idx < procCount; ++idx)
        {
			char fileName[2048]; // please tell me this is big enough! :)
            if (GetFullName(procList[idx].kp_proc.p_pid, fileName, sizeof(fileName)))
                syslog(LOG_NOTICE, "%ld. '%s' (pid=%d)", idx + 1, fileName, procList[idx].kp_proc.p_pid);
            
            else
                syslog(LOG_NOTICE, "%ld. Unable to access filename for pid=%d", idx + 1, procList[idx].kp_proc.p_pid);
        }
        
        free(procList);
    }
    
    else
        syslog(LOG_NOTICE, "Error while trying to get list of processes (%d)", errorCode);
}


// Search the running processes for the Origin client and extract its executable path and use it to create the path to the dylib that implements
// the functionality of the daemon
char* LookupOriginBundlePath(const char* bundleName)
{
    char* path = NULL;    
    size_t procCount = 0;
    kinfo_proc* procList = NULL;
    
    char fileName[2048]; // please tell me this is big enough! :)

    size_t bundleNameLen = strlen(bundleName);
    
    int errorCode = GetBSDProcessList(&procList, &procCount);
    if (!errorCode)
    {
        for (size_t idx = 0; idx < procCount; ++idx)
        {
            if (GetFullName(procList[idx].kp_proc.p_pid, fileName, sizeof(fileName)))
            {
                size_t fullNameLen = strlen(fileName);
                if (fullNameLen > bundleNameLen)
                {
                    // Is it the correct suffix?
                    const char* fileNameSuffix = fileName + (fullNameLen - bundleNameLen);
                    if (strncmp(fileNameSuffix, bundleName, bundleNameLen) == 0)
                    {
                        // That looks good -> time to replace the bundle name with our dylib name
                        size_t dylibSuffixLen = strlen(EscalationServicesImplLibRelativePath);
                        size_t finalLen = fullNameLen + (dylibSuffixLen - bundleNameLen) + 1;

                        path = new char[finalLen];
                        strncpy(path, fileName, fullNameLen - bundleNameLen);
                        path[fullNameLen - bundleNameLen] = '\0';
                        strncat(path, EscalationServicesImplLibRelativePath, dylibSuffixLen);
                        
                        break;
                    }
                }
            }
        }
        
        free(procList);
    }
    
    else
        syslog(LOG_NOTICE, "Error while trying to get list of processes (%d)", errorCode);
    
    
    return path;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    syslog(LOG_NOTICE, "Starting ES V.%s - Looking up Client bundle....\n", ESCALATIONSERVICE_VERSION_P_DELIMITED);

    // The low-level API we use require quite a large buffer - just don't want to keep on re-allocating that guy,
    // so allocate it once here
    GlobalArgsBufferSize = GetMaxArgsBufferSize();
    if (GlobalArgsBufferSize == 0)
    {
        GlobalArgsBufferSize = DefaultMaxArgsBufferSize;
        syslog(LOG_NOTICE, "UNABLE TO QUERY MAX ARGS BUFFER SIZE - USING DEFAULT OF %d bytes", DefaultMaxArgsBufferSize);
    }
    
    GlobalArgsBuffer = new char[GlobalArgsBufferSize];

    // Check if we should log more than normal
    struct stat debugFileStat;
    bool debugEnabled = stat(OriginDebugFile, &debugFileStat) == 0;

    if (debugEnabled)
    {
        syslog(LOG_NOTICE, "ES Debug mode enabled");
        LogSearchableProcesses();
    }

    
    // Attempt to lookup main Origin bundle a few times
    char* implPath = NULL;    
    for (int i = 0; i < MaxOriginClientLookupTries; ++i)
    {
        implPath = LookupOriginBundlePath(OriginBundleEXE);
        if (!implPath)
        {
            // This check should only happen during a devlopment build that is not using Bootstrap.
            implPath = LookupOriginBundlePath(OriginClientBundleEXE);
            if (!implPath)
            {
                // Log is only generated if file exists
                if (debugEnabled)
                    LogSearchableProcesses();

                sleep(OriginClientLookupDelayInSeconds);
            }
            else
                break;
        }
        else
            break;
    }
    
    delete []GlobalArgsBuffer;
    
    // If we are still null return the error.
    if(!implPath)
    {
        return ReportError("%s", "Unable to locate Origin client bundle...\n", -1);
    }
    
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "Found client bundle at: %s\n", implPath);
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
        return ReportError("Unable to load ESHelper implementation (%s)\n", dlerror(), -4);
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
        return ReportError("%s", "Unable to find ESHelper implementation entry point\n", -5);
    }
    
    if (retVal)
    {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "Internal error (%d) while running server\n", retVal);
        
        return ReportError("%s", tmp, 0);
    }
    
    syslog(LOG_NOTICE, "Stopping ES\n");
    
    return 0;
}


