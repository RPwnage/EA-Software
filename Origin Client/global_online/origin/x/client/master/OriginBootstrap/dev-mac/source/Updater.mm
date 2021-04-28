//
//  Updater.m
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "Updater.h"
#import "AppDelegate.h"
#import "Locale.h"
#import "LogServiceWrapper.h"
#import "Utilities.h"

#include "Unpacker.h"

#include "version/version.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdint.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <sys/time.h>

#include <map>
#include <string>

#include <EULAVersions.h>

#import <CoreServices/CoreServices.h>

// local prototypes
bool WasLaunchedAsLoginOrResumeItem();

@implementation StagedDataInfo

@synthesize stagedUpdateAvailable;
@synthesize matchesAutoPatchVersion;
@synthesize version;
@synthesize filePath;

- (id)init
{
    self = [super init];
    if(self) 
    {
        stagedUpdateAvailable = false;
        matchesAutoPatchVersion = false;
        version = @"";
        filePath = @"";
    }
    
    return self;
}

- (void) dealloc
{
    [filePath release];
    [version release];
    [super dealloc];
}

@end

@interface UnpackThreadData : NSObject {
    NSString* unpackFileLocation;
    NSString* unpackVersion;
    BOOL      unpackIsBeta;
    NSValue* selectorOnComplete;
}

@property (retain) NSString* unpackFileLocation;
@property (retain) NSString* unpackVersion;
@property (assign) BOOL unpackIsBeta;
@property (retain) NSValue* selectorOnComplete;

@end

@implementation UnpackThreadData
@synthesize unpackFileLocation;
@synthesize unpackVersion;
@synthesize unpackIsBeta;
@synthesize selectorOnComplete;

- (id)init
{
    self = [super init];
    if(self) {
        unpackFileLocation = @"";
        unpackIsBeta = NO;
        selectorOnComplete = nil;
    }
    
    return self;
}

- (void) dealloc
{
    [unpackFileLocation release];
    [super dealloc];
}
@end
 
@interface Updater (hidden)

-(void) downloadDidBegin:(NSURLDownload*)download;
-(void) downloadDidFinish:(NSURLDownload*)download;
-(void) download:(NSURLDownload*)download didReceiveDataOfLength:(NSUInteger)length;
-(void) download:(NSURLDownload*)download didFailWithError:(NSError *)error;
-(void) postEulaStep:(bool)eulaUpdated;
-(void) findStagedData;

-(void) unpackThreadRoutine:(UnpackThreadData*)threadInfo;
-(void) unpackUpdate:(NSString*)archiveFilename :(NSString*)updateVersion :(SEL)onUnpackCompleteMethod;

-(bool) loadOriginClientLib;
-(void) unloadOriginClientLib;

-(int) getLocalEulaVersion;
@end

@implementation Updater

@synthesize originClientArgs;
@synthesize downloader;
@synthesize stagedInfo;
@synthesize currentUpdateSource;
@synthesize updated;
@synthesize isOfflineMode;
@synthesize isDownloadSuccessful;
@synthesize haveValidClientLib;
@synthesize isDataError;
@synthesize bypassOverrideEnabled;
@synthesize downloadedBytes;
@synthesize appDelegate;
@synthesize libOriginClientHandle;
@synthesize newEulaVersion;
@synthesize environment;
@synthesize updateVersion;
@synthesize declinedUpdate;
@synthesize downloadConfigErrorString;
@synthesize downloadConfigErrorCode;
@synthesize downloadConfigRequestDurationMilliseconds;

- (id)init
{
    self = [super init];
    if(self) {
        [self queryEnvironment];
        downloader = [[Downloader alloc] init];
        [downloader querySettingsFile:environment];
        
        stagedInfo = [[StagedDataInfo alloc] init];
        downloadedBytes = 0;
        updated = false;
        isOfflineMode = false;
        isDownloadSuccessful = false;
        haveValidClientLib = false;
        isDataError = false;
        bypassOverrideEnabled = false;
        declinedUpdate = false;
    }
    
    return self;
}

- (void) dealloc
{
    [stagedInfo release];
    [downloader release];
    [super dealloc];
}

-(void)queryEnvironment
{
    [self setEnvironment:@"production"];
    
    NSString* envOverride = NULL;
    
    // If command line contains -EnvironmentName=<environment> that overrides any EACore.ini values
    // Check whether we have a valid command line to override the app version
    for(NSString* arg in [[NSProcessInfo processInfo] arguments])
    {
        NSRange range = [arg rangeOfString:@"-EnvironmentName:" options:(NSCaseInsensitiveSearch|NSAnchoredSearch)];
        if(range.location != NSNotFound)
        {
            envOverride = [[NSString alloc] initWithString:[arg substringFromIndex:range.length]];
            ORIGIN_LOG_EVENT(@"Read override environment value from command line: %@", envOverride);
        }
    }
    
    if(envOverride == NULL)
    {
        IniFileReader* iniFileReader = [IniFileReader defaultIniFileReader];
        NSString* env = [iniFileReader valueForKey:@"EnvironmentName" inSection:@"Connection"];
        if (env != nil)
        {
            envOverride = [NSString stringWithCString:[env UTF8String] encoding:NSUTF8StringEncoding];
            ORIGIN_LOG_EVENT(@"Read override environment value from ini: %@", envOverride);
        }
    }
    
    if(envOverride != NULL)
    {
        NSString* normalizedEnv = [[envOverride lowercaseString] stringByReplacingOccurrencesOfString:@"." withString:@"-"];
        ORIGIN_LOG_EVENT(@"Setting normalized environment override to: %@", normalizedEnv);
        [self setEnvironment:normalizedEnv];
    }
}

+(NSString*) appVersion
{
    static bool checkVersionOverride = false;
    static NSString* myAppVersion = @EALS_VERSION_P_DELIMITED;
    
    if (!checkVersionOverride)
    {
        checkVersionOverride = true;
        
        // Check whether we have a valid command line to override the app version
        for(NSString* arg in [[NSProcessInfo processInfo] arguments])
        {
            NSRange range = [arg rangeOfString:@"-version=" options:(NSCaseInsensitiveSearch|NSAnchoredSearch)];
            if(range.location != NSNotFound)
            {
                myAppVersion = [[NSString alloc] initWithString:[arg substringFromIndex:range.length]];
                break;
            }
        }
    }

    return myAppVersion;
}

+ (uint64_t) time
{
    //return EA::StdC::GetTime();
    struct timeval tv;
    int error = gettimeofday(&tv, NULL);
    (void) error;
    uint64_t time = (uint64_t)((tv.tv_sec * UINT64_C(1000000000)) + (tv.tv_usec * UINT64_C(1000)));
    return time;
}

-(void) downloadDidBegin:(NSURLDownload *)download
{
    ORIGIN_LOG_EVENT(@"Download began successfully!");
    
    // Make sure the download progress window is shown
    if(appDelegate != nil)
    {
        [appDelegate showDownloadProgressDialog];
    }
}

-(void) downloadDidFinish:(NSURLDownload *)download
{
    ORIGIN_LOG_EVENT(@"Download finished successfully!");
    
    if(appDelegate != nil)
    {
        [appDelegate updateDownloadProgress:1.0 estimatedTimeRemaining:@""];
        [appDelegate hideDownloadProgressDialog];
    }
    
    // Finalize the download
    [downloader finalizeDownload];
    
    [self unpackUpdate:[[downloader info] outputFilename] :[[downloader info] versionNumber] :@selector(onUnpackDownloadComplete:)];
}

-(void) onUnpackDownloadComplete:(NSNumber*)isUnpackSuccessful
{
    // If we failed to unpack the downloaded data, attempt to install from staged data
    if(![isUnpackSuccessful boolValue] && [stagedInfo stagedUpdateAvailable])
    {
        ORIGIN_LOG_WARNING(@"Failed to unpack downloaded update, attempting to unpack available staged update.");
        [self unpackUpdate:[stagedInfo filePath] :[stagedInfo version] :@selector(onUnpackStagedDataComplete:)];
        return;
    }
    else if([stagedInfo stagedUpdateAvailable]) // if we successfully unpacked the download, delete staged data
    {
        [[NSFileManager defaultManager] removeItemAtPath:[stagedInfo filePath] error:nil];
    }
    
    if(appDelegate != nil)
    {
        [appDelegate updateProcessingComplete];
    }
}

-(void) onUnpackStagedDataComplete:(NSValue*)unpackResult
{
    if(appDelegate != nil)
    {
        [appDelegate updateProcessingComplete];
    }
}

-(int) secondsElapsedFromStart:(uint64_t) startTime
{
    uint64_t timeDiff = mach_absolute_time() - startTime;
    static double conversion = 0.0;
    
    if(conversion == 0.0)
    {
        mach_timebase_info_data_t info;
        kern_return_t err = mach_timebase_info( &info );
        
        if( err == 0 )
        {
            conversion = 1e-9 * (double) info.numer / (double) info.denom;
        }
    }
    
    return conversion * (double) timeDiff;
}

-(void) download:(NSURLDownload*)download didReceiveResponse:(NSURLResponse *)response
{
    NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
    
    if([httpResponse statusCode] != 200)
    {
        ORIGIN_LOG_ERROR(@"Server failed to return update information [status code %d]", [httpResponse statusCode]);
        ORIGIN_LOG_DEBUG(@"Download URL was [%@]", [httpResponse URL]);
        [download cancel];
        [appDelegate updateProcessingComplete];
        return;
    }
    
    NSString* contentLength = [[httpResponse allHeaderFields] objectForKey:@"Content-Length"];
    
    if(contentLength == nil || [httpResponse expectedContentLength] == NSURLResponseUnknownLength)
    {
        ORIGIN_LOG_ERROR(@"Failed to retrieve update's content length from server.");
        [download cancel];
        [appDelegate updateProcessingComplete];
        return;
    }
    
    [[downloader info] setDownloadSizeBytes:strtoull([contentLength UTF8String], NULL, 0)];
    
    // Check disk space
    NSError* error = nil;
    NSDictionary* attr = [[NSFileManager defaultManager] attributesOfFileSystemForPath:[[NSBundle mainBundle] bundlePath] error:&error];
    NSNumber* freeBytes = [attr objectForKey:NSFileSystemFreeSize];
    
    if([freeBytes unsignedLongLongValue] < [[downloader info] downloadSizeBytes])
    {
        ORIGIN_LOG_WARNING(@"System does not have enough disk space [%@] to download update [%lld].", freeBytes, [[downloader info] downloadSizeBytes]);
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:[Locale localize:@"ebisu_client_not_enough_disk_space_uppercase"]];
        [alert setInformativeText:[Locale localize:@"ebisu_client_not_enough_disk_space"]];
        [alert runModal];
        
        // If this wasn't an optional update, exit the application
        if(![[downloader info] isOptionalUpdate])
        {
            exit(0);
        }
        else
        {
            [download cancel];
            [appDelegate updateProcessingComplete];
        }
    }
}

-(void) download:(NSURLDownload*)download didReceiveDataOfLength:(NSUInteger)length
{
    static int lastLog = 0;
    static int curSecondsElapsed = 0;
    static uint64_t startTime = mach_absolute_time();
    
    self.downloadedBytes += length;

    int secondsElapsed = [self secondsElapsedFromStart:startTime];
    
    // Make sure we only update every second to keep this from spamming
    if(curSecondsElapsed != secondsElapsed)
    {
        int kbDownloaded = self.downloadedBytes / 1024;
        int downloadSpeed = kbDownloaded/secondsElapsed;
        int secondsRemaining = (([[downloader info] downloadSizeBytes] - self.downloadedBytes) / 1024) / (downloadSpeed + 1);
        int minutesRemaining = secondsRemaining / 60;
        // int hoursRemaining = minutesRemaining / 60;
        
        secondsRemaining %= 60;
        minutesRemaining %= 60;
        
        NSMutableString* downloadEtaString = [[[NSMutableString alloc] initWithString:@""] autorelease];
        
        /*
        if(hoursRemaining != 0)
        {
            [downloadEtaString appendFormat:@"%d %@ ", hoursRemaining, [Locale localize:@"origin_bootstrap_hour"]];
        }
        if(minutesRemaining != 0)
        {
            [downloadEtaString appendFormat:@"%d %@ ", minutesRemaining, [Locale localize:@"origin_bootstrap_min"]];
        }
        if(secondsRemaining != 0)
        {
            [downloadEtaString appendFormat:@"%d %@", secondsRemaining, [Locale localize:@"origin_bootstrap_sec"]];
        }
        */
        
        NSString* bytesDownloaded;
        double downloadedValue = (((double)self.downloadedBytes/1024.0/1024.0));
        
        if(true) //downloadedValue > 100)
        {
            bytesDownloaded = [NSString stringWithFormat:@"%.1f", downloadedValue];
        }
        else
        {
            bytesDownloaded = [NSString stringWithFormat:@"%d", (int)downloadedValue];
        }
        
        NSString* totalBytes;
        double totalValue = ((double)[[downloader info] downloadSizeBytes])/1024.0/1024.0;
        
        if(true) //totalValue > 100)
        {
            totalBytes = [NSString stringWithFormat:@"%.1f", totalValue];
        }
        else
        {
            totalBytes = [NSString stringWithFormat:@"%d", (int)totalValue];
        }
        
        [downloadEtaString appendFormat:@"%@", [NSString stringWithFormat:[Locale localize:@"origin_bootstrap_copied_mac"], bytesDownloaded, totalBytes ]];
        
        double downloadPercent = ((double)self.downloadedBytes/(double)[[downloader info] downloadSizeBytes]);
         
        [appDelegate updateDownloadProgress:downloadPercent estimatedTimeRemaining:downloadEtaString];
        
        curSecondsElapsed = secondsElapsed;
    }
        
    // Log every 10 megs or so, so we aren't spamming console
    if(self.downloadedBytes - lastLog > (10*1024*1024))
    {
        ORIGIN_LOG_DEBUG(@"Download progress, %d bytes downloaded", self.downloadedBytes);
        lastLog = self.downloadedBytes;
    }
    

}

-(void) download:(NSURLDownload*)download didFailWithError:(NSError *)error
{
    ORIGIN_LOG_ERROR(@"Download failed with error %@", error);
    ORIGIN_LOG_ERROR(@"Underlying error: %@", [[error userInfo] valueForKey:NSUnderlyingErrorKey]);
    
    [appDelegate hideDownloadProgressDialog];
    
    // If we have staged data, try to unpack that
    if([stagedInfo stagedUpdateAvailable])
    {
        ORIGIN_LOG_EVENT(@"Download from autopatch failed, attempting to unpack available mismatched staged update.");
        [self unpackUpdate:[stagedInfo filePath] :[stagedInfo version] :@selector(onUnpackStagedDataComplete:)];
    }
    else 
    {
        if(![[downloader info] isOptionalUpdate])
        {
            [self addClientArg:@"/StartOffline"];
        }
        [appDelegate updateProcessingComplete];
    }
}

-(void) onInstallStagedUpdateComplete:(NSNumber*)unpackResult
{
    // if we failed to unpack staged and there's an update online, start the download
    if(![unpackResult boolValue] && [[[downloader info] url] length] > 0)
    {
        ORIGIN_LOG_WARNING(@"Failed to unpack staged download, attempting to download from autopatch service.");
        [stagedInfo setStagedUpdateAvailable:NO];
        
        if(![downloader beginDownload:[NSURL URLWithString:[[downloader info] url]] :[Updater appVersion]  : self])
        {
            [appDelegate updateProcessingComplete];
        }
        return;
    }
    else 
    {
        ORIGIN_LOG_ERROR(@"Failed to unpack staged download and no autopatch version was available.");
        [appDelegate updateProcessingComplete];
    }
}

-(void) installUpdate
{
   // If we have a staged source, we just unpack it here
    if(currentUpdateSource == UPDATE_SOURCE_STAGED)
    {
        [self unpackUpdate:[stagedInfo filePath] :[stagedInfo version] :@selector(onInstallStagedUpdateComplete:)];
    }
    else
    {
        if(![downloader beginDownload:[NSURL URLWithString:[[downloader info] url]] :[Updater appVersion]  : self])
        {
            [appDelegate updateProcessingComplete];
        }
    }
}

-(void) eulaAccepted
{
    [self postEulaStep:YES];
}

-(void) findStagedData
{
    // We'll use the currently installed version and delete staged data if it matches
    NSString* clientVersion = [self getOriginClientVersion];
    
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    NSPredicate* filter = [NSPredicate predicateWithFormat:@"self BEGINSWITH 'OriginUpdate' AND self ENDSWITH '.zip'"];
    NSArray* stagedDownloads = [[[NSFileManager defaultManager] contentsOfDirectoryAtPath:bundlePath error:nil] filteredArrayUsingPredicate:filter];
    
    ORIGIN_LOG_EVENT(@"Found %d staged downloads", [stagedDownloads count]);
    
    NSString* selectedVersion = @"";
    int selectedVersionComponents[4] = { 0, 0, 0, 0 };
    int selectedStagedData = -1;
    
    bool foundAutoPatchVersion = false;
    
    for(unsigned int stageIndex = 0; stageIndex < [stagedDownloads count]; stageIndex++)
    {
        // Parse version number out of file format: OriginUpdate_9_0_2_2064.zip
        NSMutableCharacterSet* tokenDelimSet = [NSMutableCharacterSet characterSetWithCharactersInString:@"_."];
        NSArray* components = [(NSString*)[stagedDownloads objectAtIndex:stageIndex] componentsSeparatedByCharactersInSet:tokenDelimSet];
        
        // This will give us an array of ["OriginUpdate", "9", "0", "2", "2064", "zip"], so just grab the version components and
        // format them in the expected period delimited format.
        if([components count] > 4)
        {
            NSString* tempVersion = [NSString stringWithFormat:@"%@.%@.%@.%@", (NSString*)[components objectAtIndex:1], (NSString*)[components objectAtIndex:2],
                                     (NSString*)[components objectAtIndex:3], (NSString*)[components objectAtIndex:4]];
            
            // We don't need to use a staged update that matches our currently installed client.
            if([clientVersion compare:tempVersion] == 0)
            {
                continue;
            }
            
            // If we find a staged update that matches the autopatch service version then we use it and stop looking.
            if([[[downloader info] versionNumber] compare:tempVersion] == 0)
            {
                selectedVersion = [[downloader info] versionNumber];
                selectedStagedData = stageIndex;
                foundAutoPatchVersion = true;
                break;
            }
            
            // We only used staged data that doesn't match the autopatch update version if we don't have a good
            // client install already or the autopatch service is unavailable for any reason.
            if(![self haveValidClientLib] || ![[downloader info] serviceAvailable])
            {
                for(int j = 1; j <= 4; j++)
                {
                    if([(NSString*)[components objectAtIndex:j] intValue] > selectedVersionComponents[j])
                    {
                        selectedVersion = [tempVersion retain];
                        selectedStagedData = stageIndex;
                        for(int k = j; j <= 4; j++)
                        {
                            selectedVersionComponents[k] = [(NSString*)[components objectAtIndex:j] intValue];
                        }
                    }
                }
            }
        }
    }
    
    if(selectedStagedData != -1)
    {
        ORIGIN_LOG_EVENT(@"Found staged update version: %@.  Matches autopatch version? %@", selectedVersion, foundAutoPatchVersion ? @"YES" : @"NO");
        [stagedInfo setVersion:selectedVersion];
        [stagedInfo setMatchesAutoPatchVersion:foundAutoPatchVersion];
        [stagedInfo setStagedUpdateAvailable:true];
        [stagedInfo setFilePath:[NSString stringWithFormat:@"%@/%@", bundlePath, (NSString*)[stagedDownloads objectAtIndex:selectedStagedData]]];
    }
    
    // Clean up other staged data files
    for(int i = 0; i < (int)[stagedDownloads count]; i++)
    {
        if(i == selectedStagedData)
            continue;
        
        NSString* stagedDownloadPath = (NSString*)[stagedDownloads objectAtIndex:i];
        NSString* fullPath = [NSString stringWithFormat:@"%@/%@", bundlePath, stagedDownloadPath];
        ORIGIN_LOG_ACTION(@"Removing staged download %@", fullPath);
        [[NSFileManager defaultManager] removeItemAtPath:fullPath error:nil];
    }
}

-(void) updateBetaFlag:(BOOL)isBeta
{
    // Open the ClientInfo.plist file, add an IsBeta key, serialize it out
    NSString* plistPath = [NSString stringWithFormat:@"%@/Contents/ClientInfo.plist", [[NSBundle mainBundle] bundlePath]];
    
    NSMutableDictionary* temp;
    
    if([[NSFileManager defaultManager] fileExistsAtPath:plistPath])
    {
        temp = [NSMutableDictionary dictionaryWithContentsOfFile:plistPath];
    }
    else
    {
        temp = [[NSMutableDictionary alloc] initWithCapacity:1];
    }
    
    [temp setValue:(isBeta ? @"true" : @"false") forKey:@"IsBeta"];
    [temp writeToFile:plistPath atomically:YES];
}

// This is the background thread routine used to unpack updates so the main application thraed
// can continue so the application stays responsive.  The thread info provided should contain
// the complete file path to the archive to be unpacked.  Archive will be unpacked to the root of
// the application bundle and then deleted if unpacked successfully.  If a selector is provided
// in the threadInfo, it will be called with an NSNumber containing a boolean that indicates if the
// update was unpacked successfully
-(void) unpackThreadRoutine:(UnpackThreadData*)threadInfo
{
    if(appDelegate != nil)
    {
        [appDelegate showProgressDialog];
    }
    
    ORIGIN_LOG_ACTION(@"Unpacking update from %@", [threadInfo unpackFileLocation]);
    
    // Extract it
    Unpacker unpacker;
    bool unpacked = unpacker.Extract([[threadInfo unpackFileLocation] cStringUsingEncoding:NSASCIIStringEncoding],
                                     [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSASCIIStringEncoding]);
    
    if(unpacked)
    {
        ORIGIN_LOG_EVENT(@"Update unpacked successfully, updated version is %@.", [threadInfo unpackVersion]);
        [self updateBetaFlag:[threadInfo unpackIsBeta]];
        [self addClientArg:[NSString stringWithFormat:@"/Installed:%@", [threadInfo unpackVersion]]];
        [self setUpdated:true];
    }
    else 
    {
        ORIGIN_LOG_ERROR(@"Failed to unpack update...");
    }
    
    [[NSFileManager defaultManager] removeItemAtPath:[threadInfo unpackFileLocation] error:nil];
    
    if(appDelegate != nil)
    {
        [appDelegate hideProgressDialog];
    }
    
    // Callback on GUI thread
    if([threadInfo selectorOnComplete] != nil)
    {
        SEL selector;
        [[threadInfo selectorOnComplete] getValue:&selector];
        [self performSelectorOnMainThread:selector withObject:[NSNumber numberWithBool:unpacked]  waitUntilDone:true];
    }
}

// This call sets up the background unpack operation and kicks it off
-(void) unpackUpdate:(NSString*)unpackFileLocation :(NSString*)unpackVersion :(SEL) onUnpackCompleteMethod
{    
    // Always unload existing client lib, if it was loaded
    [self unloadOriginClientLib];

    UnpackThreadData* data = [[[UnpackThreadData alloc] init] autorelease];
    
    [data setUnpackFileLocation:unpackFileLocation];
    [data setUnpackVersion:unpackVersion];
    [data setUnpackIsBeta:([[[downloader info] type] caseInsensitiveCompare:@"BETA"] == 0)];
    
    if(onUnpackCompleteMethod != nil)
    {
        [data setSelectorOnComplete:[NSValue valueWithBytes:&onUnpackCompleteMethod objCType:@encode(SEL)]];
    }
    
    NSOperationQueue *queue = [[[NSOperationQueue alloc] init] autorelease];
    
    NSInvocationOperation* operation = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(unpackThreadRoutine:) object:data];
    
    [queue addOperation:operation];
    [operation release];
}

// This is called after the EULA step of the update process.  If a EULA was presented to the user (or updated but not presented because it was only a minor update), then this
// function is only called after they accept the EULA
-(void) postEulaStep:(bool)eulaUpdated
{
    // Get our currently installed version, if available
    NSString* clientVersion = [self getOriginClientVersion];
    
    if(clientVersion != nil && [clientVersion length] > 0)
    {
        ORIGIN_LOG_HELP(@"Current installed client version is: %@", clientVersion);
    }
    
    // TODO: we may need to move this into the client for the escalation service to write this setting
    // once the global settings are moved to /Library/Application Support/Origin/
    if(eulaUpdated)
    {
        ORIGIN_LOG_ACTION(@"Updating EULA information in Origin settings");
        [downloader writeEulaSettings:[NSString stringWithFormat:@"%d", newEulaVersion] :[[downloader info] eulaUrl]];
    }

    if([stagedInfo stagedUpdateAvailable])
    {
        ORIGIN_LOG_HELP(@"Staged version is %@", [stagedInfo version]);
    }
    
    ORIGIN_LOG_HELP(@"Network version is %@", [[downloader info] versionNumber]);
    
    if([clientVersion length] > 0 && [clientVersion compare:[[downloader info] versionNumber]] == 0) // We are up to date.
    {
        // delete any staged updates if any were found
        if([stagedInfo stagedUpdateAvailable])
        {
            [[NSFileManager defaultManager] removeItemAtPath:[stagedInfo filePath] error:nil];
            [stagedInfo setStagedUpdateAvailable:FALSE];
        }
        
        ORIGIN_LOG_HELP(@"Installed client is already latest version, launching...");
        [appDelegate updateProcessingComplete];
        return;
    }
       
    // We have a few different options we can use to download and apply updates. The first thing we want to do
    // is check to apply a staged update package. We will apply staged updates in the following scenarios
    // 1. If we have a staged update and it matches the update on the server
    // 2. If we have a staged update and don't see an update on the server (Either because we are testing and
    // there isn't one or we couldn't communicate with the server)
    // 3. If we attempted to download an update and failed
    // In all other cases we want to download the update that we have found online
    if([stagedInfo matchesAutoPatchVersion] || (([[[downloader info] url] length] == 0) && [stagedInfo stagedUpdateAvailable]))
    {
        currentUpdateSource = UPDATE_SOURCE_STAGED;
        
        if([stagedInfo matchesAutoPatchVersion])
        {
            ORIGIN_LOG_ACTION(@"Staged version matches autopatch version, attempting to unpack.");
        }
        else
        {
            ORIGIN_LOG_ACTION(@"No autopatch version available, attempting to unpack available staged version.");
        }
        
        // If we are auto-updating, or we don't already have a valid client skip the
        // dialog to prompt user to update.
        if(![[downloader info] isAutoUpdate] && haveValidClientLib)
        {
            [appDelegate displayUpdateDialog:[stagedInfo version] :[self environment] :[Updater appVersion]];
        }
        else 
        {
            [self unpackUpdate:[stagedInfo filePath] :[stagedInfo version] :@selector(onInstallStagedUpdateComplete:)];
        }
    }
    else if([[[downloader info] url] length] > 0)
    {
        // This is a debugging check only, if we read information from EACore.ini, the type could be set to BETA 
        // and if our users hasn't opted in, then there is no update available for them
        if(bypassOverrideEnabled && [[[downloader info] type] caseInsensitiveCompare:@"BETA"] == 0 && ![[downloader info] isBetaOptIn])
        {
            ORIGIN_LOG_WARNING(@"Override specifies BETA update type, but user has not opted in for betas so skipping and launching current client...");
            [appDelegate updateProcessingComplete];
            return;
        }
        
        // If we are auto-updating, or we don't already have a valid client skip the
        // dialog to prompt user to update.
        if(![[downloader info] isAutoUpdate] && haveValidClientLib)
        {
            currentUpdateSource = UPDATE_SOURCE_DOWNLOAD;
            [appDelegate displayUpdateDialog:[[downloader info] versionNumber] :[self environment] :[Updater appVersion]];
        }
        else
        {
            if(![downloader beginDownload:[NSURL URLWithString:[[downloader info] url]] :[Updater appVersion] : self])
            {
                [appDelegate updateProcessingComplete];
                return;
            }
        }
    }
    else 
    {
        ORIGIN_LOG_EVENT(@"No staged data or download URL found for update, launching current client version...");
        [appDelegate updateProcessingComplete];
    }
}

-(int) getLocalEulaVersion
{
    // Now that we have the case of same language (de) for multiple zones (EU/German/ROW), we need to remap
    // this current locale to access the shared versioning table (and simplify the code sharing with PC)
    NSString* locale = [Locale regionBasedLocale];
    // BootstrapShared::EULA::GetVersion requires a wchar_t*
    // The size of wchar_t was changed from 2 bytes to 4
    if ( sizeof( wchar_t ) == 2 )
        return BootstrapShared::EULA::GetVersion((const wchar_t*)[locale cStringUsingEncoding:NSUTF16LittleEndianStringEncoding]);
    else
        return BootstrapShared::EULA::GetVersion((const wchar_t*)[locale cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
}

-(void) doUpdates:(AppDelegate*)appDelegateIn
{
    appDelegate = appDelegateIn;
    
    // Delete any partially downloaded files from previous runs
    [downloader clearPartialDownloads];
    
    // Look for debug directives in EACore.ini

    bool doUpdateCheck = true;
    bool updateInfoRetrieved = false;
    
    IniFileReader* iniFileReader = [IniFileReader defaultIniFileReader];
    if(iniFileReader)
    {
        NSString* debugUrl = [iniFileReader valueForKey:@"URL" inSection:@"Bootstrap"];
        if (debugUrl != nil)
        {
            [downloader setUrlOverride:[NSString stringWithCString:[debugUrl UTF8String] encoding:NSUTF8StringEncoding]];
            ORIGIN_LOG_EVENT(@"Update server URL override detected: %@", [downloader urlOverride]);
        }
        
        NSString* debugVersion = [iniFileReader valueForKey:@"Version" inSection:@"Bootstrap"];
        NSString* debugUpdateType = [iniFileReader valueForKey:@"Type" inSection:@"Bootstrap"];
        NSString* debugDownloadLocation = [iniFileReader valueForKey:@"DownloadLocation" inSection:@"Bootstrap"];
        NSString* debugRule = [iniFileReader valueForKey:@"Rule" inSection:@"Bootstrap"];
        NSString* debugEulaVersion = [iniFileReader valueForKey:@"EulaVersion" inSection:@"Bootstrap"];
        NSString* debugEulaLocation = [iniFileReader valueForKey:@"EulaLocation" inSection:@"Bootstrap"];
        NSString* debugEulaLatestMajorVersion = [iniFileReader valueForKey:@"EulaLatestMajorVersion" inSection:@"Bootstrap"];
        
        if (debugVersion && debugUpdateType && debugDownloadLocation && debugRule && debugEulaVersion && debugEulaLocation && debugEulaLatestMajorVersion)
        {
            [downloader useDebugResponseWithVersion:[NSString stringWithCString:[debugVersion UTF8String] encoding:NSUTF8StringEncoding]
                                        type:[NSString stringWithCString:[debugUpdateType UTF8String] encoding:NSUTF8StringEncoding]
                                        downloadLocation:[NSString stringWithCString:[debugDownloadLocation UTF8String] encoding:NSUTF8StringEncoding]
                                        rule:[NSString stringWithCString:[debugRule UTF8String] encoding:NSUTF8StringEncoding]
                                        eulaVersion:[NSString stringWithCString:[debugEulaVersion UTF8String] encoding:NSUTF8StringEncoding]
                                        eulaLocation:[NSString stringWithCString:[debugEulaLocation UTF8String] encoding:NSUTF8StringEncoding]
                             eulaLatestMajorVersion:[NSString stringWithCString:[debugEulaLatestMajorVersion UTF8String] encoding:NSUTF8StringEncoding]];
            ORIGIN_LOG_ACTION(@"Using debug response: (version: %@) (updateType: %@) (downloadLocation: %@) (rule: %@) (eulaVersion: %@) (eulaLocation: %@) (eulaLatestMajorVersion: %@)",
                  [[downloader info] versionNumber], 
                  [[downloader info] type],
                  [[downloader info] url],
                  [[downloader info] updateRule],
                  [[downloader info] eulaVersion],
                  [[downloader info] eulaUrl],
                  [[downloader info] eulaLatestMajorVersion]);
            doUpdateCheck = false;
            updateInfoRetrieved = true;
            bypassOverrideEnabled = true;
        }
    }
    else 
    {
        // This is normal operation, but if tester was expecting a EACore.ini this would be helpful
        ORIGIN_LOG_HELP(@"No override file found.");
    }
    
    if(doUpdateCheck)
    {
        ORIGIN_LOG_EVENT(@"Querying update service for environment %@ with current version string %@", [self environment], [Updater appVersion]);
        // Ping the update service to see what updates are available
        updateInfoRetrieved = [downloader updateCheckForEnvironment:[self environment] withVersion:[Updater appVersion]];
    }
    
    // Collect information about a possible staged update
    [self findStagedData];
    
    bool showLocalEULA = false;
    int curEULAVersion = [[downloader info] acceptedEulaVersion];
    int eulaLatestMajorVersion = -1;
    
    if(!updateInfoRetrieved || [self useEmbeddedEULA])
    {
        showLocalEULA = true;
        newEulaVersion = [self getLocalEulaVersion];
    }
    else
    {
        newEulaVersion = [[downloader info] eulaVersionAsInt];
        eulaLatestMajorVersion = [[downloader info] eulaLatestMajorVersionAsInt];
    }
    
    ORIGIN_LOG_HELP(@"Current EULA Version is %d, New EULA Version is %d, Major Version is %d", curEULAVersion, newEulaVersion, eulaLatestMajorVersion);

    if(curEULAVersion < newEulaVersion)
    {
        if (!showLocalEULA)
        {
            // K, we got a connection and valid info for the EULA
            
            // Show EULA if
            // 1) installing Origin
            // 2) a major EULA version was published since we last accepted a EULA
            bool isInstalling = curEULAVersion == 0;
            if (eulaLatestMajorVersion > curEULAVersion || isInstalling)
            {
                ORIGIN_LOG_ACTION(@"Displaying major EULA Version %d (%d)", newEulaVersion, eulaLatestMajorVersion);
                if(![appDelegate displayEula:([[downloader info] eulaUrl]) :[Updater appVersion]])
                {
                    // if we failed to load EULA at URL, we fall back to showing the local EULA version
                    // update the new eula version appropriately so the user can't skip the EULA version
                    newEulaVersion = [self getLocalEulaVersion];
                    showLocalEULA = true;
                }
            }
            
            else
            {
                ORIGIN_LOG_ACTION(@"Minor EULA Version %d (%d), continuing update process.", newEulaVersion, eulaLatestMajorVersion);
                // Although we are not presenting the EULA to the user, we still want to store the latest version
                [self postEulaStep:YES];
            }
        }
        
        if (showLocalEULA)
        {
            // Re-check in case we are here because of the failure to load the EULA URL.
            if(curEULAVersion < newEulaVersion)
            {
                ORIGIN_LOG_ACTION(@"Displaying local EULA Version %d", newEulaVersion);
                [appDelegate displayEula:nil :[Updater appVersion]];
            }
            else
            {
                ORIGIN_LOG_ACTION(@"Local EULA Version %d already accepted, continuing update process.", newEulaVersion);
                // We'll get the updated EULA on next launch.
                [self postEulaStep:NO];
            }
        }
    }
    else if(updateInfoRetrieved || [stagedInfo stagedUpdateAvailable])
    {
        // No EULA to present, go straight to post-eula step
        [self postEulaStep:NO];
    }
    else // We did get any update information, EULAs are up to date, attempt to launch what we have, if any
    {
        // Check to be sure we have a valid origin client available
        [self loadOriginClientLib];
        // Tell app delegate we are done
        [appDelegate updateProcessingComplete];
    }
}

-(bool) checkShouldUpdate
{
    // If we started up on 10.6, we need to force an update check that will downgrade us to 9.3.xx since
    // the Origin client isn't compatible with 10.6.
    SystemVersion* osVersion = [SystemVersion defaultSystemVersion];
    if ([osVersion major] == 10 && [osVersion minor] < 7)
        return true;
    
    for(NSString* arg in [[NSProcessInfo processInfo] arguments])
    {
        // We are restarting after an update, but we still need to check just in case.
        if([arg rangeOfString:@"/Installed:"].location != NSNotFound)
        {
            return true;
        }

        if([arg caseInsensitiveCompare:@"-noUpdate"] == 0)
        {
            return false;
        }
        
        if([arg caseInsensitiveCompare:@"-update"] == 0)
        {
            return true;
        }
        
        if([arg caseInsensitiveCompare:@"-useEmbeddedEULA"] == 0)
        {
            return true;
        }
    }
    
#ifdef DEBUG
    return false;
#else
    return true;
#endif
}

-(bool) useEmbeddedEULA
{
    for(NSString* arg in [[NSProcessInfo processInfo] arguments])
    {
        if([arg caseInsensitiveCompare:@"-useEmbeddedEULA"] == 0)
        {
            return true;
        }
    }
    
    return false;
}

typedef int (*startOriginFcn)(int argc, char *argv[]);
typedef const char* (*getOriginVersionFcn)();

-(bool) loadOriginClientLib
{
   if(libOriginClientHandle == nil)
   {
       NSString* libPath = [NSString stringWithFormat:@"%@/Contents/MacOS/OriginClient", [[NSBundle mainBundle] bundlePath]];
       ORIGIN_LOG_ACTION(@"Attempting to load OriginClient lib from %@", libPath);
       libOriginClientHandle = [[NSURL alloc ] initFileURLWithPath:libPath isDirectory:NO];
   }

   haveValidClientLib = (libOriginClientHandle != nil);
   
   if (haveValidClientLib == false)
   {
        ORIGIN_LOG_ERROR(@"Couldn't find a valid OriginClient lib to use");
   }
   
   return haveValidClientLib;
}

-(void) unloadOriginClientLib
{
   if(libOriginClientHandle != nil)
   {
       ORIGIN_LOG_ACTION(@"Closing OriginClient lib.");
       [libOriginClientHandle release];
       libOriginClientHandle = nil;
   }
    
   haveValidClientLib = false;
}

-(NSString*) getOriginClientVersion
{
    if(![self loadOriginClientLib]) 
    {
        ORIGIN_LOG_ERROR(@"Unable to load library, error was: %s.", dlerror());
        return @"";
    }
    else 
    {
        static NSString* version = nil;
        
        if (!version)
        {
            NSDictionary* metaData = (NSDictionary*)CFBundleCopyInfoDictionaryForURL((CFURLRef)libOriginClientHandle);
            
            if (metaData)
            {
                version = [[NSString alloc] initWithString:[metaData valueForKey: (NSString*)kCFBundleVersionKey]];
                [metaData release];
            }
        }
        
        return version;
    }
}

-(void)addClientArg:(NSString *)arg
{
    if(originClientArgs == nil)
    {
        originClientArgs = [[NSMutableArray alloc] initWithCapacity:1];
    }
    
    [originClientArgs addObject:arg];
}

-(NSArray*)getClientArgsUsingLaunchURL:(NSString*)launchUrl forRestart:(BOOL)appRestart
{
    NSMutableArray* cocoaArgs = [[NSMutableArray alloc] initWithArray:[[NSProcessInfo processInfo] arguments]];
    
    // Remove first arg (path to bundle) if we are relaunching, since it will be added
    // with the relaunched command line
    if(appRestart)
    {
        [cocoaArgs removeObjectAtIndex:0];
    }
    
    if(originClientArgs != nil)
    {
        [cocoaArgs addObjectsFromArray:originClientArgs];
    }
    
    // Launch URL has to be last argument, in quotes, if present.
    if(launchUrl != nil && [launchUrl length] > 0)
    {
        [cocoaArgs addObject:[NSString stringWithFormat:@"\"%@\"", launchUrl]];
    }
    
    return cocoaArgs;
}

-(void)logCommandLine:(NSArray*)args restart:(BOOL)restart
{
    NSMutableString* commandLine = [[NSMutableString alloc] init];
    
    [commandLine appendFormat:@"%s client: cmdLine =[%@", (restart ? "Restarting" : "Launching"), [[NSBundle mainBundle] bundlePath]];
    
    for(NSString* arg in args)
    {
        if([arg rangeOfString:@"persona" options:NSCaseInsensitiveSearch].location != NSNotFound ||
           [arg rangeOfString:@"token" options:NSCaseInsensitiveSearch].location != NSNotFound)
        {
#ifdef DEBUG
            [commandLine appendFormat:@" %@", arg];
#endif
        }
        else
        {
            [commandLine appendFormat:@" %@", arg];
        }
    }
    
    [commandLine appendString:@"]"];
    ORIGIN_LOG_EVENT(@"%@", commandLine);
}

-(void)launchOriginClient:(NSString *)launchUrl
{
    // If we just completed an update, we must restart the application
    if([self updated])
    {
        // The shell script waits until the original app process terminates.
        // This is done so that the relaunched app opens as the front-most app.
        int pid = [[NSProcessInfo processInfo] processIdentifier];       
        NSMutableString* launchCommand = [[NSMutableString alloc] init];
        [launchCommand appendFormat:@"open '%@'", [[NSBundle mainBundle] bundlePath]];
        
        NSArray* args = [self getClientArgsUsingLaunchURL:launchUrl forRestart:YES];
        if([args count] > 0)
        {
            [launchCommand appendString:@" --args"];
            
            for(NSString* arg in args)
            {
                [launchCommand appendFormat:@" %@", arg];
            }
        }
        
        [self logCommandLine:args restart:YES];
        
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsentinel"

        NSString *script = [NSString stringWithFormat:@"(while [ `ps -p %d | wc -l` -gt 1 ]; do sleep 0.1; done; %@) &", pid, launchCommand];
        [NSTask launchedTaskWithLaunchPath:@"/bin/sh" arguments:[NSArray arrayWithObjects:@"-c", script, nil]];

#pragma clang diagnostic pop

        exit(0);
    }
    else
    {
        // If we started up on an older OS (e.g. 10.6), we need to avoid starting up the Origin Client since
        // it isn't compatible with 10.6.
        SystemVersion* osVersion = [SystemVersion defaultSystemVersion];
        if ([osVersion major] == 10 && [osVersion minor] < 7)
        {
            ORIGIN_LOG_ERROR(@"Unable to continue Origin due to OS version (%d.%d).", [osVersion major], [osVersion minor]);
            exit(-1);
        }
        
        if(![self DownloadConfiguration])
        {
            NSString* downloadConfigErrorArg = [NSString stringWithFormat:@"/downloadConfigFailed:%@:%d", [self downloadConfigErrorString], [self downloadConfigErrorCode]];
            [self addClientArg:downloadConfigErrorArg];
        }
        else
        {
            NSString* downloadConfigSuccessArg = [NSString stringWithFormat:@"/downloadConfigSuccess:%d", [self downloadConfigRequestDurationMilliseconds]];
            [self addClientArg:downloadConfigSuccessArg];
        }
        
        if(![self loadOriginClientLib]) 
        {
            ORIGIN_LOG_ERROR(@"Unable to load library, error was: %s.", dlerror());
            exit(-1);
        }
        else
        {
            // Check to see if we have been auto-launched by the OS at startup.  If we have then our
            // app will be in the "hidden" state - since we only auto-launch hidden.
            if (WasLaunchedAsLoginOrResumeItem())
            {
                // Add the AutoStart flag to let the client know it was auto-started.
                [self addClientArg:@"-AutoStart"];
                // We only want to start hidden if the user chose to Auto-Update and Auto Login or we did not have an update
                //Check to see if we have been launched due to restart after update.
                bool restarted = false;
                for(NSString* arg in [[NSProcessInfo processInfo] arguments])
                {
                    // We are restarting after an update, no need to update again
                    if([arg rangeOfString:@"/Installed:"].location != NSNotFound)
                    {
                        //If we are here we have been restared after an update
                        restarted = true;
                    }
                }

                // We only want to show hidden after a restart if the user chose to Auto-Update and Auto Login
                if(restarted && ([[downloader info] isAutoUpdate] && [[downloader info] isAutoLogin]))
                {
                    // EBIBUGS-19992: Add flag to prevent login window sometimes appearing.
                    [self addClientArg:@"-StartClientMinimized"];
                }
                // If we have not restarted only show hidden if the user has had no interaction with the bootstrap
                if(!restarted && !declinedUpdate)
                {
                    // EBIBUGS-19992: Add flag to prevent login window sometimes appearing.
                    [self addClientArg:@"-StartClientMinimized"];
                }
            }
            
            // Add the startup time for the benefit of performance measurements
            uint64_t startupTime = [(AppDelegate*)[[NSApplication sharedApplication] delegate] startupTime];
            [self addClientArg:[NSString stringWithFormat:@"-startupTime:%llu", startupTime]];

            // Get the arguments to use when starting the actual client
            NSArray* cocoaArgs = [self getClientArgsUsingLaunchURL:launchUrl forRestart:NO];
            [self logCommandLine:cocoaArgs restart:NO];
            
            NSString* nsExePath = [NSString stringWithFormat:@"%@/Contents/MacOS/OriginClient", [[NSBundle mainBundle] bundlePath]];
            const char* exePath = [nsExePath cStringUsingEncoding:NSUTF8StringEncoding];

            // Simple (= if you really want to hack it you can!) trick to stop a regular user from running the client directly
            char tmpFileName[32];
            snprintf(tmpFileName, sizeof(tmpFileName), "%s", "/var/tmp/tmp.XXXXXXXX");
            int fd = mkstemp(tmpFileName);
            if (fd != -1)
            {
                // And just to be annoying to anybody who's actually going to hack it
                time_t t = time(NULL);
                write(fd, &t, sizeof(t));

                char data[64];
                snprintf(data, sizeof(data), "%d", fd);
                setenv("mac-param", data, 1);
              

                int argc = [cocoaArgs count];
                const char *argv[argc+1];
                
                argv[0] = exePath;
                for(int i = 1; i < argc; i++)
                {
                    NSString* str = (NSString*)[cocoaArgs objectAtIndex:i];
                    argv[i] = [str cStringUsingEncoding:NSUTF8StringEncoding];
                }
                
                argv[argc] = 0;
                int err = execv(exePath, (char * const *)argv);
                if (err == -1) {
                    err = errno;
                    ORIGIN_LOG_ERROR(@"Error sublaunching client: %d", err);
                }
            }
        }
    }
}
    
-(bool)DownloadConfiguration
{
    NSString* fetchUrl;
    
    if ([[self environment] caseInsensitiveCompare:@"production"] == 0)
    {
        fetchUrl = [NSString stringWithFormat:@"https://secure.download.dm.origin.com/production/autopatch/2/init/%@", [Updater appVersion]];
    }
    else
    {
        fetchUrl = [NSString stringWithFormat:@"https://stage.secure.download.dm.origin.com/%@/autopatch/2/init/%@", [self environment], [Updater appVersion]];
    }

	ORIGIN_LOG_EVENT(@"Requesting dynamic url config: %@", fetchUrl);
	
    NSString* errorString = @"SUCCESS";
    int errorCode = 0;
    int requestDurationMilliseconds = 0;
    bool success = [downloader downloadConfigurationData : fetchUrl : [Updater appVersion] : [self environment] : &errorString : &errorCode : &requestDurationMilliseconds];
    
    self.downloadConfigErrorString = errorString;
    self.downloadConfigErrorCode = errorCode;
    self.downloadConfigRequestDurationMilliseconds = requestDurationMilliseconds;
    
    return success;
}

@end

template <class T>
class ScopedNSObject
{
    T* _ref;
    
public:

    ScopedNSObject(T* ref) : _ref(ref) {}
    ScopedNSObject(CFTypeRef ref) : _ref(const_cast<T*>(reinterpret_cast<const T*>(ref))) {}
    ScopedNSObject() { [_ref release]; }
    
    T* get() { return _ref; }
    operator T*() { return _ref; }
};

bool WasLaunchedAsLoginOrResumeItem()
{
    NSString* LOGINWINDOW_CREATOR_TYPE = @"lgnw";
    ProcessSerialNumber psn = { 0, kCurrentProcess };

    ScopedNSObject<NSDictionary> process_info(
        ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask));

    long long temp = [[process_info objectForKey:@"ParentPSN"] longLongValue];
    ProcessSerialNumber parent_psn = {
        static_cast<UInt32>((temp >> 32) & 0x00000000FFFFFFFFLL),
        static_cast<UInt32>(temp & 0x00000000FFFFFFFFLL) };

    ScopedNSObject<NSDictionary> parent_info(
        ProcessInformationCopyDictionary(&parent_psn, kProcessDictionaryIncludeAllInformationMask));

    // Check that creator process code is that of loginwindow.
    BOOL result = [[parent_info objectForKey:@"FileCreator"] isEqualToString:LOGINWINDOW_CREATOR_TYPE];

    return result == YES;
}
