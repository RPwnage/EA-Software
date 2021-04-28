/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPXMLPROTOCOL_H
#define BLAZE_HTTPXMLPROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/rpcprotocol.h"
#include <EASTL/fixed_string.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class HttpXmlProtocol : public RpcProtocol
{
public:
    HttpXmlProtocol();
    ~HttpXmlProtocol() override;

    Type getType() const override { return Protocol::HTTPXML; }

    void setMaxFrameSize(uint32_t maxFrameSize) override;
    bool receive(RawBuffer& buffer, size_t& needed) override;
    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;

    uint32_t getFrameOverhead() const override { return HEADER_SIZE; }
    const char8_t* getName() const override { return HttpXmlProtocol::getClassName(); }
    bool isPersistent() const override { return false; }
    bool supportsNotifications() const override { return false; }

    uint32_t getNextSeqno() override { return mNextSeqno++; }

    ClientType getDefaultClientType() const override { return CLIENT_TYPE_HTTP_USER; }

    static const char8_t* getClassName() { return "httpxml"; }
    static void setDefaultEnumEncodingIsValue(bool defaultEnumEncodingIsValue) { mDefaultEnumEncodingIsValue = defaultEnumEncodingIsValue; }
    static void setDefaultXmlOutputIsElements(bool defaultXmlOutputIsElements) { mDefaultXmlOutputIsElements = defaultXmlOutputIsElements; }
    static void setUseCompression(bool useCompression) { sUseCompression = useCompression; }
    static void setCompressionLevel(int32_t compressionLevel) { sCompressionLevel = compressionLevel; }
    static const char8_t* getContentTypeFromEncoderType(const Blaze::Encoder::Type encoderType);
    static Blaze::Encoder::Type getEncoderTypeFromMIMEType(const char8_t* mimeType, Encoder::Type defaultEncoderType = Encoder::INVALID);

protected:
    static const uint32_t HEADER_SIZE = 512;
    bool isCompressionEnabled() const { return mIsCompressionEnabled; }

private:
    static const char8_t* CONTENT_ENCODING_STRING;
    static bool mDefaultEnumEncodingIsValue;
    static bool mDefaultXmlOutputIsElements;
    static bool sUseCompression;
    static int32_t sCompressionLevel;

    uint32_t mNextSeqno;
    enum { HTTP_UNKNOWN, HTTP_GET, HTTP_POST } mMethod;

    int32_t mContentLength;

    bool mIsCompressionEnabled;

    uint32_t mMaxFrameSize;

    bool compressRawBuffer(RawBuffer& responseBuffer, StringBuilder* responseHeader = nullptr);

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor 
    HttpXmlProtocol(const HttpXmlProtocol& otherObj);

    // Assignment operator
    HttpXmlProtocol& operator=(const HttpXmlProtocol& otherObj);
};

} // Blaze

#endif // BLAZE_HTTPXMLPROTOCOL_H

