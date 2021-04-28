/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIREFRAME_H
#define BLAZE_FIREFRAME_H
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

class BLAZESDK_API FireFrame
{
    NON_COPYABLE(FireFrame);

public:
    static const uint32_t HEADER_SIZE = 12;

    // Number of bytes in the frame used to store the jumbo frame size
    static const uint32_t JUMBO_SIZE = sizeof(uint16_t);
    // Number of bytes in frame used to store the new 64 bit jumbo context
    static const uint32_t JUMBO_CONTEXT_SIZE = sizeof(uint64_t);
    // Number of bytes in frame used to store the old 32 bit context
    static const uint32_t SMALL_CONTEXT_SIZE = sizeof(uint32_t);
    static const uint32_t EXTRA_SIZE = JUMBO_SIZE + JUMBO_CONTEXT_SIZE; // used only as a hint

    static const uint16_t OPTION_NONE                   = 0;
    static const uint16_t OPTION_JUMBO_FRAME            = 1;
    static const uint16_t OPTION_HAS_CONTEXT            = 2;
    static const uint16_t OPTION_IMMEDIATE              = 4;
    static const uint16_t OPTION_JUMBO_CONTEXT          = 8;
    static const uint16_t OPTION_MASK = (OPTION_JUMBO_FRAME | OPTION_HAS_CONTEXT | OPTION_IMMEDIATE | OPTION_JUMBO_CONTEXT);

    typedef enum { MESSAGE = 0, REPLY = 1, NOTIFICATION = 2, ERROR_REPLY = 3 } MessageType;

    static const uint32_t MSGNUM_MASK = 0x000fffff;

    FireFrame(uint8_t* buf) : mFrame(buf) { }
    FireFrame() : mFrame(nullptr) { }

    inline FireFrame(uint8_t* buf, uint32_t size, uint16_t component, uint16_t command,
            uint16_t errorCode, MessageType msgType, uint32_t userIndex, uint32_t options,
            uint32_t msgNum, uint64_t context = 0);

    inline void setBuffer(uint8_t* buf) { mFrame = buf; }

    inline uint16_t getHeaderSize() const;
    inline uint32_t getSize() const;
    inline void setSize(uint32_t size);
    inline uint16_t getComponent() const;
    inline uint16_t getCommand() const;
    inline uint16_t getErrorCode() const;
    inline MessageType getMsgType() const;
    inline uint32_t getUserIndex() const;
    inline uint32_t getOptions() const;
    inline void setOptions(uint32_t options);
    inline uint32_t getMsgNum() const;
    inline bool hasContext() const;
    inline uint64_t getContext() const;
    inline bool isJumboFrame() const;
    inline size_t getExtraSize() const;
    inline bool isImmediate() const;

    static const char* MessageTypeToString(MessageType type);
    static bool parseMessageType(const char8_t* typeStr, MessageType& type);
    static const char8_t* optionsToString(uint32_t options, char8_t* buf, size_t len);

private:
    uint8_t* mFrame;
};

inline FireFrame::FireFrame(uint8_t* buf, uint32_t size, uint16_t component, uint16_t command,
        uint16_t errorCode, MessageType msgType, uint32_t userIndex, uint32_t options,
        uint32_t msgNum, uint64_t context)
    : mFrame(buf)
{
    mFrame[0] = (uint8_t)(size >> 8);
    mFrame[1] = (uint8_t)(size);
    mFrame[2] = (uint8_t)(component >> 8);
    mFrame[3] = (uint8_t)(component);
    mFrame[4] = (uint8_t)(command >> 8);
    mFrame[5] = (uint8_t)(command);
    mFrame[6] = (uint8_t)(errorCode >> 8);
    mFrame[7] = (uint8_t)(errorCode);
    mFrame[8] = (uint8_t)((msgType << 4) | userIndex);
    mFrame[9] = (uint8_t)((options << 4) | ((msgNum >> 16) & 0xf));
    mFrame[10] = (uint8_t)(msgNum >> 8);
    mFrame[11] = (uint8_t)(msgNum);
    uint8_t* extra = &mFrame[12];
    if (options & OPTION_JUMBO_FRAME)
    {
        extra[0] = (uint8_t)(size >> 24);
        extra[1] = (uint8_t)(size >> 16);
        extra += 2;
    }
    if (options & OPTION_HAS_CONTEXT)
    {
        extra[0] = (uint8_t)(context >> 24);
        extra[1] = (uint8_t)(context >> 16);
        extra[2] = (uint8_t)(context >> 8);
        extra[3] = (uint8_t)(context >> 0);
        if (options & OPTION_JUMBO_CONTEXT)
        {
            extra[4] = (uint8_t)(context >> 56);
            extra[5] = (uint8_t)(context >> 48);
            extra[6] = (uint8_t)(context >> 40);
            extra[7] = (uint8_t)(context >> 32);
        }
    }
}

inline uint16_t FireFrame::getHeaderSize() const
{
    size_t size = HEADER_SIZE + getExtraSize();
    return (uint16_t) size;
}

inline uint32_t FireFrame::getSize() const
{
    uint32_t size = (uint32_t)((mFrame[0] << 8) | mFrame[1]);
    if (getOptions() & OPTION_JUMBO_FRAME)
    {
        size |= (mFrame[12] << 24) | (mFrame[13] << 16);
    }
    return size;
}

inline void FireFrame::setSize(uint32_t size)
{
    mFrame[0] = (uint8_t)(size >> 8);
    mFrame[1] = (uint8_t)(size);
    if (size > UINT16_MAX || isJumboFrame())
    {
        setOptions(getOptions() | OPTION_JUMBO_FRAME);
        mFrame[12] = (uint8_t)(size >> 24);
        mFrame[13] = (uint8_t)(size >> 16);
    }
}

inline uint16_t FireFrame::getComponent() const
{
    return (uint16_t)((mFrame[2] << 8) | mFrame[3]);
}

inline uint16_t FireFrame::getCommand() const
{
    return (uint16_t)((mFrame[4] << 8) | mFrame[5]);
}

inline uint16_t FireFrame::getErrorCode() const
{
    return (uint16_t)((mFrame[6] << 8) | mFrame[7]);
}

inline FireFrame::MessageType FireFrame::getMsgType() const
{
    return static_cast<MessageType>((mFrame[8] >> 4) & 0xf);
}

inline uint32_t FireFrame::getUserIndex() const
{
    return (uint32_t)(mFrame[8] & 0xf);
}

inline uint32_t FireFrame::getOptions() const
{
    return (uint32_t)((mFrame[9] >> 4) & 0xf);
}

inline void FireFrame::setOptions(uint32_t options)
{
    mFrame[9] = (uint8_t)((mFrame[9] & 0xf) | (options << 4));
}

inline uint32_t FireFrame::getMsgNum() const
{
    return (uint32_t)(((mFrame[9] & 0xf) << 16) | (mFrame[10] << 8) | mFrame[11]);
}

inline bool FireFrame::hasContext() const 
{
    return ((getOptions() & FireFrame::OPTION_HAS_CONTEXT) != 0);
}

inline uint64_t FireFrame::getContext() const 
{
    uint64_t context = 0;
    uint32_t options = getOptions();
    if (options & FireFrame::OPTION_HAS_CONTEXT)
    {
        uint8_t* ctx = &mFrame[12];
        if (options & FireFrame::OPTION_JUMBO_FRAME)
            ctx += JUMBO_SIZE;
        context =
            ((uint64_t)ctx[0] << 24) |
            ((uint64_t)ctx[1] << 16) |
            ((uint64_t)ctx[2] << 8)  |
            ((uint64_t)ctx[3] << 0);
        if (options & FireFrame::OPTION_JUMBO_CONTEXT)
        {
            context |=
                ((uint64_t)ctx[4] << 56) |
                ((uint64_t)ctx[5] << 48) |
                ((uint64_t)ctx[6] << 40) |
                ((uint64_t)ctx[7] << 32);
        }
    }
    return context;
}

inline bool FireFrame::isJumboFrame() const
{
    return ((getOptions() & FireFrame::OPTION_JUMBO_FRAME) != 0);
}

inline size_t FireFrame::getExtraSize() const
{
    uint32_t options = getOptions();
    size_t extraSize = (options & FireFrame::OPTION_JUMBO_FRAME) ? JUMBO_SIZE : 0;
    if (options & FireFrame::OPTION_HAS_CONTEXT)
    {
        if (options & FireFrame::OPTION_JUMBO_CONTEXT)
            extraSize += JUMBO_CONTEXT_SIZE;
        else
            extraSize += SMALL_CONTEXT_SIZE;
    }
    return extraSize;
}

inline bool FireFrame::isImmediate() const
{
    return ((getOptions() & FireFrame::OPTION_IMMEDIATE) != 0);
}

} // Blaze

#endif // BLAZE_FIREFRAME_H

