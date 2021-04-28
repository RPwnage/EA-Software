// JRSwizzle.h semver:1.0
//   Copyright (c) 2007-2011 Jonathan 'Wolf' Rentzsch: http://rentzsch.com
//   Some rights reserved: http://opensource.org/licenses/MIT
//   https://github.com/rentzsch/jrswizzle

#if defined(ORIGIN_MAC)

#import <Foundation/Foundation.h>

// This is a simple wrapper for the services/log/LogServices when working with Objective-C files.
void PlatformServiceLogError(const char* format, ...);

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
    if ([classInstance ojr_swizzleMethod:@selector(name args) withMethod:@selector(SwizzleMethod_ ## name args) error:&errorDescription] == YES) \
    { \
        class ## _ ## name ## _MethodSizzled = true; \
        result &= true; \
    } \
    else \
    { \
        const char* errorMsg = [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]]; \
        PlatformServiceLogError("Unable to swizzle " #class " -" #name ": %s", errorMsg); \
        result &= false; \
    }

#define SWIZZLE_CLASS_METHOD(result, class, classInstance, name, args, errorDescription) \
    if ([classInstance ojr_swizzleClassMethod:@selector(name args) withClassMethod:@selector(SwizzleClass_ ## name args) error:&errorDescription] == YES) \
    { \
        class ## _ ## name ## _ClassSwizzled = true; \
        result &= true; \
    } \
    else \
    { \
        const char* errorMsg = [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]]; \
        PlatformServiceLogError("Unable to swizzle " #class " -" #name ": %s", errorMsg); \
        result &= false; \
    }


#define UNSWIZZLE_METHOD(class, classInstance, name, args, errorDescription) \
    if (class ## _ ## name ## _MethodSizzled) \
    { \
        if ([classInstance ojr_swizzleMethod:@selector(name args) withMethod:@selector(SwizzleMethod_ ## name args) error:&errorDescription] == YES) \
        { \
            class ## _ ## name ## _MethodSizzled = true; \
        } \
        else \
        { \
            const char* errorMsg = [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]]; \
            PlatformServiceLogError("Unable to unswizzle " #class " -" #name ": %s", errorMsg); \
        } \
    }

#define UNSWIZZLE_CLASS_METHOD(class, classInstance, name, args, errorDescription) \
    if (class ## _ ## name ## _ClassSwizzled) \
    { \
        if ([classInstance ojr_swizzleClassMethod:@selector(name args) withClassMethod:@selector(SwizzleClass_ ## name args) error:&errorDescription] == YES) \
        { \
            class ## _ ## name ## _ClassSwizzled = true; \
        } \
        else \
        { \
            const char* errorMsg = [(NSString*)[[errorDescription userInfo] objectForKey:NSLocalizedDescriptionKey] cStringUsingEncoding:[NSString defaultCStringEncoding]]; \
            PlatformServiceLogError("Unable to swizzle " #class " -" #name ": %s", errorMsg); \
        } \
    }


@interface NSObject (OJRSwizzle)

+ (BOOL)ojr_swizzleMethod:(SEL)origSel_ withMethod:(SEL)altSel_ error:(NSError**)error_;
+ (BOOL)ojr_swizzleClassMethod:(SEL)origSel_ withClassMethod:(SEL)altSel_ error:(NSError**)error_;

@end

#endif