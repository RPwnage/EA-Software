/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DECODER_H
#define BLAZE_DECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API Decoder
{
    NON_COPYABLE(Decoder);
public:
    enum Type
    {        
        HTTP = 0,
        REST,
        HEAT2,
        XML2,
        JSON,
        INVALID,
        MAX = INVALID
    };

    Decoder() {};
    virtual ~Decoder() {};

    virtual const char8_t* getName() const = 0;
    virtual Type getType() const = 0;
};

} // Blaze

#endif // BLAZE_DECODER_H
