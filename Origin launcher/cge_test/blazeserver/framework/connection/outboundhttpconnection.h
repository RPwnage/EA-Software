/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDHTTPCONNECTION_H
#define BLAZE_OUTBOUNDHTTPCONNECTION_H

/*** Include files *******************************************************************************/

#include "framework/connection/connection.h"
#include "framework/connection/sslcontext.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/protocol/shared/httpprotocolutil.h"

#include "EASTL/intrusive_list.h"
#include "curl/curl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConnectionOwner;
class LibcurlSocketChannel;
class RawBuffer;
class Endpoint;
class OutboundHttpResult;
class OutboundHttpConnection;
class OutboundHttpConnectionManager;
class SslContext;

enum CURL_DEBUG_SCRUB_TYPE
{
    DEFAULT,
    HEADER,
    CENSOR_ALL,
    VAULT_PAYLOAD
};

class OutboundHttpResponseHandler
{
    friend class OutboundHttpConnection;
public:
    OutboundHttpResponseHandler() : mOwnerConnection(nullptr), mHttpStatusCode(0), mHeaderMap(BlazeStlAllocator("OutboundHttpResponseHandler::mHeaderMap")) {}
    virtual ~OutboundHttpResponseHandler() {}

    OutboundHttpConnection* getOwnerConnection() { return mOwnerConnection; }

    virtual const HttpStatusCode getHttpStatusCode() { return mHttpStatusCode; }
    void setHttpStatusCode(int32_t statusCode) { mHttpStatusCode = statusCode; }

    virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) = 0;

    const HttpHeaderMap& getHeaderMap() { return mHeaderMap; }
    const char8_t* getHeader(const char8_t* name)
    {
        HttpHeaderMap::iterator it;
        return ((it = mHeaderMap.find(name)) != mHeaderMap.end() ? it->second.c_str() : "");
    }

    virtual uint32_t processRecvData(const char8_t* data, uint32_t size, bool isResultInCache = false) = 0;

protected:
    virtual void processHeaders() {}
    virtual uint32_t processSendData(char8_t* data, uint32_t size) = 0;
    
    virtual BlazeRpcError requestComplete(CURLcode res) = 0;

    static CURL_DEBUG_SCRUB_TYPE getDefaultScrubType(curl_infotype infoType);
    virtual CURL_DEBUG_SCRUB_TYPE getDebugScrubType(curl_infotype infoType) const
    {
        return getDefaultScrubType(infoType);
    }

private:
    void setOwnerConnection(OutboundHttpConnection* ownerConnection) { mOwnerConnection = ownerConnection; }

    OutboundHttpConnection* mOwnerConnection;

    HttpStatusCode mHttpStatusCode;
    HttpHeaderMap mHeaderMap;
};

class OutboundHttpConnection : eastl::intrusive_list_node
{
    NON_COPYABLE(OutboundHttpConnection);

public:

    OutboundHttpConnection(OutboundHttpConnectionManager& owner);
    virtual ~OutboundHttpConnection();

    enum ContentPartType
    {
        INVALID,
        CONTENT,
        FILE
    };

    struct ContentPart
    {
        ContentPart()
            : mType(INVALID),
              mName(""),
              mContentType(HttpProtocolUtil::HTTP_POST_CONTENTTYPE),
              mFileName(""),
              mContent(""),
              mContentLength(0),
              mWriteOffset(0)
        {}
        
        ContentPart(ContentPartType partType)
            : mType(partType),
              mName(""),
              mContentType(HttpProtocolUtil::HTTP_POST_CONTENTTYPE),
              mFileName(""),
              mContent(""),
              mContentLength(0),
              mWriteOffset(0)
        {}

        ContentPartType mType;
        const char8_t* mName;
        const char8_t* mContentType;
        const char8_t* mFileName;
        const char8_t* mContent;
        uint32_t mContentLength;
        uint32_t mWriteOffset; // Offset into mContent where curl should grab next data chunk in send_body_callback

    private:
        ContentPart(const ContentPart&);
        ContentPart& operator=(const ContentPart&);
    };

    typedef eastl::vector<ContentPart*> ContentPartList;

    bool initialize(SslContextPtr sslContext);

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        int32_t logCategory = HTTP_LOGGER_CATEGORY,
        bool xmlPayload = false, const RpcCallOptions &options = RpcCallOptions()));

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        const char8_t* contentType, const char8_t* contentBody, uint32_t contentLength,
        int32_t logCategory = HTTP_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions()));

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        ContentPartList* content, int32_t logCategory = HTTP_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions()));

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult *result,
        ContentPartList* content, int32_t logCategory = HTTP_LOGGER_CATEGORY,
        bool xmlPayload = false, const char8_t* httpUserPwd = nullptr, uint16_t maxHandledUrlRedirects = 0, const RpcCallOptions &options = RpcCallOptions()));

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendStreamRequest(HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler,
        ContentPartList* content, int32_t logCategory = HTTP_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions()));

    virtual void close();
    virtual bool closeOnRelease(){return mCloseOnRelease;}

    //Easy Handle
    CURL *mEasyHandle;
    static size_t recv_header_callback(const void* ptr, size_t size, size_t nmemb, void *requestData);
    static size_t recv_body_callback(const void* ptr, size_t size, size_t nmemb, void *requestData);
    static size_t send_body_callback(void* ptr, size_t size, size_t nmemb, void *requestData);
    static int seek_callback(void* ptr, curl_off_t offset, int origin);
    static int debug_callback(const CURL*, curl_infotype, char *, size_t, void *);
    static int sockopt_callback(void* ptr, curl_socket_t curlfd, curlsocktype purpose);
    static CURLcode sslctx_callback(CURL *curl, void *sslctx, void *parm);

    LibcurlSocketChannel* createChannel(SOCKET sockfd);
    void destroyChannel(SOCKET sockfd);

    static const size_t INITIAL_BUFFER_SIZE = 1024;

    void processMessage(CURLcode res);

    OutboundHttpConnectionManager& getOwnerManager() { return mOwnerManager; }

protected:
        static HttpProtocolUtil::HttpReturnCode buildPayloadParametersInXml(RawBuffer& buffer, CURL *easyHandle, const HttpParam params[] = nullptr, uint32_t paramsize = 0);

private:
    friend class eastl::intrusive_list<OutboundHttpConnection>;

    bool decompressRawBuffer(RawBuffer& rawBuffer);

    void setEasyOpts(CURL* easyHandle, HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const HttpParam params[], uint32_t paramsize,
        const char8_t* httpHeaders[], uint32_t headerCount,
        ContentPartList* contentList, ContentPart*& content,
        char8_t*& url, char8_t*& paramString, struct curl_slist*& headerList,
        HttpParam*& uriEncodedParams, bool xmlPayload, const char8_t* httpUserPwd = nullptr, uint16_t maxHandledUrlRedirects = 0);

    void setEasyOpts(CURL* easyHandle, HttpProtocolUtil::HttpMethod method,
        const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount,
        ContentPartList* content,
        char8_t*& url, char8_t*& paramString, struct curl_slist*& headerList, 
        const char8_t* paramsStr = nullptr, uint32_t paramLength = 0, const char8_t* httpUserPwd = nullptr, uint16_t maxHandledUrlRedirects = 0);

    BlazeRpcError doSendRequest(CURL *easyHandle, HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize,
                                const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResponseHandler *handler, int32_t logCategory,
                                char8_t *url, char8_t *paramString, struct curl_slist *headerList, HttpParam *uriEncodedParams, const EA::TDF::TimeValue &timeoutOverride);

    static const char8_t* findFirstFiltered(const char8_t* data, uint32_t& filterIndex, uint32_t& jsonTagLength);
    static const char8_t* findJSONValueStart(const char8_t* keyStart, const char8_t* expectedKey, size_t expectedKeyLen);
    static const char8_t* findJSONValueEnd(const char8_t* valueStart);
    static bool writeStringToBuffer(RawBuffer& buf, const char8_t* data, size_t dataLen);
    static const char8_t* scrubUrlAndPayload(RawBuffer& buf, const char8_t* data, uint32_t dataSize);
    static const char8_t* censorHeaders(RawBuffer& buf, const char8_t* data, uint32_t dataSize);
    static const char8_t* censorVaultData(RawBuffer& buf, const char8_t* data, uint32_t dataSize);
    static void scrub(const char8_t* scrubLoc, const char8_t* endLoc, char8_t* scrubbed, size_t scrubbedLen);

    static void xmlEscapeString(const char8_t* sourceStr, size_t sourceStrSize, eastl::string& escapedStr);

    struct OutgoingRequest : public eastl::intrusive_list_node
    {
        Fiber::EventHandle eventHandle;
        LogContextWrapper logContext;
        CURLcode curlResult;
        int32_t statusCode;
        bool statusCodeReceived;
        CURL* easyHandle;
        uint32_t requestId;
        int32_t logCategory;
        OutboundHttpResponseHandler *handler;
    };
    typedef eastl::intrusive_list<OutgoingRequest> OutgoingRequests;
    static void resetStatusCode(OutgoingRequest& request);

    // List of outgoing requests
    OutgoingRequests mSentRequests;
    bool mCloseOnRelease;
    uint32_t mInstanceId;
    uint32_t mNextRequestId;

    static uint32_t mNextInstanceId;

    OutboundHttpConnectionManager& mOwnerManager;
    typedef eastl::intrusive_list<LibcurlSocketChannel> ChannelList;
    ChannelList mChannels;
    char8_t mCurlErrorBuffer[CURL_ERROR_SIZE];
};

} // Blaze

#endif // BLAZE_OUTBOUNDHTTPCONNECTION_H


