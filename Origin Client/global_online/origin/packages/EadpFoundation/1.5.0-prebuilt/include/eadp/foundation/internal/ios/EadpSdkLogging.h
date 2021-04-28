// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

// this header file is for Objective-C(.m) code
#import <Foundation/Foundation.h>

// same value as ILoggingService::LogLevel enum
typedef enum tag_EadpSdkLogLevel {
    EADPSDK_LOG_LEVEL_VERBOSE    = 1,
    EADPSDK_LOG_LEVEL_DEBUG      = 2,
    EADPSDK_LOG_LEVEL_INFO       = 4,
    EADPSDK_LOG_LEVEL_WARN       = 8,
    EADPSDK_LOG_LEVEL_ERROR      = 16,
    EADPSDK_LOG_LEVEL_FATAL      = 32,
} EadpSdkLogLevel;

#ifdef EADPSDK_DISABLE_LOGGING

#   define LOGV(...)
#   define LOGD(...)
#   define LOGI(...)
#   define LOGW(...)
#   define LOGE(...)
#   define LOGF(...)

#else // not define DISABLE_EADPSDK_LOGGING

#	define LOGV(logger, ...) [logger write:EADPSDK_LOG_LEVEL_VERBOSE message:__VA_ARGS__]
#	define LOGD(logger, ...) [logger write:EADPSDK_LOG_LEVEL_DEBUG message:__VA_ARGS__]
#	define LOGI(logger, ...) [logger write:EADPSDK_LOG_LEVEL_INFO message:__VA_ARGS__]
#	define LOGW(logger, ...) [logger write:EADPSDK_LOG_LEVEL_WARN message:__VA_ARGS__]
#	define LOGE(logger, ...) [logger write:EADPSDK_LOG_LEVEL_ERROR message:__VA_ARGS__]
#	define LOGF(logger, ...) [logger write:EADPSDK_LOG_LEVEL_FATAL message:__VA_ARGS__]

#endif

@protocol EadpSdkLogger <NSObject>

/*!
 * log a message
 */
- (void)write:(EadpSdkLogLevel)level message:(NSString*)message, ...;

@end
