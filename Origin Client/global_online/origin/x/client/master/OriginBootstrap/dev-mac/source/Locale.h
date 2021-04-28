//
//  Locale.h
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/9/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

// This class is used when we want to localize a text field
// using BootstrapStrings_<locale>.xml.  On first draw it 
// will replace the localize key (origin_bootstrap_exit) with
// the specific language version based on current locale.
@interface LocaleNSTextField : NSTextField {
    bool localeSet;
}

@property bool localeSet;
-(void) drawRect:(NSRect)dirtyRect;

@end

// This is a helper class that is used when we want to localize 
// a text field using BootstrapStrings_<locale>.xml.  On first 
// draw it  will replace the localize key (origin_bootstrap_ok) 
// with the specific language version based on current locale.
@interface LocaleNSButton : NSButton {
    bool localeSet;
}


// This is the main class to manage the buttons for the Tabbed View
// After altering the buttons text and adjusting the button size to
// fit the new text this class moves the buttons so they do no overlap
@property bool localeSet;
-(void) drawRect:(NSRect)dirtyRect;
-(void) setStringValue:(NSString*)aString;
-(void) setLocaleString;
@end

@interface LocaleNSView : NSView {
    NSArray* buttons;
    LocaleNSButton* offline;
    LocaleNSButton* install;
    LocaleNSButton* disagree;
    LocaleNSButton* agree;
    LocaleNSButton* save;
    LocaleNSButton* print;
}
-(void) adjustButtons;
@end


// This is our main Locale management class, using the current
// locale of the user's machine, it loads the appropriate 
// BootstrapStrings_<locale>.xml from bundle resources into the
// locale map.  localize returns the appropriate localized string
// for the specified key.
@interface Locale : NSObject

// Returns the current locale for the machine ("en_US")
+(NSString*) currentLocale;

// Returns the path to the "correct" EULA file
+(NSString*) currentEULAPath;

// Returns the current locale extracted from the "correct" EULA file path
+(NSString*) currentEULALocale;

// Returns the region-based locale filename (as defined in the Wiki - to help accessing the proper local EULA version when we have multiple EULAs using the
// same language in different regions (EU/German/ROW)
+(NSString*) regionBasedLocale;

// Called at startup, loads the appropriate locale strings for
// the current machine locale.
+(void) loadLocaleInfo:(NSString*)locale;

// Looks up and returns the localized string for the given key.  
// If there is no localized string avaliable, the key is returned
// instead.
+(NSString*) localize:(NSString*)localizeKey;

@end
