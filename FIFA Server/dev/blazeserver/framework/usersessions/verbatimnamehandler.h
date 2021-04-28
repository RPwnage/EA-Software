/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_VERBATIMCNAMEHANDLER_H
#define BLAZE_VERBATIMCNAMEHANDLER_H

/*** Include files *******************************************************************************/

#include "framework/usersessions/namehandler.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class VerbatimNameHandler

    This implementation of the NameHandler interface does no conversion on names.  Comparisons
    are done byte-for-byte.  The canonical name is exactly the same as the display name.
*/
/*************************************************************************************************/
class VerbatimNameHandler : public NameHandler
{
public:
    size_t generateHashCode(const char8_t* name) const override
    {
        eastl::hash<const char8_t*> h;
        return h(name);
    }

    int32_t compareNames(const char8_t* name1, const char8_t* name2) const override
    {
        eastl::str_equal_to<const char8_t*> cmp;
        return cmp(name1, name2);
    }

    bool generateCanonical(const char8_t* name, char8_t* canonical, size_t len) const override
    {
        if (name == nullptr)
            return false;
        while ((*name != '\0') && (len > 0))
        {
            *canonical++ = *name++;
            len--;
        }
        *canonical = '\0';
        return (len > 0);
    }

    static const char8_t* getType() { return "verbatim"; }
};

} // Blaze

#endif // BLAZE_VERBATIMCNAMEHANDLER_H

