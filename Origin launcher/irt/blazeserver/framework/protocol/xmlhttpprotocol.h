/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMLHTTPPROTOCOL_H
#define BLAZE_XMLHTTPPROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/rpcprotocol.h"
#include "framework/util/xmlbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class XmlHttpProtocol : public RpcProtocol, XmlCallback
{
public:
    XmlHttpProtocol();
    ~XmlHttpProtocol() override;

    Type getType() const override { return Protocol::XMLHTTP; }

    void setMaxFrameSize(uint32_t maxFrameSize) override;
    bool receive(RawBuffer& buffer, size_t& needed) override;
    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;

    uint32_t getFrameOverhead() const override { return HEADER_SIZE; }
    const char8_t* getName() const override { return XmlHttpProtocol::getClassName(); }
    bool isPersistent() const override { return false; }
    bool supportsNotifications() const override { return false; }

    uint32_t getNextSeqno() override { return mNextSeqno++; }

    ClientType getDefaultClientType() const override { return CLIENT_TYPE_HTTP_USER; }

    static const char8_t* getClassName() { return "xmlhttp"; }

    // XmlCallback implementations for parsing
    void startDocument() override;
    void endDocument() override;
    void startElement(const char *name, size_t nameLen, const XmlAttribute *attributes, size_t attributeCount) override;
    void endElement(const char *name, size_t nameLen) override;
    void characters(const char *characters, size_t charLen) override;
    void cdata(const char *data, size_t dataLen) override;

protected:
    static const uint32_t HEADER_SIZE = 256;

private:
    uint32_t mNextSeqno;
    int32_t mContentLength;
    
    bool parseErrorCode;
    uint32_t mErrorCode;
    enum { XML_UNKNOWN, XML_REPLY, XML_REPLY_ERROR } mReply;

    uint32_t mMaxFrameSize;

    // The following two methods are intentionally private and unimplemented to prevent passing
    // and returning objects by value.  The compiler will give a linking error if either is used.

    // Copy constructor
    XmlHttpProtocol(const XmlHttpProtocol& otherObj);

    // Assignment operator
    XmlHttpProtocol& operator=(const XmlHttpProtocol& otherObj);
};

} // Blaze

#endif // BLAZE_XMLHTTPPROTOCOL_H

