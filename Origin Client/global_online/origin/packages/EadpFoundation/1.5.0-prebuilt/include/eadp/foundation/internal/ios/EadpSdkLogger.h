// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

// this header file is for Objective-C++(.mm) code
#import "EadpSdkLogging.h"
#include <eadp/foundation/Hub.h>

@interface EadpSdkLogger : NSObject <EadpSdkLogger>
{
    eadp::foundation::IHub* m_hub;
    eadp::foundation::String m_packageName;
    eadp::foundation::String m_title;
}

/*!
 * creation function with hub, packageName and title
 */
+ (instancetype)loggerWithHub:(eadp::foundation::IHub*)hub andPackageName:(NSString*)packageName andTitle:(NSString*)title;

/*!
 * init a instance with hub, packageName and title
 */
- (id)initWithHub:(eadp::foundation::IHub*)hub andPackageName:(NSString*)packageName andTitle:(NSString*)title;

/*!
 * log a message
 */
- (void)write:(EadpSdkLogLevel)level message:(NSString*)message, ...;

@end
