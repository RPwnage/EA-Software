/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RESTPROTOCOL_H
#define BLAZE_RESTPROTOCOL_H

/*** Include files *******************************************************************************/

#include "framework/protocol/rpcprotocol.h"
#include "framework/protocol/shared/httpprotocolutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class RestProtocol : public RpcProtocol
{
public:
    RestProtocol();
    ~RestProtocol() override;

    Type getType() const override { return Protocol::REST; }

    void setMaxFrameSize(uint32_t maxFrameSize) override;
    bool receive(RawBuffer& buffer, size_t& needed) override;
    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;
    const char8_t* getName() const override { return RestProtocol::getClassName(); }
    bool isPersistent() const override { return false; }
    bool supportsNotifications() const override { return false; }
    uint32_t getFrameOverhead() const override { return HEADER_SIZE; }

    uint32_t getNextSeqno() override { return mNextSeqno++; }

    ClientType getDefaultClientType() const override { return CLIENT_TYPE_HTTP_USER; }

    static const char8_t* getClassName() { return "rest"; }
    static void setUseCompression(bool useCompression) { sUseCompression = useCompression; }
    static void setCompressionLevel(int32_t compressionLevel) { sCompressionLevel = compressionLevel; }
    static int32_t getCompressionLevel() { return sCompressionLevel; }

protected:
    static const uint32_t HEADER_SIZE = 512;

private:
    static bool sUseCompression;
    static int32_t sCompressionLevel;

    uint32_t mNextSeqno;
    HttpProtocolUtil::HttpMethod mMethod;

    int32_t mContentLength;
    
    bool mIsCompressionEnabled;

    uint32_t mMaxFrameSize;
    bool compressRawBuffer(RawBuffer& responseBuffer, char8_t* header, size_t headerLen);

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor 
    RestProtocol(const RestProtocol& otherObj);

    // Assignment operator
    RestProtocol& operator=(const RestProtocol& otherObj);
};

} // Blaze

#endif // BLAZE_RESTHTTPXMLPROTOCOL_H

