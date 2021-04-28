/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ENCODER_H
#define BLAZE_ENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h" 
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API Encoder
{
    NON_COPYABLE(Encoder);

public:
    enum Type
    {
        XML2 = 0,
        EVENTXML,
        HEAT2,
        HTTP,
        JSON,
        INVALID,
        MAX = INVALID
    };

    enum SubType
    {
        NORMAL = 0,
        DEFAULTDIFFERNCE,
        MAX_SUBTYPE
    };

    Encoder() {};
    virtual ~Encoder() {};
    
    virtual const char8_t* getName() const = 0;
    virtual Type getType() const = 0;
    virtual SubType getSubType() const { return NORMAL; }
};

} // Blaze

#endif // BLAZE_ENCODER_H
