// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <EAJson/Json.h>
#include <eadp/foundation/Allocator.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

// TEMP: working around allocation limitation of EA::Json::StringWriteStream until offical support arrives
class JsonStringWriteStream : public EA::Json::IWriteStream
{
public:
    JsonStringWriteStream(const Allocator& allocator)
    : mString(allocator.emptyString())
    {
    }
    
    JsonStringWriteStream() = default;
    
    bool Write(const void* pData, EA::Json::size_type nSize)
    {
        typedef typename String::size_type string_size_type;
        mString.append((const char8_t*)pData, (string_size_type)nSize);
        return true;
    }
    
    String mString;
};

}
}
}
