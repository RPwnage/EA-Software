/*************************************************************************************************/
/*!
    \file httpparam.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPOUTBOUNDRESULT_H
#define BLAZE_HTTPOUTBOUNDRESULT_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/connection/outboundhttpconnection.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/



namespace Blaze
{

struct XmlAttribute;
class RawBuffer;
class XmlBuffer;

static const HttpStatusCode HTTP_STATUS_CONTINUE = 100;
static const HttpStatusCode HTTP_STATUS_OK = 200;
static const HttpStatusCode HTTP_STATUS_SUCCESS = 201;
static const HttpStatusCode HTTP_STATUS_SUCCESS_END = 206;
static const HttpStatusCode HTTP_STATUS_FOUND = 302;
static const HttpStatusCode HTTP_STATUS_NOT_MODIFIED = 304;
static const HttpStatusCode HTTP_STATUS_BAD_REQUEST = 400;
static const HttpStatusCode HTTP_STATUS_UNAUTHORIZED = 401;
static const HttpStatusCode HTTP_STATUS_FORBIDDEN = 403;
static const HttpStatusCode HTTP_STATUS_NOT_FOUND = 404;
static const HttpStatusCode HTTP_STATUS_CONFLICT = 409;
static const HttpStatusCode HTTP_STATUS_CLIENT_ERROR_START = 400;
static const HttpStatusCode HTTP_STATUS_CLIENT_ERROR_END = 499;
static const HttpStatusCode HTTP_STATUS_SERVER_ERROR_START = 500;
static const HttpStatusCode HTTP_STATUS_SERVER_ERROR_END = 599;

/*! ************************************************************/
/*! \class 
    This class defines an interface for processing raw http results.
*/
class OutboundHttpResult : public OutboundHttpResponseHandler
{
    NON_COPYABLE(OutboundHttpResult);

public:
    OutboundHttpResult();
    ~OutboundHttpResult() override;

    virtual BlazeRpcError decodeResult(RawBuffer& response);

    virtual void startElement(const char8_t* fullname, size_t nameLen) {}
    virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) = 0;
    virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) = 0;
    virtual void endElement(const char8_t* fullname, size_t nameLen) {}

    virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) = 0;
    virtual bool checkSetError(const char8_t* fullname, const char8_t* data, size_t dataLen) { return false; };

    virtual void setUserSecurityState(const char8_t* securityState) {};

    static const char8_t* const lookupHttpStatusReason(const HttpStatusCode statusCode);

    virtual RawBuffer& getBuffer() { return mRawBuffer; }
    uint32_t processRecvData(const char8_t* data, uint32_t size, bool isResultInCache = false) override;

protected:
    uint32_t processSendData(char8_t* dest, uint32_t size) override;    
    BlazeRpcError requestComplete(CURLcode res) override;

    RawBuffer mRawBuffer;

private:
    bool decompressRawBuffer();


    uint32_t mDecompressedOutputSize;
    uint8_t* mDecompressedOutput;
};


class RawOutboundHttpResult : public OutboundHttpResult
{
    NON_COPYABLE(RawOutboundHttpResult);
public:
    RawOutboundHttpResult() {}
    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override {}
    void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) override {}
    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override {}
    bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) override
    {
        return false;
    }
};


class OutboundHttpXmlResult : public RawOutboundHttpResult
{

public: 
    OutboundHttpXmlResult(eastl::string& buf) : mBuf(buf) {}
    ~OutboundHttpXmlResult() override {};

    BlazeRpcError decodeResult(RawBuffer& response) override;

    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override {}
    void setAttributes(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override {}
    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override {}
    bool checkSetError(const char8_t* fullname,
        const Blaze::XmlAttribute* attributes, size_t attributeCount) override { return false; }

private:
    eastl::string& mBuf;
};

} // Blaze

#endif // BLAZE_HTTPOUTBOUNDRESULT_H
