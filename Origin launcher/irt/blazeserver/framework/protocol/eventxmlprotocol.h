/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EVENTXMLPROTOCOL_H
#define BLAZE_EVENTXMLPROTOCOL_H

/*** Include files *******************************************************************************/
#include "framework/protocol/httpxmlprotocol.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class EventXmlProtocol : public HttpXmlProtocol
{
public:
    EventXmlProtocol();
    
    Type getType() const override { return Protocol::EVENTXML; }

    bool isPersistent() const override { return true; }
    bool supportsNotifications() const override { return true; }
    bool compressNotifications() const override { return isCompressionEnabled() && sCompressNotifications; }
    static void setCompressNotifications(bool compressEventNotifications) { sCompressNotifications = compressEventNotifications; }

    const char8_t* getName() const override { return EventXmlProtocol::getClassName(); }
    static const char8_t* getClassName() { return "eventxml"; }

    void process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder) override;
    void framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf = nullptr) override;

private:
    static bool sCompressNotifications;
};

} // Blaze

#endif // BLAZE_EVENTXMLPROTOCOL_H

