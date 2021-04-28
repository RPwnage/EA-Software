/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DECODERFACTORY_H
#define BLAZE_DECODERFACTORY_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/decoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API DecoderFactory
{
    NON_COPYABLE(DecoderFactory);
    DecoderFactory() {};
    ~DecoderFactory() {};

public:

    static Decoder* create(Decoder::Type type);
    static Decoder::Type getDecoderType(const char8_t* name);
};

} // Blaze

#endif // BLAZE_DECODERFACTORY_H
