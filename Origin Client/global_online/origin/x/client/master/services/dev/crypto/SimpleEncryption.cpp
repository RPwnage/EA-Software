///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SimpleEncryption.cpp
//	Simple encryption class that uses hardcoded key. other party should use the same key
//
//	Author: Sergey Zavadski

#include <stdlib.h>
#include "SimpleEncryption.h"
#include "rijndael.h"

namespace Origin {
namespace Services {
namespace Crypto {

SimpleEncryption::SimpleEncryption(int seed)
: m_rijndael(new Rijndael)
{
    //
    //	hardcode key
    //
    if(seed == 0)
    {
        for ( unsigned int i = 0; i < sizeof( m_key ); i++ )
        {
            m_key[i] = (unsigned char)i;
        }
    }
    else
    {
        srand(7);
        srand(rand() + seed);
        for ( unsigned int i = 0; i < sizeof( m_key ); i++ )
        {
            m_key[i] = (unsigned char)(rand()&0xFF);
        }
    }
}

SimpleEncryption::~SimpleEncryption()
{
    delete m_rijndael;

}

std::string SimpleEncryption::decrypt( const std::string& source )
{
	unsigned int length = (unsigned int )source.length() / 2;
	if (length == 0) return std::string();

	unsigned char * output = new unsigned char [length + 1];
	memset( output, 0, sizeof(char) * length + 1);
	
	//	
	//	restore binary representation
	//

	unsigned char* input = new unsigned char[length];
	
	for ( unsigned int i = 0; i < length; i++ )
	{
		unsigned int temp;
		sscanf( source.substr( i * 2, 2 ).c_str(), "%x", &temp );
		input[i] = (unsigned char) temp;
	}

	m_rijndael->init( Rijndael::ECB, Rijndael::Decrypt, m_key, Rijndael::Key16Bytes );
	m_rijndael->padDecrypt( input, length, output );

	delete[] input;

	std::string outString((char *)output);

	delete[] output;
	
	return outString;
}

std::string SimpleEncryption::encrypt( const std::string& source )
{
	// Output array will be same as source length, rounded up to next 16 byte multiple.
	const int BLOCK_SIZE = 16;
	unsigned int numBlocks = (source.length()/BLOCK_SIZE) + 1;
	unsigned int outSize = numBlocks * BLOCK_SIZE;

	unsigned char * output = new unsigned char[outSize];
	memset( output, 0, sizeof(char) * outSize );

	m_rijndael->init( Rijndael::ECB, Rijndael::Encrypt, m_key, Rijndael::Key16Bytes );
	int length = m_rijndael->padEncrypt( ( unsigned char* ) source.c_str(), (unsigned int) source.length(), output );

	// Result is string representation of encrypted value. Add 1 for null terminator.
	unsigned int resultSize = length*2 + 1;
	char * result = new char [resultSize];
	memset( result, 0, sizeof(char) * resultSize );
	int offset = 0;

	//
	//	convert to string
	//
	for ( int i = 0; i < length; i++ )
	{
		offset += sprintf( result + offset, "%02x", output[i] );
	}

	delete[] output;

	std::string strResult(result);
	delete[] result;

	return strResult; // TODO return as reference
}

uint32_t SimpleEncryption::ShiftBits(uint32_t nValue)
{
    // 10000000 = 0x80
    // 01111111 = 0x7F

    // Preserving the high order bit 0x80
    // Shift all of the other bits over
    uint8_t nLowBit = nValue&0x01;
    uint32_t nNewValue = 0x80 | (nValue&0x7F)>>1 | (nLowBit<<6);

    return nNewValue;
}


std::string SimpleEncryption::deobfuscate(const std::string& source)
{
    uint32_t nLength = source.length();
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

    for (int32_t i = 0; i < deobfuscated.length(); i++)
    {
        if (deobfuscated[i] != 0)       // Nulls get left alone
            deobfuscated[i] ^= nXOR;

        nXOR = ShiftBits(nXOR);
    }

    return deobfuscated;
}


}
}
}

