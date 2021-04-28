/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class HttpXmlProtocol

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

        */
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/connection.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/util/locales.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/component/blazerpc.h"
#include "EAStdC/EACType.h"

// Zlib Compression
#include "framework/util/compression/zlibdeflate.h"

namespace Blaze
{

    bool HttpXmlProtocol::mDefaultEnumEncodingIsValue = false;
    bool HttpXmlProtocol::mDefaultXmlOutputIsElements = false;
    
    // XML Compression
    bool HttpXmlProtocol::sUseCompression = false;
    int32_t HttpXmlProtocol::sCompressionLevel = ZlibDeflate::DEFAULT_COMPRESSION_LEVEL;
    const char8_t* HttpXmlProtocol::CONTENT_ENCODING_STRING = "Content-Encoding: gzip\r\n";

    /*** Defines/Macros/Constants/Typedefs ***********************************************************/

    /*** Public Methods ******************************************************************************/

    /*************************************************************************************************/
    /*!
    \brief HttpXmlProtocol

    Constructor.
    */
    /*************************************************************************************************/
    HttpXmlProtocol::HttpXmlProtocol()
        : mNextSeqno(0),
          mMethod(HTTP_UNKNOWN),
          mContentLength(-1),
          mIsCompressionEnabled(false),
          mMaxFrameSize(UINT16_MAX+1)
    {
    }

    /*************************************************************************************************/
    /*!
    \brief ~HttpXmlProtocol

    Destructor.
    */
    /*************************************************************************************************/
    HttpXmlProtocol::~HttpXmlProtocol()
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
    bool HttpXmlProtocol::receive(RawBuffer& buffer, size_t& needed)
    {
        BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Receiving request");

        size_t available = buffer.datasize();
        char8_t* data = reinterpret_cast<char8_t*>(buffer.data());

        // First step is to determine the type of HTTP request (i.e. GET, POST, etc.)
        if (mMethod == HTTP_UNKNOWN)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Reading method.");

            mContentLength = -1;

            // The longest currently defined HTTP request header is 7 characters long, so once we
            // have 8 characters we should definitely have a space-terminated request method
            if (available < 8)
            {
                needed = 1024;
                return true;
            }

            const char GET[] = "GET ";
            const char POST[] = "POST ";

            if (blaze_strncmp(data, GET, strlen(GET)) == 0)
                mMethod = HTTP_GET;
            else if (blaze_strncmp(data, POST, strlen(POST)) == 0)
                mMethod = HTTP_POST;
            else
            {
                // It's not a request type that we handle, so don't bother asking for more data
                BLAZE_WARN_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Unknown method.");
                needed = 0;
                return false;
            }
        }

        if (mMethod == HTTP_GET)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Looking for complete Get request.");

            // Look for two CRLF's which terminate an HTTP Get request
            if (blaze_strnstr(data, "\r\n\r\n", available) != nullptr)
            {
                BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Received complete Get request.");
                needed = 0;
                return true;
            }
            else
            {
                BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Need more data for Get request.");
                needed = 1024;
                return true;
            }
        }
        else if (mMethod == HTTP_POST)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Looking for complete Post request.");

            if (mContentLength == -1)
            {
                const char8_t header[] = "\r\nContent-Length: ";
                size_t headerLen = strlen(header);
                const char8_t* str = blaze_strnistr(data, header, available);
                if (str == nullptr)
                {
                    BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Still need content length header for Post request.");
                    needed = 1024;
                    return true;
                }
                if (blaze_strnstr(str + headerLen, "\r\n", available - (str + headerLen - data)) != nullptr)
                {
                    blaze_str2int(str + headerLen, &mContentLength);
                    if (mContentLength >= 0 && (uint32_t)mContentLength > mMaxFrameSize)
                    {
                        BLAZE_WARN_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Incoming frame is too large (" << mContentLength << "); "
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
                    BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Still need more headers for Post request.");
                    needed = 1024;
                    return true;
                }

                size_t desired = mContentLength + (str + strlen(emptyLine) - data);

                // If we have finished at least the headers, we know how much data we want
                if (available >= desired)
                {
                    BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Received complete Post request.");
                    needed = 0;
                    return true;
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Still need more body for Post request.");
                    needed = (desired - available);
                    return true;
                }
            }
            needed = 0;
            return true;
        }

        BLAZE_WARN_LOG(Log::HTTPXML, "[HttpXmlProtocol].receive: Unexpected end of function.");
        needed = 0;
        mMethod = HTTP_UNKNOWN;
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
    void HttpXmlProtocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
    {
        BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].process: Examining URL.");

        frame.errorCode = ERR_OK;
        frame.context = 0;
        frame.componentId = RPC_COMPONENT_UNKNOWN;
        frame.commandId = RPC_COMMAND_UNKNOWN;

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
        if (code != HttpProtocolUtil::HTTP_OK) {
            BLAZE_WARN_LOG(Log::HTTPXML, "Problem parsing URL '" << url << "'");
            mMethod = HTTP_UNKNOWN;
            return;
        }

        //Parse header names and values from the full request
        HttpHeaderMap headerMap(BlazeStlAllocator("HttpXmlProtocol::headerMap"));
        code = HttpProtocolUtil::buildHeaderMap(buffer, headerMap);
        if (code != HttpProtocolUtil::HTTP_OK) {
            BLAZE_ERR_LOG(Log::HTTPXML, "Problem parsing headers from HTTP request (" << code << ")!");
            mMethod = HTTP_UNKNOWN;
            return;
        }

        // If the BLAZE-SESSION header exists, use it to get the session key
        const char8_t* sessionKey = HttpProtocolUtil::getHeaderValue(headerMap, "BLAZE-SESSION");
        if(sessionKey != nullptr) {
            frame.setSessionKey(sessionKey);
        }

        // If the X-BLAZE-ID header exists, set the service name
        const char8_t* serviceName = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-ID");
        if (serviceName != nullptr)
        {
            blaze_strnzcpy(frame.serviceName, serviceName, MAX_SERVICENAME_LENGTH);
        }

        //Get the locale blaze x-header
        const char8_t * value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-LOCALE");
        if(value != nullptr && strlen(value) == 4) {
            frame.locale = LocaleTokenCreateFromString(value);
        }

        //Get response format blaze x-header
        //Currently only supports one value: tag.  In the future more formatters could be added,
        //Eg. "gzip".  Should probably tokenize by space so multiple options can be combined.
        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-FORMAT");
        if(value != nullptr && blaze_strncmp(value,"tag",3) == 0) {
            frame.format = RpcProtocol::FORMAT_USE_TDF_TAGS;
        }
        else if (value != nullptr && blaze_strncmp(value,"raw",3) == 0) {
            frame.format = RpcProtocol::FORMAT_USE_TDF_RAW_NAMES;
        }
        else {
            frame.format = RpcProtocol::FORMAT_USE_TDF_NAMES;
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

        // Parse http method is whether POST
        HttpProtocolUtil::HttpMethod httpMethod = HttpProtocolUtil::HTTP_INVALID_METHOD;
        const char8_t* data = reinterpret_cast<const char8_t*>(buffer.data());
        {
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

        if(HttpProtocolUtil::parseMethod(data, buffer.datasize(), httpMethod) == HttpProtocolUtil::HTTP_OK)
        {
            if (httpMethod == HttpProtocolUtil::HTTP_POST || httpMethod == HttpProtocolUtil::HTTP_PUT)
            {
                value = HttpProtocolUtil::getHeaderValue(headerMap, "Content-Type");

                if (value != nullptr)
                {
                    if ((blaze_strnicmp(value, XML_CONTENTTYPE, sizeof(XML_CONTENTTYPE)-1) == 0) && (value[sizeof(XML_CONTENTTYPE)-1] == '\0' || value[sizeof(XML_CONTENTTYPE)-1] == ';'))
                    {
                        frame.requestEncodingType = Decoder::XML2;
                    }
                    else if ((blaze_strnicmp(value, HEAT_CONTENTTYPE, sizeof(HEAT_CONTENTTYPE)-1) == 0) && (value[sizeof(HEAT_CONTENTTYPE)-1] == '\0' || value[sizeof(HEAT_CONTENTTYPE)-1] == ';'))
                    {
                        frame.requestEncodingType = Decoder::HEAT2;
                    }
                    else if ((blaze_strnicmp(value, JSON_CONTENTTYPE, sizeof(JSON_CONTENTTYPE)-1) == 0) && (value[sizeof(JSON_CONTENTTYPE)-1] == '\0' || value[sizeof(JSON_CONTENTTYPE)-1] == ';'))
                    {
                        frame.requestEncodingType = Decoder::JSON;
                    }
                }

                size_t datasize = buffer.datasize();

                const char8_t* body = blaze_strnstr(data, "\r\n\r\n", datasize);
                if (body == nullptr)
                {
                    BLAZE_WARN_LOG(Log::HTTPXML, "There is no any content in the POST Request");
                }
                else
                {
                    body += 4;                          // Skip to start of body
                    datasize = buffer.datasize() - static_cast<size_t>(body - data);

                    if (static_cast<size_t>(mContentLength) != datasize)
                    {
                        // Content-Length and actual length disagree - The data may have been compressed
                        mContentLength = datasize;
                    }
                    
                    if (frame.requestEncodingType != Decoder::HTTP)
                    {
                        // Most decoders expect that the incoming buffer will point to the start of the request body,
                        // with the exception of the HTTP decoder, which expects that the fully framed request is provided.
                        buffer.pull(static_cast<size_t>(body - data));
                        *(buffer.data() + mContentLength) = '\0';
                    }
                }

            }
        }

        //Get the encoding type from Accept-header
        value = HttpProtocolUtil::getHeaderValue(headerMap, "Accept");
        frame.responseEncodingType = getEncoderTypeFromMIMEType(value, Encoder::XML2);

        //Get enumeration encoding from blaze x-header
        //Supported values are: value - for integer value of enumeration constant
        //                      identifier - for string identifier of enumeration constant (default)
        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-ENUM-FORMAT");
        if(value != nullptr)
        { 
            if (blaze_strncmp(value, "value", 5) == 0) 
            {
                frame.enumFormat = RpcProtocol::ENUM_FORMAT_VALUE;
            } 
            else 
            {
                frame.enumFormat = RpcProtocol::ENUM_FORMAT_IDENTIFIER;
            }
        }
        else
        {
            // check for default value
            if (mDefaultEnumEncodingIsValue)
                frame.enumFormat = RpcProtocol::ENUM_FORMAT_VALUE;
            else
                frame.enumFormat = RpcProtocol::ENUM_FORMAT_IDENTIFIER;
        }

        /***********************************************************************/

        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-VOID-RESP");
        
        if ( value != nullptr && (blaze_strncmp(value, "xml", 3) == 0) )
        {
            frame.voidRpcResponse = true;
        } 
        else 
        {
            frame.voidRpcResponse = false;
        }
        
        /***********************************************************************/

        // Get XML output format from blaze x-header
        // Supported values are:   elements - Use elements
        //                       attributes - Use attributes
        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-XML-FORMAT");
        if (value != nullptr)
        { 
            if (blaze_strncmp(value, "attributes", 10) == 0) 
            {
                frame.xmlResponseFormat = RpcProtocol::XML_RESPONSE_FORMAT_ATTRIBUTES;
            } 
            else 
            {
                frame.xmlResponseFormat = RpcProtocol::XML_RESPONSE_FORMAT_ELEMENTS;
            }
        }
        else
        {
            // check for default value
            frame.xmlResponseFormat = (mDefaultXmlOutputIsElements) ? RpcProtocol::XML_RESPONSE_FORMAT_ELEMENTS : RpcProtocol::XML_RESPONSE_FORMAT_ATTRIBUTES;
        }

        //  extract the sequence number if supplied.
        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-BLAZE-SEQNO");
        if (value != nullptr)
        {
            uint32_t seqno = 0;
            blaze_str2int(value, &seqno);
            if (seqno != 0)
            {
                frame.msgNum = seqno;
            }
        }

        value = HttpProtocolUtil::getHeaderValue(headerMap,"X-USE-COMMON-LIST-ENTRY-NAME");
        if (value != nullptr)
        {
            if (blaze_strncmp(value, "true", 4) == 0) 
            {
                frame.useCommonListEntryName = true;
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
                BLAZE_TRACE_LOG(Log::HTTPXML, "XML compression enabled, but client doesn't support gzip compression, response will not be compressed.");
            }            
        }        
        
        //Get Connection HTTP 1.1 standard header
        value = HttpProtocolUtil::getHeaderValue(headerMap,"Connection");
        if(value != nullptr && blaze_strncmp(value,"close",5) == 0) {
            frame.transform = RpcProtocol::CONNECTION_CLOSE;
        }

        // Reset HTTP method for next frame
        mMethod = HTTP_UNKNOWN;

        if (blaze_strcmp(url, "/favicon.ico") == 0)
        {
            frame.isFaviconRequest = true;
            return;
        }

        char8_t* ch = url;
        if (ch[0] == '/')
            ++ch;
        else
            BLAZE_WARN_LOG(Log::HTTPXML, "URL doesn't start with a slash: " << url);

        char8_t* slash = strchr(ch, '/');
        if (slash != nullptr)
        {
            // Null-terminate the component and parse it
            *slash = '\0';
            frame.componentId = BlazeRpcComponentDb::getComponentIdByName(ch);
            //blaze_str2int(ch, component);

            ch = slash + 1;
            slash = strchr(ch, '/');
            if (slash != nullptr)
            {            
                *slash = '\0';
            }
            frame.commandId = BlazeRpcComponentDb::getCommandIdByName(frame.componentId, ch);

            //Lastly try for a session key
            if (slash != nullptr && sessionKey == nullptr)
            {
                ch = slash + 1;
                slash = strchr(ch, '/');
                if (slash != nullptr)
                {
                    *slash = '\0';
                }
                frame.setSessionKey(ch);
            }
        }
    }

    /*********************************************************************************************/
    /*!
    \brief framePayload

    Setup the frame header in the given buffer using the provided request object.  The
    buffer's data point on input will be set to point to the frame's payload and the payload
    will be present.

    On exit, the given buffer's data pointer must be pointing to the head of the frame.

    \param[out] buffer - the buffer containing the response data into which header is written
    \param[in] component - the component id
    \param[in] command - the command id
    \param[in] errorCode - the error code
    \param[in] msgNum - the message num
    \param[in] type - the message type
    \param[in] payload - the buffer containing the payload
    */
    /*********************************************************************************************/
    void HttpXmlProtocol::framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf /*= nullptr*/)
    {
        BLAZE_TRACE_LOG(Log::HTTPXML, "[HttpXmlProtocol].framePayload: Writing HTTP header.");

        if (tdf != nullptr)
        {
            if (encoder.encode(buffer, *tdf, &frame))
            {
                dataSize = buffer.datasize();
            }
            else
            {
                BLAZE_ERR_LOG(Log::HTTPXML, "[HttpXmlProtocol].framePayload: Failed to encode TDF response buffer.");
                buffer.trim(buffer.datasize());
                dataSize = 0;
            }
        }

        Blaze::StringBuilder responseHeader;
        switch (frame.messageType)
        {
        case RpcProtocol::MESSAGE:
        case RpcProtocol::REPLY:
            {
                if (frame.voidRpcResponse == true && buffer.datasize() == 0)
                {
                    char voidXml[HEADER_SIZE];
                    
                    if (frame.responseEncodingType == Encoder::JSON )
                    {
                        blaze_snzprintf(voidXml, sizeof(voidXml),                            
                            "{\n\"error\" : {\n    \"component\": %u,\n    \"errorcode\": %u,\n    \"errorname\": \"%s\"\n    }\n}\n", 
                            frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }
                    else 
                    {
                        blaze_snzprintf(voidXml, sizeof(voidXml), 
                            "%s<error>\n<component>%u</component>\n<errorCode>%u</errorCode>\n<errorName>%s</errorName>\n</error>", 
                            HttpProtocolUtil::HTTP_XML_PAYLOAD, frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                    }

                    responseHeader.append(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %" PRIu64 "\r\n"
                        "X-BLAZE-COMPONENT: %s\r\n"
                        "X-BLAZE-COMMAND: %s\r\n"
                        "\r\n%s",
                        getContentTypeFromEncoderType(frame.responseEncodingType),
                        strlen(voidXml),
                        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId),
                        voidXml);
                }
                else 
                {
                    responseHeader.append(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "X-BLAZE-COMPONENT: %s\r\n"
                        "X-BLAZE-COMMAND: %s\r\n",
                        getContentTypeFromEncoderType(frame.responseEncodingType),
                        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId));

                    compressRawBuffer(buffer, &responseHeader);

                    //  dataSize will be -1 for events, otherwise use buffer.datasize() as the content-length.
                    //  in a later version this logic will be cleaned up.
                    if (dataSize < 0)
                    {
                        // If the event notifications will be compressed, add the gzip content encoding header
                        if (compressNotifications())
                            responseHeader.append(CONTENT_ENCODING_STRING);
                    }
                    else
                    {
                        // Do not send content length in case when buffer contains no data
                        // because that indicates that content will be appended to the stream 
                        // continuously. This is required for xml events to work properly.
                        responseHeader.append("Content-Length: %" PRIu64 "\r\n", buffer.datasize());
                    }

                    if (frame.messageType == RpcProtocol::REPLY)
                    {
                        responseHeader.append("X-BLAZE-SEQNO: %" PRIu32 "\r\n", frame.msgNum);
                    }
                    responseHeader.append("\r\n");

                }
            }     
            break;

        case RpcProtocol::NOTIFICATION:
            responseHeader.reset();
            // Only event notifications may be compressed. The content encoding header is added to the 
            // response of the event subscribe RPC, not to each event notification.
            if (compressNotifications())
                compressRawBuffer(buffer);
            break;

        case RpcProtocol::ERROR_REPLY:
            {
                if (frame.movedToAddr.isValid())
                {
                    const char8_t* hostNameOrIp = frame.movedToAddr.getHostname();
                    if (hostNameOrIp[0] == '\0')
                    {
                        hostNameOrIp = frame.movedToAddr.getIpAsString();
                    }

                    responseHeader.append(
                        "HTTP/1.1 301 Moved Permanently\r\n"
                        "Location: %s://%s:%" PRIu16 "%s\r\n"
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n"
                        "X-BLAZE-MOVEDTO-ID: %u\r\n"
                        "\r\n",
                        (frame.requestIsSecure ? "https" : "http"),
                        hostNameOrIp,
                        frame.movedToAddr.getPort(InetAddress::HOST),
                        frame.requestUrl.c_str(),
                        frame.movedTo);
                }
                else
                {
                    // expandable array here guards vs overflow for the error elements
                    Blaze::StringBuilder errorBuffer(nullptr);

                    responseHeader.append(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "X-BLAZE-COMPONENT: %s\r\n"
                        "X-BLAZE-COMMAND: %s\r\n"
                        "X-BLAZE-SEQNO: %u\r\n",
                        getContentTypeFromEncoderType(frame.responseEncodingType),
                        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
                        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId),
                        frame.msgNum);

                    // If the tdf data is not json- or xml-encoded, just drop it - we don't want it in the error reply
                    if (frame.responseEncodingType != Encoder::JSON && frame.responseEncodingType != Encoder::XML2 && frame.responseEncodingType != Encoder::EVENTXML)
                        buffer.trim(buffer.datasize());

                    if (buffer.datasize() == 0)
                    {
                        if (frame.responseEncodingType == Encoder::JSON )
                        {
                            errorBuffer.append(
                                "{\n\"error\" : {\n    \"component\": %u,\n    \"errorcode\": %u,\n    \"errorname\": \"%s\"\n    }\n}\n", 
                                frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                        }
                        else 
                        {
                            errorBuffer.append(
                                "%s<error>\n<component>%u</component>\n<errorCode>%u</errorCode>\n<errorName>%s</errorName>\n</error>", 
                                HttpProtocolUtil::HTTP_XML_PAYLOAD, frame.componentId, frame.errorCode,
                                ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode));
                        }

                        // Try and save error XML to buffer and compress it
                        if (HttpProtocolUtil::replaceRawBufferData(buffer, reinterpret_cast<uint8_t*>(&const_cast<char*>(errorBuffer.get())[0]),
                            errorBuffer.length()) == true)
                        {
                            compressRawBuffer(buffer, &responseHeader);

                            responseHeader.append("Content-Length: %" PRIu64 "\r\n\r\n", buffer.datasize());
                        }  
                        // Could not save error xml to buffer, append to header
                        else
                        {
                            responseHeader.append("Content-Length: %" PRIu64 "\r\n\r\n%s", errorBuffer.length(), errorBuffer.get());                    
                        }
                    }
                    else
                    {
                        size_t dataLen = buffer.datasize();
                        size_t len=0;
                    
                        if (frame.responseEncodingType == Encoder::JSON )
                        {
                            errorBuffer.append(                            
                                "{\n\"error\" : {\n    \"component\": %u,\n    \"errorcode\": %u,\n    \"errorname\": \"%s\",\n\"errortdf\": %s", 
                                frame.componentId, frame.errorCode, ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode), (char8_t*)buffer.data());
                        }
                        else 
                        {
                            // Insert the error XML before the <serverinstanceerror> tag
                            len = strlen(HttpProtocolUtil::HTTP_XML_PAYLOAD);
                            buffer.pull(len); // Strip out the XML declaration

                            errorBuffer.append(
                                "%s<error>\n<component>%u</component>\n<errorCode>%u</errorCode>\n<errorName>%s</errorName>\n%s", 
                                HttpProtocolUtil::HTTP_XML_PAYLOAD, frame.componentId, frame.errorCode,
                                ErrorHelp::getErrorName((BlazeRpcError) frame.errorCode), (char8_t*)buffer.data());
                        }

                        size_t errorLen = errorBuffer.length();
                        buffer.acquire(errorLen-dataLen);
                        memcpy(buffer.push(len), errorBuffer.get(), errorLen); // Set the buffer data
                        buffer.put(errorLen-dataLen); // Move the tail of the buffer, accounting for the existing data now included in the errorBuffer

                        errorBuffer.reset();
                        if (frame.responseEncodingType == Encoder::JSON )
                        {
                            errorBuffer.append("    }\n}\n");
                        }
                        else
                        {
                            errorBuffer.append("</error>");
                        }                    
                        errorLen = errorBuffer.length();
                        buffer.acquire(errorLen);
                        memcpy(buffer.tail(), errorBuffer.get(), errorLen);
                        buffer.put(errorLen);

                        compressRawBuffer(buffer, &responseHeader);

                        responseHeader.append("Content-Length: %" PRIu64 "\r\n\r\n", buffer.datasize());
                    }
                }
            }
        break;
        default:
            EA_FAIL();
            return;
        }                  

        if (buffer.headroom() < responseHeader.length())
        {
            // There isn't enough room to copy the headers in front of the payload.  Unfortunately, we need to re-jig some memory here.
            size_t needed = responseHeader.length() - buffer.headroom();
            buffer.acquire(needed);
            memmove(buffer.data() + needed, buffer.data(), buffer.datasize());
            buffer.put(needed);
            buffer.pull(needed);
        }
        memcpy(buffer.push(responseHeader.length()), responseHeader.get(), responseHeader.length());
    }

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

    /*********************************************************************************************/
    /*!
        \brief compressRawBuffer

        Compress data in raw buffer    

        NOTE: This will not compress the entire raw buffer, but the data from data() to tail() (datasize())

        \return success - Was the data successfully compressed
        \param[in] rawBuffer - Buffer to compress
        \param[in] responseHeader (optional) - Header to update with content encoding information
    */
    /*********************************************************************************************/
    bool HttpXmlProtocol::compressRawBuffer(RawBuffer& rawBuffer, StringBuilder* responseHeader /*= nullptr*/)
    {
        uint8_t* uncompressedData = rawBuffer.data();
        size_t uncompressedDataSize = rawBuffer.datasize();

        if (uncompressedData == nullptr || uncompressedDataSize <= 0 || mIsCompressionEnabled == false)
        {
            // Nothing to compress or compression is disabled
            return false;
        }

        // Setup ZLib for gzip compression
        ZlibDeflate zlibDeflate(ZlibDeflate::COMPRESSION_TYPE_GZIP, sCompressionLevel);

        // Allocate maximum buffer for compressed response
        uint32_t maxCompressedSize = zlibDeflate.getWorstCaseCompressedDataSize(uncompressedDataSize); 
        uint8_t* compressedData = BLAZE_NEW_ARRAY_MGID(uint8_t, maxCompressedSize, MEM_GROUP_FRAMEWORK_CATEGORY, "compressRawBuffer::compressedData");

        // Setup input/output buffers
        zlibDeflate.setInputBuffer(uncompressedData, uncompressedDataSize);
        zlibDeflate.setOutputBuffer(compressedData, maxCompressedSize);

        // Compress all data at once
        int32_t zlibErr = zlibDeflate.execute(Z_FINISH);
        bool success = (zlibErr == Z_STREAM_END);
        
        if (success)
        {
            size_t compressedDataSize = zlibDeflate.getTotalOut();           

            // Replace uncompressed data with compressed data
            success = HttpProtocolUtil::replaceRawBufferData(rawBuffer, compressedData, compressedDataSize);        

            // If we successfully updated the raw buffer data with compressed data
            // add the encoding header to the response
            if (success)
            {
                if (responseHeader != nullptr)
                    responseHeader->append(CONTENT_ENCODING_STRING);
            }
            else
            {
                BLAZE_ERR(Log::HTTPXML, "Problem saving compressed payload to raw buffer!!");
            }
        }
        else
        {
            BLAZE_ERR(Log::HTTPXML, "Problem compression XML payload (Zlib Error: %d)", zlibErr);
        }

        delete[] compressedData;
        compressedData = nullptr;

        return success;
    }
 
    const char8_t* HttpXmlProtocol::getContentTypeFromEncoderType(const Blaze::Encoder::Type encoderType)
    {
        switch (encoderType)
        {
        case Encoder::XML2:
            return XML_CONTENTTYPE;
        case Encoder::HEAT2:
            return HEAT_CONTENTTYPE;
        case Encoder::JSON:
            return JSON_CONTENTTYPE;
        default:
            return "text/xml";
        }

    }

    Blaze::Encoder::Type HttpXmlProtocol::getEncoderTypeFromMIMEType(const char8_t* mimeType, Encoder::Type defaultEncoderType/* = Encoder::INVALID*/)
    {
        if (mimeType != nullptr)
        {
            if (blaze_stricmp(mimeType, XML_CONTENTTYPE) == 0 || blaze_strnicmp(mimeType, XML_CONTENTTYPE ";", 16) == 0)
            {
                return Encoder::XML2;
            }
            else if (blaze_stricmp(mimeType, HEAT_CONTENTTYPE) == 0 || blaze_strnicmp(mimeType, HEAT_CONTENTTYPE ";", 23) == 0)
            {
                return Encoder::HEAT2;
            }
            else if (blaze_stricmp(mimeType, JSON_CONTENTTYPE) == 0 || blaze_strnicmp(mimeType, JSON_CONTENTTYPE ";", 17) == 0)
            {
                return Encoder::JSON;
            }
        }

        return defaultEncoderType;
    }
    
    void HttpXmlProtocol::setMaxFrameSize(uint32_t maxFrameSize)
    {
        mMaxFrameSize = maxFrameSize;
    }
 
} // Blaze

