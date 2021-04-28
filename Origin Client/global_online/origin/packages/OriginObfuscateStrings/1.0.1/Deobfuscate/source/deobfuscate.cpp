///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: deobfuscate.cpp
//	simple way to obfuscate strings. Define strings using OBFUSCATE_STRING macro in deobfuscate.h
//
//	Author: Sergey Zavadski

#include "EABase/eabase.h"
#include <stdlib.h>
#include "ObfuscateStrings/deobfuscate.h"
#include "ObfuscateStrings/shiftbits.h"

namespace Origin {
namespace ObfuscateStrings {


std::string deobfuscate(const std::string& source)
{
    size_t nLength = source.length();
    if (nLength < 24)       // can only deobfuscate strings surrounded by "**OB_START**" and "***OB_END***".  12 characters each for start and end
        return source;
    
    // If this is development time and the string isn't obfuscated, just strip the tags
    if (source[0] == '*' && source[1] == '*' && source[2] == 'O' && source[3] == 'B' && source[4] == '_' && source[5] == 'S' && source[6] == 'T' && source[7] == 'A' && source[8] == 'R' && source[9] == 'T' && source[10] == '*' && source[11] == '*')
    {
        // unobfuscated already but with tags, just strip the tags
        return source.substr(12, nLength-24);
    }
    
    uint8_t nXOR = source[0];       // first byte is the starting xor value

    // Strip the start/end tags
    std::string deobfuscated = source.substr(12, nLength-24);

    for (size_t i = 0; i < deobfuscated.length(); i++)
    {
        if (deobfuscated[i] != 0)       // Nulls get left alone
            deobfuscated[i] ^= nXOR;

        nXOR = ShiftBits(nXOR);
    }

    return deobfuscated;
}

std::wstring deobfuscate(const std::wstring& source)
{
    size_t nLength = source.length();
    if (nLength < 24)       // can only deobfuscate strings surrounded by "**OB_START**" and "***OB_END***".  12 characters each for start and end
        return source;

    // If this is development time and the string isn't obfuscated, just strip the tags
    if (source[0] == L'*' && source[1] == L'*' && source[2] == L'O' && source[3] == L'B' && source[4] == L'_' && source[5] == L'S' && source[6] == L'T' && source[7] == L'A' && source[8] == L'R' && source[9] == L'T' && source[10] == L'*' && source[11] == L'*')
    {
        // unobfuscated already but with tags, just strip the tags
        return source.substr(12, nLength - 24);
    }

#ifdef WIN32
    uint16_t nXOR = source[0];       // first byte is the starting xor value
#else
    uint32_t nXOR = source[0];
#endif
    
    // Strip the start/end tags
    std::wstring deobfuscated = source.substr(12, nLength - 24);

    for (size_t i = 0; i < deobfuscated.length(); i++)
    {
        if (deobfuscated[i] != 0)       // Nulls get left alone
            deobfuscated[i] ^= nXOR;

        nXOR = ShiftBits(nXOR);
    }

    return deobfuscated;
}



}
}

