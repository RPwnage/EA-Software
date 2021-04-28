/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ENCODERFACTORY_H
#define BLAZE_ENCODERFACTORY_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/encoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API EncoderFactory
{
    NON_COPYABLE(EncoderFactory);
    EncoderFactory() {};
    ~EncoderFactory() {};

public:
    static Encoder* create(Encoder::Type type, Encoder::SubType subType = Encoder::NORMAL);
    static Encoder* createDefaultDifferenceEncoder(Encoder::Type type);
    static Encoder::Type getEncoderType(const char8_t* name);
};

} // Blaze

#endif // BLAZE_ENCODERFACTORY_H
