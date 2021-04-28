/*************************************************************************************************/
/*!
    \file httpconnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

#include "BlazeSDK/httpconnection.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "framework/protocol/shared/httpparam.h"
#include "framework/protocol/shared/restdecoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/stringbuilder.h"
#include "DirtySDK/proto/protohttputil.h"
#include "BlazeSDK/component.h"
#include "BlazeSDK/util/utilapi.h"
#include "EACrypto/EAObfuscation.h"
#include "EAStdC/EAString.h"

namespace Blaze
{

// Obfuscate strings to deter reverse engineer editing title's binary and point to their own proxy.  See GOS-28564 for more info.
EA_CRYPTO_DECLARE_OBFUSCATED_STRING(OBFUSCATED_HTTP_SECURE_URL_PREFIX, "https://");
EA_CRYPTO_DECLARE_OBFUSCATED_STRING(OBFUSCATED_HTTP_INSECURE_URL_PREFIX, "http://");

const char8_t* HttpConnection::RPC_ERROR_HEADER_NAME = "X-BLAZE-ERRORCODE";
const char8_t* HttpConnection::S2S_AUTH_HEADER_NAME = "Authorization:";

HttpConnection::RequestData::RequestData() : paramVector(MEM_GROUP_FRAMEWORK_TEMP, "RequestData::paramVector") { reset(); }
HttpConnection::RequestData::~RequestData() { reset(); }

void HttpConnection::RequestData::reset()
{
    RestRequestBuilder::freeCustomParamVector(paramVector);

    restInfo = nullptr;
    paramVector.clear();
    appendHeader.clear();
    uri.reset();
    component = 0;
    command = 0;
    userIndex = 0;
}

HttpConnection::HttpConnection(BlazeHub &hub, Encoder::Type encoderType)
  : BlazeSender(hub),
    mProtoHttpRef(nullptr),
    mRestDecoder(nullptr),
    mDefaultEncoderType(encoderType)
{
    mProtoHttpRef = ProtoHttpCreate(RECEIVE_BUFFER_SIZE_BYTES);
    //use config overrides found in util.cfg
    Blaze::Util::UtilAPI::createAPI(hub);
    getBlazeHub().getUtilAPI()->OverrideConfigs(mProtoHttpRef, "HttpConnection");
    mRestDecoder = BLAZE_NEW(MEM_GROUP_FRAMEWORK, "HttpConnection::mRestDecoder") RestDecoder();
}

HttpConnection::~HttpConnection()
{
    getBlazeHub().removeIdler(this);
    BLAZE_DELETE(MEM_GROUP_FRAMEWORK, mRestDecoder);
    ProtoHttpDestroy(mProtoHttpRef);
    mProtoHttpRef = nullptr;
}

void HttpConnection::setAuthenticationData(const char8_t* certData, const char8_t* keyData, size_t certDataSize /*= 0*/, size_t keyDataSize /*= 0*/)
{
    if (isActive())
    {
        BLAZE_SDK_DEBUGF("[HttpConnection:%p].setAuthentication: Cannot change the client cert and key data on an active connection. \r\n", this);
    }
    else if (mProtoHttpRef == nullptr)
    {
        BLAZE_SDK_DEBUGF("[HttpConnection:%p].setAuthentication: Failed to create ProtoHttpRef, cannot set the client cert and key data.\r\n", this);
    }
    else
    {
        int32_t cSize, kSize;
        if (certDataSize == 0 && certData != nullptr)
            cSize = (int32_t)strlen(certData);
        else
            cSize = (int32_t)certDataSize;
        if (keyDataSize == 0 && keyData != nullptr)
            kSize = (int32_t)strlen(keyData);
        else
            kSize = (int32_t)keyDataSize;

        if (cSize != 0)
            ProtoHttpControl(mProtoHttpRef, 'scrt', cSize, 0, (void*)certData);
        if (kSize != 0)
            ProtoHttpControl(mProtoHttpRef, 'skey', kSize, 0, (void*)keyData);
    }
}

bool HttpConnection::isActive()
{
    return (getBlazeHub().getScheduler()->getJob(this, 0) != nullptr);
}

BlazeError HttpConnection::canSendRequest()
{
    if (mProtoHttpRef == nullptr)
    {
        BLAZE_SDK_DEBUG("[HttpConnection] Failed to create ProtoHttpRef, cannot send.\r\n");
        return SDK_ERR_RPC_SEND_FAILED;
    }

    if (getSendBuffer().datasize() != 0)
    {
        BLAZE_SDK_DEBUG("[HttpConnection] Outstanding http request, cannot send.\r\n");
        return SDK_ERR_IN_PROGRESS;
    }

    if (getBlazeHub().getComponentManager() == nullptr)
    {
        BLAZE_SDK_DEBUG("[HttpConnection] ComponentManager is null, unable to send request.\r\n");
        return SDK_ERR_RPC_SEND_FAILED;
    }

    return ERR_OK;
}

/*! ***************************************************************/
/*! \brief Send a request for a proxy component utilizing this HTTP connection.
        
    \param[in] userIndex - the local index sending the request
    \param[in] component - which component the command is part of.
    \param[in] command - the command we are executing
    \param[in] request - the request object
    \param[in] reserveId - the reserved job id that will be associated with the request
    \param[in] timeout - a timeout override for the request.
*******************************************************************/
BlazeError HttpConnection::sendRequestToBuffer(uint32_t userIndex, uint16_t component, uint16_t command, MessageType type, const EA::TDF::Tdf *msg, uint32_t msgId)
{
    mRequestData.reset();

    Component *comp = getBlazeHub().getComponentManager()->getComponentById(component);
    if (comp == nullptr)
    {
        BLAZE_SDK_DEBUGF("[HttpConnection] No component with id(%u), unable to send request.\r\n", component);
        return ERR_COMPONENT_NOT_FOUND;
    }

    mRequestData.restInfo = comp->getRestInfo(command);
    if (mRequestData.restInfo == nullptr)
    {
        BLAZE_SDK_DEBUGF("[HttpConnection] Command id(%u) on component id(%u) does not contain rest information, unable to send request.\r\n", command, component);
        return SDK_ERR_RPC_SEND_FAILED;
    }

    mRequestData.userIndex = userIndex;
    mRequestData.component = component;
    mRequestData.command = command;

    // Content Type preference is to use what is in the specific request.
    // If no content type was specified, we default to using the value associated with the encoder.
    Encoder::Type encoderType = mDefaultEncoderType;
    const char8_t* contentType = mRequestData.restInfo->contentType;
    if (contentType == nullptr)
    {
        contentType = RestRequestBuilder::getContentTypeFromEncoderType(encoderType, true);
    }
    else
    {
        // If a specific content type was requested see if we need to override the default encoder
        Encoder::Type contentEncoderType = RestRequestBuilder::getEncoderTypeFromContentType(contentType);
        if (contentEncoderType != Encoder::INVALID)
        {
            contentType = RestRequestBuilder::getContentTypeFromEncoderType(encoderType, true);
            encoderType = contentEncoderType;
        }
    }

    // encode the payload
    if (mRequestData.restInfo->addEncodedPayload)
    {
        if (!RestRequestBuilder::encodePayload(mRequestData.restInfo, encoderType, msg, getSendBuffer()))
        {
            BLAZE_SDK_DEBUGF("[HttpConnection] Command id(%u) on component id(%u) was not able to encode the payload, unable to send request.\r\n", command, component);
            return SDK_ERR_RPC_SEND_FAILED;
        }
    }

    // build the custom header of the request
    RestRequestBuilder::HeaderVector headerVector(MEM_GROUP_FRAMEWORK_TEMP, "HttpConnection::sendRequestToBuffer::headerVector");
    if (msg != nullptr)
    {
        RestRequestBuilder::buildCustomHeaderVector(*mRequestData.restInfo, *msg, headerVector);

        createOwnedHeaderString(headerVector, mRequestData.appendHeader);

        // parse the params
        mRequestData.paramVector.reserve((RestRequestBuilder::HttpParamVector::size_type)mRequestData.restInfo->urlParamsArrayCount);
        RestRequestBuilder::buildCustomParamVector(*mRequestData.restInfo, *msg, mRequestData.paramVector);
    }

    // Append the contentType only if it's set and not an empty string.
    if (contentType != nullptr && contentType[0] != '\0')
        mRequestData.appendHeader.append_sprintf("%s\r\n", contentType);

    char8_t uriPrefix[URI_PREFIX_SIZE] = "";
    if (mRequestData.restInfo->apiVersion != nullptr)
        blaze_snzprintf(uriPrefix, sizeof(uriPrefix), "/%s", mRequestData.restInfo->apiVersion);

    // construct the uri
    RestRequestBuilder::constructUri(*mRequestData.restInfo, msg, mRequestData.uri, uriPrefix);

    return ERR_OK;
}

/*! ***************************************************************/
/*! \brief Send a request through ProtoHTTP.
*******************************************************************/
BlazeError HttpConnection::sendRequestFromBuffer()
{
    if (getServerConnInfo().getAutoS2S())
    {
        // Add ourselves as a S2SManagerListener and wait for onFetchedS2SToken to return a valid access token
        getBlazeHub().getS2SManager()->addListener(this);
        getBlazeHub().getS2SManager()->getS2SToken();
        return ERR_OK;
    }

    return sendRequestFromBufferInternal();
}

BlazeError HttpConnection::sendRequestFromBufferInternal()
{
    ProtoHttpControl(mProtoHttpRef, 'spam', 2, 0, nullptr);

    // construct the url
    char8_t url[URL_SIZE];
    constructUrl(url, sizeof(url), mRequestData.uri.get(), mRequestData.paramVector);

    if (ProtoHttpControl(mProtoHttpRef, 'apnd', 0, 0, const_cast<char8_t*>(mRequestData.appendHeader.c_str())) != 0)
    {
        // error setting header
        return SDK_ERR_RPC_SEND_FAILED;
    }

    getBlazeHub().addIdler(this, "HttpConnection");

    int32_t protoResult;

    HttpProtocolUtil::HttpMethod method = HttpProtocolUtil::getMethodType(mRequestData.restInfo->methodName);
    switch (method)
    {
    case HttpProtocolUtil::HTTP_GET:
        protoResult = ProtoHttpGet(mProtoHttpRef, url, false);
        break;
    case HttpProtocolUtil::HTTP_HEAD:
        protoResult = ProtoHttpGet(mProtoHttpRef, url, true);
        break;
    case HttpProtocolUtil::HTTP_PUT:
        // trailing true argument sets put
        protoResult = ProtoHttpPost(mProtoHttpRef, url, (char8_t*)getSendBuffer().data(), getSendBuffer().datasize(), true);
        break;
    case HttpProtocolUtil::HTTP_POST:
        // trailing false argument sets post
        protoResult = ProtoHttpPost(mProtoHttpRef, url, (char8_t*)getSendBuffer().data(), getSendBuffer().datasize(), false);
        break;
    case HttpProtocolUtil::HTTP_DELETE:
        protoResult = ProtoHttpDelete(mProtoHttpRef, url);
        break;
    default:
        // unsupported method
        getBlazeHub().removeIdler(this);
        return SDK_ERR_RPC_SEND_FAILED;
        break;
    }

    if ((method == HttpProtocolUtil::HTTP_POST) || (method == HttpProtocolUtil::HTTP_PUT))
    {
        // for post/put, return value is amount of data sent.  We then need to idle and call
        // ProtoHttpSend until all data is sent.
        if (protoResult < 0) 
        {
            // error
            BLAZE_SDK_DEBUGF("[HttpConnection] doSendRequest - ProtoHttp %s request failed with %d.\r\n", HttpProtocolUtil::getStringFromHttpMethod(method), protoResult);
            return ERR_SYSTEM;
        }
        else 
        {
            // result is number of bytes sent.
            getSendBuffer().pull(protoResult);
            BLAZE_SDK_DEBUGF("[HttpConnection] doSendRequest - ProtoHttp %s request has %d sent size.\r\n", HttpProtocolUtil::getStringFromHttpMethod(method), protoResult)
        }
    }
    else if (method == HttpProtocolUtil::HTTP_GET || method == HttpProtocolUtil::HTTP_HEAD || method == HttpProtocolUtil::HTTP_DELETE)
    {
        size_t dataSize = getSendBuffer().datasize();
        if (dataSize > 0)
        {
            getSendBuffer().pull(dataSize);
            BLAZE_SDK_DEBUGF("[HttpConnection] doSendRequest - ProtoHttp %s request has %u sent size.\r\n", HttpProtocolUtil::getStringFromHttpMethod(method), (uint32_t)dataSize);
        }
    }
    else if (protoResult < 0)
    {
        BLAZE_SDK_DEBUGF("[HttpConnection] doSendRequest - ProtoHttp %s request failed with %d.\r\n", HttpProtocolUtil::getStringFromHttpMethod(method), protoResult);
    }

    return ERR_OK;
}

void HttpConnection::finishRequest(MessageType msgType, BlazeError rpcError)
{
    handleReceivedPacket(
        0, msgType,
        mRequestData.component, mRequestData.command,
        mRequestData.userIndex, rpcError,
        *mRestDecoder, getReceiveBuffer().data(), getReceiveBuffer().datasize());

    getReceiveBuffer().resetToPrimary();
    getSendBuffer().resetToPrimary();
    getBlazeHub().removeIdler(this);
}

void HttpConnection::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    HttpProtocolUtil::HttpMethod method = HttpProtocolUtil::getMethodType(mRequestData.restInfo->methodName); 
    if ((method == HttpProtocolUtil::HTTP_PUT) || (method == HttpProtocolUtil::HTTP_POST)) 
    { 
        if (getSendBuffer().datasize() > 0)
        {
            BLAZE_SDK_SCOPE_TIMER("HttpConnection_httpSend");

            int32_t sentSize = ProtoHttpSend(mProtoHttpRef, (char8_t*)getSendBuffer().data(), (int32_t)getSendBuffer().datasize());
            if (sentSize < 0)
            {
                BLAZE_SDK_DEBUGF("[HttpConnection] idle - ProtoHttp request failed to send data with %d.\n", sentSize);
                finishRequest(ERROR_REPLY, SDK_ERR_CONN_FAILED);
                return;
            }
            else
            {
                getSendBuffer().pull(sentSize);
            }
        }
    }

    // Allow for proto HTTP to do its thing.
    {
        BLAZE_SDK_SCOPE_TIMER("HttpConnection_httpUpdate");
        ProtoHttpUpdate(mProtoHttpRef);
    }


    BLAZE_SDK_SCOPE_TIMER("HttpConnection_httpRecv");
    // poll for completion
    int32_t recvStatus = ProtoHttpRecvAll(mProtoHttpRef, (char8_t*)getReceiveBuffer().head(), (int32_t)getReceiveBuffer().capacity());
    // recvStatus can be 0 when iRecvResult = PROTOHTTP_RECVDONE inside ProtoHttpRecvAll and when PROTOHTTP_RECVDONE is completed we need to finish the request
    if (recvStatus >= 0) 
    {
        // did the operation succeed?
        int32_t httpErrorCode = ProtoHttpStatus(mProtoHttpRef, 'code', nullptr, 0);
        int32_t bodySize = ProtoHttpStatus(mProtoHttpRef, 'body', nullptr, 0);

        // set the buffer to have written the body size.
        getReceiveBuffer().put(bodySize-getReceiveBuffer().datasize());

        // get header size.
        int32_t headerSize = ProtoHttpStatus(mProtoHttpRef, 'head', nullptr, 0);

        // get the receive headers
        HttpHeaderMap headerMap(MEM_GROUP_FRAMEWORK_TEMP, "HttpConnection::idle::headerMap");
        if (headerSize > 0)
        {
            RawBuffer headerBuffer(headerSize);
            ProtoHttpStatus(mProtoHttpRef, 'htxt', (char *)headerBuffer.head(), headerSize);
            headerBuffer.put(headerSize);
            processReceiveHeader((const char8_t *)headerBuffer.head(), headerSize, headerMap);
        }

#ifdef EA_PLATFORM_XBOXONE
        // Xbox One is unable to retrieve content-length header if the the body is gzip'd.  
        // we can assume the body size is correct, and force feed the content length into
        // our header map.
        HttpHeaderMap::const_iterator contentLengthIter = headerMap.find("Content-Length");
        if (contentLengthIter == headerMap.end())
        {
            const uint16_t BUF_BODY = 16;
            char8_t bodySizeBuf[BUF_BODY];
            blaze_snzprintf(bodySizeBuf, BUF_BODY, "%d", bodySize);
            headerMap["Content-Length"] = bodySizeBuf;
        }
#endif

        BlazeError httpError = PROTOHTTP_GetResponseClass(httpErrorCode) == PROTOHTTP_RESPONSE_SUCCESSFUL ? ERR_OK : ERR_SYSTEM;

        mRestDecoder->decodeRestResponse(true, (httpError != ERR_OK), httpErrorCode);
        mRestDecoder->setRestResourceInfo(mRequestData.restInfo);
        mRestDecoder->setHttpResponseHeaders(&headerMap);

        BlazeError rpcError = ERR_OK;
        if (httpError != ERR_OK)
            rpcError = parseResponseRpcError(headerMap);

        if (rpcError == ERR_OK)
        {
            // if still haven't flagged an error, we special check for Blaze Server WAL
            // return response, which is currently hardcoded on the server
            rpcError = preParseXmlResponse(headerMap);
        }

        BlazeSender::MessageType msgType = BlazeSender::REPLY;
        if (rpcError != ERR_OK)
            msgType = BlazeSender::ERROR_REPLY;

        finishRequest(msgType, rpcError);
    }
    else if ((recvStatus == PROTOHTTP_RECVFAIL) || (recvStatus == PROTOHTTP_TIMEOUT))
    {
        // The request failed. We're done.
        finishRequest(ERROR_REPLY, SDK_ERR_CONN_FAILED);
    }
    else if (recvStatus == PROTOHTTP_RECVBUFF)
    {
        // Our entire buffer has been filled with data; make sure the datasize reflects this
        getReceiveBuffer().put(getReceiveBuffer().capacity()-getReceiveBuffer().datasize());
        int32_t newSize = ProtoHttpStatus(mProtoHttpRef, 'body', nullptr, 0);
        // If this is a chunked transfer, we may not yet know the response size.
        // In that case, just double the buffer capacity.
        if (newSize <= 0)
            newSize = (int32_t)getReceiveBuffer().capacity()*2;
        if (!prepareReceiveBuffer(newSize))
        {
            // We failed to acquire enough space in the buffer, so finish the request
            finishRequest(ERROR_REPLY, SDK_ERR_NO_MEM);
        }
    }
    

}

void HttpConnection::onFetchedS2SToken(BlazeError err, const char8_t* token)
{
    getBlazeHub().getS2SManager()->removeListener(this);
    if (err == ERR_OK)
    {
        mRequestData.appendHeader.append_sprintf("%s %s\r\n", S2S_AUTH_HEADER_NAME, token);
        sendRequestFromBufferInternal();
    }
    else
    {
        finishRequest(ERROR_REPLY, SDK_ERR_S2S_FAILURE);
    }
}

void HttpConnection::createOwnedHeaderString(const RestRequestBuilder::HeaderVector& headerVector, Blaze::string& appendHeader)
{
    if (!headerVector.empty())
    {
        RestRequestBuilder::HeaderVector::const_iterator iter = headerVector.begin();
        RestRequestBuilder::HeaderVector::const_iterator end = headerVector.end();
        for ( ; iter != end; ++iter)
        {
            if (getServerConnInfo().getAutoS2S())
            {
                // If using auto S2S, skip any auto-created S2S authentication header
                // (we'll append our own S2S header instead)
                if (blaze_strncmp(S2S_AUTH_HEADER_NAME, iter->c_str(), S2S_AUTH_HEADER_NAME_LEN) == 0)
                    continue;
            }
            appendHeader += *iter;
            appendHeader += "\r\n";
        }
    }
}


void HttpConnection::constructUrl(char8_t* url, size_t urlSize, const char8_t* uri, const RestRequestBuilder::HttpParamVector& paramVector)
{
    size_t count = 0;
    url[count] = '\0';

    // Start with the address
    if (getServerConnInfo().getSecure())
    {
        blaze_strnzcat(url, EA_CRYPTO_DECODE_OBFUSCATED_STRING(OBFUSCATED_HTTP_SECURE_URL_PREFIX), urlSize);
    }
    else
    {
        blaze_strnzcat(url, EA_CRYPTO_DECODE_OBFUSCATED_STRING(OBFUSCATED_HTTP_INSECURE_URL_PREFIX), urlSize);
    }

    blaze_strnzcat(url, getServerConnInfo().getAddress(), urlSize);

    // Tack on the port
    char8_t urlPort[URI_PREFIX_SIZE];
    blaze_snzprintf(urlPort, sizeof(urlPort), ":%u", getServerConnInfo().getPort());
    blaze_strnzcat(url, urlPort, urlSize);

    // Tack on the URI information
    count = blaze_strnzcat(url, uri, urlSize);

    if (!paramVector.empty())
    {
        blaze_strnzcat(url, "?", urlSize);

        // Tack on url parameters.
        RestRequestBuilder::HttpParamVector::const_iterator iter = paramVector.begin();
        RestRequestBuilder::HttpParamVector::const_iterator end = paramVector.end();
        for (; iter != end; ++iter)
        {
            const HttpParam& param = *iter;
            ProtoHttpUrlEncodeStrParm(url, (int32_t)urlSize, "", param.name);
            blaze_strnzcat(url, "=", urlSize);
            ProtoHttpUrlEncodeStrParm(url, (int32_t)urlSize, "", param.value);
            count = blaze_strnzcat(url, "", urlSize); // This is a hack to make sure that count is set to the correct value
                                                      // Ideally, ProtoHttpUrlEncodeStrParm would return the size of the encoded param
                                                      // so that we can add that to the count

            if (iter+1 != end)
            {
                count = blaze_strnzcat(url, "&", urlSize);
            }
        }
    }

    url[count] = '\0';
}


void HttpConnection::processReceiveHeader(const char8_t* data, size_t dataSize, HttpHeaderMap& headerMap)
{
    char name[256];
    int32_t headerValueLen;

    // loop through the header while our header value size query returns a result
    while ((headerValueLen = ProtoHttpGetNextHeader(nullptr, data, name, sizeof(name), nullptr, 0, nullptr)) > 0)
    {
        Blaze::string headerName(name), headerValue;

        // size the string based on the result of the size query
        headerValue.resize(headerValueLen);
        // write to the string and move onto the next header
        ProtoHttpGetHeaderValue(nullptr, data, name, headerValue.begin(), headerValueLen, &data);
        // put it in the map.
        headerMap[headerName] = headerValue;
    }
}


BlazeError HttpConnection::parseResponseRpcError(const HttpHeaderMap &headerMap)
{
    BlazeError rpcError = ERR_SYSTEM;

    HttpHeaderMap::const_iterator iter = headerMap.find(RPC_ERROR_HEADER_NAME);
    if (iter != headerMap.end())
    {
        const Blaze::string& errorCodeValue = iter->second;
        rpcError = EA::StdC::AtoU32(errorCodeValue.c_str());
    }

    return rpcError;
}


static const char8_t* ERRORCODE_TAG = "<errorCode>";
static size_t const ERRORCODE_SIZE = sizeof("<errorCode>")-1;
BlazeError HttpConnection::preParseXmlResponse(const HttpHeaderMap &headerMap)
{
    BlazeError err = ERR_OK;
        
    HttpHeaderMap::const_iterator iter = headerMap.find("Content-Type");
    if (iter != headerMap.end())
    {
            const Blaze::string& value = iter->second;

            if ((blaze_stricmp(value.c_str(), XML_CONTENTTYPE) == 0) || (blaze_strnicmp(value.c_str(), XML_CONTENTTYPE ";", 16) == 0) ||
                (blaze_stricmp(value.c_str(), TEXT_XML_CONTENTTYPE) == 0) || (blaze_strnicmp(value.c_str(), TEXT_XML_CONTENTTYPE ";", 9) == 0 ))
            {
                // When parsing XML, we special check the buffer for the hardcoded blaze server error block, ie:
                // <error>
                //     <component>5</component>
                //     <errorCode>65541</errorCode>
                //     <errorName>REDIRECTOR_SERVER_NOT_FOUND</errorName>
                //     <serverinstanceerror></serverinstanceerror>
                // </error>

                int32_t errorCode = 0;

                char8_t* data = (char8_t*)getReceiveBuffer().data();
                char8_t* errorCodeTag = blaze_strstr(data, ERRORCODE_TAG);
                if (errorCodeTag != nullptr)
                {
                    // Get the error code
                    blaze_str2int((errorCodeTag+ERRORCODE_SIZE), &errorCode);
                    err = errorCode;
                }
            }

    }
    return err;
}

}
