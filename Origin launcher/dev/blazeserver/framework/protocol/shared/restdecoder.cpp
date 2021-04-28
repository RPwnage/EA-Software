/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RestDecoder

    This class provides a mechanism to decode data from a RESTful HTTP request.
    Typically the caller will use this class to decode a REST-specific HTTP request into a TDF object.
    The RESTful HTTP request may pass in parameters four ways:  as RESTful "template parameters", as 
    traditional query parameters, request headers, or in the body for a POST or PUT request.

    The request uri and the method type will be used for looking up the corresponding rest resource
    info definition which determines how to map the RESTful requests into the supplied request TDF.

    Request header and query parameters included in the string buffer will be decoded into TDF object 
    based on the mapping specified in the RPC definition.

    http = {
    ...
    custom_request_headers = {
    "Authorization" = "authCredentials.token",
    "X-USER-ID" = "authCredentials.user.id",
    "X-USER-TYPE" = "authCredentials.user.type"
    }
    }

    The value of the header "Authorization" will be assigned to the TDF object 
    "authCredentials.token". The dotted notation represents a nested struct like what
    HTTP parameter uses.

    The "Content-Type" specified in the header determines the decoding type for the payload. 
    The payload will be decoded as binary if the decoding type is not recognizable and the request
    payload blob is defined in the RPC file.

    Each HTTP parameter represents one value in the TDF object.  At the top level of a TDF
    a member name for a primitive member (string, integer) matches one-to-one to an HTTP parameter
    name. Nested structs are specified using a dotted notation.  This is best illustrated in an example.

    Given the following two classes defined in a TDF file:

    class Nested
    {
    [tag="leaf", default="2"] int32_t mLeaf;
    }

    class Request
    {
    [tag="num", default="1"] int32_t mNumber;
    [tag="text"] string(256) mTextString;
    [tag="ints", default="2"] list<int32_t> mIntegers;
    [tag="nest"] Nested mNested;
    }

    And the url params mapping defined in an RPC file:

    http = {
    ...
    url_params = {
    nestedLeaf = "nested.leaf"
    }
    }

    One would format a URL as follows in order to call an API which takes in an argument of
    type Request:

    http://<webhost>/<component>/<resource>?number=64&textString=bob&nestedLeaf=20

    Currently, all the mappings in the rest resource info definition only support primitive
    members (string, integer) at the top level or inside a nested struct.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/protocol/shared/restdecoder.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/shared/jsondecoder.h"
#include "framework/util/shared/rawbuffer.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

#include "EAStdC/EAString.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
/*************************************************************************************************/
/*!
    \brief RestDecoder

    Construct a rest decoder that decodes from the given buffer

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
RestDecoder::RestDecoder() :
#ifdef BLAZE_CLIENT_SDK
    mHeaderMap(MEM_GROUP_FRAMEWORK, "RestDecoder::mHeaderMap"),
#else
    mHeaderMap(BlazeStlAllocator("RestDecoder::mHeaderMap")),
#endif
    mMethod(HttpProtocolUtil::HTTP_INVALID_METHOD),
    mHeaderMapManuallySet(false),
    mDecodingType(Decoder::JSON),
    mRecordPayload(nullptr),
    mContentLength(0),
    mRestResourceInfo(nullptr),
    mIsRestResponse(false),
    mHasError(false),
    mHttpErrorCode(-1)
{
}

/*************************************************************************************************/
/*!
    \brief ~RestDecoder

    Destructor for RestDecoder class.
*/
/*************************************************************************************************/
RestDecoder::~RestDecoder()
{
}



bool RestDecoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // initialize our state
    initializeState();

    bool parseResult = (mIsRestResponse) ? parseResponse(tdf.getClassName()) : parseRequest();
    if (parseResult == false)
    {
        return false;
    }

    // need to decode http and json content separately and then merge with a non-default copy.
    // since we want the decoded content from http to have priority over json (with the exception of the JSON blob)
    EA::TDF::TdfPtr httpDecodedTdf = tdf.clone(*Allocator::getAllocator(DEFAULT_BLAZE_MEMGROUP));    

    // decode to httpDecodedTdf with this rest decoder
    parseTdf(*httpDecodedTdf);
    bool bIgnoreError = false;

    // rest resource info should not be nullptr at this point as the nullptr has been
    // done in either parseRequest() or parseResponse()
    if (mRecordPayload != nullptr)
    {
        // write the entire payload data into the blob 
        // we write directly to the httpDecodedTdf here as the payload blob takes priority
        if (mIsRestResponse && !mHasError && mRestResourceInfo->responsePayloadBlobFunc != nullptr)
        {
            EA::TDF::TdfBlob& payloadBlob = (*mRestResourceInfo->responsePayloadBlobFunc)(*httpDecodedTdf);
            payloadBlob.setData(reinterpret_cast<const uint8_t*>(mRecordPayload), (EA::TDF::TdfSizeVal)mContentLength);
            bIgnoreError = true;
        }
        else if (!mIsRestResponse && mRestResourceInfo->requestPayloadBlobFunc != nullptr)
        {
            EA::TDF::TdfBlob& payloadBlob = (*mRestResourceInfo->requestPayloadBlobFunc)(*httpDecodedTdf);
            payloadBlob.setData(reinterpret_cast<const uint8_t*>(mRecordPayload), (EA::TDF::TdfSizeVal)mContentLength);
            bIgnoreError = true;
        }
        else
        {
            TdfDecoder* payloadDecoder = static_cast<TdfDecoder*>(DecoderFactory::create(mDecodingType));
            if (payloadDecoder != nullptr)
            {
                if (payloadDecoder->getType() == Decoder::JSON && !mHasError)
                {
                    JsonDecoder* jsonDecoder = static_cast<JsonDecoder*>(payloadDecoder);

                    const uint32_t* tag = (mIsRestResponse) ? mRestResourceInfo->responsePayloadMemberTags : mRestResourceInfo->requestPayloadMemberTags;
                    if (tag != nullptr)
                        jsonDecoder->setSubField(*tag);
                }
    #ifdef BLAZE_CLIENT_SDK
                BlazeError decodeResult = payloadDecoder->decode(*mBuffer, tdf);
    #else
                BlazeRpcError decodeResult = payloadDecoder->decode(*mBuffer, tdf, true /*only decode changed*/);
                
    #endif
                if (decodeResult != ERR_OK)
                {
                    ++mErrorCount;
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_ERR_LOG(Log::REST, "[RestDecoder].visit: Error decoding value: " << payloadDecoder->getErrorMessage() << ".");
#endif
                }

                BLAZE_DELETE_MGID(DEFAULT_BLAZE_MEMGROUP, payloadDecoder);
            }
        }
    }    

    
    EA::TDF::MemberVisitOptions copyOpts;
    copyOpts.onlyIfNotDefault = true;
    httpDecodedTdf->copyInto(tdf, copyOpts);
    
    return (bIgnoreError || mErrorCount == 0);
}

/*** Private Methods *****************************************************************************/

void RestDecoder::initializeState()
{
    if (!mHeaderMapManuallySet)
        mHeaderMap.clear();
    mParamMap.clear();
    mErrorCount = 0;
    mRecordPayload = nullptr;
    mContentLength = 0;
    mUri[0] = '\0';
    mMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
}


/*************************************************************************************************/
/*!
    \brief parseRequest

    Avoiding parsing the request while decoding by parsing it in advance.

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
bool RestDecoder::parseRequest()
{
    //Parse method, URL, query params
    if (HttpProtocolUtil::parseMethod(reinterpret_cast<const char8_t*>(mBuffer->data()),mBuffer->datasize(), mMethod) != HttpProtocolUtil::HTTP_OK ||
        HttpProtocolUtil::parseInlineRequest(*mBuffer, mUri, sizeof(mUri), mParamMap, false, &mHeaderMap) != HttpProtocolUtil::HTTP_OK)
    {
        ++mErrorCount;
        return false;
    }

    if (parseCommonHeaders() == false)
    {
        ++mErrorCount;
        return false;
    }

    // The REST information should be set when the decoder is created
    if (mRestResourceInfo == nullptr)
    {
        ++mErrorCount;
        return false;
    }

    //Map URL params
    mapUrlParams();

    //Add header values as params
    parseHeaderMap(mRestResourceInfo->customRequestHeaders, mRestResourceInfo->restRequestHeaderArrayCount);

    //Parse template params and body 
    if (parseTemplateParams() == false)
    {
        ++mErrorCount;
        return false;
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief parseResponse

    Avoiding parsing the response while decoding by parsing it in advance.

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
bool RestDecoder::parseResponse(const char8_t * tdfName)
{
    if (mRestResourceInfo == nullptr || mHttpErrorCode < 0)
    {
        ++mErrorCount;
        return false;
    }

    if (mRestResourceInfo->methodName != nullptr)
    {
        mMethod = HttpProtocolUtil::getMethodType(mRestResourceInfo->methodName);
    }

    if (mMethod == HttpProtocolUtil::HTTP_INVALID_METHOD)
    {
        return false;
    }

    if (!mHeaderMapManuallySet)
    {
        if (mBuffer->size() < HttpProtocolUtil::RESPONSE_HEADER_END_SIZE)
        {
            return false;
        }

        char8_t* httpHeader = reinterpret_cast<char8_t *>(mBuffer->head());
        char8_t* httpHeaderEnd = blaze_stristr(httpHeader, HttpProtocolUtil::RESPONSE_HEADER_END);

        if (httpHeaderEnd == nullptr)
        {
            return false;
        }

        size_t headerSize = httpHeaderEnd - httpHeader + HttpProtocolUtil::RESPONSE_LINE_END_SIZE;
        if (mBuffer->size() < headerSize)
        {
            return false;
        }

        HttpProtocolUtil::HttpReturnCode code = HttpProtocolUtil::buildHeaderMap(httpHeader, headerSize, mHeaderMap);
        if (code != HttpProtocolUtil::HTTP_OK)
        {
            return false;
        }
    }

    if (parseCommonHeaders(tdfName) == false)
    {
        ++mErrorCount;
        return false;
    }

    //Add header values as params
    if (mHasError == false)
         parseHeaderMap(mRestResourceInfo->customResponseHeaders, mRestResourceInfo->restResponseHeaderArrayCount);
    else
         parseHeaderMap(mRestResourceInfo->customErrorHeaders, mRestResourceInfo->restErrorHeaderArrayCount);

    return true;
}

bool RestDecoder::parseCommonHeaders(const char8_t * tdfName)
{
    //If the request type is POST, look for a method type override
    const char8_t* value = HttpProtocolUtil::getHeaderValue(mHeaderMap, "X-BLAZE-METHOD");

    if (value != nullptr)
    {
        HttpProtocolUtil::HttpMethod method;
        size_t len = strlen(value);
        if (HttpProtocolUtil::parseMethod(value, len, method) == HttpProtocolUtil::HTTP_OK)
        {
            //Need to directly modify request buffer
            mMethod = method;
        }
        else
            return false;
    }

    value = HttpProtocolUtil::getHeaderValue(mHeaderMap, "Content-Type");

    if (value != nullptr)
    {
        if (blaze_stricmp(value, XML_CONTENTTYPE) == 0 || blaze_strnicmp(value, XML_CONTENTTYPE ";", 16) == 0
            || blaze_stricmp(value, TEXT_XML_CONTENTTYPE) == 0 || blaze_strnicmp(value, TEXT_XML_CONTENTTYPE ";", 9) == 0 )
        {
            mDecodingType = Decoder::XML2;
        }
        else if (blaze_stricmp(value, HEAT_CONTENTTYPE) == 0 || blaze_strnicmp(value, HEAT_CONTENTTYPE ";", 23) == 0)
        {
            mDecodingType = Decoder::HEAT2;
        }
        else if (blaze_stricmp(value, JSON_CONTENTTYPE) == 0 || blaze_strnicmp(value, JSON_CONTENTTYPE ";", 17) == 0)
        {
            mDecodingType = Decoder::JSON;
        }
        else
        {
            mDecodingType = Decoder::JSON;
        }
    }

    // mHttpErrorCode is only set for responses
    bool isRequest = mHttpErrorCode < 0;
    bool noPayload = true;

    if (isRequest)
    {
        // This is a request. Look for the "Content-Length" or "Transfer-Encoding: chunked" headers to determine whether it has a message body.
        const char8_t *transferEncoding = HttpProtocolUtil::getHeaderValue(mHeaderMap, "Transfer-Encoding");
        if ((transferEncoding != nullptr) && (blaze_stricmp(transferEncoding, "chunked") == 0))
        {
            noPayload = false;
            value = nullptr;
        }
        else
        {
            value = HttpProtocolUtil::getHeaderValue(mHeaderMap, "Content-Length");
            if (value != nullptr)
                noPayload = false;
        }
    }
    else
    {
        // This is a response. According to RFC 2616 Section 4.3 (https://tools.ietf.org/html/rfc2616#section-4.3):
        //
        // "All responses to the HEAD request method MUST NOT include a message-body, even though the presence of entity- header fields
        // might lead one to believe they do. All 1xx (informational), 204 (no content), and 304 (not modified) responses MUST NOT include
        // a message-body. All other responses do include a message-body"

        noPayload = (mMethod == HttpProtocolUtil::HTTP_HEAD) || (mHttpErrorCode >= 100 && mHttpErrorCode < 200) || (mHttpErrorCode == 204) || (mHttpErrorCode == 304);
        if (!noPayload)
            value = HttpProtocolUtil::getHeaderValue(mHeaderMap, "Content-Length");
    }
    
    if (!noPayload)
    {
        if (value != nullptr)
        {
            mContentLength = (size_t)(EA::StdC::AtoU64(value));

            // HEAD request or response should not include any payloads. Skip parsing the payload
            if (mMethod != HttpProtocolUtil::HTTP_HEAD && mContentLength > 0)
            {
                const char8_t* data = reinterpret_cast<const char8_t*>(mBuffer->data());
                size_t datasize = mBuffer->datasize();

                // Go to the body of the request and extract out the payload data
                mRecordPayload = blaze_strnstr(data, "\r\n\r\n", datasize);
                if (mRecordPayload == nullptr)
                {
                    mRecordPayload = data;
                }
                else
                {
                    mRecordPayload += 4;                          // Skip to start of body
                    datasize = mBuffer->datasize() - static_cast<size_t>(mRecordPayload - data);
                }

                if (mContentLength != datasize)
                {
                    // Content-Length and actual length disagree - The data may have been compressed
                    mContentLength = datasize;
                }

                mBuffer->pull(static_cast<size_t>(mRecordPayload - data));
                if ((mDecodingType == Decoder::XML2 || mDecodingType == Decoder::JSON) && tdfName != nullptr)
                {
                    // Due to our httpxmlprotocol/restprotocol classes on Blaze will wrap extra XML around the actual error TDF, that we need to advance our buffer to point to where that begins.
                    char8_t* beginTag = blaze_stristr((char8_t*)mBuffer->data(), tdfName);
                    if (beginTag != nullptr)
                    {
                        // beginTag ptr now points to the beginning of tdf name. We need to move pointer backward by 1, to point to"<"
                        size_t totalLength = beginTag - (char8_t*)mBuffer->data() - 1;
                        mBuffer->pull(totalLength);
                        *(mBuffer->head() + mContentLength) = '\0';
                    }
                    else
                        *(mBuffer->data() + mContentLength) = '\0';
                }
                else
                    *(mBuffer->data() + mContentLength) = '\0';
            }
        }
        else
        {
            mRecordPayload = (const char8_t*)mBuffer->data();
            mContentLength = (int32_t)mBuffer->datasize();
        }
    }
    return true;
}

/*************************************************************************************************/
/*!
    \brief mapUrlParams

    Map to request object with friendly names

    \return - true if headers successfully parsed, false on error

*/
/*************************************************************************************************/
void RestDecoder::mapUrlParams()
{
    const HttpFieldMapping *urlParams = mRestResourceInfo->urlParams;

    for (size_t counter = 0; counter < mRestResourceInfo->urlParamsArrayCount; ++counter)
    {
        HttpParamMap::iterator key = mParamMap.find(urlParams[counter].header);
        if (key != mParamMap.end())
        {
            mParamMap[urlParams[counter].tdfReference] = key->second.c_str();
            mParamMap.erase(key->first.c_str());
        }
    }
}

/*************************************************************************************************/
/*!
    \brief parseTemplateParams

    Parses the template parameters of the request

    \return - true if parameters successfully parsed, false on error

*/
/*************************************************************************************************/
bool RestDecoder::parseTemplateParams()
{
    const char8_t* resourcePath = mRestResourceInfo->resourcePath;
    const char8_t* uri = mUri;

    // Skip the version number of the URI
    // After this the URI and resource path should be the same (except for 
    // the token substitution).
    ++uri;
    while (*uri != '/' && *uri != '\0')
    {
        ++uri;
    }
    if (*uri == '\0')
    {
        return false;   // Malformed URL
    }
    if (uri[0] == '/')
        ++uri;

    // Add any template parameters to the parameter map
    while (*resourcePath != '\0' && *uri != '\0')
    {
        // Find the next start of token marker.  The URI should otherwise match exactly
        if (*resourcePath != '{')
        {
            if (*resourcePath != *uri) {
                return false;       // URI should match exactly
            }

            ++resourcePath;
            ++uri;
            continue;
        }
        else
        {
            // Got a match, pull the token name out and add the value it corresponds with
            // (in the URI) to the parameter map.
            const char8_t* tokenEnd = resourcePath+1;
            const char8_t* valueStart = uri;
            const char8_t* valueEnd = uri;
            char8_t paramKey[512];
            char8_t* paramCh = paramKey;

            while (*tokenEnd != '}')
            {
                if (*tokenEnd == '\0' || *tokenEnd == '/')
                {
                    return false;   // Malformed token
                }
                *paramCh = *tokenEnd;
                ++paramCh;
                ++tokenEnd;
            }
            *paramCh = '\0';

            while (*valueEnd != '/' && *valueEnd != '\0')
            {
                ++valueEnd;
            }

            // tokenEnd now points to the ending brace of the token name.
            // valueEnd now points to the next part of the path, or end of the URI
            // Copy them out and make a parameter from them
            char8_t paramValue[512] = "";
            uint32_t valueLen = static_cast<uint32_t>(valueEnd - valueStart);
            blaze_strnzcpy(paramValue, valueStart, valueLen+1);

            // Make a copy of the paramValue since we're going to munge it for the URL decode
            char8_t outputBuffer[512] = "";   
            blaze_strnzcpy(outputBuffer, paramValue, sizeof(paramValue));
            HttpProtocolUtil::urlDecode(outputBuffer, sizeof(outputBuffer), paramValue, sizeof(paramValue), true);

            mParamMap[paramKey] = outputBuffer;
            resourcePath = tokenEnd + 1;                           // Skip past the closing brace
            uri = valueEnd;
        }
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief parseHttpHeader

    Parses the HTTP headers of the request and store them in the private param map
*/
/*************************************************************************************************/
void RestDecoder::parseHeaderMap(const HttpFieldMapping *headers, size_t arrayCount)
{
    for (size_t counter = 0; counter < arrayCount; ++counter)
    {
        const char8_t* headerValue = HttpProtocolUtil::getHeaderValue(mHeaderMap, headers[counter].header);
        if (headerValue != nullptr)
        {
            mParamMap[headers[counter].tdfReference] = headerValue;
        }
    }
}

void RestDecoder::setHttpResponseHeaders(const HttpHeaderMap *headerMap)
{
    mHeaderMapManuallySet = (headerMap != nullptr);
    if (mHeaderMapManuallySet)
    {
        mHeaderMap = *headerMap;
    }
}

} // Blaze
