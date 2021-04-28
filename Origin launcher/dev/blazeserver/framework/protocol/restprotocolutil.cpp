/*************************************************************************************************/
/*!
    \file restprotocolutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RestProtocolUtil

    Utility functions for the REST HTTP Protocol.

    \notes

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/protocol/restprotocol.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/restdecoder.h"
#include "framework/protocol/shared/jsonencoder.h"

#include "framework/util/hashutil.h"

// Zlib Compression
#include "framework/util/compression/zlibdeflate.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
const char8_t RestProtocolUtil::HEADER_BLAZE_ERRORCODE[] =  "X-BLAZE-ERRORCODE";

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief getRestResourceInfo
    
    Parses the passed URI and returns a pointer to a RestResourceInfo object.

    \param[in] uri - The expected format of the WAL URI is /version/resource_path
    Where:
        version = version number of REST protocol (e.g. 1.0)
        resource path = path to resource (e.g. message/123/987)
    \param[in] method - The HTTP method name of the request.

    \return - Pointer to RestResourceInfo object.
        nullptr if there is no REST resource at the passed URI.
*/
/************************************************************************************************/
const RestResourceInfo* RestProtocolUtil::getRestResourceInfo(const char8_t* method, const char8_t* uri)
{
    const char8_t* ch = uri;
    if (ch[0] == '/')
        ++ch;
    else
        BLAZE_WARN_LOG(Log::REST, "[RestProtocolUtil].getRestResourceInfo: URI doesn't start with a slash: " << uri);

    const char8_t* slash = strchr(ch, '/');

    if (slash == nullptr)
    {
        BLAZE_WARN_LOG(Log::REST, "[RestProtocolUtil].getRestResourceInfo: URI does not contain a version: " << uri);
        return nullptr;
    }

    // The resource is the rest of the line up to the first '?' which begins the params, if any.
    ch = slash + 1;

    return BlazeRpcComponentDb::getRestResourceInfo(method, ch);
}

const RestResourceInfo* RestProtocolUtil::getRestResourceInfo(uint16_t componentId, uint16_t commandId)
{
    return BlazeRpcComponentDb::getRestResourceInfo(componentId, commandId);
}

/*************************************************************************************************/
/*!
    \brief buildCustomHeaderMap

    Build a mapping of header names to header values given the custom header mapping and the TDF

    \param[in] headerTdfPair - Custom header to tdf field mapping.
    \param[in] arrayCount - Size of the headerTdfPair.
    \param[in] tdf - Where header values are extracted from
    \param[out] stringMap - Where to store the entries in the map.
*/
/*************************************************************************************************/
void RestProtocolUtil::buildCustomHeaderMap(const HttpFieldMapping* headerTdfPair, size_t arrayCount, const EA::TDF::Tdf& tdf, HttpHeaderMap& stringMap, Encoder::SubType encSubType)
{
    for (size_t counter = 0; counter < arrayCount; ++counter)
    {   
        StringBuilder headerValue;
        if (tdfToStringBuilder(headerValue, tdf, headerTdfPair[counter], encSubType, RestRequestOptions()))
            stringMap[headerTdfPair[counter].header] = headerValue.get();
    }
}


/*************************************************************************************************/
/*!
    \brief createHeaderString

    Build a string with header name-value pairs from the given header map separated by return and newline characters

    \param[in] headerMap - Map of header names and values
    \param[out] customString - Where the string is written to
    \param[in] customStringSize - Size of the output string
*/
/*************************************************************************************************/
void RestProtocolUtil::createHeaderString(const HttpHeaderMap& headerMap, char8_t* customString, size_t customStringSize)
{
    customString[0] = '\0';

    HttpHeaderMapIter iter = headerMap.begin();
    for (; iter != headerMap.end(); ++iter)
    {
        size_t headerLen = strlen(customString);
        blaze_snzprintf(customString + headerLen, customStringSize - headerLen, "%s: %s\r\n", iter->first.c_str(), iter->second.c_str());
    }
}

void RestProtocolUtil::printHttpRequest(uint16_t componentId, uint16_t commandId, const EA::TDF::Tdf* requestTdf, StringBuilder& sb)
{
    const RestResourceInfo* restInfo = getRestResourceInfo(componentId, commandId);
    if (restInfo == nullptr)
        return; 
    
    HeaderVector headerVector;
    HttpParamVector httpParams;

    if (requestTdf != nullptr)
    {
        buildCustomHeaderVector(*restInfo, *requestTdf, headerVector, true);

        // parse the params
        buildCustomParamVector(*restInfo, *requestTdf, httpParams, true);
    }

    StringBuilder uri;
    constructUri(*restInfo, requestTdf, uri, nullptr, true);

    printHttpRequest(uri, *restInfo, headerVector, httpParams, requestTdf, nullptr, sb);

    for(auto& param : httpParams)
    {
        // clean up memory allocated via str_dup
        BLAZE_FREE(param.value);
    }
}

void RestProtocolUtil::printHttpRequest(const StringBuilder &uri, const RestResourceInfo& restInfo, const HeaderVector& headers, const HttpParamVector& params, const EA::TDF::Tdf* requestTdf, const RawBuffer *payload, StringBuilder& sb)
{
    sb.append("%s %s\n", restInfo.methodName, uri.get());

    if (!headers.empty())
    {
        sb.append("Headers:\n");
        for (auto& header : headers)
        {
            sb.append("\t%s\n", header.c_str());
        }
    }

    if (!params.empty())
    {
        sb.append("Params:\n");
        for (HttpParamVector::const_iterator it = params.begin(); it != params.end(); ++it)
        {
            const HttpParam& p = *it;
            sb.append("\t%s:%s\n", p.name, p.value);
        }
    }

    if (restInfo.addEncodedPayload)
    {
        if (payload != nullptr && !restInfo.compressPayload)
        {
            sb.append("Payload:\n%s", reinterpret_cast<const char8_t*>(payload));
        }
        else if (requestTdf != nullptr)
        {
            sb.append("Payload:\n");
            sb << requestTdf;
        }
    }
}

void RestProtocolUtil::printHttpResponse(const StringBuilder &uri, const RestResourceInfo& restInfo, const HttpHeaderMap& headers, const EA::TDF::Tdf* responseTdf, StringBuilder& sb)
{
    sb.append("%s %s\n", restInfo.methodName, uri.get());

    if (!headers.empty())
    {
        sb.append("Headers:\n");
        for (auto& header : headers)
        {
            sb.append("\t%s: %s\n", header.first.c_str(), header.second.c_str());
        }
    }

    if (responseTdf != nullptr)
    {
        sb.append("Response:\n");
        sb << responseTdf;
    }
};

/*! ************************************************************************************************/
/*! \brief send the HTTP request
    \param[out] responseTdf - If not nullptr, and response can be decoded to TDF, holds the decoded result
    \param[out] responseRaw - If not nullptr, and response available, holds the raw response data
***************************************************************************************************/
BlazeRpcError RestProtocolUtil::sendHttpRequest(HttpConnectionManagerPtr connMgr, ComponentId componentId, CommandId commandId, 
    const EA::TDF::Tdf* requestTdf, const char8_t* contentType, bool addEncodedPayload, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf, 
    HttpHeaderMap* responseHeaderMap, const char8_t* uriPrefix, HttpStatusCode* statusCode, int32_t logCategory, const RpcCallOptions &options/* = RpcCallOptions()*/,
    RawBuffer* responseRaw /*= nullptr*/)
{
    const RestResourceInfo* restInfo = getRestResourceInfo(componentId, commandId);
    if (restInfo == nullptr)
    {
        BLAZE_WARN_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: No rest resource info found for (component= '" 
            << BlazeRpcComponentDb::getComponentNameById(componentId) << "', command='" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "').");
        return ERR_SYSTEM;
    }

    RawBuffer* payload = nullptr;

    if (addEncodedPayload && requestTdf != nullptr)
    {
        payload = BLAZE_NEW RawBuffer(2048);
        if (!encodePayload(restInfo, HttpXmlProtocol::getEncoderTypeFromMIMEType(contentType), requestTdf, *payload))
        {
            BLAZE_WARN_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: Failed to encode payload for (component= '" 
                << BlazeRpcComponentDb::getComponentNameById(componentId) << "', command='" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "').");
        }
    }

    BlazeRpcError error = sendHttpRequest(connMgr, restInfo, requestTdf, contentType, responseTdf, errorTdf,
        payload, responseHeaderMap, uriPrefix, statusCode, logCategory, options, responseRaw);
    if (addEncodedPayload)
        delete payload;
    return error;
}

BlazeRpcError RestProtocolUtil::sendHttpRequest(HttpConnectionManagerPtr connMgr, const RestResourceInfo* restInfo, 
    const EA::TDF::Tdf* requestTdf, const char8_t* contentType, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf, RawBuffer* payload, 
    HttpHeaderMap* responseHeaderMap, const char8_t* uriPrefix, HttpStatusCode* statusCode, int32_t logCategory, const RpcCallOptions &options/* = RpcCallOptions()*/,
    RawBuffer* responseRaw /*= nullptr*/)
{
    if (restInfo == nullptr)
    {
        BLAZE_WARN_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: The rest resource info provided is invalid.");
        return ERR_SYSTEM;
    }

    // build the custom header of the request
    HeaderVector headerVector;
    HttpParamVector httpParams;
    if (requestTdf != nullptr)
    {
        buildCustomHeaderVector(*restInfo, *requestTdf, headerVector, false);

        // parse the params
        buildCustomParamVector(*restInfo, *requestTdf, httpParams, false);
    }

    // parse the user and password for HTTP authorization
    const char8_t* httpUserPwd = nullptr;
    if (restInfo->authorizationFunc != nullptr)
    {
        httpUserPwd = (*restInfo->authorizationFunc)(const_cast<EA::TDF::Tdf&>(*requestTdf));
    }

    // construct the uri
    StringBuilder uri;
    constructUri(*restInfo, requestTdf, uri, uriPrefix, true);

    RestOutboundHttpResult httpResult(restInfo, responseTdf, errorTdf, responseHeaderMap, responseRaw);

    if (payload != nullptr && restInfo->compressPayload)
    {
        char8_t conentEncodingHeader[256] = "";
        compressRawBuffer(*payload, conentEncodingHeader, sizeof(conentEncodingHeader), RestProtocol::getCompressionLevel());
        headerVector.push_back(BLAZE_EASTL_STRING(conentEncodingHeader));
    }
    size_t payloadSize = (payload != nullptr ? payload->datasize() : 0);

    const char8_t** httpHeaders = nullptr;
    if (!headerVector.empty())
        httpHeaders = createHeaderCharArray(headerVector);

    OutboundHttpConnection::ContentPartList contentList;
    OutboundHttpConnection::ContentPart content;
    if (payloadSize > 0)
    {
        content.mName = "content";
        content.mContentType = contentType;
        content.mContent = reinterpret_cast<const char8_t*>(payload->data());
        content.mContentLength = payloadSize;
        contentList.push_back(&content);
    }

    // send request
    BlazeRpcError requestError = connMgr->sendRequest(HttpProtocolUtil::getMethodType(restInfo->methodName), uri.get(), 
        &httpParams[0], httpParams.size(), httpHeaders, headerVector.size(),
        &httpResult, &contentList, logCategory, nullptr, httpUserPwd, restInfo->maxHandledUrlRedirects, options);

    if (requestError == ERR_OK)
        requestError = httpResult.getBlazeErrorCode();

    bool logErr = ((requestError != ERR_OK) && (connMgr->getHttpStatusCodeErrSet().find(httpResult.getStatusCode()) != connMgr->getHttpStatusCodeErrSet().end()));
    if (logErr || BLAZE_IS_LOGGING_ENABLED(Log::REST, Logging::TRACE))
    {
        StringBuilder requestLog;
        printHttpRequest(restInfo->componentId, restInfo->commandId, requestTdf, requestLog);

        StringBuilder responseLog;
        printHttpResponse(uri, *restInfo, httpResult.getHeaderMap(), ((requestError == ERR_OK) ? responseTdf : errorTdf), responseLog);
        if (!logErr)
        {
            BLAZE_TRACE_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: Sent request to " << connMgr->getAddress().getHostname() << " " << requestLog);
            BLAZE_TRACE_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: Received response " << ErrorHelp::getErrorName(requestError) << ", HttpStatus " << httpResult.getStatusCode() << " from " << connMgr->getAddress().getHostname() << " " << responseLog);
        }
        else
        {
            BLAZE_ERR_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: Sent request to " << connMgr->getAddress().getHostname() << " " << requestLog);
            BLAZE_ERR_LOG(Log::REST, "[RestProtocolUtil].sendHttpRequest: Received response " << ErrorHelp::getErrorName(requestError) << ", HttpStatus " << httpResult.getStatusCode() << " from " << connMgr->getAddress().getHostname() << " " << responseLog);
        }
    }

    if (statusCode != nullptr)
        *statusCode = httpResult.getStatusCode();

    // clean up locally allocated memory
    if (httpHeaders != nullptr)
        delete[] httpHeaders;

    HttpParamVector::const_iterator itr = httpParams.begin();
    HttpParamVector::const_iterator end = httpParams.end();
    for(; itr != end; itr++)
    {
        // clean up memory allocated via str_dup
        BLAZE_FREE((*itr).value);
    }

    return requestError;
}

/* Private methods *******************************************************************************/


Blaze::HttpStatusCode RestProtocolUtil::parseSuccessStatusCode(const SuccessStatusCodeMapping* statusCodePair, size_t arrayCount, const EA::TDF::Tdf& requestTdf)
{
    if (statusCodePair != nullptr)
    {
        for (size_t counter = 0; counter < arrayCount; ++counter)
        {
            EA::TDF::TdfGenericReferenceConst tdfResult;
            if (requestTdf.getValueByTags(statusCodePair[counter].tagArray, statusCodePair[counter].tagCount, tdfResult) &&  
                (tdfResult.getType() == EA::TDF::TDF_ACTUAL_TYPE_UINT8 || tdfResult.getType() == EA::TDF::TDF_ACTUAL_TYPE_UINT16 || tdfResult.getType() == EA::TDF::TDF_ACTUAL_TYPE_UINT32 || tdfResult.getType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64) &&
                tdfResult.asUInt64() > 0)
            {
                return statusCodePair[counter].statusCode;
            }
        }
    }

    return HTTP_STATUS_OK;
}

BlazeRpcError RestProtocolUtil::lookupBlazeRpcError(const StatusCodeErrorMapping* statusCodePair, size_t arrayCount, HttpStatusCode statusCode)
{
    if (statusCodePair != nullptr)
    {
        for (size_t counter = 0; counter < arrayCount; ++counter)
        {
            if (statusCodePair[counter].statusCode == statusCode)
            {
                return statusCodePair[counter].error;
            }
        }
    }

    return ERR_SYSTEM;
}

bool RestProtocolUtil::compressRawBuffer(RawBuffer& rawBuffer, char8_t* header, size_t headerSize, int32_t compressionLevel)
{
    static const char8_t* CONTENT_ENCODING_STRING = "Content-Encoding: gzip";

    uint8_t* uncompressedData = rawBuffer.data();
    size_t uncompressedDataSize = rawBuffer.datasize();

    // Setup ZLib for gzip compression
    ZlibDeflate zlibDeflate(ZlibDeflate::COMPRESSION_TYPE_GZIP, compressionLevel);

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
            size_t headerLen = strlen(header);
            blaze_snzprintf(header + headerLen, headerSize - headerLen, CONTENT_ENCODING_STRING);
        }
        else
        {
            BLAZE_ERR(Log::REST, "Problem saving compressed respose to raw buffer!!");
        }
    }
    else
    {
        BLAZE_ERR(Log::REST, "Problem compressing XML response (Zlib Error: %d)", zlibErr);
    }

    delete[] compressedData;
    compressedData = nullptr;

    return success;
}

uint32_t RestOutboundHttpResult::processRecvData(const char8_t* data, uint32_t size, bool isResultInCache)
{
    // For cached responses, we may not have an owner http connection so we bypass that here (this does not make a difference
    // as the cached response must have fit in the limits we defined earlier).
    if (getOwnerConnection() != nullptr)
    {
        if (mRawBuffer.size() + size > getOwnerConnection()->getOwnerManager().getMaxHttpResponseSize())
        {
            BLAZE_ERR(Log::REST, "[RestOutboundHttpResult].processRecvData: "
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

        if (isResultInCache && mResponseTdf != nullptr)
        {
            // Decode from http cache into mResponseTdf
            RestDecoder decoder;
            decoder.decodeRestResponse(true, false, HTTP_STATUS_OK); // Pass in HTTP_STATUS_OK here intentionally to trigger decoding the data as message-body

            decoder.setRestResourceInfo(mRestInfo);

            HttpHeaderMap headerMap;
            decoder.setHttpResponseHeaders(&headerMap);
            mRawBuffer.mark();
            mErrorCode = decoder.decode(mRawBuffer, *mResponseTdf);
            if (mErrorCode != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::REST, "[RestOutboundHttpResult].processRecvData: Failed to decode received data as tdf '" <<
                    mResponseTdf->getClassName() << "'. Received data: " << reinterpret_cast<char8_t*>(mRawBuffer.head()));
            }
            mRawBuffer.resetToMark();

            // Set the status codes correctly, so that we don't have to call decodeResult.
            mStatusCode = HTTP_STATUS_OK;
        }
    }

    return size;
}

BlazeRpcError RestOutboundHttpResult::decodeResult(RawBuffer& response)
{
    // Process the payload.

    mStatusCode = getHttpStatusCode();

    // return the blaze error code that maps to the received HTTP error code
    mErrorCode = RestProtocolUtil::lookupBlazeRpcError(mRestInfo->statusCodeErrors, mRestInfo->statusCodeErrorsArrayCount, mStatusCode);

    if (mHeaderMap != nullptr)
    {
        *mHeaderMap = getHeaderMap();
    }

    EA::TDF::Tdf* decodeTdf = nullptr;

    if ((mStatusCode >= HTTP_STATUS_OK && mStatusCode <= HTTP_STATUS_SUCCESS_END) || mStatusCode == HTTP_STATUS_FOUND || mStatusCode == HTTP_STATUS_NOT_MODIFIED)
    {
        decodeTdf = mResponseTdf;
        // override with ERR_OK if no custom mapping is defined since such code lies in the success status code range
        if (mErrorCode == ERR_SYSTEM || mStatusCode == HTTP_STATUS_NOT_MODIFIED)
            mErrorCode = ERR_OK;
    }
    else if (mStatusCode >= HTTP_STATUS_CLIENT_ERROR_START && mStatusCode <= HTTP_STATUS_CLIENT_ERROR_END)
    {
        decodeTdf = mErrorTdf;

        //retrieve blaze error code via the response header as needed
        if (mRestInfo->useBlazeErrorCode)
        {
            const char8_t* errorCodeString = getHeader(RestProtocolUtil::HEADER_BLAZE_ERRORCODE);
            if (errorCodeString != nullptr)
            {
                BlazeRpcError tmp;
                if (blaze_str2int(errorCodeString, &tmp) != errorCodeString)
                    mErrorCode = tmp;
                else
                    BLAZE_WARN_LOG(Log::REST, "[RestOutboundHttpResult].decodeResult: unable to parse error code from header string '" << errorCodeString << "'.");
            }
        }
    }

    if (mStatusCode >= HTTP_STATUS_SERVER_ERROR_START && mStatusCode <= HTTP_STATUS_SERVER_ERROR_END)
    {
        //overwrite error code to ERR_SERVICE_INTERNAL_ERROR if no custom error code mapping
        if (mErrorCode == ERR_SYSTEM)
        {
            mErrorCode = ERR_SERVICE_INTERNAL_ERROR;
        }

        eastl::string rspBuf;
        rspBuf.append_sprintf("%s", reinterpret_cast<const char8_t*>(response.data()), response.datasize());
        BLAZE_ERR_LOG(Log::REST, "[RestOutboundHttpResult].decodeResult: " << mRestInfo->methodName << " " << mRestInfo->resourcePath
            << " returned HTTP status code(" << mStatusCode << "), err(" << ErrorHelp::getErrorName(mErrorCode) << "), response: " << rspBuf.c_str());
    }
    else
    {
        BLAZE_TRACE_LOG(Log::REST, "[RestOutboundHttpResult].decodeResult: " << mRestInfo->methodName << " " << mRestInfo->resourcePath
            << " returned HTTP status code(" << mStatusCode << "), err(" << ErrorHelp::getErrorName(mErrorCode) << ")");
    }

    if (mResponseRaw != nullptr)
    {
        mResponseRaw->acquire(response.datasize());
        memcpy(mResponseRaw->tail(), response.data(), response.datasize());
        mResponseRaw->put(response.datasize());
    }

    if (decodeTdf != nullptr)
    {
        RestDecoder decoder;
        decoder.decodeRestResponse(true, (mErrorCode != ERR_OK), mStatusCode);
        
        decoder.setRestResourceInfo(mRestInfo);
        decoder.setHttpResponseHeaders(&getHeaderMap());
        BlazeRpcError error = decoder.decode(response, *decodeTdf);
        if (error != ERR_OK)
        {
            // Only replace the error code if there was no error detected earlier. 
            // This avoids some stupid issues where MS services return invalid strings instead of JSON when an Error occurs. 
            if (mErrorCode == ERR_OK)
                mErrorCode = error;

            BLAZE_ERR_LOG(Log::REST, "[RestOutboundHttpResult].decodeResult: Failed to decode received data as tdf '" <<
                decodeTdf->getClassName() << "'. Error code: " << error << ". Received data: " << reinterpret_cast<char8_t*>(response.head()));
        }
        response.resetToMark();
    }

    return mErrorCode;
}

CURL_DEBUG_SCRUB_TYPE RestOutboundHttpResult::getDebugScrubType(curl_infotype infoType) const
{
    if (infoType != CURLINFO_DATA_IN)
        return OutboundHttpResponseHandler::getDefaultScrubType(infoType);

    if (mResponseTdf != nullptr)
    {
        if ((blaze_strcmp(mResponseTdf->getFullClassName(), "Blaze::SecretVault::SecretVaultSecret") == 0)
            || (blaze_strcmp(mResponseTdf->getFullClassName(), "Blaze::SecretVault::SecretVaultKv2Secret") == 0))
        {
            return VAULT_PAYLOAD;
        }
    }

    return DEFAULT;
}

BlazeRpcError RawRestOutboundHttpResult::requestComplete(CURLcode res)
{
    if (res != CURLE_OK)
    {
        return ERR_SYSTEM;
    }

    return ERR_OK;
}

} // Blaze

