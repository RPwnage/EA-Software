//////////////////////////////////////////////////////////////////////////////////////////////////
// ShiftBits
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// 
#include "ObfuscateStrings/shiftbits.h"
namespace Origin
{
    namespace ObfuscateStrings
    {

        uint8_t ShiftBits(uint8_t nValue)
        {
            // 10000000 = 0x80
            // 01111111 = 0x7F

            // Preserving the high order bit 0x80
            // Shift all of the other bits over

            //We preserve the high order bit so we don't accidentally change an obfuscated
            //character to a null
            uint8_t nLowBit = nValue & 0x01;
            uint8_t nNewValue = 0x80 | (nValue & 0x7F) >> 1 | (nLowBit << 6);

            return nNewValue;
        }
        
        uint16_t ShiftBits(uint16_t nValue)
        {
            // 10000000 00000000 = 0x8000
            // 01111111 11111111 = 0x7FFF

            // Preserving the high order bit 0x8000
            // Shift all of the other bits over
            uint16_t nLowBit = nValue & 0x01;
            uint16_t nNewValue = 0x8000 | (nValue & 0x7FFF) >> 1 | (nLowBit << 14);

            return nNewValue;
        }
        
        uint32_t ShiftBits(uint32_t nValue)
        {
            // 10000000 00000000 00000000 00000000 = 0x80000000
            // 01111111 11111111 11111111 11111111 = 0x7FFFFFFF
            
            // Preserving the high order bit 0x80000000
            // Shift all of the other bits over
            uint32_t nLowBit = nValue & 0x01;
            uint32_t nNewValue = 0x80000000 | (nValue & 0x7FFFFFFF) >> 1 | (nLowBit << 14);
            
            return nNewValue;
        }
    }

}
