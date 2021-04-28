#include "ServiceUtilsOSX.h"

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>

#include "services/log/LogService.h"

namespace Origin
{
    namespace Escalation
    {
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
        
        bool getHelperAgentPath(const char* helperToolName, char* buffer, size_t size)
        {
            char originPath[2048];
            if (!getExecutablePath(originPath, sizeof(originPath)))
                return false;
            
            //syslog(LOG_NOTICE, "OriginWebHelperInstall - exe path: %s\n", originPath);
            
            // Format the path according to where the helpers live
            char fpath[2048];
            memset(fpath, 0, sizeof(fpath));
            snprintf(fpath, sizeof(fpath), "%s/../../Library/LaunchServices/%s", originPath, helperToolName);

            //syslog(LOG_NOTICE, "OriginWebHelperInstall - concat exe path: %s\n", fpath);
            
            memset(buffer, 0, size);
            realpath(fpath, buffer);

            //syslog(LOG_NOTICE, "OriginWebHelperInstall - real exe path: %s\n", buffer);
            
            return true;
        }
        
        bool installOrUninstallHelperAgent(const char* helperAgentPath, bool install, bool waitForExit)
        {
            @autoreleasepool
            {
                NSString *agentPath = [NSString stringWithCString:helperAgentPath encoding:NSUTF8StringEncoding];
                
                NSTask *task;
                task = [[NSTask alloc] init];
                [task setLaunchPath:agentPath];
                
                NSArray *arguments;
                if (install)
                {
                    arguments = [NSArray arrayWithObjects:@"install", nil];
                }
                else
                {
                    arguments = [NSArray arrayWithObjects:@"uninstall", nil];
                }
                [task setArguments:arguments];
                
                //NSPipe *pipe;
                //pipe = [NSPipe pipe];
                //[task setStandardOutput:pipe];
                
                NSLog(@"Launching %@ with args: %@",[task launchPath], arguments);
                
                [task launch];
                if (waitForExit)
                {
                    [task waitUntilExit];
                }
                
                [task autorelease];
                
                return true;
            }
        }
        
        bool InstallService(const char* serviceName)
        {
            if (!serviceName)
            {
                ORIGIN_LOG_ERROR << "Service name must not be null.";
                return false;
            }
            
            char helperAgentPath[2048];
            if (!getHelperAgentPath(serviceName, helperAgentPath, sizeof(helperAgentPath)))
            {
                ORIGIN_LOG_ERROR << "Unable to install service (can't get path): " << serviceName;
                return false;
            }
            
            installOrUninstallHelperAgent(helperAgentPath, true, false);
            
            return true;
        }
        
        bool UninstallService(const char* serviceName)
        {
            if (!serviceName)
            {
                ORIGIN_LOG_ERROR << "Service name must not be null.";
                return false;
            }
            
            char helperAgentPath[2048];
            if (!getHelperAgentPath(serviceName, helperAgentPath, sizeof(helperAgentPath)))
            {
                ORIGIN_LOG_ERROR << "Unable to uninstall service (can't get path): " << serviceName;
                return false;
            }
       
            installOrUninstallHelperAgent(helperAgentPath, false, true);
            
            return true;
        }
    } // Escalation
}// Origin