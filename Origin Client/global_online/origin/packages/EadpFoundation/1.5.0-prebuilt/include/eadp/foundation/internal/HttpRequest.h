// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/internal/Serialization.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API Url
{
public:
    Url(Allocator& allocator, const char8_t* urlString);
    Url(Allocator& allocator, const char8_t* configurationKey, const char8_t* relativeUrl);

    bool isReady() const { return m_ready; }
    const String& getUrlString() const { return m_url; }
    const String& getConfigurationKey() const { return m_key; }
    bool update(IDirectorService* director);

private:
    bool m_ready;
    String m_url;
    String m_key;
};

struct EADPSDK_API HttpRequest
{
    // In memory data buffer threshold - 1M bytes
    // RestJob will host both request and response data in memory buffer, so data size cannot be over the threshold
    // For big data, will need to use different Network/Job classes
    static const int32_t kDataSizeThreshold = 0x100000;

    enum class Type
    {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_HEAD,
        HTTP_DELETE,
        HTTP_OPTIONS,
        HTTP_PATCH
    };

    HttpRequest(Allocator& allocator, const char8_t* requestUrl, Type requestType);
    HttpRequest(IHub* hub, const char8_t* configurationKey, const char8_t* relativeUrl, Type requestType);

    Url url;
    Type type;
    MultiMap<String, String> customHeaders;
    MultiMap<String, String> customParameters;
    RawBuffer payload;
    bool redirectDisabled;

    String toString(const Allocator& allocator);
    String typeToString(const Allocator& allocator);

    EA_NON_COPYABLE(HttpRequest);
};

}
}
}

