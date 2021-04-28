//
//  LogServiceWrapper.mm
//  OriginBootstrap
//
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "LogServiceWrapper.h"
#include "LogService.h"

@implementation LogServiceWrapper

+(void) initLogService:(NSString *)logFileName
{
#ifdef DEBUG
        bool logDebug = true;
#else
        bool logDebug = false;
#endif
        
    LogService::init();
    Logger::Instance()->Init(logFileName, logDebug);
}

+(void) log:(LogLevel)level function:(const char*)function file:(const char *)file line:(int)line input:(NSString *)input
{
#ifdef DEBUG
    NSLog(@"%@", input);
#endif
    
    Logger::Instance()->Log((LogMsgType)level, function, input, file, line);
}
@end
