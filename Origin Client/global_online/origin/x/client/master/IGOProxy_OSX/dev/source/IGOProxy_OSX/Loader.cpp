//
//  Loader.cpp
//  IGOProxy_OSX
//
//  Created by Frederic Meraud on 3/27/13.
//  Copyright (c) 2013 Frederic Meraud. All rights reserved.
//

#include "Loader.h"
#include <Carbon/Carbon.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <version/version.h>
#include <new>

#include "MacMach_override.h"
#include "IGOLogger.h"

#import <Security/Security.h>
#import <Security/SecStaticCode.h>
#import <Security/SecRequirement.h>
#include <ApplicationServices/ApplicationServices.h>




// Enable full logging when debugging
static bool FulLoggingEnabled = false;

// Name of the main Origin executable
static const char* OriginBundleEXE = "/Origin";

// Used to check agianst a dev build not using Bootstrap
static const char* OriginClientBundleEXE = "/OriginClient";

// Relative path (inside the main Origin bundle) to the actual OIG implementation
static const char* OIGLibRelativePath = "/IGO.dylib";

// How many times to try looking up the Origin client executable
static const int MaxOriginClientLookupTries = 5;

// How long do we want to wait between each try
static const int OriginClientLookupDelayInSeconds = 2;

// How big a buffer do we need when requesting a process filename + arguments?
static const int DefaultMaxArgsBufferSize = 1024 * 1024 / 4;

// Set of requirements to check against library to load
#ifdef ORIGIN_MAC_OFFICIAL_CERT
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.IGO and certificate leaf[subject.CN] = \"Developer ID Application: Electronic Arts Inc.\"";
#else
static const char* DYNAMIC_LIB_REQUIREMENTS = "identifier com.ea.origin.IGO and certificate leaf[subject.CN] = \"OriginDev\"";
#endif

// Keyword used to automatically load libs
static const char* DYLD_INSERT_LIBRARIES_ENV_KEY = "DYLD_INSERT_LIBRARIES";

// Keyword used to force full logging for loader
static const char* OIG_LOADER_LOGGING_ENV_KEY = "OIG_LOADER_LOGGING";

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// FOR DEBUGGING ONLY = OVERRIDE PTRACE

#define DEFINE_HOOK(result, name, args) \
typedef result (*name ## Func) args; \
name ## Func name ## Next = NULL; \
result name args

bool HookAPI(const char* apiName, void* pCallback, void** pNextHook)
{
    if (!apiName || !pCallback)
        return false;
    
    // we don't want to rehook
    if (pNextHook && *pNextHook)
        return false;
    
    void* fcn = dlsym(RTLD_DEFAULT, apiName);
    
    if (!fcn)
    {
        OriginIGO::IGOLogWarn("Unable to find '%s'", apiName);
        return false;
    }
    
    Dl_info info;
    if (dladdr(fcn, &info))
    {
        if (info.dli_fname)
        {
            OriginIGO::IGOLogInfo("Symbol '%s' picked from library '%s'", apiName, info.dli_fname);
        }
        
        else
        {
            OriginIGO::IGOLogInfo("Unable to grab image name for symbol '%s'", apiName);
        }
    }
    
    else
        OriginIGO::IGOLogInfo("Unable to find image containing the symbol '%s'", apiName);

    
    mach_error_t err = mach_override_ptr(fcn, pCallback, pNextHook);
    if (err)
        OriginIGO::IGOLogWarn("Failed to process '%s' (0x%08x)", apiName, err);
    
    return err == 0;
}

#ifdef DEBUG

DEFINE_HOOK(int, ptraceHook, (int request, pid_t pid, caddr_t addr, int data))
{
    OriginIGO::IGOLogInfo("Got ptrace %d/%d", request, pid);
    
    if (request == PT_DENY_ATTACH)
    {
        OriginIGO::IGOLogInfo("Bypass DENY_ATTACH");
        return 0;
    }

    return ptraceHookNext(request, pid, addr, data);
}

#endif

// Get module filename
bool GetModuleFileName(void* addr, char* buffer, size_t bufferSize)
{
    bool success = false;
    
    if (!buffer || bufferSize == 0)
        return success;
    
    Dl_info info;
    if (dladdr(addr, &info))
    {
        if (info.dli_fname)
        {
            size_t len = ::strlen(info.dli_fname);
            if (len < bufferSize)
            {
                strcpy(buffer, info.dli_fname);
                success = true;
            }
            
            else
                OriginIGO::IGOLogInfo("Buffer too small to store module filename (%ld/%ld)", len, bufferSize);
        }
        
        else
            OriginIGO::IGOLogInfo("No module name information");
    }
    
    else
        OriginIGO::IGOLogWarn( "Unable to locate module that contains addr=%p (%s)", addr, dlerror());
    
    return success;
}

// Set/Get the location of the OIG loader
static char IGOLoaderPath[1024] = {0};

extern "C" const char* GetIGOLoaderPath(); // Easier to lookup

__attribute__((visibility("default")))
const char* GetIGOLoaderPath()
{
    if (IGOLoaderPath[0])
        return IGOLoaderPath;
    
    GetModuleFileName((void*)GetIGOLoaderPath, IGOLoaderPath, sizeof(IGOLoaderPath));

    return IGOLoaderPath;
}

// Dump content of an environment array
void DumpEnvArray(char** env)
{
    if (env)
    {
        int idx = 0;
        while (*env)
        {
            OriginIGO::IGOLogInfo("    %d. %s", idx + 1, *env);
            ++env;
            ++idx;
        }
    }
}

// Our own little string duplicate method
char* StringDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dest = new char[len];
    snprintf(dest, len * sizeof(char), "%s", src);
    
    return dest;
}

// Insert our environment variable to force loading of this module
char** UpdateEnvironment(char *const envp[__restrict])
{
    // Make sure we have access to this module's path
    char injectEnv[1024];
    const char* loaderPath = GetIGOLoaderPath();
    if (loaderPath)
    {
        int s = snprintf(injectEnv, sizeof(injectEnv), "%s=%s", DYLD_INSERT_LIBRARIES_ENV_KEY, loaderPath);
        if (s <= 0 || s >= sizeof(injectEnv))
            loaderPath = NULL;
    }
    
    if (!loaderPath)
    {
        OriginIGO::IGOLogWarn("Unable to compute IGOLoader path env var...");
        return (char**)envp;
    }
    
    char** newEnv = NULL;
    if (envp)
    {
        OriginIGO::IGOLogInfo(" - Initial envs:");
        DumpEnvArray((char**)envp);
        
        int cnt = 0;
        bool foundEntry = false;
        
        newEnv = (char**)envp;
        while (*newEnv)
        {
            if (strstr(*newEnv, DYLD_INSERT_LIBRARIES_ENV_KEY))
            {
                foundEntry = true;
                if (strstr(*newEnv, injectEnv))
                {
                    OriginIGO::IGOLogInfo("Loader already found in path");
                    return (char**)envp;
                }
            }
            
            ++newEnv;
            ++cnt;
        }
        
        ++cnt; // for NULL
        if (!foundEntry)
            ++cnt; // for DYLD entry
        
        int idx = 0;
        newEnv = new char*[cnt];
        while (*envp)
        {
            if (strstr(*envp, DYLD_INSERT_LIBRARIES_ENV_KEY))
            {
#ifdef OVERRIDE_STEAM
                newEnv[idx] = StringDup(injectEnv);
#else
                const char* value = strchr(*envp, '=');
                if (value)
                {
                    ++value;
                    while (*value && isspace(*value))
                        ++value;
                    
                    size_t len = strlen(injectEnv) + 1 + strlen(value) + 1;
                    char* entry = new char[len];
                    snprintf(entry, len * sizeof(char), "%s:%s", injectEnv, value);
                    newEnv[idx] = entry;
                }
                
                else
                    newEnv[idx] = StringDup(injectEnv);
#endif
            }
            
            else
                newEnv[idx] = StringDup(*envp);
            
            ++idx;
            ++envp;
        }
        
        if (!foundEntry)
        {
            newEnv[idx] = StringDup(injectEnv);
            ++idx;
        }
        
        newEnv[idx] = NULL;
    }
    
    else
    {
        newEnv = new char*[1];
        newEnv[0] = StringDup(injectEnv);
    }
    
    OriginIGO::IGOLogInfo(" - Actual envs:");
    DumpEnvArray(newEnv);
    
    return newEnv;
}

DEFINE_HOOK(int, posix_spawnHook, (pid_t * __restrict pid, const char * __restrict path,
                                   const posix_spawn_file_actions_t *file_actions,
                                   const posix_spawnattr_t * __restrict attrp,
                                   char *const __argv[ __restrict],
                                   char *const __envp[ __restrict]))
{
    OriginIGO::IGOLogInfo("Calling posix_spawn (%d)...", getpid());
    
    char** newEnv = UpdateEnvironment(__envp);

    int retVal = posix_spawnHookNext(pid, path, file_actions, attrp, __argv, newEnv);
    
    if (newEnv != __envp)
    {
        int idx = 0;
        while (newEnv[idx])
        {
            delete[] newEnv[idx];
            ++idx;
        }
        
        delete[] newEnv;
    }
    
    return retVal;
}

DEFINE_HOOK(int, posix_spawnpHook, (pid_t * __restrict pid, const char * __restrict path,
                                    const posix_spawn_file_actions_t *file_actions,
                                    const posix_spawnattr_t * __restrict attrp,
                                    char *const __argv[ __restrict],
                                    char *const __envp[ __restrict]))
{
    OriginIGO::IGOLogInfo("Calling posix_spawnp (%d)...", getpid());

    char** newEnv = UpdateEnvironment(__envp);
    
    int retVal = posix_spawnpHookNext(pid, path, file_actions, attrp, __argv, newEnv);
    
    if (newEnv != __envp)
    {
        int idx = 0;
        while (newEnv[idx])
        {
            delete[] newEnv[idx];
            ++idx;
        }
        
        delete[] newEnv;
    }
    
    return retVal;
}

// Hook to change environment when started from Steam
DEFINE_HOOK(int, execveHook, (const char* path, char *const argv[], char *const envp[]))
{
    OriginIGO::IGOLogInfo("Calling execve (%d)...", getpid());
    
    char** newEnv = UpdateEnvironment(envp);
    
    return execveHookNext(path, argv, newEnv);
}

// Hook to change environment when started from Marvel Heroes/bad games that require root privileges (don't like it!)
DEFINE_HOOK(OSStatus, authorizationExecuteWithPrivilegesHook, (AuthorizationRef authorization, const char* pathToTool, AuthorizationFlags options, char* const* arguments, FILE** communicationsPipe))
{
    OriginIGO::IGOLogInfo("Calling AuthorizationExecuteWithPrivileges '%s', %ld (%d)...", pathToTool, (unsigned long)options, getpid());
    if (arguments)
    {
        int idx = 0;
        char* const* args = arguments;
        while (*args)
        {
            OriginIGO::IGOLogInfo("%d. %s", idx, *args);
            ++args;
            ++idx;
        }
    }
    
    else
        OriginIGO::IGOLogInfo("No arguments");

    int argCnt = 0;
    if (arguments)
    {
        char* const* args = arguments;
        while (*args)
        {
            ++args;
            ++argCnt;
        }
    }
    
    char** newArgs = NULL;
    if (strcmp(pathToTool, "/bin/sh") == 0)
    {
        if (arguments && arguments[0] && arguments[1] && strcmp(arguments[0], "-c") == 0)
        {
            const char* loaderPath = GetIGOLoaderPath();
            if (loaderPath)
            {
                newArgs = new char*[argCnt + 1];
                newArgs[argCnt] = '\0';
                
                for (int idx = 0; idx < argCnt; ++idx)
                    newArgs[idx] = arguments[idx];
                
                size_t len = strlen(DYLD_INSERT_LIBRARIES_ENV_KEY) + strlen(loaderPath) + strlen(arguments[1]) + 10;
                newArgs[1] = new char[len];

                int s = snprintf(newArgs[1], len * sizeof(char), "export %s=%s;%s", DYLD_INSERT_LIBRARIES_ENV_KEY, loaderPath, arguments[1]);
                if (s == len - 1)
                {
                    OriginIGO::IGOLogInfo("New command for AuthorizationExecuteWithPrivileges: %s", newArgs[1]);
                }
                
                else
                {
                    OriginIGO::IGOLogWarn("Failed to setup AuthorizationExecuteWithPrivileges");
                    delete [](newArgs[1]);
                    delete []newArgs;
                    newArgs = NULL;
                }
            }
        }
    }
    
    OSStatus result = authorizationExecuteWithPrivilegesHookNext(authorization, pathToTool, options, newArgs? newArgs : arguments, communicationsPipe);
        
    if (newArgs)
    {
        delete [](newArgs[1]);
        delete []newArgs;
    }
        
    return result;
}

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
    OriginIGO::IGOLogInfo("Checking requirements for library '%s'\n", fileName);
    
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
                OriginIGO::IGOLogInfo("Library certificate requirements verified\n");
            }
            
            else
                OriginIGO::IGOLogWarn("The library didn't pass the requirements (%ld)\n", static_cast<long>(stErr));
            
            CFRelease(staticCodeRef);
        }
        
        else
            OriginIGO::IGOLogWarn("Unable to access code object for '%s' (%ld)\n", fileName, static_cast<long>(stErr));
        
        CFRelease(requirements);
    }
    
    else
        OriginIGO::IGOLogWarn("Unable to create certificate requirements to check the helper library against (%ld)\n", static_cast<long>(stErr));
    
    CFRelease(requirementsStr);
    CFRelease(libUrl);
    
    return isValid;
}

// Log the list of processes we find from our GetBSDProcessList
void LogSearchableProcesses()
{
    size_t procCount = 0;
    kinfo_proc* procList = NULL;
       
    int errorCode = GetBSDProcessList(&procList, &procCount);
    if (!errorCode)
    {
        OriginIGO::IGOLogInfo("Found %ld processes:", procCount);
        
        for (size_t idx = 0; idx < procCount; ++idx)
        {
			char fileName[2048]; // please tell me this is big enough! :)
            if (GetFullName(procList[idx].kp_proc.p_pid, fileName, sizeof(fileName)))
                OriginIGO::IGOLogInfo("%ld. '%s' (pid=%d)", idx + 1, fileName, procList[idx].kp_proc.p_pid);
            
            //else
            //    OriginIGO::IGOLogInfo("%ld. Unable to access filename for pid=%d", idx + 1, procList[idx].kp_proc.p_pid);
        }
        
        free(procList);
    }
    
    else
        OriginIGO::IGOLogWarn("Error while trying to get list of processes (%d)", errorCode);
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
                        size_t dylibSuffixLen = strlen(OIGLibRelativePath);
                        size_t finalLen = fullNameLen + (dylibSuffixLen - bundleNameLen) + 1;
                        
                        path = new char[finalLen];
                        strncpy(path, fileName, fullNameLen - bundleNameLen);
                        path[fullNameLen - bundleNameLen] = '\0';
                        strncat(path, OIGLibRelativePath, dylibSuffixLen);
                        
                        break;
                    }
                }
            }
        }
        
        free(procList);
    }
    
    else
        OriginIGO::IGOLogWarn("Error while trying to get list of processes (%d)", errorCode);
    
    return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pascal
void LoaderEventLoopTimerEntry(EventLoopTimerRef inTimer, void* params)
{
    OriginIGO::IGOLogInfo("Loading module...");
    
    // The low-level API we use require quite a large buffer - just don't want to keep on re-allocating that guy,
    // so allocate it once here
    GlobalArgsBufferSize = GetMaxArgsBufferSize();
    if (GlobalArgsBufferSize == 0)
    {
        GlobalArgsBufferSize = DefaultMaxArgsBufferSize;
        OriginIGO::IGOLogInfo("UNABLE TO QUERY MAX ARGS BUFFER SIZE - USING DEFAULT OF %d bytes", DefaultMaxArgsBufferSize);
    }
    
    GlobalArgsBuffer = new char[GlobalArgsBufferSize];
    
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
                if (FulLoggingEnabled)
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

    if (implPath)
    {
        OriginIGO::IGOLogInfo("Module found: '%s'", implPath);
        
        // Check the signature of the dylib - make sure to open the file first to create "a virtual lock"
        // so that nobody can replace it in between validation/loading. "Virtual lock" = somebody can still replace the file, but the process seems
        // to cache the previous inode of the open file and reuse it for the dlopen
        LockFile lock(implPath);
        if (!lock.IsValid())
        {
            OriginIGO::IGOLogWarn("Unable to open file '%s'\n", implPath);
            
            delete []implPath;
        }

        else
        {
            if (!FulfillsRequirements(implPath))
            {
                OriginIGO::IGOLogWarn("File '%s' doesn't fulfill the security requirements\n", implPath);
                delete []implPath;
            }
            
            else
            {
                // K, time to load the library and get things going
                void* module = dlopen(implPath, RTLD_NOW | RTLD_LOCAL);
                if (!module)
                {
                    delete []implPath;
                    OriginIGO::IGOLogWarn("Unable to load ESHelper implementation (%s)\n", dlerror());
                }                
            }
            
            // Don't need to lock lib anymore
            lock.Release();
            
            // Don't need path either
            delete []implPath;
        }
    }
    
    else
        OriginIGO::IGOLogWarn( "Unable to locate Origin client bundle...\n");
}

void* LoaderThread(void* params)
{
    OriginIGO::IGOLogInfo("Loader thread started...");
 
    sleep(5);
    
    OriginIGO::IGOLogInfo("Dispatching load cmd...");

    EventLoopTimerProcPtr proc = (EventLoopTimerProcPtr)LoaderEventLoopTimerEntry;
    EventLoopTimerUPP upp = NewEventLoopTimerUPP(proc);
    
    if (GetMainEventLoop)
    {
        EventLoopRef evtLoop = GetMainEventLoop();
        if (!evtLoop)
            OriginIGO::IGOLogWarn("NOT main event loop");
    }
    
    else
        OriginIGO::IGOLogWarn("NOT GETMainEventLoop()");
    
    OriginIGO::IGOLogInfo("OK, main event loop seems fine");
    
    InstallEventLoopTimer(GetMainEventLoop(), 0, 0, upp, params, NULL);
    return NULL;
}

// Check whether we are running from a valid location
bool HasValidRootFolder(const char* exeFullName, uint32_t bufferSize)
{
    static const char* SupportedPrefixes[] = { "/Applications/", "/Users/", "/System/Library/Java/JavaVirtualMachines/" };
    size_t cnt = sizeof (SupportedPrefixes) / sizeof(const char*);
    size_t idx = 0;
    for (; idx < cnt; ++idx)
    {
        if (strncmp(SupportedPrefixes[idx], exeFullName, strlen(SupportedPrefixes[idx])) == 0)
            break;
    }
    
    if (idx == cnt)
    {
        // K, this could also be an app on a separate volume
        static const char* SeparateVolumePrefix = "/Volumes/";
        if (strncmp(SeparateVolumePrefix, exeFullName, strlen(SeparateVolumePrefix)) == 0)
        {
            // Skip the volumes name...
            const char* localPathName = strchr(exeFullName + strlen(SeparateVolumePrefix), '/');
            if (localPathName)
            {
                idx = 0;
                for (; idx < cnt; ++idx)
                {
                    if (strncmp(SupportedPrefixes[idx], localPathName, strlen(SupportedPrefixes[idx])) == 0)
                        break;
                }
            }
        }
        
        if (idx == cnt)
        {
            OriginIGO::IGOLogWarn("Invalid root location - No injection for such a process");
            return false;
        }
    }
    
    return true;
}

// Check whether this EXE is blacklisted (for example don't inject into browsers)
bool IsBlacklisted(const char* exeFullName, uint32_t bufferSize)
{
    static const char* BlackListedBundles[] = { "com.apple.Safari", "com.google.Chrome", "org.mozilla.firefox", "com.operasoftware.Opera" };
    size_t cnt = sizeof(BlackListedBundles) / sizeof(const char*);
    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle)
    {
        CFStringRef bundleIDRef = CFBundleGetIdentifier(mainBundle);
        
        if (bundleIDRef)
        {
			char bundleID[512];
            if (CFStringGetCString(bundleIDRef, bundleID, sizeof(bundleID), kCFStringEncodingUTF8))
            {
                OriginIGO::IGOLogInfo("Checking bundleID: %s", bundleID);
                
                for (size_t idx = 0; idx < cnt; ++idx)
                {
                    if (strncmp(BlackListedBundles[idx], bundleID, strlen(BlackListedBundles[idx])) == 0)
                    {
                        OriginIGO::IGOLogInfo("Blacklisted bundleID '%s'", BlackListedBundles[idx]);
                        return true;
                    }
                }
            }
        }
        
        else
            OriginIGO::IGOLogWarn("Unable to access EXE main bundle identifier");
    }
    
    else
        OriginIGO::IGOLogWarn("Unable to access EXE main bundle info");
    
    return false;
}

// Reset the environment to stop injection from here on
void ResetInjectionEnvironment()
{
    OriginIGO::IGOLogInfo("Resetting injection environment...");
    
    static const char* DYLD_INSERT_LIBRARIES_ENV_KEY = "DYLD_INSERT_LIBRARIES";
    char* currValue = getenv(DYLD_INSERT_LIBRARIES_ENV_KEY);
    if (currValue)
    {
        size_t maxSize = strlen(currValue) + 1;
        char* buffer = new char[maxSize];
        
        strcpy(buffer, currValue);
        
        OriginIGO::IGOLogInfo("Before %s = %s", DYLD_INSERT_LIBRARIES_ENV_KEY, currValue);
        char moduleName[512];
        if (GetModuleFileName((void*)ResetInjectionEnvironment, moduleName, sizeof(moduleName)))
        {
            char* fileNamePtr = NULL;
            while ((fileNamePtr = strstr(buffer, moduleName)))
            {
                // Look for the beginning of the path
                fileNamePtr[0] = '\0';
                char* beginPtr = strrchr(buffer, ':');
                
                // Check out whether we have other values in there we should keep around
                char* afterPtr = strchr(fileNamePtr + 1, ':');
                if (afterPtr)
                {
                    if (!beginPtr)
                        memmove(buffer, afterPtr + 1, strlen(afterPtr + 1) + 1);
                    
                    else
                        memmove(beginPtr, afterPtr, strlen(afterPtr) + 1);
                }
                
                else
                {
                    if (!beginPtr)
                        buffer[0] = '\0';
                    
                    else
                        beginPtr[0] = '\0';
                }
            }
            
            OriginIGO::IGOLogInfo("After %s = %s", DYLD_INSERT_LIBRARIES_ENV_KEY, buffer);
            setenv(DYLD_INSERT_LIBRARIES_ENV_KEY, buffer, 1);
        }
        
        delete[] buffer;
    }
}

extern "C" void OIGSetupLoader(); // Easier to lookup

static OriginIGO::IGOLogger* logger = NULL;

static void MacLogInfo(const char* file, int line, int alwaysLogMessage, const char* fmt, va_list arglist)
{
    OriginIGO::IGOLogger::instance()->infoWithArgs(file, line, alwaysLogMessage ? true : false, fmt, arglist);
}

static void MacLogError(const char* file, int line, const char* fmt, va_list arglist)
{
    OriginIGO::IGOLogger::instance()->warnWithArgs(file, line, true, fmt, arglist);
}

__attribute__((constructor))
__attribute__((visibility("default")))
void OIGSetupLoader()
{
    // Check if we should log more than normal
    FulLoggingEnabled = getenv(OIG_LOADER_LOGGING_ENV_KEY) != NULL;
#ifdef DEBUG
    FulLoggingEnabled = true;
#endif

    // Always enable logging for injection/cleanup - do not use the c++ allocator for this, as it may conflict with the
    // game builds (think The Sims4 DebugRelease build)
    //logger = new OriginIGO::IGOLogger("IGOProxy"); // we'll let that one leak
    void* mem = malloc(sizeof(OriginIGO::IGOLogger));
    logger = new (mem) OriginIGO::IGOLogger("IGOProxy");
    
    OriginIGO::IGOLogger::setInstance(logger);
    OriginIGO::IGOLogger::instance()->enableLogging(FulLoggingEnabled);
    
    mach_setup_logging(MacLogInfo, MacLogError);
    
    // Start logger in async mode
    OriginIGO::IGOLogger::instance()->startAsyncMode();
    
    OriginIGO::IGOLogInfo("Starting loader V.%s\n", ESCALATIONSERVICE_VERSION_P_DELIMITED);

    
    char exeFullName[512];
    uint32_t bufferSize = sizeof(exeFullName);
    if (_NSGetExecutablePath(exeFullName, &bufferSize) == 0)
    {
        OriginIGO::IGOLogInfo("Exe name:'%s'", exeFullName);
        
        // Is this EXE coming from a valid location?
        bool isValid = HasValidRootFolder(exeFullName, bufferSize);
        
        // Make sure we skip specific software (for example browsers)
        if (isValid && IsBlacklisted(exeFullName, bufferSize))
        {
            isValid = false;
            
            // Don't allow injection from any blacklisted software
            ResetInjectionEnvironment();
        }
        
        if (!isValid)
            return;
        
        OriginIGO::IGOLogInfo("Ok to inject in this process");
    }

    else
    {
        OriginIGO::IGOLogWarn("Unable to access executable name");
        return;
    }
    
    // FOR DEBUGGING ONLY
#ifdef DEBUG
    OriginIGO::IGOLogInfo("Disabling ptrace");
    HookAPI("ptrace", (void*)ptraceHook, (void**)&ptraceHookNext);
#endif
    
    // Immediately hook up core process spawning APIs - no need to delay like we
    // do for the other Carbon/Cocoa APIs, so that we don't fail to inject into the actual
    // game if there's a "fast" launcher the user can bypass easily
    OriginIGO::IGOLogInfo("Hooking process spawning");
 
    HookAPI("posix_spawn", (void*)posix_spawnHook, (void**)&posix_spawnHookNext);
    HookAPI("posix_spawnp", (void*)posix_spawnpHook, (void**)&posix_spawnpHookNext);
    HookAPI("execve", (void*)execveHook, (void**)&execveHookNext);
    HookAPI("AuthorizationExecuteWithPrivileges", (void*)authorizationExecuteWithPrivilegesHook, (void**)&authorizationExecuteWithPrivilegesHookNext);
    
    // Create a separate thread to load our OIG after n seconds
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    int policy;
    pthread_attr_getschedpolicy(&attr, &policy);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    
    struct sched_param sched;
    sched.sched_priority = sched_get_priority_max(policy);
    pthread_attr_setschedparam(&attr, &sched);
    
    void* params = NULL;
    
    pthread_t thread;
    pthread_create(&thread, &attr, LoaderThread, (void*) params);
    pthread_attr_destroy(&attr);
}

