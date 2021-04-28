// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Error.h>

#import <Foundation/Foundation.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

// objective-c -> C++ conversions
inline String convert(NSString* value, Allocator& allocator)
{
    return value == nil ? allocator.emptyString() : allocator.make<String>([value UTF8String]);
}

extern ErrorPtr convert(NSError* value, Allocator& allocator);

template<typename T>
inline T convert(NSObject* value, Allocator& allocator);

template<>
inline String convert<String>(NSObject* value, Allocator& allocator)
{
    return convert((NSString*)value, allocator);
}

template<typename T>
T convertContainer(NSArray* value, Allocator& allocator)
{
    T result = allocator.make<T>();

    if (value != nil)
    {
        for (NSObject* object in value)
        {
            result.push_back(convert<typename T::value_type>(object, allocator));
        }
    }
    return result;
}

// C++ -> objective-c conversions
inline NSString* convert(const String& value)
{
    return [NSString stringWithUTF8String:value.c_str()];
}

inline NSString* convert(const char8_t* value)
{
    return [NSString stringWithUTF8String:value];
}

template<typename T>
NSArray* convert(const Vector<T>& value)
{
    NSMutableArray* array = [NSMutableArray arrayWithCapacity : value.size()];
    typename Vector<T>::const_iterator it;
    for (it = value.begin(); it != value.end(); ++it)
    {
        [array addObject:convert(*it)];
    }
    return array;
}

template<typename T, typename U>
NSDictionary* convert(const Map<T, U>& value)
{
    NSMutableDictionary* dictionary = [NSMutableDictionary dictionaryWithCapacity : value.size()];
    typename Map<T, U>::const_iterator it;
    for (it = value.begin(); it != value.end(); ++it)
    {
        [dictionary setObject : convert(it->second) forKey : convert(it->first)];
    }
    return dictionary;
}

} // namespace Internal
} // namespace foundation
} // namespace eadp

