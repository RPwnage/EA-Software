/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbObfuscator
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbobfuscator.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*! ***************************************************************************/
/*! \brief Obfuscates source string using obfuscator string and custom algorithm.
           The algorithm will produce two characters for each character from source:
           C1 = lower4bits((obf + src) ^ obf) + 'A'
           C2 = upper4bits((obf + src) ^ obf) + 'a'
           Where src is source caracter and obf is its position corresponding obfuscator 
           character (if obfuscator string is smaller then source then it is recycled)

    \param obfuscator String used as obfuscator string
    \param source     Source string to obfuscate
    \param buffer     Destination buffer where resulting obfuscated string is stored
    \param size       Size of buffer
    \return           Number of characters used in buffer, including trailing null terminator
*******************************************************************************/
size_t DbObfuscator::obfuscate(const char8_t *obfuscator, const char8_t *source, char8_t *buffer, size_t size)
{
    uint8_t src = 0, obf = 0, dst = 0;
    size_t len = 0;
    const char8_t *ptObf = obfuscator;

    while ((src = (uint8_t)*source++) != '\0' && len < size - 1)
    {
        obf = (uint8_t)*ptObf++;
        if (obf == '\0')
        {
            ptObf = obfuscator;
            obf = *ptObf++;
        }

        uint8_t hlp = (obf + src) ^ obf;
        dst = 'A' + (hlp & 0x0f);
        buffer[len++] = dst;
        if (len == size)
            break;
        dst = 'a' + ((hlp & 0xf0) >> 4);
        buffer[len++] = dst;
    }

    buffer[len] = '\0';

    return len;
}

/*! ***************************************************************************/
/*! \brief Decodes string obfuscated using DbObfuscator::obfuscate() method.
           Please look at DbObfuscator::obfuscate() description for algorithm details

    \param obfuscator String used as obfuscator string
    \param source     Source string to decode
    \param buffer     Destination buffer where resulting decoded string is stored
    \param size       Size of buffer
    \return           Number of characters used in buffer, including trailing null terminator
*******************************************************************************/
size_t DbObfuscator::decode(const char8_t *obfuscator, const char8_t *source, char8_t *buffer, size_t size)
{
    uint8_t src = 0, obf = 0, dst = 0;
    size_t len = 0;
    const char8_t *ptObf = obfuscator;

    while ((src = (uint8_t)*source++) != '\0' && len < size - 1)
    {
        obf = (uint8_t)*ptObf++;
        if (obf == '\0')
        {
            ptObf = obfuscator;
            obf = *ptObf++;
        }

        uint8_t hlp = src - 'A';

        src = *source++;
        if (src == '\0')
            break;
        
        hlp |= ((src - 'a') << 4);

        dst = (hlp ^ obf) - obf;
        buffer[len++] = dst;
    }
    
    buffer[len] = '\0';

    return len;
}


}// Blaze
