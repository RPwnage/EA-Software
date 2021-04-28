#include "stdafx.h"
#include "OriginSDK/Origin.h"

#ifndef ORIGIN_PC

#include "OriginSDK/OriginTypes.h"

#include "../OriginSDKimpl.h"
#include "../OriginErrorFunctions.h"
#include "../OriginSDKMemory.h"

#include "OriginSDKmac.h"

#include "OriginSDK/OriginSDK.h"

#include <ApplicationServices/ApplicationServices.h>

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <mach-o/dyld.h>

namespace Origin
{

#ifndef DEBUG
    // Name of the file to check when deciding whether to log more info
    static const char* OriginDebugFile = "/var/tmp/DebugES";
#endif
        
    // Name of the main Origin executable
    static const char* OriginBundleEXE = "/Origin";
    
    // Used to check agianst a dev build not using Bootstrap
    static const char* OriginClientBundleEXE = "/OriginClient";
    
    // Relative path (inside the main Origin bundle) to the actual OIG implementation
    static const char* OIGLoaderLibRelativePath = "/IGOLoader.dylib";
    static const char* OIGLoaderLibRelativePathFromBundle = "/Contents/MacOS/IGOLoader.dylib";
    
    // How many times to try looking up the Origin client executable
    static const int MaxOriginClientLookupTries = 3;
    
    // How long do we want to wait between each try
    static const int OriginClientLookupDelayInSeconds = 2;
    
    // How big a buffer do we need when requesting a process filename + arguments?
    static const int DefaultMaxArgsBufferSize = 1024 * 1024 / 4;
    
    
    
    
    // Allow more debugging logs in Mac console
    bool IsDebugLoggingEnabled()
    {
        static bool initialized = false;
        static bool debugEnabled = false;
        if (!initialized)
        {
            initialized = true;
#ifdef DEBUG
            debugEnabled = true;
#else
            struct stat debugFileStat;
            debugEnabled = stat(OriginDebugFile, &debugFileStat) == 0;
#endif
            if (debugEnabled)
                syslog(LOG_NOTICE, "SDK:Debug logging enabled");
        }
        
        return debugEnabled;
    }
    
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
            // Call sysctl with a NULL buffer.
            length = 0;
            err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
            if (err == -1)
                err = errno;
            
            // Allocate an appropriately sized buffer based on the results
            // from the previous call.
            if (err == 0)
            {
                result = static_cast<kinfo_proc*>(malloc(length));
                if (result == NULL)
                    err = ENOMEM;
            }
            
            // Call sysctl again with the new buffer.  If we get an ENOMEM
            // error, toss away our buffer and start again.
            if (err == 0)
            {
                err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1, result, &length, NULL, 0);
                if (err == -1)
                    err = errno;
            
                if (err == 0)
                    done = true;
                
                else
                if (err == ENOMEM)
                {
                    free(result);
                    result = NULL;
                    err = 0;
                }
            }
        } while (err == 0 && !done);
        
        // Clean up and establish post conditions.
        if (err != 0 && result != NULL)
        {
            free(result);
            result = NULL;
        }
        
        *procList = result;
        if (err == 0)
            *procCount = length / sizeof(kinfo_proc);
        
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
    static char* GlobalArgsBuffer = NULL;
    static size_t GlobalArgsBufferSize = 0;
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
                            if (IsDebugLoggingEnabled())
                                syslog(LOG_NOTICE, "SDK: Found bundle '%s' : '%s'", bundleName, fileName);
                            
                            // That looks good -> time to replace the bundle name with our dylib name
                            size_t dylibSuffixLen = strlen(OIGLoaderLibRelativePath);
                            size_t finalLen = fullNameLen + (dylibSuffixLen - bundleNameLen) + 1;
                            
                            path = new char[finalLen];
                            strncpy(path, fileName, fullNameLen - bundleNameLen);
                            path[fullNameLen - bundleNameLen] = '\0';
                            strncat(path, OIGLoaderLibRelativePath, dylibSuffixLen);
                            
                            break;
                        }
                    }
                }
            }
            
            free(procList);
        }
        
        else
        {
            if (IsDebugLoggingEnabled())
                syslog(LOG_NOTICE, "SDK:Error while trying to get list of processes (%d)", errorCode);
        }
        
        return path;
    }
    
    // Check whether IGO was already injected (we actually check the loader: maybe that guy decides not to continue and inject
    // the actual IGO code for some reason)
    bool IsIGOLoaded()
    {
        size_t igoModuleNameLen = strlen(OIGLoaderLibRelativePath);
        
        uint32_t imageCnt = _dyld_image_count();
        for (uint32_t cnt = 0; cnt < imageCnt; ++cnt)
        {
            const char* imageName = _dyld_get_image_name(cnt);
            if (imageName)
            {
                size_t imageLen = strlen(imageName);
                if (imageLen > igoModuleNameLen)
                {
                    // Here we get a full path -> check the end of it
                    const char* imageSuffix = imageName + (imageLen - igoModuleNameLen);
                    if (strncmp(imageSuffix, OIGLoaderLibRelativePath, igoModuleNameLen) == 0)
                    {
                        if (IsDebugLoggingEnabled())
                            syslog(LOG_NOTICE, "SDK:IGO already loaded '%s'", imageName);
                        
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // These functions aren't really needed, so it's safe to return
    OriginErrorT OriginSDK::StartOrigin(const OriginStartupInputT& input)
    {
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:Starting Origin...");
        
        FSRef appRef;
        OSStatus status = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("com.ea.origin"), NULL, &appRef, NULL);
        if (status == noErr)
        {
            LSApplicationParameters params = {0, kLSLaunchDefaults, &appRef, NULL, NULL, NULL};
            status = LSOpenApplication(&params, NULL);
            if (status == noErr)
            {
                return REPORTERROR(ORIGIN_SUCCESS);
            }
            else
            {
                syslog(LOG_NOTICE, "SDK:Failed to start Origin client (0x%08x)", (int)status);
                return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
            }
        }
        
        else
        {
            syslog(LOG_NOTICE, "SDK:Unable to find registered Origin client (0x%08x)", (int)status);
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED); // maybe should be ORIGIN_ERROR_CORE_NOT_INSTALLED, but not sure whether pre-existing game code is dependent on the return value
        }
    }

    OriginErrorT OriginSDK::StartIGO(bool loadDebugVersion)
    {
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:Starting IGO...");
        
        // When running the unit test we do not want the SDK to start the IGO
        if (m_Flags & ORIGIN_FLAG_UNIT_TEST_MODE)
            return REPORTERROR(ORIGIN_SUCCESS);
        
        // Is the IGO dylib already loaded?
        if (IsIGOLoaded())
            return REPORTERROR(ORIGIN_SUCCESS);
        
        // Search for any running instance of Origin
        GlobalArgsBufferSize = GetMaxArgsBufferSize();
        if (GlobalArgsBufferSize == 0)
        {
            GlobalArgsBufferSize = DefaultMaxArgsBufferSize;
            syslog(LOG_NOTICE, "SDK:UNABLE TO QUERY MAX ARGS BUFFER SIZE - USING DEFAULT OF %d bytes", DefaultMaxArgsBufferSize);
        }
        
        GlobalArgsBuffer = new char[GlobalArgsBufferSize];
        
        // Attempt to lookup main Origin bundle a few times
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:IGOLoader not present - searching for Origin process");
        
        char* implPath = NULL;
        for (int i = 0; i < MaxOriginClientLookupTries; ++i)
        {
            implPath = LookupOriginBundlePath(OriginBundleEXE);
            if (!implPath)
            {
                // This check should only happen during a devlopment build that is not using Bootstrap.
                implPath = LookupOriginBundlePath(OriginClientBundleEXE);
                if (!implPath)
                    sleep(OriginClientLookupDelayInSeconds);
                
                else
                    break;
            }
            else
                break;
        }
        
        delete []GlobalArgsBuffer;
        GlobalArgsBuffer = NULL;

        if (!implPath)
        {
            // Ok, couldn't find a running instance, default to any registered client
            if (IsDebugLoggingEnabled())
                syslog(LOG_NOTICE, "SDK:Failed to find running instance of Origin - using any registered client");
            
            CFURLRef appURL = NULL;
            OSStatus status = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("com.ea.origin"), NULL, NULL, &appURL);
            if (status == noErr && appURL)
            {
                char appPath[1024];
                Boolean appPathAvailable = CFURLGetFileSystemRepresentation(appURL, true, reinterpret_cast<UInt8*>(appPath), sizeof(appPath));
                CFRelease(appURL);
                
                if (appPathAvailable)
                {
                    // That looks good -> time to replace the bundle name with our dylib name
                    size_t dylibSuffixLen = strlen(OIGLoaderLibRelativePathFromBundle);
                    size_t finalLen = strlen(appPath) + dylibSuffixLen + 1;
                    
                    implPath = new char[finalLen];
                    snprintf(implPath, finalLen, "%s%s", appPath, OIGLoaderLibRelativePathFromBundle);
                }
            }
            
            else
            {
                if (IsDebugLoggingEnabled())
                    syslog(LOG_NOTICE, "SDK:Failed to find a registered client (0x%08x)", (int)status);
            }
        }
        
        // At this point hopefully we do have a location to use!
        if (implPath)
        {
            if (IsDebugLoggingEnabled())
                syslog(LOG_NOTICE, "SDK:Explicitly loading '%s'", implPath);
                
            void* module = dlopen(implPath, RTLD_NOW | RTLD_LOCAL);
            if (!module)
                syslog(LOG_NOTICE, "SDK:Failed to load IGO loader '%s'", implPath);

            delete []implPath;
            
            if (module)
                return REPORTERROR(ORIGIN_SUCCESS);
        }
        
        else
            syslog(LOG_NOTICE, "SDK:Couldn't determine path to IGO");
        
        return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
    }

    bool OriginSDK::IsOriginInstalled()
    {
        bool installed = false;
        CFURLRef appURL = NULL;
        OSStatus result = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("com.ea.origin"), NULL, NULL, &appURL);
        
        if (appURL)
            CFRelease(appURL);
        
        switch (result)
        {
            case noErr:
                installed = true;
                break;
            case kLSApplicationNotFoundErr:
            default:
                installed = false;
                break;
        }
        
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:IsOriginInstalled=%d", installed);
        
        return installed;
    }

    unsigned int FindProcessOrigin()
    {
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:Looking for Origin client running instance...");
        
        size_t procCount = 0;
        kinfo_proc* procList = NULL;
        
        char fileName[2048]; // please tell me this is big enough! :)
        
        size_t bundleName1Len = strlen(OriginBundleEXE);
        size_t bundleName2Len = strlen(OriginClientBundleEXE);

        unsigned int pid = 0;
        
        GlobalArgsBufferSize = GetMaxArgsBufferSize();
        if (GlobalArgsBufferSize == 0)
        {
            GlobalArgsBufferSize = DefaultMaxArgsBufferSize;
            syslog(LOG_NOTICE, "SDK:UNABLE TO QUERY MAX ARGS BUFFER SIZE - USING DEFAULT OF %d bytes", DefaultMaxArgsBufferSize);
        }
        
        GlobalArgsBuffer = new char[GlobalArgsBufferSize];

        int errorCode = GetBSDProcessList(&procList, &procCount);
        if (!errorCode)
        {
            for (size_t idx = 0; idx < procCount; ++idx)
            {
                if (GetFullName(procList[idx].kp_proc.p_pid, fileName, sizeof(fileName)))
                {
                    size_t fullNameLen = strlen(fileName);
                    if (fullNameLen > bundleName1Len)
                    {
                        // Is it the correct suffix?
                        const char* fileNameSuffix = fileName + (fullNameLen - bundleName1Len);
                        if (strncmp(fileNameSuffix, OriginBundleEXE, bundleName1Len) == 0)
                        {
                            pid = procList[idx].kp_proc.p_pid;
                            break;
                        }
                    }
                    
                    if (fullNameLen > bundleName2Len)
                    {
                        // Is it the correct suffix?
                        const char* fileNameSuffix = fileName + (fullNameLen - bundleName2Len);
                        if (strncmp(fileNameSuffix, OriginClientBundleEXE, bundleName2Len) == 0)
                        {
                            pid = procList[idx].kp_proc.p_pid;
                            break;
                        }
                    }
                }
            }
            
            free(procList);
        }
        
        delete []GlobalArgsBuffer;
        GlobalArgsBuffer = NULL;
        
        if (IsDebugLoggingEnabled())
            syslog(LOG_NOTICE, "SDK:Origin client pid=%d", pid);
        
        return pid;
    }
    
} // Origin

#endif
