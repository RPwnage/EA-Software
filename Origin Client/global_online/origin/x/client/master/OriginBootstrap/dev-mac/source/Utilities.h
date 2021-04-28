//  SystemVersion.h
//  Copyright (c) 2013 Electronic Arts. All rights reserved.

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface SystemVersion : NSObject
{
    NSArray* parsedData;
    
    int majorVersion;
    int minorVersion;
    int bugFixVersion;
}

+ (id) defaultSystemVersion;

- (int) major;
- (int) minor;
- (int) bugFix;

@end

@interface IniFileReader : NSObject {
    NSMutableDictionary* sections;
}

+ (instancetype) defaultIniFileReader;
- (instancetype) initWithContentsOfFile: (NSString*)path;
- (void) dealloc;

- (void) setValue:(NSString*) value forKey:(NSString*) key inSection:(NSString*) section;
- (id) valuesForSection: (NSString*) section;
- (id) valueForKey: (NSString*) key inSection: (NSString*) sectionName;

@end
