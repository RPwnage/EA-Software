/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OBFUSCATOR_H
#define BLAZE_OBFUSCATOR_H

/*** Include files *******************************************************************************/


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*! ***************************************************************************/
/*! \class Non-instantiatable class which provides static methods for string obfuscation.
*******************************************************************************/
class DbObfuscator
{
private:
    DbObfuscator() { }

public:
    static size_t obfuscate(const char8_t *obfuscator, const char8_t *source, char8_t *buffer, size_t size);
    static size_t decode(const char8_t *obfuscator, const char8_t *source, char8_t *buffer, size_t size);
};

} // Blaze

#endif // BLAZE_OBFUSCATOR_H

