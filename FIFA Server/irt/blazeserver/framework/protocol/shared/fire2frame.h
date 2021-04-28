/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIRE2FRAME_H
#define BLAZE_FIRE2FRAME_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if defined(EA_PLATFORM_WINDOWS)
#include <winsock2.h>
#endif

#ifdef BLAZE_CLIENT_SDK
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#ifndef ntohs
#define ntohs SocketNtohs
#endif

#ifndef ntohl
#define ntohl SocketNtohl
#endif

#ifndef htons
#define htons SocketHtons
#endif

#ifndef htonl
#define htonl SocketHtonl
#endif
#else
#include "framework/connection/platformsocket.h"
#endif //BLAZE_CLIENT_SDK

namespace Blaze
{

class BLAZESDK_API Fire2Frame
{
    NON_COPYABLE(Fire2Frame);

public:
    static const uint16_t HEADER_SIZE       = 16;
    static const uint16_t METADATA_SIZE_MAX = 512;

    static const uint8_t OPTION_NONE        = 0x00;
    static const uint8_t OPTION_IMMEDIATE   = 0x01;
    static const uint8_t OPTION_MASK        = (OPTION_IMMEDIATE);

    static const uint32_t MSGNUM_MASK       = 0x00ffffff;
    static const uint32_t MSGNUM_INVALID    = 0xffffffff;

    typedef enum { MESSAGE = 0, REPLY = 1, NOTIFICATION = 2, ERROR_REPLY = 3, PING = 4, PING_REPLY = 5 } MessageType;

    inline Fire2Frame() : mFrame(nullptr) { }
    inline Fire2Frame(uint8_t* buf) : mFrame(buf) { }
    inline Fire2Frame(uint8_t* buf, uint32_t payloadSize, uint16_t metadataSize,
                      uint16_t component, uint16_t command,
                      uint32_t msgNum, MessageType msgType,
                      uint32_t userIndex, uint32_t options);

    inline void setBuffer(uint8_t* buf) { mFrame = buf; }

    inline uint64_t getTotalSize() const { return (uint64_t)getHeaderSize() + (uint64_t)getPayloadSize(); }
    inline uint32_t getHeaderSize() const { return (uint32_t)HEADER_SIZE + (uint32_t)getMetadataSize(); }
    inline uint32_t getPayloadSize() const;
    inline void setPayloadSize(uint32_t size);
    inline uint16_t getMetadataSize() const;
    inline void setMetadataSize(uint16_t size);
    inline uint16_t getComponent() const;
    inline uint16_t getCommand() const;
    inline uint32_t getMsgNum() const;
    inline MessageType getMsgType() const;
    inline uint32_t getUserIndex() const;
    inline uint8_t getOptions() const;
    inline void setOptions(uint8_t options);
    inline bool isImmediate() const;

    static const char* MessageTypeToString(MessageType type);
    static bool parseMessageType(const char8_t* typeStr, MessageType& type);
    static const char8_t* optionsToString(uint32_t options, char8_t* buf, size_t len);

    static uint32_t getNextMessageNum(uint32_t currentMessageNum)
    {
        // Reset back to 0 if we're now beyond available msgNum values.
        return (currentMessageNum + 1) & MSGNUM_MASK;
    }

private:
    uint8_t* mFrame;
};

inline Fire2Frame::Fire2Frame(uint8_t* buf, uint32_t payloadSize, uint16_t metadataSize,
                              uint16_t component, uint16_t command,
                              uint32_t msgNum, MessageType msgType,
                              uint32_t userIndex, uint32_t options)
    : mFrame(buf)
{
    // Size for the payload
    setPayloadSize(payloadSize);

    // Metadata size
    setMetadataSize(metadataSize);

    // ComponentId
    mFrame[6] = (uint8_t)(component >> 8);
    mFrame[7] = (uint8_t)(component >> 0);

    // CommandId
    mFrame[8] = (uint8_t)(command >> 8);
    mFrame[9] = (uint8_t)(command >> 0);

    // Message Number
    mFrame[10] = (uint8_t)(msgNum >> 16);
    mFrame[11] = (uint8_t)(msgNum >> 8);
    mFrame[12] = (uint8_t)(msgNum >> 0);

    // 3 bits for the MessageType, and 5 bits for the user index
    mFrame[13] = (uint8_t)((msgType << 5) | userIndex);

    // The option bits
    mFrame[14] = (uint8_t)(options);

    // Reserved byte for future use
    mFrame[15] = (uint8_t)(0);
}

inline uint32_t Fire2Frame::getPayloadSize() const
{
    return (uint32_t)((mFrame[0] << 24) | (mFrame[1] << 16) | (mFrame[2] << 8) | (mFrame[3] << 0));
}

inline void Fire2Frame::setPayloadSize(uint32_t size)
{
    mFrame[0] = (uint8_t)(size >> 24);
    mFrame[1] = (uint8_t)(size >> 16);
    mFrame[2] = (uint8_t)(size >> 8);
    mFrame[3] = (uint8_t)(size >> 0);
}

inline uint16_t Fire2Frame::getMetadataSize() const
{
    return (uint16_t)((mFrame[4] << 8) | (mFrame[5] << 0));
}

inline void Fire2Frame::setMetadataSize(uint16_t size)
{
    mFrame[4] = (uint8_t)(size >> 8);
    mFrame[5] = (uint8_t)(size >> 0);
}

inline uint16_t Fire2Frame::getComponent() const
{
    return (uint16_t)((mFrame[6] << 8) | (mFrame[7] << 0));
}

inline uint16_t Fire2Frame::getCommand() const
{
    return (uint16_t)((mFrame[8] << 8) | (mFrame[9] << 0));
}

inline uint32_t Fire2Frame::getMsgNum() const
{
    return (uint32_t)((mFrame[10] << 16) | (mFrame[11] << 8) | (mFrame[12] << 0));
}

inline Fire2Frame::MessageType Fire2Frame::getMsgType() const
{
    return static_cast<MessageType>(mFrame[13] >> 5);
}

inline uint32_t Fire2Frame::getUserIndex() const
{
    return (uint32_t)(mFrame[13] & 0x1f); // 0x1f == 00011111
}

inline uint8_t Fire2Frame::getOptions() const
{
    return (uint8_t)(mFrame[14]);
}

inline void Fire2Frame::setOptions(uint8_t options)
{
    mFrame[14] = (uint8_t)(options);
}

inline bool Fire2Frame::isImmediate() const
{
    return ((getOptions() & Fire2Frame::OPTION_IMMEDIATE) != 0);
}

} // Blaze

#endif // BLAZE_FIRE2FRAME_H

