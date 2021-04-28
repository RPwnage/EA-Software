/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OutboundHttpConnectionManager

    Represents a pool of outbound HTTP connections to a specific HTTP server.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/libcurlsocketchannel.h"
#include "framework/connection/sslsocketchannel.h"
#include "framework/connection/sslcontext.h"
#include "framework/protocol/xml2encoder.h"
#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/util/shared/blazestring.h"
#include "framework/controller/controller.h"

#include "openssl/err.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define MAX_CONNECTION_RETRY 3
#define MAX_RATE_EXCEEDED_SLEEP_PERIOD 3
#define CHECK_HTTP_REQUEST_RATE \
    { \
        if (!checkHttpRequestRate()) { return ERR_TIMEOUT; } \
    }

const char8_t* HttpProxyCommand::toString(Metrics::TagValue& buffer) const
{
    buffer.sprintf("%s.%s", mCategory, mCommand);
    return buffer.c_str();
}


namespace Metrics
{
    namespace Tag
    {
        Metrics::TagInfo<HttpProxyCommand>* http_proxy_command = BLAZE_NEW Metrics::TagInfo<HttpProxyCommand>("http_proxy_command", [](const HttpProxyCommand& value, Metrics::TagValue& buffer) { return value.toString(buffer); });
    }
}


/*** Public Methods ******************************************************************************/

//These methods define the memory handlers curl will use
static void *curl_malloc_callback(size_t size)
{
    return BLAZE_ALLOC_MGID(size, MEM_GROUP_FRAMEWORK_HTTP, "HttpMalloc");
}

static void curl_free_callback(void *ptr)
{
    BLAZE_FREE(ptr);
}

static void *curl_realloc_callback(void *ptr, size_t size)
{
    return BLAZE_REALLOC_MGID(ptr, size, MEM_GROUP_FRAMEWORK_HTTP, "HttpMalloc");
}

static char *curl_strdup_callback(const char *str)
{
    return blaze_strdup(str, MEM_GROUP_FRAMEWORK_HTTP);
}

static void *curl_calloc_callback(size_t nmemb, size_t size)
{
    void* result = BLAZE_ALLOC_MGID(size * nmemb, MEM_GROUP_FRAMEWORK_HTTP, "HttpCalloc");
    memset(result, 0, nmemb * size);
    return result;
}

void OutboundHttpConnectionManager::initHttpForProcess()
{
    curl_global_init_mem(CURL_GLOBAL_SSL,
        &curl_malloc_callback, 
        &curl_free_callback, 
        &curl_realloc_callback, 
        &curl_strdup_callback, 
        &curl_calloc_callback);
}

void OutboundHttpConnectionManager::shutdownHttpForProcess()
{
    curl_global_cleanup();
}

void OutboundHttpConnectionManager::shutdownHttpForInstance()
{

}

OutboundHttpConnectionManager::OutboundHttpConnectionManager(const char8_t* serviceName)
    : mMultiHandle(nullptr),
      mName(serviceName),
      mShutdown(false),
      mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::http_pool, serviceName)),
      mPoolSize(mMetricsCollection, "http_pool.poolSize"),
      mMaxPoolSize(mMetricsCollection, "http_pool.maxPoolSize"),
      mPeakConns(mMetricsCollection, "http_pool.peakConns"),
      mConnsInUse(mMetricsCollection, "http_pool.connsInUse"),
      mSecure(false),
      mSecureVerifyPeer(false),
      mSslVersion(SSLVERSION_DEFAULT),
      mSslContext(),
      mGetRateLimitCb(nullptr),
      mRateLimitCounter(0),
      mCurrentRateInterval(0),
      mConnectionTimeout(DEFAULT_HTTP_CONNECTION_TIMEOUT_MICROSECONDS),
      mRequestTimeout(0),
      mHandleCompletedRequestsTimeout(DEFAULT_COMPLETE_HTTP_REQUESTS_TIMEOUT_MICROSECONDS),
      mTimeId(INVALID_TIMER_ID),
      mNumRunningHandles(0),
      mPrevRunningHandles(0),
      mCallTimer(mMetricsCollection, "http_pool.proxyCalls", Metrics::Tag::http_proxy_command),
      mCallErrors(mMetricsCollection, "http_pool.proxyCallErrors", Metrics::Tag::http_proxy_command, Metrics::Tag::http_error),
      mMetricTotalTimeouts(mMetricsCollection, "http_pool.timeouts"),
      mRefCount(0),
      mOwningComponentId(Component::INVALID_COMPONENT_ID),
      mIsManaged(true),
      mMaxHttpCacheMemInBytes(0),
      mTotalHttpCacheMemAllocInBytes(0),
      mMaxHttpResponseSizeInBytes(Blaze::MAX_HTTP_RESPONSE_SIZE) 
{
    EA_ASSERT_MSG(!mName.empty(), "OutboundHttpConnectionManager must have a non-nullptr name.");
}

OutboundHttpConnectionManager::~OutboundHttpConnectionManager()
{
    if (mTimeId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mTimeId);
    
    while (!mActiveConnections.empty())
    {
        OutboundHttpConnection *conn = &mActiveConnections.front();
        mActiveConnections.pop_front();
        delete conn;
    }

    while (!mInactiveConnections.empty())
    {
        OutboundHttpConnection *conn = &mInactiveConnections.front();
        mInactiveConnections.pop_front();
        delete conn;
    }

    if (mMultiHandle != nullptr)
    {
        curl_multi_cleanup(mMultiHandle);
    }

    // delete cache
    HttpCachedResponseByAccessList::iterator it = mHttpCachedResponseByAccessList.begin();
    while (it != mHttpCachedResponseByAccessList.end())
    {
        HttpCachedResponse& node = static_cast<HttpCachedResponse&>(*it);
        it = mHttpCachedResponseByAccessList.erase(it);

        delete &node;
    }
}

void OutboundHttpConnectionManager::initialize(const InetAddress& address, size_t maxPoolSize, bool secure, bool secureVerifyPeer, 
    SslVersion sslVersion, const char8_t* defaultContentType, GetRateLimitCb* rateLimitCb, 
    const TimeValue& connectTimeout, const TimeValue& requestTimeout, const TimeValue& handleCompletedRequestsTimeout, const char8_t* sslContextName, ComponentId owningComponentId,
    const HttpStatusCodeRangeList& codeRangesToErr/* = HttpStatusCodeRangeList()*/, bool managed/* = true*/)
{
    // Optionally outbound http connections can be configured to be passed through a proxy, an example use-case would be
    // to force traffic destined to another party to go through a proxy and thus appear to a remote party as coming from 
    // a consistent location (in case they require the use of an allow-list based on source IP address).
    // In this constructor we default proxy to an empty address and traffic will go directly to the destination.
    InetAddress emptyProxyAddress;
    initialize(address, emptyProxyAddress, maxPoolSize, secure, secureVerifyPeer, sslVersion, defaultContentType, rateLimitCb,
        connectTimeout, requestTimeout, handleCompletedRequestsTimeout, sslContextName, owningComponentId, codeRangesToErr, managed);
}

void OutboundHttpConnectionManager::initialize(const InetAddress& address, const InetAddress& proxyAddress, size_t maxPoolSize, bool secure, bool secureVerifyPeer, 
    SslVersion sslVersion, const char8_t* defaultContentType, GetRateLimitCb* rateLimitCb, 
    const TimeValue& connectTimeout, const TimeValue& requestTimeout, const TimeValue& handleCompletedRequestsTimeout, const char8_t* sslContextName, ComponentId owningComponentId,
    const HttpStatusCodeRangeList& codeRangesToErr/* = HttpStatusCodeRangeList()*/, bool managed/* = true*/)
{
    if (!(mAddress == address) || !(mProxyAddress == proxyAddress) || (mSecure != secure) || (secure && (mSslContext != gSslContextManager->get(sslContextName))))
    {
        char8_t addrBuf[256];
        char8_t proxyBuf[256];
        BLAZE_INFO_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << address.getHostname() <<"].initialize: " 
                       << address.toString(addrBuf, sizeof(addrBuf), InetAddress::HOST_PORT)
                       << ", secure: " << (secure ? "yes" : "no")
                       << (proxyAddress.isValid() ? ", proxy address: " : "")
                       << (proxyAddress.isValid() ? proxyAddress.toString(proxyBuf, sizeof(proxyBuf), InetAddress::HOST_PORT) : ""));

        if (!mActiveConnections.empty())
        {
            BLAZE_INFO_LOG(Log::HTTP, 
                "[OutboundHttpConnectionManager:" << mAddress.getHostname() << "].initialize: dropping " << (int32_t)mActiveConnections.size() << " active connections.");
            do
            {
                OutboundHttpConnection &conn = mActiveConnections.front();
                mActiveConnections.pop_front();
                delete &conn;
            } while (!mActiveConnections.empty());
        }

        if (!mInactiveConnections.empty())
        {
            BLAZE_INFO_LOG(Log::HTTP, 
                "[OutboundHttpConnectionManager:" << mAddress.getHostname() << "].initialize: dropping " << (int32_t)mInactiveConnections.size() << " inactive connections.");
            do
            {
                OutboundHttpConnection &conn = mInactiveConnections.front();
                mInactiveConnections.pop_front();
                delete &conn;
            } while (!mInactiveConnections.empty());
        }
        mPoolSize.set(0);
    }

    mAddress = address;
    mProxyAddress = proxyAddress;
    mMaxPoolSize.set(maxPoolSize);
    mSecure = secure;
    mSecureVerifyPeer = secureVerifyPeer;
    mGetRateLimitCb = rateLimitCb;
    if (requestTimeout.getMicroSeconds() != 0)
    {
        mRequestTimeout = requestTimeout;
    }
    if (connectTimeout.getMicroSeconds() != 0)
    {
        mConnectionTimeout = connectTimeout;
    }
    if (handleCompletedRequestsTimeout.getMicroSeconds() != 0)
    {
        mHandleCompletedRequestsTimeout = handleCompletedRequestsTimeout;
    }
    if (defaultContentType != nullptr)
    {
        mDefaultContentType = defaultContentType;
    }
    mSslVersion = sslVersion;

    if (mMultiHandle == nullptr)
    {
        mMultiHandle = curl_multi_init();
        if (mMultiHandle != nullptr)
        {
            curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETFUNCTION, callback_multi);
            curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERFUNCTION, callback_timer);
            curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETDATA, this);
            curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERDATA, this);
        }
    }

    if (sslContextName == nullptr)
        mSslContextName = "";
    else
        mSslContextName = sslContextName;

    mSslContext = gSslContextManager->get(sslContextName);

    eastl::string errorString;
    OutboundHttpService::buildHttpStatusCodeSet(codeRangesToErr, mHttpStatusCodeErrSet, errorString);

    mOwningComponentId = owningComponentId;

    mIsManaged = managed;
}

bool checkCacheControlOnlyCache(const char8_t* httpHeaders[], uint32_t headerCount)
{
    // Check if any "Cache-Control" header exists:
    eastl::string cacheHeader;
    for (uint32_t i = 0; i < headerCount; ++i)
    {
        if (blaze_strstr(httpHeaders[i], "Cache-Control") != nullptr)
        {
            cacheHeader = httpHeaders[i];
            break;
        }
    }

    if (!cacheHeader.empty())
    {
        // need to parse the key and find out if a cache duration was specified:
        const eastl::string onlyIfCachedStr("only-if-cached");
        eastl_size_t pos = cacheHeader.find(onlyIfCachedStr);
        if (pos != eastl::string::npos)
            return true;
    }

    return false;
}

bool OutboundHttpConnectionManager::checkCacheControlHeaders(HttpCachedResponse& cacheEntry, const char8_t* httpHeaders[], uint32_t headerCount, bool& staleIfError, bool& noStore )
{
    noStore = false;
    bool acceptAllStale = false;
    bool useCache = true;
    TimeValue cacheTime = cacheEntry.mCacheDuration;  // How long will we accept the cache value:
    TimeValue staleTime, staleIfErrorTime;

    // Check if any "Cache-Control" header exists:
    eastl::string cacheHeader;
    for (uint32_t i = 0; i < headerCount; ++i)
    {
        if (blaze_strstr(httpHeaders[i], "Cache-Control") != nullptr)
        {
            cacheHeader = httpHeaders[i];
            break;
        }
    }

    if (!cacheHeader.empty())
    {
        // need to parse the key and find out if a cache duration was specified:
        const eastl::string noCacheStr("no-cache"), noStoreStr("no-store"), maxAgeStr("max-age="), maxStaleAgeStr("max-stale"), minFreshStr("min-fresh="), 
                            maxStaleOnFailStr("max-stale-on-fail="), staleIfErrorStr("stale-if-error=");
        // "no-transform", "stale-while-revalidate", and "immutable" are irrelevant to Blaze.

        eastl_size_t pos = cacheHeader.find(maxAgeStr);
        if (pos != eastl::string::npos)
        {
            int32_t newTime = EA::StdC::AtoI32(&cacheHeader[pos + maxAgeStr.size()]);
            if (newTime < cacheTime.getSec())
                cacheTime.setSeconds(newTime);
        }

        // freshness req reduces the cache duration supported:
        pos = cacheHeader.find(minFreshStr);
        if (pos != eastl::string::npos)
        {
            int32_t freshTime = EA::StdC::AtoI32(&cacheHeader[pos + minFreshStr.size()]);
            cacheTime.setSeconds(cacheTime.getSec() - freshTime);
        }

        pos = cacheHeader.find(maxStaleAgeStr);
        if (pos != eastl::string::npos)
        {
            char nextChar = cacheHeader[pos + maxStaleAgeStr.size()];
            if (nextChar == '=')
            {
                staleTime.setSeconds(EA::StdC::AtoI32(&cacheHeader[pos + maxStaleAgeStr.size() + 1]));
            }
            else if (nextChar != '-') //We need to make sure it's not max-stale-on-fail
            {
                acceptAllStale = true;
            }
        }

        pos = cacheHeader.find(maxStaleOnFailStr);
        if (pos != eastl::string::npos)
        {
            staleIfErrorTime.setSeconds(EA::StdC::AtoI32(&cacheHeader[pos + maxStaleOnFailStr.size()]));
        }
        pos = cacheHeader.find(staleIfErrorStr);
        if (pos != eastl::string::npos)
        {
            staleIfErrorTime.setSeconds(EA::StdC::AtoI32(&cacheHeader[pos + staleIfErrorStr.size()]));
        }

        pos = cacheHeader.find(noCacheStr);
        if (pos != eastl::string::npos)
            useCache = false;

        pos = cacheHeader.find(noStoreStr);
        if (pos != eastl::string::npos)
        {
            noStore = true;
            useCache = false;
        }
    }

    // If Cache-Control was used, and it hasn't expired yet, just use the data directly:
    staleIfError = false;
    if (useCache)
    {
        if (cacheEntry.mMustRevalidate)
        {
            // must-revalidate means that we can't accept stale entries, but it's unclear if that means stale from the server's perspective, the client's, or both's
            // We assume that it only applies to the server's concept of stale.  Client stale messages can still use the cache, if they set the request to allow it. 
            if (cacheEntry.mCacheDuration < EA::TDF::TimeValue::getTimeOfDay() - cacheEntry.mCacheStartTime)
            {
                return false;
            }
        }

        if (cacheTime + staleIfErrorTime > EA::TDF::TimeValue::getTimeOfDay() - cacheEntry.mCacheStartTime)
            staleIfError = true;

        if (acceptAllStale || (cacheTime + staleTime > EA::TDF::TimeValue::getTimeOfDay() - cacheEntry.mCacheStartTime))
        {
            return (!cacheEntry.mNoCache);  // true in most cases, false when "no-cache" was in the response.
        }
    }

    return false;
}

BlazeRpcError OutboundHttpConnectionManager::sendRequest(HttpProtocolUtil::HttpMethod method,
    const char8_t* uri, const HttpParam params[], uint32_t paramsize,
    const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
    OutboundHttpConnection::ContentPartList* contentList,
    int32_t logCategory,
    bool xmlPayload,
    const char8_t* command,
    const char8_t* httpUserPwd,
    uint16_t maxHandledUrlRedirects,
    const RpcCallOptions &options,
    SendRequestType sendRequestType)
{
    uint8_t retry = 0;
    BlazeRpcError err = Blaze::ERR_OK;

    uint32_t rawHeaderCount = headerCount;
    const char8_t** rawHttpHeaders = httpHeaders;
    do {
        CHECK_HTTP_REQUEST_RATE
        
        // http headers might point to cache that is valid in last iteration, but evicted now. Reset http header.
        httpHeaders = rawHttpHeaders;
        headerCount = rawHeaderCount;

        // we need to be able to cache the etag version locally if there is one, as the HttpCachedResponse may vanish during blocking calls 
        // it's only needed for the current iteration of the loop, re-set it each time, if needed
        eastl::string eTagVersionStr; 
        eastl::string censoredHttpCachedKeyStr;
        eastl::string httpCachedKeyStr;
        bool staleIfError = false;
        bool noStore = false;
        bool onlyIfCached = checkCacheControlOnlyCache(httpHeaders, headerCount);  // We do intentionally do this check, even if the cache support is disabled.
        EA::TDF::TdfBlobPtr cacheRawResponse;

        // If eTag found in cache, return headers including eTag. Otherwise, return empty list.
        eastl::vector<const char8_t*> headerListWithCacheSupport;
        if (isHttpCacheEnabled())
        {
            // Get the cache key:  (Uncensored, no size limit)
            HttpProtocolUtil::printHttpRequest(mSecure, method, getAddress().getHostname(), getAddress().getPort(InetAddress::HOST),
                uri, params, paramsize, httpHeaders, headerCount, httpCachedKeyStr, false);
            HttpProtocolUtil::printHttpRequest(mSecure, method, getAddress().getHostname(), getAddress().getPort(InetAddress::HOST),
                uri, params, paramsize, httpHeaders, headerCount, censoredHttpCachedKeyStr);

            // Check if the Cache exist: 
            const auto& cacheIter = mHttpCachedResponseByReqMap.find(httpCachedKeyStr);
            if (cacheIter != mHttpCachedResponseByReqMap.end() && !httpCachedKeyStr.empty())
            {
                HttpCachedResponse& cacheEntry = static_cast<HttpCachedResponse&>(*cacheIter);
                cacheRawResponse = cacheEntry.mRawBuffer;
                if (checkCacheControlHeaders(cacheEntry, httpHeaders, headerCount, staleIfError, noStore))
                {
                    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].sendRequest: Using cached result for (" << censoredHttpCachedKeyStr << ") from cache.  "
                        << "Expiring at " << (cacheEntry.mCacheStartTime + cacheEntry.mCacheDuration) << ".");

                    // Update the Cache entry priority, and just use the cache response directly:
                    if ((cacheRawResponse->getCount() != 0) && (result != nullptr))
                    {
                        result->processRecvData(reinterpret_cast<char8_t*>(cacheRawResponse->getData()), cacheRawResponse->getCount(), true);
                    }

                    // move element to be the latest
                    mHttpCachedResponseByAccessList.remove(cacheEntry);
                    mHttpCachedResponseByAccessList.push_back(cacheEntry);

                    // Exit early because we're just using the cached value:
                    if (result != nullptr)
                        result->setHttpStatusCode(HTTP_STATUS_OK);

                    return ERR_OK;
                }

                // If an ETag was found:
                if (!cacheEntry.mETagVersion.empty())
                {
                    // move element to be the latest
                    mHttpCachedResponseByAccessList.remove(cacheEntry);
                    mHttpCachedResponseByAccessList.push_back(cacheEntry);

                    // Create a new array of headers, and use that: 
                    for (size_t i = 0; i < headerCount; ++i)
                        headerListWithCacheSupport.push_back(httpHeaders[i]);
                    // cache our etag version here in case cacheEntry goes away when we block below
                    // eTagVersionStr is valid for the current iteration of this loop
                    eTagVersionStr = cacheEntry.mETagVersion;
                    headerListWithCacheSupport.push_back(eTagVersionStr.c_str());

                    // Update the headers and header count: 
                    httpHeaders = headerListWithCacheSupport.data();
                    ++headerCount;
                }
            }
        }

        if (onlyIfCached)
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].sendRequest: Cache-Control: only-if-cached was set for (" << censoredHttpCachedKeyStr <<
                ") but data was not in cache.  Failing request.");

            // Return 504 if no cache exists and the client required it:
            if (result != nullptr)
                result->setHttpStatusCode(504);
            return ERR_SYSTEM;
        }

        OutboundHttpConnection *conn = nullptr;
        err = acquireConnection(conn);

        if (err != ERR_OK)
        {
            return err;
        }

        switch (sendRequestType)
        {
        case Xml_Payload:   err = conn->sendRequest(method, uri, params, paramsize, httpHeaders, headerCount, result, logCategory, xmlPayload, options); break;
        case Url_Redirects: err = conn->sendRequest(method, uri, params, paramsize, httpHeaders, headerCount, result, contentList, logCategory, false, httpUserPwd, maxHandledUrlRedirects, options); break;
        case No_Params:     err = conn->sendRequest(method, uri, httpHeaders, headerCount, result, contentList, logCategory, options); break;
        }

        // If we have a stale value that we'll accept, set the error to OK so we do the cache code, and exit out. 
        if (staleIfError)
            err = ERR_OK;

        if (err == ERR_OK)
        {
            // This updates the cached values if a ETag or CacheControl header was included, and uses the cache if a 304 response happened
            updateHttpCache(result, method, httpCachedKeyStr, censoredHttpCachedKeyStr, staleIfError, noStore, cacheRawResponse);

            // Set the Http Status after the cache call, since if an error was returned we've already used the cached data.
            if (staleIfError && result != nullptr)
                result->setHttpStatusCode(HTTP_STATUS_OK);
        }

        releaseConnection(*conn);
        retry++;
    } while (err == Blaze::ERR_DISCONNECTED && retry < MAX_CONNECTION_RETRY);

    return err;
}


BlazeRpcError OutboundHttpConnectionManager::sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
        bool xmlPayload/* = false*/,
        const char8_t* command/* = nullptr*/,
        const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    return sendRequest(method, URI, params, paramsize,
        httpHeaders, headerCount, result,
        nullptr,
        logCategory,
        xmlPayload,
        command,
        nullptr, 0,
        options, Xml_Payload);
}

bool OutboundHttpConnectionManager::isHttpCacheEnabled() const
{ 
    return (mMaxHttpCacheMemInBytes != 0);
}

void OutboundHttpConnectionManager::setMaxHttpCacheSize(size_t bytes)
{
    mMaxHttpCacheMemInBytes = bytes;
}

bool OutboundHttpConnectionManager::isHTTPMethodForFetch(HttpProtocolUtil::HttpMethod method) const
{
    return (method == HttpProtocolUtil::HTTP_GET || method == HttpProtocolUtil::HTTP_HEAD);
}

BlazeRpcError OutboundHttpConnectionManager::sendRequest(HttpProtocolUtil::HttpMethod method,
    const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
    const char8_t* contentType, const char8_t* contentBody, uint32_t contentLength,
    int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
    const char8_t* command/* = nullptr*/, 
    const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    OutboundHttpConnection::ContentPartList contentList;
    OutboundHttpConnection::ContentPart content(OutboundHttpConnection::CONTENT);
    content.mContent = contentBody;
    content.mContentLength = contentLength;
    content.mContentType = contentType;
    content.mName = "content";

    contentList.push_back(&content);

    return sendRequest(method, URI, httpHeaders, headerCount, result, &contentList, logCategory, command, options);
}

BlazeRpcError OutboundHttpConnectionManager::sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
        const char8_t* command/* = nullptr*/,
        const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    return sendRequest(method, URI, nullptr, 0,
        httpHeaders, headerCount, result,
        contentList,
        logCategory,
        false,
        command,
        nullptr, 0,
        options, 
        No_Params);
}

BlazeRpcError OutboundHttpConnectionManager::sendRequest(HttpProtocolUtil::HttpMethod method,
    const char8_t* URI, const HttpParam params[], uint32_t paramsize,
    const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
    OutboundHttpConnection::ContentPartList* contentList,
    int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
    const char8_t* command/* = nullptr*/,
    const char8_t* httpUserPwd/* = nullptr*/,
    uint16_t maxHandledUrlRedirects/* = 0*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    return sendRequest(method, URI, params, paramsize,
        httpHeaders, headerCount, result,
        contentList,
        logCategory,
        false,
        command,
        httpUserPwd, maxHandledUrlRedirects,
        options,
        Url_Redirects);
}

void OutboundHttpConnectionManager::removeHttpCacheIfExists(const eastl::string& cachedKey)
{
    HttpCachedResponseByHttpRequestMap::iterator delItr = mHttpCachedResponseByReqMap.find(cachedKey);
    if (delItr != mHttpCachedResponseByReqMap.end())
    {
        HttpCachedResponse& node = static_cast<HttpCachedResponse&>(*delItr);
        removeHttpCacheResponse(node);
    }
}
void OutboundHttpConnectionManager::removeHttpCacheResponse(OutboundHttpConnectionManager::HttpCachedResponse& node)
{
    // Can't log the cached key here, since the cache key held on the node is uncensored. 
    // Using the start time instead, since the censored key should be logged then. 
    BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].removeHttpCacheIfExists: remove http cache entry key with start time(" << node.mCacheStartTime << ").");

    size_t itrSize = node.getResponseLength();
    mHttpCachedResponseByAccessList.remove(node);
    mHttpCachedResponseByReqMap.remove(node);
    mTotalHttpCacheMemAllocInBytes -= itrSize;        

    delete &node;
}

OutboundHttpConnectionManager::HttpCachedResponse::HttpCachedResponse(bool noCache, bool mustRevalidate, const eastl::string& cachedKey, OutboundHttpResult *result, 
    eastl::string version, EA::TDF::TimeValue cacheTime)
{
    mNoCache = noCache;
    mMustRevalidate = mustRevalidate;
    setHttpRequest(cachedKey);
    mRawBuffer = BLAZE_NEW EA::TDF::TdfBlob;
    mRawBuffer->setData(result->getBuffer().data(), result->getBuffer().datasize());

    if (!version.empty())
    {
        mETagVersion = "If-None-Match: ";
        mETagVersion.append(version);
    }

    mCacheStartTime = EA::TDF::TimeValue::getTimeOfDay();
    if (cacheTime != 0)
    {
        mCacheDuration = cacheTime;
    }

}

void OutboundHttpConnectionManager::updateHttpCache(OutboundHttpResult *result, HttpProtocolUtil::HttpMethod method, const eastl::string& cachedKey, 
    const eastl::string& censoredCacheKey, bool staleIfError, bool requestSaysNoStore, EA::TDF::TdfBlobPtr& rawCachePtr)
{
    if (result == nullptr || cachedKey.empty() || !isHttpCacheEnabled() || !isHTTPMethodForFetch(method) || requestSaysNoStore)
    {
        return;
    }

    // if http result is 200, and has etag, update cache
    if (result->getHttpStatusCode() == 200)
    {
        bool noCache = false;  // "no-cache" header value
        bool noStore = false;  // "no-store" header value
        bool mustRevalidate = false; // "must-revalidate" header value
        EA::TDF::TimeValue cacheTime = 0;
        HttpHeaderMap::const_iterator cacheHeader = result->getHeaderMap().find("Cache-Control");
        if (cacheHeader != result->getHeaderMap().end())
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Found [" << cacheHeader->first.c_str() << ": " << cacheHeader->second.c_str()
                << "] for http request " << censoredCacheKey);

            // need to parse the key and find out if a cache duration was specified:
            const eastl::string noCacheStr("no-cache"), noStoreStr("no-store"), mustRevalidateStr("must-revalidate"), maxAgeStr("max-age="), sharedMaxAgeStr("s-maxage=");
            // "public", "private", "proxy-revalidate", "no-transform", and "immutable" are irrelevant to Blaze.
            
            eastl_size_t pos = cacheHeader->second.find(maxAgeStr);
            if (pos != eastl::string::npos)
                cacheTime.setSeconds( EA::StdC::AtoI32(&cacheHeader->second[pos + maxAgeStr.size()]) );

            // Shared max age takes precedence:
            pos = cacheHeader->second.find(sharedMaxAgeStr);
            if (pos != eastl::string::npos)
                cacheTime.setSeconds( EA::StdC::AtoI32(&cacheHeader->second[pos + sharedMaxAgeStr.size()]) );

            pos = cacheHeader->second.find(noCacheStr);
            if (pos != eastl::string::npos)
                noCache = true;

            pos = cacheHeader->second.find(noStoreStr);
            if (pos != eastl::string::npos)
                noStore = true;

            pos = cacheHeader->second.find(mustRevalidateStr);
            if (pos != eastl::string::npos)
                mustRevalidate = true;
        }

        // Check for the ETag cache support:
        eastl::string version;
        HttpHeaderMap::const_iterator it = result->getHeaderMap().find("ETag");
        if (it != result->getHeaderMap().end())
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Found [" << it->first.c_str() << ": " << it->second.c_str() 
                << "] for http request " << censoredCacheKey);
            version = it->second.c_str();
        }

        // Only Cache if the request gives us caching info, and doesn't specifically disallow us. 
        if ((!version.empty() || cacheTime != 0) && noStore == false && result->getBuffer().datasize() != 0)
        {
            // remove stale cache if any
            removeHttpCacheIfExists(cachedKey);

            HttpCachedResponse* node = BLAZE_NEW HttpCachedResponse(noCache, mustRevalidate, cachedKey, result, version, cacheTime);
            size_t cachedRespSize = node->getResponseLength();
            if (cachedRespSize >= mMaxHttpCacheMemInBytes)
            {
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: " << censoredCacheKey << " size(" << cachedRespSize
                    << ") is larger than http cache max memory limit(" << mMaxHttpCacheMemInBytes << "). Skip caching.");
                delete node;
                return;
            }

            // check if not reaching max cache memory limit
            while (mTotalHttpCacheMemAllocInBytes + cachedRespSize >= mMaxHttpCacheMemInBytes && !mHttpCachedResponseByAccessList.empty())
            {
                // evict oldest element to make more space
                HttpCachedResponse& dirtyNode = static_cast<HttpCachedResponse&>(mHttpCachedResponseByAccessList.front());
                removeHttpCacheResponse(dirtyNode);
            }
            mHttpCachedResponseByAccessList.push_back(*node);
            mHttpCachedResponseByReqMap.insert(*node);
            mTotalHttpCacheMemAllocInBytes += cachedRespSize;
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Add request(" << censoredCacheKey << ") size " << cachedRespSize << " bytes to cache.");
        }
    }
    else if (result->getHttpStatusCode() == 304 || staleIfError)
    {
        // It's possible, though unlikely, that we got new cache entry data in between the original lookup and now, so we always do the cache lookup 
        // and only use the results of the previous lookup if this one failed. 

        // if http error is 304 (not modified), or the request failed and we wanted to use the stale data, populate result from previous cached http response
        HttpCachedResponseByHttpRequestMap::iterator mapItr = mHttpCachedResponseByReqMap.find(cachedKey);
        if (mapItr != mHttpCachedResponseByReqMap.end())
        {
            BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Using cached value for http request " << censoredCacheKey);
            HttpCachedResponse& node = static_cast<HttpCachedResponse&>(*mapItr);
            const EA::TDF::TdfBlobPtr& raw = node.mRawBuffer;
            if (raw->getCount() != 0)
            {
                result->processRecvData(reinterpret_cast<char8_t*>(raw->getData()), raw->getCount(), true);
            }
            // move element to be the latest
            mHttpCachedResponseByAccessList.remove(node);
            mHttpCachedResponseByAccessList.push_back(node);
        }
        else
        {
            if (rawCachePtr != nullptr)
            {
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Using previously looked up value for http request " << censoredCacheKey << ".");
                result->processRecvData(reinterpret_cast<char8_t*>(rawCachePtr->getData()), rawCachePtr->getCount(), true);
            }
            else
            {
                BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager].updateHttpCache: Using previously looked up value for http request " << censoredCacheKey << ".");
            }
        }
    }
}

/*! ************************************************************************************************/
/*! \brief sendRequest inherited from RpcProxySender.  
    NOTE: This method should only be called via the proxy sender which is managed by the OutboundHttpService.

    \param[in] componentId - the component the request command lives on
    \param[in] commandId - the command being requested
    \param[in] requestTdf - the request
    \param[in] responseTdf - the response
    \param[in] errorTdf - the error object.
    \param[in] logCategory - log category override.
    \param[in] options - RPC call options for this request
***************************************************************************************************/
BlazeRpcError OutboundHttpConnectionManager::sendRequest(ComponentId componentId, CommandId commandId, const EA::TDF::Tdf* requestTdf,
    EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory/* = CONNECTION_LOGGER_CATEGORY*/,
    const RpcCallOptions &options/* = RpcCallOptions()*/, RawBuffer* responseRaw/* = nullptr*/)
{
    CHECK_HTTP_REQUEST_RATE

    BlazeRpcError err = Blaze::ERR_OK;

    const RestResourceInfo* restInfo = BlazeRpcComponentDb::getRestResourceInfo(componentId, commandId);
    if (restInfo == nullptr)
    {
        BLAZE_WARN_LOG(Log::REST, "[OutboundHttpConnectionManager]: No rest resource info found for (component= '"
            << BlazeRpcComponentDb::getComponentNameById(componentId) << "', command='" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "').");
        return ERR_SYSTEM;
    }

    if (mDefaultContentType.empty() && restInfo->contentType == nullptr)
    {
        BLAZE_WARN_LOG(Log::REST, "[OutboundHttpConnectionManager] Unable to send request from component " << componentId << " and command " << commandId << " without encoder type");
        return ERR_SYSTEM;
    }

    // Unless this OutboundHttpConnectionManager has been explicitly initialized as "unmanaged" (e.g. because, like PAS, it can never be reconfigured), 
    // this method must be called via the proxy sender, which is managed by the outbound http service.
    HttpConnectionManagerPtr connMgr = nullptr;
    if (mIsManaged)
    {
        connMgr = gOutboundHttpService->getConnection(getName());
        if (connMgr.get() == nullptr)
        {
            BLAZE_WARN_LOG(Log::REST, "[OutboundHttpConnectionManager] This connection manager to address " << getAddress().getHostname() << " is not being managed by the HTTP service.");
            return ERR_SYSTEM;
        }
    }
    else
    {
        connMgr = this;
    }

    eastl::string uriPrefix;
    if (restInfo->apiVersion != nullptr && restInfo->apiVersion[0] != '\0')
    {
        uriPrefix.append_sprintf("/%s", restInfo->apiVersion);
    }

    const char8_t* contentType = mDefaultContentType.c_str();
    if (restInfo->contentType != nullptr)
        contentType = restInfo->contentType;

    gController->getExternalServiceMetrics().incCallsStarted(connMgr->getName(), restInfo->resourcePathFormatted);
    gController->getExternalServiceMetrics().incRequestsSent(connMgr->getName(), restInfo->resourcePathFormatted);

    HttpStatusCode statusCode = 0;
    TimeValue startTime = TimeValue::getTimeOfDay();
    err = RestProtocolUtil::sendHttpRequest(connMgr, componentId, commandId, requestTdf, contentType, restInfo->addEncodedPayload, responseTdf, errorTdf,
        nullptr, uriPrefix.c_str(), &statusCode, logCategory, options, responseRaw);
    TimeValue endTime = TimeValue::getTimeOfDay();
    
    gController->getExternalServiceMetrics().incCallsFinished(connMgr->getName(), restInfo->resourcePathFormatted);
    
    eastl::string statusString;
    if (statusCode == 0) // status code is not populated
        statusString.append_sprintf("BLAZE_%s", LOG_ENAME(err));
    else 
        statusString.append_sprintf("HTTP_%" PRIu32, statusCode);

    gController->getExternalServiceMetrics().incResponseCount(connMgr->getName(), restInfo->resourcePathFormatted, statusString.c_str());
    gController->getExternalServiceMetrics().recordResponseTime(connMgr->getName(), restInfo->resourcePathFormatted, statusString.c_str(), (endTime - startTime).getMicroSeconds());

    metricProxyComponentRequest(componentId, commandId, endTime - startTime, statusCode);
    return err;
}


BlazeRpcError OutboundHttpConnectionManager::sendStreamRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory/* = HTTP_LOGGER_CATEGORY*/,
        const char8_t* command/* = nullptr*/, const RpcCallOptions &options/* = RpcCallOptions()*/)
{
    uint8_t retry = 0;
    BlazeRpcError err = Blaze::ERR_OK;

    do {
        CHECK_HTTP_REQUEST_RATE

        OutboundHttpConnection *conn = nullptr;
        err = acquireConnection(conn);

        if (err != ERR_OK)
        {
            return err;
        }

        err = conn->sendStreamRequest(method, URI, httpHeaders, headerCount, handler, contentList, logCategory, options);
        releaseConnection(*conn);
        retry++;
    } while (err == Blaze::ERR_DISCONNECTED && retry < MAX_CONNECTION_RETRY);

    return err;
}

BlazeRpcError OutboundHttpConnectionManager::acquireConnection(OutboundHttpConnection*& result)
{
    // if 'missing host' or 'no pool' is not a misconfiguration, then we're intentionally disabling access to this pool/service
    // and we should review any attempted usage
    if (mAddress.getHostname()[0] == '\0')
    {
        BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnectionManager].acquireConnection: missing host");
        return ERR_SYSTEM;
    }
    if (mMaxPoolSize.get() == 0)
    {
        BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << mAddress.getHostname() << "].acquireConnection: no pool");
        return ERR_SYSTEM;
    }

    //Nothing if we're shutdown
    if (mShutdown)
    {
        return ERR_SYSTEM;
    }

    result = nullptr;

    //See if there's an inactive connection available
    if (!mInactiveConnections.empty())
    {
        result = &mInactiveConnections.front();
        mInactiveConnections.pop_front();
        mActiveConnections.push_back(*result);

        // status tracking.
        trackConnectionAcquire();

        return ERR_OK;
    }

    //See if we can create one.
    if (mPoolSize.get() < mMaxPoolSize.get())
    {
        result = BLAZE_NEW_HTTP OutboundHttpConnection(*this);

        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << mAddress.getHostname() << "].acquireConnection: Creating connection; sess=" << result);

        result->initialize(mSslContext);

        mActiveConnections.push_back(*result);

        mPoolSize.increment();
    }
    else
    {
        WaitingFiber node;
        node.connection = &result;

        //Push the node back into the list in priority order
        WaitingFiberList::iterator i = mWaitingFibers.begin();
        WaitingFiberList::iterator e = mWaitingFibers.end();
        for (; i != e; ++i)
        {
            if ((*i).eventHandle.priority > Fiber::getCurrentPriority())
            {
                mWaitingFibers.insert(i, node);
                break;
            }
        }

        if (i == e)
        {
            mWaitingFibers.push_back(node);
        }

        // Only wait for the expected duration of the call: 
        // (This avoids an issue where we could be delaying responses for a long time if the pool size is too small.)
        // (We don't include mConnectionTimeout, since the connection should already be established given that the pool is full.)
        TimeValue maxWaitTime = (mRequestTimeout != 0) ? (TimeValue::getTimeOfDay() + mRequestTimeout) : 0;

        BlazeRpcError err = Fiber::getAndWait(node.eventHandle, "OutboundhttpConnectionManager::acquireConnection", maxWaitTime);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << mAddress.getHostname() << "].acquireConnection: Failed to acquire connection with Error: " << ErrorHelp::getErrorName(err) 
                                    << "If timing out, consider increasing pool size. (" << mWaitingFibers.size() << " requests waiting)" );

            //Clean up the node since its on the stack
            mWaitingFibers.remove(node);
            return err;
        }
    }

    // status tracking.
    trackConnectionAcquire();

    return (result == nullptr ? ERR_SYSTEM : ERR_OK);
}

void OutboundHttpConnectionManager::releaseConnection(OutboundHttpConnection& connection)
{
    // Decrease conns in use.
    mConnsInUse.decrement();

    if (mConnsInUse.get() == 0)
    {
        mConnectionDispatcher.dispatch<OutboundHttpConnectionManager&>(&OutboundHttpConnectionListener::onNoActiveConnections, *this);
    }

    if (connection.closeOnRelease())
    {
        connection.close();
        mActiveConnections.remove(connection);
        mPoolSize.decrement();
        delete &connection;
        return;
    }
    
    //Before we make the connection inactive, see if someone is waiting for it
    if (!mWaitingFibers.empty())
    {
        WaitingFiber &node = mWaitingFibers.front();
        mWaitingFibers.pop_front();

        *node.connection = &connection;
        Fiber::signal(node.eventHandle, ERR_OK);
    }
    else
    {
        //Just push us back on the inactive list.
        mActiveConnections.remove(connection);
        if (mPoolSize.get() < mMaxPoolSize.get()) //need to do this check here in case of reconfigure
        {
            mInactiveConnections.push_back(connection);
        }
        else
        {
            mPoolSize.decrement();
            delete &connection;
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getStatusInfo

    Fills out the details for it's own status.

*/
/*************************************************************************************************/
void OutboundHttpConnectionManager::getStatusInfo(OutboundConnMgrStatus& status) const
{
    status.setHost(mAddress.getHostname());
    status.setPort(mAddress.getPort(InetAddress::HOST));
    status.setTotalConnections((uint32_t)totalConnectionsCount());
    status.setActiveConnections((uint32_t)connectionsInUseCount());
    status.setPeakConnections((uint32_t)peakConnectionsCount());
    status.setMaxConnections((uint32_t)maxConnectionsCount());
    status.setTotalTimeouts(mMetricTotalTimeouts.getTotal());

} // getStatusInfo


// A separate status getter for handling specific metrics tracked only for http services that have proxy components.
void OutboundHttpConnectionManager::getHttpServiceStatusInfo(HttpServiceStatus& status) const
{
    OutboundConnMgrStatus& connMgrStatus = status.getConnMgrStatus();
    getStatusInfo(connMgrStatus);

    HttpServiceStatus::CommandErrorCodeList& errorCodeList = status.getCommandErrorCodeList();
    mCallErrors.iterate([&errorCodeList](const Metrics::TagPairList& tags, const Metrics::Counter& metric) {
            const char8_t* cmd = tags[0].second.c_str();
            uint32_t errorCode = 0;
            blaze_str2int(tags[1].second.c_str(), &errorCode);

            HttpServiceStatus::CommandErrorCodes* cmdEntry = nullptr;
            for(auto& itr : errorCodeList)
            {
                if (strcmp(itr->getCommand(), cmd) == 0)
                {
                    cmdEntry = itr;
                    break;
                }
            }
            if (cmdEntry == nullptr)
            {
                cmdEntry = errorCodeList.pull_back();
                cmdEntry->setCommand(cmd);
            }

            cmdEntry->getErrorCodeCountMap()[errorCode] += metric.getTotal();
        });
}

void OutboundHttpConnectionManager::metricProxyComponentRequest(ComponentId componentId, CommandId commandId, const TimeValue& duration, uint32_t httpResult)
{
    metricProxyRequest(BlazeRpcComponentDb::getComponentNameById(componentId), BlazeRpcComponentDb::getCommandNameById(componentId, commandId), duration, httpResult);
}

void OutboundHttpConnectionManager::metricProxyRequest(const char8_t* category, const char8_t* command, const TimeValue& duration, uint32_t httpResult)
{
    HttpProxyCommand cmd(category, command);
    mCallTimer.record(duration, cmd);
    mCallErrors.increment(1, cmd, httpResult);
}


void OutboundHttpConnectionManager::trackConnectionAcquire()
{
    mConnsInUse.increment();
    if (mConnsInUse.get() > mPeakConns.get())
    {
        mPeakConns.set(mConnsInUse.get());
    }

    // Note: Having trouble w/ fibers or context being lost due to the timer
    // being scheduled.
} // trackConnectionAcquire

bool OutboundHttpConnectionManager::checkHttpRequestRate()
{
    uint32_t rateLimit = 0;
    if (mGetRateLimitCb != nullptr)
        (*mGetRateLimitCb)(rateLimit);

    if (rateLimit == 0)
        return true;

    int32_t sleepCount = 0;
    while (sleepCount < MAX_RATE_EXCEEDED_SLEEP_PERIOD)
    {
        TimeValue now = TimeValue::getTimeOfDay();
        TimeValue intervalDelta = now - mCurrentRateInterval;
        bool elapsed = (intervalDelta > 1000 * 1000);
        if (elapsed)
        {
            mCurrentRateInterval = now;
            mRateLimitCounter = 1;
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].checkHttpRequestRate: " <<
                "Elapsed " << intervalDelta.getSec() << " secs. Reset rate limit counter and interval.");
            return true;
        }
        else if (rateLimit >= mRateLimitCounter)
        {
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].checkHttpRequestRate: " <<
                "current http request rate (" << mRateLimitCounter << "/sec) is under the limit (" << rateLimit << "/sec)");
            mRateLimitCounter++;
            return true;
        }
        else
        {
            BLAZE_WARN_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].checkHttpRequestRate: " <<
                 "current http request rate (" << mRateLimitCounter << "/sec) has exceeded the limit (" << rateLimit << "/sec)");
            Fiber::sleep(1000 * 1000, "send http request rate check squelch");
            ++sleepCount;
        }
    }

    return false;
}

int OutboundHttpConnectionManager::callback_multi(CURL *easyHandle, curl_socket_t sockfd, int action, void *userp, void *socketp)
{
    // Since this is a static function, we don't have (this) pointer
    // But we can get the pointer to the Outbound HTTP Connection Manager through the user pointer passed to the callback function
    // Not that we set this pointer to (this) in the Constructor using CURLMOPT_SOCKETDATA
    OutboundHttpConnectionManager *myself = reinterpret_cast<OutboundHttpConnectionManager *>(userp);
    if (myself == nullptr)
    {
        BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnectionManager].callback_multi: Callback made without valid outbound connection manager.  Indicates coding issue.");
        return 0; //bail if we don't have a correct userp pointer
    }
    
    // Get the pointer to the Connection contains this Easy Handle... its the Easy Handle's Private Pointer
    OutboundHttpConnection *connection;
    char8_t *ptr = nullptr;
    curl_easy_getinfo(easyHandle, CURLINFO_PRIVATE, &ptr);
    connection = reinterpret_cast<OutboundHttpConnection *>(ptr);

    LibcurlSocketChannel *channel;
    
    // If this is the first time for the socket:
    //  1. Create a Channel
    //  2. Assign the pointer of this Channel to the socket
    //  3. Set the Outbound HTTP Connection Manager as the Handler for the Connection
    //  4. Register the Channel with the Selector with the Interest Ops Specified by (action)
    // We can know whether its the first time or not by checking whether any pointer has been assigned to the socket.
    // The socket's private pointer is passed into the callback through (socketp)

    // ****CAVEAT****
    // You GOT TO register the channel with the interest ops specified by (action).
    // You CANNOT register with OP_NONE and then Set Interest Ops to (action)
    // Esp for windows selector.

    if (socketp == nullptr)
    {
        channel = connection->createChannel(sockfd);
        curl_multi_assign(myself->mMultiHandle, sockfd, channel);
        channel->setHandler(myself);

        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << myself->mAddress.getHostname() 
                        << "].callback_multi: Creating new LibCurl socket channel " << sockfd << ", action " << action);

        switch (action)
        {
            case CURL_POLL_IN:
                channel->registerChannel(Channel::OP_READ);
                break;

            case CURL_POLL_OUT:
                channel->registerChannel(Channel::OP_WRITE);
                break;

            case CURL_POLL_INOUT:
                channel->registerChannel(Channel::OP_READ | Channel::OP_WRITE);
                break;

            case CURL_POLL_NONE:
                channel->registerChannel(Channel::OP_NONE);
                break;

            case CURL_POLL_REMOVE:
                BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << myself->mAddress.getHostname() 
                               << "].callback_multi: Tried to remove unregistered channel!!");
                break;
            
            default:
                BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << myself->mAddress.getHostname() << "].callback_multi (channel created): Unknown action: " 
                               << action << " - channel will not be registered.");
        }
    }

    // Else if the Socket has already got a Channel
    // Simply get the channel pointer from the socket's private pointer
    // Set the Interest Ops specified by (action)

    else
    {
        channel = reinterpret_cast<LibcurlSocketChannel*>(socketp);      
        BLAZE_TRACE_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << myself->mAddress.getHostname() << "].callback_multi: Setting interest ops on socket channel " << (uint32_t) channel->getHandle() << ", action " << action);

      

        switch (action)
        {
            case CURL_POLL_IN:
                channel->setInterestOps(Channel::OP_READ);
                break;

            case CURL_POLL_OUT:
                channel->setInterestOps(Channel::OP_WRITE);
                break;
            
            case CURL_POLL_INOUT:
                channel->setInterestOps(Channel::OP_READ | Channel::OP_WRITE);
                break;
            
            case CURL_POLL_REMOVE:
                channel->unregisterChannel(true);
                connection->destroyChannel(sockfd);
                break;
            
            case CURL_POLL_NONE:
                channel->setInterestOps(Channel::OP_NONE);
                break;            

            default:
                BLAZE_WARN_LOG(Log::HTTP, "[OutboundHttpConnectionManager:" << myself->mAddress.getHostname() << "].callback_multi: Unknown action: " << action << " - channel will not be registered.");
        }  
    }

    //The callback must return zero
    return 0;
}

int OutboundHttpConnectionManager::callback_timer(const CURL *multiHandle, long timeout_ms, void *userp)
{
    //HACK - There is a timing issue in libcurl due to clock skew, where they adjust 1ms of time past our clock, and
    //use an alternate method of gathering the time that doesn't seem to be exactly in sync with our windows clock.  This
    //can greatly effect timers on a small scale (2 ms) but not 1 or 0ms timeouts.  Therefore if the timeout is < 5ms but
    //greater than 1ms, we adjust by adding an offset to ensure that clock skew does not cause us to miss a call.  The real
    //fix for this seems to be to sync the clocks, but there doesn't seem to be a good way of doing that now.
    timeout_ms = (timeout_ms > 1 && timeout_ms < 20) ? 20 : timeout_ms;

    OutboundHttpConnectionManager *myself = reinterpret_cast<OutboundHttpConnectionManager *>(userp);
    if (myself == nullptr)
    {
        BLAZE_ERR_LOG(Log::HTTP, "[OutboundHttpConnectionManager].callback_timer: Callback made without valid outbound connection manager.  Indicates coding issue.");
        return 0; //bail if the pointer is invalid
    }
        

    //According to the curl docs, timeout can be -1 which means no timeout is to be set
    if (timeout_ms != -1)
    {
        if (myself->mTimeId == INVALID_TIMER_ID)
        {                         
            myself->mTimeId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + timeout_ms * 1000 + (timeout_ms == 2 ? 20000 : 0), myself, &OutboundHttpConnectionManager::onTimeout, false,
                "OutboundHttpConnectionManager::onTimeout");            
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << myself->mAddress.getHostname() << "].callback timer: scheduling new timer " << myself->mTimeId << " for " << (uint32_t) timeout_ms << " ms");
        }
        else
        {
            TimeValue val = TimeValue::getTimeOfDay() + timeout_ms * 1000;
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << myself->mAddress.getHostname() << "].callback timer: rescheduling timer " << myself->mTimeId << " for " << (uint32_t) timeout_ms << " ms");
            gSelector->updateTimer(myself->mTimeId, val);
        }
    }
    else
    {
        if (myself->mTimeId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(myself->mTimeId);
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << myself->mAddress.getHostname() << "].callback timer: cancelling timer " << myself->mTimeId );
            myself->mTimeId = INVALID_TIMER_ID;
        }
        else
        {
            BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << myself->mAddress.getHostname() << "].callback timer: tried to cancel timer, but no valid timer id");
        }
    }
    return 0;
}

void OutboundHttpConnectionManager::onRead(Channel &channel)
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].onRead: got read for channel " << (uint32_t) channel.getHandle());
    

    mPrevRunningHandles = mNumRunningHandles;
    curl_multi_socket_action(mMultiHandle, channel.getHandle(), CURL_CSELECT_IN, &mNumRunningHandles);
    handleCompletedRequests();
}

void OutboundHttpConnectionManager::onWrite(Channel &channel)
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].onWrite: got write for channel " << (uint32_t) channel.getHandle());

    mPrevRunningHandles = mNumRunningHandles;
    curl_multi_socket_action(mMultiHandle, channel.getHandle(), CURL_CSELECT_OUT, &mNumRunningHandles);
    handleCompletedRequests();
}

void OutboundHttpConnectionManager::onError(Channel &channel)
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].onError: got error for channel " << (uint32_t) channel.getHandle());

    mPrevRunningHandles = mNumRunningHandles;
    curl_multi_socket_action(mMultiHandle, channel.getHandle(), CURL_CSELECT_ERR, &mNumRunningHandles);
    handleCompletedRequests();
}

void OutboundHttpConnectionManager::onClose(Channel &channel)
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].onClose: got close for channel " << (uint32_t)channel.getHandle());
    

    mPrevRunningHandles = mNumRunningHandles;
    curl_multi_socket_action(mMultiHandle, channel.getHandle(), 0, &mNumRunningHandles);
    handleCompletedRequests();
}

void OutboundHttpConnectionManager::onTimeout(bool all)
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].callback timer: timer " << mTimeId << " expired");
  
    mTimeId = INVALID_TIMER_ID;
    mPrevRunningHandles = mNumRunningHandles;
    if (!all)
    {
        curl_multi_socket_action(mMultiHandle, CURL_SOCKET_TIMEOUT, 0, &mNumRunningHandles);
    }
    else
    {
        curl_multi_socket_all(mMultiHandle, &mNumRunningHandles); 
    }

    handleCompletedRequests();
}

void OutboundHttpConnectionManager::handleCompletedRequests()
{
    BLAZE_TRACE_LOG(Log::HTTP, "OutboundHttpConnectionManager[" << mAddress.getHostname() << "].handleCompletedRequests: Number of running handles = " << mNumRunningHandles << ", Number of previous running handles = " << mPrevRunningHandles);

    if (mNumRunningHandles < mPrevRunningHandles || mNumRunningHandles == 0)
    {
        CURLMsg *msg = nullptr;
        int32_t msgs_left = 0;
        CURL *easyHandle = nullptr;
        CURLcode res;
        OutboundHttpConnection *connection = nullptr;
        char8_t *ptr = nullptr;

        msg = curl_multi_info_read(mMultiHandle, &msgs_left);
        while (msg)
        {
            if (msg->msg == CURLMSG_DONE)
            {
                easyHandle = msg->easy_handle;
                res = msg->data.result;
                if (easyHandle)
                {
                    curl_easy_getinfo(easyHandle, CURLINFO_PRIVATE, &ptr);
                    connection = reinterpret_cast<OutboundHttpConnection *>(ptr);
                    connection->processMessage(res);
                    if (res == CURLE_OPERATION_TIMEDOUT)
                    {
                        mMetricTotalTimeouts.increment();
                    }
                }
            }
            msg = curl_multi_info_read(mMultiHandle, &msgs_left);
        }
    }
    
    mPrevRunningHandles = mNumRunningHandles;

    //Schedule our timeout
    if (mTimeId == INVALID_TIMER_ID && mNumRunningHandles > 0)
    {
        //Schedule a timer
        mTimeId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + mHandleCompletedRequestsTimeout, this, &OutboundHttpConnectionManager::onTimeout, true, "OutboundHttpConnectionManager::onTimeout");
    }
}
/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

