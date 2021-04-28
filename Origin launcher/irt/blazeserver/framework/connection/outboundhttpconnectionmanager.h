/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDHTTPCONNECTIONMANAGER_H
#define BLAZE_OUTBOUNDHTTPCONNECTIONMANAGER_H

/*** Include files *******************************************************************************/

#include "framework/component/rpcproxysender.h"
#include "framework/connection/connection.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/outboundhttpconnection.h"
#include "framework/connection/sslcontext.h"
#include "framework/system/fiber.h"
#include "framework/util/average.h"
#include "framework/util/intervalmap.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_ptr.h"
#include "EASTL/intrusive_hash_map.h"

#include "framework/protocol/outboundhttpresult.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/httpparam.h"
#include "curl/curl.h"
#include "framework/util/lrucache.h"
#include "framework/component/component.h" // for Component::INVALID_COMPONENT_ID in OutboundHttpConnectionManager::initialize()

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
typedef eastl::hash_set<HttpStatusCode> HttpStatusCodeSet;

class ConnectionManagerStatus;
class OutboundHttpConnection;
class OutboundConnMgrStatus;
class HttpServiceStatus;
class OutboundHttpResult;
class OutboundHttpResponseHandler;
class OutboundHttpConnectionListener;

// How long we give to connect to the server we're going to send requests to.
#define DEFAULT_HTTP_CONNECTION_TIMEOUT_MICROSECONDS 30*1000000
// The interval we use when polling curl for completed requests.
#define DEFAULT_COMPLETE_HTTP_REQUESTS_TIMEOUT_MICROSECONDS 250*1000

class HttpProxyCommand
{
public:
    HttpProxyCommand(const char8_t* category, const char8_t* command)
        : mCategory(category)
        , mCommand(command)
    {
    }

    const char8_t* toString(Metrics::TagValue& buffer) const;

private:
    const char8_t* mCategory;
    const char8_t* mCommand;
};

namespace Metrics
{
    namespace Tag
    {
        extern Metrics::TagInfo<HttpProxyCommand>* http_proxy_command;
    }
}


class OutboundHttpConnectionManager : ChannelHandler, public RpcProxySender
{
public:

    static void initHttpForProcess();
    static void shutdownHttpForProcess();
    static void shutdownHttpForInstance();

    typedef Functor1<uint32_t&> GetRateLimitCb;

    typedef enum
    {
        SSLVERSION_DEFAULT = CURL_SSLVERSION_DEFAULT,
        SSLVERSION_SSLv2 = CURL_SSLVERSION_SSLv2,
        SSLVERSION_SSLv3 = CURL_SSLVERSION_SSLv3,
        SSLVERSION_TLSv1 = CURL_SSLVERSION_TLSv1,
        SSLVERSION_TLSv1_0 = CURL_SSLVERSION_TLSv1_0,
        SSLVERSION_TLSv1_1 = CURL_SSLVERSION_TLSv1_1,
        SSLVERSION_TLSv1_2 = CURL_SSLVERSION_TLSv1_2,
        SSLVERSION_TLSv1_3 = CURL_SSLVERSION_TLSv1_3
    } SslVersion;

public:
    OutboundHttpConnectionManager(const char8_t* serviceName);
    ~OutboundHttpConnectionManager() override;

    // Warning: Calling initialize() on an OutboundHttpConnectionManager that has already been initialized will
    // cause it to drop any existing connections IF:
    // (1) 'address' is different from the OutboundHttpConnectionManager's current address, OR
    // (2) the OutboundHttpConnectionManager was initially insecure/secure and is being re-initialized as secure/insecure, OR
    // (3) the OutboundHttpConnectionManager is secure and requires a new SslContext (either because
    //     'sslContextName' is different from the OutboundHttpConnectionManager's current SslContextName, or
    //     because the SslContext with that name has been updated).
    void initialize(const InetAddress& address, size_t maxPoolSize, bool secure = false, bool secureVerifyPeer = true,
        SslVersion sslVersion = SSLVERSION_DEFAULT, const char8_t* defaultContentType = nullptr, GetRateLimitCb* rateLimitCb = nullptr,
        const EA::TDF::TimeValue& connectTimeout = EA::TDF::TimeValue(), const EA::TDF::TimeValue& requestTimeout = EA::TDF::TimeValue(),
        const EA::TDF::TimeValue& handleCompletedRequestsTimeout = EA::TDF::TimeValue(), const char8_t* sslContextName = nullptr, EA::TDF::ComponentId owningComponentId = Component::INVALID_COMPONENT_ID,
        const HttpStatusCodeRangeList& codeRangesToErr = HttpStatusCodeRangeList(), bool managed = true);

    //Helper function for calling one send on a connection
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        bool xmlPayload = false,
        const char8_t* command = nullptr, 
        const RpcCallOptions &options = RpcCallOptions()));

    //Helper function for calling one send on a connection
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        const char8_t* contentType, const char8_t* contentBody, uint32_t contentLength,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        const char8_t* command = nullptr, 
        const RpcCallOptions &options = RpcCallOptions()));

    //Helper function for calling one send on a connection
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        const char8_t* command = nullptr,
        const RpcCallOptions &options = RpcCallOptions()));

    //Helper function for calling one send on a connection
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        const char8_t* command = nullptr, const char8_t* httpUserPwd = nullptr, uint16_t maxHandledUrlRedirects = 0,
        const RpcCallOptions &options = RpcCallOptions()));

    // From RpcProxySender
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(EA::TDF::ComponentId component, CommandId command, const EA::TDF::Tdf* request,
        EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory = CONNECTION_LOGGER_CATEGORY,
        const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr) override);

    // Helper function which allows you to provide an OutboundHttpResponseHandler.  In order to
    // process response data as it arrives.  The response data is sent to the OutboundHttpResponseHandler::processData()
    // method as it arrives.  This function is useful if you need to call an HTTP live stream, for example.
    BlazeRpcError DEFINE_ASYNC_RET(sendStreamRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        const char8_t* command = nullptr, const RpcCallOptions &options = RpcCallOptions()));

    // Get a connection connected to the given endpoint.
    BlazeRpcError DEFINE_ASYNC_RET(acquireConnection(OutboundHttpConnection*& connection));
    void releaseConnection(OutboundHttpConnection& connection);

    // Retrieves status information about these connections.
    size_t connectionsInUseCount() const { return mConnsInUse.get(); }
    size_t peakConnectionsCount() const { return mPeakConns.get(); }
    size_t totalConnectionsCount() const { return mPoolSize.get(); }
    size_t maxConnectionsCount() const { return mMaxPoolSize.get(); }
    void getStatusInfo(OutboundConnMgrStatus& status) const;
    void getHttpServiceStatusInfo(HttpServiceStatus& status) const;

    //ChannelHandler Interface
    void onRead(Channel &channel) override;
    void onWrite(Channel &channel) override;
    void onError(Channel &channel) override;
    void onClose(Channel &channel) override;

    //Callback functions
    static int callback_multi(CURL *easyHandle, curl_socket_t sockfd, int action, void *userp, void *socketp);
    static int callback_timer(const CURL *multiHandle, long timeout_ms, void *userp);

    // Connection Listener methods
    void addConnectionListener(OutboundHttpConnectionListener& listener) { mConnectionDispatcher.addDispatchee(listener); }
    void removeConnectionListener(OutboundHttpConnectionListener& listener) { mConnectionDispatcher.removeDispatchee(listener); }

    void onTimeout(bool all = false);

    const InetAddress& getAddress() const { return mAddress; }
    bool isSecure() const { return mSecure; }
    bool isSecureVerifyPeer() const { return mSecureVerifyPeer; }
    bool hasLatestSslContext() const { return mSslContext == gSslContextManager->get(mSslContextName.c_str()); }
    void setSslPathType(HttpServiceConnection::SSLPathType sslPathType) { mSslPathType = sslPathType; }
    void setVaultPath(const char8_t* path) { mVaultPath = path; }
    void setPrivateCert(const char8_t* cert) { mPrivateCert = cert; }
    void setPrivateKey(const char8_t* key) { mPrivateKey = key; }
    void setKeyPassphrase(const char8_t* pass) { mPrivateKeyPassphrase = pass; }
    void setDefaultContentType(const char8_t* contentType) { mDefaultContentType = contentType; }
    HttpServiceConnection::SSLPathType getSslPathType() const { return mSslPathType; }
    const char8_t* getVaultPath() const { return mVaultPath.c_str(); }
    const char8_t* getPrivateCert() const { return mPrivateCert.c_str(); }
    const char8_t* getPrivateKey() const { return mPrivateKey.c_str(); }
    const char8_t* getKeyPassphrase() const { return mPrivateKeyPassphrase.c_str(); }
    const char8_t* getDefaultContentType() const { return mDefaultContentType.c_str(); }
    SslVersion getSslVersion() const { return mSslVersion; }
    const EA::TDF::TimeValue& getRequestTimeout() const { return mRequestTimeout; }
    const EA::TDF::TimeValue& getConnectionTimeout() const { return mConnectionTimeout; }
    const EA::TDF::TimeValue& getHandleCompletedRequestsTimeout() const { return mHandleCompletedRequestsTimeout; }

    //OutboundMetricsThreshold for the OutboundMetricsManager
    EA::TDF::TimeValue getOutboundMetricsThreshold() const { return mOutboundMetricsThreshold; }
    void setOutboundMetricsThreshold(EA::TDF::TimeValue outboundMetricsThreshold) { mOutboundMetricsThreshold = outboundMetricsThreshold; }

    void setMaxHttpCacheSize(size_t bytes);

    void setMaxHttpResponseSize(size_t bytes) { mMaxHttpResponseSizeInBytes = bytes; }
    size_t getMaxHttpResponseSize() { return mMaxHttpResponseSizeInBytes; }

    void metricProxyComponentRequest(EA::TDF::ComponentId componentId, CommandId command, const EA::TDF::TimeValue& duration, uint32_t httpResult);
    void metricProxyRequest(const char8_t* category, const char8_t* command, const EA::TDF::TimeValue& duration, uint32_t httpResult);

    const EA::TDF::ComponentId getOwningComponentId() const { return mOwningComponentId; }

    const char8_t* getName() const { return mName.c_str(); }

    const HttpStatusCodeSet& getHttpStatusCodeErrSet() const { return mHttpStatusCodeErrSet; }

    bool isManaged() const { return mIsManaged; }

    //Multi Handle
    CURLM *mMultiHandle;

protected:

private:
    typedef eastl::intrusive_list<OutboundHttpConnection> ConnectionList;

    enum SendRequestType {
        Url_Redirects, Xml_Payload, No_Params
    };

    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        OutboundHttpConnection::ContentPartList* contentList,
        int32_t logCategory,
        bool xmlPayload,
        const char8_t* command,
        const char8_t* httpUserPwd,
        uint16_t maxHandledUrlRedirects,
        const RpcCallOptions &options,
        SendRequestType sendRequestType));

    struct WaitingFiber : public eastl::intrusive_list_node
    {
        Fiber::EventHandle eventHandle;
        OutboundHttpConnection **connection;
    };
    typedef eastl::intrusive_list<WaitingFiber> WaitingFiberList;

    typedef MovingAverage<float> TxnAverage;
    typedef MovingTimeAverage<float> TimedAverage;

    void trackConnectionAcquire();

    bool checkHttpRequestRate();

    eastl::string mName;
    bool mShutdown;

    InetAddress mAddress;

    Metrics::MetricsCollection& mMetricsCollection;
    Metrics::Gauge mPoolSize;
    Metrics::Gauge mMaxPoolSize;
    Metrics::Gauge mPeakConns;
    Metrics::Gauge mConnsInUse;
    bool mSecure;
    bool mSecureVerifyPeer;
    HttpServiceConnection::SSLPathType mSslPathType;
    eastl::string mVaultPath;
    eastl::string mPrivateCert;
    eastl::string mPrivateKey;
    eastl::string mPrivateKeyPassphrase;
    eastl::string mDefaultContentType;
    eastl::string mSslContextName;
    SslVersion mSslVersion;
    SslContextPtr mSslContext;
    GetRateLimitCb* mGetRateLimitCb;
    uint32_t mRateLimitCounter;
    EA::TDF::TimeValue mCurrentRateInterval;
    EA::TDF::TimeValue mOutboundMetricsThreshold;
    EA::TDF::TimeValue mConnectionTimeout;
    EA::TDF::TimeValue mRequestTimeout;
    EA::TDF::TimeValue mHandleCompletedRequestsTimeout;

    ConnectionList mActiveConnections;
    ConnectionList mInactiveConnections;

    WaitingFiberList mWaitingFibers;

    TimerId mTimeId;

    int32_t mNumRunningHandles;
    int32_t mPrevRunningHandles;
    void handleCompletedRequests();

    Dispatcher<OutboundHttpConnectionListener> mConnectionDispatcher;
    
    Metrics::TaggedTimer<HttpProxyCommand> mCallTimer;
    Metrics::TaggedCounter<HttpProxyCommand, uint32_t> mCallErrors;

    Metrics::Counter mMetricTotalTimeouts;
    uint32_t mRefCount;

    HttpStatusCodeSet mHttpStatusCodeErrSet;

    EA::TDF::ComponentId mOwningComponentId;

    bool mIsManaged;
    
    struct HttpCachedResponseListNode : public eastl::intrusive_list_node {};
    struct HttpCachedResponseHashMapNode : public eastl::intrusive_hash_node_key<eastl::string> {};

    class HttpCachedResponse :
        public HttpCachedResponseListNode, 
        public HttpCachedResponseHashMapNode
    {
    public:
        HttpCachedResponse(bool noCache, bool mustRevalidate, const eastl::string& cachedKey, OutboundHttpResult *result,
            eastl::string version, EA::TDF::TimeValue cacheTime);

        void setHttpRequest(const eastl::string& val) { mKey = val; }
        eastl::string& getHttpRequest() { return mKey; }

        eastl::string mETagVersion;         // (Optional) Technically, the "If-None-Match: <ETag>" header.
        EA::TDF::TdfBlobPtr mRawBuffer;     // Response Buffer.  May store binary data, despite being an Http response.  Uses a Ptr for ref counting. 
        EA::TDF::TimeValue mCacheDuration;  // (Optional) When does this cache value become stale?  (Values are only removed when pushed out of cache.)
        EA::TDF::TimeValue mCacheStartTime; // (Optional) When did the cache value get added
        bool mNoCache; // (Optional) If set, indicates that the cached value can only be used after getting a 304 response. Used when "no-cache" is returned in the response.   
        bool mMustRevalidate; // (Optional) If set, indicates that stale cache entries cannot be used. 

        size_t getResponseLength()
        {
            return mETagVersion.size() + mRawBuffer->getCount();
        }
    };

    typedef eastl::intrusive_hash_map<eastl::string, HttpCachedResponseHashMapNode, 100> HttpCachedResponseByHttpRequestMap;
    HttpCachedResponseByHttpRequestMap mHttpCachedResponseByReqMap;

    typedef eastl::intrusive_list<HttpCachedResponseListNode> HttpCachedResponseByAccessList;
    // Cache evicts from list front, insert to list back
    HttpCachedResponseByAccessList mHttpCachedResponseByAccessList;

    size_t mMaxHttpCacheMemInBytes;
    uint64_t mTotalHttpCacheMemAllocInBytes;

    size_t mMaxHttpResponseSizeInBytes;

    bool checkCacheControlHeaders(HttpCachedResponse& cacheEntry, const char8_t* httpHeaders[], uint32_t headerCount, bool& wasStale, bool& noStore);

    void removeHttpCacheIfExists(const eastl::string& cachedKey);
    void removeHttpCacheResponse(OutboundHttpConnectionManager::HttpCachedResponse& node);
    void updateHttpCache(OutboundHttpResult *result, HttpProtocolUtil::HttpMethod method, const eastl::string& cachedKey, const eastl::string& censoredCacheKey, bool staleIfError, bool requestSaysNoStore, EA::TDF::TdfBlobPtr& rawCachePtr);

    bool isHttpCacheEnabled() const;
    bool isHTTPMethodForFetch(HttpProtocolUtil::HttpMethod method) const;

    friend void intrusive_ptr_add_ref(OutboundHttpConnectionManager* ptr);
    friend void intrusive_ptr_release(OutboundHttpConnectionManager* ptr);

    NON_COPYABLE(OutboundHttpConnectionManager);
};

typedef eastl::intrusive_ptr<OutboundHttpConnectionManager> HttpConnectionManagerPtr;
typedef eastl::vector<HttpConnectionManagerPtr> HttpConnectionManagerList;

inline void intrusive_ptr_add_ref(OutboundHttpConnectionManager* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(OutboundHttpConnectionManager* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

class OutboundHttpConnectionListener
{
public:
    virtual void onNoActiveConnections(OutboundHttpConnectionManager& connMgr) = 0;
    virtual ~OutboundHttpConnectionListener() {}
};

} // Blaze

#endif // BLAZE_OUTBOUNDHTTPCONNECTIONMANAGER_H

