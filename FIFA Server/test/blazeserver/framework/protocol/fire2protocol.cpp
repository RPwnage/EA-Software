/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Fire2Protocol

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
#include "framework/protocol/fire2protocol.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/tdf/protocol.h"
#include "blazerpcerrors.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


Fire2Protocol::Fire2Protocol()
    : mState(READ_HEADER),
      mNextSeqno(Fire2Frame::MSGNUM_INVALID),
      mMaxFrameSize(UINT16_MAX+1)
{
}

Fire2Protocol::~Fire2Protocol()
{
}

void Fire2Protocol::setMaxFrameSize(uint32_t maxFrameSize)
{
    mMaxFrameSize = maxFrameSize;
}

bool Fire2Protocol::receive(RawBuffer& buffer, size_t& needed)
{
    Fire2Frame header;
    size_t available = buffer.datasize();
    if (mState == READ_HEADER)
    {
        if (available < Fire2Frame::HEADER_SIZE)
        {
            needed = Fire2Frame::HEADER_SIZE - available;
            return true;
        }

        // Got a complete header so decode it into host order
        header.setBuffer(buffer.data());

        if (header.getTotalSize() > mMaxFrameSize)
        {
            BLAZE_WARN_LOG(Log::FIRE, "[Fire2Protocol].receive: Incoming frame is too large (" << header.getTotalSize() << "); " <<
                    "max frame size allowed is " << mMaxFrameSize << ", " <<
                    "component " << BlazeRpcComponentDb::getComponentNameById(header.getComponent()) << "(" << header.getComponent() << "), " <<
                    "command " << BlazeRpcComponentDb::getCommandNameById(header.getComponent(), header.getCommand()) << "(" << header.getCommand() <<")");
            needed = 0;
            return false;
        }

        mState = READ_BODY;
    }
    else
    {
        header.setBuffer(buffer.data());
    }

    if (mState == READ_BODY)
    {
        if (available < header.getTotalSize())
        {
            needed = (header.getTotalSize() - available);
            return true;
        }

        // Entire frame has been read so clear our state and return the frame
        mState = READ_HEADER;
        buffer.trim(available - header.getTotalSize());

        BLAZE_TRACE_LOG(Log::FIRE, "[Fire2Protocol].receive: type=" << header.getMsgType() << " size=" << header.getPayloadSize() 
                << " component=" << SbFormats::Raw("0x%04x", header.getComponent()) << " command=" << header.getCommand()
                << " user=" << header.getUserIndex() << " seqno=" << header.getMsgNum() << " options=0x" << SbFormats::HexLower(header.getOptions()));
        needed = 0;
        return true;
    }

    needed = Fire2Frame::HEADER_SIZE;
    return true;
}

void Fire2Protocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
{
    // Extract the component and command from the request and setup the payload for decoding
    Fire2Frame header(buffer.data());
    buffer.pull(Fire2Frame::HEADER_SIZE);

    frame.componentId = header.getComponent();
    frame.commandId = header.getCommand();
    frame.msgNum = header.getMsgNum();
    frame.userIndex = header.getUserIndex();
    frame.immediate = header.isImmediate();
    frame.requestEncodingType = decoder.getType();
    switch (header.getMsgType())
    {
        case Fire2Frame::MESSAGE:
            frame.messageType = RpcProtocol::MESSAGE;
            break;
        case Fire2Frame::REPLY:
            frame.messageType = RpcProtocol::REPLY;
            break;
        case Fire2Frame::NOTIFICATION:
            frame.messageType = RpcProtocol::NOTIFICATION;
            break;
        case Fire2Frame::ERROR_REPLY:
            frame.messageType = RpcProtocol::ERROR_REPLY;
            break;
        case Fire2Frame::PING:
            frame.messageType = RpcProtocol::PING;
            break;
        case Fire2Frame::PING_REPLY:
            frame.messageType = RpcProtocol::PING_REPLY;
            break;
    }

    if (header.getMetadataSize() > 0)
    {
        // The decoder will pull() the mData pointer all the way to mTail, so we temporarily adjust
        // the mTail so that it is at the end of the metadata.  Then once we're done decoding we need
        // to put() the mTail back were it was.
        buffer.trim(header.getPayloadSize());

        Fire2Metadata metadata;
        if (decoder.decode(buffer, metadata, true) == ERR_OK)
        {
            frame.context = metadata.getContext();
            frame.errorCode = metadata.getErrorCode();
            frame.movedTo = metadata.getMovedTo();
            frame.movedToAddr.setIp(metadata.getMovedToAddr().getIp(), InetAddress::Order::HOST);
            frame.movedToAddr.setPort(metadata.getMovedToAddr().getPort(), InetAddress::Order::HOST);
            frame.movedToAddr.setHostname(metadata.getMovedToHostName());
            metadata.getLogContext().copyInto(frame.logContext);
            frame.superUserPrivilege = metadata.getFlags().getHasSuperUserPriviledge();
            if (metadata.isSessionKeySet())
                frame.setSessionKey(metadata.getSessionKey());
            if (metadata.isSliverIdentitySet())
                frame.sliverIdentity = metadata.getSliverIdentity();
        }

        buffer.put(header.getPayloadSize());
    }
}

void Fire2Protocol::framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf)
{
    if (tdf != nullptr)
    {
        encoder.encode(buffer, *tdf, &frame);
        dataSize = buffer.datasize();
    }

    Fire2Frame::MessageType msgType;
    switch (frame.messageType)
    {
        case RpcProtocol::MESSAGE:
            msgType = Fire2Frame::MESSAGE;
            break;
        case RpcProtocol::REPLY:
            msgType = Fire2Frame::REPLY;
            break;
        case RpcProtocol::NOTIFICATION:
            msgType = Fire2Frame::NOTIFICATION;
            break;
        case RpcProtocol::ERROR_REPLY:
            msgType = Fire2Frame::ERROR_REPLY;
            break;
        case RpcProtocol::PING:
            msgType = Fire2Frame::PING;
            break;
        case RpcProtocol::PING_REPLY:
            msgType = Fire2Frame::PING_REPLY;
            break;
        default:
            EA_FAIL();
            return;
    }

    uint8_t options = 0;
    if (frame.immediate)
        options |= Fire2Frame::OPTION_IMMEDIATE;

    uint8_t metadataBuf[Fire2Frame::METADATA_SIZE_MAX];
    RawBuffer metadataRawBuf(metadataBuf, sizeof(metadataBuf));
    const char8_t* sessionKey = frame.getSessionKey();
    if ((frame.context != 0) || (frame.errorCode != ERR_OK) || (sessionKey != nullptr) || (frame.sliverIdentity != INVALID_SLIVER_IDENTITY) || frame.superUserPrivilege)
    {
        Fire2Metadata metadata;
        metadata.setContext(frame.context);
        metadata.setErrorCode(frame.errorCode);
        if (sessionKey != nullptr)
            metadata.setSessionKey(sessionKey);
        if (frame.sliverIdentity != INVALID_SLIVER_IDENTITY)
            metadata.setSliverIdentity(frame.sliverIdentity);
        if (frame.superUserPrivilege)
            metadata.getFlags().setHasSuperUserPriviledge(true);

        metadata.setLogContext(const_cast<LogContextWrapper&>(frame.logContext));

        if (frame.errorCode == ERR_MOVED)
        {
            metadata.setMovedTo(frame.movedTo);
            if (frame.movedToAddr.isValid())
            {
                metadata.getMovedToAddr().setIp(frame.movedToAddr.getIp(InetAddress::HOST));
                metadata.getMovedToAddr().setPort(frame.movedToAddr.getPort(InetAddress::HOST));
                metadata.setMovedToHostName(frame.movedToAddr.getHostname());
            }
        }

        if (encoder.encode(metadataRawBuf, metadata, nullptr, true))
        {
            memcpy(buffer.push(metadataRawBuf.datasize()), metadataRawBuf.data(), metadataRawBuf.datasize());
        }
        else
        {
            BLAZE_ERR_LOG(Log::FIRE, "[Fire2Protocol].framePayload: Failed to encode the metadata, "
                "context(" << metadata.getContext() << "), errorCode(" << metadata.getErrorCode() << "), sessionKey(" << metadata.getSessionKey() << ")");
            metadataRawBuf.reset();
        }
    }

    Fire2Frame outFrame(
        buffer.push(Fire2Frame::HEADER_SIZE),
        (uint32_t)dataSize, metadataRawBuf.datasize(),
        frame.componentId, frame.commandId,
        frame.msgNum, msgType,
        frame.userIndex, options);
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

