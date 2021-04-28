//
//  AppDelegate.m
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "AppDelegate.h"

#import "Updater.h"
#import "Locale.h"

#import "3rdParty/LetsMove/PFMoveApplication.h"

#import "LogServiceWrapper.h"

// The Origin live download link.
#define ORIGIN_DMG_LIVE_DOWNLOAD_URL "http://www.dm.origin.com/mac/download"

// The location and base filename for bootstrapper logs.
#define BOOTSTRAPPER_LOG_PATH "~/Library/Application Support/Origin/Logs/Bootstrapper_Log"

@implementation AppDelegate

@synthesize eulaWindow;
@synthesize eulaWebView;
@synthesize eulaAgreeButton;
@synthesize eulaDisagreeButton;

@synthesize updateWindow;
@synthesize updateWebView;
@synthesize updateVersionInfo;
@synthesize updateAutoChecked;
@synthesize updateButtonTabView;

@synthesize progressWindow;
@synthesize progressIndicator;

@synthesize downloadProgressWindow;
@synthesize downloadProgressIndicator;
@synthesize downloadProgressEta;

@synthesize updateErrorDialog;
@synthesize updateErrorText;

@synthesize autoUpdateCurrent;

@synthesize originLaunchUrl;
@synthesize updater;

// local startup time
@synthesize startupTime;

-(id) init
{
    if ((self = [super init]))
    {
        originLaunchUrl = @"";
        
        // Add the following to set your app as the default for this scheme
        NSString * bundleID = [[NSBundle mainBundle] bundleIdentifier];
        LSSetDefaultHandlerForURLScheme((CFStringRef)@"origin", (CFStringRef)bundleID);
        LSSetDefaultHandlerForURLScheme((CFStringRef)@"origin2", (CFStringRef)bundleID);		
        LSSetDefaultHandlerForURLScheme((CFStringRef)@"eadm", (CFStringRef)bundleID);
        
        // set startup time
        startupTime = [Updater time];
    }
                                        
    return self;
}
                                        
- (void)dealloc
{
    [updater release];
    [super dealloc];
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    // Install event handler for GetURL Apple Event.
    NSAppleEventManager *appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

//
// The following two functions are duplicated in:
//       OriginBootstrap/dev-mac/appDelegate.m
//       originClient/common/OriginApplicationDelegateOSX.mm
//
// Please keep them identical.
//

/// \brief Returns true if the passed Bundle ID matches a known web browser.
bool isWebBrowser(NSString* bundleID)
{
    // A lower-case list of Mac web browsers.
    NSArray* macBrowserList = [NSArray arrayWithObjects:
           @"safari"
           , @"omniweb"
           , @"cruz"
           , @"chrome"
           , @"firefox"
           , @"camino"
           , @"stainless"
           , @"opera"
           , @"flock"
           , @"fake"
           , nil
           ];
    
    // Get the lower-case name of the event sender.  (From the last component of the bundle identifier.)
    NSArray* bundleComponents = [bundleID componentsSeparatedByString: @"."];
    NSString* name = [(NSString*)[bundleComponents lastObject] lowercaseString];
    
    // Return true if the name is in the list of Mac web browsers,
    if ( [macBrowserList containsObject: name] ) return true;
    
    // Return false when the sender cannot be confirmed to be a web browser.
    return false;
}

/// \brief Returns the Bundle ID of the event sender, if possible.
NSString* getEventSenderBundleID(NSAppleEventDescriptor * event)
{
    // Find the original sender of the event.
    NSAppleEventDescriptor *eventSource = [event attributeDescriptorForKeyword:keyOriginalAddressAttr];
    if (! eventSource) return nil;
    NSData *eventSourcePsnData = [[eventSource coerceToDescriptorType:typeProcessSerialNumber] data];
    if (! eventSourcePsnData) return nil;
    ProcessSerialNumber eventPsn = *(ProcessSerialNumber *) [eventSourcePsnData bytes];
    
    // Lookup the BundleId for the event sender.
    CFDictionaryRef procInfo = ProcessInformationCopyDictionary(&eventPsn, kProcessDictionaryIncludeAllInformationMask);
    if (! procInfo) return nil;
    NSString* senderId = (NSString*) CFDictionaryGetValue(procInfo, kCFBundleIdentifierKey);
    if (! senderId) return nil;
    
    // Return the Bundle ID.
    return senderId;
}

- (void)handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent {
    
    // Extract the URL from the Apple event.
    NSString* url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];

    // Extract the sender of the event source.
    NSString* senderId = getEventSenderBundleID(event);
    
    // If the event sender is a web browser,
    if ( isWebBrowser( senderId ) )
    {
        // Inform Origin that the URL is from a web browser.
        [updater addClientArg:@"-Origin_LaunchedFromBrowser"];
    }
    ORIGIN_LOG_EVENT(@"Bootstrap GetURL request from: %@ for URL %@", senderId, url);
    
    // Save the URL for launch.
    [self setOriginLaunchUrl:url];
}

- (void)setLocalizedUiTitles
{
    [eulaWindow setTitle:[Locale localize:[eulaWindow title]]];
    [updateWindow setTitle:[Locale localize:[updateWindow title]]];
    [progressWindow setTitle:[Locale localize:[progressWindow title]]];
    [downloadProgressWindow setTitle:[Locale localize:[downloadProgressWindow title]]];

    NSString* applicationName = [Locale localize:@"application_name"];
    NSString* title = [Locale localize:[aboutMenuItem title]];
    [aboutMenuItem setTitle:[title stringByReplacingOccurrencesOfString:@"%1" withString:applicationName]];
    [servicesMenuItem setTitle:[Locale localize:[servicesMenuItem title]]];
    title = [Locale localize:[hideAppMenuItem title]];
    [hideAppMenuItem setTitle:[title stringByReplacingOccurrencesOfString:@"%1" withString:applicationName]];
    [hideOthersMenuItem setTitle:[Locale localize:[hideOthersMenuItem title]]];
    [showAllMenuItem setTitle:[Locale localize:[showAllMenuItem title]]];
    title = [Locale localize:[quitAppMenuItem title]];
    [quitAppMenuItem setTitle:[title stringByReplacingOccurrencesOfString:@"%1" withString:applicationName]];
}

- (void)displayUpdateError
{
    NSString* localizedText = [NSString stringWithFormat:[Locale localize:@"ebisu_client_update_error_text"], ORIGIN_DMG_LIVE_DOWNLOAD_URL];
    NSRange urlRange = [localizedText rangeOfString:@ORIGIN_DMG_LIVE_DOWNLOAD_URL];
    
    NSURL* url = [NSURL URLWithString:@ORIGIN_DMG_LIVE_DOWNLOAD_URL];
    NSMutableAttributedString* attributedString = [[[NSMutableAttributedString alloc] initWithString:localizedText] autorelease];
    
    [attributedString addAttribute:NSLinkAttributeName value:[url absoluteString] range:urlRange];
    [attributedString addAttribute:NSForegroundColorAttributeName value:[NSColor blueColor] range:urlRange];
    [attributedString addAttribute:NSUnderlineStyleAttributeName value:[NSNumber numberWithInt:NSSingleUnderlineStyle] range:urlRange];
    
    [updateErrorText setAllowsEditingTextAttributes:YES];
    [updateErrorText setSelectable:YES];
    [updateErrorText setAttributedStringValue:attributedString];
    
    // Select and deslect the text in order to workaround a strange NSTextField layout bug.  If this isn't done the cursor doesn't
    // change over the hyper link and the text itself re-layouts and changes size when first clicked.
    [updateErrorText selectText:self];
    NSText* fieldEditor = [updateErrorDialog fieldEditor:YES forObject:updateErrorText];
    [fieldEditor setSelectedRange:NSMakeRange([[fieldEditor string] length], 0)];
    
    [updateErrorDialog makeKeyAndOrderFront:nil];
}

-(IBAction)updateErrorClosed:(id)sender
{
    [updateErrorDialog orderOut:nil];
    [NSApp terminate:nil];
}

-(void)displayConnectionError
{
    // Setup the alert
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert setMessageText:[Locale localize:@"origin_bootstrap_could_not_connect_lower"]];
    [alert setInformativeText:[Locale localize:@"ebisu_client_could_not_connect_text"]];
        
    // Add ok button
    [alert addButtonWithTitle:[Locale localize:@"origin_bootstrap_ok"]];
    
    [[alert window] center];
    [alert runModal];
    [updater launchOriginClient:[self originLaunchUrl]];
}

-(void)showProgressDialog
{
    [progressIndicator startAnimation:nil];
    [progressWindow makeKeyAndOrderFront:nil];
}

-(void)hideProgressDialog
{
    [progressIndicator stopAnimation:nil];
    [progressWindow orderOut:nil];
}

-(void)showDownloadProgressDialog
{
    [downloadProgressIndicator setMinValue:0.0];
    [downloadProgressIndicator setMaxValue:1.0];
    [downloadProgressIndicator setDoubleValue:0.0];
    [downloadProgressIndicator startAnimation:nil];
    [downloadProgressWindow makeKeyAndOrderFront:nil];
}

-(void)hideDownloadProgressDialog
{
    [downloadProgressWindow orderOut:nil];
}

-(IBAction) cancelDownloadClicked:(id)sender
{
    ORIGIN_LOG_EVENT(@"User requested to cancel the download.");
    [self hideDownloadProgressDialog];
    [[updater downloader] cancelDownload];
    
    // If this wasn't an optional update, canceling exits the application
    if(![[[updater downloader] info] isOptionalUpdate] || ![updater haveValidClientLib])
    {
        exit(0);
    }
    else 
    {
        [self updateProcessingComplete];
    }
}

-(void)updateDownloadProgress:(double)downloadPercent estimatedTimeRemaining:(NSString*) estimatedTimeRemaining
{
    ORIGIN_LOG_DEBUG(@"Progress update %f percent, eta: %@", downloadPercent, estimatedTimeRemaining);
    [downloadProgressIndicator setDoubleValue:downloadPercent];
    [downloadProgressIndicator displayIfNeeded];
    [downloadProgressEta setStringValue:estimatedTimeRemaining];
    [downloadProgressEta display];
}

-(void)displayUpdateDialog:(NSString*)updateVersion :(NSString*)environment :(NSString*)currentVersion
{
    NSURL* url;
    
    // Create strings containing only the first 3 components of version (9.0.12) so we strip the most minor component.
    NSString* oldVersion = [currentVersion substringWithRange:NSMakeRange(0, [currentVersion rangeOfString:@"." options:NSBackwardsSearch].location)];
    NSString* newVersion = [updateVersion substringWithRange:NSMakeRange(0, [updateVersion rangeOfString:@"." options:NSBackwardsSearch].location)];
    NSString* formatString;
    
    // If the first three strings are the same,
    if ( [oldVersion compare:newVersion] == NSOrderedSame )
    {
        // Prompt the user with the full version specifier.
        formatString = [NSString stringWithFormat:[Locale localize:@"origin_bootstrap_new_version_info"], updateVersion, currentVersion];
    }
    // Otherwise,
    else
    {
        // Prompt the user using only the first 3 components of version.
        formatString = [NSString stringWithFormat:[Locale localize:@"origin_bootstrap_new_version_info"], newVersion, oldVersion];
    }

    [updateVersionInfo setLocaleSet:YES];
    [updateVersionInfo setStringValue:[formatString retain]];

    [self setAutoUpdateCurrent:[[[updater downloader] info] isAutoUpdate]];
    
    if([self autoUpdateCurrent])
    {
        [updateAutoChecked setState:NSOnState];
    }
    else
    {
        [updateAutoChecked setState:NSOffState];
    }
    
    if([environment compare:@"production"] == 0)
    {
        url=[NSURL URLWithString:[NSString stringWithFormat:@"https://secure.download.dm.origin.com/releasenotes/%@/%@.html", 
                                  newVersion, [Locale currentLocale]]];
    }
    else 
    {
        url=[NSURL URLWithString:[NSString stringWithFormat:@"https://stage.secure.download.dm.origin.com/releasenotes/%@/%@.html", 
                                  newVersion, [Locale currentLocale]]];
    }
 
	ORIGIN_LOG_EVENT(@"Loading Release Notes from URL: %@", url);
	NSHTTPURLResponse* response;
	NSError* error;
	NSData* data = [NSURLConnection sendSynchronousRequest:[Downloader buildUrlRequestWithOriginHeaders:url :currentVersion] returningResponse:&response error:&error];
	
	if([response statusCode] != 200 || data == nil)
	{
		ORIGIN_LOG_ERROR(@"Could not retrieve release notes with status code: [%d].  Attempting to load directly", [response statusCode]);
        [[updateWebView mainFrame] loadRequest:[Downloader buildUrlRequestWithOriginHeaders:url :currentVersion]];
	}
	else 
	{
		ORIGIN_LOG_EVENT(@"Successfully loaded Release Notes.");
		[[updateWebView mainFrame] loadData:data MIMEType:[response MIMEType] textEncodingName:[response textEncodingName] baseURL:nil];
	}
	
    NSButton* closeButton = [updateWindow standardWindowButton:NSWindowCloseButton];
    [closeButton setTarget:self];
    
    if([[[updater downloader] info] isOptionalUpdate])
    {
        ORIGIN_LOG_EVENT(@"Showing OPTIONAL update dialog.");
        [updateButtonTabView selectTabViewItemWithIdentifier:@"OPTIONAL"];
        [closeButton setAction:@selector(onRemindMeLater)];
    }
    else 
    {
        ORIGIN_LOG_EVENT(@"Showing MANDATORY update dialog.");
        [updateButtonTabView selectTabViewItemWithIdentifier:@"MANDATORY"];
        [closeButton setAction:@selector(onExitOrigin)];
    }
    
    [updateWindow makeKeyAndOrderFront:nil];
}

// Look up best/supported encoding for a specific language (based on http://www.basistech.com/language-identifier/languages/ )
// Wish we would be using UTF-8 for ALL of those guys!
-(NSStringEncoding)getEULABestEncoding:(NSString*)locale
{
    // For https://developer.origin.com/support/browse/EBIBUGS-25249
    // We might need to add more laguages here if the encoding needs to be changed
    // For https://developer.origin.com/support/browse/EBIBUGS-25066
    if ([locale hasPrefix:@"pl"])
        return NSWindowsCP1250StringEncoding;
    
    return NSWindowsCP1252StringEncoding;
}

-(bool)displayEula:(NSString*)eulaUrl :(NSString*)currentVersion
{
    ORIGIN_LOG_EVENT(@"Displaying EULA...");
    
    if(eulaUrl == nil || [eulaUrl length] == 0)
    {
        ORIGIN_LOG_EVENT(@"Loading EULA from bundle...");
        // Load EULA from bundle
        NSString *path = [Locale currentEULAPath];
        if (!path)
            ORIGIN_LOG_EVENT(@"We did not find a valid path the the EUAL html file");
        else
        {
            // Check which encoding we should be using initially
            NSStringEncoding encoding = [self getEULABestEncoding:[Locale currentEULALocale]];
            
            NSError* error;
            NSString *htmlData = [NSString stringWithContentsOfFile:path encoding:encoding error:&error];
            if (!htmlData)
            {
                NSString* encodingName = encoding == NSWindowsCP1250StringEncoding ? @"Windows-1250" : @"Windows-1252";
                ORIGIN_LOG_EVENT(@"Unable to parse the file in %@, error description: %@", encodingName, [error localizedDescription]);
                
                // Flip encoding, just in case we get lucky! (and to preserve previous behavior)
                encoding = (encoding == NSWindowsCP1252StringEncoding) ? NSWindowsCP1250StringEncoding : NSWindowsCP1252StringEncoding;
                
                htmlData = [NSString stringWithContentsOfFile:path encoding:encoding error:&error];
                if (!htmlData)
                {
                    encodingName = encoding == NSWindowsCP1250StringEncoding ? @"Windows-1250" : @"Windows-1252";
                    ORIGIN_LOG_EVENT(@"Unable to parse the file in %@, error description: %@", encodingName, [error localizedDescription]);
                    
                    NSStringEncoding encoding;
                    htmlData = [NSString stringWithContentsOfFile:path usedEncoding:&encoding error:&error];
                    if (!htmlData)
                    {
                        // Yep still failed log it.
                        ORIGIN_LOG_EVENT(@"Still unable to parse the file, error description: %@", [error localizedDescription]);
                    }
                }
            }
            [[eulaWebView mainFrame] loadHTMLString:htmlData baseURL:nil];
        }
    }
    else {
        ORIGIN_LOG_EVENT(@"Loading EULA from URL...");
        // Load EULA from URL
        NSURL* url=[NSURL URLWithString:eulaUrl];
        NSHTTPURLResponse* response;
        NSError* error;
        NSData* data = [NSURLConnection sendSynchronousRequest:[Downloader buildUrlRequestWithOriginHeaders:url :currentVersion] returningResponse:&response error:&error];
        
        if([response statusCode] != 200 || data == nil)
        {
            ORIGIN_LOG_ERROR(@"Could not retrieve EULA [status code:%d].  Checking against embedded EULA.", [response statusCode]);
            return false;
        }
        else 
        {
            [[eulaWebView mainFrame] loadData:data MIMEType:[response MIMEType] textEncodingName:[response textEncodingName] baseURL:nil];
        }
    }

    // Find the old right hand side of the agree/disagree buttons
    float oldRightSide = [eulaAgreeButton frame].origin.x + [eulaAgreeButton frame].size.width;
    float oldHeight = [eulaAgreeButton frame].size.height;

    // Make both buttons resize to fit their contents
    [eulaAgreeButton sizeToFit];
    [eulaDisagreeButton sizeToFit];
    
    // Find their new width
    float newWidth = MAX([eulaAgreeButton frame].size.width, [eulaDisagreeButton frame].size.width);

    NSSize newSize = {.width=newWidth, .height=oldHeight};

    // Set their new size
    [eulaAgreeButton setFrameSize: newSize];
    [eulaDisagreeButton setFrameSize: newSize];

    // Reposition them
    NSPoint agreeOrigin = {.y = [eulaAgreeButton frame].origin.y,
                           .x = (oldRightSide - newSize.width)};
    
    NSPoint disagreeOrigin = {.y = [eulaDisagreeButton frame].origin.y,
                              .x = (oldRightSide - newSize.width - newSize.width)};

    [eulaAgreeButton setFrameOrigin: agreeOrigin];
    [eulaDisagreeButton setFrameOrigin: disagreeOrigin];

    // Default EULA text size is a little large for OS X...
    // Can call this more times to make even smaller.
    [eulaWebView makeTextSmaller:nil];
    [NSApp activateIgnoringOtherApps: YES];
    [eulaWindow makeKeyAndOrderFront:nil];
    return true;
}

- (void)updateProcessingComplete
{
    [self hideProgressDialog];
    
    if(![updater updated] && ![updater haveValidClientLib])
    {
        if([[[updater downloader] info] internetConnection])
        {
            [self displayUpdateError];
        }
        else 
        {
            [self displayConnectionError];
        }
    }
    else 
    {
        [updater launchOriginClient:[self originLaunchUrl]];
    }
}

- (bool)checkIsCrashDialog
{
    for(NSString* arg in [[NSProcessInfo processInfo] arguments])
    {
        if([arg caseInsensitiveCompare:@"-CrashDialog"] == 0)
        {
            return true;
        }
    }

    return false;
}

#ifdef DEBUG
- (void)debuggingPrivateDisplayMethods
{
    // displayUpdateDialog:(NSString*)updateVersion :(NSString*)environment :(NSString*)currentVersion
    [self displayUpdateDialog:@"9.0.12.1223" :@"production" :@"9.0.11.77"];

    [self showDownloadProgressDialog];
    
     for(int i = 1; i <= 100; i++)
     {
         NSString* bytesDownloaded = [NSString stringWithFormat:@"%.1f", (double)(((double)i/100)*137.5)];
         NSString* totalBytes = [NSString stringWithFormat:@"%.1f", (double)(137.5)];
         NSString* eta = [NSString stringWithFormat:[Locale localize:@"origin_bootstrap_copied_mac"], bytesDownloaded, totalBytes];
         [self updateDownloadProgress:(double)i/100 estimatedTimeRemaining:[eta retain]];
         [NSThread sleepForTimeInterval:.10];
     }
     
     [self hideDownloadProgressDialog];
     
     [self showProgressDialog];
     [NSThread sleepForTimeInterval:5];
     [self hideProgressDialog];
     
     [self displayEula:nil :nil];
}
#endif

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [LogServiceWrapper initLogService:[@BOOTSTRAPPER_LOG_PATH stringByExpandingTildeInPath]];

    ORIGIN_LOG_EVENT(@"Version: %@", [Updater appVersion]);
    ORIGIN_LOG_EVENT(@"\"%@\"", [[NSBundle mainBundle] bundlePath]);
    
    // Make sure to disable App Nap
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
    NSArray* args = [NSArray arrayWithObjects:@"write", bundleId, @"NSAppSleepDisabled", @"-bool", @"YES", nil];
    NSTask* task = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/defaults" arguments:args];
    [task waitUntilExit];
    int status = [task terminationStatus];
    if (status != 0)
        ORIGIN_LOG_EVENT(@"Unable to disable App Nap (%d)", status);

    if([[self originLaunchUrl] length] > 0)
    {
        ORIGIN_LOG_EVENT(@"Launch URL: %@", [self originLaunchUrl]);
    }
    
    // The Mac client relaunches itself with -CrashDialog in order to display the
    // crash dialog and send the telemetry back to the servers.  If that is the case
    // we will skip any processing including LetsMove and just load up the application
    bool isCrashDialogLaunch = [self checkIsCrashDialog];
    
    if(!isCrashDialogLaunch)
    {
        // Init language structures
        [Locale loadLocaleInfo:[Locale currentLocale]];
        [self setLocalizedUiTitles];
        
        // Force move to /Applications if running from DMG, or provide option
        // if running from anywhere else.
        ORIGIN_LOG_ACTION(@"Checking for running from mounted DMG or non-Applications folder...");
        PFMoveToApplicationsFolderIfNecessary();
        ORIGIN_LOG_ACTION(@"DMG Check completed, continuing launch.");
    }
    
    updater = [[Updater alloc] init];

    if(!isCrashDialogLaunch && [updater checkShouldUpdate])
    {
        ORIGIN_LOG_ACTION(@"Starting update processing...");
        [updater doUpdates:self];
    }
    else 
    {
        ORIGIN_LOG_ACTION(@"Skipping update processing and launching client...");
        
        if([updater isOfflineMode])
        {
            [updater addClientArg:@"/StartOffline"];
        }
        
        [updater launchOriginClient:[self originLaunchUrl]];
    }
}

-(IBAction)printEulaClicked:(id)sender
{
    [[[[eulaWebView mainFrame] frameView] documentView] print:sender];
}

-(IBAction)saveEulaClicked:(id)sender
{
    NSSavePanel* save = [NSSavePanel savePanel];
    
    [save setAllowedFileTypes:[NSArray arrayWithObject:@"pdf"]];
    [save setAllowsOtherFileTypes:NO];
    [save setExtensionHidden:NO];
    
    int result = [save runModal];
    
    if(result == NSOKButton)
    {
        NSPrintInfo* printInfo;
        NSPrintInfo* sharedInfo;
        NSPrintOperation* printOp;
        NSMutableDictionary* printInfoDict;
        NSMutableDictionary* sharedDict;
        
        sharedInfo = [NSPrintInfo sharedPrintInfo];
        sharedDict = [sharedInfo dictionary];
        printInfoDict = [NSMutableDictionary dictionaryWithDictionary:sharedDict];
        
        [printInfoDict setObject:NSPrintSaveJob forKey:NSPrintJobDisposition];
        [printInfoDict setObject:[save filename] forKey:NSPrintSavePath];
        
        printInfo = [[NSPrintInfo alloc] initWithDictionary:printInfoDict];
        [printInfo setHorizontalPagination:NSAutoPagination];
        [printInfo setVerticalPagination:NSAutoPagination];
        [printInfo setVerticallyCentered:NO];
        
        printOp = [NSPrintOperation printOperationWithView:[[[eulaWebView mainFrame] frameView] documentView] printInfo:printInfo];
        [printOp setShowsPrintPanel:NO];
        [printOp runOperation];
    }
}

-(IBAction)eulaOkClicked:(id)sender
{
    // Hide eula window
    [eulaWindow orderOut:nil];
    // Tell the updater that EULA was accepted
    [updater eulaAccepted];
}

-(IBAction)eulaCancelClicked:(id)sender
{
    // Hide eula window
    [eulaWindow orderOut:nil];
    
    ORIGIN_LOG_ERROR(@"User Declined EULA, Application will close.");
    
    // Exit the application
    [NSApp terminate:self];
}

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id < WebPolicyDecisionListener >)listener
{
    NSString *host = [[request URL] host];
    if (host) {
        [[NSWorkspace sharedWorkspace] openURL:[request URL]];
    } else {
        [listener use];
    }
}

-(void)saveAutoUpdateSetting
{
    if(([self autoUpdateCurrent] && [updateAutoChecked state] == NSOffState) ||
       (![self autoUpdateCurrent] && [updateAutoChecked state] == NSOnState))
    {
        [[updater downloader] writeAutoUpdateSetting:([updateAutoChecked state] == NSOnState)];
    }
}

-(IBAction)installNowClicked:(id)sender
{
    [updateWindow orderOut:nil];
    [self saveAutoUpdateSetting];
    [updater installUpdate];
}

-(void)onRemindMeLater
{
    [updater setDeclinedUpdate:true];
    [updateWindow orderOut:nil];
    [self saveAutoUpdateSetting];
    [updater launchOriginClient:[self originLaunchUrl]];
}

-(IBAction)remindLaterClicked:(id)sender
{
    [self onRemindMeLater];
}

-(void)onExitOrigin
{
    [updateWindow orderOut:nil];
    [self saveAutoUpdateSetting];
    [NSApp terminate:self];
}

-(IBAction)exitOrigin:(id)sender
{
    [self onExitOrigin];
}

-(IBAction)goOfflineClicked:(id)sender
{
    [updateWindow orderOut:nil];
    [self saveAutoUpdateSetting];
    [updater addClientArg:@"/StartOffline"];
    [updater launchOriginClient:[self originLaunchUrl]];
}

@end
