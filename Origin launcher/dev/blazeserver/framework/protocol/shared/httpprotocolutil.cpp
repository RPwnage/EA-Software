/*************************************************************************************************/
/*!
    \file httpprotocolutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class HttpProtocolUtil

    Utility functions for the HTTP Protocol.

    \notes

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/httpparam.h"
#include "framework/util/shared/rawbuffer.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "DirtySDK/crypt/cryptsha2.h"
#else
#include "framework/util/shared/blazestring.h"
#include "framework/util/hashutil.h"
#endif

#include "EAStdC/EAString.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

const char8_t HttpProtocolUtil::HTTP_LINE_BREAK[] = "\r\n";
const char8_t HttpProtocolUtil::HTTP_HEADER_HOST[] = "Host:";
const char8_t HttpProtocolUtil::HTTP_HEADER_KEEP_ALIVE[] = "Connection: Keep-Alive";
const char8_t HttpProtocolUtil::HTTP_POST_CONTENTTYPE[] = "Content-Type: application/x-www-form-urlencoded";
const char8_t HttpProtocolUtil::HTTP_POST_CONTENTTYPE_XML[] = "Content-Type: application/xml;charset=UTF-8";
const char8_t HttpProtocolUtil::HTTP_XML_CONTENTTYPE[] = "Content-Type: " XML_CONTENTTYPE;
const char8_t HttpProtocolUtil::HTTP_HEAT_CONTENTTYPE[] = "Content-Type: " HEAT_CONTENTTYPE;
const char8_t HttpProtocolUtil::HTTP_JSON_CONTENTTYPE[] = "Content-Type: " JSON_CONTENTTYPE;

const char8_t HttpProtocolUtil::HTTP_REQUEST_METHOD_ENDING[] = " HTTP/1.1\r\n";
const char8_t HttpProtocolUtil::RESPONSE_CONNECTION[] = "Connection: ";
const char8_t HttpProtocolUtil::HTTP_XML_PAYLOAD[] =  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
const char8_t HttpProtocolUtil::HTTP_XML_PAYLOAD_SEPARATOR[] = "/";

const char8_t HttpProtocolUtil::RESPONSE_LINE_END[] = "\r\n";
size_t HttpProtocolUtil::RESPONSE_LINE_END_SIZE = 2; //size of line end string
const char8_t HttpProtocolUtil::RESPONSE_HEADER_END[] = "\r\n\r\n";
size_t HttpProtocolUtil::RESPONSE_HEADER_END_SIZE = 4; //size of header end string
const char8_t HttpProtocolUtil::RESPONSE_CONTENT_LENGTH[] = "Content-Length: ";
size_t HttpProtocolUtil::RESPONSE_CONTENT_LENGTH_SIZE = 16; //sizeof content length string
const char8_t HttpProtocolUtil::RESPONSE_CONTENT_LOCATION[] = "Content-Location: ";
size_t HttpProtocolUtil::RESPONSE_CONTENT_LOCATION_SIZE = 18; //sizeof the content location string
const char8_t HttpProtocolUtil::RESPONSE_TRANSFER_ENCODING[] = "Transfer-Encoding: chunked";
size_t HttpProtocolUtil::RESPONSE_TRANSFER_ENCODING_SIZE = 26; //sizeof content length string

const char8_t HttpProtocolUtil::strMethods[HTTP_INVALID_METHOD][MAX_HTTPMETHOD_LEN] =
{ {"GET"}, {"POST"}, {"PUT"}, {"DELETE"},{ "HEAD" }, { "PATCH" } };

const char8_t* HttpProtocolUtil::strHttpEncodingTypes[] = 
{ "*", "compress", "deflate", "gzip", "identity" };

const char8_t HttpProtocolUtil::CENSORED_STR[] = "<CENSORED>";
size_t HttpProtocolUtil::CENSORED_STR_LEN = 10;
const char8_t* HttpProtocolUtil::HTTP_CENSORED_HEADERS[] = { "X-Vault-Token: ", nullptr }; // syntax: "<header-name>:<space>"
const char8_t* HttpProtocolUtil::HTTP_FILTERED_FIELDS[] = { "password", "currentPassword", "secretKey", "userPassword", "secret_id", "client_token", "email", "parentalEmail", "dob", nullptr }; // fields to censor should always be listed before fields to scrub
size_t HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[] = { 8, 15, 9, 12, 9, 12, 5, 13, 3 };
size_t HttpProtocolUtil::HTTP_LAST_CENSORED_FIELD_IDX = 5;
size_t HttpProtocolUtil::HTTP_LAST_SCRUBBED_FIELD_IDX = 8;


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief isConnectionClose

    Reads the HTTP header to see if this connection is supposed to stay "alive".

    Looks for:
        Connection: close

    \param[in] header - Buffer to read the HTTP header from
    \param[in] headerSize - Size of the header.

    \return - True if close, false for any other value (or if not found).
*/
/*************************************************************************************************/
bool HttpProtocolUtil::isConnectionClose(const char8_t* header, size_t headerSize)
{
    static const char8_t CLOSE[] = "close";
    bool result = false;

    // Look for the the content-length line.
    char8_t* connectionValue = blaze_stristr(header, RESPONSE_CONNECTION);

    // Make sure it finds it and that it's not past the header length.
    if ((connectionValue != nullptr) && (connectionValue < (header + headerSize)))
    {
        connectionValue += (uint32_t) strlen(RESPONSE_CONNECTION);
        if (blaze_strnicmp(connectionValue, CLOSE, sizeof(CLOSE) - 1) == 0)
        {
            result = true;
        }
    } // if

    return result;
} // isConnectionClose

/*************************************************************************************************/
/*!
    \brief parseUrl

    Parses just the URL out of an HTTP request.

    \param[in] buffer - Buffer to read the HTTP request from
    \param[out] uri - URI being requested
    \param[in] uriLen - size of uri buffer provided by caller

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseUrl(RawBuffer& buffer,
            char8_t* uri, size_t uriLen)
{
    return parseUrl(buffer, uri, uriLen, nullptr);
}

/*************************************************************************************************/
/*!
    \brief parseMethod

    Parses just the method part of an HTTP request.
    Can pass either the full request or just the method part to determine the HttpMethod enum

    \param[in] buffer - char buffer to read the method string from.
    \param[out] method - HTTP method enum. 

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseMethod(const char8_t* data, size_t datalen, HttpMethod& method)
{
    size_t len = 0;
    while (len < datalen)
    {
        const char8_t ch = data[len];
        if (ch == ' ' || ch == '\0')
            break;
        ++len;
    }

    if(len == 0 || len > MAX_HTTPMETHOD_LEN)
        return HTTP_INVALID_REQUEST;

    HttpReturnCode code = HTTP_OK;

    if (blaze_strncmp(data, strMethods[HTTP_GET], len) == 0)
        method = HTTP_GET;
    else if (blaze_strncmp(data, strMethods[HTTP_POST], len) == 0)
        method = HTTP_POST;
    else if (blaze_strncmp(data, strMethods[HTTP_PUT], len) == 0)
        method = HTTP_PUT;
    else if (blaze_strncmp(data, strMethods[HTTP_DELETE], len) == 0)
        method = HTTP_DELETE;
    else if (blaze_strncmp(data, strMethods[HTTP_HEAD], len) == 0)
        method = HTTP_HEAD;
    else if (blaze_strncmp(data, strMethods[HTTP_PATCH], len) == 0)
        method = HTTP_PATCH;
    else
        code = HTTP_INVALID_REQUEST;

    return code;
}

/*************************************************************************************************/
/*!
    \brief hasAcceptEncodingType

    Parse and check whether specified encoding type is specified in the header's Accept-Encoding list, of form
      Accept-Encoding: <encoding type>[;..] [,<encoding type>..]  (http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.3)

    \param[in] encoding - HTTP encoding type enum, to check whether its string exists in the list.
    \param[in] headerMap - header's char buffer, to read Accept-Encoding list from.

    \return - return whether encoding present. If header does not include 'Accept-Encoding', false.
*/
/*************************************************************************************************/
bool HttpProtocolUtil::hasAcceptEncodingType(const HttpHeaderMap& headerMap, HttpAcceptEncoding encoding)
{
    const char8_t* value = HttpProtocolUtil::getHeaderValue(const_cast<HttpHeaderMap&>(headerMap),"Accept-Encoding");
    return hasHttpEncodingType(value, static_cast<int32_t>(encoding));
}

/*************************************************************************************************/
/*!
    \brief hasContentEncodingType

    Parse and check whether specified encoding type is specified in the header's Content-Encoding list, of form
      Content-Encoding: <encoding type>[;..] [,<encoding type>..]  (http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.11)

    \param[in] encoding - HTTP encoding type enum, to check whether its string exists in the list.
    \param[in] headerMap - header's char buffer, to read Content-Encoding list from.

    \return - return whether encoding present. If header does not include 'Content-Encoding', false.
*/
/*************************************************************************************************/
bool HttpProtocolUtil::hasContentEncodingType(const HttpHeaderMap& headerMap, HttpContentEncoding encoding)
{
    const char8_t* value = HttpProtocolUtil::getHeaderValue(const_cast<HttpHeaderMap&>(headerMap),"Content-Encoding");
    return hasHttpEncodingType(value, static_cast<int32_t>(encoding));
}

/*************************************************************************************************/
/*!
    \brief hasHttpEncodingType

    Parse and check whether specified encoding value is specified in the strHttpEncodingTypes

    \param[in] value - string of encoding value.
    \param[in] encoding - HTTP encoding type enum, to check whether its string exists in the list.

    \return - return whether value present. If strHttpEncodingTypes does not include encodingValue, false.
*/
/*************************************************************************************************/
bool HttpProtocolUtil::hasHttpEncodingType(const char8_t* value, int32_t encoding)
{
    if (value == nullptr)
        return false;

    const char8_t* next = value;
    const uint32_t encodingStrLen = static_cast<uint32_t>(strlen(strHttpEncodingTypes[encoding]));
    while ((next != nullptr) && (next[0] != '\0'))
    {
        // skip non-relevant chars before next type string
        if ((next[0] == ' ') || ((next[0] == ',')))
        {
            next++;
            continue;
        }
        // compare the type prefix part of next accept-encoding list entry of form:  <type>[ ;,'\0']
        if ((blaze_strnistr(next, strHttpEncodingTypes[encoding], encodingStrLen) != nullptr) &&
            ((next[encodingStrLen] == '\0') || (next[encodingStrLen] == ' ') ||
             (next[encodingStrLen] == ',') || (next[encodingStrLen] == ';')))
        {
            // next type string matches
            return true;
        }
        // Find delimiter for next accept-encoding list entry
        next = strchr(next, ',');
    }
    return false;
}

/*************************************************************************************************/
/*!
    \brief parseRequest

    Parses an HTTP request.

    \param[in] buffer - Buffer to read the HTTP request from.
    \param[out] method - HTTP method enum.
    \param[out] uri - URI being requested
    \param[in] uriLen - size of uri buffer provided by caller
    \param[out] paramMap - map provided by caller to be filled in with parameters

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseRequest(RawBuffer& buffer,
            HttpMethod& method, char8_t* uri, size_t uriLen, HttpParamMap& paramMap, bool upperCaseKeys /* = false */,
            HttpHeaderMap* headerMap /* = nullptr */)
{
    char8_t* data = reinterpret_cast<char8_t*>(buffer.data());
    
    if (parseMethod(data, buffer.datasize(), method) == HTTP_OK)
    {
        switch(method)
        {
        case HTTP_GET:
        case HTTP_DELETE:
        case HTTP_HEAD:
            return parseInlineRequest(buffer, uri, uriLen, paramMap, upperCaseKeys, headerMap);
        break;
        case HTTP_POST:
        case HTTP_PUT:
        case HTTP_PATCH:
            {
                HttpReturnCode rc = parseInlineRequest(buffer, uri, uriLen, paramMap, upperCaseKeys, headerMap);
                if (rc == HTTP_OK)
                {
                    rc = parsePayloadParams(buffer, paramMap, upperCaseKeys, headerMap);
                }
                return rc;
            }
        default:
        break;
        }
    }

    return HTTP_INVALID_REQUEST;
}

///////////////////////////////////////////////////////////////////////////////
// adapted from UTFInternet URL.cpp URL::ConvertPathToEncodedForm (c) Electronic Arts. All Rights Reserved.
//
// See RFC 1738 for the standard encoding rules.
//
// The file:
//     "/blah/test file.txt"
// becomes:
//     "/blah/test%20file.txt"
//
bool HttpProtocolUtil::urlencode(const char8_t* src, BLAZE_EASTL_STRING& encodedString)
{
    // To do: Make this function more efficient by not using temp String8 objects here.
    size_t len = 0;

    if (src != nullptr)
        len = strlen(src);

    const BLAZE_EASTL_STRING sSafeChars("-._~:?[]@!$&'()*,;=");  // These chars are always safe for URLs.  Note that we intentionally leave out '\' from this list, 
        // even though the URL standard suggests that it is a safe char. We also don't include '+', which is encoded as "%2B", because the HTTP decoder assumes '+' == ' '.
    encodedString.clear(); // Do this assignment after src assignment above, as src may be the same as encodedString.

    for(size_t i = 0; i < len; i++)
    { 
        const char8_t c = src[i];

        if(isalnum(c) ||
            sSafeChars.find(c) < sSafeChars.length())
        {
            encodedString += src[i]; //Char is safe because it is alphanumeric, safe, or one of the not-to-encode chars.
        }
        else
        {
            encodedString += '%';
            // Basically, we do a quick binary to character hex conversion here.
            uint8_t nTemp = (uint8_t)c >> 4;  // Get first hex digit
            nTemp = '0' + nTemp;       // Shift it to be starting at '0' instead of 0.
            if(nTemp > '9')            // If digit needs to be in 'A' to 'F' range.
                nTemp += 7;                     // Shift it to the 'A' to 'F' range.
            encodedString += (char8_t)nTemp;

            nTemp = (uint8_t)c & 0x0f;
            nTemp = '0' + nTemp;
            if(nTemp > '9')
                nTemp += 7;
            encodedString += (char8_t)nTemp;
        }
    }

    return true;
}

HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::urlDecode(char8_t* buffer, size_t bufferSize,
        const char8_t* const input, size_t inputSize, bool terminateBuffer)
{
    HttpReturnCode result = HTTP_OK;
    char8_t* pBuf = buffer;
    char8_t* pInput = const_cast<char8_t*>(input);
    char8_t charconvert = '\0';

    // If inputSize not supplied, find it.
    if (inputSize == 0) inputSize = static_cast<uint32_t>(strlen(input));

    // If termination required, reduce bufferSize by 1.
    if (terminateBuffer)
    {
        bufferSize--;
        *(pBuf + bufferSize) = 0;   // guaranteed termination.
    }

    // loop through and encode input making sure not to write past the end of buffer.
    while((bufferSize > 0) && (inputSize > 0))
    {
        // Decrement these unsigned values inside the loop rather than as part
        // of the while statement which could cause them to wrap around
        --bufferSize;
        --inputSize;

        if (*pInput == '+')
        {
            // make space.
            *pBuf++ = ' ';
        }
        else if (*pInput == '%')
        {
            // convert it from hex.
            charconvert = charHextoInt(++pInput);
            *pBuf = charconvert << 4;
            charconvert = charHextoInt(++pInput);
            *pBuf |= charconvert;
            pBuf++;

            // Reduce buffer size by 2 more.
            inputSize -= 2;
        }
        else if (*pInput == '\0')
        {
            // Just in case there's a nullptr in the middle of pInput,
            // terminate early.
            break;
        } // if
        else
        {
            // just plain old copy.
            *pBuf++ = *pInput;
        } // if

        // Advance to next character.
        pInput++;
    } // while

    // If there's more input, that means output buffer size was not big enough.
    if ((bufferSize == 0) && (inputSize > 0))
    {
        result = HTTP_BUFFER_TOO_SMALL;
    }

    // terminate end.
    if (terminateBuffer) *pBuf = 0;

    return result;
} // urlDecode


/*************************************************************************************************/
/*!
    \brief getHeaderValue

    Get the value of a header given the raw request

    \param[in]  request - The raw HTTP request.
    \param[in]  header - What header to look for (either with or without trailing colon ':')

    \return - Returns pointer to start of header value in the buffer.  Null-termination is not guaranteed.
*/
/*************************************************************************************************/
const char8_t* HttpProtocolUtil::getHeaderValue(const RawBuffer& request, const char8_t* header)
{
    if (header == nullptr || *header == '\0')
    {
        return nullptr;
    }

    const char8_t* data = reinterpret_cast<const char8_t*>(request.data());
    size_t datasize = request.datasize();

    // Skip past the URI to the header
    char8_t* headerData = blaze_strnistr(data,"\r\n",datasize);
    if(headerData == nullptr)
    {
        return nullptr;     // No headers
    }
    datasize -= (headerData - data);

    const char8_t* headerStart = blaze_strnistr(headerData,header,datasize);
    if(headerStart == nullptr)
    {
        return nullptr;     // Specific header not found
    }
    datasize -= (headerStart - headerData);

    // Skip past header name to value
    headerStart += strlen(header);
    datasize -= strlen(header);

    // Skip past any whitespace
    while (datasize > 0 && (*headerStart == ' ' || *headerStart == ':'))
    {
        ++headerStart;
        --datasize;
    }

    if(datasize > 0) {
        return headerStart;
    }

    return nullptr;
}


/*************************************************************************************************/
/*!
    \brief getHeaderValue

    Get the value of a header given a previously created HeaderMap

    \param[in]  headerMap - Previously created map of header names to header values.
    \param[in]  header - What header to look for (without trailing colon ':')

    \return - Returns pointer to header value.  Null-termination is guaranteed.
*/
/*************************************************************************************************/
const char8_t* HttpProtocolUtil::getHeaderValue(const HttpHeaderMap& headerMap, const char8_t* header)
{
    if (headerMap.size() == 0 || header == nullptr || *header == '\0')
    {
        return nullptr;
    }

    HttpHeaderMap::const_iterator key = headerMap.find(header);
    if(key != headerMap.end()) {
         return key->second.c_str();
    }

    return nullptr;
}

HttpProtocolUtil::HttpMethod HttpProtocolUtil::getMethodType(const char8_t* methodName)
{
    if (blaze_strcmp(methodName, "GET") == 0)
    {
        return HTTP_GET;
    }
    else if (blaze_strcmp(methodName, "POST") == 0)
    {
        return HTTP_POST;
    }
    else if (blaze_strcmp(methodName, "PUT") == 0)
    {
        return HTTP_PUT;
    }
    else if (blaze_strcmp(methodName, "DELETE") == 0)
    {
        return HTTP_DELETE;
    }
    else if (blaze_strcmp(methodName, "HEAD") == 0)
    {
        return HTTP_HEAD;
    }
    else if (blaze_strcmp(methodName, "PATCH") == 0)
    {
        return HTTP_PATCH;
    }

    return HTTP_INVALID_METHOD;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

/*************************************************************************************************/
/*!
    \brief digits
    
    Counts the number of base 10 digits in n

    \return - number of digits or 1 if n is zero
*/
/*************************************************************************************************/
static uint32_t digits(uint32_t n)
{
    uint32_t i = 0;
    do
    {
        n/=10;
        ++i;
    }
    while(n > 0);
    return i;
}

HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::buildPayloadHeader(RawBuffer& buffer, size_t& headerSize, const char8_t* hostname, uint32_t port,
                                                                      const char8_t* const method, const char8_t* const URI, 
                                                                      const char8_t* contentType, const char8_t* httpHeaders[]/*=nullptr*/,
                                                                      uint32_t headerCount/* = 0*/)
{
    // allocating an encode buffer on the stack.
    headerSize = strlen(method) + 1 + strlen(HTTP_REQUEST_METHOD_ENDING)
        + strlen(HTTP_HEADER_KEEP_ALIVE) + strlen(contentType)
        + RESPONSE_CONTENT_LENGTH_SIZE + (2 * strlen(HTTP_LINE_BREAK));
    headerSize += strlen(URI);
    headerSize += strlen(HTTP_HEADER_HOST) + 1 + strlen(hostname) + 1
            + digits(port) + strlen(HTTP_LINE_BREAK);
 
    // Including any special headers.
    if (headerCount > 0)
    {
        headerSize += headerCount * strlen(HTTP_LINE_BREAK);
        for (uint32_t i = 0; i < headerCount; i++)
        {
            headerSize += strlen(httpHeaders[i]);
        } // for
    } // if

    // Reserve the header size.
    if (buffer.acquire(headerSize + 10) == nullptr)
        return HTTP_BUFFER_TOO_SMALL;

    buffer.reserve(headerSize + 10);

    return HTTP_OK;
}


/*************************************************************************************************/
/*!
    \brief buildHeaderMap

    Build a mapping of header names to header values given the raw request

    \param[in]  buffer - The raw HTTP request
    \param[out] headerMap - Where to store the entries in the map.

    \return - Returns HTTP_INVALID_REQUEST if the request is malformed somehow.
        Returns HTTP_OK if the header map is successfully built (even if there are 0 headers)
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::buildHeaderMap(const RawBuffer& buffer, HttpHeaderMap& headerMap)
{
    char8_t* data = reinterpret_cast<char8_t*>(buffer.data());
    size_t datasize = buffer.datasize();

    return buildHeaderMap(data, datasize, headerMap);
}

HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::buildHeaderMap(const char8_t* data, size_t bufferSize, HttpHeaderMap& headerMap)
{
    // Need to parse the headers as well.
    size_t datasize = bufferSize;

    // Find the beginning of the first header
    const char8_t* headerNameStart = blaze_strnstr(data, "\r\n", datasize);

    while (headerNameStart != nullptr)    // Break conditions also inside the loop
    {
        headerNameStart += 2;       // Skip \r\n which will put us at the start of the header
        datasize = bufferSize - static_cast<size_t>(headerNameStart - data);

        // Our stopping condition:  reaching the end of the buffer, or the end of the headers
        if (datasize <= 0 || (headerNameStart[0] == '\r' && headerNameStart[1] == '\n'))
        {
            return HTTP_OK;     // No more headers
        }

        // Pull out the header name
        const char8_t* tmp = headerNameStart;
        while (datasize > 0 && *tmp != ':')
        {
            ++tmp;
            --datasize;
        }
        if (datasize == 0)
        {
            return HTTP_INVALID_REQUEST;    // Bad header data
        }
        uint32_t length = static_cast<uint32_t>(tmp - headerNameStart);

        BLAZE_EASTL_STRING headerName(headerNameStart, length);

        // Pull out the header value
        ++tmp;              // Skip past ':'
        --datasize;

        while (datasize > 0 && *tmp == ' ')
        {
            ++tmp;          // Skip past whitespace
            --datasize;
        }
        if (datasize == 0)
        {
            return HTTP_INVALID_REQUEST;    // Bad header data
        }

        const char8_t* headerValueStart = tmp;
        tmp = blaze_strnstr(headerValueStart, "\r\n", datasize);
        if (tmp == nullptr)
        {
            return HTTP_INVALID_REQUEST;    // Bad header data
        }
        length = static_cast<uint32_t>(tmp - headerValueStart);
        BLAZE_EASTL_STRING headerValue(headerValueStart, length);

        // Add the name/value pair to the map
        headerMap[headerName] = headerValue;

        headerNameStart = tmp;      // Prepare for the next header
    }

    return HTTP_OK;
}

/*************************************************************************************************/
/*!
    \brief parseUrl

    Parses just the URL out of an HTTP request.

    \param[in] buffer - Buffer to read the HTTP request from
    \param[out] uri - URI being requested
    \param[in] uriLen - size of uri buffer provided by caller
    \param[out] uriEnd - if non nullptr will provide the caller with a pointer to the character at
                         the end of the URI (either a ' ' or a '?')

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseUrl(const RawBuffer& buffer,
            char8_t* uri, size_t uriLen, char8_t** uriEnd)
{
    char8_t* data = reinterpret_cast<char8_t*>(buffer.data());
    char8_t* tail = reinterpret_cast<char8_t*>(buffer.tail());

    // find first space
    char8_t* ch = blaze_strnstr(data, " ", tail - data);
    if (ch == nullptr)
        return HTTP_INVALID_REQUEST;
    
    ++ch; // consume space
    
    // find first carriage return/linefeed after first space
    char8_t* end = blaze_strnstr(ch, "\r\n", tail - ch);
    if (end == nullptr)
        return HTTP_INVALID_REQUEST;

    char8_t* uriMax = uri + uriLen;
    while (ch < end && *ch != ' ' && *ch != '?' && uri < uriMax)
    {
        *uri++ = *ch++;
    }

    if (uri == uriMax)
        return HTTP_BUFFER_TOO_SMALL;

    *uri = '\0';
    if (uriEnd != nullptr)
        *uriEnd = ch;

    return HTTP_OK;
}


/*************************************************************************************************/
/*!
    \brief parseParameters

    Parse standard URLencoded parameters using the name1=value1&name2=value2&... syntax

    \param[in]  buffer - Buffer containing parameters to be parsed
    \param[in]  length - Lenth of the data in the buffer
    \param[out] paramMap - Where to store the parameter name to value mapping
    \param[in]  upperCaseKeys - If true, parameter (key) names will be converted to upper case

    \return - Returns HTTP_INVALID_REQUEST if the request is malformed somehow.
        Returns HTTP_OK if the parameters are successfully parsed (even if there are 0 parameters)

    \notes
        URL decodes the parameters before parsing.
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseParameters(const char8_t* buffer, const size_t length, HttpParamMap& paramMap, bool upperCaseKeys /*= false*/)
{
    if (buffer == nullptr || length == 0)
    {
        return HTTP_OK;             // Nothing to do
    }

    // Make a copy of the buffer since we're going to munge it for the URL decode
    char8_t* bufferCopy = (char8_t*) BLAZE_ALLOC_MGID(length + 1, MEM_GROUP_FRAMEWORK_HTTP, "HTTPBUFFERCOPY");    // We're going to nullptr-terminate it
    blaze_strnzcpy(bufferCopy, buffer, length + 1);

    int32_t lengthCopy = static_cast<int32_t>(length);
    char8_t* data = bufferCopy;
    char8_t* start = bufferCopy;
    char8_t* key = bufferCopy;
    char8_t* value = nullptr;
    bool done = false;
    do
    {
        char ch = *data;

        if ((ch == '=') || (ch == '&') || (ch == ' ') || (ch == '\0'))
        {
            // Caution - this assumes that url decoding can safely accept
            // the same input and output buffer since decoded data should
            // only get shorter than the input
            urlDecode(start, (data - start) + 1, start, data - start, true);

            start = data + 1;
            if ((ch == '&') || (ch == ' ') || (ch == '\0'))
            {
                if (ch == ' ' || ch == '\0')
                    done = true;

                // Reject values with empty key
                if (strlen(key) > 0 && value != nullptr)
                {
                    if (upperCaseKeys)
                        blaze_strupr(key);
                    paramMap[key] = value;
                }

                key = start;
                value = nullptr;
            }
            else if (ch == '=')
            {
                value = start;
            }
        }
        ++data;
        --lengthCopy;
    }
    while (lengthCopy >= 0 && !done);

    BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_HTTP, bufferCopy);

    return (done == true) ? HTTP_OK : HTTP_INVALID_REQUEST;
}


/*************************************************************************************************/
/*!
    \brief parseInlineRequest

    Parses an HTTP GET, HTTP HEAD, or HTTP DELETE request.

    \param[in] buffer - Buffer to read the HTTP request from
    \param[out] uri - URI being requested
    \param[in] uriLen - size of uri buffer provided by caller
    \param[out] paramMap - map provided by caller to be filled in with parameters
    \param[in] upperCaseKeys - if true, parameter names are converted to upper case
    \param[out] headerMap - optional map provided by caller to be filled in with header names and values

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parseInlineRequest(RawBuffer& buffer,
            char8_t* uri, size_t uriLen, HttpParamMap& paramMap, bool upperCaseKeys /* = false */, HttpHeaderMap* headerMap /* = nullptr */)
{
    char8_t* data = nullptr; // this is set to point somewhere within 'buffer'
    HttpReturnCode code = parseUrl(buffer, uri, uriLen, &data);
    if (code != HTTP_OK)
        return code;

    if (*data++ == '?')
    {
        char8_t* tail = reinterpret_cast<char8_t*>(buffer.tail());
        code = parseParameters(data, tail - data, paramMap, upperCaseKeys);
        if (code != HTTP_OK)
        {
            return code;
        }
    }
    
    return (headerMap != nullptr) ? buildHeaderMap(buffer, *headerMap) : HTTP_OK;   // Doesn't do anything if no headerMap provided
}

/*************************************************************************************************/
/*!
    \brief parsePayloadParams

    Parses the query parameters out of an HTTP POST or HTTP PUT request iff the payload
    data is URL encoded.

    \param[in] buffer - Buffer to read the HTTP request from.
    \param[out] paramMap - map provided by caller to be filled in with parameters

    \return - return code indicating success or an error
*/
/*************************************************************************************************/
HttpProtocolUtil::HttpReturnCode HttpProtocolUtil::parsePayloadParams(RawBuffer& buffer,
            HttpParamMap& paramMap, bool upperCaseKeys /* = false */, HttpHeaderMap* headerMap /* = nullptr */)
{
    // If there is a header map, use it to get the content type
    const char8_t *encoding = nullptr;
    if (headerMap != nullptr)
    {
        encoding = getHeaderValue(*headerMap, "Content-Type");
    }
    else
    {
        encoding = getHeaderValue(buffer, "Content-Type");
    }

    if (encoding == nullptr)
    {
        return HTTP_INVALID_REQUEST;
    }

    const char URL_ENCODING[] = "application/x-www-form-urlencoded";
    if (blaze_strncmp(URL_ENCODING, encoding, strlen(URL_ENCODING)) != 0)
    {
        return HTTP_OK;     // Not encoded for us to parse, but that's okay
    }

    // The encoding is correct, now get the content length
    const char8_t *contentLength = nullptr;
    if (headerMap != nullptr)
    {
        contentLength = getHeaderValue(*headerMap, "Content-Length");
    }
    else
    {
        contentLength = getHeaderValue(buffer, "Content-Length");
    }

    if (contentLength == nullptr)
    {
        return HTTP_INVALID_REQUEST;
    }

    size_t length = (size_t)(EA::StdC::AtoU64(contentLength));   

    const char8_t* data = reinterpret_cast<const char8_t*>(buffer.data());
    size_t datasize = buffer.datasize();

    // Go to the body of the request and extract out the name/value parameters.
    const char8_t* body = blaze_strnstr(data, "\r\n\r\n", datasize);
    if (body == nullptr)
    {
        return HTTP_INVALID_REQUEST;    // Should be a request body
    }
    
    body += 4;                          // Skip to start of body
    datasize = buffer.datasize() - static_cast<size_t>(body - data);

    if (length != datasize)
    {
        // Content-Length and actual length disagree - The data may have been compressed
        length = datasize;
    }

    // Extract out the name/value pairs
    return parseParameters(body, length, paramMap, upperCaseKeys);
}

uint8_t HttpProtocolUtil::charHextoInt(const char8_t* const inptr)
{
    uint8_t result = 0;
    char8_t inchar = *inptr;
    if ((inchar >= 'a') && (inchar <= 'f'))
    {
        inchar -= ('a' - 'A');
    } // if

    if ((inchar >= 'A') && (inchar <= 'F'))
    {
        result = 10 + (inchar - 'A');
    }
    else if ((inchar >= '0') && (inchar <= '9'))
    {
        result = (inchar - '0');
    } // if

    return result;

} // charHextoInt

bool HttpProtocolUtil::isUnreservedChar(const char8_t chrCheck)
{
    if (isalnum(chrCheck))
    {
        return true;
    }

    // Check for unreserved characters: -._*
    return false;

} // isUnreservedChar

/*************************************************************************************************/
/*!
    \brief readStatusCodeLine

    Reads the HTTP header and stores the status code and reason line.

    Looks for:
        HTTP/1.x xxx yyyyy\r\n

    Where xxx is the 3 digit status code and yyyy is the reason line text.

    \param[in] header - Buffer to read the HTTP header from
    \param[in] headerSize - Size of the header.

    \return - Return the size found, or -1 if error.
*/
/*************************************************************************************************/
bool HttpProtocolUtil::readStatusCodeLine(const char8_t* header, size_t headerSize,
        int32_t& statusCode, char8_t** reasonLine, size_t& reasonSize)
{
    bool result = false;

    // Make sure the line starts w/ "HTTP"
    if (blaze_strncmp(header, "HTTP", 4) != 0)
    {
        return result;
    }

    // Look for the 1st space.
    char8_t* space = blaze_stristr(header, " ");
    if ((space == nullptr) || (space > (header + headerSize)))
    {
        return result;
    }

    // Parse out the code. Store the location of the reason line.
    int32_t status;
    char8_t* reason = blaze_str2int(++space, &status);
    if (reason == space)
    {
        return result;
    }
    reason++;

    // Get the size of the reason line.
    char8_t* crlf = blaze_stristr(header, "\r\n");
    if ((crlf == nullptr) || (crlf > (header + headerSize)))
    {
        return result;
    }

    statusCode = status;

    if (reason < crlf)
    {
        *reasonLine = reason;
        reasonSize = crlf - reason;
    }

    return true;
}// readStatusCodeLine


/*! ************************************************************************************************/
/*! \brief parse paramName, form xml tags base on paramName 
    
    \param[in]  paramName - xml item tags, could be a single tag or tags separated by HTTP_XML_PAYLOAD_SEPARATOR
    \param[in]  paramValue - value for the closest xml item tag
    \param[in]  repeatItem - repeat item closest tag, if it's nullptr, means this is not a repeat item
    \param[out] xmlOutput - xml payload formed for the given paramName and paramValue
    \param[out] xmlClosingTagList - cached closing tags for the given paramName and paramValue, in case the next
                                item still belongs to same parent tag, we cached the closing tag for previous 
                                item and check with current item to determine when we should add the closing tag
    \return void
***************************************************************************************************/
void HttpProtocolUtil::buildXMLItem(const char8_t* paramName, const char8_t* paramValue, const char8_t* repeatItem, char8_t* tempStr, size_t tempSize, char8_t* xmlOutput, XmlItemList& xmlClosingTagList)
{
    XmlItemList BLAZE_SHARED_EASTL_VAR(xmlTagList, MEM_GROUP_FRAMEWORK_TEMP, "HttpProtocolUtil::buildXMLItem::xmlTagList");
   
    // first let's parse the paramName to tag(s) using the separator
    char8_t* newPos = nullptr;
    char8_t* curPos = const_cast<char8_t*>(paramName);
    char8_t xmlTemp[Blaze::Collections::MAX_ATTRIBUTENAME_LEN];
    while (curPos != nullptr)    
    {
        newPos = blaze_strstr(curPos, HTTP_XML_PAYLOAD_SEPARATOR);
        if (newPos != nullptr)    
        {
            Blaze::Collections::AttributeValue xmlTag;
            blaze_strnzcpy(xmlTemp, curPos, strlen(curPos) - strlen(newPos) + 1);
            xmlTagList.push_back(xmlTemp);
            curPos = ++newPos;
        }
        else
        {
            if (curPos != nullptr)
            {
                Blaze::Collections::AttributeValue xmlTag;
                blaze_snzprintf(xmlTemp, sizeof(xmlTemp), "%s", curPos);
                xmlTagList.push_back(xmlTemp);
            }
            break;
        }
    }

    // if we got parsed tag(s), we need to use tag and paramValue to form xml payload
    // in the format of <tag1>...<tagN>paramvalue</tagN>
    if (!xmlTagList.empty())
    {
        // form xml start tags
        processXMLStartTags(xmlTagList, xmlClosingTagList, repeatItem, tempStr, tempSize, xmlOutput);

        // add value to xml string
        blaze_strnzcat(xmlOutput, paramValue, XML_PAYLOAD_CONTENT_MAX_LENGTH);

        // add closing tag to xml string
        processXMLClosingTags(xmlTagList, xmlClosingTagList, tempStr, tempSize, xmlOutput);
    }

    // if previous tag list is empty and the tag for current paramName
    // is not a single tag, we cached current tag list to the ClosingTagList 
    // for later using
    if (xmlClosingTagList.empty() && (xmlTagList.size() >= 1))
    {
        xmlClosingTagList = xmlTagList;
    }

    return;
}

/*! ************************************************************************************************/
/*! \brief generate start tags for xml item based on current start tag list and previous end tag list

    \param[in]  xmlCurrentStartTagList - tag list for this xml item
    \param[in]  xmlPreviousClosingTagList - tag list except the closest tag for last xml item
    \param[in]  repeatItem - repeat item closest tag, if it's nullptr, means this is not a repeat item
    \param[out] xmlOutput - xml generated for this item
****************************************************************************************************/
void HttpProtocolUtil::processXMLStartTags(XmlItemList& xmlCurrentStartTagList, XmlItemList& xmlPreviousClosingTagList, const char8_t* repeatItem, char8_t* tempStr, size_t tempSize, char8_t* xmlOutput)
{
    int32_t tagNum = static_cast<int32_t>(xmlCurrentStartTagList.size());
    if (tagNum == 0)
        return;

    // In the case current xml item has more than 1 tags
    // check if there is any level tag are same as previous item
    int32_t previousTagNum = static_cast<int32_t>(xmlPreviousClosingTagList.size());
    int32_t commTagNum = (previousTagNum > tagNum) ? tagNum : previousTagNum;
    int32_t differentTag = 0;
    int32_t i = 0;
    for (; i < commTagNum; ++i)
    {
        if (blaze_stricmp(xmlCurrentStartTagList.at(i).c_str(), xmlPreviousClosingTagList.at(i).c_str()) == 0)
            continue;

        break;
    }

    // the first different tag between current item and previous item
    differentTag = i;

    if ((previousTagNum > 0) && (differentTag <= (previousTagNum - 1)))
    {
        // add closing tag(s) which current item does not belong to to previous item 
        int32_t cachedTagNum = previousTagNum - differentTag;
        for (int32_t num = previousTagNum - 1; (num > cachedTagNum); num--)
        {
            blaze_snzprintf(tempStr, tempSize, "</%s>", xmlPreviousClosingTagList.at(num).c_str());
            blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
        }

        if ((cachedTagNum == differentTag) && (differentTag == 1))
        {
            blaze_snzprintf(tempStr, tempSize, "</%s>", xmlPreviousClosingTagList.at(cachedTagNum).c_str());
            blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
        }

        // clear previous end tag list if both items are totally different and all closing tags
        // has been added to previous item
        if (differentTag == 0)
            xmlPreviousClosingTagList.clear();
    }
    else
    {
        if (repeatItem)
        {
            blaze_snzprintf(tempStr, tempSize, "</%s>", repeatItem);
            blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
        }
    }
    
    if (repeatItem)
    {
        blaze_snzprintf(tempStr, tempSize, "<%s>", repeatItem);
        blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
    }

    // Now we add start tags to current item, from the first one which is different from previous tag
    for (int32_t j = differentTag; j < tagNum; j++)
    {
        blaze_snzprintf(tempStr,tempSize, "<%s>", xmlCurrentStartTagList.at(j).c_str());
        blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
    }

    // we may need to add some extra tag to previous end tag list, we clear
    // previous tag list so later on we use current tag to do re-cache
    if ((differentTag != 0) && ((tagNum - differentTag) >= 1))
    {
        xmlPreviousClosingTagList.clear();
    }

    return;
}

/*! ************************************************************************************************/
/*! \brief generate start tags for xml item based on current start tag list and previous end tag list

    \param[in]  xmlCurrentClosingTagList - tag list for this xml item
    \param[in]  xmlClosingTagList - closing tag list for previous tag except the most close one
    \param[out] xmlOutput - xml generated for this item
****************************************************************************************************/
void HttpProtocolUtil::processXMLClosingTags(XmlItemList& xmlCurrentClosingTagList, XmlItemList& xmlClosingTagList, char8_t* tempStr, size_t tempSize, char8_t* xmlOutput)
{
    size_t tagNum = xmlCurrentClosingTagList.size();
    if (tagNum <= 1)
    {
        return;
    }
 
    // add the lowest level close tag </abc> 
    blaze_snzprintf(tempStr, tempSize, "</%s>", xmlCurrentClosingTagList.at((XmlItemList::size_type)(tagNum - 1)).c_str());
    blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
    xmlCurrentClosingTagList.pop_back();

    int32_t currentTagNum = static_cast<int32_t>(xmlCurrentClosingTagList.size());
    int32_t previousTagNum = static_cast<int32_t>(xmlClosingTagList.size());
    if (previousTagNum == currentTagNum)
    {
        // when tag num are same and  current closing tag list are different from previous one
        // do nothing
        for (int32_t i = 0; i < currentTagNum; ++i)
        {
            if (blaze_stricmp(xmlClosingTagList.at(i).c_str(), xmlCurrentClosingTagList.at(i).c_str()) != 0)
                return;
        }
    }
    else if (previousTagNum > currentTagNum)
    {
        // when current closing tag has smaller list than previous tag num, add extra one as closing tag
        // for current item
        for (int32_t i = previousTagNum - 1; i > currentTagNum ; i--)
        {
            blaze_snzprintf(tempStr, tempSize, "</%s>", xmlClosingTagList.at(i).c_str());
            blaze_strnzcat(xmlOutput, tempStr, XML_PAYLOAD_CONTENT_MAX_LENGTH);
            xmlClosingTagList.pop_back();
        }
    }
  
    return;
}

/*! ************************************************************************************************/
/*! \brief returns string form of the HTTP method

    \param[in]  method - the HTTP method
****************************************************************************************************/

const char8_t* HttpProtocolUtil::getStringFromHttpMethod(Blaze::HttpProtocolUtil::HttpMethod method)
{
    switch (method)
    {
    case HTTP_GET:
        return "GET";
    case HTTP_PUT:
        return "PUT";
    case HTTP_POST:
        return "POST";
    case HTTP_DELETE:
        return "DELETE";
    case HTTP_HEAD:
        return "HEAD";
    case HTTP_PATCH:
        return "PATCH";
    default:
        return "INVALID_METHOD";
    }
}

/*! ************************************************************************************************/
/*! \brief Fetch HTTP request into provided buffer. Also update to cache key, to no longer incorrectly apply censoring to headers
****************************************************************************************************/
void HttpProtocolUtil::getHttpCacheKey(bool isSecure, HttpMethod method, const char8_t* hostname, uint16_t port, const char8_t* uri, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[],
    uint32_t headerCount, eastl::string& httpCacheKey)
{
    bool success = false;
    StringBuilder key;
    if (port == 0)
    {
        success = key.append("%s %s://%s%s%s", getStringFromHttpMethod(method),
            isSecure ? "https" : "http", hostname, (uri == nullptr || *uri != '/') ? "/" : "", uri);
    }
    else
    {
        success = key.append("%s %s://%s:%u%s%s", getStringFromHttpMethod(method),
            isSecure ? "https" : "http", hostname, port, (uri == nullptr || *uri != '/') ? "/" : "", uri);

    }
    if (!success)
        return; //overflow
    
    for (uint32_t i = 0; i < paramsize; i++)
    {
        success = key.append("%s%s=%s", (i > 0) ? "&" : "", params[i].name, params[i].value);
        if (!success)
            return; //overflow
    }

    for (uint32_t i = 0; i < headerCount; i++)
    {
        success = key.append("\n%s", httpHeaders[i]);

        if (!success)
            return; //overflow
    }
    httpCacheKey = key.get();
}

/*! ************************************************************************************************/
/*! \brief prints HTTP request into logging format into provided buffer

    \param[in]  method - the HTTP method
    \param[in]  url - the url set by setEasyOpts()
    \param[in]  paramString - the parameter string set by setEasyOpts()
    \param[in]  headerList - the list of headers set by setEasyOpts()
    \param[in]  buffer - the buffer to print log into
    \param[in]  bufferSize - the size of the buffer
****************************************************************************************************/
void HttpProtocolUtil::printHttpRequest(bool isSecure, HttpMethod method, const char8_t* hostname, uint16_t port, const char8_t* uri, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], 
        uint32_t headerCount, eastl::string& outString, bool censorValues)
{
    if (port == 0)
    {
        outString.sprintf("%s %s://%s%s%s", getStringFromHttpMethod(method),
            isSecure ? "https" : "http", hostname, (uri == nullptr || *uri != '/')? "/":"", uri);
    }
    else
    {
        outString.sprintf("%s %s://%s:%u%s%s", getStringFromHttpMethod(method),
            isSecure ? "https" : "http", hostname, port, (uri == nullptr || *uri != '/')? "/":"", uri);
        
    }

    if (paramsize > 0)
    {
        outString.append("?");
    }

    for (uint32_t i = 0; i < paramsize; i++)
    {
        EA::TDF::TdfMemberInfo::PrintFormat printFormat = censorValues ? getPrintFormatForField(params[i].name) : EA::TDF::TdfMemberInfo::NORMAL;
        switch(printFormat)
        {
        case EA::TDF::TdfMemberInfo::CENSOR:
            outString.append_sprintf("%s%s=%s", (i > 0) ? "&" : "", params[i].name, CENSORED_STR);
            break;
        case EA::TDF::TdfMemberInfo::HASH:
#ifdef BLAZE_CLIENT_SDK
            uint8_t pIIHash[CRYPTSHA256_HASHSIZE];
            CryptSha2T sha256;
            CryptSha2Init(&sha256, CRYPTSHA256_HASHSIZE);
            CryptSha2Update(&sha256, (uint8_t*)params[i].value, (uint32_t)strlen(params[i].value));
            CryptSha2Final(&sha256, pIIHash, CRYPTSHA256_HASHSIZE);
            outString.append_sprintf("%s%s=%s", (i > 0) ? "&" : "", params[i].name, (char8_t*)pIIHash);
#else
            char8_t pIIHash[HashUtil::SHA256_STRING_OUT];
            HashUtil::generateSHA256Hash(params[i].value, strlen(params[i].value), pIIHash, HashUtil::SHA256_STRING_OUT);
            outString.append_sprintf("%s%s=%s", (i > 0) ? "&" : "", params[i].name, pIIHash);
#endif
            break;
        default:
            // these parameters are not properly URL encoded, just plain text formatted for logging purposes
            outString.append_sprintf("%s%s=%s", (i > 0) ? "&" : "", params[i].name, params[i].value);
            break;
        }
    }

    for (uint32_t i = 0; i < headerCount; i++)
    {
        size_t headerIndex;
        if (censorValues && shouldCensorHeader(httpHeaders[i], &headerIndex))
            outString.append_sprintf("\n%s%s", HTTP_CENSORED_HEADERS[headerIndex], CENSORED_STR);
        else
            outString.append_sprintf("\n%s", httpHeaders[i]);
    }
}

/*********************************************************************************************/
/*!
    \brief replaceRawBufferData

    Replace data in raw buffer with new data

    NOTE: This will not replace the data in the buffer from data() to tail() (datasize())

    \param[out] success - Was the data successfully replaced
    \param[in] rawBuffer - Buffer to replace data in
    \param[in] newData - New data
    \param[in] newDataLen - size of new data
*/
/*********************************************************************************************/
bool HttpProtocolUtil::replaceRawBufferData(RawBuffer& rawBuffer, uint8_t* newData, size_t newDataLen)
{          
    // There is not enough space for the compressed data, reuse buffer
    if (newDataLen > (rawBuffer.datasize() + rawBuffer.tailroom()))
    {            
        // Resize raw buffer            
        uint8_t* data = rawBuffer.acquire(newDataLen);

        // Early return if we were unable to allocate a new buffer
        if (data == nullptr)
        {
            //Unable to allocate buffer large enough for new data
            return false;
        }            
    }

    // Set new end of data
    rawBuffer.trim(rawBuffer.datasize());
    if (rawBuffer.datasize() != 0)
    {
        return false;
    }
    rawBuffer.put(newDataLen);

    // Copy compressed data to raw buffer
    memcpy(rawBuffer.data(), newData, newDataLen);

    return true;
}



/*! ************************************************************************************************/
/*! \brief getHostnameFromConfig

    Get hostname from address, and check the http/https to get the port.

    \param[in]  serverAddress - url
    \param[in]  hostname 
    \param[in]  isSecure
****************************************************************************************************/
void HttpProtocolUtil::getHostnameFromConfig(const char8_t* serverAddress, const char8_t*& hostname, bool &isSecure)
{
    // Check for http/https in address.  If none, then default is "http".
    //According to the http/https to set the secure. 
    char8_t* pMatchHttp = nullptr;
    char8_t* pMatchHttps = nullptr;

    pMatchHttp = blaze_strnistr(serverAddress, "http://", strlen(serverAddress));
    pMatchHttps = blaze_strnistr(serverAddress, "https://", strlen(serverAddress));

    if (pMatchHttp != nullptr)

    {
        pMatchHttp += strlen("http://");
        isSecure = false;
        hostname = pMatchHttp;
    }
    else if (pMatchHttps != nullptr)
    {
        pMatchHttps += strlen("https://");
        isSecure = true;
        hostname = pMatchHttps;
    }
    else
    {
        isSecure = false;
        hostname = serverAddress;
    }
}

/*! ************************************************************************************************/
/*! \brief shouldCensorHeader

    Returns true if the given header should be censored in Blaze logs; false otherwise

    \param[in] header - the header name
    \param[out] headerIndex (optional) - if provided, this is set to the index of the matched header in HTTP_CENSORED_HEADERS

****************************************************************************************************/
bool HttpProtocolUtil::shouldCensorHeader(const char8_t* header, size_t* headerIndex)
{
    uint32_t i = 0;
    while (HTTP_CENSORED_HEADERS[i] != nullptr)
    {
        // Only compare header name up to the colon
        if (blaze_strncmp(header, HTTP_CENSORED_HEADERS[i], strlen(HTTP_CENSORED_HEADERS[i]) - 2) == 0)
        {
            if (headerIndex != nullptr)
                *headerIndex = i;
            return true;
        }
        ++i;
    }
    return false;
}

/*! ************************************************************************************************/
/*! \brief getPrintFormatForField

    Returns:
        CENSOR if the given url parameter / payload field should be censored in Blaze logs
        HASH   if the given url parameter / payload field should be scrubbed in Blaze logs
        NORMAL if the given url parameter / payload field should be printed in plaintext in Blaze logs

    \param[in] fieldName - the url parameter / payload field name

****************************************************************************************************/
EA::TDF::TdfMemberInfo::PrintFormat HttpProtocolUtil::getPrintFormatForField(const char8_t* fieldName)
{
    for (uint32_t i = 0; i <= HTTP_LAST_CENSORED_FIELD_IDX; ++i)
    {
        if (blaze_strncmp(fieldName, HttpProtocolUtil::HTTP_FILTERED_FIELDS[i], HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]) == 0)
            return EA::TDF::TdfMemberInfo::CENSOR;
    }
    for (uint32_t i = 0; i <= HTTP_LAST_SCRUBBED_FIELD_IDX; ++i)
    {
        if (blaze_strncmp(fieldName, HttpProtocolUtil::HTTP_FILTERED_FIELDS[i], HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]) == 0)
            return EA::TDF::TdfMemberInfo::HASH;
    }
    return EA::TDF::TdfMemberInfo::NORMAL;
}

#ifndef BLAZE_CLIENT_SDK
/*********************************************************************************************/
/*! 
    \brief getClientIPFromXForwardForHeader

    Parse the X-Forward-For header to retrieve the Client IP Address.

    \param[in]  forwardedForHeader - value of the X-Forwarded-For header. nullptr allowed.
    \param[out]  clientAddr - clientAddr parsed from header value
*/
/*********************************************************************************************/
void HttpProtocolUtil::getClientIPFromXForwardForHeader(const char8_t* forwardedForHeader, InetAddress& clientAddr)
{
    if (forwardedForHeader != nullptr)
    {
        //Parse X-Forward-For header to retrieve Client IP. Format: "X-Forwarded-For: clientIPAddress, ELBIPAddress-1, ..."
        const char8_t* clientIPAddressEnd = strchr(forwardedForHeader, ',');

        int result = 0;
        in_addr sinAddr;

        if (clientIPAddressEnd != nullptr)
        {
            char8_t clientIPAddress[InetAddress::MAX_HOSTNAME_LEN];
            blaze_strcpyFixedLengthToNullTerminated(clientIPAddress, InetAddress::MAX_HOSTNAME_LEN, forwardedForHeader, clientIPAddressEnd - forwardedForHeader);
            result = inet_pton(AF_INET, clientIPAddress, &sinAddr);
        }
        else
        {
            result = inet_pton(AF_INET, forwardedForHeader, &sinAddr);
        }

        //Verify value returned from inet_addr is not invalid
        if (result > 0)
        {
            clientAddr.setIp(sinAddr.s_addr, InetAddress::NET);
        }
    }
}
#endif

} // Blaze

