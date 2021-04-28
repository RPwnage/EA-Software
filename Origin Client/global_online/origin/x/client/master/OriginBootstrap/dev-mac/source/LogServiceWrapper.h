//
//  LogServiceWrapper.h
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

typedef enum
{
    LogError = 1,
    LogWarning,
    LogDebug,
    LogHelp,
    LogEvent,
    LogAction
} LogLevel;

#define ORIGIN_LOG_DEBUG(...) [LogServiceWrapper log:LogDebug function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]]
#define ORIGIN_LOG_HELP(...) [LogServiceWrapper log:LogHelp function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]]
#define ORIGIN_LOG_EVENT(...) [LogServiceWrapper log:LogEvent function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]]
#define ORIGIN_LOG_ACTION(...) [LogServiceWrapper log:LogAction function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]] 
#define ORIGIN_LOG_WARNING(...) [LogServiceWrapper log:LogWarning function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]]
#define ORIGIN_LOG_ERROR(...) [LogServiceWrapper log:LogError function:__FUNCTION__ file:__FILE__ line:__LINE__ input:[NSString stringWithFormat:__VA_ARGS__]]

@interface LogServiceWrapper : NSObject

+(void)initLogService:(NSString*)logFileName;
+(void)log:(LogLevel)level function:(const char*)function file:(const char*)file line:(int)line input:(NSString*)input;

@end