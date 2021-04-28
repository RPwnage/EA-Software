//
//  AppDelegate.h
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>
#import "Locale.h"

@class Updater;

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *eulaWindow;
    WebView *eulaWebView;
    LocaleNSButton *eulaAgreeButton;
    LocaleNSButton *eulaDisagreeButton;
    
    NSWindow *updateWindow;
    WebView *updateWebView;
    LocaleNSTextField *updateVersionInfo;
    NSButton *updateAutoChecked;
    NSTabView *updateButtonTabView;
    
    NSWindow *progressWindow;
    NSProgressIndicator *progressIndicator;

    NSWindow *downloadProgressWindow;
    NSProgressIndicator *downloadProgressIndicator;
    NSTextField *downloadProgressEta;
    
    NSWindow *updateErrorDialog;
    NSTextField *updateErrorText;
    
    Updater *updater;
    NSString *originLaunchUrl;
    uint64_t startupTime;
    
    IBOutlet NSMenuItem *aboutMenuItem;
    IBOutlet NSMenuItem *servicesMenuItem;
    IBOutlet NSMenuItem *hideAppMenuItem;
    IBOutlet NSMenuItem *hideOthersMenuItem;
    IBOutlet NSMenuItem *showAllMenuItem;
    IBOutlet NSMenuItem *quitAppMenuItem;
    
    bool autoUpdateCurrent;
}

@property (assign) IBOutlet NSWindow* eulaWindow;
@property (assign) IBOutlet WebView* eulaWebView;
@property (assign) IBOutlet LocaleNSButton* eulaAgreeButton;
@property (assign) IBOutlet LocaleNSButton* eulaDisagreeButton;

@property (assign) IBOutlet NSWindow* updateWindow;
@property (assign) IBOutlet WebView* updateWebView;
@property (assign) IBOutlet LocaleNSTextField* updateVersionInfo;
@property (assign) IBOutlet NSButton* updateAutoChecked;

@property (assign) IBOutlet NSTabView* updateButtonTabView;

@property (assign) IBOutlet NSWindow* progressWindow;
@property (assign) IBOutlet NSProgressIndicator* progressIndicator;

@property (assign) IBOutlet NSWindow* downloadProgressWindow;
@property (assign) IBOutlet NSProgressIndicator* downloadProgressIndicator;

@property (assign) IBOutlet NSTextField* downloadProgressEta;

@property (assign) IBOutlet NSWindow* updateErrorDialog;
@property (assign) IBOutlet NSTextField* updateErrorText;

@property (assign) Updater* updater;
@property (retain) NSString* originLaunchUrl;
@property (assign) uint64_t startupTime;

@property (assign) bool autoUpdateCurrent;

// EULA Actions

// Look up best/supported encoding for a specific language (based on http://www.basistech.com/language-identifier/languages/ )
-(NSStringEncoding)getEULABestEncoding:(NSString*)locale;
// Displays the EULA dialog - if eulaURL is nil, the resource EULA for the current language
// will be presented, otherwise the eulaURL will be loaded and displayed
-(bool) displayEula:(NSString*)eulaUrl :(NSString*)currentVersion;
// Connected to EULA dialog "Print" button click
-(IBAction)printEulaClicked:(id)sender;
// Connected to EULA dialog "Print" button click
-(IBAction)saveEulaClicked:(id)sender;
// Connected to EULA dialog "OK" button click
-(IBAction)eulaOkClicked:(id)sender;
// Connected to EULA dialog "Cancel" button click
-(IBAction)eulaCancelClicked:(id)sender;
// PolicyDelegate needed to open links in EULA in external browser
- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id < WebPolicyDecisionListener >)listener;

// Update Actions

// Displays the update dialog containing the release notes for the specified version and
// environment.
-(void) displayUpdateDialog:(NSString*)updateVersion :(NSString*)environment :(NSString*)currentVersion;
// Connected to update dialog "Install Now" button click
-(IBAction)installNowClicked:(id)sender;
// Connected to the update dialog "Remind Me Later" button click
-(IBAction)remindLaterClicked:(id)sender;
// Connected to the update dialog "Go Offline" button click
-(IBAction)goOfflineClicked:(id)sender;
// Connected to the update dialog "Exit Origin" button click
-(IBAction)exitOrigin:(id)sender;

// Update Progress / Processing

// Callback when the updater has completed the update operations and the bootstrap should either
// launch the Origin client or error out
-(void) updateProcessingComplete;

// Shows the interdeterminate progress dialog
-(void) showProgressDialog;
// Hides the interdeterminate progress dialog
-(void) hideProgressDialog;

// Download Progress / Processing
-(void) showDownloadProgressDialog;
-(void) hideDownloadProgressDialog;
-(void) updateDownloadProgress:(double) downloadPercent estimatedTimeRemaining:(NSString*) estimatedTimeRemaining;
-(IBAction) cancelDownloadClicked:(id)sender;

// Error dialogs

// Shows error dialog when update failed for reason other than connection being offline
-(void)displayUpdateError;
// Triggered when user clicks OK or closes the update error dialog
-(IBAction)updateErrorClosed:(id)sender;

// Shows error dialog when no internet connection is available
-(void)displayConnectionError;

@end
