/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/eventxmlprotocol.h"
#include "framework/rpc/eventslave.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
bool EventXmlProtocol::sCompressNotifications = false;

/*** Public Methods ******************************************************************************/

EventXmlProtocol::EventXmlProtocol()
{
}

void EventXmlProtocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
{
    HttpXmlProtocol::process(buffer, frame, decoder);

    if (frame.componentId != Event::EventSlave::COMPONENT_ID)
    {
        // Restrict incoming RPCs to calls to the event component
        frame.componentId = RPC_COMPONENT_UNKNOWN;
        frame.commandId = RPC_COMMAND_UNKNOWN;
    }
}

void EventXmlProtocol::framePayload(
        RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf /*= nullptr*/)
{
    if (tdf != nullptr)
    {
        encoder.encode(buffer, *tdf, &frame);
    }

    HttpXmlProtocol::framePayload(buffer, frame, (compressNotifications() && frame.messageType == RpcProtocol::NOTIFICATION ? dataSize : -1), encoder);

}

} // Blaze

