/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OutboundHttpConnection

    This class defines a connection object for an outbound HTTP request.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/component/component.h"
#include "framework/connection/outboundhttpconnection.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/protocol/shared/httpparam.h"

#include "framework/connection/libcurlsocketchannel.h"
#include "framework/connection/selector.h"
#include "framework/connection/sslcontext.h"
#include "framework/connection/socketutil.h"

#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"

#include "framework/protocol/shared/tdfencoder.h"
#include "framework/protocol/shared/tdfdecoder.h"

#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/compression/zlibinflate.h"
#include "framework/metrics/outboundmetricsmanager.h"

#include "framework/util/hashutil.h"

#include "blazerpcerrors.h"
#include "framework/component/blazerpc.h"

#include "EATDF/printencoder.h"

namespace Blaze
{

uint32_t OutboundHttpConnection::mNextInstanceId = 0;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

OutboundHttpConnection::OutboundHttpConnection(OutboundHttpConnectionManager& owner)
    : mEasyHandle(nullptr),
      mCloseOnRelease(false),
      mInstanceId(mNextInstanceId++),
      mNextRequestId(0),
      mOwnerManager(owner)
{    
    mpNext = nullptr;
    mpPrev = nullptr;
}

OutboundHttpConnection::~OutboundHttpConnection()
{
    if (mEasyHandle != nullptr)
    {
        curl_easy_cleanup(mEasyHandle);
    }

    while (!mChannels.empty())
    {
        LibcurlSocketChannel& channel = mChannels.front();
        mChannels.pop_front();
        delete &channel;
    }
}

bool OutboundHttpConnection::initialize(SslContextPtr sslContext)
{
    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: creating an easy handle");

    mEasyHandle = curl_easy_init();

    if (mEasyHandle != nullptr)
    {
        curl_easy_setopt(mEasyHandle, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(mEasyHandle, CURLOPT_ERRORBUFFER, mCurlErrorBuffer);
        curl_easy_setopt(mEasyHandle, CURLOPT_DEBUGFUNCTION, debug_callback);
        curl_easy_setopt(mEasyHandle, CURLOPT_CONNECTTIMEOUT_MS, mOwnerManager.getConnectionTimeout().getMillis());
        curl_easy_setopt(mEasyHandle, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(mEasyHandle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
        const TimeValue& requestTimeout = mOwnerManager.getRequestTimeout();
        if (requestTimeout.getMicroSeconds() != 0)
        {
            curl_easy_setopt(mEasyHandle, CURLOPT_TIMEOUT_MS, requestTimeout.getMillis());
        }

        if (BLAZE_IS_LOGGING_ENABLED(Log::HTTP, Logging::TRACE))
        {
            //DEBUG3 prints curl feature and capabilities
            curl_version_info_data* curlSslVersionInfo = curl_version_info(CURLVERSION_NOW);

            BLAZE_TRACE_LOG(Log::HTTP, "curl version : " << curlSslVersionInfo->version);
            BLAZE_TRACE_LOG(Log::HTTP, "    features : " << SbFormats::HexLower(curlSslVersionInfo->features) );
            BLAZE_TRACE_LOG(Log::HTTP, "         ssl : " << ((curlSslVersionInfo->features & CURL_VERSION_SSL)?"on":"off") << " (version:" << curlSslVersionInfo->ssl_version << ")");
            BLAZE_TRACE_LOG(Log::HTTP, "        libz : " << ((curlSslVersionInfo->features & CURL_VERSION_LIBZ)?"on":"off"));
            BLAZE_TRACE_LOG(Log::HTTP, "       debug : " << ((curlSslVersionInfo->features & CURL_VERSION_DEBUG)?"on":"off"));            
            BLAZE_TRACE_LOG(Log::HTTP, "    asyncdns : " << ((curlSslVersionInfo->features & CURL_VERSION_ASYNCHDNS)?"on":"off"));
            BLAZE_TRACE_LOG(Log::HTTP, "   largefile : " << ((curlSslVersionInfo->features & CURL_VERSION_LARGEFILE)?"on":"off"));
            BLAZE_TRACE_LOG(Log::HTTP, "        conv : " << ((curlSslVersionInfo->features & CURL_VERSION_CONV)?"on":"off"));
        }
        if (mOwnerManager.isSecure())
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: set up SSL connection");

            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: using SSL version: " << mOwnerManager.getSslVersion());
            curl_easy_setopt(mEasyHandle, CURLOPT_SSLVERSION, mOwnerManager.getSslVersion());

            if (mOwnerManager.getPrivateCert() != nullptr && mOwnerManager.getPrivateCert()[0] != '\0')
            {
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: using client certificate");
                curl_easy_setopt(mEasyHandle, CURLOPT_SSL_CTX_FUNCTION, sslctx_callback);
                curl_easy_setopt(mEasyHandle, CURLOPT_SSL_CTX_DATA, this);
            }
            else
            {
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: not using client certificate");
            }

            if (sslContext != nullptr)
            {
                if (sslContext->getCaCertDir() != nullptr)
                {
                    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: using CA Cert Path: " << sslContext->getCaCertDir());
                    curl_easy_setopt(mEasyHandle, CURLOPT_CAPATH, sslContext->getCaCertDir());
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: failed to get ca path");
                }

                if (sslContext->getCaFile() != nullptr)
                {
                    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: using CA bundle file: " << sslContext->getCaFile());
                    curl_easy_setopt(mEasyHandle, CURLOPT_CAINFO, sslContext->getCaFile());
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: failed to get CA bundle file("<<sslContext->getCaFile()<<")");
                }
            }

            if(!mOwnerManager.isSecureVerifyPeer())
            { 
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: Secure verify peer disabled.");
                curl_easy_setopt(mEasyHandle, CURLOPT_SSL_VERIFYPEER, 0); // FALSE 
            } 
        }
        else 
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: set up NON-SSL connection");
        }

        if ((curl_easy_setopt(mEasyHandle, CURLOPT_HEADERFUNCTION, recv_header_callback) == CURLE_OK) &&
            (curl_easy_setopt(mEasyHandle, CURLOPT_WRITEFUNCTION, recv_body_callback) == CURLE_OK) &&
            (curl_easy_setopt(mEasyHandle, CURLOPT_READFUNCTION, send_body_callback) == CURLE_OK) &&
            (curl_easy_setopt(mEasyHandle, CURLOPT_SEEKFUNCTION, seek_callback) == CURLE_OK) &&
            (curl_easy_setopt(mEasyHandle, CURLOPT_PRIVATE, this) == CURLE_OK))
        {
            return true;
        }
        else 
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].initialize: call to libcurl failed. Msg: " << mCurlErrorBuffer);
        }
    }

    return false;
}

void OutboundHttpConnection::setEasyOpts(CURL* easyHandle, HttpProtocolUtil::HttpMethod method,
    const char8_t* URI, const HttpParam params[], uint32_t paramsize,
    const char8_t* httpHeaders[], uint32_t headerCount,
    ContentPartList* contentList, ContentPart*& content,
    char8_t*& url, char8_t*& paramString, struct curl_slist*& headerList,
    HttpParam*& uriEncodedParams, bool xmlPayload, const char8_t* httpUserPasswd, uint16_t maxHandledUrlRedirects)
{
    size_t paramStrLength = 0;
    char8_t* paramsStr = nullptr;

    //if there are parameters, prepare parameter string of the form "param1=value1&param2=value2\0"
    if (params != nullptr && paramsize > 0)
    {
        if (xmlPayload)
        {
            content = BLAZE_NEW ContentPart(CONTENT);
            contentList->push_back(content);

            RawBuffer* buffer = BLAZE_NEW RawBuffer(HttpProtocolUtil::XML_PAYLOAD_CONTENT_MAX_LENGTH);

            OutboundHttpConnection::buildPayloadParametersInXml(*buffer, easyHandle, params, paramsize);
            content->mName = "content";
            content->mContent = blaze_strdup(reinterpret_cast<char8_t*>(buffer->head()));
            content->mContentType = HttpProtocolUtil::HTTP_POST_CONTENTTYPE_XML;
            content->mContentLength = strlen(content->mContent);
            delete buffer;
        }
        else
        {
            uriEncodedParams = BLAZE_NEW_ARRAY_MGID(HttpParam, paramsize, MEM_GROUP_FRAMEWORK_HTTP, "encParams");
            for (uint32_t i = 0; i < paramsize; i++)
            {
                uriEncodedParams[i].name = curl_easy_escape(easyHandle, params[i].name, static_cast<int>(strlen(params[i].name)));
                uriEncodedParams[i].value = curl_easy_escape(easyHandle, params[i].value, static_cast<int>(strlen(params[i].value)));

                //count the total bytes required for holding the complete parameter string
                paramStrLength += strlen(uriEncodedParams[i].name) + strlen(uriEncodedParams[i].value);
                paramStrLength++;    //for '='
                if (i < paramsize - 1)
                {
                    paramStrLength++; //for '&'
                }
            }

            paramsStr = (char8_t*) BLAZE_ALLOC_MGID(paramStrLength + 1, MEM_GROUP_FRAMEWORK_HTTP, "params"); //+1 for '\0'
            paramsStr[0] = '\0';

            for (uint32_t i = 0; i < paramsize; i++)
            {
                blaze_strnzcat(paramsStr, uriEncodedParams[i].name, paramStrLength + 1);
                blaze_strnzcat(paramsStr, "=", paramStrLength + 1);
                blaze_strnzcat(paramsStr, uriEncodedParams[i].value, paramStrLength + 1);
                if (i < paramsize - 1)
                {
                    blaze_strnzcat(paramsStr, "&", paramStrLength + 1);
                }
            }
        }
    }

    setEasyOpts(easyHandle, method, URI, httpHeaders, headerCount, contentList, url, paramString, headerList, paramsStr, paramStrLength, httpUserPasswd, maxHandledUrlRedirects);

    if (paramsStr != nullptr)
    {
        BLAZE_FREE(paramsStr);
    }
}

void OutboundHttpConnection::setEasyOpts(CURL* easyHandle, HttpProtocolUtil::HttpMethod method,
                                    const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, 
                                    ContentPartList* contentList,
                                    char8_t *&url, char8_t *&paramString, struct curl_slist*& headerList, 
                                    const char8_t* paramsStr, uint32_t paramLength, const char8_t* httpUserPasswd, uint16_t maxHandledUrlRedirects)
{
    size_t urlLength = 0;

    if (paramsStr != nullptr)
    {
        paramString = blaze_strdup(paramsStr);
    }

    //Determine Length of URL...
    //URL contains server name and URI
    urlLength = mOwnerManager.isSecure()?1:0; //for "http://" + 1 for https
    urlLength += 7;
    urlLength += strlen(mOwnerManager.getAddress().getHostname());
    if (URI != nullptr && URI[0] != '/')
    {
        urlLength += 1;
    }
    urlLength += strlen(URI);

    bool addInlineParams = false;

    // If we're using URL encoding, make room for the contentBody (only for GET/HEAD/DELETE)
    if (paramsStr != nullptr)
    {
        if (method == HttpProtocolUtil::HTTP_GET || method == HttpProtocolUtil::HTTP_HEAD || method == HttpProtocolUtil::HTTP_DELETE)
        {
            // Check if we're form-urlencoding the params
            addInlineParams = true;
        }
        else if (contentList != nullptr && !contentList->empty() && (*contentList->begin())->mContent != paramsStr)
        {
            // We have content to post in the body, so we need to in-line the params
            addInlineParams = true;
        }

        if (addInlineParams)
        {
            ++urlLength; // for '?'
            urlLength += paramLength;
        }
    }

    //Create URL buffer of length urlLength + 1
    url = (char8_t*) BLAZE_ALLOC_MGID(urlLength + 1, MEM_GROUP_FRAMEWORK_HTTP, "url"); //+1 for '\0'

    //Prepare URL
    url[0] = '\0';
    if (mOwnerManager.isSecure())
    {
        blaze_strnzcat(url, "https://", urlLength + 1);
    }
    else
    {
        blaze_strnzcat(url, "http://", urlLength + 1);
    }

    blaze_strnzcat(url, mOwnerManager.getAddress().getHostname(), urlLength + 1);
    if (URI != nullptr && URI[0] != '/')
    {
        blaze_strnzcat(url, "/", urlLength + 1);
    }
    blaze_strnzcat(url, URI, urlLength + 1);

    if (addInlineParams)
    {
        if (blaze_strstr(url,"?") == nullptr) // If the url itself does not have a query parameter, add one. 
            blaze_strnzcat(url, "?", urlLength + 1);
        else
            blaze_strnzcat(url, "&", urlLength + 1); // Add the query separator between uri's query and explicit url params. 
        
        blaze_strnzcat(url, paramsStr, urlLength + 1);
    }

    //Set URL
    curl_easy_setopt(easyHandle, CURLOPT_URL, url);

    //Set Port
    curl_easy_setopt(easyHandle, CURLOPT_PORT, ntohs(mOwnerManager.getAddress().getPort()));

    if (mOwnerManager.getProxyAddress().getHostname()[0] != '\0')
    {
        curl_easy_setopt(easyHandle, CURLOPT_PROXY, mOwnerManager.getProxyAddress().getHostname());
        curl_easy_setopt(easyHandle, CURLOPT_PROXYPORT, ntohs(mOwnerManager.getProxyAddress().getPort()));

        // Currently only plain text http proxy is supported.
        // Note that this does not mean https calls will be plain text, rather libcurl needs to send an initial
        // Http CONNECT with the target hostname to the proxy in order to establish an end-to-end TCP pathway.
        // After that is established, libcurl will then establish a TLS connection over this path and
        // the https request will flow over this secure channel.
        // If desired additional support could be added for CURLPROXY_HTTPS, in order to do that properly
        // the proxy itself will need a certificate signed by a CA that Blaze trusts.
        curl_easy_setopt(easyHandle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    }

    //Set Method
    curl_easy_setopt(easyHandle, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(easyHandle, CURLOPT_HTTPGET, 0);
    curl_easy_setopt(easyHandle, CURLOPT_POST, 0);
    curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(easyHandle, CURLOPT_NOBODY, 0);

    if (maxHandledUrlRedirects > 0)
    {
        curl_easy_setopt(easyHandle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(easyHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        curl_easy_setopt(easyHandle, CURLOPT_MAXREDIRS, maxHandledUrlRedirects);
    }

    bool addContentBody = false;

    switch (method)
    {
    case HttpProtocolUtil::HTTP_HEAD:
        curl_easy_setopt(easyHandle, CURLOPT_NOBODY, 1);
        break;
    case HttpProtocolUtil::HTTP_GET:
        curl_easy_setopt(easyHandle, CURLOPT_HTTPGET, 1);
        break;
    case HttpProtocolUtil::HTTP_PUT:
    case HttpProtocolUtil::HTTP_PATCH:
        {
            addContentBody = true;
            curl_easy_setopt(easyHandle, CURLOPT_CUSTOMREQUEST, HttpProtocolUtil::getStringFromHttpMethod(method)); //"PUT" or "PATCH"
            char8_t header[] = "Expect:"; // Disable 100-continue
            headerList = curl_slist_append(headerList, header);
            if (contentList != nullptr && !contentList->empty())
            {
                curl_easy_setopt(easyHandle, CURLOPT_UPLOAD, 1);
                curl_easy_setopt(easyHandle, CURLOPT_READDATA, *contentList->begin());
                curl_easy_setopt(easyHandle, CURLOPT_SEEKDATA, *contentList->begin());
                curl_easy_setopt(easyHandle, CURLOPT_INFILESIZE_LARGE, (*contentList->begin())->mContentLength);
            }
            else
            {
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, "");
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDSIZE, 0);
            }
            break;
        }
    case HttpProtocolUtil::HTTP_POST:
        {
            curl_easy_setopt(easyHandle, CURLOPT_POST, 1);
            char8_t header[] = "Expect:"; // Disable 100-continue
            headerList = curl_slist_append(headerList, header);
            addContentBody = true;

            uint32_t numParts = contentList != nullptr ? contentList->size() : 0;
            if (numParts > 1)
            {
                struct curl_httppost* first = nullptr;
                struct curl_httppost* last = nullptr;

                ContentPartList::const_iterator itr = contentList->begin();
                for (; itr != contentList->end(); ++itr)
                {
                    if ((*itr)->mFileName != nullptr)
                    {
                        curl_formadd(&first, &last,
                            CURLFORM_COPYNAME, (*itr)->mName,
                            CURLFORM_FILENAME, (*itr)->mFileName,
                            CURLFORM_CONTENTTYPE, (*itr)->mContentType,
                            CURLFORM_STREAM, (*itr),
                            CURLFORM_CONTENTSLENGTH, (*itr)->mContentLength,
                            CURLFORM_END);
                    }
                    else
                    {
                        curl_formadd(&first, &last,
                            CURLFORM_COPYNAME, (*itr)->mName,
                            CURLFORM_CONTENTTYPE, (*itr)->mContentType,
                            CURLFORM_STREAM, (*itr),
                            CURLFORM_CONTENTSLENGTH, (*itr)->mContentLength,
                            CURLFORM_END);
                    }
                }
                curl_easy_setopt(easyHandle, CURLOPT_HTTPPOST, first);
            }
            else if (numParts == 1)
            {
                curl_easy_setopt(easyHandle, CURLOPT_READDATA, *contentList->begin());
                curl_easy_setopt(easyHandle, CURLOPT_SEEKDATA, *contentList->begin());
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDSIZE_LARGE, (*contentList->begin())->mContentLength);
            }
            else
            {
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, "");
            }
            break;
        }
    case HttpProtocolUtil::HTTP_DELETE:
        {
            curl_easy_setopt(easyHandle, CURLOPT_HTTPGET, 1);
            curl_easy_setopt(easyHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
            char8_t header[] = "Expect:"; // Disable 100-continue
            headerList = curl_slist_append(headerList, header);
            if (contentList != nullptr && !contentList->empty())
            {
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, (*contentList->begin())->mContent);
                curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDSIZE_LARGE, (*contentList->begin())->mContentLength);
            }
            addContentBody = true;
            break;
        }
    default:
        break;
    }

    // Add the params to the contentBody if they weren't already inlined
    if (addContentBody && !addInlineParams && paramString != nullptr)
    {
        curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, paramString);
        curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDSIZE, paramLength);
    }

    //Set Headers
    if (httpHeaders != nullptr && headerCount > 0)
    {
        for (uint32_t i = 0; i < headerCount; i++)
        {
            headerList = curl_slist_append(headerList, httpHeaders[i]);
        }
    }

    if (!addContentBody)
    {
        if (addInlineParams)
            headerList = curl_slist_append(headerList, HttpProtocolUtil::HTTP_POST_CONTENTTYPE);
    }
    else
    {
        if (contentList != nullptr && !contentList->empty())
        {
            ContentPart* content = *contentList->begin();
            if (content->mContentType != nullptr && content->mContentType[0] != '\0')
            {
                if (blaze_strncmp(content->mContentType, HttpProtocolUtil::HTTP_POST_CONTENTTYPE_XML, strlen(content->mContentType)) == 0)
                {
                    headerList = curl_slist_append(headerList, HttpProtocolUtil::HTTP_HEADER_KEEP_ALIVE);
                }
                eastl::string contentType;
                const char8_t* contentTypePrefix = "Content-Type";
                if (blaze_strstr(content->mContentType, contentTypePrefix) == nullptr)
                    contentType.sprintf("%s: %s", contentTypePrefix, content->mContentType);
                else
                    contentType.append(content->mContentType);

                headerList = curl_slist_append(headerList, contentType.c_str());
            }
        }
    }

    curl_easy_setopt(easyHandle, CURLOPT_HTTPHEADER, headerList);

    //Set HTTP Basic Authentication
    if (httpUserPasswd != nullptr && httpUserPasswd[0] != '\0')
    {
        curl_easy_setopt(easyHandle, CURLOPT_USERPWD, httpUserPasswd);
    }
}

BlazeRpcError OutboundHttpConnection::sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
        bool xmlPayload/* = false*/, const RpcCallOptions &options /*= RpcCallOptions()*/)
{
    if (URI == nullptr)
        return ERR_SYSTEM;

    char8_t *url = nullptr;
    char8_t *paramString = nullptr;
    struct curl_slist *headerList = nullptr;
    HttpParam *uriEncodedParams = nullptr;

    CURL *easyHandle = curl_easy_duphandle(mEasyHandle);

    ContentPartList contentList;
    ContentPart* contentPart = nullptr;

    //CAVEAT: This guy (setEasyOpts) allocates memory to url, paramString, uriEncodedParams and headerList, but does not clean up.
    //We (sendRequest) need to clean it up before we return. We do that in doSendRequest.
    setEasyOpts(easyHandle, method, URI, params, paramsize, httpHeaders, headerCount, &contentList, contentPart, url, paramString, headerList, uriEncodedParams, xmlPayload);
    BlazeRpcError rc = doSendRequest(easyHandle, method, URI, params, paramsize, httpHeaders, headerCount, result, logCategory, url, paramString, headerList, uriEncodedParams, options.timeoutOverride);

    // If this was an XML payload request, and we had parameters to encode, setEasyOpts will have allocated the contentPart for us
    if (contentPart != nullptr)
    {
        BLAZE_FREE(contentPart->mContent);
        delete contentPart;
    }
    return rc;
}

BlazeRpcError OutboundHttpConnection::sendRequest(HttpProtocolUtil::HttpMethod method,
    const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
    const char8_t* contentType, const char8_t* contentBody, uint32_t contentLength,
    int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/, const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    ContentPartList contentList;
    ContentPart content(CONTENT);
    content.mContent = contentBody;
    content.mContentType = contentType;
    content.mContentLength = contentLength;
    content.mName = "content";

    contentList.push_back(&content);

    return sendRequest(method, URI, httpHeaders, headerCount, result, &contentList, logCategory);
}

BlazeRpcError OutboundHttpConnection::sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        ContentPartList* contentList, int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/, const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    if (URI == nullptr)
        return ERR_SYSTEM;

    char8_t *url = nullptr;
    char8_t *paramString = nullptr;
    struct curl_slist *headerList = nullptr;
    HttpParam *uriEncodedParams = nullptr;

    CURL *easyHandle = curl_easy_duphandle(mEasyHandle);

    //CAVEAT: This guy (setEasyOpts) allocates memory to url, paramString, uriEncodedParams and headerList, but does not clean up.
    //We (sendRequest) need to clean it up before we return. We do that in doSendRequest.
    setEasyOpts(easyHandle, method, URI, httpHeaders, headerCount, contentList, url, paramString, headerList);
    return doSendRequest(easyHandle, method, URI, nullptr, 0, httpHeaders, headerCount, result, logCategory, url, paramString, headerList, uriEncodedParams, options.timeoutOverride);
}

BlazeRpcError OutboundHttpConnection::sendRequest(HttpProtocolUtil::HttpMethod method,
    const char8_t* URI, const HttpParam params[], uint32_t paramsize,
    const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
    ContentPartList* contentList, int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
    bool xmlPayload/* = false*/, const char8_t* httpUserPwd/* = nullptr*/, 
    uint16_t maxHandledUrlRedirects/* = 0*/, const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    if (URI == nullptr)
        return ERR_SYSTEM;

    char8_t *url = nullptr;
    char8_t *paramString = nullptr;
    struct curl_slist *headerList = nullptr;
    HttpParam *uriEncodedParams = nullptr;

    CURL *easyHandle = curl_easy_duphandle(mEasyHandle);

    // Caller may pass in a list of their own, if not we need to use a locally created one
    ContentPartList contentListLocal;
    if (contentList == nullptr)
        contentList = &contentListLocal;
    ContentPart* contentPart = nullptr;

    //CAVEAT: This guy (setEasyOpts) allocates memory to url, paramString, uriEncodedParams and headerList, but does not clean up.
    //We (sendRequest) need to clean it up before we return. We do that in doSendRequest.
    setEasyOpts(easyHandle, method, URI, params, paramsize, httpHeaders, headerCount, contentList, contentPart, url, paramString, headerList, uriEncodedParams, xmlPayload, httpUserPwd, maxHandledUrlRedirects);
    BlazeRpcError rc = doSendRequest(easyHandle, method, URI, params, paramsize, httpHeaders, headerCount, result, logCategory, url, paramString, headerList, uriEncodedParams, options.timeoutOverride);

    // If this was an XML payload request with params, setEasyOpts will have allocated us a contentPart - it will also have stuck said part
    // into the back of the list.  Make sure we free this part and strip it out of the list before returning to our caller.
    if (contentPart != nullptr)
    {
        contentList->pop_back();
        BLAZE_FREE(contentPart->mContent);
        delete contentPart;
    }
    return rc;
}

BlazeRpcError OutboundHttpConnection::sendStreamRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler,
        ContentPartList* contentList, int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/, const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    if (URI == nullptr)
        return ERR_SYSTEM;

    char8_t *url = nullptr;
    char8_t *paramString = nullptr;
    struct curl_slist *headerList = nullptr;
    HttpParam *uriEncodedParams = nullptr;

    CURL *easyHandle = curl_easy_duphandle(mEasyHandle);

    //CAVEAT: This guy (setEasyOpts) allocates memory to url, paramString, uriEncodedParams and headerList, but does not clean up.
    //We (sendRequest) need to clean it up before we return. We do that in doSendRequest.
    setEasyOpts(easyHandle, method, URI, httpHeaders, headerCount, contentList, url, paramString, headerList);
    return doSendRequest(easyHandle, method, URI, nullptr, 0, httpHeaders, headerCount, handler, logCategory, url, paramString, headerList, uriEncodedParams, options.timeoutOverride);
}

BlazeRpcError OutboundHttpConnection::doSendRequest(CURL *easyHandle, HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize,
                                                    const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler, int32_t logCategory,
                                                    char8_t *url, char8_t *paramString, struct curl_slist *headerList, HttpParam *uriEncodedParams, const TimeValue &timeoutOverride /* = 0*/)
{
    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].sendRequest: Adding easy handle (" << easyHandle << ") to multi handle (" << mOwnerManager.mMultiHandle << ")");
    curl_multi_add_handle(mOwnerManager.mMultiHandle, easyHandle);

    TimeValue sendRequestTime = TimeValue::getTimeOfDay();
    
    //Prepare the request tracker.
    OutgoingRequest requestInfo;
    requestInfo.logContext = *gLogContext;
    requestInfo.curlResult = CURLE_OK;
    requestInfo.easyHandle = easyHandle;
    requestInfo.handler = handler;
    requestInfo.requestId = mNextRequestId++;
    requestInfo.logCategory = logCategory;
    resetStatusCode(requestInfo);
    mSentRequests.push_back(requestInfo);

    if (requestInfo.handler != nullptr)
        requestInfo.handler->setOwnerConnection(this);

    curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, &requestInfo);
    curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, &requestInfo);
    curl_easy_setopt(easyHandle, CURLOPT_DEBUGDATA, &requestInfo);
    if (timeoutOverride > 0)
    {
        // request provided an override timeout value, set it in curl options.
        // give each one the full timeout, the fiber getAndWait will end things if they take too long combined
        curl_easy_setopt(easyHandle, CURLOPT_CONNECTTIMEOUT_MS, timeoutOverride.getMillis());
        curl_easy_setopt(easyHandle, CURLOPT_TIMEOUT_MS, timeoutOverride.getMillis());
    }

    //Now we go to sleep - when we wake up result will have been filled out.
    BlazeRpcError resultErr = Fiber::getAndWait(requestInfo.eventHandle, "OutboundHttpConnection::sendRequest", timeoutOverride > 0 ? timeoutOverride + TimeValue::getTimeOfDay() : 0);

    // hook up to report metrics.  make sure we scrub the URL for sensitive info that we know are being sent by our core Authentication component to Nucleus 1.0
    eastl::string strHttpRequestLog;
    HttpProtocolUtil::printHttpRequest(mOwnerManager.isSecure(), method, mOwnerManager.getAddress().getHostname(), mOwnerManager.getAddress().getPort(InetAddress::HOST),
        URI, params, paramsize, httpHeaders, headerCount, strHttpRequestLog);
    const char8_t* resource = strHttpRequestLog.c_str() + strlen(HttpProtocolUtil::getStringFromHttpMethod(method)) + 1; //skip over the method and space

    if (gOutboundMetricsManager != nullptr)
        gOutboundMetricsManager->tickAndCheckThreshold(HTTP, resource, TimeValue::getTimeOfDay() - sendRequestTime);

    // In order to guard against spurious fiber wakes (GOS-5538) we always try to remove OutgoingRequest
    // since it points to stack variables that _will_ become invalid after this stack frame is popped.    
    if (requestInfo.mpNext != nullptr && requestInfo.mpPrev != nullptr) 
    {
        mSentRequests.remove(requestInfo); 
    }

    //Check to see if the fiber was iterrupted, or error in curl
    if ((resultErr != Blaze::ERR_OK) || (requestInfo.curlResult != CURLE_OK))
    {
        char8_t strSendRequestTime[32];
        sendRequestTime.toString(strSendRequestTime, sizeof(strSendRequestTime));
        BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnection].sendRequest: "
            "(server: " << mOwnerManager.getAddress().getHostname() << ") "
            "failed with sleepError[" << ErrorHelp::getErrorName(resultErr) << "], curl result[" << requestInfo.curlResult
            << "] for request sent at " << strSendRequestTime << "\n" << strHttpRequestLog);
        // close the socket if any error to avoid stale data sitting on socket 
        // and to ensure that the connection state is reset
        mCloseOnRelease = true;

        if (requestInfo.handler != nullptr)
        {
            requestInfo.handler->setHttpError((resultErr == ERR_OK) ? ERR_SYSTEM : resultErr);
        }
    }

    switch (requestInfo.curlResult)
    {
    // If it is a recv error (libcurl error code 56), retry the send request after the connection is re-established
    case CURLE_RECV_ERROR:              resultErr = ERR_DISCONNECTED;           break;
    // If it is a timeout error (libcurl error code 28) return ERR_TIMEOUT. Calling code will handle (retry etc) as required
    case CURLE_OPERATION_TIMEDOUT:      resultErr = ERR_TIMEOUT;                break;
    case CURLE_COULDNT_CONNECT:         resultErr = ERR_COULDNT_CONNECT;        break;
    case CURLE_COULDNT_RESOLVE_HOST:    resultErr = ERR_COULDNT_RESOLVE_HOST;   break;
    default: break;
    }

    curl_easy_setopt(easyHandle, CURLOPT_DEBUGDATA, nullptr);
    curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, nullptr);

    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnection].sendRequest: Removing easy handle (" << easyHandle << ") from multi handle (" << mOwnerManager.mMultiHandle << ")");
    curl_multi_remove_handle(mOwnerManager.mMultiHandle, easyHandle);

    BLAZE_FREE(url);

    if (paramString != nullptr)
    {
        BLAZE_FREE(paramString);
    }

    curl_slist_free_all(headerList);

    if (uriEncodedParams != nullptr)
    {
        for (uint32_t i = 0; i < paramsize; i++)
        {
            if (uriEncodedParams[i].name != nullptr)
            {
                curl_free((void *)(uriEncodedParams[i].name));
            }

            if (uriEncodedParams[i].value != nullptr)
            {
                curl_free((void *)(uriEncodedParams[i].value));
            }
        }
    }

    delete [] uriEncodedParams;

    curl_easy_cleanup(easyHandle);

    return resultErr;
}

const char8_t* OutboundHttpConnection::findJSONValueStart(const char8_t* keyStart, const char8_t* expectedKey, size_t expectedKeyLen)
{
    const char8_t* valueStart = keyStart;
    
    if (expectedKey != nullptr && expectedKeyLen > 0)
    {
        if (*keyStart != '\"' || blaze_strncmp(keyStart+1, expectedKey, expectedKeyLen) != 0 || *(keyStart+1+expectedKeyLen) != '\"')
            return nullptr;

        valueStart += expectedKeyLen + 2; // expected key length + opening and closing '"'
    }
    else
    {
        // We don't care what the key is; just skip past it
        do 
        {
            ++valueStart;
        } while (*valueStart != '\0' && (*valueStart != '"' || *(valueStart-1) == '\\'));

        if (*valueStart == '"')
            ++valueStart;
    }

    valueStart = strchr(valueStart, ':');
    if (valueStart == nullptr)
        return nullptr;

    do 
    {
        ++valueStart;
    } while (*valueStart != '\0' && isspace(*valueStart));

    if (*valueStart == '\0')
        return nullptr;

    return valueStart;
}

const char8_t* OutboundHttpConnection::findJSONValueEnd(const char8_t* valueStart)
{
    // Skip over JSON objects and arrays
    if (*valueStart == '[' || *valueStart == '{')
        return nullptr;

    bool isQuoted = (*valueStart == '"');
    const char8_t* valueEnd = valueStart + 1;
    while (*valueEnd != 0)
    {
        if (isQuoted)
        {
            if (*valueEnd == '"' && *(valueEnd - 1) != '\\')
                return valueEnd + 1;
        }
        else
        {
            if (isspace(*valueEnd) || *valueEnd == ',' || *valueEnd == '}')
                return valueEnd;
        }
        ++valueEnd;
    }

    BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnection].findJSONValueEnd: While scrubbing curl debug log, unexpected JSON end of input: expected " << (isQuoted ? "'\"'" : "whitespace, '}', or ','"));
    return valueEnd;
}

const char8_t* OutboundHttpConnection::findFirstFiltered(const char8_t* data, uint32_t& filterIndex, uint32_t& tagLength)
{
    for (const char8_t* p = data; *p != '\0'; ++p)
    {
        uint32_t i = 0;
        while (HttpProtocolUtil::HTTP_FILTERED_FIELDS[i] != nullptr)
        {
            if (blaze_strncmp(p, HttpProtocolUtil::HTTP_FILTERED_FIELDS[i], HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]) == 0)
            {
                // Look for the whole word match only
                if (p[HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]] == '=' || p[HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]] == '>')
                {
                    filterIndex = i;
                    tagLength = HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i] + 1;
                    return p;
                }
                else //it might be a JSON
                {
                   // Locate the opening quote for JSON
                   if (p[HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]] == '"')
                   {
                       const char8_t* s = findJSONValueStart(p-1, HttpProtocolUtil::HTTP_FILTERED_FIELDS[i], HttpProtocolUtil::HTTP_FILTERED_FIELD_LENS[i]);
                       if (s != nullptr)
                       {
                           filterIndex = i;
                           tagLength = static_cast<uint32_t>(s - p);
                           return p;
                       }
                   }
                }
            }
            ++i;
        }
    }

    return nullptr;
}

bool OutboundHttpConnection::writeStringToBuffer(RawBuffer& buf, const char8_t* data, size_t dataLen)
{
    uint8_t* tail = buf.acquire(dataLen);
    if (tail == nullptr)
    {
        BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnection].debug_callback: Failed to allocate buffer for scrubbed curl debug log");
        return false;
    }
    memcpy(tail, data, dataLen);
    buf.put(dataLen);
    return true;
}

// Make sure we don't print out any personally identifiable information or passwords
// that may be contained in HTTP url parameters or JSON/XML payloads
const char8_t* OutboundHttpConnection::scrubUrlAndPayload(RawBuffer& buf, const char8_t *data, uint32_t dataSize)
{
    // First, find the start, scrub, and end locations of the tags we need to filter
    bool success = true;
    bool found = false;
    const char8_t* curLoc = data;
    for (uint32_t i = 0; i <= HttpProtocolUtil::HTTP_LAST_SCRUBBED_FIELD_IDX; ++i)
    {
        uint32_t filterIndex = 0;
        const char8_t* loc = nullptr;
        const char8_t* scrubLoc = nullptr;
        const char8_t* endLoc = nullptr;
        uint32_t tagLength = 0;

        loc = findFirstFiltered(curLoc, filterIndex, tagLength);

        if (loc != nullptr)
        {
            found = true;
            char8_t endXmlFilter[256] = "";

            // It might be a JSON, look for the opening JSON quote
            if (loc != data && *(loc - 1) == '"')
            {
                  scrubLoc = loc + tagLength;
                  endLoc = findJSONValueEnd(scrubLoc);

                  if (endLoc == nullptr)
                  {
                      // This was a JSON object or array. None of the fields we're targeting with our current filters should be objects or arrays,
                      // so just ignore it (don't scrub or censor)
                      success = writeStringToBuffer(buf, curLoc, scrubLoc - curLoc);
                      if (!success)
                          break;
                      curLoc = scrubLoc;
                      continue;
                  }
            }
            else
            {
               // This is an XML tag
               if (loc != data && *(loc - 1) == '<')
               {
                     blaze_snzprintf(endXmlFilter, sizeof(endXmlFilter), "</%s>", HttpProtocolUtil::HTTP_FILTERED_FIELDS[filterIndex]);
                     endLoc = blaze_strstr(loc, endXmlFilter);
               }
               else
               {
                  endLoc = ::strpbrk(loc, "& ");
                  if (endLoc == nullptr)
                  {
                     // We've hit the end of the data, so set endLoc to the end
                     endLoc = strchr(loc, 0);
                  }               
               }

               // We use strlen(filter) because the filter found may or may not be the same as filters[i]
               scrubLoc = loc + tagLength; //start + length of filter + '>' or '='
            }

            success = writeStringToBuffer(buf, curLoc, scrubLoc - curLoc);
            if (!success)
                break;

            // Decide whether to scrub or censor
            if (filterIndex > HttpProtocolUtil::HTTP_LAST_CENSORED_FIELD_IDX)
            {
                char8_t scrubbed[HashUtil::SHA256_STRING_OUT] = "";
                scrub(scrubLoc, endLoc, scrubbed, HashUtil::SHA256_STRING_OUT);
                success = writeStringToBuffer(buf, scrubbed, strlen(scrubbed));
            }
            else
            {
                success = writeStringToBuffer(buf, HttpProtocolUtil::CENSORED_STR, HttpProtocolUtil::CENSORED_STR_LEN);
            }

            if (!success)
                break;

            if (endLoc == nullptr)
            {
                curLoc = scrubLoc;
            }
            else
            {
                curLoc = endLoc;
                if (*endXmlFilter != '\0')
                {
                    success = writeStringToBuffer(buf, endXmlFilter, strlen(endXmlFilter));
                    if (!success)
                        break;
                    curLoc += strlen(endXmlFilter);
                }
            }
        }
        else
        {
            // We failed to find any of the filters
            break;
        }
    }

    if (!found)
        return nullptr;

    // Copy the rest of the data to the buffer (including the terminating nullptr character)
    if (success)
        writeStringToBuffer(buf, curLoc, strlen(curLoc)+1);

    return reinterpret_cast<const char8_t*>(buf.data());
}

const char8_t* OutboundHttpConnection::censorHeaders(RawBuffer& buf, const char8_t* data, uint32_t dataSize)
{
    bool found = false;
    bool success = true;

    // Skip the HTTP method line, if it's there
    // (assume the first line is a header if a colon appears before the first whitespace character -
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2 indicates there should not be whitespace until after the colon)
    const char8_t* startLoc = data;
    while (*startLoc != '\0' && !isspace(*startLoc) && *startLoc != ':')
        ++startLoc;

    if (*startLoc == '\0')
        return nullptr;

    if (*startLoc == ':')
        startLoc = data;
    else
    {
        // https://www.w3.org/Protocols/rfc2616/rfc2616-sec19.html#sec19.3 suggests recognizing a single LF as a line terminator
        startLoc = strchr(startLoc, '\n');
        if (startLoc == nullptr)
            return nullptr;
        ++startLoc;
    }

    const char8_t* curLoc = nullptr;
    while (success && *startLoc != '\0')
    {
        // Check if we've reached the end of the header block
        if (*startLoc == '\n' || *(startLoc + 1) == '\n')
            break;

        // Find the start of the header value
        curLoc = strchr(startLoc, ':');
        if (curLoc == nullptr)
            break;

        // Check if the header name matches one of our filters
        uint32_t filterIndex = 0;
        while (HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex] != nullptr)
        {
            if (blaze_strncmp(startLoc, HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex], curLoc-startLoc) == 0)
                break;
            ++filterIndex;
        }

        if (HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex] != nullptr)
        {
            if (!found)
            {
                // We've found our first match; write all the data we've read so far to the buffer (up to the start of the header name)
                success = writeStringToBuffer(buf, data, startLoc - data);
                found = true;
            }

            // Write the header name to the buffer (add +2 to include the colon and the additional space after it)
            if (success)
                success = writeStringToBuffer(buf, HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex], curLoc - startLoc +2);

            // Censor the header value
            if (success)
                success = writeStringToBuffer(buf, HttpProtocolUtil::CENSORED_STR, HttpProtocolUtil::CENSORED_STR_LEN);

            if (!success)
                break;
        }

        // Find the start of the next header
        curLoc = strchr(curLoc, '\n');
        if (curLoc == nullptr)
        {
            if (!found)
                break;

            if (HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex] != nullptr)
            {
                // We censored the last header; make sure we don't write it to the buffer
                startLoc = strchr(startLoc, 0);
            }
            break;
        }


        if (found)
        {
            if (HttpProtocolUtil::HTTP_CENSORED_HEADERS[filterIndex] == nullptr)
            {
                // We didn't censor this header; write it to the buffer
                success = writeStringToBuffer(buf, startLoc, curLoc - startLoc);
            }
            else
            {
                // We censored this header; write just the line terminator to the buffer
                success = writeStringToBuffer(buf, "\r\n", 2);
            }
        }

        startLoc = curLoc+1;
    }

    if (!found)
        return nullptr;

    // Copy the rest of the data to the buffer (including the terminating nullptr character)
    if (success)
        writeStringToBuffer(buf, startLoc, strlen(startLoc) + 1);

    return reinterpret_cast<const char8_t*>(buf.data());

}

const char8_t* OutboundHttpConnection::censorVaultData(RawBuffer& buf, const char8_t* data, uint32_t dataSize)
{
    eastl::string mapKey = "\"data\"";
    bool success = false;

    // Find the JSON map
    const char8_t* startLoc = data;
    const char8_t* curLoc = nullptr;
    while (*startLoc != '\0')
    {
        curLoc = blaze_strstr(startLoc, mapKey.c_str());
        if (curLoc == nullptr)
            return nullptr;

        startLoc = curLoc;
        curLoc = findJSONValueStart(startLoc, nullptr, 0);

        if (curLoc != nullptr && *curLoc == '{')
            break;

        startLoc += mapKey.length();
    }

    // Nothing to censor; we didn't find a 'data' map
    if (*startLoc == '\0' || curLoc == nullptr)
        return nullptr;

    // Write data to the buffer, up to the 'data' map content
    success = writeStringToBuffer(buf, data, curLoc - data);

    // Censor the map values, but not the keys.
    startLoc = curLoc;
    ++curLoc;
    while (success && *startLoc != '\0' && *startLoc != '}')
    {
        // Find the start of the next map key
        while (*curLoc != '\0' && *curLoc != '"' && *curLoc != '}')
            ++curLoc;

        if (*curLoc == '\0' || *curLoc == '}')
            break;

        // Find the start of the map value
        curLoc = findJSONValueStart(curLoc, nullptr, 0);
        if (curLoc == nullptr)
            break;

        // Write data to the buffer, up to the start of the map value
        success = writeStringToBuffer(buf, startLoc, curLoc - startLoc);
        if (!success)
            break;

        // Find the end of the map value
        startLoc = curLoc;
        curLoc = findJSONValueEnd(curLoc);

        // HashiCorp Vault 0.9.2+ does allow JSON objects and arrays to be written to Vault, but
        // we should not encounter this; our SecretVault proxy component is only able to parse secrets of type 'string'.
        if (curLoc == nullptr)
        {
            success = false;
            eastl::string errString;
            errString.append_sprintf("%c <unexpected JSON object or array; censoring remaining data>", *startLoc);
            writeStringToBuffer(buf, errString.c_str(), errString.length());
            break;
        }

        // Censor the map value
        success = writeStringToBuffer(buf, HttpProtocolUtil::CENSORED_STR, HttpProtocolUtil::CENSORED_STR_LEN);

        startLoc = curLoc;
    }

    // Copy the rest of the data to the buffer (including the terminating nullptr character)
    if (success)
        writeStringToBuffer(buf, startLoc, strlen(startLoc)+1);

    return reinterpret_cast<const char8_t*>(buf.data());
}

void OutboundHttpConnection::scrub(const char8_t* scrubLoc, const char8_t* endLoc, char8_t* scrubbed, size_t scrubbedLen)
{
    if (endLoc != nullptr && scrubLoc != endLoc)
    {
        HashUtil::generateSHA256Hash(scrubLoc, static_cast<size_t>(endLoc - scrubLoc), scrubbed, scrubbedLen);
    }
}

void OutboundHttpConnection::xmlEscapeString(const char8_t* sourceStr, size_t sourceStrSize, eastl::string& escapedStr)
{
    escapedStr.reserve(sourceStrSize * 2); // Reserve 2x source string length
    for (size_t i = 0; i < sourceStrSize; ++i)
    {
        char8_t c = sourceStr[i];
        switch (c)
        {
        case '&': escapedStr.append("&amp;"); break;
        case '<': escapedStr.append("&lt;"); break;
        case '>': escapedStr.append("&gt;"); break;
        case '"': escapedStr.append("&quot;"); break;
        case '\'': escapedStr.append("&apos;"); break;

        default:
            if (c < 32 || c > 127)
            {
                escapedStr.append_sprintf("&#%c;", c);
            }
            else
            {
                escapedStr.append_sprintf("%c", c);
            }
        }
    }
}

void OutboundHttpConnection::resetStatusCode(OutgoingRequest& request)
{
    request.statusCode = 0;
    request.statusCodeReceived = false;
    request.handler->setHttpStatusCode(0);
}

void OutboundHttpConnection::processMessage(CURLcode res)
{
    if (!mSentRequests.empty())
    {
        //Pop off the waiting request and wake it up
        OutgoingRequest &request = mSentRequests.front();
        mSentRequests.pop_front();

        //We want our log to show up with the right context, so we pull it from the request
        LogContextWrapper logContext = *gLogContext;
        *gLogContext = request.logContext;

        //Clear the request list vars.
        request.mpNext = request.mpPrev = nullptr;
        request.curlResult = res;

        BlazeRpcError result = request.handler->requestComplete(res);

        if (res != CURLE_OK)
        {
            BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnection].processMessage: "
                "(server: " << mOwnerManager.getAddress().getHostname() << ") "
                "Returned CURL error code " << res << " (" << curl_easy_strerror(res) << ")");
            mCloseOnRelease = true;
            result = ERR_SYSTEM;
        }

        if (result != ERR_OK)
        {
            request.handler->setHttpError(result);
        }

        //reset the log context to what it was before
        *gLogContext = logContext;
        //delay signaling the event since we're in the middle of OutboundHttpConnectionManager::handleCompletedRequests
        //and we don't want OutboundHttpConnectionManager to be deleted out from under us (in the case of authentication reconfiguration).
        Fiber::delaySignal(request.eventHandle, ERR_OK);
    }
}

void OutboundHttpConnection::close()
{
    // It has done its job, wait for next one
    mCloseOnRelease = false;

    while (!mSentRequests.empty())
    {
        //Pop off the waiting request and wake it up
        OutgoingRequest &request = mSentRequests.front();
        mSentRequests.pop_front();
        request.mpNext = request.mpPrev = nullptr;
        Fiber::signal(request.eventHandle, ERR_DISCONNECTED);
    }
}

size_t OutboundHttpConnection::recv_header_callback(const void *ptr, size_t size, size_t nmemb, void *requestData)
{
    OutgoingRequest *request = reinterpret_cast<OutgoingRequest *>(requestData);

    //Note: The total number of bytes read in equals (nmemb x size)
    size_t bytes = size * nmemb;
    const char8_t* data = reinterpret_cast<const char8_t*>(ptr);

    // The last two characters in all header lines should always given to this function should
    // always end with \r\n.
    if ((bytes < 2) || (data[bytes - 2] != '\r') || (data[bytes - 1] != '\n'))
    {
        // This should never happen, but if it does, something not right so return 0 to end the request
        return 0;
    }

    // We've already guaranteed the last two characters in this line are \r\n, therefore, if the
    // number of bytes in this line is 2 then this is a termination line (e.g. end of HTTP headers)
    bool termination = (bytes == 2);

    long redirects = 0;;
    curl_easy_getinfo(request->easyHandle, CURLINFO_REDIRECT_COUNT, &redirects);
    if ((request->statusCode >= 301 && request->statusCode <= 308 && request->statusCode != 304) &&
         redirects > 0 &&
         request->statusCodeReceived)
    {
        // We've been redirected, so we need to reset our status code so that the automatic
        // redirect request will process the subsequent HTTP status code correctly
        resetStatusCode(*request);
    }

    // If we don't already know the status code, then this must be the HTTP Response Status line (e.g HTTP 200 OK)
    if (request->statusCode == 0)
    {
        // Get the status code from Curl
        // Note that curl_easy_getinfo() requires a long* when passing CURLINFO_RESPONSE_CODE.
        // We use a local long variable here because request->statusCode is an int32_t, but on Unix,
        // a long is 64bits.  Therefore, passing &request->statusCode would stomp 32bits on Unix platforms.
        long sc = 0; 
        curl_easy_getinfo(request->easyHandle, CURLINFO_RESPONSE_CODE, &sc);

        request->statusCode = (int32_t)sc;
        request->statusCodeReceived = true;
    }
    else if (request->statusCode == 100)
    {
        // If we're currently traversing HTTP headers for an HTTP 100 Continue response,
        // then we can just skip over it.  Unless it's a termination line, then we reset our status code
        // so that the next set of HTTP response will be read.
        if (termination)
        {
            resetStatusCode(*request);
        }
    }
    else // We're collecting headers for a normal response
    {
        // If this isn't the termination line, then it's an HTTP header that we should store
        if (!termination)
        {
            const char8_t* delim = data;
            while (*delim != ':')
            {
                // We know the line ends with \r\n, so if for some reason we hit the '\r' before finding ':' then
                // something got messed up, so just return 0 ending the request/response.
                if (*delim == '\r')
                    return 0;
                ++delim;
            }

            // Grab the HTTP header name
            eastl::string headerName(data, (size_t)(delim - data));

            // Skip the ':' character
            ++delim;

            // Skip any white-space to find the begining of the HTTP header value, or the end of the line (e.g. '\r')
            while (*delim == ' ')
            {
                ++delim;
            }

            // The HTTP header value is everything from delim to the end of the line, minus the \r\n.
            eastl::string headerValue(delim, (size_t)(bytes - (delim - data) - 2));

            // Put it in the map.
            request->handler->mHeaderMap[headerName] = headerValue;
        }
        else if (request->handler->getHttpStatusCode() == 0)
        {
            request->handler->setHttpStatusCode(request->statusCode);

            request->handler->getOwnerConnection()->mCloseOnRelease =
                (blaze_stricmp(request->handler->getHeader("Connection"), "close") == 0);

            request->handler->processHeaders();
        }
        /*
          The else case here would be if the response is Chunked Encoding, and we've hit the last chunk.
          Curl will still call this callback function (for some reason) even though we're not dealing with
          a HTTP header.  It's just the fact that it is a \r\n.
        */
    }

    return bytes;
}

size_t OutboundHttpConnection::recv_body_callback(const void *ptr, size_t size, size_t nmemb, void *requestData)
{
    OutgoingRequest *request = reinterpret_cast<OutgoingRequest *>(requestData);

    //Note: The total number of bytes read in equals (nmemb x size)
    size_t bytes = size * nmemb;
    const char8_t* data = reinterpret_cast<const char8_t*>(ptr);

    if (request->handler != nullptr)
    {
        return request->handler->processRecvData(data, bytes);
    }

    return 0;
}

size_t OutboundHttpConnection::send_body_callback(void *ptr, size_t size, size_t nmemb, void *requestData)
{
    ContentPart *request = reinterpret_cast<ContentPart *>(requestData);

    if (request->mContentLength <= request->mWriteOffset)
        return 0;

    //Note: The total number of bytes read in equals (nmemb x size)
    size_t bytes = size * nmemb;
    size_t toCopy = (bytes < (request->mContentLength - request->mWriteOffset) ? bytes : (request->mContentLength - request->mWriteOffset));
    memcpy(ptr, request->mContent + request->mWriteOffset, toCopy);

    request->mWriteOffset += static_cast<uint32_t>(toCopy);

    return toCopy;
}

CURL_DEBUG_SCRUB_TYPE OutboundHttpResponseHandler::getDefaultScrubType(curl_infotype infoType)
{
    switch (infoType)
    {
    case CURLINFO_HEADER_IN:
    case CURLINFO_HEADER_OUT:
        return HEADER;
    case CURLINFO_SSL_DATA_IN:
    case CURLINFO_SSL_DATA_OUT:
        return CENSOR_ALL;
    default:
        return DEFAULT;
    }
}

int OutboundHttpConnection::debug_callback(const CURL* easy_handle, curl_infotype infoType, char *data, size_t size, void * ptr)
{
    const char *strInfoType;

    OutgoingRequest* request = reinterpret_cast<OutgoingRequest*>(ptr);

    switch (infoType)
    {
        case CURLINFO_TEXT:
            strInfoType = "== Info:";
            break;
        case CURLINFO_HEADER_IN:
            strInfoType = "<- Receive header";
            break;
        case CURLINFO_DATA_IN:
            strInfoType = "<- Receive data";
            break;
        case CURLINFO_SSL_DATA_IN:
            strInfoType = "<- Receive SSL data";
            break;
        case CURLINFO_HEADER_OUT:
            strInfoType = "-> Send header";
            break;
        case CURLINFO_DATA_OUT:
            strInfoType = "-> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            strInfoType = "-> Send SSL data";
            break;
        default:
            return 0;
    }

    if (request != nullptr && data != nullptr && BLAZE_IS_TRACE_HTTP_ENABLED(request->logCategory))
    {
        *gLogContext = request->logContext;
        CURL_DEBUG_SCRUB_TYPE scrubType = OutboundHttpResponseHandler::getDefaultScrubType(infoType);
        if (request->handler != nullptr)
            scrubType = request->handler->getDebugScrubType(infoType);

        if (scrubType == CENSOR_ALL)
        {
            BLAZE_SPAM_LOG(request->logCategory, "http: " << strInfoType << " [requestId=" << request->requestId << "] ("
                << static_cast<int>(size) << " bytes): " << HttpProtocolUtil::CENSORED_STR);
        }
        else if (scrubType == VAULT_PAYLOAD)
        {
            // See GOSREQ-2912 - note that we do not call the censorVaultData method for the time being,
            // as this callback can be executed multiple times on chunks of data in the case of large JSON
            // response payloads (which are not uncommon when fetching SSL certificates from Vault),
            // and without the full body of the JSON payload, any attempt to censor a fragment is problematic.
            // Thus, until the gosreq is addressed to improve our overall censoring strategy, we err on the side
            // of caution and censor the entire response payload of Vault calls.
            BLAZE_TRACE_HTTP_LOG(request->logCategory, "http: " << strInfoType << " [requestId=" << request->requestId << "] ("
                << static_cast<int>(size) << " bytes): " << HttpProtocolUtil::CENSORED_STR);
        }
        else
        {
            // The data pointed to by the char* passed to this function is not zero terminated,
            // but will be exactly of the size as told by the size_t argument. Therefore, we
            // replace the last character of data with null terminator character as it is a
            // newline character most of the time. This will prevent invalid read in logger.
            char lastChar = data[size];
            data[size] = '\0';

            RawBuffer rawbuf(nullptr, 0, true); // The scrubbing method will expand this buffer if needed
            const char8_t* buf = nullptr;

            switch (scrubType)
            {
            case HEADER:
                buf = censorHeaders(rawbuf, data, size);
                break;
            default:
                buf = scrubUrlAndPayload(rawbuf, data, size);
                break;
            }

            BLAZE_TRACE_HTTP_LOG(request->logCategory, "http: " << strInfoType << " [requestId=" << request->requestId << "] ("
                << (static_cast<int>(size)) << " bytes): " << (buf != nullptr ? buf : data));

            data[size] = lastChar;
        }

        gLogContext->clear();
    }

    return 0;
}

int OutboundHttpConnection::sockopt_callback(void* ptr, curl_socket_t curlfd, curlsocktype purpose)
{
    SocketUtil::setKeepAlive(curlfd, true);
    return 0;
}

int OutboundHttpConnection::seek_callback(void* ptr, curl_off_t offset, int origin)
{
    int retVal = CURL_SEEKFUNC_CANTSEEK;
    
    if (offset < 0)
        return retVal;

    ContentPart *request = reinterpret_cast<ContentPart *>(ptr);
    uint32_t offsetU = static_cast<uint32_t>(offset);    

    //libcurl currently only passes SEEK_SET, including SEEK_CUR and SEEK_END in case libcurl adds support in the future     
    switch (origin)
    {
        case SEEK_SET:
            if (offsetU <= request->mContentLength)
            {
                request->mWriteOffset = offsetU;
                retVal = CURL_SEEKFUNC_OK;
            }
            else 
            {
                retVal = CURL_SEEKFUNC_FAIL;
                BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnection].seek_callback: CURL seek failed. offset(=" << offsetU << ") is greater than content length(=" << request->mContentLength << ").");
            }
            break;
        
        case SEEK_CUR:
            if ((request->mWriteOffset + offsetU) <= request->mContentLength)
            {
                request->mWriteOffset += offsetU;
                retVal = CURL_SEEKFUNC_OK;
            }
            else
            {
                retVal = CURL_SEEKFUNC_FAIL;
                BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnection].seek_callback: CURL seek failed. offset(=" << (request->mWriteOffset+offsetU) << ") is greater than content length(=" << request->mContentLength << ").");
            }
            break;

        case SEEK_END:
            EA_ASSERT_MSG(false, "SEEK_END case needs to be implemented if supported by libcurl.");
            break;
        
        default:
            break;
    }

    if (retVal == CURL_SEEKFUNC_CANTSEEK) 
    {                                                                                                                                                                                                                                                                                               
       BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnection].seek_callback: CURL seek cannot seek. Unexpected value for origin(=" << origin << ").");
    }
    
    return retVal;
}

CURLcode OutboundHttpConnection::sslctx_callback(CURL *curl, void *sslctx, void *parm)
{
    OutboundHttpConnection* conn = (OutboundHttpConnection*) parm;
    bool vault = (conn->getOwnerManager().getSslPathType() == HttpServiceConnection::VAULT);

    // Passphrases are useful to protect ssl keys in transit from the admins who generate them
    // until they are placed in Vault, however it does not make sense to place a passphrase
    // in Vault right next to the key itself, thus we will only store decrypted keys in vault.
    // In order to ensure that we do not collide with a lingering passphrase set previously
    // in a config file to be used with a disk-based key, ensure that we explicitly
    // set passphrase to empty string when calling initSslCtx
    const char8_t* passphrase = (vault ? "" : conn->getOwnerManager().getKeyPassphrase());

    if (SslContext::initSslCtx((SSL_CTX*) sslctx, vault,
        conn->getOwnerManager().getPrivateCert(), conn->getOwnerManager().getPrivateKey(), passphrase))
    {
        return CURLE_OK;
    }
    return CURLE_SSL_CERTPROBLEM;
}

LibcurlSocketChannel* OutboundHttpConnection::createChannel(SOCKET sockfd)
{
    ChannelList::iterator itr = mChannels.begin();
    ChannelList::iterator end = mChannels.end();
    for(; itr != end; ++itr)
    {
        if (itr->getHandle() == static_cast<ChannelHandle>(sockfd))
        {
            LibcurlSocketChannel& channel = *itr;
            return &channel;
        }
    }

    LibcurlSocketChannel* channel = BLAZE_NEW_HTTP LibcurlSocketChannel(sockfd);
    mChannels.push_back(*channel);
    return channel;
}

void OutboundHttpConnection::destroyChannel(SOCKET sockfd)
{
    ChannelList::iterator itr = mChannels.begin();
    ChannelList::iterator end = mChannels.end();
    for(; itr != end; ++itr)
    {
        if (itr->getHandle() == static_cast<ChannelHandle>(sockfd))
        {
            mChannels.erase(itr);
            delete &(*itr);
            return;
        }
    }
}


HttpProtocolUtil::HttpReturnCode OutboundHttpConnection::buildPayloadParametersInXml(RawBuffer& buffer, CURL *easyHandle, const HttpParam params[]/* = nullptr*/, uint32_t paramsize/* = 0*/)
{
    if (params == nullptr)
        return HttpProtocolUtil::HTTP_OK;

    if (easyHandle == nullptr)
        return HttpProtocolUtil::HTTP_INVALID_REQUEST;

    char8_t* writeLocation = reinterpret_cast<char8_t*>(buffer.tail());
    size_t consumedSize = 0;

    // allocating an encode buffer on the stack.
    eastl::string value = "";

    uint32_t checkSize = static_cast<uint32_t>(strlen(HttpProtocolUtil::HTTP_XML_PAYLOAD));
    uint8_t* buf = buffer.acquire(checkSize + 1);
    if (buf == nullptr)
    {
        BLAZE_WARN_LOG(Log::HTTP, "[HttpProtocolUtil] There is not enough buffer size to "
            "write out " << checkSize << " more bytes.  Quitting!");
        return HttpProtocolUtil::HTTP_BUFFER_TOO_SMALL;
    }
    writeLocation = reinterpret_cast<char8_t*>(buf);
    consumedSize = blaze_snzprintf(writeLocation, buffer.tailroom(), "%s", HttpProtocolUtil::HTTP_XML_PAYLOAD);
    buffer.put(consumedSize);

    // Process the params.
    // Loop through params and apply them to buffer.

    HttpProtocolUtil::XmlItemList xmlClosingTagList;
    char8_t* repeatItem = nullptr;

    // create a buffer for xml string for the item
    char8_t xmlOutput[HttpProtocolUtil::XML_PAYLOAD_CONTENT_MAX_LENGTH];
    char8_t tempStr[Blaze::Collections::MAX_ATTRIBUTENAME_LEN + HttpProtocolUtil::XML_CLOSE_TAG_EXTRA_CHAR];
    for (uint32_t i = 0; i < paramsize; i++)
    {
        xmlOutput[0] = '\0';
        if (params[i].encodeValue)
        {
            OutboundHttpConnection::xmlEscapeString(params[i].value, static_cast<int>(strlen(params[i].value)), value);
        }
        else
        {
            value = params[i].value;
        }

        // build xml string for the item
        if (value.empty())
        {
            repeatItem = const_cast<char8_t*>(params[i].name);
            continue;
        }

        HttpProtocolUtil::buildXMLItem(params[i].name, value.c_str(), repeatItem, tempStr, sizeof(tempStr), xmlOutput, xmlClosingTagList);

        if (repeatItem)
            repeatItem = nullptr;

        // for last item in the list, we need to add all closing tag 
        if (i == paramsize - 1)
        {
            int32_t closingTagNum = static_cast<int32_t> (xmlClosingTagList.size());
            for (int32_t j = closingTagNum - 1; j >= 0; j--)
            {
                char8_t tempStr2[Blaze::Collections::MAX_ATTRIBUTENAME_LEN + HttpProtocolUtil::XML_CLOSE_TAG_EXTRA_CHAR];
                blaze_snzprintf(tempStr2, sizeof(tempStr2), "</%s>", xmlClosingTagList.at(j).c_str());
                blaze_strnzcat(xmlOutput, tempStr2, HttpProtocolUtil::XML_PAYLOAD_CONTENT_MAX_LENGTH);
            }
            xmlClosingTagList.clear();
        }
        checkSize = static_cast<uint32_t>(strlen(xmlOutput));

        // Check to see if there is enough buffer size.
        uint8_t* bufTemp = buffer.acquire(checkSize + 1);
        if (bufTemp != nullptr)
        {
            // Get new write location.
            writeLocation = reinterpret_cast<char8_t*>(bufTemp);
            consumedSize = blaze_snzprintf(writeLocation, buffer.tailroom(), "%s",
                xmlOutput);

            // Move the tail.
            buffer.put(consumedSize);
        }
        else
        {
            BLAZE_WARN_LOG(Log::HTTP, "[HttpProtocolUtil] There is not enough buffer size to "
                "write out " << checkSize << " more bytes.  Quitting!");
            return HttpProtocolUtil::HTTP_BUFFER_TOO_SMALL;
        } // if

    } // for

    return HttpProtocolUtil::HTTP_OK;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/


} // Blaze


