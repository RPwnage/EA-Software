// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <stdarg.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API Utils
{
public:
    static String& appendSprintf(String& str, const char8_t* fmt, va_list args);
    static String& appendSprintf(String& str, const char8_t* fmt, ...);
    static String convertToString(Allocator& allocator, const char16_t* val);
    static String16 convertToString16(Allocator& allocator, const char8_t* val);
    static String convertToString(Allocator& allocator, const wchar_t* wideString);
    static WString convertToWString(Allocator& allocator, const char8_t* char8String);
};

}
}
}
