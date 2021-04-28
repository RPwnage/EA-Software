//  SystemVersion.cpp
//  Copyright (c) 2013 Electronic Arts. All rights reserved.

#import "Utilities.h"
#import "LogServiceWrapper.h"

///
/// GetSystemProductVersionString() -- returns the version number of the operating system.
///
/// Checks the ServerVersion.plist file first, then the SystemVersion.plist.
///
/// This is the same technique Apple software uses to obtain the OS version number.
/// See, e.g., http://opensource.apple.com/source/CF/CF-744/CFUtilities.c
///
NSString* GetSystemProductVersionString()
{
    NSDictionary *systemVersionDictionary = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/ServerVersion.plist"];
    if (!systemVersionDictionary)
        systemVersionDictionary = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
    NSString *systemVersion = [systemVersionDictionary objectForKey:@"ProductVersion"];
    return systemVersion;
}

@implementation SystemVersion

static SystemVersion* DefaultSystemVersion = nil;

+ (id)defaultSystemVersion {

   static dispatch_once_t singletonPredicate;

   dispatch_once(&singletonPredicate, ^{
      DefaultSystemVersion = [[self alloc] init];
   });

   return DefaultSystemVersion;
}

- (id)init {
    self = [super init];
 
    if (self) {
    
        // first, check in ini file for possible override
        IniFileReader* iniReader = [IniFileReader defaultIniFileReader];
        NSString* versionString = [iniReader valueForKey:@"OverrideOSVersion" inSection:@"QA"];
        
        // if no override, use the system values
        if (versionString == nil)
            versionString = GetSystemProductVersionString();
        
        NSArray* versions = [versionString componentsSeparatedByString:@"."];
        majorVersion = 0;
        if ( versions.count >= 1 )
            majorVersion = [[versions objectAtIndex:0] integerValue];
        minorVersion = 0;
        if ( versions.count >= 2 )
            minorVersion = [[versions objectAtIndex:1] integerValue];
        bugFixVersion = 0;
        if ( versions.count >= 3 )
            bugFixVersion = [[versions objectAtIndex:2] integerValue];
    }
 
    return self;
}

- (int)major {
    return majorVersion;
}

- (int)minor {
    return minorVersion;
}

- (int)bugFix {
    return bugFixVersion;
}

@end


@implementation IniFileReader

static NSString* EACORE_INI_PATH = @"~/Library/Application Support/Origin/EACore.ini";
static IniFileReader* DefaultIniFileReader = nil;

+ (instancetype) defaultIniFileReader {
    NSString* path = [EACORE_INI_PATH stringByExpandingTildeInPath];
    
   static dispatch_once_t singletonPredicate;

   dispatch_once(&singletonPredicate, ^{
      DefaultIniFileReader = [[self alloc] initWithContentsOfFile:path];
   });

   return DefaultIniFileReader;
}

- (instancetype) initWithContentsOfFile: (NSString*)path {

    self = [super init];
    if (self) {
        sections = [[NSMutableDictionary dictionaryWithCapacity:20] retain];
        
        NSError* error;
        NSString* fileContents = [NSString stringWithContentsOfFile:path encoding:NSASCIIStringEncoding error:&error];
        NSArray* separatedFileContents = [fileContents componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];

        __block NSString* currentSection = nil;
        
        [separatedFileContents enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL* stop){
        
            NSString* trimmed = [obj stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

            if ([trimmed length] == 0) {
                // ignore blank line
            } else {
                unichar currChar = [trimmed characterAtIndex:0];
                if (currChar == ';') {
                    // ignore comment line
                } else if (currChar == '[' && ([trimmed length] > 2)) {
                    unichar lastChar = [trimmed characterAtIndex:([trimmed length] - 1)];
                    if (lastChar == ']') {
                        // section header
                        NSRange range = { 1, [trimmed length]-2 };
                        NSString* sectionHeader = [trimmed substringWithRange:range];
                        if ([sectionHeader length] > 0) {
                            [currentSection release];
                            currentSection = [sectionHeader retain];
                        }
                    }
                } else {
                    // possible setting
                    NSArray* items = [trimmed componentsSeparatedByString:@"="];
                    if ([items count] == 2) {
                        NSString* setting = [[[items objectAtIndex:0] lowercaseString] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        NSString* value = [[items objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        [self setValue:value forKey:setting inSection:currentSection];
                    }
                }
            }
        }];

        [currentSection release];
    }
    return self;
}

- (void) dealloc {
    [sections release];
    [super dealloc];
}

- (void) setValue:(NSString*) value forKey:(NSString*) key inSection:(NSString*) sectionName {
    NSMutableDictionary* section = [sections valueForKey:sectionName];
    if (section == nil) {
        section = [NSMutableDictionary dictionaryWithCapacity:20];
        [sections setObject:section forKey:sectionName];
    }
    
    NSAssert(section, @"Couldn't find/make section");
    [section setObject:value forKey:[key lowercaseString]];
}

- (id) valuesForSection: (NSString*) section {
    return [sections valueForKey:section];
}

- (id) valueForKey: (NSString*) key inSection: (NSString*) section {
    return [[sections valueForKey:section] valueForKey:[key lowercaseString]];
}

@end
