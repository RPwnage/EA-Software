/*! ***************************************************************************/
/*!
    \file 
    
    Blaze String Wrapper


    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_STRING_H
#define BLAZE_EASTL_STRING_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/string.h"

namespace Blaze
{
    /// string / wstring
    typedef eastl::basic_string<char, blaze_eastl_allocator>    string;
    typedef eastl::basic_string<wchar_t, blaze_eastl_allocator> wstring;

    /// string8 / string16 / string32
    typedef eastl::basic_string<char8_t, blaze_eastl_allocator>  string8;
    typedef eastl::basic_string<char16_t, blaze_eastl_allocator> string16;
    typedef eastl::basic_string<char32_t, blaze_eastl_allocator> string32;

}        // namespace Blaze

#endif    // BLAZE_EASTL_STRING_H
