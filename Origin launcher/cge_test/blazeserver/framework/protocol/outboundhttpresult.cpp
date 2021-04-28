/*************************************************************************************************/
/*!
    \file httpparam.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/protocol/outboundhttpxmlparser.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/util/compression/zlibinflate.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/



namespace Blaze
{

OutboundHttpResult::OutboundHttpResult()
    : mRawBuffer(OutboundHttpConnection::INITIAL_BUFFER_SIZE),
      mDecompressedOutputSize(1024)  // the initial size is 1k byte
{    
    mDecompressedOutput = BLAZE_NEW_ARRAY_MGID(uint8_t, mDecompressedOutputSize, MEM_GROUP_FRAMEWORK_CATEGORY, "OutboundHttpConnection::mDecompressedOutput");
    //initialize mRawBuffer
    mRawBuffer.data()[0] = '\0';
}

OutboundHttpResult::~OutboundHttpResult()
{
    delete[] mDecompressedOutput;
}

uint32_t OutboundHttpResult::processSendData(char8_t* dest, uint32_t size)
{
    return 0;
}

uint32_t OutboundHttpResult::processRecvData(const char8_t* data, uint32_t size, bool isResultInCache)
{
    // For cached responses, we may not have an owner http connection so we bypass that here (this does not make a difference
    // as the cached response must have fit in the limits we defined earlier).
    if (getOwnerConnection() != nullptr) 
    {
        if (mRawBuffer.size() + size > getOwnerConnection()->getOwnerManager().getMaxHttpResponseSize())
        {
            BLAZE_ERR(Log::HTTP, "[OutboundHttpResult].processRecvData: "
                "(server: %s) sent response that exceeds maximum allowed response buffer (%d bytes). Request aborted."
                , getOwnerConnection()->getOwnerManager().getAddress().getHostname()
                , (int32_t)(getOwnerConnection()->getOwnerManager().getMaxHttpResponseSize()));

            return 0;
        }
    }
    
    uint8_t *cursor = mRawBuffer.acquire(size + 1);

    if (cursor == nullptr)
    {
        return 0;
    }
    else
    {
        memcpy(cursor, data, size);
        mRawBuffer.put(size);
        *(mRawBuffer.tail()) = 0;
    }

    return size;
}

BlazeRpcError OutboundHttpResult::requestComplete(CURLcode res)
{
    BlazeRpcError result = ERR_OK;

    if (res == CURLE_OK)
    {
        setUserSecurityState(getHeader("X-AUTH-SECURITY-STATE"));

        bool isDataCompressed = HttpProtocolUtil::hasContentEncodingType(getHeaderMap(), HttpProtocolUtil::HTTP_CONTENTENCODING_GZIP);
        if (isDataCompressed)
        {
            if (!decompressRawBuffer())
            {
                BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpResult].requestComplete: "
                    "(server: " << getOwnerConnection()->getOwnerManager().getAddress().getHostname() << ") "
                    "Failed to decompress the message.");
                result = ERR_SYSTEM;
            }
        }

        if (result == ERR_OK)
        {
            result = decodeResult(mRawBuffer);
        }
    }
    return result;
}

bool OutboundHttpResult::decompressRawBuffer()
{
    uint8_t* compressedData = mRawBuffer.data();
    size_t compressedDataSize = mRawBuffer.datasize();

    if (compressedData == nullptr || compressedDataSize <= 0)
    {
        // Nothing to decompress
        return false;
    }

    // Setup ZLib for gzip decompression
    ZlibInflate zlibInflate;

    // Allocate maximum buffer for decompressed data
    uint32_t maxDecompressedSize = 0;
    uint8_t numTryout = 0;  
    bool allocatedLargeBuffer = false;
    bool success = false;

    uint32_t maxDecompressedSizeAllowed = (getOwnerConnection() != nullptr) ? getOwnerConnection()->getOwnerManager().getMaxHttpResponseSize() : UINT32_MAX;
    while (true)
    {
        numTryout++;
         
        // getDecompressedDataSize() returns 9 times of original size.
        maxDecompressedSize = zlibInflate.getDecompressedDataSize(compressedDataSize) * numTryout;
        if (maxDecompressedSize > maxDecompressedSizeAllowed)
        {
            maxDecompressedSize = maxDecompressedSizeAllowed; // cap the size of buffer
        }

        if (maxDecompressedSize > mDecompressedOutputSize)
        {
            delete[] mDecompressedOutput;
            mDecompressedOutput = BLAZE_NEW_ARRAY_MGID(uint8_t, maxDecompressedSize, MEM_GROUP_FRAMEWORK_CATEGORY, "OutboundHttpConnection::mDecompressedOutput");
            mDecompressedOutputSize = maxDecompressedSize;
        }

        // Setup input/output buffers
        zlibInflate.setInputBuffer(compressedData, compressedDataSize);
        zlibInflate.setOutputBuffer(mDecompressedOutput, mDecompressedOutputSize);

        // Decompress all data at once
        int32_t zlibErr = zlibInflate.execute(Z_FINISH);
        success = (zlibErr == Z_STREAM_END);

        if (success)
        {
            // Replace compressed data with decompressed data
            success = HttpProtocolUtil::replaceRawBufferData(mRawBuffer, mDecompressedOutput, zlibInflate.getTotalOut());
            break;
        }
        else if (zlibErr == Z_BUF_ERROR)
        {
            // Z_BUF_ERROR happens if there was not enough room in the output buffer when Z_FINISH is used. 
            // Note that Z_BUF_ERROR is not fatal, and inflate() can be called again with more input 
            //                                                          and more output space to continue decompressing. 
            // we try until we have reached maximum allowed response size. If Z_BUF_ERROR still, we have to give up for security reason (a malicious/compromised response can create astronomically large 
            // decompressed response).
            if (maxDecompressedSize == maxDecompressedSizeAllowed)
            {
                BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpResult].decompressRawBuffer: "
                    "(server: " << getOwnerConnection()->getOwnerManager().getAddress().getHostname() << ") "
                    "Problem decompressing HTTP response (Zlib Error: " << zlibErr << ") after allocating maximum allowed response buffer (" << maxDecompressedSize << " bytes)");
                break;
            }
            else
            {
                allocatedLargeBuffer = true;
                zlibInflate.reset();
                continue;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpResult].decompressRawBuffer: "
                            "(server: " << getOwnerConnection()->getOwnerManager().getAddress().getHostname() << ") "
                        "Problem decompressing HTTP response (Zlib Error: " << zlibErr << ")");
            break;
        }
    }

    if (allocatedLargeBuffer)
    {
        // change the mDecompressedOutput and mDecompressedOutputSize back to default.
        // high compression rate rarely happens, so we free the large buffer to save memory. 
        mDecompressedOutputSize = 1024;
        delete[] mDecompressedOutput;
        mDecompressedOutput = BLAZE_NEW_ARRAY_MGID(uint8_t, mDecompressedOutputSize, MEM_GROUP_FRAMEWORK_CATEGORY, "OutboundHttpConnection::mDecompressedOutput");
    }

    return success;
}

BlazeRpcError OutboundHttpResult::decodeResult(RawBuffer& response)
{
    BlazeRpcError parseResult = ERR_OK;
    if ((getHttpStatusCode() != HTTP_STATUS_OK) && (getHttpStatusCode() != HTTP_STATUS_SUCCESS) && (getHttpStatusCode() != HTTP_STATUS_FOUND) && (getHttpStatusCode() != HTTP_STATUS_NOT_MODIFIED))
    {
        setHttpError();
        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpResult].decodeResult: Response StatusCode (" << getHttpStatusCode() << ")");
    }

    OutboundHttpXmlParser reader(*this);
    XmlReader xmlReader(response);
    parseResult = xmlReader.parse(&reader) ? ERR_OK : ERR_SYSTEM;

    return parseResult;
}


BlazeRpcError OutboundHttpXmlResult::decodeResult(RawBuffer& response)
{
    BlazeRpcError parseResult = ERR_OK;
    if ((getHttpStatusCode() != HTTP_STATUS_OK) && (getHttpStatusCode() != HTTP_STATUS_SUCCESS) && (getHttpStatusCode() != HTTP_STATUS_FOUND) && (getHttpStatusCode() != HTTP_STATUS_NOT_MODIFIED))
    {
        setHttpError();
        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpResult].decodeResult: Response StatusCode (" << getHttpStatusCode() << ")");
    }

    OutboundHttpXmlParser reader(*this);
    XmlReader xmlReader(response);
    parseResult = xmlReader.parse(&reader) ? ERR_OK : ERR_SYSTEM;

    mBuf.append((char8_t*)response.data(), response.size());
    mBuf[mBuf.size()] = '\0';
    return parseResult;
}

//Suggested HTTP Reason-Phrase mapping: http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
const char8_t* const OutboundHttpResult::lookupHttpStatusReason(const HttpStatusCode statusCode)
{
    switch (statusCode)
    {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Time-out";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Large";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested range not satisfiable";
        case 417: return "Expectation Failed";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Time-out";
        case 505: return "HTTP Version not supported";
        default:
           return "HTTP Reason-Phrase is not available";
    }
}

} // Blaze


