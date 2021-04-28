/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/fire2frame.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

// static
const char* Fire2Frame::MessageTypeToString(Fire2Frame::MessageType type)
{
    switch (type)
    {
        case MESSAGE:
            return "MESSAGE";
        case REPLY:
            return "REPLY";
        case NOTIFICATION:
            return "NOTIFICATION";
        case ERROR_REPLY:
            return "ERROR_REPLY";
        case PING:
            return "PING";
        case PING_REPLY:
            return "PING_REPLY";
    }
    return  "UNKNOWN";
}

// static
bool Fire2Frame::parseMessageType(const char8_t* typeStr, Fire2Frame::MessageType& type)
{
    if (blaze_stricmp(typeStr, "MESSAGE") == 0)
    {   
        type = MESSAGE;
        return true;
    }
    if (blaze_stricmp(typeStr, "REPLY") == 0)
    {   
        type = REPLY;
        return true;
    }
    if (blaze_stricmp(typeStr, "NOTIFICATION") == 0)
    {
        type = NOTIFICATION;
        return true;
    }
    if (blaze_stricmp(typeStr, "ERROR_REPLY") == 0)
    {
        type = ERROR_REPLY;
        return true;
    }
    if (blaze_stricmp(typeStr, "PING") == 0)
    {
        type = PING;
        return true;
    }
    if (blaze_stricmp(typeStr, "PING_REPLY") == 0)
    {
        type = PING_REPLY;
        return true;
    }

    return false;
}

// static
const char8_t* Fire2Frame::optionsToString(uint32_t options, char8_t* buf, size_t len)
{
    if ((buf == nullptr) || (len == 0))
        return buf;

    buf[0] = '\0';
    if (options & Fire2Frame::OPTION_IMMEDIATE)
        blaze_strnzcat(buf, "(IMMEDIATE)", len);
    return buf;
}

} // Blaze

