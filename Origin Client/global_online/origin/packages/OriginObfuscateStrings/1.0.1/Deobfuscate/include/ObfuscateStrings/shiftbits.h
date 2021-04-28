//////////////////////////////////////////////////////////////////////////////////////////////////
// ShiftBits
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// 
#ifndef _SHIFT_BITS_H_
#define _SHIFT_BITS_H_

#include "EABase/eabase.h"
namespace Origin
{
    namespace ObfuscateStrings
    {
        uint8_t                 ShiftBits(uint8_t nValue);
        uint16_t                ShiftBits(uint16_t nValue);
        uint32_t                ShiftBits(uint32_t nValue);
    }
}

#endif // _SHIFT_BITS_H_