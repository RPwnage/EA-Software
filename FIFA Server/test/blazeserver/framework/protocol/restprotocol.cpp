/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RestProtocol

    Parses Web Access Layer (REST) requests and transmits standard HTTP responses.

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

        */
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/component/blazerpc.h"
#include "framework/connection/connection.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/restprotocol.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/locales.h"

// Zlib Compression
#include "framework/util/compression/zlibdeflate.h"

namespace Blaze
{

    /*** Defines/Macros/Constants/Typedefs ***********************************************************/
    bool RestProtocol::sUseCompression = false;
    int32_t RestProtocol::sCompressionLevel = ZlibDeflate::DEFAULT_COMPRESSION_LEVEL;

    /*** Public Methods ******************************************************************************/

    /*************************************************************************************************/
    /*!
    \brief RestProtocol

    Constructor.
    */
    /*************************************************************************************************/
    RestProtocol::RestProtocol()
        : mNextSeqno(0),
          mMethod(HttpProtocolUtil::HTTP_INVALID_METHOD),
          mContentLength(-1),
          mIsCompressionEnabled(false),
          mMaxFrameSize(UINT16_MAX+1)
    {
    }

    /*************************************************************************************************/
    /*!
    \brief ~RestProtocol

    Destructor.
    */
    /*************************************************************************************************/
    RestProtocol::~RestProtocol()
    {
    }

    /*********************************************************************************************/
    /*!
    \brief receive

    Determine if the data in the buffer is a complete HTTP request.
    If not, return the number of bytes still required to complete the frame.  Note that HTTP
    requests are not fixed-length, and the only case where the required length will be
    known is when parsing the body of a post based on the Content-Length header.  Where the
    request is not complete but the amount of data required cannot be determined, some
    arbitrary positive number will be returned.

    NOTE: By asking for a chunk of data, we may end up requesting more data than the current
    request contains.  It is then possible that we could pick off the beginning of a
    subsequent pipelined HTTP request from the client.  If we expect pipelined requests,
    we will either need to add some ability in our protocol framework to push back the excess
    data, or we'll have to be much less efficient and request data in smaller chunks (i.e. just
    one or a handful of bytes at a time).

    \param[in]  buffer - Buffer to examine.

    \return - 0 if frame is complete; number of bytes still required if frame is not complete
    */
    /*********************************************************************************************/
    bool RestProtocol::receive(RawBuffer& buffer, size_t& needed)
    {
        BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Receiving request");

        size_t available = buffer.datasize();
        char8_t* data = reinterpret_cast<char8_t*>(buffer.data());

        // First step is to determine the type of HTTP request (i.e. GET, POST, etc.)
        if (mMethod == HttpProtocolUtil::HTTP_INVALID_METHOD)
        {
            BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Reading method.");

            mContentLength = -1;

            // The longest currently defined HTTP request header is 7 characters long, so once we
            // have 8 characters we should definitely have a space-terminated request method
            if (available < 8)
            {
                needed = 1024;
                return true;
            }

            if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_GET], 3) == 0 && data[3] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_GET;
            }
            else if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_HEAD], 4) == 0 && data[4] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_HEAD;
            }
            else if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_POST], 4) == 0 && data[4] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_POST;
            }
            else if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_PUT], 3) == 0 && data[3] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_PUT;
            }
            else if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_DELETE], 6) == 0 && data[6] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_DELETE;
            }
            else if (blaze_strncmp(data, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_PATCH], 5) == 0 && data[5] == ' ')
            {
                mMethod = HttpProtocolUtil::HTTP_PATCH;
            }
            else
            {
                // It's not a request type that we handle, so don't bother asking for more data
                BLAZE_WARN_LOG(Log::REST, "[RestProtocol].receive: Unknown method.");
                needed = 0;
                return false;
            }
        }

        if (mMethod == HttpProtocolUtil::HTTP_GET || mMethod == HttpProtocolUtil::HTTP_HEAD)  // URI-only requests
        {
            const char8_t* methodName = HttpProtocolUtil::strMethods[mMethod];
            BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Looking for complete " << methodName << " request.");

            // Look for two CRLF's which terminate an HTTP Get or Head request
            if (blaze_strnstr(data, "\r\n\r\n", available) != nullptr)
            {
                BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Received complete " << methodName << " request.");
                needed = 0;
                return true;
            }
            else
            {
                BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Need more data for " << methodName << " request.");
                needed = 1024;
                return true;
            }
        }
        else if (mMethod == HttpProtocolUtil::HTTP_POST || mMethod == HttpProtocolUtil::HTTP_PUT || mMethod == HttpProtocolUtil::HTTP_DELETE || mMethod == HttpProtocolUtil::HTTP_PATCH)   // URI+data requests
        {
            const char8_t* methodName = HttpProtocolUtil::strMethods[mMethod];
            BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Looking for complete " << methodName << " request.");

            if (mContentLength == -1)
            {
                const char8_t header[] = "\r\nContent-Length: ";
                size_t headerLen = strlen(header);
                const char8_t* str = blaze_strnstr(data, header, available);
                if (str == nullptr)
                {
                    if (mMethod == HttpProtocolUtil::HTTP_DELETE)
                    {
                        // Look for two CRLF's which terminate an HTTP Delete request as Delete request might not have Content-Length specified
                        if (blaze_strnstr(data, "\r\n\r\n", available) != nullptr)
                        {
                            BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Received complete " << methodName << " request.");
                            needed = 0;
                            return true;
                        }
                    }
                    BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Still need content length header for " << methodName 
                        << " request.");
                    needed = 1024;
                    return true;
                }
                if (blaze_strnstr(str + headerLen, "\r\n", available - (str + headerLen - data)) != nullptr)
                {
                    blaze_str2int(str + headerLen, &mContentLength);
                    if (mContentLength >= 0 && (uint32_t)mContentLength > mMaxFrameSize)
                    {
                        BLAZE_WARN_LOG(Log::HTTPXML, "[RestProtocol].receive: Incoming frame is too large (" << mContentLength << "); "
                            "max frame size allowed is " << mMaxFrameSize);
                        needed = 0;
                        return false;
                    }
                }
                else
                {
                    needed = 1024;
                    return true;
                }
            }

            if (mContentLength >= 0)
            {
                const char8_t emptyLine[] = "\r\n\r\n";
                const char8_t* str = blaze_strnstr(data, emptyLine, available);

                // If we haven't finished the headers yet, don't know how much more data we need
                // so ask for a chunk
                if (str == nullptr)
                {
                    BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Still need more headers for " << methodName << " request.");
                    needed = 1024;
                    return true;
                }

                size_t desired = mContentLength + (str + strlen(emptyLine) - data);

                // If we have finished at least the headers, we know how much data we want
                if (available >= desired)
                {
                    BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Received complete " << methodName << " request.");
                    needed = 0;
                    return true;
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].receive: Still need more body for " << methodName << " request.");
                    needed = (desired - available);
                    return true;
                }
            }
            needed = 0;
            return true;
        }

        BLAZE_WARN_LOG(Log::REST, "[RestProtocol].receive: Unexpected end of function.");
        needed = 0;
        mMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
        return false;
    }

    /*********************************************************************************************/
    /*!
    \brief process

    Parse the URL of an http request to determine what component and command the connection should
    pass the request along to.

    \param[in]  buffer - RawBuffer to read from.
    \param[out] component - the component id determined by examining the URL
    \param[out] command - the command id determined by examining the URL
    \param[out] msgNum - the sequence number assigned to this request
    \param[out] type - the type of message
    */
    /*********************************************************************************************/
    void RestProtocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
    {
        BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].process: Examining URL.");

        frame.errorCode = ERR_OK;
        frame.context = 0;
        frame.componentId = RPC_COMPONENT_UNKNOWN;
        frame.commandId = RPC_COMMAND_UNKNOWN;
        frame.requestEncodingType = decoder.getType();

        // There should only be one session on a given HTTP connection, so simply set the index to 0
        frame.userIndex = 0;

        // Assign a sequence number to the protocol since the there isn't actually a seqno
        // encoded in the incoming request
        frame.msgNum = getNextSeqno();

        // Type will always be a message for now
        frame.messageType = RpcProtocol::MESSAGE;

        //Parse the url from the full request
        char8_t url[1024];
        HttpProtocolUtil::HttpReturnCode code = HttpProtocolUtil::parseUrl(buffer, url, sizeof(url));
        if (code != HttpProtocolUtil::HTTP_OK)
        {
            BLAZE_ERR_LOG(Log::REST, "[RestProtocol].process: Problem parsing URL '" << url << "' (" << code << ")");
            mMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
            return;
        }

        // Parse the buffer to get the full URI, including the params (url is just the part before the ?)
        {
            const char8_t* data = reinterpret_cast<const char8_t*>(buffer.data());
            const char8_t* tail = reinterpret_cast<const char8_t*>(buffer.tail());
            char8_t* urlEnd = nullptr;
            char8_t* urlStart = blaze_strnstr(data, "/", tail - data);
            if (urlStart != nullptr)
            {
                urlEnd = blaze_strnstr(urlStart, " ", tail - urlStart);
                if (urlEnd != nullptr)
                    frame.requestUrl.append(urlStart, urlEnd);
            }
        }

        //Parse header names and values from the full request
        HttpHeaderMap headerMap(BlazeStlAllocator("RestProtocol::headerMap"));
        code = HttpProtocolUtil::buildHeaderMap(buffer, headerMap);
        if (code != HttpProtocolUtil::HTTP_OK)
        {
            BLAZE_ERR_LOG(Log::REST, "[RestProtocol].process: Problem parsing headers (" << code << ") from HTTP request: \n" << (const char8_t*)(buffer.data()));
            mMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
            return;
        }

        // If the BLAZE-SESSION header exists, use it to get the session key
        const char8_t* sessionKey = HttpProtocolUtil::getHeaderValue(headerMap, "BLAZE-SESSION");
        if (sessionKey != nullptr)
        {
            frame.setSessionKey(sessionKey);
        }

        //Get the encoding type from Accept-header
        const char8_t* value = HttpProtocolUtil::getHeaderValue(headerMap, "Accept");
        frame.responseEncodingType = HttpXmlProtocol::getEncoderTypeFromMIMEType(value, Encoder::JSON);

        //Get response format blaze x-header
        //Currently only supports one value: tag.  In the future more formatters could be added,
        //Eg. "gzip".  Should probably tokenize by space so multiple options can be combined.
        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-FORMAT");
        if (value != nullptr && blaze_strncmp(value,"tag",3) == 0)
        {
            frame.format = RpcProtocol::FORMAT_USE_TDF_TAGS;
        }
        else
        {
            frame.format = RpcProtocol::FORMAT_USE_TDF_NAMES;
        }

        //  extract the sequence number if supplied.
        value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-SEQNO");
        if (value != nullptr)
        {
            uint32_t seqno = 0;
            blaze_str2int(value, &seqno);
            if (seqno != 0)
            {
                frame.msgNum = seqno;
            }
        }

        //check if we should use the legacy/deprecated JSON formatting that includes tdf class names in the payload
        value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-LEGACY-JSON-FORMAT");

        if (value != nullptr)
        {
            if (blaze_strncmp(value, "true", 4) == 0) 
            {
                frame.useLegacyJsonEncoding = true;
            }
        }

        HttpProtocolUtil::getClientIPFromXForwardForHeader(HttpProtocolUtil::getHeaderValue(headerMap, "X-Forwarded-For"), frame.clientAddr);

        // Check if we should compress the XML response for this request
        mIsCompressionEnabled = false;
        if (sUseCompression == true)
        {
            // Verify client accepts gzip encoded responses
            mIsCompressionEnabled = HttpProtocolUtil::hasAcceptEncodingType(headerMap,
                HttpProtocolUtil::HTTP_ACCEPTENCODING_GZIP);
            if (mIsCompressionEnabled == false)
            {
                BLAZE_TRACE_LOG(Log::REST, "[RestProtocol].process: Compression enabled, but client doesn't support gzip compression, response will not be compressed.");
            }            
        }

        //If the request type is POST, look for a method type override
        if (mMethod == HttpProtocolUtil::HTTP_POST)
        {
            const char8_t* methodOverride = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-METHOD");
            if (methodOverride != nullptr)
            {
                size_t len = strlen(methodOverride);
                HttpProtocolUtil::HttpMethod method;
                if (HttpProtocolUtil::parseMethod(methodOverride, len, method) == HttpProtocolUtil::HTTP_OK)
                {
                    //Need to directly modify request buffer
                    mMethod = method;
                }
                else
                    BLAZE_WARN_LOG(Log::REST, "[RestProtocol].process: Unrecognized method specified for X-BLAZE-METHOD: " << methodOverride);
            }
        }

        // Get the REST resource info
        const RestResourceInfo* restResource = RestProtocolUtil::getRestResourceInfo(HttpProtocolUtil::strMethods[mMethod], url);
        if (restResource != nullptr)
        {
            frame.componentId = restResource->componentId;
            frame.commandId = restResource->commandId;
        }



        // Reset HTTP method for next frame
        mMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
    }

    /*********************************************************************************************/
    /*!
    \brief framePayload

    Setup the frame header in the given buffer using the provided request object.  The
    buffer's data point on input will be set to point to the frame's payload and the payload
    will be present.

    On exit, the given buffer's data pointer must be pointing to the head of the frame.

    \param[out] buffer - the buffer containing the response data into which header is written
    \param[in] frame - the struct containing the info of the request object
    \param[in] dataSize - size of the frame
    \param[in] tdf - tdf to be encoded and the encoded data will be included in the payload
    \param[in] encoder - tdf encoder to encoder the given tdf
    */
    /*********************************************************************************************/
    void RestProtocol::framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf /*= nullptr*/)
    {
        // get resource info by component id and command id
        const RestResourceInfo* restResourceInfo = BlazeRpcComponentDb::getRestResourceInfo(frame.componentId, frame.commandId);
        bool isHeadRequest = false;

        if (tdf != nullptr)
        {
            bool isEncoded = false;

            // Only perform the following if the reply is a response tdf
            if (restResourceInfo != nullptr && frame.messageType == RpcProtocol::REPLY)
            {
                isHeadRequest = blaze_strcmp(restResourceInfo->methodName, HttpProtocolUtil::strMethods[HttpProtocolUtil::HTTP_HEAD]) == 0;
                if (isHeadRequest)
                {
                    // do not return a message body if the request is a HEAD method
                    isEncoded = true;
                }
                else if (restResourceInfo->responsePayloadBlobFunc != nullptr )
                {
                    // inject the binary data to the response body
                    EA::TDF::TdfBlob& responsePayloadBlob = (*restResourceInfo->responsePayloadBlobFunc)(const_cast<EA::TDF::Tdf&>(*tdf));
                    uint8_t* outputBuffer = buffer.acquire(responsePayloadBlob.getSize() + 1);
                    if (outputBuffer != nullptr)
                    {
                        memcpy(outputBuffer, responsePayloadBlob.getData(), responsePayloadBlob.getSize());
                        outputBuffer[responsePayloadBlob.getSize()] = '\0';
                        buffer.put(responsePayloadBlob.getSize());
                    }
                    isEncoded = true;
                }
                else if (restResourceInfo->responsePayloadMemberTags != nullptr)
                {
                    if (encoder.getType() == Encoder::JSON)
                    {
                        JsonEncoder& jsonEncoder = static_cast<JsonEncoder&>(encoder);
                        if (restResourceInfo->responsePayloadMemberTags != nullptr)
                            jsonEncoder.setSubField(*restResourceInfo->responsePayloadMemberTags);
                    }
                }
            }

            if (isEncoded == false)
            {
                // encode the entire tdf if the response tdf blob is not specified
                encoder.encode(buffer, *tdf, &frame);
            }
            if ((restResourceInfo->responsePayloadMemberTags != nullptr) && (encoder.getType() == Encoder::JSON))
            {   
                JsonEncoder& jsonEncoder = static_cast<JsonEncoder&>(encoder);
                jsonEncoder.clearSubField();
            }
            dataSize = buffer.datasize();
        }

        char responseHeader[HEADER_SIZE];
        eastl::string contentType = RestProtocolUtil::getContentTypeFromEncoderType(frame.responseEncodingType);
        Blaze::HttpStatusCode statusCode = ErrorHelp::getHttpStatusCode(frame.errorCode);

        if (restResourceInfo != nullptr && restResourceInfo->successStatusCodes != nullptr &&
            frame.errorCode == Blaze::ERR_OK && tdf != nullptr)
        {
            statusCode = RestProtocolUtil::parseSuccessStatusCode(restResourceInfo->successStatusCodes, restResourceInfo->successStatusCodesArrayCount, *tdf);
        }

        switch (frame.messageType)
        {
        case RpcProtocol::MESSAGE:
        case RpcProtocol::REPLY:
            {
                size_t contentLength = 0;
                char8_t customResponseHeader[HEADER_SIZE];
                customResponseHeader[0] = '\0';

                if (restResourceInfo != nullptr)
                {
                    HttpHeaderMap customResponseHeadersMap(BlazeStlAllocator("RestProtocol::customResponseHeadersMap"));
                    if (tdf != nullptr)
                    {
                        RestProtocolUtil::buildCustomHeaderMap(restResourceInfo->customResponseHeaders, restResourceInfo->restResponseHeaderArrayCount, *tdf, customResponseHeadersMap, restResourceInfo->headerEncoderSubType);
                    }

                    const char8_t* customContentType = HttpProtocolUtil::getHeaderValue(customResponseHeadersMap, "Content-Type");
                    if (customContentType != nullptr)
                    {
                        // override with custom content type
                        contentType = customContentType;
                        // remove Content-Type from custom headers
                        customResponseHeadersMap.erase("Content-Type");
                    }
                    const char8_t* customContentLength = HttpProtocolUtil::getHeaderValue(customResponseHeadersMap, "Content-Length");
                    if (customContentLength != nullptr)
                    {
                        // override with custom content length
                        blaze_str2int(customContentLength, &contentLength);
                        // remove Content-Length from custom headers
                        customResponseHeadersMap.erase("Content-Length");
                    }
                    RestProtocolUtil::createHeaderString(customResponseHeadersMap, customResponseHeader, sizeof(customResponseHeader));
                }

                if (frame.voidRpcResponse == true && buffer.datasize() == 0)
                {
                    char voidXml[HEADER_SIZE];
                    
                    if (isHeadRequest)
                    {
                        voidXml[0] = '\0';
                    }
                    else if (frame.responseEncodingType == Encoder::JSON)
                    {
                        blaze_snzprintf(voidXml, sizeof(voidXml),                            
                            "{\n    \"component\": %u,\n    \"errorcode\": %u,\n    \"errorname\": \"%s\"\n    }\n", 
                            frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }
                    else 
                    {
                        blaze_snzprintf(voidXml, sizeof(voidXml), 
                            "%s<error>\n<component>%u</component>\n<errorCode>%u</errorCode>\n<errorName>%s</errorName>\n</error>", 
                            HttpProtocolUtil::HTTP_XML_PAYLOAD, frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }

                    blaze_snzprintf(responseHeader, sizeof(responseHeader),
                        "HTTP/1.1 %u %s\r\n"
                        "%s: %u\r\n"
                        "X-BLAZE-COMPONENT: %s\r\n"
                        "X-BLAZE-COMMAND: %s\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %" PRIsize "\r\n%s"
                        "\r\n%s",
                        statusCode, OutboundHttpResult::lookupHttpStatusReason(statusCode), RestProtocolUtil::HEADER_BLAZE_ERRORCODE, frame.errorCode,
                        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId),
                        contentType.c_str(), (contentLength != 0) ? contentLength : strlen(voidXml),
                        customResponseHeader, voidXml);
                }
                else 
                {
                    blaze_snzprintf(responseHeader, sizeof(responseHeader),
                        "HTTP/1.1 %u %s\r\n"
                        "%s: %u\r\n"
                        "X-BLAZE-COMPONENT: %s\r\n"
                        "X-BLAZE-COMMAND: %s\r\n"
                        "Content-Type: %s\r\n%s",
                        statusCode, OutboundHttpResult::lookupHttpStatusReason(statusCode), RestProtocolUtil::HEADER_BLAZE_ERRORCODE, frame.errorCode,
                        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId),
                        contentType.c_str(), customResponseHeader);

                    compressRawBuffer(buffer, responseHeader, sizeof(responseHeader));   

                    size_t headerLen = strlen(responseHeader);

                    //  dataSize will be -1 for events, otherwise use buffer.datasize() as the content-length.
                    //  in a later version this logic will be cleaned up.
                    if (dataSize >= 0)
                    {
                        // Do not send content length in case when buffer contains no data
                        // because that indicates that content will be appended to the stream 
                        // continuously. This is required for xml events to work properly.
                        blaze_snzprintf(responseHeader + headerLen, sizeof(responseHeader) - headerLen,
                            "Content-Length: %" PRIsize "\r\n", (contentLength == 0) ? buffer.datasize() : contentLength);
                    }

                    if (frame.messageType == RpcProtocol::REPLY)
                    {
                        size_t len = strlen(responseHeader);
                        blaze_snzprintf(responseHeader+len, sizeof(responseHeader)-len,
                            "X-BLAZE-SEQNO: %u\r\n", frame.msgNum);
                    }

                    blaze_strnzcat(responseHeader, "\r\n", sizeof(responseHeader));
                }
            }     
            break;

        case RpcProtocol::NOTIFICATION:
            responseHeader[0] = '\0';
            break;

        case RpcProtocol::ERROR_REPLY:
            {
                char8_t customErrorHeader[HEADER_SIZE];
                customErrorHeader[0] = '\0';

                if (restResourceInfo != nullptr)
                {
                    HttpHeaderMap customErrorHeadersMap(BlazeStlAllocator("RestProtocol::customErrorHeadersMap"));

                    if (tdf != nullptr)
                    {
                        RestProtocolUtil::buildCustomHeaderMap(restResourceInfo->customErrorHeaders, restResourceInfo->restErrorHeaderArrayCount, *tdf, customErrorHeadersMap, restResourceInfo->headerEncoderSubType);
                    }

                    RestProtocolUtil::createHeaderString(customErrorHeadersMap, customErrorHeader, sizeof(customErrorHeader));
                }

                char errorBuffer[HEADER_SIZE];

                blaze_snzprintf(responseHeader, sizeof(responseHeader),
                    "HTTP/1.1 %u %s\r\n"
                    "%s: %u\r\n"
                    "X-BLAZE-COMPONENT: %s\r\n"
                    "X-BLAZE-COMMAND: %s\r\n"
                    "Content-Type: %s\r\n%s",
                    statusCode, OutboundHttpResult::lookupHttpStatusReason(statusCode), RestProtocolUtil::HEADER_BLAZE_ERRORCODE, frame.errorCode,
                    BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                    BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId),
                    contentType.c_str(), (customErrorHeader[0] != '\0') ? customErrorHeader : "");

                if (buffer.datasize() == 0)
                {
                    if (isHeadRequest)
                    {
                        errorBuffer[0] = '\0';
                    }
                    else if (frame.responseEncodingType == Encoder::JSON)
                    {
                        blaze_snzprintf(errorBuffer, sizeof(errorBuffer),                            
                            "{\n    \"component\": %u,\n    \"errorcode\": %u,\n    \"errorname\": \"%s\"\n    }\n", 
                            frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }
                    else
                    {
                        blaze_snzprintf(errorBuffer, sizeof(errorBuffer), 
                            "%s<error>\n<component>%u</component>\n<errorCode>%u</errorCode>\n<errorName>%s</errorName>\n</error>", 
                            HttpProtocolUtil::HTTP_XML_PAYLOAD, frame.componentId, frame.errorCode,
                            ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }

                    // Try and save error XML to buffer and compress it
                    if (HttpProtocolUtil::replaceRawBufferData(buffer, reinterpret_cast<uint8_t*>(&errorBuffer[0]), strlen(errorBuffer)) == true)
                    {
                        compressRawBuffer(buffer, responseHeader, sizeof(responseHeader));  

                        size_t headerLen = strlen(responseHeader);
                        blaze_snzprintf(responseHeader + headerLen, sizeof(responseHeader) - headerLen,
                            "Content-Length: %" PRIsize "\r\n\r\n", buffer.datasize());
                    }  
                    // Could not save error xml to buffer, append to header
                    else
                    {
                        blaze_snzprintf(responseHeader, sizeof(responseHeader),
                            "Content-Length: %" PRIsize "\r\n\r\n%s", strlen(errorBuffer), errorBuffer);                    
                    }
                }
                else
                {
                    // encoded error tdf is already in the payload
                    compressRawBuffer(buffer, responseHeader, sizeof(responseHeader));   

                    size_t headerLen = strlen(responseHeader);
                    blaze_snzprintf(responseHeader + headerLen, sizeof(responseHeader) - headerLen,
                    "Content-Length: %" PRIsize "\r\n\r\n", buffer.datasize());
                }     
            }
            break;

        default:
            EA_FAIL();
            return;
        }

        size_t len = strlen(responseHeader);
        memcpy(buffer.push(len), responseHeader, len);
    }

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

    /*********************************************************************************************/
    /*!
        \brief compressRawBuffer

        Compress data in raw buffer    

        NOTE: This will not compress the entire raw buffer, but the data from data() to tail() (datasize())

        \param[out] success - Was the data successfully compressed
        \param[in] rawBuffer - Buffer to compress
        \param[in] header - Header to update with content encoding information
        \param[in] headerSize - Size of header buffer
    */
    /*********************************************************************************************/
    bool RestProtocol::compressRawBuffer(RawBuffer& rawBuffer, char8_t* header, size_t headerSize)
    {   
        uint8_t* uncompressedData = rawBuffer.data();
        size_t uncompressedDataSize = rawBuffer.datasize();

        if (uncompressedData == nullptr || uncompressedDataSize <= 0 || mIsCompressionEnabled == false)
        {
            // Nothing to compress or compression is disabled
            return false;
        }

        bool ret = RestProtocolUtil::compressRawBuffer(rawBuffer, header, headerSize, sCompressionLevel);
        if (ret)
        {
            // RestProtocolUtil::compressRawBuffer won't append the \r\n on the Content-Encoding header, so do it ourself
            blaze_strnzcat(header, "\r\n", headerSize);
        }

        return ret;
    }

    void RestProtocol::setMaxFrameSize(uint32_t maxFrameSize)
    {
        mMaxFrameSize = maxFrameSize;
    }
    
} // Blaze

