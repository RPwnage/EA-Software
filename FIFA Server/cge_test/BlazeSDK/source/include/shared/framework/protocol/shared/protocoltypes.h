/**************************************************************************************************/
/*!
    \file protocoltypes.h

    Shared types between client and server for protocol definitions.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#ifndef BLAZE_PROTOCOLTYPES_H
#define BLAZE_PROTOCOLTYPES_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "framework/protocol/shared/encoder.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazeerrortype.h"
#include "EATDF/tdf.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "blazerpcerrors.h"
#include "EATDF/tdf.h"
#include "framework/util/shared/blazestring.h"
#endif

#include "EASTL/hash_map.h"

namespace Blaze
{

typedef EA::TDF::TdfBlob& (*GetTdfBlobFunc)(EA::TDF::Tdf& tdf);
typedef const char8_t* (*GetStringFunc)(EA::TDF::Tdf& tdf);

typedef BLAZE_EASTL_HASH_MAP<BLAZE_EASTL_STRING, BLAZE_EASTL_STRING, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> HttpStringMap;

typedef HttpStringMap HttpParamMap;
typedef HttpStringMap HttpHeaderMap;

typedef HttpStringMap::const_iterator HttpParamMapIter;
typedef HttpStringMap::const_iterator HttpHeaderMapIter;

struct BLAZESDK_API HttpFieldMapping
{
    const char8_t* header;
    const char8_t* tdfReference;
    const uint32_t* tagArray;
    size_t tagCount;
};

struct BLAZESDK_API SuccessStatusCodeMapping
{
    const uint32_t* tagArray;
    size_t tagCount;
    uint32_t statusCode;
};

struct BLAZESDK_API StatusCodeErrorMapping
{
    HttpStatusCode statusCode;
#ifdef BLAZE_CLIENT_SDK
    BlazeError error;
#else
    BlazeRpcError error;
#endif
};

struct BLAZESDK_API RestResourceInfo
{
    uint16_t componentId;
    uint16_t commandId;
    const char8_t* apiVersion;
    const char8_t* resourcePath;
    const char8_t* resourcePathFormatted;
    const HttpFieldMapping* resourcePathComponents;
    size_t resourcePathComponentSize;
    const char8_t* methodName;
    GetStringFunc authorizationFunc;
    const HttpFieldMapping* customRequestHeaders;
    const HttpFieldMapping* customResponseHeaders;
    const HttpFieldMapping* customErrorHeaders;
    const HttpFieldMapping* urlParams;
    size_t restRequestHeaderArrayCount;
    size_t restResponseHeaderArrayCount;
    size_t restErrorHeaderArrayCount;
    size_t urlParamsArrayCount;
    const char8_t* requestPayloadMember;
    const char8_t* responsePayloadMember;
    GetTdfBlobFunc requestPayloadBlobFunc;
    GetTdfBlobFunc responsePayloadBlobFunc;
    const uint32_t* requestPayloadMemberTags;
    size_t requestPayloadMemberTagSize;
    const uint32_t* responsePayloadMemberTags;
    size_t responsePayloadMemberTagSize;
    const SuccessStatusCodeMapping* successStatusCodes;
    size_t successStatusCodesArrayCount;
    const StatusCodeErrorMapping* statusCodeErrors;
    size_t statusCodeErrorsArrayCount;
    bool addEncodedPayload;
    const char8_t* contentType;
    bool compressPayload;
    Encoder::SubType headerEncoderSubType;
    Encoder::SubType urlParamEncoderSubType;
    Encoder::SubType payloadEncoderSubType;
    bool useBlazeErrorCode;
    bool encodeEnumsAsStrings;
    bool encodeBoolsAsTrueFalse;
    bool encodeListsAsCSV;
    bool encodeVariableGenericWithoutTdfInfo;
    uint16_t maxHandledUrlRedirects;

    bool equals(const char8_t* method, const char8_t* actualPath) const
    {
        if (blaze_strcmp(methodName, method) != 0)
        {
            return false;
        }

        const char8_t* tpathPart = resourcePath;
        const char8_t* apathPart = actualPath;
        if (tpathPart == nullptr || apathPart == nullptr)
        {
            return false;
        }
        while (*tpathPart != '\0' && *apathPart != '\0')
        {
            if (*tpathPart++ == *apathPart++)
            {
                continue;       // So far so good
            }
            // Mismatch.  If we're starting a tokenized part, allow it (and skip to the next part)
            if (*(tpathPart-1) == '{')
            {
                while (*tpathPart != '/' && *tpathPart != '\0')
                {
                    ++tpathPart;
                }
                if (*(tpathPart-1) != '}')
                {
                    return false;   // Malformed token path
                }
                // it is possible that the actual path specified empty string for the tokenized part,
                // in which case, we would have gone too far already when we detected the mismatch
                if (*(apathPart-1) == '/')
                {
                    --apathPart;
                }
                while (*apathPart != '/' && *apathPart != '\0')
                {
                    ++apathPart;
                }
            }
            else
            {
                return false;   //Regular mismatch
            }
        }

        // If actual path did not specify a value for the last tokenized part of the URI, we can still match
        if (*apathPart == '\0' && *tpathPart == '{')
        {
            while (*tpathPart != '/' && *tpathPart != '\0')
            {
                ++tpathPart;
            }
            if (*(tpathPart-1) != '}')
            {
                return false;   // Malformed token path
            }
        }

        if (*tpathPart == '\0' && *apathPart == '\0')
        {
            return true;
        }

        return false;
    }
};

}

#endif
