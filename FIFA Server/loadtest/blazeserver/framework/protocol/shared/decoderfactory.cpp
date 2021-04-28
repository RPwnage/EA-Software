/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/shared/restdecoder.h"
#include "framework/protocol/shared/jsondecoder.h"
#include "framework/protocol/shared/xml2decoder.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

Decoder* DecoderFactory::create(Decoder::Type type)
{
    switch (type)
    {
        case Decoder::HTTP:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "HttpDecoder") HttpDecoder();
        case Decoder::HEAT2:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Heat2Decoder") Heat2Decoder();
        case Decoder::XML2:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "Xml2Decoder") Xml2Decoder();
        case Decoder::JSON:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "JsonDecoder") JsonDecoder();
        case Decoder::REST:
            return BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "RestDecoder") RestDecoder();
        case Decoder::INVALID:
        default:
            return nullptr;
    }
}

Decoder::Type DecoderFactory::getDecoderType(const char8_t* name)
{
    if (blaze_stricmp(name, HttpDecoder::getClassName()) == 0)
        return Decoder::HTTP;
    else if (blaze_stricmp(name, Heat2Decoder::getClassName()) == 0)
        return Decoder::HEAT2;
    else if (blaze_stricmp(name, Xml2Decoder::getClassName()) == 0)
        return Decoder::XML2;
    else if (blaze_stricmp(name, JsonDecoder::getClassName()) == 0)
        return Decoder::JSON;
    else if (blaze_stricmp(name, RestDecoder::getClassName()) == 0)
        return Decoder::REST;
    return Decoder::INVALID;
}

} // Blaze
