/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/httpencoder.h"
#include "framework/protocol/shared/defaultdifferenceencoder.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/protocol/shared/xml2encoder.h"
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#include "framework/protocol/eventxmlencoder.h"
#endif


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

Encoder* EncoderFactory::create(Encoder::Type type, Encoder::SubType subType)
{
    switch (subType)
    {
        case Encoder::DEFAULTDIFFERNCE:
            return createDefaultDifferenceEncoder(type);
        case Encoder::NORMAL:
        default:
        {
            switch (type)
            {
#ifndef BLAZE_CLIENT_SDK
                case Encoder::EVENTXML:
                    return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "EventXmlEncoder") EventXmlEncoder();
#endif
                case Encoder::XML2:
                    return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Xml2Encoder") Xml2Encoder();
                case Encoder::HEAT2:
                    return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Heat2Encoder") Heat2Encoder();
                case Encoder::HTTP:
                    return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "HttpEncoder") HttpEncoder();
                case Encoder::JSON:
                    return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "JsonEncoder") JsonEncoder();
                case Encoder::INVALID:
                default:
                    return nullptr;
            }
        }
    }
}

Encoder::Type EncoderFactory::getEncoderType(const char8_t* name)
{
    if (blaze_stricmp(name, Heat2Encoder::getClassName()) == 0)
        return Encoder::HEAT2;
    else if (blaze_stricmp(name, HttpEncoder::getClassName()) == 0)
        return Encoder::HTTP;
    else if (blaze_stricmp(name, JsonEncoder::getClassName()) == 0)
        return Encoder::JSON;
    else if (blaze_stricmp(name, Xml2Encoder::getClassName()) == 0)
        return Encoder::XML2;
#ifndef BLAZE_CLIENT_SDK
    else if (blaze_stricmp(name, EventXmlEncoder::getClassName()) == 0)
        return Encoder::EVENTXML;
#endif
    return Encoder::INVALID;
}

Encoder* EncoderFactory::createDefaultDifferenceEncoder(Encoder::Type type)
{
    switch (type)
    {
#ifndef BLAZE_CLIENT_SDK
        case Encoder::EVENTXML:
            {
                EventXmlEncoder* encoder = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "EventXmlEncoder") EventXmlEncoder();
                encoder->setUseDefaultDiff(true);
                return encoder;
            }
#endif
        case Encoder::XML2:
            {
#ifdef ENABLE_LEGACY_CODECS
                return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "DefaultXml2Encoder") DefaultDifferenceEncoder<Xml2Encoder>();
#else
                Xml2Encoder* encoder = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Xml2Encoder") Xml2Encoder();
                encoder->setUseDefaultDiff(true);
                return encoder;
#endif
            }
        case Encoder::HEAT2:
            {
                Heat2Encoder* encoder = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Heat2Encoder") Heat2Encoder();
                encoder->setUseDefaultDiff(true);
                return encoder;
            }
        case Encoder::HTTP:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "DefaultHttpEncoder") DefaultDifferenceEncoder<HttpEncoder>();
        case Encoder::JSON:
            {
#ifdef ENABLE_LEGACY_CODECS
                return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "DefaultJsonEncoder") DefaultDifferenceEncoder<JsonEncoder>();
#else
                JsonEncoder* encoder = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "JsonEncoder") JsonEncoder();
                encoder->setUseDefaultDiff(true);
                return encoder;
#endif
            }
        case Encoder::INVALID:
        default:
            return nullptr;
    }
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

