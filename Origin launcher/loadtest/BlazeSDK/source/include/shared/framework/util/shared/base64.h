/*************************************************************************************************/
/*!
    \file

    Base64 encode and decode methods.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BASE64_H
#define BLAZE_BASE64_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "EABase/eabase.h"
#include "framework/util/shared/bytearrayinputstream.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class InputStream;
class OutputStream;

class BLAZESDK_API Base64
{
public:
    // Encode the given input in Base 64 format.
    // Specify lineWidth == 0 to not append any carriage return & line feed pairs.
    static uint32_t encode(InputStream* input, OutputStream* output, uint32_t lineWidth = 0);

    // Convenience method to allow callers to pass data buffer pointers directly
    static uint32_t encode(uint8_t* data, uint32_t length, OutputStream* output, uint32_t lineWidth = 0)
    {
        ByteArrayInputStream input(data, length);
        return encode(&input, output, lineWidth);
    }

    static uint32_t decode(InputStream* input, OutputStream* output);

private:
    static void encodeBase64Block(uint8_t in[3], uint8_t out[4], uint32_t len);
    static void decodeBase64Block(uint8_t in[4], uint8_t out[3]);
};

} // namespace Blaze

#endif // BLAZE_BASE64_H

