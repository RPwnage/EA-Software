//
//  Downloader.m
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "Downloader.h"
#import "Locale.h"
#import "LogServiceWrapper.h"
#import "Utilities.h"

// Variant type string values that match
// Variant::Type enum in Origin::Services::Variant
#define VARIANT_TYPE_INVALID "0"
#define VARIANT_TYPE_BOOL "1"
#define VARIANT_TYPE_INT "2"
#define VARIANT_TYPE_ULONGLONG "5"
#define VARIANT_TYPE_STRING "10"

// Settings path
// TODO: Move to /Library/Application Support/Origin/local.xml once escalation issues are worked out
#define GLOBAL_SETTINGS_PATH "~/Library/Application Support/Origin/local.xml"

BOOL checkAndUpdatePermissions();

@implementation DownloaderInfo

@synthesize url;
@synthesize versionNumber;
@synthesize type;
@synthesize eulaUrl;
@synthesize eulaVersion;
@synthesize eulaLatestMajorVersion;
@synthesize updateRule;
@synthesize outputFilename;
@synthesize tempFilename;

@synthesize isBetaOptIn;
@synthesize isAutoUpdate;
@synthesize isAutoLogin;
@synthesize userCancelledDownload;
@synthesize internetConnection;
@synthesize serviceAvailable;

@synthesize downloadSizeBytes;

@synthesize acceptedEulaVersion;
@synthesize dataError;

- (id)init
{
    self = [super init];
    if(self) {
        url = @"";
        versionNumber = @"";
        type = @"";
        eulaUrl = @"";
        eulaVersion = @"";
        eulaLatestMajorVersion = @"";
        updateRule = @"";
        outputFilename = @"";
        tempFilename = @"";
        
        isBetaOptIn = false;
        isAutoUpdate = false;
        isAutoLogin = false;
        userCancelledDownload = false;
        internetConnection = true;
        serviceAvailable = true;
        acceptedEulaVersion = 0;
        dataError = false;
    }
    
    return self;
}

- (void) dealloc
{
    [url release];
    [versionNumber release];
    [type release];
    [eulaUrl release];
    [eulaVersion release];
    [eulaLatestMajorVersion release];
    [updateRule release];
    [outputFilename release];
    [tempFilename release];
    [super dealloc];
}

- (int) eulaVersionAsInt
{
    NSScanner* scan = [NSScanner scannerWithString:eulaVersion];
    int value;
    if([scan scanInt:&value] && [scan isAtEnd])
    {
        return value;
    }
    else
    {
        ORIGIN_LOG_ERROR(@"Failed to parse EULA version string %@", eulaVersion);
        return 0;
    }
}

- (int) eulaLatestMajorVersionAsInt
{
    NSScanner* scan = [NSScanner scannerWithString:eulaLatestMajorVersion];
    int value;
    if([scan scanInt:&value] && [scan isAtEnd])
    {
        return value;
    }
    else
    {
        ORIGIN_LOG_ERROR(@"Failed to parse EULA latest major version string %@", eulaLatestMajorVersion);
        return 0;
    }
}

- (bool) isOptionalUpdate
{
    return ([updateRule caseInsensitiveCompare:@"OPTIONAL"] == 0);
}

@end

/**
 Private downloader methods
 */
@interface Downloader (hidden)

// This method constructs the URL to query the update service for available update data, using
// the specified environment, version, and type (PROD/BETA)
-(NSString*) getUpdateServiceURLForEnvironment:(NSString*)environment andVersion:(NSString*)version ofType:(NSString*)type;

// This method parses the XML returned by the update service and populates the DownloaderInfo
// class with the parsed data.  If parsing fails, [info dataError] will return true, otherwise
// false.
-(void) parseUpdateServiceXml:(NSXMLDocument*)xml;

// Helper method to parse XML, given the XPATH returns the string value for the element
-(NSString*)getStringFrom:(NSXMLDocument*)xml forNodePath: (NSString*)nodePath;

// Helper method to parse settings XML, given the XPATH finds the node and returns attribute specified for element as a string
-(NSString*)getAttributeValueFrom:(NSXMLDocument*)xml forNodePath:(NSString*)nodePath andAttributeName:(NSString*)attributeName;

// Helper method that parses the numbers out of a '.' delimited version string as individual integers in an NSArray
-(NSArray*)parseVersionNumbers:(NSString*)version;

// Attempts to open and parse Origin global settings file (/Library/Application Support/Origin/local.xml) and 
// find the following information:
//   * Accepted EULA Version
//   * Whether user has selected Automatic updates
//   * Whether user has opted in for Beta updates
-(void)querySettingsFile;

// Helper method to open the global settings file and return as NSXMLDocument, used for both query and write settings
-(NSXMLDocument*)loadSettingsFile;

// Helper method to add the specified setting to the global local.xml file.  If local.xml already contains setting it
// will be updated with the new value, if it does not a new settings node will be created.
-(void)addOrUpdateSetting:(NSXMLElement*)settingsNode :(NSString*)settingKey :(NSString*)settingValue :(NSString*)settingType;

@end

@implementation Downloader

@synthesize info;
@synthesize theDownload;
@synthesize urlOverride;

- (id)init
{
    self = [super init];
    if(self) {
        info = [[DownloaderInfo alloc] init];
        theDownload = nil;
        urlOverride = nil;
    }
    
    return self;
}

- (void) dealloc
{
    [info release];
    [theDownload release];
    [super dealloc];
}

-(NSString*)getAttributeValueFrom:(NSXMLDocument*)xml forNodePath:(NSString*)nodePath withAttributeName:(NSString*)attributeName
{
    NSError* error = nil;
    NSArray* nodes = [xml nodesForXPath:nodePath error:&error];
    if(nodes == nil || [nodes count] == 0)
    {
        return @"";
    }
    else {
        return [[(NSXMLElement*)[nodes objectAtIndex:0] attributeForName:attributeName] stringValue];
    }
}

-(void) cancelDownload
{
    if(theDownload != nil)
    {
        [theDownload cancel];
    }
}

-(NSXMLDocument*)loadSettingsFile
{
    NSError* error = nil;
    NSString* settingsPath = [@GLOBAL_SETTINGS_PATH stringByExpandingTildeInPath];
    NSString* fullUrlStr = [NSString stringWithFormat:@"file://%@", settingsPath];
    NSURL* settingsXmlURL = [NSURL URLWithString:[fullUrlStr stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    NSXMLDocument* settingsXmlDocument = [[[NSXMLDocument alloc]initWithContentsOfURL:settingsXmlURL options:(NSXMLNodePreserveWhitespace|NSXMLNodePreserveCDATA) error:&error] autorelease];
    return settingsXmlDocument;
}

-(void)querySettingsFile:(NSString*)environment
{
    // We have to read the following information from user's settings
    // isAutoUpdate - "AutoUpdate" value="true"
    // isBetaOptIn = "BetaOptIn" value="true"
    // currentEulaVersion = "AcceptedEULAVersion" value="int"
    NSXMLDocument* settingsXmlDocument = [self loadSettingsFile];
    
    if(settingsXmlDocument)
    {
        /* Settings local.xml looks like:
         <Settings>
         <Setting key="AutoUpdate" type="1" value="true"/>
         <Setting key="BetaOptIn" type="1" value="true"/>
         <Setting key="AcceptedEULAVersion" value="1"/>
         @"./Settings/Setting[@key='AutoUpdate']"
         */
        [info setIsAutoUpdate:[[self getAttributeValueFrom:settingsXmlDocument forNodePath:@"./Settings/Setting[@key='AutoUpdate']" withAttributeName:@"value"] compare:@"true"] == 0];
        [info setIsBetaOptIn:[[self getAttributeValueFrom:settingsXmlDocument forNodePath:@"./Settings/Setting[@key='BetaOptIn']" withAttributeName:@"value"] compare:@"true"] == 0];
        
        uint size = 0;
        if ([environment compare:@"production"] == 0)
        {
            NSString* RememberMeProd = [self getAttributeValueFrom:settingsXmlDocument forNodePath:@"./Settings/Setting[@key='RememberMeProd']" withAttributeName:@"value"];
            size = RememberMeProd.length;
        }
        else
        {
            NSString* RememberMeInt = [self getAttributeValueFrom:settingsXmlDocument forNodePath:@"./Settings/Setting[@key='RememberMeInt']" withAttributeName:@"value"];
            size = RememberMeInt.length;
        }
        // If the string size is larger that 16 it means we 'should' have a valid cookie saves for auto start.  16 is the size of the encrypted string 'invalid'.
        // If not we can assume that the cookie is either invalid or the user has never selected auto-login
        [info setIsAutoLogin: (size > 16) ? true : false];
        
        NSString* acceptedEulaVersionString = [self getAttributeValueFrom:settingsXmlDocument forNodePath:@"./Settings/Setting[@key='AcceptedEULAVersion']" withAttributeName:@"value"];
        
        if([acceptedEulaVersionString length] > 0)
        {
            NSScanner* scan = [NSScanner scannerWithString:acceptedEulaVersionString];
            int value;
            if([scan scanInt:&value] && [scan isAtEnd])
            {
                [info setAcceptedEulaVersion:value];
            }
            else
            {
                ORIGIN_LOG_ERROR(@"Failed to parse accepted EULA version string %@", acceptedEulaVersionString);
                
            }
        }
        
        ORIGIN_LOG_EVENT(@"AutoUpdate value = %@", [info isAutoUpdate] ? @"true" : @"false");
        ORIGIN_LOG_EVENT(@"AutoLogin value = %@", [info isAutoLogin] ? @"true" : @"false");
        ORIGIN_LOG_EVENT(@"BetaOptIn value = %@", [info isBetaOptIn] ? @"true" : @"false");
        ORIGIN_LOG_EVENT(@"AcceptedEULAVersion value = %d", [info acceptedEulaVersion]);
    }
    else
    {
        // This must be a fresh install if no settings file exists
        ORIGIN_LOG_EVENT(@"Could not find settings file, treating as fresh install...");

        [info setIsBetaOptIn:false];
        [info setIsAutoUpdate:false];
        [info setIsAutoLogin: false];
        [info setAcceptedEulaVersion:0];
    }
}

-(void)addOrUpdateSetting:(NSXMLElement*)settingsNode :(NSString*)settingKey :(NSString*)settingValue :(NSString*)settingType
{
    NSArray* nodes = [settingsNode nodesForXPath:[NSString stringWithFormat:@"./Setting[@key='%@']", settingKey] error:nil];
    
    if(nodes == nil || [nodes count] == 0)
    {
        // We  have to create the settings node
        NSXMLElement* newSetting = (NSXMLElement*)[NSXMLNode elementWithName:@"Setting"];
        [newSetting addAttribute:[NSXMLNode attributeWithName:@"key" stringValue:settingKey]];
        [newSetting addAttribute:[NSXMLNode attributeWithName:@"value" stringValue:settingValue]];
        [newSetting addAttribute:[NSXMLNode attributeWithName:@"type" stringValue:settingType]];
        [settingsNode addChild:newSetting];
    }
    else // Update the value attribute 
    {
        NSXMLElement* elem = (NSXMLElement*)[nodes objectAtIndex:0];
        NSXMLNode* valueAttrNode = [elem attributeForName:@"value"];
        if(valueAttrNode)
        {
            [valueAttrNode setStringValue:settingValue];
        }
        else 
        {
            [elem addAttribute:[NSXMLNode attributeWithName:@"value" stringValue:settingValue]]; 
        }
        NSXMLNode* typeAttrNode = [elem attributeForName:@"type"];
        if(typeAttrNode)
        {
            [typeAttrNode setStringValue:settingType];
        }
        else 
        {
            [elem addAttribute:[NSXMLNode attributeWithName:@"type" stringValue:settingType]]; 
        }
    }
}

-(void)writeEulaSettings:(NSString*)eulaVersion :(NSString*)eulaLocation
{
    NSXMLDocument* settingsXml = [self loadSettingsFile];

    if(!settingsXml)
    {
        // Create an empty settings document here
        NSXMLElement* root = (NSXMLElement*)[NSXMLNode elementWithName:@"Settings"];
        settingsXml = [[[NSXMLDocument alloc] initWithRootElement:root] autorelease];
        [settingsXml setVersion:@"1.0"];
        [settingsXml setCharacterEncoding:@"UTF-8"];
    }

    if(settingsXml)
    {
        NSError* error = nil;
        NSArray* nodes = [settingsXml nodesForXPath:@"./Settings" error:&error];
        
        if(nodes == nil || [nodes count] == 0)
        {
            ORIGIN_LOG_ERROR(@"Invalid settings file, could not find Settings element.");
            return;
        }
        
        NSXMLElement* settingsElement = (NSXMLElement*)[nodes objectAtIndex:0];

        [self addOrUpdateSetting:settingsElement :@"AcceptedEULAVersion" :eulaVersion :@VARIANT_TYPE_INT];
        [self addOrUpdateSetting:settingsElement :@"AcceptedEULALocation" :eulaLocation :@VARIANT_TYPE_STRING];
        
        NSData* xmlData = [settingsXml XMLDataWithOptions:NSXMLNodePrettyPrint];
        [xmlData writeToFile:[@GLOBAL_SETTINGS_PATH stringByExpandingTildeInPath] atomically:YES];
    }
    else
    {
        ORIGIN_LOG_ERROR(@"Failed to write EULA information out to settings XML file.");
    }
}

-(void)writeAutoUpdateSetting:(BOOL)autoUpdateEnabled
{
    NSXMLDocument* settingsXml = [self loadSettingsFile];
    
    if(!settingsXml)
    {
        // Create an empty settings document here
        NSXMLElement* root = (NSXMLElement*)[NSXMLNode elementWithName:@"Settings"];
        settingsXml = [[[NSXMLDocument alloc] initWithRootElement:root] autorelease];
        [settingsXml setVersion:@"1.0"];
        [settingsXml setCharacterEncoding:@"UTF-8"];
    }
    
    if(settingsXml)
    {
        NSError* error = nil;
        NSArray* nodes = [settingsXml nodesForXPath:@"./Settings" error:&error];
        
        if(nodes == nil || [nodes count] == 0)
        {
            ORIGIN_LOG_ERROR(@"Invalid settings file, could not find Settings element.");
            return;
        }
        
        NSXMLElement* settingsElement = (NSXMLElement*)[nodes objectAtIndex:0];
        
        [self addOrUpdateSetting:settingsElement :@"AutoUpdate" :(autoUpdateEnabled ? @"true" : @"false") :@VARIANT_TYPE_BOOL];
        
        NSData* xmlData = [settingsXml XMLDataWithOptions:NSXMLNodePrettyPrint];
        [xmlData writeToFile:[@GLOBAL_SETTINGS_PATH stringByExpandingTildeInPath] atomically:YES];
    }
    else
    {
        ORIGIN_LOG_ERROR(@"Failed to write auto update information out to settings XML file.");
    }
}

+(NSMutableURLRequest*) buildUrlRequestWithOriginHeaders:(NSURL *)url :(NSString *)originVersion
{
    NSMutableURLRequest* request=[NSMutableURLRequest requestWithURL:url];
    [request setValue:[NSString stringWithFormat:@"Mozilla/5.0 EA Download Manager Origin/%@", originVersion] forHTTPHeaderField:@"User-Agent"];
    [request setValue:@"MAC" forHTTPHeaderField:@"X-Origin-Platform"];
    // Make sure the local cache is always ignored.  Server can still proxy/cache all it wants.
    [request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];
    
    // Since we make a blocking request, set the timeout interval to a limited
    // period of time to prevent the user from waiting too long to access the
    // Origin client (EBIBUGS-26188).
    static const NSTimeInterval TIMEOUT = 5.0;
    [request setTimeoutInterval:TIMEOUT];
    
    return request;
}

-(bool)updateCheckForEnvironment:(NSString *)environment withVersion:(NSString *)version
{
    NSString* type;
    
    if([info isBetaOptIn])
    {
        type = @"BETA";
    }
    else 
    {
        type = @"PROD";
    }
    
    // Retrieve the update XML from the server
    NSURL* updateXmlServiceURL = [NSURL URLWithString:[self getUpdateServiceURLForEnvironment:environment andVersion:version ofType:type]];
        
    NSMutableURLRequest* request = [Downloader buildUrlRequestWithOriginHeaders:updateXmlServiceURL :version];
    NSHTTPURLResponse* response;
    NSError* error = nil;

    bool result = false;
    NSData* xmlData = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];   
        
    if([response statusCode] != 200 || xmlData == nil)
    {
        ORIGIN_LOG_ERROR(@"Failed to retrieve update information from update service [statusCode = %d]", [response statusCode]);
        if(error != nil && [error domain] != nil && [[error domain] compare:NSURLErrorDomain] == 0)
        {
            switch([error code])
            {
                case NSURLErrorNotConnectedToInternet:
                    [info setInternetConnection:false];
                    [info setServiceAvailable:false];
                    break;
                default:
                    [info setInternetConnection:true];
                    [info setServiceAvailable:false];
                    break;
            }
        }
        else 
        {
            [info setServiceAvailable:false];
            [info setDataError:true];
        }
    }
    else 
    {
#ifdef DEBUG
        // A little debugging information
        //[xmlData writeToFile:[@"~/updateResponse.xml" stringByExpandingTildeInPath] atomically:YES];
#endif
        ORIGIN_LOG_EVENT(@"Update query connection successful.");
        
        NSXMLDocument* updateXmlDocument = [[NSXMLDocument alloc]initWithData:xmlData options:(NSXMLNodePreserveWhitespace|NSXMLNodePreserveCDATA) error:&error];
        
        // If server returned an XML document, parse it into DownloaderInfo
        if(updateXmlDocument)
        {
            // Clear data error flag, if parseUpdateServiceXMl fails in any way it will be set to true
            [info setDataError:false];
            [info setServiceAvailable:true];
            [info setInternetConnection:true];
            [self parseUpdateServiceXml:updateXmlDocument];
            [updateXmlDocument release];
            result = ![info dataError];
        }
        else if(error)
        {
            ORIGIN_LOG_ERROR(@"Failed to retrieve update information from update service.");
            
            if([[error domain] compare:NSURLErrorDomain] == 0)
            {
                switch([error code])
                {
                    case NSURLErrorNotConnectedToInternet:
                        [info setInternetConnection:false];
                        [info setServiceAvailable:false];
                        break;
                    default:
                        [info setInternetConnection:true];
                        [info setServiceAvailable:false];
                        break;
                }
            }
            else 
            {
                [info setServiceAvailable:false];
                [info setDataError:true];
            }
        }
    }

    return result;
}

-(void) useDebugResponseWithVersion:(NSString*)version type:(NSString*)updateType downloadLocation:(NSString*)downloadLocation rule:(NSString*)rule eulaVersion:(NSString*)eulaVersion eulaLocation:(NSString*)eulaLocation eulaLatestMajorVersion:(NSString*)eulaLatestMajorVersion
{
    [info setInternetConnection:true];
    [info setServiceAvailable:true];
    [info setVersionNumber:version];
    [info setType:updateType];
    [info setUrl:downloadLocation];
    [info setUpdateRule:rule];
    [info setEulaVersion:eulaVersion];
    [info setEulaLatestMajorVersion:eulaLatestMajorVersion];
    [info setEulaUrl:eulaLocation];
}

-(NSString*)getUpdateServiceURLForEnvironment:(NSString*)environment andVersion:(NSString*)version ofType:(NSString*)type
{
    NSString* updateURL;
    // Make sure to use the reference PC naming convention (region-based locale filenames)
    NSString* locale = [Locale regionBasedLocale];
    
    SystemVersion* systemVersion = [SystemVersion defaultSystemVersion];
    NSString* osVersion = [NSString stringWithFormat:@"%d.%d.%d", [systemVersion major], [systemVersion minor], [systemVersion bugFix]];
    
    ORIGIN_LOG_DEBUG(@"Origin running on Mac OS Version: %@", osVersion);
    
    NSString* formatString;
    
    if([self urlOverride] == nil)
    {
        if ([environment caseInsensitiveCompare:@"production"] == 0)
        {
            formatString = [NSString stringWithFormat:@"https://secure.download.dm.origin.com/autopatch/2/upgradeFrom/%%@/%%@/%%@?platform=MAC&osVersion=%@", osVersion];
        }
        else
        {
            formatString = [NSString stringWithFormat:@"https://stage.secure.download.dm.origin.com/%@/autopatch/2/upgradeFrom/%%@/%%@/%%@?platform=MAC&osVersion=%@", environment, osVersion];
        }    
    }
    else 
    {
        formatString = [NSString stringWithFormat:@"%@/%%@/%%@/%%@", [self urlOverride]];
    }
    
    updateURL = [NSString stringWithFormat:formatString, version, locale, type];
    
    ORIGIN_LOG_DEBUG(@"UpdateURL: %@", updateURL);
    
    return updateURL;
}

-(NSString*)getStringFrom:(NSXMLDocument*)xml forNodePath:(NSString*)nodePath
{
    NSError* error = nil;
    NSArray* nodes = [xml nodesForXPath:nodePath error:&error];
    if(nodes == nil || [nodes count] == 0)
    {
        ORIGIN_LOG_ERROR(@"Failed to parse update XML, could not find xpath [%@]", nodePath);
        [info setDataError:true];
        return nil;
    }
    else {
        return [(NSXMLElement*)[nodes objectAtIndex:0] stringValue];
    }
}

-(void)parseUpdateServiceXml:(NSXMLDocument *)xml
{
    /* Current Update XML Format
     <upgradeTo>
     <version>9.0.2.2064</version>
     <buildType>PROD</buildType>
     <downloadURL>
     https://download.dm.origin.com/origin/live/OriginUpdate_9_0_2_2064.zip
     </downloadURL>
     <updateRule>REQUIRED</updateRule>
     <eulaLocale>en_US</eulaLocale>
     <eulaVersion>1</eulaVersion>
     <eulaURL>https://download.dm.origin.com/eula/en_US.html</eulaURL>
     </upgradeTo>
     */
    [info setVersionNumber:[self getStringFrom:xml forNodePath:@"./upgradeTo/version"]];
    [info setType:[self getStringFrom:xml forNodePath:@"./upgradeTo/buildType"]];
    [info setUrl:[self getStringFrom:xml forNodePath:@"./upgradeTo/downloadURL"]];
    [info setUpdateRule:[self getStringFrom:xml forNodePath:@"./upgradeTo/updateRule"]];
    [info setEulaVersion:[self getStringFrom:xml forNodePath:@"./upgradeTo/eulaVersion"]];
    [info setEulaUrl:[self getStringFrom:xml forNodePath:@"./upgradeTo/eulaURL"]];
    [info setEulaLatestMajorVersion:[self getStringFrom:xml forNodePath:@"./upgradeTo/eulaLatestMajorVersion"]];
}

-(NSArray*)parseVersionNumbers:(NSString*)version;
{
    NSArray* versionComponentStrings = [version componentsSeparatedByString:@"."];
    NSMutableArray* versionComponents = [[[NSMutableArray alloc] initWithCapacity:[versionComponentStrings count]] autorelease];
    
    int i;
    for(i = 0; i < [versionComponentStrings count]; i++)
    {
        [versionComponents addObject:[NSNumber numberWithInt:[[versionComponentStrings objectAtIndex:i] intValue]]];
    }
   
    return versionComponents;
}

-(void)clearPartialDownloads
{
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    NSPredicate* filter = [NSPredicate predicateWithFormat:@"self BEGINSWITH 'OriginUpdate' AND self ENDSWITH '.part'"];
    NSArray* partialDownloads = [[[NSFileManager defaultManager] contentsOfDirectoryAtPath:bundlePath error:nil] filteredArrayUsingPredicate:filter];
    
    for(NSString* partialDownloadFile in partialDownloads)
    {
        NSString* partialPath = [NSString stringWithFormat:@"%@/%@", bundlePath, partialDownloadFile];
        ORIGIN_LOG_EVENT(@"Removing partial download %@", partialPath);
        [[NSFileManager defaultManager] removeItemAtPath:partialPath error:nil];
    }
}

-(bool)finalizeDownload
{
    return [[NSFileManager defaultManager] moveItemAtPath:[info tempFilename] toPath:[info outputFilename] error:nil];
}

-(bool)beginDownload:(NSURL*)url :(NSString*) currentVersion :(id)downloadDelegate
{
    // Check and expand the permissions of the bundle, if necessary to allow updating.
    checkAndUpdatePermissions();
    
    NSMutableURLRequest* request = [Downloader buildUrlRequestWithOriginHeaders:url :currentVersion];
   
    theDownload = [[NSURLDownload alloc] initWithRequest:request delegate:downloadDelegate];
    
    if(theDownload) {
        NSArray* version = [self parseVersionNumbers:[info versionNumber]];
        
        NSString* downloadPath = [[NSBundle mainBundle] bundlePath];
        
        [info setOutputFilename:[NSString stringWithFormat:@"%@/OriginUpdate_%@_%@_%@_%@.zip", 
            downloadPath, [version objectAtIndex:0], [version objectAtIndex:1], [version objectAtIndex:2], [version objectAtIndex:3]]];
        [info setTempFilename:[NSString stringWithFormat:@"%@.part", [info outputFilename]]];

        ORIGIN_LOG_ACTION(@"Downloading update to %@ to %@", [info outputFilename], [info tempFilename]);
        
        [theDownload setDestination:[info tempFilename] allowOverwrite:true];
        [theDownload setDeletesFileUponFailure:true];
        return true;
    }
    else {
        ORIGIN_LOG_ERROR(@"Failed to create the download.");
        ORIGIN_LOG_DEBUG(@"Download URL was [%@]", url);
        return false;
    }
}


-(bool)downloadConfigurationData:(NSString*) url :(NSString*) appVersion :(NSString*) environment :(NSString**) errorString :(int*) errorCode :(int*) requestDuration
{
    bool result = false;
    
    checkAndUpdatePermissions();
    
    NSMutableURLRequest* request = [Downloader buildUrlRequestWithOriginHeaders
                                    :[NSURL URLWithString: url] 
                                    : appVersion ];
    
    NSHTTPURLResponse* response;
    NSError* error = nil;
    
    NSTimeInterval startRequest = [NSDate timeIntervalSinceReferenceDate];
    NSData* configData = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
    
    // Report duration in milliseconds
    static const double SEC_TO_MILLISECONDS_MULTIPLIER = 1000.0;
    *requestDuration = ([NSDate timeIntervalSinceReferenceDate] - startRequest) * SEC_TO_MILLISECONDS_MULTIPLIER;
    
    int status = [response statusCode];    
    NSUInteger configLength = [configData length];
    *errorCode = error.code;
    
    if (status == 200 && configLength > 0)
    {
        NSString *bundleDirectory = [[NSBundle mainBundle] resourcePath];
        NSString *configPath = [NSString stringWithFormat:@"%@/%@.wad", bundleDirectory, [environment stringByReplacingOccurrencesOfString:@"-" withString:@"_"]];
        
        ORIGIN_LOG_ACTION(@"Downloading config to: %@", configPath);

        if([[NSFileManager defaultManager] fileExistsAtPath:configPath] )
        {       
            NSError *deleteError = nil;
            if ([[NSFileManager defaultManager] isDeletableFileAtPath: configPath] && [[NSFileManager defaultManager] removeItemAtPath: configPath error: &deleteError])
            {
                if (![[NSFileManager defaultManager] createFileAtPath:configPath contents:configData attributes:nil])
                {
                    ORIGIN_LOG_ERROR(@"Failed to create config response cache");
                    *errorString = @"FailCreateConfigResponseCache";
                }
                else
                {
                    ORIGIN_LOG_EVENT(@"Successfully downloaded config.");
                    result = true;
                }
            }
            else
            {
                NSString *errStr = @"unknown reason";
                
                if (deleteError)
                {
                    errStr = [deleteError localizedFailureReason];
                }
                
                ORIGIN_LOG_ERROR(@"Failed to remove pre-existing config response cache: %@", errStr);
                *errorString = @"FailRemovePreExistingConfigResponseCache";
            }
        }
        else 
        {
            if (![[NSFileManager defaultManager] createFileAtPath:configPath contents:configData attributes:nil])
            {
                ORIGIN_LOG_ERROR(@"Failed to create config response cache");
                *errorString = @"FailCreateConfigResponseCache";
            }
            else
            {
                ORIGIN_LOG_EVENT(@"Successfully downloaded config.");
                result = true;
            }
        }
    }
    else
    {
        ORIGIN_LOG_ACTION(@"Init response failed or empty, client falling back on old cache...");
        *errorString = @"ResponseFailOrEmpty";
    }
    
    return result;
}

@end

BOOL checkAndUpdatePermissions()
{
    // Get the path of this app bundle.
    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    
    // Prepare a script to recursively make the bundle and all its contents world-writable.
    NSString *script =  [NSString stringWithFormat:@"do shell script \"chmod -R +rw '%@'\"", bundlePath];
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsentinel"

    // Run a script to find any non-link files within the bundle that are both not owned by us and not having world-writable permissions.
    NSString* cmd = [NSString stringWithFormat:@"/bin/test -z \"`/usr/bin/find %@ -not -type l -and -not -uid %d -and -not -perm -00666`\"", bundlePath, geteuid()];
    NSTask* permCheck = [NSTask launchedTaskWithLaunchPath:@"/bin/sh" arguments: [NSArray arrayWithObjects:@"-c", cmd, nil]];
    [permCheck waitUntilExit];

#pragma clang diagnostic pop
    
    // If nothing out of the ordinary was found,
    if ([permCheck terminationStatus] == 0)
    {
        // Run the script to make everything world-writable.
        NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:script];
        NSDictionary *error = [NSDictionary new];
        NSAppleEventDescriptor *result = [appleScript executeAndReturnError:&error];
        (void)result;
        
        // Return successfully.
        ORIGIN_LOG_EVENT(@"Bundle permissions verified.");
        return YES;
    }
    
    // Otherwise,
    else
    {
        ORIGIN_LOG_EVENT(@"Attempting to expand permissions of app bundle.");
        
        // Modify the script to request administrator privileges.
        NSString* adminScript = [NSString stringWithFormat:@"%@%@", script, @" with administrator privileges"];
        
        // Run the script.
        NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:adminScript];
        NSDictionary *error = [NSDictionary new];
        NSAppleEventDescriptor *result = [appleScript executeAndReturnError:&error];
        
        // If the script succeeded,
        if ( result != nil )
        {
            // Return success.
            ORIGIN_LOG_EVENT(@"Successfully changed app bundle permissions.");
            return YES;
        }
        // Otherwise,
        else
        {
            // Return failure.
            ORIGIN_LOG_EVENT(@"Failed to change app bundle permissions.");
            return NO;
        }
    }
}


