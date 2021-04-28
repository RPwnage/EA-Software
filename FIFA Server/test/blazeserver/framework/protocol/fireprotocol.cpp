/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FireProtocol

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/connection.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/fireprotocol.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "blazerpcerrors.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


void Protocol::setupBuffer(RawBuffer& buffer) const
{
    buffer.put(getFrameOverhead());
    buffer.pull(getFrameOverhead());
}

FireProtocol::FireProtocol()
    : mState(READ_HEADER),
      mNextSeqno(0),
      mMaxFrameSize(UINT16_MAX+1)
{
}

FireProtocol::~FireProtocol()
{
}

void FireProtocol::setMaxFrameSize(uint32_t maxFrameSize)
{
    mMaxFrameSize = maxFrameSize;
}

bool FireProtocol::receive(RawBuffer& buffer, size_t& needed)
{
    FireFrame header;
    size_t available = buffer.datasize();
    if (mState == READ_HEADER)
    {
        if (available < FireFrame::HEADER_SIZE)
        {
            needed = FireFrame::HEADER_SIZE - available;
            return true;
        }

        // Got a complete header (possibly, we may still need the extra header data) so decode it into host order
        header.setBuffer(buffer.data());
        mHeaderOffset = buffer.data() - buffer.head();
        buffer.pull(FireFrame::HEADER_SIZE);
        available -= FireFrame::HEADER_SIZE;

        mState = ((header.getOptions()
                    & (FireFrame::OPTION_HAS_CONTEXT | FireFrame::OPTION_JUMBO_FRAME)) != 0)
            ? READ_EXTRA : READ_BODY;
    }
    else
    {
        header.setBuffer(buffer.head() + mHeaderOffset);
    }

    if (mState == READ_EXTRA)
    {
        size_t extraSize = header.getExtraSize();
        if (available < extraSize)
        {
            needed =  extraSize - available;
            return true;
        }

        //We can read the frame's extra data now - this just means pulling more bytes off the
        //buffer since the frame already has a pointer to it.
        buffer.pull(extraSize);
        available -= extraSize;

        mState = READ_BODY;
    }

    if (mState == READ_BODY)
    {
        // Important!  We must be in the READ_BODY state to do this check!  header.getSize() will
        // look at the EXTRA frame data before we've read it otherwise.
        if (header.getSize() > mMaxFrameSize)
        {
            BLAZE_WARN_LOG(Log::FIRE, "[FireProtocol].receive: Incoming READ_BODY frame is too large (" << header.getSize() << "); "
                    "max frame size allowed is " << mMaxFrameSize << ", component(" << header.getComponent() << "), cmd(" << header.getCommand() <<"), ");
            needed = 0;
            return false;
        }

        if (available < header.getSize())
        {
            needed = (header.getSize() - available);
            return true;
        }

        // Entire frame has been read so clear our state and return the frame
        mState = READ_HEADER;
        buffer.push(header.getHeaderSize());
        buffer.trim(available - header.getSize());

        BLAZE_TRACE_LOG(Log::FIRE, "[FireProtocol].receive: type=" << header.getMsgType() << " size=" << header.getSize() 
                << " component=" << SbFormats::Raw("0x%04x", header.getComponent()) << " command=" << header.getCommand() << " ec=" << header.getErrorCode() 
                << " user=" << header.getUserIndex() << " seqno=" << header.getMsgNum() << " options=0x" << SbFormats::HexLower(header.getOptions()));
        needed = 0;
        return true;
    }

    needed = FireFrame::HEADER_SIZE;
    return true;
}

void FireProtocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
{
    // Extract the component and command from the request and setup the payload for decoding
    FireFrame header(buffer.data());
    buffer.pull(header.getHeaderSize());

    frame.componentId = header.getComponent();
    frame.commandId = header.getCommand();
    frame.msgNum = header.getMsgNum();
    frame.userIndex = header.getUserIndex();
    frame.errorCode = (uint32_t) BLAZE_ERROR_FROM_CODE(frame.componentId, header.getErrorCode());
    frame.context = header.hasContext() ? header.getContext() : 0;
    frame.immediate = header.isImmediate();
    frame.requestEncodingType = decoder.getType();
    switch (header.getMsgType())
    {
        case FireFrame::MESSAGE:
            frame.messageType = RpcProtocol::MESSAGE;
            break;
        case FireFrame::REPLY:
            frame.messageType = RpcProtocol::REPLY;
            break;
        case FireFrame::NOTIFICATION:
            frame.messageType = RpcProtocol::NOTIFICATION;
            break;
        case FireFrame::ERROR_REPLY:
            frame.messageType = RpcProtocol::ERROR_REPLY;
            break;
    }
}

void FireProtocol::framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf /*= nullptr*/)
{
    if (tdf != nullptr)
    {
        encoder.encode(buffer, *tdf, &frame);
        dataSize = buffer.datasize();
    }

    FireFrame::MessageType msgType;
    switch (frame.messageType)
    {
        case RpcProtocol::MESSAGE:
            msgType = FireFrame::MESSAGE;
            break;
        case RpcProtocol::REPLY:
            msgType = FireFrame::REPLY;
            break;
        case RpcProtocol::NOTIFICATION:
            msgType = FireFrame::NOTIFICATION;
            break;
        case RpcProtocol::ERROR_REPLY:
            msgType = FireFrame::ERROR_REPLY;
            break;
        default:
            EA_FAIL();
            return;
    }
    
    //Calc the header size by adding the context if we use it.
    size_t headerSize = FireFrame::HEADER_SIZE;
    uint32_t options = frame.immediate ? (uint32_t)FireFrame::OPTION_IMMEDIATE : (uint32_t)FireFrame::OPTION_NONE;
    if (dataSize > UINT16_MAX)
    {
        options |= (uint32_t)FireFrame::OPTION_JUMBO_FRAME;
        headerSize += FireFrame::JUMBO_SIZE;
    }
    if (frame.context != 0)
    {
        options |= (uint32_t)FireFrame::OPTION_HAS_CONTEXT;
        uint32_t contextSize = FireFrame::SMALL_CONTEXT_SIZE;
        if ((frame.context & (uint32_t)-1) != frame.context)
        {
            options |= (uint32_t)FireFrame::OPTION_JUMBO_CONTEXT;
            contextSize = FireFrame::JUMBO_CONTEXT_SIZE;
        }
        headerSize += contextSize;
    }
    FireFrame outFrame(buffer.push(headerSize), (uint32_t) dataSize, frame.componentId,
        frame.commandId, BLAZE_CODE_FROM_ERROR(frame.errorCode), msgType, frame.userIndex, 
        options, frame.msgNum, frame.context);
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

