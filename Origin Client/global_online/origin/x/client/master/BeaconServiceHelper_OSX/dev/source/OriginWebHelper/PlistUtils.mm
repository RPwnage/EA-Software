#include "PlistUtils.h"

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>

namespace Origin
{
namespace Platform
{
    // Returns 0 if successful; otherwise returns -1 (buffer not defined, buffer size too small, unable to retrieve home directory)
    int GetHomeDirectory(char* buffer, size_t bufferSize)
    {
        if (!buffer || bufferSize <= 0)
            return -1;
        
        for (int idx = 0; idx < 2; ++idx)
        {
            const  char* directory = NULL;
            if (idx == 0)
            {
                directory = getenv("HOME");
            }
            
            else
            {
                struct passwd* pw = getpwuid(getuid());
                directory = pw->pw_dir;
            }
            
            if (directory)
            {
                size_t len = strlen(directory);
                if (snprintf(buffer, bufferSize, "%s", directory) != (int)len)
                    return -1;
                
                struct stat dirStats;
                if (stat(buffer, &dirStats) == 0 && S_ISDIR(dirStats.st_mode))
                    return 0;
            }
        }
        
        return -1;
    }
    
    bool getExecutablePath(char* buffer, size_t size)
    {
        // Get path to this executable
        char fpath[2048];
        memset(fpath, 0, sizeof(fpath));
        uint32_t fpathSize = sizeof(fpath);
        int ret = _NSGetExecutablePath(fpath, &fpathSize);
        if (ret == 0)
        {
            memset(buffer, 0, size);
            
            // Get the real path (compress ./ and things like that)
            realpath(fpath, buffer);
            
            return true;
        }
        syslog(LOG_ERR, "WebHelper NSGetExecutablePath returned %d.\n", ret);
        return false;
    }
    
    bool getDestinationPlistPath(const char* helperToolName, char* buffer, size_t size)
    {
        char homepath[2048];
        memset(homepath, 0, sizeof(homepath));
        if (GetHomeDirectory(homepath, sizeof(homepath)) != 0)
        {
            syslog(LOG_ERR, "WebHelper Unable to get home directory\n");
            return false;
        }
        
        memset(buffer, 0, size);
        snprintf(buffer, size, "%s/Library/LaunchAgents/%s.plist", homepath, helperToolName);
        
        return true;
    }
    
    bool createLaunchAgentPlist(const char * helperToolName)
    {
        // Get path to this executable
        char fpath[2048];
        if (!getExecutablePath(fpath, sizeof(fpath)))
        {
            syslog(LOG_ERR, "WebHelper Unable to get executable path.\n");
            return false;
        }
        
        char destPlistPath[2048];
        if (!getDestinationPlistPath(helperToolName, destPlistPath, sizeof(destPlistPath)))
        {
            syslog(LOG_ERR, "WebHelper Unable to get destination plist path.\n");
            return false;
        }
        
        // Get a pointer to the embedded launchd.plist
        unsigned long sectsize = 0;
        void *sectptr = getsectiondata(&_mh_execute_header, "__TEXT", "__launchd_plist", &sectsize);
        if (sectptr && sectsize > 0)
        {
            syslog(LOG_NOTICE, "WebHelper got embedded launchd plist - %d bytes\n", (int)sectsize);
        }
        else
        {
            syslog(LOG_ERR, "WebHelper Unable to get embedded launchd plist\n");
            return false;
        }

        @autoreleasepool
        {
            NSError *e;
            NSData *plistData = [NSData dataWithBytesNoCopy:sectptr length:sectsize freeWhenDone:NO];
            NSPropertyListFormat format;
            NSMutableDictionary *launchdPlistContents = [NSPropertyListSerialization propertyListWithData:plistData options:NSPropertyListMutableContainersAndLeaves format:&format error:&e];
            
            if (!launchdPlistContents)
            {
                NSLog(@"WebHelper Unable to parse embedded launchd plist: %@", e);
                syslog(LOG_ERR, "WebHelper Unable to parse embedded launchd plist\n");
                return false;
            }
            
            NSString *binPath = [NSString stringWithCString:fpath encoding:NSUTF8StringEncoding];
            [launchdPlistContents setValue:binPath forKey:@"Program"];
            //launchdPlistContents[@"Program"] = binPath;  // Older XCode complains :(
            
            NSLog(@"WebHelper tool bin path: %@", binPath);
            
            NSData *newPlistData = [NSPropertyListSerialization dataWithPropertyList:launchdPlistContents format:NSPropertyListXMLFormat_v1_0 options:0 error:&e];
            
            if (!newPlistData)
            {
                NSLog(@"WebHelper Unable to serialize launchd plist: %@", e);
                syslog(LOG_ERR, "WebHelper Unable to serialize launchd plist\n");
                return false;
            }
            
            NSInteger newPlistLen = [newPlistData length];
            char* newPlistRawData = new char[(int)newPlistLen];
            memset(newPlistRawData, 0, (int)newPlistLen);
            
            [newPlistData getBytes:(void*)newPlistRawData length:(NSUInteger)newPlistLen];
            
            FILE *destPlistFile = fopen(destPlistPath,"wb");
            if (!destPlistFile)
            {
                syslog(LOG_ERR, "WebHelper Unable to open destination file for launchd plist: %s\n", destPlistPath);
                delete [] newPlistRawData;
                return false;
            }
            
            NSString *nsDestPlistPath = [NSString stringWithCString:destPlistPath encoding:NSUTF8StringEncoding];
            
            size_t written = fwrite(newPlistRawData, sizeof(char), (size_t)newPlistLen, destPlistFile);
            
            delete [] newPlistRawData;
            fclose(destPlistFile);
            
            bool success = false;
            if (written == (size_t)newPlistLen)
            {
                success = true;
                syslog(LOG_NOTICE, "WebHelper wrote plist data to: %s\n", destPlistPath);
                NSLog(@"WebHelper wrote plist data to: %@", nsDestPlistPath);
                
                chmod(destPlistPath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            }
            else
            {
                syslog(LOG_ERR, "WebHelper Unable to write launchd plist\n");
            }
            
            return success;
        }
    }
    
    bool loadOrUnloadLaunchAgent(const char* helperToolName, bool load)
    {
        char destPlistPath[2048];
        if (!getDestinationPlistPath(helperToolName, destPlistPath, sizeof(destPlistPath)))
        {
            syslog(LOG_ERR, "WebHelper Unable to get destination plist path.\n");
            return false;
        }
        
        @autoreleasepool
        {
            NSString *plistPath = [NSString stringWithCString:destPlistPath encoding:NSUTF8StringEncoding];
            //NSString *helperName = [NSString stringWithCString:helperToolName encoding:NSUTF8StringEncoding];
            
            NSTask *task;
            task = [[NSTask alloc] init];
            [task setLaunchPath:@"/bin/launchctl"];
        
            NSArray *arguments;
            if (load)
            {
                arguments = [NSArray arrayWithObjects:@"load", plistPath, nil];
            }
            else
            {
                arguments = [NSArray arrayWithObjects:@"unload", plistPath, nil];
            }
            [task setArguments:arguments];
            
            //NSPipe *pipe;
            //pipe = [NSPipe pipe];
            //[task setStandardOutput:pipe];
            
            NSLog(@"Launching %@ with args: %@",[task launchPath], arguments);
            
            [task launch];
            [task waitUntilExit];
            
            [task autorelease];
            
            return true;
        }
    }
    
    bool removeLaunchAgent(const char* helperToolName)
    {
        @autoreleasepool
        {
            NSString *helperName = [NSString stringWithCString:helperToolName encoding:NSUTF8StringEncoding];
            
            NSTask *task;
            task = [[NSTask alloc] init];
            [task setLaunchPath:@"/bin/launchctl"];
            
            NSArray *arguments;
            arguments = [NSArray arrayWithObjects:@"remove", helperName, nil];
            [task setArguments:arguments];
            
            //NSPipe *pipe;
            //pipe = [NSPipe pipe];
            //[task setStandardOutput:pipe];
            
            NSLog(@"Launching %@ with args: %@",[task launchPath], arguments);
            
            [task launch];
            [task waitUntilExit];
            
            [task autorelease];
            
            return true;
        }
    }
    
    bool doesLaunchAgentPlistExist(const char* helperToolName)
    {
        char destPlistPath[2048];
        if (!getDestinationPlistPath(helperToolName, destPlistPath, sizeof(destPlistPath)))
        {
            syslog(LOG_ERR, "WebHelper Unable to get destination plist path.\n");
            return false;
        }
        
        @autoreleasepool
        {
            NSString *plistPath = [NSString stringWithCString:destPlistPath encoding:NSUTF8StringEncoding];
            
            NSFileManager* fileManager = [NSFileManager defaultManager];
            BOOL alreadyExists = [fileManager fileExistsAtPath:plistPath];
            if (alreadyExists)
            {
                return true;
            }
        }
        
        return false;
    }
    
    bool removeLaunchAgentPlist(const char* helperToolName)
    {
        char destPlistPath[2048];
        if (!getDestinationPlistPath(helperToolName, destPlistPath, sizeof(destPlistPath)))
        {
            syslog(LOG_ERR, "WebHelper Unable to get destination plist path.\n");
            return false;
        }
        
        @autoreleasepool
        {
            NSString *plistPath = [NSString stringWithCString:destPlistPath encoding:NSUTF8StringEncoding];
            
            NSFileManager* fileManager = [NSFileManager defaultManager];
            BOOL alreadyExists = [fileManager fileExistsAtPath:plistPath];
            if (alreadyExists)
            {
                if ([fileManager removeItemAtPath:plistPath error:nil])
                {
                    NSLog(@"Removed launch agent plist: %@", plistPath);
                }
            }
        }
        
        return true;
    }
    
    bool installLaunchAgent(const char* helperToolName)
    {
        NSLog(@"Installing WebHelper agent...");
        
        // Unload any existing one first
        if (!uninstallLaunchAgent(helperToolName))
            return false;
        
        if (!createLaunchAgentPlist(helperToolName))
            return false;
        
        if (!loadOrUnloadLaunchAgent(helperToolName, true))
            return false;
        
        return true;
    }
    
    bool uninstallLaunchAgent(const char* helperToolName)
    {
        if (doesLaunchAgentPlistExist(helperToolName))
        {
            NSLog(@"Uninstalling existing WebHelper agent...");
        
            // Unload from launchd
            if (!loadOrUnloadLaunchAgent(helperToolName, false))
                return false;
        
            // Remove the plist
            if (!removeLaunchAgentPlist(helperToolName))
                return false;
        }
        else
        {
            if (!removeLaunchAgent(helperToolName))
                return false;
        }
        return true;
    }
}
}