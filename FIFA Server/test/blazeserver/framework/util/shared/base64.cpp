/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Base64

    This class provides static methods for encoding and decoding Base64 format.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/shared/base64.h"
#include "framework/util/shared/inputstream.h"
#include "framework/util/shared/outputstream.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


// Base 64 translation table as described in RFC1113
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Decode translation table
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*************************************************************************************************/
/*!
    \brief encodeBase64Block

    Encode 3 8-bit binary bytes as 4 '6-bit' characters
*/
/*************************************************************************************************/
void Base64::encodeBase64Block(uint8_t in[3], uint8_t out[4], uint32_t len)
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4) ];
    out[2] = (uint8_t) (len > 1 ? cb64[ ((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6) ] : '=');
    out[3] = (uint8_t) (len > 2 ? cb64[ in[2] & 0x3F ] : '=');
}

/*************************************************************************************************/
/*!
    \brief encode

    Encode the given binary data into base 64 format and output the encoded data into the
    XmlBuffer.

    \return - The number of characters written to the XmlBuffer
*/
/*************************************************************************************************/
uint32_t Base64::encode(InputStream* input, OutputStream* output, uint32_t lineWidth)
{
    uint8_t in[3], out[4];
    int32_t i;
    int32_t blockLen;
    uint32_t length = input->available();
    uint32_t blocksOut = 0;
    uint32_t numWritten = 0;
    uint32_t paddedLength = ((length % 3) == 0) ? length : length + (3 - (length % 3));

    for (uint32_t j = 0; j < paddedLength; j += 3)
    {
        blockLen = 0;
        for (i = 0; i < 3; i++)
        {
            if ((j+i) < length && input->read(&in[i]))
            {
                blockLen++;
            }
            else
            {
                in[i] = 0;
            }
        }

        if (blockLen)
        {
            encodeBase64Block(in, out, blockLen);
            output->write(&out[0], 4);
            numWritten += 4;
            blocksOut++;
        }

        // Only add carriage returns if lineWidth is specified.
        if (lineWidth > 0)
        {
            // Append a carriage return and line feed at the end of each line, and at the end
            // of encoding the last byte.
            if ((blocksOut >= (lineWidth/4)) || (j == paddedLength))
            {
                if (blocksOut)
                {
                    output->write((uint8_t*)"\r\n", 2);
                    numWritten += 2;
                }
                blocksOut = 0;
            }
        }
    }

    return numWritten;
}

/*************************************************************************************************/
/*!
    \brief decodeBase64Block

    Decode 4 6-bit characters into 3 8-bit binary bytes
*/
/*************************************************************************************************/
void Base64::decodeBase64Block(uint8_t in[4], uint8_t out[3])
{   
    out[ 0 ] = (uint8_t)(in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (uint8_t)(in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (uint8_t)(((in[2] << 6) & 0xC0) | in[3]);
}

/*************************************************************************************************/
/*!
    \brief decode

    Decode a base64 encoded stream discarding padding, line breaks and noise.
*/
/*************************************************************************************************/
uint32_t Base64::decode(InputStream* input, OutputStream* output)
{
    uint8_t in[4], out[3], v;
    uint32_t i;
    uint32_t len;
    uint32_t numWritten = 0;
    //EA_ASSERT(input->available() %4 == 0);
    if (input->available() %4 != 0) return 0;

    while (input->available() > 0)
    {
        for (len = 0, i = 0; i < 4 ; i++ )
        {
            v = 0;
            while ((input->available() > 0) && (v == 0))
            {
                input->read(&v);
                v = (uint8_t)((v < 43 || v > 122) ? 0 : cd64[v - 43]);
                if (v)
                {
                    v = (uint8_t)((v == '$') ? 0 : v - 61);
                }
            }

            if (v)
            {
                len++;
                in[i] = (uint8_t)(v - 1);
            }
            else
            {
                in[i] = 0;
            }
        }

        if (len)
        {
            decodeBase64Block(in, out);
            output->write(&out[0], len - 1);
            numWritten += len - 1;
        }
    }

    return numWritten;
}

} // Blaze

