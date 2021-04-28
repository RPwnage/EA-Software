/**************************************************************************************************/
/*! 
    \file nucleusstringtocode.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef BLAZE_NUCLEUS_NUCLEUSSTRINGTOCODE_H
#define BLAZE_NUCLEUS_NUCLEUSSTRINGTOCODE_H

// Include files
#include "framework/tdf/nucleuscodes.h"

namespace Blaze
{

namespace Nucleus
{

namespace AccountStatus
{
    Code stringToCode(const char8_t* str);
}

namespace EmailStatus
{
    Code stringToCode(const char8_t* str);
}

namespace PersonaStatus
{
    Code stringToCode(const char8_t* str);
}

namespace EntitlementStatus
{
    Code stringToCode(const char8_t* str);
}

namespace EntitlementType
{
    Code stringToCode(const char8_t* str);
}

namespace ProductCatalog
{
    Code stringToCode(const char8_t* str);
}

namespace StatusReason
{
    Code stringToCode(const char8_t* str);
}

namespace NucleusCode
{
    Code stringToCode(const char8_t* str);
}

namespace NucleusField
{
    Code stringToCode(const char8_t* str);
}

namespace NucleusCause
{
    Code stringToCode(const char8_t* str);
}


} // Nucleus
} // Blaze

#endif // BLAZE_NUCLEUS_NUCLEUSSTRINGTOCODE_H
