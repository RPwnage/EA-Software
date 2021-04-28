//
//  Locale.m
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/9/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "Locale.h"
#import "LogServiceWrapper.h"

@interface Locale (hidden)
+(bool) isValidLocale:(NSString *)locale forPathsForResourcesOfType:(NSString*)fileType inDirectory:(NSString*)path;
@end

@implementation LocaleNSTextField

@synthesize localeSet;

- (id)init
{
    self = [super init];
    if(self) {
        [self setLocaleSet:false];
    }
    
    return self;
}

-(void) drawRect:(NSRect)dirtyRect
{
    if(![self localeSet])
    {
        [self setLocaleSet:true];
        [super setStringValue:[Locale localize:[super stringValue]]];
    }
    [super drawRect:dirtyRect];
}

@end

@implementation LocaleNSButton
@synthesize localeSet;

- (id)init
{
    self = [super init];
    if(self) {
        [self setLocaleSet:false];
    }
    
    return self;
}

-(void) sizeToFit
{
    if(![self localeSet])
    {
        [self setLocaleSet:true];
        [self setStringValue:[Locale localize:[super title]]];
    }

    [super sizeToFit];
}

-(void) drawRect:(NSRect)dirtyRect
{
    if(![self localeSet])
    {
        [self setLocaleSet:true];
        [self setStringValue:[Locale localize:[super title]]];
    }
    
    [super drawRect:dirtyRect];
}

-(void) setLocaleString
{
    if(![self localeSet])
    {
        [self setLocaleSet:true];
        [self setStringValue:[Locale localize:[super title]]];
    }
}

- (void)setStringValue:(NSString*)aString
{
    // store xib specified width as minimum width
    CGFloat minWidth = self.frame.size.width;
    
    //call super
    [super setTitle:aString];
    
    //resize to fit the new string
    [self sizeToFit];
    
    // enforce minimum width
    if(self.frame.size.width < minWidth) {
        [self setFrameSize:NSMakeSize(minWidth, self.frame.size.height)];
    }
}
@end

@implementation LocaleNSView

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self) {
            // Grab all the buttons which are sub views
            buttons = [NSArray arrayWithArray:[self subviews]];
        
            if (buttons != nil)
            {
                for (int i=0; i<buttons.count; i++)
                {
                    // Make sure we are working with the right button
                    if ([[buttons objectAtIndex: i] isMemberOfClass:[LocaleNSButton class]])
                    {
                        LocaleNSButton* button = [buttons objectAtIndex: i];
                        if (button != nil)
                        {
                            // we have to check for title 10.6 does not know about the identifier property
                            NSString* identifier =  [button title];
                            if (identifier)
                            {
                                // save each button for later use
                                if ([identifier compare:@"origin_bootstrap_agree"]== 0)
                                {
                                    agree = button;
                                }
                                else if ([identifier compare:@"origin_bootstrap_disagree"] == 0)
                                {
                                    disagree = button;
                                }
                                else if ([identifier compare:@"origin_bootstrap_save"] == 0)
                                {
                                    save = button;
                                }
                                else if ([identifier compare:@"origin_bootstrap_print"] == 0)
                                {
                                    print = button;
                                }

                                else if (([identifier compare:@"origin_bootstrap_offline"] == 0) || ([identifier compare:@"origin_bootstrap_later"] == 0))
                                {
                                    offline = button;
                                }
                                // this is the button furthest to the right
                                else if (([identifier compare:@"origin_bootstrap_install"] == 0) || ([identifier compare:@"origin_bootstrap_agree"]== 0))
                                {
                                    install = button;
                                }
                                
                                if ((offline != nil) && (install != nil))
                                    break;
                                if ((disagree != nil) && (agree != nil) && (save != nil) && (print != nil))
                                    break;
                            }
                        }
                    }
                }
            }
        }
    
    return self;
}

-(void) drawRect:(NSRect)dirtyRect
{
    if (buttons != nil)
    {
        [self adjustButtons];
    }
    
    [super drawRect:dirtyRect];
}

-(void) adjustButtons
{
    if ((offline != nil) && (install != nil))
    {
        [offline setLocaleString];
        [install setLocaleString];
    
        NSRect offlineRect = offline.frame;
        NSRect installRect = install.frame;
        int viewFrameWidth = self.frame.size.width;

        // move the button furthest right making sure it stays within the windows frame
        [install setFrameOrigin:NSMakePoint(viewFrameWidth - (installRect.size.width - 5), NSMinY(install.frame))];
        [install setNeedsDisplay:YES];
        installRect = install.frame;

        // move the next button making sure it does not overlap the first
        [offline setFrameOrigin:NSMakePoint(installRect.origin.x - (offlineRect.size.width), NSMinY(offline.frame))];
        [offline setNeedsDisplay:YES];
        offlineRect = offline.frame;
    }
    if ((disagree != nil) && (agree != nil) && (save != nil) && (print != nil))
    {
        // save the print button rect before the re-size so we know it will not move on us
        NSRect printRect = print.frame;
        
        [disagree setLocaleString];
        [agree setLocaleString];
        [save setLocaleString];
        [print setLocaleString];
        
        NSRect disagreeRect = disagree.frame;
        NSRect agreeRect = agree.frame;
        NSRect saveRect = save.frame;
        
        int viewFrameWidth = self.frame.size.width;
        
        // move the agree button and make sure it's not outside the frame of the window
        [agree setFrameOrigin:NSMakePoint(viewFrameWidth - (agreeRect.size.width + 20), NSMinY(agreeRect))];
        [agree setNeedsDisplay:YES];
        agreeRect = agree.frame;
        
        // move the disagree button making sure it does not over lap the agree button
        [disagree setFrameOrigin:NSMakePoint(agreeRect.origin.x - (disagreeRect.size.width), NSMinY(disagreeRect))];
        [disagree setNeedsDisplay:YES];
        disagreeRect = disagree.frame;

        // set the print button back to it original position
        [print setFrameOrigin:NSMakePoint(NSMinX(printRect), NSMinY(printRect))];
        [print setNeedsDisplay:YES];
        printRect = print.frame;
        
        // move the save button next to the print button
        [save setFrameOrigin:NSMakePoint((printRect.origin.x + printRect.size.width), NSMinY(saveRect))];
        [save setNeedsDisplay:YES];
    }
    
}
@end

@implementation Locale

static NSMutableDictionary* localeStringLookup = nil;
static NSMutableArray* EuropeanRegions = nil;

+(bool) isValidLocale:(NSString *)locale forPathsForResourcesOfType:(NSString*)fileType inDirectory:(NSString*)path
{
    // This creates an array of the given file type contained with in the given path folder
    NSArray* supportedLocales = [[NSBundle mainBundle] pathsForResourcesOfType:fileType inDirectory:path];
    for (int i=0; i<supportedLocales.count; i++)
    {
        if ([[supportedLocales objectAtIndex:i]  isKindOfClass:[NSString class]])
        {
            // grab the path for checking against out requested locale
            NSString* localePath = [supportedLocales objectAtIndex:i];
            int size = localePath.length;
            // Attempt to find the requested locale in the file path
            NSRange localeRange = [localePath rangeOfString:locale];
            // Check to see if we found anything
            if (localeRange.location != 0 && localeRange.location < size)
            {
                // Found it
                return true;
            }
        }
    }
    // Requested locale was not in the folder
    return false;
}

+(NSString*) getValidLanguage:(NSString *)prefLang andCompleteLocale:currentLocale forPathsForResourcesOfType:(NSString*)fileType inDirectory:(NSString*)path
{
    // This creates an array of the given file type contained with in the given path folder
    NSArray* supportedLanguages = [[NSBundle mainBundle] pathsForResourcesOfType:fileType inDirectory:path];
    
    // First look for "full locale specific" entry
    for (int i=0; i<supportedLanguages.count; i++)
    {
        if ([[supportedLanguages objectAtIndex:i]  isKindOfClass:[NSString class]])
        {
            // grab the path for checking against out requested language
            NSString* langPath = [supportedLanguages objectAtIndex:i];
            int size = langPath.length;
            
            // Grab just the locale from the file path
            NSRange langRange = NSMakeRange(size - 10, 5);
            NSString* suggestedLocale = [langPath substringWithRange:langRange];
            // try to find the preffered laguage in the suggestedLocale
            NSRange langCheck = [suggestedLocale rangeOfString:currentLocale];
            // Check to see if we found anything
            if (langCheck.location == 0)
            {
                // Found it
                return suggestedLocale;
            }
        }
    }
    
    // Then only look for a specific language
    for (int i=0; i<supportedLanguages.count; i++)
    {
        if ([[supportedLanguages objectAtIndex:i]  isKindOfClass:[NSString class]])
        {
            // grab the path for checking against out requested language
            NSString* langPath = [supportedLanguages objectAtIndex:i];
            int size = langPath.length;

            // Grab just the locale from the file path
            NSRange langRange = NSMakeRange(size - 10, 5);
            NSString* suggestedLocale = [langPath substringWithRange:langRange];
            // try to find the preffered laguage in the suggestedLocale
            NSRange langCheck = [suggestedLocale rangeOfString:prefLang];
            // Check to see if we found anything
            if (langCheck.location == 0)
            {
                // Found it
                return suggestedLocale;
            }
        }
    }
    // Requested language was not in the folder
    return nil;
}

+(NSString*) getValidLocale:(NSString*) prefLang
{
    if([prefLang compare:@"cs"] == 0)
    {
        return @"cs_CZ";
    }
    else if ([prefLang compare:@"da"] == 0)
    {
        return @"da_DK";
    }
    else if ([prefLang compare:@"de"] == 0)
    {
        return @"de_DE";
    }
    else if ([prefLang compare:@"el"] == 0)
    {
        return @"el_GR";
    }
    else if ([prefLang compare:@"es"] == 0)
    {
        return @"es_MX";
    }
    else if ([prefLang compare:@"fi"] == 0)
    {
        return @"fi_FI";
    }
    else if ([prefLang compare:@"fr"] == 0)
    {
        return @"fr_FR";
    }
    else if ([prefLang compare:@"hu"] == 0)
    {
        return @"hu_HU";
    }
    else if ([prefLang compare:@"it"] == 0)
    {
        return @"it_IT";
    }
    else if ([prefLang compare:@"ja"] == 0)
    {
        return @"ja_JP";
    }
    else if ([prefLang compare:@"ko"] == 0)
    {
        return @"ko_KR";
    }
    else if ([prefLang compare:@"nl"] == 0)
    {
        return @"nl_NL";
    }
    else if ([prefLang compare:@"no"] == 0 || [prefLang compare:@"nb"] == 0)
    {
        return @"no_NO";
    }
    else if ([prefLang compare:@"pl"] == 0)
    {
        return @"pl_PL";
    }
    else if ([prefLang compare:@"pt"] == 0)
    {
        return @"pt_BR";
    }
    else if ([prefLang compare:@"ru"] == 0)
    {
        return @"ru_RU";
    }
    else if ([prefLang compare:@"sv"] == 0)
    {
        return @"sv_SE";
    }
    else if ([prefLang compare:@"th"] == 0)
    {
        return @"th_TH";
    }
    else if ([prefLang compare:@"zh"] == 0)
    {
        return @"zh_CN";
    }
    else
    {
        return @"en_US";
    }    
}

+(NSString*) getValidEULA:(NSString*)currentLocaleStr forPreferredLanguage:(NSString*)prefLang
{
    @synchronized(EuropeanRegions)
    {
        // Make sure we have a valid array of Eurpoean Union Regions
        if (EuropeanRegions == nil)
        {
            EuropeanRegions = [[NSMutableArray alloc] initWithObjects:@"AT", @"BE", @"BG", @"CY", @"CZ", @"DK", @"EE", @"FI", @"GR", @"HU", @"IE", @"IT", @"LV", @"LT", @"LU", @"MT", @"NL", @"PL", @"PT", @"RO", @"SK", @"SI", @"ES", @"SE", @"GB", nil];
        }
    }
    // Get users preffered language and current region
    NSString* region = [currentLocaleStr substringWithRange:NSMakeRange(3, 2)];
    
    // Is user in Germany?
    if ([region compare:@"DE"] == 0)
    {
        // Do they speak German?
        if ([prefLang compare:@"en"] == 0)
        {
            return [[NSBundle mainBundle] pathForResource:@"en_DE" ofType:@"html" inDirectory:@"EULAs/German"];
        }
        // No give them Englisn German EULA
        else
            return [[NSBundle mainBundle] pathForResource:@"de_DE" ofType:@"html" inDirectory:@"EULAs/German"];
    }
    // Is user if France?
    else if ([region compare:@"FR"] == 0)
    {
        // Special case alwasy give the French EULA
        return [[NSBundle mainBundle] pathForResource:@"fr_FR" ofType:@"html" inDirectory:@"EULAs/EU"];
    }
    // If we are here must check agianst EU regions
    for( int i=0; i<EuropeanRegions.count; i++)
    {
        NSString* EURegion = [EuropeanRegions objectAtIndex:i];
        if (EURegion != nil)
        {
            // Check to see if the user's region is in the EU list
            if ([region compare:EURegion] == 0)
            {
                // We are in Eurpoe somewhere show EU EULA
                NSString* suggestedLocale = [Locale getValidLanguage:prefLang andCompleteLocale:currentLocaleStr forPathsForResourcesOfType:@"html" inDirectory:@"EULAs/EU"];
                
                if(suggestedLocale)
                {
                    // We found the users prefferd language show it
                    return [[NSBundle mainBundle] pathForResource:suggestedLocale ofType:@"html" inDirectory:@"EULAs/EU"];;
                }
                // We did not find the user's prefferd language need to show British/ English EU EULA
                else
                    return [[NSBundle mainBundle] pathForResource:@"en_GB" ofType:@"html" inDirectory:@"EULAs/EU"];
            }
        }
    }
    // If we get here user must be in the Rest of the world
    {
        // Special case for Norwegian
        if ([prefLang compare:@"nb"] == 0)
        {
            prefLang = @"no";
        }
        
        NSString* suggestedLocale = [Locale getValidLanguage:prefLang andCompleteLocale:currentLocaleStr forPathsForResourcesOfType:@"html" inDirectory:@"EULAs/ROW"];
        
        if(suggestedLocale)
        {
            // We found the users preffered language show it
            return [[NSBundle mainBundle] pathForResource:suggestedLocale ofType:@"html" inDirectory:@"EULAs/ROW"];
        }
    }
    
    // in case every check fails use this default
    return [[NSBundle mainBundle] pathForResource:@"en_US" ofType:@"html" inDirectory:@"EULAs/ROW"];
}

//#define DEBUG_LANGUAGE
#ifdef DEBUG_LANGUAGE
static NSString* DebugLanguage = @"pl_PL";
#endif

+(NSString*) currentLocale
{
    static NSString* currentLocaleStr = nil;
    
#ifndef DEBUG_LANGUAGE
    // Grab the users defaults
    NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
    // Get the list of the users preffered languages
    NSArray* languages = [defs objectForKey:@"AppleLanguages"];
    // Grab the first language in the list
    NSString* prefLang = [languages objectAtIndex:0];
#else
    NSString* prefLang = [NSString stringWithString:DebugLanguage];
#endif
    // ensure we only have a two character language key
    prefLang = [prefLang substringWithRange:NSMakeRange(0, 2)];
    if(currentLocaleStr == nil)
    {
        // Get users current locale
#ifndef DEBUG_LANGUAGE
        currentLocaleStr = [[NSLocale currentLocale] localeIdentifier];
#else
        currentLocaleStr = [NSString stringWithString:DebugLanguage];
#endif
        // Test to see if the users current locale includes their preffered language
        NSRange langRange = [currentLocaleStr rangeOfString:prefLang];
        if (!langRange.location == 0)
        {
            // If it did not create a new locale using their preffered language
            currentLocaleStr = [currentLocaleStr stringByReplacingCharactersInRange:NSMakeRange(0,2) withString:prefLang];
            ORIGIN_LOG_WARNING(@"Locale not using users preferred Language, changing to locale: %@", currentLocaleStr);
        }

        // Test our current locale against our supported locales
        if(![Locale isValidLocale:currentLocaleStr forPathsForResourcesOfType:@"xml" inDirectory:@"lang"])
        {
            // Current locale not found find the next best locale based off of preffered laguage
            ORIGIN_LOG_WARNING(@"No valid locale found for %@", currentLocaleStr);
            currentLocaleStr = [Locale getValidLocale:prefLang];
            ORIGIN_LOG_WARNING(@"Defaulting to locale: %@", currentLocaleStr);
        }
        ORIGIN_LOG_EVENT(@"Locale is %@", currentLocaleStr);
    }

    return currentLocaleStr;
}

+(NSString*) currentEULAPath
{
    // This will return the file path to the EULA.html
    static NSString* currentLocaleStr = nil;
    
#ifndef DEBUG_LANGUAGE
    // Grab the users defaults
    NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
    // Get the list of the users preffered languages
    NSArray* languages = [defs objectForKey:@"AppleLanguages"];
    // Grab the first language in the list
    NSString* prefLang = [languages objectAtIndex:0];
#else
    NSString* prefLang = [NSString stringWithString:DebugLanguage];
#endif
    // ensure we only have a two character language key
    prefLang = [prefLang substringWithRange:NSMakeRange(0, 2)];
    if(currentLocaleStr == nil)
    {
        // Get users current locale
#ifndef DEBUG_LANGUAGE
        currentLocaleStr = [[NSLocale currentLocale] localeIdentifier];
#else
        currentLocaleStr = [NSString stringWithString:DebugLanguage];
#endif
        // Test to see if the users current locale includes their preffered language
        NSRange langRange = [currentLocaleStr rangeOfString:prefLang];
        if (!langRange.location == 0)
        {
            // If it did not create a new locale using their preffered language
            currentLocaleStr = [currentLocaleStr stringByReplacingCharactersInRange:NSMakeRange(0,2) withString:prefLang];
            ORIGIN_LOG_WARNING(@"Locale not using users prefered Language, changing to locale: %@", currentLocaleStr);
        }

        // Test our current locale against our supported locales
        ORIGIN_LOG_WARNING(@"Looking for valid locale: %@", currentLocaleStr);
        currentLocaleStr = [Locale getValidEULA:currentLocaleStr forPreferredLanguage:prefLang];
        ORIGIN_LOG_EVENT(@"Locale we are using is %@", currentLocaleStr);
    }
    
    return currentLocaleStr;
}

+(NSString*) currentEULALocale;
{
    // Grab the EULA file we are using
    NSString* eulaLocaleFileName = [self currentEULAPath];
    
    // Extract the actual locale assuming file is named for locale with html format: "en_US.html"
    NSString* locale = [eulaLocaleFileName substringWithRange:NSMakeRange(eulaLocaleFileName.length - 10, 5)];
    
    return locale;
}

+(NSString*) regionBasedLocale;

{
    static NSString* regionBasedLocaleStr = nil;
    if (regionBasedLocaleStr == nil)
    {        
        // Extract the actual locale
        NSString* locale = [self currentEULALocale];
        
        // Is it one of those cases that supports the same language over multiple regions (de_DE, de_EU, de_RW)?
        if ([locale hasPrefix:@"de_"] == YES)
        {
            // Grab the EULA file we are using
            NSString* eulaLocaleFileName = [self currentEULAPath];

            // Then we need to extract the region folder name
            NSString* regionFolder = [[eulaLocaleFileName stringByDeletingLastPathComponent] lastPathComponent];
            
            // Remap it (yet another mapping!)
            NSDictionary* mapping = @{@"EU" : @"EU", @"German" : @"DE", @"ROW" : @"RW"};
            NSString* region = [mapping valueForKey:regionFolder];
            if (region)
                locale = [locale stringByReplacingCharactersInRange:NSMakeRange(3, 2) withString:region];
        }
        
        regionBasedLocaleStr = locale;
    }
    
    return regionBasedLocaleStr;
}

+(void) loadLocaleInfo:(NSString *)locale
{
    @synchronized(localeStringLookup)
    {
        if(localeStringLookup == nil)
        {
            localeStringLookup = [[NSMutableDictionary alloc] init];
            NSString *path = [[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"BootstrapStrings_%@", locale] ofType:@"xml" inDirectory:@"lang"];
            // Check to ensure we have a valid file path if not Hard default to English
            if (path == nil)
            {
                ORIGIN_LOG_ERROR(@"Failed to find file name BootstrapStrings_%@.xml", locale);
                path = [[NSBundle mainBundle] pathForResource:[NSString stringWithFormat:@"BootstrapStrings_en_US"] ofType:@"xml" inDirectory:@"lang"];
            }
            NSData *xmlData = [NSData dataWithContentsOfFile:path];

            NSError* error;
            NSXMLDocument* xmlDocument = [[NSXMLDocument alloc] initWithData:xmlData options:(NSXMLNodePreserveWhitespace|NSXMLNodePreserveCDATA) error:&error];

            if(xmlDocument == nil)
            {
                ORIGIN_LOG_ERROR(@"Failed to load strings for locale %@", locale);
            }
            else 
            {
                NSArray *stringNodes = [xmlDocument nodesForXPath:@"./locStrings/locString" error:&error];
                if(stringNodes != nil)
                {
                    int i;
                    for(i = 0; i < [stringNodes count]; i++)
                    {
                        NSXMLElement* elem = (NSXMLElement*) [stringNodes objectAtIndex:i];
                        [localeStringLookup setValue:[elem stringValue] forKey:[[elem attributeForName:@"Key"] stringValue]];
                    }
                }
            }
        }
    }
}

+(NSString*) localize:(NSString *)localizeValue
{
    if(localeStringLookup == nil)  
    {
        return localizeValue;
    }
    
    NSString* lookup = [localeStringLookup valueForKey:localizeValue];

    if(lookup == nil)
    {
        return localizeValue;
    }
    else 
    {
        return lookup;
    }
}

@end
