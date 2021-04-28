// GENERATED CODE (Source: notification_protocol.proto) - DO NOT EDIT DIRECTLY.
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

enum class SessionNotificationTypeV1
{
    DEFAULT = 1,
    /**
     *  Indicate forced logout when user logged in somewhere else for single session login
     */
    FORCE_DISCONNECT = 2,
    /**
     *  Indicate user logged in somewhere else for multiple session login
     */
    USER_LOGGED_IN = 3,
    /**
     *  Indicate user logged out somewhere else for multiple session login
     */
    USER_LOGGED_OUT = 4,
};

}
}
}
}
}
}
