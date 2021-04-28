/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/fireframe.h"

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
const char* FireFrame::MessageTypeToString(FireFrame::MessageType type)
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
    }
    return  "UNKNOWN";
}

// static
bool FireFrame::parseMessageType(const char8_t* typeStr, FireFrame::MessageType& type)
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
    if (blaze_stricmp(typeStr, "ERROR_REPLY") == 0)
    {
        type = ERROR_REPLY;
        return true;
    }
    if (blaze_stricmp(typeStr, "NOTIFICATION") == 0)
    {
        type = NOTIFICATION;
        return true;
    }

    return false;
}

// static
const char8_t* FireFrame::optionsToString(uint32_t options, char8_t* buf, size_t len)
{
    if ((buf == nullptr) || (len == 0))
        return buf;

    buf[0] = '\0';
    if (options & FireFrame::OPTION_JUMBO_FRAME)
        blaze_strnzcat(buf, "(JUMBO)", len);
    if (options & FireFrame::OPTION_HAS_CONTEXT)
        blaze_strnzcat(buf, "(HAS_CONTEXT)", len);
    if (options & FireFrame::OPTION_IMMEDIATE)
        blaze_strnzcat(buf, "(IMMEDIATE)", len);
    if (options & FireFrame::OPTION_JUMBO_CONTEXT)
        blaze_strnzcat(buf, "(JUMBO_CONTEXT)", len);
    return buf;
}

} // Blaze

