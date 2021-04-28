// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API HttpUtils
{
public:
    static Map<String, String> convertUrlQueryToMap(Allocator& allocator, const String& url);
    static String convertMapToUrlQuery(Allocator& allocator, const Map<String, String>& map);
    
    static String encodeUrlParameter(Allocator& allocator, const String& parameter);
    static String decodeUrlParameter(Allocator& allocator, const String& parameter);
};

}
}
}
