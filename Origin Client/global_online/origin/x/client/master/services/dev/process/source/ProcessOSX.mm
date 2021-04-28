// Copyright (c) 2012 Electronic Arts Inc.

#import <Foundation/Foundation.h>
#import <AppKit/NSWorkspace.h>
#import <AppKit/NSRunningApplication.h>

#include "services/process/ProcessOSX.h"
#include "services/exception/ExceptionSystem.h"

#include <sys/stat.h>

@interface OriginProcessNotificationHandler : NSObject

// Notification handler called when a new process has started
-(void)ProcessStartedNotification:(NSNotification*)notification;

// Notification handler called when a process closed down
-(void)ProcesssTerminatedNotification:(NSNotification*)notification;

@end

namespace Origin { namespace Services {
    
    // Applications are directly launched through the NSTask API and not with /usr/bin/open
    // starting with El Capitan because of SIP (System Integrity Protection)
    pid_t launchTaskWithNSTask( QString applicationPath, QStringList args, QStringList environment, Exception::ExceptionSystem* exceptionSystem)
    {
        @try
        {
            // Create the arguments array.
            NSMutableArray* theArgs = [NSMutableArray arrayWithCapacity: args.size()];
            
            for ( int i=0; i != args.size(); ++i )
                [theArgs addObject: [NSString stringWithUTF8String: args.at( i ).toUtf8().constData()]];
            
            // Convert the environment to an NSDictionary of NSString=NSString.
            NSMutableDictionary* theEnv = [NSMutableDictionary dictionaryWithCapacity: environment.size()];
            for ( int i=0; i != environment.size(); ++i )
            {
                // Split environment strings at the '=' char.
                const QString& item = environment.at( i );
                int offset = item.indexOf( "=" );
                if ( offset == -1 ) offset = item.length();
                NSString* key = [NSString stringWithUTF8String: item.left( offset ).toUtf8().constData()];
                NSString* value;
                if ( item.length() > offset )
                    value = [NSString stringWithUTF8String: item.mid( offset + 1 ).toUtf8().constData()];
                else
                    value = [[[NSString alloc] init] autorelease];
                
                // Insert this key/value pair into the dictionary.
                [theEnv setObject: value forKey: key];
            }
            
#ifdef DEBUG
            NSLog(@"Starting process with args:\n%@", theArgs);
            NSLog(@"\nand envs:\n%@", theEnv);
#endif
            // We need to remove our exception handle temporarily so that it doesn't make it to the child
            // process - otherwise we would get exceptions from the games
            bool restoreExceptionSystem = false;
            if (exceptionSystem && exceptionSystem->enabled())
            {
                exceptionSystem->setEnabled(false);
                restoreExceptionSystem = true;
            }
            
            // Prepare to start the process.
            NSTask* task = [[[NSTask alloc] init] autorelease];
            
            [task setLaunchPath: [NSString stringWithUTF8String: applicationPath.toUtf8().constData()]];
            [task setArguments: theArgs];
            [task setEnvironment: theEnv];
            
            // Start the process.
            [task launch];
            pid_t pid = [task processIdentifier];
            
            if (restoreExceptionSystem)
                exceptionSystem->setEnabled(true);
            
            return pid;
        }
        @catch (NSException* ex)
        {
            return 0;
        }
        return 0;
    }

    bool terminateThroughFinder( pid_t pid )
    {
        NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier: pid];
        if ( app == nil ) return false;
        BOOL success = [app terminate];
        return success == YES;
    }
    
    bool openURL( const char* url )
    {
        return [[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: [NSString stringWithUTF8String: url]]];
    }
    
    void initializeProcessNotificationHandler()
    {
        static BOOL initialized = NO;
        static OriginProcessNotificationHandler* object;
        
        if ( ! initialized )
        {
            initialized = YES;
            
            object = [[OriginProcessNotificationHandler alloc] init];
            
            NSNotificationCenter * workspaceNotificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
            
            // Finder usually provides WillLaunchApplication if we launch through Finder.
            // This was disabled because calling both "Will" and "Did" can cause duplicate processes in our list
            //[workspaceNotificationCenter addObserver:object selector:@selector( ProcessStartedNotification: ) name:NSWorkspaceWillLaunchApplicationNotification object:nil];
            
            // Finder will notify DidLaunchApplication for almost all Cocoa apps, but not many others.
            // This is re-enabled because  not all applications send the WillLaunchApplication event.
            [workspaceNotificationCenter addObserver:object selector:@selector( ProcessStartedNotification: ) name:NSWorkspaceDidLaunchApplicationNotification object:nil];
            
            // Finder provides DidTerminateApplication for almost all GUI apps, Cocoa or not.
            [workspaceNotificationCenter addObserver:object selector:@selector( ProcesssTerminatedNotification: ) name:NSWorkspaceDidTerminateApplicationNotification object:nil];
        }
    }
    
    void populateCurrentlyRunningProcesses( Origin::Services::AppProcessWatcher* watcher )
    {
        // Get the list of running processes.
        NSArray* apps = [[NSWorkspace sharedWorkspace] runningApplications];
        
        // Iterate over the list of running processes.
        for ( NSRunningApplication* app in apps )
        {
            // Get the folderName.
            NSString* folderName = [[app executableURL] path];
            
            watcher->processScanned( [app processIdentifier], [folderName UTF8String] );
        }
    }

    void AppProcessWatcher::scanProcesses()
    {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
        
        // Prepare to scan all processes.
        QMutexLocker lock( &mMutex );
        mPrepopulated = false;
        int previousProcessCount = mProcesses.size();
        mProcesses.clear();
        
        // Rescan all processes.
        populateCurrentlyRunningProcesses( this );
        
        // Reset the prepopulated flag.
        mPrepopulated = true;
        
        // Notify clients if we no longer have matching processes.
        if ( processCount() == 0 && previousProcessCount )
        {
            // Emit the finished signal.
            emit finished();
        }
        
        [pool drain];
    }

} } // namespace Origin::Services

@implementation OriginProcessNotificationHandler

// Notification handler called when a new process has started
-(void)ProcessStartedNotification: ( NSNotification* ) notification
{
    @try
    {
        // Extract process path
        NSRunningApplication* app = [[notification userInfo] objectForKey:NSWorkspaceApplicationKey];
        NSString* folderName = [[app executableURL] path];
        
        Origin::Services::AppProcessWatcher::notifyProcessCreated( [app processIdentifier], [folderName UTF8String] );
    }
    @catch (NSException* ex)
    {
        // TODO - LOG
    }
}

// Notification handler called when a process closed down
-(void)ProcesssTerminatedNotification: ( NSNotification* ) notification
{
    @try
    {
        // Extract process path
        NSRunningApplication* app = [[notification userInfo] objectForKey:NSWorkspaceApplicationKey];
        NSString* folderName = [[app executableURL] path];
        
        Origin::Services::AppProcessWatcher::notifyProcessDestroyed( [app processIdentifier], [folderName UTF8String] );
    }
    @catch (NSException* ex) 
    {
        // TODO - LOG
    }
}

@end
