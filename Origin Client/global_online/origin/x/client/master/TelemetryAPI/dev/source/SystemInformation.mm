// Copyright (c) 2013 Electronic Arts. All rights reserved.

#include "SystemInformation.h"
#import <Foundation/Foundation.h>
#include <eastl/string.h>
#include <string>
#include <sstream>
#include <sys/sysctl.h>
#include <sys/utsname.h>

// local prototypes
namespace
{
    const std::string UNKNOWN_VERSION("0.0.0");
    
    std::string KernelToOSVersion(char const* kernel_version)
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
        
        std::stringstream osx_version;
        osx_version << to_major << "." << to_minor << "." << to_build;
        return osx_version.str();
    }
}

namespace NonQTOriginServices
{
    std::string SystemInformation::GetOSXVersion()
    {
        // try reading from SystemVersion.plist
        NSDictionary* versionDict = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
        if (versionDict)
        {
            NSString *versionString = [versionDict objectForKey:@"ProductVersion"];
            if (versionString)
            {
                std::string version([versionString cStringUsingEncoding:NSASCIIStringEncoding]);
                if (version.length() > 0) // improve error checking here
                    return version;
            }
        }

    #if 0
    
    //    This has been disabled for the time being because the sysctl() to return the length seemed not to work.return
    //    Using uname() to retrieve the kernel version instead.
    
        // try getting the kernel version from sysctl
        int mib[2] = { CTL_KERN, KERN_OSRELEASE };
        size_t len = 0;
        int err = sysctl(mib, 2, NULL, &len, NULL, 0);
        if (!err || len > 0)
        {
            char* p = reinterpret_cast<char*>(malloc(len));
            err = sysctl(mib, 2, p, &len, NULL, 0);
            if (!err)
            {
                std::string os_version(KernelToOSVersion(p));
                if (os_version != UNKNOWN_VERSION)
                    return os_version;
            }
        }
    #endif

        // try getting the kernel version from uname
        struct utsname name;
        int err = uname(&name);
        if (!err)
        {
            std::string os_version(KernelToOSVersion(name.release));
            return os_version;
        }
    
        return UNKNOWN_VERSION;
    }
}
