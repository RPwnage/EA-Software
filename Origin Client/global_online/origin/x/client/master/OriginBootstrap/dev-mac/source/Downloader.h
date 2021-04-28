//
//  Downloader.h
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

// DownloaderInfo contains all information queried from the update service
// about available updates, as well as information garnered from the user
// settings such as last accepted EULA version, and whether the user has
// auto updates selected and opted in for Betas.
@interface DownloaderInfo : NSObject
{
    NSString* url;
    NSString* versionNumber;
    NSString* type;
    NSString* eulaUrl;
    NSString* eulaVersion;
    NSString* eulaLatestMajorVersion;
    NSString* updateRule;
    NSString* tempFilename;
    NSString* outputFilename;
    
    bool isBetaOptIn;
    bool isAutoUpdate;
    bool isAutoLogin;
    bool userCancelledDownload;
    bool internetConnection;
    bool serviceAvailable;
    
    int acceptedEulaVersion;
    bool dataError;
    
    unsigned long long downloadSizeBytes;
}

@property (retain) NSString* url;
@property (retain) NSString* versionNumber;
@property (retain) NSString* type;
@property (retain) NSString* eulaUrl;
@property (retain) NSString* eulaVersion;
@property (retain) NSString* eulaLatestMajorVersion;
@property (retain) NSString* updateRule;
@property (retain) NSString* outputFilename;
@property (retain) NSString* tempFilename;

@property bool isBetaOptIn;
@property bool isAutoUpdate;
@property bool isAutoLogin;
@property bool userCancelledDownload;
@property bool internetConnection;
@property bool serviceAvailable;
@property int acceptedEulaVersion;
@property bool dataError;
@property unsigned long long downloadSizeBytes;

-(int) eulaVersionAsInt;
-(int) eulaLatestMajorVersionAsInt;
-(bool) isOptionalUpdate;

@end

// Downloader class has logic to query the user settings and update service.
// It also has the logic to download the update zip.  If EACore.ini is present
// with a [Bootstrap] section with all necessary items populated, it will populate
// itself from there instead of the update service.
@interface Downloader : NSObject
{
@private
    DownloaderInfo* info;
    NSURLDownload* theDownload;
    NSString* urlOverride;
}

@property (retain) DownloaderInfo* info;
@property (retain) NSURLDownload* theDownload;
@property (retain) NSString* urlOverride;

+(NSMutableURLRequest*) buildUrlRequestWithOriginHeaders:(NSURL*)url :(NSString*)originVersion;

// Retrieve settings from EACore.ini for the given environment
-(void) querySettingsFile:(NSString*)environment;

// cancels the download if in progress
-(void) cancelDownload;

// updateCheck queries the update service for the specified environment and version,
// and populates info with the response.
-(bool) updateCheckForEnvironment:(NSString*)environment withVersion:(NSString*)version;

// useDebugResponse populates the info data with the specified information provided.
-(void) useDebugResponseWithVersion:(NSString*)version type:(NSString*)updateType downloadLocation:(NSString*)downloadLocation rule:(NSString*)rule eulaVersion:(NSString*)eulaVersion eulaLocation:(NSString*)eulaLocation eulaLatestMajorVersion:(NSString*)eulaLatestMajorVersion;

// beginDownload starts the download from the specified URL, the version is used to build the 
// correct User-Agent string.  To receive updates on download progress, the downloadDelegate 
// class provided should implement:
//      -(void) downloadDidBegin:(NSURLDownload*)download;
//      -(void) downloadDidFinish:(NSURLDownload*)download;
//      -(void) download:(NSURLDownload*)download didReceiveDataOfLength:(NSUInteger)length;
//      -(void) download:(NSURLDownload*)download didFailWithError:(NSError *)error;
//
-(bool) beginDownload:(NSURL*)url :(NSString*)currentVersion :(id)downloadDelegate;

// When the downloadDelegate receives downloadDidFinish, it calls finalizeDownload
// to move the temporary .part download file over to the staged .zip update file.
-(bool) finalizeDownload;

// This method removes any partial downloads found in the root of the bundle directory
-(void) clearPartialDownloads;

// This method updates or creates the global settings file with the provided information.
-(void) writeEulaSettings:(NSString*)eulaVersion :(NSString*)eulaLocation;

// This method updates or creates the global settings file with the provide information.
-(void) writeAutoUpdateSetting:(BOOL)autoUpdate;

// This method downloads the client's configuration data
-(bool) downloadConfigurationData:(NSString*)url :(NSString*)appVersion :(NSString*) environment :(NSString**) errorString :(int*) errorCode :(int*) requestDuration;

@end
