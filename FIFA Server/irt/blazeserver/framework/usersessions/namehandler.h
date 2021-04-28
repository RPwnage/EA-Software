/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_NAMEHANDLER_H
#define BLAZE_NAMEHANDLER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class NameHandler
{
public:
    virtual ~NameHandler() { }

    /*! Generate a hash code for the given name. */
    virtual size_t generateHashCode(const char8_t* name) const = 0;

    /*! Compare two names for equality (return codes equivalent to strcmp). */
    virtual int32_t compareNames(const char8_t* name1, const char8_t* name2) const = 0;

    /*! Generate the canonical form of the given name */
    virtual bool generateCanonical(const char8_t* name, char8_t* canonical, size_t len) const = 0;

    static NameHandler* createNameHandler(const char8_t* type);
    static const char8_t* getDefaultNameHandlerType();
    static const char8_t* getValidTypes(char8_t* buffer, size_t len);
};

} // Blaze

#endif // BLAZE_NAMEHANDLER_H

