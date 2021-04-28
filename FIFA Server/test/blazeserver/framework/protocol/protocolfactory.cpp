/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/util/shared/blazestring.h"
#include "framework/protocol/fireprotocol.h"
#include "framework/protocol/fire2protocol.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/eventxmlprotocol.h"
#include "framework/protocol/xmlhttpprotocol.h"
#include "framework/protocol/restprotocol.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


Protocol* ProtocolFactory::create(Protocol::Type type, uint32_t maxFrameSize)
{
    Protocol* result = nullptr;
    switch (type)
    {
        case Protocol::FIRE:
            result = BLAZE_NEW FireProtocol();
            break;
        case Protocol::FIRE2:
            result = BLAZE_NEW Fire2Protocol();
            break;
        case Protocol::HTTPXML:
            result = BLAZE_NEW HttpXmlProtocol();
            break;
        case Protocol::EVENTXML:
            result = BLAZE_NEW EventXmlProtocol();
            break;
        case Protocol::XMLHTTP:
            result = BLAZE_NEW XmlHttpProtocol();
            break;
        case Protocol::REST:
            result = BLAZE_NEW RestProtocol();
            break;
        case Protocol::INVALID:
        default:
            result = nullptr;
            break;
    }

    if (result != nullptr)
    {
        result->setMaxFrameSize(maxFrameSize);
    }

    return result;
}

Protocol::Type ProtocolFactory::getProtocolType(const char8_t* name)
{
    if (blaze_stricmp(name, FireProtocol::getClassName()) == 0)
        return Protocol::FIRE;
    else if (blaze_stricmp(name, Fire2Protocol::getClassName()) == 0)
        return Protocol::FIRE2;
    else if (blaze_stricmp(name, HttpXmlProtocol::getClassName()) == 0)
        return Protocol::HTTPXML;
    else if (blaze_stricmp(name, EventXmlProtocol::getClassName()) == 0)
        return Protocol::EVENTXML;
    else if (blaze_stricmp(name, XmlHttpProtocol::getClassName()) == 0)
        return Protocol::XMLHTTP;
    else if (blaze_stricmp(name, RestProtocol::getClassName()) == 0)
        return Protocol::REST;
    return Protocol::INVALID;
}

} // Blaze
