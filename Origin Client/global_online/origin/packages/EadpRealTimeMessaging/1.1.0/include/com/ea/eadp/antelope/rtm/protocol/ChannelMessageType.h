// GENERATED CODE (Source: enum_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace rtm
{

namespace protocol
{

/**
 *  Channel message types
 *  This enum is a duplicate of SocialProtocol.ChannelMessageType.
 *  Cannot use SocialProtocol.ChannelMessageType because of circular dependency.
 *  Need to keep them in sync.
 */
enum class ChannelMessageType
{
    TEXT_MESSAGE = 1,
    BINARY_MESSAGE = 2,
    GROUP_MEMBERSHIP_CHANGE = 3,
    CHAT_CONNECTED = 4,
    CHAT_DISCONNECTED = 5,
    CHAT_LEFT = 6,
    CUSTOM_MESSAGE = 7,
    CHAT_USER_MUTED = 8,
    CHAT_USER_UNMUTED = 9,
    CHANNEL_MEMBERSHIP_CHANGE = 10,
    STICKY_MESSAGE_CHANGED = 11,
    CHAT_TYPING_EVENT = 12,
};

}
}
}
}
}
}
