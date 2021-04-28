/*************************************************************************************************/
/*!
    \file restprotocolutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RESTPROTOCOLUTIL_H
#define BLAZE_RESTPROTOCOLUTIL_H

/*** Include files *******************************************************************************/

#include "framework/component/blazerpc.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/protocol/shared/restrequestbuilder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if defined(_WIN32) || defined(WIN32)
    #define snprintf _snprintf
#endif

namespace Blaze
{

class RestOutboundHttpResult : public OutboundHttpResult
{
public:
    RestOutboundHttpResult(const RestResourceInfo* restInfo, EA::TDF::Tdf* responseTdf, EA::TDF::Tdf* errorTdf, HttpHeaderMap* headerMap, RawBuffer* responseRaw) :
       mRestInfo(restInfo), mResponseTdf(responseTdf), mErrorTdf(errorTdf), mHeaderMap(headerMap), mStatusCode(0), mErrorCode(ERR_SYSTEM), mResponseRaw(responseRaw) { }
    ~RestOutboundHttpResult() override { }

public:
    BlazeRpcError decodeResult(RawBuffer& response) override;
    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override {}
    void setAttributes(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override {}
    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override {}
    bool checkSetError(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override { return false; }
    HttpStatusCode getStatusCode() const { return mStatusCode; }
    BlazeRpcError getBlazeErrorCode() const { return mErrorCode; }
    uint32_t processRecvData(const char8_t* data, uint32_t size, bool isResultInCache = false) override;
protected:
    CURL_DEBUG_SCRUB_TYPE getDebugScrubType(curl_infotype infoType) const override;

private:
    const RestResourceInfo* mRestInfo;
    EA::TDF::Tdf* mResponseTdf;
    EA::TDF::Tdf* mErrorTdf;
    HttpHeaderMap* mHeaderMap;
    HttpStatusCode mStatusCode;
    BlazeRpcError mErrorCode;
    RawBuffer* mResponseRaw;
};


class RawRestOutboundHttpResult : public OutboundHttpResult
{
public:
    RawRestOutboundHttpResult() {}
    ~RawRestOutboundHttpResult() override {}

    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override {}
    void setAttributes(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override {}
    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override {}
    bool checkSetError(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override { return false; }

    RawBuffer& getBuffer() override { return mRawBuffer; }

protected:
    BlazeRpcError requestComplete(CURLcode res) override;
};

class RestProtocolUtil : public RestRequestBuilder
{
public:
    static const char8_t HEADER_BLAZE_ERRORCODE[];

    static const RestResourceInfo* getRestResourceInfo(const char8_t* method, const char8_t* url);
    static const RestResourceInfo* getRestResourceInfo(uint16_t componentId, uint16_t commandId);
    static Encoder::Type getEncoderTypeFromAcceptHeader(const char8_t* acceptHeader);
    static void buildCustomHeaderMap(const HttpFieldMapping* headerTdfPair, size_t arrayCount, const EA::TDF::Tdf& tdf, HttpStringMap& stringMap, Encoder::SubType encSubType);
    static void createHeaderString(const HttpHeaderMap& headersMap, char8_t* customString, size_t customStringSize);
    static void printHttpRequest(uint16_t componentId, uint16_t commandId, const EA::TDF::Tdf* request, StringBuilder& sb);
    static void printHttpResponse(const StringBuilder &uri, const RestResourceInfo& restInfo, const HttpHeaderMap& headers, const EA::TDF::Tdf* responseTdf, StringBuilder& sb);

    // WARNING: This version of printHttpRequest does not implement any scrubbing. Use printHttpRequest(uint16_t, uint16_t, Tdf*, StringBuilder&) for requests that contain sensitive data.
    static void printHttpRequest(const StringBuilder &uri, const RestResourceInfo& restInfo, const HeaderVector& headers, const HttpParamVector& params, const EA::TDF::Tdf* requestTdf, const RawBuffer *payload, StringBuilder& sb);

    static BlazeRpcError sendHttpRequest(HttpConnectionManagerPtr connMgr, const RestResourceInfo* restInfo,
        const EA::TDF::Tdf* requestTdf, const char8_t* contentType, EA::TDF::Tdf* responseTdf = nullptr, EA::TDF::Tdf* errorTdf = nullptr, RawBuffer* payload = nullptr,
        HttpHeaderMap* responseHeaderMap = nullptr, const char8_t* uriPrefix = nullptr, HttpStatusCode* statusCode = nullptr,
        int32_t logCategory = HTTP_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr);

    static BlazeRpcError sendHttpRequest(HttpConnectionManagerPtr, ComponentId componentId, CommandId commandId,
        const EA::TDF::Tdf* requestTdf, const char8_t* contentType, bool addEncodedPayload, EA::TDF::Tdf* responseTdf = nullptr, EA::TDF::Tdf* errorTdf = nullptr, 
        HttpHeaderMap* responseHeaderMap = nullptr, const char8_t* uriPrefix = nullptr, HttpStatusCode* statusCode = nullptr,
        int32_t logCategory = HTTP_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr);


    static Blaze::HttpStatusCode parseSuccessStatusCode(const SuccessStatusCodeMapping* statusCodePair, size_t arrayCount, const EA::TDF::Tdf& requestTdf);
    static BlazeRpcError lookupBlazeRpcError(const StatusCodeErrorMapping* statusCodePair, size_t arrayCount, HttpStatusCode statusCode);

    static bool compressRawBuffer(RawBuffer& rawBuffer, char8_t* header, size_t headerSize, int32_t compressionLevel);

private:
    RestProtocolUtil();
    ~RestProtocolUtil() override { };


    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor
    RestProtocolUtil(const RestProtocolUtil& otherObj);

    // Assignment operator
    RestProtocolUtil& operator=(const RestProtocolUtil& otherObj);
};

} // Blaze

#endif // BLAZE_RESTPROTOCOLUTIL_H


