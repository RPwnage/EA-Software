/*************************************************************************************************/
/*!
    \file httpparam.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPPARAM_H
#define BLAZE_HTTPPARAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct BLAZESDK_API HttpParam
{
    enum ParamType {CHAR_DATA, BINARY};
    const char8_t* name;
    const char8_t* value;
    ParamType type;
    union
    {
        size_t valueBinSize;        // valid for ParamType == BINARY
        bool encodeValue;           // valid for ParamType == CHAR_DATA
    };

    HttpParam() {
        name = nullptr;
        value = nullptr;
        encodeValue = false;
        type = CHAR_DATA;
    }
};

} // Blaze

#endif // BLAZE_HTTPPARAM_H
