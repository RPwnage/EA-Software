/*************************************************************************************************/
/*!
    \file httpprotocolutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPPROTOCOLUTIL_H
#define BLAZE_HTTPPROTOCOLUTIL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/framework/tdf/attributes.h" 
#else
#include "framework/tdf/attributes.h"
#endif
#include "framework/protocol/shared/protocoltypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define XML_CONTENTTYPE "application/xml"
#define TEXT_XML_CONTENTTYPE "text/xml"
#define HEAT_CONTENTTYPE "application/heat"
#define JSON_CONTENTTYPE "application/json"

#ifndef HTTP_LOGGER_CATEGORY
    #ifdef LOGGER_CATEGORY
        #define HTTP_LOGGER_CATEGORY LOGGER_CATEGORY
    #else
        #define HTTP_LOGGER_CATEGORY Log::HTTP
    #endif
#endif

namespace Blaze
{
// Forward declarations
class RawBuffer;
struct HttpParam;


class BLAZESDK_API HttpProtocolUtil
{
public:
    enum HttpMethod { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE, HTTP_HEAD, HTTP_PATCH, HTTP_INVALID_METHOD };
    enum HttpReturnCode { HTTP_OK, HTTP_BUFFER_TOO_SMALL, HTTP_NOT_FOUND, HTTP_INVALID_REQUEST };
    enum HttpAcceptEncoding { HTTP_ACCEPTENCODING_ANY, HTTP_ACCEPTENCODING_COMPRESS, HTTP_ACCEPTENCODING_DEFLATE, HTTP_ACCEPTENCODING_GZIP, HTTP_ACCEPTENCODING_IDENTITY };
    enum HttpContentEncoding { HTTP_CONTENTENCODING_ANY, HTTP_CONTENTENCODING_COMPRESS, HTTP_CONTENTENCODING_DEFLATE, HTTP_CONTENTENCODING_GZIP };

    static bool isConnectionClose(const char8_t* header, size_t headerSize);

    static HttpReturnCode parseUrl(RawBuffer& buffer, char8_t* uri, size_t uriLen);

    static HttpReturnCode parseRequest(RawBuffer& buffer, HttpMethod& method,
            char8_t* uri, size_t uriLen, HttpParamMap& paramMap, bool upperCaseKeys = false, HttpHeaderMap* headerMap = nullptr);

    static HttpReturnCode parseMethod(const char8_t* cbuf, size_t datalen, HttpMethod& method);

    static bool hasAcceptEncodingType(const HttpHeaderMap& headerMap, HttpAcceptEncoding encoding);
    static bool hasContentEncodingType(const HttpHeaderMap& headerMap, HttpContentEncoding encoding);
    static bool hasHttpEncodingType(const char8_t* value, int32_t encoding);

    static HttpReturnCode buildHeaderMap(const RawBuffer& buffer, HttpHeaderMap& headerMap);
    static HttpReturnCode buildHeaderMap(const char8_t* data, size_t bufferSize, HttpHeaderMap& headerMap);

    static bool urlencode(const char8_t* src, BLAZE_EASTL_STRING& encodedString);
    static HttpReturnCode urlDecode(char8_t* buffer, size_t bufferSize,
            const char8_t* const input, size_t inputSize = 0,
            bool terminateBuffer = false);

    static HttpReturnCode parseParameters(const char8_t* buffer, const size_t length, HttpParamMap& paramMap, bool upperCaseKeys = false);

    static HttpReturnCode parseInlineRequest(RawBuffer& buffer,
        char8_t* uri, size_t uriLen, HttpParamMap& paramMap, bool upperCaseKeys = false, HttpHeaderMap* headerMap = nullptr);

    static const char8_t* getHeaderValue(const RawBuffer& request, const char8_t* header);
    static const char8_t* getHeaderValue(const HttpHeaderMap& headerMap, const char8_t* header);

    static bool readStatusCodeLine(const char8_t* header, size_t headerSize,
            int32_t& statusCode, char8_t** reasonLine, size_t& reasonSize);

    static bool isXmlRepeatItemSymbol(const char8_t* paramName);
    static const char8_t* getStringFromHttpMethod(HttpMethod method);
    static void printHttpRequest(bool isSecure, HttpMethod method, const char8_t* hostname, uint16_t port, const char8_t* uri, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], 
        uint32_t headerCount, eastl::string& outString, bool censorValues = true);

    static void getHttpCacheKey(bool isSecure, HttpMethod method, const char8_t* hostname, uint16_t port, const char8_t* uri, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[],
        uint32_t headerCount, eastl::string& httpCacheKey);

    static bool replaceRawBufferData(RawBuffer& rawBuffer, uint8_t* newData, size_t newDataLen);
    static HttpMethod getMethodType(const char8_t* methodName);
    // get host name from address
    static void getHostnameFromConfig(const char8_t* serverAddress, const char8_t*& hostname, bool &isSecure);

    typedef BLAZE_EASTL_VECTOR<Blaze::Collections::AttributeName> XmlItemList;
    static void buildXMLItem(const char8_t* paramName, const char8_t* paramValue, const char8_t* repeatItem, char8_t* tempStr, size_t tempSize, char8_t* xmlOutput, XmlItemList& xmlEndTagList);

    static bool shouldCensorHeader(const char8_t* header, size_t* headerIndex = nullptr);
    static EA::TDF::TdfMemberInfo::PrintFormat getPrintFormatForField(const char8_t* fieldName);

#ifndef BLAZE_CLIENT_SDK
    static void getClientIPFromXForwardForHeader(const char8_t* forwardedForHeader, InetAddress& clientAddr);
#endif

    static const char8_t RESPONSE_LINE_END[];
    static size_t RESPONSE_LINE_END_SIZE;
    static const char8_t RESPONSE_HEADER_END[];
    static size_t RESPONSE_HEADER_END_SIZE;
    static const char8_t RESPONSE_CONTENT_LENGTH[];
    static size_t RESPONSE_CONTENT_LENGTH_SIZE;
    static const char8_t RESPONSE_CONTENT_LOCATION[];
    static size_t RESPONSE_CONTENT_LOCATION_SIZE;
    static const char8_t RESPONSE_TRANSFER_ENCODING[];
    static size_t RESPONSE_TRANSFER_ENCODING_SIZE;
    static const uint8_t MAX_HTTPMETHOD_LEN = 8;
    static const char8_t strMethods[HTTP_INVALID_METHOD][MAX_HTTPMETHOD_LEN];      // String versions of the methods
    static const char8_t* strHttpEncodingTypes[]; // String versions of the encoding types

    static const char8_t HTTP_LINE_BREAK[];
    static const char8_t HTTP_HEADER_HOST[];
    static const char8_t HTTP_HEADER_KEEP_ALIVE[];
    static const char8_t HTTP_POST_CONTENTTYPE[];
    static const char8_t HTTP_POST_CONTENTTYPE_XML[];
    static const char8_t HTTP_XML_CONTENTTYPE[];
    static const char8_t HTTP_HEAT_CONTENTTYPE[];
    static const char8_t HTTP_JSON_CONTENTTYPE[];
    static const char8_t HTTP_REQUEST_METHOD_ENDING[];
    static const char8_t RESPONSE_CONNECTION[];
    static const char8_t HTTP_XML_PAYLOAD[];
    static const char8_t HTTP_XML_PAYLOAD_SEPARATOR[];

    static const uint8_t  XML_START_TAG_EXTRA_CHAR = 3; // <,>
    static const uint8_t  XML_CLOSE_TAG_EXTRA_CHAR = 4; // <,/,>
    static const uint32_t XML_PAYLOAD_CONTENT_MAX_LENGTH = 8192; // Max length used for xml payload

    static const char8_t CENSORED_STR[];
    static size_t CENSORED_STR_LEN;
    static const char8_t* HTTP_CENSORED_HEADERS[];
    static const char8_t* HTTP_FILTERED_FIELDS[];
    static size_t HTTP_FILTERED_FIELD_LENS[];
    static size_t HTTP_LAST_CENSORED_FIELD_IDX;
    static size_t HTTP_LAST_SCRUBBED_FIELD_IDX;

protected:

private:
    HttpProtocolUtil();
    virtual ~HttpProtocolUtil() { };

    static HttpReturnCode buildPayloadHeader(RawBuffer& buffer, size_t& headerSize, const char8_t* hostname, uint32_t port,
        const char8_t* const method, const char8_t* const URI, const char8_t* contentType, const char8_t* httpHeaders[] = nullptr, uint32_t headerCount = 0);
   
    static HttpReturnCode parseUrl(const RawBuffer& buffer,
            char8_t* uri, size_t uriLen, char8_t** uriEnd);

    static HttpReturnCode parsePayloadParams(RawBuffer& buffer, HttpParamMap& paramMap, bool upperCaseKeys = false, 
        HttpHeaderMap* headerMap = nullptr);

    static uint8_t charHextoInt(const char8_t* const inptr);
    static bool isUnreservedChar(const char8_t chrCheck);
    
    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor
    HttpProtocolUtil(const HttpProtocolUtil& otherObj);

    // Assignment operator
    HttpProtocolUtil& operator=(const HttpProtocolUtil& otherObj);

    static void processXMLStartTags(XmlItemList& xmlCurrentStartTagList, XmlItemList& xmlPreviousEndTagList, const char8_t* repeatItem, char8_t* tempStr, size_t tempSize, char8_t* xmlWholeTag);
    static void processXMLClosingTags(XmlItemList& xmlCurrentEndTagList, XmlItemList& xmlEndTagList, char8_t* tempStr, size_t tempSize, char8_t* xmlWholeTag);
};

} // Blaze

#endif // BLAZE_HTTPPROTOCOLUTIL_H

