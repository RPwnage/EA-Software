// JRSwizzle.h semver:1.0
//   Copyright (c) 2007-2011 Jonathan 'Wolf' Rentzsch: http://rentzsch.com
//   Some rights reserved: http://opensource.org/licenses/MIT
//   https://github.com/rentzsch/jrswizzle

#include "IGO.h"
#include "IGOLogger.h"

#if defined(ORIGIN_MAC)

#import <Foundation/Foundation.h>

#define IS_METHOD_SWIZZLED(class, name) \
    class ## _ ## name ## _MethodSizzled == true

#define IS_CLASS_METHOD_SWIZZLED(class, name) \
    class ## _ ## name ## _ClassSwizzled == true

#define SWIZZLE_METHOD_NAME(name) \
    SwizzleMethod_ ## name

#define SWIZZLE_CLASS_METHOD_NAME(name) \
    SwizzleClass_ ## name

#define DEFINE_SWIZZLE_METHOD_HOOK(class, result, name, args) \
    static bool class ## _ ## name ## _MethodSizzled = false; \
    -(result) SwizzleMethod_ ## name args

#define DEFINE_SWIZZLE_CLASS_METHOD_HOOK(class, result, name, args) \
    static bool class ## _ ## name ## _ClassSwizzled = false; \
    +(result) SwizzleClass_ ## name args

#define CALL_SWIZZLED_METHOD(instance, name, args) \
    [instance SwizzleMethod_ ## name args]

#define CALL_SWIZZLED_CLASS_METHOD(class, name, args) \
    [class SwizzleClass_ ## name args]

#define SWIZZLE_METHOD(result, class, classInstance, name, args, errorDescription) \
    if ([classInstance jr_swizzleMethod:@selector(name args) withMethod:@selector(SwizzleMethod_ ## name args) error:&errorDescription] == YES) \
    { \
        class ## _ ## name ## _MethodSizzled = true; \
        result &= true; \
    } \
    else \
    { \
        OriginIGO::IGOLogger::instance()->warn(__FILE__, __LINE__, "Unable to swizzle " #class " -" #name ": %s", [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] UTF8String]); \
        result &= false; \
    }

#define SWIZZLE_CLASS_METHOD(result, class, classInstance, name, args, errorDescription) \
    if ([classInstance jr_swizzleClassMethod:@selector(name args) withClassMethod:@selector(SwizzleClass_ ## name args) error:&errorDescription] == YES) \
    { \
        class ## _ ## name ## _ClassSwizzled = true; \
        result &= true; \
    } \
    else \
    { \
        OriginIGO::IGOLogger::instance()->warn(__FILE__, __LINE__, "Unable to swizzle " #class " -" #name ": %s", [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] UTF8String]); \
        result &= false; \
    }


#define UNSWIZZLE_METHOD(class, classInstance, name, args, errorDescription) \
    if (class ## _ ## name ## _MethodSizzled) \
    { \
        if ([classInstance jr_swizzleMethod:@selector(name args) withMethod:@selector(SwizzleMethod_ ## name args) error:&errorDescription] == YES) \
        { \
            class ## _ ## name ## _MethodSizzled = true; \
        } \
        else \
        { \
            OriginIGO::IGOLogger::instance()->warn(__FILE__, __LINE__, "Unable to unswizzle " #class " -" #name ": %s", [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] UTF8String]); \
        } \
    }

#define UNSWIZZLE_CLASS_METHOD(class, classInstance, name, args, errorDescription) \
    if (class ## _ ## name ## _ClassSwizzled) \
    { \
        if ([classInstance jr_swizzleClassMethod:@selector(name args) withClassMethod:@selector(SwizzleClass_ ## name args) error:&errorDescription] == YES) \
        { \
            class ## _ ## name ## _ClassSwizzled = true; \
        } \
        else \
        { \
            OriginIGO::IGOLogger::instance()->warn(__FILE__, __LINE__, "Unable to swizzle " #class " -" #name ": %s", [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] UTF8String]); \
        } \
    }


@interface NSObject (JRSwizzle)

+ (BOOL)jr_swizzleMethod:(SEL)origSel_ withMethod:(SEL)altSel_ error:(NSError**)error_;
+ (BOOL)jr_swizzleClassMethod:(SEL)origSel_ withClassMethod:(SEL)altSel_ error:(NSError**)error_;

@end

#endif