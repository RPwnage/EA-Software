//
//  Updater.h
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//
#import <Foundation/Foundation.h>
#import "Downloader.h"

@class AppDelegate;

// StagedDataInfo is populated by the updater based on whether it finds
// a staged download .zip already in the bundle file.  Generally this staged
// zip is downloaded by the OriginClient when it detects an update so it is
// available on next launch.  
@interface StagedDataInfo : NSObject {
    bool stagedUpdateAvailable;
    bool matchesAutoPatchVersion;
    NSString* version;
    NSString* filePath;
}

@property bool stagedUpdateAvailable;
@property bool matchesAutoPatchVersion;
@property (retain) NSString* version;
@property (retain) NSString* filePath;

@end

typedef enum
{
    UPDATE_SOURCE_STAGED,
    UPDATE_SOURCE_DOWNLOAD
} UpdateSource;

// The Update class contains most of the update logic for the bootstrapper.  It presents and interacts
// with the user via the AppDelegate class.  It interacts with the update server and user settings via
// the Downloader class, and detect the presence of staged downloads and check if any network updates
// are available.  If a new EULA is available on either the server or in the bundle, it will present that
// as well.
@interface Updater : NSObject {
    
    UpdateSource currentUpdateSource;
    
    bool updated;
    bool isOfflineMode;
    bool isDownloadSuccessful;
    bool haveValidClientLib;
    bool isDataError;
    bool bypassOverrideEnabled;
    bool declinedUpdate;
    
    int downloadedBytes;
    
    int newEulaVersion;
    NSString* environment;
    
    NSString* updateVersion;
    
    NSString* downloadConfigErrorString;
    int downloadConfigErrorCode;
    int downloadConfigRequestDurationMilliseconds;
    
    Downloader* downloader;
    StagedDataInfo* stagedInfo;
    AppDelegate* appDelegate;
    
    NSMutableArray* originClientArgs;
    NSURL* libOriginClientHandle;
}

@property (retain) Downloader* downloader;
@property (retain) StagedDataInfo* stagedInfo;
@property (assign) NSMutableArray* originClientArgs;

@property UpdateSource currentUpdateSource;
@property bool updated;
@property bool isOfflineMode;
@property bool isDownloadSuccessful;
@property bool haveValidClientLib;
@property bool isDataError;
@property bool bypassOverrideEnabled;
@property bool declinedUpdate;

@property (assign) NSURL* libOriginClientHandle;

@property int newEulaVersion;
@property (retain) NSString* environment;
@property (retain) NSString* updateVersion;

@property (retain) NSString* downloadConfigErrorString;
@property int downloadConfigErrorCode;
@property int downloadConfigRequestDurationMilliseconds;

@property(nonatomic,assign) int downloadedBytes;
@property (assign) AppDelegate* appDelegate;

+(NSString*) appVersion;
+(uint64_t) time;

// Called to start the update process by the main application start method
// in AppDelegate.
-(void) doUpdates:(AppDelegate*) delegate;

// Checks command line arguments to determine if an update should be ignored (/noUpdate)
// or forced (/update).  If in DEBUG mode this will return false until /update is specified.
// In RELEASE it will return true until /noUpdate is specified.
-(bool) checkShouldUpdate;

// Debug flag to force the use of embedded EULAs.
-(bool) useEmbeddedEULA;

// Called by AppDelegate after the user accepts the presented EULA.
-(void) eulaAccepted;

// Called by AppDelegate after the user clicks "Install Update" in Update dialog.
-(void) installUpdate;

// Loads the originClient.dylib and determines its current version.  If no originClient.dylib can
// be loaded from application bundle, this will return an empty string.
-(NSString*) getOriginClientVersion;

// Adds argument to set of arguments to be sent to the Origin client when launched via launchOriginClient
-(void)addClientArg:(NSString*)arg;

// Returns array with all command line arguments, which is a combination of arguments passed to bootstrap,
// those created during the update process, and possibly the launch URL from an origin:// launch.
-(NSArray*)getClientArgsUsingLaunchURL:(NSString*)launchUrl forRestart:(BOOL)restarting;

// Loads the OriginClient.dylib and launches the Origin client with the OriginApplicationStart method.
-(void)launchOriginClient:(NSString*)launchUrl;

// Fetches dynamic URLs
-(bool)DownloadConfiguration;

@end
