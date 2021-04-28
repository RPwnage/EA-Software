/*H*******************************************************************/
/*!
    \File base64.c

    \Description
        This module implements Base-64 encoding/decoding as defined in RFC 
        989, 1040, 1113, 1421, 1521 and 2045.

    \Notes
        No strict compliance check has been made.  Instead it has only
        been confirmed that Decode(Encode(x)) = x for various inputs
        which was all that was required for the original application.

    \Copyright
        Copyright (c) Electronic Arts 2003. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (SJB) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/util/base64.h"

/*** Defines *********************************************************/

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

/*
   The table used in converting an unsigned char -> Base64.
   This is as per the RFC.
*/
static const char _Base64_strEncode[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/*
   The table used in coverting Base64 -> unsigned char.
   This is not strictly needed since this mapping can be determined
   by searching _Base64_Encode, but by having it as a separate table
   it means the decode can run in O(n) time where n is the length of
   the input rather than O(n*m) where m is 64.
  
   The table is designed such that after subtracting '+' from the
   base 64 value, the corresponding char value can be found by
   indexing into this table.  Since the characters chosen by the
   Base64 designers are not contiguous in ASCII there are some holes
   in the table and these are filled with -1 to indicate an invalid
   value.
*/
static const signed char _Base64_strDecode[] =
{
    62,                                                 // +
    -1, -1, -1,                                         // ,-.
    63,                                                 // /
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,             // 0-9
    -1, -1, -1, -1, -1, -1, -1,                         // :;<=>?@
    0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, // A-
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, //  -Z
    -1, -1, -1, -1, -1, -1,                             // [\]^_`
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // a-
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51  //  -z
};

/*** Public functions ************************************************/

/*F*******************************************************************/
/*!
    \Function Base64Encode

    \Description
        Base-64 encode a string.

    \Input iInputLen - the length of the input
    \Input *pInput   - the input
    \Input *pOutput  - the output

    \Output None

    \Notes
        pOutput is NUL terminated.

        It is assumed that pOutput is large enough to hold
        Base64EncodedSize(iInputLen)+1 bytes.

        The following chart should help explain the various bit shifts
        that are in the code.  On the top are the 8-bit bytes of the
        input and on the bottom are the 6-bit bytes of the output :-
       
     \verbatim
          |       0       |       1       |       2       |  in
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |               |               |               |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |     0     |     1     |       2   |     3     |  out
     \endverbatim
        
        As the above shows the out0 is the top 6 bits of in0, out1 is
        the bottom two bits of in0 along with the top 4 bits of in1,
        ... etc.
       
        To speed things up the encoder tries to work on chunks of 3
        input bytes at a time since that produces 4 output bytes
        without having to maintain any inter-byte processing state.
       
        For any input that is not a multiple of 3 then 1 or 2 padding
        bytes are are added as appropriate as described in the RFC.

    \Version 1.0 12/11/2002 (SJB) First Version
*/
/*******************************************************************F*/
void Base64Encode(int32_t iInputLen, const char *pInput, char *pOutput)
{
    int32_t o = 0;
    int32_t i = 0;
    const char *encode = _Base64_strEncode;

    while (iInputLen >= 3)
    {
        pOutput[o+0] = encode[(pInput[i]>>2)&0x3F];
        pOutput[o+1] = encode[((pInput[i]&0x3)<<4)|((pInput[i+1]>>4)&0x0F)];
        pOutput[o+2] = encode[((pInput[i+1]&0xF)<<2)|((pInput[i+2]>>6)&0x03)];
        pOutput[o+3] = encode[(pInput[i+2]&0x3F)];
        o += 4;
        i += 3;
        iInputLen -= 3;
    }
    switch (iInputLen)
    {
    case 1:
        pOutput[o+0] = encode[(pInput[i]>>2)&0x3F];
        pOutput[o+1] = encode[((pInput[i]&0x3)<<4)];
        pOutput[o+2] = '=';
        pOutput[o+3] = '=';
        o += 4;
        break;
    case 2:
        pOutput[o+0] = encode[(pInput[i]>>2)&0x3F];
        pOutput[o+1] = encode[((pInput[i]&0x3)<<4)|((pInput[i+1]>>4)&0x0F)];
        pOutput[o+2] = encode[((pInput[i+1]&0xF)<<2)];
        pOutput[o+3] = '=';
        o += 4;
        break;
    }
    pOutput[o] = '\0';
}


/*F*******************************************************************/
/*!
    \Function Base64Decode

    \Description
        Decode a Base-64 encoded string.

    \Input iInputLen    - the length of the encoded input
    \Input *pInput      - the Base-64 encoded string
    \Input *pOutput     - [out[ the decoded string, or NULL to calculate output size

    \Output
        int32_t         - non-zero on success, zero on error.

    \Notes
        The only reasons for failure are if the input base64 sequence does
        not amount to a number of characters comprising an even multiple
        of four characters, or if the input contains a character not in
        the Base64 character set and not a whitespace character.  This
        behavior is mentioned in the RFC:
        
            "In base64 data, characters other than those in Table 1,
             line breaks, and other white space probably indicate a
             transmission error, about which a warning message or even
             a message rejection might be appropriate under some
             circumstances."

        It is assumed that pOutput is large enough to hold
        Base64DecodedSize(iInputLen) bytes.  If pOutput is NULL,
        the function will return the number of characters required to
        buffer the output or zero on error.

        The following chart should help explain the various bit shifts
        that are in the code.  On the top are the 6-bit bytes in the
        input and and on the bottom are the 8-bit bytes of the output :-

        \verbatim
          |     0     |     1     |       2   |     3     |   in
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |               |               |               |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |       0       |       1       |       2       |   out
        \endverbatim
        
        As the above shows the out0 consists of all of in0 and the top
        2 bits of in1, out1 is the bottom 4 bits of in1 and the top 4
        bits of in2, ... etc.

    \Version 12/11/2002 (sbevan)   First Version
    \Version 01/29/2009 (jbrookes) Enhanced to skip whitespace, added decoded length functionality
*/
/*******************************************************************F*/
int32_t Base64Decode(int32_t iInputLen, const char *pInput, char *pOutput)
{
    char ci[4];
    char co0, co1, co2, co3;
    int32_t iInputCnt, iInputOff, iOutputOff;
    const signed char *decode = _Base64_strDecode;

    for (iInputOff = 0, iOutputOff = 0; iInputOff < iInputLen; )
    {
        // scan input
        for (iInputCnt = 0; (iInputCnt < 4) && (iInputOff < iInputLen) && (pInput[iInputOff] != '\0'); iInputOff += 1)
        {
            // ignore whitespace
            if ((pInput[iInputOff] == ' ') || (pInput[iInputOff] == '\t') || (pInput[iInputOff] == '\r') || (pInput[iInputOff] == '\n'))
            {
                continue;
            }
            // basic range validation
            else if ((pInput[iInputOff] < '+') || (pInput[iInputOff] > 'z'))
            {
                return(0);
            }
            // fetch input character
            else
            {                
                ci[iInputCnt++] = pInput[iInputOff];
            }
        }
        // if we didn't get anything, we're done
        if (iInputCnt == 0)
        {
            break;
        }
        // error if we did not get full input quad
        if (iInputCnt < 4)
        {
            return(0);
        }
        // decode the sequence
        co0 = decode[(int32_t)ci[0]-'+'];
        co1 = decode[(int32_t)ci[1]-'+'];
        co2 = decode[(int32_t)ci[2]-'+'];
        co3 = decode[(int32_t)ci[3]-'+'];
        
        if ((co0 >= 0) && (co1 >= 0))
        {
            if ((co2 >= 0) && (co3 >= 0))
            {
                if (pOutput != NULL)
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                    pOutput[iOutputOff+1] = (co1&0x3f)<<4|((co2>>2)&0x3F);
                    pOutput[iOutputOff+2] = ((co2&0x3)<<6)|co3;
                }
                iOutputOff += 3;
            }
            else if ((co2 >= 0) && (ci[3] == '='))
            {
                if (pOutput != NULL)
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                    pOutput[iOutputOff+1] = (co1&0x3f)<<4|((co2>>2)&0x3F);
                }
                iOutputOff += 2;
                // consider input complete
                iInputOff = iInputLen;
            }
            else if ((ci[2] == '=') && (ci[3] == '='))
            {
                if (pOutput != NULL)
                {
                    pOutput[iOutputOff+0] = (co0<<2)|((co1>>4)&0x3);
                }
                iOutputOff += 1;
                // consider input complete
                iInputOff = iInputLen;
            }
            else
            {
                // illegal input character
                return(0);
            }
        }
        else
        {
            // illegal input character
            return(0);
        }
    }
    // if no output, return length of validated data or zero if decoding failed
    if (pOutput == NULL)
    {
        return((iInputOff == iInputLen) ? iOutputOff : 0);
    }
    else
    {
        return(iInputOff == iInputLen);
    }
}
